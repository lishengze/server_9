// link_timer_op - 链接定时事件实现
//
// 注：timerfd 创建/读/设置时间等系统调用需 #include <sys/timerfd.h>，已在头中包含。
// 决策：心跳 / 重连 / 心跳超时 三种检查均由本对象在 timerfd 触发时统一执行。

#include "link_timer_op.h"
#include "api_errno.h"
#include "mthread.h"

// 显式实例化所需的链接/引擎/柜台类型 (见文件末尾实例化列表)
#include "aio_socket_link.h"
#ifdef HAS_TCPDIRECT
#include "tcpdir_link.h"
#endif
#include "counter98.h"
#include "gw_counter_direct.h"
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "multi_socket_engine.h"
#include "single_socket_engine.h"
#ifdef HAS_TCPDIRECT
#include "tcpdirect_engine.h"
#endif

#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace lb_api {

// 构造函数：创建 timerfd (CLOCK_MONOTONIC, 非阻塞)
template <class TLink, class TEngine>
link_timer_op<TLink, TEngine>::link_timer_op() : timer_fd_(-1), interval_(1), link_(nullptr), engine_(nullptr) {}

template <class TLink, class TEngine> link_timer_op<TLink, TEngine>::~link_timer_op() { close(); }

// 绑定 link + engine + link_type + interval
template <class TLink, class TEngine>
int32 link_timer_op<TLink, TEngine>::init_timer(TLink *link, TEngine *engine, int32 interval) {
  link_ = link;
  engine_ = engine;
  interval_ = interval;
  set_event(0, 0, 1);

  if ((timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC)) < 0) {
    return LBAPI_ERR_TIMER_INIT;
  }
  int rc = 0;

  if ((rc = fcntl(timer_fd_, F_GETFL, 0)) < 0) {
    return LBAPI_ERR_ATTR_INIT;
  }
  rc |= O_NONBLOCK;
  if ((rc = fcntl(timer_fd_, F_SETFL, rc)) < 0) {
    return LBAPI_ERR_ATTR_INIT;
  }

  return 0;
}

// 将 timerfd 以自动定时循环方式加入线程的 epoll，使用水平触发
template <class TLink, class TEngine> int32 link_timer_op<TLink, TEngine>::add_timer_poll(lb_common::mthread *th) {
  struct itimerspec newtm;
  newtm.it_value.tv_nsec = 0;
  newtm.it_value.tv_sec = interval_ * 2;
  newtm.it_interval.tv_nsec = 0;
  newtm.it_interval.tv_sec = interval_;
  if (timerfd_settime(timer_fd_, 0, &newtm, NULL) < 0) {
    return LBAPI_ERR_TIMER_INIT;
  }
  // 将 timerfd 加入 mthread 的 epoll
  return th->add_poll_event(*this);
}

template <class TLink, class TEngine> void link_timer_op<TLink, TEngine>::close() {
  if (timer_fd_ >= 0) {
    ::close(timer_fd_);
    timer_fd_ = -1;
  }
}

template <class TLink, class TEngine>
int32 link_timer_op<TLink, TEngine>::delive_link_event(int16 event_type, int32 event_data) {

  if (event_type < LINK_EVENT_TYPE_SEND_HEART || event_type > LINK_EVENT_TYPE_LINK_CONNECT) {
    return LBAPI_ERR_UNSUPPORTED_TYPE;
  }
  lb_common::que_mth_buf *tpq = engine_->get_queue();
  link_engine_outop *op = engine_->get_out_op();
  if (NULL == tpq) {
    return LBAPI_ERR_UNSUPPORTED_OP;
  }

  int32 total_len = sizeof(link_send_event) + 24;
  char *data = nullptr;
  int64 pos = tpq->write_get_mth(data, total_len);
  if (pos <= 0) {
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }
  link_send_event *evt = reinterpret_cast<link_send_event *>(data);
  evt->link_type = link_->get_link_type();
  evt->type = event_type;
  evt->data_len = 24;

  if (event_type == LINK_EVENT_TYPE_SEND_HEART) {

  } else if (event_type == LINK_EVENT_TYPE_LINK_CLOSE) {
    link_close_event_info *tclose = reinterpret_cast<link_close_event_info *>(data + sizeof(link_send_event));
    tclose->err_code = event_data;
  } else if (event_type == LINK_EVENT_TYPE_LINK_CONNECT) {
    link_connect_event_info *tcon = reinterpret_cast<link_connect_event_info *>(data + sizeof(link_send_event));
    tcon->need_switch = event_data;
  }

  tpq->write_cmt_mth(pos, total_len);
  op->trigger_send();
  return 0;
}

// 定时器触发回调: 读 timerfd -> 决策 -> 调 link 检查函数, 投递事件到 engine
template <class TLink, class TEngine> void link_timer_op<TLink, TEngine>::deal_event() {

  // 1. 读 timerfd (8 字节) — 清除可读事件
  uint64_t expirations = 0;
  ::read(timer_fd_, &expirations, sizeof(expirations));
  // 2. 决策顺序 (3 种失败语义严格区分):
  //    a) 心跳超时 (直接 close 链接, 不重连) — 上层识别后 close
  //    b) 物理断线 (is_free) → check_reconnect → 投 LINK_CONNECT
  //    c) 心跳发送时机 (is_work) → 投 LINK_HEART

  if (link_->is_work()) {
    if (link_->check_heart_timeout()) {
      //心跳超时
      delive_link_event(LINK_EVENT_TYPE_LINK_CLOSE, LBAPI_ERR_LINK_HEART_TIMEOUT);
    } else if (link_->check_heart_send()) {
      // 心跳发送时机
      delive_link_event(LINK_EVENT_TYPE_SEND_HEART, 0);
    }
  } else if (link_->is_free()) {
    // 物理断线场景：检查是否需要重连
    int32 need_switch = 0;
    if (link_->check_reconnect(need_switch)) {
      delive_link_event(LINK_EVENT_TYPE_LINK_CONNECT, need_switch);
    }
  }
}

// 显式实例化: 模板成员函数体定义在本文件, 显式实例化必须置于定义所在的编译单元
// —— multi 引擎 (98 链接 + fpga GW 链接)
template class link_timer_op<aio_socket_link<counter98>, multi_socket_engine<gw_counter_direct>>;
template class link_timer_op<aio_socket_link<counter98>, multi_socket_engine<fpga_counter_direct>>;
template class link_timer_op<aio_socket_link<counter98>, multi_socket_engine<fpga_counter_gateway>>;
template class link_timer_op<aio_socket_link<fpga_counter_direct>, multi_socket_engine<fpga_counter_direct>>;
template class link_timer_op<aio_socket_link<fpga_counter_gateway>, multi_socket_engine<fpga_counter_gateway>>;
// multi_socket_engine<gw_counter_direct> 的成员引用本组合, 必须实例化 (即使 gw_direct 不用 GW 槽)
template class link_timer_op<aio_socket_link<gw_counter_direct>, multi_socket_engine<gw_counter_direct>>;
// —— single 引擎 (个微 / fpga 直连)
template class link_timer_op<aio_socket_link<gw_counter_direct>, single_socket_engine<gw_counter_direct>>;
template class link_timer_op<aio_socket_link<fpga_counter_direct>, single_socket_engine<fpga_counter_direct>>;
#ifdef HAS_TCPDIRECT
// —— tcpdirect 引擎 (solarflare, 仅 WITH_TCPDIRECT 变体)
template class link_timer_op<tcpdir_link<gw_counter_direct>, tcpdirect_engine<gw_counter_direct>>;
template class link_timer_op<tcpdir_link<fpga_counter_direct>, tcpdirect_engine<fpga_counter_direct>>;
#endif

} // namespace lb_api

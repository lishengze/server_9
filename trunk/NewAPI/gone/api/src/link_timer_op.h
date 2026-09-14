// 链接定时事件类
//
// 模板参数: TLink (链接类型) + TEngine (该 link 所属工作引擎)
// 由工作引擎持有, 知道 link + engine 两个指针.
// deal_event 中调 link 的检查函数, 根据结果向 engine 投递事件.
//
// 不再由 link 内部持有, link 也不再有 on_timer() 方法.
//
// 注意: 所有成员函数均已内联在此头文件中, 以确保显式模板实例化时
//       编译器能看到完整定义, 避免链接时出现未定义符号。

#pragma once

#include "comm_sys.h"
#include "mthread.h"
#include "que_mth_buf.h"
#include "wait_poll.h"

#include "api_event_msg.h"
#include "api_errno.h"

#include <cstdint>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// 链接定时事件 (双模板)
/// @tparam TLink   链接类型
/// @tparam TEngine 该 link 所属工作引擎类型
template <class TLink, class TEngine> class link_timer_op : public lb_common::epoll_event_op {
public:
  /// 构造函数: 创建 timerfd
  link_timer_op() : timer_fd_(-1), interval_(1), link_(nullptr), engine_(nullptr) {}

  /// 析构函数: 关闭 timerfd
  ~link_timer_op() { close(); }

  /// 获取 timerfd 文件描述符
  int32 get_fd() override { return timer_fd_; }
  /// 获取定时器间隔 (秒)
  int32 get_interval() const { return interval_; }

  /// 绑定 link + engine + link_type (由持有本对象的 engine 在 init 阶段调用)
  /// @param link      链接指针
  /// @param engine    link 所属工作引擎指针
  /// @param interval  定时循环的事件间隔
  int32 init_timer(TLink *link, TEngine *engine, int32 interval) {
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

  /// 将timerfd 以自动定时循环方式加入线程的epoll,使用水平触发
  int32 add_timer_poll(lb_common::mthread *th) {
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

  /// 停止定时器
  void close() {
    if (timer_fd_ >= 0) {
      ::close(timer_fd_);
      timer_fd_ = -1;
    }
  }

  int32 delive_link_event(int16 event_type, int32 event_data) {
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

  /// 定时器触发回调: 读 timerfd -> 决策 -> 调 link 检查函数, 投递事件到 engine
  void deal_event() override {
    // 1. 读 timerfd (8 字节) — 清除可读事件
    uint64_t expirations = 0;
    ::read(timer_fd_, &expirations, sizeof(expirations));
    // 2. 决策顺序 (D28: 3 种失败语义严格区分):
    //    a) 心跳超时 (D28: 直接 close 链接, 不重连) — 上层识别后 close
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

  /// 错误回调 (timerfd 自身出错几乎不会发生, 保留)
  void deal_error() override {}
  /// 关闭回调
  void deal_close() override {}

private:
  int32 timer_fd_;  ///< timerfd 文件描述符,重复循环定时
  int32 interval_;  ///< 定时循环间隔(秒, 由 init_timer 注入)
  TLink *link_;     ///< 链接指针
  TEngine *engine_; ///< link 所属工作引擎指针
};

} // namespace lb_api

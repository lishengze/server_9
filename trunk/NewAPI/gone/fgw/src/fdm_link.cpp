// fdm_link - fdm 物理链接实现
//
// Step 8 场景切片（参照 api_link 模式重构）：
//   S1: send_msg / close_link  — 纯 IO 转发（发送线程串行消费 link_send_que）
//   S3: connect / on_connected — 同步 connect_ch + start_ch（参考 gone/api aio_socket_link）

#include "fdm_link.h"

#include "fdm_board.h"
#include <cstring>

#include "fgw_errno.h"

namespace lb_fgw {

/// 主动连接（同步，首次和重连统一入口）
int32_t fdm_link::connect(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr, lb_common::mthread *recvth,
                          lb_common::mthread *sendth, link_send_que<fdm_link, fdm_engine> *sendque,
                          lb_common::ch_recv_cb<lb_common::tcp_buf_ch> *cb_msg,
                          lb_common::aio_ch_op<lb_common::tcp_buf_ch> *cb_ch, fdm_board *fdm) {

  send_que_ = sendque;
  board_no_ = fdm->get_board_no();
  heart_interval_ = buf_attr.heartinterval;
  send_thread_ = sendth;

  // deal_ch_* 回调通过 pch->get_user_data() 反查 fdm_board
  buf_attr.puserdata = reinterpret_cast<void *>(fdm);
  // 保存远端地址（供 get_remote 日志 / 重连用）
  std::memset(&remote_, 0, sizeof(remote_));
  if (!fdm->get_addr(remote_)) {
    return FGW_ERR_PARAM;
  }
  // 异步连接：deal_ch_connect 回调中调 on_connected 启动接收
  int32_t ret = aio_.connect_ch(buf_attr, ch_attr, cb_msg, cb_ch, recvth, &remote_, nullptr, 1);
  if (ret < 0)
    return ret;
  return 0;
}

/// 连接已建立：set_connected + add_ref（发送线程引用）+ start_ch（recv 线程引用）
int32_t fdm_link::on_connected() {
  rc_.set_connected();
  if (aio_.add_ref()) {            // 为发送线程增加计数
    send_thread_->mod_load(1);     //增加发送线程负载
    int32_t ret = aio_.start_ch(); // 链接添加到接收线程的 epoll，并增加计数
    if (ret == 0) {
      return 0;
    }
    aio_.close_ch(ret);
    //aio_.sub_ref();
    return ret;
  }
  return FGW_ERR_LINK_STATE;
}

/// 检查是否应重连（供 fdm_engine 周期检查，使用内部 heart_interval_）
bool fdm_link::check_reconnect() {
  if (aio_.is_free()) {
    if (rc_.check_reconnect(heart_interval_)) {
      return true;
    }
  }
  return false;
}

/// 心跳超时检测（主线程/periodic_check 调用）
bool fdm_link::check_heart_timeout() {
  if (aio_.is_work()) {
    if (aio_.check_heart_timeout()) {
      return true;
    }
  }
  return false;
}
bool fdm_link::check_heart_send() {
  if (aio_.is_work()) {
    if (aio_.check_heart_send()) {
      return true;
    }
  }
  return false;
}

/// 关闭链接（deal_ch_error 或心跳超时时调用）
void fdm_link::close_link(int32_t errcode) { aio_.close_ch(errcode); }

/// 投递关闭事件到发送队列（deal_ch_closing 中调用，发送线程消费 close_link 时 sub_ref）
int32_t fdm_link::push_close_event() {
  if (nullptr != send_thread_ && nullptr != send_que_) {
    return send_que_->push_close_link(this);
  }
  return 0;
}

/// 关闭事件处理（deal_ch_closed 中调用，sub_ref 释放引用，与 on_connected 中 add_ref 配对）
void fdm_link::deal_close_event() {
  rc_.set_disconnected();
  if (nullptr != send_thread_) {
    send_thread_->mod_load(-1); //减去发送线程负载
    send_thread_ = nullptr;
  }
  aio_.sub_ref();
}

} // namespace lb_fgw

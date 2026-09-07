// api_link - api 协议实体实现
//
// Step 8 场景切片：
//   S1: send_msg / close_link  — 纯 IO 转发（发送线程串行消费 link_send_que）
//   S2: accept / on_connected   — accept_ch + start_ch（同步建立，参考 gone/api aio_socket_link）
//        on_closed             — 关闭通知（业务清理留待登录场景）

#include "api_link.h"
#include "fgw_errno.h"
#include "g1msghead.h"
#include "matomic.h"

#include <cstdint>
#include <cstring>

namespace lb_fgw {

// ---- S2: 链接建立 / 关闭 ----

/// accept 时初始化并接受连接
int32_t api_link::accept(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr, lb_common::mthread *recvth,
                         lb_common::mthread *sendth, link_send_que<api_link, api_engine> *sendque,
                         lb_common::ch_recv_cb<lb_common::tcp_buf_ch> *cb_msg,
                         lb_common::aio_ch_op<lb_common::tcp_buf_ch> *cb_ch, int32_t connect_id,
                         lb_common::sock_fd fd) {
  send_thread_ = sendth;
  send_que_ = sendque;
  api_connect_id_ = connect_id;
  link_type_ = LINK_TYPE_UNKNOWN; // accept 时未知，由后续业务消息确定
  board_no_ = 0;
  user_id_ = 0;
  // deal_ch_* 回调通过 pch->get_user_data() 反查本 link
  buf_attr.puserdata = this;
  int32_t ret = aio_.accept_ch(buf_attr, ch_attr, cb_msg, cb_ch, recvth, fd);
  return ret;
}

int32_t api_link::on_connected() {
  if (aio_.add_ref()) {            // 为发送线程增加计数
    int32_t ret = aio_.start_ch(); // 链接添加到接收线程的 epoll ， 并增加计数
    if (ret == 0) {
      send_thread_->mod_load(1); //增加发送线程负载
      return 0;
    }
    aio_.sub_ref();
    return ret;
  }
  return FGW_ERR_LINK_STATE;
}

bool api_link::check_heart_timeout() {
  if (aio_.is_work()) {
    if (aio_.check_heart_timeout()) {
      return true;
    }
  }
  return false;
}

void api_link::close_link(int32_t errcode) { aio_.close_ch(errcode); }

/// 投递关闭事件到发送队列（发送线程消费 close_link 时 sub_ref，与分配时的 add_ref 配对）
int32_t api_link::push_close_event() {
  if (aio_.is_work())
    return send_que_->push_close_link(this);
  return FGW_ERR_LINK_STATE;
}

void api_link::deal_close_event() {
  send_thread_->mod_load(-1); //减去发送线程负载
  aio_.sub_ref();             //减去发送线程引用
}

void api_link::set_user_board(int32_t log_type, uint16_t board, uint16_t user) {
  if (log_type == 1) {
    board_no_ = board;
    user_id_ = user;
    atomic_seq_fence();
    link_type_ = LINK_TYPE_DIRECT;
  } else {
    link_type_ = LINK_TYPE_GATEWAY;
  }
}
void api_link::push_onboard_state(g1_msg_head *head) {
  if (link_type_ == LINK_TYPE_DIRECT) {
    atomic_seq_fence();
    if (head->board_no != board_no_ || head->user_id != user_id_)
      return;
  }

  send_que_->push_send_msg(this, reinterpret_cast<const char *>(head), head->msg_len + sizeof(g1_msg_head));
}

} // namespace lb_fgw

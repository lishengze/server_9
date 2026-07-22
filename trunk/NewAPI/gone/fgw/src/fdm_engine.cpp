// fdm_engine - fdm 端连接管理和操作引擎实现

#include "fdm_engine.h"

#include <cstdint>
#include <cstring>

#include "data_def.h"
#include "fdm_board.h"
#include "fgw_errno.h"
#include "g1comdefine.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "mlog.h"

namespace lb_fgw {

static constexpr int fdm_link_recv_once_num = 4; // 链接一次epoll 唤醒接收次数
static constexpr int fdm_link_send_once_num = 6; // 链接一次epoll 唤醒发送次数

// ---- 构造 aio_attr / channel_attr ----
void fdm_engine::build_aio_attr_(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr) {
  std::memset(&buf_attr, 0, sizeof(buf_attr));
  std::memset(&ch_attr, 0, sizeof(ch_attr));
  buf_attr.onerecvtimes = fdm_link_recv_once_num;
  buf_attr.oncerecvlen = sizeof(trade_rtn) * 2;
  buf_attr.maxmsglen = G1_MSG_MAX_LEN;
  buf_attr.buf_size = 2 * G1_MSG_MAX_LEN;
  buf_attr.dispatch_zero_copy = 0;
  buf_attr.heartinterval = check_interval_;
  ch_attr.family = CHANNEL_FAMILY_IPV4;
  ch_attr.recvsockbuflen = 1 * 1024 * 1024;
  ch_attr.sendsockbuflen = 1 * 1024 * 1024;
  ch_attr.tcpdelayack = 1; // 禁用 Nagle（低延迟）
  ch_attr.sendrecvtime = 5;
  ch_attr.localloop = 0;
}

// ---- init：初始化线程池 + 发送队列 ----
int32_t fdm_engine::init(data_repo *datas, int32_t heart_interval, int32_t thread_num, int32_t que_size,
                         api_engine *apis, lb_common::lb_log *log, std::vector<int32_t> &cpus) {
  datas_ = datas;
  thread_num_ = thread_num;
  check_interval_ = heart_interval;
  api_eng_ = apis;
  log_ = log;

  thread_num = thread_num < 2 ? 2 : thread_num;
  que_size = que_size < 16 ? 16 : que_size;
  int64_t tqs = que_size;
  tqs = tqs * 1024 * 1024;

  // 内部构造发送队列数组
  send_ques_ = new link_send_que<fdm_link, fdm_engine>[thread_num];

  int32_t ret = th_pool_.init(thread_num, 256, 1024, 2, 100);
  if (ret < 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fdm engine thread pool init failed, thread_num=" << thread_num << ", ret=" << ret << end_log;
    return ret;
  }

  if ((int32_t)(cpus.size()) >= thread_num) {
    for (int32_t i = 0; i < thread_num; ++i) {
      th_pool_.init_set_cpu(i, cpus[i]);
    }
  }

  for (int32_t i = 0; i < thread_num; ++i) {
    ret = send_ques_[i].init(this, tqs, fdm_link_send_once_num); // fdm 发送优先，once_send=4
    if (ret < 0) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fdm engine send_que init failed, idx=" << i << ", que_size=" << que_size << ", ret=" << ret
                     << end_log;
      return ret;
    }
    lb_common::mthread *th = th_pool_.get_thread(i);
    ret = th->add_poll_event(send_ques_[i]);
    if (ret < 0) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fdm engine add send_que to epoll failed, idx=" << i << ", ret=" << ret << end_log;
      return ret;
    }
  }

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fdm engine init ok, thread_num=" << thread_num << ", que_size=" << que_size << end_log;
  return 0;
}

int32_t fdm_engine::start() {
  lb_common::lb_log_hand tlh(log_);
  int32_t ret = th_pool_.run();
  if (ret < 0) {
    error_log(tlh) << "fdm engine thread pool run failed, ret=" << ret << end_log;
    return ret;
  }
  info_log(tlh) << "fdm engine start ok" << end_log;
  return 0;
}

void fdm_engine::stop() {
  th_pool_.to_stop();
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fdm engine stop: thread pool stopped" << end_log;
}

// ---- start_connect：对单个 fdm_link 发起连接（参照 api_engine accept 模式）----
int32_t fdm_engine::start_connect(fdm_link *link, fdm_board *fdm) {
  lb_common::lb_log_hand tlh(log_);

  // 分配接收线程（负载最小）
  lb_common::mthread *recvth = nullptr;
  int32_t recvth_id = th_pool_.assign_thread(recvth, -1);

  // 分配发送线程（与接收线程分离——排除 recvth）
  lb_common::mthread *sendth = nullptr;
  int32_t sendth_id = th_pool_.assign_thread(sendth, recvth_id);

  lb_common::aio_attr buf_attr;
  lb_common::channel_attr ch_attr;
  build_aio_attr_(buf_attr, ch_attr);

  // 异步建连：立即返回，TCP 握手完成后 deal_ch_connect 调 on_connected
  int32_t ret = link->connect(buf_attr, ch_attr, recvth, sendth, &send_ques_[sendth_id], this, this, fdm);
  if (ret < 0) {
    error_log(tlh) << "fdm link connect error, board_no=" << fdm->get_board_no() << ", recvth=" << recvth_id
                   << ", sendth=" << sendth_id << ", ret=" << ret << end_log;
    return ret;
  }

  info_log(tlh) << "fdm link connecting, board_no=" << fdm->get_board_no() << ", fdm_ip=" << link->get_remote().ip
                << ", fdm_port=" << link->get_remote().port << ", recvth=" << recvth_id << ", sendth=" << sendth_id
                << ", ret=" << ret << end_log;
  return 0;
}

// ---- check_board_link：心跳超时、心跳发送、断线重连 ----
void fdm_engine::check_board_link() {
  std::vector<fdm_board *> boards;
  datas_->get_fdm_boards(boards);

  for (auto *board : boards) {
    if (!board->is_loaded())
      continue;
    fdm_link *link = board->get_link();

    if (link->check_reconnect()) {
      start_connect(link, board);
      continue;
    }

    // 心跳超时检测
    if (link->check_heart_timeout()) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fdm link heart timeout to close, board_no=" << board->get_board_no()
                     << ", fdm_ip=" << link->get_remote().ip << ", fdm_port=" << link->get_remote().port << end_log;
      link->close_link(FGW_ERR_HEART_TIMEOUT);
      continue;
    }

    // 心跳发送
    if (link->check_heart_send()) {
      g1_msg_head tm;
      tm.msg_id = G1_MSG_HEART_REQ;
      tm.msg_len = 0;
      tm.board_no = link->get_board_no();
      tm.user_id = 0;
      tm.session_id = 0;
      link->push_send((const char *)(&tm), sizeof(tm));
    }
  }
}

// ---- deal_msg：解析 g1 头 → 按 msg_id 分发（S4 投递骨架）----
lb_common::int32 fdm_engine::deal_msg(lb_common::tcp_buf_ch *pch, lb_common::aio_msg &msg) {
  int32_t deal_len = 0;
  int32_t full_len = 0;
  fdm_board *fdm = static_cast<fdm_board *>(pch->get_user_data());
  fdm_link *link = fdm->get_link();

  while (deal_len < msg.msglen) {
    // 头不完整：返回 0，等更多数据
    if (unlikely(msg.msglen - deal_len < (lb_common::int32)sizeof(g1_msg_head)))
      break;
    g1_msg_head *head = reinterpret_cast<g1_msg_head *>(msg.pmsg + deal_len);
    full_len = (int32_t)(sizeof(g1_msg_head) + head->msg_len);
    if (unlikely(full_len > G1_MSG_MAX_LEN)) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fdm msg too long, board_no=" << fdm->get_board_no() << ", msg_id=" << head->msg_id
                     << ", msg_len=" << head->msg_len << ", full_len=" << full_len << end_log;
      return FGW_ERR_MSG_LEN;
    }
    if (unlikely(msg.msglen - deal_len < full_len)) // 体不完整
      break;

    switch (head->msg_id) {
    case G1_MSG_HEART_ANS: {
      link->on_heart_msg();
      break;
    }
    case G1_MSG_ORDER_RTN:
    case G1_MSG_TRADE_RTN:
    case G1_MSG_CANCEL_RSP:
      link->on_msg_heart();
      route_downstream(fdm, head);
      break;
    case G1_MSG_LOGIN_ANS:
      link->on_msg_heart();
      deal_login_ans(fdm, head);
      break;
    case G1_MSG_OFFLINE_PUSH:
      link->on_msg_heart();
      // todo S9：板卡客户状态推送 → 通知 api 客户下线
      deal_user_offline(fdm, head);
      break;
    default: {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "fdm unknown msg_id, board_no=" << fdm->get_board_no() << ", fdm_ip=" << link->get_remote().ip
                    << ", fdm_port=" << link->get_remote().port << ", msg_id=" << head->msg_id
                    << ", msg_len=" << head->msg_len << end_log;
      break;
    }
    }

    deal_len += full_len;
  }
  return deal_len;
}

/// 业务下行（ORDER_RTN/TRADE_RTN/CANCEL_RSP）
void fdm_engine::route_downstream(fdm_board *pfdm, g1_msg_head *head) {
  // (board_no, session_id) → fdm_session → api_link（fdm recv 线程同线程，免锁）
  fdm_session *fs = pfdm->find_fdm_session(head->session_id);
  if (fs == nullptr) {
    lb_common::lb_log_hand tlh(log_);
    warning_log(tlh) << "session not found to discard msg, board_no=" << head->board_no
                     << ", session_id=" << head->session_id << ", msg_id=" << head->msg_id
                     << ", msg_len=" << head->msg_len << end_log;
    return; // 找不到会话，丢弃
  }
  api_link *alink = fs->ln_api_;
  if (alink == nullptr) {
    lb_common::lb_log_hand tlh(log_);
    warning_log(tlh) << "link not found to discard msg, board_no=" << head->board_no
                     << ", session_id=" << head->session_id << ", msg_id=" << head->msg_id
                     << ", msg_len=" << head->msg_len << end_log;
    return; // 会话的 api 链接不存在，丢弃
  }

  int32_t ret = alink->push_send(reinterpret_cast<const char *>(head), (int32_t)(sizeof(g1_msg_head) + head->msg_len));
  if (ret < 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "put msg to api queue to send, board_no=" << head->board_no << ", session_id=" << head->session_id
                   << ", msg_id=" << head->msg_id << ", msg_len=" << head->msg_len << ", ret=" << ret << end_log;
  }
}

/// 登录应答下行中转（login_ans）
void fdm_engine::deal_login_ans(fdm_board *board, g1_msg_head *head) {
  if (head->msg_len < (int32_t)sizeof(login_ans)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "login ans msg len error, board_no=" << head->board_no << ", session_id=" << head->session_id
                   << ", user_id=" << head->user_id << ", msg_len=" << head->msg_len << end_log;
    return;
  }

  login_ans *ans = reinterpret_cast<login_ans *>(head + 1);
  if (ans->board_no != head->board_no)
    head->board_no = ans->board_no;
  if (ans->user_id != head->user_id)
    head->user_id = ans->user_id;
  if (ans->session_id != head->session_id)
    head->session_id = ans->session_id;

  int32_t full_len = (int32_t)(sizeof(g1_msg_head) + head->msg_len);
  int32_t ret = 0;
  api_link *alink = nullptr;
  ret = api_eng_->find_api_link(alink, ans->req_connect_id);
  if (ret < 0 || alink == nullptr) {
    lb_common::lb_log_hand tlh(log_);
    warning_log(tlh) << "api_link not found, board_no=" << head->board_no << ", session_id=" << head->session_id
                     << ", user_id=" << head->user_id << ", req_connect_id=" << ans->req_connect_id
                     << ", cust_id=" << ans->cust_id << ", branch_id=" << ans->branch_id
                     << ", fund_account=" << ans->fund_account_id << ", agw_user=" << ans->session << end_log;
    return;
  }

  if (ans->err_code == 0) {
    agw_user_key akey(ans->session);
    agw_user *agw = datas_->add_agwuser_link(akey, alink);
    if (agw == nullptr) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "build api link and agw user relation error, board_no=" << head->board_no
                     << ", session_id=" << head->session_id << ", user_id=" << head->user_id
                     << ", req_connect_id=" << ans->req_connect_id << ", cust_id=" << ans->cust_id
                     << ", branch_id=" << ans->branch_id << ", fund_account=" << ans->fund_account_id
                     << ", agw_user=" << ans->session << ", ret=" << ret << end_log;

      alink->sub_ref();
      return;
    }

    ret = board->add_fdm_session(head->session_id, alink);
    if (ret < 0) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "build api link and fdm session id relation error, board_no=" << head->board_no
                     << ", session_id=" << head->session_id << ", user_id=" << head->user_id
                     << ", req_connect_id=" << ans->req_connect_id << ", cust_id=" << ans->cust_id
                     << ", branch_id=" << ans->branch_id << ", fund_account=" << ans->fund_account_id
                     << ", agw_user=" << ans->session << ", ret=" << ret << end_log;
      alink->sub_ref();
      return;
    }

    fdm_session *fs = board->find_fdm_session(head->session_id);
    if (fs != nullptr) {
      agw->add_fdm_session(head->board_no, head->session_id);
    }
  }

  // 透传 login_ans 给 api
  ret = alink->push_send(reinterpret_cast<const char *>(head), full_len);
  if (ret < 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "push login ans to api link to send error, board_no=" << head->board_no
                   << ", session_id=" << head->session_id << ", user_id=" << head->user_id
                   << ", req_connect_id=" << ans->req_connect_id << ", cust_id=" << ans->cust_id
                   << ", branch_id=" << ans->branch_id << ", fund_account=" << ans->fund_account_id
                   << ", session=" << ans->session << ", ret=" << ret << end_log;
  }
  alink->sub_ref();
}

void fdm_engine::deal_user_offline(fdm_board *pfdm, g1_msg_head *head) {
  // todo , 投递用户下线通知事件
  fpga_user_state *info = reinterpret_cast<fpga_user_state *>(head + 1);
  if (head->user_id != info->user_id)
    head->user_id = info->user_id;
  head->board_no = pfdm->get_board_no();
  customer *pcust = pfdm->find_customer(head->user_id);
  if (nullptr == pcust) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "not find customer, board_no=" << head->board_no << ", session_id=" << head->session_id
                   << ", user_id=" << head->user_id << ", user_state=" << info->state
                   << ", branch_id=" << info->branch_id << ", fund_account=" << info->fund_account_id << end_log;
  }
  if (info->state == 1)
    pcust->onboard_state = 1;
  else
    pcust->onboard_state = 2;

  pfdm->dispatch_onboard_state(head);
}

void fdm_engine::deal_send_error(fdm_link *link, char *pmsg, int32_t msg_len, int32_t err_ret) {
  lb_common::lb_log_hand tlh(log_);
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(pmsg);
  error_log(tlh) << "fdm link send msg error, boardno=" << link->get_board_no() << ", msg_id=" << head->msg_id
                 << ", msg_len=" << head->msg_len << ", ret=" << err_ret << end_log;

  if (head->msg_id == G1_MSG_LOGIN_REQ) {
    login_req *req = reinterpret_cast<login_req *>(head + 1);
    api_link *alink = nullptr;
    int32_t ret = api_eng_->find_api_link(alink, req->req_connect_id);
    if (ret < 0 || alink == nullptr) {
      lb_common::lb_log_hand tlh(log_);
      warning_log(tlh) << "api_link not found, board_no=" << head->board_no << ", session_id=" << head->session_id
                       << ", user_id=" << head->user_id << ", req_connect_id=" << req->req_connect_id
                       << ", cust_id=" << req->cust_id << ", branch_id=" << req->branch_id
                       << ", fund_account=" << req->fund_account_id << ", agw_user=" << req->session << end_log;
      return;
    }
    api_eng_->send_login_error(alink, head, FGW_ERR_LINK_SEND, "fdm link send msg error");
    alink->sub_ref();
  } else {
    fdm_board *pboard = nullptr;
    int32_t ret = datas_->find_fdm(pboard, head->board_no);
    if (ret < 0) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "not find board, board_no=" << head->board_no << ", session_id=" << head->session_id
                     << ", msg_id=" << head->msg_id << ", msg_len=" << head->msg_len << end_log;
      return;
    }
    fdm_session *fs = pboard->find_fdm_session(head->session_id);
    if (fs == nullptr) {
      lb_common::lb_log_hand tlh(log_);
      warning_log(tlh) << "session not found to discard msg, board_no=" << head->board_no
                       << ", session_id=" << head->session_id << ", msg_id=" << head->msg_id
                       << ", msg_len=" << head->msg_len << end_log;
      return; // 找不到会话，丢弃
    }
    api_link *alink = fs->ln_api_;
    if (alink == nullptr) {
      lb_common::lb_log_hand tlh(log_);
      warning_log(tlh) << "link not found to discard msg, board_no=" << head->board_no
                       << ", session_id=" << head->session_id << ", msg_id=" << head->msg_id
                       << ", msg_len=" << head->msg_len << end_log;
      return; // 会话的 api 链接不存在，丢弃
    }
    api_eng_->send_gw_rej(alink, head, FGW_ERR_LINK_SEND, "fdm link send msg error");
  }
}
// ---- aio_ch_op override ----

void fdm_engine::deal_ch_connect(lb_common::tcp_buf_ch *pch, lb_common::csock_addr &localaddr) {
  fdm_board *fdm = static_cast<fdm_board *>(pch->get_user_data());
  uint16_t board_no = fdm->get_board_no();
  fdm_link *link = fdm->get_link();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fdm link connected, board_no=" << board_no << ", local_ip=" << localaddr.ip
                << ", local_port=" << localaddr.port << ", fdm_ip=" << link->get_remote().ip
                << ", fdm_port=" << link->get_remote().port << end_log;

  // 异步建连成功 → 启动接收
  int32_t ret = link->on_connected();
  if (ret < 0) {
    //link->close_link(ret);
    error_log(tlh) << "fdm link on_connected failed, board_no=" << board_no << ", fdm_ip=" << link->get_remote().ip
                   << ", fdm_port=" << link->get_remote().port << ", ret=" << ret << end_log;
    return;
  }
  info_log(tlh) << "fdm link start ok, board_no=" << board_no << ", fdm_ip=" << link->get_remote().ip
                << ", fdm_port=" << link->get_remote().port << end_log;
}

void fdm_engine::deal_ch_error(lb_common::tcp_buf_ch *pch, lb_common::int32 err_type, lb_common::int32 err_code) {
  fdm_board *fdm = static_cast<fdm_board *>(pch->get_user_data());
  uint16_t board = fdm->get_board_no();
  fdm_link *link = fdm->get_link();

  link->close_link(err_code); // 触发关闭流程 → deal_ch_closing → deal_ch_closed

  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "fdm link get error to close, board_no=" << board << ", fdm_ip=" << link->get_remote().ip
                 << ", fdm_port=" << link->get_remote().port << ", err_type=" << err_type << ", err_code=" << err_code
                 << end_log;
}

void fdm_engine::deal_ch_closing(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) {
  fdm_board *fdm = static_cast<fdm_board *>(pch->get_user_data());
  uint16_t board = fdm->get_board_no();
  fdm_link *link = fdm->get_link();

  // 投递关闭事件到发送队列（发送线程消费时 close_link + sub_ref）
  link->push_close_event();

  lb_common::lb_log_hand tlh(log_);
  warning_log(tlh) << "fdm link closing, board_no=" << board << ", fdm_ip=" << link->get_remote().ip
                   << ", fdm_port=" << link->get_remote().port << ", errcode=" << errcode << end_log;
}

void fdm_engine::deal_recv_stop(lb_common::tcp_buf_ch *pch) {
  fdm_board *fdm = static_cast<fdm_board *>(pch->get_user_data());
  uint16_t board = fdm->get_board_no();
  fdm_link *link = fdm->get_link();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fdm link stop to recv, board_no=" << board << ", fdm_ip=" << link->get_remote().ip
                << ", fdm_port=" << link->get_remote().port << end_log;
}

void fdm_engine::deal_ch_closed(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) {
  fdm_board *fdm = static_cast<fdm_board *>(pch->get_user_data());
  uint16_t board_no = fdm->get_board_no();
  fdm_link *link = fdm->get_link();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fdm link closed, board_no=" << board_no << ", fdm_ip=" << link->get_remote().ip
                << ", fdm_port=" << link->get_remote().port << ", errcode=" << errcode << end_log;
}

} // namespace lb_fgw

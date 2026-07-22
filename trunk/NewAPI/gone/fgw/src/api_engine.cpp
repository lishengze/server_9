// api_engine - api 端连接管理和操作引擎实现
//
// Step 8 场景切片 S2：api 链接建立 / 关闭
//   - init           ：初始化线程池 + 每线程 link_send_que（waker 挂 epoll）
//   - start_listen   ：监听 + start_listen 挂到线程池
//   - deal_listen_accept：分配 connect_id + 线程 → new api_link → accept + start_ch
//   - deal_ch_closed ：移除索引 + 入延迟清理队列（不在回调线程 delete，避免 UAF）
//   - deal_ch_error  ：close_ch 触发关闭流程
//   - deal_msg       ：S4 占位（消费消息）
//
// 参考：gone/api/src/aio_socket_link.cpp（accept_ch + start_ch 同步建立模式）

#include "api_engine.h"

#include "comm_sys.h"
#include "data_def.h"

#include <cstdint>
#include <cstring>
#include <new>

#include "fdm_board.h"
#include "fgw_errno.h"
#include "g1comdefine.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "link_send_que.h"
#include "mlog.h"
#include "mutils.h"

namespace lb_fgw {

static constexpr int api_link_recv_once_num = 6; // 链接一次epoll 唤醒接收次数
static constexpr int api_link_send_once_num = 4; // 链接一次epoll 唤醒发送次数

int32_t api_engine::find_api_link(api_link *&o_link, int32_t connect_id) {
  lb_common::clock_guard<lb_common::cmutex> lk(map_lock_);
  auto it = api_links_.find(connect_id);
  if (it != api_links_.end()) {
    api_link *link = it->second;
    if (nullptr != link && link->add_ref()) {
      o_link = link;
      return 0;
    }
  }
  return FGW_ERR_LINK_NA;
}

// ---- 构造 aio_attr / channel_attr（参考 gone/api build_aio_attr_）----
void api_engine::build_aio_attr_(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr) {
  std::memset(&buf_attr, 0, sizeof(buf_attr));
  std::memset(&ch_attr, 0, sizeof(ch_attr));
  buf_attr.onerecvtimes = api_link_recv_once_num; // 一次 epoll 唤醒最多 recv 次数
  buf_attr.oncerecvlen = sizeof(order_req) * 3;   // 单次接收长度
  buf_attr.maxmsglen = G1_MSG_MAX_LEN;            // 最大消息长度
  buf_attr.buf_size = 2 * G1_MSG_MAX_LEN;         // 接收缓冲 2MB
  buf_attr.dispatch_zero_copy = 0;                // 关闭零拷贝
  buf_attr.heartinterval = heart_interval_;       // 心跳间隔（可配，0=禁用超时检测）
  ch_attr.family = CHANNEL_FAMILY_IPV4;
  ch_attr.recvsockbuflen = 1 * 1024 * 1024;
  ch_attr.sendsockbuflen = 1 * 1024 * 1024;
  ch_attr.tcpdelayack = 1;  // 禁用 Nagle（低延迟）
  ch_attr.sendrecvtime = 5; // 收发超时 5 秒
  ch_attr.localloop = 0;
}

// ---- init：初始化线程池 + 发送队列 ----
int32_t api_engine::init(data_repo *datas, int32_t heart_interval, int32_t thread_num, int32_t que_size,
                         lb_common::csock_addr &listen_addr, lb_common::lb_log *log, std::vector<int32_t> &cpus) {
  datas_ = datas;
  log_ = log;
  heart_interval_ = heart_interval;
  connect_id_ = 1;
  lb_common::comm_utils::str_copy_format(listen_addr_.ip, listen_addr.ip, sizeof(listen_addr_.ip));
  listen_addr_.port = listen_addr.port;

  thread_num = thread_num < 2 ? 2 : thread_num;
  que_size = que_size < 16 ? 16 : que_size;
  int64_t tqs = que_size;
  tqs = tqs * 1024 * 1024;

  // 内部构造发送队列数组
  send_ques_ = new link_send_que<api_link, api_engine>[thread_num];

  int32_t ret = th_pool_.init(thread_num, 1024, 0, 0, 100);
  if (ret < 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "api engine thread pool init failed, thread_num=" << thread_num << ", ret=" << ret << end_log;
    return ret;
  }

  if ((int32_t)(cpus.size()) >= thread_num) {
    for (int32_t i = 0; i < thread_num; ++i) {
      th_pool_.init_set_cpu(i, cpus[i]);
    }
  }

  // 每线程一个 link_send_que：init + waker 挂到对应线程 epoll
  for (int32_t i = 0; i < thread_num; ++i) {
    ret = send_ques_[i].init(this, tqs, api_link_send_once_num);
    if (ret < 0) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "api engine send_que init failed, idx=" << i << ", que_size=" << que_size << ", ret=" << ret
                     << end_log;
      return ret;
    }
    lb_common::mthread *th = th_pool_.get_thread(i);
    ret = th->add_poll_event(send_ques_[i]);
    if (ret < 0) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "api engine add send_que to epoll failed, idx=" << i << ", ret=" << ret << end_log;
      return ret;
    }
  }

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "api engine init ok, thread_num=" << thread_num << ", que_size=" << que_size
                << ", heart_interval=" << heart_interval << end_log;

  return 0;
}

int32_t api_engine::start() {
  lb_common::lb_log_hand tlh(log_);
  int32_t ret = th_pool_.run();
  if (ret < 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "api engine thread pool run failed, ret=" << ret << end_log;
    return ret;
  }

  lb_common::aio_attr buf_attr;
  lb_common::channel_attr ch_attr;
  build_aio_attr_(buf_attr, ch_attr);
  ret = api_listen_.listen_ch(ch_attr, static_cast<tcp_listen_op *>(this), &listen_addr_, nullptr);
  if (ret < 0) {
    error_log(tlh) << "api listen listen_ch failed, ip=" << listen_addr_.ip << ", port=" << listen_addr_.port
                   << ", ret=" << ret << end_log;
    return ret;
  }
  // 监听挂到线程池第 0 个线程
  lb_common::mthread *lth = th_pool_.get_thread(0);
  ret = api_listen_.start_listen(lth);
  if (ret < 0) {
    error_log(tlh) << "api listen start_listen failed, ret=" << ret << end_log;
    return ret;
  }

  info_log(tlh) << "api listen started ok, ip=" << listen_addr_.ip << ", port=" << listen_addr_.port << end_log;

  return 0;
}

// ---- stop：关闭监听 + 所有 api 链接 + 停止线程池 ----
void api_engine::stop() {
  api_listen_.close_ch();
  // 关闭所有 api 链接（close_ch 触发 deal_ch_closing/closed）
  std::vector<api_link *> links;

  map_lock_.lock();
  links.reserve(api_links_.size());
  for (auto &kv : api_links_) {
    api_link *link = kv.second;
    links.push_back(link);
  }
  api_links_.clear();
  map_lock_.unlock();

  for (auto *l : links) {
    l->close_link(0);
    l->sub_ref(); //减去 map 中的计数
  }

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "api engine stop: closed " << links.size() << " api links" << end_log;

  th_pool_.to_stop();
  info_log(tlh) << "api engine stop: thread pool stopped" << end_log;
}

// ---- check_heartbeat：心跳超时检查 + 超时关闭 ----
void api_engine::check_heartbeat() {
  api_link *need_close_links[32];
  int32_t tn = 0;

  map_lock_.lock();
  auto it = api_links_.begin();
  while (it != api_links_.end()) {
    api_link *link = it->second;
    if (!link->check_heart_timeout()) {
      it++;
      continue;
    }
    it = api_links_.erase(it);
    need_close_links[tn] = link;
    tn++;
    if (tn == 32)
      break;
  }
  map_lock_.unlock();

  for (int32_t i = 0; i < tn; i++) {
    api_link *link = need_close_links[i];
    uint32_t cid = link->get_connect_id();
    link->close_link(FGW_ERR_HEART_TIMEOUT);

    lb_common::lb_log_hand tlh(log_);
    warning_log(tlh) << "api link heartbeat timeout to close, connect_id=" << cid << end_log;

    link->sub_ref(); //减去 map 中的计数
  }
}

// ---- deal_listen_accept：接受新连接 ----
lb_common::int32 api_engine::deal_listen_accept(lb_common::tcp_listen_ch *plistench, lb_common::csock_addr &clientaddr,
                                                lb_common::sock_fd fd) {

  lb_common::lb_log_hand tlh(log_);
  int32_t cid = connect_id_.fetch_add(1);

  info_log(tlh) << "api new client connecting, client_ip=" << clientaddr.ip << ", client_port=" << clientaddr.port
                << ", connect_id=" << cid << end_log;

  api_link *link = new (std::nothrow) api_link();
  if (link == nullptr) {
    error_log(tlh) << "api link new failed, connect_id=" << cid << end_log;
    return FGW_ERR_ALLOC_MEM;
  }

  // 分配接收线程（负载最小）
  lb_common::mthread *recvth = nullptr;
  int32_t recvth_id = th_pool_.assign_thread(recvth, -1);

  // 分配发送线程（与接收线程分离，不再 onload 单线程）——排除 recvth
  lb_common::mthread *sendth = nullptr;
  int32_t sendth_id = th_pool_.assign_thread(sendth, recvth_id);

  lb_common::aio_attr buf_attr;
  lb_common::channel_attr ch_attr;
  build_aio_attr_(buf_attr, ch_attr);

  int32_t ret = link->accept(buf_attr, ch_attr, recvth, sendth, &send_ques_[sendth_id], this, this, cid, fd);
  if (ret < 0) {
    error_log(tlh) << "api link accept error, connect_id=" << cid << ", recvth=" << recvth_id
                   << ", sendth=" << sendth_id << ", ret=" << ret << end_log;
    delete link;
    return ret;
  }

  link->add_ref(); //预先增加 map 中的计数

  // accept_ch 同步建立，立即 start_ch 加入 recv 线程 epoll（参考 gone/api 模式）
  ret = link->on_connected();
  if (ret < 0) {
    error_log(tlh) << "api link on_connected error, connect_id=" << cid << ", recvth=" << recvth_id
                   << ", sendth=" << sendth_id << ", ret=" << ret << end_log;
    delete link;
    return ret;
  }

  map_lock_.lock();
  api_links_[cid] = link;
  map_lock_.unlock();

  info_log(tlh) << "api link accepted, client_ip=" << clientaddr.ip << ",client_port=" << clientaddr.port
                << ",connect_id=" << cid << ", recvth=" << recvth_id << ", sendth=" << sendth_id << end_log;
  return 0;
}

// ---- deal_msg：解析 g1 头 → 按 msg_id 分发（S4 投递骨架）----
// 心跳自答已实现；业务路由留待 S8（上行）/ S10（证券）
lb_common::int32 api_engine::deal_msg(lb_common::tcp_buf_ch *pch, lb_common::aio_msg &msg) {
  int32_t deal_len = 0;
  int32_t full_len = 0;
  while (deal_len < msg.msglen) {
    // 头不完整：返回 0，等更多数据（aio_recv_buf 累积）
    if (unlikely(msg.msglen - deal_len < (lb_common::int32)sizeof(g1_msg_head)))
      break;
    g1_msg_head *head = reinterpret_cast<g1_msg_head *>(msg.pmsg + deal_len);
    full_len = (int32_t)(sizeof(g1_msg_head) + head->msg_len);
    if (unlikely(full_len > G1_MSG_MAX_LEN)) {
      api_link *elink = static_cast<api_link *>(pch->get_user_data());
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "api msg too long, connect_id=" << elink->get_connect_id() << ", msg_id=" << head->msg_id
                     << ", msg_len=" << head->msg_len << ", full_len=" << full_len << end_log;
      return FGW_ERR_MSG_LEN;
    }
    if (unlikely(msg.msglen - deal_len < full_len)) // 体不完整
      break;

    api_link *link = static_cast<api_link *>(pch->get_user_data());
    switch (head->msg_id) {
    case G1_MSG_HEART_REQ: {
      link->on_heart_msg(); // 心跳维持（aio_tcp on_msg 被注释，手动调）
      send_heart_ans(link);
      break;
    }
    case G1_MSG_ORDER_REQ:
    case G1_MSG_CANCEL_REQ:
      //link->on_msg_heart(); // 业务消息维持活性（手动调）
      route_upstream(link, head);
      break;
    case G1_MSG_LOGIN_REQ:
      link->on_msg_heart(); // 业务消息维持活性（手动调）
      deal_login_req(link, head);
      break;
    case G1_MSG_SEC_INFO_REQ:
      link->on_msg_heart(); // 业务消息维持活性（手动调）
      deal_sec_req(link, head);
      break;
    default: {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "api unknown msg_id, connect_id=" << link->get_connect_id() << ", msg_id=" << head->msg_id
                    << ", msg_len=" << head->msg_len << end_log;
    } break;
    }

    deal_len += full_len;
  }
  return deal_len; // 消费一条（支持粘包，loop_deal_recv 循环处理）
}

// ---- S8 业务上行路由 ----

/// 业务上行（ORDER/CANCEL）：board_no → fdm_link 透传；失败回 GW_REJ
void api_engine::route_upstream(api_link *link, g1_msg_head *head) {
  //查找 fdm board
  fdm_board *pboard = nullptr;
  int32_t ret = datas_->find_fdm(pboard, head->board_no);
  if (ret < 0) {
    send_gw_rej(link, head, FGW_ERR_BOARD_NA, "not find board");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "not find board, connect_id=" << link->get_connect_id() << ", msg_id=" << head->msg_id
                   << ", msg_len=" << head->msg_len << ", board_no=" << head->board_no << ", user_id=" << head->user_id
                   << ", session_id=" << head->session_id << ", ret=" << ret << end_log;
    return;
  }

  customer *pcust = pboard->find_customer(head->user_id);
  if (nullptr == pcust) {
    send_gw_rej(link, head, FGW_ERR_CUST_NA, "not find customer");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "not find customer, connect_id=" << link->get_connect_id() << ", msg_id=" << head->msg_id
                   << ", msg_len=" << head->msg_len << ", board_no=" << head->board_no << ", user_id=" << head->user_id
                   << ", session_id=" << head->session_id << ", ret=" << ret << end_log;
    return;
  }

  fdm_session *pses = pboard->find_fdm_session(head->session_id);
  if (nullptr == pses) {
    send_gw_rej(link, head, FGW_ERR_AGWSESS_NA, "not find fdm session");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "not find fdm session, connect_id=" << link->get_connect_id() << ", msg_id=" << head->msg_id
                   << ", msg_len=" << head->msg_len << ", board_no=" << head->board_no << ", user_id=" << head->user_id
                   << ", session_id=" << head->session_id << ", ret=" << ret << end_log;
    return;
  }

  fdm_link *flink = pboard->get_link();
  ret = flink->push_send(reinterpret_cast<char *>(head), head->msg_len + sizeof(g1_msg_head));
  if (ret < 0) {
    send_gw_rej(link, head, ret, "push to send to board link failed");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "push to send to board link failed, connect_id=" << link->get_connect_id()
                   << ", msg_id=" << head->msg_id << ", msg_len=" << head->msg_len << ", board_no=" << head->board_no
                   << ", user_id=" << head->user_id << ", session_id=" << head->session_id << ", ret=" << ret
                   << end_log;
    return;
  }
}

/// 投资者登录上行中转：fundacc → (board_no,user_id) 补全头 → fdm
void api_engine::deal_login_req(api_link *link, g1_msg_head *head) {
  login_req *nreq = reinterpret_cast<login_req *>(head + 1);
  lb_common::comm_utils::str_format(nreq->cust_id, sizeof(nreq->cust_id));
  lb_common::comm_utils::str_format(nreq->fund_account_id, sizeof(nreq->fund_account_id));
  lb_common::comm_utils::str_format(nreq->branch_id, sizeof(nreq->branch_id));
  lb_common::comm_utils::str_format(nreq->holder_acc, sizeof(nreq->holder_acc));
  lb_common::comm_utils::str_format(nreq->session, sizeof(nreq->session)); // = agwuser name

  if (head->msg_len > sizeof(login_req)) {
    //send_gw_rej(link, head, FGW_ERR_MSG_LEN, "not support msg len");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "login msg len not support, connect_id=" << link->get_connect_id()
                   << ", msg_len=" << head->msg_len << ", agw_user=" << nreq->session << ", cust_id=" << nreq->cust_id
                   << ", branch_id=" << nreq->branch_id << ", fund_account=" << nreq->fund_account_id << end_log;
    return;
  }

  if (std::strncmp(g1_msg_ver, nreq->version, std::strlen(g1_msg_ver)) < 0) {
    send_login_error(link, head, FGW_ERR_MSG_VER, "not support msg version");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "login not support msg version, connect_id=" << link->get_connect_id()
                   << ", version=" << nreq->version << ", agw_user=" << nreq->session << ", cust_id=" << nreq->cust_id
                   << ", branch_id=" << nreq->branch_id << ", fund_account=" << nreq->fund_account_id << end_log;
    return;
  }

  // 查找 客户资金账户 是否存在
  fgw_fundacc_key fundkey(nreq->fund_account_id, nreq->branch_id);
  board_user_id buid;
  int32_t ret = datas_->find_cust(buid, fundkey);
  if (ret < 0) {
    send_login_error(link, head, FGW_ERR_CUST_NA, "not find cust board");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "login not find cust board, connect_id=" << link->get_connect_id()
                   << ", agw_user=" << nreq->session << ", cust_id=" << nreq->cust_id
                   << ", branch_id=" << nreq->branch_id << ", fund_account=" << nreq->fund_account_id << ", ret=" << ret
                   << end_log;
    return;
  }

  // 补全头 + req_connect_id（关联 key，fdm 应答带回，S9 回找 api 链接）
  head->board_no = buid.board_no_;
  head->user_id = buid.user_id_;
  head->session_id = 0;
  nreq->req_connect_id = link->get_connect_id();

  //查找 fdm board
  fdm_board *pboard = NULL;
  ret = datas_->find_fdm(pboard, buid.board_no_);
  if (ret < 0 || pboard == nullptr) {
    send_login_error(link, head, FGW_ERR_BOARD_NA, "not find board");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "login not find board, connect_id=" << link->get_connect_id() << ", agw_user=" << nreq->session
                   << ", cust_id=" << nreq->cust_id << ", branch_id=" << nreq->branch_id
                   << ", fund_account=" << nreq->fund_account_id << ", board_no=" << buid.board_no_ << ", ret=" << ret
                   << end_log;
    return;
  }

  link->set_user_board(nreq->log_type, buid.board_no_, buid.user_id_);

  // 添加 agw user 及其 api link（返回 agw_user*，登录应答时由 fdm_engine 登记 fdm_ses）
  agw_user_key agwkey(nreq->session);
  agw_user *agw = datas_->add_agwuser_link(agwkey, link);
  if (agw == nullptr) {
    send_login_error(link, head, FGW_ERR_ADD_AGWSESS, "build api link and agwuser session relation error");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "build api link and agwuser session relation error, connect_id=" << link->get_connect_id()
                   << ", session=" << nreq->session << ", agw_user=" << nreq->session << ", cust_id=" << nreq->cust_id
                   << ", branch_id=" << nreq->branch_id << ", fund_account=" << nreq->fund_account_id << end_log;
    return;
  }

  customer *pcust = pboard->find_customer(buid.user_id_);
  if (nullptr == pcust) {
    send_login_error(link, head, FGW_ERR_CUST_NA, "not find customer");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "login not found customer, connect_id=" << link->get_connect_id()
                   << ", agw_user=" << nreq->session << ", cust_id=" << nreq->cust_id
                   << ", branch_id=" << nreq->branch_id << ", fund_account=" << nreq->fund_account_id
                   << ", board_no=" << buid.board_no_ << ", ret=" << ret << end_log;
    return;
  }

  char off_buf[sizeof(fpga_user_state) + sizeof(g1_msg_head)];
  g1_msg_head *off_head = reinterpret_cast<g1_msg_head *>(off_buf);
  off_head->msg_id = G1_MSG_OFFLINE_PUSH;
  off_head->msg_len = sizeof(fpga_user_state);
  off_head->board_no = pboard->get_board_no();
  off_head->user_id = pcust->user_id_;
  off_head->session_id = 0;
  fpga_user_state *off_body = reinterpret_cast<fpga_user_state *>(off_head + 1);
  off_body->cur_time = 0;
  off_body->board_no = pboard->get_board_no();
  off_body->market_type = pcust->market_type_;
  off_body->user_id = pcust->user_id_;
  off_body->state = pcust->onboard_state;
  std::memcpy(off_body->branch_id, pcust->branch_id_, sizeof(off_body->branch_id));
  std::memcpy(off_body->fund_account_id, pcust->fund_account_id_, sizeof(off_body->fund_account_id));
  link->push_onboard_state(off_head);

  if (pcust->onboard_state != 1) {
    send_login_error(link, head, FGW_ERR_CUST_OFFLINE, "customer on board state error");

    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "login customer on board state error, connect_id=" << link->get_connect_id()
                  << ", agw_user=" << nreq->session << ", cust_id=" << nreq->cust_id
                  << ", branch_id=" << nreq->branch_id << ", fund_account=" << nreq->fund_account_id
                  << ", board_no=" << buid.board_no_ << ", ret=" << ret << end_log;

    return;
  }

  fdm_link *flink = pboard->get_link();
  ret = flink->push_send(reinterpret_cast<char *>(head), head->msg_len + sizeof(g1_msg_head));
  if (ret < 0) {
    send_login_error(link, head, ret, "push to send to board link failed");

    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "login push to send to board link failed, connect_id=" << link->get_connect_id()
                   << ", board_no=" << buid.board_no_ << ", agw_user=" << nreq->session << ", cust_id=" << nreq->cust_id
                   << ", branch_id=" << nreq->branch_id << ", fund_account=" << nreq->fund_account_id << ", ret=" << ret
                   << end_log;
    return;
  }

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "route_login_upstream ok, connect_id=" << nreq->req_connect_id << ", agw_user=" << nreq->session
                << ", cust_id=" << nreq->cust_id << ", branch_id=" << nreq->branch_id
                << ", fund_account=" << nreq->fund_account_id << ", user_id=" << buid.user_id_
                << ", board_no=" << buid.board_no_ << end_log;
}

/// 构造 GW_REJ 回 api 链接
void api_engine::send_gw_rej(api_link *link, g1_msg_head *req_head, int32_t err_code, const char *err_msg) {
  char buf[G1_MSG_MAX_LEN + sizeof(g1_msg_head)];
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(buf);
  head->msg_id = G1_MSG_GW_REJ;
  head->msg_len = req_head->msg_len + sizeof(g1_gw_rej_head);
  head->board_no = req_head->board_no;
  head->user_id = req_head->user_id;
  head->session_id = req_head->session_id;
  g1_gw_rej_head *rej = reinterpret_cast<g1_gw_rej_head *>(buf + sizeof(g1_msg_head));
  std::memset(rej, 0, sizeof(g1_gw_rej_head));
  rej->rej_msg_id = req_head->msg_id;
  rej->err_code = err_code;
  uint32_t tlen = std::strlen(err_msg);
  tlen = tlen >= sizeof(rej->err_msg) ? (sizeof(rej->err_msg) - 1) : tlen;
  std::strncpy(rej->err_msg, err_msg, tlen);
  int32_t ret = link->push_send(buf, head->msg_len + sizeof(g1_msg_head));

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "to send gw reject msg, connect_id=" << link->get_connect_id() << ", rej_msg_id=" << rej->rej_msg_id
                << ", msg_len=" << head->msg_len << ", board_no=" << head->board_no << ", user_id=" << head->user_id
                << ", session_id=" << head->session_id << ", ret=" << ret << ", rej_err_code=" << rej->err_code
                << ", rej_err_msg=" << rej->err_msg << end_log;
}

void api_engine::send_login_error(api_link *link, g1_msg_head *req_head, int32_t err_code, const char *err_msg) {
  char buf[sizeof(login_ans) + sizeof(g1_msg_head)];
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(buf);
  head->msg_id = G1_MSG_LOGIN_ANS;
  head->msg_len = sizeof(login_ans);
  head->board_no = req_head->board_no;
  head->user_id = req_head->user_id;
  head->session_id = req_head->session_id;
  login_req *req = reinterpret_cast<login_req *>(req_head + 1);
  login_ans *rej = reinterpret_cast<login_ans *>(buf + sizeof(g1_msg_head));
  std::memset(rej, 0, sizeof(login_ans));
  rej->cust_req_no = req->cust_req_no;
  lb_common::comm_utils::str_copy_format(rej->cust_id, req->cust_id, sizeof(rej->cust_id));
  lb_common::comm_utils::str_copy_format(rej->fund_account_id, req->fund_account_id, sizeof(rej->fund_account_id));
  lb_common::comm_utils::str_copy_format(rej->branch_id, req->branch_id, sizeof(rej->branch_id));
  lb_common::comm_utils::str_copy_format(rej->holder_acc, req->holder_acc, sizeof(rej->holder_acc));
  lb_common::comm_utils::str_copy_format(rej->session, req->session, sizeof(rej->session));
  std::memcpy(rej->end_code, req->end_code, sizeof(rej->end_code));
  rej->user_id = 0;
  rej->board_no = 0;
  rej->proto_type = 1;
  rej->order_way[0] = req->order_way[0];
  rej->order_way[1] = req->order_way[1];
  rej->req_connect_id = 0;
  rej->log_type = req->log_type;
  rej->session_id = 0;
  rej->err_code = err_code;
  rej->reserved = 0;
  if (nullptr != err_msg) {
    uint32_t tlen = std::strlen(err_msg);
    tlen = tlen >= sizeof(rej->err_msg) ? (sizeof(rej->err_msg) - 1) : tlen;
    std::strncpy(rej->err_msg, err_msg, tlen);
  }
  std::strncpy(rej->version, g1_msg_ver, strlen(g1_msg_ver));
  rej->login_time = lb_common::comm_utils::get_time_ms();
  int32_t ret = link->push_send(buf, head->msg_len + sizeof(g1_msg_head));

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "to send login ans msg, connect_id=" << link->get_connect_id() << ", cust_id=" << rej->cust_id
                << ", branch_id=" << rej->branch_id << ", fund_account=" << rej->fund_account_id
                << ", session=" << rej->session << ", ret=" << ret << ", err_code=" << rej->err_code
                << ", err_msg=" << rej->err_msg << end_log;
}

void api_engine::deal_sec_req(api_link *link, g1_msg_head *req_head) {
  sec_info_req *treq = reinterpret_cast<sec_info_req *>(req_head + 1);
  int32_t idx = 0;
  while (1) {
    char tbuf[1200 + sizeof(g1_msg_head)];
    g1_msg_head *thead = reinterpret_cast<g1_msg_head *>(tbuf);
    thead->msg_id = G1_MSG_SEC_INFO_ANS;
    thead->board_no = 0;
    thead->user_id = req_head->user_id;
    thead->session_id = req_head->session_id;
    sec_push_head *sec_head = reinterpret_cast<sec_push_head *>(thead + 1);
    sec_head->cust_req_no = treq->cust_req_no;

    int32_t tnum = datas_->patch_secs_msg(sec_head, 1200, idx);
    if (likely(tnum > 0)) {
      thead->msg_len = sec_head->cur_num * sizeof(sec_push_info) + sizeof(sec_push_head);
      int32_t ret = link->push_send(tbuf, thead->msg_len + sizeof(g1_msg_head));
      if (ret < 0) {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "push sec ans msg to send error to close api link, connect_id=" << link->get_connect_id()
                       << ", ret =" << ret << end_log;

        link->close_link(FGW_ERR_BUF_FULL);
        return;
      }
      idx += tnum;
    } else {
      break;
    }
  }
}
void api_engine::send_heart_ans(api_link *link) {
  g1_msg_head ans;
  ans.msg_id = G1_MSG_HEART_ANS;
  ans.msg_len = 0;
  ans.board_no = 0;
  ans.user_id = 0;
  ans.session_id = 0;
  link->push_send(reinterpret_cast<const char *>(&ans), (int32_t)sizeof(ans));
}

void api_engine::deal_send_error(api_link *link, char *pmsg, int32_t msg_len, int32_t err_ret) {
  lb_common::lb_log_hand tlh(log_);
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(pmsg);
  error_log(tlh) << "api link send msg error, connect_id=" << link->get_connect_id() << ", msg_id=" << head->msg_id
                 << ", msg_len=" << head->msg_len << ", ret=" << err_ret << end_log;
}

// ---- aio_ch_op override ----
void api_engine::deal_ch_error(lb_common::tcp_buf_ch *pch, lb_common::int32 err_type, lb_common::int32 err_code) {
  api_link *link = static_cast<api_link *>(pch->get_user_data());
  uint32_t cid = link->get_connect_id();

  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "api link get error to close, connect_id=" << cid << ", err_type=" << err_type
                 << ", err_code=" << err_code << end_log;

  link->close_link(err_code);
}

void api_engine::deal_ch_connect(lb_common::tcp_buf_ch *pch, lb_common::csock_addr &localaddr) {
  // accept_ch 不会回调该接口
}

void api_engine::deal_ch_closing(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) {
  api_link *link = static_cast<api_link *>(pch->get_user_data());
  uint32_t cid = link->get_connect_id();

  // 解绑 agw_user 并投递 fdm_session 解除引用事件（内部完成 sub_ref）
  datas_->detach_agwuser_link(link);

  // 投递关闭事件到发送队列
  link->push_close_event();

  // 移除索引（不再可见）
  map_lock_.lock();
  api_links_.erase(cid);
  map_lock_.unlock();
  link->sub_ref(); // 减去 map 中引用

  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "api link closing to put close event, connect_id=" << cid << ", errcode=" << errcode << end_log;
}

void api_engine::deal_recv_stop(lb_common::tcp_buf_ch *pch) {
  api_link *link = static_cast<api_link *>(pch->get_user_data());
  uint32_t cid = link->get_connect_id();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "api link stop to recv, connect_id=" << cid << end_log;
}

void api_engine::deal_ch_closed(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) {
  api_link *link = static_cast<api_link *>(pch->get_user_data());
  uint32_t cid = link->get_connect_id();

  delete link; // deal_after_closed 后引用归零、无其他线程持有 aio_，回调栈上 delete 安全

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "api link closed end, connect_id=" << cid << ", errcode=" << errcode << end_log;
}

// ---- tcp_listen_op override ----

void api_engine::deal_listen_error(lb_common::tcp_listen_ch *pch, lb_common::int32 errcode,
                                   lb_common::int32 reasonerr) {
  (void)pch;
  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "api listen error, errcode=" << errcode << ", reasonerr=" << reasonerr << end_log;
}

void api_engine::deal_listen_to_close(lb_common::tcp_listen_ch *pch, lb_common::int32 errcode) {
  (void)pch;
  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "api listen to close, errcode=" << errcode << end_log;
}

void api_engine::deal_listen_closed(lb_common::tcp_listen_ch *pch, lb_common::int32 errcode) {
  (void)pch;
  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "api listen closed, errcode=" << errcode << end_log;
}

} // namespace lb_fgw

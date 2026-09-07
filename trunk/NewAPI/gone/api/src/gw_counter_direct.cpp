// gw_counter_direct - 个微软件极速柜台 (直连模式) 实现
//
// 个微柜台无查询接口, 仅直连模式 (C1/C2 配置)
// 业务消息: 委托 / 撤单 / ETF申购赎回 / BSE
// 推送: 委托 / 成交回报
//
// 协议状态:
//   - 个微真实协议是外部定义, 当前未提供.
//   - 临时替代: 消息头用 g1_msg_head, 登录消息体用 g1 login_req/login_ans.
//   - 委托/撤单等业务消息体暂留空 + // todo.

#include "gw_counter_direct.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "matomic.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

#include <cstring>

namespace lb_api {

// 构造 / 析构
gw_counter_direct::gw_counter_direct()
    : market_type(0), heart_interval(5), login_state(0), trade_send_queue_(nullptr), cb_mgr_(nullptr), session_seq_(0),
      log_(nullptr) {}

gw_counter_direct::~gw_counter_direct() = default;

// init
int32 gw_counter_direct::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log) {
  cb_mgr_ = cb;
  log_ = log;
  market_type = static_cast<int16>(cfg.get_market_type());           ///< 市场
  heart_interval = static_cast<int16>(cfg.get_heartbeat_interval()); ///< 心跳间隔
  login_state = 0;                                                   ///< 0-未登陆
  trade_link_connect_ = 0;                                           ///< 0-未链接/断开
  session_seq_ = 0;

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "init gw counter direct ok, market_type=" << market_type << ", heart_interval=" << heart_interval
                << end_log;
  return 0;
}

// ---- 业务发送函数 (与 counter98 同样模式: 状态检查 -> take_req_que_mem -> build -> cmt_req_que_mem) ----

// deal_order_req: 买卖委托 (个微真实协议未知, build_order_msg 留空)
int32 gw_counter_direct::deal_order_req(const OrderReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_order_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  if (login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_order_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }

  // todo : 需要依据正式个微协议，重新实现消息构造和入队
  int32 take_len = sizeof(g1_msg_head);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_order_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

// deal_etf_order_req: ETF 申购赎回 (个微真实协议未知, build_etf_order_msg 留空)
int32 gw_counter_direct::deal_etf_order_req(const OrderReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_etf_order_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  if (login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_etf_order_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }

  // todo : 需要依据正式个微协议，重新实现消息构造和入队
  int32 take_len = sizeof(g1_msg_head);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_etf_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_etf_order_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

// deal_cancel_req: 委托撤单 (个微真实协议未知, build_cancel_msg 留空)
int32_t gw_counter_direct::deal_cancel_req(const CancelReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_cancel_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  if (login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_cancel_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }

  // todo : 需要依据正式个微协议，重新实现消息构造和入队
  int32 take_len = sizeof(g1_msg_head);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_cancel_req: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_cancel_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

// ---- 业务消息构建: 真实协议未知, 全部留空 ----

// build_order_msg: 构造个微委托消息 (头 + 个微协议体)
void gw_counter_direct::build_order_msg(const OrderReq &req, char *o_buf) {
  // todo : 依据正式个微协议重写
  (void)req;
  (void)o_buf;
}

// build_etf_order_msg: 构造个微 ETF 申购赎回消息 (头 + 个微协议体)
void gw_counter_direct::build_etf_order_msg(const OrderReq &req, char *o_buf) {
  // todo : 依据正式个微协议重写
  (void)req;
  (void)o_buf;
}

// build_cancel_msg: 构造个微撤单消息 (头 + 个微协议体)
void gw_counter_direct::build_cancel_msg(const CancelReq &req, char *o_buf) {
  // todo : 依据正式个微协议重写
  (void)req;
  (void)o_buf;
}

// ---- 登录消息构建: 临时使用 g1 login_req 替代 ----

// build_login_msg: 构造个微账户登录消息 (g1_msg_head + login_req)
// (个微真实协议未知, 临时使用 g1 login_req 替代, 待真实协议补充后改回)
void gw_counter_direct::build_login_msg(const acc_login_event_info &info, int16_t log_type, g1_msg_head *o_req) {
  o_req->msg_id = G1_MSG_LOGIN_REQ;
  o_req->msg_len = sizeof(login_req);
  o_req->user_id = 0;
  o_req->board_no = 0;
  o_req->session_id = 0;

  login_req *body = reinterpret_cast<login_req *>(o_req + 1);
  std::memset(body, 0, sizeof(*body));
  body->cust_req_no = info.cust_req_no;
  std::memcpy(body->cust_id, info.cust_id, std::min<int32>(sizeof(body->cust_id), sizeof(info.cust_id)));
  std::memcpy(body->fund_account_id, info.fund_account_id,
              std::min<int32>(sizeof(body->fund_account_id), sizeof(info.fund_account_id)));
  std::memcpy(body->branch_id, info.branch_id, std::min<int32>(sizeof(body->branch_id), sizeof(info.branch_id)));
  std::memcpy(body->holder_acc, info.account_id, std::min<int32>(sizeof(body->holder_acc), sizeof(info.account_id)));
  std::memcpy(body->session, info.session, std::min<int32>(sizeof(body->session), sizeof(info.session)));
  std::memcpy(body->end_code, info.client_feature_code,
              std::min<int32>(sizeof(body->end_code), sizeof(info.client_feature_code)));
  body->market_type = market_type;
  body->order_way[0] = info.order_way_ext[0];
  body->order_way[1] = info.order_way_ext[1];
  body->log_type = log_type;
  body->heart_bt_int = static_cast<int32_t>(heart_interval);
  body->req_connect_id = 0; // API 侧设为 0, 网关填写
  std::memset(body->version, 0, sizeof(body->version));
  std::strncpy(body->version, g1_msg_ver, std::strlen(g1_msg_ver));
}

// ---- 登录应答构造 (复用 fpga 逻辑, 临时使用 g1 login_ans 替代) ----

// build_login_rtn: 从 acc_login_event_info 构建 API 层 LoginAns (供 ans_cust_login 失败路径使用)
void gw_counter_direct::build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg,
                                        LoginAns &ans) {
  ans.client_req_no = info.cust_req_no;
  std::memcpy(ans.cust_id.data(), info.cust_id, std::min<int32>(sizeof(ans.cust_id), sizeof(info.cust_id)));
  std::memcpy(ans.fund_account_id.data(), info.fund_account_id,
              std::min<int32>(sizeof(ans.fund_account_id.data()), sizeof(info.fund_account_id)));
  std::memcpy(ans.account_id.data(), info.account_id,
              std::min<int32>(sizeof(ans.account_id.data()), sizeof(info.account_id)));
  std::memcpy(ans.branch_id.data(), info.branch_id,
              std::min<int32>(sizeof(ans.branch_id.data()), sizeof(info.branch_id)));
  ans.market_type = market_type;
  ans.err_code = err_ret;
  if (err_msg != nullptr) {
    lb_common::comm_utils::str_copy_format(ans.err_msg.data(), err_msg, sizeof(ans.err_msg));
  } else {
    std::memset(ans.err_msg.data(), 0, sizeof(ans.err_msg));
  }
  ans.login_time = 0; // 个微协议未知, 暂时置 0, 待真实协议补充
}

// build_login_rtn: 从 g1 login_ans 构建 API 层 LoginAns
void gw_counter_direct::build_login_rtn(const login_ans &msg, LoginAns &o_ans) {
  std::memset(&o_ans, 0, sizeof(o_ans));
  o_ans.client_req_no = msg.cust_req_no;
  lb_common::comm_utils::str_copy_format(o_ans.cust_id.data(), msg.cust_id, sizeof(o_ans.cust_id));
  lb_common::comm_utils::str_copy_format(o_ans.fund_account_id.data(), msg.fund_account_id,
                                         sizeof(o_ans.fund_account_id));
  lb_common::comm_utils::str_copy_format(o_ans.account_id.data(), msg.holder_acc,
                                         std::min<int32>(sizeof(o_ans.account_id.data()), sizeof(msg.holder_acc)));
  lb_common::comm_utils::str_copy_format(o_ans.branch_id.data(), msg.branch_id,
                                         std::min<int32>(sizeof(o_ans.branch_id.data()), sizeof(msg.branch_id)));
  o_ans.market_type = market_type;
  o_ans.err_code = msg.err_code;
  if (msg.err_msg[0] != '\0') {
    int32 tlen = static_cast<int32>(strnlen(msg.err_msg, sizeof(msg.err_msg)));
    tlen = tlen < static_cast<int32>(sizeof(o_ans.err_msg)) ? tlen : static_cast<int32>(sizeof(o_ans.err_msg)) - 1;
    std::memcpy(o_ans.err_msg.data(), msg.err_msg, tlen);
  }
  o_ans.login_time = msg.login_time;
}

// ---- deal_cust_login / ans_cust_login: 构造登录消息并切换状态 ----

// deal_cust_login: 处理账户登录事件, 构造个微登录消息 (o_buf 由调用方分配, 至少 sizeof(g1_msg_head)+sizeof(login_req))
// 成功返回消息长度 (含消息头), 0-不需重复登陆, <0 出错
int32 gw_counter_direct::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len) {
  if (login_state == 2) {
    LoginAns ans;
    build_login_rtn(req, 0, NULL, ans);
    cb_mgr_->on_login(ans);
    return 0;
  }

  int32 msg_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(login_req));
  if (buf_len < msg_len) {
    return LBAPI_ERR_MSG_LEN;
  }
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
  // log_type=1 表示用户登录 (个微直连单客户, 使用用户登录)
  build_login_msg(req, 1, head);
  login_state = 1; ///< 进入登陆中状态
  return msg_len;
}

// ans_cust_login: 登录结果回调 (失败路径: 在引擎同步阶段失败时调用)
void gw_counter_direct::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg) {
  login_state = 0; ///< 失败重置状态

  LoginAns ans;
  build_login_rtn(req, err_ret, err_msg, ans);
  cb_mgr_->on_login(ans);
}

// deal_log_ans: 处理个微账户登录应答 (临时用 g1 login_ans 替代, 待真实协议补充后改回)
// 个微直连无核心链接, 成功则直接 cb_mgr_->on_login
void gw_counter_direct::deal_log_ans(login_ans &msg) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "recv gw cust login answer msg, user_seq_no=" << msg.cust_req_no << ", branch_id=" << msg.branch_id
                << ", fund_account=" << msg.fund_account_id << ", session=" << msg.session
                << ", err_code=" << msg.err_code << end_log;

  LoginAns ans;
  build_login_rtn(msg, ans);
  if (msg.err_code == 0) {
    login_state = 2; ///< 登陆成功
    cb_mgr_->on_login(ans);
  } else {
    login_state = 0;
    cb_mgr_->on_login(ans); ///< 失败也回调, 让上层感知
  }
}

// ---- deal_recv_msg: 解析个微返回消息 (临时按 g1 msg_id 分发) ----

// deal_recv_msg: 处理个微返回消息
// (个微真实协议未知, 临时按 g1 msg_id 分发, 待真实协议补充后改回)
int32 gw_counter_direct::deal_recv_msg(const char *buf, int32 len, int16 link_type) {
  (void)link_type;
  if (buf == nullptr || len < (int32)sizeof(g1_msg_head)) {
    return 0;
  }

  int32 deal_len = 0;
  int32 mlen = 0;
  const g1_msg_head *head;

  while (static_cast<int32>(len) - deal_len >= static_cast<int32>(sizeof(g1_msg_head))) {
    head = reinterpret_cast<const g1_msg_head *>(buf + deal_len);
    mlen = static_cast<int32>(head->msg_len) + static_cast<int32>(sizeof(g1_msg_head));
    if (unlikely(mlen > len - deal_len)) {
      if (mlen < G1_MSG_MAX_LEN) {
        return deal_len;
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "gw counter recv msg len error, mlen=" << mlen << ", link_type=" << link_type << end_log;
        // 返回错误, 让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
    }

    switch (head->msg_id) {
    case G1_MSG_LOGIN_ANS: {
      if (head->msg_len >= sizeof(login_ans)) {
        const login_ans *ans = reinterpret_cast<const login_ans *>(head + 1);
        deal_log_ans(*const_cast<login_ans *>(ans));
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "gw counter recv login_ans msg len too short, msg_len=" << head->msg_len << end_log;
        return LBAPI_ERR_MSG_LEN;
      }
      break;
    }
    case G1_MSG_HEART_ANS: {
      if (trade_eng_op_ != nullptr) {
        trade_eng_op_->deal_heart_msg_ans(LINK_TYPE_SPEED_TRADE);
      }
      break;
    }
    // 个微真实协议未知, 委托/成交/撤单回报等 msg_id 分发暂留空 + // todo
    case G1_MSG_ORDER_RTN:
    case G1_MSG_TRADE_RTN:
    case G1_MSG_CANCEL_RSP:
    default: {
      // 未知/未实现消息, 跳过, 不返回错误, 以避免链接关闭
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "gw counter recv unimplemented msg, msg_id=" << head->msg_id << ", msg_len=" << head->msg_len
                    << end_log;
      // todo : 依据正式个微协议重写
      break;
    }
    }

    deal_len += mlen;
  }

  return deal_len;
}

// ---- deal_send_error / build_api_*_rej: 复用 fpga 模式 (临时按 g1 头构造 API 层回报) ----

void gw_counter_direct::build_api_order_rej(const g1_msg_head *msg, int32 err_code, OrderRtn &o_rtn,
                                            StreamInfo &o_stream) {
  // todo : 依据正式个微协议重写
  std::memset(&o_rtn, 0, sizeof(o_rtn));
  o_rtn.market_type = market_type;
  o_rtn.err_code = err_code;
  o_rtn.rtn_type = RSP_TYPE_ORDER_DISCARD;
  o_rtn.order_status = ORDER_STATE_DISCARD;
  o_stream.counter_type = get_counter_type();
  (void)msg;
  o_stream.stream_seq = 0;
}

void gw_counter_direct::build_api_cancel_rej(const g1_msg_head *msg, int32 err_code, CancelRsp &o_rtn,
                                             StreamInfo &o_stream) {
  // todo : 依据正式个微协议重写
  std::memset(&o_rtn, 0, sizeof(o_rtn));
  o_rtn.market_type = market_type;
  o_rtn.err_code = err_code;
  o_rtn.rej_api = 1; ///< API 层拒绝
  o_stream.counter_type = get_counter_type();
  (void)msg;
  o_stream.stream_seq = 0;
}

void gw_counter_direct::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret) {
  (void)msg_len;
  (void)link_type;
  if (msg_buf == nullptr || cb_mgr_ == nullptr) {
    return;
  }
  if (msg_len < (int32)sizeof(g1_msg_head)) {
    return;
  }
  const g1_msg_head *head = reinterpret_cast<const g1_msg_head *>(msg_buf);
  // todo : 依据正式个微协议重写
  switch (head->msg_id) {
  case G1_MSG_ORDER_REQ: {
    OrderRtn o_rtn;
    StreamInfo o_stream;
    build_api_order_rej(head, err_ret, o_rtn, o_stream);
    cb_mgr_->on_order_rtn(o_stream, o_rtn);
    break;
  }
  case G1_MSG_CANCEL_REQ: {
    CancelRsp o_rtn;
    StreamInfo o_stream;
    build_api_cancel_rej(head, err_ret, o_rtn, o_stream);
    cb_mgr_->on_cancel_rsp(o_stream, o_rtn);
    break;
  }
  default:
    break;
  }
}

// ---- build_heart_msg: 构造个微心跳消息 (临时按 g1 头实现) ----

int32 gw_counter_direct::build_heart_msg(char *o_buf, int32 buf_len) {
  if (o_buf == nullptr || buf_len < (int32)sizeof(g1_msg_head)) {
    return -1;
  }
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
  head->msg_id = G1_MSG_HEART_REQ;
  head->msg_len = 0;
  head->user_id = 0;
  head->board_no = 0;
  head->session_id = 0;
  return static_cast<int32>(sizeof(g1_msg_head));
}

// ---- 链接状态通知 ----

int32 gw_counter_direct::deal_link_connect(int16 link_type, int32 have_switch) {
  (void)have_switch; // 个微柜台不支持地址切换, 此参数忽略
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    trade_link_connect_ = 1;
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(get_counter_type(), 0, 1);
    }
    // todo : 链接建立，是否重新登陆？
  }
  return 0;
}

void gw_counter_direct::deal_link_close(int16 link_type) {
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    trade_link_connect_ = 0;
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(get_counter_type(), 0, 0);
    }
    // todo : 链接断开时, 是否登录状态重置
    // login_state = 0;
  }
}

} // namespace lb_api

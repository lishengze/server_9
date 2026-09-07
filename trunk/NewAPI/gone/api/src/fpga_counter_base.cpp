// fpga_counter_base - FPGA 柜台公共基类实现
// 注意：基类不持有 get_counter_type()（这是派生类的语义），所有需要
// counter_type 的派生类在构造 StreamInfo 时自行提供.

// 注意: api_errno.h 必须在 fpga_counter_base.h 之前包含,
// 因为 callback_manager.h -> api_callback.h 依赖 err_event_type.
#include "fpga_counter_base.h"
#include "api_config_impl.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "hash_map_mth.h"
#include "matomic.h"
#include "mlog.h"
#include "mutils.h"
#include "order_trade_type.h"

#include <cstring>

namespace lb_api {

// 构造函数: 初始化所有公共字段 (顺序与头文件声明顺序保持一致)
fpga_counter_base::fpga_counter_base()
    : trade_link_connect(0), sec_state_(0), market_type(0), cb_mgr_(nullptr), session_seq_(0), log_(nullptr) {}

fpga_counter_base::~fpga_counter_base() = default;

// init_base: 派生类 init 时调用, 初始化回调 / 日志 / 市场类型
int32 fpga_counter_base::init_base(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log) {
  sec_state_ = 0; ///< 证券信息获取状态:0-未进行，1-进行中，2-成功
  trade_link_connect = 0;
  market_type = static_cast<int16>(cfg.get_market_type());           ///< 市场
  heart_interval = static_cast<int16>(cfg.get_heartbeat_interval()); ///< 心跳间隔
  cb_mgr_ = cb;                                                      ///< 回调管理器
  session_seq_ = 0;                                                  ///< 会话消息序号 (原子访问)
  log_ = log;                                                        ///< 日志指针

  lb_common::lb_log_hand tlh(log_);
  int32 ret = sec_map_.init(16384);
  if (ret < 0) {
    info_log(tlh) << "fpga_counter_base init sec map error, ret=" << ret << end_log;
    return LBAPI_ERR_ALLOC_MEM;
  }

  info_log(tlh) << "fpga_counter_base init ok, market_type=" << market_type << end_log;
  return 0;
}

// ---- get_sec_index: 查询 sec_index (由派生类在 deal_order_req 中调用) ----
int32 fpga_counter_base::get_sec_index(const char security_id[8], uint16_t &sec_index) {
  fpga_sec_key key(security_id);
  int32 ret = sec_map_.find(sec_index, key);
  if (unlikely(ret < 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "not find sec code, security_id=" << security_id << ",ret=" << ret << end_log;
    return LBAPI_ERR_NO_SEC;
  }
  return 0; // 证券未找到
}

// ---- build_order_msg: 构造 g1 委托消息 (头 + order_req) ----
void fpga_counter_base::build_order_msg(const OrderReq &req, const fpga_cust_info &cust, uint16 sec_index,
                                        g1_msg_head *o_req) {
  // ---- 填充消息头 ----
  o_req->msg_id = G1_MSG_ORDER_REQ;
  o_req->msg_len = sizeof(order_req);
  o_req->user_id = cust.user_id;
  o_req->board_no = cust.board_no;
  o_req->session_id = cust.session_id;

  // ---- 填充消息体 (order_req) ----
  order_req *body = reinterpret_cast<order_req *>(o_req + 1);
  body->user_id = cust.user_id;
  body->board_no = cust.board_no;
  body->sec_index = sec_index;
  body->side = req.side;
  body->order_type = req.order_type;
  body->order_price = req.order_price;
  body->order_qty = req.order_qty;
  body->cust_req_no = req.client_seq_id;
  body->stop_price = req.stop_price;
  body->tgw_id = req.tgw_id;
  body->policy_id = req.policy_id;
  body->order_way[0] = cust.order_way_ext[0];
  body->order_way[1] = cust.order_way_ext[1];
  body->reserve = 0;
}

// ---- build_cancel_msg: 构造 g1 撤单消息 (头 + cancel_req) ----
void fpga_counter_base::build_cancel_msg(const CancelReq &req, const fpga_cust_info &cust, g1_msg_head *o_req) {
  // ---- 填充消息头 ----
  o_req->msg_id = G1_MSG_CANCEL_REQ;
  o_req->msg_len = sizeof(cancel_req);
  o_req->user_id = cust.user_id;
  o_req->board_no = cust.board_no;
  o_req->session_id = cust.session_id;

  // ---- 填充消息体 (cancel_req) ----
  cancel_req *body = reinterpret_cast<cancel_req *>(o_req + 1);
  body->cust_req_no = req.client_req_no;
  body->order_sys_no = req.order_sys_no;
  body->user_id = cust.user_id;
  body->board_no = cust.board_no;
  body->reserve = 0;
  body->org_cust_req_no = req.client_seq_id;
}

// ---- build_api_order_rej: 从原始 g1 委托消息构建 API 层委托拒绝回报 ----
// 调用场景: 委托消息 send_error 后, 引擎用此函数构造一个 OrderRtn 通知客户拒绝
void fpga_counter_base::build_api_order_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 err_code,
                                            OrderRtn &o_rtn, StreamInfo &o_stream) {
  std::memset(&o_rtn, 0, sizeof(o_rtn));
  const order_req *body = reinterpret_cast<const order_req *>(msg + 1);

  std::memcpy(o_rtn.cust_id.data(), cust.cust_id, sizeof(o_rtn.cust_id));
  std::memcpy(o_rtn.fund_account_id.data(), cust.fund_account_id, sizeof(o_rtn.fund_account_id));
  std::memcpy(o_rtn.account_id.data(), cust.holder_acc, sizeof(o_rtn.account_id));
  std::memcpy(o_rtn.branch_id.data(), cust.branch_id, sizeof(o_rtn.branch_id));

  o_rtn.side = body->side;
  o_rtn.order_type = body->order_type;
  // todo : 依据委托状态字典变动而修改
  o_rtn.order_status = static_cast<uint16>(ORDER_STATE_DISCARD);

  o_rtn.policy_id = body->policy_id;
  o_rtn.market_type = market_type;
  o_rtn.reserved = 0;
  o_rtn.order_price = body->order_price;
  o_rtn.order_qty = body->order_qty;
  o_rtn.client_seq_id = body->cust_req_no;
  // sec_index -> security_id
  if (body->sec_index < secs_.size()) {
    const fpga_sec_info &si = secs_[body->sec_index];
    std::memcpy(o_rtn.security_id.data(), si.security_id, sizeof(o_rtn.security_id));
  }

  // todo : 依据回报类型字典变动而修改
  o_rtn.rtn_type = RSP_TYPE_ORDER_DISCARD;

  o_rtn.err_code = err_code;

  // ---- 流信息 ----
  o_stream.counter_type = STREAM_API_COUNTER;
  o_stream.stream_seq = 0;
}

// ---- build_api_cancel_rej: 从原始 g1 撤单消息构建 API 层撤单拒绝响应 ----
// 调用场景: 撤单消息 send_error 后, 引擎用此函数构造 CancelRsp 通知客户拒绝
void fpga_counter_base::build_api_cancel_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 err_code,
                                             CancelRsp &o_rtn, StreamInfo &o_stream) {
  std::memset(&o_rtn, 0, sizeof(o_rtn));
  const cancel_req *body = reinterpret_cast<const cancel_req *>(msg + 1);

  o_rtn.client_req_no = body->cust_req_no;

  // ---- 从 cust 取客户标识 (fund_account_id / account_id / branch_id) ----
  std::memcpy(o_rtn.cust_id.data(), cust.cust_id, sizeof(o_rtn.cust_id));
  std::memcpy(o_rtn.fund_account_id.data(), cust.fund_account_id, sizeof(o_rtn.fund_account_id));
  std::memcpy(o_rtn.account_id.data(), cust.holder_acc, sizeof(o_rtn.account_id));
  std::memcpy(o_rtn.branch_id.data(), cust.branch_id, sizeof(o_rtn.branch_id));

  o_rtn.market_type = market_type;
  o_rtn.order_sys_no = body->order_sys_no;
  o_rtn.client_seq_id = body->org_cust_req_no;
  o_rtn.err_code = err_code;
  o_rtn.rej_api = 1; // 1 = api 层拒绝 (队列满 / 发送失败等场景)

  // ---- 流信息 ----
  o_stream.counter_type = STREAM_API_COUNTER;
  o_stream.stream_seq = 0;
}

// ---- deal_order_rtn: 处理委托回报 (派发到 callback_manager) ----
void fpga_counter_base::deal_order_rtn(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type) {
  if (unlikely(msg->msg_len < sizeof(order_rtn))) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "order rtn msg len too short, msg_len=" << msg->msg_len << ", need=" << sizeof(order_rtn)
                   << end_log;
    return;
  }

  // 依据 body.session_seq_no 做可靠消息去重 (>0 表示可靠消息, 0 表示非可靠)
  const order_rtn *rtn = reinterpret_cast<const order_rtn *>(msg + 1);
  if (rtn->session_seq_no > 0) {
    if (unlikely(rtn->session_seq_no <= session_seq_)) {
      return;
    }
    session_seq_ = rtn->session_seq_no;
  }

  // 流消息号 (D29/D35): 同步后台可靠流水到 stream_seq
  StreamInfo si_out;
  si_out.counter_type = counter_type;
  si_out.stream_seq = rtn->session_seq_no;

  alignas(32) OrderRtn out;
  // ---- 从 cust 取客户标识 (fund_account_id / account_id / branch_id) ----
  std::memcpy(out.cust_id.data(), cust.cust_id, sizeof(out.cust_id));
  std::memcpy(out.fund_account_id.data(), cust.fund_account_id, sizeof(out.fund_account_id));
  std::memcpy(out.account_id.data(), cust.holder_acc, sizeof(out.account_id));
  std::memcpy(out.branch_id.data(), cust.branch_id, sizeof(out.branch_id));

  out.side = rtn->side;
  out.order_type = rtn->order_type;
  out.policy_id = rtn->policy_id;
  out.order_status = static_cast<uint16>(rtn->order_status);
  out.market_type = market_type;
  out.reserved = 0;
  // sec_index -> security_id
  if (rtn->sec_index < secs_.size()) {
    const fpga_sec_info &si = secs_[rtn->sec_index];
    std::memcpy(out.security_id.data(), si.security_id, sizeof(out.security_id));
  } else {
    std::memset(out.security_id.data(), 0, sizeof(out.security_id));
  }
  out.order_price = rtn->order_price;
  out.order_qty = rtn->order_qty;
  out.client_seq_id = rtn->cust_req_no;
  out.rtn_type = static_cast<int32_t>(rtn->rtn_type);
  out.err_code = rtn->err_code;
  out.order_sys_no = rtn->order_sys_no;
  out.frozen_amount = rtn->frozen_amount;
  out.fee = rtn->fee;
  out.trade_qty = rtn->trade_qty;
  out.cancel_qty = rtn->cancel_qty;
  out.order_time = rtn->order_time;
  out.update_time = rtn->update_time;

  cb_mgr_->on_order_rtn(si_out, out);
}

// ---- deal_trade_rtn: 处理成交回报 ----
void fpga_counter_base::deal_trade_rtn(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type) {
  if (unlikely(msg->msg_len < sizeof(trade_rtn))) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "trade rtn msg len too short, msg_len=" << msg->msg_len << ", need=" << sizeof(trade_rtn)
                   << end_log;
    return;
  }

  // 依据 body.session_seq_no 做可靠消息去重 (>0 表示可靠消息, 0 表示非可靠)
  const trade_rtn *rtn = reinterpret_cast<const trade_rtn *>(msg + 1);
  if (rtn->session_seq_no > 0) {
    if (unlikely(rtn->session_seq_no <= session_seq_)) {
      return;
    }
    session_seq_ = rtn->session_seq_no;
  }

  StreamInfo si_out;
  si_out.counter_type = counter_type;
  si_out.stream_seq = rtn->session_seq_no;

  alignas(32) TradeRtn out;
  std::memcpy(out.cust_id.data(), cust.cust_id, sizeof(out.cust_id));
  std::memcpy(out.fund_account_id.data(), cust.fund_account_id, sizeof(out.fund_account_id));
  std::memcpy(out.account_id.data(), cust.holder_acc, sizeof(out.account_id));
  std::memcpy(out.branch_id.data(), cust.branch_id, sizeof(out.branch_id));

  out.side = rtn->side;
  out.order_type = rtn->order_type;
  out.policy_id = rtn->policy_id;
  out.order_status = static_cast<int32_t>(rtn->order_status);
  out.market_type = market_type;
  out.reserved = 0;
  if (rtn->sec_index < secs_.size()) {
    const fpga_sec_info &si = secs_[rtn->sec_index];
    std::memcpy(out.security_id.data(), si.security_id, sizeof(out.security_id));
  } else {
    std::memset(out.security_id.data(), 0, sizeof(out.security_id));
  }
  out.order_price = rtn->order_price;
  out.order_qty = rtn->order_qty;
  out.client_seq_id = rtn->cust_req_no;
  out.order_sys_no = rtn->order_sys_no;
  out.frozen_amount = rtn->frozen_amount;
  out.fee = rtn->fee;
  out.trade_qty = rtn->trade_qty;
  out.cancel_qty = rtn->cancel_qty;
  out.order_time = rtn->order_time;
  out.exec_time = rtn->exec_time;
  lb_common::comm_utils::str_copy_format(out.exec_id.data(), rtn->exec_id, sizeof(out.exec_id));
  out.exec_price = rtn->exec_price;
  out.exec_qty = rtn->exec_qty;
  out.exec_amount = rtn->exec_amount;
  out.exec_fee = rtn->exec_fee;

  cb_mgr_->on_trade_rtn(si_out, out);
}

void fpga_counter_base::deal_cancel_rsp(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type) {
  if (unlikely(msg->msg_len < sizeof(cancel_rsp))) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cancel rsp msg len too short, msg_len=" << msg->msg_len << ", need=" << sizeof(trade_rtn)
                   << end_log;
    return;
  }

  // 依据 body.session_seq_no 做可靠消息去重
  const cancel_rsp *rtn = reinterpret_cast<const cancel_rsp *>(msg + 1);
  if (rtn->session_seq_no > 0) {
    if (unlikely(rtn->session_seq_no <= session_seq_)) {
      return;
    }
    session_seq_ = rtn->session_seq_no;
  }

  StreamInfo si_out;
  si_out.counter_type = counter_type;
  si_out.stream_seq = rtn->session_seq_no;

  alignas(32) CancelRsp out;

  out.client_req_no = rtn->cust_req_no; ///< 客户请求号
  std::memcpy(out.cust_id.data(), cust.cust_id, sizeof(out.cust_id));
  std::memcpy(out.fund_account_id.data(), cust.fund_account_id, sizeof(out.fund_account_id));
  std::memcpy(out.account_id.data(), cust.holder_acc, sizeof(out.account_id));
  std::memcpy(out.branch_id.data(), cust.branch_id, sizeof(out.branch_id));

  out.market_type = market_type;
  out.order_sys_no = rtn->order_sys_no;
  out.client_seq_id = rtn->org_cust_req_no;
  out.err_code = rtn->err_code;
  out.rej_api = 0;

  cb_mgr_->on_cancel_rsp(si_out, out);
}

// ---- deal_gw_rej: 处理网关路由拒绝 (G1_MSG_GW_REJ) ----
// 消息结构: g1_msg_head + g1_gw_rej_head + 被拒绝的请求消息
// 被拒绝的请求只可能是 order_req 或 cancel_req (不含 login_req / sec_info_req)
// void fpga_counter_base::deal_gw_rej(...)

// ---- delive_cust_login: 重新投递指定客户的登录事件 (v2.1 GW 链接重连后自动重登) ----
int32 fpga_counter_base::delive_cust_login(fpga_cust_info &cust, lb_common::que_mth_buf *que,
                                           link_engine_outop *link_op) {
  if (que == nullptr || link_op == nullptr) {
    return LBAPI_ERR_STATE_LIMITED;
  }

  lb_common::lb_log_hand tlh(log_);
  int32 evt_len = sizeof(link_send_event) + sizeof(acc_login_event_info);
  char *data = nullptr;
  int64 pos = que->write_get_mth(data, evt_len);
  if (pos <= 0) {
    error_log(tlh) << "delive re-login event to queue error,branch_id=" << cust.branch_id
                   << ",fund_account=" << cust.fund_account_id << ",ret=" << pos << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  cust.login_state = 1;

  link_send_event *evt = reinterpret_cast<link_send_event *>(data);
  evt->link_type = LINK_TYPE_SPEED_GW;
  evt->type = LINK_EVENT_TYPE_ACCOUNT_LOGIN;
  evt->data_len = sizeof(acc_login_event_info);
  acc_login_event_info *info = reinterpret_cast<acc_login_event_info *>(evt + 1);
  build_login_event(cust, *info);

  que->write_cmt_mth(pos, evt_len);
  link_op->trigger_send();

  info_log(tlh) << "delive re-login event ok, branch_id=" << cust.branch_id << ",fund_account=" << cust.fund_account_id
                << end_log;
  return LBAPI_OK;
}
void fpga_counter_base::deal_gw_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type) {
  lb_common::lb_log_hand tlh(log_);

  // 长度校验: g1_gw_rej_head + (order_req or cancel_req) 的最小长度
  int32_t min_body = static_cast<int32_t>(sizeof(g1_gw_rej_head)) + static_cast<int32_t>(sizeof(order_req));
  if (static_cast<int32_t>(msg->msg_len) < min_body) {
    error_log(tlh) << "gw_rej msg len too short, msg_len=" << msg->msg_len << ", min=" << min_body << end_log;
    return;
  }

  const g1_gw_rej_head *rej = reinterpret_cast<const g1_gw_rej_head *>(msg + 1);
  const char *body = reinterpret_cast<const char *>(rej + 1);

  info_log(tlh) << "recv gw route reject, rej_msg_id=" << rej->rej_msg_id << ", err_code=" << rej->err_code
                << ", err_msg=" << rej->err_msg << end_log;

  // 依据被拒绝的消息号分发
  if (rej->rej_msg_id == G1_MSG_ORDER_REQ) {
    OrderRtn rtn;
    StreamInfo stream;
    build_api_gw_rej_order(reinterpret_cast<const order_req *>(body), rej, cust, counter_type, rtn, stream);
    cb_mgr_->on_order_rtn(stream, rtn);
  } else if (rej->rej_msg_id == G1_MSG_CANCEL_REQ) {
    CancelRsp rsp;
    StreamInfo stream;
    build_api_gw_rej_cancel(reinterpret_cast<const cancel_req *>(body), rej, cust, counter_type, rsp, stream);
    cb_mgr_->on_cancel_rsp(stream, rsp);
  } else {
    // 未知被拒消息号, 记录但不影响其他
    // (login_req / sec_info_req 不会走此路径, 仅做兜底)
    error_log(tlh) << "gw_rej unknown rej_msg_id=" << rej->rej_msg_id << ", ignore" << end_log;
  }
}

// ---- build_api_gw_rej_order: 从 order_req + rej head 构造 OrderRtn ----
void fpga_counter_base::build_api_gw_rej_order(const order_req *body, const g1_gw_rej_head *rej,
                                               const fpga_cust_info &cust, int32 counter_type, OrderRtn &o_rtn,
                                               StreamInfo &o_stream) {
  std::memset(&o_rtn, 0, sizeof(o_rtn));

  // ---- 从 cust 取客户标识 ----
  std::memcpy(o_rtn.cust_id.data(), cust.cust_id, sizeof(o_rtn.cust_id));
  std::memcpy(o_rtn.fund_account_id.data(), cust.fund_account_id, sizeof(o_rtn.fund_account_id));
  std::memcpy(o_rtn.account_id.data(), cust.holder_acc, sizeof(o_rtn.account_id));
  std::memcpy(o_rtn.branch_id.data(), cust.branch_id, sizeof(o_rtn.branch_id));

  // ---- 从 order_req 拷贝委托信息 ----
  o_rtn.side = body->side;
  o_rtn.order_type = body->order_type;
  o_rtn.policy_id = body->policy_id;
  o_rtn.order_status = ORDER_STATE_DISCARD; ///< GW 拒单 = 委托废单
  o_rtn.market_type = market_type;
  o_rtn.reserved = 0;

  // sec_index -> security_id (GW 拒绝时 sec_index 通常仍有效)
  if (body->sec_index < secs_.size()) {
    const fpga_sec_info &si = secs_[body->sec_index];
    std::memcpy(o_rtn.security_id.data(), si.security_id, sizeof(o_rtn.security_id));
  } else {
    std::memset(o_rtn.security_id.data(), 0, sizeof(o_rtn.security_id));
  }

  o_rtn.order_price = body->order_price;
  o_rtn.order_qty = body->order_qty;
  o_rtn.client_seq_id = body->cust_req_no;
  o_rtn.rtn_type = RSP_TYPE_ORDER_DISCARD;
  o_rtn.err_code = rej->err_code;

  // ---- 流信息: GW 拒单非流式消息, stream_seq=0 ----
  o_stream.counter_type = counter_type;
  o_stream.stream_seq = 0;
}

// ---- build_api_gw_rej_cancel: 从 cancel_req + rej head 构造 CancelRsp ----
void fpga_counter_base::build_api_gw_rej_cancel(const cancel_req *body, const g1_gw_rej_head *rej,
                                                const fpga_cust_info &cust, int32 counter_type, CancelRsp &o_rsp,
                                                StreamInfo &o_stream) {
  std::memset(&o_rsp, 0, sizeof(o_rsp));

  o_rsp.client_req_no = body->cust_req_no;

  // ---- 从 cust 取客户标识 ----
  std::memcpy(o_rsp.cust_id.data(), cust.cust_id, sizeof(o_rsp.cust_id));
  std::memcpy(o_rsp.fund_account_id.data(), cust.fund_account_id, sizeof(o_rsp.fund_account_id));
  std::memcpy(o_rsp.account_id.data(), cust.holder_acc, sizeof(o_rsp.account_id));
  std::memcpy(o_rsp.branch_id.data(), cust.branch_id, sizeof(o_rsp.branch_id));

  o_rsp.market_type = market_type;
  o_rsp.order_sys_no = body->order_sys_no;
  o_rsp.client_seq_id = body->org_cust_req_no;
  o_rsp.err_code = rej->err_code;
  o_rsp.rej_api = 0; ///< GW 拒绝, 非 API 层拒绝

  // ---- 流信息 ----
  o_stream.counter_type = counter_type;
  o_stream.stream_seq = 0;
}

// ---- build_sec_info_req_msg: 构造证券信息请求 (头 + sec_info_req) ----
// 注: 证券信息请求是系统级请求, 头中不携带 customer session_id (用 0)
void fpga_counter_base::build_sec_info_req_msg(int64 cust_req_no, g1_msg_head *o_req) {
  o_req->msg_id = G1_MSG_SEC_INFO_REQ;
  o_req->msg_len = sizeof(sec_info_req);
  o_req->user_id = 0;
  o_req->board_no = 0;
  o_req->session_id = 0;

  sec_info_req *body = reinterpret_cast<sec_info_req *>(o_req + 1);
  body->cust_req_no = cust_req_no;
}

// ---- deal_sec_info_ans: 处理证券信息推送应答 ----
int32 fpga_counter_base::deal_sec_info_ans(const g1_msg_head *msg) {
  lb_common::lb_log_hand tlh(log_);

  int32_t msg_len = static_cast<int32_t>(msg->msg_len);
  if (msg_len < (int32_t)sizeof(sec_push_head)) {
    error_log(tlh) << "sec_info_ans msg len too short, msg_len=" << msg_len << end_log;
    sec_state_ = 0;
    cb_mgr_->on_error(err_event_type::get_sec_info, LBAPI_ERR_MSG_LEN, "recv sec ans msg len too short");
    return LBAPI_ERR_MSG_LEN;
  }

  const sec_push_head *head = reinterpret_cast<const sec_push_head *>(msg + 1);
  int32_t total = static_cast<int32_t>(head->total_num);
  int32_t cur = static_cast<int32_t>(head->cur_num);

  if (head->err_code == 0) {
    info_log(tlh) << "recv fpga sec_info_ans, total_num=" << total << ", cur_num=" << cur << end_log;
  } else {
    sec_state_ = 0;
    error_log(tlh) << "fpga sec_info_ans error, err_code=" << head->err_code << ", total_num=" << total
                   << ", cur_num=" << cur << end_log;
    cb_mgr_->on_error(err_event_type::get_sec_info, LBAPI_ERR_NO_SEC, "recv ans sec  msg failed");
    return LBAPI_ERR_NO_SEC;
  }

  int32_t need_len = cur * (int32_t)sizeof(sec_push_info) + (int32_t)(sizeof(sec_push_head));
  if (need_len != msg_len) {
    info_log(tlh) << "fpga sec_info_ans len error, need_len=" << need_len << ", msg_len=" << msg_len << end_log;
    sec_state_ = 0;
    cb_mgr_->on_error(err_event_type::get_sec_info, LBAPI_ERR_MSG_LEN, "recv sec ans msg len not match");

    return LBAPI_ERR_MSG_LEN;
  }

  if (total >= (int32_t)(secs_.size())) {
    secs_.resize(total + 1);
  }

  const sec_push_info *info = reinterpret_cast<const sec_push_info *>(head + 1);

  for (int32_t i = 0; i < cur; ++i) {
    fpga_sec_info sec;
    if (info->market_type != market_type || info->security_id[0] == '\0' || info->sec_index == 0)
      continue;

    lb_common::comm_utils::str_copy_format(sec.security_id, info->security_id, sizeof(sec.security_id));
    sec.market_type = info->market_type;
    sec.sec_index = info->sec_index;
    sec.buy_qty_unit = info->buy_qty_unit;

    info_log(tlh) << "recv fpga secinfo, market_type=" << sec.market_type << ", security_id=" << sec.security_id
                  << ", sec_index=" << sec.sec_index << ", buy_qty_unit=" << sec.buy_qty_unit << end_log;

    // 扩容 secs_ 到容纳 sec_index
    if (sec.sec_index >= secs_.size()) {
      secs_.resize(sec.sec_index + 1);
    }
    secs_[sec.sec_index] = sec;

    // 更新 sec_map_
    fpga_sec_key key(sec.security_id);
    uint16_t old_index = 0;
    if (sec_map_.find(old_index, key) != 0) {
      sec_map_.insert_new(sec.sec_index, key);
    }

    info++;
  }

  // ---- 全部接收完成 ----
  if (cur < total) {
    return 0;
  } else {
    sec_state_ = 2;
    info_log(tlh) << "fpga sec_info receive complete, total secs=" << secs_.size() << end_log;
    cb_mgr_->on_error(err_event_type::get_sec_info, 0, "recv sec info complete");
    return 1;
  }
}

// ---- build_login_msg: 构造 fpga 账户登录消息 (头 + login_req) ----
void fpga_counter_base::build_login_msg(const acc_login_event_info &info, int16_t log_type, g1_msg_head *o_req) {
  o_req->msg_id = G1_MSG_LOGIN_REQ;
  o_req->msg_len = sizeof(login_req);
  o_req->user_id = 0;
  o_req->board_no = 0;
  o_req->session_id = 0; // 登录请求自身不携带 session_id, 登录成功后从应答回填

  login_req *body = reinterpret_cast<login_req *>(o_req + 1);
  body->cust_req_no = info.cust_req_no;
  std::memcpy(body->cust_id, info.cust_id, sizeof(body->cust_id));
  std::memcpy(body->fund_account_id, info.fund_account_id, sizeof(body->fund_account_id));
  std::memset(body->branch_id, 0, sizeof(body->branch_id));
  std::memcpy(body->branch_id, info.branch_id, std::min<int32>(sizeof(body->branch_id), sizeof(info.branch_id)));
  std::memcpy(body->holder_acc, info.account_id, sizeof(body->holder_acc));
  std::memcpy(body->session, info.session, sizeof(body->session));
  std::memcpy(body->end_code, info.client_feature_code, sizeof(body->end_code));
  body->market_type = market_type;
  body->heart_bt_int = static_cast<int32_t>(heart_interval);
  body->order_way[0] = info.order_way_ext[0];
  body->order_way[1] = info.order_way_ext[1];
  body->log_type = log_type;
  body->req_connect_id = 0; // API 侧设为 0, 网关填写
  std::memset(body->version, 0, sizeof(body->version));
  std::strncpy(body->version, g1_msg_ver, strlen(g1_msg_ver));
}

void fpga_counter_base::build_login_event(fpga_cust_info &cust, acc_login_event_info &o_info) {
  // todo : 依据正式协议重写
  o_info.cust_req_no = 0; //cust.cust_req_no;
  std::memcpy(o_info.cust_id, cust.cust_id, sizeof(o_info.cust_id));
  std::memcpy(o_info.fund_account_id, cust.fund_account_id, sizeof(o_info.fund_account_id));
  std::memcpy(o_info.account_id, cust.holder_acc, sizeof(o_info.account_id));
  std::memcpy(o_info.branch_id, cust.branch_id, sizeof(o_info.branch_id));

  o_info.order_way_ext[0] = cust.order_way_ext[0];
  o_info.order_way_ext[1] = cust.order_way_ext[1];
  std::memcpy(o_info.session, agw_user, sizeof(o_info.session));
  std::memset(o_info.password, 0, sizeof(o_info.password));
  std::memset(o_info.user_info, 0, sizeof(o_info.user_info));
  std::memcpy(o_info.client_feature_code, cust.end_code, sizeof(o_info.client_feature_code));
}

void fpga_counter_base::build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg,
                                        LoginAns &ans) {
  //std::memset(&ans, 0, sizeof(ans));
  ans.client_req_no = info.cust_req_no;
  std::memcpy(ans.cust_id.data(), info.cust_id, sizeof(ans.cust_id));
  std::memcpy(ans.fund_account_id.data(), info.fund_account_id, sizeof(ans.fund_account_id));
  std::memcpy(ans.account_id.data(), info.account_id, sizeof(ans.account_id));
  std::memcpy(ans.branch_id.data(), info.branch_id, sizeof(ans.branch_id));
  ans.market_type = market_type;
  ans.err_code = err_ret;
  if (err_msg != nullptr) {
    lb_common::comm_utils::str_copy_format(ans.err_msg.data(), err_msg, sizeof(ans.err_msg));
  } else {
    std::memset(ans.err_msg.data(), 0, ans.err_msg.size());
  }
  ans.login_time = 0; //lb_common::comm_utils::get_time_ms();
}

// ---- build_login_rtn (login_ans 重载): 从 fpga 协议 login_ans 构建 API 层 LoginAns ----
// 调用场景: 派生类 deal_log_ans 收到 fpga login_ans 应答, 用此函数
// 转换为 API LoginAns 后调用 cb_mgr_->on_login(o_ans)
void fpga_counter_base::build_login_rtn(const login_ans &msg, LoginAns &o_ans) {
  std::memset(&o_ans, 0, sizeof(o_ans));
  o_ans.client_req_no = msg.cust_req_no;
  lb_common::comm_utils::str_copy_format(o_ans.cust_id.data(), msg.cust_id, sizeof(o_ans.cust_id));
  lb_common::comm_utils::str_copy_format(o_ans.fund_account_id.data(), msg.fund_account_id,
                                         sizeof(o_ans.fund_account_id));
  lb_common::comm_utils::str_copy_format(o_ans.account_id.data(), msg.holder_acc,
                                         std::min<int32>(sizeof(o_ans.account_id), sizeof(msg.holder_acc)));
  lb_common::comm_utils::str_copy_format(o_ans.branch_id.data(), msg.branch_id,
                                         std::min<int32>(sizeof(o_ans.branch_id), sizeof(msg.branch_id)));

  o_ans.market_type = market_type;
  o_ans.err_code = msg.err_code;
  // err_msg: G1_ERRMSG_LEN=64 -> LoginAns.err_msg=124, 截断到协议侧长度 (含 \0)
  if (msg.err_msg[0] != '\0') {
    int32 tlen = static_cast<int32>(strnlen(msg.err_msg, sizeof(msg.err_msg)));
    tlen = tlen < static_cast<int32>(sizeof(o_ans.err_msg)) ? tlen : static_cast<int32>(sizeof(o_ans.err_msg)) - 1;
    std::memcpy(o_ans.err_msg.data(), msg.err_msg, tlen);
  }
  o_ans.login_time = msg.login_time;
}

// ---- build_login_rtn (fpga_cust_info 重载): 从本地 fpga_cust_info 构建 API 层 LoginAns ----
// 调用场景:
//   1. fpga core 链接建立成功回调 (fpga_direct::ans_cust_login(fpga_core_connect_info, 0, ...))
//   2. fpga 柜台自身 fail 路径: 仅持有本地 cust 而无 fpga login_ans 协议消息时
void fpga_counter_base::build_login_rtn(const fpga_cust_info &cust, int32 err_ret, const char *err_msg,
                                        LoginAns &o_ans) {
  std::memset(&o_ans, 0, sizeof(o_ans));

  o_ans.client_req_no = cust.cust_req_no;
  std::memcpy(o_ans.cust_id.data(), cust.cust_id, sizeof(o_ans.cust_id));
  std::memcpy(o_ans.fund_account_id.data(), cust.fund_account_id, sizeof(o_ans.fund_account_id));
  std::memcpy(o_ans.account_id.data(), cust.holder_acc,
              std::min<int32>(sizeof(o_ans.account_id), sizeof(cust.holder_acc)));
  std::memcpy(o_ans.branch_id.data(), cust.branch_id, std::min<int32>(sizeof(o_ans.branch_id), sizeof(cust.branch_id)));

  o_ans.market_type = static_cast<int16_t>(market_type);
  o_ans.err_code = err_ret; // 默认成功 (失败路径由调用方覆写)
  if (err_msg != nullptr) {
    lb_common::comm_utils::str_copy_format(o_ans.err_msg.data(), err_msg, sizeof(o_ans.err_msg));
  }
  o_ans.login_time = lb_common::comm_utils::get_time_ms(); //0;
}

void fpga_counter_base::save_client_info(const login_ans &msg, fpga_cust_info &o_cust) {
  o_cust.user_id = msg.user_id;
  o_cust.board_no = msg.board_no;
  // 从 login_ans 回填 session_id 到客户结构 (v2.1: per-customer)
  o_cust.session_id = msg.session_id;
  o_cust.order_way_ext[0] = msg.order_way[0]; ///< 客户委托方式
  o_cust.order_way_ext[1] = msg.order_way[1]; ///< 客户委托方式
  o_cust.trade_port = msg.trade_port;
  lb_common::comm_utils::str_copy_format(o_cust.trade_ip, msg.trade_ip, sizeof(o_cust.trade_ip));
  lb_common::comm_utils::str_copy_format(o_cust.cust_id, msg.cust_id, sizeof(o_cust.cust_id));
  lb_common::comm_utils::str_copy_format(o_cust.fund_account_id, msg.fund_account_id, sizeof(o_cust.fund_account_id));
  lb_common::comm_utils::str_copy_format(o_cust.branch_id, msg.branch_id, sizeof(o_cust.branch_id));
  lb_common::comm_utils::str_copy_format(o_cust.holder_acc, msg.holder_acc, sizeof(o_cust.holder_acc));
  std::memcpy(o_cust.end_code, msg.end_code, sizeof(o_cust.end_code));
  o_cust.cust_req_no = msg.cust_req_no;
  atomic_seq_fence();
  o_cust.fpga_state = 1; ///< 该客户在fpga中状态
  o_cust.login_state = 2;
}

// ---- build_heart_msg: 构造心跳消息 (仅头) ----
// 注: 心跳是系统级消息, 头中不携带 customer session_id (用 0)
int32 fpga_counter_base::build_heart_msg(char *o_buf, int32 buf_len) {
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
  head->msg_id = G1_MSG_HEART_REQ;
  head->msg_len = 0;
  head->user_id = 0;
  head->board_no = 0;
  head->session_id = 0;
  return static_cast<int32>(sizeof(g1_msg_head));
}

} // namespace lb_api
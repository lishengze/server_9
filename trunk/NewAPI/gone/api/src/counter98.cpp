<<<<<<< .mine
// counter98 - 98 柜台协议对象实现
//
// 负责 98 柜台的消息组包 / 解析 / 状态管理 / 查询应答处理
// 业务发送函数内部完成组包 + 入队 (D33: 队列满立即返回错误)

#include "counter98.h"
#include "api_config.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "c98msg_tmp.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "matomic.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

// 类型实例化 (api_instance.cpp 通过模板调用本类, 此处仅做符号实例化)
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "gw_counter_direct.h"

#include <cstring>
#include <unistd.h>

namespace lb_api {

// ---- 构造 / 析构 ----
counter98::counter98()
    : agw_login_state(0), market_type(0), gw_send_queue_(nullptr), trade_send_queue_(nullptr),
      agw_user_login_timeout_(10), cb_mgr_(nullptr), session_seq_(0), log_(nullptr) {
  std::memset(agw_session, 0, sizeof(agw_session));
}

counter98::~counter98() = default;

// init
int32 counter98::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log) {
  market_type = static_cast<int16>(cfg.get_market_type());           ///< 市场
  heart_interval = static_cast<int16>(cfg.get_heartbeat_interval()); ///< 心跳间隔
  fast_counter_type_ = cfg.get_fast_counter_type();                  ///< 极速柜台类型
  agw_login_state = 0;     ///< agw 用户登陆状态:0-未登陆，1-登陆中，2-登陆成功
  trade_link_connect_ = 0; ///< 98 链接状态:0-未链接/断开, 1-已链接
  agw_user_login_timeout_ = cfg.get_agw_user_login_timeout(); ///< AGW 登录超时秒数（默认 10）
  if (agw_user_login_timeout_ < 5)
    agw_user_login_timeout_ = 5;
  // ---- 队列与回调 ----
  gw_send_queue_ = nullptr;    ///< 98柜台发送队列
  trade_send_queue_ = nullptr; ///< 极速柜台发送队列
  cb_mgr_ = cb;
  session_seq_ = 0;
  log_ = log;
  gw_eng_op_ = nullptr;    ///< 98链接引擎导出的链接相关操作
  trade_eng_op_ = nullptr; ///< 极速引擎导出的链接相关操作

  std::memset(agw_session, 0, sizeof(agw_session));
  agw_user_ = cfg.get_agw98_user();
  agw_password_ = cfg.get_agw98_user_password();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "init counter 98 ok,market_type=" << market_type << ",agw_user=" << cfg.get_agw98_user()
                << ",agw_user_login_timeout=" << agw_user_login_timeout_
                << ",fast_counter_type=" << static_cast<int32_t>(fast_counter_type_)
                << ",heart_interval=" << heart_interval << end_log;

  return 0;
}

// deal_order_req: 买卖委托 (D22: 降级目标)
int32 counter98::deal_order_req(const OrderReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_req: 98 link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_req: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  // todo : 需要依据正式98协议，重新实现消息构造和入队
  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_order_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_order_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}

// deal_etf_order_req: ETF 申购赎回 (98 模式下走 c98 委托)
int32 counter98::deal_etf_order_req(const OrderReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_etf_order_req: 98 link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_etf_order_req: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  // todo : 需要依据正式98协议，重新实现消息构造和入队
  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_order_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_etf_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_etf_order_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}

// deal_bse_order_req: BSE 永远走 98
int32 counter98::deal_bse_order_req(const OrderReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_bse_order_req: 98 link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_bse_order_req: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  // todo : 需要依据正式98协议，重新实现消息构造和入队
  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_order_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_bse_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_bse_order_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}

// deal_cancel_req: 撤单 (D22 降级目标)
int32_t counter98::deal_cancel_req(const CancelReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cancel_req: 98 link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cancel_req: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  // todo : 需要依据正式98协议，重新实现消息构造和入队
  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_cancel_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cancel_req: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_cancel_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}

// deal_order_query: 委托查询
int32 counter98::deal_order_query(const OrderQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_query: 98 link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_query: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_query_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_order_query_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}
// deal_order_batch_query: 委托批量查询
int32 counter98::deal_order_batch_query(const OrderBatchQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_batch_query: 98 link not connected, fund_account="
                   << req.fund_account_id.data() << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_batch_query: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_query_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_batch_query: queue full, pos=" << pos
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_order_batch_query_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}
// deal_trade_query: 成交查询
int32 counter98::deal_trade_query(const TradeQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_query: 98 link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_query: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_query_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_trade_query_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}
// deal_trade_batch_query: 成交批量查询
int32 counter98::deal_trade_batch_query(const TradeBatchQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_batch_query: 98 link not connected, fund_account="
                   << req.fund_account_id.data() << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_batch_query: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_query_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_batch_query: queue full, pos=" << pos
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_trade_batch_query_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}
// deal_fund_query: 资金查询
int32 counter98::deal_fund_query(const FundQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_query: 98 link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_query: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_fund_query_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_fund_query_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}
// deal_position_query: 持仓查询
int32 counter98::deal_position_query(const PositionQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_position_query: 98 link not connected, fund_account="
                   << req.fund_account_id.data() << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // todo : 需要依据正式缓存和柜台规则，重新实现检查
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_position_query: agw not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = sizeof(c98_msg_head_tmp) + sizeof(c98_position_query_req);
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_position_query: queue full, pos=" << pos
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_position_query_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}

// ---- build_*_msg: 98 协议消息构造 (头 + body) ----

// build_order_msg: 构造 c98 委托消息 (头 + c98_order_req)
void counter98::build_order_msg(const OrderReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_etf_order_msg: 构造 c98 ETF 申购赎回消息 (头 + body)
void counter98::build_etf_order_msg(const OrderReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_bse_order_msg: 构造 c98 北交所 (BSE) 委托消息 (头 + body)
void counter98::build_bse_order_msg(const OrderReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_cancel_msg: 构造 c98 撤单消息 (头 + c98_cancel_req)
void counter98::build_cancel_msg(const CancelReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_order_query_msg: 构造 c98 委托查询消息 (头 + c98_query_req, 按单笔委托号)
void counter98::build_order_query_msg(const OrderQueryReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_order_batch_query_msg: 构造 c98 委托批量查询消息 (头 + c98_query_req, 按委托号区间)
void counter98::build_order_batch_query_msg(const OrderBatchQueryReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_trade_query_msg: 构造 c98 成交查询消息 (头 + c98_query_req, 按单笔委托号)
void counter98::build_trade_query_msg(const TradeQueryReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_trade_batch_query_msg: 构造 c98 成交批量查询消息 (头 + c98_query_req, 按成交号区间)
void counter98::build_trade_batch_query_msg(const TradeBatchQueryReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_fund_query_msg: 构造 c98 资金查询消息 (头 + c98_fund_query_req)
void counter98::build_fund_query_msg(const FundQueryReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

// build_position_query_msg: 构造 c98 持仓查询消息 (头 + c98_position_query_req)
void counter98::build_position_query_msg(const PositionQueryReq &req, char *o_buf) {
  // todo : 依据正式协议重写
  (void)req;
  (void)o_buf;
}

/// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
void counter98::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret) {
  // todo : 依据正式协议重写

  const c98_msg_head_tmp *head = reinterpret_cast<const c98_msg_head_tmp *>(msg_buf);
  switch (head->msg_id) {
  case G1_MSG_ORDER_REQ: {
    // 委托发送失败: 构造 OrderRtn 通知客户

    break;
  }
  case G1_MSG_CANCEL_REQ: {
    // 撤单发送失败: 构造 CancelRsp 通知客户

    break;
  }
  default:
    break;
  }
}

// deal_recv_msg: 收到 98 消息处理
int32 counter98::deal_recv_msg(const char *buf, int32 len, int16 link_type) {
  // todo : 依据正式协议重写

  int32 deal_len = 0;
  int32 mlen = 0;
  const c98_msg_head_tmp *head;

  while (static_cast<int32>(len) - deal_len >= static_cast<int32>(sizeof(c98_msg_head_tmp))) {
    head = reinterpret_cast<const c98_msg_head_tmp *>(buf + deal_len);
    mlen = static_cast<int32>(head->msg_len) + static_cast<int32>(sizeof(c98_msg_head_tmp));
    if (unlikely(mlen > len - deal_len)) {
      if (mlen < C98_MSG_MAX_LEN) {
        return deal_len;
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "98 counter recv msg: msg len error, mlen=" << mlen << ", link_type=" << link_type << end_log;
        // 返回错误，让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
    }

    switch (head->msg_id) {
    case C98_MSG_AGW_LOGIN_ANS: {
      deal_agwuser_login_ans(head);
      break;
    }
    case C98_MSG_ACC_LOGIN_ANS: {
      deal_cust_login_ans(head);
      break;
    }
    case C98_MSG_HEART_ANS: {
      gw_eng_op_->deal_heart_msg_ans(link_type);
      break;
    }
    default: {
      // 未知消息, 跳过，不返回错误，以避免链接关闭，保证旧api对新增消息的兼容性
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "98 counter recv unknown msg, msg_id=" << head->msg_id << end_log;
      break;
      //return LBAPI_ERR_MSG_TYPE;
    }
    }

    deal_len += mlen;
  }

  return deal_len;
}

// do_agw_login: 发起 agw 用户登录, 同步等待结果
int32 counter98::deal_agw_login() {
  lb_common::lb_log_hand tlh(log_);

  int32 tn = agw_user_login_timeout_ * 10000;
  while (tn > 0) {
    int16 t = lb_common::atomic_load16(&agw_login_state);
    if (t == 0) {
      if (lb_common::atomic_cas16(&agw_login_state, &t, 1)) {
        break;
      }
    } else if (t == 1) {
      lb_common::comm_utils::sleep_us(100);
      tn--;
    } else {
      info_log(tlh) << "agw user have login" << end_log;
      return LBAPI_OK;
    }
  }

  int32 evt_len = sizeof(link_send_event);
  char *data = nullptr; // TODO -- 赋值登陆信息;
  int64 pos = gw_send_queue_->write_get_mth(data, evt_len);
  if (pos <= 0) {
    lb_common::atomic_store16(&agw_login_state, 0);
    error_log(tlh) << "add agw user login event error,ret=" << pos << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }
  link_send_event *evt = reinterpret_cast<link_send_event *>(data);
  evt->link_type = LINK_TYPE_98;
  evt->type = LINK_EVENT_TYPE_AGWUSER_LOGIN;
  evt->data_len = 0;
  evt->leave_time_ptr = nullptr;  // 登录事件不参与性能测试
  gw_send_queue_->write_cmt_mth(pos, evt_len);
  gw_eng_op_->trigger_send();

  info_log(tlh) << "add agw user login event ok" << end_log;

  // 同步等待 agw 登录结果 (D16: 超时仅适用登录阶段)
  tn = agw_user_login_timeout_ * 10000;
  while (tn > 0) {
    int32 t = lb_common::atomic_load16(&agw_login_state);
    if (t == 0) {
      error_log(tlh) << "agw user login error" << end_log;
      return LBAPI_ERR_LOGIN_FAIL;
    } else if (t == 1) {
      lb_common::comm_utils::sleep_us(100);
      tn--;
    } else {
      info_log(tlh) << "agw user login ok" << end_log;
      return LBAPI_OK;
    }
  }

  // todo : 超时是否需要重新发起登出请求？

  error_log(tlh) << "agw user login timeout" << end_log;
  return LBAPI_ERR_LOGIN_TIMEOUT;
}

// build_agw_login_msg: 构造 agw 用户登录消息
int32 counter98::build_agw_login_msg(char *o_buf, int32 buf_len) {
  // todo : 依据正式协议重写
  if (o_buf == nullptr || buf_len < (int32)sizeof(c98_msg_head_tmp) + (int32)sizeof(c98_agw_login_req)) {
    return LBAPI_ERR_MSG_LEN;
  }
  c98_msg_head_tmp *head = reinterpret_cast<c98_msg_head_tmp *>(o_buf);
  head->msg_id = C98_MSG_AGW_LOGIN_REQ;
  head->msg_len = sizeof(c98_agw_login_req);
  head->seq_no = 0;
  c98_agw_login_req *body = reinterpret_cast<c98_agw_login_req *>(o_buf + sizeof(c98_msg_head_tmp));
  std::memset(body, 0, sizeof(*body));
  body->client_req_no = 0;
  for (size_t i = 0; i < sizeof(body->agw_user) && i < agw_user_.size(); ++i) {
    body->agw_user[i] = agw_user_[i];
  }
  for (size_t i = 0; i < sizeof(body->agw_user_password) && i < agw_password_.size(); ++i) {
    body->agw_user_password[i] = agw_password_[i];
  }
  // version 字段保留
  return static_cast<int32>(sizeof(c98_msg_head_tmp) + sizeof(c98_agw_login_req));
}

void counter98::ans_agwuser_login(int32 err_code, const char *err_msg) {
  if (err_code == LBAPI_OK) {
    lb_common::atomic_store16(&agw_login_state, 2);
    cb_mgr_->on_error(err_event_type::agw_user_login, 0, err_msg);
  } else {
    lb_common::atomic_store16(&agw_login_state, 0);
    cb_mgr_->on_error(err_event_type::agw_user_login, err_code, err_msg);
  }
}

void counter98::deal_agwuser_login_ans(const c98_msg_head_tmp *msg) {
  lb_common::lb_log_hand tlh(log_);

  // todo : 依据正式协议重写
  const c98_agw_login_ans *ans = reinterpret_cast<const c98_agw_login_ans *>(msg + 1);
  if (ans->err_code == 0) {
    lb_common::comm_utils::str_copy_format(agw_session, ans->session, sizeof(agw_session));
    info_log(tlh) << "agw user login answer msg to ok,session=" << ans->session << end_log;
    ans_agwuser_login(0, NULL);

    // todo : 检查已经登陆的用户, 重新发起账户登陆事件
  } else {
    error_log(tlh) << "agw user login answer to failed,err_code=" << ans->err_code << ",err_msg=" << ans->err_msg
                   << end_log;
    ans_agwuser_login(LBAPI_ERR_LOGIN_FAIL, "agw user login answer to failed");
  }
}

// deal_login_req: 发起账户登录 (D8: 内部完成 LoginReq → acc_login_event_info 转换)
int32 counter98::deal_login_req(const LoginReq &req) {
  lb_common::lb_log_hand tlh(log_);

  if (agw_login_state != 2) {
    error_log(tlh) << "98 age user not login error,branch_id=" << req.branch_id.data()
                   << ",fund_account=" << req.fund_account_id.data() << ",client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  // 转换 LoginReq → acc_login_event_info
  acc_login_event_info info;
  build_cust_login_event(req, info);

  // 投账户登录事件到 multi 引擎
  int32 evt_len = sizeof(link_send_event) + sizeof(acc_login_event_info);
  char *data = nullptr;
  int64 pos = gw_send_queue_->write_get_mth(data, evt_len);
  if (pos <= 0) {
    error_log(tlh) << "add 98 cust login event to queue error,branch_id=" << info.branch_id
                   << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                   << ",client_req_no=" << info.cust_req_no << ",ret= " << pos << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }
  link_send_event *evt = reinterpret_cast<link_send_event *>(data);
  evt->link_type = LINK_TYPE_98;
  evt->type = LINK_EVENT_TYPE_ACCOUNT_LOGIN;
  evt->data_len = sizeof(acc_login_event_info);
  evt->leave_time_ptr = nullptr;  // 登录事件不参与性能测试
  std::memcpy(evt->data, &info, sizeof(acc_login_event_info));
  gw_send_queue_->write_cmt_mth(pos, evt_len);
  gw_eng_op_->trigger_send();

  info_log(tlh) << "add 98 cust login event to queue,branch_id=" << info.branch_id
                << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                << ",client_req_no=" << info.cust_req_no << end_log;
  return LBAPI_OK;
}

void counter98::build_cust_login_event(const LoginReq &req, acc_login_event_info &o_info) {
  o_info.cust_req_no = req.client_req_no;
  // 注意：fund_account_id/cust_id 等为定长字段，可能占满字段（无 '\0' 结尾）。
  // 不能用 str_copy_format（其最多拷贝 dst_size-1 字节，会把满字段截断），改用 memcpy 完整拷贝。
  std::memcpy(o_info.cust_id, req.cust_id.data(),
              std::min<size_t>(sizeof(o_info.cust_id), req.cust_id.size()));
  std::memcpy(o_info.fund_account_id, req.fund_account_id.data(),
              std::min<size_t>(sizeof(o_info.fund_account_id), req.fund_account_id.size()));
  std::memcpy(o_info.account_id, req.account_id.data(),
              std::min<size_t>(sizeof(o_info.account_id), req.account_id.size()));
  // branch_id 定长 10 字节，FTE 端 CopyToArray 用空格填充；此处同样用空格填充以便 key 匹配
  std::memset(o_info.branch_id, ' ', sizeof(o_info.branch_id));
  std::memcpy(o_info.branch_id, req.branch_id.data(),
              std::min<size_t>(sizeof(o_info.branch_id), req.branch_id.size()));

  o_info.order_way_ext[0] = req.order_way_ext[0];
  o_info.order_way_ext[1] = req.order_way_ext[1];
  std::memcpy(o_info.session, agw_session, sizeof(o_info.session));
  std::memcpy(o_info.password, req.password.data(), sizeof(o_info.password));
  std::memcpy(o_info.user_info, req.user_info.data(), sizeof(o_info.user_info));
  std::memcpy(o_info.client_feature_code, req.client_feature_code.data(), sizeof(o_info.client_feature_code));
}

// 检查该用户是否需要登陆，若是构造登陆消息到o_buf;若否，发起极速柜台登陆事件，成功返回0
int32 counter98::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len) {
  // todo : 依据正式协议和柜台规则重写
  // 默认需要账户登录，后续可基于缓存优化（如已登录则跳过）
  bool cust_need_login = true;
  if (cust_need_login) {
    return build_login_msg(req, o_buf, buf_len);
  } else {
    return delive_fast_counter_login(req);
  }
}
int32 counter98::build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len) {
  // todo : 依据正式协议重写
  if (buf_len < (int32)sizeof(c98_msg_head_tmp) + (int32)sizeof(c98_acc_login_req)) {
    return LBAPI_ERR_MSG_LEN;
  }

  c98_msg_head_tmp *head = reinterpret_cast<c98_msg_head_tmp *>(o_buf);
  head->msg_id = C98_MSG_ACC_LOGIN_REQ;
  head->msg_len = sizeof(c98_acc_login_req);
  head->seq_no = info.cust_req_no;
  c98_acc_login_req *body = reinterpret_cast<c98_acc_login_req *>(o_buf + sizeof(c98_msg_head_tmp));

  std::memset(body, 0, sizeof(*body));
  body->client_req_no = info.cust_req_no;
  std::memcpy(body->cust_id, info.cust_id, sizeof(body->cust_id));
  std::memcpy(body->fund_account_id, info.fund_account_id, sizeof(body->fund_account_id));
  std::memcpy(body->branch_id, info.branch_id, sizeof(body->branch_id));
  std::memcpy(body->account_id, info.account_id, sizeof(body->account_id));
  std::memcpy(body->password, info.password, sizeof(body->password));
  std::memcpy(body->session, info.session, sizeof(body->session));
  std::memcpy(body->end_code, info.client_feature_code, sizeof(body->end_code));
  body->market_type = market_type;
  body->heart_bt_int = static_cast<uint16>(heart_interval);
  body->order_way[0] = info.order_way_ext[0];
  body->order_way[1] = info.order_way_ext[1];

  return static_cast<int32>(sizeof(c98_msg_head_tmp) + sizeof(c98_acc_login_req));
}

int32 counter98::delive_fast_counter_login(const acc_login_event_info &info) {
  lb_common::lb_log_hand tlh(log_);

  //note : 这里对极速柜台做了特殊处理，因为个微和fpga柜台登陆的不一致。
  lb_common::que_mth_buf *tq = nullptr;
  int16 tlink_type = 0;
  if (fast_counter_type_ == counter_type::gw_direct) {
    tlink_type = LINK_TYPE_SPEED_TRADE;
    tq = trade_send_queue_;
  } else {
    tlink_type = LINK_TYPE_SPEED_GW;
    tq = gw_send_queue_;
  }

  int32 total_len = sizeof(link_send_event) + sizeof(acc_login_event_info);
  char *data = nullptr;
  int64 pos = tq->write_get_mth(data, total_len);
  if (pos <= 0) {
    error_log(tlh) << "add fast counter cust login event to queue error,branch_id=" << info.branch_id
                   << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                   << ",client_req_no=" << info.cust_req_no << ",ret= " << pos << end_log;
    ans_cust_login(info, LBAPI_ERR_SEND_QUEUE_FULL, "add fast counter cust login event to queue error");
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  link_send_event *te_head = reinterpret_cast<link_send_event *>(data);
  if (fast_counter_type_ == counter_type::gw_direct)
    te_head->link_type = LINK_TYPE_SPEED_TRADE;
  else
    te_head->link_type = LINK_TYPE_SPEED_GW;
  te_head->type = LINK_EVENT_TYPE_ACCOUNT_LOGIN;
  te_head->data_len = sizeof(acc_login_event_info);
  te_head->leave_time_ptr = nullptr;  // 登录事件不参与性能测试

  std::memcpy(te_head->data, (const char *)(&info), sizeof(acc_login_event_info));

  // NOTE: 实际链接 link_type (LINK_TYPE_SPEED_TRADE) 应由 fast_engine 消费时识别
  tq->write_cmt_mth(pos, total_len);
  if (tlink_type == LINK_TYPE_SPEED_GW) {
    gw_eng_op_->trigger_send();
  }

  info_log(tlh) << "add fast counter cust login event to queue,branch_id=" << info.branch_id
                << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                << ",client_req_no=" << info.cust_req_no << end_log;

  return LBAPI_OK;
}

void counter98::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg) {
  LoginAns ans;
  //std::memset(&ans, 0, sizeof(ans));

  ans.client_req_no = req.cust_req_no;
  std::memcpy(ans.cust_id.data(), req.cust_id, sizeof(ans.cust_id));
  std::memcpy(ans.fund_account_id.data(), req.fund_account_id, sizeof(ans.fund_account_id));
  std::memcpy(ans.account_id.data(), req.account_id, sizeof(ans.account_id));
  std::memcpy(ans.branch_id.data(), req.branch_id, sizeof(ans.branch_id));
  ans.market_type = market_type;
  ans.err_code = err_ret;
  if (err_msg != nullptr) {
    lb_common::comm_utils::str_copy_format(ans.err_msg.data(), err_msg, sizeof(ans.err_msg));
  } else {
    std::memset(ans.err_msg.data(), 0, sizeof(ans.err_msg));
  }
  ans.login_time = 0; // lb_common::comm_utils::get_time_ms(); ///< 登陆时间，HHMMSSmmm

  cb_mgr_->on_login(ans);
}

void counter98::deal_cust_login_ans(const c98_msg_head_tmp *msg) {
  // todo : 依据正式协议重写
  const c98_acc_login_ans *ans = reinterpret_cast<const c98_acc_login_ans *>(msg + 1);
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "recv 98 cust login answer msg,branch_id=" << ans->branch_id
                << ",fund_account=" << ans->fund_account_id << ",session=" << ans->session
                << ",client_req_no=" << ans->client_req_no << ",err_code= " << ans->err_code
                << ",err_msg=" << ans->err_msg << end_log;

  if (ans->err_code == 0) {
    // todo : 存储已经成功登陆的用户

    // 账户登录成功, 提取 session 并触发极速柜台登录 (D12)
    // 构造 acc_login_event_info 投到 fast_counter
    acc_login_event_info info;
    build_fast_counter_login_event(*ans, info);

    delive_fast_counter_login(info);

  } else {
    // todo : 处理客户登陆状态

    // 回调客户失败
    LoginAns ans_info;
    build_cust_login_rtn(*ans, ans_info);

    cb_mgr_->on_login(ans_info);
  }
}
void counter98::build_fast_counter_login_event(const c98_acc_login_ans &msg, acc_login_event_info &o_info) {
  // todo : 依据正式协议重写
  o_info.cust_req_no = msg.client_req_no;
  // 定长字段用 memcpy 完整拷贝，避免 str_copy_format 截断满字段（同 build_cust_login_event）
  std::memcpy(o_info.cust_id, msg.cust_id, std::min<size_t>(sizeof(o_info.cust_id), sizeof(msg.cust_id)));
  std::memcpy(o_info.fund_account_id, msg.fund_account_id,
              std::min<size_t>(sizeof(o_info.fund_account_id), sizeof(msg.fund_account_id)));
  std::memcpy(o_info.account_id, msg.account_id,
              std::min<size_t>(sizeof(o_info.account_id), sizeof(msg.account_id)));
  // branch_id 定长 10 字节，FTE 端 CopyToArray 用空格填充；此处同样用空格填充以便 key 匹配
  std::memset(o_info.branch_id, ' ', sizeof(o_info.branch_id));
  std::memcpy(o_info.branch_id, msg.branch_id,
              std::min<size_t>(sizeof(o_info.branch_id), sizeof(msg.branch_id)));
  // 拷贝密码，用于后续 FTE 登录验证
  std::memcpy(o_info.password, msg.password,
              std::min<size_t>(sizeof(o_info.password), sizeof(msg.password)));

  o_info.order_way_ext[0] = msg.order_way_ext[0];
  o_info.order_way_ext[1] = msg.order_way_ext[1];
  std::memcpy(o_info.session, agw_session, sizeof(o_info.session));
  std::memcpy(o_info.password, msg.password, sizeof(o_info.password));
  std::memcpy(o_info.user_info, msg.user_info, sizeof(o_info.user_info));
  std::memcpy(o_info.client_feature_code, msg.end_code, sizeof(o_info.client_feature_code));
}
void counter98::build_cust_login_rtn(const c98_acc_login_ans &msg, LoginAns &o_ans) {
  // todo : 依据正式协议重写
  std::memset(&o_ans, 0, sizeof(o_ans));
  o_ans.client_req_no = msg.client_req_no;
  lb_common::comm_utils::str_copy_format(o_ans.cust_id.data(), msg.cust_id, sizeof(o_ans.cust_id));
  lb_common::comm_utils::str_copy_format(o_ans.fund_account_id.data(), msg.fund_account_id,
                                         sizeof(o_ans.fund_account_id));
  lb_common::comm_utils::str_copy_format(o_ans.account_id.data(), msg.account_id, sizeof(o_ans.account_id));
  lb_common::comm_utils::str_copy_format(o_ans.branch_id.data(), msg.branch_id, sizeof(o_ans.branch_id));

  o_ans.market_type = market_type;
  o_ans.err_code = msg.err_code;
  if (msg.err_msg[0] != '\0') {
    int32 tlen = static_cast<int32>(strnlen(msg.err_msg, sizeof(msg.err_msg)));
    tlen = tlen < static_cast<int32>(sizeof(o_ans.err_msg)) ? tlen : static_cast<int32>(sizeof(o_ans.err_msg)) - 1;
    std::memcpy(o_ans.err_msg.data(), msg.err_msg, tlen);
  }
  o_ans.login_time = msg.login_time;
}

// build_heart_msg: 98 心跳
int32 counter98::build_heart_msg(char *o_buf, int32 buf_len) {
  if (o_buf == nullptr || buf_len < (int32)sizeof(c98_msg_head_tmp)) {
    return -1;
  }
  c98_msg_head_tmp *head = reinterpret_cast<c98_msg_head_tmp *>(o_buf);
  head->msg_id = C98_MSG_HEART_REQ;
  head->msg_len = 0;
  head->seq_no = 0;
  return static_cast<int32>(sizeof(c98_msg_head_tmp));
}

int32 counter98::deal_link_connect(int16 link_type, int32 have_switch) {
  if (link_type == LINK_TYPE_98) {
    trade_link_connect_ = 1;
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(0, 0, 1);
    }
    // 发生地址切换时, 重新发起 agw 用户登陆
    if (have_switch != 0) {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "98 link address switched, re-deliver agw user login" << end_log;
      int32 evt_len = sizeof(link_send_event);
      char *data = nullptr;
      int64 pos = gw_send_queue_->write_get_mth(data, evt_len);
      if (pos <= 0) {
        fatal_log(tlh) << "add agw user login event after switch error,ret=" << pos << end_log;
        return LBAPI_ERR_SEND_QUEUE_FULL;
      }
      link_send_event *evt = reinterpret_cast<link_send_event *>(data);
      evt->link_type = LINK_TYPE_98;
      evt->type = LINK_EVENT_TYPE_AGWUSER_LOGIN;
      evt->data_len = 0;
      evt->leave_time_ptr = nullptr;  // 登录事件不参与性能测试
      gw_send_queue_->write_cmt_mth(pos, evt_len);
      gw_eng_op_->trigger_send();
    }
  }
  return 0;
}

void counter98::deal_link_close(int16 link_type) {
  if (link_type == LINK_TYPE_98) {
    trade_link_connect_ = 0;
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(0, 0, 0);
    }
    // todo 处理登陆过程中的断线
  }
}

} // namespace lb_api
=======
// counter98 - 98 柜台协议对象实现
//
// 使用 lb_counter98 命名空间下的协议消息类完成消息组包/解析.
// 消息格式: AgwHeader(16B) + Body(变长)
// 编码: SerializeBuffer + MessageEncoder → msg.Encode(encoder)
// 解码: MessageDecoder(body, len) → msg.Decode(decoder)

#include "counter98.h"
#include "api_errno.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "counter98/msg_map_agw_external.h"
#include "matomic.h"
#include "mutils.h"

// 模板实例化: 确保 counter98 与各柜台类型的模板组合被实例化
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "gw_counter_direct.h"

namespace lb_api {

// ---- 内部常量 ----
static constexpr int32 C98_MAX_MSG_LEN = 2048; ///< 消息体最大长度(含变长字段安全余量)

// ---- AgwHeader 原始结构(16字节, 与协议线格式一致) ----
// 由于 AgwHeader 继承 RootPacket 含虚函数, 不能直接 reinterpret_cast,
// 用此 POD 结构读写线格式.
struct raw_agw_header {
  uint32_t msg_type;
  uint16_t msg_ver;
  uint16_t reserver;
  int32_t seq_num;   ///< 协议头序号 (98柜台未使用, 保持为0)
  uint32_t body_length;
};
static_assert(sizeof(raw_agw_header) == 16, "raw_agw_header must be 16 bytes");

// ---- 辅助: 计算 CheckSum (对 Header + Body 逐字节累加, 取 % 256) ----
static uint32_t calc_check_sum(const char *data, uint32_t len) {
  uint32_t sum = 0;
  for (uint32_t i = 0; i < len; ++i) {
    sum += static_cast<uint32_t>(static_cast<unsigned char>(data[i]));
  }
  return sum % 256;
}

// ---- 辅助: 写 AgwHeader + Body + CheckSum 到 o_buf ----
// 线格式: [AgwHeader 16B] [Body 变长] [CheckSum 4B 网络字节序]
// body_length 仅含 Body, 不含 CheckSum; CheckSum 覆盖 Header + Body
static int32 write_msg(char *o_buf, uint32_t msg_type, uint16_t msg_ver,
                       const char *body, uint32_t body_len) {
  raw_agw_header hdr{};
  hdr.msg_type = msg_type;
  hdr.msg_ver = msg_ver;
  hdr.reserver = 0;
  hdr.body_length = body_len;
  std::memcpy(o_buf, &hdr, sizeof(hdr));
  if (body_len > 0 && body != nullptr) {
    std::memcpy(o_buf + sizeof(hdr), body, body_len);
  }
  // CheckSum: 对 Header + Body 计算, 以 uint32_t 网络字节序追加
  uint32_t check_sum = mp::HostToNetwork(
      calc_check_sum(o_buf, static_cast<uint32_t>(sizeof(hdr) + body_len)));
  std::memcpy(o_buf + sizeof(hdr) + body_len, &check_sum, sizeof(check_sum));
  return static_cast<int32>(sizeof(hdr) + body_len + sizeof(uint32_t));
}

// ---- 辅助: 编码消息体到 SerializeBuffer, 返回 body 指针和长度 ----
template <typename MsgType>
static bool encode_body(const MsgType &msg, lb_counter98::SerializeBuffer &buf) {
  lb_counter98::MessageEncoder encoder(buf);
  auto ec = msg.Encode(encoder);
  return ec == lb_counter98::kSuccess;
}

// ---- 辅助: 解码消息体 ----
template <typename MsgType>
static bool decode_body(MsgType &msg, const char *body, int32 body_len) {
  lb_counter98::MessageDecoder decoder(body, static_cast<size_t>(body_len));
  auto ec = msg.Decode(decoder);
  return ec == lb_counter98::kSuccess;
}

// ============================================================
// 构造 / 析构 / 初始化
// ============================================================

counter98::counter98() {}

counter98::~counter98() = default;

int32 counter98::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log) {
  cb_mgr_ = cb;
  log_ = log;
  gw_send_queue_ = nullptr;
  trade_send_queue_ = nullptr;
  gw_eng_op_ = nullptr;
  trade_eng_op_ = nullptr;
  market_type = static_cast<int16>(cfg.get_market_type());
  heart_interval = static_cast<int16>(cfg.get_heartbeat_interval());
  fast_counter_type_ = cfg.get_fast_counter_type();
  agw_login_state = 0;
  trade_link_connect_ = 0;
  agw_user_login_timeout_ = cfg.get_agw_user_login_timeout();
  if (agw_user_login_timeout_ < 5)
    agw_user_login_timeout_ = 5;

  std::memset(agw_session, 0, sizeof(agw_session));
  agw_user_ = cfg.get_agw98_user();
  agw_password_ = cfg.get_agw98_user_password();
  counter98_mode_ = cfg.get_counter98_mode();
  counter98_report_sync_ = cfg.get_counter98_report_sync();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "init counter 98 ok,market_type=" << market_type
                << ",agw_user=" << cfg.get_agw98_user()
                << ",agw_user_login_timeout=" << agw_user_login_timeout_
                << end_log;

  init_dispatch_map();
  return LBAPI_OK;
}

// ============================================================
// init_dispatch_map — 注册下行消息处理函数
// ============================================================
// 使用 REGISTER_DEAL_FUNC 宏注册, 新增下行消息只需加一行:
//   REGISTER_DEAL_FUNC(kAgwMsgXxx, 2, deal_xxx_ans);
// deal_recv_msg 中通过 deal_dispatch_map_ 查找并调用, 无需 switch-case.

void counter98::init_dispatch_map() {
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgLogin,                0, deal_agwuser_login_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgSyncFinish,           0, deal_sync_finish_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgCustLoginResp,        2, deal_cust_login_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgOrderStatusAck,       2, deal_order_ack)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgCashAuctionTradeER,   2, deal_trade_ter)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgCancelOrder,          2, deal_cancel_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultOrder,  2, deal_order_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultTradeOrder, 2, deal_trade_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultFund,   2, deal_fund_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultSelfShare, 2, deal_position_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryMaxOrderQtyResult, 2, deal_max_order_qty_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultTotalFundAsset, 2, deal_total_fund_asset_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultFundMaxdraw, 2, deal_fund_maxdraw_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultCustMarginAvl, 2, deal_cust_margin_avl_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryResultAssetDebts, 2, deal_asset_debts_query_ans)
  REGISTER_DEAL_FUNC(lb_counter98::kAgwMsgExtQueryCreditMaxOrderQtyResult, 2, deal_credit_max_order_qty_query_ans)
}

// ============================================================
// deal_*_req — 业务请求入队
// ============================================================

/// 处理买卖委托请求 — 组包并入队发送
///
/// 业务流程:
///   1. 前置校验: 链路状态 + AGW 登录状态
///   2. 从发送队列预分配内存 (AgwHeader + 最大消息体)
///   3. 调用 build_order_msg 组包写入队列内存
///   4. 设置 link_send_event.data_len 为实际消息长度
///   5. 提交队列内存并触发发送
///
/// @param req  委托请求 (含证券代码、价格、数量、客户序列号等)
/// @return LBAPI_OK 成功; LBAPI_ERR_LINK_DISCONNECTED 链路断开;
///         LBAPI_ERR_NOT_LOG_AGW AGW未登录; LBAPI_ERR_SEND_QUEUE_FULL 队列满
int32 counter98::deal_order_req(const OrderReq &req) {
  // 前置校验: 98 链路必须已连接
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_req: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  // 前置校验: AGW 用户必须已登录成功
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_req: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  // 预分配队列内存: AgwHeader(16B) + 消息体最大长度
  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  // 组包: 将 OrderReq 编码为 98 协议消息, 写入 data+sizeof(link_send_event)
  int32 msg_len = build_order_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_order_req: build_order_msg failed, client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  // 提交队列内存并触发底层发送
  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_etf_order_req(const OrderReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_etf_order_req: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_etf_order_req: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_etf_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_etf_order_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_etf_order_req: build_etf_order_msg failed, client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_bse_order_req(const OrderReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_bse_order_req: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_bse_order_req: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_bse_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_bse_order_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_bse_order_req: build_bse_order_msg failed, client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32_t counter98::deal_cancel_req(const CancelReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cancel_req: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cancel_req: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cancel_req: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_cancel_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_cancel_req: build_cancel_msg failed, client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_order_query(const OrderQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_order_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_order_query: build_order_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_order_batch_query(const OrderBatchQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_batch_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_batch_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_order_batch_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_order_batch_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_order_batch_query: build_order_batch_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_trade_query(const TradeQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_trade_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_trade_query: build_trade_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_trade_batch_query(const TradeBatchQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_batch_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_batch_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_trade_batch_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_trade_batch_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_trade_batch_query: build_trade_batch_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_fund_query(const FundQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_fund_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_fund_query: build_fund_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_position_query(const PositionQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_position_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_position_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_position_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_position_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_position_query: build_position_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_max_order_qty_query(const MaxOrderQtyQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_max_order_qty_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_max_order_qty_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_max_order_qty_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_max_order_qty_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_max_order_qty_query: build_max_order_qty_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_total_fund_asset_query(const TotalFundAssetQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_total_fund_asset_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_total_fund_asset_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_total_fund_asset_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_total_fund_asset_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_total_fund_asset_query: build_total_fund_asset_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_fund_maxdraw_query(const FundMaxdrawQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_maxdraw_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_maxdraw_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_fund_maxdraw_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_fund_maxdraw_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_fund_maxdraw_query: build_fund_maxdraw_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_cust_margin_avl_query(const CustMarginAvlQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cust_margin_avl_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cust_margin_avl_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_cust_margin_avl_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_cust_margin_avl_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_cust_margin_avl_query: build_cust_margin_avl_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_asset_debts_query(const AssetDebtsQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_asset_debts_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_asset_debts_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_asset_debts_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_asset_debts_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_asset_debts_query: build_asset_debts_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

int32 counter98::deal_credit_max_order_qty_query(const CreditMaxOrderQtyQueryReq &req) {
  if (unlikely(trade_link_connect_ == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_credit_max_order_qty_query: 98 link not connected, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_credit_max_order_qty_query: agw not login, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_credit_max_order_qty_query: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_credit_max_order_qty_query_msg(req, data + sizeof(link_send_event));
  if (msg_len <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_credit_max_order_qty_query: build_credit_max_order_qty_query_msg failed, client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_BUILD_MSG;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

// ============================================================
// build_*_msg — 消息组包
// ============================================================

int32 counter98::build_order_msg(const OrderReq &req, char *o_buf) {
  // 查找客户缓存
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgCashAuctionOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.business_type = 1; // 现货集中竞价
  msg.security_id = req.security_id;
  msg.market_id = static_cast<lb_counter98::MarketID_def>(market_type);
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.side = req.side;
  msg.order_type = req.order_type;
  msg.price = req.order_price;
  msg.order_qty = req.order_qty;
  msg.client_seq_id = req.client_seq_id;
  msg.branch_id = session->branch_id;
  msg.policy_id = req.policy_id;
  msg.user_info = session->user_info;
  msg.enforce_flag = 0;
  msg.order_way_ext = session->order_way_ext;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgCashAuctionOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_etf_order_msg(const OrderReq &req, char *o_buf) {
  // ETF 申购赎回也使用 AgwMsgCashAuctionOrder, business_type 不同
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgCashAuctionOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.business_type = 2; // ETF 申购赎回
  msg.security_id = req.security_id;
  msg.market_id = static_cast<lb_counter98::MarketID_def>(market_type);
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.side = req.side;
  msg.order_type = req.order_type;
  msg.price = req.order_price;
  msg.order_qty = req.order_qty;
  msg.client_seq_id = req.client_seq_id;
  msg.branch_id = session->branch_id;
  msg.policy_id = req.policy_id;
  msg.user_info = session->user_info;
  msg.enforce_flag = 0;
  msg.order_way_ext = session->order_way_ext;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgCashAuctionOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_bse_order_msg(const OrderReq &req, char *o_buf) {
  // 北交所委托也使用 AgwMsgCashAuctionOrder
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgCashAuctionOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.business_type = 1; // 现货集中竞价
  msg.security_id = req.security_id;
  msg.market_id = static_cast<lb_counter98::MarketID_def>(market_type);
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.side = req.side;
  msg.order_type = req.order_type;
  msg.price = req.order_price;
  msg.order_qty = req.order_qty;
  msg.client_seq_id = req.client_seq_id;
  msg.branch_id = session->branch_id;
  msg.policy_id = req.policy_id;
  msg.user_info = session->user_info;
  msg.enforce_flag = 0;
  msg.order_way_ext = session->order_way_ext;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgCashAuctionOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_cancel_msg(const CancelReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgCancelOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.fund_account_id = session->fund_account_id;
  msg.branch_id = session->branch_id;
  msg.cust_id = session->cust_id;
  msg.account_id = session->account_id;
  msg.client_seq_id = req.client_seq_id;
  msg.orig_client_seq_id = req.client_req_no;
  msg.orig_cli_ord_no = req.order_sys_no;
  msg.market_id = static_cast<lb_counter98::MarketID_def>(market_type);
  msg.user_info = session->user_info;
  msg.order_way_ext = session->order_way_ext;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgCancelOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_order_query_msg(const OrderQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.cl_ord_no = req.user_seq_id;
  msg.user_info = session->user_info;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_order_batch_query_msg(const OrderBatchQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.cl_ord_no = req.order_sys_no_begin;
  msg.user_info = session->user_info;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_trade_query_msg(const TradeQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryTradeOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.cl_ord_no = req.user_seq_id;
  msg.user_info = session->user_info;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryTradeOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_trade_batch_query_msg(const TradeBatchQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryTradeOrder msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryTradeOrder, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_fund_query_msg(const FundQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryFund msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryFund, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_position_query_msg(const PositionQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQuerySelfShare msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.market_id = static_cast<uint16_t>(req.market_type);
  msg.security_id = req.security_id;
  msg.user_info = session->user_info;
  msg.query_index = req.page_ctrl.query_index;
  msg.return_num = req.page_ctrl.return_num;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQuerySelfShare, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_max_order_qty_query_msg(const MaxOrderQtyQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryMaxOrderQty msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;
  msg.market_id = static_cast<uint16_t>(req.market_type);
  msg.security_id = req.security_id;
  msg.business_type = req.business_type;
  msg.side = req.side;
  msg.price = req.price;
  msg.sh_account_id = req.sh_account_id;
  msg.sz_account_id = req.sz_account_id;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryMaxOrderQty, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_total_fund_asset_query_msg(const TotalFundAssetQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExternalQueryTotalFundAsset msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExternalQueryTotalFundAsset, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_fund_maxdraw_query_msg(const FundMaxdrawQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryFundMaxdraw msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;
  msg.currency = req.currency;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryFundMaxdraw, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_cust_margin_avl_query_msg(const CustMarginAvlQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryCustMarginAvl msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;
  msg.currency = req.currency;
  msg.market_id = req.market_id;
  msg.security_id = req.security_id;
  msg.credit_direct = req.credit_direct;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryCustMarginAvl, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_asset_debts_query_msg(const AssetDebtsQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryAssetDebts msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;
  msg.currency = req.currency;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryAssetDebts, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

int32 counter98::build_credit_max_order_qty_query_msg(const CreditMaxOrderQtyQueryReq &req, char *o_buf) {
  GwSessionInfo *session = GwSessionCache::instance().get_session(
      std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  if (!session)
    return -1;

  lb_counter98::AgwMsgExtQueryCreditMaxOrderQty msg;
  msg.password = std::string(session->password.data(), strnlen(session->password.data(), session->password.size()));
  msg.cust_id = session->cust_id;
  msg.fund_account_id = session->fund_account_id;
  msg.account_id = session->account_id;
  msg.branch_id = session->branch_id;
  msg.client_seq_id = req.client_req_no;
  msg.user_info = session->user_info;
  msg.market_id = req.market_type;
  msg.security_id = req.security_id;
  msg.business_type = req.business_type;
  msg.side = req.side;
  msg.price = req.price;

  lb_counter98::SerializeBuffer buf;
  if (!encode_body(msg, buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgExtQueryCreditMaxOrderQty, 2,
                   buf.Begin(), static_cast<uint32_t>(buf.Size()));
}

// ============================================================

// ============================================================
// deal_send_error — 发送失败处理
// ============================================================

void counter98::deal_send_error(char *msg_buf, int32 msg_len, int32 err_ret) {
  if (msg_len < static_cast<int32>(sizeof(raw_agw_header)))
    return;

  raw_agw_header *hdr = reinterpret_cast<raw_agw_header *>(msg_buf);
  switch (hdr->msg_type) {
  case lb_counter98::kAgwMsgCashAuctionOrder: {
    // 委托发送失败 → 构造 OrderRtn 回调
    OrderRtn rtn = {};
    rtn.rtn_type = RSP_TYPE_SEND_ERROR;    //ToDos: 此处待定？？？
    rtn.err_code = err_ret;
    rtn.order_status = ORDER_STATE_DISCARD;  //ToDos: 此处待定？？？
    // 从消息体解码获取字段
    if (hdr->body_length > 0) {
      lb_counter98::AgwMsgCashAuctionOrder msg;
      if (decode_body(msg, msg_buf + sizeof(raw_agw_header), hdr->body_length)) {
        rtn.cust_id = msg.cust_id;
        rtn.fund_account_id = msg.fund_account_id;
        rtn.account_id = msg.account_id;
        rtn.branch_id = msg.branch_id;
        rtn.security_id = msg.security_id;
        rtn.side = msg.side;
        rtn.order_type = msg.order_type;
        rtn.order_price = msg.price;
        rtn.order_qty = msg.order_qty;
        rtn.client_seq_id = msg.client_seq_id;
        rtn.market_type = static_cast<int16_t>(msg.market_id);
      }
    }
    StreamInfo si = {};
    si.counter_type = STREAM_API_COUNTER;
    si.stream_seq = 0;
    cb_mgr_->on_order_rtn(si, rtn);
    break;
  }
  case lb_counter98::kAgwMsgCancelOrder: {
    // 撤单发送失败 → 构造 CancelRsp 回调
    CancelRsp rsp = {};
    rsp.err_code = err_ret;
    rsp.rej_api = 1;
    if (hdr->body_length > 0) {
      lb_counter98::AgwMsgCancelOrder msg;
      if (decode_body(msg, msg_buf + sizeof(raw_agw_header), hdr->body_length)) {
        rsp.cust_id = msg.cust_id;
        rsp.fund_account_id = msg.fund_account_id;
        rsp.account_id = msg.account_id;
        rsp.branch_id = msg.branch_id;
        rsp.client_seq_id = msg.client_seq_id;
        rsp.order_sys_no = msg.orig_cli_ord_no;
      }
    }
    StreamInfo si = {};
    si.counter_type = STREAM_API_COUNTER;
    si.stream_seq = 0;
    cb_mgr_->on_cancel_rsp(si, rsp);
    break;
  }
  default: {
    // 其他消息类型(查询/登录/心跳等)发送失败 → on_error 通知
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_send_error: unhandled msg_type=" << hdr->msg_type
                   << ", err_ret=" << err_ret << end_log;
    char err_desc[64];
    snprintf(err_desc, sizeof(err_desc), "send error, msg_type=%u", hdr->msg_type);
    cb_mgr_->on_error(err_event_type::send_error, err_ret, err_desc);
    break;
  }
  }
}

// ============================================================
// deal_recv_msg — 接收消息分发
// ============================================================

int32 counter98::deal_recv_msg(const char *buf, int32 len, int16 link_type) {
  int32 offset = 0;
  while (offset + static_cast<int32>(sizeof(raw_agw_header)) <= len) {
    raw_agw_header hdr;
    std::memcpy(&hdr, buf + offset, sizeof(hdr));
    // msg_total 包含 CheckSum(4B): Header + Body + CheckSum
    int32 msg_total = sizeof(hdr) + static_cast<int32>(hdr.body_length) + static_cast<int32>(sizeof(uint32_t));
    
    // 剩余数据不够一条完整消息，则等下次再处理
    if (offset + msg_total > len)
      break;

    const char *body = buf + offset + sizeof(hdr);
    int32 body_len = static_cast<int32>(hdr.body_length);
    // 跳过末尾 4 字节 CheckSum (暂不校验)
    // const char *check_sum_ptr = buf + offset + sizeof(hdr) + body_len;
    // uint32_t check_sum_in_msg = 0;
    // std::memcpy(&check_sum_in_msg, check_sum_ptr, sizeof(check_sum_in_msg));
    // check_sum_in_msg = mp::NetworkToHost(check_sum_in_msg);
    // uint32_t check_sum_calc = calc_check_sum(buf + offset,
    //     static_cast<uint32_t>(sizeof(hdr) + body_len));
    // if (check_sum_in_msg != check_sum_calc) {
    //   offset += msg_total;
    //   continue;
    // }

    // 心跳应答: 通知引擎心跳已应答 (不走分发表)
    if (hdr.msg_type == lb_counter98::kAgwMsgHeartBeat) {
      gw_eng_op_->deal_heart_msg_ans(link_type);
      offset += msg_total;
      continue;
    }

    // 通过分发表查找 deal_* 函数, 替代 switch-case
    auto it = deal_dispatch_map_.find(make_dispatch_key(hdr.msg_type, hdr.msg_ver));
    if (it != deal_dispatch_map_.end()) {
      (this->*(it->second))(body, body_len);

      // 回报同步完成后, 保存下行回报消息的 partition/index
      // 仅对继承自 AgwMsgOrderReport 的回报消息 (成交执行报告) 保存
      // 查询类消息继承自 AgwMsgExtQueryResultBase, 无 partition/index 字段, 不应解码
      if (sync_finished_ && hdr.msg_type == lb_counter98::kAgwMsgCashAuctionTradeER) {
        save_report_sync_pos(body, body_len);
      }
    } else {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "98 counter recv unknown msg, msg_type=" << hdr.msg_type
                    << ", msg_ver=" << hdr.msg_ver << end_log;
    }

    offset += msg_total;
  }
  return offset;
}

// ============================================================
// deal_recv_msg_factory — Factory 模式版本 (已注释, 暂不使用)
// ============================================================
//
// 与 deal_recv_msg 的区别:
// 1. 使用 MP_CREATE_MESSAGE_ATP_GW 根据 (msg_type, msg_ver) 自动创建消息对象
//    — 无需为每种消息类型手写 case + decode_body<T>
// 2. 使用 RootPacket::PopFields 统一解码消息体
//    — 无需在各 deal_*_ans 内部各自调用 decode_body<T>
// 3. 新增下行消息处理时, 只需:
//    a. 在 msg_map_agw_external.cpp 中注册 (MP_REGISTER_MESSAGE_ATP_GW)
//    b. 在下方 switch 中加一个 case 分发
//    c. 实现 deal_*_ans 处理函数
//    而不需要修改消息创建/解码逻辑.
//
// 注意: 本实现中 deal_*_ans 仍接收原始 body 并自行解码,
//       Factory 创建+解码的结果用于验证消息类型正确性.
//       若后续将 deal_*_ans 改为接收 RootPacket&, 可完全消除重复解码.

// int32 counter98::deal_recv_msg_factory(const char *buf, int32 len, int16 link_type) {
//   (void)link_type;
//   int32 offset = 0;
//   while (offset + static_cast<int32>(sizeof(raw_agw_header)) <= len) {
//     raw_agw_header hdr;
//     std::memcpy(&hdr, buf + offset, sizeof(hdr));
//     // msg_total 包含 CheckSum(4B): Header + Body + CheckSum
//     int32 msg_total = sizeof(hdr) + static_cast<int32>(hdr.body_length) + static_cast<int32>(sizeof(uint32_t));
//     if (offset + msg_total > len)
//       break;
//
//     const char *body = buf + offset + sizeof(hdr);
//     int32 body_len = static_cast<int32>(hdr.body_length);
//     // 跳过末尾 4 字节 CheckSum (暂不校验)
//     // const char *check_sum_ptr = buf + offset + sizeof(hdr) + body_len;
//     // uint32_t check_sum_in_msg = 0;
//     // std::memcpy(&check_sum_in_msg, check_sum_ptr, sizeof(check_sum_in_msg));
//     // check_sum_in_msg = mp::NetworkToHost(check_sum_in_msg);
//     // uint32_t check_sum_calc = calc_check_sum(buf + offset,
//     //     static_cast<uint32_t>(sizeof(hdr) + body_len));
//     // if (check_sum_in_msg != check_sum_calc) {
//     //   offset += msg_total;
//     //   continue;
//     // }
//
//     // ---- Factory 模式: 统一创建 + 解码 ----
//     // 1. 根据 (msg_type, msg_ver) 从 Factory 创建消息对象
//     //    消息类型已在 msg_map_agw_external.cpp 中注册 (ver=2)
//     std::unique_ptr<mp::RootPacket> msg(
//         MP_CREATE_MESSAGE_ATP_GW(hdr.msg_type, hdr.msg_ver));
//
//     if (msg) {
//       // 2. 用 PopFields 统一解码消息体 (内部调用虚函数 Decode)
//       lb_counter98::MessageDecoder decoder(body, static_cast<size_t>(body_len));
//       auto ec = msg->PopFields(decoder);
//       if (ec != lb_counter98::kSuccess) {
//         // 解码失败, 跳过此消息
//         offset += msg_total;
//         continue;
//       }
//       // msg 已解码完成, 可通过 dynamic_cast<具体类型*>(msg.get()) 直接访问字段
//     }
//     // msg 为 nullptr 表示该 msg_type 未注册, 仍尝试按分发表分发
//
//     // 3. 通过分发表查找 deal_* 函数, 替代 switch-case
//     auto it = deal_dispatch_map_.find(make_dispatch_key(hdr.msg_type, hdr.msg_ver));
//     if (it != deal_dispatch_map_.end()) {
//       (this->*(it->second))(body, body_len);
//     }
//
//     offset += msg_total;
//   }
//   return offset;
// }

// ============================================================
// AGW 用户登录
// ============================================================

int32 counter98::deal_agw_login() {
  lb_common::lb_log_hand tlh(log_);

  // CAS: 只能从状态0→1, 已登录(2)直接返回
  int32 tn = agw_user_login_timeout_ * 10000;
  while (tn > 0) {
    int16 t = lb_common::atomic_load16(&agw_login_state);
    if (t == 0) {
      if (lb_common::atomic_cas16(&agw_login_state, &t, 1)) {
        break;
      }
    } else if (t == 1) {
      lb_common::comm_utils::sleep_us(100);
      tn--;
    } else {
      info_log(tlh) << "agw user have login" << end_log;
      return LBAPI_OK;
    }
  }

  // 申请队列内存 (封装模式, 与 deal_order_req 等业务函数统一)
  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::atomic_store16(&agw_login_state, 0);
    error_log(tlh) << "add agw user login event error, ret=" << pos << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  // 直接在队列内存中组包 (零拷贝)
  int32 msg_len = build_agw_login_msg(data + sizeof(link_send_event), take_len);
  if (msg_len <= 0) {
    lb_common::atomic_store16(&agw_login_state, 0);
    error_log(tlh) << "build agw login msg failed" << end_log;
    return LBAPI_ERR_LOGIN_FAIL;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  // 提交队列内存并触发发送
  cmt_req_que_mem(pos, take_len);

  info_log(tlh) << "add agw user login event ok" << end_log;

  // 同步等待登录结果
  tn = agw_user_login_timeout_ * 10000;
  while (tn > 0) {
    int16 t = lb_common::atomic_load16(&agw_login_state);
    if (t == 0) {
      error_log(tlh) << "agw user login error" << end_log;
      return LBAPI_ERR_LOGIN_FAIL;
    } else if (t == 1) {
      lb_common::comm_utils::sleep_us(100);
      tn--;
    } else {
      info_log(tlh) << "agw user login ok" << end_log;
      return LBAPI_OK;
    }
  }

  error_log(tlh) << "agw user login timeout" << end_log;
  return LBAPI_ERR_LOGIN_TIMEOUT;
}

int32 counter98::build_agw_login_msg(char *o_buf, int32 buf_len) {
  if (buf_len < static_cast<int32>(sizeof(raw_agw_header)) + 256)
    return -1;

  lb_counter98::AgwMsgLogin msg;
  // 构造函数已将 char 数组填充为空格(' '), 整数置 0, 无需额外 memset

  // 填充 AGW 登录字段
  std::memcpy(msg.sender_comp_id.data(), agw_user_.c_str(),
              std::min(agw_user_.size(), sizeof(msg.sender_comp_id)));
  std::memcpy(msg.password.data(), agw_password_.c_str(),
              std::min(agw_password_.size(), sizeof(msg.password)));
  msg.heart_bt_int = heart_interval;
  msg.spec_cust_flag = 0;

  lb_counter98::SerializeBuffer body_buf;
  if (!encode_body(msg, body_buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgLogin, 0,
                   body_buf.Begin(), static_cast<uint32_t>(body_buf.Size()));
}

void counter98::ans_agwuser_login(int32 err_code, const char *err_msg) {
  if (err_code == LBAPI_OK) {
    lb_common::atomic_store16(&agw_login_state, 2);
    cb_mgr_->on_error(err_event_type::agw_user_login, 0, err_msg);
  } else {
    lb_common::atomic_store16(&agw_login_state, 0);
    cb_mgr_->on_error(err_event_type::agw_user_login, err_code, err_msg);
  }
}

void counter98::deal_agwuser_login_ans(const char *body, int32 body_len) {
  // AGW 登录应答: 解码 AgwMsgLogin 响应
  lb_counter98::AgwMsgLogin ans;
  if (!decode_body(ans, body, body_len)) {
    ans_agwuser_login(LBAPI_ERR_LOGIN_FAIL, "agw login decode failed");
    return;
  }

  // 不含错误字段, 解码成功即表示AGW登录成功: 从 sender_comp_id 提取 session
  std::memcpy(agw_session, ans.sender_comp_id.data(),
              std::min(sizeof(agw_session), sizeof(ans.sender_comp_id)));
  agw_session[sizeof(agw_session) - 1] = '\0';

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "agw user login ok, session=" << agw_session << end_log;

  // AGW 登录成功后, 发送回报同步请求
  send_agw_sync();

  ans_agwuser_login(0, nullptr);
}

// ============================================================
// 回报同步
// ============================================================

int32 counter98::build_sync_msg(char *o_buf, int32 buf_len) {
  if (buf_len < static_cast<int32>(sizeof(raw_agw_header)) + 256)
    return -1;

//ToDos: 确认好AgwMsgSync是否必须发，若否则先判断counter98_mode为是否为1

  lb_counter98::AgwMsgSync msg;

  if (counter98_mode_ == 0) {
    // 默认登录模式: 根据 counter98_report_sync_ 填充分区/索引
    auto it = counter98_report_sync_.find(0);
    if (it != counter98_report_sync_.end() && it->second == -1) {
      // partition=0, index=-1 → 同步全部分区
      lb_counter98::AgwNoPartitionArray info;
      info.partition = 0;
      info.index = -1;
      msg.no_parition_array.push_back(info);
    } else {
      for (const auto &item : counter98_report_sync_) {
        lb_counter98::AgwNoPartitionArray info;
        info.partition = item.first;
        info.index = item.second;
        msg.no_parition_array.push_back(info);
      }
    }
  } else {
    
    // 目前不会进入该分支，因为快速登录模式下，在构造同步回报的请求消息前就会提前返回
    // 快速登录模式(mode==1): 不处理回报同步
    msg.no_parition_array.clear();
  }

  lb_counter98::SerializeBuffer body_buf;
  if (!encode_body(msg, body_buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgSync, 0,
                   body_buf.Begin(), static_cast<uint32_t>(body_buf.Size()));
}

void counter98::send_agw_sync() {
  lb_common::lb_log_hand tlh(log_);

  // 快速登录模式且无同步配置时, 跳过
  if (counter98_mode_ == 1) {
    info_log(tlh) << "skip report sync, counter98_mode=1 (fast login)" << end_log;
    return;
  }

  if (counter98_mode_ == 0 && counter98_report_sync_.empty()) {
    info_log(tlh) << "skip report sync, no counter98_report_sync config" << end_log;
    return;
  }

  // 申请队列内存
  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    error_log(tlh) << "send report sync error, queue full, ret=" << pos << end_log;
    return;
  }

  // 在队列内存中组包
  int32 msg_len = build_sync_msg(data + sizeof(link_send_event), take_len);
  if (msg_len <= 0) {
    error_log(tlh) << "build report sync msg failed" << end_log;
    return;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  // 提交并发送
  cmt_req_que_mem(pos, take_len);
  info_log(tlh) << "send report sync ok, mode=" << counter98_mode_
                << ", partition_count=" << counter98_report_sync_.size() << end_log;
}

void counter98::deal_sync_finish_ans(const char *body, int32 body_len) {
  sync_finished_ = true;

  lb_counter98::AgwMsgSyncFinish ans;
  if (!decode_body(ans, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "report sync finish decode failed" << end_log;
    return;
  }

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "report sync finish ok" << end_log;
}

void counter98::save_report_sync_pos(const char *body, int32 body_len) {
  // 用基类 AgwMsgOrderReport 解码, 仅提取 partition 和 index (前两个字段)
  lb_counter98::AgwMsgOrderReport msg;
  if (!decode_body(msg, body, body_len)) {
    return; // 解码失败静默跳过
  }
  counter98_report_sync_[msg.partition] = msg.index;
}

// ============================================================
// 客户登录
// ============================================================

int32 counter98::deal_login_req(const LoginReq &req) {
  // 前置校验: AGW 用户必须已登录成功
  if (agw_login_state != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98 age user not login error, fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_AGW;
  }

  // 构建 acc_login_event_info
  acc_login_event_info info;
  std::memset(&info, 0, sizeof(info));
  build_cust_login_event(req, info);

  // 检查是否已登录 (缓存中存在即已登录)
  std::string fa_key(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16));
  if (GwSessionCache::instance().get_session(fa_key)) {
    // 已登录, 直接发起极速柜台登录 (info.session 已在 build_cust_login_event 中赋值)
    delive_fast_counter_login(info);
    return LBAPI_OK;
  }

  // 缓存登录事件 (供 deal_cust_login_ans 回调和极速柜台登录使用)
  pending_logins_[fa_key] = info;

  // 构建并发送客户登录消息 (封装模式, 与 deal_order_req 等业务函数统一)
  // info.session 已在 build_cust_login_event 中赋值
  int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    pending_logins_.erase(fa_key);
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "98counter deal_login_req: queue full, pos=" << pos << ", fund_account=" << std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()))
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  int32 msg_len = build_login_msg(info, data + sizeof(link_send_event), take_len);
  if (msg_len <= 0) {
    pending_logins_.erase(fa_key);
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_login_req: build_login_msg failed" << end_log;
    return LBAPI_ERR_LOGIN_FAIL;
  }
  reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

  cmt_req_que_mem(pos, take_len);

  return LBAPI_OK;
}

void counter98::build_cust_login_event(const LoginReq &req, acc_login_event_info &o_info) {
  o_info.cust_req_no = req.client_req_no;
  // 注意：fund_account_id/cust_id 等为定长字段，可能占满字段（无 '\0' 结尾）。
  std::memcpy(o_info.cust_id, req.cust_id.data(),
              std::min<size_t>(sizeof(o_info.cust_id), req.cust_id.size()));
  std::memcpy(o_info.fund_account_id, req.fund_account_id.data(),
              std::min<size_t>(sizeof(o_info.fund_account_id), req.fund_account_id.size()));
  std::memcpy(o_info.account_id, req.account_id.data(),
              std::min<size_t>(sizeof(o_info.account_id), req.account_id.size()));
  // branch_id 定长 10 字节，FTE 端 CopyToArray 用空格填充；此处同样用空格填充以便 key 匹配
  std::memset(o_info.branch_id, ' ', sizeof(o_info.branch_id));
  std::memcpy(o_info.branch_id, req.branch_id.data(),
              std::min<size_t>(sizeof(o_info.branch_id), req.branch_id.size()));

  o_info.order_way_ext[0] = req.order_way_ext[0];
  o_info.order_way_ext[1] = req.order_way_ext[1];
  std::memcpy(o_info.session, agw_session, sizeof(o_info.session));
  std::memcpy(o_info.password, req.password.data(),
              std::min<size_t>(sizeof(o_info.password), req.password.size()));
  std::memcpy(o_info.user_info, req.user_info.data(),
              std::min<size_t>(sizeof(o_info.user_info), req.user_info.size()));
  std::memcpy(o_info.client_feature_code, req.client_feature_code.data(),
              std::min<size_t>(sizeof(o_info.client_feature_code), req.client_feature_code.size()));
}



// 重连重登：重新构建 98 柜台登录消息并发送。极速柜台登录将在登录应答成功后触发(deal_cust_login_ans) 
int32 counter98::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len) {
  // 检查是否已登录 (缓存中存在即已登录)
  std::string fa_key(req.fund_account_id, strnlen(req.fund_account_id, 16));
  if (GwSessionCache::instance().get_session(fa_key)) {
    // 已登录, 不需重复登录
    return 0;
  }

  if (buf_len < C98_MAX_MSG_LEN)
    return -1;

  // 构建客户登录消息
  int32 msg_len = build_login_msg(req, o_buf, buf_len);
  return msg_len;
}

int32 counter98::build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len) {
  if (buf_len < static_cast<int32>(sizeof(raw_agw_header)) + 512)
    return -1;

  lb_counter98::AgwMsgCustLoginReq msg;
  std::memcpy(msg.cust_id.data(), info.cust_id,
              std::min<size_t>(sizeof(msg.cust_id), sizeof(info.cust_id)));
  std::memcpy(msg.fund_account_id.data(), info.fund_account_id,
              std::min<size_t>(sizeof(msg.fund_account_id), sizeof(info.fund_account_id)));
  std::memcpy(msg.branch_id.data(), info.branch_id,
              std::min<size_t>(sizeof(msg.branch_id), sizeof(info.branch_id)));
  std::memcpy(msg.account_id.data(), info.account_id,
              std::min<size_t>(sizeof(msg.account_id), sizeof(info.account_id)));
  msg.client_seq_id = info.cust_req_no;
  msg.client_feature_code = std::string(info.client_feature_code,
      strnlen(info.client_feature_code, sizeof(info.client_feature_code)));
  std::memcpy(msg.user_info.data(), info.user_info,
              std::min<size_t>(sizeof(msg.user_info), sizeof(info.user_info)));
  msg.password = std::string(info.password,
      strnlen(info.password, sizeof(info.password)));
  msg.order_way = 0;
  std::memcpy(msg.order_way_ext.data(), info.order_way_ext,
              std::min<size_t>(sizeof(msg.order_way_ext), sizeof(info.order_way_ext)));

  lb_counter98::SerializeBuffer body_buf;
  if (!encode_body(msg, body_buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgCustLoginReq, 2,
                   body_buf.Begin(), static_cast<uint32_t>(body_buf.Size()));
}


int32 counter98::delive_fast_counter_login(const acc_login_event_info &info) {
  lb_common::lb_log_hand tlh(log_);
  lb_common::que_mth_buf *tq = nullptr;
  link_engine_outop *teng = nullptr;
  int16 tlink_type = 0;

  if (fast_counter_type_ == counter_type::gw_direct) {
    tlink_type = LINK_TYPE_SPEED_TRADE;
    tq = trade_send_queue_;
    teng = trade_eng_op_;
  } else {
    tlink_type = LINK_TYPE_SPEED_GW;
    tq = gw_send_queue_;
    teng = gw_eng_op_;
  }

  if (!tq || !teng)
    return LBAPI_OK;

  char *data = nullptr;
  int32 evt_len = static_cast<int32>(sizeof(link_send_event) + sizeof(acc_login_event_info));
  int64 pos = tq->write_get_mth(data, evt_len);
  if (pos <= 0) {
    error_log(tlh) << "98counter delive_fast_counter_login: queue full, pos=" << pos
                   << ", fund_account=" << std::string(info.fund_account_id, strnlen(info.fund_account_id, sizeof(info.fund_account_id)))
                   << ", client_req_no=" << info.cust_req_no << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  link_send_event *evt = reinterpret_cast<link_send_event *>(data);
  evt->link_type = tlink_type;
  evt->type = LINK_EVENT_TYPE_ACCOUNT_LOGIN;
  evt->data_len = sizeof(acc_login_event_info);
  std::memcpy(data + sizeof(link_send_event), &info, sizeof(acc_login_event_info));
  tq->write_cmt_mth(pos, evt_len);
  teng->trigger_send();
  return LBAPI_OK;
}

void counter98::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg) {
  // 登录成功时创建全局会话缓存
  if (err_ret == 0) {
    GwSessionCache::instance().create_session(req);
  }

  // 构建登录应答回调
  LoginAns ans = {};
  ans.client_req_no = req.cust_req_no;
  std::memcpy(ans.cust_id.data(), req.cust_id,
              std::min<size_t>(sizeof(ans.cust_id), sizeof(req.cust_id)));
  std::memcpy(ans.fund_account_id.data(), req.fund_account_id,
              std::min<size_t>(sizeof(ans.fund_account_id), sizeof(req.fund_account_id)));
  std::memcpy(ans.account_id.data(), req.account_id,
              std::min<size_t>(sizeof(ans.account_id), sizeof(req.account_id)));
  std::memcpy(ans.branch_id.data(), req.branch_id,
              std::min<size_t>(sizeof(ans.branch_id), sizeof(req.branch_id)));
  ans.market_type = market_type;
  ans.err_code = err_ret;
  if (err_ret != 0 && err_msg) {
    std::memcpy(ans.err_msg.data(), err_msg, std::min(std::strlen(err_msg), sizeof(ans.err_msg) - 1));
  }
  cb_mgr_->on_login(ans);
}

void counter98::deal_cust_login_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgCustLoginResp msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cust login ans decode failed" << end_log;
    return;
  }

  // 查找登录事件: 优先从 fund_account_array 提取资金账号
  std::string fa_key;
  if (!msg.fund_account_array.empty()) {
    //ToDos: 如果存在多个资金账户，需要遍历整个 vector
    fa_key = std::string(msg.fund_account_array[0].fund_account_id.data(),
                         strnlen(msg.fund_account_array[0].fund_account_id.data(), 16));
  }
  if (fa_key.empty()) {
    // fund_account_array 为空时按 cust_id 遍历 pending_logins_
    for (auto &kv : pending_logins_) {
      if (std::memcmp(kv.second.cust_id, msg.cust_id.data(),
                      std::min(sizeof(kv.second.cust_id), msg.cust_id.size())) == 0) {
        fa_key = kv.first;
        break;
      }
    }
  }

  auto it = pending_logins_.find(fa_key);
  if (it == pending_logins_.end()) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cust login ans: customer not found" << end_log;
    return;
  }

  // 判断登录是否成功 (通过 key_error_code 是否为空)
  bool login_ok = msg.key_error_code.empty();

  // 回调登录结果
  ans_cust_login(it->second, login_ok ? 0 : LBAPI_ERR_LOGIN_FAIL,
                 login_ok ? nullptr : msg.key_error_msg.c_str());

  // 登录成功, 发起极速柜台登录
  if (login_ok) {
    // info.session 已在 build_cust_login_event 中赋值为 agw_session
    delive_fast_counter_login(it->second);
  }
}

void counter98::build_fast_counter_login_event(const lb_counter98::AgwMsgCustLoginResp &ans,
                                                acc_login_event_info &o_info) {
  std::memcpy(o_info.cust_id, ans.cust_id.data(),
              std::min<size_t>(sizeof(o_info.cust_id), ans.cust_id.size()));
  if (!ans.fund_account_array.empty()) {
   //ToDos: 如果存在多个资金账户，需要遍历整个 vector
    std::memcpy(o_info.fund_account_id, ans.fund_account_array[0].fund_account_id.data(),
              std::min<size_t>(sizeof(o_info.fund_account_id), sizeof(ans.fund_account_array[0].fund_account_id)));
    
    // branch_id 定长 10 字节，FTE 端 CopyToArray 用空格填充；此处同样用空格填充以便 key 匹配
    std::memset(o_info.branch_id, ' ', sizeof(o_info.branch_id));
    std::memcpy(o_info.branch_id, ans.fund_account_array[0].branch_id.data(),
              std::min<size_t>(sizeof(o_info.branch_id), sizeof(ans.fund_account_array[0].branch_id)));

    // account_id: 遍历 account_array, 优先取匹配当前市场的证券账户, 找不到则取第一个
    std::memset(o_info.account_id, ' ', sizeof(o_info.account_id));
    bool account_found = false;
    for (const auto &acct : ans.fund_account_array[0].account_array) {
      if (acct.market_id == static_cast<uint16_t>(market_type)) {
        std::memcpy(o_info.account_id, acct.account_id.data(),
                    std::min<size_t>(sizeof(o_info.account_id), acct.account_id.size()));
        account_found = true;
        break;
      }
    }
    if (!account_found && !ans.fund_account_array[0].account_array.empty()) {
      std::memcpy(o_info.account_id, ans.fund_account_array[0].account_array[0].account_id.data(),
                  std::min<size_t>(sizeof(o_info.account_id), ans.fund_account_array[0].account_array[0].account_id.size()));
    }

  }
  o_info.cust_req_no = ans.client_seq_id;
  std::memcpy(o_info.session, agw_session, sizeof(o_info.session));

  // 拷贝密码，用于后续 FTE 登录验证
  // AgwMsgCustLoginResp 中密码字段为 pwd_ciphertext (std::string)
  std::memset(o_info.password, 0, sizeof(o_info.password));
  if (!ans.pwd_ciphertext.empty()) {
    std::memcpy(o_info.password, ans.pwd_ciphertext.data(),
                std::min<size_t>(sizeof(o_info.password) - 1, ans.pwd_ciphertext.size()));
  }

  // user_info: std::array<char, 64> → char[64]
  std::memcpy(o_info.user_info, ans.user_info.data(),
              std::min<size_t>(sizeof(o_info.user_info), ans.user_info.size()));

  // order_way_ext / client_feature_code: AgwMsgCustLoginResp 无对应字段，置 0
  std::memset(o_info.order_way_ext, 0, sizeof(o_info.order_way_ext));
  std::memset(o_info.client_feature_code, 0, sizeof(o_info.client_feature_code));
}

// ============================================================
// deal_*_ans — 应答处理
// ============================================================

void counter98::deal_trade_ter(const char *body, int32 body_len) {
  // 成交执行报告
  lb_counter98::AgwMsgCashAuctionTradeER msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_trade_ter: trade ER decode failed" << end_log;
    return;
  }

  TradeRtn rtn = {};
  rtn.cust_id = msg.cust_id;
  rtn.fund_account_id = msg.fund_account_id;
  rtn.account_id = msg.account_id;
  rtn.branch_id = msg.branch_id;
  rtn.security_id = msg.security_id;
  rtn.side = msg.side;
  rtn.market_type = static_cast<int16_t>(msg.market_id);
  rtn.order_price = msg.price;
  rtn.order_qty = msg.order_qty;
  rtn.client_seq_id = msg.client_seq_id;
  rtn.order_status = static_cast<uint16_t>(msg.order_status);
  rtn.trade_qty = msg.cum_qty;
  rtn.cancel_qty = msg.order_qty - msg.leaves_qty - msg.cum_qty;
  rtn.fee = msg.fee;
  rtn.frozen_amount = 0;
  rtn.order_sys_no = 0; // 从 cl_ord_id 转换或使用 order_id
  rtn.order_time = msg.transact_time;
  rtn.exec_time = msg.transact_time;
  rtn.exec_id = msg.exec_id;
  rtn.exec_price = msg.last_px;
  rtn.exec_qty = msg.last_qty;
  rtn.exec_amount = msg.total_value_traded;
  rtn.exec_fee = 0;

  StreamInfo si = {};
  si.counter_type = STREAM_API_COUNTER;
  si.stream_seq = 0;
  cb_mgr_->on_trade_rtn(si, rtn);
}

void counter98::deal_order_ack(const char *body, int32 body_len) {
  // 委托状态响应 (kAgwMsgOrderStatusAck)
  lb_counter98::AgwMsgOrderStatusAck msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_order_ack: status ack decode failed" << end_log;
    return;
  }

  OrderRtn rtn = {};
  rtn.cust_id = msg.cust_id;
  rtn.fund_account_id = msg.fund_account_id;
  rtn.account_id = msg.account_id;
  rtn.branch_id = msg.branch_id;
  rtn.security_id = msg.security_id;
  rtn.side = msg.side;
  rtn.order_type = msg.ord_type;
  rtn.market_type = static_cast<int16_t>(msg.market_id);
  rtn.order_price = msg.price;
  rtn.order_qty = msg.order_qty;
  rtn.client_seq_id = msg.client_seq_id;
  rtn.order_status = static_cast<uint16_t>(msg.order_status);
  rtn.rtn_type = RSP_TYPE_COUNTER_RSP;
  rtn.err_code = msg.key_error_code.empty() ? 0 : LBAPI_ERR_INVALID_PARAM;
  rtn.order_sys_no = 0;
  rtn.frozen_amount = msg.frozen_trade_value;
  rtn.fee = msg.frozen_fee;
  rtn.trade_qty = msg.cum_qty;
  rtn.cancel_qty = msg.order_qty - msg.leaves_qty - msg.cum_qty;
  rtn.order_time = msg.transact_time;
  rtn.update_time = msg.transact_time;

  StreamInfo si = {};
  si.counter_type = STREAM_API_COUNTER;
  si.stream_seq = 0;
  cb_mgr_->on_order_rtn(si, rtn);
}

void counter98::deal_cancel_ans(const char *body, int32 body_len) {
  // 撤单应答使用 AgwMsgOrderStatusAck (撤单也是委托的一种, 返回状态响应)
  lb_counter98::AgwMsgOrderStatusAck msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_cancel_ans: decode failed" << end_log;
    return;
  }

  CancelRsp rsp = {};
  rsp.cust_id = msg.cust_id;
  rsp.fund_account_id = msg.fund_account_id;
  rsp.account_id = msg.account_id;
  rsp.branch_id = msg.branch_id;
  rsp.market_type = static_cast<int16_t>(msg.market_id);
  rsp.client_seq_id = msg.client_seq_id;
  rsp.order_sys_no = msg.orig_cli_ord_no;
  rsp.err_code = msg.key_error_code.empty() ? 0 : LBAPI_ERR_INVALID_PARAM;
  rsp.rej_api = 0;

  StreamInfo si = {};
  si.counter_type = STREAM_API_COUNTER;
  si.stream_seq = 0;
  cb_mgr_->on_cancel_rsp(si, rsp);
}

void counter98::deal_order_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultOrder msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_order_query_ans: decode failed" << end_log;
    return;
  }

  // 将查询结果转换为 OrderRtn 数组
  int32 count = static_cast<int32>(msg.order_array.size());
  if (count <= 0) {
    QueryAnsCtl ctl = {};
    ctl.count = 0;
    ctl.is_last = 1;
    ctl.client_req_no = msg.client_seq_id;
    OrderRtn dummy = {};
    cb_mgr_->on_order_query_ans(&dummy, ctl);
    return;
  }

  // 按实际数量分配
  std::vector<OrderRtn> rtn_arr(count);

  for (int32 i = 0; i < count; i++) {
    auto &item = msg.order_array[i];
    OrderRtn &rtn = rtn_arr[i];
    std::memset(&rtn, 0, sizeof(rtn));
    rtn.security_id = item.security_id;
    rtn.account_id = item.account_id;
    rtn.branch_id = item.branch_id;
    rtn.side = item.side;
    rtn.order_type = item.ord_type;
    rtn.market_type = static_cast<int16_t>(item.market_id);
    rtn.order_status = static_cast<uint16_t>(item.ord_status);
    rtn.order_price = item.order_price;
    rtn.order_qty = item.order_qty;
    rtn.client_seq_id = item.client_seq_id;
    rtn.trade_qty = item.cum_qty;
    rtn.cancel_qty = item.cancel_qty;
    rtn.frozen_amount = item.frozen_trade_value;
    rtn.fee = item.frozen_fee;
    rtn.order_time = item.transact_time;
    rtn.update_time = item.transact_time;
    rtn.order_sys_no = 0;
  }

  QueryAnsCtl ctl = {};
  ctl.count = count;
  ctl.is_last = (msg.last_index + 1 >= msg.total_num) ? 1 : 0;
  ctl.client_req_no = msg.client_seq_id;
  cb_mgr_->on_order_query_ans(rtn_arr.data(), ctl);
}

void counter98::deal_trade_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultTradeOrder msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_trade_query_ans: decode failed" << end_log;
    return;
  }

  int32 count = static_cast<int32>(msg.order_array.size());
  if (count <= 0) {
    QueryAnsCtl ctl = {};
    ctl.count = 0;
    ctl.is_last = 1;
    ctl.client_req_no = msg.client_seq_id;
    TradeInfo dummy = {};
    cb_mgr_->on_trade_query_ans(&dummy, ctl);
    return;
  }

  // 按实际数量分配
  std::vector<TradeInfo> info_arr(count);

  for (int32 i = 0; i < count; i++) {
    auto &item = msg.order_array[i];
    TradeInfo &info = info_arr[i];
    std::memset(&info, 0, sizeof(info));
    info.security_id = item.security_id;
    info.account_id = item.account_id;
    info.branch_id = msg.branch_id;   // 注意：这里你用的是基类中的branch_id
    info.side = item.side;
    info.order_type = item.ord_type;
    info.market_type = static_cast<int16_t>(item.market_id);
    info.client_seq_id = item.cl_ord_no;
    info.exec_time = item.transact_time;
    std::memcpy(info.exec_id, item.exec_id.data(),
                std::min(sizeof(item.exec_id), sizeof(info.exec_id)));
    info.exec_price = item.last_px;
    info.exec_qty = item.last_qty;
    info.exec_amount = item.total_value_traded;
    info.exec_fee = item.fee;
  }

  QueryAnsCtl ctl = {};
  ctl.count = count;
  ctl.is_last = (msg.last_index + 1 >= msg.total_num) ? 1 : 0;
  ctl.client_req_no = msg.client_seq_id;
  cb_mgr_->on_trade_query_ans(info_arr.data(), ctl);
}

void counter98::deal_fund_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultFund msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_fund_query_ans: decode failed" << end_log;
    return;
  }

  CustFundInfo info = {};
  info.client_req_no = msg.client_seq_id;
  info.cust_id = msg.cust_id;
  info.fund_account_id = msg.fund_account_id;
  info.branch_id = msg.branch_id;
  info.market_type = market_type;
  info.avail_amount = msg.available_t0;
  info.token_amount = msg.leaves_value;

  cb_mgr_->on_fund_query_ans(info);
}

void counter98::deal_position_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultSelfShare msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_position_query_ans: decode failed" << end_log;
    return;
  }

  int32 count = static_cast<int32>(msg.order_array.size());
  if (count <= 0) {
    QueryAnsCtl ctl = {};
    ctl.count = 0;
    ctl.is_last = 1;
    ctl.client_req_no = msg.client_seq_id;
    CustPositionInfo dummy = {};
    cb_mgr_->on_position_query_ans(&dummy, ctl);
    return;
  }

  // 按实际数量分配(自研股份查询最多约500条)
  std::vector<CustPositionInfo> info_arr(count);

  for (int32 i = 0; i < count; i++) {
    auto &item = msg.order_array[i];
    CustPositionInfo &info = info_arr[i];
    std::memset(&info, 0, sizeof(info));
    info.client_req_no = msg.client_seq_id;
    info.cust_id = msg.cust_id;
    info.fund_account_id = msg.fund_account_id;
    info.account_id = item.account_id;
    info.branch_id = msg.branch_id;
    info.market_type = static_cast<int16_t>(item.market_id);
    info.security_id = item.security_id;
    info.security_symbol = item.security_symbol;
    info.security_type = item.security_type;
    info.currency = item.currency;
    info.pbu_id = item.pbu_id;
    info.stk_bal = item.stk_bal;
    info.stk_avl = item.stk_avl;
    info.stk_can_trans = item.stk_can_trans;
    info.stk_frz = item.stk_frz;
    info.leaves_qty = item.leaves_qty;
    info.market_value = item.market_value;
    info.cost_price = item.cost_price;
    info.profit_cost = item.profit_cost;
    info.pro_income = item.pro_income;
    info.income = item.income;
    info.profit_rate = item.profit_rate;
    info.limit_up = item.limit_up;
    info.limit_down = item.limit_down;
  }

  QueryAnsCtl ctl = {};
  ctl.count = count;
  ctl.is_last = (msg.last_index + 1 >= msg.total_num) ? 1 : 0;
  ctl.client_req_no = msg.client_seq_id;
  cb_mgr_->on_position_query_ans(info_arr.data(), ctl);
}

void counter98::deal_max_order_qty_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryMaxOrderQtyResult msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_max_order_qty_query_ans: decode failed" << end_log;
    return;
  }

  CustMaxOrderQtyInfo info = {};
  info.client_req_no = msg.client_seq_id;
  info.cust_id = msg.cust_id;
  info.fund_account_id = msg.fund_account_id;
  info.branch_id = msg.branch_id;
  info.account_id = msg.account_id;
  info.max_order_qty = msg.max_order_qty;
  info.theoretical_order_qty = msg.theoretical_order_qty;
  info.odd_order_qty = msg.odd_order_qty;
  info.limit_flag = msg.limit_flag;
  info.jj_max_limit_qty = msg.jj_max_limit_qty;

  cb_mgr_->on_max_order_qty_query_ans(info);
}

void counter98::deal_total_fund_asset_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultTotalFundAsset msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_total_fund_asset_query_ans: decode failed" << end_log;
    return;
  }

  QueryAnsCtl ctl = {};
  ctl.client_req_no = msg.client_seq_id;
  ctl.count = static_cast<int32_t>(msg.order_array.size());
  ctl.is_last = 1; // 总资产查询无分页，一次返回全部

  std::vector<CustTotalFundAssetInfo> info_arr;
  info_arr.resize(msg.order_array.size());
  for (size_t i = 0; i < msg.order_array.size(); ++i) {
    const auto &src = msg.order_array[i];
    auto &dst = info_arr[i];
    dst.client_req_no = msg.client_seq_id;
    dst.cust_id = msg.cust_id;
    dst.fund_account_id = msg.fund_account_id;
    dst.branch_id = msg.branch_id;
    dst.currency = src.currency;
    dst.fund_bal = src.fund_bal;
    dst.fund_avl = src.fund_avl;
    dst.fund_asset_total_new = src.fund_asset_total_new;
    dst.fund_asset_total = src.fund_asset_total;
    dst.fund_asset = src.fund_asset;
    dst.fund_asset_ggt = src.fund_asset_ggt;
    dst.fund_asset_stktotal = src.fund_asset_stktotal;
    dst.fund_asset_stk = src.fund_asset_stk;
    dst.fund_asset_bond_total = src.fund_asset_bond_total;
    dst.fund_asset_bond = src.fund_asset_bond;
    dst.fund_asset_rzgh = src.fund_asset_rzgh;
    dst.fund_asset_rqgh = src.fund_asset_rqgh;
    dst.fund_asset_stkzy = src.fund_asset_stkzy;
    dst.fund_asset_rzcs = src.fund_asset_rzcs;
    dst.fund_asset_rqcs = src.fund_asset_rqcs;
    dst.fund_asset_stkxyzy = src.fund_asset_stkxyzy;
    dst.fund_asset_adjust = src.fund_asset_adjust;
    dst.fund_asset_oftotal = src.fund_asset_oftotal;
    dst.fund_asset_of = src.fund_asset_of;
    dst.fund_asset_bjhgsh = src.fund_asset_bjhgsh;
    dst.fund_asset_bjhgsz = src.fund_asset_bjhgsz;
    dst.fund_asset_ofuncome = src.fund_asset_ofuncome;
    dst.fund_asset_xjgj = src.fund_asset_xjgj;
    dst.fund_asset_zfzz = src.fund_asset_zfzz;
    dst.fund_asset_jdlsec = src.fund_asset_jdlsec;
    dst.fund_asset_fjjj = src.fund_asset_fjjj;
    dst.fund_asset_curdayrqgh = src.fund_asset_curdayrqgh;
    dst.fund_asset_curdayrzgh = src.fund_asset_curdayrzgh;
    dst.fund_asset_otc_end = src.fund_asset_otc_end;
    dst.debts_total = src.debts_total;
    dst.zy_mtkvalue = src.zy_mtkvalue;
    dst.zy_debts = src.zy_debts;
    dst.en_mktvalue = src.en_mktvalue;
    dst.en_debts = src.en_debts;
    dst.zywy_mktvalue = src.zywy_mktvalue;
    dst.fund_asset_matchrqgh = src.fund_asset_matchrqgh;
    dst.sum_profit_cost = src.sum_profit_cost;
    dst.sump_rofit_amt = src.sump_rofit_amt;
    dst.fund_asset_stkwxxj = src.fund_asset_stkwxxj;
    dst.fund_asset_adjust_xsf = src.fund_asset_adjust_xsf;
    dst.fund_asset_adjust_qt = src.fund_asset_adjust_qt;
    dst.fund_asset_adjust_hgt = src.fund_asset_adjust_hgt;
    dst.fund_asset_adjust_lof = src.fund_asset_adjust_lof;
    dst.fund_asset_stk_only = src.fund_asset_stk_only;
    dst.fund_asset_adjust_etf = src.fund_asset_adjust_etf;
    dst.trade_date = src.trade_date;
  }

  cb_mgr_->on_total_fund_asset_query_ans(info_arr.data(), ctl);
}

void counter98::deal_fund_maxdraw_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultFundMaxdraw msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_fund_maxdraw_query_ans: decode failed" << end_log;
    return;
  }

  QueryAnsCtl ctl = {};
  ctl.client_req_no = msg.client_seq_id;
  ctl.count = static_cast<int32_t>(msg.order_array.size());
  ctl.is_last = 1; // 可取资金查询无分页，一次返回全部

  std::vector<CustFundMaxdrawInfo> info_arr;
  info_arr.resize(msg.order_array.size());
  for (size_t i = 0; i < msg.order_array.size(); ++i) {
    const auto &src = msg.order_array[i];
    auto &dst = info_arr[i];
    dst.client_req_no = msg.client_seq_id;
    dst.cust_id = msg.cust_id;
    dst.fund_account_id = msg.fund_account_id;
    dst.branch_id = msg.branch_id;
    dst.currency = src.currency;
    dst.fund_bal = src.fund_bal;
    dst.fund_avl = src.fund_avl;
    dst.fund_buy = src.fund_buy;
    dst.fund_sale = src.fund_sale;
    dst.market_value = src.market_value;
    dst.fund_all = src.fund_all;
    dst.fund_seq = src.fund_seq;
    dst.stk_value = src.stk_value;
    dst.max_draw = src.max_draw;
    // dst.net_addr = src.net_addr;       // 终端MAC地址(暂不开启)
    dst.bank_code = src.bank_code;
    dst.fund_bjhg_avl = src.fund_bjhg_avl;
    dst.credit_flag = src.credit_flag;
    dst.stk_value_non_of_cash = src.stk_value_non_of_cash;
    dst.of_cash = src.of_cash;
  }

  cb_mgr_->on_fund_maxdraw_query_ans(info_arr.data(), ctl);
}

void counter98::deal_cust_margin_avl_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultCustMarginAvl msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_cust_margin_avl_query_ans: decode failed" << end_log;
    return;
  }

  CustMarginAvlInfo info = {};
  info.client_req_no = msg.client_seq_id;
  info.cust_id = msg.cust_id;
  info.fund_account_id = msg.fund_account_id;
  info.branch_id = msg.branch_id;
  info.credit_direct = msg.credit_direct;
  info.margin_avl = msg.margin_avl;
  info.credit_bal = msg.credit_bal;
  info.credit_avl = msg.credit_avl;
  info.margin_rate = msg.margin_rate;
  info.debts_bal = msg.debts_bal;
  info.max_amt = msg.max_amt;
  info.fund_avl = msg.fund_avl;
  info.credit_fund_avl = msg.credit_fund_avl;
  info.g_dis_market_avl = msg.g_dis_market_avl;
  info.mar_stk_val = msg.mar_stk_val;
  info.total_dis_f_mk_val = msg.total_dis_f_mk_val;
  info.mar_fund_profit = msg.mar_fund_profit;
  info.mar_stk_profit = msg.mar_stk_profit;
  info.total_rate_f_debts_bal = msg.total_rate_f_debts_bal;
  info.mar_fund_margin = msg.mar_fund_margin;
  info.mar_fund_margin_frz = msg.mar_fund_margin_frz;
  info.total_rate_d_sale_bal = msg.total_rate_d_sale_bal;
  info.mar_stk_margin = msg.mar_stk_margin;
  info.mar_stk_margin_frz = msg.mar_stk_margin_frz;
  info.total_fee = msg.total_fee;
  info.market_id = msg.market_id;
  info.security_id = msg.security_id;
  info.security_symbol = msg.security_symbol;
  info.margin_fund_bal = msg.margin_fund_bal;
  info.margin_credit_bal = msg.margin_credit_bal;
  info.fund_margin_avl = msg.fund_margin_avl;
  info.fund_margin_avl_sub = msg.fund_margin_avl_sub;

  cb_mgr_->on_cust_margin_avl_query_ans(info);
}

void counter98::deal_asset_debts_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryResultAssetDebts msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_asset_debts_query_ans: decode failed" << end_log;
    return;
  }

  AssetDebtsInfo info = {};
  info.client_req_no = msg.client_seq_id;
  info.cust_id = msg.cust_id;
  info.fund_account_id = msg.fund_account_id;
  info.branch_id = msg.branch_id;
  info.currency = msg.currency;
  info.cust_name = msg.cust_name;
  info.credit_status = msg.credit_status;
  info.keep_rate = msg.keep_rate;
  info.fund = msg.fund;
  info.fund_asset = msg.fund_asset;
  info.stk_asset = msg.stk_asset;
  info.bond_asset = msg.bond_asset;
  info.asset = msg.asset;
  info.old_asset = msg.old_asset;
  info.liability = msg.liability;
  info.fund_bal = msg.fund_bal;
  info.fund_avl = msg.fund_avl;
  info.fund_avl_frz = msg.fund_avl_frz;
  info.d_sale_amt_bal = msg.d_sale_amt_bal;
  info.d_sale_amt_frz = msg.d_sale_amt_frz;
  info.credit_buy = msg.credit_buy;
  info.credit_sale = msg.credit_sale;
  info.credit_buy_sale = msg.credit_buy_sale;
  info.relese_amt_unfrz = msg.relese_amt_unfrz;
  info.credit_amt = msg.credit_amt;
  info.hbjj_bal = msg.hbjj_bal;
  info.hbjj_buy = msg.hbjj_buy;
  info.hbjj_sale = msg.hbjj_sale;
  info.hbjj_amt = msg.hbjj_amt;
  info.hbjj_remain_amt = msg.hbjj_remain_amt;
  info.gage_mkt_avl = msg.gage_mkt_avl;
  info.gage_mkt_avl_zs = msg.gage_mkt_avl_zs;
  info.gage_mkt_avl_db = msg.gage_mkt_avl_db;
  info.rights_amt = msg.rights_amt;
  info.f_deal_avl = msg.f_deal_avl;
  info.fund_fee = msg.fund_fee;
  info.fund_pro_fee = msg.fund_pro_fee;
  info.fund_manage_fee = msg.fund_manage_fee;
  info.fund_over_due_fee = msg.fund_over_due_fee;
  info.fund_puni_fee = msg.fund_puni_fee;
  info.fund_puni_debts = msg.fund_puni_debts;
  info.fund_manage_over_due = msg.fund_manage_over_due;
  info.fund_manage_puni_ntr = msg.fund_manage_puni_ntr;
  info.f_fee = msg.f_fee;
  info.f_total_debts = msg.f_total_debts;
  info.deal_f_mkt_avl = msg.deal_f_mkt_avl;
  info.stk_fee = msg.stk_fee;
  info.stk_pro_fee = msg.stk_pro_fee;
  info.stk_manage_fee = msg.stk_manage_fee;
  info.stk_over_due_fee = msg.stk_over_due_fee;
  info.stk_puni_fee = msg.stk_puni_fee;
  info.over_due_rights = msg.over_due_rights;
  info.puni_rights = msg.puni_rights;
  info.stk_puni_debts = msg.stk_puni_debts;
  info.stk_manage_over_due = msg.stk_manage_over_due;
  info.stk_manage_puni_ntr = msg.stk_manage_puni_ntr;
  info.d_fee = msg.d_fee;
  info.d_total_debts = msg.d_total_debts;
  info.fund_draw = msg.fund_draw;
  info.fill_amt = msg.fill_amt;
  info.credit_bal = msg.credit_bal;
  info.credit_avl = msg.credit_avl;
  info.f_credit_bal = msg.f_credit_bal;
  info.f_credit_avl = msg.f_credit_avl;
  info.d_credit_bal = msg.d_credit_bal;
  info.d_credit_avl = msg.d_credit_avl;
  info.total_profit_cost = msg.total_profit_cost;
  info.stk_unused_fee = msg.stk_unused_fee;
  info.stk_puni_unused_fee = msg.stk_puni_unused_fee;
  info.fund_margin_frz = msg.fund_margin_frz;
  info.stk_margin_frz = msg.stk_margin_frz;
  info.market_value = msg.market_value;
  info.fund_seq = msg.fund_seq;
  info.stk_value = msg.stk_value;
  info.d_sale_bal = msg.d_sale_bal;
  info.credit_frz_amt = msg.credit_frz_amt;
  info.long_profit_cost = msg.long_profit_cost;
  info.short_profit_cost = msg.short_profit_cost;
  info.lever_rate = msg.lever_rate;
  info.asset_frz_package_out = msg.asset_frz_package_out;
  info.asset_frz_package_in = msg.asset_frz_package_in;
  info.trade_date = msg.trade_date;
  info.max_draw = msg.max_draw;
  info.max_draw2 = msg.max_draw2;

  cb_mgr_->on_asset_debts_query_ans(info);
}

void counter98::deal_credit_max_order_qty_query_ans(const char *body, int32 body_len) {
  lb_counter98::AgwMsgExtQueryCreditMaxOrderQtyResult msg;
  if (!decode_body(msg, body, body_len)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "deal_credit_max_order_qty_query_ans: decode failed" << end_log;
    return;
  }

  CreditMaxOrderQtyInfo info = {};
  info.client_req_no = msg.client_seq_id;
  info.cust_id = msg.cust_id;
  info.fund_account_id = msg.fund_account_id;
  info.branch_id = msg.branch_id;
  info.account_id = msg.account_id;
  info.business_type = msg.business_type;
  info.side = msg.side;
  info.max_order_qty = msg.max_order_qty;
  info.theoretical_order_qty = msg.theoretical_order_qty;
  info.max_pay_qty = msg.max_pay_qty;
  info.max_pay_amt = msg.max_pay_amt;
  info.sep_rem_qty = msg.sep_rem_qty;
  info.fund_credit_avl = msg.fund_credit_avl;
  info.stk_debts_avl = msg.stk_debts_avl;
  info.sg_max_stk_qty = msg.sg_max_stk_qty;
  info.zero_reason = msg.zero_reason;
  info.limit_flag = msg.limit_flag;
  info.jj_max_limit_qty = msg.jj_max_limit_qty;

  cb_mgr_->on_credit_max_order_qty_query_ans(info);
}

// ============================================================
// 心跳
// ============================================================

int32 counter98::build_heart_msg(char *o_buf, int32 buf_len) {
  if (buf_len < static_cast<int32>(sizeof(raw_agw_header)) + 32)
    return -1;

  lb_counter98::AgwMsgHeartBeat msg;
  // 构造函数已将 test_req_id 填充为空格(' '), 无需额外 fill(0)

  lb_counter98::SerializeBuffer body_buf;
  if (!encode_body(msg, body_buf))
    return -1;

  return write_msg(o_buf, lb_counter98::kAgwMsgHeartBeat, 0,
                   body_buf.Begin(), static_cast<uint32_t>(body_buf.Size()));
}

// ============================================================
// 链接管理
// ============================================================

int32 counter98::deal_link_connect(int16 link_type, int32 have_switch) {
  if (link_type != LINK_TYPE_98)
    return LBAPI_OK;

  trade_link_connect_ = 1;

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "counter98 link connected" << end_log;

  // 通知链接状态
  cb_mgr_->on_link_status(0, 0, 1);

  // 发生地址切换时, 重新发送 AGW 登录消息
  if (have_switch != 0) {
    info_log(tlh) << "98 link address switched, re-send agw user login" << end_log;

    // 重置登录状态
    lb_common::atomic_store16(&agw_login_state, 0);

    // 申请队列内存
    int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;
    char *data = nullptr;
    int64 pos = take_req_que_mem(data, take_len);
    if (unlikely(pos <= 0)) {
      error_log(tlh) << "re-send agw login: queue full, ret=" << pos << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    // 组包
    int32 msg_len = build_agw_login_msg(data + sizeof(link_send_event), take_len);
    if (msg_len <= 0) {
      error_log(tlh) << "re-send agw login: build msg failed" << end_log;
      return LBAPI_ERR_BUILD_MSG;
    }
    reinterpret_cast<link_send_event *>(data)->data_len = msg_len;

    // 设置登录中状态并提交发送
    lb_common::atomic_store16(&agw_login_state, 1);
    cmt_req_que_mem(pos, take_len);
  }
  return LBAPI_OK;
}

void counter98::deal_link_close(int16 link_type) {
  if (link_type != LINK_TYPE_98)
    return;

  trade_link_connect_ = 0;
  lb_common::atomic_store16(&agw_login_state, 0);

  // 清除全局会话缓存和登录事件
  GwSessionCache::instance().clear_all();
  pending_logins_.clear();
  std::memset(agw_session, 0, sizeof(agw_session));

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "counter98 link closed" << end_log;

  cb_mgr_->on_link_status(0, 0, 0);
}

} // namespace lb_api
>>>>>>> .r17

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
void deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret) {
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
  char *data = nullptr;
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
  lb_common::comm_utils::str_copy_format(o_info.cust_id, req.cust_id.data(), sizeof(o_info.cust_id));
  lb_common::comm_utils::str_copy_format(o_info.fund_account_id, req.fund_account_id.data(),
                                         sizeof(o_info.fund_account_id));
  lb_common::comm_utils::str_copy_format(o_info.account_id, req.account_id.data(), sizeof(o_info.account_id));
  lb_common::comm_utils::str_copy_format(o_info.branch_id, req.branch_id.data(), sizeof(o_info.branch_id));

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
  bool cust_need_login = false;
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
  std::memcpy(body->session, info.session, sizeof(body->session));
  std::memcpy(body->end_code, info.client_feature_code, sizeof(body->end_code));
  body->market_type = market_type;
  body->heart_bt_int = static_cast<uint16>(heart_interval);
  body->order_way[0] = info.order_way_ext[0];
  body->order_way[1] = info.order_way_ext[1];
  std::memcpy(body->session, info.session, sizeof(body->session));

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
  lb_common::comm_utils::str_copy_format(o_info.cust_id, msg.cust_id, sizeof(o_info.cust_id));
  lb_common::comm_utils::str_copy_format(o_info.fund_account_id, msg.fund_account_id, sizeof(o_info.fund_account_id));
  lb_common::comm_utils::str_copy_format(o_info.account_id, msg.account_id, sizeof(o_info.account_id));
  lb_common::comm_utils::str_copy_format(o_info.branch_id, msg.branch_id, sizeof(o_info.branch_id));

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

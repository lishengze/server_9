// counter98 - 98 柜台协议对象 (骨架)
//
// 负责 98 柜台的消息组包、解析、状态管理、查询应答处理.
// 柜台对外只提供业务 API 接口对应的发送函数, 不暴露 build 消息函数.
// 业务发送函数内部完成组包 + 入队 (que_mth_buf::write_get_mth).

#pragma once

#include "api_config.h"
#include "api_config_impl.h"
#include "api_event_msg.h"
#include "c98msg_tmp.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "mlog.h"
#include "order_trade_type.h"
#include "que_mth_buf.h"
#include <pthread.h>

#include <cstdint>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// 98 柜台协议对象
class counter98 {
public:
  /// 获取柜台类型(恒为 fixed_98)
  FORCE_INLINE int32 get_counter_type() const { return static_cast<int32_t>(counter_type::fixed_98); }
  /// 原子读取会话消息序号
  FORCE_INLINE int64 get_session_seq_no() const { return lb_common::atomic_load64(&session_seq_); }

  /// api instance 调用，在 init 中
  int32 init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);

  void init_trade(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {
    trade_send_queue_ = que;
    trade_eng_op_ = link_outop;
  }
  void init_gateway(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {
    gw_send_queue_ = que;
    gw_eng_op_ = link_outop;
  }

  /// api instance 调用，处理买卖委托
  int32 deal_order_req(const OrderReq &req);

  /// api instance 调用，处理ETF申购赎回委托
  int32 deal_etf_order_req(const OrderReq &req);

  /// api instance 调用，处理北交所委托
  int32 deal_bse_order_req(const OrderReq &req);

  /// api instance 调用，处理委托撤单
  int32_t deal_cancel_req(const CancelReq &req);

  /// api instance 调用，处理客户委托查询
  int32 deal_order_query(const OrderQueryReq &req);

  /// api instance 调用，处理客户委托批量查询
  int32 deal_order_batch_query(const OrderBatchQueryReq &req);

  /// api instance 调用，处理客户成交查询
  int32 deal_trade_query(const TradeQueryReq &req);

  /// api instance 调用，处理客户成交批量查询
  int32 deal_trade_batch_query(const TradeBatchQueryReq &req);

  /// api instance 调用，处理客户资金查询
  int32 deal_fund_query(const FundQueryReq &req);

  /// api instance 调用，处理客户持仓查询
  int32 deal_position_query(const PositionQueryReq &req);

  /// engine 调用，接收消息处理（由 link 回调, link_type 从哪个类型链接接收）
  /// 可能有多个完整消息+不完整消息，需要依据消息头一个个解析，确认一个个什么业务并处理
  /// 返回成功解析处理的长度，出错若需要可返回<0,底层会关闭链接
  /// @param link_type 链接类型（98 必为 LINK_TYPE_98）
  int32 deal_recv_msg(const char *buf, int32 len, int16 link_type);

  /// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
  void deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret);

  /// api instance 调用，在 start 中，启动线程前
  /// 发起 agw 用户登陆事件，入队，并同步等待登陆结果（sleep 循环检查 agw_log_state）
  /// agw_user_login_timeout 超时返回 LBAPI_ERR_LOGIN_98_TIMEOUT
  int32 deal_agw_login();

  /// multi engine 调用，处理 agw 用户登陆事件
  /// 成功返回消息长度，含消息头
  int32 build_agw_login_msg(char *o_buf, int32 buf_len);

  void ans_agwuser_login(int32 err_code, const char *err_msg);

  int32 deal_login_req(const LoginReq &req);
  /// engine 调用，处理账户登陆事件,检查是否需要登陆，若是构造登陆消息到o_buf;若否，发起极速柜台登陆事件，成功返回0
  /// 成功返回消息长度，含消息头，0-不需重复登陆,<0 出错
  int32 deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len);

  void ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg);

  /// engine 调用，处理心跳发送事件
  /// 返回消息长度，含消息头
  int32 build_heart_msg(char *o_buf, int32 buf_len);

  /// engine 调用，处理链接重连是否可建立链接
  /// @param link_type  链接类型 (LINK_TYPE_98 / LINK_TYPE_SPEED_GW)
  bool can_link_connect(int16 link_type) {
    if (link_type == LINK_TYPE_98) {
      return true;
    }
    return false;
  }

  /// Link 在 engine 中调用，链接成功时
  /// @param link_type    链接类型
  /// @param have_switch  1=链接建立时发生地址切换 (need_switch=1 + 实际切换)
  /// 返回错误时，底层关闭链接
  int32 deal_link_connect(int16 link_type, int32 have_switch);

  /// Link 在 engine 中调用，链接关闭时
  void deal_link_close(int16 link_type);

  counter98();
  ~counter98();

  /// 禁用拷贝
  counter98(const counter98 &) = delete;
  counter98 &operator=(const counter98 &) = delete;

protected:
  FORCE_INLINE int64 take_req_que_mem(char *&o_buf, int32 take_len) {
    char *data = nullptr;
    int64 pos = gw_send_queue_->write_get_mth(data, take_len + sizeof(link_send_event));
    if (unlikely(pos <= 0)) {
      return pos;
    }
    o_buf = data;

    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_98;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = take_len;
    return pos;
  }
  FORCE_INLINE void cmt_req_que_mem(int64 get_pos, int32 take_len) {
    gw_send_queue_->write_cmt_mth(get_pos, take_len + sizeof(link_send_event));
    gw_eng_op_->trigger_send();
  }

  /// 构造 98 委托消息 (头 + c98_order_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_order_msg(const OrderReq &req, char *o_buf);

  /// 构造 98 ETF 申购赎回消息 (头 + body) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_etf_order_msg(const OrderReq &req, char *o_buf);

  /// 构造 98 北交所 (BSE) 委托消息 (头 + body) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_bse_order_msg(const OrderReq &req, char *o_buf);

  /// 构造 98 撤单消息 (头 + c98_cancel_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_cancel_msg(const CancelReq &req, char *o_buf);

  /// 构造 98 委托查询消息 (头 + c98_query_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_order_query_msg(const OrderQueryReq &req, char *o_buf);

  /// 构造 98 委托批量查询消息 (头 + c98_query_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_order_batch_query_msg(const OrderBatchQueryReq &req, char *o_buf);

  /// 构造 98 成交查询消息 (头 + c98_query_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_trade_query_msg(const TradeQueryReq &req, char *o_buf);

  /// 构造 98 成交批量查询消息 (头 + c98_query_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_trade_batch_query_msg(const TradeBatchQueryReq &req, char *o_buf);

  /// 构造 98 资金查询消息 (头 + c98_fund_query_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_fund_query_msg(const FundQueryReq &req, char *o_buf);

  /// 构造 98 持仓查询消息 (头 + c98_position_query_req) 到 o_buf
  /// (98 真实协议未知, 当前为留空实现)
  void build_position_query_msg(const PositionQueryReq &req, char *o_buf);

  void deal_agwuser_login_ans(const c98_msg_head_tmp *msg);

  int32 build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len);

  void build_cust_login_rtn(const c98_acc_login_ans &msg, LoginAns &o_ans);

  void build_fast_counter_login_event(const c98_acc_login_ans &ans, acc_login_event_info &o_info);

  void build_cust_login_event(const LoginReq &req, acc_login_event_info &o_info);

  void deal_cust_login_ans(const c98_msg_head_tmp *msg);

  int32 delive_fast_counter_login(const acc_login_event_info &info);

private:
  int16 trade_link_connect_ = 0; ///< 98 链接状态:0-未链接/断开, 1-已链接
  int16 agw_login_state = 0;     ///< agw 用户登陆状态:0-未登陆，1-登陆中，2-登陆成功
  int16 market_type = 0;         ///< 市场
  int16 heart_interval = 5;      ///< 心跳间隔
  lb_common::que_mth_buf *gw_send_queue_ = nullptr;    ///< 98柜台发送队列
  link_engine_outop *gw_eng_op_ = nullptr;             ///< 98链接引擎导出的链接相关操作
  lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列

  //todo : 定义 98 柜台缓存结构，添加缓存对象

  counter_type fast_counter_type_ = counter_type::fixed_98; ///< 极速柜台类型
  int32_t agw_user_login_timeout_ = 10;                     ///< AGW 登录超时秒数（默认 10）
  callback_manager *cb_mgr_ = nullptr;                      ///< 回调管理器 (deal_recv_msg 成功后回调用户)
  int64 session_seq_ = 0;                                   ///< 会话消息序号 (原子访问)
  lb_common::lb_log *log_ = nullptr;                        ///< 日志指针 (构造/解析失败时打错误)
  link_engine_outop *trade_eng_op_ = nullptr;               ///< 极速引擎导出的链接相关操作

  char agw_session[32]; ///< agw 用户登陆成功返回的会话号
  // 拷贝自 cfg 的配置项
  std::string agw_user_;     ///< 98 agw 用户名
  std::string agw_password_; ///< 98 agw 密码
};

} // namespace lb_api

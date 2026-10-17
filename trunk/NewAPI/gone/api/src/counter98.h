// counter98 - 98 柜台协议对象 (骨架)
//
// 负责 98 柜台的消息组包、解析、状态管理、查询应答处理.
// 柜台对外只提供业务 API 接口对应的发送函数, 不暴露 build 消息函数.
// 业务发送函数内部完成组包 + 入队 (que_mth_buf::write_get_mth).

#pragma once

#include "api_config.h"
#include "api_config_impl.h"
#include "api_event_msg.h"
#include "counter98/lb_counter98_protocol.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "gw_session_cache.h"
#include "mlog.h"
#include "order_trade_type.h"
#include "que_mth_buf.h"
#include <pthread.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <unordered_map>

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

  /// api instance 调用，处理最大可委托数查询
  int32 deal_max_order_qty_query(const MaxOrderQtyQueryReq &req);

  /// api instance 调用，处理客户总资产查询
  int32 deal_total_fund_asset_query(const TotalFundAssetQueryReq &req);

  /// api instance 调用，处理可取资金查询
  int32 deal_fund_maxdraw_query(const FundMaxdrawQueryReq &req);

  /// api instance 调用，处理融资融券信用数据（可用保证金）查询
  int32 deal_cust_margin_avl_query(const CustMarginAvlQueryReq &req);

  /// api instance 调用，处理资产负债查询
  int32 deal_asset_debts_query(const AssetDebtsQueryReq &req);

  /// api instance 调用，处理两融最大可委托数查询
  int32 deal_credit_max_order_qty_query(const CreditMaxOrderQtyQueryReq &req);

  /// engine 调用，接收消息处理（由 link 回调, link_type 从哪个类型链接接收）
  /// 可能有多个完整消息+不完整消息，需要依据消息头一个个解析，确认一个个什么业务并处理
  /// 返回成功解析处理的长度，出错若需要可返回<0,底层会关闭链接
  /// @param link_type 链接类型（98 必为 LINK_TYPE_98）
  int32 deal_recv_msg(const char *buf, int32 len, int16 link_type);

  /// engine 调用，接收消息处理（Factory 模式版本）
  /// 使用协议自带的 Factory 机制统一创建+解码消息, 替代手动 switch-case 创建.
  /// 保留原 deal_recv_msg 以便对比和回退.
  // int32 deal_recv_msg_factory(const char *buf, int32 len, int16 link_type);

  /// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
  void deal_send_error(char *msg_buf, int32 msg_len, int32 err_ret);

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
    evt->leave_time_ptr = nullptr;  // 98 不参与 send 前时间戳记录（无 api_leave_time_ns 目标）
    return pos;
  }
  FORCE_INLINE void cmt_req_que_mem(int64 get_pos, int32 take_len) {
    gw_send_queue_->write_cmt_mth(get_pos, take_len + sizeof(link_send_event));
    gw_eng_op_->trigger_send();
  }

  /// 组包到临时缓冲后，按实际消息长度精确预留、拷贝、提交到 gw 发送队列。
  /// 避免"预留上限 take_len、实际 msg_len"导致的预留/提交长度不一致，
  /// 从而防止引擎线程把预留空隙读成零长度事件造成错位。
  /// @param temp_buf 已组包的消息缓冲（含 link_send_event 头，消息体在 temp_buf+sizeof 处）
  /// @param msg_len  实际消息体长度(不含 link_send_event 头)
  /// @return 0 成功，<0 队列满
  FORCE_INLINE int32 send_req_msg(char *temp_buf, int32 msg_len) {
    char *data = nullptr;
    int64 pos = gw_send_queue_->write_get_mth(data, msg_len + sizeof(link_send_event));
    if (unlikely(pos <= 0)) {
      return static_cast<int32>(pos);
    }
    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_98;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = msg_len;
    evt->leave_time_ptr = nullptr;  // 98 不参与 send 前时间戳记录（无 api_leave_time_ns 目标）
    if (msg_len > 0) {
      memcpy(data + sizeof(link_send_event), temp_buf + sizeof(link_send_event), msg_len);
    }
    gw_send_queue_->write_cmt_mth(pos, msg_len + sizeof(link_send_event));
    gw_eng_op_->trigger_send();
    return 0;
  }

  /// 构造 98 委托消息 (AgwHeader + AgwMsgCashAuctionOrder) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_order_msg(const OrderReq &req, char *o_buf);

  /// 构造 98 ETF 申购赎回消息 (AgwHeader + body) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_etf_order_msg(const OrderReq &req, char *o_buf);

  /// 构造 98 北交所 (BSE) 委托消息 (AgwHeader + body) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_bse_order_msg(const OrderReq &req, char *o_buf);

  /// 构造 98 撤单消息 (AgwHeader + AgwMsgCancelOrder) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_cancel_msg(const CancelReq &req, char *o_buf);

  /// 构造 98 委托查询消息 (AgwHeader + AgwMsgExtQueryOrder) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_order_query_msg(const OrderQueryReq &req, char *o_buf);

  /// 构造 98 委托批量查询消息 (AgwHeader + body) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_order_batch_query_msg(const OrderBatchQueryReq &req, char *o_buf);

  /// 构造 98 成交查询消息 (AgwHeader + AgwMsgExtQueryTradeOrder) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_trade_query_msg(const TradeQueryReq &req, char *o_buf);

  /// 构造 98 成交批量查询消息 (AgwHeader + body) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_trade_batch_query_msg(const TradeBatchQueryReq &req, char *o_buf);

  /// 构造 98 资金查询消息 (AgwHeader + AgwMsgExtQueryFund) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_fund_query_msg(const FundQueryReq &req, char *o_buf);

  /// 构造 98 自研股份查询消息 (AgwHeader + AgwMsgExtQuerySelfShare) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_position_query_msg(const PositionQueryReq &req, char *o_buf);

  /// 构造 98 最大可委托数查询消息 (AgwHeader + AgwMsgExtQueryMaxOrderQty) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_max_order_qty_query_msg(const MaxOrderQtyQueryReq &req, char *o_buf);

  /// 构造 98 客户总资产查询消息 (AgwHeader + AgwMsgExternalQueryTotalFundAsset) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_total_fund_asset_query_msg(const TotalFundAssetQueryReq &req, char *o_buf);

  /// 构造 98 可取资金查询消息 (AgwHeader + AgwMsgExtQueryFundMaxdraw) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_fund_maxdraw_query_msg(const FundMaxdrawQueryReq &req, char *o_buf);

  /// 构造 98 融资融券信用数据（可用保证金）查询消息 (AgwHeader + AgwMsgExtQueryCustMarginAvl) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_cust_margin_avl_query_msg(const CustMarginAvlQueryReq &req, char *o_buf);

  /// 构造 98 资产负债查询消息 (AgwHeader + AgwMsgExtQueryAssetDebts) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_asset_debts_query_msg(const AssetDebtsQueryReq &req, char *o_buf);

  /// 构造 98 两融最大可委托数查询消息 (AgwHeader + AgwMsgExtQueryCreditMaxOrderQty) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_credit_max_order_qty_query_msg(const CreditMaxOrderQtyQueryReq &req, char *o_buf);

  void deal_agwuser_login_ans(const char *body, int32 body_len);

  /// 构造回报同步消息 (AgwHeader + AgwMsgSync) 到 o_buf
  /// @return 消息总长度(含消息头), <0 出错
  int32 build_sync_msg(char *o_buf, int32 buf_len);

  /// 发送回报同步请求 (AGW 登录成功后调用)
  void send_agw_sync();

  /// 处理回报同步结束应答
  void deal_sync_finish_ans(const char *body, int32 body_len);

  int32 build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len);

  void build_fast_counter_login_event(const lb_counter98::AgwMsgCustLoginResp &ans, acc_login_event_info &o_info);

  void build_cust_login_event(const LoginReq &req, acc_login_event_info &o_info);

  void deal_cust_login_ans(const char *body, int32 body_len);

  /// 处理98委托状态响应 → on_order_rtn 回调
  void deal_order_ack(const char *body, int32 body_len);

  /// 处理98成交执行报告 → on_trade_rtn 回调
  void deal_trade_ter(const char *body, int32 body_len);

  /// 处理98撤单应答 → on_cancel_rsp 回调
  void deal_cancel_ans(const char *body, int32 body_len);

  /// 处理98委托查询应答 → on_order_query_ans 回调
  void deal_order_query_ans(const char *body, int32 body_len);

  /// 处理98成交查询应答 → on_trade_query_ans 回调
  void deal_trade_query_ans(const char *body, int32 body_len);

  /// 处理98资金查询应答 → on_fund_query_ans 回调
  void deal_fund_query_ans(const char *body, int32 body_len);

  /// 处理98自研股份查询应答 → on_position_query_ans 回调
  void deal_position_query_ans(const char *body, int32 body_len);

  /// 处理98最大可委托数查询应答 → on_max_order_qty_query_ans 回调
  void deal_max_order_qty_query_ans(const char *body, int32 body_len);

  /// 处理98客户总资产查询应答 → on_total_fund_asset_query_ans 回调
  void deal_total_fund_asset_query_ans(const char *body, int32 body_len);

  /// 处理98可取资金查询应答 → on_fund_maxdraw_query_ans 回调
  void deal_fund_maxdraw_query_ans(const char *body, int32 body_len);

  /// 处理98融资融券信用数据（可用保证金）查询应答 → on_cust_margin_avl_query_ans 回调
  void deal_cust_margin_avl_query_ans(const char *body, int32 body_len);

  /// 处理98资产负债查询应答 → on_asset_debts_query_ans 回调
  void deal_asset_debts_query_ans(const char *body, int32 body_len);

  /// 处理98两融最大可委托数查询应答 → on_credit_max_order_qty_query_ans 回调
  void deal_credit_max_order_qty_query_ans(const char *body, int32 body_len);

  // ---- 下行消息分发表 (替代 deal_recv_msg 中的 switch-case) ----
  /// deal_* 函数指针类型
  using deal_func = void (counter98::*)(const char *, int32);

  /// 注册下行消息处理函数: (msg_type, msg_ver) → deal_* 函数指针
  void init_dispatch_map();

  /// 复合 key: 高32位=msg_type, 低32位=msg_ver
  static constexpr uint64 make_dispatch_key(uint32 msg_type, uint32 msg_ver) {
    return (static_cast<uint64>(msg_type) << 32) | static_cast<uint64>(msg_ver);
  }

  /// 下行消息分发表
  std::unordered_map<uint64, deal_func> deal_dispatch_map_;

  /// 分发宏 — 在 init_dispatch_map() 中使用
  /// 用法: REGISTER_DEAL_FUNC(kAgwMsgOrderStatusAck, 2, deal_order_ack)
#define REGISTER_DEAL_FUNC(msg_type, msg_ver, func) \
  deal_dispatch_map_[make_dispatch_key(msg_type, msg_ver)] = &counter98::func;

  int32 delive_fast_counter_login(const acc_login_event_info &info);

private:
  int16 trade_link_connect_ = 0; ///< 98 链接状态:0-未链接/断开, 1-已链接
  int16 agw_login_state = 0;     ///< agw 用户登陆状态:0-未登陆，1-登陆中，2-登陆成功
  int16 market_type = 0;         ///< 市场
  int16 heart_interval = 5;      ///< 心跳间隔
  lb_common::que_mth_buf *gw_send_queue_ = nullptr;    ///< 98柜台发送队列
  link_engine_outop *gw_eng_op_ = nullptr;             ///< 98链接引擎导出的链接相关操作
  lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列

  // ---- 登录事件缓存 (fund_account_id → acc_login_event_info) ----
  /// 登录请求发出时缓存, 登录应答到达时取出用于回调和极速柜台登录
  std::unordered_map<std::string, acc_login_event_info> pending_logins_;

  counter_type fast_counter_type_ = counter_type::fixed_98; ///< 极速柜台类型
  int32_t agw_user_login_timeout_ = 10;                     ///< AGW 登录超时秒数（默认 10）
  callback_manager *cb_mgr_ = nullptr;                      ///< 回调管理器 (deal_recv_msg 成功后回调用户)
  lb_common::lb_log *log_ = nullptr;                        ///< 日志指针 (构造/解析失败时打错误)
  link_engine_outop *trade_eng_op_ = nullptr;               ///< 极速引擎导出的链接相关操作

  char agw_session[32]; ///< agw 用户登陆成功返回的会话号
  // 拷贝自 cfg 的配置项
  std::string agw_user_;     ///< 98 agw 用户名
  std::string agw_password_; ///< 98 agw 密码
  int32_t counter98_mode_ = 0; ///< 98柜台登录模式(0=默认, 1=快速登录)
  std::map<uint8_t, int32_t> counter98_report_sync_; ///< 回报同步分区/索引配置
  bool sync_finished_ = false; ///< 回报同步是否已完成 (deal_sync_finish_ans 中置 true)

  /// 回报同步完成后, 保存下行回报消息的 partition/index
  void save_report_sync_pos(const char *body, int32 body_len);
};

} // namespace lb_api

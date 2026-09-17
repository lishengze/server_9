// gw_counter_direct - 个微软件极速柜台 直连模式
//
// 负责个微软件极速柜台 (仅直连模式) 单 TCP 链接的消息组包/解析.
// 个微柜台无查询接口, 与 fpga_counter_gateway 一致 (无 deal_order_query 等).
//
// 协议：FTE TCP Binary（gw_message::* 结构体，大端字节序）
//   - 报文格式：[PktNewHeader 8B | 消息体 | 校验和 4B]
//   - 消息头：PktNewHeader（msg_id + msg_len）
//   - 校验和：GenerateSzCheckSum 对 [头+体] 逐字节求和 %256，转大端追加
//   - 消息类型：1001 登录 / 1003 委托 / 1004 撤单 / 1010 ETF / 2001 登录应答 / 2003/2004/2005/2010 回报 / 3 心跳 / 9 拒绝
//
// 命名风格约定（与 fpga 一致）：
//   - 类名：<prefix>_counter_<mode>  (e.g., fpga_counter_direct, gw_counter_direct)
//   - 枚举值：counter_type::<prefix>_<mode>  (e.g., fpga_direct, gw_direct)

#pragma once

#include "api_config_impl.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "gw_head.h"
#include "gw_session_cache.h"
#include "matomic.h"
#include "mlog.h"
#include "que_mth_buf.h"

#include <array>
#include <cstdint>
#include <cstring>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// 个微软件极速柜台协议对象（直连模式，非模板具体类）
///
/// 命名：与 fpga_counter_direct / fpga_counter_gateway 风格统一。
/// 注意：当前 enum 值是 counter_type::gw_direct（短名），类名是 gw_counter_direct（长名）。
class gw_counter_direct {
public:
  FORCE_INLINE int32 get_counter_type() const { return static_cast<int32_t>(counter_type::gw_direct); }
  /// 原子读取会话消息序号（方案 F：session_seq_ 在引擎线程自增、外部线程读取，须原子访问）
  FORCE_INLINE int64 get_session_seq_no() const { return lb_common::atomic_load64(&session_seq_); }

  /// api instance 调用，init 中
  int32 init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);
  void init_trade(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {
    trade_send_queue_ = que;
    trade_eng_op_ = link_outop;
  }
  void init_gateway(lb_common::que_mth_buf *que, link_engine_outop *heart_op) {}

  /// api instance 调用，处理买卖委托
  /// 若返回柜台离线错误，让上层重新路由到98柜台
  int32 deal_order_req(const OrderReq &req);

  /// api instance 调用，处理ETF申购赎回委托
  /// 若返回柜台不支持的错误，让上层重新路由到98柜台
  int32 deal_etf_order_req(const OrderReq &req);

  /// api instance 调用，处理委托撤单
  int32_t deal_cancel_req(const CancelReq &req);

  /// engine 调用，接收消息处理（由 link 回调, link_type 从哪个类型链接接收）
  /// 可能有多个完整消息+不完整消息，需要依据消息头一个个解析，确认一个个什么业务并处理
  /// 返回成功解析处理的长度，出错若需要可返回<0,底层会关闭链接
  /// @param link_type 链接类型（个微必为 LINK_TYPE_SPEED_TRADE）
  int32 deal_recv_msg(const char *buf, int32 len, int16 link_type);

  /// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
  void deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret);

  /// engine 调用，处理账户登陆事件
  /// 成功返回消息长度 (含消息头), 0-不需重复登陆, <0 出错
  int32 deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len);

  /// 引擎同步阶段失败时回调
  void ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg);

  /// engine 调用，处理心跳发送事件
  /// 返回消息长度，含消息头
  int32 build_heart_msg(char *o_buf, int32 buf_len);

  /// engine 调用，处理链接重连是否可建立链接
  /// @param link_type  链接类型 (LINK_TYPE_SPEED_TRADE)
  bool can_link_connect(int16 link_type) {
    if (link_type == LINK_TYPE_SPEED_TRADE) {
      return true;
    }
    return false;
  }

  /// Link 在 engine 中调用，链接成功时
  /// @param link_type    链接类型
  /// @param have_switch  1=链接建立时发生地址切换 (need_switch=1 + 实际切换)
  ///                    个微柜台不支持地址切换, 此参数忽略
  /// 返回错误时，底层关闭链接
  int32 deal_link_connect(int16 link_type, int32 have_switch);

  /// Link 在 engine 中调用，链接关闭时
  void deal_link_close(int16 link_type);

  gw_counter_direct();
  ~gw_counter_direct();

  /// 禁用拷贝
  gw_counter_direct(const gw_counter_direct &) = delete;
  gw_counter_direct &operator=(const gw_counter_direct &) = delete;

protected:
  /// 申请队列内存 (头 + take_len 字节 payload), 失败时 o_buf=NULL, 返回 <0
  FORCE_INLINE int64 take_req_que_mem(char *&o_buf, int32 take_len) {
    char *data = nullptr;
    int64 pos = trade_send_queue_->write_get_mth(data, take_len + sizeof(link_send_event));
    if (unlikely(pos <= 0)) {
      return pos;
    }
    o_buf = data;

    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_SPEED_TRADE;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = take_len;
    evt->leave_time_ptr = nullptr;  // 默认无时间戳写入目标（委托路径会在 deal_order_req 中覆盖）
    return pos;
  }
  /// 提交队列内存并触发发送
  FORCE_INLINE void cmt_req_que_mem(int64 get_pos, int32 take_len) {
    trade_send_queue_->write_cmt_mth(get_pos, take_len + sizeof(link_send_event));
    //trade_eng_op_->trigger_send();
  }

  /// FTE 报文校验和计算（对 [头+体] 逐字节求和 %256）
  static uint32_t GenerateSzCheckSum(const char *buf, uint32_t len);

  // ---- 消息构建 (FTE 协议) ----
  /// 构造个微委托消息 (PktNewHeader + TradeOrderReq + 校验和)
  void build_order_msg(const OrderReq &req, char *o_buf);

  /// 构造个微 ETF 申购赎回消息 (PktNewHeader + TradeOrderReq + 校验和)
  void build_etf_order_msg(const OrderReq &req, char *o_buf);

  /// 构造个微撤单消息 (PktNewHeader + CancelOrderReq + 校验和)
  void build_cancel_msg(const CancelReq &req, char *o_buf);

  /// 构造个微账户登录消息 (PktNewHeader + LogOnReq + 校验和)
  void build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len);

  void build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg, LoginAns &ans);
  void build_login_rtn(const gw_message::LogOnAns &msg, LoginAns &o_ans);

  // ---- 应答解析 (FTE 协议) ----
  /// 处理个微账户登录应答 (LogOnAns)
  void deal_log_ans(const char *body, int32 body_len);

  /// 处理委托回报 (TradeOrderER, exec_type='0'/'8')
  void deal_order_rtn(const char *body, int32 body_len);

  /// 处理成交回报 (TradeOrderER, exec_type='F')
  void deal_trade_rtn(const char *body, int32 body_len);

  /// 处理撤单回报 (TradeOrderER, exec_type='4')
  void deal_cancel_rsp(const char *body, int32 body_len);

  /// 处理 ETF 成交回报 (TradeOrderER + ConstituentStock[])
  void deal_etf_trade_rtn(const char *body, int32 body_len);

  /// 处理拒绝消息 (RejectMsg)
  void deal_reject_msg(const char *body, int32 body_len);

  // ---- 状态字典映射 ----
  int32_t map_ord_status(uint8_t fte_status);
  int32_t map_exec_type(char exec_type);
  int16_t map_market_id(uint16_t fte_market_id);

  // ---- 错误码构造 (FTE 协议) ----
  void build_api_order_rej(const gw_message::TradeOrderReq *req, int32 err_code, OrderRtn &o_rtn, StreamInfo &o_stream);
  void build_api_cancel_rej(const gw_message::CancelOrderReq *req, int32 err_code, CancelRsp &o_rtn, StreamInfo &o_stream);

private:
  int16 trade_link_connect_ = 0; ///< 极速链接状态:0-未链接/断开, 1-已链接（跨线程，用 atomic_load16/store16 访问）
  int16 login_state = 0;         ///< 用户登陆状态:0-未登陆，1-登陆中，2-登陆成功（跨线程，用 atomic_load16/store16 访问）
  int16 market_type = 0;         ///< 市场
  int16 heart_interval = 5;      ///< 心跳间隔
  lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列

  callback_manager *cb_mgr_ = nullptr;
  int64 session_seq_ = 0;        ///< 会话消息序号（引擎线程自增、外部线程读取，用 atomic_fetch_add64/atomic_load64 访问）
  lb_common::lb_log *log_ = nullptr;
  link_engine_outop *trade_eng_op_ = nullptr; ///< 极速引擎导出的链接相关操作

  /// 缓存 fund_account_id 字符串键，避免每次委托构造临时 std::string（大小固定，复用 buffer）
  std::string fa_key_cache_;

  /// 单链接单客户模式（true=单客户，false=多客户；默认 true，从配置读取）
  bool single_cust_per_link_ = true;
  /// 单客户模式下的本地会话（登录成功后直接缓存，含 order_locators；不存全局 map）
  GwSessionInfo local_session_;

  /// 获取会话：单客户模式直接返回本地成员，多客户模式从全局缓存按 fund_account_id 获取
  GwSessionInfo *get_session_for_order(const char *fund_account_id);
  /// 记录原单定位：单客户模式写本地成员，多客户模式写全局缓存
  void record_order_locator(const char *fund_account_id, int64_t order_sys_no, int64_t clordno, int64_t client_seq_id);
  /// 撤单反查原单 client_seq_id：单客户模式查本地成员，多客户模式查全局缓存
  int64_t get_orig_client_seq_id(const char *fund_account_id, int64_t order_sys_no);
  /// 撤单反查原单 clordno：单客户模式查本地成员，多客户模式查全局缓存
  int64_t get_clordno(const char *fund_account_id, int64_t order_sys_no);
};

} // namespace lb_api
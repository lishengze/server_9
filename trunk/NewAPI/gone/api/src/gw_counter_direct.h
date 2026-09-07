// gw_counter_direct - 个微软件极速柜台 直连模式
//
// 负责个微软件极速柜台 (仅直连模式) 单 TCP 链接的消息组包/解析.
// 个微柜台无查询接口, 与 fpga_counter_gateway 一致 (无 deal_order_query 等).
//
// 协议状态:
//   - 个微真实协议是外部定义, 当前未提供.
//   - 临时替代方案: 消息头使用 g1_msg_head, 登录消息体使用 login_req/login_ans.
//   - 委托/撤单等业务消息体无法替代, 暂留空实现 + // todo 标注.
//
// 命名风格约定（与 fpga 一致）：
//   - 类名：<prefix>_counter_<mode>  (e.g., fpga_counter_direct, gw_counter_direct)
//   - 枚举值：counter_type::<prefix>_<mode>  (e.g., fpga_direct, gw_direct)

#pragma once

#include "api_config_impl.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "matomic.h"
#include "mlog.h"
#include "que_mth_buf.h"

#include <array>
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

/// 个微软件极速柜台协议对象（直连模式，非模板具体类）
///
/// 命名：与 fpga_counter_direct / fpga_counter_gateway 风格统一。
/// 注意：当前 enum 值是 counter_type::gw_direct（短名），类名是 gw_counter_direct（长名）。
class gw_counter_direct {
public:
  FORCE_INLINE int32 get_counter_type() const { return static_cast<int32_t>(counter_type::gw_direct); }
  /// 原子读取会话消息序号
  FORCE_INLINE int64 get_session_seq_no() const { return session_seq_; }

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
    return pos;
  }
  /// 提交队列内存并触发发送
  FORCE_INLINE void cmt_req_que_mem(int64 get_pos, int32 take_len) {
    trade_send_queue_->write_cmt_mth(get_pos, take_len + sizeof(link_send_event));
    //trade_eng_op_->trigger_send();
  }

  // ---- 消息构建 (个微真实协议未知, 仅登录/心跳有临时替代实现) ----
  /// 构造个微委托消息 (g1_msg_head + 个微协议体)
  /// (个微真实协议未知, 当前为留空实现)
  void build_order_msg(const OrderReq &req, char *o_buf);

  /// 构造个微 ETF 申购赎回消息 (g1_msg_head + 个微协议体)
  /// (个微真实协议未知, 当前为留空实现)
  void build_etf_order_msg(const OrderReq &req, char *o_buf);

  /// 构造个微撤单消息 (g1_msg_head + 个微协议体)
  /// (个微真实协议未知, 当前为留空实现)
  void build_cancel_msg(const CancelReq &req, char *o_buf);

  /// 构造个微账户登录消息 (g1_msg_head + login_req)
  /// (个微真实协议未知, 临时使用 g1 login_req 替代)
  void build_login_msg(const acc_login_event_info &info, int16_t log_type, g1_msg_head *o_req);

  void build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg, LoginAns &ans);
  void build_login_rtn(const login_ans &msg, LoginAns &o_ans);

  // ---- 应答解析 (个微真实协议未知, 临时使用 g1 替代) ----
  /// 处理个微账户登录应答 (临时用 g1 login_ans 替代)
  void deal_log_ans(login_ans &msg);

  // ---- 错误码构造 (复用 fpga 逻辑, 临时用 g1 头) ----
  void build_api_order_rej(const g1_msg_head *msg, int32 err_code, OrderRtn &o_rtn, StreamInfo &o_stream);
  void build_api_cancel_rej(const g1_msg_head *msg, int32 err_code, CancelRsp &o_rtn, StreamInfo &o_stream);

private:
  int16 trade_link_connect_ = 0; ///< 极速链接状态:0-未链接/断开, 1-已链接
  int16 login_state = 0;         ///< 用户登陆状态:0-未登陆，1-登陆中，2-登陆成功
  int16 market_type = 0;         ///< 市场
  int16 heart_interval = 5;      ///< 心跳间隔
  lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列

  //todo : 定义个微柜台缓存结构，添加缓存对象

  callback_manager *cb_mgr_ = nullptr;
  int64 session_seq_ = 0;
  lb_common::lb_log *log_ = nullptr;
  link_engine_outop *trade_eng_op_ = nullptr; ///< 极速引擎导出的链接相关操作
};

} // namespace lb_api

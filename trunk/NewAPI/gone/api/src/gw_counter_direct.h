// gw_counter_direct - 个微软件极速柜台 直连模式
//
// 负责个微软件极速柜台 (仅直连模式) 单 TCP 链接的消息组包/解析.
// 个微柜台无查询接口, 与 fpga_counter_gateway 一致 (无 deal_order_query 等).
//
// 协议与架构状态:
//   - 已接入 FTE 真实协议 (api/src/gw_head.h, 本目录). 报文混合字节序: 8B 头(大端) + 消息体(主机序
//     小端直通) + 4B 校验和(大端), 见 gw_head.h 注释.
//   - 单/多客户两种模式 (need_multi_cust 配置, init 时读入并设置到 GwLoginCache):
//     登录会话条目的存储策略不同 (单条目直存 / 逐账户 map),
//     登录行为完全一致 —— 逐账户 log_gw 状态机 (log_state_ctl) 承担状态/重复/在途控制
//     (幂等回调/在途合并/内部重登, 三场景语义见 api_event_msg.h), 详见 gw_login_cache.h.
//   - 断线重连: 已登录账户 (log_gw OK) 批量自动重登 (collect_relogin_events).
//   - 登出为本地语义 (不发 LogOutReq): 置 LOGOUT, agw_seq_id 保留 (重登携带);
//     多账户共用链路不关链。
//   - 撤单定位: orig_clordno/orig_client_seq_id 直接取自请求 (order_sys_no=柜台委托号,
//     client_seq_id=原委托用户报单号), 不做服务端定位缓存。

#pragma once

#include "api_config_impl.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "gw_head.h"
#include "gw_login_cache.h"
#include "matomic.h"
#include "mlog.h"
#include "que_mth_buf.h"

#include <array>
#include <cstdint>

namespace lb_api
{

  using lb_common::int16;
  using lb_common::int32;
  using lb_common::int64;
  using lb_common::int8;
  using lb_common::uint16;
  using lb_common::uint32;
  using lb_common::uint64;
  using lb_common::uint8;

  /// 个微软件极速柜台协议对象（直连模式，非模板具体类, 单/多客户共用单链接）
  ///
  /// 命名：与 fpga_counter_direct / fpga_counter_gateway 风格统一。
  /// 现实对象：个微软件极速柜台（FTE 协议直连，单 TCP 链接的消息组包/解析）
  /// 注意：柜台类型恒返回 counter_type::gewei（单/多客户是登录缓存的存储模式，
  /// 由 need_multi_cust 配置决定，不体现在柜台类型）。
  class gw_counter_direct
  {
  public:
    /// 现实操作：取本实例个微柜台的柜台类型（恒 gewei）
    FORCE_INLINE int32 get_counter_type() const
    {
      return static_cast<int32>(counter_type::gewei);
    }
    /// api instance 调用，init 中
    /// 现实操作：对个微柜台对象的初始化（配置/回调/日志注入，按 need_multi_cust 配置设单/多客户模式）
    int32 init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);
    /// 现实操作：注入极速柜台发送队列与引擎操作出口
    void init_trade(lb_common::que_mth_buf *que, link_engine_outop *link_outop)
    {
      trade_send_queue_ = que;
      trade_eng_op_ = link_outop;
    }
    /// 现实操作：无（个微无网关链接，接口仅为引擎模板对齐）
    void init_gateway(lb_common::que_mth_buf *que, link_engine_outop *heart_op) {}

    /// api instance 调用，统一处理沪深买卖和 ETF 申赎委托
    /// 若返回柜台离线错误，让上层重新路由到98柜台
    /// 现实操作：对客户委托单的个微柜台报单（组包入队）
    int32 deal_order_req(const OrderReq &req);

    /// api instance 调用，处理委托撤单
    /// 现实操作：对客户撤单请求的个微柜台撤单报单（组包入队）
    int32_t deal_cancel_req(const CancelReq &req);

    /// engine 调用，接收消息处理（由 link 回调, link_type 从哪个类型链接接收）
    /// 可能有多个完整消息+不完整消息，需要依据消息头一个个解析，确认一个个什么业务并处理
    /// 返回成功解析处理的长度，出错若需要可返回<0,底层会关闭链接
    /// @param link_type 链接类型（个微必为 LINK_TYPE_SPEED_TRADE）
    /// 现实操作：对个微柜台下行报文流的切帧、解析与分发
    int32 deal_recv_msg(const char *buf, int32 len, int16 link_type);

    /// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
    /// 现实操作：对发送失败报单的 API 层拒绝回报构造与回调
    void deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret);

    /// engine 调用，处理账户登陆事件 (多账户: 凭据暂存/刷新后按该账户 log_gw 状态机
    /// 幂等/合并/占位发送; 内部重登(user_origin=0)在 OK 态重发)
    /// 成功返回消息长度 (含消息头与校验和), 0-不需重复登陆, <0 出错
    /// 现实操作：对客户登录事件的个微柜台登录报单（组包）
    int32 deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len);

    /// 引擎同步阶段失败时回调
    /// 现实操作：对登录同步阶段失败的该账户在途清理与失败回调
    void ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg);

    /// api_impl 登出编排调用: 柜台侧登出 —— 该账户 log_gw 置 LOGOUT;
    /// 多账户共用链路, 不关链 (本地登出语义, 不发 LogOutReq)
    /// 现实操作：对客户会话的本地登出（状态机置 LOGOUT）
    int32_t deal_logout_req(const LogoutReq &req);

    /// engine 调用，处理心跳发送事件 (8B 头 + 4B 校验和, 无消息体)
    /// 返回消息长度，含消息头
    /// 现实操作：对个微柜台心跳报文的组包
    int32 build_heart_msg(char *o_buf, int32 buf_len);

    /// engine 调用，处理链接重连是否可建立链接
    /// @param link_type  链接类型 (LINK_TYPE_SPEED_TRADE)
    /// 现实操作：对个微柜台链接可建性判定
    bool can_link_connect(int16 link_type)
    {
      if (link_type == LINK_TYPE_SPEED_TRADE)
      {
        return true;
      }
      return false;
    }

    /// Link 在 engine 中调用，链接成功时: 已登录账户批量自动重登 (断线重连需要重登)
    /// @param link_type    链接类型
    /// @param have_switch  1=链接建立时发生地址切换 (need_switch=1 + 实际切换)
    ///                    个微柜台不支持地址切换, 此参数忽略
    /// 返回错误时，底层关闭链接
    /// 现实操作：对个微柜台链接重连后的已登录账户批量自动重登
    int32 deal_link_connect(int16 link_type, int32 have_switch);

    /// Link 在 engine 中调用，链接关闭时: 清全部账户在途 (不清登录状态, 重连后按 OK 态重登)
    /// 现实操作：对个微柜台链接断开的全账户在途清理（登录状态保留）
    void deal_link_close(int16 link_type);

    /// 现实操作：无
    gw_counter_direct();
    ~gw_counter_direct();

    /// 禁用拷贝
    gw_counter_direct(const gw_counter_direct &) = delete;
    gw_counter_direct &operator=(const gw_counter_direct &) = delete;

  protected:
    /// 申请队列内存 (头 + take_len 字节 payload), 失败时 o_buf=NULL, 返回 <0
    /// 【队列完整性契约】take 成功后调用方**必须无条件 cmt**:
    ///   take 与 cmt 之间禁止任何可能失败/提前 return 的逻辑 (构建失败必须在 take 之前完成,
    ///   或构建本身保证无失败路径)——take 后不 cmt 会在队列留下写指针空洞, 破坏队列。
    ///   当前满足模式: build_*_msg 直写定长帧 (take_len==帧长, gw_pkt_encode 失败分支不可达) /
    ///   事件投递 (take 后直写 acc_login_event_info, 无失败分支)。
    FORCE_INLINE int64 take_req_que_mem(char *&o_buf, int32 take_len, int32 qevt_type = LINK_EVENT_TYPE_SEND_MSG)
    {
      char *data = nullptr;
      int64 pos = trade_send_queue_->write_get_mth(data, take_len + sizeof(link_send_event));
      if (unlikely(pos <= 0))
      {
        return pos;
      }
      o_buf = data;

      link_send_event *evt = reinterpret_cast<link_send_event *>(data);
      evt->link_type = LINK_TYPE_SPEED_TRADE;
      evt->type = qevt_type;
      evt->data_len = take_len;
      return pos;
    }
    /// 提交队列内存 (fast 引擎线程轮询消费队列, 无需显式触发)
    FORCE_INLINE void cmt_req_que_mem(int64 get_pos, int32 take_len)
    {
      trade_send_queue_->write_cmt_mth(get_pos, take_len + sizeof(link_send_event));
      trade_eng_op_->trigger_send();
    }

    // ---- 消息构建 (FTE 真实协议, 完整包 = 8B 头(大端) + 体(主机序直通) + 4B 校验和(大端)) ----
    /// 构造个微买卖委托报文 (TradeOrderReq, 消息号 1003)，账户字段从登录会话取值。
    void build_order_msg(const OrderReq &req, const GwLoginInfo *info, char *o_buf, int32 buf_len);

    /// 构造个微 ETF 申购赎回报文 (TradeOrderReq, 消息号 1010)。
    void build_etf_order_msg(const OrderReq &req, const GwLoginInfo *info, char *o_buf, int32 buf_len);

    /// 构造个微撤单消息 (CancelOrderReq, 消息号 1004, orig_clordno/orig_client_seq_id
    /// 直接取自请求: order_sys_no=柜台委托号, client_seq_id=原委托用户报单号)
    void build_cancel_msg(const CancelReq &req, const GwLoginInfo *info, char *o_buf, int32 buf_len);

    /// 构造个微账户登录消息 (LogOnReq, 消息号 1001), 返回完整包长 (含头与校验和)
    /// 字段全部取自登录事件 (不走会话条目回填)
    int32 build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len);

    void build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg, LoginAns &ans);
    void build_login_rtn(const gw_message::LogOnAns &msg, LoginAns &o_ans);

    // ---- 应答解析 (FTE 真实协议) ----
    /// 处理个微账户登录应答 (LogOnAns, 消息号 2001): 按应答资金账户定位条目,
    /// 成功回填 cust/branch/account/agw_seq_id 并驱动该账户 log_gw 状态机
    void deal_log_ans(const gw_message::LogOnAns &msg);

    /// 委托回报 (TradeOrderER, 消息号 2003)
    void deal_order_rtn(const gw_message::TradeOrderER &er);

    /// 成交回报 (TradeOrderER, 消息号 2005/2010)
    void deal_trade_rtn(const gw_message::TradeOrderER &er);

    /// 撤单应答 (TradeOrderER, 消息号 2004)
    void deal_cancel_rsp(const gw_message::TradeOrderER &er);

    /// 拒绝回报 (RejectMsg, 消息号 9)
    void deal_reject(const gw_message::RejectMsg &rej);

    /// 投递单账户重登事件 (重连批量重登/需求6; 在途占位由 deal_cust_login 的 to_login 处理, S3)
    int32 delive_relogin_event(const acc_login_event_info &ev);

    // ---- 错误码构造 (API 层拒绝: 发送失败路径) ----
    void build_api_order_rej(const gw_message::TradeOrderReq &msg, int32 err_code, OrderRtn &o_rtn,
                             StreamInfo &o_stream);
    void build_api_cancel_rej(const gw_message::CancelOrderReq &msg, int32 err_code, CancelRsp &o_rtn,
                              StreamInfo &o_stream);

  private:
    int16 trade_link_connect_ = 0; ///< 极速链接状态:0-未链接/断开, 1-已链接
    int16 market_type = 0;         ///< 市场

    lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列

    GwLoginCache login_cache_; ///< 已登录账户缓存 (本柜台专属, 见 gw_login_cache.h)

    callback_manager *cb_mgr_ = nullptr;
    lb_common::lb_log *log_ = nullptr;
    int32 heart_interval = 5;                   ///< 心跳间隔
    link_engine_outop *trade_eng_op_ = nullptr; ///< 极速引擎导出的链接相关操作
  };

} // namespace lb_api

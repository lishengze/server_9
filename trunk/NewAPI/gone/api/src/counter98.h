// counter98 - 98 柜台协议对象
//
// 负责 98 柜台的消息组包、解析、状态管理、查询应答处理.
// 柜台对外只提供业务 API 接口对应的发送函数, 不暴露 build 消息函数.
// 业务发送函数内部完成组包 + 入队 (que_mth_buf::write_get_mth).
//
// 协议状态:
//   - 已接入 Archforce/华锐 ATP AGW 真实协议 (counter98/ vendor 协议库).
//   - 报文帧 = AgwHeader 16B (msg_type/msg_ver/reserver/seq_num/body_length) + mp 编码消息体.
//   - msg_ver 单一来源 = vendor 各消息类 GetMsgVer(): 会话类(登录/心跳/注销/回报同步)=0,
//     业务类(账户登录/委托/撤单/查询)=2 (RUL-26).
//   - 入站分发 = (msg_type, msg_ver) 组合键精确匹配 (RUL-26);
//     心跳在分发前单独处理; 未知消息跳过不关链.
//   - 回报同步: agw 登录成功后按配置发送 AgwMsgSync (分区→记录号); 每投递一条下行回报
//     (290001/190001/210115) 按分区推进已消费位置并去重 (≤已存值丢弃, RUL-30), 重连
//     重登后按断点续传。

#pragma once

#include "api_config.h"
#include "api_config_impl.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "c98_session_cache.h"
#include "hash_map_mth.h"
#include "mlog.h"
#include "order_trade_type.h"
#include "que_mth_buf.h"
#include <pthread.h>

#include <map>

// ---- ATP AGW 协议引入 [接线修正] ----
// lb_counter98_protocol.h 的 :44 "using mp::error_code" 编译不过 (error_code 是 atp::gw 内
// 对 mp::ErrorCode 的 typedef, mp 命名空间并无该名); 此处直接引入底层头并自建等价别名,
// 不改动 counter98\ 目录任何文件
#include "serialize_buffer.h"
#include "message_encoder.h"
#include "message_decoder.h"
#include "msg_proto_types.h"
#include "root_packet.h"
#include "error_code.h"
#include "string_util.h"
#include "factory.h"
#include "mp_export.h"
#include "export_macro.h"
#include "types_agw_external.h"
#include "constants_agw_external.h"
#include "messages_agw_external.h"

namespace lb_counter98
{
  using namespace atp::gw;
  using atp::gw::AgwHeader;
  using mp::ErrorCode;
  using mp::MessageDecoder;
  using mp::MessageEncoder;
  using mp::RootPacket;
  using mp::SerializeBuffer;
  static constexpr size_t AGW_HEADER_SIZE = 16;
}

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

  /// 98 柜台协议对象
  /// 现实对象：98 柜台（ATP 协议、agw 网关用户登录，报文组包/解析与账户登录会话管理）
  class counter98
  {
  public:
    /// 获取柜台类型(恒为 fixed_98)
    /// 现实操作：取本实例 98 柜台的柜台类型（恒 fixed_98）
    FORCE_INLINE int32 get_counter_type() const { return static_cast<int32_t>(counter_type::fixed_98); }

    /// api instance 调用，在 init 中
    /// 现实操作：对 98 柜台对象的初始化（配置/回调/日志注入）
    int32 init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);

    /// 现实操作：注入极速柜台发送队列与引擎操作出口（向极速柜台投递登录事件用）
    void init_trade(lb_common::que_mth_buf *que, link_engine_outop *link_outop)
    {
      trade_send_queue_ = que;
      trade_eng_op_ = link_outop;
    }
    /// 现实操作：注入 98 链接发送队列与引擎操作出口
    void init_gateway(lb_common::que_mth_buf *que, link_engine_outop *link_outop)
    {
      gw_send_queue_ = que;
      gw_eng_op_ = link_outop;
    }

    /// api instance 调用，处理各业务统一委托（不支持的业务返回 UNSUPPORTED_OP）
    /// 现实操作：对客户委托单的 98 柜台报单（组包入队）
    int32 deal_order_req(const OrderReq &req);

    /// api instance 调用，处理委托撤单
    /// 现实操作：对客户撤单请求的 98 柜台撤单报单（组包入队）
    int32_t deal_cancel_req(const CancelReq &req);

    /// api instance 调用，处理客户委托查询
    /// 现实操作：对客户委托查询的 98 柜台查询报单（组包入队）
    int32 deal_order_query(const OrderQueryReq &req);

    /// api instance 调用，处理客户委托批量查询
    /// 现实操作：对客户委托批量查询的 98 柜台查询报单（组包入队）
    int32 deal_order_batch_query(const OrderBatchQueryReq &req);

    /// api instance 调用，处理客户成交查询
    /// 现实操作：对客户成交查询的 98 柜台查询报单（组包入队）
    int32 deal_trade_query(const TradeQueryReq &req);

    /// api instance 调用，处理客户成交批量查询
    /// 现实操作：对客户成交批量查询的 98 柜台查询报单（组包入队）
    int32 deal_trade_batch_query(const TradeBatchQueryReq &req);

    /// api instance 调用，处理客户资金查询
    /// 现实操作：对客户资金查询的 98 柜台查询报单（组包入队）
    int32 deal_fund_query(const FundQueryReq &req);

    /// api instance 调用，处理客户持仓查询
    /// 现实操作：对客户持仓查询的 98 柜台查询报单（组包入队）
    int32 deal_position_query(const PositionQueryReq &req);

    /// api instance 调用，处理最大可委托数查询 (AgwMsgExtQueryMaxOrderQty, 300001)
    /// 现实操作：对最大可委托数查询的 98 柜台查询报单（组包入队）
    int32 deal_max_order_qty_query(const MaxOrderQtyQueryReq &req);

    /// api instance 调用，处理客户总资产查询 (AgwMsgExternalQueryTotalFundAsset, 7301735)
    /// 现实操作：对客户总资产查询的 98 柜台查询报单（组包入队）
    int32 deal_total_fund_asset_query(const TotalFundAssetQueryReq &req);

    /// api instance 调用，处理可取资金查询 (AgwMsgExtQueryFundMaxdraw, 7390701)
    /// 现实操作：对可取资金查询的 98 柜台查询报单（组包入队）
    int32 deal_fund_maxdraw_query(const FundMaxdrawQueryReq &req);

    /// api instance 调用，处理融资融券信用数据(可用保证金)查询 (AgwMsgExtQueryCustMarginAvl, 7390724)
    /// 现实操作：对可用保证金查询的 98 柜台查询报单（组包入队）
    int32 deal_cust_margin_avl_query(const CustMarginAvlQueryReq &req);

    /// api instance 调用，处理资产负债查询 (AgwMsgExtQueryAssetDebts, 7390700)
    /// 现实操作：对资产负债查询的 98 柜台查询报单（组包入队）
    int32 deal_asset_debts_query(const AssetDebtsQueryReq &req);

    /// api instance 调用，处理两融最大可委托数查询 (AgwMsgExtQueryCreditMaxOrderQty, 7390710)
    /// 现实操作：对两融最大可委托数查询的 98 柜台查询报单（组包入队）
    int32 deal_credit_max_order_qty_query(const CreditMaxOrderQtyQueryReq &req);

    /// engine 调用，接收消息处理（由 link 回调, link_type 从哪个类型链接接收）
    /// 可能有多个完整消息+不完整消息，需要依据 AgwHeader 一个个解析处理
    /// 返回成功解析处理的长度，出错若需要可返回<0,底层会关闭链接
    /// @param link_type 链接类型（98 必为 LINK_TYPE_98）
    /// 现实操作：对 98 柜台下行报文流的切帧、解析与分发
    int32 deal_recv_msg(const char *buf, int32 len, int16 link_type);

    /// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
    /// 现实操作：对发送失败报单的 API 层拒绝回报构造与回调
    void deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret);

    /// 登录请求处理中调用 (api_impl::login 第一步, REQ 4.4-2)
    /// 发起 agw 用户登陆事件，入队，并同步等待登陆结果（sleep 循环检查 agw_log_state）
    /// agw_user_login_timeout 超时返回 LBAPI_ERR_LOGIN_TIMEOUT
    /// 重入: CAS 抢占在途状态, 已在途则等待首者结果, 已登录幂等返回 OK
    /// 现实操作：对 98 网关用户的登录（事件入队并同步等待结果）
    int32 deal_agw_login();

    /// 现实操作：对 98 网关用户登录失败的登记与处置（错误码回填同步等待方、失败关链）
    void error_agwuser_login(int32 err_code);

    /// 现实操作：对客户登录请求的受理（转登录事件投递 98 引擎）
    int32 deal_login_req(const LoginReq &req);

    /// 现实操作：对客户会话的 98 柜台登出
    int32 deal_logout_req(const LogoutReq &req);

    /// engine 调用，处理账户登陆事件,检查是否需要登陆，若是构造登陆消息到o_buf;若否，发起极速柜台登陆事件，成功返回0
    /// 成功返回消息长度，含消息头，0-不需重复登陆,<0 出错
    /// 现实操作：对客户登录事件的 98 账户登录处理（按需构造登录报文，或转投极速柜台登录）
    int32 deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len);

    /// 现实操作：对登录阶段失败事件的失败回调
    void ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg);

    /// engine 调用，处理心跳发送事件 (AgwMsgHeartBeat, 消息号 0)
    /// 返回消息长度，含消息头
    /// 现实操作：对 98 柜台心跳报文的组包
    int32 build_heart_msg(char *o_buf, int32 buf_len);

    /// engine 调用，处理链接重连是否可建立链接
    /// @param link_type  链接类型 (LINK_TYPE_98 / LINK_TYPE_SPEED_GW)
    /// 现实操作：对 98 柜台链接可建性判定
    bool can_link_connect(int16 link_type)
    {
      if (link_type == LINK_TYPE_98)
      {
        return true;
      }
      return false;
    }

    /// Link 在 engine 中调用，链接成功时
    /// @param link_type    链接类型
    /// @param have_switch  1=链接建立时发生地址切换 (need_switch=1 + 实际切换)
    /// 返回错误时，底层关闭链接
    /// 现实操作：对 98 柜台链接建立成功的处置（重连后重发 agw 登录、链接状态回调）
    int32 deal_link_connect(int16 link_type, int32 have_switch);

    /// Link 在 engine 中调用，链接关闭时
    /// 现实操作：对 98 柜台链接断开的处置（清账户在途、链接状态回调；账户登录状态保留，agw 用户登录状态复位待重连重登；agw 登录在途者以 LBAPI_ERR_LINK_DISCONNECTED 回填同步等待方）
    void deal_link_close(int16 link_type);

    /// 现实操作：无
    counter98();
    /// 现实操作：无
    ~counter98();

    /// 禁用拷贝
    counter98(const counter98 &) = delete;
    counter98 &operator=(const counter98 &) = delete;

  protected:
    /// 申请队列内存 (头 + take_len 字节 payload), 失败时 o_buf=NULL, 返回 <0
    /// 【队列完整性契约】take 成功后调用方**必须无条件 cmt**:
    ///   take 与 cmt 之间禁止任何可能失败/提前 return 的逻辑 (构建失败必须在 take 之前完成,
    ///   或构建本身保证无失败路径)——take 后不 cmt 会在队列留下写指针空洞, 破坏队列。
    ///   当前满足模式: 98 send_pkt (先组包后 take, take 后仅 memcpy) /
    ///   事件投递 (take 后直写定长结构体, 无失败分支)。
    FORCE_INLINE int64 take_req_que_mem(char *&o_buf, int32 take_len, int32 evt_type)
    {
      char *data = nullptr;
      int64 pos = gw_send_queue_->write_get_mth(data, take_len + sizeof(link_send_event));
      if (unlikely(pos <= 0))
      {
        return pos;
      }
      o_buf = data;

      link_send_event *evt = reinterpret_cast<link_send_event *>(data);
      evt->link_type = LINK_TYPE_98;
      evt->type = evt_type;
      evt->data_len = take_len;
      return pos;
    }
    FORCE_INLINE void cmt_req_que_mem(int64 get_pos, int32 take_len)
    {
      gw_send_queue_->write_cmt_mth(get_pos, take_len + sizeof(link_send_event));
      gw_eng_op_->trigger_send();
    }

    /// 组完整包 (AgwHeader + 消息体) 写入 o_buf, 返回总长; 缓冲不足/编码失败返回 <0
    int32 encode_pkt_to(mp::RootPacket &msg, char *o_buf, int32 buf_len);

    /// 填充完毕的消息 (AgwHeader + body) 按精确长度入队发送
    int32 send_pkt(mp::RootPacket &msg, int32 qevt_type = LINK_EVENT_TYPE_SEND_MSG);

    /// 填充委托基类公共字段 [对齐 source build_order_msg]: 身份四元组取会话 (登录应答
    /// 回填值), 密码/委托方式扩展/user_info 取会话 (登录请求缓存值); user_info 为登录
    /// 缓存原值 (RUL-29; 回报/撤单关联由 client_seq_id 数值字段唯一承载); password 经锁内快照 (可被
    /// 改密重登并发刷新, 见 c98_session_cache.h §3); order_way_ext/user_info 同经快照;
    /// client_feature_code/stop_px/
    /// time_in_force/cash_margin 按 source 不填 (走协议构造默认: ' '/""/0/' '/' ')
    /// @tparam OrderT AgwMsgOrder 派生 (AgwMsgCashAuctionOrder / AgwMsgCancelOrder)
    template <typename OrderT>
    void fill_order_base(OrderT &msg, const C98SessionInfo *sess,
                         const std::array<char, MAX_SECURITY_ID_LEN> &security)
    {
      msg.fund_account_id = sess->fund_account_id;
      msg.branch_id = sess->branch_id;
      msg.account_id = sess->account_id;
      msg.cust_id = sess->cust_id;
      /// security_id 原样赋值 (NUL 尾直传, 不空格填充, RUL-31; 其余定长
      /// 字段仍走 to_atp_str 空格填充, 待联调统一口径)
      msg.security_id = security;
      std::array<char, INNER_PASSWORD_LEN> tpwd;
      std::array<char, 2> tordway;
      std::array<char, INNER_USER_INFO_LEN> tuser;
      /// 锁内快照 (order_way_ext/user_info 盘中可变)
      session_cache_.copy_pack_password(sess, tpwd, tordway, tuser);
      msg.password = std::string(tpwd.data());
      msg.order_way_ext = tordway;
      msg.user_info = tuser;
    }

    /// 填充查询基类公共字段 [对齐 source]: cust/fund/branch/account/user_info/password 全取会话
    /// (password/user_info 经锁内快照, 同 fill_order_base);
    /// client_feature_code 按 source 不填 (构造默认 "")
    /// @tparam QueryT AgwMsgExtQueryBase 派生
    template <typename QueryT>
    void fill_query_base(QueryT &msg, const C98SessionInfo *sess, int64_t client_req_no)
    {
      msg.cust_id = sess->cust_id;
      msg.fund_account_id = sess->fund_account_id;
      msg.branch_id = sess->branch_id;
      msg.account_id = sess->account_id;
      msg.client_seq_id = client_req_no;
      std::array<char, INNER_PASSWORD_LEN> tpwd;
      std::array<char, INNER_USER_INFO_LEN> tuser;
      session_cache_.copy_pack_credentials(sess, tpwd, tuser);
      msg.user_info = tuser;
      msg.password = std::string(tpwd.data());
    }

    // ---- 消息构建: 填充消息对象 (组包入队统一走 send_pkt; 身份字段经会话指针回填) ----
    /// 填充各业务原生委托消息的会话和请求字段；业务码及方向由本柜台分派。
    template <typename OrderT>
    void build_order_msg(const OrderReq &req, const C98SessionInfo *sess, OrderT &msg,
                         uint8_t business_type, char side);

    /// 将 98 的业务码及买卖方向还原为 API 业务方向，不保存第二份委托状态。
    static char api_order_side(uint8_t business_type, char side);

    /// 98 回报的委托类型暂填 0，待确认如何映射为 API 类型。
    static char api_order_type(char order_type);

    /// 解码各业务成交消息，统一交给已有回报去重及回调流程。
    template <typename ReportT>
    void decode_trade_er(const char *body, size_t body_len);

    /// 解码发送失败的原业务委托，通知调用方该笔委托已拒绝。
    template <typename OrderT>
    void reject_sent_order(const char *body, size_t body_len, int32 err_code);

    /// 构造 98 撤单消息 (AgwMsgCancelOrder, 消息号 190001)
    void build_cancel_msg(const CancelReq &req, const C98SessionInfo *sess, lb_counter98::AgwMsgCancelOrder &msg);

    /// 构造 98 委托查询消息 (AgwMsgExtQueryOrder, 消息号 300002)
    void build_order_query_msg(const OrderQueryReq &req, const C98SessionInfo *sess,
                               lb_counter98::AgwMsgExtQueryOrder &msg);

    /// 构造 98 委托批量查询消息 (同 AgwMsgExtQueryOrder)
    void build_order_batch_query_msg(const OrderBatchQueryReq &req, const C98SessionInfo *sess,
                                     lb_counter98::AgwMsgExtQueryOrder &msg);

    /// 构造 98 成交查询消息 (AgwMsgExtQueryTradeOrder, 消息号 300003)
    void build_trade_query_msg(const TradeQueryReq &req, const C98SessionInfo *sess,
                               lb_counter98::AgwMsgExtQueryTradeOrder &msg);

    /// 构造 98 成交批量查询消息 (同 AgwMsgExtQueryTradeOrder)
    void build_trade_batch_query_msg(const TradeBatchQueryReq &req, const C98SessionInfo *sess,
                                     lb_counter98::AgwMsgExtQueryTradeOrder &msg);

    /// 构造 98 资金查询消息 (AgwMsgExtQueryFund, 消息号 300004)
    void build_fund_query_msg(const FundQueryReq &req, const C98SessionInfo *sess,
                              lb_counter98::AgwMsgExtQueryFund &msg);

    /// 构造 98 持仓查询消息 (AgwMsgExtQuerySelfShare 自研股份查询, 消息号 7300714; 对齐 source)
    void build_position_query_msg(const PositionQueryReq &req, const C98SessionInfo *sess,
                                  lb_counter98::AgwMsgExtQuerySelfShare &msg);

    // ---- 98 扩展查询消息构建 (98 柜台扩展查询消息集, RUL-26) ----
    void build_max_order_qty_query_msg(const MaxOrderQtyQueryReq &req, const C98SessionInfo *sess,
                                       lb_counter98::AgwMsgExtQueryMaxOrderQty &msg);
    void build_total_fund_asset_query_msg(const TotalFundAssetQueryReq &req, const C98SessionInfo *sess,
                                          lb_counter98::AgwMsgExternalQueryTotalFundAsset &msg);
    void build_fund_maxdraw_query_msg(const FundMaxdrawQueryReq &req, const C98SessionInfo *sess,
                                      lb_counter98::AgwMsgExtQueryFundMaxdraw &msg);
    void build_cust_margin_avl_query_msg(const CustMarginAvlQueryReq &req, const C98SessionInfo *sess,
                                         lb_counter98::AgwMsgExtQueryCustMarginAvl &msg);
    void build_asset_debts_query_msg(const AssetDebtsQueryReq &req, const C98SessionInfo *sess,
                                     lb_counter98::AgwMsgExtQueryAssetDebts &msg);
    void build_credit_max_order_qty_query_msg(const CreditMaxOrderQtyQueryReq &req, const C98SessionInfo *sess,
                                              lb_counter98::AgwMsgExtQueryCreditMaxOrderQty &msg);

    void build_agw_login_msg(lb_counter98::AgwMsgLogin &msg);

    // ---- 回报同步 (AgwMsgSync=5 / AgwMsgSyncFinish=6, RUL-26) ----
    /// 构造回报同步请求 (按 counter98_mode/counter98_report_sync_ 填充分区记录号)
    void build_sync_msg(lb_counter98::AgwMsgSync &msg);

    /// 发送回报同步请求 (agw 登录成功后调用): 快速登录模式或无同步配置时跳过(返回 OK,
    /// "不需要 sync"不算失败); 发送失败返回错误码 —— 调用方按 agw 登录失败处理
    /// (RUL-27/28: 需要同步而发送失败 = agw 登录失败, 走置错误码+复位+关链;
    ///  agw 成功标志后置(RUL-28)故失败不置位, 重连后不自动重登, 由用户重新 login 发起)
    /// @return LBAPI_OK-已发送或按配置跳过; 负数-发送错误码 (队列满/组包失败)
    int32_t send_agw_sync();

    /// 处理 agw 用户登录应答 (AgwMsgLogin 入站, 应答语义见 field_mapping_c98.md §2)
    void deal_agwuser_login_ans(const lb_counter98::AgwMsgLogin &msg);

    /// 处理回报同步结束应答 (AgwMsgSyncFinish 入站): on_event(report_sync_finish)
    /// 回调用户 (回报同步流程完成通知)
    void deal_sync_finish_ans(const lb_counter98::AgwMsgSyncFinish &msg);

    /// 消费一条下行回报记录: **分区去重 + 断点推进统一收口** (RUL-30, 取代 RUL-27③
    /// 的 sync_finished_ 门控与已删除的 session_seq_ 单计数器)——
    /// 覆盖全部携带 AgwMsgOrderReport 基类 partition/index 的回报通道:
    /// 委托确认 290001 / 撤单应答 190001 / 成交回报 210115 (查询应答
    /// AgwMsgExtQueryResultBase 派生无分区/索引字段, 不参与):
    ///   index<=0      → 视为未填记录号, 直通不判重不推进 (安全例外, 联调确认起算形态)
    ///   分区无记录    → 登记并投递 (新分区首条)
    ///   index<=已存值 → 重复回报, 丢弃
    ///   index>已存值  → 推进断点并投递 (断点即重连重登 AgwMsgSync 续传起点)
    /// @return true-投递; false-重复丢弃
    bool consume_report_pos(const lb_counter98::AgwMsgOrderReport &rpt);

    int32 build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len);

    /// 从 ATP 账户登录应答构造 API 层 LoginAns (失败回调用)
    void build_cust_login_rtn(const C98SessionInfo *sess, LoginAns &o_ans, int32 errcode, const char *errmsg);

    /// 从会话构造极速柜台登录事件 (含 ATP 应答回填值与登录请求缓存的
    /// password/user_info/feature; 无应答参数 —— 全部取自会话)
    void build_fast_counter_login_event(const C98SessionInfo *sess, acc_login_event_info &o_info);

    void build_cust_login_event(const LoginReq &req, acc_login_event_info &o_info);

    /// 投递 AGWUSER_LOGIN 事件到 98 发送队列 (deal_agw_login 与重连重登共用)
    /// 前置: 调用方已占位 agw_login_state=1; 队满时复位为 0 并返回错误
    int32 delive_agw_login_event();

    // ---- 接收处理: 回报/查询应答 → API 回调 ----
    /// 委托状态响应 (AgwMsgOrderStatusAck, 290001) → 委托确认 on_order_rtn (对齐 source 单一路由)
    void deal_order_status_ack(const lb_counter98::AgwMsgOrderStatusAck &msg);

    /// 撤单应答 (msg_type=190001 路由, 解码同 AgwMsgOrderStatusAck; 对齐 source deal_cancel_ans)
    void deal_cancel_ans(const lb_counter98::AgwMsgOrderStatusAck &msg);

    /// 各业务成交回报的公共字段 → 成交或委托回调；沿用分区去重。
    void deal_trade_er(const lb_counter98::AgwMsgTradeExecutionReportSeqNum &msg);

    /// 委托查询结果 (310002) → on_order_query_ans (分块回调)
    void deal_order_query_result(const lb_counter98::AgwMsgExtQueryResultOrder &msg);

    /// 成交查询结果 (310003) → on_trade_query_ans (分块回调)
    void deal_trade_query_result(const lb_counter98::AgwMsgExtQueryResultTradeOrder &msg);

    /// 资金查询结果 (310004) → on_fund_query_ans (单条)
    void deal_fund_query_result(const lb_counter98::AgwMsgExtQueryResultFund &msg);

    /// 持仓查询结果 (7400714 自研股份查询结果) → on_position_query_ans (对齐 source)
    void deal_share_query_result(const lb_counter98::AgwMsgExtQueryResultSelfShare &msg);

    // ---- 98 扩展查询结果处理 (98 柜台扩展查询消息集, RUL-26) ----
    /// 最大可委托数查询结果 (310001) → on_max_order_qty_query_ans (单条)
    void deal_max_order_qty_query_result(const lb_counter98::AgwMsgExtQueryMaxOrderQtyResult &msg);

    /// 客户总资产查询结果 (7401735) → on_total_fund_asset_query_ans (按币种多条, 无分页)
    void deal_total_fund_asset_query_result(const lb_counter98::AgwMsgExtQueryResultTotalFundAsset &msg);

    /// 可取资金查询结果 (7490701) → on_fund_maxdraw_query_ans (按币种多条, 无分页)
    void deal_fund_maxdraw_query_result(const lb_counter98::AgwMsgExtQueryResultFundMaxdraw &msg);

    /// 融资融券信用数据查询结果 (7490724) → on_cust_margin_avl_query_ans (单条)
    void deal_cust_margin_avl_query_result(const lb_counter98::AgwMsgExtQueryResultCustMarginAvl &msg);

    /// 资产负债查询结果 (7490700) → on_asset_debts_query_ans (单条)
    void deal_asset_debts_query_result(const lb_counter98::AgwMsgExtQueryResultAssetDebts &msg);

    /// 两融最大可委托数查询结果 (7490710) → on_credit_max_order_qty_query_ans (单条)
    void deal_credit_max_order_qty_query_result(const lb_counter98::AgwMsgExtQueryCreditMaxOrderQtyResult &msg);

  protected:
    /// 业务请求(委托/撤单/查询等)发送前提检查:
    /// 98 链接已建立 + agw 用户登录成功 + 该账户 98 登录成功 (任一不满足返回对应错误码)。
    /// 单次缓存查询: 通过时经 o_sess 返回会话指针, 调用方直接用于组包, 无需再查缓存。
    /// @return LBAPI_OK-可发送(o_sess 有效), 负数-错误码 (o_sess 置空, 不发)
    int32 check_req_precond(const char *fund_account_id, size_t fund_len, C98SessionInfo *&o_sess);

    /// 处理 ATP 账户登录应答 (AgwMsgCustLoginResp, 800006)
    void deal_cust_login_ans(const lb_counter98::AgwMsgCustLoginResp &msg);

    int32 delive_fast_counter_login(const acc_login_event_info &info);

    // ---- 错误码构造 (API 层拒绝: 发送失败路径) ----
    void build_api_order_rej(const lb_counter98::AgwMsgOrder &msg, int32 err_code, OrderRtn &o_rtn,
                             StreamInfo &o_stream);
    void build_api_cancel_rej(const lb_counter98::AgwMsgCancelOrder &msg, int32 err_code, CancelRsp &o_rtn,
                              StreamInfo &o_stream);

    /// C 串 → 定长 array (空格填充) —— 返回值版 (用于 security_id 整组赋值; 哑元推导宽度)
    template <size_t N>
    static std::array<char, N> to_atp_fixed(const char *src, const std::array<char, N> &)
    {
      std::array<char, N> dst;
      to_atp_str(dst, src);
      return dst;
    }

  protected:
    /// C 串(NUL 结尾) → ATP 定长字段(尾部空格填充) —— 填充约定【待确认】见 field_mapping_c98.md
    template <size_t N>
    static void to_atp_str(std::array<char, N> &dst, const char *src)
    {
      size_t n = 0;
      while (n < N && src[n] != '\0')
      {
        dst[n] = src[n];
        ++n;
      }
      for (size_t i = n; i < N; ++i)
      {
        dst[i] = ' ';
      }
    }

    /// ATP 定长字段(空格填充) → API std::array(裁尾空格；满宽时不补 NUL)
    /// 注意（2026-11-11 修正）：来源有效长度 >= 目标宽度时按满宽拷贝（同 gw 侧口径），
    /// 避免 16 字节资金账号/客户号末位丢失；满宽字段按定长消费，勿当 C 串。
    template <size_t N, size_t M>
    static void from_atp_str(std::array<char, N> &dst, const std::array<char, M> &src)
    {
      size_t n = 0;
      while (n < M && src[n] != '\0')
      {
        ++n;
      }
      while (n > 0 && src[n - 1] == ' ')
      {
        --n;
      }
      if (n > N)
      {
        n = N; ///< 允许占满整宽（此时无 NUL 终止符）
      }
      for (size_t i = 0; i < n; ++i)
      {
        dst[i] = src[i];
      }
      for (size_t i = n; i < N; ++i)
      {
        dst[i] = '\0';
      }
    }

    /// ATP 定长字段(空格填充) → C 串缓冲 (裁尾空格)
    /// 注意（2026-11-11 修正）：定长业务字段(fund/cust/account/branch)允许**占满整宽**，
    /// 仅当 n < dst_len 时才补 NUL。此前一律按 dst_len-1 截断，导致 16 字节资金账号被截成
    /// 15 位 → 极速柜台(FTE) CheckPasswd 的 key 匹配不到 → 登录回 kPasswdErr(4)。
    template <size_t M>
    static void from_atp_cstr(char *dst, size_t dst_len, const std::array<char, M> &src)
    {
      size_t n = 0;
      while (n < M && src[n] != '\0')
      {
        ++n;
      }
      while (n > 0 && src[n - 1] == ' ')
      {
        --n;
      }
      if (n > dst_len)
      {
        n = dst_len; ///< 允许占满整宽（此时无 NUL 终止符，下游按定长处理）
      }
      for (size_t i = 0; i < n; ++i)
      {
        dst[i] = src[i];
      }
      for (size_t i = n; i < dst_len; ++i)
      {
        dst[i] = '\0';
      }
    }

  private:
    C98SessionCache session_cache_;                      ///< 98 账户会话缓存 (本柜台专属)
    int16 trade_link_connect_ = 0;                       ///< 98 链接状态:0-未链接/断开, 1-已链接
    int16 agw_login_state = 0;                           ///< agw 用户登陆状态:0-未登陆，1-登陆中，2-登陆成功
    int16 have_agw_ok_ = 0;                              ///< agw 是否成功登陆过 (98 断线重连后据此重发 agw 登录, REQ 4.5-1)
    int16 market_type = 0;                               ///< 市场
    lb_common::que_mth_buf *gw_send_queue_ = nullptr;    ///< 98柜台发送队列
    link_engine_outop *gw_eng_op_ = nullptr;             ///< 98链接引擎导出的链接相关操作
    lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列

    /// 98 柜台无重连重登; 并发首次登录的重复消息由柜台步骤 S1 合并与 98 迟到失败忽略(val==OK)消化
    // 账户缓存: C98SessionCache (本柜台专属, fund 为键) ——
    // 会话内 log_98 状态机承担 98 账户登录的状态/重复/在途控制; deal_cust_login (引擎线程)
    // 为会话插入唯一入口 (不存在则插入, 存在则状态机幂等/合并); ATP 应答仅回客户/账户结构,
    // 不回传的组包字段 (password/user_info/feature) 缓存于会话; 委托/查询组包经会话快照
    // (get_session 指针) 按请求资金账户取身份字段。

    int32 agw_login_err_ = 0;                                    ///< agw 异步登录错误码 (异步失败点设置, deal_agw_login 同步等待读取返回; 原子访问)
    counter_type fast_counter_type_ = counter_type::fpga; ///< 极速柜台类型(init 从配置覆盖)
    int32_t agw_user_login_timeout_ = 10;                        ///< AGW 登录超时秒数（默认 10）
    int32 heart_interval = 5;                                    ///< 心跳间隔
    callback_manager *cb_mgr_ = nullptr;                         ///< 回调管理器 (deal_recv_msg 成功后回调用户)
    lb_common::lb_log *log_ = nullptr;                           ///< 日志指针 (构造/解析失败时打错误)
    link_engine_outop *trade_eng_op_ = nullptr;                  ///< 极速引擎导出的链接相关操作

    char agw_session[INNER_SESSION_LEN]; ///< agw 会话号 (ATP 协议应从 send comp id 映射)
    std::string agw_user_;               ///< 98 agw 用户名，拷贝自 cfg 的配置项
    std::string agw_password_;           ///< 98 agw 密码，拷贝自 cfg 的配置项

    // ---- 回报同步状态 (引擎线程读写; init 阶段用户线程写入先于 start) ----
    int32_t counter98_mode_ = 0;                       ///< 98 登录模式: 0=同步登录, 1=快速登录(跳过回报同步)
    std::map<uint8_t, int32_t> counter98_report_sync_; ///< 分区已消费位置(兼回报同步断点): 分区号 → 回报记录号
                                                       ///< (init 取配置种子; 每投递一条回报按 consume_report_pos
                                                       ///<   推进并作分区去重; 重连重登后 build_sync_msg 按此
                                                       ///<   续传; 仅内存态, 进程重启从配置起点重新同步)
  };

} // namespace lb_api

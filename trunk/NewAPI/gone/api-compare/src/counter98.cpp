// counter98 - 98 柜台协议对象实现
//
// 负责 98 柜台的消息组包 / 解析 / 状态管理 / 查询应答处理
// 业务发送函数内部完成组包 + 入队 (队列满立即返回错误)
//
// 协议与架构状态:
//   - 已接入 Archforce/华锐 ATP AGW 真实协议; 报文帧 = AgwHeader 16B + mp 编码消息体
//     (host_to_network=false 主机序直通【待确认, 联调首验点】)。
//   - msg_ver 单一来源 = vendor 消息类 GetMsgVer() (会话类 0 / 业务类 2, RUL-26);
//     入站分发按 (msg_type, msg_ver) 组合键精确匹配。
//   - 回报同步与分区去重: agw 登录成功后按配置发 AgwMsgSync; 每投递一条下行回报
//     按分区推进已消费位置并去重, 重连重登按断点续传 (RUL-26/27/30)。
//   - 账户缓存: C98SessionCache (本柜台专属, fund 为键), 会话内 log_98 状态机
//     承担 98 账户登录状态/重复/在途控制; deal_cust_login (引擎线程) 为会话插入唯一入口
//     (不存在则插入, 存在则状态机幂等/合并); ATP 应答不回传的组包字段缓存于会话,
//     委托/查询组包经会话指针 (get_session) 按请求资金账户取身份字段。
//   - agw 会话号从登录应答 sender_comp_id 映射。

#include "counter98.h"
#include "api_config.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "c98_session_cache.h"
#include "matomic.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

// 类型实例化 (api_instance.cpp 通过模板调用本类, 此处仅做符号实例化)
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "gw_counter_direct.h"

#include <cstring>
#include <vector>
#include <string>
#include <unistd.h>

namespace lb_api
{

  namespace
  {
    /// 单包消息体上限(防御): 拦脏数据 —— body_length 为 uint32, 强转 int32 为负后
    /// 半包判定失效, deal_len 会负向游走 (对齐 g1 框架同类防御); 正常报文远小于此值,
    /// source 的 C98_MAX_MSG_LEN=2048 是发送侧预分配上限, 与此接收侧防御不冲突
    constexpr uint32_t C98_RECV_BODY_MAX = 1u << 20;

    /// AgwHeader 线格式 POD 镜像 (16B, 与协议线格式一致)。
    /// AgwHeader 继承 RootPacket 含虚函数, 不能对线格式字节直接 reinterpret_cast,
    /// 用此 POD 结构读写线格式 (对齐 source 的 raw_agw_header)。
    struct raw_agw_header
    {
      uint32_t msg_type;
      uint16_t msg_ver;
      uint16_t reserver;
      int32_t seq_num;      ///< 协议头序号 (98 柜台未使用, 恒 0)
      uint32_t body_length; ///< 仅含 Body, 不含 CheckSum 尾
    };
    static_assert(sizeof(raw_agw_header) == 16, "raw_agw_header must be 16 bytes");

    /// CheckSum: 对 [AgwHeader+Body] 逐字节求和 % 256 (对齐 source 的 calc_check_sum)
    uint32_t c98_calc_check_sum(const char *data, size_t len)
    {
      uint32_t sum = 0;
      for (size_t i = 0; i < len; ++i)
      {
        sum += static_cast<uint8_t>(data[i]);
      }
      return sum % 256;
    }

    /// 入站分发键: 高 32 位 = msg_type, 低 32 位 = msg_ver —— (type, ver) 组合键精确
    /// 匹配 (ver 不符按未知消息跳过不关链, RUL-26);
    /// case 常量的 ver 与 vendor 各消息类 GetMsgVer() 一致 (会话类 0 / 业务类 2)
    static constexpr uint64_t c98_disp_key(uint32_t msg_type, uint16_t msg_ver)
    {
      return (static_cast<uint64_t>(msg_type) << 32) | static_cast<uint64_t>(msg_ver);
    }

    /// 消息体解码 (失败返回 false; allow_msg_short 语义由框架兜底)
    template <typename MsgT>
    bool c98_decode_body(MsgT &msg, const char *body, size_t body_len)
    {
      return msg.PopFields(body, body_len, false) == mp::kSuccess;
    }
  } // namespace

  // ---- 构造 / 析构 ----
  counter98::counter98()
      : agw_login_state(0), market_type(0), gw_send_queue_(nullptr), trade_send_queue_(nullptr),
        agw_user_login_timeout_(10), cb_mgr_(nullptr), log_(nullptr)
  {
    std::memset(agw_session, 0, sizeof(agw_session));
  }

  counter98::~counter98() = default;

  // init
  int32 counter98::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log)
  {
    market_type = static_cast<int16>(cfg.get_market_type());    ///< 市场
    fast_counter_type_ = cfg.get_fast_counter_type();           ///< 极速柜台类型
    lb_common::atomic_store16(&agw_login_state, 0);             ///< agw 用户登陆状态:0-未登陆，1-登陆中，2-登陆成功
    lb_common::atomic_store32(&agw_login_err_, 0);              ///< agw 异步登录错误码清零
    lb_common::atomic_store16(&trade_link_connect_, 0);         ///< 98 链接状态:0-未链接/断开, 1-已链接
    have_agw_ok_ = 0;                                           ///< agw 未成功登陆过
    agw_user_login_timeout_ = cfg.get_agw_user_login_timeout(); ///< AGW 登录超时秒数（默认 10）
    if (agw_user_login_timeout_ < 5)
      agw_user_login_timeout_ = 5;
    heart_interval = cfg.get_heartbeat_interval(); ///< 心跳间隔
    // ---- 队列与回调 ----
    gw_send_queue_ = nullptr;    ///< 98柜台发送队列
    trade_send_queue_ = nullptr; ///< 极速柜台发送队列
    cb_mgr_ = cb;
    log_ = log;
    gw_eng_op_ = nullptr;    ///< 98链接引擎导出的链接相关操作
    trade_eng_op_ = nullptr; ///< 极速引擎导出的链接相关操作

    std::memset(agw_session, 0, sizeof(agw_session)); ///< 登录应答前为空 (应答从 sender_comp_id 映射)
    agw_user_ = cfg.get_agw98_user();
    agw_password_ = cfg.get_agw98_user_password();
    counter98_mode_ = cfg.get_counter98_mode(); ///< 0=同步登录, 1=快速登录(跳过回报同步)
    // 分区已消费位置种子 (兼回报同步断点起点): 配置 map 类型对齐 StreamInfo
    // (uint16 partno / int64 stream_seq, RUL-34), 柜台内部保持 vendor 报文宽度
    // (AgwPartition_def=uint8 / AgwIndex_def=int32), 此处做窄化转换复制;
    // 越界项(分区号>255 或记录号超 int32)记警告跳过 —— 静默截断会造出错误断点
    counter98_report_sync_.clear();
    for (const auto &kv : cfg.get_counter98_report_sync())
    {
      if (kv.first > 0xFF || kv.second < INT32_MIN || kv.second > INT32_MAX)
      {
        lb_common::lb_log_hand tlh(log_);
        warning_log(tlh) << "counter98_report_sync entry out of range skipped, partition=" << kv.first
                         << ", index=" << kv.second << end_log;
        continue;
      }
      counter98_report_sync_[static_cast<uint8_t>(kv.first)] = static_cast<int32_t>(kv.second);
    }

    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "init counter 98 ok,market_type=" << market_type << ",agw_user=" << cfg.get_agw98_user()
                  << ",agw_user_login_timeout=" << agw_user_login_timeout_
                  << ",fast_counter_type=" << static_cast<int32_t>(fast_counter_type_)
                  << ",heart_interval=" << heart_interval
                  << ",counter98_mode=" << counter98_mode_
                  << ",report_sync_partitions=" << counter98_report_sync_.size() << end_log;

    return 0;
  }

  // deal_logout_req: 柜台侧登出 —— 该账户 log_98 状态机 to_logout (置 LOGOUT)
  int32_t counter98::deal_logout_req(const LogoutReq &req)
  {
    C98SessionInfo *tsess = session_cache_.get_session(
        req.fund_account_id.data(), req.fund_account_id.size());
    if (tsess == nullptr)
    {
      return 0;
    }
    int32_t tret = tsess->log_98.to_logout();
    if (tret < 0)
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "cust logout but now logining,branch_id=" << req.branch_id.data()
                     << ",fund_account=" << req.fund_account_id.data()
                     << ",client_req_no=" << req.client_req_no << end_log;
      return LBAPI_ERR_STATE_LIMITED;
    }
    if (tret == 1)
    {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "counter 98 cust logout ok,branch_id=" << req.branch_id.data()
                    << ",fund_account=" << req.fund_account_id.data()
                    << ",client_req_no=" << req.client_req_no << end_log;
    }
    return 0;
  }

  // check_req_precond: 业务请求发送前提检查 (98 链接 + agw 登录成功 + 该账户 98 登录成功)。
  // 单次缓存查询: 通过时经 o_sess 返回会话指针供组包复用, 调用方无需再查缓存。
  int32 counter98::check_req_precond(const char *fund_account_id, size_t fund_len, C98SessionInfo *&o_sess)
  {
    o_sess = nullptr;
    if (unlikely(lb_common::atomic_load16(&trade_link_connect_) == 0))
    {
      return LBAPI_ERR_LINK_DISCONNECTED;
    }
    if (unlikely(lb_common::atomic_load16(&agw_login_state) != 2))
    {
      return LBAPI_ERR_NOT_LOG_AGW;
    }
    // 该账户 98 登录成功 (ATP 语义: 委托/查询前账户须登录)
    C98SessionInfo *sess = session_cache_.get_session(fund_account_id, fund_len);
    if (unlikely(sess == nullptr || !sess->log_98.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond: cust not login,fund_account=" << fund_account_id << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }
    o_sess = sess;
    return LBAPI_OK;
  }

  // ---- 组包与入队 (ATP 消息含变长 string 字段, 包长只能赋值后 GetSize 求得:
  //      先在栈上组完整包, 再按精确长度 take → memcpy → cmt, 无 take 后失败路径) ----

  // encode_pkt_to: AgwHeader(POD 直写) + 消息体 + 4B CheckSum 组完整包写入 o_buf, 返回总长
  // 线格式对齐 source: [AgwHeader 16B(主机序, POD memcpy)] [Body(主机序, mp 编码)]
  //                    [CheckSum 4B(网络序值)]; ver = GetMsgVer() (vendor 单一来源:
  //                    会话类登录/心跳/注销/回报同步=0, 业务类=2), seq_num=0;
  // CheckSum 覆盖 Header+Body 逐字节 %256 (source write_msg)
  int32 counter98::encode_pkt_to(mp::RootPacket &msg, char *o_buf, int32 buf_len)
  {
    mp::SerializeBuffer body_buf;
    if (msg.PushFields(body_buf, false) != mp::kSuccess)
    {
      return LBAPI_ERR_BUILD_MSG;
    }
    const int32 total = static_cast<int32>(sizeof(raw_agw_header) + body_buf.Size() + sizeof(uint32_t));
    if (o_buf == nullptr || total > buf_len)
    {
      return LBAPI_ERR_MSG_LEN;
    }
    raw_agw_header hdr{};
    hdr.msg_type = msg.GetTypeID();
    hdr.msg_ver = msg.GetMsgVer();
    hdr.reserver = 0;
    hdr.seq_num = 0;
    hdr.body_length = static_cast<uint32_t>(body_buf.Size());
    std::memcpy(o_buf, &hdr, sizeof(hdr));
    std::memcpy(o_buf + sizeof(hdr), body_buf.Begin(), body_buf.Size());
    const uint32_t cks = mp::HostToNetwork(
        c98_calc_check_sum(o_buf, static_cast<size_t>(sizeof(hdr) + body_buf.Size())));
    std::memcpy(o_buf + sizeof(hdr) + body_buf.Size(), &cks, sizeof(cks));
    return total;
  }

  // send_pkt: 填充完毕的消息(POD 头 + Body + CheckSum 尾)按精确长度入队
  // (先组包后 take, 编码失败不触碰队列)
  int32 counter98::send_pkt(mp::RootPacket &msg, int32 qevt_type)
  {
    mp::SerializeBuffer buf;
    raw_agw_header hdr{};
    hdr.msg_type = msg.GetTypeID();
    hdr.msg_ver = msg.GetMsgVer();
    hdr.reserver = 0;
    hdr.seq_num = 0;
    hdr.body_length = static_cast<uint32_t>(msg.GetSize());
    buf.Append(reinterpret_cast<const char *>(&hdr), sizeof(hdr));
    if (unlikely(msg.PushFields(buf, false) != mp::kSuccess))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter encode pkt error, msg_type=" << hdr.msg_type << end_log;
      return LBAPI_ERR_BUILD_MSG;
    }
    const uint32_t cks = mp::HostToNetwork(c98_calc_check_sum(buf.Begin(), buf.Size()));
    buf.Append(static_cast<uint32_t>(cks));

    const int32 take_len = static_cast<int32>(buf.Size());
    char *data = nullptr;
    int64 pos = take_req_que_mem(data, take_len, qevt_type);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter send pkt: queue full, pos=" << pos << ", msg_type=" << hdr.msg_type << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }
    std::memcpy(data + sizeof(link_send_event), buf.Begin(), buf.Size());
    cmt_req_que_mem(pos, take_len);
    return LBAPI_OK;
  }

  // ---- 业务发送函数 (模式: 前提检查 -> 会话快照 -> 构建消息 -> send_pkt) ----

  // deal_order_req: 买卖委托 (极速侧失败时的降级目标)
  int32 counter98::deal_order_req(const OrderReq &req)
  {
    // 发送前提: 98 链接已建立 + agw 登录成功 + 该账户 98 登录成功 (单次查询, sess 供组包)
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgCashAuctionOrder msg;
    build_order_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_etf_order_req: ETF 申购赎回 (共用现货集中竞价委托消息, 业务编码待确认)
  int32 counter98::deal_etf_order_req(const OrderReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgCashAuctionOrder msg;
    build_etf_order_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_bse_order_req: BSE 永远走 98 (共用现货集中竞价委托消息)
  int32 counter98::deal_bse_order_req(const OrderReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgCashAuctionOrder msg;
    build_bse_order_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_cancel_req: 撤单 (极速侧失败时的降级目标)
  int32_t counter98::deal_cancel_req(const CancelReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgCancelOrder msg;
    build_cancel_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_order_query: 委托查询
  int32 counter98::deal_order_query(const OrderQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryOrder msg;
    build_order_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_order_batch_query: 委托批量查询
  int32 counter98::deal_order_batch_query(const OrderBatchQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryOrder msg;
    build_order_batch_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_trade_query: 成交查询
  int32 counter98::deal_trade_query(const TradeQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryTradeOrder msg;
    build_trade_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_trade_batch_query: 成交批量查询
  int32 counter98::deal_trade_batch_query(const TradeBatchQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryTradeOrder msg;
    build_trade_batch_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_fund_query: 资金查询
  int32 counter98::deal_fund_query(const FundQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryFund msg;
    build_fund_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_position_query: 持仓查询
  int32 counter98::deal_position_query(const PositionQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQuerySelfShare msg;
    build_position_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // ---- 98 扩展查询发送 (98 柜台扩展查询消息集, RUL-26; 模式同上六个查询) ----

  // deal_max_order_qty_query: 最大可委托数查询
  int32 counter98::deal_max_order_qty_query(const MaxOrderQtyQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryMaxOrderQty msg;
    build_max_order_qty_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_total_fund_asset_query: 客户总资产查询
  int32 counter98::deal_total_fund_asset_query(const TotalFundAssetQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExternalQueryTotalFundAsset msg;
    build_total_fund_asset_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_fund_maxdraw_query: 可取资金查询
  int32 counter98::deal_fund_maxdraw_query(const FundMaxdrawQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryFundMaxdraw msg;
    build_fund_maxdraw_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_cust_margin_avl_query: 融资融券信用数据(可用保证金)查询
  int32 counter98::deal_cust_margin_avl_query(const CustMarginAvlQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryCustMarginAvl msg;
    build_cust_margin_avl_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_asset_debts_query: 资产负债查询
  int32 counter98::deal_asset_debts_query(const AssetDebtsQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryAssetDebts msg;
    build_asset_debts_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // deal_credit_max_order_qty_query: 两融最大可委托数查询
  int32 counter98::deal_credit_max_order_qty_query(const CreditMaxOrderQtyQueryReq &req)
  {
    C98SessionInfo *sess = nullptr;
    int32 tret = check_req_precond(req.fund_account_id.data(), req.fund_account_id.size(), sess);
    if (unlikely(tret != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98counter req precond fail,ret=" << tret
                     << ",fund_account=" << req.fund_account_id.data() << end_log;
      return tret;
    }

    lb_counter98::AgwMsgExtQueryCreditMaxOrderQty msg;
    build_credit_max_order_qty_query_msg(req, sess, msg);
    return send_pkt(msg);
  }

  // ---- 消息构建: 填充消息对象 (组包入队统一走 send_pkt) ----

  // build_order_msg: 现货集中竞价委托 (消息号 100101; 普通/ETF/BSE 共用)
  void counter98::build_order_msg(const OrderReq &req, const C98SessionInfo *sess,
                                  lb_counter98::AgwMsgCashAuctionOrder &msg)
  {
    fill_order_base(msg, sess, req.security_id);
    msg.business_type = 1; ///< source: 1=现货集中竞价 (普通委托)
    msg.market_id = static_cast<uint16_t>(market_type);
    msg.client_order_time = 0;       ///< [待确认] 时间戳格式
    msg.side = req.side;             ///< 字典一致: API side 即 AgwSide ('1'买/'2'卖) 透传
    msg.order_type = req.order_type; ///< 字典一致: API order_type 即 AgwOrdType 透传
    msg.price = req.order_price;     ///< N13(4) 与 API ×10000 直传
    msg.order_qty = req.order_qty;   ///< 数量: API 与 ATP N15(2) 均 ×100, 直传 (RUL-35)
    msg.client_seq_id = req.client_seq_id;
    msg.policy_id = req.policy_id;
    // 按 source 不填: enforce_flag/stop_px/min_qty/max_price_levels/time_in_force/cash_margin
    // (走协议构造默认 0/0/0/0/' '/' ')
  }

  // build_etf_order_msg: ETF 申购赎回 (共用委托消息)
  void counter98::build_etf_order_msg(const OrderReq &req, const C98SessionInfo *sess,
                                      lb_counter98::AgwMsgCashAuctionOrder &msg)
  {
    build_order_msg(req, sess, msg);
    // 【待确认·联调】ETF business_type: 取 12 (AgwBusinessType::kETFRealTimePurchaseRedemption,
    // 2=质押式回购), 联调对照柜台字典裁决; 若裁决为 2 需同步修订 RUL-22。
    msg.business_type = 12;
  }

  // build_bse_order_msg: BSE 委托 (共用委托消息, source: business_type=1)
  void counter98::build_bse_order_msg(const OrderReq &req, const C98SessionInfo *sess,
                                      lb_counter98::AgwMsgCashAuctionOrder &msg)
  {
    build_order_msg(req, sess, msg);                        ///< source: BSE 同现货集中竞价
    msg.market_id = static_cast<uint16_t>(MARKET_TYPE_BSE); ///< 北交所按柜台字典报北京市场 (109, 覆盖实例配置市场)
  }

  // build_cancel_msg: 通用撤单 (消息号 190001; 字段对齐 source)
  void counter98::build_cancel_msg(const CancelReq &req, const C98SessionInfo *sess,
                                   lb_counter98::AgwMsgCancelOrder &msg)
  {
    std::array<char, MAX_SECURITY_ID_LEN> tempty_security = {}; ///< 撤单不携带证券代码 (全 NUL)
    fill_order_base(msg, sess, tempty_security);
    // 按 source: business_type/side/order_type/price/order_qty 走构造默认 (0/' '/' '/0/0)
    msg.market_id = static_cast<uint16_t>(market_type);
    msg.client_seq_id = req.client_req_no;      ///< source: 撤单请求的客户报单号
    msg.orig_cli_ord_no = req.order_sys_no;     ///< 原委托柜台委托号 (回报 client_order_no)
    msg.orig_client_seq_id = req.client_seq_id; ///< 原委托的用户私有报单号
  }

  // build_order_query_msg: 委托查询 (消息号 300002; 条件字段对齐 source: 走构造默认)
  void counter98::build_order_query_msg(const OrderQueryReq &req, const C98SessionInfo *sess,
                                        lb_counter98::AgwMsgExtQueryOrder &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.cl_ord_no = req.client_seq_id; ///< source: 委托查询按用户报单号 (0=查全部)
    // 按 source 不填: market_id/security_id/business_type/side/order_query_condition/
    // query_index/return_num/return_seq/flag/cancel_order_flag (构造默认 0/空格/' ')
  }

  // build_order_batch_query_msg: 委托批量查询 (对齐 source)
  void counter98::build_order_batch_query_msg(const OrderBatchQueryReq &req, const C98SessionInfo *sess,
                                              lb_counter98::AgwMsgExtQueryOrder &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.cl_ord_no = req.order_sys_no_begin; ///< source: 起始委托号 (0=查全部)
    // 按 source 不填 return_num 等条件字段 (构造默认 0)
  }

  // build_trade_query_msg: 成交查询 (消息号 300003; 对齐 source)
  void counter98::build_trade_query_msg(const TradeQueryReq &req, const C98SessionInfo *sess,
                                        lb_counter98::AgwMsgExtQueryTradeOrder &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.cl_ord_no = req.client_seq_id; ///< source: 成交查询按用户报单号 (0=查全部)
    // 按 source 不填其余条件字段 (构造默认)
  }

  // build_trade_batch_query_msg: 成交批量查询 (对齐 source: 不带条件查全部)
  void counter98::build_trade_batch_query_msg(const TradeBatchQueryReq &req, const C98SessionInfo *sess,
                                              lb_counter98::AgwMsgExtQueryTradeOrder &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    // 按 source: 全部条件字段走构造默认 (cl_ord_no=0 查全部)
  }

  // build_fund_query_msg: 资金查询 (消息号 300004; 对齐 source: currency 走构造默认 4 空格)
  void counter98::build_fund_query_msg(const FundQueryReq &req, const C98SessionInfo *sess,
                                       lb_counter98::AgwMsgExtQueryFund &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
  }

  // build_position_query_msg: 持仓查询 (AgwMsgExtQuerySelfShare 自研股份查询, 消息号 7300714; 对齐 source)
  void counter98::build_position_query_msg(const PositionQueryReq &req, const C98SessionInfo *sess,
                                           lb_counter98::AgwMsgExtQuerySelfShare &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.market_id = static_cast<uint16_t>(req.market_type);
    msg.security_id = req.security_id; ///< 原样赋值, 不空格填充 (RUL-31); 全 NUL=查全部证券
    msg.query_index = req.page_ctrl.query_index;
    msg.return_num = static_cast<uint32_t>(req.page_ctrl.return_num);
    // 按 source 不填: currency/mkt_cal_flag/flag/security_class (构造默认)
  }

  // ---- 98 扩展查询消息构建 (字段映射对齐 source 组包, RUL-26) ----

  // build_max_order_qty_query_msg: 最大可委托数查询 (300001)
  void counter98::build_max_order_qty_query_msg(const MaxOrderQtyQueryReq &req, const C98SessionInfo *sess,
                                                lb_counter98::AgwMsgExtQueryMaxOrderQty &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.market_id = static_cast<uint16_t>(req.market_type);
    msg.security_id = req.security_id; ///< 原样赋值, 不空格填充 (RUL-31)
    msg.business_type = req.business_type;
    msg.side = req.side;
    msg.price = req.price;
    msg.sh_account_id = to_atp_fixed(req.sh_account_id.data(), msg.sh_account_id);
    msg.sz_account_id = to_atp_fixed(req.sz_account_id.data(), msg.sz_account_id);
  }

  // build_total_fund_asset_query_msg: 客户总资产查询 (7301735; 仅基类字段)
  void counter98::build_total_fund_asset_query_msg(const TotalFundAssetQueryReq &req, const C98SessionInfo *sess,
                                                   lb_counter98::AgwMsgExternalQueryTotalFundAsset &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
  }

  // build_fund_maxdraw_query_msg: 可取资金查询 (7390701; currency 空=按配置默认)
  void counter98::build_fund_maxdraw_query_msg(const FundMaxdrawQueryReq &req, const C98SessionInfo *sess,
                                               lb_counter98::AgwMsgExtQueryFundMaxdraw &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.currency = to_atp_fixed(req.currency.data(), msg.currency);
  }

  // build_cust_margin_avl_query_msg: 融资融券信用数据查询 (7390724)
  void counter98::build_cust_margin_avl_query_msg(const CustMarginAvlQueryReq &req, const C98SessionInfo *sess,
                                                  lb_counter98::AgwMsgExtQueryCustMarginAvl &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.currency = to_atp_fixed(req.currency.data(), msg.currency);
    msg.market_id = static_cast<uint16_t>(req.market_type);
    msg.security_id = req.security_id; ///< 原样赋值, 不空格填充 (RUL-31)
    msg.credit_direct = req.credit_direct;
  }

  // build_asset_debts_query_msg: 资产负债查询 (7390700)
  void counter98::build_asset_debts_query_msg(const AssetDebtsQueryReq &req, const C98SessionInfo *sess,
                                              lb_counter98::AgwMsgExtQueryAssetDebts &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.currency = to_atp_fixed(req.currency.data(), msg.currency);
  }

  // build_credit_max_order_qty_query_msg: 两融最大可委托数查询 (7390710)
  void counter98::build_credit_max_order_qty_query_msg(const CreditMaxOrderQtyQueryReq &req,
                                                       const C98SessionInfo *sess,
                                                       lb_counter98::AgwMsgExtQueryCreditMaxOrderQty &msg)
  {
    fill_query_base(msg, sess, req.client_req_no);
    msg.market_id = static_cast<uint16_t>(req.market_type);
    msg.security_id = req.security_id; ///< 原样赋值, 不空格填充 (RUL-31)
    msg.business_type = req.business_type;
    msg.side = req.side;
    msg.price = req.price;
  }

  // ---- agw 用户登录 (消息号 3) ----

  // build_agw_login_msg: 构造 agw 用户登录消息 (AgwMsgLogin; 字段对齐 source:
  // 只填 sender_comp_id/password/heart_bt_int/spec_cust_flag, 其余走构造默认 ——
  // target_comp_id 全空格/default_app_ver_id 全空格/trade_pwd "")
  void counter98::build_agw_login_msg(lb_counter98::AgwMsgLogin &msg)
  {
    to_atp_str(msg.sender_comp_id, agw_user_.c_str()); ///< [待确认] 发送方代码 = agw 用户名
    msg.heart_bt_int = static_cast<lb_counter98::HeartBtInt_def>(heart_interval);
    to_atp_str(msg.password, agw_password_.c_str());
    msg.spec_cust_flag = 0;
    // return encode_pkt_to(msg, o_buf, buf_len);
  }

  // deal_agwuser_login_ans: 处理 agw 登录应答
  // [待确认·重要] AgwMsgLogin 结构不含错误字段, 当前语义: 收到同号(3)入站消息即视为登录成功;
  // 会话号从应答的 sender_comp_id (服务器侧发送方代码) 映射
  void counter98::deal_agwuser_login_ans(const lb_counter98::AgwMsgLogin &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "agw user login answer msg, sender=" << msg.sender_comp_id.data()
                  << ", target=" << msg.target_comp_id.data() << end_log;

    // 会话号从应答 sender_comp_id 映射 (对齐 source: 32B 原样 memcpy + 尾部 NUL)
    std::memcpy(agw_session, msg.sender_comp_id.data(), std::min(sizeof(agw_session), sizeof(msg.sender_comp_id)));
    agw_session[sizeof(agw_session) - 1] = '\0';

    // 回报同步 (RUL-26/27): agw 登录成功即发起; 重复成功
    // 应答会重复发送 (柜台按分区记录号幂等续传, 可接受); 内含模式/配置门控。
    // 需要同步而发送失败 = agw 登录失败 (RUL-27/28): 置错误码(同步等待方
    // 若未返回则读到真实失败码) + 复位状态(1→0, 此刻成功标志尚未置位、状态仍在途) +
    // 关链 —— have_agw_ok_ 未置, 重连后需用户重新 login (同步 agw 登录); 队满为暂态,
    // 受重连间隔治理。先复位再关链: deal_link_close 读到 0 不覆盖 err, 保留真实失败码
    int32_t tsync = send_agw_sync();
    if (tsync != LBAPI_OK)
    {
      error_log(tlh) << "agw login ok but send report sync failed,ret=" << tsync
                     << ", treat as agw login fail, close 98 link" << end_log;
      lb_common::atomic_store32(&agw_login_err_, tsync);
      int16 tstate = 1;
      if (!lb_common::atomic_cas16(&agw_login_state, &tstate, 0))
      {
        // 状态已被并发改动 (理论不可达: 本函数为引擎线程唯一写者, 记日志防御)
        warning_log(tlh) << "agw login state changed unexpectedly,state=" << tstate << end_log;
      }
      if (gw_eng_op_ != nullptr)
      {
        gw_eng_op_->deal_close_link(LINK_TYPE_98, tsync);
      }
      return;
    }

    // 成功: 无条件接受 (重复成功幂等), 结果经 deal_agw_login 同步接口返回码告知
    lb_common::atomic_store32(&agw_login_err_, 0);
    lb_common::atomic_store16(&agw_login_state, 2);
    have_agw_ok_ = 1;
  }

  // ---- 回报同步 (AgwMsgSync=5 / AgwMsgSyncFinish=6, 会话类 ver=0) ----

  // build_sync_msg: 按 counter98_mode/counter98_report_sync_ 填充回报同步请求
  //   (0,-1) 特殊形态 = 同步全部分区; 其余逐分区携带起点记录号
  void counter98::build_sync_msg(lb_counter98::AgwMsgSync &msg)
  {
    if (counter98_mode_ != 0)
    {
      return; ///< 快速登录模式不组回报同步 (send_agw_sync 已门控, 此为防御)
    }
    auto it = counter98_report_sync_.find(0);
    if (it != counter98_report_sync_.end() && it->second == -1)
    {
      lb_counter98::AgwNoPartitionArray info;
      info.partition = 0;
      info.index = -1; ///< partition=0, index=-1 → 同步全部分区
      msg.no_parition_array.push_back(info);
      return;
    }
    for (const auto &item : counter98_report_sync_)
    {
      lb_counter98::AgwNoPartitionArray info;
      info.partition = item.first;
      info.index = item.second;
      msg.no_parition_array.push_back(info);
    }
  }

  // send_agw_sync: 发送回报同步请求 (agw 登录成功后, 引擎线程调用)
  //   跳过条件(返回 OK, "不需要 sync"不算失败): 快速登录模式 / 同步登录模式但无分区配置;
  //   发送失败返回错误码 —— 调用方 (deal_agwuser_login_ans) 按 agw 登录失败处理
  int32_t counter98::send_agw_sync()
  {
    lb_common::lb_log_hand tlh(log_);

    if (counter98_mode_ == 1)
    {
      info_log(tlh) << "skip report sync, counter98_mode=1 (fast login)" << end_log;
      return LBAPI_OK;
    }
    if (counter98_report_sync_.empty())
    {
      info_log(tlh) << "skip report sync, no counter98_report_sync config" << end_log;
      return LBAPI_OK;
    }

    lb_counter98::AgwMsgSync msg;
    build_sync_msg(msg);
    int32 ret = send_pkt(msg);
    if (ret != LBAPI_OK)
    {
      error_log(tlh) << "send report sync error,ret=" << ret
                     << ",partition_count=" << counter98_report_sync_.size() << end_log;
      return ret;
    }
    info_log(tlh) << "send report sync ok,mode=" << counter98_mode_
                  << ",partition_count=" << counter98_report_sync_.size() << end_log;
    return LBAPI_OK;
  }

  // deal_sync_finish_ans: 回报同步结束 (空消息体) —— 经 on_event(report_sync_finish)
  // 回调用户 (RUL-27)。断点推进/去重不依赖本应答 (consume_report_pos
  // 始终生效, RUL-30); 重复到达重复通知 (到达驱动事件)
  void counter98::deal_sync_finish_ans(const lb_counter98::AgwMsgSyncFinish &msg)
  {
    (void)msg; ///< AgwMsgSyncFinish 无业务字段
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "report sync finish ok" << end_log;
    cb_mgr_->on_event(api_event_type::report_sync_finish, 0, "98 report sync finished");
  }

  // consume_report_pos: 消费一条下行回报记录 (AgwMsgOrderReport 派生: 委托确认 290001/
  // 撤单应答 190001/成交回报 210115) —— 分区去重 + 断点推进统一收口 (RUL-30):
  //   index<=0      → 视为未填记录号, 直通不判重不推进 (安全例外: 若 0 为合法首条记录
  //                   仅该条不去重, 不误杀; 联调确认记录号起算/未填形态)
  //   分区无记录    → 登记 index 并投递 (新分区首条)
  //   index<=已存值 → 重复回报, 丢弃
  //   index>已存值  → 推进断点并投递 (断点即重连重登 AgwMsgSync 续传起点)
  // 仅引擎线程调用 (与 build_sync_msg 同线程无竞争); init 配置种子先于 start 写入
  bool counter98::consume_report_pos(const lb_counter98::AgwMsgOrderReport &rpt)
  {
    if (rpt.index <= 0)
    {
      return true;
    }
    auto it = counter98_report_sync_.find(rpt.partition);
    if (it == counter98_report_sync_.end())
    {
      counter98_report_sync_[rpt.partition] = rpt.index;
      return true;
    }
    if (rpt.index <= it->second)
    {
      return false; ///< 重复回报 (分区记录号未超过已消费位置)
    }
    it->second = rpt.index;
    return true;
  }

  void counter98::error_agwuser_login(int32 err_code)
  {
    // 失败应答仅在"登陆中"(1)接受: 设置异步错误码供同步等待方读取返回, 并复位状态;
    // 其余状态忽略 (已成功(2): 旧失败应答迟到, 当前会话仍有效; 未登录(0): 无在途无主应答)
    // 先写错误码再 CAS 复位状态: 同步等待方(deal_agw_login)以 state==0 为结果可见点,
    // 先 CAS 后写会留出读到旧错误码(0)的窗口, 使真实失败原因被泛化为 LOGIN_FAIL;
    // CAS 失败时多写的错误码无消费者 (等待方仅在自己发起的请求周期内读, 发起时先清零)
    lb_common::atomic_store32(&agw_login_err_, err_code);
    int16 t = 1;
    if (lb_common::atomic_cas16(&agw_login_state, &t, 0))
    {
      // agw 登录与 98 链接绑定: 登录流程任何阶段失败 (组包/发送/应答, 均经本函数) 关闭链接,
      // 重连后由 deal_link_connect 依据 have_agw_ok_ 重新发起; 从未成功过则等用户重新 login
      if (gw_eng_op_ != nullptr)
      {
        gw_eng_op_->deal_close_link(LINK_TYPE_98, err_code);
      }
    }
    else
    {
      lb_common::lb_log_hand tlh(log_);
      warning_log(tlh) << "ignore stale agw login fail ans,cur_state=" << t << end_log;
    }
  }

  // deal_agw_login: 发起 agw 用户登录, 同步等待结果
  // 失败错误码来自异步失败点设置的 agw_login_err_ (应答失败/引擎发送失败/链接断开等)
  int32 counter98::deal_agw_login()
  {
    lb_common::lb_log_hand tlh(log_);

    int32 tn = agw_user_login_timeout_ * 10000;
    while (1)
    {
      int16 t = lb_common::atomic_load16(&agw_login_state);
      if (t == 0)
      {
        if (lb_common::atomic_cas16(&agw_login_state, &t, 1))
        {
          break; ///< 抢占成功, 由本线程投递
        }
      }
      else if (t == 1)
      {
        lb_common::comm_utils::sleep_us(100);
        tn--;
        if (tn <= 0)
        {
          // 等待在途请求超时: 直接返回, 不再投递 (在途请求仍在, 重复投递会造成双请求双应答;
          // 在途结果到达后状态自愈, 用户可再次 login 幂等获取)
          error_log(tlh) << "agw user login wait in-flight timeout" << end_log;
          return LBAPI_ERR_LOGIN_TIMEOUT;
        }
      }
      else
      {
        info_log(tlh) << "agw user have login" << end_log;
        return LBAPI_OK;
      }
    }

    // 新请求开始: 清除上次的异步错误码
    lb_common::atomic_store32(&agw_login_err_, 0);

    int32 ret = delive_agw_login_event();
    if (ret != 0)
    {
      return ret; // 队满: delive_agw_login_event 内部已设置错误码并复位状态
    }

    // 同步等待 agw 登录结果 (超时仅适用登录阶段)
    tn = agw_user_login_timeout_ * 10000;
    while (1)
    {
      int16 t = lb_common::atomic_load16(&agw_login_state);
      if (t == 0)
      {
        // 失败: 返回异步失败点设置的错误码 (未设置则兜底通用失败码)
        int32 terr = lb_common::atomic_load32(&agw_login_err_);
        return (terr != 0) ? terr : LBAPI_ERR_LOGIN_FAIL;
      }
      else if (t == 1)
      {
        tn--;
        if (tn <= 0)
        {
          if (lb_common::atomic_cas16(&agw_login_state, &t, 0))
          {
            error_log(tlh) << "agw user login timeout" << end_log;
            return LBAPI_ERR_LOGIN_TIMEOUT;
          }
        }
        else
        {
          lb_common::comm_utils::sleep_us(100);
        }
      }
      else
      {
        info_log(tlh) << "agw user login ok" << end_log;
        return LBAPI_OK;
      }
    }

    return LBAPI_ERR_LOGIN_FAIL;
  }

  // delive_agw_login_event: 投递 AGWUSER_LOGIN 事件 (deal_agw_login / 重连重登共用)
  // 前置: 调用方已占位 agw_login_state=1 (先占位后投递, 保证与并发发起方互斥不重复投递)
  int32 counter98::delive_agw_login_event()
  {
    lb_common::lb_log_hand tlh(log_);

    // 再次检查
    if (lb_common::atomic_load16(&agw_login_state) == 2)
    {
      return 0;
    }

    lb_counter98::AgwMsgLogin msg;
    build_agw_login_msg(msg);
    int32 ret = send_pkt(msg, LINK_EVENT_TYPE_AGWUSER_LOGIN);
    if (ret < 0)
    {
      // 投递失败: 设置异步错误码并复位状态 (同步等待方读取返回)
      lb_common::atomic_store32(&agw_login_err_, LBAPI_ERR_SEND_QUEUE_FULL);
      lb_common::atomic_store16(&agw_login_state, 0);
      error_log(tlh) << "add agw user login event error,ret=" << ret << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }
    info_log(tlh) << "add agw user login event ok" << end_log;
    return 0;
  }

  // ---- 账户登录 (消息号 800005/800006; 会话缓存 + log_98 状态机驱动) ----

  // deal_login_req: 发起账户登录 (用户线程; 内部完成 LoginReq → acc_login_event_info 转换)
  // 前置: agw 已登录成功 (api_impl::login 第一步 deal_agw_login 保证)
  int32 counter98::deal_login_req(const LoginReq &req)
  {
    lb_common::lb_log_hand tlh(log_);

    // 转换 LoginReq → acc_login_event_info
    acc_login_event_info info;
    build_cust_login_event(req, info);

    // 即使 98 账户已登录成功(log_98 val==OK) 仍投递事件，让事件处理去控制分级跳过98 账户登录
    // 投账户登录事件到 multi 引擎 (98 步骤不设在途缓存: 账户一旦成功登录永不再次登录,
    // 并发首次登录的重复消息由引擎侧 deal_cust_login 的会话存在性 + log_98 在途合并消化)
    char *data = nullptr;
    int64 pos = take_req_que_mem(data, sizeof(acc_login_event_info), LINK_EVENT_TYPE_ACCOUNT_LOGIN);
    if (pos <= 0)
    {
      error_log(tlh) << "add 98 cust login event to queue error,branch_id=" << info.branch_id
                     << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                     << ",client_req_no=" << info.cust_req_no << ",ret= " << pos << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }
    std::memcpy(data + sizeof(link_send_event), &info, sizeof(acc_login_event_info));
    cmt_req_que_mem(pos, sizeof(acc_login_event_info));

    info_log(tlh) << "add 98 cust login event to queue,branch_id=" << info.branch_id
                  << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                  << ",client_req_no=" << info.cust_req_no << end_log;
    return LBAPI_OK;
  }

  void counter98::build_cust_login_event(const LoginReq &req, acc_login_event_info &o_info)
  {
    o_info.cust_req_no = req.client_req_no;
    lb_common::comm_utils::str_copy_format(o_info.cust_id, req.cust_id.data(), sizeof(o_info.cust_id));
    lb_common::comm_utils::str_copy_format(o_info.fund_account_id, req.fund_account_id.data(),
                                           sizeof(o_info.fund_account_id));
    lb_common::comm_utils::str_copy_format(o_info.account_id, req.account_id.data(), sizeof(o_info.account_id));
    lb_common::comm_utils::str_copy_safe(o_info.branch_id, (int32)sizeof(o_info.branch_id),
                                         req.branch_id.data(), (int32)sizeof(req.branch_id));

    o_info.order_way_ext[0] = req.order_way_ext[0];
    o_info.order_way_ext[1] = req.order_way_ext[1];
    std::memcpy(o_info.session, agw_session, sizeof(o_info.session)); ///< agw 登录应答回填的会话号
    // 用户数组源不保证 NUL 后清零: 经 str_copy_format 规范化(有效字符后全\0)
    lb_common::comm_utils::str_copy_format(o_info.password, req.password.data(), (int32)sizeof(o_info.password));
    lb_common::comm_utils::str_copy_format(o_info.user_info, req.user_info.data(), (int32)sizeof(o_info.user_info));
    lb_common::comm_utils::str_copy_format(o_info.client_feature_code, req.client_feature_code.data(),
                                           (int32)sizeof(o_info.client_feature_code));
    o_info.user_origin = 1;
    o_info.filled = 0;
  }

  // deal_cust_login: 引擎消费 ACCOUNT_LOGIN 事件 (会话插入唯一入口, 引擎线程):
  //   会话不存在 → 插入会话缓存 + log_98 占位(USER) + 组包发送 800005;
  //   会话存在   → 该账户 log_98.to_login: 已登录→幂等直达极速柜台登录 / 在途→合并忽略 /
  //               NONE/LOGOUT→覆盖会话字段+占位发送。
  // 成功返回消息长度 (含消息头), 0-不需重复登陆, <0 出错
  int32 counter98::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len)
  {
    lb_common::lb_log_hand tlh(log_);

    C98SessionInfo *sess = session_cache_.get_session(req.fund_account_id, sizeof(req.fund_account_id));
    const bool tfresh = (sess == nullptr);
    if (tfresh)
    {
      // 不存在则插入 (map 增删唯一入口: 本引擎线程)
      session_cache_.create_session(req);
      sess = session_cache_.get_session(req.fund_account_id, sizeof(req.fund_account_id));
    }

    if (unlikely(sess == nullptr))
    {
      error_log(tlh) << "98 cust login but session absent after create,fund_account=" << req.fund_account_id << end_log;
      // ans_cust_login(req, LBAPI_ERR_STATE_LIMITED, "98 login session create error");
      return LBAPI_ERR_STATE_LIMITED;
    }
    int32 thavelog = 0;
    int32 tret = sess->log_98.to_login(thavelog, req.user_origin);
    if (tret == 0)
    {
      if (thavelog == 1)
      {
        // 幂等直达: 用会话缓存字段刷新事件副本 (应答回填值), 再投递极速柜台
        acc_login_event_info info = req;
        lb_common::comm_utils::str_copy_format(info.cust_id, sess->cust_id.data(), sizeof(info.cust_id));
        lb_common::comm_utils::str_copy_format(info.account_id, sess->account_id.data(), sizeof(info.account_id));
        lb_common::comm_utils::str_copy_safe(info.branch_id, (int32)sizeof(info.branch_id),
                                             sess->branch_id.data(), (int32)sizeof(sess->branch_id));

        if (!tfresh && req.user_origin == 1)
        {
          // 保存最新的账户信息
          session_cache_.update_session_info(sess, req);
        }
        // 已登录账户重复登录事件: 幂等直达极速柜台登录 (deal_login_req 已分级, 此为防御)
        info_log(tlh) << "98 cust have login ok (engine path), direct to fast counter login,fund_account="
                      << req.fund_account_id << ",client_req_no=" << req.cust_req_no << end_log;
        return delive_fast_counter_login(info);
      }
      return 0; ///< 在途合并, 忽略
    }

    if (!tfresh && req.user_origin == 1)
    {
      // 保存最新的账户信息
      session_cache_.update_session_info(sess, req);
    }

    return build_login_msg(req, o_buf, buf_len);
  }

  // build_login_msg: 构造 ATP 账户登录消息 (AgwMsgCustLoginReq, 消息号 800005)
  int32 counter98::build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len)
  {
    lb_counter98::AgwMsgCustLoginReq msg;
    msg.time = 0; ///< [待确认] 发起验证时间格式
    msg.client_seq_id = info.cust_req_no;
    msg.client_feature_code = std::string(info.client_feature_code);
    to_atp_str(msg.cust_id, info.cust_id);
    to_atp_str(msg.fund_account_id, info.fund_account_id);
    to_atp_str(msg.branch_id, info.branch_id);
    to_atp_str(msg.account_id, info.account_id);
    to_atp_str(msg.user_info, info.user_info);
    msg.extra_data = "";
    // 组包对齐 source 消息包结构 (RUL-26): order_way 填 0,
    // 委托方式仅经 order_way_ext 两字符携带
    msg.order_way = 0;
    msg.password = std::string(info.password); ///< 明文 [待确认] pwd_type
    msg.pwd_type = 0;
    msg.order_way_ext[0] = info.order_way_ext[0];
    msg.order_way_ext[1] = info.order_way_ext[1];

    const int32 need = static_cast<int32>(lb_counter98::AGW_HEADER_SIZE) + static_cast<int32>(msg.GetSize());
    if (o_buf == nullptr || buf_len < need)
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "98 user login and msg len error,branch_id=" << info.branch_id
                     << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                     << ",client_req_no=" << info.cust_req_no << end_log;
      // ans_cust_login(info, LBAPI_ERR_BUILD_MSG, "build fast counter login msg error");
      return LBAPI_ERR_MSG_LEN;
    }
    return encode_pkt_to(msg, o_buf, buf_len);
  }

  int32 counter98::delive_fast_counter_login(const acc_login_event_info &info)
  {
    lb_common::lb_log_hand tlh(log_);

    // note : 这里对极速柜台做了特殊处理，因为个微和fpga柜台登陆的不一致。
    // fpga 用户下线的降级判定不在此处: 柜台 deal_cust_login 在 to_login 占位后判定
    // (fpga_state 非正常(故障/故障切换柜台) → 不登录 FPGA, 本地降级成功回调)
    lb_common::que_mth_buf *tq = nullptr;
    int16 tlink_type = 0;
    if (fast_counter_type_ == counter_type::gw_direct)
    {
      tlink_type = LINK_TYPE_SPEED_TRADE;
      tq = trade_send_queue_;
    }
    else
    {
      tlink_type = LINK_TYPE_SPEED_GW;
      tq = gw_send_queue_;
    }

    int32 total_len = sizeof(link_send_event) + sizeof(acc_login_event_info);
    char *data = nullptr;
    int64 pos = tq->write_get_mth(data, total_len);
    if (pos <= 0)
    {
      error_log(tlh) << "add fast counter cust login event to queue error,branch_id=" << info.branch_id
                     << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                     << ",client_req_no=" << info.cust_req_no << ",ret= " << pos << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    link_send_event *te_head = reinterpret_cast<link_send_event *>(data);
    te_head->link_type = tlink_type;
    te_head->type = LINK_EVENT_TYPE_ACCOUNT_LOGIN;
    te_head->data_len = sizeof(acc_login_event_info);

    std::memcpy(te_head->data, (const char *)(&info), sizeof(acc_login_event_info));

    // NOTE: 实际链接 link_type (LINK_TYPE_SPEED_TRADE) 应由 fast_engine 消费时识别
    tq->write_cmt_mth(pos, total_len);
    if (tlink_type == LINK_TYPE_SPEED_GW)
    {
      gw_eng_op_->trigger_send();
    }
    else
    {
      trade_eng_op_->trigger_send();
    }

    info_log(tlh) << "add fast counter cust login event to queue,branch_id=" << info.branch_id
                  << ",fund_account=" << info.fund_account_id << ",session=" << info.session
                  << ",client_req_no=" << info.cust_req_no << end_log;

    return LBAPI_OK;
  }

  void counter98::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg)
  {
    LoginAns ans;

    ans.client_req_no = req.cust_req_no;
    std::memcpy(ans.cust_id.data(), req.cust_id, sizeof(ans.cust_id));
    std::memcpy(ans.fund_account_id.data(), req.fund_account_id, sizeof(ans.fund_account_id));
    std::memcpy(ans.account_id.data(), req.account_id, sizeof(ans.account_id));
    std::memcpy(ans.branch_id.data(), req.branch_id, sizeof(ans.branch_id));
    ans.market_type = market_type;
    ans.err_code = err_ret;
    if (err_msg != nullptr)
    {
      lb_common::comm_utils::str_copy_format(ans.err_msg.data(), err_msg, sizeof(ans.err_msg));
    }
    else
    {
      std::memset(ans.err_msg.data(), 0, sizeof(ans.err_msg));
    }
    ans.login_time = 0; ///< 登陆时间 (本地构造无协议应答, 不填充)

    C98SessionInfo *tsess = session_cache_.get_session(req.fund_account_id, sizeof(req.fund_account_id));
    int32 tpending = 0;
    if (tsess != nullptr)
    {
      tpending = tsess->log_98.clear_pending();
    }
    // 门控形态与 gw/fpga (仅 !=0) 有意不同: 98 的分级直达失败 (幂等直达投递队满) 以负值
    // 穿出 deal_cust_login 且该路径无 pending, 需 user_origin 条件覆盖以保证终态回调
    if (req.user_origin == 1 || tpending != 0)
    {
      cb_mgr_->on_login(ans);
    }
  }

  // deal_cust_login_ans: 处理 ATP 账户登录应答 (AgwMsgCustLoginResp, 800006, 引擎线程)
  // 遍历应答资金账户组, 逐个匹配已缓存的会话: 成功回填 + log_98 置 OK + 投递极速柜台登录;
  // 失败走 log_98 统一处理 (已成功账户忽略迟到失败并仍投递极速柜台登录)
  void counter98::deal_cust_login_ans(const lb_counter98::AgwMsgCustLoginResp &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    // 成功判定 [对齐 source]: key_error_code 为空 (错误代码串); permisson_error_code 仅记日志
    const bool tok = msg.key_error_code.empty();
    int32 terr = static_cast<int32>(msg.permisson_error_code);
    info_log(tlh) << "recv 98 cust login answer msg,fund_units=" << msg.fund_account_array.size()
                  << ",client_seq_id=" << msg.client_seq_id << ",permisson_error_code=" << terr
                  << ",key_error_code=" << msg.key_error_code.c_str() << end_log;

    // 应答资金账户组中每个已缓存会话的账户逐一处理 (多账户并发在途按资金账号自然区分)
    for (const auto &fu : msg.fund_account_array)
    {
      C98SessionInfo *sess = session_cache_.get_session(fu.fund_account_id.data(),
                                                        fu.fund_account_id.size());
      if (sess == nullptr)
      {
        continue; ///< 非本实例发起的登录 (共享单例下其他实例/异常), 忽略
      }

      if (tok)
      {
        // 成功: 回填会话 (cust_id/branch_id/account_id) + log_98 置 OK (用户在途→不直接回调,
        // 最终登录结果由极速柜台回调; delive 失败才经 ans_cust_login 回调失败)
        char tcust[INNER_CUST_ID_LEN];
        char tbranch[INNER_BRANCH_ID_LEN];
        char taccount[INNER_ACCOUNT_ID_LEN];
        from_atp_cstr(tcust, sizeof(tcust), msg.cust_id);
        from_atp_cstr(tbranch, sizeof(tbranch), fu.branch_id);
        taccount[0] = '\0';
        // account_id 从账户组按市场匹配 (无匹配取第一个; market_id 字典 [待确认])
        if (!fu.account_array.empty())
        {
          const auto *pick = &fu.account_array.front();
          for (const auto &tacc : fu.account_array)
          {
            if (static_cast<int16>(tacc.market_id) == market_type)
            {
              pick = &tacc;
              break;
            }
          }
          from_atp_cstr(taccount, sizeof(taccount), pick->account_id);
        }
        session_cache_.fill_session_login_ok(sess, tcust, tbranch, taccount);
        // 必须先保存信息，再设置状态，以此形成屏障; 返回值=用户在途 (98 成功不直接
        // 回调, 登录终态回调由极速柜台应答出)
        int32_t tuser_wait = sess->log_98.ans_login_ok();

        // 投递极速柜台登录 (不管 98 是否登录成功过都投递, 由极速柜台 deal_cust_login 过滤:
        // 用户登录时 98 成功而极速失败, 再次发起登录时仍可发起极速登录)
        acc_login_event_info info;
        build_fast_counter_login_event(sess, info);
        if (delive_fast_counter_login(info) < 0 && tuser_wait == 1)
        {
          // 投递失败且用户在等终态: 在途已被 ans_login_ok 消费, ans_cust_login 的
          // clear_pending 门控不再放行 —— 直接回调失败终态; log_98 保持 OK,
          // 用户重试 login 经幂等直达重新投递
          LoginAns ans_info;
          build_cust_login_rtn(sess, ans_info, LBAPI_ERR_SEND_QUEUE_FULL,
                               "add fast counter cust login event to queue error");
          cb_mgr_->on_login(ans_info);
        }
      }
      else
      {
        // 登录成功过(log_98 val==OK)则忽略迟到失败应答 (旧请求超时后重发、旧应答迟到,
        // 当前会话仍有效) 并仍投递极速柜台登录; 未成功过则经状态机判定回调失败
        if (sess->log_98.ans_login_err() == 1)
        {
          LoginAns ans_info;
          build_cust_login_rtn(sess, ans_info, terr, msg.key_error_msg.c_str());
          cb_mgr_->on_login(ans_info);
        }
        else if (sess->log_98.is_logined())
        {
          info_log(tlh) << "ignore late 98 cust login fail ans,cust have login ok" << end_log;
          acc_login_event_info info;
          build_fast_counter_login_event(sess, info);
          if (delive_fast_counter_login(info) < 0)
          {
            // 已成功账户的补投递失败: 无用户在途 (早经终态回调), 仅记日志
            error_log(tlh) << "98 late fail ans delive fast counter login error,client_seq_id="
                           << msg.client_seq_id << end_log;
          }
        }
      }
    }

    // 应答资金账户组未匹配到任何已缓存会话且为失败应答: 无在途匹配, 直接丢弃
    // [B6·待联调确认, SYSTEM_CONTRACT §7] 成功形态(key_error_code 空)但 fund_account_array
    // 为空/不含所请求账户时, 该会话 USER pending 永挂且后续 login 全部"在途合并"静默无回调;
    // 联调确认柜台是否出现该形态, 确认后在此补兜底 (未被覆盖的 USER pending → 失败回调+清位)
  }

  // build_fast_counter_login_event: 从会话 + ATP 应答构造极速柜台登录事件
  // (其余字段取会话 —— 含 ATP 应答不回传的 password/user_info/feature; 会话为空返回 false)
  void counter98::build_fast_counter_login_event(const C98SessionInfo *sess, acc_login_event_info &o_info)
  {
    std::memset(&o_info, 0, sizeof(o_info));
    o_info.cust_req_no = sess->last_cust_req_no;
    from_atp_cstr(o_info.cust_id, sizeof(o_info.cust_id), sess->cust_id);
    from_atp_cstr(o_info.fund_account_id, sizeof(o_info.fund_account_id), sess->fund_account_id);
    from_atp_cstr(o_info.account_id, sizeof(o_info.account_id), sess->account_id);
    from_atp_cstr(o_info.branch_id, sizeof(o_info.branch_id), sess->branch_id);
    o_info.order_way_ext[0] = sess->order_way_ext[0];
    o_info.order_way_ext[1] = sess->order_way_ext[1];
    std::memcpy(o_info.session, agw_session, sizeof(o_info.session));
    lb_common::comm_utils::str_copy_format(o_info.password, sess->password.data(), sizeof(o_info.password));
    from_atp_cstr(o_info.user_info, sizeof(o_info.user_info), sess->user_info);
    from_atp_cstr(o_info.client_feature_code, sizeof(o_info.client_feature_code), sess->client_feature_code);
    o_info.user_origin = 1;
    o_info.filled = 0;
    return;
  }

  // build_cust_login_rtn: 从会话 + 错误码构造 API 层 LoginAns (失败回调用)
  void counter98::build_cust_login_rtn(const C98SessionInfo *sess, LoginAns &o_ans, int32 errcode, const char *errmsg)
  {
    std::memset(&o_ans, 0, sizeof(o_ans));
    o_ans.client_req_no = sess->last_cust_req_no;
    from_atp_str(o_ans.cust_id, sess->cust_id);
    from_atp_str(o_ans.fund_account_id, sess->fund_account_id);
    from_atp_str(o_ans.branch_id, sess->branch_id);
    from_atp_str(o_ans.account_id, sess->account_id);
    o_ans.market_type = market_type;
    o_ans.err_code = errcode;
    // errmsg 为 C 串 (std::string::c_str() 源): str_copy_format 按 NUL 截断复制, 安全
    lb_common::comm_utils::str_copy_format(o_ans.err_msg.data(), errmsg, sizeof(o_ans.err_msg));
    o_ans.login_time = 0; ///< source: 失败回调不填登录时间
  }

  // build_heart_msg: 98 心跳 (AgwMsgHeartBeat, 消息号 0; 对齐 source: test_req_id 走构造
  // 默认全空格 —— 协议"全空表示心跳消息", 全 0x00 会被误判为测试请求)
  int32 counter98::build_heart_msg(char *o_buf, int32 buf_len)
  {
    lb_counter98::AgwMsgHeartBeat msg;
    return encode_pkt_to(msg, o_buf, buf_len);
  }

  // ---- deal_recv_msg: AgwHeader 切帧 + msg_type 分发 ----

  // deal_recv_msg: 收到 98 消息处理
  int32 counter98::deal_recv_msg(const char *buf, int32 len, int16 link_type)
  {
    if (buf == nullptr || len < (int32)lb_counter98::AGW_HEADER_SIZE)
    {
      return 0;
    }

    int32 deal_len = 0;

    while (len - deal_len >= (int32)lb_counter98::AGW_HEADER_SIZE)
    {
      // 解 AgwHeader (16B 线格式 POD 直读, 对齐 source; 字段: msg_type/msg_ver/reserver/seq_num/body_length)
      raw_agw_header hdr;
      std::memcpy(&hdr, buf + deal_len, sizeof(hdr));
      // body_length 为 uint32: 上界校验防脏数据 (对齐既有防御语义)
      if (unlikely(hdr.body_length > C98_RECV_BODY_MAX))
      {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "98 counter recv msg: msg len error, msg_type=" << hdr.msg_type
                       << ", body_length=" << hdr.body_length << ", link_type=" << link_type << end_log;
        // 返回错误，让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
      const int32 total = static_cast<int32>(lb_counter98::AGW_HEADER_SIZE) +
                          static_cast<int32>(hdr.body_length) +
                          4; ///< +4B CheckSum 尾 (source: msg_total 含 CheckSum, 接收侧暂不校验)
      if (unlikely(total > len - deal_len))
      {
        return deal_len; ///< 半包
      }

      const char *body = buf + deal_len + lb_counter98::AGW_HEADER_SIZE;
      const size_t body_len = hdr.body_length;

      // 心跳应答: 分发前单独处理 (不参与 (type,ver) 匹配, RUL-26)
      if (hdr.msg_type == lb_counter98::kAgwMsgHeartBeat)
      {
        gw_eng_op_->deal_heart_msg_ans(link_type);
        deal_len += total;
        continue;
      }

      // 入站分发: (msg_type, msg_ver) 组合键精确匹配 (RUL-26);
      // case 的 ver 与 vendor 各消息类 GetMsgVer() 一致 (会话类 0 / 业务类 2),
      // 服务端应答 ver 不符时按未知消息跳过 (不关链)
      switch (c98_disp_key(hdr.msg_type, hdr.msg_ver))
      {
      case c98_disp_key(lb_counter98::kAgwMsgLogin, 0): // 3 agw 登录应答 [待确认] 同号应答语义
      {
        lb_counter98::AgwMsgLogin msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_agwuser_login_ans(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv agw login ans decode error" << end_log;
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgLogout, 0): // 4 注销 (被动注销: 会话失效; 会话类 ver=0)
      {
        // 强制复位 agw 状态并关链 (对"登陆中/已登录"两种状态均生效):
        // 重连后由 deal_link_connect 依据 have_agw_ok_ 重新发起 agw 登录
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "98 counter recv logout msg, session closed by peer" << end_log;
        lb_common::atomic_store32(&agw_login_err_, LBAPI_ERR_LOGIN_FAIL);
        lb_common::atomic_store16(&agw_login_state, 0);
        if (gw_eng_op_ != nullptr)
        {
          gw_eng_op_->deal_close_link(LINK_TYPE_98, LBAPI_ERR_LOGIN_FAIL);
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgCustLoginResp, 2): // 800006 账户登录应答
      {
        lb_counter98::AgwMsgCustLoginResp msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_cust_login_ans(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv cust login ans decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "98 cust login ans decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgOrderStatusAck, 2): // 290001 委托状态响应(委托确认; 对齐 source 单一路由)
      {
        lb_counter98::AgwMsgOrderStatusAck msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_order_status_ack(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv order status ack decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "98 order status ack decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgCancelOrder, 2): // 190001 撤单应答 (对齐 source: 按 msg_type 路由,
        //                                 解码同 AgwMsgOrderStatusAck —— 撤单也是委托的一种)
        {
          lb_counter98::AgwMsgOrderStatusAck msg;
          if (c98_decode_body(msg, body, body_len))
          {
            deal_cancel_ans(msg);
          }
          else
          {
            lb_common::lb_log_hand tlh(log_);
            error_log(tlh) << "98 counter recv cancel ans decode error" << end_log;
          }
          break;
        }
      case c98_disp_key(lb_counter98::kAgwMsgCashAuctionTradeER, 2): // 210115 现货集中竞价成交回报
      {
        lb_counter98::AgwMsgCashAuctionTradeER msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_trade_er(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv trade er decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "98 trade er decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultOrder, 2): // 310002 委托查询结果
      {
        lb_counter98::AgwMsgExtQueryResultOrder msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_order_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv order query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 order query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultTradeOrder, 2): // 310003 成交查询结果
      {
        lb_counter98::AgwMsgExtQueryResultTradeOrder msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_trade_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv trade query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 trade query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultFund, 2): // 310004 资金查询结果
      {
        lb_counter98::AgwMsgExtQueryResultFund msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_fund_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv fund query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "98 fund query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultSelfShare, 2): // 7400714 自研股份查询结果 (对齐 source)
      {
        lb_counter98::AgwMsgExtQueryResultSelfShare msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_share_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv share query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 share query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgSyncFinish, 0): // 6 回报同步结束应答 (会话类 ver=0)
      {
        lb_counter98::AgwMsgSyncFinish msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_sync_finish_ans(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv sync finish ans decode error" << end_log;
        }
        break;
      }
      // ---- 98 扩展查询结果 (98 柜台扩展查询消息集, RUL-26) ----
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryMaxOrderQtyResult, 2): // 310001 最大可委托数查询结果
      {
        lb_counter98::AgwMsgExtQueryMaxOrderQtyResult msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_max_order_qty_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv max order qty query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 max order qty query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultTotalFundAsset, 2): // 7401735 总资产查询结果
      {
        lb_counter98::AgwMsgExtQueryResultTotalFundAsset msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_total_fund_asset_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv total fund asset query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 total fund asset query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultFundMaxdraw, 2): // 7490701 可取资金查询结果
      {
        lb_counter98::AgwMsgExtQueryResultFundMaxdraw msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_fund_maxdraw_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv fund maxdraw query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 fund maxdraw query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultCustMarginAvl, 2): // 7490724 两融可用保证金查询结果
      {
        lb_counter98::AgwMsgExtQueryResultCustMarginAvl msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_cust_margin_avl_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv cust margin avl query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 cust margin avl query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryResultAssetDebts, 2): // 7490700 资产负债查询结果
      {
        lb_counter98::AgwMsgExtQueryResultAssetDebts msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_asset_debts_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv asset debts query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 asset debts query result decode error");
        }
        break;
      }
      case c98_disp_key(lb_counter98::kAgwMsgExtQueryCreditMaxOrderQtyResult, 2): // 7490710 两融最大可委托数查询结果
      {
        lb_counter98::AgwMsgExtQueryCreditMaxOrderQtyResult msg;
        if (c98_decode_body(msg, body, body_len))
        {
          deal_credit_max_order_qty_query_result(msg);
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 counter recv credit max order qty query result decode error" << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN,
                            "98 credit max order qty query result decode error");
        }
        break;
      }
      default:
      {
        // 未知消息(含 (type,ver) 不匹配), 跳过，不返回错误，以避免链接关闭，
        // 保证旧api对新增消息/服务端 ver 变化的兼容性
        lb_common::lb_log_hand tlh(log_);
        info_log(tlh) << "98 counter recv unknown msg, msg_type=" << hdr.msg_type
                      << ", msg_ver=" << hdr.msg_ver << end_log;
        break;
      }
      }

      deal_len += total;
    }

    return deal_len;
  }

  // ---- 回报处理: OrderStatusAck / TradeER → API 回调 ----

  // deal_order_status_ack: 委托状态响应 → 委托确认(on_order_rtn) 或 撤单确认(on_cancel_rsp)
  // 区分依据: orig_cli_ord_no != 0 为撤单响应 (被撤原单号), 否则为委托确认 [待确认]
  void counter98::deal_order_status_ack(const lb_counter98::AgwMsgOrderStatusAck &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv 98 order status ack, exec_type=" << msg.exec_type << ", client_seq_id=" << msg.client_seq_id
                  << ", client_order_no=" << msg.client_order_no << ", orig_cli_ord_no=" << msg.orig_cli_ord_no
                  << ", ord_status=" << static_cast<uint32>(msg.order_status)
                  << ", reject_reason=" << msg.reject_reason_code << end_log;

    // 分区去重 + 断点推进 (RUL-30): 记录号未超过该分区已消费位置 → 重复回报丢弃
    if (!consume_report_pos(msg))
    {
      info_log(tlh) << "drop duplicated 98 order status ack, partition=" << static_cast<uint32>(msg.partition)
                    << ", index=" << msg.index << ", client_seq_id=" << msg.client_seq_id << end_log;
      return;
    }

    StreamInfo si_out;
    si_out.counter_type = static_cast<int32_t>(counter_type::fixed_98); ///< 98 柜台回报流 (RUL-32)
    si_out.partno = msg.partition;                                      ///< 回报分区号
    si_out.stream_seq = msg.index;                                      ///< 分区内回报记录号 (RUL-33)

    // 委托确认 (对齐 source deal_order_ack: err_code 由 key_error_code 派生, cancel_qty 不钳制)
    alignas(32) OrderRtn out;
    std::memset(&out, 0, sizeof(out));
    from_atp_str(out.cust_id, msg.cust_id);
    from_atp_str(out.fund_account_id, msg.fund_account_id);
    from_atp_str(out.account_id, msg.account_id);
    from_atp_str(out.branch_id, msg.branch_id);
    out.side = msg.side;
    out.order_type = msg.ord_type;
    out.order_status = msg.order_status;
    out.policy_id = 0;
    out.market_type = static_cast<int16_t>(msg.market_id);
    from_atp_str(out.security_id, msg.security_id);
    out.order_price = msg.price;
    out.order_qty = msg.order_qty;
    out.client_seq_id = msg.client_seq_id;
    out.order_sys_no = msg.client_order_no; ///< 协议: 委托返回的 client_order_no = 柜台委托编号 (撤单 orig_cli_ord_no 用)
    out.frozen_amount = msg.frozen_trade_value;
    out.fee = msg.frozen_fee;
    out.trade_qty = msg.cum_qty;
    out.cancel_qty = msg.order_qty - msg.leaves_qty - msg.cum_qty; ///< source: 不钳制
    out.order_time = msg.transact_time;
    out.update_time = msg.transact_time;
    out.rtn_type = msg.exec_type;                                            ///< 字典基准: ATP ExecType 透传 (RSP_TYPE_*, char)
    out.err_code = msg.key_error_code.empty() ? 0 : LBAPI_ERR_INVALID_PARAM; ///< source 判定

    cb_mgr_->on_order_rtn(si_out, out);
  }

  // deal_cancel_ans: 撤单应答 (msg_type=190001, 解码 AgwMsgOrderStatusAck; 对齐 source)
  // [待确认] 是否包括撤单确认，撤单成交和撤单废单？？？
  void counter98::deal_cancel_ans(const lb_counter98::AgwMsgOrderStatusAck &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv 98 cancel ans, client_seq_id=" << msg.client_seq_id
                  << ", orig_cli_ord_no=" << msg.orig_cli_ord_no
                  << ", key_error_code=" << msg.key_error_code.c_str() << end_log;

    // 分区去重 + 断点推进 (RUL-30, 撤单应答通道; 解码同 AgwMsgOrderStatusAck)
    if (!consume_report_pos(msg))
    {
      info_log(tlh) << "drop duplicated 98 cancel ans, partition=" << static_cast<uint32>(msg.partition)
                    << ", index=" << msg.index << ", client_seq_id=" << msg.client_seq_id << end_log;
      return;
    }

    StreamInfo si_out;
    si_out.counter_type = static_cast<int32_t>(counter_type::fixed_98); ///< 98 柜台回报流 (RUL-32)
    si_out.partno = msg.partition;                                      ///< 回报分区号
    si_out.stream_seq = msg.index;                                      ///< 分区内回报记录号 (RUL-33)

    CancelRsp out;
    std::memset(&out, 0, sizeof(out));
    from_atp_str(out.cust_id, msg.cust_id);
    from_atp_str(out.fund_account_id, msg.fund_account_id);
    from_atp_str(out.account_id, msg.account_id);
    from_atp_str(out.branch_id, msg.branch_id);
    out.market_type = static_cast<int16_t>(msg.market_id);
    out.client_req_no = msg.client_seq_id;  ///< 撤单请求号回显 (组包时写入 client_seq_id)
    out.client_seq_id = msg.client_seq_id;  ///< source: 撤单应答客户号
    out.order_sys_no = msg.orig_cli_ord_no; ///< 被撤原单柜台委托号
    out.err_code = msg.key_error_code.empty() ? 0 : LBAPI_ERR_INVALID_PARAM;
    out.rej_api = 0;
    cb_mgr_->on_cancel_rsp(si_out, out);
  }

  // deal_trade_er: 现货集中竞价成交回报 → on_trade_rtn
  void counter98::deal_trade_er(const lb_counter98::AgwMsgCashAuctionTradeER &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    char texec[33];
    from_atp_cstr(texec, sizeof(texec), msg.exec_id);
    info_log(tlh) << "recv 98 trade er, client_seq_id=" << msg.client_seq_id
                  << ", client_order_no=" << msg.client_order_no << ", exec_id=" << texec
                  << ", last_px=" << msg.last_px << ", last_qty=" << msg.last_qty << end_log;

    // 分区去重 + 断点推进 (RUL-30, 成交回报通道)
    if (!consume_report_pos(msg))
    {
      info_log(tlh) << "drop duplicated 98 trade er, partition=" << static_cast<uint32>(msg.partition)
                    << ", index=" << msg.index << ", client_seq_id=" << msg.client_seq_id << end_log;
      return;
    }

    StreamInfo si_out;
    si_out.counter_type = static_cast<int32_t>(counter_type::fixed_98); ///< 98 柜台回报流 (RUL-32)
    si_out.partno = msg.partition;                                      ///< 回报分区号
    si_out.stream_seq = msg.index;                                      ///< 分区内回报记录号 (RUL-33)

    // RUL-23: 成交回报通道按 exec_type 分流 —— 仅 'F' 已成交进 on_trade_rtn;
    // 其余事件类型(委托确认'0'/撤单成交'4'/委托废单'8'/内部'1'等)转 OrderRtn 走 on_order_rtn
    if (msg.exec_type != RSP_TYPE_TRADE)
    {
      alignas(32) OrderRtn oord;
      std::memset(&oord, 0, sizeof(oord));
      from_atp_str(oord.cust_id, msg.cust_id);
      from_atp_str(oord.fund_account_id, msg.fund_account_id);
      from_atp_str(oord.account_id, msg.account_id);
      from_atp_str(oord.branch_id, msg.branch_id);
      oord.side = msg.side;
      oord.order_type = 0; ///< TradeER 链无订单类型字段 (同下方成交回报口径)
      oord.order_status = msg.order_status;
      oord.market_type = static_cast<int16_t>(msg.market_id);
      from_atp_str(oord.security_id, msg.security_id);
      oord.order_price = msg.price;
      oord.order_qty = msg.order_qty;
      oord.client_seq_id = msg.client_seq_id;
      oord.rtn_type = msg.exec_type;                                            ///< ATP ExecType 透传 (RSP_TYPE_*)
      oord.err_code = msg.key_error_code.empty() ? 0 : LBAPI_ERR_INVALID_PARAM; ///< 同 deal_order_status_ack 判定
      oord.order_sys_no = msg.client_order_no;                                  ///< 协议: client_order_no = 柜台委托编号
      oord.frozen_amount = 0;
      oord.fee = msg.fee;
      oord.trade_qty = msg.cum_qty;
      oord.cancel_qty = msg.order_qty - msg.leaves_qty - msg.cum_qty; ///< source: 不钳制
      oord.order_time = msg.transact_time;
      oord.update_time = msg.transact_time;
      cb_mgr_->on_order_rtn(si_out, oord);
      return;
    }

    alignas(32) TradeRtn out;
    std::memset(&out, 0, sizeof(out));
    from_atp_str(out.cust_id, msg.cust_id);
    from_atp_str(out.fund_account_id, msg.fund_account_id);
    from_atp_str(out.account_id, msg.account_id);
    from_atp_str(out.branch_id, msg.branch_id);
    out.side = msg.side;
    out.order_type = 0; ///< OrderReport 无订单类型字段
    out.order_status = msg.order_status;
    out.rtn_type = RSP_TYPE_TRADE;
    out.policy_id = 0;
    out.market_type = static_cast<int16_t>(msg.market_id);
    from_atp_str(out.security_id, msg.security_id);
    out.order_price = msg.price;
    out.order_qty = msg.order_qty;
    out.client_seq_id = msg.client_seq_id;
    out.order_sys_no = msg.client_order_no; ///< 协议: 成交回报的 client_order_no = 柜台委托编号
    out.frozen_amount = 0;
    out.fee = msg.fee; ///< source: 成交回报 fee = 单笔费用
    out.trade_qty = msg.cum_qty;
    out.cancel_qty = msg.order_qty - msg.leaves_qty - msg.cum_qty; ///< source: 不钳制
    out.order_time = msg.transact_time;
    out.exec_time = msg.transact_time;
    from_atp_str(out.exec_id, msg.exec_id);
    out.exec_price = msg.last_px;
    out.exec_qty = msg.last_qty;
    out.exec_amount = msg.total_value_traded; ///< source: 成交金额专字段
    out.exec_fee = 0;                         ///< source: 单笔费用置 0

    cb_mgr_->on_trade_rtn(si_out, out);
  }

  // ---- 查询应答处理 (对齐 source: vector 按实际条数一次回调) ----

  // deal_order_query_result: 委托查询结果 → on_order_query_ans(OrderRtn*, ctl)
  void counter98::deal_order_query_result(const lb_counter98::AgwMsgExtQueryResultOrder &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 order query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 order query result, count=" << msg.order_array.size()
                  << ", total_num=" << msg.total_num << ", last_index=" << msg.last_index << end_log;

    const int32 count = static_cast<int32>(msg.order_array.size());
    if (count <= 0)
    {
      QueryAnsCtl ctl = {};
      ctl.count = 0;
      ctl.is_last = 1;
      ctl.client_req_no = msg.client_seq_id;
      OrderRtn dummy = {};
      cb_mgr_->on_order_query_ans(&dummy, ctl);
      return;
    }

    std::vector<OrderRtn> rtn_arr(count);
    for (int32 i = 0; i < count; ++i)
    {
      const auto &e = msg.order_array[i];
      OrderRtn &out = rtn_arr[i];
      std::memset(&out, 0, sizeof(out));
      // 按 source: 不填 cust_id/fund_account_id/rtn_type/err_code (保持零值)
      from_atp_str(out.account_id, e.account_id);
      from_atp_str(out.branch_id, msg.branch_id);
      out.side = e.side;
      out.order_type = e.ord_type;
      out.order_status = e.ord_status;
      out.market_type = static_cast<int16_t>(e.market_id);
      from_atp_str(out.security_id, e.security_id);
      out.order_price = e.order_price;
      out.order_qty = e.order_qty;
      out.client_seq_id = e.client_seq_id;
      out.order_sys_no = 0; ///< source: 查询结果不回柜台委托号
      out.frozen_amount = e.frozen_trade_value;
      out.fee = e.frozen_fee;
      out.trade_qty = e.cum_qty;
      out.cancel_qty = e.cancel_qty;
      out.order_time = e.transact_time;
      out.update_time = e.transact_time;
    }
    QueryAnsCtl ctl = {};
    ctl.count = count;
    ctl.is_last = (msg.last_index + 1 >= msg.total_num) ? 1 : 0;
    ctl.client_req_no = msg.client_seq_id;
    cb_mgr_->on_order_query_ans(rtn_arr.data(), ctl);
  }

  // deal_trade_query_result: 成交查询结果 → on_trade_query_ans(TradeInfo*, ctl)
  void counter98::deal_trade_query_result(const lb_counter98::AgwMsgExtQueryResultTradeOrder &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 trade query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 trade query result, count=" << msg.order_array.size()
                  << ", total_num=" << msg.total_num << ", last_index=" << msg.last_index << end_log;

    const int32 count = static_cast<int32>(msg.order_array.size());
    if (count <= 0)
    {
      QueryAnsCtl ctl = {};
      ctl.count = 0;
      ctl.is_last = 1;
      ctl.client_req_no = msg.client_seq_id;
      TradeInfo dummy = {};
      cb_mgr_->on_trade_query_ans(&dummy, ctl);
      return;
    }

    std::vector<TradeInfo> rtn_arr(count);
    for (int32 i = 0; i < count; ++i)
    {
      const auto &e = msg.order_array[i];
      TradeInfo &out = rtn_arr[i];
      std::memset(&out, 0, sizeof(out));
      // 按 source: 不填 cust_id/fund_account_id (保持零值)
      from_atp_str(out.branch_id, msg.branch_id);
      out.side = e.side;
      out.order_type = e.ord_type;
      out.market_type = static_cast<int16_t>(e.market_id);
      from_atp_str(out.security_id, e.security_id);
      out.client_seq_id = e.cl_ord_no; ///< source: 成交查询客户号 = 客户订单编号
      out.order_sys_no = 0;            ///< source: 不回柜台委托号
      out.exec_time = e.transact_time;
      from_atp_cstr(out.exec_id, sizeof(out.exec_id), e.exec_id); ///< TradeInfo.exec_id 为裸 char 数组
      out.exec_price = e.last_px;
      out.exec_qty = e.last_qty;
      out.exec_amount = e.total_value_traded;
      out.exec_fee = e.fee;
    }
    QueryAnsCtl ctl = {};
    ctl.count = count;
    ctl.is_last = (msg.last_index + 1 >= msg.total_num) ? 1 : 0;
    ctl.client_req_no = msg.client_seq_id;
    cb_mgr_->on_trade_query_ans(rtn_arr.data(), ctl);
  }

  // deal_fund_query_result: 资金查询结果 → on_fund_query_ans (单条)
  void counter98::deal_fund_query_result(const lb_counter98::AgwMsgExtQueryResultFund &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 fund query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 fund query result, avail_t0=" << msg.available_t0
                  << ", avail_tall=" << msg.available_tall << end_log;

    CustFundInfo info;
    std::memset(&info, 0, sizeof(info));
    info.client_req_no = msg.client_seq_id;
    from_atp_str(info.cust_id, msg.cust_id);
    from_atp_str(info.fund_account_id, msg.fund_account_id);
    from_atp_str(info.branch_id, msg.branch_id);
    info.market_type = market_type;
    info.avail_amount = msg.available_t0; ///< source: 可用资金 = T+0 可用
    info.token_amount = msg.leaves_value; ///< source: 可取资金 = 资金余额(leaves_value)
    cb_mgr_->on_fund_query_ans(info);
  }

  // deal_share_query_result: 持仓查询结果 → on_position_query_ans(CustPositionInfo*, ctl)
  void counter98::deal_share_query_result(const lb_counter98::AgwMsgExtQueryResultSelfShare &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 share query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 share query result, count=" << msg.order_array.size()
                  << ", total_num=" << msg.total_num << ", last_index=" << msg.last_index << end_log;

    const int32 count = static_cast<int32>(msg.order_array.size());
    if (count <= 0)
    {
      QueryAnsCtl ctl = {};
      ctl.count = 0;
      ctl.is_last = 1;
      ctl.client_req_no = msg.client_seq_id;
      CustPositionInfo dummy = {};
      cb_mgr_->on_position_query_ans(&dummy, ctl);
      return;
    }

    std::vector<CustPositionInfo> info_arr(count);
    for (int32 i = 0; i < count; ++i)
    {
      const auto &e = msg.order_array[i]; // AgwSelfShareArray (自研股份查询富字段, 对齐 source)
      CustPositionInfo &out = info_arr[i];
      std::memset(&out, 0, sizeof(out));
      out.client_req_no = msg.client_seq_id;
      from_atp_str(out.cust_id, msg.cust_id);
      from_atp_str(out.fund_account_id, msg.fund_account_id);
      from_atp_str(out.account_id, e.account_id);
      from_atp_str(out.branch_id, msg.branch_id);
      out.market_type = static_cast<int16_t>(e.market_id);
      from_atp_str(out.security_id, e.security_id);
      from_atp_str(out.security_symbol, e.security_symbol);
      out.security_type = e.security_type;
      from_atp_str(out.currency, e.currency);
      from_atp_str(out.pbu_id, e.pbu_id);
      out.stk_bal = e.stk_bal;
      out.stk_avl = e.stk_avl;
      out.stk_can_trans = e.stk_can_trans;
      out.stk_frz = e.stk_frz;
      out.leaves_qty = e.leaves_qty;
      out.market_value = e.market_value;
      out.cost_price = e.cost_price;
      out.profit_cost = e.profit_cost;
      out.pro_income = e.pro_income;
      out.income = e.income;
      out.profit_rate = e.profit_rate;
      out.limit_up = e.limit_up;
      out.limit_down = e.limit_down;
    }
    QueryAnsCtl ctl = {};
    ctl.count = count;
    ctl.is_last = (msg.last_index + 1 >= msg.total_num) ? 1 : 0;
    ctl.client_req_no = msg.client_seq_id;
    cb_mgr_->on_position_query_ans(info_arr.data(), ctl);
  }

  // ---- 98 扩展查询结果处理 (字段映射对齐 source, RUL-26;
  //      数组型一次回调, 单条型直接回调, 均无分页) ----

  // deal_max_order_qty_query_result: 最大可委托数查询结果 (310001) → on_max_order_qty_query_ans (单条)
  void counter98::deal_max_order_qty_query_result(const lb_counter98::AgwMsgExtQueryMaxOrderQtyResult &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 max order qty query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 max order qty query result, max_order_qty=" << msg.max_order_qty
                  << ", theoretical_order_qty=" << msg.theoretical_order_qty << end_log;

    CustMaxOrderQtyInfo info;
    std::memset(&info, 0, sizeof(info));
    info.client_req_no = msg.client_seq_id;
    from_atp_str(info.cust_id, msg.cust_id);
    from_atp_str(info.fund_account_id, msg.fund_account_id);
    from_atp_str(info.branch_id, msg.branch_id);
    from_atp_str(info.account_id, msg.account_id);
    info.max_order_qty = msg.max_order_qty;
    info.theoretical_order_qty = msg.theoretical_order_qty;
    info.odd_order_qty = msg.odd_order_qty;
    info.limit_flag = msg.limit_flag;
    info.jj_max_limit_qty = msg.jj_max_limit_qty;
    cb_mgr_->on_max_order_qty_query_ans(info);
  }

  // deal_total_fund_asset_query_result: 总资产查询结果 (7401735) → on_total_fund_asset_query_ans
  void counter98::deal_total_fund_asset_query_result(const lb_counter98::AgwMsgExtQueryResultTotalFundAsset &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 total fund asset query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 total fund asset query result, count=" << msg.order_array.size() << end_log;

    const int32 count = static_cast<int32>(msg.order_array.size());
    if (count <= 0)
    {
      QueryAnsCtl ctl = {};
      ctl.count = 0;
      ctl.is_last = 1;
      ctl.client_req_no = msg.client_seq_id;
      CustTotalFundAssetInfo dummy = {};
      cb_mgr_->on_total_fund_asset_query_ans(&dummy, ctl);
      return;
    }

    std::vector<CustTotalFundAssetInfo> info_arr(count);
    for (int32 i = 0; i < count; ++i)
    {
      const auto &e = msg.order_array[i];
      CustTotalFundAssetInfo &info = info_arr[i];
      std::memset(&info, 0, sizeof(info));
      info.client_req_no = msg.client_seq_id;
      from_atp_str(info.cust_id, msg.cust_id);
      from_atp_str(info.fund_account_id, msg.fund_account_id);
      from_atp_str(info.branch_id, msg.branch_id);
      from_atp_str(info.currency, e.currency);
      info.fund_bal = e.fund_bal;
      info.fund_avl = e.fund_avl;
      info.fund_asset_total_new = e.fund_asset_total_new;
      info.fund_asset_total = e.fund_asset_total;
      info.fund_asset = e.fund_asset;
      info.fund_asset_ggt = e.fund_asset_ggt;
      info.fund_asset_stktotal = e.fund_asset_stktotal;
      info.fund_asset_stk = e.fund_asset_stk;
      info.fund_asset_bond_total = e.fund_asset_bond_total;
      info.fund_asset_bond = e.fund_asset_bond;
      info.fund_asset_rzgh = e.fund_asset_rzgh;
      info.fund_asset_rqgh = e.fund_asset_rqgh;
      info.fund_asset_stkzy = e.fund_asset_stkzy;
      info.fund_asset_rzcs = e.fund_asset_rzcs;
      info.fund_asset_rqcs = e.fund_asset_rqcs;
      info.fund_asset_stkxyzy = e.fund_asset_stkxyzy;
      info.fund_asset_adjust = e.fund_asset_adjust;
      info.fund_asset_oftotal = e.fund_asset_oftotal;
      info.fund_asset_of = e.fund_asset_of;
      info.fund_asset_bjhgsh = e.fund_asset_bjhgsh;
      info.fund_asset_bjhgsz = e.fund_asset_bjhgsz;
      info.fund_asset_ofuncome = e.fund_asset_ofuncome;
      info.fund_asset_xjgj = e.fund_asset_xjgj;
      info.fund_asset_zfzz = e.fund_asset_zfzz;
      info.fund_asset_jdlsec = e.fund_asset_jdlsec;
      info.fund_asset_fjjj = e.fund_asset_fjjj;
      info.fund_asset_curdayrqgh = e.fund_asset_curdayrqgh;
      info.fund_asset_curdayrzgh = e.fund_asset_curdayrzgh;
      info.fund_asset_otc_end = e.fund_asset_otc_end;
      info.debts_total = e.debts_total;
      info.zy_mtkvalue = e.zy_mtkvalue;
      info.zy_debts = e.zy_debts;
      info.en_mktvalue = e.en_mktvalue;
      info.en_debts = e.en_debts;
      info.zywy_mktvalue = e.zywy_mktvalue;
      info.fund_asset_matchrqgh = e.fund_asset_matchrqgh;
      info.sum_profit_cost = e.sum_profit_cost;
      info.sump_rofit_amt = e.sump_rofit_amt;
      info.fund_asset_stkwxxj = e.fund_asset_stkwxxj;
      info.fund_asset_adjust_xsf = e.fund_asset_adjust_xsf;
      info.fund_asset_adjust_qt = e.fund_asset_adjust_qt;
      info.fund_asset_adjust_hgt = e.fund_asset_adjust_hgt;
      info.fund_asset_adjust_lof = e.fund_asset_adjust_lof;
      info.fund_asset_stk_only = e.fund_asset_stk_only;
      info.fund_asset_adjust_etf = e.fund_asset_adjust_etf;
      info.trade_date = e.trade_date;
    }
    QueryAnsCtl ctl = {};
    ctl.count = count;
    ctl.is_last = 1; ///< 总资产查询无分页, 一次返回全部
    ctl.client_req_no = msg.client_seq_id;
    cb_mgr_->on_total_fund_asset_query_ans(info_arr.data(), ctl);
  }

  // deal_fund_maxdraw_query_result: 可取资金查询结果 (7490701) → on_fund_maxdraw_query_ans
  void counter98::deal_fund_maxdraw_query_result(const lb_counter98::AgwMsgExtQueryResultFundMaxdraw &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 fund maxdraw query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 fund maxdraw query result, count=" << msg.order_array.size() << end_log;

    const int32 count = static_cast<int32>(msg.order_array.size());
    if (count <= 0)
    {
      QueryAnsCtl ctl = {};
      ctl.count = 0;
      ctl.is_last = 1;
      ctl.client_req_no = msg.client_seq_id;
      CustFundMaxdrawInfo dummy = {};
      cb_mgr_->on_fund_maxdraw_query_ans(&dummy, ctl);
      return;
    }

    std::vector<CustFundMaxdrawInfo> info_arr(count);
    for (int32 i = 0; i < count; ++i)
    {
      const auto &e = msg.order_array[i]; // AgwFundMaxdrawArray (身份字段取消息基类, 对齐 source)
      CustFundMaxdrawInfo &info = info_arr[i];
      std::memset(&info, 0, sizeof(info));
      info.client_req_no = msg.client_seq_id;
      from_atp_str(info.cust_id, msg.cust_id);
      from_atp_str(info.fund_account_id, msg.fund_account_id);
      from_atp_str(info.branch_id, msg.branch_id);
      from_atp_str(info.currency, e.currency);
      info.fund_bal = e.fund_bal;
      info.fund_avl = e.fund_avl;
      info.fund_buy = e.fund_buy;
      info.fund_sale = e.fund_sale;
      info.market_value = e.market_value;
      info.fund_all = e.fund_all;
      info.fund_seq = e.fund_seq;
      info.stk_value = e.stk_value;
      info.max_draw = e.max_draw;
      from_atp_str(info.bank_code, e.bank_code);
      info.fund_bjhg_avl = e.fund_bjhg_avl;
      info.credit_flag = e.credit_flag;
      info.stk_value_non_of_cash = e.stk_value_non_of_cash;
      info.of_cash = e.of_cash;
    }
    QueryAnsCtl ctl = {};
    ctl.count = count;
    ctl.is_last = 1; ///< 可取资金查询无分页, 一次返回全部
    ctl.client_req_no = msg.client_seq_id;
    cb_mgr_->on_fund_maxdraw_query_ans(info_arr.data(), ctl);
  }

  // deal_cust_margin_avl_query_result: 两融可用保证金查询结果 (7490724) → on_cust_margin_avl_query_ans (单条)
  void counter98::deal_cust_margin_avl_query_result(const lb_counter98::AgwMsgExtQueryResultCustMarginAvl &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 cust margin avl query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 cust margin avl query result, margin_avl=" << msg.margin_avl << end_log;

    CustMarginAvlInfo info;
    std::memset(&info, 0, sizeof(info));
    info.client_req_no = msg.client_seq_id;
    from_atp_str(info.cust_id, msg.cust_id);
    from_atp_str(info.fund_account_id, msg.fund_account_id);
    from_atp_str(info.branch_id, msg.branch_id);
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
    info.market_type = static_cast<int16_t>(msg.market_id);
    from_atp_str(info.security_id, msg.security_id);
    from_atp_str(info.security_symbol, msg.security_symbol);
    info.margin_fund_bal = msg.margin_fund_bal;
    info.margin_credit_bal = msg.margin_credit_bal;
    info.fund_margin_avl = msg.fund_margin_avl;
    info.fund_margin_avl_sub = msg.fund_margin_avl_sub;
    cb_mgr_->on_cust_margin_avl_query_ans(info);
  }

  // deal_asset_debts_query_result: 资产负债查询结果 (7490700) → on_asset_debts_query_ans (单条)
  void counter98::deal_asset_debts_query_result(const lb_counter98::AgwMsgExtQueryResultAssetDebts &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 asset debts query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 asset debts query result, asset=" << msg.asset
                  << ", liability=" << msg.liability << end_log;

    AssetDebtsInfo info;
    std::memset(&info, 0, sizeof(info));
    info.client_req_no = msg.client_seq_id;
    from_atp_str(info.cust_id, msg.cust_id);
    from_atp_str(info.fund_account_id, msg.fund_account_id);
    from_atp_str(info.branch_id, msg.branch_id);
    from_atp_str(info.currency, msg.currency);
    from_atp_str(info.cust_name, msg.cust_name);
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

  // deal_credit_max_order_qty_query_result: 两融最大可委托数查询结果 (7490710) → on_credit_max_order_qty_query_ans (单条)
  void counter98::deal_credit_max_order_qty_query_result(
      const lb_counter98::AgwMsgExtQueryCreditMaxOrderQtyResult &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    if (msg.query_result_code != 0)
    {
      warning_log(tlh) << "98 credit max order qty query result error, code=" << msg.query_result_code
                       << ", err=" << msg.key_error_msg.c_str() << end_log;
    }
    info_log(tlh) << "recv 98 credit max order qty query result, max_order_qty=" << msg.max_order_qty << end_log;

    CreditMaxOrderQtyInfo info;
    std::memset(&info, 0, sizeof(info));
    info.client_req_no = msg.client_seq_id;
    from_atp_str(info.cust_id, msg.cust_id);
    from_atp_str(info.fund_account_id, msg.fund_account_id);
    from_atp_str(info.branch_id, msg.branch_id);
    from_atp_str(info.account_id, msg.account_id);
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
    // zero_reason: 协议 String_def(std::string) → 定长数组规范化拷贝 (规则 R, 截断安全)
    lb_common::comm_utils::str_copy_format(info.zero_reason.data(), msg.zero_reason.c_str(),
                                           sizeof(info.zero_reason));
    info.limit_flag = msg.limit_flag;
    info.jj_max_limit_qty = msg.jj_max_limit_qty;
    cb_mgr_->on_credit_max_order_qty_query_ans(info);
  }

  // ---- deal_send_error: 发送失败 → API 层拒绝回报 (字段对齐 source) ----

  void counter98::build_api_order_rej(const lb_counter98::AgwMsgCashAuctionOrder &msg, int32 err_code, OrderRtn &o_rtn,
                                      StreamInfo &o_stream)
  {
    std::memset(&o_rtn, 0, sizeof(o_rtn));
    from_atp_str(o_rtn.cust_id, msg.cust_id);
    from_atp_str(o_rtn.fund_account_id, msg.fund_account_id);
    from_atp_str(o_rtn.account_id, msg.account_id);
    from_atp_str(o_rtn.branch_id, msg.branch_id);
    from_atp_str(o_rtn.security_id, msg.security_id);
    o_rtn.side = msg.side;
    o_rtn.order_type = msg.order_type;
    o_rtn.order_price = msg.price;
    o_rtn.order_qty = msg.order_qty;
    o_rtn.client_seq_id = msg.client_seq_id;
    o_rtn.market_type = static_cast<int16_t>(msg.market_id);
    o_rtn.order_status = ORDER_STATE_REJECTED; ///< API 层拒绝 → 已拒绝 (新字典 0=已申报, 不可留零值)
    o_rtn.rtn_type = RSP_TYPE_REJECTED;        ///< API 层拒绝 ('8')
    o_rtn.err_code = err_code;

    o_stream.counter_type = static_cast<int32_t>(counter_type::fixed_98); ///< API 层拒绝 = 柜台回报流 (RUL-32)
    o_stream.partno = 0;                                                  ///< 无回报消息, 无分区
    o_stream.stream_seq = 0;
  }

  void counter98::build_api_cancel_rej(const lb_counter98::AgwMsgCancelOrder &msg, int32 err_code, CancelRsp &o_rtn,
                                       StreamInfo &o_stream)
  {
    std::memset(&o_rtn, 0, sizeof(o_rtn));
    from_atp_str(o_rtn.cust_id, msg.cust_id);
    from_atp_str(o_rtn.fund_account_id, msg.fund_account_id);
    from_atp_str(o_rtn.account_id, msg.account_id);
    from_atp_str(o_rtn.branch_id, msg.branch_id);
    o_rtn.client_seq_id = msg.client_seq_id;  ///< source: 撤单请求号
    o_rtn.order_sys_no = msg.orig_cli_ord_no; ///< source: 被撤原单号
    o_rtn.err_code = err_code;
    o_rtn.rej_api = 1; ///< API 层拒绝 (队列满/发送失败)

    o_stream.counter_type = static_cast<int32_t>(counter_type::fixed_98); ///< API 层拒绝 = 柜台回报流 (RUL-32)
    o_stream.partno = 0;                                                  ///< 无回报消息, 无分区
    o_stream.stream_seq = 0;
  }

  void counter98::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret)
  {
    (void)link_type;
    if (msg_buf == nullptr || cb_mgr_ == nullptr)
    {
      return;
    }
    if (msg_len < (int32)lb_counter98::AGW_HEADER_SIZE)
    {
      return;
    }
    raw_agw_header hdr;
    std::memcpy(&hdr, msg_buf, sizeof(hdr));
    const char *body = msg_buf + lb_counter98::AGW_HEADER_SIZE;
    const size_t body_len = (size_t)msg_len - lb_counter98::AGW_HEADER_SIZE;
    // 此函数对应于 event_type = LINK_EVENT_TYPE_SEND_MSG, 发送失败后调用。
    // 对于98 柜台，应该还有很多其他委托和请求的错误处理
    switch (hdr.msg_type)
    {
    case lb_counter98::kAgwMsgCashAuctionOrder: // 100101 委托
    {
      // 委托发送失败: 解回原请求体, 构造 OrderRtn 通知客户
      lb_counter98::AgwMsgCashAuctionOrder m;
      if (c98_decode_body(m, body, body_len))
      {
        OrderRtn o_rtn;
        StreamInfo o_stream;
        build_api_order_rej(m, err_ret, o_rtn, o_stream);
        cb_mgr_->on_order_rtn(o_stream, o_rtn);
      }
      break;
    }
    case lb_counter98::kAgwMsgCancelOrder: // 190001 撤单
    {
      // 撤单发送失败: 解回原请求体, 构造 CancelRsp 通知客户
      lb_counter98::AgwMsgCancelOrder m;
      if (c98_decode_body(m, body, body_len))
      {
        CancelRsp o_rtn;
        StreamInfo o_stream;
        build_api_cancel_rej(m, err_ret, o_rtn, o_stream);
        cb_mgr_->on_cancel_rsp(o_stream, o_rtn);
      }
      break;
    }
    default:
      break;
    }
  }

  // ---- 链接状态通知 ----

  int32 counter98::deal_link_connect(int16 link_type, int32 have_switch)
  {
    if (link_type == LINK_TYPE_98)
    {
      lb_common::atomic_store16(&trade_link_connect_, 1);

      // 断线重连后必须重新发起 agw 用户登陆 (REQ 4.5-1, 触发时机二: 重连 + 成功登陆过):
      // 前提 have_agw_ok_ (此前收到过登录成功应答), 与是否切换地址无关;
      // 首连时 have_agw_ok_==0 (agw 登陆由用户 login() 发起), 此处不触发。
      // 先占位在途状态再投递 (deal_link_close 已将会话失效复位为 0):
      // 占位用 CAS(0→1), 与用户线程 deal_agw_login 的 CAS 抢占严格互斥,
      // 先占者投递, 后到者跳过 (其请求覆盖本次重连刷新), 避免双请求窗口
      if (have_agw_ok_ == 1)
      {
        int16 tagw_state = 0;
        if (!lb_common::atomic_cas16(&agw_login_state, &tagw_state, 1))
        {
          lb_common::lb_log_hand tlh(log_);
          info_log(tlh) << "98 link reconnected, agw login in-flight by other, state=" << tagw_state << end_log;
          if (cb_mgr_ != nullptr)
          {
            cb_mgr_->on_link_status(0, 0, 1);
          }
          return 0;
        }
        int32 ret = delive_agw_login_event();
        if (ret != 0)
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 link reconnected, re-deliver agw user login error, ret=" << ret << end_log;

          return ret; // 队满: delive 内部已复位状态; 返回错误由底层关链, 待下次重连重试
        }
      }

      if (cb_mgr_ != nullptr)
      {
        cb_mgr_->on_link_status(0, 0, 1);
      }

      return 0;
    }
    return LBAPI_ERR_UNSUPPORTED_TYPE;
  }

  void counter98::deal_link_close(int16 link_type)
  {
    if (link_type == LINK_TYPE_98)
    {
      lb_common::atomic_store16(&trade_link_connect_, 0);

      session_cache_.clear_all_pending();

      // 解耦不变量 (§3.2.0): 断链不清除各账户 98 登录状态 (log_98) 与 agwuser 登录状态
      // (已成功客户重连后不重登, §3.2.5; agw 重连后重登, 见下)

      // 链接断开: agw 会话随之失效 (REQ 4.5-1, 重连后由 deal_link_connect 依据 have_agw_ok_ 重新发起)
      //   已登录(2) → 复位 (会话失效, 重连后必须重登)
      //   在途(1) → 复位并设断开错误码 (同步等待方感知失败)
      //   已结束(0) → 不动, 不覆盖既有登录失败错误码 (登录失败关链路径:
      //              error_agwuser_login 已设置真实失败码后才触发本回调)

      while (1)
      {
        int16 t = lb_common::atomic_load16(&agw_login_state);
        if (t == 2)
        {
          if (lb_common::atomic_cas16(&agw_login_state, &t, 0))
            break;
        }
        else if (t == 1)
        {
          // 先写错误码再 CAS 复位(顺序同 error_agwuser_login): 避免同步等待方在
          // 复位与写码之间读到旧值, 使断链原因被泛化为 LOGIN_FAIL
          lb_common::atomic_store32(&agw_login_err_, LBAPI_ERR_LINK_DISCONNECTED);
          int16 t1 = 1;
          if (lb_common::atomic_cas16(&agw_login_state, &t1, 0))
          {
            break;
          }
        }
        else
        {
          break;
        }
      }

      if (cb_mgr_ != nullptr)
      {
        cb_mgr_->on_link_status(0, 0, 0);
      }
    }
  }

} // namespace lb_api

// gw_counter_direct - 个微软件极速柜台 (直连模式) 实现
//
// 个微柜台无查询接口, 仅直连模式 (C1/C2 配置)
// 业务消息: 委托 / 撤单 / ETF申购赎回
// 推送: 委托 / 成交 / 撤单回报 / 拒绝
//
// 协议与架构状态:
//   - 已接入 FTE 真实协议 (gw_head.h), 报文为混合字节序: 8B 头大端 + 体主机序
//     (小端)直通 + 4B 校验和大端, 见 gw_head.h 注释。
//   - 多账户: 每资金账户一条 GwLoginCache 登录条目 (本柜台专属), 条目内 log_gw
//     状态机 (log_state_ctl) 承担状态/重复/在途控制;
//     断线重连对全部已登录账户批量自动重登; 登出本地化(置 LOGOUT, 不关链)。
//   - 组包 user 块 (cust_id/account_id) 经登录条目回填; 撤单定位直接取自请求
//     (order_sys_no=柜台委托号, client_seq_id=原委托用户报单号)。
//     字段映射与【待确认】项: 仓库根 field_mapping_gw.md。

#include "gw_counter_direct.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "gw_head.h"
#include "gw_login_cache.h"
#include "matomic.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

#include <cstring>
#include <string>
#include <vector>

namespace lb_api
{

  namespace
  {
    // ---- FTE 报文帧常量 ----
    constexpr int32 GW_PKT_HEAD_SIZE = 8;           ///< PktNewHeader 定长
    constexpr int32 GW_PKT_TAIL_SIZE = 4;           ///< 校验和定长
    constexpr uint32_t GW_RECV_BODY_MAX = 1u << 20; ///< 单包消息体上限(防御): ETF 成分券回报可变长

    /// C 串(NUL 结尾) → FTE 定长字段(尾部空格填充, 无 NUL)
    template <size_t N>
    void to_fte_str(std::array<char, N> &dst, const char *src)
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

    /// FTE 定长字段(空格填充) → API 数组(NUL 结尾, 裁尾空格)
    template <size_t N, size_t M>
    void from_fte_str(std::array<char, N> &dst, const std::array<char, M> &src)
    {
      size_t n = M;
      while (n > 0 && src[n - 1] == ' ')
      {
        --n;
      }
      if (n > N - 1)
      {
        n = N - 1; ///< 保留 NUL 位
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

    /// 校验和: 对 [头+体] 逐字节求和 % 256
    uint32_t gw_pkt_checksum(const char *data, size_t len)
    {
      uint32_t sum = 0;
      for (size_t i = 0; i < len; ++i)
      {
        sum += static_cast<uint8_t>(data[i]);
      }
      return sum & 0xFFu;
    }

    /// 组完整包: 8B 头(大端) + 体(主机序直通, gw_head.h HostToNetwork) + 4B 校验和(大端)
    /// @return 完整包长; 缓冲不足返回 0
    template <typename BodyT>
    int32 gw_pkt_encode(char *o_buf, int32 buf_len, uint32_t msg_id, const BodyT &body)
    {
      const int32 need = GW_PKT_HEAD_SIZE + static_cast<int32>(sizeof(BodyT)) + GW_PKT_TAIL_SIZE;
      if (o_buf == nullptr || buf_len < need)
      {
        return 0;
      }
      gw_message::PktNewHeader head;
      head.msg_id = msg_id;
      head.msg_len = static_cast<uint32_t>(sizeof(BodyT));
      size_t off = head.encode(o_buf, static_cast<size_t>(buf_len));
      off += body.encode(o_buf + off, static_cast<size_t>(buf_len) - off);
      const uint32_t sum = gw_pkt_checksum(o_buf, off);
      const uint32_t be = gw_message::detail::ByteSwap32(sum);
      std::memcpy(o_buf + off, &be, GW_PKT_TAIL_SIZE);
      return static_cast<int32>(off) + GW_PKT_TAIL_SIZE;
    }

    /// 组包 user 块 (TradeOrderUser 展开, 各请求消息字段名一致):
    /// branch/fund 来自请求, account/cust_id 来自登录条目(登录应答回填), agw_seq_id 恒 0
    template <typename BodyT>
    void fill_req_user(BodyT &body, const char *fund_cstr, const char *branch_cstr, const GwLoginInfo *info,
                       int64_t client_seq_id)
    {
      to_fte_str(body.fund_account_id, fund_cstr);
      to_fte_str(body.branch_id, branch_cstr);
      to_fte_str(body.account_id, info->account_id.data());
      to_fte_str(body.cust_id, info->cust_id.data());
      body.client_seq_id = client_seq_id;
      body.agw_seq_id = 0; ///< source: 组包恒 0 (会话缓存字段保留仅作记录)
    }

    /// ER 错误码: 交易所错误码(ord_rej_reason) 非 0 优先, 否则内部错误码(code) (source 顺序)
    int32 er_err_code(const gw_message::TradeOrderER &er)
    {
      return er.ord_rej_reason != 0 ? static_cast<int32_t>(er.ord_rej_reason) : static_cast<int32_t>(er.code);
    }

    // ---- FTE ↔ API 字典映射 (对齐 api-tmp-source) ----

    /// API 市场 → FTE 市场 (转换挂点: FTE 与 98 ATP 市场值域相同 101/102, 字典统一后直通)
    uint16_t map_api_market_to_fte(uint16_t api_market)
    {
      return api_market;
    }

    /// FTE 市场 → API 市场 (转换挂点: 值域相同直通; 修复旧版折回 1/2 与 API 字典 101/102 自相矛盾)
    int16_t map_fte_market_to_api(uint16_t fte_market)
    {
      return static_cast<int16_t>(fte_market);
    }

    /// FTE 订单状态 → API ORDER_STATE_* (API 字典=98 ATP AgwOrdStatus; 映射表, RUL-16):
    /// FTE {0空闲,1新报,2部成,3全成,4撤中,5全撤,8废} 与 ATP 同值不同义, 须显式映射
    uint8_t map_fte_ord_status(uint8_t st)
    {
      switch (st)
      {
      case 0:
        return ORDER_STATE_UNSENT; // 9 未申报
      case 1:
        return ORDER_STATE_NEW; // 0 已申报
      case 2:
        return ORDER_STATE_PART_FILLED;
      case 3:
        return ORDER_STATE_FILLED;
      case 4:
        return ORDER_STATE_WAIT_CANCEL; // 11 待撤销
      case 5:
        return ORDER_STATE_CANCELED; // 4 已撤销
      case 8:
        return ORDER_STATE_REJECTED;
      default:
        return st; ///< 未知值透传不吞信息, 联调关注
      }
    }

    /// FTE exec_type → API 回报类型 (转换挂点: 字符集与 ATP AgwExecType 相同 '0'/'4'/'8'/'F', 直通)
    char map_fte_exec_type(char et)
    {
      return et;
    }

    /// TradeOrderER 委托公共块 → OrderRtn/TradeRtn 委托块 (两结构前缀字段同名; 映射对齐 source)
    template <typename ApiT>
    void fill_er_common(ApiT &out, const gw_message::TradeOrderER &er)
    {
      from_fte_str(out.cust_id, er.cust_id);
      from_fte_str(out.fund_account_id, er.fund_account_id);
      from_fte_str(out.account_id, er.account_id);
      from_fte_str(out.branch_id, er.branch_id);
      out.side = er.side;
      out.order_type = er.ord_type;
      out.policy_id = 0;                                     ///< FTE 协议无策略佣金字段
      out.order_status = map_fte_ord_status(er.ord_status);  ///< [带确认] source 字典映射
      out.market_type = map_fte_market_to_api(er.market_id); ///< [带确认] 市场字段
      from_fte_str(out.security_id, er.security_id);
      out.order_price = er.price;
      out.order_qty = er.order_qty;
      out.client_seq_id = er.client_seq_id;
      out.order_sys_no = er.clordno; ///< 裁决: 个微 clordno = order_sys_no (委托/成交/撤单应答同源)
      out.frozen_amount = er.frozen_trade_value;
      out.fee = 0; ///< source: 委托回报费用置 0 (成交回报单独覆盖)
      out.trade_qty = er.cum_qty;
      out.cancel_qty = 0;                ///< [带确认] 撤单的返回设置
      out.order_time = er.transact_time; ///< FTE 无独立委托时间字段
    }
  } // namespace

  // 构造 / 析构
  gw_counter_direct::gw_counter_direct()
      : market_type(0), heart_interval(5), trade_send_queue_(nullptr), cb_mgr_(nullptr),
        log_(nullptr) {}

  gw_counter_direct::~gw_counter_direct() = default;

  // init
  int32 gw_counter_direct::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log)
  {
    cb_mgr_ = cb;
    log_ = log;
    market_type = static_cast<int16>(cfg.get_market_type());           ///< 市场
    heart_interval = static_cast<int16>(cfg.get_heartbeat_interval()); ///< 心跳间隔
    lb_common::atomic_store16(&trade_link_connect_, 0);                ///< 0-未链接/断开

    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "init gw counter direct ok, market_type=" << market_type << ", heart_interval=" << heart_interval
                  << end_log;
    return 0;
  }

  // ---- 业务发送函数 (模式: 状态检查 -> 会话快照 -> take_req_que_mem -> build -> cmt_req_que_mem) ----

  // deal_order_req: 买卖委托 (FTE TradeOrderReq, 消息号 1003; 多账户按请求资金账户门控)
  int32 gw_counter_direct::deal_order_req(const OrderReq &req)
  {
    if (unlikely(lb_common::atomic_load16(&trade_link_connect_) == 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_order_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_LINK_DISCONNECTED;
    }

    GwLoginInfo *info = login_cache_.find(req.fund_account_id.data(), req.fund_account_id.size());
    if (info == nullptr || unlikely(!info->log_gw.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_order_req: not login, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }

    const int32 take_len =
        GW_PKT_HEAD_SIZE + static_cast<int32>(sizeof(gw_message::TradeOrderReq)) + GW_PKT_TAIL_SIZE;
    char *data = nullptr;
    int64 pos = take_req_que_mem(data, take_len);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                     << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    build_order_msg(req, info, data + sizeof(link_send_event), take_len);

    cmt_req_que_mem(pos, take_len);
    return LBAPI_OK;
  }

  // deal_etf_order_req: ETF 申购赎回 (FTE TradeOrderReq 共用, 消息号 1010)
  int32 gw_counter_direct::deal_etf_order_req(const OrderReq &req)
  {
    if (unlikely(lb_common::atomic_load16(&trade_link_connect_) == 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_etf_order_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_LINK_DISCONNECTED;
    }

    GwLoginInfo *info = login_cache_.find(req.fund_account_id.data(), req.fund_account_id.size());
    if (info == nullptr || unlikely(!info->log_gw.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_etf_order_req: not login, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }

    const int32 take_len =
        GW_PKT_HEAD_SIZE + static_cast<int32>(sizeof(gw_message::TradeOrderReq)) + GW_PKT_TAIL_SIZE;
    char *data = nullptr;
    int64 pos = take_req_que_mem(data, take_len);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_etf_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                     << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    build_etf_order_msg(req, info, data + sizeof(link_send_event), take_len);

    cmt_req_que_mem(pos, take_len);
    return LBAPI_OK;
  }

  // deal_cancel_req: 委托撤单 (FTE CancelOrderReq, 消息号 1004, 原单经会话缓存定位)
  int32_t gw_counter_direct::deal_cancel_req(const CancelReq &req)
  {
    // 个微撤单仅支持系统委托号定位 (RUL-37, 同 FPGA RUL-36): order_sys_no 非正数
    // (0=仅私有号/双零, 负数非法)本地拒绝参数错误, 不发单不降级 (INVALID_PARAM
    // 非降级码); 98 维持双键定位
    if (unlikely(req.order_sys_no <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_cancel_req: order_sys_no required (gw not support private-no cancel), "
                        "fund_account="
                     << req.fund_account_id.data() << ", client_req_no=" << req.client_req_no
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_INVALID_PARAM;
    }

    if (unlikely(lb_common::atomic_load16(&trade_link_connect_) == 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_cancel_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                     << ", order_sys_no=" << req.order_sys_no << ", client_req_no=" << req.client_req_no << end_log;
      return LBAPI_ERR_LINK_DISCONNECTED;
    }

    GwLoginInfo *info = login_cache_.find(req.fund_account_id.data(), req.fund_account_id.size());
    if (info == nullptr || unlikely(!info->log_gw.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_cancel_req: not login, fund_account=" << req.fund_account_id.data()
                     << ", order_sys_no=" << req.order_sys_no << ", client_req_no=" << req.client_req_no << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }

    const int32 take_len =
        GW_PKT_HEAD_SIZE + static_cast<int32>(sizeof(gw_message::CancelOrderReq)) + GW_PKT_TAIL_SIZE;
    char *data = nullptr;
    int64 pos = take_req_que_mem(data, take_len);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_cancel_req: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                     << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    build_cancel_msg(req, info, data + sizeof(link_send_event), take_len);

    cmt_req_que_mem(pos, take_len);
    return LBAPI_OK;
  }

  // ---- 业务消息构建 (FTE 真实协议) ----

  // build_order_msg: 构造个微委托消息 (8B 头 + TradeOrderReq + 4B 校验和, 消息号 1003)
  void gw_counter_direct::build_order_msg(const OrderReq &req, const GwLoginInfo *info, char *o_buf,
                                          int32 buf_len)
  {
    gw_message::TradeOrderReq body;
    body.reset();
    fill_req_user(body, req.fund_account_id.data(), req.branch_id.data(), info, req.client_seq_id);
    to_fte_str(body.security_id, req.security_id.data());
    body.market_id = map_api_market_to_fte(req.market_type); ///< 市场: FTE 与 API(=ATP) 值域相同, 直通
    body.side = req.side;                                    ///< [待确认-联调] FTE side 字典未证实, 现按与 ATP 相同 ('1'买/'2'卖) 透传, 差异在此挂映射
    body.order_type = req.order_type;                        ///< [待确认-联调] FTE order_type 字典未证实, 现按与 ATP 相同透传, 差异在此挂映射
    body.order_qty = req.order_qty;                          ///< 数量: API 与 FTE 均 ×100, 直传 (RUL-35)
    body.order_price = req.order_price;                      ///< 单位口径: 价×10000 直传
    body.stop_px = req.stop_price;
    gw_pkt_encode(o_buf, buf_len, gw_message::kPktOrderReq, body);
  }

  // build_etf_order_msg: 构造个微 ETF 申购赎回消息 (TradeOrderReq 共用, 消息号 1010)
  void gw_counter_direct::build_etf_order_msg(const OrderReq &req, const GwLoginInfo *info, char *o_buf,
                                              int32 buf_len)
  {
    gw_message::TradeOrderReq body;
    body.reset();
    fill_req_user(body, req.fund_account_id.data(), req.branch_id.data(), info, req.client_seq_id);
    to_fte_str(body.security_id, req.security_id.data());
    body.market_id = map_api_market_to_fte(req.market_type);
    body.side = req.side;
    body.order_type = req.order_type;
    body.order_qty = req.order_qty;                          ///< 数量: API 与 FTE 均 ×100, 直传 (RUL-35)
    body.order_price = req.order_price;
    body.stop_px = req.stop_price;
    gw_pkt_encode(o_buf, buf_len, gw_message::kPktETFReq, body);
  }

  // build_cancel_msg: 构造个微撤单消息 (8B 头 + CancelOrderReq + 4B 校验和, 消息号 1004)
  // orig_clordno/orig_client_seq_id 直接取自请求 (用户从回报取得, 无服务端定位缓存):
  // order_sys_no = 柜台委托号, client_seq_id = 原委托的用户私有报单号
  void gw_counter_direct::build_cancel_msg(const CancelReq &req, const GwLoginInfo *info, char *o_buf,
                                           int32 buf_len)
  {
    gw_message::CancelOrderReq body;
    body.reset();
    fill_req_user(body, req.fund_account_id.data(), req.branch_id.data(), info, req.client_req_no);

    body.orig_client_seq_id = req.client_seq_id;
    body.orig_clordno = req.order_sys_no;

    gw_pkt_encode(o_buf, buf_len, gw_message::kPktCancelOrderReq, body);
  }

  // ---- 登录消息构建 (FTE LogOnReq, 消息号 1001) ----

  // build_login_msg: 构造个微账户登录消息, 返回完整包长 (含头与校验和)
  // 字段填充对齐 source: account_id/cust_id/client_feature_code 原样 memcpy (保留源尾部字节),
  // password 截 99 字节 (尾部保持 reset 空格)
  int32 gw_counter_direct::build_login_msg(const acc_login_event_info &info, const GwLoginInfo *linfo,
                                           char *o_buf, int32 buf_len)
  {
    gw_message::LogOnReq body;
    body.reset();
    to_fte_str(body.fund_account_id, info.fund_account_id);
    to_fte_str(body.branch_id, info.branch_id);
    std::memcpy(body.account_id.data(), info.account_id, sizeof(body.account_id));
    std::memcpy(body.cust_id.data(), info.cust_id, sizeof(body.cust_id)); // 可留空, 由 FTE 回填
    body.client_seq_id = info.cust_req_no;
    body.agw_seq_id = 0; ///< 恒 0: 重登不携带会话序号 (RUL-26); 缓存 agw_seq_id 仅作记录不再组包
    body.heart_bt_int = static_cast<uint32_t>(heart_interval);
    // password: 截 99 字节, 尾部保持 reset 空格 (对齐 source)
    size_t pwd_len = strnlen(info.password, sizeof(info.password));
    if (pwd_len > sizeof(body.password) - 1)
    {
      pwd_len = sizeof(body.password) - 1;
    }
    std::memcpy(body.password.data(), info.password, pwd_len);
    std::memcpy(body.client_feature_code.data(), info.client_feature_code, sizeof(body.client_feature_code));
    body.agw_user.fill(' '); ///< source: 非统一接入填空格
    return gw_pkt_encode(o_buf, buf_len, gw_message::kPktLoginReq, body);
  }

  // ---- 登录应答构造 ----

  // build_login_rtn: 从 acc_login_event_info 构建 API 层 LoginAns (供 ans_cust_login 失败路径使用)
  void gw_counter_direct::build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg,
                                          LoginAns &ans)
  {
    ans.client_req_no = info.cust_req_no;
    std::memcpy(ans.cust_id.data(), info.cust_id, std::min<int32>(sizeof(ans.cust_id), sizeof(info.cust_id)));
    std::memcpy(ans.fund_account_id.data(), info.fund_account_id,
                std::min<int32>(sizeof(ans.fund_account_id), sizeof(info.fund_account_id)));
    std::memcpy(ans.account_id.data(), info.account_id,
                std::min<int32>(sizeof(ans.account_id), sizeof(info.account_id)));
    std::memcpy(ans.branch_id.data(), info.branch_id,
                std::min<int32>(sizeof(ans.branch_id), sizeof(info.branch_id)));
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
    ans.login_time = 0; ///< FTE 协议无登录时间字段
  }

  // build_login_rtn: 从 FTE LogOnAns 构建 API 层 LoginAns
  void gw_counter_direct::build_login_rtn(const gw_message::LogOnAns &msg, LoginAns &o_ans)
  {
    std::memset(&o_ans, 0, sizeof(o_ans));
    o_ans.client_req_no = msg.client_seq_id;
    from_fte_str(o_ans.cust_id, msg.cust_id);
    from_fte_str(o_ans.fund_account_id, msg.fund_account_id);
    from_fte_str(o_ans.account_id, msg.account_id);
    from_fte_str(o_ans.branch_id, msg.branch_id);
    o_ans.market_type = market_type;
    o_ans.err_code = static_cast<int32_t>(msg.error_code);
    std::memset(o_ans.err_msg.data(), 0, sizeof(o_ans.err_msg)); ///< FTE 协议无错误文本字段
    o_ans.login_time = 0;                                        ///< FTE 协议无登录时间字段
  }

  // ---- deal_cust_login / ans_cust_login: 多账户登录事件处理 ----

  // deal_cust_login: 处理账户登录事件 (引擎线程; 条目凭据暂存唯一入口):
  //   先凭据暂存/刷新 (幂等/合并路径同样刷新 —— 盘中改密重登即时生效), 再按该账户
  //   log_gw 状态机 to_login 三场景:
  //     用户(1): 已登录→幂等回调成功 / 在途→合并忽略 / NONE/LOGOUT→占位发送;
  //     内部(0): OK→RELOGIN 重发 / 非 OK→静默终止。
  // 成功返回消息长度 (含消息头与校验和), 0-幂等/合并/静默终止(不发包), <0 出错
  int32 gw_counter_direct::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len)
  {
    lb_common::lb_log_hand tlh(log_);

    GwLoginInfo *info = login_cache_.prepare_login(req);

    int32 thavelog = 0;
    int32 tret = info->log_gw.to_login(thavelog, req.user_origin);
    if (tret == 0)
    {
      if (thavelog == 1)
      {
        // 幂等: 已登录状态下用户重复 login → 直接回调成功, 不发后台
        info_log(tlh) << "gw user have login ok,branch_id=" << req.branch_id << ",fund_account=" << req.fund_account_id
                      << ",client_req_no=" << req.cust_req_no << end_log;
        LoginAns ans;
        build_login_rtn(req, 0, NULL, ans);
        cb_mgr_->on_login(ans);
      }
      return 0; ///< 合并(在途等待首应答)/内部事件非 OK 静默终止
    }

    const int32 pkt_len = GW_PKT_HEAD_SIZE + static_cast<int32>(sizeof(gw_message::LogOnReq)) + GW_PKT_TAIL_SIZE;
    if (buf_len < pkt_len)
    {
      error_log(tlh) << "gw user login and msg len error,branch_id=" << req.branch_id
                     << ",fund_account=" << req.fund_account_id << ",client_req_no=" << req.cust_req_no << end_log;
      ans_cust_login(req, LBAPI_ERR_BUILD_MSG, "build fast counter login msg error");
      return LBAPI_ERR_MSG_LEN;
    }

    int32 tlen = build_login_msg(req, info, o_buf, buf_len);
    if (unlikely(tlen <= 0))
    {
      ans_cust_login(req, LBAPI_ERR_BUILD_MSG, "build fast counter login msg error");
      return LBAPI_ERR_BUILD_MSG;
    }
    return tlen;
  }

  // ans_cust_login: 引擎同步阶段失败 (connect/组包/发送失败) —— 该账户 log_gw 清在途:
  // 原用户在途 → 回调失败 (状态位本为 NONE); 原内部重登在途 → 清在途回退 (保登录
  // 状态, 下次重连重试) 但仍回调客户; 一个链接有多个客户，不可因为某个客户失败而关闭链接！！！
  void gw_counter_direct::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg)
  {
    LoginAns ans;
    build_login_rtn(req, err_ret, err_msg, ans);

    GwLoginInfo *info = login_cache_.find(req.fund_account_id, sizeof(req.fund_account_id));
    int32 tpending = 0;
    if (info != nullptr)
    {
      tpending = info->log_gw.clear_pending();
    }
    if (tpending != 0)
    {
      cb_mgr_->on_login(ans);
    }
  }

  // deal_log_ans: 处理 FTE 账户登录应答 (LogOnAns, 消息号 2001, 引擎线程)
  // 按应答资金账户定位条目: 成功先回填应答字段 (cust/branch/account/agw_seq_id,
  // 空值不覆盖) 再置 OK 形成屏障, 经 log_gw 状态机判定回调; 失败走统一状态机处理
  void gw_counter_direct::deal_log_ans(const gw_message::LogOnAns &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv gw cust login answer msg, user_seq_no=" << msg.client_seq_id
                  << ", fund_account=" << gw_message::AsString(msg.fund_account_id).c_str()
                  << ", session_status=" << msg.session_status << ", err_code=" << msg.error_code << end_log;

    GwLoginInfo *info = login_cache_.find(msg.fund_account_id.data(), msg.fund_account_id.size());
    if (nullptr == info)
    {
      return; ///< 非本实例发起的迟到应答, 忽略
    }

    LoginAns ans;
    build_login_rtn(msg, ans);
    if (msg.error_code == 0)
    {
      // 应答字段回填 (组包 user 块的 cust_id/account_id 来源)
      std::array<char, INNER_CUST_ID_LEN> tcust;
      std::array<char, INNER_BRANCH_ID_LEN> tbranch;
      std::array<char, INNER_ACCOUNT_ID_LEN> taccount;
      from_fte_str(tcust, msg.cust_id);
      from_fte_str(tbranch, msg.branch_id);
      from_fte_str(taccount, msg.account_id);
      login_cache_.fill_login_ok(info, tcust.data(), tbranch.data(), taccount.data(), msg.agw_seq_id);

      // 用户在途 → 回调一次; 内部重登/迟到 → 静默
      if (info->log_gw.ans_login_ok() == 1)
      {
        cb_mgr_->on_login(ans);
      }
    }
    else
    {
      // 失败应答: 状态机统一处理 (USER/RELOGIN → 清状态 NONE + 回调失败[RELOGIN 曾成功
      // 客户重登被拒视为异常, 不再自动重登]; LOGOUT/无在途迟到 → 忽略)
      if (info->log_gw.ans_login_err() == 1)
      {
        error_log(tlh) << "gw cust login failed, fund_account=" << gw_message::AsString(msg.fund_account_id).c_str()
                       << ",err_code=" << ans.err_code << end_log;
        cb_mgr_->on_login(ans);
      }
      else
      {
        warning_log(tlh) << "ignore late gw cust login fail ans without pending,fund_account="
                         << gw_message::AsString(msg.fund_account_id).c_str() << ",err_code=" << msg.error_code << end_log;
      }
    }
  }

  // ---- deal_recv_msg: 解析 FTE 返回消息 (按 PktNewHeader 切帧 + 校验和校验 + msg_id 分发) ----

  // deal_recv_msg: 处理个微返回消息
  // 帧格式: 8B 头(大端 msg_id+msg_len) + 消息体(主机序直通) + 4B 校验和(大端)
  int32 gw_counter_direct::deal_recv_msg(const char *buf, int32 len, int16 link_type)
  {
    (void)link_type;
    const int32 min_pkt = GW_PKT_HEAD_SIZE + GW_PKT_TAIL_SIZE; ///< 最小完整包 = 心跳 12B
    if (buf == nullptr || len < min_pkt)
    {
      return 0;
    }

    int32 deal_len = 0;

    while (len - deal_len >= min_pkt)
    {
      gw_message::PktNewHeader head;
      if (!head.decode(buf + deal_len, static_cast<size_t>(len - deal_len)))
      {
        return deal_len; ///< 不足 8B 头, 半包留待后续数据
      }
      // msg_len 为 uint32: 上界校验防脏数据/负数游走 (对齐既有防御语义)
      if (unlikely(head.msg_len > GW_RECV_BODY_MAX))
      {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "gw counter recv msg len error, msg_len=" << head.msg_len << ", link_type=" << link_type
                       << end_log;
        // 返回错误, 让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
      const int32 total = GW_PKT_HEAD_SIZE + static_cast<int32>(head.msg_len) + GW_PKT_TAIL_SIZE;
      if (unlikely(total > len - deal_len))
      {
        return deal_len; ///< 半包
      }

      // 校验和校验: 不匹配视为脏包, 记日志跳过 (仍按头定界继续解析后续包)
      const uint32_t sum = gw_pkt_checksum(buf + deal_len, static_cast<size_t>(GW_PKT_HEAD_SIZE) + head.msg_len);
      uint32_t tail = 0;
      std::memcpy(&tail, buf + deal_len + GW_PKT_HEAD_SIZE + head.msg_len, GW_PKT_TAIL_SIZE);
      if (unlikely(gw_message::detail::ByteSwap32(tail) != sum))
      {
        lb_common::lb_log_hand tlh(log_);
        warning_log(tlh) << "gw counter recv checksum mismatch, skip pkt, msg_id=" << head.msg_id
                         << ", msg_len=" << head.msg_len << end_log;
        deal_len += total;
        continue;
      }

      const char *body = buf + deal_len + GW_PKT_HEAD_SIZE;
      const size_t body_len = head.msg_len;

      switch (head.msg_id)
      {
      case gw_message::kPktLoginAns: // 2001 登录应答
      {
        gw_message::LogOnAns msg;
        msg.reset();
        if (unlikely(!msg.decode(body, body_len)))
        {
          // 短于本版本已知最小长度: 协议不可兼容, 通知用户销毁 API; 跳过该消息继续解析
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "gw counter recv login_ans msg len too short, msg_len=" << head.msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "login_ans msg len too short");
          break;
        }
        deal_log_ans(msg);
        break;
      }
      case gw_message::kPktNewHeartBeat: // 3 心跳应答 (无消息体)
      {
        if (trade_eng_op_ != nullptr)
        {
          trade_eng_op_->deal_heart_msg_ans(LINK_TYPE_SPEED_TRADE);
        }
        break;
      }
      case gw_message::kPktOrderAns: // 2003 委托回报
      {
        gw_message::TradeOrderER er;
        er.reset();
        if (unlikely(!er.decode(body, body_len)))
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "gw counter recv order ans msg len too short, msg_len=" << head.msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "order ans msg len too short");
          break;
        }
        deal_order_rtn(er);
        break;
      }
      case gw_message::kPktOrderMatch:    // 2005 成交回报
      case gw_message::kPktEtfOrderMatch: // 2010 ETF 成交回报 (source: 映射为委托回报 OrderRtn)
      {
        gw_message::TradeOrderER er;
        er.reset();
        if (unlikely(!er.decode(body, body_len)))
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "gw counter recv trade rtn msg len too short, msg_len=" << head.msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "trade rtn msg len too short");
          break;
        }
        if (head.msg_id == gw_message::kPktEtfOrderMatch && er.no_security > 0)
        {
          lb_common::lb_log_hand tlh(log_);
          info_log(tlh) << "gw recv etf trade rtn with constituents, no_security=" << er.no_security
                        << ", clordno=" << er.clordno << end_log;
        }
        if (head.msg_id == gw_message::kPktEtfOrderMatch)
        {
          // [待裁决·A3] 2010 无条件走 on_order_rtn (含 exec_type='F'), 与 RUL-23 "2005/2010
          // 同规则分流" 文字矛盾; 沿用 source 行为暂保留, 裁决后对齐 (SYSTEM_CONTRACT §7)
          deal_order_rtn(er); ///< source: ETF 成交回报走 on_order_rtn
        }
        else
        {
          deal_trade_rtn(er);
        }
        break;
      }
      case gw_message::kPktCancelOrderAns: // 2004 撤单应答
      {
        gw_message::TradeOrderER er;
        er.reset();
        if (unlikely(!er.decode(body, body_len)))
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "gw counter recv cancel ans msg len too short, msg_len=" << head.msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "cancel ans msg len too short");
          break;
        }
        deal_cancel_rsp(er);
        break;
      }
      case gw_message::kPktRejectMsg: // 9 拒绝回报
      {
        gw_message::RejectMsg rej;
        rej.reset();
        if (unlikely(!rej.decode(body, body_len)))
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "gw counter recv reject msg len too short, msg_len=" << head.msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "reject msg len too short");
          break;
        }
        deal_reject(rej);
        break;
      }
      default:
      {
        // 未知/未实现消息, 跳过, 不返回错误, 以避免链接关闭 (兼容 FTE 后续新增消息)
        lb_common::lb_log_hand tlh(log_);
        info_log(tlh) << "gw counter recv unknown msg, msg_id=" << head.msg_id << ", msg_len=" << head.msg_len
                      << end_log;
        break;
      }
      }

      deal_len += total;
    }

    return deal_len;
  }

  // ---- 回报处理: TradeOrderER / RejectMsg → API 回调 ----

  // deal_order_rtn: 委托回报 (2003) → on_order_rtn
  void gw_counter_direct::deal_order_rtn(const gw_message::TradeOrderER &er)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv gw order rtn, fund_account=" << er.fund_account_id.data()
                  << ",client_seq_id=" << er.client_seq_id << ", clordno=" << er.clordno
                  << ", security_id=" << gw_message::AsString(er.security_id).c_str()
                  << ", ord_status=" << static_cast<uint32>(er.ord_status) << ", exec_type=" << er.exec_type
                  << ", code=" << er.code << ", rej_reason=" << er.ord_rej_reason << end_log;

    StreamInfo si_out;
    si_out.counter_type = get_counter_type();
    si_out.partno = 0;            ///< 个微无分区概念 (RUL-32)
    si_out.stream_seq = 0;

    alignas(32) OrderRtn out;
    std::memset(&out, 0, sizeof(out));
    fill_er_common(out, er);
    out.err_code = er_err_code(er);
    out.rtn_type = map_fte_exec_type(er.exec_type); ///< source: exec_type 字典 ('0'/'8'/'4'/'F')
    out.update_time = er.transact_time;

    cb_mgr_->on_order_rtn(si_out, out);
  }

  // deal_trade_rtn: 成交回报 (2005/2010) → on_trade_rtn
  void gw_counter_direct::deal_trade_rtn(const gw_message::TradeOrderER &er)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv gw trade rtn, fund_account=" << er.fund_account_id.data()
                  << ",client_seq_id=" << er.client_seq_id << ", clordno=" << er.clordno
                  << ", exec_id=" << gw_message::AsString(er.exec_id).c_str() << ", last_px=" << er.last_px
                  << ", last_qty=" << er.last_qty << ", cum_qty=" << er.cum_qty << end_log;

    StreamInfo si_out;
    si_out.counter_type = get_counter_type();
    si_out.partno = 0;            ///< 个微无分区概念 (RUL-32)
    si_out.stream_seq = 0;

    // RUL-23: 成交回报通道按 exec_type 分流 —— 仅 'F' 已成交进 on_trade_rtn;
    // 其余事件类型(委托确认'0'/撤单成交'4'/委托废单'8'等)复用 deal_order_rtn
    // (fill_er_common) 转 OrderRtn 走 on_order_rtn
    if (er.exec_type != RSP_TYPE_TRADE)
    {
      info_log(tlh) << "gw trade channel exec_type=" << er.exec_type << " not trade, route to order rtn,client_seq_id="
                    << er.client_seq_id << ", clordno=" << er.clordno << end_log;
      deal_order_rtn(er);
      return;
    }

    alignas(32) TradeRtn out;
    std::memset(&out, 0, sizeof(out));
    fill_er_common(out, er);
    out.rtn_type = RSP_TYPE_TRADE;
    out.fee = er.frozen_fee + er.fee; ///< source: 成交回报费用 = 冻结 + 单笔
    out.exec_time = er.transact_time;
    from_fte_str(out.exec_id, er.exec_id);
    out.exec_price = er.last_px;
    out.exec_qty = er.last_qty;
    out.exec_amount = er.total_value_traded; ///< source: FTE 成交金额专字段
    out.exec_fee = er.fee;

    cb_mgr_->on_trade_rtn(si_out, out);
  }

  // deal_cancel_rsp: 撤单应答 (2004) → on_cancel_rsp
  void gw_counter_direct::deal_cancel_rsp(const gw_message::TradeOrderER &er)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv gw cancel ans, fund_account=" << er.fund_account_id.data()
                  << ",client_seq_id=" << er.client_seq_id << ", clordno=" << er.clordno
                  << ", orig_clordno=" << er.orig_clordno << ", code=" << er.code
                  << ", rej_reason=" << er.ord_rej_reason << end_log;

    StreamInfo si_out;
    si_out.counter_type = get_counter_type();
    si_out.partno = 0;            ///< 个微无分区概念 (RUL-32)
    si_out.stream_seq = 0;

    alignas(32) CancelRsp out;
    std::memset(&out, 0, sizeof(out));
    out.client_req_no = er.client_seq_id; ///< 撤单请求号 (CancelOrderReq.user.client_seq_id)
    from_fte_str(out.cust_id, er.cust_id);
    from_fte_str(out.fund_account_id, er.fund_account_id);
    from_fte_str(out.account_id, er.account_id);
    from_fte_str(out.branch_id, er.branch_id);
    out.market_type = map_fte_market_to_api(er.market_id);
    out.order_sys_no = er.orig_clordno;   ///< 被撤原单柜台委托号
    out.client_seq_id = er.client_seq_id; ///< source: 撤单回报客户号即撤单请求号
    out.err_code = er_err_code(er);
    out.rej_api = 0;

    cb_mgr_->on_cancel_rsp(si_out, out);
  }

  // deal_reject: 拒绝回报 (9) → on_order_rtn (source: 统一映射为委托废单, 不区分撤单拒绝)
  // [待确认] 是否包括撤单等请求的拒绝？？
  void gw_counter_direct::deal_reject(const gw_message::RejectMsg &rej)
  {
    lb_common::lb_log_hand tlh(log_);
    warning_log(tlh) << "recv gw reject msg, client_seq_id=" << rej.client_seq_id
                     << ", reject_reason=" << rej.reject_reason_code << ", cancel_flag=" << rej.cancel_flag
                     << ", business_type=" << static_cast<uint32>(rej.business_type) << end_log;

    StreamInfo si_out;
    si_out.counter_type = get_counter_type();
    si_out.partno = 0;            ///< 个微无分区概念 (RUL-32)
    si_out.stream_seq = 0;       ///< 恒 0 (RUL-32, 本地计数器已删)

    alignas(32) OrderRtn out;
    std::memset(&out, 0, sizeof(out));
    from_fte_str(out.cust_id, rej.cust_id);
    from_fte_str(out.fund_account_id, rej.fund_account_id);
    from_fte_str(out.account_id, rej.account_id);
    from_fte_str(out.branch_id, rej.branch_id);
    out.client_seq_id = rej.client_seq_id;   ///< source: market_type 不填 (保持 0)
    out.order_status = ORDER_STATE_REJECTED; ///< FTE 废单 → 已拒绝
    out.rtn_type = RSP_TYPE_REJECTED;
    out.err_code = static_cast<int32_t>(rej.reject_reason_code);
    cb_mgr_->on_order_rtn(si_out, out);
  }

  // ---- deal_send_error / build_api_*_rej: 发送失败 → API 层拒绝回报 ----

  void gw_counter_direct::build_api_order_rej(const gw_message::TradeOrderReq &msg, int32 err_code, OrderRtn &o_rtn,
                                              StreamInfo &o_stream)
  {
    std::memset(&o_rtn, 0, sizeof(o_rtn));
    from_fte_str(o_rtn.cust_id, msg.cust_id);
    from_fte_str(o_rtn.fund_account_id, msg.fund_account_id);
    from_fte_str(o_rtn.account_id, msg.account_id);
    from_fte_str(o_rtn.branch_id, msg.branch_id);
    o_rtn.side = msg.side;
    o_rtn.order_type = msg.order_type;
    o_rtn.order_status = ORDER_STATE_REJECTED; ///< API 层拒绝 → 已拒绝
    from_fte_str(o_rtn.security_id, msg.security_id);
    o_rtn.market_type = static_cast<int16_t>(msg.market_id);
    o_rtn.order_price = msg.order_price;
    o_rtn.order_qty = msg.order_qty;
    o_rtn.client_seq_id = msg.client_seq_id;
    o_rtn.rtn_type = RSP_TYPE_REJECTED; ///< API 层拒绝 ('8')
    o_rtn.err_code = err_code;

    o_stream.counter_type = get_counter_type(); ///< API 层拒绝 = 柜台回报流 (RUL-32)
    o_stream.partno = 0;            ///< 无回报消息, 无分区
    o_stream.stream_seq = 0;
  }

  void gw_counter_direct::build_api_cancel_rej(const gw_message::CancelOrderReq &msg, int32 err_code, CancelRsp &o_rtn,
                                               StreamInfo &o_stream)
  {
    std::memset(&o_rtn, 0, sizeof(o_rtn));
    o_rtn.client_req_no = msg.client_seq_id; ///< 撤单请求号 (组包时写入 user.client_seq_id)
    from_fte_str(o_rtn.cust_id, msg.cust_id);
    from_fte_str(o_rtn.fund_account_id, msg.fund_account_id);
    from_fte_str(o_rtn.account_id, msg.account_id);
    from_fte_str(o_rtn.branch_id, msg.branch_id);
    o_rtn.market_type = market_type;
    o_rtn.order_sys_no = msg.orig_clordno;
    o_rtn.client_seq_id = msg.orig_client_seq_id;
    o_rtn.err_code = err_code;
    o_rtn.rej_api = 1; ///< API 层拒绝 (队列满/发送失败)

    o_stream.counter_type = get_counter_type(); ///< API 层拒绝 = 柜台回报流 (RUL-32)
    o_stream.partno = 0;            ///< 无回报消息, 无分区
    o_stream.stream_seq = 0;
  }

  void gw_counter_direct::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret)
  {
    (void)link_type;
    if (msg_buf == nullptr || cb_mgr_ == nullptr)
    {
      return;
    }
    if (msg_len < GW_PKT_HEAD_SIZE + GW_PKT_TAIL_SIZE)
    {
      return;
    }
    gw_message::PktNewHeader head;
    if (!head.decode(msg_buf, static_cast<size_t>(msg_len)))
    {
      return;
    }
    const char *body = msg_buf + GW_PKT_HEAD_SIZE;

    switch (head.msg_id)
    {
    case gw_message::kPktOrderReq:         // 1003 普通委托
    case gw_message::kPktMarginTradingReq: // 1005 两融委托 (预留)
    case gw_message::kPktCreditAuctionReq: // 1006 竞价委托 (预留)
    case gw_message::kPktETFReq:           // 1010 ETF 申购赎回
    {
      // 委托发送失败: 解回原请求体, 构造 OrderRtn 通知客户
      gw_message::TradeOrderReq m;
      m.reset();
      if (m.decode(body, head.msg_len))
      {
        OrderRtn o_rtn;
        StreamInfo o_stream;
        build_api_order_rej(m, err_ret, o_rtn, o_stream);
        cb_mgr_->on_order_rtn(o_stream, o_rtn);
      }
      break;
    }
    case gw_message::kPktCancelOrderReq: // 1004 撤单
    {
      // 撤单发送失败: 解回原请求体, 构造 CancelRsp 通知客户
      gw_message::CancelOrderReq m;
      m.reset();
      if (m.decode(body, head.msg_len))
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

  // ---- build_heart_msg: 构造个微心跳 (消息号 3, 无消息体: 8B 头 + 4B 校验和) ----

  int32 gw_counter_direct::build_heart_msg(char *o_buf, int32 buf_len)
  {
    if (o_buf == nullptr || buf_len < GW_PKT_HEAD_SIZE + GW_PKT_TAIL_SIZE)
    {
      return -1;
    }
    gw_message::PktNewHeader head;
    head.msg_id = gw_message::kPktNewHeartBeat;
    head.msg_len = 0;
    const size_t off = head.encode(o_buf, static_cast<size_t>(buf_len));
    const uint32_t sum = gw_pkt_checksum(o_buf, off);
    const uint32_t be = gw_message::detail::ByteSwap32(sum);
    std::memcpy(o_buf + off, &be, GW_PKT_TAIL_SIZE);
    return GW_PKT_HEAD_SIZE + GW_PKT_TAIL_SIZE;
  }

  // ---- 链接状态通知 ----

  // deal_link_connect: 断线重连后, 全部已登录账户 (log_gw OK) 批量自动重登 (需求6)
  // (资格预查; 在途控制统一到 deal_cust_login 的状态机 to_login;
  //  成功静默 / 失败清状态+回调+不再自动重登)
  int32 gw_counter_direct::deal_link_connect(int16 link_type, int32 have_switch)
  {
    (void)have_switch; // 个微柜台不支持地址切换, 此参数忽略
    if (link_type == LINK_TYPE_SPEED_TRADE)
    {
      lb_common::atomic_store16(&trade_link_connect_, 1);

      std::vector<acc_login_event_info> evs;
      login_cache_.collect_relogin_events(evs);
      for (size_t i = 0; i < evs.size(); ++i)
      {
        int32 ret = delive_relogin_event(evs[i]);
        if (ret != 0)
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "gw delive relogin event error,ret=" << ret << ",idx=" << i << ",total=" << evs.size()
                         << end_log;
          return ret; ///< 关链重来, 下次重连重试全部 (队满为暂态)
        }
      }

      if (cb_mgr_ != nullptr)
      {
        cb_mgr_->on_link_status(get_counter_type(), LINK_TYPE_SPEED_TRADE, 1);
      }
    }
    return 0;
  }

  // delive_relogin_event: 单账户重登事件投递 (直投, 不做状态控制 ——
  // 资格与回调控制统一到 deal_cust_login 的状态机 to_login 内部分支)
  int32 gw_counter_direct::delive_relogin_event(const acc_login_event_info &ev)
  {
    if (trade_send_queue_ == nullptr || trade_eng_op_ == nullptr)
    {
      return LBAPI_ERR_STATE_LIMITED;
    }

    char *data = nullptr;
    int64 pos = take_req_que_mem(data, sizeof(acc_login_event_info), LINK_EVENT_TYPE_ACCOUNT_LOGIN);
    if (pos <= 0)
    {
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    acc_login_event_info *pinfo = reinterpret_cast<acc_login_event_info *>(data + sizeof(link_send_event));
    *pinfo = ev;
    pinfo->user_origin = 0; ///< 内部重登 (缓存事件可能来自用户发起, 强制改内部来源)

    cmt_req_que_mem(pos, sizeof(acc_login_event_info));

    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "gw delive relogin event ok,branch_id=" << ev.branch_id << ",fund_account=" << ev.fund_account_id
                  << end_log;
    return 0;
  }

  void gw_counter_direct::deal_link_close(int16 link_type)
  {
    if (link_type == LINK_TYPE_SPEED_TRADE)
    {
      lb_common::atomic_store16(&trade_link_connect_, 0);

      // 断链不清除任何账户登录状态 (解耦不变量, §3.2.0), 仅清全部条目在途
      // (无登录回调, 用户经链接断开事件感知), 重连后按 OK 态批量自动重登 (需求6)
      login_cache_.clear_all_pending();

      if (cb_mgr_ != nullptr)
      {
        cb_mgr_->on_link_status(get_counter_type(), LINK_TYPE_SPEED_TRADE, 0);
      }
    }
  }

  // deal_logout_req: 柜台侧登出 (该账户 log_gw 状态机 to_logout 统一判定)
  // 本地登出语义: 不发 LogOutReq、置 LOGOUT; 多账户共用链路, 不关链
  // 返回 1 → 置 LOGOUT 成功; 0 → 本来就未登录 → 成功; <0 → 用户在途 → 登出失败
  int32_t gw_counter_direct::deal_logout_req(const LogoutReq &req)
  {
    GwLoginInfo *info = login_cache_.find(req.fund_account_id.data(), req.fund_account_id.size());
    if (info == nullptr)
    {
      return 0;
    }
    int32_t tret = info->log_gw.to_logout();
    if (tret < 0)
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw counter cust logout but now logining,branch_id=" << req.branch_id.data()
                     << ",fund_account=" << req.fund_account_id.data()
                     << ",client_req_no=" << req.client_req_no << end_log;
      return LBAPI_ERR_STATE_LIMITED;
    }
    if (tret == 1)
    {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "gw counter cust logout ok,branch_id=" << req.branch_id.data()
                    << ",fund_account=" << req.fund_account_id.data() << end_log;
    }
    return 0;
  }

} // namespace lb_api

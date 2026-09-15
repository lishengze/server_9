// gw_counter_direct - 个微软件极速柜台 (直连模式) 实现
//
// 个微柜台无查询接口, 仅直连模式 (C1/C2 配置)
// 业务消息: 委托 / 撤单 / ETF申购赎回
// 推送: 委托 / 成交 / 撤单回报
//
// 协议: FTE TCP Binary（gw_message::* 结构体，大端字节序）
//   报文格式: [PktNewHeader 8B | 消息体 | 校验和 4B]
//   校验和: GenerateSzCheckSum 对 [头+体] 逐字节求和 %256，转大端追加

#include "gw_counter_direct.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "matomic.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

#include <cstdlib>
#include <cstring>

namespace lb_api {

// ============================================================
// 构造 / 析构 / init
// ============================================================

gw_counter_direct::gw_counter_direct()
    : market_type(0), heart_interval(5), login_state(0), trade_send_queue_(nullptr), cb_mgr_(nullptr), session_seq_(0),
      log_(nullptr) {}

gw_counter_direct::~gw_counter_direct() = default;

int32 gw_counter_direct::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log) {
  cb_mgr_ = cb;
  log_ = log;
  market_type = static_cast<int16>(cfg.get_market_type());
  heart_interval = static_cast<int16>(cfg.get_heartbeat_interval());
  login_state = 0;
  trade_link_connect_ = 0;
  session_seq_ = 0;

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "init gw counter direct ok, market_type=" << market_type << ", heart_interval=" << heart_interval
                << end_log;
  return 0;
}

// ============================================================
// FTE 校验和计算（保留供非热路径使用）
// ============================================================

uint32_t gw_counter_direct::GenerateSzCheckSum(const char *buf, uint32_t len) {
  uint32_t sum = 0;
  for (uint32_t i = 0; i < len; ++i) {
    sum += static_cast<uint8_t>(buf[i]);
  }
  return sum % 256;
}

// ============================================================
// 性能优化辅助函数（方案 B/C/D）
// ============================================================

// 写单个字节并累加校验和
static inline void cksum_put(char*& p, uint8_t b, uint32_t& sum) {
  *p++ = static_cast<char>(b); sum += b;
}

// 写定长块并累加校验和
static inline void cksum_write(char*& p, const void* src, size_t n, uint32_t& sum) {
  const uint8_t* s = static_cast<const uint8_t*>(src);
  for (size_t i = 0; i < n; ++i) { uint8_t b = s[i]; *p++ = static_cast<char>(b); sum += b; }
}

// 拷贝并空格填充 + 累加校验和（方案 B：一次遍历完成拷贝 + 尾部补空格，消除 memcpy + space_pad 两次遍历）
// 语义：将 src 的数据（到首个 \0 或 n）复制到 p，剩余部分补空格，同时累加校验和
static inline void cksum_copy_pad(char*& p, const char* src, size_t n, uint32_t& sum) {
  size_t i = 0;
  for (; i < n && src[i] != '\0'; ++i) { uint8_t b = static_cast<uint8_t>(src[i]); *p++ = static_cast<char>(b); sum += b; }
  for (; i < n; ++i) { *p++ = ' '; sum += static_cast<uint32_t>(' '); }
}

// 写大端 uint32（ByteSwap32，用于消息头）并累加校验和
static inline void cksum_be32(char*& p, uint32_t v, uint32_t& sum) {
  uint32_t be = gw_message::detail::ByteSwap32(v);
  cksum_write(p, &be, 4, sum);
}

// 写网络序 int64（HostToNetwork 当前为 no-op，保留调用以兼容未来）并累加校验和
static inline void cksum_net64(char*& p, int64_t v, uint32_t& sum) {
  uint64_t be = gw_message::detail::HostToNetwork(static_cast<uint64_t>(v));
  cksum_write(p, &be, 8, sum);
}

// 写网络序 uint32 并累加校验和
static inline void cksum_net32(char*& p, uint32_t v, uint32_t& sum) {
  uint32_t be = gw_message::detail::HostToNetwork(v);
  cksum_write(p, &be, 4, sum);
}

// 写网络序 uint16 并累加校验和
static inline void cksum_net16(char*& p, uint16_t v, uint32_t& sum) {
  uint16_t be = gw_message::detail::HostToNetwork(v);
  cksum_write(p, &be, 2, sum);
}

// 写入校验和尾部（sum % 256 → ByteSwap32 → memcpy 4 字节，不累加到 sum）
static inline void cksum_finish(char*& p, uint32_t sum) {
  uint32_t calc = sum % 256;
  uint32_t be = gw_message::detail::ByteSwap32(calc);
  memcpy(p, &be, 4);
  p += 4;
}

// ============================================================
// 业务发送函数
// ============================================================

// deal_order_req: 买卖委托
int32 gw_counter_direct::deal_order_req(const OrderReq &req) {
  if (unlikely(lb_common::atomic_load16(&trade_link_connect_) == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_order_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  if (lb_common::atomic_load16(&login_state) != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_order_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }

  // FTE 报文长度 = PktNewHeader(8) + TradeOrderReq(106) + 校验和(4) = 118
  int32 take_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::TradeOrderReq) + sizeof(uint32_t));
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_order_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

// deal_etf_order_req: ETF 申购赎回
int32 gw_counter_direct::deal_etf_order_req(const OrderReq &req) {
  if (unlikely(lb_common::atomic_load16(&trade_link_connect_) == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_etf_order_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  if (lb_common::atomic_load16(&login_state) != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_etf_order_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }

  int32 take_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::TradeOrderReq) + sizeof(uint32_t));
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_etf_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_etf_order_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

// deal_cancel_req: 委托撤单
int32_t gw_counter_direct::deal_cancel_req(const CancelReq &req) {
  if (unlikely(lb_common::atomic_load16(&trade_link_connect_) == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_cancel_req: trade link not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  if (lb_common::atomic_load16(&login_state) != 2) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_cancel_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_req_no=" << req.client_req_no << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }

  int32 take_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::CancelOrderReq) + sizeof(uint32_t));
  char *data = nullptr;
  int64 pos = take_req_que_mem(data, take_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_cancel_req: queue full, pos=" << pos << ", client_req_no=" << req.client_req_no
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  build_cancel_msg(req, data + sizeof(link_send_event));

  cmt_req_que_mem(pos, take_len);
  return LBAPI_OK;
}

// ============================================================
// 消息构建 (FTE 协议)
// ============================================================

// 将 NewAPI 市场ID（1=上海, 2=深圳/北交所）映射为 FTE 市场ID（101=上海, 102=深圳, 109=北京）
static uint16_t map_api_market_id_to_fte(uint16_t api_market_id) {
  switch (api_market_id) {
    case 1:  return 101;  // SH → kShangHai
    case 2:  return 102;  // SZ → kShenZhen
    default: return api_market_id;
  }
}

// build_order_msg: 构造 FTE 委托消息 (PktNewHeader + TradeOrderReq + 校验和)
// 方案 C/D：直接序列化到 o_buf，边写边累加校验和（单趟），消除中间 body 对象与二次校验和遍历
void gw_counter_direct::build_order_msg(const OrderReq &req, char *o_buf) {
  // 获取会话缓存（补充 account_id / cust_id）
  std::string fa_key(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16));
  GwSessionInfo *session = GwSessionCache::instance().get_session(fa_key);

  char* p = o_buf;
  uint32_t sum = 0;
  // PktNewHeader（ByteSwap32 大端）
  cksum_be32(p, gw_message::kPktOrderReq, sum);
  cksum_be32(p, sizeof(gw_message::TradeOrderReq), sum);
  // TradeOrderUser：fund_account_id / branch_id / account_id / cust_id（拷贝+空格填充+校验和）
  cksum_copy_pad(p, req.fund_account_id.data(), sizeof(gw_message::TradeOrderReq::fund_account_id), sum);
  cksum_copy_pad(p, req.branch_id.data(), sizeof(gw_message::TradeOrderReq::branch_id), sum);
  if (session) {
    cksum_copy_pad(p, session->account_id.data(), sizeof(gw_message::TradeOrderReq::account_id), sum);
    cksum_copy_pad(p, session->cust_id.data(), sizeof(gw_message::TradeOrderReq::cust_id), sum);
  } else {
    for (size_t i = 0; i < sizeof(gw_message::TradeOrderReq::account_id); ++i) cksum_put(p, ' ', sum);
    for (size_t i = 0; i < sizeof(gw_message::TradeOrderReq::cust_id); ++i) cksum_put(p, ' ', sum);
  }
  cksum_net64(p, req.client_seq_id, sum);
  cksum_net64(p, 0, sum);  // agw_seq_id
  // TradeOrderInfo
  cksum_copy_pad(p, req.security_id.data(), sizeof(gw_message::TradeOrderReq::security_id), sum);
  cksum_net16(p, map_api_market_id_to_fte(req.market_type), sum);
  cksum_put(p, static_cast<uint8_t>(req.side), sum);
  cksum_put(p, static_cast<uint8_t>(req.order_type), sum);
  cksum_net64(p, req.order_qty, sum);
  cksum_net64(p, req.order_price, sum);
  cksum_net64(p, req.stop_price, sum);
  // 校验和
  cksum_finish(p, sum);
}

// build_etf_order_msg: 构造 FTE ETF 委托消息 (PktNewHeader + TradeOrderReq + 校验和, msg_id=1010)
// 方案 C/D：与 build_order_msg 结构一致，仅 msg_id 不同；直接序列化 + 单趟校验和
void gw_counter_direct::build_etf_order_msg(const OrderReq &req, char *o_buf) {
  std::string fa_key(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16));
  GwSessionInfo *session = GwSessionCache::instance().get_session(fa_key);

  char* p = o_buf;
  uint32_t sum = 0;
  // PktNewHeader（ByteSwap32 大端）
  cksum_be32(p, gw_message::kPktETFReq, sum);
  cksum_be32(p, sizeof(gw_message::TradeOrderReq), sum);
  // TradeOrderUser
  cksum_copy_pad(p, req.fund_account_id.data(), sizeof(gw_message::TradeOrderReq::fund_account_id), sum);
  cksum_copy_pad(p, req.branch_id.data(), sizeof(gw_message::TradeOrderReq::branch_id), sum);
  if (session) {
    cksum_copy_pad(p, session->account_id.data(), sizeof(gw_message::TradeOrderReq::account_id), sum);
    cksum_copy_pad(p, session->cust_id.data(), sizeof(gw_message::TradeOrderReq::cust_id), sum);
  } else {
    for (size_t i = 0; i < sizeof(gw_message::TradeOrderReq::account_id); ++i) cksum_put(p, ' ', sum);
    for (size_t i = 0; i < sizeof(gw_message::TradeOrderReq::cust_id); ++i) cksum_put(p, ' ', sum);
  }
  cksum_net64(p, req.client_seq_id, sum);
  cksum_net64(p, 0, sum);  // agw_seq_id
  // TradeOrderInfo
  cksum_copy_pad(p, req.security_id.data(), sizeof(gw_message::TradeOrderReq::security_id), sum);
  cksum_net16(p, map_api_market_id_to_fte(req.market_type), sum);
  cksum_put(p, static_cast<uint8_t>(req.side), sum);
  cksum_put(p, static_cast<uint8_t>(req.order_type), sum);
  cksum_net64(p, req.order_qty, sum);
  cksum_net64(p, req.order_price, sum);
  cksum_net64(p, req.stop_price, sum);
  // 校验和
  cksum_finish(p, sum);
}

// build_cancel_msg: 构造 FTE 撤单消息 (PktNewHeader + CancelOrderReq + 校验和)
// 方案 C/D：直接序列化 + 单趟校验和
void gw_counter_direct::build_cancel_msg(const CancelReq &req, char *o_buf) {
  std::string fa_key(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16));
  GwSessionInfo *session = GwSessionCache::instance().get_session(fa_key);

  char* p = o_buf;
  uint32_t sum = 0;
  // PktNewHeader（ByteSwap32 大端）
  cksum_be32(p, gw_message::kPktCancelOrderReq, sum);
  cksum_be32(p, sizeof(gw_message::CancelOrderReq), sum);
  // TradeOrderUser
  cksum_copy_pad(p, req.fund_account_id.data(), sizeof(gw_message::CancelOrderReq::fund_account_id), sum);
  cksum_copy_pad(p, req.branch_id.data(), sizeof(gw_message::CancelOrderReq::branch_id), sum);
  if (session) {
    cksum_copy_pad(p, session->account_id.data(), sizeof(gw_message::CancelOrderReq::account_id), sum);
    cksum_copy_pad(p, session->cust_id.data(), sizeof(gw_message::CancelOrderReq::cust_id), sum);
  } else {
    for (size_t i = 0; i < sizeof(gw_message::CancelOrderReq::account_id); ++i) cksum_put(p, ' ', sum);
    for (size_t i = 0; i < sizeof(gw_message::CancelOrderReq::cust_id); ++i) cksum_put(p, ' ', sum);
  }
  cksum_net64(p, req.client_req_no, sum);
  cksum_net64(p, 0, sum);  // agw_seq_id

  // 撤单定位原单：从 GwSessionCache 映射表反查
  cksum_net64(p, GwSessionCache::instance().get_orig_client_seq_id(fa_key, req.order_sys_no), sum);
  cksum_net64(p, GwSessionCache::instance().get_clordno(fa_key, req.order_sys_no), sum);
  // 校验和
  cksum_finish(p, sum);
}

// build_login_msg: 构造 FTE 登录消息 (PktNewHeader + LogOnReq + 校验和)
void gw_counter_direct::build_login_msg(const acc_login_event_info &info, char *o_buf, int32 buf_len) {
  int32 msg_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::LogOnReq) + sizeof(uint32_t));
  if (buf_len < msg_len) return;

  gw_message::PktNewHeader header;
  header.msg_id = gw_message::kPktLoginReq;
  header.msg_len = sizeof(gw_message::LogOnReq);

  gw_message::LogOnReq body;
  body.reset();

  // TradeOrderUser 字段
  // FTE 协议用空格填充（CopyToArray），确保定长字段匹配
  body.fund_account_id.fill(' ');
  size_t fa_len = strnlen(info.fund_account_id, sizeof(body.fund_account_id));
  if (fa_len > sizeof(body.fund_account_id)) fa_len = sizeof(body.fund_account_id);
  memcpy(body.fund_account_id.data(), info.fund_account_id, fa_len);

  body.branch_id.fill(' ');
  size_t br_len = strnlen(info.branch_id, sizeof(body.branch_id));
  if (br_len > sizeof(body.branch_id)) br_len = sizeof(body.branch_id);
  memcpy(body.branch_id.data(), info.branch_id, br_len);

  memcpy(body.account_id.data(), info.account_id, sizeof(body.account_id));
  memcpy(body.cust_id.data(), info.cust_id, sizeof(body.cust_id));  // 可留空，由 FTE 回填
  body.client_seq_id = info.cust_req_no;
  body.agw_seq_id = 0;

  // LogOnReq 特有字段
  body.heart_bt_int = static_cast<uint32_t>(heart_interval);

  // password: 截断到 100 字节（acc_login_event_info.password 为 256）
  size_t pwd_len = strnlen(info.password, sizeof(info.password));
  if (pwd_len > sizeof(body.password) - 1) pwd_len = sizeof(body.password) - 1;
  memcpy(body.password.data(), info.password, pwd_len);

  // client_feature_code
  memcpy(body.client_feature_code.data(), info.client_feature_code, sizeof(body.client_feature_code));

  // agw_user 填空（非统一接入）
  body.agw_user.fill(' ');

  // 序列化
  size_t off = header.encode(o_buf, static_cast<size_t>(buf_len));
  body.encode(o_buf + off, static_cast<size_t>(buf_len - static_cast<int32>(off)));
  off += sizeof(gw_message::LogOnReq);
  // 校验和
  uint32_t calc_cks = GenerateSzCheckSum(o_buf, static_cast<uint32_t>(off));
  uint32_t be_cks = gw_message::detail::ByteSwap32(calc_cks);
  memcpy(o_buf + off, &be_cks, 4);
}

// ============================================================
// 登录应答构造
// ============================================================

// build_login_rtn: 从 acc_login_event_info 构建 API 层 LoginAns (供 ans_cust_login 失败路径使用)
void gw_counter_direct::build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg,
                                        LoginAns &ans) {
  ans.client_req_no = info.cust_req_no;
  memcpy(ans.cust_id.data(), info.cust_id, std::min<int32>(sizeof(ans.cust_id), sizeof(info.cust_id)));
  memcpy(ans.fund_account_id.data(), info.fund_account_id,
         std::min<int32>(sizeof(ans.fund_account_id), sizeof(info.fund_account_id)));
  memcpy(ans.account_id.data(), info.account_id,
         std::min<int32>(sizeof(ans.account_id), sizeof(info.account_id)));
  memcpy(ans.branch_id.data(), info.branch_id,
         std::min<int32>(sizeof(ans.branch_id), sizeof(info.branch_id)));
  ans.market_type = market_type;
  ans.err_code = err_ret;
  if (err_msg != nullptr) {
    lb_common::comm_utils::str_copy_format(ans.err_msg.data(), err_msg, sizeof(ans.err_msg));
  } else {
    memset(ans.err_msg.data(), 0, sizeof(ans.err_msg));
  }
  ans.login_time = 0;
}

// build_login_rtn: 从 gw_message::LogOnAns 构建 API 层 LoginAns
void gw_counter_direct::build_login_rtn(const gw_message::LogOnAns &msg, LoginAns &o_ans) {
  memset(&o_ans, 0, sizeof(o_ans));
  o_ans.client_req_no = msg.client_seq_id;
  memcpy(o_ans.cust_id.data(), msg.cust_id.data(), sizeof(o_ans.cust_id));
  memcpy(o_ans.fund_account_id.data(), msg.fund_account_id.data(), sizeof(o_ans.fund_account_id));
  memcpy(o_ans.account_id.data(), msg.account_id.data(), sizeof(o_ans.account_id));
  memcpy(o_ans.branch_id.data(), msg.branch_id.data(), sizeof(o_ans.branch_id));
  o_ans.market_type = market_type;
  o_ans.err_code = static_cast<int32>(msg.error_code);
  o_ans.login_time = 0;
}

// ============================================================
// deal_cust_login / ans_cust_login
// ============================================================

// deal_cust_login: 处理账户登录事件, 构造 FTE 登录报文
int32 gw_counter_direct::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len) {
  if (lb_common::atomic_load16(&login_state) == 2) {
    LoginAns ans;
    build_login_rtn(req, 0, NULL, ans);
    cb_mgr_->on_login(ans);
    return 0;
  }

  int32 msg_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::LogOnReq) + sizeof(uint32_t));
  if (buf_len < msg_len) {
    return LBAPI_ERR_MSG_LEN;
  }

  // 先创建会话缓存（存 order_way_ext/user_info 等，供后续业务使用）
  GwSessionCache::instance().create_session(req);

  build_login_msg(req, o_buf, buf_len);
  lb_common::atomic_store16(&login_state, 1);
  return msg_len;
}

// ans_cust_login: 登录结果回调（失败路径）
void gw_counter_direct::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg) {
  lb_common::atomic_store16(&login_state, 0);

  LoginAns ans;
  build_login_rtn(req, err_ret, err_msg, ans);
  cb_mgr_->on_login(ans);
}

// deal_log_ans: 处理 FTE 账户登录应答 (LogOnAns)
void gw_counter_direct::deal_log_ans(const char *body, int32 body_len) {
  gw_message::LogOnAns ans;
  ans.reset();
  if (!ans.decode(body, static_cast<size_t>(body_len))) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_log_ans: decode LogOnAns failed, body_len=" << body_len << end_log;
    return;
  }

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "recv gw cust login answer msg, client_seq_id=" << ans.client_seq_id
                << ", fund_account=" << std::string(ans.fund_account_id.data(), strnlen(ans.fund_account_id.data(), 16)).c_str()
                << ", error_code=" << ans.error_code << end_log;

  // 回填会话缓存（cust_id / account_id 由 FTE 在应答中回填）
  GwSessionCache::instance().fill_session_from_ans(ans);

  LoginAns login_ans;
  build_login_rtn(ans, login_ans);

  if (ans.error_code == 0) {
    lb_common::atomic_store16(&login_state, 2);
    cb_mgr_->on_login(login_ans);
  } else {
    lb_common::atomic_store16(&login_state, 0);
    cb_mgr_->on_login(login_ans);
  }
}

// ============================================================
// deal_recv_msg: FTE 拆包分发
// ============================================================

int32 gw_counter_direct::deal_recv_msg(const char *buf, int32 len, int16 link_type) {
  (void)link_type;
  if (buf == nullptr || len < (int32)sizeof(gw_message::PktNewHeader)) {
    return 0;
  }

  int32 deal_len = 0;

  while (len - deal_len >= (int32)sizeof(gw_message::PktNewHeader)) {
    // 1. 解析消息头
    gw_message::PktNewHeader header;
    if (!header.decode(buf + deal_len, static_cast<size_t>(len - deal_len))) break;

    // 2. 消息长度上限校验（参考 FTE 服务端：msg_len > 65536 拒绝）
    if (header.msg_len > 65536) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_recv_msg: msg_len too large, msg_id=" << header.msg_id
                     << ", msg_len=" << header.msg_len << end_log;
      deal_len += static_cast<int32>(sizeof(gw_message::PktNewHeader));
      continue;
    }

    int32 whole_msg_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + header.msg_len + sizeof(uint32_t));
    if (whole_msg_len > len - deal_len) {
      // 半包，等待更多数据
      return deal_len;
    }

    // 3. 校验校验和
    uint32_t recv_cks = 0;
    memcpy(&recv_cks, buf + deal_len + sizeof(gw_message::PktNewHeader) + header.msg_len, 4);
    uint32_t calc_cks = GenerateSzCheckSum(buf + deal_len,
        static_cast<uint32_t>(sizeof(gw_message::PktNewHeader) + header.msg_len));
    recv_cks = gw_message::detail::ByteSwap32(recv_cks);
    if (recv_cks != calc_cks) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "gw deal_recv_msg: checksum mismatch, msg_id=" << header.msg_id
                     << ", msg_len=" << header.msg_len
                     << ", whole_len=" << whole_msg_len
                     << ", recv_cks=" << recv_cks << ", calc_cks=" << calc_cks << end_log;
      deal_len += whole_msg_len;
      continue;
    }

    // 4. 按 msg_id 分发
    const char *body = buf + deal_len + sizeof(gw_message::PktNewHeader);
    {
      // 方案 E：每笔回报日志降级为 debug，避免高吞吐下日志 I/O 成为瓶颈
      lb_common::lb_log_hand tlh(log_);
      uint32_t raw_id = 0;
      memcpy(&raw_id, buf + deal_len, 4);
      char hexbuf[16];
      snprintf(hexbuf, sizeof(hexbuf), "0x%08x", (unsigned)raw_id);
      debug_log(tlh) << "gw deal_recv_msg: msg_id=" << header.msg_id
                     << " raw=" << hexbuf
                     << " msg_len=" << header.msg_len << end_log;
    }
    switch (header.msg_id) {
    case gw_message::kPktLoginAns:
      deal_log_ans(body, static_cast<int32>(header.msg_len));
      break;

    case gw_message::kPktOrderAns:
      deal_order_rtn(body, static_cast<int32>(header.msg_len));
      break;

    case gw_message::kPktCancelOrderAns:
      deal_cancel_rsp(body, static_cast<int32>(header.msg_len));
      break;

    case gw_message::kPktOrderMatch:
      deal_trade_rtn(body, static_cast<int32>(header.msg_len));
      break;

    case gw_message::kPktEtfOrderMatch:
      deal_etf_trade_rtn(body, static_cast<int32>(header.msg_len));
      break;

    case gw_message::kPktRejectMsg:
      deal_reject_msg(body, static_cast<int32>(header.msg_len));
      break;

    case gw_message::kPktNewHeartBeat:
      // FTE 心跳无消息体，确认心跳（避免 aio_tcp 心跳超时误判）
      if (trade_eng_op_ != nullptr) {
        trade_eng_op_->deal_heart_msg_ans(LINK_TYPE_SPEED_TRADE);
      }
      break;

    default:
      // 未知消息，跳过，不返回错误，以避免链接关闭
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "gw deal_recv_msg: unknown msg_id=" << header.msg_id
                    << ", msg_len=" << header.msg_len << end_log;
      break;
    }

    deal_len += whole_msg_len;
  }

  return deal_len;
}

// ============================================================
// 回报处理 (TradeOrderER 解析)
// ============================================================

// deal_order_rtn: 委托回报 (exec_type='0' New / '8' Reject)
void gw_counter_direct::deal_order_rtn(const char *body, int32 body_len) {
  if (body_len < (int32)sizeof(gw_message::TradeOrderER)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_order_rtn: body too short, body_len=" << body_len << end_log;
    return;
  }

  gw_message::TradeOrderER er;
  er.reset();
  if (!er.decode(body, static_cast<size_t>(body_len))) return;

  // 记录 order_id → {clordno, client_seq_id} 映射（用于撤单）
  std::string fa_id(er.fund_account_id.data(), strnlen(er.fund_account_id.data(), 16));
  int64_t order_sys_no = strtoll(er.order_id.data(), nullptr, 10);
  GwSessionCache::instance().record_order_locator(fa_id, order_sys_no, er.clordno, er.client_seq_id);

  // 构造 OrderRtn
  OrderRtn rtn;
  memset(&rtn, 0, sizeof(rtn));
  memcpy(rtn.cust_id.data(), er.cust_id.data(), sizeof(rtn.cust_id));
  memcpy(rtn.fund_account_id.data(), er.fund_account_id.data(), sizeof(rtn.fund_account_id));
  memcpy(rtn.account_id.data(), er.account_id.data(), sizeof(rtn.account_id));
  memcpy(rtn.branch_id.data(), er.branch_id.data(), sizeof(rtn.branch_id));
  rtn.side = er.side;
  rtn.order_type = er.ord_type;
  rtn.order_status = map_ord_status(er.ord_status);
  rtn.market_type = map_market_id(er.market_id);
  memcpy(rtn.security_id.data(), er.security_id.data(), sizeof(rtn.security_id));
  rtn.order_price = er.price;
  rtn.order_qty = er.order_qty;
  rtn.client_seq_id = er.client_seq_id;
  rtn.rtn_type = map_exec_type(er.exec_type);
  rtn.err_code = (er.ord_rej_reason != 0) ? static_cast<int32>(er.ord_rej_reason) : static_cast<int32>(er.code);
  rtn.order_sys_no = order_sys_no;
  rtn.frozen_amount = er.frozen_trade_value;
  rtn.fee = 0;
  rtn.trade_qty = er.cum_qty;
  rtn.cancel_qty = 0;
  rtn.order_time = er.transact_time;
  rtn.update_time = er.transact_time;

  StreamInfo stream;
  stream.counter_type = get_counter_type();
  stream.stream_seq = lb_common::atomic_fetch_add64(&session_seq_, 1) + 1;

  cb_mgr_->on_order_rtn(stream, rtn);
}

// deal_trade_rtn: 成交回报 (exec_type='F')
void gw_counter_direct::deal_trade_rtn(const char *body, int32 body_len) {
  if (body_len < (int32)sizeof(gw_message::TradeOrderER)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_trade_rtn: body too short, body_len=" << body_len << end_log;
    return;
  }

  gw_message::TradeOrderER er;
  er.reset();
  {
    // 方案 E：每笔成交回报日志降级为 debug，避免高吞吐下日志 I/O 成为瓶颈
    lb_common::lb_log_hand tlh(log_);
    debug_log(tlh) << "gw deal_trade_rtn: enter, body_len=" << body_len
                   << ", sizeof(TradeOrderER)=" << (int)sizeof(gw_message::TradeOrderER) << end_log;
  }
  if (!er.decode(body, static_cast<size_t>(body_len))) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_trade_rtn: decode failed, body_len=" << body_len << end_log;
    return;
  }

  // 记录 order_id → {clordno, client_seq_id} 映射
  std::string fa_id(er.fund_account_id.data(), strnlen(er.fund_account_id.data(), 16));
  int64_t order_sys_no = strtoll(er.order_id.data(), nullptr, 10);
  GwSessionCache::instance().record_order_locator(fa_id, order_sys_no, er.clordno, er.client_seq_id);

  // 构造 TradeRtn（复用 OrderRtn 字段 + 成交特有字段）
  TradeRtn rtn;
  memset(&rtn, 0, sizeof(rtn));
  memcpy(rtn.cust_id.data(), er.cust_id.data(), sizeof(rtn.cust_id));
  memcpy(rtn.fund_account_id.data(), er.fund_account_id.data(), sizeof(rtn.fund_account_id));
  memcpy(rtn.account_id.data(), er.account_id.data(), sizeof(rtn.account_id));
  memcpy(rtn.branch_id.data(), er.branch_id.data(), sizeof(rtn.branch_id));
  rtn.side = er.side;
  rtn.order_type = er.ord_type;
  rtn.order_status = map_ord_status(er.ord_status);
  rtn.market_type = map_market_id(er.market_id);
  memcpy(rtn.security_id.data(), er.security_id.data(), sizeof(rtn.security_id));
  rtn.order_price = er.price;
  rtn.order_qty = er.order_qty;
  rtn.client_seq_id = er.client_seq_id;
  rtn.order_sys_no = order_sys_no;
  rtn.frozen_amount = er.frozen_trade_value;
  rtn.fee = er.frozen_fee + er.fee;
  rtn.trade_qty = er.cum_qty;
  rtn.cancel_qty = 0;
  rtn.order_time = er.transact_time;
  // 成交特有字段
  rtn.exec_time = er.transact_time;
  memcpy(rtn.exec_id.data(), er.exec_id.data(), std::min<size_t>(sizeof(rtn.exec_id), sizeof(er.exec_id)));
  rtn.exec_price = er.last_px;
  rtn.exec_qty = er.last_qty;
  rtn.exec_amount = er.total_value_traded;
  rtn.exec_fee = er.fee;

  StreamInfo stream;
  stream.counter_type = get_counter_type();
  stream.stream_seq = lb_common::atomic_fetch_add64(&session_seq_, 1) + 1;

  cb_mgr_->on_trade_rtn(stream, rtn);
}

// deal_cancel_rsp: 撤单回报 (exec_type='4')
void gw_counter_direct::deal_cancel_rsp(const char *body, int32 body_len) {
  if (body_len < (int32)sizeof(gw_message::TradeOrderER)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_cancel_rsp: body too short, body_len=" << body_len << end_log;
    return;
  }

  gw_message::TradeOrderER er;
  er.reset();
  if (!er.decode(body, static_cast<size_t>(body_len))) return;

  CancelRsp rsp;
  memset(&rsp, 0, sizeof(rsp));
  memcpy(rsp.cust_id.data(), er.cust_id.data(), sizeof(rsp.cust_id));
  memcpy(rsp.fund_account_id.data(), er.fund_account_id.data(), sizeof(rsp.fund_account_id));
  memcpy(rsp.account_id.data(), er.account_id.data(), sizeof(rsp.account_id));
  memcpy(rsp.branch_id.data(), er.branch_id.data(), sizeof(rsp.branch_id));
  rsp.client_req_no = er.client_seq_id;
  rsp.market_type = map_market_id(er.market_id);
  rsp.order_sys_no = strtoll(er.order_id.data(), nullptr, 10);
  rsp.client_seq_id = er.client_seq_id;
  rsp.err_code = (er.ord_rej_reason != 0) ? static_cast<int32>(er.ord_rej_reason) : static_cast<int32>(er.code);
  rsp.rej_api = 0;

  StreamInfo stream;
  stream.counter_type = get_counter_type();
  stream.stream_seq = lb_common::atomic_fetch_add64(&session_seq_, 1) + 1;

  cb_mgr_->on_cancel_rsp(stream, rsp);
}

// deal_etf_trade_rtn: ETF 成交回报 (TradeOrderER + ConstituentStock[])
void gw_counter_direct::deal_etf_trade_rtn(const char *body, int32 body_len) {
  // 先解析固定部分得到 no_security
  gw_message::TradeOrderER er;
  er.reset();
  if (!er.decode(body, static_cast<size_t>(body_len))) return;

  // ETF 成交回报映射为 OrderRtn（与普通委托回报相同）
  std::string fa_id(er.fund_account_id.data(), strnlen(er.fund_account_id.data(), 16));
  int64_t order_sys_no = strtoll(er.order_id.data(), nullptr, 10);
  GwSessionCache::instance().record_order_locator(fa_id, order_sys_no, er.clordno, er.client_seq_id);

  OrderRtn rtn;
  memset(&rtn, 0, sizeof(rtn));
  memcpy(rtn.cust_id.data(), er.cust_id.data(), sizeof(rtn.cust_id));
  memcpy(rtn.fund_account_id.data(), er.fund_account_id.data(), sizeof(rtn.fund_account_id));
  memcpy(rtn.account_id.data(), er.account_id.data(), sizeof(rtn.account_id));
  memcpy(rtn.branch_id.data(), er.branch_id.data(), sizeof(rtn.branch_id));
  rtn.side = er.side;
  rtn.order_type = er.ord_type;
  rtn.order_status = map_ord_status(er.ord_status);
  rtn.market_type = map_market_id(er.market_id);
  memcpy(rtn.security_id.data(), er.security_id.data(), sizeof(rtn.security_id));
  rtn.order_price = er.price;
  rtn.order_qty = er.order_qty;
  rtn.client_seq_id = er.client_seq_id;
  rtn.rtn_type = map_exec_type(er.exec_type);
  rtn.order_sys_no = order_sys_no;
  rtn.frozen_amount = er.frozen_trade_value;
  rtn.fee = 0;
  rtn.trade_qty = er.cum_qty;
  rtn.cancel_qty = 0;
  rtn.order_time = er.transact_time;
  rtn.update_time = er.transact_time;

  StreamInfo stream;
  stream.counter_type = get_counter_type();
  stream.stream_seq = lb_common::atomic_fetch_add64(&session_seq_, 1) + 1;

  cb_mgr_->on_order_rtn(stream, rtn);
}

// deal_reject_msg: 处理拒绝消息 (RejectMsg)
void gw_counter_direct::deal_reject_msg(const char *body, int32 body_len) {
  if (body_len < (int32)sizeof(gw_message::RejectMsg)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "gw deal_reject_msg: body too short, body_len=" << body_len << end_log;
    return;
  }

  gw_message::RejectMsg rej;
  rej.reset();
  if (!rej.decode(body, static_cast<size_t>(body_len))) return;

  StreamInfo stream;
  stream.counter_type = get_counter_type();
  stream.stream_seq = lb_common::atomic_fetch_add64(&session_seq_, 1) + 1;

  OrderRtn rtn;
  memset(&rtn, 0, sizeof(rtn));
  memcpy(rtn.cust_id.data(), rej.cust_id.data(), sizeof(rtn.cust_id));
  memcpy(rtn.fund_account_id.data(), rej.fund_account_id.data(), sizeof(rtn.fund_account_id));
  memcpy(rtn.account_id.data(), rej.account_id.data(), sizeof(rtn.account_id));
  memcpy(rtn.branch_id.data(), rej.branch_id.data(), sizeof(rtn.branch_id));
  rtn.client_seq_id = rej.client_seq_id;
  rtn.err_code = rej.reject_reason_code;
  rtn.rtn_type = RSP_TYPE_ORDER_DISCARD;
  rtn.order_status = ORDER_STATE_DISCARD;

  cb_mgr_->on_order_rtn(stream, rtn);
}

// ============================================================
// 状态字典映射
// ============================================================

int32_t gw_counter_direct::map_ord_status(uint8_t fte_status) {
  switch (fte_status) {
  case 0:  return ORDER_STATE_ORDER_IDLE;    // kNull
  case 1:  return ORDER_STATE_ORDER_NEW;     // kSended
  case 2:  return ORDER_STATE_DONE_PART;     // kPartiallyFilled
  case 3:  return ORDER_STATE_DONE_FULL;     // kFilled
  case 4:  return ORDER_STATE_CANCEL_ING;    // kPendingCancel
  case 5:  return ORDER_STATE_CANCEL_ALL;    // kCancelled
  case 8:  return ORDER_STATE_DISCARD;       // kReject
  default: return ORDER_STATE_ORDER_IDLE;
  }
}

int32_t gw_counter_direct::map_exec_type(char exec_type) {
  switch (exec_type) {
  case '0':  return RSP_TYPE_COUNTER_RSP;     // New
  case '8':  return RSP_TYPE_ORDER_DISCARD;   // Reject
  case '4':  return RSP_TYPE_CANCEL_RSP;      // Cancelled
  case 'F':  return RSP_TYPE_ORDER_TRADE;     // Trade
  default:   return RSP_TYPE_COUNTER_RSP;
  }
}

int16_t gw_counter_direct::map_market_id(uint16_t fte_market_id) {
  // FTE market_id: 101=上海, 102=深圳
  // NewAPI market_type: 1=上海, 2=深圳
  switch (fte_market_id) {
  case 101: return 1;
  case 102: return 2;
  default:  return static_cast<int16_t>(fte_market_id);
  }
}

// ============================================================
// deal_send_error / build_api_*_rej (FTE 版)
// ============================================================

void gw_counter_direct::build_api_order_rej(const gw_message::TradeOrderReq *req, int32 err_code,
                                            OrderRtn &o_rtn, StreamInfo &o_stream) {
  memset(&o_rtn, 0, sizeof(o_rtn));
  memcpy(o_rtn.fund_account_id.data(), req->fund_account_id.data(), sizeof(o_rtn.fund_account_id));
  memcpy(o_rtn.branch_id.data(), req->branch_id.data(), sizeof(o_rtn.branch_id));
  o_rtn.client_seq_id = req->client_seq_id;
  o_rtn.market_type = market_type;
  o_rtn.err_code = err_code;
  o_rtn.rtn_type = RSP_TYPE_ORDER_DISCARD;
  o_rtn.order_status = ORDER_STATE_DISCARD;
  o_stream.counter_type = get_counter_type();
  o_stream.stream_seq = lb_common::atomic_fetch_add64(&session_seq_, 1) + 1;
}

void gw_counter_direct::build_api_cancel_rej(const gw_message::CancelOrderReq *req, int32 err_code,
                                             CancelRsp &o_rtn, StreamInfo &o_stream) {
  memset(&o_rtn, 0, sizeof(o_rtn));
  memcpy(o_rtn.fund_account_id.data(), req->fund_account_id.data(), sizeof(o_rtn.fund_account_id));
  memcpy(o_rtn.branch_id.data(), req->branch_id.data(), sizeof(o_rtn.branch_id));
  o_rtn.client_req_no = req->client_seq_id;
  o_rtn.market_type = market_type;
  o_rtn.err_code = err_code;
  o_rtn.rej_api = 1;
  o_stream.counter_type = get_counter_type();
  o_stream.stream_seq = lb_common::atomic_fetch_add64(&session_seq_, 1) + 1;
}

void gw_counter_direct::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret) {
  (void)link_type;
  if (msg_buf == nullptr || cb_mgr_ == nullptr) return;
  if (msg_len < (int32)sizeof(gw_message::PktNewHeader)) return;

  gw_message::PktNewHeader header;
  if (!header.decode(msg_buf, static_cast<size_t>(msg_len))) return;

  switch (header.msg_id) {
  case gw_message::kPktOrderReq:
  case gw_message::kPktETFReq: {
    // 解析 TradeOrderReq 获取用户信息
    if (msg_len >= (int32)(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::TradeOrderReq))) {
      gw_message::TradeOrderReq req;
      req.reset();
      if (req.decode(msg_buf + sizeof(gw_message::PktNewHeader), sizeof(gw_message::TradeOrderReq))) {
        OrderRtn o_rtn;
        StreamInfo o_stream;
        build_api_order_rej(&req, err_ret, o_rtn, o_stream);
        cb_mgr_->on_order_rtn(o_stream, o_rtn);
      }
    }
    break;
  }
  case gw_message::kPktCancelOrderReq: {
    if (msg_len >= (int32)(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::CancelOrderReq))) {
      gw_message::CancelOrderReq req;
      req.reset();
      if (req.decode(msg_buf + sizeof(gw_message::PktNewHeader), sizeof(gw_message::CancelOrderReq))) {
        CancelRsp o_rtn;
        StreamInfo o_stream;
        build_api_cancel_rej(&req, err_ret, o_rtn, o_stream);
        cb_mgr_->on_cancel_rsp(o_stream, o_rtn);
      }
    }
    break;
  }
  default:
    break;
  }
}

// ============================================================
// build_heart_msg: FTE 心跳（仅 8 字节头 + 4 字节校验和）
// ============================================================

int32 gw_counter_direct::build_heart_msg(char *o_buf, int32 buf_len) {
  int32 msg_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + sizeof(uint32_t));
  if (o_buf == nullptr || buf_len < msg_len) {
    return -1;
  }

  gw_message::PktNewHeader header;
  header.msg_id = gw_message::kPktNewHeartBeat;
  header.msg_len = 0;

  size_t off = header.encode(o_buf, static_cast<size_t>(buf_len));
  // 校验和：对 [头] 求和 %256
  uint32_t calc_cks = GenerateSzCheckSum(o_buf, static_cast<uint32_t>(off));
  uint32_t be_cks = gw_message::detail::ByteSwap32(calc_cks);
  memcpy(o_buf + off, &be_cks, 4);

  return msg_len;
}

// ============================================================
// 链接状态通知
// ============================================================

int32 gw_counter_direct::deal_link_connect(int16 link_type, int32 have_switch) {
  (void)have_switch;
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    lb_common::atomic_store16(&trade_link_connect_, 1);
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(get_counter_type(), 0, 1);
    }
  }
  return 0;
}

void gw_counter_direct::deal_link_close(int16 link_type) {
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    lb_common::atomic_store16(&trade_link_connect_, 0);
    lb_common::atomic_store16(&login_state, 0);  // 链接断开重置登录态
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(get_counter_type(), 0, 0);
    }
  }
}

} // namespace lb_api
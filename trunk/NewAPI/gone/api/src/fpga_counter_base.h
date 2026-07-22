// fpga_counter_base - FPGA 柜台公共基类（骨架）
//
// 由 fpga_counter_direct (直连模式, 单客户) 和 fpga_counter_gateway (网关模式,
// 多客户) 继承.
//
// 提供:
//   - 公共协议状态 (fpga_connection_id, fpga_session_id, sec_state_ 等)
//   - 证券代码映射 (sec_map_, secs)
//   - 公共消息构建 (build_login_msg, build_order_msg, build_cancel_msg,
//   build_sec_info_req_msg 等)
//   - 公共消息解析 (deal_sec_info_ans, deal_order_rtn, deal_trade_rtn)
//   - 公共对外 (build_acc_login_msg, build_heart_msg)
//
// 重要：多态通过**模板特化**实现（无 virtual 钩子）
//   - TFastCounter 总是派生类类型（fpga_counter_direct / fpga_counter_gateway）
//   - 派生类的 deal_recv_msg 必须显式调用基类的 protected 方法
//   - 派生类的方法不是"override"，是派生类独有（同名方法被设计用于"显式调用"约定）

#pragma once

#include "api_callback.h"
#include "g1msghead.h"
#include "g1trademsg.h"

#include "api_config_impl.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
// #include "hash_comm.h"
#include "hash_map_mth.h"
#include "mlog.h"
#include "order_trade_type.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

// ============================================================================
// 公共结构体 (FPGA 协议层)
// ============================================================================

/// FPGA 客户信息 (单客户/多客户都用此结构)
struct fpga_cust_info {
  // ---- 热路径: 每次委托/撤单必用 ----
  int32_t fpga_state;  ///< 该客户在fpga中状态
  int16_t login_state; ///< 登陆状态:0-未登陆，1-登陆中，2-已登陆
  uint16_t user_id;    ///< 用户索引ID(登录后分配)
  uint16_t board_no;   ///< 所在FPGA编号(登录后分配)
  uint32_t session_id; ///< fdm board 映射的会话 ID (v2.1: 每客户独立, session_id+board_no 唯一标识 agwuser)
  char order_way_ext[2];        ///< 客户委托方式
  int32_t trade_port;           ///< 交易接口端口(登录后分配)
  char trade_ip[G1_IPADDR_LEN]; ///< 交易接口地址IP(登录后分配)
  // ---- 冷路径: 仅登录/管理使用 ----
  char cust_id[G1_CUSTID_LEN];                ///< 客户号
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; ///< 客户资金账号
  char branch_id[G1_BRANCHID_LEN];            ///< 分支机构代码
  char holder_acc[G1_HOLDERACC_LEN];          ///< 客户股东账号
  int64_t cust_req_no;                        ///< 客户私有请求号
  char end_code[G1_CUST_END_LEN];             //终端信息

  fpga_cust_info() : fpga_state(0), login_state(0), user_id(0), board_no(0), session_id(0) {
    std::memset(end_code, 0, sizeof(end_code));
  }
};

/// FPGA 证券信息
struct fpga_sec_info {
  char security_id[G1_SECURITYID_MAXLEN]; ///< 证券代码
  int16_t market_type;                    ///< 市场
  uint16_t sec_index;                     ///< 证券在 FPGA 内的索引
  int32_t buy_qty_unit;                   ///< 买数量单位
  fpga_sec_info() : market_type(0), sec_index(0) { std::memset(security_id, 0, sizeof(security_id)); }
};

/// FPGA 证券代码 hash 键 (8 字节定长)
struct fpga_sec_key {
  char security_id[G1_SECURITYID_MAXLEN]; ///< 证券代码 (8 字节)

  /// 默认构造
  fpga_sec_key() {}
  /// 拷贝构造 (按 64-bit 块复制, 避免逐字节)
  fpga_sec_key(const fpga_sec_key &src) {
    *reinterpret_cast<uint64_t *>(security_id) = *reinterpret_cast<const uint64_t *>(src.security_id);
  }
  /// 字符串构造
  fpga_sec_key(const char *id) {
    int32_t i = 0;
    while (id[i] != '\0') {
      security_id[i] = id[i];
      i++;
    }
    while (i < G1_SECURITYID_MAXLEN) {
      security_id[i] = '\0';
      i++;
    }
  }
  /// 拷贝赋值
  fpga_sec_key &operator=(const fpga_sec_key &src) {
    *reinterpret_cast<uint64_t *>(security_id) = *reinterpret_cast<const uint64_t *>(src.security_id);
    return *this;
  }
  /// 相等比较 (按 64-bit 块)
  bool operator==(const fpga_sec_key &src) const {
    return *reinterpret_cast<const uint64_t *>(security_id) == *reinterpret_cast<const uint64_t *>(src.security_id);
  }
  /// 析构
  ~fpga_sec_key() {}
};

/// FPGA 证券代码 hash 函数 (按前 8 字节做 64-bit 哈希)
struct fpga_sec_hash {
  uint64_t operator()(const fpga_sec_key &key) const noexcept {
    return *reinterpret_cast<const uint64_t *>(key.security_id);
  }
};

// ============================================================================
// fpga_counter_base - FPGA 柜台公共基类
// ============================================================================

/// FPGA 柜台公共基类
/// (直连/网关两种模式共用此基类, 派生类重写登录/业务发送等钩子)
class fpga_counter_base {
public:
  fpga_counter_base();
  ~fpga_counter_base();

  /// 禁用拷贝
  fpga_counter_base(const fpga_counter_base &) = delete;
  fpga_counter_base &operator=(const fpga_counter_base &) = delete;

  /// 原子读取会话消息序号
  FORCE_INLINE int64 get_session_seq_no() const { return session_seq_; }

  /// engine 调用，处理心跳发送事件
  /// 返回消息长度，含消息头
  int32 build_heart_msg(char *o_buf, int32 buf_len);

protected:
  /// 初始化
  int32 init_base(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);

  /// 查询 sec_index
  /// @param security_id  证券代码 (8 字节定长)
  /// @param sec_index    输出: 该证券在 FPGA 内的索引
  int32 get_sec_index(const char security_id[8], uint16_t &sec_index);

  // ---- 消息构建 (公共协议层) ----
  void build_order_msg(const OrderReq &req, const fpga_cust_info &cust, uint16 sec_index, g1_msg_head *o_req);
  void build_cancel_msg(const CancelReq &req, const fpga_cust_info &cust, g1_msg_head *o_req);

  void deal_order_rtn(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);
  void deal_trade_rtn(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);
  void deal_cancel_rsp(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);

  /// v2.1: GW 链接重连后重新投递指定客户的登录事件
  /// @param cust     已成功登录的客户 (login_state==2)
  /// @param que      GW 链接发送队列 (multi 引擎 send_queue_)
  /// @param link_op  GW 链接 out_op (multi 引擎 link_outop_)
  /// @return 0 成功, 负数 队列满
  int32 delive_cust_login(fpga_cust_info &cust, lb_common::que_mth_buf *que, link_engine_outop *link_op);

  /// 处理网关路由拒绝 (G1_MSG_GW_REJ)
  /// 消息结构: g1_msg_head + g1_gw_rej_head + 被拒绝的请求 (order_req / cancel_req)
  /// 依据 rej_msg_id 分发到 OrderRtn / CancelRsp 回调
  void deal_gw_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);

  void build_api_order_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 err_code, OrderRtn &o_rtn,
                           StreamInfo &o_stream);
  void build_api_cancel_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 err_code, CancelRsp &o_rtn,
                            StreamInfo &o_stream);

  /// 构造 GW 拒单响应 OrderRtn (从 order_req + rej head)
  void build_api_gw_rej_order(const order_req *body, const g1_gw_rej_head *rej, const fpga_cust_info &cust,
                              int32 counter_type, OrderRtn &o_rtn, StreamInfo &o_stream);
  /// 构造 GW 拒撤单响应 CancelRsp (从 cancel_req + rej head)
  void build_api_gw_rej_cancel(const cancel_req *body, const g1_gw_rej_head *rej, const fpga_cust_info &cust,
                               int32 counter_type, CancelRsp &o_rsp, StreamInfo &o_stream);

  void build_sec_info_req_msg(int64 cust_req_no, g1_msg_head *o_req);
  // ---- 基类可默认实现的消息解析 ----
  int32 deal_sec_info_ans(const g1_msg_head *msg);

  void build_login_msg(const acc_login_event_info &info, int16_t log_type, g1_msg_head *o_req);
  void build_login_rtn(const acc_login_event_info &info, int32 err_ret, const char *err_msg, LoginAns &ans);
  void build_login_rtn(const login_ans &msg, LoginAns &o_ans);
  void build_login_rtn(const fpga_cust_info &cust, int32 err_ret, const char *err_msg, LoginAns &o_ans);
  void save_client_info(const login_ans &msg, fpga_cust_info &o_cust);
  void build_login_event(fpga_cust_info &cust, acc_login_event_info &o_info);

  // ---- FPGA 协议标识 ----
  // 注: session_id 移至 fpga_cust_info (v2.1: per-customer, 一个 agwuser 在不同 fdm board 产生不同 session_id)
  int32 trade_link_connect = 0; ///< 交易通道是否链接
  int32 sec_state_ = 0;         ///< 证券信息获取状态:0-未进行，1-进行中，2-成功
  int16 market_type = 0;        ///< 市场
  int16 heart_interval = 5;     ///< 心跳间隔

  // ---- 证券代码映射 ----
  lb_common::hash_map_mth<1020, fpga_sec_key, uint16_t, fpga_sec_hash> sec_map_; ///< 证券代码 -> sec_index
  std::vector<fpga_sec_info> secs_;    ///< 证券信息数组 (按 sec_index 索引)
  callback_manager *cb_mgr_ = nullptr; ///< 回调管理器
  int64 session_seq_ = 0;              ///< 会话消息序号 (原子访问)
  lb_common::lb_log *log_ = nullptr;   ///< 日志指针
  char agw_user[32];
};

} // namespace lb_api
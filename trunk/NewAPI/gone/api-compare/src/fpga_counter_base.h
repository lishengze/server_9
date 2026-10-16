// fpga_counter_base - FPGA 柜台公共基类
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
#include "matomic.h"
#include "hash_map_mth.h"
#include "mlog.h"
#include "order_trade_type.h"

#include <cstdint>
#include <cstring>
#include <vector>

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

  /// @brief 最大支持的证券信息数量
  static constexpr int32_t FPGA_MAX_SEC_NUM = 16384;

  // ============================================================================
  // 公共结构体 (FPGA 协议层)
  // ============================================================================

  /// FPGA 客户信息 (单客户/多客户都用此结构)
  struct fpga_cust_info
  {
    // ---- 热路径: 每次委托/撤单必用 ----
    log_state_ctl log_state;          ///< 登录控制状态机
    int32_t fpga_state;               ///< 该客户在fpga中状态,1-正常，2-故障，3-故障切换柜台 (独立故障维度, 不触碰登录状态; 2/3 均为不可用态)
    uint16_t user_id;                 ///< 用户索引ID(登录后分配)
    uint16_t board_no;                ///< 所在FPGA编号(登录后分配)
    uint32_t session_id;              ///< fdm board 映射的会话 ID (每客户独立, session_id+board_no 唯一标识 agwuser)
    char order_way_ext[2];            ///< 客户委托方式
    int32_t trade_port;               ///< 交易接口端口(登录后分配)
    char trade_ip[INNER_IP_ADDR_LEN]; ///< 交易接口地址IP(登录后分配)
    // ---- 冷路径: 仅登录/管理使用 ----
    char cust_id[INNER_CUST_ID_LEN];                 ///< 客户号
    char fund_account_id[INNER_FUND_ACCOUNT_ID_LEN]; ///< 客户资金账号
    char branch_id[INNER_BRANCH_ID_LEN];             ///< 分支机构代码
    char holder_acc[INNER_ACCOUNT_ID_LEN];           ///< 客户股东账号
    int64_t cust_req_no;                             ///< 客户私有请求号
    char end_code[INNER_FEATURE_CODE_LEN];           // 终端信息
  };

  /// FPGA 证券信息
  struct fpga_sec_info
  {
    char security_id[INNER_SECURITY_ID_LEN]; ///< 证券代码
    int16_t market_type;                     ///< 市场
    uint16_t sec_index;                      ///< 证券在 FPGA 内的索引
    int32_t buy_qty_unit;                    ///< 买数量单位
    fpga_sec_info() : market_type(0), sec_index(0) { std::memset(security_id, 0, sizeof(security_id)); }
  };

  /// FPGA 证券代码 hash 键 (8 字节定长)
  struct fpga_sec_key
  {
    char security_id[INNER_SECURITY_ID_LEN]; ///< 证券代码 (8 字节)

    /// 默认构造 (全零初始化: operator==/hash 按全宽 8 字节读取, 不清零是未定义行为)
    fpga_sec_key()
    {
      std::memset(security_id, 0, sizeof(security_id));
    }
    /// 拷贝构造 (按 64-bit 块复制, 避免逐字节)
    fpga_sec_key(const fpga_sec_key &src)
    {
      *reinterpret_cast<uint64_t *>(security_id) = *reinterpret_cast<const uint64_t *>(src.security_id);
    }
    /// 字符串构造
    fpga_sec_key(const char *id)
    {
      int32_t i = 0;
      while (i < INNER_SECURITY_ID_LEN && id[i] != '\0')
      {
        security_id[i] = id[i];
        i++;
      }
      while (i < INNER_SECURITY_ID_LEN)
      {
        security_id[i] = '\0';
        i++;
      }
    }
    /// 拷贝赋值
    fpga_sec_key &operator=(const fpga_sec_key &src)
    {
      *reinterpret_cast<uint64_t *>(security_id) = *reinterpret_cast<const uint64_t *>(src.security_id);
      return *this;
    }
    /// 相等比较 (按 64-bit 块)
    bool operator==(const fpga_sec_key &src) const
    {
      return *reinterpret_cast<const uint64_t *>(security_id) == *reinterpret_cast<const uint64_t *>(src.security_id);
    }
    /// 析构
    ~fpga_sec_key() {}
  };

  /// FPGA 证券代码 hash 函数 (按前 8 字节做 64-bit 哈希)
  struct fpga_sec_hash
  {
    uint64_t operator()(const fpga_sec_key &key) const noexcept
    {
      return *reinterpret_cast<const uint64_t *>(key.security_id);
    }
  };

  // ============================================================================
  // fpga_counter_base - FPGA 柜台公共基类
  // ============================================================================

  /// FPGA 柜台公共基类
  /// (直连/网关两种模式共用此基类, 派生类重写登录/业务发送等钩子)
  class fpga_counter_base
  {
  public:
    fpga_counter_base();
    ~fpga_counter_base();

    /// 禁用拷贝
    fpga_counter_base(const fpga_counter_base &) = delete;
    fpga_counter_base &operator=(const fpga_counter_base &) = delete;

    /// engine 调用，处理心跳发送事件
    /// 返回消息长度，含消息头
    int32 build_heart_msg(char *o_buf, int32 buf_len);

  protected:
    /// 初始化
    int32 init_base(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);

    /// 查询 sec_index
    /// @param security_id  证券代码 (8 字节定长)
    /// @param sec_index    输出: 该证券在 FPGA 内的索引
    int32 get_sec_index(const char security_id[INNER_SECURITY_ID_LEN], uint16_t &sec_index);

    /// FPGA 柜台支持域二道校验 (RUL-19/21): 仅买卖方向(spot 集含回购, FPGA 拒)。
    /// 防线依赖: 市场(==实例,RUL-20)与限价(RUL-21)已由 api_impl 一道校验保证——
    /// 若一道校验移除/放宽, 此处将静默失防线 (审查 round8 建议, 变更一道时须复核此处)
    FORCE_INLINE int32 check_order_dict(const OrderReq &req)
    {
      if (req.side != ORDER_SIDE_BUY && req.side != ORDER_SIDE_SELL)
      {
        return LBAPI_ERR_INVALID_PARAM;
      }
      /// 数量: API 放大100(股数×100, RUL-35), g1 报文整型股 —— 非 100 整数倍无法
      /// 精确映射, 本地拒绝 (98/个微报文本身 ×100, 非整除值直传由柜台裁决)
      if (req.order_qty % 100 != 0)
      {
        return LBAPI_ERR_INVALID_PARAM;
      }
      return 0;
    }

    /// FPGA 柜台撤单支持域校验 (RUL-36): FPGA 后台仅支持系统委托号撤单
    /// (g1 cancel_req.order_sys_no), 不支持客户私有号(org_cust_req_no 不构成
    /// 定位键) —— 撤单请求 order_sys_no≤0(非正数即非法: 0=仅私有号/双零, 双零
    /// 已被 api_impl 一道 B5 拒绝)本地拒绝参数错误, 不发单不降级 (INVALID_PARAM 非降级码)
    FORCE_INLINE int32 check_cancel_dict(const CancelReq &req)
    {
      if (req.order_sys_no <= 0)
      {
        return LBAPI_ERR_INVALID_PARAM;
      }
      return 0;
    }

    // ---- 消息构建 (公共协议层) ----
    void build_order_msg(const OrderReq &req, const fpga_cust_info &cust, uint16 sec_index, g1_msg_head *o_req);
    void build_cancel_msg(const CancelReq &req, const fpga_cust_info &cust, g1_msg_head *o_req);

    void deal_order_rtn(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);
    void deal_trade_rtn(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);
    void deal_cancel_rsp(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);

    /// 将已构造好的登录事件投递到 GW 队列 (delive_cust_login 的底层实现,
    /// 亦用于"未成功登录过"的未决登录请求在 GW 重连后的原样重投)
    int32 delive_login_event(const acc_login_event_info &info, lb_common::que_mth_buf *que,
                             link_engine_outop *link_op);

    /// 处理网关路由拒绝 (G1_MSG_GW_REJ)
    /// 消息结构: g1_msg_head + g1_gw_rej_head + 被拒绝的请求 (order_req / cancel_req)
    /// 依据 rej_msg_id 分发到 OrderRtn / CancelRsp 回调
    void deal_gw_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type);

    void build_api_order_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type, int32 err_code,
                             OrderRtn &o_rtn,
                             StreamInfo &o_stream);
    void build_api_cancel_rej(const g1_msg_head *msg, const fpga_cust_info &cust, int32 counter_type, int32 err_code,
                              CancelRsp &o_rtn,
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
    int32 trade_link_connect = 0;         ///< 交易通道是否链接
    int32 sec_state_ = 0;                 ///< 证券信息获取状态:0-未进行，1-进行中，2-成功
    int16 market_type = 0;                ///< 市场
    int32 heart_interval = 5;             ///< 心跳间隔
    char agw_session_[INNER_SESSION_LEN]; ///< agw 会话号 (首登时从登录事件缓存, GW 重连重登时复用, 与 counter98 路径同源)

    // ---- 证券代码映射 ----
    lb_common::hash_map_mth<1020, fpga_sec_key, uint16_t, fpga_sec_hash> sec_map_; ///< 证券代码 -> sec_index
    std::vector<fpga_sec_info> secs_;                                              ///< 证券信息数组 (按 sec_index 索引)
    callback_manager *cb_mgr_ = nullptr;                                           ///< 回调管理器
    int64 session_seq_ = 0;                                                        ///< 会话消息序号 (原子访问)
    lb_common::lb_log *log_ = nullptr;                                             ///< 日志指针
  };

} // namespace lb_api
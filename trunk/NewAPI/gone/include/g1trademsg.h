#ifndef G1TRADEMSG_H
#define G1TRADEMSG_H
#include "g1msghead.h"

// api 请求消息
#define G1_MSG_ORDER_REQ 1001u    ///< 委托请求
#define G1_MSG_CANCEL_REQ 1002u   ///< 撤单请求
#define G1_MSG_HEART_REQ 1003u    ///< 心跳请求（fwm↔fdm 管理链路形态：体=8字节管理层头、msg_len=8、board/user=0、req_no 回带；与 fgw/api 交易链路纯头帧 msg_len=0 形态不同——联调契约见 doc/SYSTEM_CONTRACT.md §7）
#define G1_MSG_SEC_INFO_REQ 1004u ///< 证券信息请求
#define G1_MSG_LOGIN_REQ 1005u    ///< 登录请求
// 后台应答推送消息
#define G1_MSG_ORDER_RTN 2001u    ///< 委托回报
#define G1_MSG_TRADE_RTN 2002u    ///< 成交推送
#define G1_MSG_CANCEL_RSP 2003u   ///< 撤单响应
#define G1_MSG_HEART_ANS 2004u    ///< 心跳响应
#define G1_MSG_SEC_INFO_ANS 2005u ///< 证券信息推送
#define G1_MSG_LOGIN_ANS 2006u    ///< 登录响应
#define G1_MSG_OFFLINE_PUSH 2007u ///< 板卡客户状态推送
#define G1_MSG_GW_REJ 2008u       ///< 网关路由拒绝

// 登录消息
struct login_req
{
  int64_t cust_req_no;                        ///< 客户私有请求号
  char cust_id[G1_CUSTID_LEN];                // 客户号
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号，前16字节有效
  char branch_id[G1_BRANCHID_LEN];            // 分支机构,前4字节有效
  char holder_acc[G1_HOLDERACC_LEN];          // 股东账户
  char session[G1_SESSION_LEN];               // 会话信息
  char end_code[G1_CUST_END_LEN];             // 终端信息
  int16_t market_type;                        // 市场
  char order_way[G1_ORDERWAY_LEN];            // 委托通道类型
  int32_t log_type;                           // 1-用户，2-网关
  int32_t heart_bt_int;                       // 心跳时间
  int32_t req_connect_id;                     // 网关填写，api 设为0
  char version[G1_VERSION_LEN];               // 版本号
};

struct login_ans
{
  int64_t cust_req_no;                        ///< 客户私有请求号
  char cust_id[G1_CUSTID_LEN];                // 客户号
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号，前16字节有效
  char branch_id[G1_BRANCHID_LEN];            // 分支机构,前4字节有效
  char holder_acc[G1_HOLDERACC_LEN];          // 股东账户
  char session[G1_SESSION_LEN];               // 会话信息
  char end_code[G1_CUST_END_LEN];             // 终端信息
  uint16_t user_id;                           // 用户ID索引
  uint16_t board_no;                          // 板卡号
  int16_t proto_type;                         // 协议，1-TCP,2-udp；目前固定1
  char order_way[G1_ORDERWAY_LEN];            // 委托通道类型
  int32_t req_connect_id;                     // 网关填写，fdm 带回
  int32_t log_type;                           // 1-用户，2-网关
  uint32_t session_id;                        // 会话号
  int32_t trade_port;                         // 交易端口
  char trade_ip[G1_IPADDR_LEN];               // 交易IP
  int32_t err_code;                           // 错误码
  int32_t reserved;                           // 保留填充
  char err_msg[G1_ERRMSG_LEN];                // 错误信息
  char version[G1_VERSION_LEN];               // 后台版本号
  int64_t login_time;                         // 登录时间，HHMMSSmmm (与 API LoginAns.login_time 同口径)
};

// 获取证券信息请求
struct sec_info_req
{
  int64_t cust_req_no; ///< 客户私有请求号
};

struct sec_push_head
{
  int64_t cust_req_no; ///< 客户私有请求号
  uint16_t total_num;  ///< 剩余总数量
  uint16_t cur_num;    ///< 本次返回数量，若=剩余总数量，则完成
  int32_t err_code;
};

struct sec_push_info
{
  char security_id[G1_SECURITYID_MAXLEN]; // 证券代码
  int16_t market_type;                    // 市场
  uint16_t sec_index;                     // 证券索引
  int32_t buy_qty_unit;                   // 买数量交易单位，不放大
  int32_t sell_qty_unit;                  // 卖数量交易单位，不放大
  int32_t reserved;                       // 填充未使用
  int64_t price_unit;                     // 价格变动单位，放大10000
};

struct sec_info_ans
{
  sec_push_head head;
  sec_push_info info[];
};

// 板卡客户状态消息
struct fpga_user_state
{
  int32_t cur_time;                           // 通知时间，HHMMSS (comm_utils::get_time 压缩码)
  uint16_t board_no;                          // 板卡号
  int16_t market_type;                        // 市场
  uint16_t user_id;                           // 用户索引
  int16_t state;                              // 状态，1-正常，2-故障，3-故障切换柜台
  char branch_id[G1_BRANCHID_LEN];            // 分支机构,前4字节有效
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号，前16字节有效
};

struct order_req
{
  uint16_t user_id;                       // 用户索引
  uint16_t board_no;                      // 板卡号
  uint16_t sec_index;                     // 证券索引
  char side;                              // 买卖方向 (G1_ORDER_SIDE_*, '1'买/'2'卖, =98 ATP AgwSide)
  char order_type;                        // 委托类型 ('1'市价/'2'限价, API 字典=98 ATP AgwOrdType)
  char security_id[G1_SECURITYID_MAXLEN]; // 证券代码
  int64_t order_price;                    // 委托价格，放大10000
  int64_t order_qty;                      // 委托数量，不放大100
  int64_t cust_req_no;                    ///< 客户私有请求号
  int64_t stop_price;                     // 止损价
  uint16_t tgw_id;                        // TGW编号
  uint16_t policy_id;                     // 策略佣金ID
  char order_way[2];                      // 委托方式
  int16_t reserve;                        // 预留
};

// ---- g1 字典常量 (与 API 字典同源, 均以 98 ATP Agw 字典为准; fgw 产出侧须遵守, RUL-16/18) ----

// 委托状态 (AgwOrdStatus, uint8_t, 完整字典; 与 order_trade_type.h ORDER_STATE_* 同值)
static constexpr uint8_t G1_ORDER_STATE_NEW = 0;                      // 已申报
static constexpr uint8_t G1_ORDER_STATE_PART_FILLED = 1;              // 部分成交
static constexpr uint8_t G1_ORDER_STATE_FILLED = 2;                   // 全部成交
static constexpr uint8_t G1_ORDER_STATE_PART_FILLED_PART_CANCEL = 3;  // 部成部撤
static constexpr uint8_t G1_ORDER_STATE_CANCELED = 4;                 // 已撤销
static constexpr uint8_t G1_ORDER_STATE_PART_CANCELED = 5;            // 部分撤销
static constexpr uint8_t G1_ORDER_STATE_REJECTED = 8;                 // 已拒绝
static constexpr uint8_t G1_ORDER_STATE_UNSENT = 9;                   // 未申报
static constexpr uint8_t G1_ORDER_STATE_SENDING = 10;                 // 正报
static constexpr uint8_t G1_ORDER_STATE_WAIT_CANCEL = 11;             // 待撤销
static constexpr uint8_t G1_ORDER_STATE_PART_FILLED_WAIT_CANCEL = 12; // 部成待撤

// 回报类型 (AgwExecType 字符值, char; 与 order_trade_type.h RSP_TYPE_* 同源同型)
static constexpr char G1_RSP_TYPE_NEW = '0';      // 新订单 (48)
static constexpr char G1_RSP_TYPE_INTERNAL = '1'; // 内部执行 (49)
static constexpr char G1_RSP_TYPE_CANCELED = '4'; // 已撤销 (52)
static constexpr char G1_RSP_TYPE_REJECTED = '8'; // 已拒绝 (56)
static constexpr char G1_RSP_TYPE_TRADE = 'F';    // 已成交 (70)
// 买卖方向 (AgwSide, g1 仅用买/卖)
static constexpr char G1_ORDER_SIDE_BUY = '1';  // 买
static constexpr char G1_ORDER_SIDE_SELL = '2'; // 卖

struct order_rtn
{
  uint16_t user_id;                       // 用户索引
  uint16_t board_no;                      // 板卡号
  uint16_t sec_index;                     // 证券索引
  char side;                              // 买卖方向 (G1_ORDER_SIDE_*, '1'买/'2'卖, =98 ATP AgwSide)
  char order_type;                        // 委托类型 ('1'市价/'2'限价, API 字典=98 ATP AgwOrdType)
  char security_id[G1_SECURITYID_MAXLEN]; // 证券代码
  int64_t order_price;                    // 放大10000
  int64_t order_qty;                      // 委托数量, 整型股不放大
  int64_t cust_req_no;                    ///< 客户私有请求号

  uint8_t order_status; // 委托状态 (G1_ORDER_STATE_*, =98 ATP AgwOrdStatus)
  char rtn_type;        // 回报类型 (G1_RSP_TYPE_*, AgwExecType 字符值)
  uint16_t policy_id;   // 策略佣金ID，fpga不用存
  int32_t err_code;     // 错误码

  int64_t order_sys_no;   // 柜台委托号
  int64_t frozen_amount;  // 冻结金额
  int64_t trade_amount;   // 累计成交金额
  int64_t trade_qty;      // 累计成交数量
  int64_t fee;            // 累计费用（含冻结）
  int64_t cancel_qty;     // 撤单数量
  int64_t order_time;     // 委托时间，HHMMSSmmm，看交易所什么类型，取交易所时间
  int64_t update_time;    // 更新时间，FPGA不用存，取交易所时间
  int64_t session_seq_no; ///< 后台会话可靠消息流水号(0-非可靠消息)
};

struct cancel_req
{
  int64_t cust_req_no;     ///< 客户私有请求号
  int64_t order_sys_no;    // 柜台原始报单编号
  uint16_t user_id;        // 用户索引
  uint16_t board_no;       // 板卡号
  int32_t reserve;         // 预留
  int64_t org_cust_req_no; ///< 原客户私有请求号(被撤委托的)
};

// 一般用于撤单废单响应，如中间路径出错返回
struct cancel_rsp
{
  int64_t cust_req_no;     ///< 客户私有请求号
  int64_t order_sys_no;    // 柜台原始报单编号
  uint16_t user_id;        // 用户索引
  uint16_t board_no;       // 板卡号
  int32_t err_code;        // 错误代码
  int64_t org_cust_req_no; ///< 原客户私有请求号(被撤委托的)
  int64_t session_seq_no;  ///< 后台会话可靠消息流水号(0-非可靠消息)
};

struct trade_rtn
{
  // 契约 (RUL-23): 本消息仅承载成交 (G1_RSP_TYPE_TRADE 'F'); 成交回报通道的其他
  // 事件类型(委托确认/撤单成交/委托废单)须由 fgw 产出侧转 order_rtn 通道下发 ——
  // 此为对 fgw 产出的契约要求 (尚未实施, 见 SYSTEM_CONTRACT §7 联调项);
  // api 消费侧 (fpga_counter_base::deal_trade_rtn) 按本契约固定填 'F'
  uint16_t user_id;                       // 用户索引
  uint16_t board_no;                      // 板卡号
  uint16_t sec_index;                     // 证券索引
  char side;                              // 买卖方向 (G1_ORDER_SIDE_*, '1'买/'2'卖, =98 ATP AgwSide)
  char order_type;                        // 委托类型 ('1'市价/'2'限价, API 字典=98 ATP AgwOrdType)
  char security_id[G1_SECURITYID_MAXLEN]; // 证券代码
  int64_t order_price;                    // 放大10000
  int64_t order_qty;                      // 委托数量, 整型股不放大
  int64_t cust_req_no;                    // 客户私有请求号
  uint8_t order_status;                   // 委托状态 (G1_ORDER_STATE_*, =98 ATP AgwOrdStatus)
  char rtn_type;                          // 回报类型 (G1_RSP_TYPE_*, AgwExecType 字符值)
  uint16_t policy_id;                     // 策略佣金ID，fpga不用存
  int32_t err_code;                       // 错误码

  int64_t order_sys_no;  // 柜台委托号
  int64_t frozen_amount; // 冻结金额
  int64_t trade_amount;  // 累计成交金额
  int64_t trade_qty;     // 累计成交数量
  int64_t cancel_qty;    // 撤单成交数量
  int64_t fee;           // 累计费用（含冻结）
  int64_t order_time;    // 委托时间，HHMMSSmmm，看交易所什么类型，取交易所时间
  int64_t exec_time;     // 成交时间，FPGA不用存，取交易所时间

  // 成交信息
  char exec_id[G1_EXECID_LEN]; // 交易所成交编号
  int64_t exec_price;          // 成交价格
  int64_t exec_qty;            // 成交数量
  int64_t exec_amount;         // 成交金额
  int64_t exec_fee;            // 单笔成交费用

  int64_t session_seq_no; ///< 后台会话可靠消息流水号(0-非可靠消息)
};

// 网关路由拒绝消息扩展头
struct g1_gw_rej_head
{
  uint32_t rej_msg_id;         // 被拒绝的消息号
  int32_t err_code;            // 拒绝错误码
  char err_msg[G1_ERRMSG_LEN]; // 错误信息
};

#endif // G1TRADEMSG_H

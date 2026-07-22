#pragma once

#include <array>
#include <stdint.h>

namespace lb_api {
/// 登录请求
class LoginReq {
public:
  int64_t client_req_no;                      ///< 客户请求号
  std::array<char, 16> cust_id;               ///< 客户号
  std::array<char, 16> fund_account_id;       ///< 客户资金账号
  std::array<char, 12> account_id;            ///< 客户股东账号
  std::array<char, 10> branch_id;             ///< 分支机构代码
  std::array<char, 2> order_way_ext;          ///< 客户委托方式
  std::array<char, 256> password;             ///< 密码
  std::array<char, 64> user_info;             ///< 用户私有信息
  std::array<char, 1024> client_feature_code; ///< 客户终端信息
};

/// 登录应答
class LoginAns {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 12> account_id;      ///< 客户股东账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int16_t market_type;                  ///< 市场
  int32_t err_code;                     ///< 错误码
  std::array<char, 124> err_msg;        ///< 错误信息
  int64_t login_time;                   ///< 登陆时间，HHMMSSmmm
};

/// 委托请求
class OrderReq {
public:
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  char side;                            ///< 买卖方向
  char order_type;                      ///< 市价限价标记
  uint16_t policy_id;                   ///< 策略佣金ID
  uint16_t tgw_id;                      ///< TGW编号
  std::array<char, 8> security_id;      ///< 证券代码
  int64_t order_price;                  ///< 委托价格，放大10000
  int64_t order_qty;                    ///< 委托数量，不放大100
  int64_t stop_price;                   ///< 止损价
  int64_t client_seq_id;                ///< 用户私有报单号
};

/// 撤单请求
class CancelReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int64_t order_sys_no;                 ///< 柜台原始报单编号
  int64_t client_seq_id;                ///< 用户私有报单号，预留，暂不支持
};

/// 撤单失败响应
class CancelRsp {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 12> account_id;      ///< 客户股东账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int16_t market_type;                  ///< 市场
  int64_t order_sys_no;                 ///< 柜台原始报单编号
  int64_t client_seq_id;                ///< 用户私有报单号，预留，暂不支持
  int32_t err_code;                     ///< 撤单失败错误码
  int32_t rej_api;                      ///< 是否为api错误
};

// todo : 待对齐原有字典修正
/** @name 订单状态定义
 *  @brief 订单空闲 */
#define ORDER_STATE_ORDER_IDLE 0
/** @brief 订单待报 */
#define ORDER_STATE_ORDER_PEND 1
/** @brief 订单已报 */
#define ORDER_STATE_ORDER_NEW 2
/** @brief 订单部分成交 */
#define ORDER_STATE_DONE_PART 3
/** @brief 订单全部成交 */
#define ORDER_STATE_DONE_FULL 4
/** @brief 已报待撤 */
#define ORDER_STATE_CANCEL_ING 5
/** @brief 部成待撤 */
#define ORDER_STATE_PART_CANCEL_ING 6
/** @brief 全部撤销 */
#define ORDER_STATE_CANCEL_ALL 7
/** @brief 部成部撤 */
#define ORDER_STATE_PART_CANCEL 8
/** @brief 内部废单 */
#define ORDER_STATE_DISCARD 9
/** @brief 路经拒绝 */
//#define ORDER_STATE_REJECTED 10
/** @} */

// todo : 待对齐原有字典修正
/** @name 委托回报类型定义 */
/** @brief 柜台响应 */
#define RSP_TYPE_COUNTER_RSP 1
/** @brief 交易所响应 */
#define RSP_TYPE_EXCHANGE_RSP 2
/** @brief 委托成交 */
#define RSP_TYPE_ORDER_TRADE 3
/** @brief 委托废单 */
#define RSP_TYPE_ORDER_DISCARD 4
/** @brief 撤单响应 */
#define RSP_TYPE_CANCEL_RSP 5
/** @brief 撤单回报 */
#define RSP_TYPE_CANCEL_TRADE 6

/// 委托回报
class OrderRtn {
public:
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 12> account_id;      ///< 客户股东账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  char side;                            ///< 买卖方向
  char order_type;                      ///< 市价限价标记
  uint16_t order_status;                ///< 委托状态
  uint16_t policy_id;                   ///< 策略佣金ID
  int16_t market_type;                  ///< 市场
  int16_t reserved;                     ///< 保留
  std::array<char, 8> security_id;      ///< 证券代码
  int64_t order_price;                  ///< 委托价格，放大10000
  int64_t order_qty;                    ///< 委托数量，不放大100
  int64_t client_seq_id;                ///< 用户私有报单号
  int32_t rtn_type;                     ///< 回报类型
  int32_t err_code;                     ///< 错误码
  int64_t order_sys_no;                 ///< 柜台委托号
  int64_t frozen_amount;                ///< 冻结金额
  int64_t fee;                          ///< 累计费用（含冻结）
  //int64_t trade_amount;                 ///< 累计成交金额
  int64_t trade_qty;   ///< 累计成交数量
  int64_t cancel_qty;  ///< 撤单成交数量
  int64_t order_time;  ///< 委托时间，HHMMSSmmm，取交易所时间
  int64_t update_time; ///< 更新时间，HHMMSSmmm，取交易所时间
};

/// 成交回报
class TradeRtn {
public:
  // 委托信息
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 12> account_id;      ///< 客户股东账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  char side;                            ///< 买卖方向
  char order_type;                      ///< 市价限价标记
  uint16_t order_status;                ///< 委托状态
  uint16_t policy_id;                   ///< 策略佣金ID
  int16_t market_type;                  ///< 市场
  int16_t reserved;                     ///< 保留
  std::array<char, 8> security_id;      ///< 证券代码
  int64_t order_price;                  ///< 委托价格，放大10000
  int64_t order_qty;                    ///< 委托数量，不放大100
  int64_t client_seq_id;                ///< 用户私有报单号
  int64_t order_sys_no;                 ///< 柜台委托号
  int64_t frozen_amount;                ///< 冻结金额
  int64_t fee;                          ///< 累计费用（含冻结）
  //int64_t trade_amount;                 ///< 累计成交金额
  int64_t trade_qty;  ///< 累计成交数量
  int64_t cancel_qty; ///< 撤单成交数量
  int64_t order_time; ///< 委托时间，HHMMSSmmm，取交易所时间
  // 成交信息
  int64_t exec_time;            ///< 成交时间，HHMMSSmmm，取交易所时间
  std::array<char, 32> exec_id; ///< 柜台成交编号
  int64_t exec_price;           ///< 成交价格
  int64_t exec_qty;             ///< 成交数量
  int64_t exec_amount;          ///< 成交金额
  int64_t exec_fee;             ///< 单笔成交费用
};

/// 成交信息(成交查询应答单条)
class TradeInfo {
public:
  // 委托信息
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 12> account_id;      ///< 客户股东账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  char side;                            ///< 买卖方向
  char order_type;                      ///< 市价限价标记
  int16_t market_type;                  ///< 市场
  int32_t reserved;                     ///< 保留
  std::array<char, 8> security_id;      ///< 证券代码
  int64_t client_seq_id;                ///< 用户私有报单号
  int64_t order_sys_no;                 ///< 柜台委托号
  // 成交信息
  int64_t exec_time;   ///< 成交时间，HHMMSSmmm，取交易所时间
  char exec_id[16];    ///< 柜台成交编号
  int64_t exec_price;  ///< 成交价格
  int64_t exec_qty;    ///< 成交数量
  int64_t exec_amount; ///< 成交金额
  int64_t exec_fee;    ///< 单笔成交费用
};

/// 客户委托查询请求
class OrderQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int64_t order_sys_no;                 ///< 柜台委托号
  int64_t user_seq_id;                  ///< 用户私有报单号
};

/// 客户委托批量查询请求
class OrderBatchQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int64_t order_sys_no_begin;           ///< 柜台委托号起始号(填0表示所有)
  int64_t order_sys_no_end;             ///< 柜台委托号结束(填0表示大于起始的所有)
};

/// 客户成交查询请求
class TradeQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int64_t order_sys_no;                 ///< 柜台委托号
  int64_t user_seq_id;                  ///< 用户私有报单号
};

/// 客户成交批量查询请求
class TradeBatchQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int64_t exec_id_begin;                ///< 柜台成交起始号(填0表示所有)
  int64_t exec_id_end;                  ///< 柜台成交号结束(填0表示大于起始的所有)
};

/// 客户资金查询请求
class FundQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
};

/// 客户持仓查询请求
class PositionQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 8> security_id;      ///< 证券代码(填空表示所有)
};

/// 客户资金信息(资金查询应答单条)
class CustFundInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int16_t market_type;                  ///< 市场
  int32_t reserved;                     ///< 保留
  int64_t avail_amount;                 ///< 可用资金
  int64_t token_amount;                 ///< 可取资金
};

/// 客户持仓信息(持仓查询应答单条)
class CustPositionInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 12> account_id;      ///< 客户股东账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int16_t market_type;                  ///< 市场
  std::array<char, 8> security_id;      ///< 证券代码
  int64_t avail_qty;                    ///< 可用数量
  int64_t buy_done_qty;                 ///< 买入成交数量
  int64_t sell_done_qty;                ///< 卖出成交数量
};

} // namespace lb_api
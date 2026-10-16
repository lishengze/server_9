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
  uint16_t market_type;                  ///< 市场ID

  // ---- 性能测试临时字段（Task: 性能测试, 单位纳秒, mutable 以便 api 内部在 const 引用上记录）----
  mutable uint64_t api_arrive_time_ns;  ///< 请求到达 api 的时间（单调时钟纳秒）
  mutable uint64_t api_leave_time_ns;   ///< 请求离开 api 的时间（单调时钟纳秒）
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
/** @brief 发送失败 */
#define RSP_TYPE_SEND_ERROR 7

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

/// 分页查询控制
struct PageQueryCtrl {
  int64_t query_index; ///< 起始查询索引号，填0表示从头开始查
  int64_t return_num;  ///< 指定查询返回数量，填0表示不指定
};

/// 客户持仓查询请求
class PositionQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int16_t market_type;                  ///< 证券所属市场,填0查所有
  std::array<char, 8> security_id;      ///< 证券代码(填空表示所有)
  PageQueryCtrl page_ctrl;              ///< 分页控制
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
  std::array<char, 12> account_id;      ///< 客户股东账号(证券账户)
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int16_t market_type;                  ///< 市场
  std::array<char, 8> security_id;      ///< 证券代码
  // int64_t avail_qty;                    ///< 可用数量
  // int64_t buy_done_qty;                 ///< 买入成交数量
  // int64_t sell_done_qty;                ///< 卖出成交数量
  std::array<char, 40> security_symbol; ///< 证券简称
  uint16_t security_type;               ///< 证券类别
  std::array<char, 4> currency;         ///< 货币种类
  std::array<char, 6> pbu_id;           ///< 席位代码
  int64_t stk_bal;                      ///< 股份余额
  int64_t stk_avl;                      ///< 股份可用
  int64_t stk_can_trans;                ///< 股份可转
  int64_t stk_frz;                      ///< 股份冻结
  int64_t leaves_qty;                   ///< 市值持仓
  int64_t market_value;                 ///< 市值
  int64_t cost_price;                   ///< 成本价格
  int64_t profit_cost;                  ///< 盈亏成本
  int64_t pro_income;                   ///< 累计浮动盈亏
  int64_t income;                       ///< 买入浮动盈亏
  int64_t profit_rate;                  ///< 浮亏比例
  int64_t limit_up;                     ///< 涨停价格
  int64_t limit_down;                   ///< 跌停价格
};

/// 最大可委托数查询请求
class MaxOrderQtyQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  int16_t market_type;                  ///< 证券所属市场
  std::array<char, 8> security_id;      ///< 证券代码
  uint8_t business_type;                ///< 业务类型
  char side;                            ///< 买卖方向
  int64_t price;                        ///< 委托价格
  std::array<char, 12> sh_account_id;   ///< 沪A股东代码
  std::array<char, 12> sz_account_id;   ///< 深A股东代码
};

/// 最大可委托数查询应答(单条)
class CustMaxOrderQtyInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 12> account_id;      ///< 账户ID
  int64_t max_order_qty;                ///< 最大可委托数
  int64_t theoretical_order_qty;        ///< 理论可委托数
  int64_t odd_order_qty;                ///< 港股通零股卖出最大可委托数
  uint8_t limit_flag;                   ///< 减持受限标识
  int64_t jj_max_limit_qty;             ///< 减持受限名单内的竞价卖出可用额度
};

/// 客户总资产查询请求
class TotalFundAssetQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
};

/// 可取资金查询请求
class FundMaxdrawQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 4> currency;         ///< 货币种类
};

/// 客户总资产查询应答(单条)
class CustTotalFundAssetInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 4> currency;         ///< 币种
  int64_t fund_bal;                     ///< 资金余额
  int64_t fund_avl;                     ///< 资金可用
  int64_t fund_asset_total_new;         ///< 总资产新
  int64_t fund_asset_total;             ///< 总资产
  int64_t fund_asset;                   ///< 资金资产
  int64_t fund_asset_ggt;               ///< 港股通T+1日未交收资金
  int64_t fund_asset_stktotal;          ///< 证券总资产
  int64_t fund_asset_stk;               ///< 证券资产
  int64_t fund_asset_bond_total;        ///< 债券总资产
  int64_t fund_asset_bond;              ///< 债券资产
  int64_t fund_asset_rzgh;              ///< 国债正回购
  int64_t fund_asset_rqgh;              ///< 国债逆回购
  int64_t fund_asset_stkzy;             ///< 债券质押资产（回购资产）
  int64_t fund_asset_rzcs;              ///< 协议融资
  int64_t fund_asset_rqcs;              ///< 协议融券
  int64_t fund_asset_stkxyzy;           ///< 协议债券质押资产
  int64_t fund_asset_adjust;            ///< 待交收证券资产
  int64_t fund_asset_oftotal;           ///< 理财总资产
  int64_t fund_asset_of;                ///< 基金资产
  int64_t fund_asset_bjhgsh;            ///< 上海报价回购
  int64_t fund_asset_bjhgsz;            ///< 深圳报价回购
  int64_t fund_asset_ofuncome;          ///< 基金认申购定投未确认在途资金
  int64_t fund_asset_xjgj;              ///< 现金管家
  int64_t fund_asset_zfzz;              ///< 支付转账
  int64_t fund_asset_jdlsec;            ///< 君得利二号
  int64_t fund_asset_fjjj;              ///< 分级基金赎回日提前释放
  int64_t fund_asset_curdayrqgh;        ///< 融券购回当日到期
  int64_t fund_asset_curdayrzgh;        ///< 融资购回当日到期
  int64_t fund_asset_otc_end;           ///< OTC当前到期
  int64_t debts_total;                  ///< 总负债
  int64_t zy_mtkvalue;                  ///< 应收股票质押市值
  int64_t zy_debts;                     ///< 股票质押负债
  int64_t en_mktvalue;                  ///< 应收约定购回市值
  int64_t en_debts;                     ///< 约定购回负债
  int64_t zywy_mktvalue;                ///< 股票质押违约卖出市值
  int64_t fund_asset_matchrqgh;         ///< 国债逆回购当日成交
  int64_t sum_profit_cost;              ///< 证券总成本
  int64_t sump_rofit_amt;               ///< 证券资产总浮动盈亏
  int64_t fund_asset_stkwxxj;           ///< 网下现金认购资产
  int64_t fund_asset_adjust_xsf;        ///< 新三板新股申购在途资金
  int64_t fund_asset_adjust_qt;         ///< 新股配债在途资金
  int64_t fund_asset_adjust_hgt;        ///< 港股通在途资金
  int64_t fund_asset_adjust_lof;        ///< lof在途资金
  int64_t fund_asset_stk_only;          ///< 股票资产
  int64_t fund_asset_adjust_etf;        ///< etf在途资金
  int64_t trade_date;                   ///< 当前资产交易日期
};

/// 可取资金查询应答(单条)
class CustFundMaxdrawInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 4> currency;         ///< 货币种类
  int64_t fund_bal;                     ///< 资金余额
  int64_t fund_avl;                     ///< 资金可用金额
  int64_t fund_buy;                     ///< 买入冻结
  int64_t fund_sale;                    ///< 卖出解冻
  int64_t market_value;                 ///< 资产总值
  int64_t fund_all;                     ///< 资金资产
  uint8_t fund_seq;                     ///< 主资金标志
  int64_t stk_value;                    ///< 市值
  int64_t max_draw;                     ///< 可取金额
  // std::array<char, 2048> net_addr;    ///< 终端MAC地址(暂不开启, 协议层为String_def=std::string)
  std::array<char, 5> bank_code;        ///< 银行代码
  int64_t fund_bjhg_avl;                ///< 报价回购可用资金
  char credit_flag;                     ///< 资金帐号属性
  int64_t stk_value_non_of_cash;        ///< 含开放式基金不含现金管理产品的市值
  int64_t of_cash;                      ///< 现金管理产品的市值
};

/// 融资融券信用数据（可用保证金）查询请求
class CustMarginAvlQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 4> currency;         ///< 货币种类
  uint16_t market_id;                   ///< 交易市场
  std::array<char, 8> security_id;      ///< 证券代码
  char credit_direct;                   ///< 信用方向
};

/// 融资融券信用数据（可用保证金）查询应答(单条)
class CustMarginAvlInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  char credit_direct;                   ///< 信用方向
  int64_t margin_avl;                   ///< 保证金可用余额
  int64_t credit_bal;                   ///< 融资/融券授信额度
  int64_t credit_avl;                   ///< 融资/融券可用的额度
  int64_t margin_rate;                  ///< 融资/融券保证金比例
  int64_t debts_bal;                    ///< 应付融资款/应付融券市值
  int64_t max_amt;                      ///< 最大融资/融券金额
  int64_t fund_avl;                     ///< 资金可用数
  int64_t credit_fund_avl;              ///< 融券卖出资金
  int64_t g_dis_market_avl;             ///< 担保证券的折算价值
  int64_t mar_stk_val;                  ///< ∑(充抵保证金的证券市值*折算率)
  int64_t total_dis_f_mk_val;           ///< ∑融资的市值*折算率
  int64_t mar_fund_profit;              ///< ∑融资盈亏*折算率
  int64_t mar_stk_profit;               ///< ∑融券盈亏*折算率
  int64_t total_rate_f_debts_bal;       ///< ∑融资金额*融资保证金比例
  int64_t mar_fund_margin;              ///< 已占用融资保证金
  int64_t mar_fund_margin_frz;          ///< 委托冻结融资保证金
  int64_t total_rate_d_sale_bal;        ///< ∑融券市值*融券保证金比例
  int64_t mar_stk_margin;               ///< 已占用融券保证金
  int64_t mar_stk_margin_frz;           ///< 委托冻结融券保证金
  int64_t total_fee;                    ///< 利息和费用总和
  uint16_t market_id;                   ///< 交易市场
  std::array<char, 8> security_id;      ///< 证券代码
  std::array<char, 40> security_symbol; ///< 证券简称
  int64_t margin_fund_bal;              ///< 资金余额（保证金公式部分）
  int64_t margin_credit_bal;            ///< 融券卖出冻结资金（保证金公式部分）
  int64_t fund_margin_avl;              ///< 资金账号级保证金可用金额(不考虑安全垫影响)
  int64_t fund_margin_avl_sub;          ///< 资金账号级保证金可用金额(考虑安全垫影响)
};

/// 资产负债查询请求
class AssetDebtsQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 4> currency;         ///< 货币种类
};

/// 资产负债查询应答(单条)
class AssetDebtsInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 4> currency;         ///< 货币种类
  std::array<char, 64> cust_name;       ///< 客户姓名
  char credit_status;                   ///< 信用状态
  int64_t keep_rate;                    ///< 维持担保比例
  int64_t fund;                         ///< 资金
  int64_t fund_asset;                   ///< 资金资产
  int64_t stk_asset;                    ///< 证券资产
  int64_t bond_asset;                   ///< 债券资产
  int64_t asset;                        ///< 总资产（折算后）
  int64_t old_asset;                    ///< 原总资产
  int64_t liability;                    ///< 总负债
  int64_t fund_bal;                     ///< 资金余额
  int64_t fund_avl;                     ///< 资金可用数
  int64_t fund_avl_frz;                 ///< 资金可用冻结数
  int64_t d_sale_amt_bal;               ///< 融券卖出资金余额
  int64_t d_sale_amt_frz;               ///< 融券卖出资金冻结数
  int64_t credit_buy;                   ///< 买券还券使用成交
  int64_t credit_sale;                  ///< 融券卖出成交
  int64_t credit_buy_sale;              ///< 融券卖出及买券还券差额
  int64_t relese_amt_unfrz;             ///< 当日累计释放
  int64_t credit_amt;                   ///< 当日累计释放（息费偿还部分）
  int64_t hbjj_bal;                     ///< 使用融券卖出资金购买货基的昨日存量
  int64_t hbjj_buy;                     ///< 使用融券卖出资金当日买入货基冻结
  int64_t hbjj_sale;                    ///< 专用资金货基当日退还
  int64_t hbjj_amt;                     ///< 专用资金货基累计占用
  int64_t hbjj_remain_amt;              ///< 专用资金货基剩余占用
  int64_t gage_mkt_avl;                 ///< 担保证券市值
  int64_t gage_mkt_avl_zs;              ///< 修正市值
  int64_t gage_mkt_avl_db;              ///< 担保持仓市值下的标的持仓市值
  int64_t rights_amt;                   ///< 修正资产
  int64_t f_deal_avl;                   ///< 应付融资款
  int64_t fund_fee;                     ///< 融资利息
  int64_t fund_pro_fee;                 ///< 融资承诺费
  int64_t fund_manage_fee;              ///< 融资管理费
  int64_t fund_over_due_fee;            ///< 融资逾期利息
  int64_t fund_puni_fee;                ///< 融资罚息
  int64_t fund_puni_debts;              ///< 融资逾期本金罚息
  int64_t fund_manage_over_due;         ///< 融资逾期管理费
  int64_t fund_manage_puni_ntr;         ///< 融资逾期管理费罚息
  int64_t f_fee;                        ///< 融资息费合计
  int64_t f_total_debts;                ///< 融资负债合计
  int64_t deal_f_mkt_avl;               ///< 应付融券市值
  int64_t stk_fee;                      ///< 融券息费
  int64_t stk_pro_fee;                  ///< 融券承诺费
  int64_t stk_manage_fee;               ///< 融券管理费
  int64_t stk_over_due_fee;             ///< 融券逾期费用
  int64_t stk_puni_fee;                 ///< 融券罚息
  int64_t over_due_rights;              ///< 融券权益逾期费用
  int64_t puni_rights;                  ///< 融券权益罚息
  int64_t stk_puni_debts;               ///< 融券逾期本金罚息
  int64_t stk_manage_over_due;          ///< 融券逾期管理费
  int64_t stk_manage_puni_ntr;          ///< 融券逾期管理费罚息
  int64_t d_fee;                        ///< 融券息费合计
  int64_t d_total_debts;                ///< 融券负债合计
  int64_t fund_draw;                    ///< 可取资产
  int64_t fill_amt;                     ///< 补仓金额
  int64_t credit_bal;                   ///< 授信额度
  int64_t credit_avl;                   ///< 授信可用的额度
  int64_t f_credit_bal;                 ///< 融资授信额度
  int64_t f_credit_avl;                 ///< 融资可用额度
  int64_t d_credit_bal;                 ///< 融券授信额度
  int64_t d_credit_avl;                 ///< 融券可用额度
  int64_t total_profit_cost;            ///< 总盈亏
  int64_t stk_unused_fee;               ///< 锁券占用费
  int64_t stk_puni_unused_fee;          ///< 逾期锁券占用费罚息
  int64_t fund_margin_frz;              ///< 冻结融资保证金
  int64_t stk_margin_frz;               ///< 冻结融券保证金
  int64_t market_value;                 ///< 资产总值
  uint8_t fund_seq;                     ///< 资金序号
  int64_t stk_value;                    ///< 所有持仓的证券市值
  int64_t d_sale_bal;                   ///< 融券卖出资金余额
  int64_t credit_frz_amt;               ///< 买券还券委托冻结
  int64_t long_profit_cost;             ///< 多头盈亏
  int64_t short_profit_cost;            ///< 空头盈亏
  int64_t lever_rate;                   ///< 杠杆率
  int64_t asset_frz_package_out;        ///< 划出股份冻结包市值
  int64_t asset_frz_package_in;         ///< 划入股份冻结包市值
  int64_t trade_date;                   ///< 当前资产交易日期
  int64_t max_draw;                     ///< 可取金额
  int64_t max_draw2;                    ///< 可取金额(不考虑维保)
};

/// 两融最大可委托数查询请求
class CreditMaxOrderQtyQueryReq {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  uint16_t market_type;                 ///< 证券所属市场
  std::array<char, 8> security_id;      ///< 证券代码
  uint8_t business_type;                ///< 业务类型
  char side;                            ///< 买卖方向
  int64_t price;                        ///< 委托价格
};

/// 两融最大可委托数查询应答(单条)
class CreditMaxOrderQtyInfo {
public:
  int64_t client_req_no;                ///< 客户请求号
  std::array<char, 16> cust_id;         ///< 客户号
  std::array<char, 16> fund_account_id; ///< 客户资金账号
  std::array<char, 10> branch_id;       ///< 分支机构代码
  std::array<char, 12> account_id;      ///< 账户ID
  uint8_t business_type;                ///< 业务类型
  char side;                            ///< 买卖方向
  int64_t max_order_qty;                ///< 最大可委托数
  int64_t theoretical_order_qty;        ///< 理论可委托数
  int64_t max_pay_qty;                  ///< 买券还券应补数量
  int64_t max_pay_amt;                  ///< 应偿还资金负债
  int64_t sep_rem_qty;                  ///< 专户余券
  int64_t fund_credit_avl;              ///< 买券还券可用资金
  int64_t stk_debts_avl;                ///< 融券负债可用额度
  int64_t sg_max_stk_qty;               ///< 个股申购上限
  std::string zero_reason;              ///< 可委托数为0原因描述
  uint8_t limit_flag;                   ///< 减持受限标识
  int64_t jj_max_limit_qty;             ///< 减持受限名单内的竞价卖出可用额度
};

} // namespace lb_api
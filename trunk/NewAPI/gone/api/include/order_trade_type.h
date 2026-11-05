#pragma once

#include <array>
#include <stdint.h>

namespace lb_api
{
  /// ---- api 对外接口字段长度定义 ----
  /// 约束: 柜台协议侧字段长度要求 ≥ 本层 ; api 内部长度定义见 api_event_msg.h 的 INNER_*_LEN
  static constexpr int32_t MAX_CUST_ID_LEN = 16;         ///< 客户号
  static constexpr int32_t MAX_FUND_ACCOUNT_ID_LEN = 16; ///< 客户资金账号
  static constexpr int32_t MAX_ACCOUNT_ID_LEN = 12;      ///< 客户股东账号
  static constexpr int32_t MAX_BRANCH_ID_LEN = 10;       ///< 分支机构代码
  static constexpr int32_t MAX_PASSWORD_LEN = 256;       ///< 密码
  static constexpr int32_t MAX_USER_INFO_LEN = 64;       ///< 用户私有信息
  static constexpr int32_t MAX_FEATURE_CODE_LEN = 1024;  ///< 客户终端信息
  static constexpr int32_t MAX_ERR_MSG_LEN = 124;        ///< 错误信息
  static constexpr int32_t MAX_SECURITY_ID_LEN = 8;      ///< 证券代码
  static constexpr int32_t MAX_EXEC_ID_LEN = 32;         ///< 柜台成交编号 [对齐 api-tmp-source: 32 宽, 与 ATP ExecID_def 一致]
  static constexpr int32_t MAX_AGW_USER_LEN = 32;        ///< 98 agw 用户名

  /// ---- 字典常量 (以 98 ATP 数据字典为唯一基准, 权威: api/counter98/constants_agw_external.h;
  /// 值域/语义见 SYSTEM_CONTRACT RUL-16, 表示法修订见 RUL-18;
  /// 委托市价类型/证券类别/委托方式扩展三组字典的值域来源与命名见 RUL-47) ----

  /// 市场 (AgwMarketID, int16_t)
  static constexpr int16_t MARKET_TYPE_NULL = 0;     ///< 空
  static constexpr int16_t MARKET_TYPE_SH = 101;     ///< 上交所
  static constexpr int16_t MARKET_TYPE_SZ = 102;     ///< 深交所
  static constexpr int16_t MARKET_TYPE_BSE = 109;    ///< 北交所/北京 (委托 market_type 使用；仅深圳 API 实例支持，实例不配置此值)

  /// 买卖方向 (AgwSide, char, 完整字典)
  static constexpr char ORDER_SIDE_ALL = '0';             ///< 所有
  static constexpr char ORDER_SIDE_BUY = '1';             ///< 买
  static constexpr char ORDER_SIDE_SELL = '2';            ///< 卖
  static constexpr char ORDER_SIDE_REPO = '3';            ///< 正回购
  static constexpr char ORDER_SIDE_ANTI_REPO = '4';       ///< 逆回购
  static constexpr char ORDER_SIDE_CALL = '9';            ///< 认购
  static constexpr char ORDER_SIDE_PURCHASE = 'D';        ///< 申购
  static constexpr char ORDER_SIDE_REDEEM = 'E';          ///< 赎回
  static constexpr char ORDER_SIDE_PLEDGE_IN = 'H';       ///< 入库
  static constexpr char ORDER_SIDE_PLEDGE_OUT = 'I';      ///< 出库
  static constexpr char ORDER_SIDE_SUBSCRIPTION = 'S';    ///< 配股认购(独立, 非98、个微字典)
  static constexpr char ORDER_SIDE_COLLATERAL_BUY = 'T';  ///< 担保品买入(独立，非98、个微字典)
  static constexpr char ORDER_SIDE_COLLATERAL_SELL = 'U'; ///< 担保品卖出(独立，非98、个微字典)
  static constexpr char ORDER_SIDE_MARGIN_BUY = 'V';      ///< 融资买入(独立，非98、个微字典)
  static constexpr char ORDER_SIDE_SHORT_SELL = 'W';      ///< 融券卖出(独立，非98、个微字典)
  static constexpr char ORDER_SIDE_SELL_TO_REPAY = 'X';   ///< 卖券还款；todo: 待确认98业务映射，当前委托返回不支持

  /// 委托市价类型 (char, '1'-'7'; G1 FPGA 柜台时, 仅支持 '2' 限价)
  /// 98 回报中的委托类型（order_type）暂填 0，待确认如何映射为 API 类型。
  static constexpr char ORDER_TYPE_JDSC = '1'; ///< 最优五档即时成交剩余撤销
  static constexpr char ORDER_TYPE_FIX = '2';  ///< 限价订单
  static constexpr char ORDER_TYPE_JDSZ = '3'; ///< 最优五档即时成交剩余转限价(上交所, 北交所)
  static constexpr char ORDER_TYPE_BFZY = '4'; ///< 本方最优
  static constexpr char ORDER_TYPE_DFZY = '5'; ///< 对手方最优
  static constexpr char ORDER_TYPE_FAK = '6';  ///< 即时成交剩余撤销申报(深交所)
  static constexpr char ORDER_TYPE_FOK = '7';  ///< 全额成交或撤销申报(深交所)

  /// 委托状态 (AgwOrdStatus, uint8_t, 完整字典)
  static constexpr uint8_t ORDER_STATE_NEW = 0;                      ///< 已申报
  static constexpr uint8_t ORDER_STATE_PART_FILLED = 1;              ///< 部分成交
  static constexpr uint8_t ORDER_STATE_FILLED = 2;                   ///< 全部成交
  static constexpr uint8_t ORDER_STATE_PART_FILLED_PART_CANCEL = 3;  ///< 部成部撤
  static constexpr uint8_t ORDER_STATE_CANCELED = 4;                 ///< 已撤销
  static constexpr uint8_t ORDER_STATE_PART_CANCELED = 5;            ///< 部分撤销
  static constexpr uint8_t ORDER_STATE_REJECTED = 8;                 ///< 已拒绝 (含 API 层拒绝/废单)
  static constexpr uint8_t ORDER_STATE_UNSENT = 9;                   ///< 未申报
  static constexpr uint8_t ORDER_STATE_SENDING = 10;                 ///< 正报
  static constexpr uint8_t ORDER_STATE_WAIT_CANCEL = 11;             ///< 待撤销
  static constexpr uint8_t ORDER_STATE_PART_FILLED_WAIT_CANCEL = 12; ///< 部成待撤

  /// 回报类型 (AgwExecType, char, 完整字典)
  static constexpr char RSP_TYPE_NEW = '0';      ///< 新订单 (委托确认)
  static constexpr char RSP_TYPE_INTERNAL = '1'; ///< 内部执行
  static constexpr char RSP_TYPE_CANCELED = '4'; ///< 已撤销
  static constexpr char RSP_TYPE_REJECTED = '8'; ///< 已拒绝 (含 API 层拒绝)
  static constexpr char RSP_TYPE_TRADE = 'F';    ///< 已成交

  /// 证券类别 (SecurityType_def, uint16_t, 完整字典)
  static constexpr uint16_t SECURITY_TYPE_NULL = 0;                       ///< 默认类型
  static constexpr uint16_t SECURITY_TYPE_MAIN_A_STOCK = 1;               ///< 主板A股
  static constexpr uint16_t SECURITY_TYPE_SME_STOCK = 2;                  ///< 中小板股票
  static constexpr uint16_t SECURITY_TYPE_GEM_STOCK = 3;                  ///< 创业板股票
  static constexpr uint16_t SECURITY_TYPE_MAIN_B_STOCK = 4;               ///< 主板B股
  static constexpr uint16_t SECURITY_TYPE_GOV_BOND = 5;                   ///< 国债(含地方债)
  static constexpr uint16_t SECURITY_TYPE_ENTERPRISE_BOND = 6;            ///< 企业债
  static constexpr uint16_t SECURITY_TYPE_CORPORATE_BOND = 7;             ///< 公司债
  static constexpr uint16_t SECURITY_TYPE_CB = 8;                         ///< 可转债
  static constexpr uint16_t SECURITY_TYPE_PRIVATE_BOND = 9;               ///< 私募债
  static constexpr uint16_t SECURITY_TYPE_PRIVATE_EB = 10;                ///< 可交换私募债
  static constexpr uint16_t SECURITY_TYPE_BROKER_SUBORD_BOND = 11;        ///< 证券公司次级债
  static constexpr uint16_t SECURITY_TYPE_PLEDGE_REPO = 12;               ///< 质押式回购
  static constexpr uint16_t SECURITY_TYPE_ABS = 13;                       ///< 资产支持证券
  static constexpr uint16_t SECURITY_TYPE_STOCK_ETF = 14;                 ///< 股票ETF
  static constexpr uint16_t SECURITY_TYPE_XMKT_STOCK_ETF = 15;            ///< 跨市场股票ETF
  static constexpr uint16_t SECURITY_TYPE_XBORDER_ETF = 16;               ///< 跨境ETF
  static constexpr uint16_t SECURITY_TYPE_PHYS_BOND_ETF = 17;             ///< 实物债券ETF
  static constexpr uint16_t SECURITY_TYPE_CASH_BOND_ETF = 18;             ///< 现金债券ETF
  static constexpr uint16_t SECURITY_TYPE_GOLD_ETF = 19;                  ///< 黄金ETF
  static constexpr uint16_t SECURITY_TYPE_CURRENCY_ETF = 20;              ///< 货币ETF
  static constexpr uint16_t SECURITY_TYPE_LEVERAGED_ETF = 21;             ///< 杠杆ETF
  static constexpr uint16_t SECURITY_TYPE_COMMODITY_FUT_ETF = 22;         ///< 商品期货ETF
  static constexpr uint16_t SECURITY_TYPE_STANDARD_LOF = 23;              ///< 标准LOF
  static constexpr uint16_t SECURITY_TYPE_GRADED_SUB_FUND = 24;           ///< 分级子基金
  static constexpr uint16_t SECURITY_TYPE_CLOSED_END_FUND = 25;           ///< 封闭式基金
  static constexpr uint16_t SECURITY_TYPE_PURCHASE_REDEEM_ONLY_FUND = 26; ///< 仅申赎基金
  static constexpr uint16_t SECURITY_TYPE_RIGHTS_WARRANT = 27;            ///< 配售权证
  static constexpr uint16_t SECURITY_TYPE_WARRANT = 28;                   ///< 权证
  static constexpr uint16_t SECURITY_TYPE_STOCK_OPTION = 29;              ///< 个股期权
  static constexpr uint16_t SECURITY_TYPE_ETF_OPTION = 30;                ///< ETF期权
  static constexpr uint16_t SECURITY_TYPE_AM_PRODUCT = 31;                ///< 资管产品
  static constexpr uint16_t SECURITY_TYPE_OFFER_REPO = 32;                ///< 报价回购
  static constexpr uint16_t SECURITY_TYPE_PREFERRED_STOCK = 33;           ///< 优先股
  static constexpr uint16_t SECURITY_TYPE_BROKER_SHORT_BOND = 34;         ///< 证券公司短期债
  static constexpr uint16_t SECURITY_TYPE_CORPORATE_EB = 35;              ///< 可交换公司债
  static constexpr uint16_t SECURITY_TYPE_BOND_DISTRIBUTION = 36;         ///< 债券分销
  static constexpr uint16_t SECURITY_TYPE_GOV_BOND_DISTRIBUTION = 37;     ///< 国债分销
  static constexpr uint16_t SECURITY_TYPE_OTHER_BOND = 38;                ///< 其他债券
  static constexpr uint16_t SECURITY_TYPE_XMKT_XBORDER_FUND = 39;         ///< 跨市场/跨境资金
  static constexpr uint16_t SECURITY_TYPE_OTHER_FUND = 40;                ///< 其他基金
  static constexpr uint16_t SECURITY_TYPE_INTL_STOCK = 41;                ///< 国际版股票
  static constexpr uint16_t SECURITY_TYPE_OTHER_STOCK = 42;               ///< 其他股票
  static constexpr uint16_t SECURITY_TYPE_BUYOUT_REPO = 43;               ///< 买断式债券回购
  static constexpr uint16_t SECURITY_TYPE_GEM_CB = 44;                    ///< 创业板可转债
  static constexpr uint16_t SECURITY_TYPE_MAIN_SME_DR = 45;               ///< 主板中小板存托凭证
  static constexpr uint16_t SECURITY_TYPE_SH_LONDON_CDR = 46;             ///< 沪伦通CDR
  static constexpr uint16_t SECURITY_TYPE_MAIN_SME_IE_STOCK = 47;         ///< 主板中小板创新企业股票
  static constexpr uint16_t SECURITY_TYPE_PLEDGE_CODE = 48;               ///< 质押代码
  static constexpr uint16_t SECURITY_TYPE_RT_MM_FUND = 49;                ///< 实时申赎货币基金
  static constexpr uint16_t SECURITY_TYPE_NEW_ISSUE = 50;                 ///< 新股新债发行
  static constexpr uint16_t SECURITY_TYPE_STAR_STOCK = 51;                ///< 科创板
  static constexpr uint16_t SECURITY_TYPE_HK_STOCK = 52;                  ///< 港股
  static constexpr uint16_t SECURITY_TYPE_STAR_CDR = 53;                  ///< 科创板CDR
  static constexpr uint16_t SECURITY_TYPE_STAR_ETF = 63;                  ///< 科创板ETF-不使用华锐实现
  static constexpr uint16_t SECURITY_TYPE_STAR_LOF = 64;                  ///< 科创板相关LOF-不使用华锐实现
  static constexpr uint16_t SECURITY_TYPE_REITS = 65;                     ///< REITs
  static constexpr uint16_t SECURITY_TYPE_DIRECTIONAL_CB = 66;            ///< 定向可转债
  static constexpr uint16_t SECURITY_TYPE_B_TO_H = 67;                    ///< B转H股
  static constexpr uint16_t SECURITY_TYPE_HKT_ETF = 69;                   ///< 港股通ETF
  static constexpr uint16_t SECURITY_TYPE_INVALID = 999;                  ///< 无效的证券类型
  static constexpr uint16_t SECURITY_TYPE_GEM_CDR = 1000;                 ///< 创业板存托凭证
  static constexpr uint16_t SECURITY_TYPE_GEM_IE_STOCK = 1001;            ///< 创业板创新企业股票
  static constexpr uint16_t SECURITY_TYPE_NEEQ_MM = 1002;                 ///< 股转挂牌做市
  static constexpr uint16_t SECURITY_TYPE_NEEQ_CONT_AUCTION = 1004;       ///< 股转挂牌连续竞价
  static constexpr uint16_t SECURITY_TYPE_NEEQ_CALL_AUCTION = 1005;       ///< 股转挂牌集合竞价
  static constexpr uint16_t SECURITY_TYPE_NEEQ_TENDER = 1006;             ///< 股转要约收购
  static constexpr uint16_t SECURITY_TYPE_NEEQ_PRIVATE_PREFERRED = 1007;  ///< 非公开优先股
  static constexpr uint16_t SECURITY_TYPE_NEEQ_DELISTED = 1008;           ///< 股转摘牌证券
  static constexpr uint16_t SECURITY_TYPE_NEEQ_CSDC_CERTIFIED = 1009;     ///< 中登认证证券
  static constexpr uint16_t SECURITY_TYPE_DELISTED_CB = 1010;             ///< 退市可转债

  /// 委托方式扩展 (OrderWayExt_def, char; 本组常量填 order_way_ext[0], 两字符整体透传柜台;
  /// 值域=用户提供 GONE 既有字典, vendor 包无此字典, 第二字符语义仓内无证据)
  static constexpr char ORDER_WAY_EXT_SELF_SERVICE = '0'; ///< 自助委托
  static constexpr char ORDER_WAY_EXT_TEL = '1';          ///< 电话委托
  static constexpr char ORDER_WAY_EXT_RESIDENT = '2';     ///< 驻留委托
  static constexpr char ORDER_WAY_EXT_REMOTE = '3';       ///< 远程委托
  static constexpr char ORDER_WAY_EXT_COUNTER = '4';      ///< 柜台委托
  static constexpr char ORDER_WAY_EXT_ROAMING = '5';      ///< 漫游委托
  static constexpr char ORDER_WAY_EXT_BRANCH = '6';       ///< 分支委托
  static constexpr char ORDER_WAY_EXT_ONLINE = '7';       ///< 网上委托
  static constexpr char ORDER_WAY_EXT_OTHER_TEL = '8';    ///< 其他电话
  static constexpr char ORDER_WAY_EXT_OTHER_ONLINE = '9'; ///< 其他网上

  /// 登录请求
  class LoginReq
  {
  public:
    int64_t client_req_no;                                      ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                  ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id;  ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;            ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;              ///< 分支机构代码
    std::array<char, 2> order_way_ext;                          ///< 客户委托方式
    std::array<char, MAX_PASSWORD_LEN> password;                ///< 密码
    std::array<char, MAX_USER_INFO_LEN> user_info;              ///< 用户私有信息
    std::array<char, MAX_FEATURE_CODE_LEN> client_feature_code; ///< 客户终端信息
  };

  /// 登录应答
  class LoginAns
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int16_t market_type;                                       ///< 市场
    int32_t err_code;                                          ///< 错误码
    std::array<char, MAX_ERR_MSG_LEN> err_msg;                 ///< 错误信息
    int64_t login_time;                                        ///< 登陆时间，HHMMSSmmm
  };

  /// 登出请求
  class LogoutReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, 2> order_way_ext;                         ///< 客户委托方式
  };

  /// 登出应答
  class LogoutAns
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int32_t err_code;                                          ///< 错误码
    std::array<char, MAX_ERR_MSG_LEN> err_msg;                 ///< 错误信息
  };

  /// 委托请求
  class OrderReq
  {
  public:
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    char side;                                                 ///< 买卖方向 (ORDER_SIDE_* 字典)
    char order_type;                                           ///< 委托类型 (ORDER_TYPE_* 委托市价类型)
    uint16_t policy_id;                                        ///< 策略佣金ID
    uint16_t tgw_id;                                           ///< TGW编号
    int16_t market_type;                                       ///< 市场ID (101沪/102深/109北, AgwMarketID 字典)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    int64_t order_price;                                       ///< 委托价格，放大10000
    int64_t order_qty;                                         ///< 委托数量, 放大100倍(股数×100)
    int64_t stop_price;                                        ///< 止损价, 放大10000
    int64_t client_seq_id;                                     ///< 用户私有报单号
  };

  /// 撤单请求
  class CancelReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int64_t order_sys_no;                                      ///< 柜台原始报单编号
    int64_t client_seq_id;                                     ///< 用户私有报单号，预留，暂不支持
  };

  /// 撤单失败响应
  class CancelRsp
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int16_t market_type;                                       ///< 市场
    int64_t order_sys_no;                                      ///< 柜台原始报单编号
    int64_t client_seq_id;                                     ///< 用户私有报单号，预留，暂不支持
    int32_t err_code;                                          ///< 撤单失败错误码
    int32_t rej_api;                                           ///< 是否为api错误
  };

  /// 委托响应 + 委托确认 + 委托废单  + 撤单成交
  class OrderRtn
  {
  public:
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    char side;                                                 ///< 买卖方向
    char order_type;                                           ///< 委托类型
    uint8_t order_status;                                      ///< 委托状态 (ORDER_STATE_*, AgwOrdStatus 字典)
    char rtn_type;                                             ///< 回报类型 (RSP_TYPE_*, AgwExecType 字典)
    uint16_t policy_id;                                        ///< 策略佣金ID
    int16_t market_type;                                       ///< 市场 (101沪/102深/109北, AgwMarketID 字典)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    int64_t order_price;                                       ///< 委托价格，放大10000
    int64_t order_qty;                                         ///< 委托数量, 放大100倍(股数×100)
    int64_t client_seq_id;                                     ///< 用户私有报单号
    int64_t order_sys_no;                                      ///< 柜台委托号
    int64_t frozen_amount;                                     ///< 冻结金额, 放大10000
    int64_t fee;                                               ///< 累计费用（含冻结）, 放大10000
    // int64_t trade_amount;                 ///< 累计成交金额
    int64_t trade_qty;   ///< 累计成交数量, 放大100倍(股数×100, RUL-35)
    int64_t cancel_qty;  ///< 撤单数量, 放大100倍(股数×100, RUL-35)
    int32_t err_code;    ///< 错误码
    int64_t order_time;  ///< 委托时间，HHMMSSmmm
    int64_t update_time; ///< 更新时间，HHMMSSmmm
  };

  /// 成交回报
  class TradeRtn
  {
  public:
    // 委托信息
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    char side;                                                 ///< 买卖方向 ('1'买/'2'卖, ORDER_SIDE_*, AgwSide 字典)
    char order_type;                                           ///< 委托类型 (ORDER_TYPE_* '1'-'7' 委托市价类型; 本期校验域仅 '2' 限价, 值域分歧见 SYSTEM_CONTRACT §7)
    uint8_t order_status;                                      ///< 委托状态 (ORDER_STATE_*, AgwOrdStatus 字典)
    char rtn_type;                                             ///< 回报类型 (RSP_TYPE_*, AgwExecType 字典，仅为 'F')
    uint16_t policy_id;                                        ///< 策略佣金ID
    int16_t market_type;                                       ///< 市场 (101沪/102深/109北, AgwMarketID 字典)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    int64_t order_price;                                       ///< 委托价格，放大10000
    int64_t order_qty;                                         ///< 委托数量, 放大100倍(股数×100, RUL-35)
    int64_t client_seq_id;                                     ///< 用户私有报单号
    int64_t order_sys_no;                                      ///< 柜台委托号
    int64_t frozen_amount;                                     ///< 冻结金额, 放大10000
    int64_t fee;                                               ///< 累计费用（含冻结）, 放大10000
    // int64_t trade_amount;                 ///< 累计成交金额
    int64_t trade_qty;  ///< 累计成交数量, 放大100倍(股数×100, RUL-35)
    int64_t cancel_qty; ///< 撤单数量, 放大100倍(股数×100, RUL-35)
    int64_t order_time; ///< 委托时间，HHMMSSmmm
    // 成交信息
    int64_t exec_time;                         ///< 成交时间，HHMMSSmmm
    std::array<char, MAX_EXEC_ID_LEN> exec_id; ///< 柜台成交编号
    int64_t exec_price;                        ///< 成交价格, 放大10000
    int64_t exec_qty;                          ///< 成交数量, 放大100倍(股数×100, RUL-35)
    int64_t exec_amount;                       ///< 成交金额, 放大10000
    int64_t exec_fee;                          ///< 单笔成交费用, 放大10000
  };

  /// 成交信息(成交查询应答单条)
  class TradeInfo
  {
  public:
    // 委托信息
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    char side;                                                 ///< 买卖方向 ('1'买/'2'卖, ORDER_SIDE_*, AgwSide 字典)
    char order_type;                                           ///< 委托类型 (ORDER_TYPE_*)
    int16_t market_type;                                       ///< 市场 (101沪/102深/109北, AgwMarketID 字典)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    int64_t client_seq_id;                                     ///< 用户私有报单号
    int64_t order_sys_no;                                      ///< 柜台委托号
    // 成交信息
    int64_t exec_time;             ///< 成交时间，HHMMSSmmm
    char exec_id[MAX_EXEC_ID_LEN]; ///< 柜台成交编号
    int64_t exec_price;            ///< 成交价格, 放大10000
    int64_t exec_qty;              ///< 成交数量, 放大100倍(股数×100, RUL-35)
    int64_t exec_amount;           ///< 成交金额, 放大10000
    int64_t exec_fee;              ///< 单笔成交费用, 放大10000
  };

  /// 客户委托查询请求
  class OrderQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int64_t order_sys_no;                                      ///< 柜台委托号
    int64_t client_seq_id;                                     ///< 用户私有报单号 (统一命名, 原user_seq_id)
  };

  /// 客户委托批量查询请求
  class OrderBatchQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int64_t order_sys_no_begin;                                ///< 柜台委托号起始号(填0表示所有)
    int64_t order_sys_no_end;                                  ///< 柜台委托号结束(填0表示大于起始的所有)
  };

  /// 客户成交查询请求
  class TradeQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int64_t order_sys_no;                                      ///< 柜台委托号
    int64_t client_seq_id;                                     ///< 用户私有报单号 (统一命名, 原user_seq_id)
  };

  /// 客户成交批量查询请求
  class TradeBatchQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int64_t exec_id_begin;                                     ///< 柜台成交起始号(填0表示所有)
    int64_t exec_id_end;                                       ///< 柜台成交号结束(填0表示大于起始的所有)
  };

  /// 客户资金查询请求
  class FundQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
  };

  /// 分页查询控制 [对齐 api-tmp-source]
  struct PageQueryCtrl
  {
    int64_t query_index; ///< 起始查询索引号，填0表示从头开始查
    int64_t return_num;  ///< 指定查询返回数量，填0表示不指定
  };

  /// 客户持仓查询请求
  class PositionQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int16_t market_type;                                       ///< 证券所属市场 (101/102/109), 填0查所有
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码(填空表示所有)
    PageQueryCtrl page_ctrl;                                   ///< 分页控制 [对齐 source]
  };

  /// 客户资金信息(资金查询应答单条)
  class CustFundInfo
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int16_t market_type;                                       ///< 市场 (101沪/102深/109北, AgwMarketID 字典)
    int32_t reserved;                                          ///< 保留
    int64_t avail_amount;                                      ///< 可用资金, 放大10000
    int64_t token_amount;                                      ///< 可取资金, 放大10000
  };

  /// 客户持仓信息(持仓查询应答单条) [对齐 api-tmp-source: 自研股份查询富字段版]
  class CustPositionInfo
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 客户股东账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int16_t market_type;                                       ///< 市场 (101沪/102深/109北, AgwMarketID 字典)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    std::array<char, 40> security_symbol;                      ///< 证券简称
    uint16_t security_type;                                    ///< 证券类别
    std::array<char, 4> currency;                              ///< 货币种类
    std::array<char, 6> pbu_id;                                ///< 席位代码
    int64_t stk_bal;                                           ///< 股份余额, 放大100倍(股数×100, RUL-35)
    int64_t stk_avl;                                           ///< 股份可用, 放大100倍(股数×100, RUL-35)
    int64_t stk_can_trans;                                     ///< 股份可转, 放大100倍(股数×100, RUL-35)
    int64_t stk_frz;                                           ///< 股份冻结, 放大100倍(股数×100, RUL-35)
    int64_t leaves_qty;                                        ///< 市值持仓, 放大100倍(股数×100, RUL-35)
    int64_t market_value;                                      ///< 市值, 放大10000
    int64_t cost_price;                                        ///< 成本价格, 放大10000
    int64_t profit_cost;                                       ///< 盈亏成本, 放大10000
    int64_t pro_income;                                        ///< 累计浮动盈亏, 放大10000
    int64_t income;                                            ///< 买入浮动盈亏, 放大10000
    int64_t profit_rate;                                       ///< 浮亏比例, ATP N12(8)=×10^8 透传
    int64_t limit_up;                                          ///< 涨停价格, 放大10000
    int64_t limit_down;                                        ///< 跌停价格, 放大10000
  };

  // ---- 98 柜台扩展查询 (结构按本仓 POD 风格重述; ) ----
  // 金额/费用字段统一 ATP 口径放大10000, 比率(N12(8))/日期字段按协议透传

  /// 最大可委托数查询请求 (AgwMsgExtQueryMaxOrderQty, 300001)
  class MaxOrderQtyQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int16_t market_type;                                       ///< 证券所属市场 (101/102/109)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    uint8_t business_type;                                     ///< 业务类型
    char side;                                                 ///< 买卖方向 (ORDER_SIDE_*, AgwSide 字典)
    int64_t price;                                             ///< 委托价格, 放大10000
    std::array<char, MAX_ACCOUNT_ID_LEN> sh_account_id;        ///< 沪A股东代码
    std::array<char, MAX_ACCOUNT_ID_LEN> sz_account_id;        ///< 深A股东代码
  };

  /// 最大可委托数查询应答(单条, AgwMsgExtQueryMaxOrderQtyResult, 310001)
  class CustMaxOrderQtyInfo
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 股东账号
    int64_t max_order_qty;                                     ///< 最大可委托数, 放大100倍(股数×100, RUL-35)
    int64_t theoretical_order_qty;                             ///< 理论可委托数, 放大100倍(股数×100, RUL-35)
    int64_t odd_order_qty;                                     ///< 港股通零股卖出最大可委托数, 放大100倍(股数×100, RUL-35)
    uint8_t limit_flag;                                        ///< 减持受限标识
    int64_t jj_max_limit_qty;                                  ///< 减持受限名单内的竞价卖出可用额度(数量), 放大100倍(股数×100, RUL-35)
  };

  /// 客户总资产查询请求 (AgwMsgExternalQueryTotalFundAsset, 7301735)
  class TotalFundAssetQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
  };

  /// 客户总资产查询应答(单条, AgwMsgExtQueryResultTotalFundAsset, 7401735; 按币种一条)
  class CustTotalFundAssetInfo
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, 4> currency;                              ///< 币种
    int64_t fund_bal;                                          ///< 资金余额, 放大10000
    int64_t fund_avl;                                          ///< 资金可用, 放大10000
    int64_t fund_asset_total_new;                              ///< 总资产新, 放大10000
    int64_t fund_asset_total;                                  ///< 总资产, 放大10000
    int64_t fund_asset;                                        ///< 资金资产, 放大10000
    int64_t fund_asset_ggt;                                    ///< 港股通T+1日未交收资金, 放大10000
    int64_t fund_asset_stktotal;                               ///< 证券总资产, 放大10000
    int64_t fund_asset_stk;                                    ///< 证券资产, 放大10000
    int64_t fund_asset_bond_total;                             ///< 债券总资产, 放大10000
    int64_t fund_asset_bond;                                   ///< 债券资产, 放大10000
    int64_t fund_asset_rzgh;                                   ///< 国债正回购, 放大10000
    int64_t fund_asset_rqgh;                                   ///< 国债逆回购, 放大10000
    int64_t fund_asset_stkzy;                                  ///< 债券质押资产(回购资产), 放大10000
    int64_t fund_asset_rzcs;                                   ///< 协议融资, 放大10000
    int64_t fund_asset_rqcs;                                   ///< 协议融券, 放大10000
    int64_t fund_asset_stkxyzy;                                ///< 协议债券质押资产, 放大10000
    int64_t fund_asset_adjust;                                 ///< 待交收证券资产, 放大10000
    int64_t fund_asset_oftotal;                                ///< 理财总资产, 放大10000
    int64_t fund_asset_of;                                     ///< 基金资产, 放大10000
    int64_t fund_asset_bjhgsh;                                 ///< 上海报价回购, 放大10000
    int64_t fund_asset_bjhgsz;                                 ///< 深圳报价回购, 放大10000
    int64_t fund_asset_ofuncome;                               ///< 基金认申购定投未确认在途资金, 放大10000
    int64_t fund_asset_xjgj;                                   ///< 现金管家, 放大10000
    int64_t fund_asset_zfzz;                                   ///< 支付转账, 放大10000
    int64_t fund_asset_jdlsec;                                 ///< 君得利二号, 放大10000
    int64_t fund_asset_fjjj;                                   ///< 分级基金赎回日提前释放, 放大10000
    int64_t fund_asset_curdayrqgh;                             ///< 融券购回当日到期, 放大10000
    int64_t fund_asset_curdayrzgh;                             ///< 融资购回当日到期, 放大10000
    int64_t fund_asset_otc_end;                                ///< OTC当前到期, 放大10000
    int64_t debts_total;                                       ///< 总负债, 放大10000
    int64_t zy_mtkvalue;                                       ///< 应收股票质押市值, 放大10000
    int64_t zy_debts;                                          ///< 股票质押负债, 放大10000
    int64_t en_mktvalue;                                       ///< 应收约定购回市值, 放大10000
    int64_t en_debts;                                          ///< 约定购回负债, 放大10000
    int64_t zywy_mktvalue;                                     ///< 股票质押违约卖出市值, 放大10000
    int64_t fund_asset_matchrqgh;                              ///< 国债逆回购当日成交, 放大10000
    int64_t sum_profit_cost;                                   ///< 证券总成本, 放大10000
    int64_t sump_rofit_amt;                                    ///< 证券资产总浮动盈亏, 放大10000
    int64_t fund_asset_stkwxxj;                                ///< 网下现金认购资产, 放大10000
    int64_t fund_asset_adjust_xsf;                             ///< 新三板新股申购在途资金, 放大10000
    int64_t fund_asset_adjust_qt;                              ///< 新股配债在途资金, 放大10000
    int64_t fund_asset_adjust_hgt;                             ///< 港股通在途资金, 放大10000
    int64_t fund_asset_adjust_lof;                             ///< lof在途资金, 放大10000
    int64_t fund_asset_stk_only;                               ///< 股票资产, 放大10000
    int64_t fund_asset_adjust_etf;                             ///< etf在途资金, 放大10000
    int64_t trade_date;                                        ///< 当前资产交易日期, 透传
  };

  /// 可取资金查询请求 (AgwMsgExtQueryFundMaxdraw, 7390701)
  class FundMaxdrawQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, 4> currency;                              ///< 货币种类
  };

  /// 可取资金查询应答(单条, AgwMsgExtQueryResultFundMaxdraw, 7490701; 按币种一条)
  class CustFundMaxdrawInfo
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, 4> currency;                              ///< 货币种类
    int64_t fund_bal;                                          ///< 资金余额, 放大10000
    int64_t fund_avl;                                          ///< 资金可用金额, 放大10000
    int64_t fund_buy;                                          ///< 买入冻结, 放大10000
    int64_t fund_sale;                                         ///< 卖出解冻, 放大10000
    int64_t market_value;                                      ///< 资产总值, 放大10000
    int64_t fund_all;                                          ///< 资金资产, 放大10000
    uint8_t fund_seq;                                          ///< 主资金标志
    int64_t stk_value;                                         ///< 市值, 放大10000
    int64_t max_draw;                                          ///< 可取金额, 放大10000
    std::array<char, 5> bank_code;                             ///< 银行代码
    int64_t fund_bjhg_avl;                                     ///< 报价回购可用资金, 放大10000
    char credit_flag;                                          ///< 资金帐号属性
    int64_t stk_value_non_of_cash;                             ///< 含开放式基金不含现金管理产品的市值, 放大10000
    int64_t of_cash;                                           ///< 现金管理产品的市值, 放大10000
  };

  /// 融资融券信用数据(可用保证金)查询请求 (AgwMsgExtQueryCustMarginAvl, 7390724)
  class CustMarginAvlQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, 4> currency;                              ///< 货币种类
    int16_t market_type;                                       ///< 交易市场 (101/102/109)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    char credit_direct;                                        ///< 信用方向
  };

  /// 融资融券信用数据(可用保证金)查询应答(单条, AgwMsgExtQueryResultCustMarginAvl, 7490724)
  class CustMarginAvlInfo
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    char credit_direct;                                        ///< 信用方向
    int64_t margin_avl;                                        ///< 保证金可用余额, 放大10000
    int64_t credit_bal;                                        ///< 融资/融券授信额度, 放大10000
    int64_t credit_avl;                                        ///< 融资/融券可用的额度, 放大10000
    int64_t margin_rate;                                       ///< 融资/融券保证金比例, ATP N12(8)=×10^8 透传
    int64_t debts_bal;                                         ///< 应付融资款/应付融券市值, 放大10000
    int64_t max_amt;                                           ///< 最大融资/融券金额, 放大10000
    int64_t fund_avl;                                          ///< 资金可用数, 放大10000
    int64_t credit_fund_avl;                                   ///< 融券卖出资金, 放大10000
    int64_t g_dis_market_avl;                                  ///< 担保证券的折算价值, 放大10000
    int64_t mar_stk_val;                                       ///< ∑(充抵保证金的证券市值*折算率), 放大10000
    int64_t total_dis_f_mk_val;                                ///< ∑融资的市值*折算率, 放大10000
    int64_t mar_fund_profit;                                   ///< ∑融资盈亏*折算率, 放大10000
    int64_t mar_stk_profit;                                    ///< ∑融券盈亏*折算率, 放大10000
    int64_t total_rate_f_debts_bal;                            ///< ∑融资金额*融资保证金比例, 放大10000
    int64_t mar_fund_margin;                                   ///< 已占用融资保证金, 放大10000
    int64_t mar_fund_margin_frz;                               ///< 委托冻结融资保证金, 放大10000
    int64_t total_rate_d_sale_bal;                             ///< ∑融券市值*融券保证金比例, 放大10000
    int64_t mar_stk_margin;                                    ///< 已占用融券保证金, 放大10000
    int64_t mar_stk_margin_frz;                                ///< 委托冻结融券保证金, 放大10000
    int64_t total_fee;                                         ///< 利息和费用总和, 放大10000
    int16_t market_type;                                       ///< 交易市场 (101/102/109)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    std::array<char, 40> security_symbol;                      ///< 证券简称
    int64_t margin_fund_bal;                                   ///< 资金余额(保证金公式部分), 放大10000
    int64_t margin_credit_bal;                                 ///< 融券卖出冻结资金(保证金公式部分), 放大10000
    int64_t fund_margin_avl;                                   ///< 资金账号级保证金可用金额(不考虑安全垫影响), 放大10000
    int64_t fund_margin_avl_sub;                               ///< 资金账号级保证金可用金额(考虑安全垫影响), 放大10000
  };

  /// 资产负债查询请求 (AgwMsgExtQueryAssetDebts, 7390700)
  class AssetDebtsQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, 4> currency;                              ///< 货币种类
  };

  /// 资产负债查询应答(单条, AgwMsgExtQueryResultAssetDebts, 7490700)
  class AssetDebtsInfo
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, 4> currency;                              ///< 货币种类
    std::array<char, 64> cust_name;                            ///< 客户姓名
    char credit_status;                                        ///< 信用状态
    int64_t keep_rate;                                         ///< 维持担保比例, ATP N12(8)=×10^8 透传
    int64_t fund;                                              ///< 资金, 放大10000
    int64_t fund_asset;                                        ///< 资金资产, 放大10000
    int64_t stk_asset;                                         ///< 证券资产, 放大10000
    int64_t bond_asset;                                        ///< 债券资产, 放大10000
    int64_t asset;                                             ///< 总资产(折算后), 放大10000
    int64_t old_asset;                                         ///< 原总资产, 放大10000
    int64_t liability;                                         ///< 总负债, 放大10000
    int64_t fund_bal;                                          ///< 资金余额, 放大10000
    int64_t fund_avl;                                          ///< 资金可用数, 放大10000
    int64_t fund_avl_frz;                                      ///< 资金可用冻结数, 放大10000
    int64_t d_sale_amt_bal;                                    ///< 融券卖出资金余额, 放大10000
    int64_t d_sale_amt_frz;                                    ///< 融券卖出资金冻结数, 放大10000
    int64_t credit_buy;                                        ///< 买券还券使用成交, 放大10000
    int64_t credit_sale;                                       ///< 融券卖出成交, 放大10000
    int64_t credit_buy_sale;                                   ///< 融券卖出及买券还券差额, 放大10000
    int64_t relese_amt_unfrz;                                  ///< 当日累计释放, 放大10000
    int64_t credit_amt;                                        ///< 当日累计释放(息费偿还部分), 放大10000
    int64_t hbjj_bal;                                          ///< 使用融券卖出资金购买货基的昨日存量, 放大10000
    int64_t hbjj_buy;                                          ///< 使用融券卖出资金当日买入货基冻结, 放大10000
    int64_t hbjj_sale;                                         ///< 专用资金货基当日退还, 放大10000
    int64_t hbjj_amt;                                          ///< 专用资金货基累计占用, 放大10000
    int64_t hbjj_remain_amt;                                   ///< 专用资金货基剩余占用, 放大10000
    int64_t gage_mkt_avl;                                      ///< 担保证券市值, 放大10000
    int64_t gage_mkt_avl_zs;                                   ///< 修正市值, 放大10000
    int64_t gage_mkt_avl_db;                                   ///< 担保持仓市值下的标的持仓市值, 放大10000
    int64_t rights_amt;                                        ///< 修正资产, 放大10000
    int64_t f_deal_avl;                                        ///< 应付融资款, 放大10000
    int64_t fund_fee;                                          ///< 融资利息, 放大10000
    int64_t fund_pro_fee;                                      ///< 融资承诺费, 放大10000
    int64_t fund_manage_fee;                                   ///< 融资管理费, 放大10000
    int64_t fund_over_due_fee;                                 ///< 融资逾期利息, 放大10000
    int64_t fund_puni_fee;                                     ///< 融资罚息, 放大10000
    int64_t fund_puni_debts;                                   ///< 融资逾期本金罚息, 放大10000
    int64_t fund_manage_over_due;                              ///< 融资逾期管理费, 放大10000
    int64_t fund_manage_puni_ntr;                              ///< 融资逾期管理费罚息, 放大10000
    int64_t f_fee;                                             ///< 融资息费合计, 放大10000
    int64_t f_total_debts;                                     ///< 融资负债合计, 放大10000
    int64_t deal_f_mkt_avl;                                    ///< 应付融券市值, 放大10000
    int64_t stk_fee;                                           ///< 融券息费, 放大10000
    int64_t stk_pro_fee;                                       ///< 融券承诺费, 放大10000
    int64_t stk_manage_fee;                                    ///< 融券管理费, 放大10000
    int64_t stk_over_due_fee;                                  ///< 融券逾期费用, 放大10000
    int64_t stk_puni_fee;                                      ///< 融券罚息, 放大10000
    int64_t over_due_rights;                                   ///< 融券权益逾期费用, 放大10000
    int64_t puni_rights;                                       ///< 融券权益罚息, 放大10000
    int64_t stk_puni_debts;                                    ///< 融券逾期本金罚息, 放大10000
    int64_t stk_manage_over_due;                               ///< 融券逾期管理费, 放大10000
    int64_t stk_manage_puni_ntr;                               ///< 融券逾期管理费罚息, 放大10000
    int64_t d_fee;                                             ///< 融券息费合计, 放大10000
    int64_t d_total_debts;                                     ///< 融券负债合计, 放大10000
    int64_t fund_draw;                                         ///< 可取资产, 放大10000
    int64_t fill_amt;                                          ///< 补仓金额, 放大10000
    int64_t credit_bal;                                        ///< 授信额度, 放大10000
    int64_t credit_avl;                                        ///< 授信可用的额度, 放大10000
    int64_t f_credit_bal;                                      ///< 融资授信额度, 放大10000
    int64_t f_credit_avl;                                      ///< 融资可用额度, 放大10000
    int64_t d_credit_bal;                                      ///< 融券授信额度, 放大10000
    int64_t d_credit_avl;                                      ///< 融券可用额度, 放大10000
    int64_t total_profit_cost;                                 ///< 总盈亏, 放大10000
    int64_t stk_unused_fee;                                    ///< 锁券占用费, 放大10000
    int64_t stk_puni_unused_fee;                               ///< 逾期锁券占用费罚息, 放大10000
    int64_t fund_margin_frz;                                   ///< 冻结融资保证金, 放大10000
    int64_t stk_margin_frz;                                    ///< 冻结融券保证金, 放大10000
    int64_t market_value;                                      ///< 资产总值, 放大10000
    uint8_t fund_seq;                                          ///< 资金序号
    int64_t stk_value;                                         ///< 所有持仓的证券市值, 放大10000
    int64_t d_sale_bal;                                        ///< 融券卖出资金余额, 放大10000
    int64_t credit_frz_amt;                                    ///< 买券还券委托冻结, 放大10000
    int64_t long_profit_cost;                                  ///< 多头盈亏, 放大10000
    int64_t short_profit_cost;                                 ///< 空头盈亏, 放大10000
    int64_t lever_rate;                                        ///< 杠杆率, ATP N12(8)=×10^8 透传
    int64_t asset_frz_package_out;                             ///< 划出股份冻结包市值, 放大10000
    int64_t asset_frz_package_in;                              ///< 划入股份冻结包市值, 放大10000
    int64_t trade_date;                                        ///< 当前资产交易日期, 透传
    int64_t max_draw;                                          ///< 可取金额, 放大10000
    int64_t max_draw2;                                         ///< 可取金额(不考虑维保), 放大10000
  };

  /// 两融最大可委托数查询请求 (AgwMsgExtQueryCreditMaxOrderQty, 7390710)
  class CreditMaxOrderQtyQueryReq
  {
  public:
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    int16_t market_type;                                       ///< 证券所属市场 (101/102/109)
    std::array<char, MAX_SECURITY_ID_LEN> security_id;         ///< 证券代码
    uint8_t business_type;                                     ///< 业务类型
    char side;                                                 ///< 买卖方向 (ORDER_SIDE_*, AgwSide 字典)
    int64_t price;                                             ///< 委托价格, 放大10000
  };

  /// 两融最大可委托数查询应答(单条, AgwMsgExtQueryCreditMaxOrderQtyResult, 7490710)
  class CreditMaxOrderQtyInfo
  {
  public:
    static constexpr int32_t MAX_ZERO_REASON_LEN = 128;        ///< 可委托数为0原因描述最大长度(含结尾\0)
    int64_t client_req_no;                                     ///< 客户请求号
    std::array<char, MAX_CUST_ID_LEN> cust_id;                 ///< 客户号
    std::array<char, MAX_FUND_ACCOUNT_ID_LEN> fund_account_id; ///< 客户资金账号
    std::array<char, MAX_BRANCH_ID_LEN> branch_id;             ///< 分支机构代码
    std::array<char, MAX_ACCOUNT_ID_LEN> account_id;           ///< 股东账号
    uint8_t business_type;                                     ///< 业务类型
    char side;                                                 ///< 买卖方向 (ORDER_SIDE_*, AgwSide 字典)
    int64_t max_order_qty;                                     ///< 最大可委托数, 放大100倍(股数×100, RUL-35)
    int64_t theoretical_order_qty;                             ///< 理论可委托数, 放大100倍(股数×100, RUL-35)
    int64_t max_pay_qty;                                       ///< 买券还券应补数量, 放大100倍(股数×100, RUL-35)
    int64_t max_pay_amt;                                       ///< 应偿还资金负债, 放大10000
    int64_t sep_rem_qty;                                       ///< 专户余券, 放大100倍(股数×100, RUL-35)
    int64_t fund_credit_avl;                                   ///< 买券还券可用资金, 放大10000
    int64_t stk_debts_avl;                                     ///< 融券负债可用额度, 放大10000
    int64_t sg_max_stk_qty;                                    ///< 个股申购上限, 放大100倍(股数×100, RUL-35)
    std::array<char, MAX_ZERO_REASON_LEN> zero_reason;         ///< 可委托数为0原因描述 (定长化, 协议 String 截断)
    uint8_t limit_flag;                                        ///< 减持受限标识
    int64_t jj_max_limit_qty;                                  ///< 减持受限名单内的竞价卖出可用额度(数量), 放大100倍(股数×100, RUL-35)
  };

} // namespace lb_api

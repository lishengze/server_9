#pragma once
#include <array>
#include <cstdint>
#include <string>
#include "message/endian_util.h"
#include "common/external_types.h"

namespace message 
{

const uint32_t kPktNewHeartBeat = 3;
const uint32_t kPktLoginReq = 1001;
const uint32_t kPktLoginAns = 2001;
const uint32_t kPktLogoutReq = 1002;
const uint32_t kPktLogoutAns = 2002;
const uint32_t kPktOrderReq = 1003;
const uint32_t kPktCancelOrderReq = 1004;
const uint32_t kPktMarginTradingReq = 1005;
const uint32_t kPktCreditAuctionReq = 1006;
const uint32_t kPktETFReq = 1010;
const uint32_t kPktOrderAns = 2003;
const uint32_t kPktCancelOrderAns= 2004;
const uint32_t kPktOrderMatch = 2005;
const uint32_t kPktFundQuery = 1007;
const uint32_t kPktFundQueryRes = 2007;
const uint32_t kPktShareQuery = 1008;
const uint32_t kPktShareQueryRes = 2008;
const uint32_t kPktGatewayLoginReq = 1009;
const uint32_t kPktGatewayLoginAns = 2009;
const uint32_t kPktEtfOrderMatch = 2010;
const uint32_t kPktRejectMsg = 9;
const uint32_t kPktStrategyExit = 999999;

#pragma pack(push)
#pragma pack(1)

#ifndef UTE_API_CODE
class InternalOrder;
class InternalETFBasketReport;
#endif

// PktNewHeader: 消息头，独立类，不被嵌套
class PktNewHeader
{
public:
    uint32_t msg_id;
    uint32_t msg_len;
    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// ConstituentStock: 成分股信息，作为数组元素使用（不展开）
class ConstituentStock
{
public:
    uint16_t market_id;                                 // 市场
    std::array<char, 8> security_id;                    // 成分股证券代码
    int64_t order_qty;                                  // 股份交付数量
    int64_t subst_cash;                                 // 现金替代金额
    int64_t price;                                      // 成交价格
    uint8_t etf_trade_report_type;                      // ETF成交回报类型
    std::array<char, 16> exec_id;                       // 执行编号
    char margin_amt_type;                               // 上交所现金替代资金类型

    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
    void reset(){};
#ifndef UTE_API_CODE
    void FillWithInternalETFBasketReport(InternalETFBasketReport& etf_basket_report);
#endif
};

// LogOnReq: 登录请求（TradeOrderUser 已展开）
class LogOnReq
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    uint32_t heart_bt_int;
    std::array<char, 100> password;
    std::array<char, 1024> client_feature_code;
    std::array<char, 32> agw_user;

    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// GatewayLogOnReq: 网关登录请求（无嵌套对象）
class GatewayLogOnReq
{
public:
    uint32_t heart_bt_int;
    std::array<char, 64> password;
    std::array<char, 64> agw_user;

    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// LogOnAns: 登录应答（TradeOrderUser 已展开）
class LogOnAns
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    int32_t session_status;
    uint32_t error_code;
    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// GatewayLogOnAns: 网关登录应答（无嵌套对象）
class GatewayLogOnAns
{
public:
    std::array<char, 64> agw_user;
    int32_t session_status;
    uint32_t error_code;
    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// LogOutReq: 登出请求（TradeOrderUser 已展开）
class LogOutReq 
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    std::array<char, 100> password;
    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// LogOutAns: 登出应答（TradeOrderUser 已展开）
class LogOutAns
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    int32_t session_status;
    uint32_t error_code;
    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// TradeOrderReq: 委托请求（TradeOrderUser、TradeOrderInfo 已展开）
class TradeOrderReq 
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    // TradeOrderInfo 展开开始
    std::array<char, 8> security_id;
    uint16_t market_id;
    char side;
    char order_type;
    int64_t order_qty;
    int64_t order_price;
    int64_t stop_px;
    // TradeOrderInfo 展开结束

    void reset();
    bool decode(char* data, size_t len);
    bool encode();
    bool decode();
    std::string dump();
};

// CancelOrderReq: 撤单请求（TradeOrderUser、CancelOrderInfo 已展开）
class CancelOrderReq
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    // CancelOrderInfo 展开开始
    int64_t orig_client_seq_id;
    int64_t orig_clordno;
    // CancelOrderInfo 展开结束
    
    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
};

// TradeOrderER: 委托执行报告（TradeOrderUser、OrdERInfo 已展开）
// 注意：OrdERInfo 中的 ConstituentStock constituent_stock[0] 是数组，不展开
class TradeOrderER
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    // OrdERInfo 展开开始
    std::array<char, 16> order_id;
    std::array<char, 10> clordid;
    std::array<char, 8> security_id;
    uint16_t market_id;
    char exec_type;
    uint8_t ord_status;
    int64_t price;
    int64_t order_qty;
    int64_t leaves_qty;
    int64_t cum_qty;
    char side;
    int64_t transact_time;
    std::array<char, 64> user_info;
    std::array<char, 16> exec_id;
    std::array<char, 10> orig_clordid;
    char ord_type;
    uint16_t ord_rej_reason;            // 交易所错误码
    char time_in_force;
    int64_t last_px;
    int64_t last_qty;
    char cash_margin;
    char cancel_flag;
    int64_t clordno;
    int64_t orig_clordno;
    int64_t index;
    uint16_t code;                      // 内部错误码
    int64_t frozen_trade_value;         // 冻结交易金额
    int64_t frozen_fee;                 // 冻结费用
    int64_t fee;                        // 单笔成交费用
    int64_t total_value_traded;         // 成交金额
    uint8_t business_type;              // 业务类型
    uint32_t no_security;               // 成分券数量
    ConstituentStock constituent_stock[0];  // 成分股数组（数组，不展开）
    // OrdERInfo 展开结束

    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
    void reset(){};

#ifndef UTE_API_CODE
    void FillWithInternalOrderInfo(InternalOrder* internal_order);
    void FillInternalOrder(InternalOrder* internal_order);
#endif
};

// RejectMsg: 拒绝返回消息（TradeOrderUser 已展开）
class RejectMsg
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    uint16_t reject_reason_code;
    char cancel_flag;
    uint8_t business_type;              // 业务类型

    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
    void reset(){};
};

// PktFundQuery: 资金查询（TradeOrderUser 已展开）
class PktFundQuery
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束

    bool encode();
    bool decode(char* data, size_t len);
    std::string dump();
    void reset();
};

// PktFundQueryRes: 资金查询应答（TradeOrderUser 已展开）
class PktFundQueryRes
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束

    uint16_t error_code;
    std::array<char, 6> offer_pbu;
    std::array<char, 10> offer_branch_id;
    int64_t t0;
    int64_t t1;
    int64_t t2;
    int64_t t3;

    void reset();
    void FillWithQuery(PktFundQuery query);
    bool encode();
    std::string dump();
};

// PktShareQuery: 股份查询（TradeOrderUser 已展开）
class PktShareQuery
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束

    std::array<char, 8> security_id;
    uint16_t market_id;

    bool decode(char* data, size_t len);
    bool encode();
    std::string dump();
    void reset();
};

// PktShareQueryRes: 股份查询应答（TradeOrderUser、SingleShareRecord 已展开）
class PktShareQueryRes
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束

    uint32_t error_code;
    uint32_t total_len;
    // SingleShareRecord 展开开始
    std::array<char, 8> security_id;
    uint16_t market_id;
    int64_t stk_avl_;           // 日间股份可用 ShareType-1
    int64_t stk_buy_;           // 当日买入 ShareType-3
    int64_t stk_subscribe_;     // 当日申赎股份 ShareType-4
    int64_t stk_subscribe_ph_;  // 当日实物申赎股份 ShareType-5
    int64_t stk_block_buy_;     // 当日大宗买入股份 ShareType-10
    int64_t stk_other_;         // 日间不可用股份，总资产计算用
    // SingleShareRecord 展开结束

    bool encode();
    std::string dump();
    void FillWithQuery(PktShareQuery query);
    void reset();
};

#pragma pack(pop)

} // namespace message

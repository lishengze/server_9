#pragma once

// ============================================================================
// gw_head.h
// FTE 对外接口数据结构（扁平化、纯标准库依赖版本）
//
// 说明：
//   1. 所有自定义类型（FundAccountID_def 等）均已替换为对应的内置类型。
//   2. 每个数据结构仅保留 reset() 与 dump() 接口，并已在头文件内完成实现。
//   3. 本头文件仅依赖 C++ 标准库，不依赖项目内部任何头文件。
//   4. 不含查询接口相关的请求回报结构（PktFundQuery / PktShareQuery 等）。
//   5. dump() 输出格式为：每行一个属性，形如 "属性名: 属性值\n"。
// ============================================================================

#include <array>
#include <cstdint>
#include <string>
#include <sstream>

namespace gw_message
{

// 消息类型常量
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
const uint32_t kPktCancelOrderAns = 2004;
const uint32_t kPktOrderMatch = 2005;
const uint32_t kPktGatewayLoginReq = 1009;
const uint32_t kPktGatewayLoginAns = 2009;
const uint32_t kPktEtfOrderMatch = 2010;
const uint32_t kPktRejectMsg = 9;
const uint32_t kPktStrategyExit = 999999;

// 辅助函数：将 std::array<char, N> 转换为 std::string
template <size_t Size>
inline std::string AsString(const std::array<char, Size>& from)
{
    return std::string(&from[0], Size);
}

#pragma pack(push)
#pragma pack(1)

// PktNewHeader: 消息头，独立类，不被嵌套
class PktNewHeader
{
public:
    uint32_t msg_id;
    uint32_t msg_len;

    void reset()
    {
        msg_id = 0;
        msg_len = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "msg_id: " << msg_id << "\n"
           << "msg_len: " << msg_len << "\n";
        return os.str();
    }
};

// ConstituentStock: 成分股信息，作为数组元素使用（不展开）
class ConstituentStock
{
public:
    uint16_t market_id;                         // 市场
    std::array<char, 8> security_id;            // 成分股证券代码
    int64_t order_qty;                          // 股份交付数量
    int64_t subst_cash;                         // 现金替代金额
    int64_t price;                              // 成交价格
    uint8_t etf_trade_report_type;              // ETF成交回报类型
    std::array<char, 16> exec_id;               // 执行编号
    char margin_amt_type;                       // 上交所现金替代资金类型

    void reset()
    {
        market_id = 0;
        security_id.fill(' ');
        order_qty = 0;
        subst_cash = 0;
        price = 0;
        etf_trade_report_type = 0;
        exec_id.fill(' ');
        margin_amt_type = ' ';
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "market_id: " << static_cast<int>(market_id) << "\n"
           << "security_id: " << AsString(security_id) << "\n"
           << "order_qty: " << order_qty << "\n"
           << "subst_cash: " << subst_cash << "\n"
           << "price: " << price << "\n"
           << "etf_trade_report_type: " << static_cast<int>(etf_trade_report_type) << "\n"
           << "exec_id: " << AsString(exec_id) << "\n"
           << "margin_amt_type: " << margin_amt_type << "\n";
        return os.str();
    }
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

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        heart_bt_int = 0;
        password.fill(' ');
        client_feature_code.fill(' ');
        agw_user.fill(' ');
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "heart_bt_int: " << heart_bt_int << "\n"
           << "password: " << AsString(password) << "\n"
           << "client_feature_code: " << AsString(client_feature_code) << "\n"
           << "agw_user: " << AsString(agw_user) << "\n";
        return os.str();
    }
};

// GatewayLogOnReq: 网关登录请求（无嵌套对象）
class GatewayLogOnReq
{
public:
    uint32_t heart_bt_int;
    std::array<char, 64> password;
    std::array<char, 64> agw_user;

    void reset()
    {
        heart_bt_int = 0;
        password.fill(' ');
        agw_user.fill(' ');
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "heart_bt_int: " << heart_bt_int << "\n"
           << "password: " << AsString(password) << "\n"
           << "agw_user: " << AsString(agw_user) << "\n";
        return os.str();
    }
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

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        session_status = 0;
        error_code = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "session_status: " << session_status << "\n"
           << "error_code: " << error_code << "\n";
        return os.str();
    }
};

// GatewayLogOnAns: 网关登录应答（无嵌套对象）
class GatewayLogOnAns
{
public:
    std::array<char, 64> agw_user;
    int32_t session_status;
    uint32_t error_code;

    void reset()
    {
        agw_user.fill(' ');
        session_status = 0;
        error_code = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "agw_user: " << AsString(agw_user) << "\n"
           << "session_status: " << session_status << "\n"
           << "error_code: " << error_code << "\n";
        return os.str();
    }
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

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        password.fill(' ');
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "password: " << AsString(password) << "\n";
        return os.str();
    }
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

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        session_status = 0;
        error_code = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "session_status: " << session_status << "\n"
           << "error_code: " << error_code << "\n";
        return os.str();
    }
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
    uint16_t market_id;                 // 市场ID
    char side;
    char order_type;
    int64_t order_qty;
    int64_t order_price;
    int64_t stop_px;
    // TradeOrderInfo 展开结束

    uint16_t policy_id;  // 策略佣金ID
    uint16_t tgw_id;     // TGW 编号

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        security_id.fill(' ');
        market_id = 0;
        side = ' ';
        order_type = ' ';
        order_qty = 0;
        order_price = 0;
        stop_px = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "security_id: " << AsString(security_id) << "\n"
           << "market_id: " << static_cast<int>(market_id) << "\n"
           << "side: " << side << "\n"
           << "order_type: " << order_type << "\n"
           << "order_qty: " << order_qty << "\n"
           << "order_price: " << order_price << "\n"
           << "stop_px: " << stop_px << "\n";
        return os.str();
    }
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

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        orig_client_seq_id = 0;
        orig_clordno = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "orig_client_seq_id: " << orig_client_seq_id << "\n"
           << "orig_clordno: " << orig_clordno << "\n";
        return os.str();
    }
};

// TradeOrderER: 委托执行报告（TradeOrderUser、OrdERInfo 已展开）
// 注意：constituent_stock[0] 是零长度数组（成分券），不展开
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

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        order_id.fill(' ');
        clordid.fill(' ');
        security_id.fill(' ');
        market_id = 0;
        exec_type = ' ';
        ord_status = 0;
        price = 0;
        order_qty = 0;
        leaves_qty = 0;
        cum_qty = 0;
        side = ' ';
        transact_time = 0;
        user_info.fill(' ');
        exec_id.fill(' ');
        orig_clordid.fill(' ');
        ord_type = ' ';
        ord_rej_reason = 0;
        time_in_force = ' ';
        last_px = 0;
        last_qty = 0;
        cash_margin = ' ';
        cancel_flag = ' ';
        clordno = 0;
        orig_clordno = 0;
        index = 0;
        code = 0;
        frozen_trade_value = 0;
        frozen_fee = 0;
        fee = 0;
        total_value_traded = 0;
        business_type = 0;
        no_security = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "order_id: " << AsString(order_id) << "\n"
           << "clordid: " << AsString(clordid) << "\n"
           << "security_id: " << AsString(security_id) << "\n"
           << "market_id: " << static_cast<int>(market_id) << "\n"
           << "exec_type: " << exec_type << "\n"
           << "ord_status: " << static_cast<uint16_t>(ord_status) << "\n"
           << "price: " << price << "\n"
           << "order_qty: " << order_qty << "\n"
           << "leaves_qty: " << leaves_qty << "\n"
           << "cum_qty: " << cum_qty << "\n"
           << "side: " << side << "\n"
           << "transact_time: " << transact_time << "\n"
           << "user_info: " << AsString(user_info) << "\n"
           << "exec_id: " << AsString(exec_id) << "\n"
           << "orig_clordid: " << AsString(orig_clordid) << "\n"
           << "ord_type: " << ord_type << "\n"
           << "ord_rej_reason: " << ord_rej_reason << "\n"
           << "time_in_force: " << time_in_force << "\n"
           << "last_px: " << last_px << "\n"
           << "last_qty: " << last_qty << "\n"
           << "cash_margin: " << cash_margin << "\n"
           << "cancel_flag: " << cancel_flag << "\n"
           << "clordno: " << clordno << "\n"
           << "orig_clordno: " << orig_clordno << "\n"
           << "index: " << index << "\n"
           << "code: " << code << "\n"
           << "frozen_trade_value: " << frozen_trade_value << "\n"
           << "frozen_fee: " << frozen_fee << "\n"
           << "fee: " << fee << "\n"
           << "total_value_traded: " << total_value_traded << "\n"
           << "business_type: " << static_cast<uint16_t>(business_type) << "\n"
           << "no_security: " << no_security << "\n";
        return os.str();
    }
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

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        reject_reason_code = 0;
        cancel_flag = ' ';
        business_type = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "reject_reason_code: " << reject_reason_code << "\n"
           << "cancel_flag: " << cancel_flag << "\n"
           << "business_type: " << static_cast<uint16_t>(business_type) << "\n";
        return os.str();
    }
};

#pragma pack(pop)

} // namespace message

#include <array>
#include <string>
#include <vector>   
#include <map>
#include <cstdint>


/*
 * @brief 报告公共字段
 */
class  AgwMsgOrderReport 
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
};
/*
 * @brief 成交执行报告
 */
class  AgwMsgTradeExecutionReport : public AgwMsgOrderReport
{
public:
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
};
/*
 * @brief 成交执行报告client_seq_id
 */
class  AgwMsgTradeExecutionReportSeqNum 
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别

std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用

int64_t client_seq_id; 		///< 用户系统消息序号
};

/*
 * @brief 质押式回购集中竞价业务成交回报 210215
 */
class  AgwMsgRepoAuctionTradeER {
public:
int64_t maturity_date; 		///< 到期日
int64_t client_seq_id; 		///< 用户系统消息序号
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
};



/*
 * @brief 大宗盘后定价业务成交回报 2006015
 */
class  AgwMsgAfterHoursPricingTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号

// 自有属性
uint8_t price_property; 		///< 价格类型
char cash_margin; 		///< 信用标识
};



/*
 * @brief 开放式基金申赎成交回报 211715
 */
class  AgwMsgLOFTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号
};




/*
 * @brief 信用账户普通买入/普通卖出成交回报 7200715
 */
class  AgwMsgCreditAuctionTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号
};

/*
 * @brief 融资买入/融券卖出成交回报 7201715
 */
class  AgwMsgMarginTradingTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号
};




/*
 * @brief 买券还券成交回报 
 */
class  AgwMsgBuySecuRepayTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号
};



/*
 * @brief 信用投票业务应答 7207702
 */
class  AgwMsgCreditVoteResult 
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint16_t reject_reason_code; 		///< 订单拒绝原因
std::string reject_desc; 		///< 拒绝原因描述
int64_t transact_time; 		///< 回报时间
int64_t client_seq_id; 		///< 用户系统消息序号
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> account_id; 		///< 账户ID
std::array<char, 64> user_info; 		///< 用户私有信息
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
};



/*
 * @brief 指定合约还款应答 7208702
 */
class  AgwMsgExtDirectRepayAppointResult 
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint16_t reject_reason_code; 		///< 订单拒绝原因
std::string reject_desc; 		///< 拒绝原因描述
int64_t transact_time; 		///< 回报时间
int64_t client_seq_id; 		///< 用户系统消息序号
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> account_id; 		///< 账户ID
std::array<char, 4> currency; 		///< 货币种类
int64_t repay_value; 		///< 还款金额
int64_t real_repay_value; 		///< 实际还款金额
std::array<char, 64> user_info; 		///< 用户私有信息
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
};



/*
 * @brief 股转协议转让定价申报成交回报 7220715
 */
class  AgwMsgThirdPricingTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号
};



/*
 * @brief 新三板买卖成交回报 7223715
 */
class  AgwMsgThirdCashAuctionTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号
};



/*
 * @brief 股转大宗交易成交确认申报成交回报 7224715
 */
class  AgwMsgThirdBlockTradeTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号

// 自有属性
std::array<char, 6> target_pbu_id; 		///< 对手方PBUID
std::array<char, 12> target_account_id; 		///< 对手方股东账户
std::array<char, 8> promise_sno; 		///< 约定号
};



/*
 * @brief 新三板买卖成交回报(两网退市股票) 7226715
 */
class  AgwMsgThirdCashAuctionDelistTradeER
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint8_t business_type; 		///< 业务类型
int64_t client_order_no; 		///< 客户订单号
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
char exec_type; 		///< 当前委托执行类型
uint8_t order_status; 		///< 当前申报的状态
std::array<char, 16> cust_id; 		///< 客户号ID
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 12> account_id; 		///< 投资者帐号
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 委托数量
int64_t leaves_qty; 		///< 未成交数量
int64_t cum_qty; 		///< 累计成交数量
char side; 		///< 买卖方向
int64_t transact_time; 		///< 回报时间
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 16> order_id; 		///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
std::array<char, 32> exec_id; 		///< 成交编号
int64_t last_px; 		///< 成交价格
int64_t last_qty; 		///< 成交数量
int64_t total_value_traded; 		///< 成交金额
int64_t fee; 		///< 费用
int64_t client_seq_id; 		///< 用户系统消息序号
};



/*
 * @brief 按证券账户划转资金响应消息 810002
 */
class  AgwExternalInsTETransFundResp 
{
public:
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t external_ins_error_code; 		///< 外部指令错误代码
int64_t client_seq_id; 		///< 用户系统消息序号
std::array<char, 64> user_info; 		///< 用户私有信息
};



/*
 * @brief 信用账户按证券账户划转资金响应消息 821001
 */
class  AgwExternalInsTETransCreditFundResp 
{
public:
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t external_ins_error_code; 		///< 外部指令错误代码
int64_t client_seq_id; 		///< 用户系统消息序号
std::array<char, 64> user_info; 		///< 用户私有信息
int64_t agw_seq_id; 		///< 业务网关序号，交易系统全局唯一
};



/*
 * @brief 信用账户按证券账户划转保证金余额响应消息 821004
 */
class  AgwExternalInsTETransCreditMarginAvlResp 
{
public:
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t external_ins_error_code; 		///< 外部指令错误代码
int64_t client_seq_id; 		///< 用户系统消息序号
std::array<char, 64> user_info; 		///< 用户私有信息
int64_t agw_seq_id; 		///< 业务网关序号，交易系统全局唯一
};



/*
 * @brief 直接还款应答  7202702
 */
class  AgwMsgExtDirectRepayResult 
{
public:
uint8_t partition; 		///< 分区号
int32_t index; 		///< 索引号
uint16_t reject_reason_code; 		///< 订单拒绝原因
std::string reject_desc; 		///< 拒绝原因描述
int64_t transact_time; 		///< 回报时间
int64_t client_seq_id; 		///< 用户系统消息序号
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> account_id; 		///< 账户ID
std::array<char, 4> currency; 		///< 货币种类
int64_t repay_value; 		///< 还款金额
int64_t real_repay_value; 		///< 实际还款金额
std::array<char, 64> user_info; 		///< 用户私有信息
std::string key_error_msg; 		///< 错误信息
std::string key_error_code; 		///< 错误代码串
uint8_t key_error_level; 		///< 错误级别
};
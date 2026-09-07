
#include <array>
#include <string>
#include <vector>   
#include <map>

/*
<Value name="Pledge" description="质押式回购交易">2</Value>
<Value name="BlockTrade" description="协议交易(大宗交易)">5</Value>
<Value name="AfterHoursPricing" description="盘后定价交易(大宗交易)">6</Value>
<Value name="Issue" description="网上发行认购">13</Value>
<Value name="RightsIssue" description="配股认购">14</Value>
<Value name="BondSellBack" description="债券转股回售">15</Value>
<Value name="OpenFundPurchaseRedemption" description="开放式基金申购赎回">17</Value>
<Value name="TenderOffer" description="要约收购">18</Value>
<Value name="Designation" description="转托管">20</Value>
<Value name="Vote" description="网络投票">21</Value>
<Value name="HKDesignationTransfer" description="港股通转托管">61</Value>
<Value name="HKCorporateAction" description="港股通公司行为">62</Value>
<Value name="HKStocksThrough" description="港股通交易">63</Value>
<Value name="HKVoting" description="港股通投票">64</Value>
<Value name="TibAfterHour" description="科创板股票盘后定价交易业务">97</Value>
<Value name="CreditAuction" description="信用账户普通买入/普通卖出（卖券还款）交易业务">200</Value>
<Value name="MarginTrading" description="融资买入/融券卖出交易业务">201</Value>
<Value name="DirectRepay" description="直接还款">202</Value>
<Value name="CollateralInOrOut" description="担保品转入/转出">203</Value>
<Value name="BuySecuRepay" description="买券还券">204</Value>
<Value name="CurSecuRepay" description="现券还券">205</Value>
<Value name="RemainSecTransform" description="余券划转">206</Value>
<Value name="CreditVote" description="信用账户投票">207</Value>
<Value name="DirectRepayAppoint" description="指定直接还款">208</Value>
<Value name="ThirdAgreementTransfer" description="股转协议转让">209</Value>
<Value name="ThirdTenderOffer" description="股转要约回购">210</Value>
<Value name="ThirdIssue" description="股转网上发行">211</Value>
<Value name="ThirdCashAuction" description="新三板集中交易(挂牌做市, 股转挂牌连续竞价, 挂牌集合竞价)">212</Value>
<Value name="ThirdBlockTrade" description="股转大宗交易">213</Value>
<Value name="ThirdCashAuctionDelist" description="新三板集中交易(两网公司及退市公司股票)">214</Value>
<Value name="OfflineETFSubscribe" description="网下ETF发行认购">215</Value>

<Message name="PktExternalInsTETransFund" pktno="810001" inherit="PktExternalIns" description="TE间按证券账户划拨资金消息">
<Message name="PktExternalInsTETransCreditFund" pktno="820001" inherit="PktExternalIns" description="信用账户TE间按证券账户划拨资金消息">
<Message name="PktExternalInsTETransCreditMarginAvl" pktno="820004" inherit="PktExternalIns" description="信用账户TE间按证券账户划拨保证金余额消息">


*/

/*
 * @brief 质押式回购集中竞价业务委托消息 100201
 */
class  AgwMsgRepoAuctionOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效时间类型
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 大宗意向申报委托消息 1005009  -- 无回报;
 */
class  AgwMsgBlockTradeIntentionOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


uint8_t trd_sub_type; 		///< 是否受限
std::array<char, 12> contactor; 		///< 联系人
std::array<char, 30> contact_info; 		///< 联系人信息
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};


/*
 * @brief 大宗盘后定价委托消息 1006001
 */
class  AgwMsgAfterHoursPricingOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


uint8_t price_property; 		///< 价格类型
char cash_margin; 		///< 信用标识
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};


/*
 * @brief 网上发行业务委托消息 101301 --todo 回报未知 --无回报 只有///< 网上发行认购执行报告;
 */
class  AgwMsgIssueOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 2> order_way_ext; 		///< 委托方式扩展
uint8_t special_ipo_type; 		///< 特殊发行类型
};


/*
 * @brief 配股业务委托消息 101401 --todo 回报未知; const MsgTypeNo_def kPktRightsIssueTER = 211415; ///< 配售业务成交回报
 */
class  AgwMsgRightsIssueOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 债券转股回售委托 101501 --todo 回报未知 -- 找不到
 */
class  AgwMsgSwapPutbackOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 2> share_negotiability; 		///< 股份性质
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};


/*
 * @brief 开放式基金申赎业务订单 101701
 */
class  AgwMsgLOFOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t cash_order_qty; 		///< 申购金额
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 要约收购业务消息 101801 --todo 回报未知;
 */
class  AgwMsgTenderOfferOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 6> tenderer; 		///< 要约收购参与人编码
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 网络投票业务消息 102101 --todo 回报未知;
 */
class  AgwMsgNetVotingOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


uint64_t general_meeting_seq; 		///< 股东大会编码
std::array<char, 10> voting_proposal; 		///< 投票议案编号
char voting_preference; 		///< 投票意向
std::array<char, 2> voting_segment; 		///< 分段统计段号（预留）
};

/*
 * @brief 港股通公司行为业务订单 106201 --todo 回报未知;
 */
class  AgwMsgHKCorporateActionOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 20> corporate_action_id; 		///< 公司行为代码
int64_t orig_cl_ord_no; 		///< 原订单编号(上海负数当日专用)
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 港股通业务委托消息 106301 --todo 回报未知;
 */
class  AgwMsgHKStocksThroughAuctionOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效期
char lot_type; 		///< 订单类型
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 港股通投票业务订单 106401 --todo 回报未知;
 */
class  AgwMsgHKVotingOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::string announcement_number; 		///< 公告编号
std::string proxy_form_number; 		///< 议案编号
std::string affirmative_votes; 		///< 赞成数量
std::string negative_votes; 		///< 反对数量
std::string abstention_votes; 		///< 弃权数量
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 科创板股票盘后定价业务委托消息 109701 --todo 回报未知;
 */
class  AgwMsgTibAfterHourAuctionOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效期
char cash_margin; 		///< 信用标识
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 信用账户普通买入/普通卖出业务委托消息 7100701
 */
class  AgwMsgCreditAuctionOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效期
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 融资买入/融券卖出业务委托消息 7101701
 */
class  AgwMsgMarginTradingOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效期
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 担保品划转业务委托消息 7103701 -- todo 回报未知;
 */
class  AgwMsgCollateralInOrOutOrder 
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 16> opp_fund_account_id; 		///< 对方资金账户ID
std::array<char, 12> opp_account_id; 		///< 对方证券账户ID
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 买券还券业务委托消息 7104701
 */
class  AgwMsgBuySecuRepayOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效期
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 现券还券业务委托消息 7105701 -- todo 回报未知;
 */
class  AgwMsgCurSecuRepayOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 信用投票业务消息 7107701
 */
class  AgwMsgCreditVote 
{
public:
int64_t time; 		///< 发起时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> account_id; 		///< 账户ID
std::string password; 		///< 密码
std::array<char, 4> currency; 		///< 货币种类
char order_way; 		///< 委托方式
int64_t order_qty; 		///< 投票数量
std::array<char, 8> security_id; 		///< 投票代码
uint16_t market_id; 		///< 市场类型
uint64_t general_meeting_seq; 		///< 股东大会编码
std::array<char, 10> voting_proposal; 		///< 投票议案编号
char voting_preference; 		///< 投票意向
std::string remark; 		///< 备注
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
std::array<char, 64> user_info; 		///< 用户私有信息
};

/*
 * @brief 指定合约还款请求 7108701
 */
class  AgwMsgExtDirectRepayAppoint 
{
public:
int64_t time; 		///< 发起时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> account_id; 		///< 账户ID
uint8_t is_account_partition; 		///< 账户是否按证券账户分区 1-是 0否
std::array<char, 4> currency; 		///< 货币种类
char order_way; 		///< 委托方式
int64_t contract_date; 		///< 合约委托日期
std::array<char, 11> contract_sno; 		///< 合约流水号
int64_t repay_value; 		///< 还款金额
std::string remark; 		///< 备注
std::string password; 		///< 密码
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 股转协议转让定价申报委托消息 7120701
 */
class  AgwMsgThirdPricingOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 股转要约收购业务消息 7121701 --- todo 回报未知;
 */
class  AgwMsgThirdTenderOfferOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 2> share_nature; 		///< 股份性质
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 新三板网上发行业务委托消息 7122701 --- todo 回报未知;
 */
class  AgwMsgThirdIssueOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 新三板买卖委托 7123701
 */
class  AgwMsgThirdCashAuctionOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效期
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 股转大宗交易成交确认申报委托消息 7124701
 */
class  AgwMsgThirdBlockTradeOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 6> target_pbu_id; 		///< 对手方PBUID
std::array<char, 12> target_account_id; 		///< 对手方股东账户
std::array<char, 8> promise_sno; 		///< 约定号
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 新三板买卖委托(两网退市股票) 7126701
 */
class  AgwMsgThirdCashAuctionDelistOrder
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


int64_t stop_px; 		///< 止损价
int64_t min_qty; 		///< 最低成交数量
uint16_t max_price_levels; 		///< 最多成交价位数
char time_in_force; 		///< 订单有效期
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief ETF网下认购（股票和现金） 7130701 --todo 回报未知;
 */
class  AgwMsgOfflineETFSubscribe
{
public:

uint8_t business_type; 		///< 业务类型
std::array<char, 8> security_id; 		///< 证券代码
uint16_t market_id; 		///< 市场ID
int64_t client_order_time; 		///< 委托时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 12> account_id; 		///< 账户id
char side; 		///< 买卖方向
char order_type; 		///< 订单类型
int64_t price; 		///< 委托价格
int64_t order_qty; 		///< 申报数量(张)/ETF份额
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
char order_way; 		///< 委托方式
std::array<char, 64> user_info; 		///< 用户私有信息
std::string password; 		///< 密码
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
uint16_t policy_id; 		///< 策略编号


char enforce_flag; 		///< 强制风险标识


std::array<char, 8> etf_security_id; 		///< etf代码
uint16_t etf_market_id; 		///< etf市场
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

/*
 * @brief 按证券账户划转资金请求消息 810001
 */
class  AgwExternalInsTETransFundReq 
{
public:
int64_t client_time; 		///< 客户发起时间
std::array<char, 16> cust_id; 		///< 客户号
std::string password; 		///< 客户密码
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> fund_out_account_id; 		///< 资金转出账户ID
uint16_t fund_out_market_id; 		///< 资金转出市场ID
std::array<char, 12> fund_in_account_id; 		///< 资金转入账户ID
uint16_t fund_in_market_id; 		///< 资金转入市场ID
int64_t value; 		///< 划拨金额
std::array<char, 4> currency; 		///< 货币种类
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
std::array<char, 64> user_info; 		///< 用户私有信息
};

/*
 * @brief 信用账户按证券账户划转资金请求消息 820001
 */
class  AgwExternalInsTETransCreditFundReq 
{
public:
int64_t client_time; 		///< 客户发起时间
std::array<char, 16> cust_id; 		///< 客户号
std::string password; 		///< 客户密码
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> fund_out_account_id; 		///< 资金转出账户ID
uint16_t fund_out_market_id; 		///< 资金转出市场ID
std::array<char, 12> fund_in_account_id; 		///< 资金转入账户ID
uint16_t fund_in_market_id; 		///< 资金转入市场ID
int64_t avl_value; 		///< 自有资金可用划拨金额
int64_t credit_avl_value; 		///< 融券卖出资金可用划拨金额
std::array<char, 4> currency; 		///< 货币种类
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
std::array<char, 64> user_info; 		///< 用户私有信息
};

/*
 * @brief 信用账户按证券账户划转保证金余额请求消息 820004
 */
class  AgwExternalInsTETransCreditMarginAvlReq 
{
public:
int64_t client_time; 		///< 客户发起时间
std::array<char, 16> cust_id; 		///< 客户号
std::string password; 		///< 客户密码
std::array<char, 16> fund_account_id; 		///< 资金帐号
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> fund_out_account_id; 		///< 资金转出账户ID
uint16_t fund_out_market_id; 		///< 资金转出市场ID
std::array<char, 12> fund_in_account_id; 		///< 资金转入账户ID
uint16_t fund_in_market_id; 		///< 资金转入市场ID
int64_t margin_avl_value; 		///< 保证金余额划拨金额
std::array<char, 4> currency; 		///< 货币种类
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
std::array<char, 64> user_info; 		///< 用户私有信息
};

/*
 * @brief 直接还款请求 7102701
 */
class  AgwMsgExtDirectRepay
{
public:
int64_t time; 		///< 发起时间
std::array<char, 16> cust_id; 		///< 客户号
std::array<char, 16> fund_account_id; 		///< 资金账户ID
std::array<char, 10> branch_id; 		///< origid+营业部ID,各占5个字节
std::array<char, 12> account_id; 		///< 账户ID
uint8_t is_account_partition; 		///< 账户是否按证券账户分区 1-是 0否
std::array<char, 4> currency; 		///< 货币种类
char order_way; 		///< 委托方式
int64_t repay_value; 		///< 还款金额
std::string remark; 		///< 备注
std::string password; 		///< 密码
int64_t client_seq_id; 		///< 用户系统消息序号
std::string client_feature_code; 		///< 终端识别码
std::array<char, 64> user_info; 		///< 用户私有信息
std::array<char, 2> order_way_ext; 		///< 委托方式扩展
};

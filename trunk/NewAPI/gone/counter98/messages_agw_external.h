#ifndef ATP_GW_MESSAGES_AGW_EXTERNAL_H_
#define ATP_GW_MESSAGES_AGW_EXTERNAL_H_
#include <stdint.h>
#include <string>
#include <map>
#include <vector>

/*
 * @brief 国君版本将boost的property_tree放到头文件，华锐改造之后放到了源文件
 */
#include "root_packet.h"
#include "string_util.h"
#include "error_code.h"
#include "msg_proto_types.h"
#include "export_macro.h"
#include "types_agw_external.h"
#include "constants_agw_external.h"
using mp::MessageEncoder;
using mp::MessageDecoder;
using mp::AsString;
using mp::CopyToArray;
using mp::StringToInteger;
namespace atp
{
namespace gw
{
typedef mp::ErrorCode error_code;
const MsgTypeNo_def kAgwMsgLogin = 3; ///< 登录消息
const MsgTypeNo_def kAgwMsgLogout = 4; ///< 注销消息
const MsgTypeNo_def kAgwMsgHeartBeat = 0; ///< 心跳消息
const MsgTypeNo_def kAgwMsgTestRequest = 1; ///< 测试请求消息
const MsgTypeNo_def kAgwMsgSync = 5; ///< 回报同步消息
const MsgTypeNo_def kAgwMsgSyncFinish = 6; ///< 回报同步结束消息
const MsgTypeNo_def kAgwMsgRejectReasonCodeSync = 11; ///< 拒绝消息代码同步消息
const MsgTypeNo_def kAgwMsgRequestAccessData = 7; ///< 接入控制消息
const MsgTypeNo_def kAgwMsgReportAccessData = 8; ///< 接入控制结果消息
const MsgTypeNo_def kAgwMsgPushFuncCodeList = 9; ///< 客户号功能权限消息
const MsgTypeNo_def kAgwMsgTerminalInfoCollection = 10; ///< API终端采集数据
const MsgTypeNo_def kAgwMsgRealTimeControlInstruction = 910001; ///< 实时控制指令消息
const MsgTypeNo_def kAgwMsgRealTimeControlInstructionResult = 910003; ///< 实时控制指令结果消息
const MsgTypeNo_def kAgwMsgRealTimeQueryInstruction = 920001; ///< 实时查询指令消息
const MsgTypeNo_def kAgwMsgRealTimeQueryInstructionResult = 920003; ///< 实时查询指令消息结果

const MsgTypeNo_def kAgwMsgBizRejection = 800002; ///< 外部业务拒绝消息
const MsgTypeNo_def kAgwMsgCustLogoutReq = 800007; ///< 登出请求
const MsgTypeNo_def kAgwMsgCustLogoutResp = 800008; ///< 登出应答
const MsgTypeNo_def kAgwMsgCustLoginReq = 800005; ///< 登入请求
const MsgTypeNo_def kAgwMsgCustLoginResp = 800006; ///< 登入应答
const MsgTypeNo_def kAgwMsgExtFundTransfer = 800009; ///< 外部资金划拨请求
const MsgTypeNo_def kAgwMsgExtFundTransferResult = 800010; ///< 外部资金划拨应答
const MsgTypeNo_def kAgwMsgCustPasswdModify = 8000004; ///< 客户密码修改请求
const MsgTypeNo_def kAgwMsgCustPasswdModifyResult = 8000005; ///< 客户密码修改应答
const MsgTypeNo_def kAgwMsgCustAuthReq = 800012; ///< 认证请求
const MsgTypeNo_def kAgwMsgCustAuthResp = 800013; ///< 认证应答
const MsgTypeNo_def kAgwMsgSyncBiz = 8000011; ///< 同步客户回报消息
const MsgTypeNo_def kAgwMsgSyncBizFinish = 8000012; ///< 同步客户回报结束消息
const MsgTypeNo_def kAgwMsgVipCustLoginReq = 800014; ///< 顶点两融VIP登入请求
const MsgTypeNo_def kAgwMsgCashAuctionOrder = 100101; ///< 现货集中竞价业务委托消息
const MsgTypeNo_def kAgwMsgTibAfterHourAuctionOrder = 109701; ///< 科创板股票盘后定价业务委托消息
const MsgTypeNo_def kAgwMsgIssueOrder = 101301; ///< 网上发行业务委托消息
const MsgTypeNo_def kAgwMsgThirdIssueOrder = 7122701; ///< 新三板网上发行业务委托消息
const MsgTypeNo_def kAgwMsgRightsIssueOrder = 101401; ///< 配股业务委托消息
const MsgTypeNo_def kAgwMsgWarrantExerciseOrder = 102301; ///< 权证行权业务委托消息
const MsgTypeNo_def kAgwMsgHKStocksThroughAuctionOrder = 106301; ///< 港股通业务委托消息
const MsgTypeNo_def kAgwMsgHKCorporateActionCancelOrder = 190004; ///< 港股通公司行为公共撤单消息
const MsgTypeNo_def kAgwMsgFDEPCancelOrder = 190003; ///< FDEP公共撤单消息
const MsgTypeNo_def kAgwMsgCancelOrder = 190001; ///< 通用撤单消息
const MsgTypeNo_def kAgwMsgBatchCancelOrder = 190002; ///< 一键撤单消息
const MsgTypeNo_def kAgwMsgOrderStatusAck = 290001; ///< 委托状态响应
const MsgTypeNo_def kAgwMsgCashAuctionTradeER = 210115; ///< 现货集中竞价业务成交回报
const MsgTypeNo_def kAgwMsgTibAfterHourAuctionTradeER = 219715; ///< 科创板股票盘后定价业务成交回报
const MsgTypeNo_def kAgwMsgRepoAuctionOrder = 100201; ///< 质押式回购集中竞价业务委托消息
const MsgTypeNo_def kAgwMsgRepoAuctionTradeER = 210215; ///< 质押式回购集中竞价业务成交回报
const MsgTypeNo_def kAgwMsgSwapPutbackOrder = 101501; ///< 债券转股回售委托
const MsgTypeNo_def kAgwMsgLOFOrder = 101701; ///< 开放式基金申赎业务订单
const MsgTypeNo_def kAgwMsgLOFTradeER = 211715; ///< 开放式基金申赎成交回报
const MsgTypeNo_def kAgwMsgPledgeInOrOutOrder = 101901; ///< 质押出入库业务消息
const MsgTypeNo_def kAgwMsgPledgeInOrOutTradeER = 211915; ///< 质押出入库成交回报
const MsgTypeNo_def kAgwMsgPasswordServiceOrder = 102401; ///< 密码服务业务消息
const MsgTypeNo_def kAgwMsgNetVotingOrder = 102101; ///< 网络投票业务消息
const MsgTypeNo_def kAgwMsgTenderOfferOrder = 101801; ///< 要约收购业务消息
const MsgTypeNo_def kAgwMsgETFRedemptionOrder = 101201; ///< ETF申赎业务委托消息
const MsgTypeNo_def kAgwMsgETFRedemptionTradeER = 211215; ///< ETF申赎业务成交回报
const MsgTypeNo_def kAgwMsgBondDistributionOrder = 100301; ///< 债券分销委托
const MsgTypeNo_def kAgwMsgBondDistributionTradeER = 210315; ///< 债券分销成交回报
const MsgTypeNo_def kAgwMsgStructuredFundOrder = 103101; ///< 分级基金实时拆分合并委托消息
const MsgTypeNo_def kAgwMsgStructuredFundTradeER = 213115; ///< 分级基金实时拆分合并成交回报
const MsgTypeNo_def kAgwMsgHKStocksThroughTradeER = 216315; ///< 港股通业务成交回报
const MsgTypeNo_def kAgwMsgHKVotingOrder = 106401; ///< 港股通投票业务订单
const MsgTypeNo_def kAgwMsgHKCorporateActionOrder = 106201; ///< 港股通公司行为业务订单
const MsgTypeNo_def kAgwMsgHKDesignationTransferOrder = 106101; ///< 港股通转托管业务订单
const MsgTypeNo_def kAgwMsgDcomCancelOrder = 7190701; ///< 深结算dcom撤单委托
const MsgTypeNo_def kAgwMsgCreditAuctionOrder = 7100701; ///< 信用账户普通买入/普通卖出业务委托消息
const MsgTypeNo_def kAgwMsgCreditAuctionTradeER = 7200715; ///< 信用账户普通买入/普通卖出成交回报
const MsgTypeNo_def kAgwMsgMarginTradingOrder = 7101701; ///< 融资买入/融券卖出业务委托消息
const MsgTypeNo_def kAgwMsgMarginTradingTradeER = 7201715; ///< 融资买入/融券卖出成交回报
const MsgTypeNo_def kAgwMsgCollateralInOrOutOrder = 7103701; ///< 担保品划转业务委托消息
const MsgTypeNo_def kAgwMsgThirdCollateralInOrder = 7192701; ///< 第三方担保品转入直达信用节点业务委托消息
const MsgTypeNo_def kAgwMsgBuySecuRepayOrder = 7104701; ///< 买券还券业务委托消息
const MsgTypeNo_def kAgwMsgBuySecuRepayTradeER = 7204715; ///< 买券还券成交回报
const MsgTypeNo_def kAgwMsgCurSecuRepayOrder = 7105701; ///< 现券还券业务委托消息
const MsgTypeNo_def kAgwMsgSellRepayAppointOrder = 7109701; ///< 指定合约卖券还款业务委托消息
const MsgTypeNo_def kAgwMsgSellRepayAppointTradeER = 7209715; ///< 指定合约卖券还款成交回报
const MsgTypeNo_def kAgwMsgExtDirectRepay = 7102701; ///< 直接还款请求
const MsgTypeNo_def kAgwMsgExtDirectRepayResult = 7202702; ///< 直接还款应答
const MsgTypeNo_def kAgwMsgCreditVote = 7107701; ///< 信用投票业务消息
const MsgTypeNo_def kAgwMsgCreditVoteResult = 7207702; ///< 信用投票业务应答
const MsgTypeNo_def kAgwMsgExtDirectRepayAppoint = 7108701; ///< 指定合约还款请求
const MsgTypeNo_def kAgwMsgExtDirectRepayAppointResult = 7208702; ///< 指定合约还款应答
const MsgTypeNo_def kAgwMsgExtCreditDebtsDelayApply = 7107702; ///< 信用预到期合约展期申请
const MsgTypeNo_def kAgwMsgExtCreditReserveStkApply = 7100702; ///< 融券预约申请
const MsgTypeNo_def kAgwMsgExtCreditReserveStkApplyResult = 7200716; ///< 融券预约申请应答
const MsgTypeNo_def kAgwMsgExtCreditDebtsDelayApplyResult = 7207716; ///< 信用预到期合约展期申请应答
const MsgTypeNo_def kAgwMsgExtCreditReserveStkCancel = 7101702; ///< 融券预约申请撤销
const MsgTypeNo_def kAgwMsgExtCreditReserveStkCancelResult = 7201716; ///< 融券预约申请撤销应答
const MsgTypeNo_def kAgwMsgExtCustSelfCloseApply = 7106702; ///< 客户自行平仓权限申请
const MsgTypeNo_def kAgwMsgExtCustSelfCloseApplyResult = 7206716; ///< 客户自行平仓权限申请应答
const MsgTypeNo_def kAgwMsgExtLockDelayApply = 7105702; ///< 客户锁券展期申请
const MsgTypeNo_def kAgwMsgExtLockDelayApplyResult = 7205716; ///< 客户锁券展期申请应答
const MsgTypeNo_def kAgwMsgCashAuctionFPGAOrder = 7800701; ///< FPGA现货集中竞价业务委托消息
const MsgTypeNo_def kAgwMsgCashAuctionFPGARejectAck = 7800702; ///< FPGA现货集中竞价业务拒绝消息
const MsgTypeNo_def kAgwMsgMarginTradingExtOrder = 7800703; ///< 外部系统两融开仓业务委托消息
const MsgTypeNo_def kAgwMsgCreditAuctionExtOrder = 7800704; ///< 外部系统信用账户普通买入/普通卖出业务委托消息
const MsgTypeNo_def kAgwMsgETFRedemptionExtOrder = 7800705; ///< 外部系统ETF申赎业务委托消息
const MsgTypeNo_def kAgwMsgCancelOrderSync = 190006; ///< 通用撤单同步消息
const MsgTypeNo_def kAgwMsgExtQueryMaxOrderQty = 300001; ///< 最大可委托数查询消息
const MsgTypeNo_def kAgwMsgNewExtQueryMaxOrderQty = 300013; ///< 大宗最大可委托数查询消息
const MsgTypeNo_def kAgwMsgExtQueryMaxOrderQtyResult = 310001; ///< 最大可委托数查询结果消息
const MsgTypeNo_def kAgwMsgNewExtQueryMaxOrderQtyResult = 310013; ///< 大宗最大可委托数查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryTradeOrder = 300003; ///< 成交查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultTradeOrder = 310003; ///< 成交查询结果
const MsgTypeNo_def kAgwMsgExtQueryFund = 300004; ///< 资金查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultFund = 310004; ///< 资金查询结果
const MsgTypeNo_def kAgwMsgExtQueryShare = 300005; ///< 股份持仓查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultShare = 310005; ///< 股份查询结果
const MsgTypeNo_def kAgwMsgExtQueryIPOQty = 300006; ///< 新股发行申购额度查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultIPOQty = 310006; ///< 新股发行申购额度查询结果
const MsgTypeNo_def kAgwMsgExtQueryTIBIPOQty = 300010; ///< 科创板新股发行申购额度查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultTIBIPOQty = 310010; ///< 科创板新股发行申购额度查询结果
const MsgTypeNo_def kAgwMsgExtQueryTradeCollect = 300009; ///< 成交汇总查询消息
const MsgTypeNo_def kAgwExtQueryResultTradeCollect = 310009; ///< 成交汇总查询结果
const MsgTypeNo_def kAgwMsgExtQueryOrder = 300002; ///< 订单查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultOrder = 310002; ///< 订单查询结果
const MsgTypeNo_def kAgwExternalInsTETransFundReq = 810001; ///< 按证券账户划转资金请求消息
const MsgTypeNo_def kAgwExternalInsTETransFundResp = 810002; ///< 按证券账户划转资金响应消息
const MsgTypeNo_def kAgwExternalInsTETransCreditFundReq = 820001; ///< 信用账户按证券账户划转资金请求消息
const MsgTypeNo_def kAgwExternalInsTETransCreditFundResp = 821001; ///< 信用账户按证券账户划转资金响应消息
const MsgTypeNo_def kAgwExternalInsTETransCreditShareReq = 820002; ///< 信用账户按证券账户划转股份请求消息
const MsgTypeNo_def kAgwExternalInsTETransCreditShareResp = 821002; ///< 信用账户按证券账户划转股份响应消息
const MsgTypeNo_def kAgwExternalInsTETransCreditMarginAvlReq = 820004; ///< 信用账户按证券账户划转保证金余额请求消息
const MsgTypeNo_def kAgwExternalInsTETransCreditMarginAvlResp = 821004; ///< 信用账户按证券账户划转保证金余额响应消息
const MsgTypeNo_def kAgwExternalInsTETransCreditLimitReq = 820005; ///< 信用账户按证券账户划转可用授信额度请求消息
const MsgTypeNo_def kAgwExternalInsTETransCreditLimitResp = 821005; ///< 信用账户按证券账户划转可用授信额度响应消息
const MsgTypeNo_def kAgwMsgExtQueryFundEx = 3000033; ///< 增强资金查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultFundEx = 3100033; ///< 增强资金查询结果
const MsgTypeNo_def kAgwMsgAfterHoursPricingOrder = 1006001; ///< 大宗盘后定价委托消息
const MsgTypeNo_def kAgwMsgBlockTradeIntentionOrder = 1005009; ///< 大宗意向申报委托消息
const MsgTypeNo_def kAgwMsgBlockTradeTransactionOrder = 1005003; ///< 大宗成交申报委托消息
const MsgTypeNo_def kAgwMsgBlockTradePricingOrder = 1005001; ///< 大宗定价申报委托消息
const MsgTypeNo_def kAgwMsgBlockTradeCancelOrder = 19000005; ///< 大宗交易撤单消息
const MsgTypeNo_def kAgwMsgAfterHoursPricingTradeER = 2006015; ///< 大宗盘后定价业务成交回报
const MsgTypeNo_def kAgwMsgBlockTradeTransactionTradeER = 2005015; ///< 大宗成交申报委托业务成交回报
const MsgTypeNo_def kAgwMsgBlockTradePricingTradeER = 2005115; ///< 大宗定价申报委托业务成交回报
const MsgTypeNo_def kAgwMsgReduceHoldingQuoteTransferOrder = 1036001; ///< 高管额度划转业务委托消息
const MsgTypeNo_def kAgwMsgReduceHoldingQuoteTransferCancelOrder = 19000006; ///< 高管额度划转撤单消息
const MsgTypeNo_def kAgwMsgThirdPricingOrder = 7120701; ///< 股转协议转让定价申报委托消息
const MsgTypeNo_def kAgwMsgThirdPricingTradeER = 7220715; ///< 股转协议转让定价申报成交回报
const MsgTypeNo_def kAgwMsgThirdTransactionOrder = 7125701; ///< 股转协议转让成交确认申报委托消息
const MsgTypeNo_def kAgwMsgThirdTransactionTradeER = 7225715; ///< 股转协议转让成交确认申报成交回报
const MsgTypeNo_def kAgwMsgThirdCashAuctionOrder = 7123701; ///< 新三板买卖委托
const MsgTypeNo_def kAgwMsgThirdCashAuctionTradeER = 7223715; ///< 新三板买卖成交回报
const MsgTypeNo_def kAgwMsgThirdCashAuctionDelistOrder = 7126701; ///< 新三板买卖委托(两网退市股票)
const MsgTypeNo_def kAgwMsgThirdCashAuctionDelistTradeER = 7226715; ///< 新三板买卖成交回报(两网退市股票)
const MsgTypeNo_def kAgwMsgThirdBlockTradeOrder = 7124701; ///< 股转大宗交易成交确认申报委托消息
const MsgTypeNo_def kAgwMsgThirdBlockTradeTradeER = 7224715; ///< 股转大宗交易成交确认申报成交回报
const MsgTypeNo_def kAgwMsgThirdTenderOfferOrder = 7121701; ///< 股转要约收购业务消息
const MsgTypeNo_def kAgwMsgThirdDelistAgreementOrder = 7132701; ///< 股转摘牌协议转让成交确认申报委托消息
const MsgTypeNo_def kAgwMsgExtQuerySecurityInfo = 7300701; ///< 证券信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultSecurityInfo = 7400701; ///< 证券信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryMd = 7300702; ///< 行情信息查询
const MsgTypeNo_def kAgwMsgExtQueryRightsIssueInfo = 7300704; ///< 配股信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultRightsIssueInfo = 7400704; ///< 配股信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryResultMd = 7400702; ///< 行情信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryIPOInfo = 7300703; ///< 新股信息查询消息
const MsgTypeNo_def kAgwMsgExtQueryBondPledge = 7300725; ///< 债券质押库查询
const MsgTypeNo_def kAgwMsgExtQueryResultBondPledge = 7400725; ///< 债券质押库查询结果
const MsgTypeNo_def kAgwMsgExtQuerySecuSeat = 7301725; ///< 查询股东报盘席位
const MsgTypeNo_def kAgwMsgExtQueryResultSecuSeat = 7401725; ///< 查询股东报盘席位结果消息
const MsgTypeNo_def kAgwMsgExtQueryResultIPOInfo = 7400703; ///< 新股信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryFundMaxdrawCrm = 7300713; ///< 资金查询(95521客服)消息
const MsgTypeNo_def kAgwMsgExtQueryResultFundMaxdrawCrm = 7400713; ///< 资金查询(95521客服)查询结果
const MsgTypeNo_def kAgwMsgExtQueryCustTotalAssets = 7301722; ///< 客户资产总值查询
const MsgTypeNo_def kAgwMsgExtQueryResultCustTotalAssets = 7401722; ///< 客户资产总值查询结果
const MsgTypeNo_def kAgwMsgExtQueryAppAssets = 7300718; ///< APP理财资产查询
const MsgTypeNo_def kAgwMsgExtQueryResultAppAssets = 7400718; ///< APP理财资产查询结果
const MsgTypeNo_def kAgwMsgExtQuerySelfShare = 7300714; ///< 自研股份查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultSelfShare = 7400714; ///< 自研股份查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditSelfShare = 7390764; ///< 信用账户自研股份查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditSelfShare = 7490764; ///< 信用账户自研股份查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditMarginAvlTransFlow = 7390768; ///< 信用账户保证金余额划转流水查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditMarginAvlTransFlow = 7490768; ///< 信用账户保证金余额划转流水查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditLimitTransFlow = 7390773; ///< 信用账户可用授信额度划转流水查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditLimitTransFlow = 7490773; ///< 信用账户可用授信额度划转流水查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryAvlPrice = 7300779; ///< 证券信息(余额、价格等)查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultAvlPrice = 7400779; ///< 证券信息(余额、价格等)查询结果消息
const MsgTypeNo_def kAgwMsgExtQuerySelfFund = 7300780; ///< 自研资金查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultSelfFund = 7400780; ///< 自研资金查询结果
const MsgTypeNo_def kAgwMsgExtQueryAssetDebts = 7390700; ///< 资产负债查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultAssetDebts = 7490700; ///< 资产负债查询结果
const MsgTypeNo_def kAgwMsgExtQueryFundMaxdraw = 7390701; ///< 可取资金查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultFundMaxdraw = 7490701; ///< 可取资金查询结果
const MsgTypeNo_def kAgwMsgExtQueryTargetStk = 7390702; ///< 标的证券查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultTargetStk = 7490702; ///< 标的证券查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsDetail = 7390703; ///< 信用客户融资融券合约明细查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsDetail = 7490703; ///< 信用客户融资融券合约明细查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditDebts = 7390704; ///< 信用客户融资融券合约查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebts = 7490704; ///< 信用客户融资融券合约查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsPB = 7390738; ///< 信用客户融资融券合约查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsPB = 7490738; ///< 信用客户融资融券合约查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsQFII = 7390739; ///< 信用客户融资融券合约查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsQFII = 7490739; ///< 信用客户融资融券合约查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsBySecurity = 7390778; ///< 按证券融资融券合约查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsBySecurity = 7490778; ///< 按证券融资融券合约查询结果
const MsgTypeNo_def kAgwMsgExtQueryPledgeStk = 7390705; ///< 担保证券查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultPledgeStk = 7490705; ///< 担保证券查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditVote = 7390706; ///< 信用投票客户查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditVote = 7490706; ///< 信用投票客户查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditVoteCount = 7390707; ///< 信用投票可投票数查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditVoteCount = 7490707; ///< 信用投票可投票数查询结果
const MsgTypeNo_def kAgwMsgExtQueryAccountContractFund = 300008; ///< 指定合约账户资金查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultAccountContractFund = 310008; ///< 指定合约账户资金查询结果
const MsgTypeNo_def kAgwMsgExtQueryConcentration = 7390736; ///< 信用持仓集中度-旧查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultConcentration = 7490736; ///< 信用持仓集中度-旧查询结果
const MsgTypeNo_def kAgwMsgExtQueryConcentrationType = 7390734; ///< 集中度类型查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultConcentrationType = 7490734; ///< 集中度类型查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditCollateralRatio = 7390708; ///< 信用交易风险四条预警线外部查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditCollateralRatio = 7490708; ///< 信用交易风险四条预警线外部查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditConcentration = 7390720; ///< 总体集中度查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditConcentration = 7490720; ///< 总体集中度查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditContract = 7390726; ///< 合同查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditContract = 7490726; ///< 合同查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsCollect = 7390728; ///< 融资融券情况汇总查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsCollect = 7490728; ///< 融资融券情况汇总查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsCollectNew = 7390740; ///< 融资融券情况汇总查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsCollectNew = 7490740; ///< 融资融券情况汇总查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditFundTransFlow = 7390742; ///< 信用账户资金划转流水查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditFundTransFlow = 7490742; ///< 信用账户资金划转流水查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditShareTransFlow = 7390743; ///< 信用账户股份划转流水查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditShareTransFlow = 7490743; ///< 信用账户股份划转流水查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryLabelConcentration = 7390744; ///< 标签集中度查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultLabelConcentration = 7490744; ///< 标签集中度查询结果
const MsgTypeNo_def kAgwMsgExtQueryLabelStkCode = 7390745; ///< 标签所属证券查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultLabelStkCode = 7490745; ///< 标签所属证券查询结果
const MsgTypeNo_def kAgwMsgExtQueryZHShareTargetStk = 7390746; ///< 公共券池标的券信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultZHShareTargetStk = 7490746; ///< 公共券池标的券信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditStkLockTargetStk = 7390747; ///< 客户锁标的券信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultCreditStkLockTargetStk = 7490747; ///< 客户锁标的券信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryCustCreditRiskTag = 7390748; ///< 融资融券个性化客户标签查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustCreditRiskTag = 7490748; ///< 融资融券个性化客户标签查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCustCreditRiskInfo = 7390749; ///< 两融客户风控指标(集中度保证金比例折算率)查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustCreditRiskInfo = 7490749; ///< 两融客户风控指标(集中度保证金比例折算率)查询消息
const MsgTypeNo_def kAgwMsgExtQueryMarginSellQtySpecialAccount = 7390750; ///< 专户融券卖数量查询
const MsgTypeNo_def kAgwMsgExtQueryResultMarginSellQtySpecialAccount = 7490750; ///< 专户融券卖数量查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditAvlCanBuySecurity = 7390761; ///< 融券卖出冻结资金可买证券清单查询
const MsgTypeNo_def kAgwMsgExtQueryResultCreditAvlCanBuySecurity = 7490761; ///< 融券卖出冻结资金可买证券清单查询結果
const MsgTypeNo_def kAgwMsgExtQueryCreditCustomerlizedPledgeSecurity = 7390762; ///< 客户个性化担保证券查询
const MsgTypeNo_def kAgwMsgExtQueryResultCreditCustomerlizedPledgeSecurity = 7490762; ///< 客户个性化担保证券查询結果
const MsgTypeNo_def kAgwMsgExtQueryCustomerlizedCreditCollateralRatio = 7390763; ///< 客户个性化信用交易风险四条预警线外部查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustomerlizedCreditCollateralRatio = 7490763; ///< 客户个性化信用交易风险四条预警线外部查询结果
const MsgTypeNo_def kAgwMsgExtQueryDDCustInfo = 7390751; ///< 顶点两融客户查询
const MsgTypeNo_def kAgwMsgExtQueryResultDDCustInfo = 7490751; ///< 顶点两融客户查询结果
const MsgTypeNo_def kAgwMsgExtQueryDDShare = 7390752; ///< 顶点两融持仓查询
const MsgTypeNo_def kAgwMsgExtQueryResultDDShare = 7490752; ///< 顶点两融持仓查询结果
const MsgTypeNo_def kAgwMsgExtQueryDDMarginStk = 7390753; ///< 顶点两融查询资格证券
const MsgTypeNo_def kAgwMsgExtQueryResultDDMarginStk = 7490753; ///< 顶点两融查询资格证券结果
const MsgTypeNo_def kAgwMsgExtQueryDDDebtsFlow = 7390754; ///< 顶点两融查询当日负债变动流水
const MsgTypeNo_def kAgwMsgExtQueryResultDDDebtsFlow = 7490754; ///< 顶点两融当日负债流水查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryDDCreditAsset = 7390755; ///< 顶点两融信用资产查询
const MsgTypeNo_def kAgwMsgExtQueryResultDDCreditAsset = 7490755; ///< 顶点两融信用资产查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryDDDebtsDelayFlow = 7390756; ///< 顶点两融查询客户合约展期流水
const MsgTypeNo_def kAgwMsgExtQueryResultDDDebtsDelayFlow = 7490756; ///< 顶点两融查询客户合约展期流水结果
const MsgTypeNo_def kAgwMsgExtQueryDDMarginSell = 7390757; ///< 顶点两融查询当日融券负债汇总
const MsgTypeNo_def kAgwMsgExtQueryResultDDMarginSell = 7490757; ///< 顶点两融查询当日融券负债汇总结果
const MsgTypeNo_def kAgwMsgExtQueryDDConcentration = 7390758; ///< 顶点两融查询集中度控制比例
const MsgTypeNo_def kAgwMsgExtQueryResultDDConcentration = 7490758; ///< 顶点两融查询集中度控制比例结果
const MsgTypeNo_def kAgwMsgExtQueryDDCreditDebts = 7390759; ///< 顶点两融查询信用负债
const MsgTypeNo_def kAgwMsgExtQueryResultDDCreditDebts = 7490759; ///< 顶点两融查询信用负债结果
const MsgTypeNo_def kAgwMsgExtQueryDDDelayDebts = 7390760; ///< 顶点两融查询客户可展期合约
const MsgTypeNo_def kAgwMsgExtQueryResultDDDelayDebts = 7490760; ///< 顶点两融查询客户可展期合约结果
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsDelayApplyResult = 7390712; ///< 信用合约展期申请结果查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsDelayApplyResult = 7490712; ///< 信用合约展期申请结果查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditPreMaturityDebts = 7390711; ///< 信用预到期合约查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditPreMaturityDebts = 7490711; ///< 信用预到期合约查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditPreMaturityDebtsByDays = 7390777; ///< 信用预到期合约(按到期天数)查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditPreMaturityDebtsByDays = 7490777; ///< 信用预到期合约(按到期天数)查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditReserveStk = 7390713; ///< 融券预约查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditReserveStk = 7490713; ///< 融券预约查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditReserveStkTrd = 7390732; ///< 融券预约未上市可预约证券查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditReserveStkTrd = 7490732; ///< 融券预约未上市可预约证券查询结果
const MsgTypeNo_def kAgwMsgExtQueryCreditTradeCollect = 7390727; ///< 信用当日成交汇总查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditTradeCollect = 7490727; ///< 信用当日成交汇总查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCustMarginAvl = 7390724; ///< 融资融券信用数据（可用保证金）查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustMarginAvl = 7490724; ///< 融资融券信用数据（可用保证金）查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryHHOrder = 7390730; ///< 新外围委托查询(宏汇专用)查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultHHOrder = 7490730; ///< 新外围委托查询(宏汇专用)查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryOrderCollectAll = 7300785; ///< 当日委托汇总查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultOrderCollectAll = 7400785; ///< 当日委托汇总查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryPlateConcentration = 7390722; ///< 板块集中度查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultPlateConcentration = 7490722; ///< 板块集中度查询结果
const MsgTypeNo_def kAgwMsgExtQueryPlateStkCode = 7390723; ///< 板块所属证券查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultPlateStkCode = 7490723; ///< 板块所属证券查询结果
const MsgTypeNo_def kAgwMsgExtQuerySelfCloseApply = 7390719; ///< 自行平仓申请记录查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultSelfCloseApply = 7490719; ///< 自行平仓申请记录查询结果
const MsgTypeNo_def kAgwMsgExtQuerySelfCloseApplyRight = 7390718; ///< 自行平仓申请条件查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultSelfCloseApplyRight = 7490718; ///< 自行平仓申请条件查询结果
const MsgTypeNo_def kAgwMsgExtQuerySelfCloseChk = 7390717; ///< 客户自行平仓审查查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultSelfCloseChk = 7490717; ///< 客户自行平仓审查查询结果
const MsgTypeNo_def kAgwMsgExtQuerySellRepayType = 7390733; ///< 卖券还款类型查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultSellRepayType = 7490733; ///< 卖券还款类型查询结果
const MsgTypeNo_def kAgwMsgExtQueryStkConcentration = 7390721; ///< 股票集中度查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultStkConcentration = 7490721; ///< 股票集中度查询结果
const MsgTypeNo_def kAgwMsgExtQueryTradeCollectAll = 7301726; ///< 当日成交汇总查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultTradeCollectAll = 7401726; ///< 当日成交汇总查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditAssetDetails = 7390709; ///< 当日资金流水查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditAssetDetails = 7490709; ///< 当日资金流水查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditDebtsDetails = 7390725; ///< 当日负债流水查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditDebtsDetails = 7490725; ///< 当日负债流水查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditLockRight = 7390731; ///< 客户锁券资质查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditLockRight = 7490731; ///< 客户锁券资质查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditMaxOrderQty = 7390710; ///< 两融最大可委托数查询消息
const MsgTypeNo_def kAgwMsgExtQueryCreditMaxOrderQtyResult = 7490710; ///< 两融最大可委托数查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditStkLock = 7390715; ///< 锁券合同查询
const MsgTypeNo_def kAgwMsgExtQueryResultCreditStkLock = 7490715; ///< 锁券合同查询结果
const MsgTypeNo_def kAgwMsgExtQueryLockDelayApply = 7390716; ///< 锁券展期申请查询
const MsgTypeNo_def kAgwMsgExtQueryResultLockDelayApply = 7490716; ///< 锁券展期申请查询结果
const MsgTypeNo_def kAgwMsgExtQuerySubCreditStkLock = 7390714; ///< 锁券子锁查询
const MsgTypeNo_def kAgwMsgExtQueryResultSubCreditStkLock = 7490714; ///< 客户子锁查询结果
const MsgTypeNo_def kAgwMsgExtQuerySellRepayAppointAmt = 7390737; ///< 融资融券指定合约卖券还款金额试算
const MsgTypeNo_def kAgwMsgExtQueryResultSellRepayAppointAmt = 7490737; ///< 融资融券指定合约卖券还款金额试算结果
const MsgTypeNo_def kAgwMsgShareCostReset = 7162703; ///< 股份成本重置请求
const MsgTypeNo_def kAgwMsgShareCostResetResult = 7262717; ///< 股份成本重置应答
const MsgTypeNo_def kAgwMsgExtQueryCustID = 7302705; ///< 客户代码查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustID = 7402705; ///< 客户代码查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCreditAcct = 7301727; ///< 普通信用账号关系查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCreditAcct = 7401727; ///< 普通信用账号关系查询结果消息
const MsgTypeNo_def kAgwMsgExternalQueryDelisingPeriodBuyRight = 7302700; ///< 退市整理期买入权限查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultDelisingPeriodBuyRight = 7402700; ///< 退市整理期买入权限查询结果
const MsgTypeNo_def kAgwMsgExternalQueryCustRiskValue = 7302701; ///< 客户风险分数和等级查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustRiskValue = 7402701; ///< 客户风险分数和等级查询结果
const MsgTypeNo_def kAgwMsgExternalQueryAccountId = 7302702; ///< 客户股东代码查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultAccountId = 7402702; ///< 客户股东代码查询结果
const MsgTypeNo_def kAgwMsgExternalQueryCustBranchId = 7302703; ///< 客户股东机构查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustBranchId = 7402703; ///< 客户股东机构查询结果
const MsgTypeNo_def kAgwMsgExternalQueryAccountInfo = 7302704; ///< 客户股东信息查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultAccountInfo = 7402704; ///< 客户股东信息查询结果
const MsgTypeNo_def kAgwMsgExternalQueryAccountAssignment = 7301728; ///< 股东账户指定交易状态查询消息
const MsgTypeNo_def kAgwMsgExternalQueryResultAccountAssignment = 7401728; ///< 股东账户指定交易状态查询结果
const MsgTypeNo_def kAgwMsgExtGiveupNewStk = 7100703; ///< 中签资金冻结后放弃认购
const MsgTypeNo_def kAgwMsgExtGiveupNewStkResult = 7200717; ///< 中签资金冻结后放弃认购应答
const MsgTypeNo_def kAgwMsgExtCustMtmSub = 7109702; ///< 盯市订阅消息
const MsgTypeNo_def kAgwMsgExtCustMtmSubResult = 7209716; ///< 盯市订阅消息应答
const MsgTypeNo_def kAgwMsgExtQueryStkHit = 7301730; ///< 中签查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultStkHit = 7401730; ///< 中签查询应答
const MsgTypeNo_def kAgwMsgExtQueryVotingProposal = 7300707; ///< 网络投票议案查询
const MsgTypeNo_def kAgwMsgExtQueryResultVotingProposal = 7400707; ///< 网络投票议案查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryETFSign = 7300731; ///< ETF是否需要签约查询
const MsgTypeNo_def kAgwMsgExtQueryResultETFSign = 7400731; ///< ETF是否需要签约查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryVotingList = 7301736; ///< 上市公司网络投票清单查询
const MsgTypeNo_def kAgwMsgExtQueryResultVotingList = 7401736; ///< 上市公司网络投票清单查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryReduceCust = 7301732; ///< 大宗股份减持受限客户查询
const MsgTypeNo_def kAgwMsgExtQueryResultReduceCust = 7401732; ///< 大宗股份减持受限客户查询结果消息
const MsgTypeNo_def kAgwMsgETFQueryOrderEx = 7301729; ///< ETF申赎成交查询消息
const MsgTypeNo_def kAgwMsgETFQueryResultTradeOrder = 7401729; ///< 成交查询结果
const MsgTypeNo_def kAgwMsgETFRedemptionTradeDealER = 2012015; ///< ETF业务成交回报
const MsgTypeNo_def kAgwMsgExternalQueryTotalFundAsset = 7301735; ///< 客户总资产查询
const MsgTypeNo_def kAgwMsgExtQueryResultTotalFundAsset = 7401735; ///< 客户总资产查询结果
const MsgTypeNo_def kAgwMsgExtQueryHKSRefExchangeRate = 7301790; ///< 外围沪港通参考汇率查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKSRefExchangeRate = 7401790; ///< 外围沪港通参考汇率查询应答
const MsgTypeNo_def kAgwMsgExtQueryHKSMarketTransStatus = 7301791; ///< 外围沪港通市场交易状态查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKSMarketTransStatus = 7401791; ///< 外围沪港通市场交易状态查询应答
const MsgTypeNo_def kAgwMsgExtQueryHKSTradeCalendar = 7301792; ///< 外围沪港通交易日历表查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKSTradeCalendar = 7401792; ///< 外围沪港通交易日历表查询应答
const MsgTypeNo_def kAgwMsgExtQueryHKSUnderlyingSecurity = 7301793; ///< 外围沪港通标的券查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKSUnderlyingSecurity = 7401793; ///< 外围沪港通标的券查询应答
const MsgTypeNo_def kAgwMsgExtQueryHKSSecurityStatus = 7301794; ///< 外围沪港通证券交易状态查询
const MsgTypeNo_def kAgwMsgExtQueryResultSecurityStatus = 7401794; ///< 外围沪港通证券交易状态查询应答
const MsgTypeNo_def kAgwMsgExtQueryHKFundAsset = 7301795; ///< 外围沪港通辅资金表资产查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKFundAsset = 7401795; ///< 外围沪港通辅资金表资产查询结果
const MsgTypeNo_def kAgwMsgExtQueryHKPriceRange = 7301796; ///< 外围沪港通最小价差查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKPriceRange = 7401796; ///< 外围沪港通最小价差查询应答
const MsgTypeNo_def kAgwMsgExtQueryHKCorporateActionRecord = 7301743; ///< 外围港股公司行为查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKCorporateActionRecord = 7401743; ///< 外围港股公司行为查询应答
const MsgTypeNo_def kAgwMsgExtQueryHKMarketValue = 7301744; ///< 外围沪港通市值查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKMarketValue = 7401744; ///< 外围沪港通市值查询应答
const MsgTypeNo_def kAgwMsgExtQueryComponentShare = 7300734; ///< ETF成分股查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultComponentShare = 7400734; ///< ETF成分股查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryETFInfo = 7300733; ///< ETF基本信息查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultETFInfo = 7400733; ///< ETF基本信息查询结果消息
const MsgTypeNo_def kAgwMsgBlockTradePreorder = 7142703; ///< 大宗协议交易预委托
const MsgTypeNo_def kAgwMsgBlockTradePreorderResult = 7242717; ///< 大宗协议交易预委托应答
const MsgTypeNo_def kAgwMsgExternalQueryBlockTradePreorderBranchRight = 7301738; ///< 大宗交易预委托营业部权限查询
const MsgTypeNo_def kAgwMsgExtQueryResultBlockTradePreorderBranchRight = 7401738; ///< 大宗交易预委托营业部权限查询结果
const MsgTypeNo_def kAgwMsgBlockTradePreorderCancel = 7143703; ///< 大宗协议交易预委托撤销
const MsgTypeNo_def kAgwMsgBlockTradePreorderCancelResult = 7243717; ///< 大宗协议交易预委托撤销应答
const MsgTypeNo_def kAgwMsgExternalQueryBlockTradePreorder = 7301737; ///< 大宗交易预委托查询
const MsgTypeNo_def kAgwMsgExtQueryResultBlockTradePreorder = 7401737; ///< 大宗交易预委托查询结果
const MsgTypeNo_def kAgwMsgExternalQueryConvertibleBonds = 7301739; ///< 可转债持仓查询
const MsgTypeNo_def kAgwMsgExtQueryResultConvertibleBonds = 7401739; ///< 可转债持仓查询结果
const MsgTypeNo_def kAgwMsgExternalQueryThirdStkAccountLimit = 7380700; ///< 股转投资者受限证券查询
const MsgTypeNo_def kAgwMsgExtQueryResultThirdStkAccountLimit = 7480700; ///< 股转投资者受限证券查询结果
const MsgTypeNo_def kAgwMsgExternalQueryThirdAgreementTransferMd = 7380706; ///< 股转协议行情查询
const MsgTypeNo_def kAgwMsgExtQueryResultThirdAgreementTransferMd = 7480706; ///< 股转协议行情查询结果
const MsgTypeNo_def kAgwMsgExternalQueryThirdStkAsset = 7380708; ///< 股转特有持仓查询
const MsgTypeNo_def kAgwMsgExtQueryResultThirdStkAsset = 7480708; ///< 股转特有持仓查询结果
const MsgTypeNo_def kAgwMsgExternalQueryThirdFgksbxx = 7380701; ///< 非公开转让申报信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultThirdFgksbxx = 7480701; ///< 非公开转让申报信息查询
const MsgTypeNo_def kAgwMsgExternalQueryThirdFgkcjxx = 7380702; ///< 非公开转让成交信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultThirdFgkcjxx = 7480702; ///< 非公开转让成交信息查询结果
const MsgTypeNo_def kAgwMsgExternalQueryThirdIssueSubscribeResult = 7380704; ///< AGW新三板发行业务认购结果查询
const MsgTypeNo_def kAgwMsgExternalQueryResultThirdIssueSubscribeResult = 7480704; ///< AGW新三板发行业务认购结果查询结果
const MsgTypeNo_def kAgwMsgExternalQueryThirdTenderOfferSecurityInfo = 7380705; ///< 股转要约收购行情查询
const MsgTypeNo_def kAgwMsgExtQueryResultThirdTenderOfferSecurityInfo = 7480705; ///< 股转要约收购行情查询结果
const MsgTypeNo_def kAgwMsgExtQueryThirdTenderOfferMaxOrderQty = 7380707; ///< 要约收购最大可委托数量查询
const MsgTypeNo_def kAgwMsgExtQueryResultThirdTenderOfferMaxOrderQty = 7480707; ///< 要约收购最大可委托数量查询结果
const MsgTypeNo_def kAgwMsgBlockTradeMdQuery = 7301741; ///< 大宗行情查询消息
const MsgTypeNo_def kAgwMsgBlockTradeMdQueryResult = 7401741; ///< 大宗交易行情查询回报
const MsgTypeNo_def kAgwMsgExternalQueryThirdIssueSecurityInfo = 7380703; ///< AGW新三板发行证券信息查询
const MsgTypeNo_def kAgwMsgExternalQueryThirdIssueSecurityInfoResult = 7480703; ///< AGW新三板发行证券信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryUniIPOQty = 7301742; ///< 新股发行申购额度统一查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultUniIPOQty = 7401742; ///< 新股发行申购额度查询结果
const MsgTypeNo_def kAgwMsgExtQueryNetVotingOrder = 7300709; ///< 网络投票订单查询
const MsgTypeNo_def kAgwMsgExtQueryResultNetVotingOrder = 7400709; ///< 网络投票订单查询结果
const MsgTypeNo_def kAgwMsgBondConsultTradeOrder = 1037001; ///< 债券协商成交委托
const MsgTypeNo_def kAgwMsgBondConsultTradeCancelOrder = 19037001; ///< 债券协商成交委托撤单委托
const MsgTypeNo_def kAgwMsgBondConsultTradeTER = 2003715; ///< 债券协商成交委托成交回报
const MsgTypeNo_def kAgwMsgExternalQueryOrderForwardInfo = 3000050; ///< 债券协商成交转发成交申报查询消息
const MsgTypeNo_def kAgwMsgExternalQueryResultOrderForwardInfo = 3100050; ///< 债券协商成交转发成交申报查询结果消息
const MsgTypeNo_def kAgwMsgExternalQueryBondMember = 7301745; ///< 交易商信息查询
const MsgTypeNo_def kAgwMsgExternalQueryResultBondMember = 7401745; ///< 交易员信息查询结果
const MsgTypeNo_def kAgwMsgExternalQueryTraderInvestor = 7301746; ///< 交易信息查询
const MsgTypeNo_def kAgwMsgExternalQueryResultTraderInvestor = 7401746; ///< 交易主体信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryBondMember = 7301745; ///< 交易商信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultBondMember = 7401745; ///< 交易商信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryBondInfo = 7301746; ///< 交易信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultBondInfo = 7401746; ///< 交易信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryThirdAdjustAsset = 7301748; ///< 新三板在途资产数据查询
const MsgTypeNo_def kAgwMsgETFBasketOrder = 7127701; ///< ETF篮子股票买卖委托
const MsgTypeNo_def kAgwMsgETFBasketTradeER = 7227715; ///< ETF篮子股票买卖订单执行报告
const MsgTypeNo_def kAgwMsgOfflineETFSubscribe = 7130701; ///< ETF网下认购（股票和现金）
const MsgTypeNo_def kAgwMsgOfflineETFSubscribeCancelOrder = 7147703; ///< 网下ETF认购撤销申请
const MsgTypeNo_def kAgwMsgOfflineETFSubscribeCancelAck = 7247717; ///< 网下ETF认购撤销申请响应
const MsgTypeNo_def kAgwMsgExtQryOfflineETFSubscribeQtyBySecurity = 7300729; ///< 网下ETF认购查询股票可认购数量
const MsgTypeNo_def kAgwMsgExtQryResultOfflineETFSubscribeQtyBySecurity = 7400729; ///< 网下ETF认购查询股票可认购数量查询结果
const MsgTypeNo_def kAgwMsgExtQueryResultThirdAdjustAsset = 7401748; ///< 新三板在途资产数据查询结果
const MsgTypeNo_def kAgwMsgExtQueryHKTMd = 7301749; ///< 港股通证券行情信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultHKTMd = 7401749; ///< 港股通证券行情信息查询结果
const MsgTypeNo_def kAgwMsgExternalQueryOfferTenderer = 7300728; ///< 要约收购人信息查询
const MsgTypeNo_def kAgwMsgExternalQueryResultOfferTenderer = 7400728; ///< 要约收购人信息查询结果
const MsgTypeNo_def kAgwMsgExternalQuerySaleLimitStkAsset = 7301750; ///< 限售股信息查询
const MsgTypeNo_def kAgwMsgExternalQueryResultSaleLimitStkAsset = 7401750; ///< 限售股信息查询结果
const MsgTypeNo_def kAgwMsgExtQryETFCreation = 7300730; ///< 网下ETF认购查询
const MsgTypeNo_def kAgwMsgExtQryResultETFCreation = 7400730; ///< 网下ETF认购查询结果
const MsgTypeNo_def kAgwMsgOfRiskSign = 7146703; ///< 基金风险揭示签署
const MsgTypeNo_def kAgwMsgOfRiskSignAck = 7246717; ///< 基金风险揭示签署结果
const MsgTypeNo_def kAgwMsgLOFDividePolicySetOrder = 7131701; ///< 外围深圳场内LOF基金分红方式设置
const MsgTypeNo_def kAgwMsgExtQueryEtfStkAsset = 7301751; ///< ETF客户股票篮查询
const MsgTypeNo_def kAgwMsgExtQueryResultEtfStkAsset = 7401751; ///< ETF客户股票篮查询结果
const MsgTypeNo_def kAgwMsgExtQueryEtfBatchOrder = 7301752; ///< ETF查询客户当日批量委托
const MsgTypeNo_def kAgwMsgExtQueryResultEtfBatchOrder = 7401752; ///< ETF查询客户当日批量委托查询结果
const MsgTypeNo_def kAgwMsgExtQueryPledgeBond = 7301753; ///< 可入质债券查询
const MsgTypeNo_def kAgwMsgExtQryResultPledgeBond = 7401753; ///< 可入质债券查询结果
const MsgTypeNo_def kAgwMsgExternalQueryLOFDividePolicyOrder = 7300732; ///< 新外围委托查询(LOF基金分红委托专用)查询消息
const MsgTypeNo_def kAgwMsgExternalQueryResultLOFDividePolicyOrder = 7400732; ///< 新外围委托查询(LOF基金分红委托专用)查询结果消息
const MsgTypeNo_def kAgwMsgExternalQuerySaleFee = 7300711; ///< 客户证券卖出费用查询消息
const MsgTypeNo_def kAgwMsgExternalQueryResultSaleFee = 7400711; ///< 客户证券卖出费用查询结果
const MsgTypeNo_def kAgwMsgExtQryDaysOfRepoFundOccupation = 7300721; ///< 债券质押式回购实际占款天数查询
const MsgTypeNo_def kAgwMsgExtQryRstDaysOfRepoFundOccupation = 7400721; ///< 债券质押式回购实际占款天数查询结果
const MsgTypeNo_def kAgwMsgExtQryCustSTDBonds = 7300722; ///< 客户新标准券查询
const MsgTypeNo_def kAgwMsgExtQryRstCustSTDBonds = 7400722; ///< 客户新标准券查询回报
const MsgTypeNo_def kAgmMsgExternalQueryUnexpiredRepo = 7300723; ///< 未到期回购查询
const MsgTypeNo_def kAgwMsgExternalQueryResultUnexpiredRepo = 7400723; ///< 未到期回购查询结果
const MsgTypeNo_def kAgwMsgExternalQueryReverseRepoTrade = 7300724; ///< 当日逆回购成交查询
const MsgTypeNo_def kAgwMsgExternalQueryResultReverseRepoTrade = 7400724; ///< 当日逆回购成交查询结果
const MsgTypeNo_def kAgwMsgExtQueryPBOrder = 7301754; ///< PB专用订单查询
const MsgTypeNo_def kAgwMsgExtQueryResultPBOrder = 7401754; ///< PB订单查询结果
const MsgTypeNo_def kAgwMsgExtQueryPBTradeOrder = 7301755; ///< PB成交查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultPBTradeOrder = 7401755; ///< PB成交查询结果
const MsgTypeNo_def kAgwMsgExtQueryAssetFrzPackage = 7390741; ///< 资产冻结包查询接口
const MsgTypeNo_def kAgwMsgExtQueryResultAssetFrzPackage = 7490741; ///< 资产冻结包查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryConvertibleBondForcedRedemption = 7301756; ///< 可转债强赎证券信息查询接口
const MsgTypeNo_def kAgwMsgExtQueryResultConvertibleBondForcedRedemption = 7401756; ///< 可转债强赎证券信息结果消息
const MsgTypeNo_def kAgwMsgExtQueryAdvanceRedressInfo = 7301757; ///< 沪市先行赔付信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultAdvanceRedressInfo = 7401757; ///< 沪市先行赔付信息查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryNewReverseRepoPosition = 7301758; ///< 新国债逆回购持仓查询
const MsgTypeNo_def kAgwMsgExtQueryResultNewReverseRepoPosition = 7401758; ///< 新国债逆回购持仓查询结果
const MsgTypeNo_def kAgwMsgExtQueryTenderOfferSecurityInfo = 7301762; ///< 沪深要约收购行情查询
const MsgTypeNo_def kAgwMsgExtQueryResultTenderOfferSecurityInfo = 7401762; ///< 沪深要约收购行情查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryTradeMarketCollect = 7301763; ///< 按市场成交汇总查询消息
const MsgTypeNo_def kAgwExtQueryResultTradeMarketCollect = 7401763; ///< 按市场成交汇总查询结果
const MsgTypeNo_def kAgwMsgExtQueryTradeSecurityCollect = 7301764; ///< 按证券成交汇总查询消息
const MsgTypeNo_def kAgwExtQueryResultTradeSecurityCollect = 7401764; ///< 按证券成交汇总查询结果组合
const MsgTypeNo_def kAgwMsgDirectRepayMultiAppoint = 7110701; ///< 顶点两融多合约还款业务订单
const MsgTypeNo_def kAgwMsgDirectRepayMultiAppointResult = 7210702; ///< 顶点两融多合约还款业务应答
const MsgTypeNo_def kAgwMsgMultiDebtsDelayApply = 7108702; ///< 顶点两融多合约展期申请
const MsgTypeNo_def kAgwMsgMultiDebtsDelayApplyResult = 7208716; ///< 顶点两融多合约展期申请应答
const MsgTypeNo_def kAgwMsgMultiDebtsDelayApplyByDays = 7108703; ///< 两融多合约展期申请(按到期天数)
const MsgTypeNo_def kAgwMsgMultiDebtsDelayApplyByDaysResult = 7208717; ///< 两融多合约展期申请(按到期天数)应答
const MsgTypeNo_def kAgwMsgExtQueryDDFund = 7390765; ///< 顶点查询客户资金消息
const MsgTypeNo_def kAgwMsgExtQueryResultDDFund = 7490765; ///< 顶点查询客户资金消息结果
const MsgTypeNo_def kAgwMsgExtQueryDDSpecialOrder = 7390766; ///< 订单查询(自定义编号)
const MsgTypeNo_def kAgwMsgExtQueryResultDDSpecialOrder = 7490766; ///< 订单查询结果
const MsgTypeNo_def kAgwMsgExtQueryDDSecurityInfo = 7390767; ///< 顶点全量证券信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultDDSecurityInfo = 7490767; ///< 顶点全量证券信息查询结果
const MsgTypeNo_def kAgwMsgExtQueryUTERevertNGTP = 7390769; ///< UTE回切NGTP消息查询
const MsgTypeNo_def kAgwMsgExtUTERevertNGTPResult = 7490769; ///< UTE回切NGTP消息
const MsgTypeNo_def kAgwMsgSingleAccountRevertNGTP = 7390770; ///< UTE单客户/UTE节点回切NGTP消息
const MsgTypeNo_def kAgwMsgExtQueryCRCPreview = 7390789; ///< CRC风控试算查询请求
const MsgTypeNo_def kAgwMsgExtQueryResultCRCPreview = 7490789; ///< CRC风控试算查询结果
const MsgTypeNo_def kAgwMsgExternalQueryUnitTimeOrderAmt = 7390799; ///< 客户单位时间委托金额查询
const MsgTypeNo_def kAgwMsgExternalQueryResultUnitTimeOrderAmt = 7490799; ///< 客户单位时间委托金额查询结果
const MsgTypeNo_def kAgwMsgExtQueryThreePartyOrder = 7301765; ///< 三方专用订单查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultThreePartyOrder = 7401765; ///< 三方专用订单查询结果
const MsgTypeNo_def kAgwMsgExtQueryThreePartyTradeOrder = 7301766; ///< 三方专用成交查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultThreePartyTradeOrder = 7401766; ///< 三方专用成交查询结果
const MsgTypeNo_def kAgwMsgExtQueryG1ShareControl = 7301767; ///< G1客户股票风控查询
const MsgTypeNo_def kAgwMsgExtQueryResultG1ShareControl = 7401767; ///< G1客户股票风控查询结果
const MsgTypeNo_def kAgwMsgG1ShareControlSet = 7150703; ///< G1客户股票风控设置
const MsgTypeNo_def kAgwMsgG1ShareControlSetResult = 7250717; ///< G1客户股票风控设置应答
const MsgTypeNo_def kAgwMsgExtQuerySwapPutbackInfo = 7301768; ///< 转股回售证券信息查询
const MsgTypeNo_def kAgwMsgExtQueryResultSwapPutbackInfo = 7401768; ///< 转股回售证券信息查询结果消息
const MsgTypeNo_def kAgwMsgExtCustSecurityMtmIndicatorPush = 7930750; ///< 客户级券盯市指标推送
const MsgTypeNo_def kAgwMsgExtCustMtmIndicatorPush = 7930751; ///< 客户级盯市指标推送
const MsgTypeNo_def kAgwMsgExtCompanySecurityMtmIndicatorPush = 7930752; ///< 公司级券盯市指标推送
const MsgTypeNo_def kAgwMsgExtQueryCustStkInd = 7390771; ///< 客户级盯市指标查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCustStkInd = 7490771; ///< 客户级盯市指标查询结果消息
const MsgTypeNo_def kAgwMsgExtQueryCompStkInd = 7390772; ///< 公司级盯市指标查询消息
const MsgTypeNo_def kAgwMsgExtQueryResultCompStkInd = 7490772; ///< 公司级盯市指标查询结果消息
class EXPORT_MACRO_DEFINE MsgMapper
{
public:
static std::map<MsgTypeNo_def, std::string> msg_mapper_g;
};
/*
 * @brief id string mapping
 */
inline std::string GetPacketName(const MsgTypeNo_def id)
{
    std::map<MsgTypeNo_def, std::string>::iterator iter = 
        MsgMapper::msg_mapper_g.find(id);
    if (iter != MsgMapper::msg_mapper_g.end())
    {
        return iter->second;
    }
    else
    {
        return "";
    }
}
/*
 * @brief 消息头
 */
class EXPORT_MACRO_DEFINE AgwHeader : public mp::RootPacket
{
public:
MsgTypeNo_def msg_type; 		///< 消息类型
VersionNo_def msg_ver; 		///< 消息版本号
Reserver_def reserver; 		///< 保留域，未启用
SeqNumHeader_def seq_num; 		///< 消息序号
BodyLength_def body_length; 		///< 消息长度
/*
 * @brief 默认构造函数
 */
  AgwHeader();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 2 + 2 + 4 + 4;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwHeader& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 登录消息
 */
class EXPORT_MACRO_DEFINE AgwMsgLogin : public mp::RootPacket
{
public:
SenderCompID_def sender_comp_id; 		///< 发送方代码
TargetCompID_def target_comp_id; 		///< 接收方代码
HeartBtInt_def heart_bt_int; 		///< 心跳间隔时间
Password_def password; 		///< 用户登录密码
DefaultAppVerID_def default_app_ver_id; 		///< 协议版本
Uint8_def spec_cust_flag; 		///< 特殊客户标识,正常为0
String_def trade_pwd; 		///< 客户交易密码-特殊用户使用
/*
 * @brief 默认构造函数
 */
  AgwMsgLogin();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgLogin& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgLogin;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 注销消息
 */
class EXPORT_MACRO_DEFINE AgwMsgLogout : public mp::RootPacket
{
public:
SessionStatus_def session_status; 		///< 退出时的会话状态
TextRes_def text; 		///< 注销原因
/*
 * @brief 默认构造函数
 */
  AgwMsgLogout();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 256;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgLogout& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgLogout;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 心跳消息
 */
class EXPORT_MACRO_DEFINE AgwMsgHeartBeat : public mp::RootPacket
{
public:
TestReqID_def test_req_id; 		///< 测试请求标志，全空表示心跳消息
/*
 * @brief 默认构造函数
 */
  AgwMsgHeartBeat();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgHeartBeat& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgHeartBeat;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 测试请求消息
 */
class EXPORT_MACRO_DEFINE AgwMsgTestRequest : public mp::RootPacket
{
public:
TestReqID_def test_req_id; 		///< 测试请求标志，全空表示心跳消息
/*
 * @brief 默认构造函数
 */
  AgwMsgTestRequest();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgTestRequest& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgTestRequest;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 分区记录号信息数组
 */
class EXPORT_MACRO_DEFINE AgwNoPartitionArray : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 回报记录号
/*
 * @brief 默认构造函数
 */
  AgwNoPartitionArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 4;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwNoPartitionArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 回报同步消息
 */
class EXPORT_MACRO_DEFINE AgwMsgSync : public mp::RootPacket
{
public:
std::vector<AgwNoPartitionArray> no_parition_array; 		///< 分区记录号信息
/*
 * @brief 默认构造函数
 */
  AgwMsgSync();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSync& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSync;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 回报同步结束消息
 */
class EXPORT_MACRO_DEFINE AgwMsgSyncFinish : public mp::RootPacket
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgSyncFinish();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSyncFinish& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSyncFinish;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 拒绝代码描述数组
 */
class EXPORT_MACRO_DEFINE AgwRejectReasonArray : public mp::RootPacket
{
public:
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_text; 		///< 拒绝原因描述
/*
 * @brief 默认构造函数
 */
  AgwRejectReasonArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwRejectReasonArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 拒绝消息代码同步消息
 */
class EXPORT_MACRO_DEFINE AgwMsgRejectReasonCodeSync : public mp::RootPacket
{
public:
RejectCodeType_def reject_code_type; 		///< 拒绝代码类型
std::vector<AgwRejectReasonArray> reject_reason_array; 		///< 拒绝代码描述信息
/*
 * @brief 默认构造函数
 */
  AgwMsgRejectReasonCodeSync();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRejectReasonCodeSync& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRejectReasonCodeSync;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 接入控制消息
 */
class EXPORT_MACRO_DEFINE AgwMsgRequestAccessData : public mp::RootPacket
{
public:
APIType_def api_type; 		///< Api厂商类型，1：华锐API
IP_def ip; 		///< ip地址
Mac_def mac; 		///< mac地址
License_def license; 		///< license序列号
/*
 * @brief 默认构造函数
 */
  AgwMsgRequestAccessData();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRequestAccessData& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRequestAccessData;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 功能权限列表
 */
class EXPORT_MACRO_DEFINE AgwFuncCodeArray : public mp::RootPacket
{
public:
MsgType_def func_code; 		///< 消息类型(功能号)
/*
 * @brief 默认构造函数
 */
  AgwFuncCodeArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwFuncCodeArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户号功能权限列表
 */
class EXPORT_MACRO_DEFINE AgwCustIDFuncCodeArray : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
std::vector<AgwFuncCodeArray> func_code_array; 		///< 功能号组合
/*
 * @brief 默认构造函数
 */
  AgwCustIDFuncCodeArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCustIDFuncCodeArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户号列表
 */
class EXPORT_MACRO_DEFINE AgwCustIDArray : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
/*
 * @brief 默认构造函数
 */
  AgwCustIDArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCustIDArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 接入控制结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgReportAccessData : public mp::RootPacket
{
public:
LinkNum_def link_num; 		///< 最大连接数限制
SpeedNum_def speed_num; 		///< 最大消息个数(单位: /每秒)
Bool_def is_blacklist; 		///< 是否是黑名单,0.白名单；1.黑名单
License_def license; 		///< License序列号
Bool_def is_enabled; 		///< license是否启用
Date8_def start_date; 		///< 开始日期(20190101)
Date8_def end_date; 		///< 截止日期(20191231)
std::vector<AgwCustIDArray> black_white_custid_list; 		///< 客户号列表
/*
 * @brief 默认构造函数
 */
  AgwMsgReportAccessData();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgReportAccessData& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgReportAccessData;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 客户号功能权限消息
 */
class EXPORT_MACRO_DEFINE AgwMsgPushFuncCodeList : public mp::RootPacket
{
public:
Bool_def is_last_one; 		///< 是否最后的报文
std::vector<AgwCustIDFuncCodeArray> custid_func_code_array; 		///< 客户号权限列表
/*
 * @brief 默认构造函数
 */
  AgwMsgPushFuncCodeList();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgPushFuncCodeList& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgPushFuncCodeList;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 硬盘序列号列表
 */
class EXPORT_MACRO_DEFINE AgwDiskSerailNumberArray : public mp::RootPacket
{
public:
String_def disk_serial_number; 		///< 客户号
/*
 * @brief 默认构造函数
 */
  AgwDiskSerailNumberArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwDiskSerailNumberArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 开放端口号列表
 */
class EXPORT_MACRO_DEFINE AgwPortArray : public mp::RootPacket
{
public:
Port_def port; 		///< 端口号
/*
 * @brief 默认构造函数
 */
  AgwPortArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwPortArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 连接实时流速组合
 */
class EXPORT_MACRO_DEFINE AgwRealTimeSpeedArray : public mp::RootPacket
{
public:
Port_def port; 		///< 连接端口号
IP_def trade_gw_ip; 		///< 交易网关IP地址
SpeedNum_def real_time_speed; 		///< 连接实时流速/s(单位: /每秒)
TimeStamp_def login_time; 		///< 登录时间
/*
 * @brief 默认构造函数
 */
  AgwRealTimeSpeedArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 16 + 4 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwRealTimeSpeedArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief API终端采集数据
 */
class EXPORT_MACRO_DEFINE AgwMsgTerminalInfoCollection : public mp::RootPacket
{
public:
APIType_def api_type; 		///< API厂商类型,1:华锐API
PID_def pid; 		///< 进程pid
IP_def ip; 		///< ip地址
Mac_def mac; 		///< mac地址
Port_def port; 		///< 当前连接端口号
String_def license; 		///< license序列号
SpeedNum_def real_time_link_num; 		///< 实时连接数
AccessMode_def access_mode; 		///< 接入方式，默认填1即为API
String_def client_name; 		///< 客户端名称
String_def client_version; 		///< 客户端版本号
std::vector<AgwRealTimeSpeedArray> real_time_speed_array; 		///< 连接实时流速组合
std::vector<AgwDiskSerailNumberArray> disk_serial_number_array; 		///< 硬盘序列号组合
std::vector<AgwCustIDArray> traded_custid_array; 		///< 已交易过的客户号组合
std::vector<AgwPortArray> open_port_array; 		///< 主机开放端口组合
/*
 * @brief 默认构造函数
 */
  AgwMsgTerminalInfoCollection();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgTerminalInfoCollection& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgTerminalInfoCollection;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 实时控制指令消息
 */
class EXPORT_MACRO_DEFINE AgwMsgRealTimeControlInstruction : public mp::RootPacket
{
public:
TimeStamp_def send_time; 		///< 发送时间
InstructionID_def instruction_id; 		///< 指令ID，同一登陆会话唯一
InstructionType_def instruction_type; 		///< 网关支持的实时指令类型
Comments_def comments; 		///< 备注信息
String_def instruction_data; 		///< Json
/*
 * @brief 默认构造函数
 */
  AgwMsgRealTimeControlInstruction();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRealTimeControlInstruction& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRealTimeControlInstruction;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 1;}
};
/*
 * @brief 实时控制指令结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgRealTimeControlInstructionResult : public mp::RootPacket
{
public:
InstanceID_def from; 		///< 结果源
TimeStamp_def send_time; 		///< 发送时间
InstructionID_def instruction_id; 		///< 指令ID
InstructionType_def instruction_type; 		///< 网关支持的实时指令类型
Result_def result; 		///< 处理结果
String_def instruction_data; 		///< Json
/*
 * @brief 默认构造函数
 */
  AgwMsgRealTimeControlInstructionResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRealTimeControlInstructionResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRealTimeControlInstructionResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 1;}
};
/*
 * @brief 实时查询指令消息
 */
class EXPORT_MACRO_DEFINE AgwMsgRealTimeQueryInstruction : public mp::RootPacket
{
public:
TimeStamp_def send_time; 		///< 发送时间
InstructionID_def query_id; 		///< 查询指令ID
InstructionType_def query_type; 		///< 网关支持的查询指令类型
String_def instruction_data; 		///< Json
/*
 * @brief 默认构造函数
 */
  AgwMsgRealTimeQueryInstruction();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRealTimeQueryInstruction& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRealTimeQueryInstruction;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 1;}
};
/*
 * @brief 实时查询指令消息结果
 */
class EXPORT_MACRO_DEFINE AgwMsgRealTimeQueryInstructionResult : public mp::RootPacket
{
public:
InstanceID_def from; 		///< 结果源
TimeStamp_def send_time; 		///< 发送时间
InstructionID_def query_id; 		///< 查询指令ID
InstructionType_def query_type; 		///< 网关支持的查询指令类型
InsRejectReasonCode_def result; 		///< 拒绝原因代码
String_def instruction_data; 		///< Json
/*
 * @brief 默认构造函数
 */
  AgwMsgRealTimeQueryInstructionResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRealTimeQueryInstructionResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRealTimeQueryInstructionResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 1;}
};
/*
 * @brief 外部业务拒绝消息
 */


/*
 * @brief 外部业务拒绝消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBizRejection : public mp::RootPacket
{
public:
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
MsgType_def msg_type; 		///< 被拒绝消息的消息类型
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
BusinessRejectText_def business_reject_text; 		///< 拒绝原因描述
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgBizRejection();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBizRejection& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBizRejection;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 登出请求
 */
class EXPORT_MACRO_DEFINE AgwMsgCustLogoutReq : public mp::RootPacket
{
public:
Time_def time; 		///< 发起登验证时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
String_def extra_data; 		///< 自定义json字符串
/*
 * @brief 默认构造函数
 */
  AgwMsgCustLogoutReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustLogoutReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustLogoutReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 登出应答
 */
class EXPORT_MACRO_DEFINE AgwMsgCustLogoutResp : public mp::RootPacket
{
public:
Time_def time; 		///< 发起登验证时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PermissonErrorCode_def permisson_error_code; 		///< 密码校验服务错误码
UserInfo_def user_info; 		///< 用户私有信息
String_def extra_data; 		///< 自定义json字符串
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgCustLogoutResp();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustLogoutResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustLogoutResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 登入请求
 */
class EXPORT_MACRO_DEFINE AgwMsgCustLoginReq : public AgwMsgCustLogoutReq
{
public:
OrderWay_def order_way; 		///< 委托方式
PWD_def password; 		///< 客户密码
Pwd_Type_def pwd_type; 		///< 密码加密类型
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgCustLoginReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustLoginReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustLoginReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 登入应答证券账户重复组
 */
class EXPORT_MACRO_DEFINE AgwMsgCustLoginRespAccountUnit : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户ID
MarketID_def market_id; 		///< 证券账户所属市场
AccountRole_def account_role; 		///< 账户角色
PartitionNo_def partition_no; 		///< 分区号
/*
 * @brief 默认构造函数
 */
  AgwMsgCustLoginRespAccountUnit();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustLoginRespAccountUnit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 登入应答资金账户重复组
 */
class EXPORT_MACRO_DEFINE AgwMsgCustLoginRespFundAccountUnit : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
FundAccountAttr_def fund_account_attr; 		///< 资金帐号属性
std::vector<AgwMsgCustLoginRespAccountUnit> account_array; 		///< 登入应答证券账户重复组
CreditFlag_def credit_flag; 		///< 信用账号标志
/*
 * @brief 默认构造函数
 */
  AgwMsgCustLoginRespFundAccountUnit();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustLoginRespFundAccountUnit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 登入应答
 */
class EXPORT_MACRO_DEFINE AgwMsgCustLoginResp : public mp::RootPacket
{
public:
SeqNum_def client_seq_id; 		///< 用户系统消息序号
TimeStamp_def transact_time; 		///< 处理时间
CustID_def cust_id; 		///< 客户号
CustName_def cust_name; 		///< 客户姓名
PermissonErrorCode_def permisson_error_code; 		///< 密码校验服务错误码
std::vector<AgwMsgCustLoginRespFundAccountUnit> fund_account_array; 		///< 登入应答资金账户重复组
UserInfo_def user_info; 		///< 用户私有信息
Qty_def pwd_err_time; 		///< 登录错误次数
BankCode_def bank_code; 		///< 银行代码
String_def pwd_ciphertext; 		///< 密码密文
Pwd_Type_def pwd_type; 		///< 密码加密类型
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
String_def extra_data; 		///< 自定义json字符串
CifAccount_def cif_account; 		///< 一码通账号
/*
 * @brief 默认构造函数
 */
  AgwMsgCustLoginResp();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustLoginResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustLoginResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外部资金划拨请求
 */
class EXPORT_MACRO_DEFINE AgwMsgExtFundTransfer : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Bool_def is_account_partition; 		///< 账户是否按证券账户分区 1-是 0否
Currency_def currency; 		///< 货币种类
TransferWay_def transfer_way; 		///< 划拨方式：1-转入；2-转出
Amt_def transfer_value; 		///< 划拨金额
String_def remark; 		///< 备注
PWD_def password; 		///< 密码
OrderWay_def order_way; 		///< 委托方式
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwMsgExtFundTransfer();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtFundTransfer& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtFundTransfer;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外部资金划拨应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtFundTransferResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Currency_def currency; 		///< 货币种类
Amt_def fund_bal; 		///< 资金余额
Amt_def pre_fund_bal; 		///< 调整前资金余额
Amt_def pre_available_t0; 		///< 调整前T+0可用资金
Amt_def pre_on_the_way_t1; 		///< 调整前T+1在途可用资金
Amt_def pre_on_the_way_t2; 		///< 调整前T+2在途可用资金
Amt_def pre_on_the_way_t3; 		///< 调整前T+3在途可用资金
Amt_def transfer_value; 		///< 转移金额
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtFundTransferResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtFundTransferResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtFundTransferResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户密码修改请求
 */
class EXPORT_MACRO_DEFINE AgwMsgCustPasswdModify : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
PWD_def old_password; 		///< 原密码
PWD_def password; 		///< 新密码
OrderWay_def order_way; 		///< 委托方式
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwMsgCustPasswdModify();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustPasswdModify& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustPasswdModify;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户密码修改应答
 */
class EXPORT_MACRO_DEFINE AgwMsgCustPasswdModifyResult : public mp::RootPacket
{
public:
SeqNum_def client_seq_id; 		///< 用户系统消息序号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwMsgCustPasswdModifyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustPasswdModifyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustPasswdModifyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 认证请求
 */
class EXPORT_MACRO_DEFINE AgwMsgCustAuthReq : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
CustID_def cust_id; 		///< 客户号
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwMsgCustAuthReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustAuthReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustAuthReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 认证应答
 */
class EXPORT_MACRO_DEFINE AgwMsgCustAuthResp : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwMsgCustAuthResp();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 16 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustAuthResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCustAuthResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 同步客户回报消息
 */
class EXPORT_MACRO_DEFINE AgwMsgSyncBiz : public mp::RootPacket
{
public:
SeqNum_def client_seq_id; 		///< 用户系统消息序号
std::vector<AgwNoPartitionArray> no_parition_array; 		///< 分区记录号信息
/*
 * @brief 默认构造函数
 */
  AgwMsgSyncBiz();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSyncBiz& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSyncBiz;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 同步客户回报结束消息
 */
class EXPORT_MACRO_DEFINE AgwMsgSyncBizFinish : public mp::RootPacket
{
public:
RejectReasonCode_def reason_code; 		///< 0:正常返回，1：索引错误,2:会话未就绪
String_def reason_message; 		///< 错误原因
SeqNum_def client_seq_id; 		///< 用户系统消息序号
/*
 * @brief 默认构造函数
 */
  AgwMsgSyncBizFinish();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSyncBizFinish& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSyncBizFinish;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融VIP登入请求
 */
class EXPORT_MACRO_DEFINE AgwMsgVipCustLoginReq : public AgwMsgCustLogoutReq
{
public:
PWD_def password; 		///< 客户密码
Pwd_Type_def pwd_type; 		///< 密码加密类型
HeartBtInt_def heart_bt_int; 		///< 心跳间隔时间
/*
 * @brief 默认构造函数
 */
  AgwMsgVipCustLoginReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgVipCustLoginReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgVipCustLoginReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 委托公共部分字段
 */
class EXPORT_MACRO_DEFINE AgwMsgOrder : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
TimeStamp_def client_order_time; 		///< 委托时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
AccountID_def account_id; 		///< 账户id
Side_def side; 		///< 买卖方向
OrdType_def order_type; 		///< 订单类型
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 申报数量(张)/ETF份额
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
OrderWay_def order_way; 		///< 委托方式
UserInfo_def user_info; 		///< 用户私有信息
PWD_def password; 		///< 密码
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
PolicyID_def policy_id; 		///< 策略编号
/*
 * @brief 默认构造函数
 */
  AgwMsgOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 带强制风险标识委托公共部分字段
 */
class EXPORT_MACRO_DEFINE AgwNewMsgOrder : public AgwMsgOrder
{
public:
EnforceFlag_def enforce_flag; 		///< 强制风险标识
/*
 * @brief 默认构造函数
 */
  AgwNewMsgOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwNewMsgOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 现货集中竞价业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCashAuctionOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
CashMargin_def cash_margin; 		///< 信用标识
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgCashAuctionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCashAuctionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCashAuctionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 科创板股票盘后定价业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgTibAfterHourAuctionOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
CashMargin_def cash_margin; 		///< 信用标识
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgTibAfterHourAuctionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgTibAfterHourAuctionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgTibAfterHourAuctionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网上发行业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgIssueOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
SpecialIPOType_def special_ipo_type; 		///< 特殊发行类型
/*
 * @brief 默认构造函数
 */
  AgwMsgIssueOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgIssueOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgIssueOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板网上发行业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdIssueOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdIssueOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdIssueOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdIssueOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 配股业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgRightsIssueOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgRightsIssueOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRightsIssueOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRightsIssueOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 权证行权业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgWarrantExerciseOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgWarrantExerciseOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgWarrantExerciseOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgWarrantExerciseOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgHKStocksThroughAuctionOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
LotType_def lot_type; 		///< 订单类型
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgHKStocksThroughAuctionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgHKStocksThroughAuctionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgHKStocksThroughAuctionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通公司行为公共撤单消息
 */
class EXPORT_MACRO_DEFINE AgwMsgHKCorporateActionCancelOrder : public AgwMsgOrder
{
public:
ClOrdNO_def orig_cli_ord_no; 		///< 原委托的客户订单编号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgHKCorporateActionCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgHKCorporateActionCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgHKCorporateActionCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief FDEP公共撤单消息
 */
class EXPORT_MACRO_DEFINE AgwMsgFDEPCancelOrder : public AgwMsgOrder
{
public:
ClOrdNO_def orig_cli_ord_no; 		///< 原委托的客户订单编号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgFDEPCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgFDEPCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgFDEPCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 通用撤单消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCancelOrder : public AgwMsgOrder
{
public:
ClOrdNO_def orig_cli_ord_no; 		///< 原委托的客户订单编号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
SeqNum_def orig_client_seq_id; 		///< 原客户端消息编号
/*
 * @brief 默认构造函数
 */
  AgwMsgCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 一键撤单消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBatchCancelOrder : public AgwMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBatchCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBatchCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBatchCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 报告公共字段
 */
class EXPORT_MACRO_DEFINE AgwMsgOrderReport : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
BusinessType_def business_type; 		///< 业务类型
ClOrdNO_def client_order_no; 		///< 客户订单号
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
ExecType_def exec_type; 		///< 当前委托执行类型
OrdStatus_def order_status; 		///< 当前申报的状态
CustID_def cust_id; 		///< 客户号ID
FundAccountID_def fund_account_id; 		///< 资金账户ID
AccountID_def account_id; 		///< 投资者帐号
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 委托数量
Qty_def leaves_qty; 		///< 未成交数量
Qty_def cum_qty; 		///< 累计成交数量
Side_def side; 		///< 买卖方向
TimeStamp_def transact_time; 		///< 回报时间
UserInfo_def user_info; 		///< 用户私有信息
OrderID_def order_id; 		///< 交易所订单编号, 取值为数字
ClOrdID_def cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgOrderReport();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOrderReport& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 成交执行报告
 */
class EXPORT_MACRO_DEFINE AgwMsgTradeExecutionReport : public AgwMsgOrderReport
{
public:
ExecID_def exec_id; 		///< 成交编号
Amt_def last_px; 		///< 成交价格
Qty_def last_qty; 		///< 成交数量
Amt_def total_value_traded; 		///< 成交金额
Amt_def fee; 		///< 费用
/*
 * @brief 默认构造函数
 */
  AgwMsgTradeExecutionReport();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgTradeExecutionReport& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 成交执行报告client_seq_id
 */
class EXPORT_MACRO_DEFINE AgwMsgTradeExecutionReportSeqNum : public AgwMsgTradeExecutionReport
{
public:
SeqNum_def client_seq_id; 		///< 用户系统消息序号
/*
 * @brief 默认构造函数
 */
  AgwMsgTradeExecutionReportSeqNum();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgTradeExecutionReportSeqNum& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 委托状态响应
 */
class EXPORT_MACRO_DEFINE AgwMsgOrderStatusAck : public AgwMsgOrderReport
{
public:
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClOrdNO_def orig_cli_ord_no; 		///< 对于撤单返回的状态响应，为原始委托的客户合同号，指示被撤消订单的ClOrdID; 对于委托确认，取值为空
Amt_def frozen_trade_value; 		///< 冻结交易金额
Amt_def frozen_fee; 		///< 冻结费用
RejectReasonCode_def reject_reason_code; 		///< 拒绝代码
OrdRejReason_def ord_rej_reason; 		///< 拒绝原因描述信息
ExErrMsg_def ex_err_msg; 		///< 交易所错误信息描述
OrdType_def ord_type; 		///< 订单类型
TimeInForce_def time_in_force; 		///< 订单有效时间类型
PositionEffect_def position_effect; 		///< 开仓/平仓
CoveredOrUncovered_def covered_or_uncovered; 		///< 备兑标签
AccountSubCode_def account_sub_code; 		///< 账户子编码
/*
 * @brief 默认构造函数
 */
  AgwMsgOrderStatusAck();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOrderStatusAck& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgOrderStatusAck;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 现货集中竞价业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgCashAuctionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
CashMargin_def cash_margin; 		///< 信用标识
/*
 * @brief 默认构造函数
 */
  AgwMsgCashAuctionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCashAuctionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCashAuctionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 科创板股票盘后定价业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgTibAfterHourAuctionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
CashMargin_def cash_margin; 		///< 信用标识
/*
 * @brief 默认构造函数
 */
  AgwMsgTibAfterHourAuctionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgTibAfterHourAuctionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgTibAfterHourAuctionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 质押式回购集中竞价业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgRepoAuctionOrder : public AgwNewMsgOrder
{
public:
Amt_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效时间类型
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgRepoAuctionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRepoAuctionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRepoAuctionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 质押式回购集中竞价业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgRepoAuctionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
Date8_def maturity_date; 		///< 到期日
/*
 * @brief 默认构造函数
 */
  AgwMsgRepoAuctionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgRepoAuctionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgRepoAuctionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券转股回售委托
 */
class EXPORT_MACRO_DEFINE AgwMsgSwapPutbackOrder : public AgwNewMsgOrder
{
public:
ShareNegotiability_def share_negotiability; 		///< 股份性质
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgSwapPutbackOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSwapPutbackOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSwapPutbackOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 开放式基金申赎业务订单
 */
class EXPORT_MACRO_DEFINE AgwMsgLOFOrder : public AgwNewMsgOrder
{
public:
Amt_def cash_order_qty; 		///< 申购金额
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgLOFOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgLOFOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgLOFOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 开放式基金申赎成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgLOFTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgLOFTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgLOFTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgLOFTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 质押出入库业务消息
 */
class EXPORT_MACRO_DEFINE AgwMsgPledgeInOrOutOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgPledgeInOrOutOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgPledgeInOrOutOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgPledgeInOrOutOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 质押出入库成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgPledgeInOrOutTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgPledgeInOrOutTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgPledgeInOrOutTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgPledgeInOrOutTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 密码服务业务消息
 */
class EXPORT_MACRO_DEFINE AgwMsgPasswordServiceOrder : public AgwNewMsgOrder
{
public:
ValidationCode_def validation_code; 		///< 密码激活校验号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgPasswordServiceOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgPasswordServiceOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgPasswordServiceOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网络投票业务消息
 */
class EXPORT_MACRO_DEFINE AgwMsgNetVotingOrder : public AgwNewMsgOrder
{
public:
GeneralMeetingSeq_def general_meeting_seq; 		///< 股东大会编码
VotingProposal_def voting_proposal; 		///< 投票议案编号
VotingPreference_def voting_preference; 		///< 投票意向
VotingSegment_def voting_segment; 		///< 分段统计段号（预留）
/*
 * @brief 默认构造函数
 */
  AgwMsgNetVotingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgNetVotingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgNetVotingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 要约收购业务消息
 */
class EXPORT_MACRO_DEFINE AgwMsgTenderOfferOrder : public AgwNewMsgOrder
{
public:
Tenderer_def tenderer; 		///< 要约收购参与人编码
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgTenderOfferOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgTenderOfferOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgTenderOfferOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF申赎业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgETFRedemptionOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
AccountID_def sh_account_id; 		///< 沪A股东代码
AccountID_def sz_account_id; 		///< 深A股东代码
/*
 * @brief 默认构造函数
 */
  AgwMsgETFRedemptionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFRedemptionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFRedemptionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 成分股数组
 */
class EXPORT_MACRO_DEFINE AgwMsgConstituentStockUnit : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Qty_def qty; 		///< 成交数量
Amt_def amt; 		///< 成交金额
Amt_def price; 		///< 成交价格
ETFTradeReportType_def etf_trade_report_type; 		///< ETF成交回报类型
ExecID_def exec_id; 		///< 执行编号
Char_def margin_amt_type; 		///< 上交所现金替代资金类型
/*
 * @brief 默认构造函数
 */
  AgwMsgConstituentStockUnit();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8 + 8 + 8 + 1 + 32 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgConstituentStockUnit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief ETF申赎业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgETFRedemptionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
std::vector<AgwMsgConstituentStockUnit> constituent_stock; 		///< 成分股与资金信息
/*
 * @brief 默认构造函数
 */
  AgwMsgETFRedemptionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFRedemptionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFRedemptionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券分销委托
 */
class EXPORT_MACRO_DEFINE AgwMsgBondDistributionOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBondDistributionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondDistributionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBondDistributionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券分销成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgBondDistributionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgBondDistributionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondDistributionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBondDistributionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 分级基金实时拆分合并委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgStructuredFundOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgStructuredFundOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgStructuredFundOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgStructuredFundOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 分级基金实时拆分合并成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgStructuredFundTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgStructuredFundTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgStructuredFundTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgStructuredFundTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgHKStocksThroughTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
Amt_def cny_net_total_value_trade; 		///< 成交金额，人民币
Amt_def cny_fee; 		///< 成交费用，人民币
/*
 * @brief 默认构造函数
 */
  AgwMsgHKStocksThroughTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgHKStocksThroughTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgHKStocksThroughTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通投票业务订单
 */
class EXPORT_MACRO_DEFINE AgwMsgHKVotingOrder : public AgwNewMsgOrder
{
public:
String_def announcement_number; 		///< 公告编号
String_def proxy_form_number; 		///< 议案编号
String_def affirmative_votes; 		///< 赞成数量
String_def negative_votes; 		///< 反对数量
String_def abstention_votes; 		///< 弃权数量
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgHKVotingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgHKVotingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgHKVotingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通公司行为业务订单
 */
class EXPORT_MACRO_DEFINE AgwMsgHKCorporateActionOrder : public AgwNewMsgOrder
{
public:
CorporateActionID_def corporate_action_id; 		///< 公司行为代码
ClOrdNO_def orig_cl_ord_no; 		///< 原订单编号(上海负数当日专用)
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgHKCorporateActionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgHKCorporateActionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgHKCorporateActionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通转托管业务订单
 */
class EXPORT_MACRO_DEFINE AgwMsgHKDesignationTransferOrder : public AgwNewMsgOrder
{
public:
ClOrdNO_def orig_cl_ord_no; 		///< 原委托的客户订单编号
DesignationTransType_def designation_trans_type; 		///< 转托管注册类型
EntitlementNumber_def entitlement_number; 		///< 权益编号
PBUID_def transferee_pbu_id; 		///< 转入交易单元代码
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgHKDesignationTransferOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgHKDesignationTransferOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgHKDesignationTransferOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 深结算dcom撤单委托
 */
class EXPORT_MACRO_DEFINE AgwMsgDcomCancelOrder : public AgwMsgOrder
{
public:
ClOrdNO_def orig_cli_ord_no; 		///< 原委托的客户订单编号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgDcomCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgDcomCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgDcomCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户普通买入/普通卖出业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCreditAuctionOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgCreditAuctionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCreditAuctionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCreditAuctionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户普通买入/普通卖出成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgCreditAuctionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgCreditAuctionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCreditAuctionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCreditAuctionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资买入/融券卖出业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgMarginTradingOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgMarginTradingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMarginTradingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgMarginTradingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资买入/融券卖出成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgMarginTradingTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgMarginTradingTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMarginTradingTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgMarginTradingTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 担保品划转业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCollateralInOrOutOrder : public AgwNewMsgOrder
{
public:
FundAccountID_def opp_fund_account_id; 		///< 对方资金账户ID
AccountID_def opp_account_id; 		///< 对方证券账户ID
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgCollateralInOrOutOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCollateralInOrOutOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCollateralInOrOutOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 第三方担保品转入直达信用节点业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdCollateralInOrder : public AgwNewMsgOrder
{
public:
FundAccountID_def opp_fund_account_id; 		///< 对方资金账户ID(填信用资金账户)
AccountID_def opp_account_id; 		///< 对方证券账户ID(填信用资金账户)
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdCollateralInOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdCollateralInOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdCollateralInOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 买券还券业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBuySecuRepayOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBuySecuRepayOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBuySecuRepayOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBuySecuRepayOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 买券还券成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgBuySecuRepayTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgBuySecuRepayTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBuySecuRepayTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBuySecuRepayTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 现券还券业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCurSecuRepayOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgCurSecuRepayOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCurSecuRepayOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCurSecuRepayOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 指定合约卖券还款业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgSellRepayAppointOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
Date8_def contract_date; 		///< 合约委托日期
Sno_def contract_sno; 		///< 合约流水号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgSellRepayAppointOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSellRepayAppointOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSellRepayAppointOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 指定合约卖券还款成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgSellRepayAppointTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgSellRepayAppointTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSellRepayAppointTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSellRepayAppointTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 直接还款请求
 */
class EXPORT_MACRO_DEFINE AgwMsgExtDirectRepay : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Bool_def is_account_partition; 		///< 账户是否按证券账户分区 1-是 0否
Currency_def currency; 		///< 货币种类
OrderWay_def order_way; 		///< 委托方式
Amt_def repay_value; 		///< 还款金额
String_def remark; 		///< 备注
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtDirectRepay();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtDirectRepay& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtDirectRepay;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 直接还款应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtDirectRepayResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Currency_def currency; 		///< 货币种类
Amt_def repay_value; 		///< 还款金额
Amt_def real_repay_value; 		///< 实际还款金额
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtDirectRepayResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtDirectRepayResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtDirectRepayResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用投票业务消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCreditVote : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
Currency_def currency; 		///< 货币种类
OrderWay_def order_way; 		///< 委托方式
Qty_def order_qty; 		///< 投票数量
SecurityID_def security_id; 		///< 投票代码
MarketID_def market_id; 		///< 市场类型
GeneralMeetingSeq_def general_meeting_seq; 		///< 股东大会编码
VotingProposal_def voting_proposal; 		///< 投票议案编号
VotingPreference_def voting_preference; 		///< 投票意向
String_def remark; 		///< 备注
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwMsgCreditVote();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCreditVote& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCreditVote;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用投票业务应答
 */
class EXPORT_MACRO_DEFINE AgwMsgCreditVoteResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgCreditVoteResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCreditVoteResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCreditVoteResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 指定合约还款请求
 */
class EXPORT_MACRO_DEFINE AgwMsgExtDirectRepayAppoint : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Bool_def is_account_partition; 		///< 账户是否按证券账户分区 1-是 0否
Currency_def currency; 		///< 货币种类
OrderWay_def order_way; 		///< 委托方式
Date8_def contract_date; 		///< 合约委托日期
Sno_def contract_sno; 		///< 合约流水号
Amt_def repay_value; 		///< 还款金额
String_def remark; 		///< 备注
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtDirectRepayAppoint();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtDirectRepayAppoint& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtDirectRepayAppoint;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 指定合约还款应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtDirectRepayAppointResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Currency_def currency; 		///< 货币种类
Amt_def repay_value; 		///< 还款金额
Amt_def real_repay_value; 		///< 实际还款金额
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtDirectRepayAppointResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtDirectRepayAppointResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtDirectRepayAppointResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用预到期合约展期申请
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCreditDebtsDelayApply : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
Date8_def sys_date; 		///< 系统日期
Sno_def sno; 		///< 合约编号
SpecKinds_def spec_kinds; 		///< 扩展字段
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCreditDebtsDelayApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCreditDebtsDelayApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCreditDebtsDelayApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约申请
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCreditReserveStkApply : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Qty_def reserve_qty; 		///< 预约数量
Qty_def min_qty; 		///< 最低数量
CreditRatio_def min_rate; 		///< 最低比例
CreditRatio_def used_fee_rate; 		///< 期望利率
CreditRatio_def unused_fee_rate; 		///< 占用费率
Date8_def begin_date; 		///< 开始日期
Date8_def end_date; 		///< 结束日期
Days_def use_days; 		///< 使用期限
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCreditReserveStkApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCreditReserveStkApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCreditReserveStkApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约申请应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCreditReserveStkApplyResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
Sno_def sno; 		///< 预约流水号
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCreditReserveStkApplyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCreditReserveStkApplyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCreditReserveStkApplyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用预到期合约展期申请应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCreditDebtsDelayApplyResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
Sno_def log_sno; 		///< 申请流水号
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCreditDebtsDelayApplyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCreditDebtsDelayApplyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCreditDebtsDelayApplyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约申请撤销
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCreditReserveStkCancel : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
Date8_def sys_date; 		///< 预约日期
Sno_def sno; 		///< 预约流水号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCreditReserveStkCancel();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCreditReserveStkCancel& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCreditReserveStkCancel;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约申请撤销应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCreditReserveStkCancelResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
Date8_def sys_date; 		///< 预约日期
Sno_def sno; 		///< 预约流水号
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCreditReserveStkCancelResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCreditReserveStkCancelResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCreditReserveStkCancelResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户自行平仓权限申请
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCustSelfCloseApply : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCustSelfCloseApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCustSelfCloseApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCustSelfCloseApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户自行平仓权限申请应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCustSelfCloseApplyResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
String_def msg_ok; 		///< 成功信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCustSelfCloseApplyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCustSelfCloseApplyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCustSelfCloseApplyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户锁券展期申请
 */
class EXPORT_MACRO_DEFINE AgwMsgExtLockDelayApply : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
Date8_def sys_date; 		///< 锁券日期
Sno_def sno; 		///< 锁券流水号
Days_def app_delay_days; 		///< 申请展期天数
CreditRatio_def app_used_fee_rate; 		///< 申请使用费率
CreditRatio_def app_unused_fee_rate; 		///< 申请占用费率
Qty_def app_delay_qty; 		///< 申请展期数量
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtLockDelayApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtLockDelayApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtLockDelayApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户锁券展期申请应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtLockDelayApplyResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtLockDelayApplyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtLockDelayApplyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtLockDelayApplyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief FPGA现货集中竞价业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCashAuctionFPGAOrder : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
AccountID_def account_id; 		///< 账户id
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 申报数量(张)/ETF份额
OrderWay_def order_way; 		///< 委托方式
TimeStamp_def transaction_time; 		///< 委托时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
PolicyID_def policy_id; 		///< 策略编号
ClOrdID_def cl_ord_id; 		///< 报盘合同号，客户报价消息编号
PWD_def password; 		///< 密码
Amt_def actual_frozen_amt; 		///< 冻结金额-不含费用
Amt_def actual_frozen_fee; 		///< 冻结费用
Char_def is_ngtp_offer; 		///< 是否低延时报盘
Char_def is_ngtp_check; 		///< 是否低延时风控
Qty_def frozen_type1; 		///< 冻结股份性质1数量
Qty_def frozen_type3; 		///< 冻结股份性质3数量
Qty_def frozen_type4; 		///< 冻结股份性质4数量
Qty_def frozen_type5; 		///< 冻结股份性质5数量
Char_def is_spec_order; 		///< 是否应急补单订单
SenderCompID_def recv_user; 		///< 接收用户
RejectReasonCode_def reject_code; 		///< 拒绝码
Char_def is_fte_process_report; 		///< 是否FTE处理回报
ClOrdNO_def cl_ord_no; 		///< 传入的客户订单编号
Uint8_def ext_order_type; 		///< 外部订单类型
Qty_def frozen_type10; 		///< 冻结股份性质10数量
OrdType_def order_type; 		///< 订单类型
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
/*
 * @brief 默认构造函数
 */
  AgwMsgCashAuctionFPGAOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCashAuctionFPGAOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCashAuctionFPGAOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief FPGA现货集中竞价业务拒绝消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCashAuctionFPGARejectAck : public mp::RootPacket
{
public:
ClOrdID_def cl_ord_id; 		///< 报盘合同号，客户报价消息编号
AccountID_def account_id; 		///< 账户id
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
SeqNum_def ref_seq_num; 		///< 被拒绝消息的消息序号(深圳用)
PBUID_def pbu_id; 		///< 申报交易单元
TimeStamp_def transact_time; 		///< 回报时间
ShExOrdRejReason_def ord_rej_reason; 		///< 拒绝原因描述信息
/*
 * @brief 默认构造函数
 */
  AgwMsgCashAuctionFPGARejectAck();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 10 + 12 + 10 + 8 + 2 + 8 + 6 + 8 + 4;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCashAuctionFPGARejectAck& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCashAuctionFPGARejectAck;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外部系统两融开仓业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgMarginTradingExtOrder : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
AccountID_def account_id; 		///< 账户id
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 申报数量(张)/ETF份额
OrderWay_def order_way; 		///< 委托方式
TimeStamp_def transaction_time; 		///< 委托时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
PolicyID_def policy_id; 		///< 策略编号
ClOrdID_def cl_ord_id; 		///< 报盘合同号，客户报价消息编号
PWD_def password; 		///< 密码
Amt_def actual_frozen_amt; 		///< 冻结金额-不含费用
Amt_def actual_frozen_fee; 		///< 冻结费用
Char_def is_ngtp_offer; 		///< 是否低延时报盘
Char_def is_ngtp_check; 		///< 是否低延时风控
SenderCompID_def recv_user; 		///< 接收用户
RejectReasonCode_def reject_code; 		///< 拒绝码
Char_def is_fte_process_report; 		///< 是否FTE处理回报
ClOrdNO_def cl_ord_no; 		///< 传入的客户订单编号
Uint8_def ext_order_type; 		///< 外部订单类型
OrdType_def order_type; 		///< 订单类型
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
Amt_def order_on_way_amt; 		///< 竞价买在途和
Amt_def rz_order_on_way_amt; 		///< 融资买在途
Amt_def margin_avl; 		///< 保证金可用
Amt_def margin_effect; 		///< 保证金发生额
CreditRatio_def keep_rate; 		///< 该笔委托的维持担保比例
/*
 * @brief 默认构造函数
 */
  AgwMsgMarginTradingExtOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMarginTradingExtOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgMarginTradingExtOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外部系统信用账户普通买入/普通卖出业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCreditAuctionExtOrder : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
AccountID_def account_id; 		///< 账户id
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 申报数量(张)/ETF份额
OrderWay_def order_way; 		///< 委托方式
TimeStamp_def transaction_time; 		///< 委托时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
PolicyID_def policy_id; 		///< 策略编号
ClOrdID_def cl_ord_id; 		///< 报盘合同号，客户报价消息编号
PWD_def password; 		///< 密码
Amt_def actual_frozen_amt; 		///< 冻结金额-不含费用
Amt_def actual_frozen_fee; 		///< 冻结费用
Char_def is_ngtp_offer; 		///< 是否低延时报盘
Char_def is_ngtp_check; 		///< 是否低延时风控
Qty_def frozen_type1; 		///< 冻结股份性质1数量
Qty_def frozen_type3; 		///< 冻结股份性质3数量
Qty_def frozen_type4; 		///< 冻结股份性质4数量
Qty_def frozen_type5; 		///< 冻结股份性质5数量
Char_def is_spec_order; 		///< 是否应急补单订单
Qty_def frozen_type10; 		///< 冻结股份性质10数量
SenderCompID_def recv_user; 		///< 接收用户
RejectReasonCode_def reject_code; 		///< 拒绝码
Char_def is_fte_process_report; 		///< 是否FTE处理回报
ClOrdNO_def cl_ord_no; 		///< 传入的客户订单编号
Uint8_def ext_order_type; 		///< 外部订单类型
OrdType_def order_type; 		///< 订单类型
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
Amt_def order_on_way_amt; 		///< 竞价买在途和
Amt_def rz_order_on_way_amt; 		///< 融资买在途
Amt_def margin_avl; 		///< 保证金可用
Amt_def margin_effect; 		///< 保证金发生额
CreditRatio_def keep_rate; 		///< 该笔委托的维持担保比例
/*
 * @brief 默认构造函数
 */
  AgwMsgCreditAuctionExtOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCreditAuctionExtOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCreditAuctionExtOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief UTE的成分股冻结记录
 */
class EXPORT_MACRO_DEFINE AgwUTEComponentShareFrozenInfo : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 成分股证券代码
MarketID_def market_id; 		///< 市场代码
Qty_def frozen_type1; 		///< 冻结股份性质1数量
Qty_def frozen_type3; 		///< 冻结股份性质3数量
Qty_def frozen_type4; 		///< 冻结股份性质4数量
Qty_def frozen_type10; 		///< 冻结股份性质10数量
Amt_def frozen_fund; 		///< 冻结资金
/*
 * @brief 默认构造函数
 */
  AgwUTEComponentShareFrozenInfo();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwUTEComponentShareFrozenInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 外部系统ETF申赎业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgETFRedemptionExtOrder : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
AccountID_def account_id; 		///< 账户id
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 申报数量(张)/ETF份额
OrderWay_def order_way; 		///< 委托方式
TimeStamp_def transaction_time; 		///< 委托时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
PolicyID_def policy_id; 		///< 策略编号
ClOrdID_def cl_ord_id; 		///< 报盘合同号，客户报价消息编号
PWD_def password; 		///< 密码
Amt_def actual_frozen_fee; 		///< 冻结费用
Char_def is_ngtp_offer; 		///< 是否低延时报盘
Char_def is_ngtp_check; 		///< 是否低延时风控
SenderCompID_def recv_user; 		///< 接收用户
RejectReasonCode_def reject_code; 		///< 拒绝码
Char_def is_fte_process_report; 		///< 是否FTE处理回报
ClOrdNO_def cl_ord_no; 		///< 传入的客户订单编号
Uint8_def ext_order_type; 		///< 外部订单类型
OrdType_def order_type; 		///< 订单类型
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
std::vector<AgwUTEComponentShareFrozenInfo> ute_component_share_frozen_info; 		///< 来自UTE的成分股冻结信息
Amt_def ute_frozen_fund; 		///< 现金替代总金额
Amt_def ute_cash_diff; 		///< 预估现金差额
/*
 * @brief 默认构造函数
 */
  AgwMsgETFRedemptionExtOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFRedemptionExtOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFRedemptionExtOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 通用撤单同步消息
 */
class EXPORT_MACRO_DEFINE AgwMsgCancelOrderSync : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
TimeStamp_def client_order_time; 		///< 委托时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
AccountID_def account_id; 		///< 账户id
Side_def side; 		///< 买卖方向
OrdType_def order_type; 		///< 订单类型
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 申报数量(张)/ETF份额
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
PWD_def password; 		///< 密码
BranchID_def branch_id; 		///< 营业部ID
PolicyID_def policy_id; 		///< 策略编号
ClOrdID_def cl_ord_id; 		///< 报盘合同号，客户报价消息编号
ClOrdNO_def cl_ord_no; 		///< 传入的客户订单编号
ClOrdID_def orig_cl_ord_id; 		///< 原委托的报盘合同号
ClOrdNO_def orig_cl_ord_no; 		///< 原委托的客户订单编号
SeqNum_def orig_client_seq_id; 		///< 原客户端消息编号
Uint8_def ext_order_type; 		///< 外部订单类型
SenderCompID_def recv_user; 		///< 接收用户
/*
 * @brief 默认构造函数
 */
  AgwMsgCancelOrderSync();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCancelOrderSync& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgCancelOrderSync;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询公共报文
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryBase : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号ID
FundAccountID_def fund_account_id; 		///< 资金账户ID
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
PWD_def password; 		///< 客户密码
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryBase();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryBase& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 查询结果公共报文
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultBase : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号ID
FundAccountID_def fund_account_id; 		///< 资金账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
QueryResultCode_def query_result_code; 		///< 查询结果代码
UserInfo_def user_info; 		///< 用户私有信息
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultBase();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultBase& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 最大可委托数查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryMaxOrderQty : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
AccountID_def sh_account_id; 		///< 沪A股东代码
AccountID_def sz_account_id; 		///< 深A股东代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryMaxOrderQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryMaxOrderQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryMaxOrderQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗最大可委托数查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgNewExtQueryMaxOrderQty : public AgwMsgExtQueryMaxOrderQty
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgNewExtQueryMaxOrderQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgNewExtQueryMaxOrderQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgNewExtQueryMaxOrderQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 最大可委托数查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryMaxOrderQtyResult : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 账户ID
Qty_def max_order_qty; 		///< 最大可委托数
Qty_def theoretical_order_qty; 		///< 理论可委托数
Qty_def odd_order_qty; 		///< 港股通零股卖出最大可委托数
Bool_def limit_flag; 		///< 减持受限标识
Qty_def jj_max_limit_qty; 		///< 减持受限名单内的竞价卖出可用额度
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryMaxOrderQtyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryMaxOrderQtyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryMaxOrderQtyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗最大可委托数查询结果消息基类
 */
class EXPORT_MACRO_DEFINE AgwMsgNewExtQueryMaxOrderQtyResultBase : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 账户ID
Qty_def max_order_qty; 		///< 最大可委托数
Qty_def theoretical_order_qty; 		///< 理论可委托数
Qty_def odd_order_qty; 		///< 港股通零股卖出最大可委托数
/*
 * @brief 默认构造函数
 */
  AgwMsgNewExtQueryMaxOrderQtyResultBase();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgNewExtQueryMaxOrderQtyResultBase& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 大宗最大可委托数查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgNewExtQueryMaxOrderQtyResult : public AgwMsgNewExtQueryMaxOrderQtyResultBase
{
public:
AccountID_def account_id; 		///< 账户ID
Qty_def max_order_qty; 		///< 最大可委托数
Qty_def theoretical_order_qty; 		///< 理论可委托数
Bool_def limit_flag; 		///< 减持受限标识
Qty_def jj_max_limit_qty; 		///< 减持受限名单内的竞价卖出可用额度
Qty_def dz_max_limit_qty; 		///< 减持受限名单内的大宗受限股份可用额度
Qty_def dz_max_unlimit_qty; 		///< 减持受限名单内的大宗非受限股份可用额度
/*
 * @brief 默认构造函数
 */
  AgwMsgNewExtQueryMaxOrderQtyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgNewExtQueryMaxOrderQtyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgNewExtQueryMaxOrderQtyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 成交查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTradeOrder : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,填0查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
Bool_def flag; 		///< 是否剔除北交所和老三板
Side_def side; 		///< 买卖方向,填0查所有
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 成交查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTradeOrderArray : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 证券账户
Side_def side; 		///< 买卖方向,填0查所有
OrdType_def ord_type; 		///< 订单类别
ExecType_def exec_type; 		///< 当前订单执行类型
ExecID_def exec_id; 		///< 执行编号
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
OrderID_def order_id; 		///< 交易所订单编号, 取值为数字
ClOrdID_def cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
TimeStamp_def transact_time; 		///< 交易所回报时间,格式为YYYY:MM:DD:HH:MM:SS.000
Price_def last_px; 		///< 成交价格
Qty_def last_qty; 		///< 成交数量
Amt_def total_value_traded; 		///< 成交金额,精确到分
Amt_def fee; 		///< 费用
Currency_def currency; 		///< 货币种类
Price_def order_price; 		///< 委托价格
Qty_def order_qty; 		///< 委托数量
BSFlag2_def bs_flag2; 		///< 集中交易买卖类别
UserInfo_def user_info; 		///< 用户私有信息
CreditDigest_def credit_digest_id; 		///< 信用交易摘要
MatchType_def match_type; 		///< 成交明细类型
ClOrdNO_def orig_cl_ord_no; 		///< 原始交易客户方订单编号,对于撤单成功响应,为原始交易客户方(券商)订单编号,指示被撤消订单的ClOrdNO; 对于订单确认,取值为0
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
ReportIndex_def report_index; 		///< 回报序号
Amt_def clear_amt; 		///< 清算金额
CancelOrderFlag_def cancel_order_flag; 		///< 撤单标志（表示该笔委托是否是撤单委托）
/*
 * @brief 默认构造函数
 */
  AgwTradeOrderArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 8 + 40 + 2 + 12 + 1 + 1 + 1 + 32 + 8 + 16 + 10 + 8 + 8 + 8 + 8 + 8 + 4 + 8 + 8 + 2 + 64 + 1 + 1 + 8 + 64 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTradeOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 成交查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultTradeOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 成交记录总数
std::vector<AgwTradeOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资金查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryFund : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资金查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultFund : public AgwMsgExtQueryResultBase
{
public:
Amt_def leaves_value; 		///< 资金余额
Amt_def init_leaves_value; 		///< 日初资金金额
Amt_def available_t0; 		///< T+0可用资金
Amt_def available_t1; 		///< T+1可用资金
Amt_def available_t2; 		///< T+2可用资金
Amt_def available_t3; 		///< T+3可用资金
Amt_def available_tall; 		///< T+All可用资金
Amt_def frozen_all; 		///< 当前所有冻结
Amt_def total_assert; 		///< 总资产
Amt_def share_total_assert; 		///< 证券总资产
Amt_def share_total_profit_loss; 		///< 证券总盈亏
Amt_def share_today_profit_loss; 		///< 证券当日盈亏
AgwPartition_def te_partition_no; 		///< 所属分区号
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 证券账户组
 */
class EXPORT_MACRO_DEFINE AgwAccountIDArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户
/*
 * @brief 默认构造函数
 */
  AgwAccountIDArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwAccountIDArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 股份持仓查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryShare : public AgwMsgExtQueryBase
{
public:
std::vector<AgwAccountIDArray> account_id_array; 		///< 证券账户组
BusinessType_def business_type; 		///< 业务类型,必填
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股份查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwShareArray : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 证券账户
Qty_def init_qty; 		///< 日初持仓量
Qty_def leaves_qty; 		///< 剩余股份数量（包含当日买入部分）
Qty_def available_qty; 		///< 可用股份数量
Amt_def profit_loss; 		///< 浮动盈亏
Amt_def market_value; 		///< 市值
Amt_def cost_price; 		///< 成本价
Currency_def currency; 		///< 货币类型
Amt_def income; 		///< 累计盈亏
Amt_def buy_cost; 		///< 当前成本（金额）
Bool_def mkt_cal_flag; 		///< 市值计算标识
Price_def last_price; 		///< 最新价格
SecurityType_def stk_type; 		///< 证券类型
Qty_def stk_diff; 		///< 可申赎数量
PBUID_def seat; 		///< 席位代码
Price_def profit_cost; 		///< 盈亏成本
/*
 * @brief 默认构造函数
 */
  AgwShareArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 40 + 2 + 12 + 8 + 8 + 8 + 8 + 8 + 8 + 4 + 8 + 8 + 1 + 8 + 2 + 8 + 6 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwShareArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 股份查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultShare : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 合约持仓记录总数
std::vector<AgwShareArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新股发行申购额度查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryIPOQty : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryIPOQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryIPOQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryIPOQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新股发行申购额度查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwIPOQtyArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
Qty_def subscription_quota; 		///< 申购额度
/*
 * @brief 默认构造函数
 */
  AgwIPOQtyArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwIPOQtyArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 新股发行申购额度查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultIPOQty : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwIPOQtyArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultIPOQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultIPOQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultIPOQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 科创板新股发行申购额度查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTIBIPOQty : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTIBIPOQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTIBIPOQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTIBIPOQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 科创板新股发行申购额度查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultTIBIPOQty : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwIPOQtyArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultTIBIPOQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultTIBIPOQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultTIBIPOQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 成交汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTradeCollect : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券ID
BusinessType_def business_type; 		///< 业务类型,必填。填0查所有
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTradeCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTradeCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTradeCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 成交汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTradeCollectArray : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务代码
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 证券账户
Price_def buy_avg_price; 		///< 买入均价
Qty_def buy_cum_qty; 		///< 买入累计总量
Amt_def buy_total_value_traded; 		///< 买入成交总金额，精确到3位小数
Price_def sell_avg_price; 		///< 卖出均价
Qty_def sell_cum_qty; 		///< 卖出累计总量
Amt_def sell_total_value_traded; 		///< 卖出成交金额，精确到3位小数
Amt_def total_value_traded; 		///< 买卖合计成交金额，精确到3位小数
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwTradeCollectArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 8 + 40 + 2 + 12 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 4;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTradeCollectArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 成交汇总查询结果
 */
class EXPORT_MACRO_DEFINE AgwExtQueryResultTradeCollect : public AgwMsgExtQueryResultBase
{
public:
Price_def all_buy_avg_price; 		///< 总买入均价
Qty_def all_buy_cum_qty; 		///< 总买入累计总量
Amt_def all_buy_total_value_traded; 		///< 总买入成交总金额，精确到3位小数
Price_def all_sell_avg_price; 		///< 总卖出均价
Qty_def all_sell_cum_qty; 		///< 总卖出累计总量
Amt_def all_sell_total_value_traded; 		///< 总卖出成交金额，精确到3位小数
Amt_def all_total_value_traded; 		///< 总买卖合计成交金额，精确到3位小数
std::vector<AgwTradeCollectArray> trade_collect_array; 		///< 成交汇总查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwExtQueryResultTradeCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExtQueryResultTradeCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExtQueryResultTradeCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 订单查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryOrder : public AgwMsgExtQueryBase
{
public:
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,同一个客户当天唯一,填0查所有
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
Side_def side; 		///< 买卖方向,填0查所有
OrderQueryCondition_def order_query_condition; 		///< 委托查询条件
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
Bool_def flag; 		///< 是否剔除北交所和老三板
CancelOrderFlag_def cancel_order_flag; 		///< 撤单标志（表示该笔委托是否是撤单委托）
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 委托查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwOrderArray : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 证券账户
Side_def side; 		///< 买卖方向
OrdType_def ord_type; 		///< 订单类别
OrdStatus_def ord_status; 		///< 原订单当前状态
TimeStamp_def transact_time; 		///< 客户委托时间
Price_def order_price; 		///< 委托价格
Price_def exec_price; 		///< 平均成交价格
Qty_def order_qty; 		///< 委托数量
Qty_def leaves_qty; 		///< 未成交部分的数量
Qty_def cum_qty; 		///< 累计成交数量
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
OrderID_def order_id; 		///< 交易所订单编号, 取值为数字
ClOrdID_def cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClOrdNO_def orig_cl_ord_no; 		///< 原委托的客户订单编号
Amt_def frozen_trade_value; 		///< 冻结交易金额
Amt_def frozen_fee; 		///< 冻结费用
RejectReasonCode_def reject_reason_code; 		///< 拒绝代码
ExErrMsg_def ord_rej_reason; 		///< 交易所错误信息描述
Currency_def money_type; 		///< 货币种类
CancelFlag_def cancel_flag; 		///< 撤单标志
Qty_def cancel_qty; 		///< 撤单数量
BSFlag2_def bs_flag2; 		///< 集中交易买卖类别
PBUID_def pbu_id; 		///< 投资者所属交易单元,交易席位
BranchID_def branch_id; 		///< 营业部代码
Amt_def total_value_traded; 		///< 成交金额,精确到分
UserInfo_def user_info; 		///< 用户私有信息
CreditDigest_def credit_digest_id; 		///< 信用交易摘要
CreditDigestName_def bs_flag_name; 		///< 信用交易摘要名称
CancelOrderFlag_def cancel_order_flag; 		///< 撤单标志（表示该笔委托是否是撤单委托）
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
ExchangeRate_def refer_rate; 		///< 参考汇率
SellRepaySource_def sell_repay_source; 		///< 卖券还款来源
Bool_def is_cancelable; 		///< 标记该笔订单是否可撤
AccountID_def link_account_id; 		///< 关联股东账户
PBUID_def link_pbu_id; 		///< 关联交易席位
/*
 * @brief 默认构造函数
 */
  AgwOrderArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 8 + 40 + 2 + 12 + 1 + 1 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 16 + 10 + 8 + 8 + 8 + 8 + 2 + 128 + 4 + 1 + 8 + 2 + 6 + 10 + 8 + 64 + 1 + 40 + 1 + 64 + 8 + 1 + 1 + 12 + 6;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 订单查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按证券账户划转资金请求消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransFundReq : public mp::RootPacket
{
public:
TimeStamp_def client_time; 		///< 客户发起时间
CustID_def cust_id; 		///< 客户号
PWD_def password; 		///< 客户密码
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def fund_out_account_id; 		///< 资金转出账户ID
MarketID_def fund_out_market_id; 		///< 资金转出市场ID
AccountID_def fund_in_account_id; 		///< 资金转入账户ID
MarketID_def fund_in_market_id; 		///< 资金转入市场ID
Amt_def value; 		///< 划拨金额
Currency_def currency; 		///< 货币种类
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransFundReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransFundReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransFundReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按证券账户划转资金响应消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransFundResp : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
ExternalInsErrorCode_def external_ins_error_code; 		///< 外部指令错误代码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransFundResp();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 16 + 10 + 2 + 8 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransFundResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransFundResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户按证券账户划转资金请求消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditFundReq : public mp::RootPacket
{
public:
TimeStamp_def client_time; 		///< 客户发起时间
CustID_def cust_id; 		///< 客户号
PWD_def password; 		///< 客户密码
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def fund_out_account_id; 		///< 资金转出账户ID
MarketID_def fund_out_market_id; 		///< 资金转出市场ID
AccountID_def fund_in_account_id; 		///< 资金转入账户ID
MarketID_def fund_in_market_id; 		///< 资金转入市场ID
Amt_def avl_value; 		///< 自有资金可用划拨金额
Amt_def credit_avl_value; 		///< 融券卖出资金可用划拨金额
Currency_def currency; 		///< 货币种类
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditFundReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditFundReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditFundReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户按证券账户划转资金响应消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditFundResp : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
ExternalInsErrorCode_def external_ins_error_code; 		///< 外部指令错误代码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
SeqNum_def agw_seq_id; 		///< 业务网关序号，交易系统全局唯一
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditFundResp();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 16 + 10 + 2 + 8 + 64 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditFundResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditFundResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
/*
 * @brief agw_seq_id接口
 */
    virtual SeqNum_def GetAgwSeqID() const { return agw_seq_id; }
};
/*
 * @brief 证券代码组
 */
class EXPORT_MACRO_DEFINE AgwSecurityIDArray : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Qty_def value; 		///< 划转数量
/*
 * @brief 默认构造函数
 */
  AgwSecurityIDArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSecurityIDArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用账户按证券账户划转股份请求消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditShareReq : public mp::RootPacket
{
public:
TimeStamp_def client_time; 		///< 客户发起时间
CustID_def cust_id; 		///< 客户号
PWD_def password; 		///< 客户密码
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def share_out_account_id; 		///< 资金转出账户ID
MarketID_def share_out_market_id; 		///< 资金转出市场ID
AccountID_def share_in_account_id; 		///< 资金转入账户ID
MarketID_def share_in_market_id; 		///< 资金转入市场ID
ShareTransDirect_def trans_direct; 		///< 划转方向
Currency_def currency; 		///< 货币种类
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
std::vector<AgwSecurityIDArray> security_id_array; 		///< 股份划拨证券代码组
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditShareReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditShareReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditShareReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户按证券账户划转股份响应消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditShareResp : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
ExternalInsErrorCode_def external_ins_error_code; 		///< 外部指令错误代码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
SeqNum_def agw_seq_id; 		///< 业务网关序号，交易系统全局唯一
SeqNum_def fail_num; 		///< 划转失败笔数
SeqNum_def success_num; 		///< 划转成功笔数
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditShareResp();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 16 + 10 + 2 + 8 + 64 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditShareResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditShareResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
/*
 * @brief agw_seq_id接口
 */
    virtual SeqNum_def GetAgwSeqID() const { return agw_seq_id; }
};
/*
 * @brief 信用账户按证券账户划转保证金余额请求消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditMarginAvlReq : public mp::RootPacket
{
public:
TimeStamp_def client_time; 		///< 客户发起时间
CustID_def cust_id; 		///< 客户号
PWD_def password; 		///< 客户密码
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def fund_out_account_id; 		///< 资金转出账户ID
MarketID_def fund_out_market_id; 		///< 资金转出市场ID
AccountID_def fund_in_account_id; 		///< 资金转入账户ID
MarketID_def fund_in_market_id; 		///< 资金转入市场ID
Amt_def margin_avl_value; 		///< 保证金余额划拨金额
Currency_def currency; 		///< 货币种类
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditMarginAvlReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditMarginAvlReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditMarginAvlReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户按证券账户划转保证金余额响应消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditMarginAvlResp : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
ExternalInsErrorCode_def external_ins_error_code; 		///< 外部指令错误代码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
SeqNum_def agw_seq_id; 		///< 业务网关序号，交易系统全局唯一
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditMarginAvlResp();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 16 + 10 + 2 + 8 + 64 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditMarginAvlResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditMarginAvlResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
/*
 * @brief agw_seq_id接口
 */
    virtual SeqNum_def GetAgwSeqID() const { return agw_seq_id; }
};
/*
 * @brief 信用账户按证券账户划转可用授信额度请求消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditLimitReq : public mp::RootPacket
{
public:
TimeStamp_def client_time; 		///< 客户发起时间
CustID_def cust_id; 		///< 客户号
PWD_def password; 		///< 客户密码
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def fund_out_account_id; 		///< 资金转出账户ID
MarketID_def fund_out_market_id; 		///< 资金转出市场ID
AccountID_def fund_in_account_id; 		///< 资金转入账户ID
MarketID_def fund_in_market_id; 		///< 资金转入市场ID
Amt_def limit_value; 		///< 可用授信额度划拨金额
Amt_def fund_limit_value; 		///< 融资可用授信额度划拨金额
Amt_def stk_limit_value; 		///< 融券可用授信额度划拨金额
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditLimitReq();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditLimitReq& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditLimitReq;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户按证券账户划转可用授信额度响应消息
 */
class EXPORT_MACRO_DEFINE AgwExternalInsTETransCreditLimitResp : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
ExternalInsErrorCode_def external_ins_error_code; 		///< 外部指令错误代码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
SeqNum_def agw_seq_id; 		///< 业务网关序号，交易系统全局唯一
/*
 * @brief 默认构造函数
 */
  AgwExternalInsTETransCreditLimitResp();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 16 + 10 + 2 + 8 + 64 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExternalInsTETransCreditLimitResp& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExternalInsTETransCreditLimitResp;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
/*
 * @brief agw_seq_id接口
 */
    virtual SeqNum_def GetAgwSeqID() const { return agw_seq_id; }
};
/*
 * @brief 增强资金查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryFundEx : public AgwMsgExtQueryFund
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryFundEx();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryFundEx& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryFundEx;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资金查询结果
 */
class EXPORT_MACRO_DEFINE AgwExtQueryFundArray : public mp::RootPacket
{
public:
Amt_def leaves_value; 		///< 资金余额
Amt_def init_leaves_value; 		///< 日初资金金额
Amt_def available_t0; 		///< T+0可用资金
Amt_def available_t1; 		///< T+1可用资金
Amt_def available_t2; 		///< T+2可用资金
Amt_def available_t3; 		///< T+3可用资金
Amt_def available_tall; 		///< T+All可用资金
Amt_def frozen_all; 		///< 当前所有冻结
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwExtQueryFundArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 4;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExtQueryFundArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 增强资金查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultFundEx : public AgwMsgExtQueryResultBase
{
public:
AgwPartition_def te_partition_no; 		///< 所属分区号
std::vector<AgwExtQueryFundArray> agw_ext_query_fund_array; 		///< 增强资金查询结果集合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultFundEx();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultFundEx& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultFundEx;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗盘后定价委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgAfterHoursPricingOrder : public AgwNewMsgOrder
{
public:
PriceProperty_def price_property; 		///< 价格类型
CashMargin_def cash_margin; 		///< 信用标识
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgAfterHoursPricingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgAfterHoursPricingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgAfterHoursPricingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗意向申报委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradeIntentionOrder : public AgwNewMsgOrder
{
public:
TrdSubType_def trd_sub_type; 		///< 是否受限
Contactor_def contactor; 		///< 联系人
ContactInfo_def contact_info; 		///< 联系人信息
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradeIntentionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradeIntentionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradeIntentionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗成交申报委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradeTransactionOrder : public AgwNewMsgOrder
{
public:
ConfirmID_def confirm_id; 		///< 约定号
TrdSubType_def trd_sub_type; 		///< 是否受限
PBUID_def counter_party_pbu_id; 		///< 对手方PBUID
CashMargin_def cash_margin; 		///< 信用标识
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradeTransactionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradeTransactionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradeTransactionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗定价申报委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradePricingOrder : public AgwNewMsgOrder
{
public:
Pricing_def pricing; 		///< 是否协议交易定价
ConfirmID_def confirm_id; 		///< 约定号
CashMargin_def cash_margin; 		///< 信用标识
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradePricingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradePricingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradePricingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗交易撤单消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradeCancelOrder : public AgwMsgOrder
{
public:
ClOrdNO_def orig_cli_ord_no; 		///< 原委托的客户订单编号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradeCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradeCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradeCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗盘后定价业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgAfterHoursPricingTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
PriceProperty_def price_property; 		///< 价格类型
CashMargin_def cash_margin; 		///< 信用标识
/*
 * @brief 默认构造函数
 */
  AgwMsgAfterHoursPricingTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgAfterHoursPricingTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgAfterHoursPricingTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗成交申报委托业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradeTransactionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
PBUID_def counter_party_pbu_id; 		///< 对手方PBUID
AccountID_def counter_party_account_id; 		///< 对手方股东账户
TrdSubType_def trd_sub_type; 		///< 是否受限
ConfirmID_def confirm_id; 		///< 约定号
CashMargin_def cash_margin; 		///< 信用标识
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradeTransactionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradeTransactionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradeTransactionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗定价申报委托业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradePricingTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
Pricing_def pricing; 		///< 是否协议交易定价
ConfirmID_def confirm_id; 		///< 约定号
CashMargin_def cash_margin; 		///< 信用标识
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradePricingTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradePricingTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradePricingTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 高管额度划转业务委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgReduceHoldingQuoteTransferOrder : public AgwNewMsgOrder
{
public:
AccountID_def counter_party_account_id; 		///< 对手方PBUID
PBUID_def counter_party_pbu_id; 		///< 对手方PBUID
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgReduceHoldingQuoteTransferOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgReduceHoldingQuoteTransferOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgReduceHoldingQuoteTransferOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 高管额度划转撤单消息
 */
class EXPORT_MACRO_DEFINE AgwMsgReduceHoldingQuoteTransferCancelOrder : public AgwMsgOrder
{
public:
ClOrdNO_def orig_cli_ord_no; 		///< 原委托的客户订单编号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgReduceHoldingQuoteTransferCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgReduceHoldingQuoteTransferCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgReduceHoldingQuoteTransferCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转协议转让定价申报委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdPricingOrder : public AgwNewMsgOrder
{
public:
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdPricingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdPricingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdPricingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转协议转让定价申报成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdPricingTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdPricingTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdPricingTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdPricingTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转协议转让成交确认申报委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdTransactionOrder : public AgwNewMsgOrder
{
public:
PBUID_def target_pbu_id; 		///< 对手方PBUID
AccountID_def target_account_id; 		///< 对手方股东账户
ConfirmID_def promise_sno; 		///< 约定号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdTransactionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdTransactionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdTransactionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转协议转让成交确认申报成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdTransactionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
PBUID_def target_pbu_id; 		///< 对手方PBUID
AccountID_def target_account_id; 		///< 对手方股东账户
ConfirmID_def promise_sno; 		///< 约定号
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdTransactionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdTransactionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdTransactionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板买卖委托
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdCashAuctionOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdCashAuctionOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdCashAuctionOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdCashAuctionOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板买卖成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdCashAuctionTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdCashAuctionTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdCashAuctionTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdCashAuctionTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板买卖委托(两网退市股票)
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdCashAuctionDelistOrder : public AgwNewMsgOrder
{
public:
Price_def stop_px; 		///< 止损价
Qty_def min_qty; 		///< 最低成交数量
MaxPriceLevels_def max_price_levels; 		///< 最多成交价位数
TimeInForce_def time_in_force; 		///< 订单有效期
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdCashAuctionDelistOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdCashAuctionDelistOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdCashAuctionDelistOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板买卖成交回报(两网退市股票)
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdCashAuctionDelistTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdCashAuctionDelistTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdCashAuctionDelistTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdCashAuctionDelistTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转大宗交易成交确认申报委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdBlockTradeOrder : public AgwNewMsgOrder
{
public:
PBUID_def target_pbu_id; 		///< 对手方PBUID
AccountID_def target_account_id; 		///< 对手方股东账户
ConfirmID_def promise_sno; 		///< 约定号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdBlockTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdBlockTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdBlockTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转大宗交易成交确认申报成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdBlockTradeTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
PBUID_def target_pbu_id; 		///< 对手方PBUID
AccountID_def target_account_id; 		///< 对手方股东账户
ConfirmID_def promise_sno; 		///< 约定号
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdBlockTradeTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdBlockTradeTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdBlockTradeTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转要约收购业务消息
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdTenderOfferOrder : public AgwNewMsgOrder
{
public:
ShareNature_def share_nature; 		///< 股份性质
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdTenderOfferOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdTenderOfferOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdTenderOfferOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转摘牌协议转让成交确认申报委托消息
 */
class EXPORT_MACRO_DEFINE AgwMsgThirdDelistAgreementOrder : public AgwNewMsgOrder
{
public:
PBUID_def target_pbu_id; 		///< 对手方PBUID
AccountID_def target_account_id; 		///< 对手方股东账户
ConfirmID_def promise_sno; 		///< 约定号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgThirdDelistAgreementOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgThirdDelistAgreementOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgThirdDelistAgreementOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 证券信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwSecurityInfoArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
Currency_def currency; 		///< 货币类型
SecuritySymbol_def security_symbol; 		///< 证券名称
SecurityID_def security_id; 		///< 证券代码
SecurityType_def security_type; 		///< 证券类别
PriceTick_def price_tick; 		///< 证券价位
Price_def limit_up; 		///< 涨停价格
Price_def limit_down; 		///< 跌停价格
SecurityStatus_def security_status; 		///< 证券状态
BondInterest_def bond_interest; 		///< 债券应计利息
Qty_def max_qty; 		///< 最大数量
Qty_def min_qty; 		///< 最小数量
Qty_def buy_tick; 		///< 买入最小单位
Qty_def sale_tick; 		///< 卖出最小单位
Bool_def has_price_limit; 		///< 是否有涨幅控制
Bool_def is_no_profit; 		///< 是否尚未盈利
Bool_def is_weighted_voting_rights; 		///< 是否存在投票权差异
Bool_def is_VIE; 		///< 是否具有协议控制架构
Bool_def is_registration; 		///< 是否注册制
Char_def spec_busi_flag; 		///< 特殊业务标识
RiseFallWidth_def limit_up_rate; 		///< 最大涨幅
RiseFallWidth_def limit_down_rate; 		///< 最大跌幅
Date8_def tz_date; 		///< 退市整理期股票首次交易日期
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
Char_def after_hours_pricing_flag; 		///< 盘后定价交易标志
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
Int64_def ext_int1; 		///< 是否为创业板或科创板可转债
Int64_def ext_int2; 		///< 扩展字段
Int64_def ext_int3; 		///< 扩展字段
Int64_def ext_int4; 		///< 扩展字段
Amt_def ext_amt1; 		///< 扩展字段
Amt_def ext_amt2; 		///< 扩展字段
Char_def ext_kind1; 		///< 扩展字段
Char_def ext_kind2; 		///< 扩展字段
Price_def ext_price1; 		///< 扩展字段
Price_def ext_price2; 		///< 扩展字段
CreditRatio_def ext_rate1; 		///< 扩展字段
CreditRatio_def ext_rate2; 		///< 扩展字段
Qty_def ext_qty1; 		///< 扩展字段
Qty_def ext_qty2; 		///< 扩展字段
Comments_def ext_char64; 		///< 扩展字段
Char_def trade_id; 		///< 可交易业务类型标识
Price_def fixprice; 		///< 昨收盘价
Price_def nqdzjymaxprice; 		///< 股转大宗交易上限价
Price_def nqdzjyminprice; 		///< 股转大宗交易下限价
StkLevel_def stk_level; 		///< 证券品种
Bool_def day_trading; 		///< 是否回转交易
BondClass_def bond_class; 		///< 债券分类
Bool_def is_physical_redemption; 		///< 是否全实物申赎
BondStatus_def bond_status; 		///< 债券状态
/*
 * @brief 默认构造函数
 */
  AgwSecurityInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 4 + 40 + 8 + 2 + 8 + 8 + 8 + 1 + 8 + 8 + 8 + 8 + 8 + 1 + 1 + 1 + 1 + 1 + 1 + 4 + 4 + 8 + 8 + 8 + 1 + 64 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 64 + 1 + 8 + 8 + 8 + 1 + 1 + 1 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSecurityInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 证券信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySecurityInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 证券信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSecurityInfo : public AgwMsgExtQueryResultBase
{
public:
TimeStamp_def server_time; 		///< 服务器时间
std::vector<AgwSecurityInfoArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 行情信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryMd : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryMd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryMd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryMd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 配股信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryRightsIssueInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryRightsIssueInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryRightsIssueInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryRightsIssueInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 配股信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultRightsIssueInfo : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 账户ID
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
SecurityType_def security_type; 		///< 证券类别
SecurityID_def underlying_security_id; 		///< 正股代码
Price_def issue_price; 		///< 配售价格
Qty_def purchase_unit; 		///< 申购单位
Qty_def max_purchase_qty; 		///< 单笔最大申购数量
Bool_def is_revoked_order; 		///< 是否允许撤单
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultRightsIssueInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultRightsIssueInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultRightsIssueInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 行情信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultMd : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 账户ID
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
SecurityType_def security_type; 		///< 证券类别
Currency_def currency; 		///< 货币类型
Price_def last_price; 		///< 最近价格
Price_def limit_up; 		///< 涨停价格
Price_def limit_down; 		///< 跌停价格
BondInterest_def bond_interest; 		///< 债券应计利息
Price_def newest_price; 		///< 最新价格
Price_def open_price; 		///< 开盘价格
Price_def close_price; 		///< 收盘价格
Price_def high_price; 		///< 最高价格
Price_def low_price; 		///< 最低价格
Price_def buy_price1; 		///< 买一价
Price_def buy_price2; 		///< 买二价
Price_def buy_price3; 		///< 买三价
Price_def buy_price4; 		///< 买四价
Price_def buy_price5; 		///< 买五价
Price_def sale_price1; 		///< 卖一价
Price_def sale_price2; 		///< 卖盘二
Price_def sale_price3; 		///< 卖盘三
Price_def sale_price4; 		///< 卖盘四
Price_def sale_price5; 		///< 卖盘五
Qty_def buy_qty1; 		///< 买盘一
Qty_def buy_qty2; 		///< 买盘二
Qty_def buy_qty3; 		///< 买盘三
Qty_def buy_qty4; 		///< 买盘四
Qty_def buy_qty5; 		///< 买盘五
Qty_def sale_qty1; 		///< 卖盘一
Qty_def sale_qty2; 		///< 卖盘二
Qty_def sale_qty3; 		///< 卖盘三
Qty_def sale_qty4; 		///< 卖盘四
Qty_def sale_qty5; 		///< 卖盘五
Qty_def total_volume_trade; 		///< 成交总量
Amt_def total_value_trade; 		///< 成交总金额
PriceTick_def price_tick; 		///< 价格档位
Qty_def buy_qty_unit; 		///< 买数量单位
Qty_def sell_qty_unit; 		///< 卖数量单位
Qty_def market_buy_qty_unit; 		///< 市价买数量单位
Qty_def market_sell_qty_unit; 		///< 市价卖数量单位
TimeStamp_def server_time; 		///< 服务器时间
Bool_def is_suspended; 		///< 涨跌停标识
Price_def ah_last_price; 		///< 盘后定价最近价格
Price_def ah_buy_price; 		///< 盘后定价买入价格
Price_def ah_sale_price; 		///< 盘后定价卖出价格
Qty_def ah_buy_qty; 		///< 盘后定价买入数量
Qty_def ah_sale_qty; 		///< 盘后定价卖出数量
Qty_def ah_total_volume_trade; 		///< 盘后定价成交总量
Amt_def ah_total_value_trade; 		///< 盘后定价成交总金额
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
Int64_def ext_int1; 		///< 扩展字段
Int64_def ext_int2; 		///< 扩展字段
Int64_def ext_int3; 		///< 扩展字段
Int64_def ext_int4; 		///< 扩展字段
Amt_def ext_amt1; 		///< 扩展字段
Amt_def ext_amt2; 		///< 扩展字段
Char_def ext_kind1; 		///< 扩展字段
Char_def ext_kind2; 		///< 扩展字段
Price_def ext_price1; 		///< 扩展字段
Price_def ext_price2; 		///< 扩展字段
CreditRatio_def ext_rate1; 		///< 扩展字段
CreditRatio_def ext_rate2; 		///< 扩展字段
Qty_def ext_qty1; 		///< 扩展字段
Qty_def ext_qty2; 		///< 扩展字段
Comments_def ext_char64; 		///< 扩展字段
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultMd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultMd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultMd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新股信息查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryIPOInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
IPOType_def ipo_type; 		///< 配售类型
Date8_def start_date; 		///< 配售时间
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryIPOInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryIPOInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryIPOInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券质押库查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryBondPledge : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryBondPledge();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryBondPledge& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryBondPledge;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券质押库查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwBondPledgeArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 账户ID
SecurityID_def security_id; 		///< 证券代码
Qty_def bond_last_bal; 		///< 上次余额
Qty_def bond_bal; 		///< 质押库余额
Qty_def bond_avl; 		///< 质押库可用
Qty_def bond_bal_n; 		///< 质押库余额折算后市值
Qty_def bond_avl_n; 		///< 质押库可用折算后市值
Qty_def bond_secu; 		///< 二级质押标准券
FeeRate_def contract_multiplier; 		///< 一级标准债券折合比例
FeeRate_def credit_bond_concentration; 		///< 信用债券入库集中度
IssuerNo_def issuer_no; 		///< 发行人编号
String_def issuer_full_name; 		///< 发行人全称
IssuerOutsideRating_def issuer_outside_rating; 		///< 发行人外部评级
FeeRate_def issuer_ratio; 		///< 单一发行人信用债入库担保集中度
FeeRate_def stk_rate; 		///< 标准券使用率上限
Bool_def is_gtja_bmbzq; 		///< 是否为国泰君安白名单债券,0.否；1.是
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部ID
SecuritySymbol_def security_symbol; 		///< 证券名称
/*
 * @brief 默认构造函数
 */
  AgwBondPledgeArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwBondPledgeArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 债券质押库查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultBondPledge : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwBondPledgeArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultBondPledge();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultBondPledge& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultBondPledge;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询股东报盘席位
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySecuSeat : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySecuSeat();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySecuSeat& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySecuSeat;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询股东报盘席位结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSecuSeat : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 证券所属市场
PBUID_def pbu_id; 		///< 席位代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSecuSeat();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSecuSeat& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSecuSeat;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网上发行数组
 */
class EXPORT_MACRO_DEFINE AgwIPOInfoArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecurityType_def security_type; 		///< 证券类型
SecuritySymbol_def security_symbol; 		///< 证券简称
Price_def price_upper_limit; 		///< 最高价格
Price_def price_lower_limit; 		///< 最低价格
IPOType_def ipo_type; 		///< 配售类型
Date8_def start_date; 		///< 开始日期
Date8_def end_date; 		///< 结束日期
Qty_def purchase_unit; 		///< 申购单位
Qty_def max_purchase_qty; 		///< 最大申购数量
IPOType_def send_status; 		///< 客户是否申购
Bool_def is_no_profit; 		///< 是否尚未盈利
Bool_def is_weighted_voting_rights; 		///< 是否存在投票权差异
Bool_def is_VIE; 		///< 是否具有协议控制架构
Bool_def is_registration; 		///< 是否注册制
Currency_def money_type; 		///< 货币代码
Price_def price; 		///< 申购价格
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
Char_def grade; 		///< 证券分层信息:3-新注册科创板成长层 4-存量科创板成长层
/*
 * @brief 默认构造函数
 */
  AgwIPOInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 2 + 40 + 8 + 8 + 2 + 8 + 8 + 8 + 8 + 2 + 1 + 1 + 1 + 1 + 4 + 8 + 64 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwIPOInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 新股信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultIPOInfo : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwIPOInfoArray> order_array; 		///< 网上发行数组
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultIPOInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultIPOInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultIPOInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资金查询(95521客服)消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryFundMaxdrawCrm : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryFundMaxdrawCrm();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryFundMaxdrawCrm& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryFundMaxdrawCrm;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资金查询(95521客服)结果组合
 */
class EXPORT_MACRO_DEFINE AgwFundMaxdrawCrmArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账号
BranchID_def branch_id; 		///< 营业部代码
Currency_def currency; 		///< 货币种类
Amt_def fund_bal; 		///< 资金余额
Amt_def fund_avl; 		///< 资金可用金额
Amt_def market_value; 		///< 资产总值
Amt_def fund_all; 		///< 资金资产
Amt_def stk_value; 		///< 市值
FundSeq_def fund_seq; 		///< 主资金标志
Amt_def max_draw; 		///< 可取金额
Amt_def fund_last_bal; 		///< 昨日余额
Amt_def fund_buy; 		///< 买入冻结
Amt_def fund_sale; 		///< 卖出解冻
Amt_def fd_fund; 		///< 禁取资金限额
Amt_def fd_fund_asset; 		///< 禁取资产限额
Amt_def fund_frz; 		///< 冻结总金额
Amt_def fund_unfrz; 		///< 解冻总金额
Amt_def fund_loan; 		///< 融资总金额
Date8_def int_date; 		///< 上次结息日期
String_def net_addr; 		///< MAC地址，取终端识别码
CreditFlag_def credit_flag; 		///< 资金帐号属性
Amt_def fund_bjhg_avl; 		///< 报价回购可用资金
Amt_def fund_asset_otc_end; 		///< otc到期部分金额
/*
 * @brief 默认构造函数
 */
  AgwFundMaxdrawCrmArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwFundMaxdrawCrmArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 资金查询(95521客服)查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultFundMaxdrawCrm : public AgwMsgExtQueryResultBase
{
public:
AgwPartition_def te_partition_no; 		///< 所属分区号
std::vector<AgwFundMaxdrawCrmArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultFundMaxdrawCrm();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultFundMaxdrawCrm& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultFundMaxdrawCrm;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户资产总值查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCustTotalAssets : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCustTotalAssets();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCustTotalAssets& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCustTotalAssets;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户资产总值查询
 */
class EXPORT_MACRO_DEFINE AgwCustTotalAssetsArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账号
BranchID_def branch_id; 		///< 营业部代码
Currency_def currency; 		///< 货币种类
Amt_def credit_avl; 		///< 融券卖出资金可用
Amt_def credit_frz_amt; 		///< 融券卖出资金买入冻结
Amt_def credit_buy; 		///< 融券卖出资金买入成交
Amt_def credit_sale; 		///< 融券卖出资金卖出成交
Amt_def credit_buy_sale; 		///< 融券卖出资金买卖实时差额
Amt_def relese_amt_unfrz; 		///< 当日累计释放（毛）
Amt_def hbjj_bal; 		///< 使用融券卖出资金购买货基的昨日存量
Amt_def hbjj_buy; 		///< 使用融券卖出资金购买货基的当日买入冻结
Amt_def hbjj_sale; 		///< 使用融券卖出资金购买货基的当日卖出解冻
Amt_def otc_fund_buy; 		///< OTC市场日间使用
Amt_def otc_fund_uncome_buy; 		///< OTC市场在途买入冻结
Amt_def otc_fund_uncome_sale; 		///< OTC市场在途卖出解冻
Amt_def oth_fund_buy; 		///< 给一组多金代销等小众使用
Amt_def oth_fund_uncome_buy; 		///< 给一组多金代销等小众使用
Amt_def oth_fund_uncome_sale; 		///< 给一组多金代销等小众使用
Amt_def fund_cash_bal; 		///< 现金管家余额
Amt_def fund_asset_total; 		///< 总资产
Amt_def fund_asset; 		///< 资金
Amt_def fund_asset_stk; 		///< 证券资产
Amt_def fund_asset_of; 		///< 开放式基金资产
Amt_def fund_asset_bond; 		///< 债券资产
Amt_def fund_asset_rzgh; 		///< 融资
Amt_def fund_asset_rqgh; 		///< 融券
Amt_def fund_asset_stkzy; 		///< 质押资产
Amt_def fund_asset_bjhg; 		///< 报价回购资产
Amt_def fund_asset_szbjhg; 		///< 深圳报价回购资产
Amt_def fund_asset_otc; 		///< OTC资产
Amt_def fund_asset_rzcs; 		///< 协议融资
Amt_def fund_asset_rqcs; 		///< 协议融券
Amt_def fund_asset_stkxyzy; 		///< 协议质押库资产
Amt_def fund_asset_otc_end; 		///< OTC到期资金信息
/*
 * @brief 默认构造函数
 */
  AgwCustTotalAssetsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCustTotalAssetsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户资产总值查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustTotalAssets : public AgwMsgExtQueryResultBase
{
public:
PartitionNo_def te_partition_no; 		///< 所属分区号
CustomerStatus_def status; 		///< 资金账户状态
CustName_def cust_name; 		///< 客户姓名
CustomerKind_def cust_type; 		///< 客户类型
std::vector<AgwCustTotalAssetsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustTotalAssets();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustTotalAssets& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustTotalAssets;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief APP理财资产查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryAppAssets : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryAppAssets();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryAppAssets& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryAppAssets;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief APP理财资产查询
 */
class EXPORT_MACRO_DEFINE AgwAppAssetsArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账号
BranchID_def branch_id; 		///< 营业部代码
Currency_def currency; 		///< 货币种类
Amt_def credit_avl; 		///< 融券卖出资金可用
Amt_def credit_frz_amt; 		///< 融券卖出资金买入冻结
Amt_def credit_buy; 		///< 融券卖出资金买入成交
Amt_def credit_sale; 		///< 融券卖出资金卖出成交
Amt_def credit_buy_sale; 		///< 融券卖出资金买卖实时差额
Amt_def relese_amt_unfrz; 		///< 当日累计释放（毛）
Amt_def hbjj_bal; 		///< 使用融券卖出资金购买货基的昨日存量
Amt_def hbjj_buy; 		///< 使用融券卖出资金购买货基的当日买入冻结
Amt_def hbjj_sale; 		///< 使用融券卖出资金购买货基的当日卖出解冻
Amt_def otc_fund_buy; 		///< OTC市场日间使用
Amt_def otc_fund_uncome_buy; 		///< OTC市场在途买入冻结
Amt_def otc_fund_uncome_sale; 		///< OTC市场在途卖出解冻
Amt_def oth_fund_buy; 		///< 给一组多金代销等小众使用
Amt_def oth_fund_uncome_buy; 		///< 给一组多金代销等小众使用
Amt_def oth_fund_uncome_sale; 		///< 给一组多金代销等小众使用
Amt_def fund_cash_bal; 		///< 现金管家余额
Amt_def fund_asset; 		///< 现金资产
Amt_def fund_asset_stk; 		///< 证券资产
Amt_def fund_asset_of; 		///< 理财资产
/*
 * @brief 默认构造函数
 */
  AgwAppAssetsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwAppAssetsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief APP理财资产查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAppAssets : public AgwMsgExtQueryResultBase
{
public:
PartitionNo_def te_partition_no; 		///< 所属分区号
CustomerStatus_def status; 		///< 资金账户状态
CustName_def cust_name; 		///< 客户姓名
CustomerKind_def cust_type; 		///< 客户类型
std::vector<AgwAppAssetsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAppAssets();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAppAssets& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAppAssets;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自研股份查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySelfShare : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
Currency_def currency; 		///< 货币种类,集中交易没有启作用
Bool_def mkt_cal_flag; 		///< 市值计算标识,集中交易没有启作用
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
Bool_def flag; 		///< 是否剔除北交所和老三板
SecurityClass_def security_class; 		///< 证券类别分类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySelfShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySelfShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySelfShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自研股份查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwSelfShareArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
SecurityType_def security_type; 		///< 证券类别
Currency_def currency; 		///< 货币种类
PBUID_def pbu_id; 		///< 席位代码
Qty_def last_stk_bal; 		///< 日初股份余额
Qty_def stk_bal; 		///< 股份余额
Qty_def stk_avl; 		///< 股份可用
Qty_def stk_can_trans; 		///< 股份可转
Qty_def stk_buy; 		///< 股份买入解冻
Qty_def stk_sell; 		///< 股份卖出冻结
Qty_def stk_uncome_buy; 		///< 在途股份买入解冻
Qty_def stk_uncome_sell; 		///< 在途股份卖出冻结
Qty_def stk_frz; 		///< 股份冻结
Qty_def stk_unfrz; 		///< 股份解冻
Qty_def stk_night_frz; 		///< 夜市冻结股份
Qty_def stk_diff; 		///< 可申赎数量
Qty_def stk_remain; 		///< 余券数量
Bool_def mkt_cal_flag; 		///< 市值计算标识
Price_def last_price; 		///< 最近价格
Price_def ext_last_price; 		///< 港股通最近价格
Price_def close_price; 		///< 收盘价格
Qty_def leaves_qty; 		///< 市值持仓,对应410503中的stkqty
Amt_def market_value; 		///< 市值
Qty_def assets_qty; 		///< 资产持仓,对应410503中的rembal
Amt_def buy_cost; 		///< 买入成本
Price_def cost_price; 		///< 成本价格
Amt_def profit_cost; 		///< 盈亏成本
Price_def profit_price; 		///< 盈亏成本价格
Amt_def pro_income; 		///< 累计浮动盈亏,对应410503中的proincome和totalprofitcost
Amt_def income; 		///< 买入浮动盈亏
CreditRatio_def profit_rate; 		///< 浮亏比例
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
TrdID_def trd_id; 		///< 是否可正常交易
SecurityID_def link_security_id; 		///< 退市前代码-预留新三板用
BondInterest_def bond_intr; 		///< 债券利息
Char2_def dividend_status; 		///< 除权除息状态，'00'非除权非除息，'10'除权非除息，'01'非除权除息，'11'除权除息
Char_def grade; 		///< 新三板证券分层信息
RightType_def right_type; 		///< 权益类别
Price_def limit_up; 		///< 涨停价格
Price_def limit_down; 		///< 跌停价格
Qty_def stk_frz_vip; 		///< 划出股份冻结包数量
Qty_def stk_internal_can_trans; 		///< 股份内部可转
Price_def hkd_profit_price; 		///< 港币盈亏成本价格
Qty_def buy_qty_unit; 		///< 买入最小申报单位数量
Char_def is_pledge; 		///< 是否担保品
CreditRatio_def pledge_rate; 		///< 折算率
Qty_def pledge_max_num; 		///< 担保品最大可划转数量
SecurityClass_def security_class; 		///< 证券类别分类
/*
 * @brief 默认构造函数
 */
  AgwSelfShareArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 40 + 2 + 4 + 6 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 64 + 1 + 8 + 8 + 2 + 1 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSelfShareArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 自研股份查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSelfShare : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwSelfShareArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSelfShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSelfShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSelfShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户自研股份查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditSelfShare : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
Currency_def currency; 		///< 货币种类,集中交易没有启作用
Bool_def mkt_cal_flag; 		///< 市值计算标识,集中交易没有启作用
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
Bool_def flag; 		///< 是否剔除北交所和老三板
SecurityClass_def security_class; 		///< 证券类别分类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditSelfShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditSelfShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditSelfShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自研股份查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditSelfShareArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
SecurityType_def security_type; 		///< 证券类别
Currency_def currency; 		///< 货币种类
PBUID_def pbu_id; 		///< 席位代码
Qty_def last_stk_bal; 		///< 日初股份余额
Qty_def stk_bal; 		///< 股份余额
Qty_def stk_avl; 		///< 股份可用
Qty_def stk_can_trans; 		///< 股份可转
Qty_def stk_buy; 		///< 股份买入解冻
Qty_def stk_sell; 		///< 股份卖出冻结
Qty_def stk_uncome_buy; 		///< 在途股份买入解冻
Qty_def stk_uncome_sell; 		///< 在途股份卖出冻结
Qty_def stk_frz; 		///< 股份冻结
Qty_def stk_unfrz; 		///< 股份解冻
Qty_def stk_night_frz; 		///< 夜市冻结股份
Qty_def stk_diff; 		///< 可申赎数量
Qty_def stk_remain; 		///< 余券数量
Bool_def mkt_cal_flag; 		///< 市值计算标识
Price_def last_price; 		///< 最近价格
Price_def ext_last_price; 		///< 港股通最近价格
Price_def close_price; 		///< 收盘价格
Qty_def leaves_qty; 		///< 市值持仓,对应410503中的stkqty
Amt_def market_value; 		///< 市值
Qty_def assets_qty; 		///< 资产持仓,对应410503中的rembal
Amt_def buy_cost; 		///< 买入成本
Price_def cost_price; 		///< 成本价格
Amt_def profit_cost; 		///< 盈亏成本
Price_def profit_price; 		///< 盈亏成本价格
Amt_def pro_income; 		///< 累计浮动盈亏,对应410503中的proincome和totalprofitcost
Amt_def income; 		///< 买入浮动盈亏
CreditRatio_def profit_rate; 		///< 浮亏比例
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
TrdID_def trd_id; 		///< 是否可正常交易
SecurityID_def link_security_id; 		///< 退市前代码-预留新三板用
BondInterest_def bond_intr; 		///< 债券利息
Char2_def dividend_status; 		///< 除权除息状态，'00'非除权非除息，'10'除权非除息，'01'非除权除息，'11'除权除息
Char_def grade; 		///< 新三板证券分层信息
RightType_def right_type; 		///< 权益类别
Price_def limit_up; 		///< 涨停价格
Price_def limit_down; 		///< 跌停价格
Qty_def stk_frz_vip; 		///< 划出股份冻结包数量
Qty_def stk_internal_can_trans; 		///< 股份内部可转
Price_def hkd_profit_price; 		///< 港币盈亏成本价格
Qty_def buy_qty_unit; 		///< 买入最小申报单位数量
Char_def is_pledge; 		///< 是否担保品
CreditRatio_def pledge_rate; 		///< 折算率
Qty_def pledge_max_num; 		///< 担保品最大可划转数量
Qty_def fund_margin_qty; 		///< 融资数量
SecurityClass_def security_class; 		///< 证券类别分类
/*
 * @brief 默认构造函数
 */
  AgwCreditSelfShareArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 40 + 2 + 4 + 6 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 64 + 1 + 8 + 8 + 2 + 1 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditSelfShareArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用账户自研股份查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditSelfShare : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwCreditSelfShareArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditSelfShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditSelfShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditSelfShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户保证金余额划转流水查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditMarginAvlTransFlow : public AgwMsgExtQueryBase
{
public:
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditMarginAvlTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditMarginAvlTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditMarginAvlTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 保证金余额划转流水查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditMarginAvlTransFlowArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部代码
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
MarketTransDirect_def margin_avl_direct; 		///< 保证金余额市场划转方向
Amt_def margin_avl_value; 		///< 保证金余额划转金额
/*
 * @brief 默认构造函数
 */
  AgwCreditMarginAvlTransFlowArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 8 + 1 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditMarginAvlTransFlowArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用账户保证金余额划转流水查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditMarginAvlTransFlow : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 资金划转流水记录总数
std::vector<AgwCreditMarginAvlTransFlowArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditMarginAvlTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditMarginAvlTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditMarginAvlTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户可用授信额度划转流水查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditLimitTransFlow : public AgwMsgExtQueryBase
{
public:
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditLimitTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditLimitTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditLimitTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可用授信额度划转流水查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditLimitTransFlowArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部代码
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
MarketTransDirect_def limit_direct; 		///< 可用授信额度市场划转方向
Amt_def limit_value; 		///< 可用授信额度划转金额
Amt_def fund_limit_value; 		///< 融资可用授信额度划转金额
Amt_def stk_limit_value; 		///< 融券可用授信额度划转金额
/*
 * @brief 默认构造函数
 */
  AgwCreditLimitTransFlowArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 8 + 1 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditLimitTransFlowArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用账户可用授信额度划转流水查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditLimitTransFlow : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 资金划转流水记录总数
std::vector<AgwCreditLimitTransFlowArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditLimitTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditLimitTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditLimitTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 证券信息(余额、价格等)查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryAvlPrice : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 交易市场
Side_def side; 		///< 买卖方向
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryAvlPrice();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryAvlPrice& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryAvlPrice;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 证券信息(余额、价格等)查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAvlPrice : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
SecurityType_def security_type; 		///< 证券类别
Currency_def currency; 		///< 货币代码
Amt_def fund_avl; 		///< 资金可用
Amt_def fund_bal; 		///< 资金余额
Qty_def stk_avl; 		///< 证券可用
Qty_def stk_max_qty; 		///< 证券可卖数量
Price_def last_price; 		///< 最近价格
Price_def up_price; 		///< 涨停价格
Price_def down_price; 		///< 跌停价格
HandFlag_def hand_flag; 		///< 股手标志
BondInterest_def bond_intr; 		///< 债券利息
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAvlPrice();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAvlPrice& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAvlPrice;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自研资金查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySelfFund : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
Amt_def xjgj_redemption_amt; 		///< 现金管家现金宝可快赎金额
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySelfFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySelfFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySelfFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自研资金查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwSelfFundArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 货币种类
FundSeq_def fund_seq; 		///< 主资金账号
Amt_def fund_last_bal; 		///< 昨日余额
Amt_def fund_bal; 		///< 资金余额
Amt_def fund_avl; 		///< 资金可用
Amt_def credit_bal; 		///< 融券卖出资金余额
Amt_def credit_avl; 		///< 融券卖出资金可用
Amt_def credit_frz_amt; 		///< 融券卖出资金买入冻结
Amt_def credit_buy; 		///< 融券卖出资金买入成交
Amt_def credit_sale; 		///< 融券卖出资金卖出成交
Amt_def credit_buy_sale; 		///< 融券卖出资金买卖实时差额
Amt_def relese_amt_unfrz; 		///< 当日累计释放（毛）
Amt_def hbjj_bal; 		///< 使用融券卖出资金购买货基的昨日存量
Amt_def hbjj_buy; 		///< 使用融券卖出资金购买货基的当日买入冻结
Amt_def hbjj_sale; 		///< 使用融券卖出资金购买货基的当日卖出解冻
Amt_def otc_fund_buy; 		///< OTC市场日间使用
Amt_def otc_fund_uncome_buy; 		///< OTC市场在途买入冻结
Amt_def otc_fund_uncome_sale; 		///< OTC市场在途卖出解冻
Amt_def oth_fund_buy; 		///< 给一组多金代销等小众使用
Amt_def oth_fund_uncome_buy; 		///< 给一组多金代销等小众使用
Amt_def oth_fund_uncome_sale; 		///< 给一组多金代销等小众使用
Amt_def fund_cash_bal; 		///< 现金管家余额
Amt_def fund_asset; 		///< 资金资产，对应410502/420502中的fund
Amt_def stk_asset; 		///< 证券资产，对应410502/420502中的stkvalue
Amt_def total_asset; 		///< 总资产，对应410502/420502中的marketvalue
Amt_def fund_buy; 		///< 买入冻结
Amt_def fund_sale; 		///< 卖出解冻
Amt_def max_draw; 		///< 可取金额
Amt_def fund_bjhg_avl; 		///< 报价回购可用资金
Amt_def max_draw_test; 		///< 考虑现金管家的试算可取金额
Amt_def max_xjgj_draw_test; 		///< 修正后现金管家可取金额
/*
 * @brief 默认构造函数
 */
  AgwSelfFundArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSelfFundArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 自研资金查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSelfFund : public AgwMsgExtQueryResultBase
{
public:
PartitionNo_def te_partition_no; 		///< 所属分区号
std::vector<AgwSelfFundArray> order_array; 		///< 自研资金查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSelfFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSelfFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSelfFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资产负债查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryAssetDebts : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryAssetDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryAssetDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryAssetDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资产负债查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAssetDebts : public AgwMsgExtQueryResultBase
{
public:
Currency_def currency; 		///< 货币种类
CustName_def cust_name; 		///< 客户姓名
CreditStatus_def credit_status; 		///< 信用状态
CreditRatio_def keep_rate; 		///< 维持担保比例
Amt_def fund; 		///< 资金
Amt_def fund_asset; 		///< 资金资产
Amt_def stk_asset; 		///< 证券资产
Amt_def bond_asset; 		///< 债券资产
Amt_def asset; 		///< 总资产（折算后）
Amt_def old_asset; 		///< 原总资产
Amt_def liability; 		///< 总负债
Amt_def fund_bal; 		///< 资金余额
Amt_def fund_avl; 		///< 资金可用数
Amt_def fund_avl_frz; 		///< 资金可用冻结数
Amt_def d_sale_amt_bal; 		///< 融券卖出资金余额
Amt_def d_sale_amt_frz; 		///< 融券卖出资金冻结数
Amt_def credit_buy; 		///< 买券还券使用成交
Amt_def credit_sale; 		///< 融券卖出成交
Amt_def credit_buy_sale; 		///< 融券卖出及买券还券差额
Amt_def relese_amt_unfrz; 		///< 当日累计释放
Amt_def credit_amt; 		///< 当日累计释放（息费偿还部分）
Amt_def hbjj_bal; 		///< 使用融券卖出资金购买货基的昨日存量
Amt_def hbjj_buy; 		///< 使用融券卖出资金当日买入货基冻结
Amt_def hbjj_sale; 		///< 专用资金货基当日退还
Amt_def hbjj_amt; 		///< 专用资金货基累计占用
Amt_def hbjj_remain_amt; 		///< 专用资金货基剩余占用
Amt_def gage_mkt_avl; 		///< 担保证券市值
Amt_def gage_mkt_avl_zs; 		///< 修正市值
Amt_def gage_mkt_avl_db; 		///< 担保持仓市值下的标的持仓市值
Amt_def rights_amt; 		///< 修正资产
Amt_def f_deal_avl; 		///< 应付融资款
Amt_def fund_fee; 		///< 融资利息
Amt_def fund_pro_fee; 		///< 融资承诺费
Amt_def fund_manage_fee; 		///< 融资管理费
Amt_def fund_over_due_fee; 		///< 融资逾期利息
Amt_def fund_puni_fee; 		///< 融资罚息
Amt_def fund_puni_debts; 		///< 融资逾期本金罚息
Amt_def fund_manage_over_due; 		///< 融资逾期管理费
Amt_def fund_manage_puni_ntr; 		///< 融资逾期管理费罚息
Amt_def f_fee; 		///< 融资息费合计
Amt_def f_total_debts; 		///< 融资负债合计
Amt_def deal_f_mkt_avl; 		///< 应付融券市值
Amt_def stk_fee; 		///< 融券息费
Amt_def stk_pro_fee; 		///< 融券承诺费
Amt_def stk_manage_fee; 		///< 融券管理费
Amt_def stk_over_due_fee; 		///< 融券逾期费用
Amt_def stk_puni_fee; 		///< 融券罚息
Amt_def over_due_rights; 		///< 融券权益逾期费用
Amt_def puni_rights; 		///< 融券权益罚息
Amt_def stk_puni_debts; 		///< 融券逾期本金罚息
Amt_def stk_manage_over_due; 		///< 融券逾期管理费
Amt_def stk_manage_puni_ntr; 		///< 融券逾期管理费罚息
Amt_def d_fee; 		///< 融券息费合计
Amt_def d_total_debts; 		///< 融券负债合计
Amt_def fund_draw; 		///< 可取资产
Amt_def fill_amt; 		///< 补仓金额
Amt_def credit_bal; 		///< 授信额度
Amt_def credit_avl; 		///< 授信可用的额度
Amt_def f_credit_bal; 		///< 融资授信额度
Amt_def f_credit_avl; 		///< 融资可用额度
Amt_def d_credit_bal; 		///< 融券授信额度
Amt_def d_credit_avl; 		///< 融券可用额度
Amt_def total_profit_cost; 		///< 总盈亏
Amt_def stk_unused_fee; 		///< 锁券占用费
Amt_def stk_puni_unused_fee; 		///< 逾期锁券占用费罚息
Amt_def fund_margin_frz; 		///< 冻结融资保证金
Amt_def stk_margin_frz; 		///< 冻结融券保证金
Amt_def market_value; 		///< 资产总值
FundSeq_def fund_seq; 		///< 资金序号
Amt_def stk_value; 		///< 所有持仓的证券市值
Amt_def d_sale_bal; 		///< 融券卖出资金余额
Amt_def credit_frz_amt; 		///< 买券还券委托冻结
Amt_def long_profit_cost; 		///< 多头盈亏
Amt_def short_profit_cost; 		///< 空头盈亏
CreditRatio_def lever_rate; 		///< 杠杆率
Amt_def asset_frz_package_out; 		///< 划出股份冻结包市值
Amt_def asset_frz_package_in; 		///< 划入股份冻结包市值
Date8_def trade_date; 		///< 当前资产交易日期
Amt_def max_draw; 		///< 可取金额
Amt_def max_draw2; 		///< 可取金额(不考虑维保)
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAssetDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAssetDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAssetDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可取资金查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryFundMaxdraw : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryFundMaxdraw();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryFundMaxdraw& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryFundMaxdraw;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可取资金查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwFundMaxdrawArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账号
BranchID_def branch_id; 		///< 营业部代码
Currency_def currency; 		///< 货币种类
Amt_def fund_bal; 		///< 资金余额
Amt_def fund_avl; 		///< 资金可用金额
Amt_def fund_buy; 		///< 买入冻结
Amt_def fund_sale; 		///< 卖出解冻
Amt_def market_value; 		///< 资产总值
Amt_def fund_all; 		///< 资金资产
FundSeq_def fund_seq; 		///< 主资金标志
Amt_def stk_value; 		///< 市值
Amt_def max_draw; 		///< 可取金额
String_def net_addr; 		///< MAC地址，取终端识别码
BankCode_def bank_code; 		///< 银行代码
Amt_def fund_bjhg_avl; 		///< 报价回购可用资金
CreditFlag_def credit_flag; 		///< 资金帐号属性
Amt_def stk_value_non_of_cash; 		///< 含开放式基金不含现金管理产品的市值
Amt_def of_cash; 		///< 现金管理产品的市值
/*
 * @brief 默认构造函数
 */
  AgwFundMaxdrawArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwFundMaxdrawArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 可取资金查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultFundMaxdraw : public AgwMsgExtQueryResultBase
{
public:
AgwPartition_def te_partition_no; 		///< 所属分区号
std::vector<AgwFundMaxdrawArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultFundMaxdraw();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultFundMaxdraw& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultFundMaxdraw;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 标的证券查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTargetStk : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTargetStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTargetStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 标的证券查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTargetStkArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券名称
CreditCtrl_def credit_fund_ctrl; 		///< 融资状态 0-允许 1-禁止
CreditCtrl_def credit_stk_ctrl; 		///< 融券状态 0-允许 1-禁止
MarginCalKind_def fund_cal_kind; 		///< 融资保证金比例计算方式
CreditRatio_def margin_rate_fund; 		///< 融资保证金比例
MarginCalKind_def stk_cal_kind; 		///< 融券保证金比例计算方式
CreditRatio_def margin_rate_stk; 		///< 融券保证金比例
Qty_def sep_rem_qty; 		///< 融券专户余券
IsStkSource_def is_stk_source; 		///< 是否券库清单
CreditCtrl_def f_permit; 		///< 融资状态 0-允许 1-禁止
CreditCtrl_def d_permit; 		///< 融券状态 0-允许 1-禁止
CreditRatio_def f_margin_rate; 		///< 融资保证金比例
CreditRatio_def d_margin_rate; 		///< 融券保证金比例
SecurityFullSymbol_def security_full_symbol; 		///< 证券长名称
/*
 * @brief 默认构造函数
 */
  AgwTargetStkArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 1 + 1 + 1 + 8 + 1 + 8 + 8 + 1 + 1 + 1 + 8 + 8 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTargetStkArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 标的证券查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultTargetStk : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 合约持仓记录总数
std::vector<AgwTargetStkArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultTargetStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultTargetStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约明细查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsDetail : public AgwMsgExtQueryBase
{
public:
Date8_def sys_date; 		///< 系统时间,填0查所有
Sno_def debts_sno; 		///< 合约编号,填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsDetail();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsDetail& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsDetail;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约明细查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditDebtsDetailArray : public mp::RootPacket
{
public:
CreditDirect_def credit_direct; 		///< 信用方向
Date8_def sys_date; 		///< 负债发生系统日期
Sno_def debts_sno; 		///< 负债流水号
CustID_def spec_date; 		///< 头寸组
FundAccountID_def spec_sno; 		///< 头寸组
DebtsDetailKind_def debts_detail_kind; 		///< 负债明细类型
Currency_def currency; 		///< 币种
CreditRatio_def fund_intr_rate; 		///< 融资年利率
CreditRatio_def fund_manage_rate; 		///< 融资管理年费率
CreditRatio_def fund_pro_rate; 		///< 融资承诺费率
CreditRatio_def stk_intr_rate; 		///< 融券年费率
CreditRatio_def stk_manage_rate; 		///< 融券管理年费率
CreditRatio_def stk_pro_rate; 		///< 融券承诺费率，暂时未启用
CreditRatio_def punish_intr_rate; 		///< 罚息年利率,暂时未启用
CreditRatio_def unused_fee_rate; 		///< 锁券未使用的费率,锁券分段模式填
CreditRatio_def violate_fee_rate; 		///< 锁券的话 是违约费率【到期回收不到的部分 计违约】
Price_def lock_price; 		///< 定价模式的头寸有意义
Qty_def use_qty; 		///< 头寸初始使用数量
Qty_def stk_repay; 		///< 累计偿还交收值
Qty_def stk_repay_unfrz; 		///< 日间偿还成交值
Amt_def fee; 		///< 融资利息或融券费用
Amt_def puni_fee; 		///< 逾期息费罚息
Amt_def puni_rights; 		///< 逾期权益罚息
Amt_def puni_debts; 		///< 逾期本金罚息
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场
/*
 * @brief 默认构造函数
 */
  AgwCreditDebtsDetailArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 8 + 11 + 16 + 16 + 1 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 2;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditDebtsDetailArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用客户融资融券合约明细查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsDetail : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 合约持仓记录总数
std::vector<AgwCreditDebtsDetailArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsDetail();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsDetail& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsDetail;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebts : public AgwMsgExtQueryBase
{
public:
CreditDirect_def credit_direct; 		///< 信用方向,必填
CreditDebtsQueryFlag_def query_flag; 		///< 查询方向,必填
Date8_def sys_date; 		///< 系统时间,填0查所有
Sno_def debts_sno; 		///< 合约编号,填空查所有
Currency_def currency; 		///< 币种,默认人民币,目前不校验
MarketID_def market_id; 		///< 市场ID,融券合约查询时使用,填空0所有
SecurityID_def security_id; 		///< 证券ID,融券合约查询时使用,填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditDebtsArray : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 负债发生系统日期
Date8_def end_date; 		///< 负债截止日期
Sno_def debts_sno; 		///< 负债流水号
Currency_def currency; 		///< 币种
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券简称
Date8_def order_date; 		///< 委托日期
ClOrdNO_def cl_ord_no; 		///< 委托序号
AccountID_def account_id; 		///< 证券账户ID
LifeStatus_def life_status; 		///< 合约状态
MarketID_def market_id; 		///< 市场ID
CreditDirect_def credit_direct; 		///< 融资方向
Qty_def match_qty; 		///< 成交数量
Qty_def stk_repay; 		///< T日之前归还数量
Amt_def clear_amt; 		///< 清算金额
Amt_def credit_repay; 		///< T日之前归还金额
Qty_def d_stk_bal; 		///< 融资数量(当前融资数量)||应偿还融券总数量
Amt_def f_deal_bal; 		///< 应付融资款余额 || 融券未偿还金额
Amt_def f_deal_avl; 		///< 应付融资款的当前数 || 当前融券未偿还金额
Price_def init_cost_price; 		///< 初始成本价格
Price_def prof_cost_price; 		///< 盈亏成本价格
Amt_def profit_cost; 		///< 浮动盈亏
Qty_def stk_repay_unfrz; 		///< 当日归还数量
Amt_def fee_unfrz; 		///< 当日息费偿还总额
Amt_def over_due_fee_unfrz; 		///< 融资逾期利息或融券逾期费用当日偿还部分
Amt_def puni_fee_unfrz; 		///< 逾期息费罚息当日偿还部分
Amt_def over_due_rights_unfrz; 		///< 融券逾期权益当日偿还部分
Amt_def puni_rights_unfrz; 		///< 逾期权益罚息当日偿还部分
Amt_def puni_debts_unfrz; 		///< 逾期本金罚息当日偿还部分
Amt_def all_fee_unfrz; 		///< 当日偿还息费总额
Amt_def relese_amt; 		///< 累计已释放金额（毛）
Amt_def relese_amt_unfrz; 		///< 当日已释放金额（毛）
Amt_def credit_repay_unfrz; 		///< 当日归还金额
Amt_def f_exp_cet_intr; 		///< 融资利息及费用
Qty_def f_debt_qty; 		///< 融资股份余额
Qty_def d_stk_avl; 		///< 应偿还融券当前数量
Price_def last_price; 		///< 最新价
Amt_def mkt_val; 		///< 最新市值
Amt_def buy_cost; 		///< 融券成本
Amt_def exp_cet_intr; 		///< 融券费用
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
Amt_def accum_profit_cost; 		///< 累计盈亏(不考虑释放金额)
CreditRatio_def fund_intr_rate; 		///< 融资年利率
/*
 * @brief 默认构造函数
 */
  AgwCreditDebtsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 11 + 4 + 8 + 40 + 8 + 8 + 12 + 1 + 2 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 64 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditDebtsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用客户融资融券合约查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebts : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 满足条件记录总数,小于等于传入的total_num
std::vector<AgwCreditDebtsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsPB : public AgwMsgExtQueryBase
{
public:
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsPB();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsPB& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsPB;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsPB : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 满足条件记录总数,小于等于传入的total_num
std::vector<AgwCreditDebtsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsPB();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsPB& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsPB;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsQFII : public AgwMsgExtQueryBase
{
public:
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsQFII();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsQFII& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsQFII;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用客户融资融券合约查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsQFII : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 满足条件记录总数,小于等于传入的total_num
std::vector<AgwCreditDebtsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsQFII();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsQFII& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsQFII;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按证券融资融券合约查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsBySecurity : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsBySecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsBySecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsBySecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按证券融资融券合约查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsBySecurity : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 满足条件记录总数,小于等于传入的total_num
std::vector<AgwCreditDebtsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsBySecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsBySecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsBySecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 担保证券查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryPledgeStk : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 查询方向
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryPledgeStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryPledgeStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryPledgeStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 担保证券查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwPledgeStkArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
CreditRatio_def pledge_rate; 		///< 折算率
SecurityFullSymbol_def security_full_symbol; 		///< 证券长名称
/*
 * @brief 默认构造函数
 */
  AgwPledgeStkArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwPledgeStkArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 担保证券查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultPledgeStk : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 合约持仓记录总数
std::vector<AgwPledgeStkArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultPledgeStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultPledgeStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultPledgeStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用投票客户查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditVote : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 投票代码
VotingProposal_def voting_proposal; 		///< 投票议案
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
Bool_def error_flag; 		///< 是否返回报错的投票信息
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditVote();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditVote& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditVote;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用投票客户结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditVoteArray : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 交易日期
Date8_def oper_date; 		///< 发生日期
Date8_def oper_time; 		///< 发生时间
Currency_def currency; 		///< 货币类型
AccountID_def account_id; 		///< 证券账户ID
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 投票代码
SecurityID_def link_security_id; 		///< 基础证券代码
VotingProposal_def voting_proposal; 		///< 投票议案
VotingPreference_def voting_preference; 		///< 投票意见
Qty_def stk_bal; 		///< 已投票数
SecuritySymbol_def security_symbol; 		///< 投票简称
SecuritySymbol_def link_security_symbol; 		///< 基础证券简称
GeneralMeetingSeq_def general_meeting_code; 		///< 股东大会编码
VotingProposalType_def voting_proposal_type; 		///< 议案类型
MeetingDesc_def meeting_desc; 		///< 股东大会名称
VotingProposalDesc_def voting_prososal_desc; 		///< 投票议案名称
SecurityFullSymbol_def security_full_symbol; 		///< 投票全称
String_def key_error_code; 		///< 关键错误码
String_def key_error_msg; 		///< 订单拒绝原因描述（返回给客户端）
/*
 * @brief 默认构造函数
 */
  AgwCreditVoteArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditVoteArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用投票客户查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditVote : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 合约持仓记录总数
std::vector<AgwCreditVoteArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditVote();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditVote& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditVote;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用投票可投票数查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditVoteCount : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 投票代码
MarketID_def market_id; 		///< 市场代码
GeneralMeetingSeq_def general_meeting_seq; 		///< 股东大会编码
VotingProposal_def voting_proposal; 		///< 投票议案
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditVoteCount();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditVoteCount& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditVoteCount;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用投票可投票数查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditVoteCount : public AgwMsgExtQueryResultBase
{
public:
SecurityID_def security_id; 		///< 投票代码
VotingProposal_def voting_proposal; 		///< 投票议案
Qty_def rest_qty; 		///< 可投票数
SecuritySymbol_def security_symbol; 		///< 证券简称
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditVoteCount();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditVoteCount& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditVoteCount;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 指定合约账户资金查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryAccountContractFund : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryAccountContractFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryAccountContractFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryAccountContractFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 指定合约账户资金查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAccountContractFund : public AgwMsgExtQueryResultBase
{
public:
Amt_def init_total_amt; 		///< 日初总资金
Amt_def init_available_amt; 		///< 日初可用资金
Amt_def available_amt; 		///< 可用资金
Amt_def frozen_amt; 		///< 冻结资金
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAccountContractFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAccountContractFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAccountContractFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 合约账户持仓结果组合
 */
class EXPORT_MACRO_DEFINE AgwContractArray : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 合约编码
MarketID_def market_id; 		///< 市场ID
Qty_def buy_init_value; 		///< 买开仓日初持仓量
Qty_def buy_value; 		///< 买开仓可用持仓量
Qty_def buy_frozen_value; 		///< 买开仓冻结量
Qty_def sell_init_value; 		///< 卖开仓日初持仓量
Qty_def sell_value; 		///< 卖开仓可用持仓量
Qty_def sell_frozen_value; 		///< 卖开仓冻结量
/*
 * @brief 默认构造函数
 */
  AgwContractArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwContractArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用持仓集中度-旧查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryConcentration : public AgwMsgExtQueryBase
{
public:
CreditParamKind_def credit_kind; 		///< 参数类别
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayConcentration : public mp::RootPacket
{
public:
CreditParamKind_def credit_kind; 		///< 参数类别
CreditName_def credit_name; 		///< 参数名称
CreditRatio_def keep_rate_begin; 		///< 维保控制起始值
CreditRatio_def keep_rate_end; 		///< 维保控制结束值
Amt_def asset_begin; 		///< 资产规模控制起始值
Amt_def asset_end; 		///< 资产规模控制结束值
CreditRatio_def ctrl_rate; 		///< 控制比例
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Amt_def debt_begin; 		///< 总负债起始值
Amt_def debt_end; 		///< 总负债结束值
CtrlType_def ctrl_type; 		///< 集中度类型
/*
 * @brief 默认构造函数
 */
  AgwArrayConcentration();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 40 + 8 + 8 + 8 + 8 + 8 + 2 + 8 + 40 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用持仓集中度-旧查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultConcentration : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 板块集中度记录总数
std::vector<AgwArrayConcentration> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 集中度类型查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryConcentrationType : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryConcentrationType();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryConcentrationType& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryConcentrationType;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 集中度类型查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultConcentrationType : public AgwMsgExtQueryResultBase
{
public:
CtrlType_def ctrl_type; 		///< 集中度类型
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultConcentrationType();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultConcentrationType& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultConcentrationType;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用交易风险四条预警线外部查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditCollateralRatio : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditCollateralRatio();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditCollateralRatio& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditCollateralRatio;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用交易风险四条预警线外部查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditCollateralRatio : public AgwMsgExtQueryResultBase
{
public:
CreditRatio_def fund_alarm_rate; 		///< 关注线
CreditRatio_def fund_force_rate; 		///< 警戒线
CreditRatio_def fund_control; 		///< 提款线
CreditRatio_def app_clear_rate; 		///< 新平仓线
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditCollateralRatio();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditCollateralRatio& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditCollateralRatio;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 总体集中度查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditConcentration : public AgwMsgExtQueryBase
{
public:
CreditParamKind_def credit_kind; 		///< 参数类别
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayCreditConcentration : public mp::RootPacket
{
public:
ConcenCreditID_def credit_id; 		///< 集中度参数组标识
CreditParamKind_def credit_kind; 		///< 参数类别
CreditName_def credit_name; 		///< 参数名称
CreditRatio_def keep_rate_begin; 		///< 维保控制起始值
CreditRatio_def keep_rate_end; 		///< 维保控制结束值
Amt_def asset_begin; 		///< 资产规模控制起始值
Amt_def asset_end; 		///< 资产规模控制结束值
CreditRatio_def ctrl_rate; 		///< 控制比例
Amt_def debts_begin; 		///< 负债规模控制起始值
Amt_def debts_end; 		///< 负债规模控制结束值
/*
 * @brief 默认构造函数
 */
  AgwArrayCreditConcentration();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 1 + 40 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayCreditConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 总体集中度查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditConcentration : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 总体集中度记录总数
std::vector<AgwArrayCreditConcentration> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 合同查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditContract : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditContract();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditContract& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditContract;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 合同查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditContract : public AgwMsgExtQueryResultBase
{
public:
Currency_def currency; 		///< 币种
ContractLimitMonth_def contract_limit; 		///< 合同期限
Date8_def begin_date; 		///< 合同起始日期 
Date8_def end_date; 		///< 合同结束日期 
ContractStatus_def contract_status; 		///< 合同状态
Amt_def credit_bal; 		///< 信用额度
Amt_def f_credit_bal; 		///< 融资额度
CreditRatio_def f_credit_rate; 		///< 融资年利率
CreditRatio_def f_year_mg_fee; 		///< 融资管理费率
CreditRatio_def f_year_sr_fee; 		///< 融资承诺费率
Amt_def d_credit_bal; 		///< 融券额度
CreditRatio_def d_credit_rate; 		///< 融券年利率
CreditRatio_def d_year_mg_fee; 		///< 融券管理费率
CreditRatio_def d_year_sr_fee; 		///< 融券承诺费率
CreditRatio_def punish_intr_rate; 		///< 罚息年利率
ContractID_def contract_no; 		///< 合同编号
Date8_def cancel_date; 		///< 合同实际终止日期
CreditRatio_def f_prate; 		///< 融资罚息年利率
CreditRatio_def d_prate; 		///< 融券罚息年利率
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditContract();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditContract& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditContract;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券情况汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsCollect : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
CreditDirect_def credit_direct; 		///< 信用方向
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券情况汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditDebtsCollectArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 币种
MarketID_def market_id; 		///< 市场ID
AccountID_def account_id; 		///< 账户ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
CreditDirect_def credit_direct; 		///< 信用方向
Amt_def clear_amt; 		///< 成交金额
Qty_def match_qty; 		///< 成交数量
Amt_def credit_repay; 		///< T日之前归还金额
Qty_def stk_repay; 		///< T日之前归还数量
Amt_def credit_repay_unfrz; 		///< T日归还金额
Qty_def stk_repay_unfrz; 		///< T日归还数量
Amt_def credit_bal; 		///< 剩余融资金额
Qty_def stk_bal; 		///< 剩余融券数量
Price_def last_price; 		///< 最新价
Amt_def market_value; 		///< 剩余负债市值
Amt_def credit_avl; 		///< 剩余融券卖出资金
Amt_def debts_profit; 		///< 浮动盈亏
Amt_def profit2; 		///< 矫正盈亏
Amt_def credit_fee; 		///< 融资利息或融券费用
Qty_def stk_debts_avl; 		///< 当日可还融券数量
Price_def profit_cost_price; 		///< 盈亏成本价格
Amt_def fee; 		///< 合约各科目费用汇总
Char2_def dividend_status; 		///< 除权除息状态，'00'非除权非除息，'10'除权非除息，'01'非除权除息，'11'除权除息
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
Char_def grade; 		///< 新三板证券分层信息
/*
 * @brief 默认构造函数
 */
  AgwCreditDebtsCollectArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 2 + 12 + 8 + 40 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 2 + 8 + 8 + 64 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditDebtsCollectArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 融资融券情况汇总查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsCollect : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 信用当日成交汇总记录总数
std::vector<AgwCreditDebtsCollectArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券情况汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsCollectNew : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
CreditDirect_def credit_direct; 		///< 信用方向
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsCollectNew();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsCollectNew& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsCollectNew;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券情况汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditDebtsCollectNewArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
Amt_def payable_margin_buy_bal; 		///< 应付融资款余额
Amt_def payable_margin_buy_cur; 		///< 应付融资款当前数
Amt_def payable_margin_buy_fee; 		///< 融资预计利息
Currency_def currency; 		///< 币种
AccountID_def account_id; 		///< 账户ID
CreditDirect_def credit_direct; 		///< 信用方向
Amt_def clear_amt; 		///< 成交金额
Qty_def match_qty; 		///< 成交数量
Amt_def credit_repay; 		///< T日之前归还金额
Qty_def stk_repay; 		///< T日之前归还数量
Amt_def credit_repay_unfrz; 		///< T日归还金额
Qty_def stk_repay_unfrz; 		///< T日归还数量
Amt_def credit_bal; 		///< 剩余融资金额
Qty_def stk_bal; 		///< 剩余融券数量
Price_def last_price; 		///< 最新价
Amt_def market_value; 		///< 剩余负债市值
Amt_def credit_avl; 		///< 剩余融券卖出资金
Amt_def debts_profit; 		///< 浮动盈亏
Amt_def profit2; 		///< 矫正盈亏
Amt_def credit_fee; 		///< 融资利息或融券费用
Qty_def stk_debts_avl; 		///< 当日可还融券数量
Price_def profit_cost_price; 		///< 盈亏成本价格
Amt_def fee; 		///< 合约各科目费用汇总
Char2_def dividend_status; 		///< 除权除息状态，'00'非除权非除息，'10'除权非除息，'01'非除权除息，'11'除权除息
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
Char_def grade; 		///< 新三板证券分层信息
RightType_def right_type; 		///< 权益类别
/*
 * @brief 默认构造函数
 */
  AgwCreditDebtsCollectNewArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 64 + 8 + 8 + 8 + 4 + 12 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 2 + 8 + 8 + 1 + 2;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditDebtsCollectNewArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 融资融券情况汇总查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsCollectNew : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 信用当日成交汇总记录总数
std::vector<AgwCreditDebtsCollectNewArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsCollectNew();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsCollectNew& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsCollectNew;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户资金划转流水查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditFundTransFlow : public AgwMsgExtQueryBase
{
public:
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditFundTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditFundTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditFundTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资金划转流水查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditFundTransFlowArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部代码
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
MarketTransDirect_def fund_avl_direct; 		///< 自有资金市场划转方向
Amt_def avl_value; 		///< 自有资金划转金额
MarketTransDirect_def fund_credit_avl_direct; 		///< 融券卖出资金市场划转方向
Amt_def credit_avl_value; 		///< 融券卖出资金划转金额
/*
 * @brief 默认构造函数
 */
  AgwCreditFundTransFlowArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 8 + 1 + 8 + 1 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditFundTransFlowArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用账户资金划转流水查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditFundTransFlow : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 资金划转流水记录总数
std::vector<AgwCreditFundTransFlowArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditFundTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditFundTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditFundTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用账户股份划转流水查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditShareTransFlow : public AgwMsgExtQueryBase
{
public:
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditShareTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditShareTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditShareTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股份划转流水查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditShareTransFlowArray : public mp::RootPacket
{
public:
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部代码
SeqNum_def flow_agw_seq_id; 		///< 流水的agw_seq_id
ShareTransDirect_def trans_direct; 		///< 股份划转方向
MarketTransDirect_def share_direct; 		///< 股份市场划转方向
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场代码
Qty_def value; 		///< 划转数量
/*
 * @brief 默认构造函数
 */
  AgwCreditShareTransFlowArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 8 + 1 + 1 + 8 + 2 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditShareTransFlowArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用账户股份划转流水查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditShareTransFlow : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 股份划转流水记录总数
std::vector<AgwCreditShareTransFlowArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditShareTransFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditShareTransFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditShareTransFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 标签集中度查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryLabelConcentration : public AgwMsgExtQueryBase
{
public:
LabelID_def label_id; 		///< 标签代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryLabelConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryLabelConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryLabelConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayLabelConcentration : public mp::RootPacket
{
public:
LabelID_def label_id; 		///< 标签代码
LabelName_def label_name; 		///< 标签名称
CreditRatio_def keep_rate_begin; 		///< 维保控制起始值
CreditRatio_def keep_rate_end; 		///< 维保控制结束值
Amt_def debt_begin; 		///< 总负债起始值
Amt_def debt_end; 		///< 总负债结束值
CreditRatio_def ctrl_rate; 		///< 控制比例
/*
 * @brief 默认构造函数
 */
  AgwArrayLabelConcentration();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 64 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayLabelConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 标签集中度查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultLabelConcentration : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 标签集中度记录总数
std::vector<AgwArrayLabelConcentration> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultLabelConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultLabelConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultLabelConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 标签所属证券查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryLabelStkCode : public AgwMsgExtQueryBase
{
public:
LabelID_def label_id; 		///< 标签代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryLabelStkCode();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryLabelStkCode& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryLabelStkCode;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayLabelStkCode : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
LabelID_def label_id; 		///< 标签代码
/*
 * @brief 默认构造函数
 */
  AgwArrayLabelStkCode();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayLabelStkCode& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 标签所属证券查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultLabelStkCode : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 标签所属证券记录总数
std::vector<AgwArrayLabelStkCode> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultLabelStkCode();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultLabelStkCode& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultLabelStkCode;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 公共券池标的券信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryZHShareTargetStk : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryZHShareTargetStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryZHShareTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryZHShareTargetStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayZHShareTargetStk : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券名称
CreditCtrl_def credit_fund_ctrl; 		///< 融资状态 0-允许 1-禁止
CreditCtrl_def credit_stk_ctrl; 		///< 融券状态 0-允许 1-禁止
MarginCalKind_def fund_cal_kind; 		///< 融资保证金比例计算方式
CreditRatio_def margin_rate_fund; 		///< 融资保证金比例
MarginCalKind_def stk_cal_kind; 		///< 融券保证金比例计算方式
CreditRatio_def margin_rate_stk; 		///< 融券保证金比例
Qty_def zh_stk_avl; 		///< 公共券池可用数量
Amt_def mkt_value; 		///< 公共券池标的券市值
SecurityFullSymbol_def security_full_symbol; 		///< 证券长名称
/*
 * @brief 默认构造函数
 */
  AgwArrayZHShareTargetStk();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 1 + 1 + 1 + 8 + 1 + 8 + 8 + 8 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayZHShareTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 公共券池标的券信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultZHShareTargetStk : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 标签所属证券记录总数
std::vector<AgwArrayZHShareTargetStk> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultZHShareTargetStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultZHShareTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultZHShareTargetStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户锁标的券信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditStkLockTargetStk : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditStkLockTargetStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditStkLockTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditStkLockTargetStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayCreditStkLockTargetStk : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券名称
CreditCtrl_def credit_fund_ctrl; 		///< 融资状态 0-允许 1-禁止
CreditCtrl_def credit_stk_ctrl; 		///< 融券状态 0-允许 1-禁止
MarginCalKind_def fund_cal_kind; 		///< 融资保证金比例计算方式
CreditRatio_def margin_rate_fund; 		///< 融资保证金比例
MarginCalKind_def stk_cal_kind; 		///< 融券保证金比例计算方式
CreditRatio_def margin_rate_stk; 		///< 融券保证金比例
Qty_def stk_lock_avl; 		///< 客户锁可用
Amt_def mkt_value; 		///< 客户锁市值
SecurityFullSymbol_def security_full_symbol; 		///< 证券长名称
/*
 * @brief 默认构造函数
 */
  AgwArrayCreditStkLockTargetStk();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 1 + 1 + 1 + 8 + 1 + 8 + 8 + 8 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayCreditStkLockTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户锁标的券信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditStkLockTargetStk : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 标签所属证券记录总数
std::vector<AgwArrayCreditStkLockTargetStk> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditStkLockTargetStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditStkLockTargetStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditStkLockTargetStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券个性化客户标签查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCustCreditRiskTag : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCustCreditRiskTag();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCustCreditRiskTag& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCustCreditRiskTag;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券个性化客户标签查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustCreditRiskTag : public AgwMsgExtQueryResultBase
{
public:
CustCreditTag_def fund_margin_rate_flag; 		///< 个性化融资保证金比例标签
CustCreditTag_def stk_margin_rate_flag; 		///< 个性化融资保证金比例标签
CustCreditTag_def pledge_rate_flag; 		///< 个性化折算率标签
CustCreditTag_def concentration_rate_flag; 		///< 个性化集中度标签
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustCreditRiskTag();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustCreditRiskTag& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustCreditRiskTag;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 两融客户风控指标(集中度保证金比例折算率)查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCustCreditRiskInfo : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCustCreditRiskInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCustCreditRiskInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCustCreditRiskInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 两融客户风控指标(集中度保证金比例折算率)查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustCreditRiskInfo : public AgwMsgExtQueryResultBase
{
public:
CreditRatio_def concentration_rate; 		///< 集中度比例
CreditRatio_def fund_margin_rate; 		///< 融资保证金比例
CreditRatio_def stk_margin_rate; 		///< 融券保证金比例
CreditRatio_def pledge_rate; 		///< 折算率
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustCreditRiskInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustCreditRiskInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustCreditRiskInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 专户融券卖数量查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryMarginSellQtySpecialAccount : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryMarginSellQtySpecialAccount();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryMarginSellQtySpecialAccount& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryMarginSellQtySpecialAccount;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayMarginSellQtySpecialAccount : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券名称
Qty_def sep_rem_qty; 		///< 融券专户余券
/*
 * @brief 默认构造函数
 */
  AgwArrayMarginSellQtySpecialAccount();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayMarginSellQtySpecialAccount& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 专户融券卖数量查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultMarginSellQtySpecialAccount : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 专户融券记录总数
std::vector<AgwArrayMarginSellQtySpecialAccount> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultMarginSellQtySpecialAccount();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultMarginSellQtySpecialAccount& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultMarginSellQtySpecialAccount;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券卖出冻结资金可买证券清单查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditAvlCanBuySecurity : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditAvlCanBuySecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditAvlCanBuySecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditAvlCanBuySecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券卖出冻结资金可买证券清单结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayCreditAvlCanBuySecurity : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券简称
CreditRatio_def pledge_rate; 		///< 折算率
Comments_def label_kind; 		///< 集中度标签分类
Bool_def limit_concentration_flag; 		///< 交易所集中度受限标识
SecurityType_def security_type; 		///< 证券类别
/*
 * @brief 默认构造函数
 */
  AgwArrayCreditAvlCanBuySecurity();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 64 + 1 + 2;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayCreditAvlCanBuySecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 融券卖出冻结资金可买证券清单查询結果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditAvlCanBuySecurity : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwArrayCreditAvlCanBuySecurity> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditAvlCanBuySecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditAvlCanBuySecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditAvlCanBuySecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户个性化担保证券查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditCustomerlizedPledgeSecurity : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditCustomerlizedPledgeSecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditCustomerlizedPledgeSecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditCustomerlizedPledgeSecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户个性化担保证券查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayCreditCustomerlizedPledgeSecurity : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
SecurityFullSymbol_def security_symbol; 		///< 证券简称
CreditRatio_def pledge_rate; 		///< 折算率
CreditRatio_def margin_rate_fund; 		///< 融资保证金比例
CreditRatio_def margin_rate_stk; 		///< 融资保证金比例
Comments_def label_kind; 		///< 集中度标签分类
Bool_def limit_concentration_flag; 		///< 交易所集中度受限标识
SecurityType_def security_type; 		///< 证券类别
/*
 * @brief 默认构造函数
 */
  AgwArrayCreditCustomerlizedPledgeSecurity();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 64 + 8 + 8 + 8 + 64 + 1 + 2;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayCreditCustomerlizedPledgeSecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户个性化担保证券查询結果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditCustomerlizedPledgeSecurity : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwArrayCreditCustomerlizedPledgeSecurity> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditCustomerlizedPledgeSecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditCustomerlizedPledgeSecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditCustomerlizedPledgeSecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户个性化信用交易风险四条预警线外部查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCustomerlizedCreditCollateralRatio : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCustomerlizedCreditCollateralRatio();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCustomerlizedCreditCollateralRatio& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCustomerlizedCreditCollateralRatio;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户个性化信用交易风险四条预警线外部查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustomerlizedCreditCollateralRatio : public AgwMsgExtQueryResultBase
{
public:
CreditRatio_def fund_alarm_rate; 		///< 关注线
CreditRatio_def fund_force_rate; 		///< 预警线
CreditRatio_def fund_control; 		///< 提取线
CreditRatio_def app_clear_rate; 		///< 平仓线
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustomerlizedCreditCollateralRatio();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustomerlizedCreditCollateralRatio& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustomerlizedCreditCollateralRatio;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融客户查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDCustInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDCustInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDCustInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDCustInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDCustInfo : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号ID
BranchID_def branch_id; 		///< 营业部ID
MarketID_def market_id; 		///< 市场ID
FundAccountID_def credit_fund_id; 		///< 信用资金账号
AccountID_def credit_account_id; 		///< 信用股东账户
ServerID_def credit_partition; 		///< 信用账户分区号
FundAccountID_def pt_fund_id; 		///< 普通资金账号
AccountID_def pt_account_id; 		///< 普通股东账户
ServerID_def pt_partition; 		///< 普通账户分区号
TradeRights_def trade_rights; 		///< 交易权限
/*
 * @brief 默认构造函数
 */
  AgwArrayDDCustInfo();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 2 + 16 + 12 + 2 + 16 + 12 + 2 + 16;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDCustInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融客户查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDCustInfo : public AgwMsgExtQueryResultBase
{
public:
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDCustInfo> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDCustInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDCustInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDCustInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融持仓查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDShare : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
Uint8_def sort_type; 		///< 排序方式
Uint8_def flag; 		///< 扩展标志
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDShare : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
SecurityType_def security_type; 		///< 证券类别
Currency_def currency; 		///< 货币种类
Qty_def last_stk_bal; 		///< 日初股份余额
Qty_def stk_avl; 		///< 股份可用
Qty_def stk_sell; 		///< 股份卖出冻结
Price_def last_price; 		///< 最近价格
Qty_def leaves_qty; 		///< 市值持仓
Amt_def market_value; 		///< 市值
Price_def cost_price; 		///< 成本价格
Amt_def profit_cost; 		///< 盈亏成本
Price_def profit_price; 		///< 盈亏成本价格
Amt_def pro_income; 		///< 累计浮动盈亏
Amt_def date_profit; 		///< 本日盈亏
Price_def dilute_cost_price; 		///< 摊薄成本价
Price_def keep_cost_price; 		///< 保本价
Price_def dilute_keep_cost_price; 		///< 摊薄保本价
Amt_def dilute_unclear_profit; 		///< 摊薄浮动盈亏
Qty_def trade_unit; 		///< 交易单位
Qty_def real_buy_qty; 		///< 当日买入成交数量
Qty_def real_sell_qty; 		///< 当日卖出成交数量
Amt_def real_buy_balance; 		///< 当日买入成交金额
Amt_def real_sell_balance; 		///< 当日卖出成交金额
Qty_def total_sell_qty; 		///< 累计卖出数量
Qty_def total_buy_qty; 		///< 累计买入数量
Amt_def total_sell_amt; 		///< 累计卖出金额
Amt_def total_buy_amt; 		///< 累计买入金额
/*
 * @brief 默认构造函数
 */
  AgwArrayDDShare();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 40 + 2 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融持仓查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDShare : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDShare> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融查询资格证券
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDMarginStk : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDMarginStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDMarginStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDMarginStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDMarginStk : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
CreditRatio_def pledge_rate; 		///< 折算率
CreditRatio_def margin_rate_fund; 		///< 融资保证金比例
CreditRatio_def margin_rate_stk; 		///< 融券保证金比例
Char_def stk_property; 		///< 证券属性
Char_def trade_status; 		///< 交易状态
Char_def cust_type; 		///< 客户类型
/*
 * @brief 默认构造函数
 */
  AgwArrayDDMarginStk();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 8 + 8 + 1 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDMarginStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融查询资格证券结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDMarginStk : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDMarginStk> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDMarginStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDMarginStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDMarginStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融查询当日负债变动流水
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDDebtsFlow : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
Sno_def details_sno; 		///< 流水号（不填查所有）
Date8_def debts_sys_date; 		///< 合约日期（填0查所有）
Sno_def debts_sno; 		///< 合约编号（不填查所有）
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDDebtsFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDDebtsFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDDebtsFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDDebtsFlow : public mp::RootPacket
{
public:
Sno_def details_sno; 		///< 流水号
Date8_def debts_sys_date; 		///< 合约日期
Sno_def debts_sno; 		///< 合约编号
ClOrdNO_def cl_ord_no; 		///< 订单编号
ReportIndex_def report_index; 		///< 回报序号
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
Date8_def sys_date; 		///< 交易日期
Date8_def oper_date; 		///< 发生日期
Time9_def oper_time; 		///< 发生时间
Qty_def stk_effect; 		///< 发生合约数量
Qty_def stk_bal; 		///< 后余数量
Amt_def clear_amt_effect; 		///< 发生合约金额
Amt_def clear_amt; 		///< 后余金额
/*
 * @brief 默认构造函数
 */
  AgwArrayDDDebtsFlow();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 11 + 8 + 11 + 8 + 8 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDDebtsFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融当日负债流水查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDDebtsFlow : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDDebtsFlow> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDDebtsFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDDebtsFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDDebtsFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融信用资产查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDCreditAsset : public AgwMsgExtQueryBase
{
public:
Uint8_def ext_flag; 		///< 扩展标识
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDCreditAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDCreditAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDCreditAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDCreditAsset : public mp::RootPacket
{
public:
Amt_def margin_avl; 		///< 可用保证金
CreditRatio_def keep_rate; 		///< 担保比例
CreditRatio_def keep_rate1; 		///< 预计维持担保比例1，考虑担保物划入划出
CreditRatio_def keep_rate2; 		///< 预计维持担保比例2，考虑担保物划出，不考虑划入
Amt_def fund_bal; 		///< 账户余额
Amt_def fund_avl; 		///< 可用资金
Amt_def clear_amt; 		///< 清算资金
Amt_def mkt_value; 		///< 证券市值_计算担保比例使用
Amt_def pledge_mkt_value; 		///< 担保证券市值
Amt_def mar_stk_val; 		///< 担保证券折算市值_计算可用保证金使用(已扣融资)
Amt_def credit_transform_asset; 		///< 担保物返回未成交_预估资产
Amt_def fund_debts; 		///< 融资负债
Amt_def stk_debts; 		///< 融券负债
Amt_def fund_debts2; 		///< 融资负债2
Amt_def stk_debts2; 		///< 融券负债2
Qty_def ummatch_return_qty; 		///< 还券委托未成
Amt_def alarm_rate_add_amt; 		///< 主动平仓资金
Amt_def out_asset; 		///< 可提资产标准
Amt_def max_draw; 		///< 可取资金
Amt_def f_fee; 		///< 尚欠利息_融资
Amt_def d_fee; 		///< 尚欠利息_融券
Amt_def total_fee; 		///< 预计利息/费用
Amt_def all_debts; 		///< 总负债
Amt_def all_asset; 		///< 总资产
Amt_def net_asset; 		///< 净资产
Amt_def mar_fund_margin; 		///< 占用保证金_融资
Amt_def mar_stk_margin; 		///< 占用保证金_融券
Amt_def fund_pro_income; 		///< 融资盈亏
Amt_def stk_pro_income; 		///< 融券盈亏
Amt_def margin_sell_amt; 		///< 融券剩余资金
Amt_def stk_clear_amt; 		///< 融券金额_负债
Amt_def fund_mkt_value; 		///< 融资当前市值
Amt_def stk_mkt_value; 		///< 融券当前市值
Amt_def contract_end_date; 		///< 合同到期日
Amt_def fund_limit_amt; 		///< 融资授信额度
Amt_def stk_limit_amt; 		///< 融券授信额度
Amt_def fund_limit_used; 		///< 融资已用额度
Amt_def stk_limit_used; 		///< 融券已用额度
Amt_def limit_amt; 		///< 合同额度
/*
 * @brief 默认构造函数
 */
  AgwArrayDDCreditAsset();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDCreditAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融信用资产查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDCreditAsset : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDCreditAsset> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDCreditAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDCreditAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDCreditAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融查询客户合约展期流水
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDDebtsDelayFlow : public AgwMsgExtQueryBase
{
public:
Date8_def apply_date; 		///< 展期申请日期
Sno_def apply_sno; 		///< 展期申请编号
Date8_def debts_date; 		///< 合约日期
Sno_def debts_sno; 		///< 合约编号
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDDebtsDelayFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDDebtsDelayFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDDebtsDelayFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDDebtsDelayFlow : public mp::RootPacket
{
public:
CustName_def cust_name; 		///< 客户名称
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
Date8_def debts_date; 		///< 合约日期
Sno_def debts_sno; 		///< 合约编号
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Time9_def apply_time; 		///< 发生时间
Date8_def end_date; 		///< 到期日期
Date8_def date; 		///< 日期，取到期日期
RollOverNumber_def delay_times; 		///< 展期次数
FundDigestName_def digest; 		///< 摘要
/*
 * @brief 默认构造函数
 */
  AgwArrayDDDebtsDelayFlow();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 64 + 8 + 11 + 8 + 11 + 8 + 40 + 8 + 8 + 8 + 4 + 128;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDDebtsDelayFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融查询客户合约展期流水结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDDebtsDelayFlow : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDDebtsDelayFlow> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDDebtsDelayFlow();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDDebtsDelayFlow& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDDebtsDelayFlow;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融查询当日融券负债汇总
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDMarginSell : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDMarginSell();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDMarginSell& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDMarginSell;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDMarginSell : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Qty_def debts_qty; 		///< 负债数量
Qty_def today_margin_sell_qty; 		///< 当日融券成交数量
Qty_def stkrepay_order_qty; 		///< 还券委托数量
Qty_def stkrepay_match_qty; 		///< 还券成交数量
Qty_def remain_sec_order_qty; 		///< 余券委托数量
Qty_def stk_remain; 		///< 余券数量
/*
 * @brief 默认构造函数
 */
  AgwArrayDDMarginSell();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDMarginSell& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融查询当日融券负债汇总结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDMarginSell : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDMarginSell> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDMarginSell();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDMarginSell& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDMarginSell;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融查询集中度控制比例
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDConcentration : public AgwMsgExtQueryBase
{
public:
Char_def account_type; 		///< 账户类型
Char_def concentration_type; 		///< 集中度类型
Char_def plate_type; 		///< 板块类型
Char_def special_flag; 		///< 特殊标志
SecurityType_def security_type; 		///< 证券类别
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDConcentration : public mp::RootPacket
{
public:
Qty_def seq_no; 		///< 序号
AccountID_def account_id; 		///< 账户ID
Char_def account_type; 		///< 账户类型
Char_def concentration_type; 		///< 集中度类型
Char_def plate_type; 		///< 板块类型
Char_def special_flag; 		///< 特殊标志
SecurityType_def security_type; 		///< 证券类别
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
CreditRatio_def keep_rate_begin; 		///< 维保控制下限
CreditRatio_def keep_rate_end; 		///< 维保控制上限
CreditRatio_def concentration_rate; 		///< 证券集中度
Date8_def end_date; 		///< 到期日期
String_def control_busi; 		///< 业务控制范围
/*
 * @brief 默认构造函数
 */
  AgwArrayDDConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融查询集中度控制比例结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDConcentration : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDConcentration> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融查询信用负债
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDCreditDebts : public AgwMsgExtQueryBase
{
public:
CreditDirect_def credit_direct; 		///< 信用方向
CreditDebtsQueryFlag_def query_flag; 		///< 查询方向
Date8_def sys_date; 		///< 系统时间,填0查所有
Sno_def debts_sno; 		///< 合约编号,填空查所有
Currency_def currency; 		///< 币种
MarketID_def market_id; 		///< 市场ID,填空0所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
Date8_def begin_date; 		///< 开始日期
Date8_def end_date; 		///< 结束日期
ClOrdNO_def cl_ord_no; 		///< 委托号
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDCreditDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDCreditDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDCreditDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDCreditDebts : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 负债发生系统日期
Sno_def debts_sno; 		///< 负债流水号
Date8_def end_date; 		///< 负债截止日期
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券简称
Date8_def order_date; 		///< 委托日期
ClOrdNO_def cl_ord_no; 		///< 委托序号
AccountID_def account_id; 		///< 证券账户ID
MarketID_def market_id; 		///< 市场ID
Qty_def match_qty; 		///< 成交数量
Amt_def clear_amt; 		///< 清算金额
ContractID_def contract_id; 		///< 合同编号
Date8_def last_date; 		///< 起息日期
Char_def debts_kind; 		///< 合约类型
Qty_def order_qty; 		///< 委托数量
Price_def order_price; 		///< 委托价格
Price_def match_price; 		///< 成交价格
Amt_def match_fee; 		///< 成交手续费
Amt_def credit_repay; 		///< 已偿还合约金额
Qty_def stk_repay; 		///< 已偿还数量
Amt_def init_clear_amt; 		///< 期初合约金额
Amt_def init_trade_fee; 		///< 期初合约交易费用
Amt_def init_fee; 		///< 期初合约利息
Amt_def init_puni_fee; 		///< 期初罚息金额
Qty_def init_match_qty; 		///< 期初合约数量
Amt_def debts_amt; 		///< 实时合约金额
Amt_def trade_fee; 		///< 实时合约交易费用
Amt_def fee; 		///< 实时合约利息
Amt_def puni_fee; 		///< 实时罚息金额
Qty_def debts_qty; 		///< 实时合约数量
CreditRatio_def margin_rate; 		///< 保证金比例
RollOverNumber_def delay_count; 		///< 展期次数
Date8_def spec_date; 		///< 头寸日期
Sno_def spec_sno; 		///< 头寸编号
Char_def fund_debts_flag; 		///< 欠款标志 0:有欠款 1:无欠款
Char_def stk_debts_flag; 		///< 欠券标志 0:有欠券 1:无欠券
/*
 * @brief 默认构造函数
 */
  AgwArrayDDCreditDebts();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 11 + 8 + 8 + 40 + 8 + 8 + 12 + 2 + 8 + 8 + 32 + 8 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 4 + 8 + 11 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDCreditDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融查询信用负债结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDCreditDebts : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDCreditDebts> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDCreditDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDCreditDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDCreditDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融查询客户可展期合约
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDDelayDebts : public AgwMsgExtQueryBase
{
public:
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDDelayDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDDelayDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDDelayDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayDDDelayDebts : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 系统日期
Sno_def sno; 		///< 合约编号
CreditDirect_def credit_direct; 		///< 信用方向
Date8_def order_date; 		///< 委托日期
ClOrdNO_def order_sno; 		///< 委托序号
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
AccountID_def account_id; 		///< 证券账户ID
LifeStatus_def life_status; 		///< 合约状态
MarketID_def market_id; 		///< 市场ID
Date8_def end_date; 		///< 负债截止日期
Qty_def match_qty; 		///< 成交数量
Amt_def clear_amt; 		///< 清算金额
Amt_def leave_amt; 		///< 剩余金额
Qty_def leave_qty; 		///< 剩余数量
Amt_def all_fee; 		///< 息费总和
ContractID_def contract_id; 		///< 合同编号
Date8_def last_date; 		///< 最后一次计算息费日期
Char_def debts_kind; 		///< 合约类型
Qty_def order_qty; 		///< 委托数量
Amt_def order_amt; 		///< 委托金额
Amt_def debts_amt; 		///< 总负债金额
Amt_def match_amt; 		///< 成交金额
Amt_def credit_repay; 		///< 还款金额
Qty_def stk_repay; 		///< 还券数量
Amt_def fee; 		///< 融资费用/融券费用
Amt_def all_repay_fee; 		///< 已归还利息
Date8_def spec_date; 		///< 头寸组
Sno_def spec_sno; 		///< 头寸组
Qty_def s1; 		///< 委托数量
Qty_def s2; 		///< 合约开仓数量
Amt_def s3; 		///< 合约开仓金额
Amt_def s4; 		///< 合约开仓费用
Qty_def s5; 		///< 期初合约数量
Amt_def s6; 		///< 期初合约金额
Amt_def s7; 		///< 期初合约费用
Amt_def s11; 		///< 未还合约金额
Qty_def s12; 		///< 未还合约数量
Amt_def s13; 		///< 未还合约费用
/*
 * @brief 默认构造函数
 */
  AgwArrayDDDelayDebts();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 11 + 1 + 8 + 8 + 8 + 40 + 12 + 1 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 32 + 8 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 11 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayDDDelayDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点两融查询客户可展期合约结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDDelayDebts : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwArrayDDDelayDebts> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDDelayDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDDelayDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDDelayDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用合约展期申请结果查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsDelayApplyResult : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
ApplyStatus_def apply_status; 		///< 申请状态
CreditDirect_def credit_direct; 		///< 信用方向
Date8_def begin_date; 		///< 起始日期
Date8_def end_date; 		///< 结束日期
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsDelayApplyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsDelayApplyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsDelayApplyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用合约展期申请结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditDebtsDelayApplyResultArray : public mp::RootPacket
{
public:
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
Date8_def sys_date; 		///< 系统日期
Sno_def sno; 		///< 合约编号
CreditDirect_def credit_direct; 		///< 信用方向
ApplyStatus_def apply_status; 		///< 申请状态
Remark_def chk_remark; 		///< 复核意见
CfmAns_def cfm_ans; 		///< 备注
Currency_def currency; 		///< 币种
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Date8_def order_date; 		///< 委托日期
ClOrdNO_def order_sno; 		///< 委托序号
AccountID_def account_id; 		///< 账户ID
LifeStatus_def life_status; 		///< 合约状态
MarketID_def market_id; 		///< 市场ID
Qty_def match_qty; 		///< 成交数量
Amt_def clear_amt; 		///< 成交金额
Amt_def leave_amt; 		///< 剩余金额
Amt_def leave_value; 		///< 剩余市值
/*
 * @brief 默认构造函数
 */
  AgwCreditDebtsDelayApplyResultArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 11 + 8 + 11 + 1 + 1 + 32 + 255 + 4 + 8 + 40 + 8 + 8 + 12 + 1 + 2 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditDebtsDelayApplyResultArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用合约展期申请结果查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsDelayApplyResult : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 客户一级锁券记录总数
std::vector<AgwCreditDebtsDelayApplyResultArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsDelayApplyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsDelayApplyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsDelayApplyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用预到期合约查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditPreMaturityDebts : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
CreditDirect_def credit_direct; 		///< 信用方向
Date8_def begin_date; 		///< 起始日期
Date8_def end_date; 		///< 结束日期
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditPreMaturityDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditPreMaturityDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditPreMaturityDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用预到期合约查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditPreMaturityDebtsArray : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 系统日期
Sno_def sno; 		///< 合约编号
CreditDirect_def credit_direct; 		///< 信用方向
Currency_def currency; 		///< 币种
Date8_def order_date; 		///< 委托日期
ClOrdNO_def order_sno; 		///< 委托序号
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
AccountID_def account_id; 		///< 账户ID
LifeStatus_def life_status; 		///< 合约状态
MarketID_def market_id; 		///< 市场ID
Date8_def end_date; 		///< 负债截止日期
Qty_def match_qty; 		///< 成交数量
Amt_def clear_amt; 		///< 成交金额
Amt_def leave_amt; 		///< 剩余金额
Qty_def leave_qty; 		///< 剩余数量
Amt_def all_fee; 		///< 息费总和
ApplyStatus_def apply_status; 		///< 申请状态
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
Date8_def append_date; 		///< 申请时的合约到期日
Date8_def oper_date; 		///< 操作日期
Time9_def oper_time; 		///< 操作时间
Remark_def chk_remark; 		///< 复核意见
Amt_def leave_mkt_value; 		///< 剩余市值
/*
 * @brief 默认构造函数
 */
  AgwCreditPreMaturityDebtsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 11 + 1 + 4 + 8 + 8 + 8 + 40 + 12 + 1 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 11 + 8 + 8 + 8 + 32 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditPreMaturityDebtsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用预到期合约查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditPreMaturityDebts : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 客户一级锁券记录总数
std::vector<AgwCreditPreMaturityDebtsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditPreMaturityDebts();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditPreMaturityDebts& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditPreMaturityDebts;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用预到期合约(按到期天数)查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditPreMaturityDebtsByDays : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
CreditDirect_def credit_direct; 		///< 信用方向
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
Uint16_def expiry_days; 		///< 到期天数
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditPreMaturityDebtsByDays();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditPreMaturityDebtsByDays& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditPreMaturityDebtsByDays;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用预到期合约(按到期天数)查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditPreMaturityDebtsByDays : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 客户一级锁券记录总数
std::vector<AgwCreditPreMaturityDebtsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditPreMaturityDebtsByDays();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditPreMaturityDebtsByDays& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditPreMaturityDebtsByDays;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditReserveStk : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Date8_def begin_date; 		///< 起始日期
Date8_def end_date; 		///< 结束日期
ReserveStatus_def status; 		///< 状态
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditReserveStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditReserveStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditReserveStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditReserveStkArray : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 交易日期
Sno_def sno; 		///< 预约流水号
AccountID_def account_id; 		///< 账户ID
CustName_def cust_name; 		///< 客户姓名
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Qty_def reserve_qty; 		///< 预约数量
Qty_def min_qty; 		///< 最低数量
CreditRatio_def min_rate; 		///< 最低比例
CreditRatio_def used_fee_rate; 		///< 期望利率
CreditRatio_def unused_fee_rate; 		///< 占用费率
CreditRatio_def chk_used_fee_rate; 		///< 已使用的费率(审批)
CreditRatio_def chk_unused_fee_rate; 		///< 锁券占用费率(审批)
Date8_def begin_date; 		///< 开始日期
Days_def use_days; 		///< 使用期限
Date8_def end_date; 		///< 结束日期
ReserveStatus_def status; 		///< 状态
Remark_def remark; 		///< 备注信息
Date8_def cancel_date; 		///< 撤销日期
CheckStatus_def chk_status; 		///< 审批状态
CheckMod_def chk_mode; 		///< 审批选项
Remark_def chk_remark; 		///< 审批备注
/*
 * @brief 默认构造函数
 */
  AgwCreditReserveStkArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 11 + 12 + 64 + 8 + 2 + 40 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 4 + 8 + 1 + 32 + 8 + 1 + 1 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditReserveStkArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 融券预约查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditReserveStk : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 合约持仓记录总数
std::vector<AgwCreditReserveStkArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditReserveStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditReserveStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditReserveStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约未上市可预约证券查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditReserveStkTrd : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditReserveStkTrd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditReserveStkTrd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditReserveStkTrd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融券预约未上市可预约证券结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditReserveStkTrdArray : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Date8_def begin_date; 		///< 有效开始日期
Date8_def end_date; 		///< 有效截止日期
TibFlag_def kcb_flag; 		///< 是否为科创板
/*
 * @brief 默认构造函数
 */
  AgwCreditReserveStkTrdArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 40 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditReserveStkTrdArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 融券预约未上市可预约证券查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditReserveStkTrd : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 合约持仓记录总数
std::vector<AgwCreditReserveStkTrdArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditReserveStkTrd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditReserveStkTrd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditReserveStkTrd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用当日成交汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditTradeCollect : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
CollectType_def collect_type; 		///< 汇总方式，填0表示依照旧的方式汇总，填1表示依照日期、客户号、买卖方向汇总
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditTradeCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditTradeCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditTradeCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用当日成交汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditTradeCollectArray : public mp::RootPacket
{
public:
CreditDigest_def credit_digest_id; 		///< 信用交易摘要
CreditDigestName_def bs_flag_name; 		///< 信用交易摘要名称
Currency_def currency; 		///< 币种
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
BSFlag2_def bs_flag2; 		///< 集中交易买卖类别
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Price_def last_px; 		///< 成交价格
Qty_def last_qty; 		///< 成交数量
Amt_def total_value_traded; 		///< 成交金额,精确到分
MatchType_def match_type; 		///< 成交类型
Qty_def cancel_qty; 		///< 撤单数量
BankCode_def bank_code; 		///< 外部银行
BankID_def bank_id; 		///< 外部账户
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
/*
 * @brief 默认构造函数
 */
  AgwCreditTradeCollectArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 40 + 4 + 1 + 1 + 2 + 2 + 8 + 40 + 8 + 8 + 8 + 1 + 8 + 5 + 32 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditTradeCollectArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 信用当日成交汇总查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditTradeCollect : public AgwMsgExtQueryResultBase
{
public:
Date8_def oper_date; 		///< 交易日期
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 信用当日成交汇总记录总数
std::vector<AgwCreditTradeCollectArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditTradeCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditTradeCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditTradeCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券信用数据（可用保证金）查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCustMarginAvl : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
CreditDirect_def credit_direct; 		///< 信用方向
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCustMarginAvl();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCustMarginAvl& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCustMarginAvl;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券信用数据（可用保证金）查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustMarginAvl : public AgwMsgExtQueryResultBase
{
public:
CreditDirect_def credit_direct; 		///< 信用方向
Amt_def margin_avl; 		///< 保证金可用余额
Amt_def credit_bal; 		///< 融资/融券授信额度
Amt_def credit_avl; 		///< 融资/融券可用的额度
CreditRatio_def margin_rate; 		///< 融资/融券保证金比例
Amt_def debts_bal; 		///< 应付融资款/应付融券市值
Amt_def max_amt; 		///< 最大融资/融券金额
Amt_def fund_avl; 		///< 资金可用数
Amt_def credit_fund_avl; 		///< 融券卖出资金
Amt_def g_dis_market_avl; 		///< 担保证券的折算价值
Amt_def mar_stk_val; 		///< ∑(充抵保证金的证券市值(不含融资买入数量)*折算率)
Amt_def total_dis_f_mk_val; 		///< ∑融资的市值*折算率
Amt_def mar_fund_profit; 		///< ∑融资盈亏*折算率
Amt_def mar_stk_profit; 		///< ∑融券盈亏*折算率
Amt_def total_rate_f_debts_bal; 		///< ∑融资金额*融资保证金比例
Amt_def mar_fund_margin; 		///< 已占用融资保证金
Amt_def mar_fund_margin_frz; 		///< 委托冻结融资保证金
Amt_def total_rate_d_sale_bal; 		///< ∑融券市值*融券保证金比例
Amt_def mar_stk_margin; 		///< 已占用融券保证金
Amt_def mar_stk_margin_frz; 		///< 委托冻结融券保证金
Amt_def total_fee; 		///< 利息和费用总和
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Amt_def margin_fund_bal; 		///< 资金余额（保证金公式部分）
Amt_def margin_credit_bal; 		///< 融券卖出冻结资金（保证金公式部分）
Amt_def fund_margin_avl; 		///< 资金账号级保证金可用金额(不考虑安全垫影响)
Amt_def fund_margin_avl_sub; 		///< 资金账号级保证金可用金额(考虑安全垫影响)
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustMarginAvl();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustMarginAvl& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustMarginAvl;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新外围委托查询(宏汇专用)查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHHOrder : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,同一个客户当天唯一,填0查所有
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHHOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHHOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHHOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 委托查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwOrderHHArray : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 证券账户
Side_def side; 		///< 买卖方向
OrdType_def ord_type; 		///< 订单类别
OrdStatus_def ord_status; 		///< 原订单当前状态
TimeStamp_def transact_time; 		///< 客户委托时间
Price_def order_price; 		///< 委托价格
Price_def exec_price; 		///< 平均成交价格
Qty_def order_qty; 		///< 委托数量
Qty_def leaves_qty; 		///< 未成交部分的数量
Qty_def cum_qty; 		///< 累计成交数量
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
OrderID_def order_id; 		///< 交易所订单编号, 取值为数字
ClOrdID_def cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClOrdNO_def orig_cl_ord_no; 		///< 原委托的客户订单编号
Amt_def frozen_trade_value; 		///< 冻结交易金额
Amt_def frozen_fee; 		///< 冻结费用
RejectReasonCode_def reject_reason_code; 		///< 拒绝代码
OrdRejReason_def ord_rej_reason; 		///< 拒绝原因描述信息
Currency_def money_type; 		///< 货币种类
CancelFlag_def cancel_flag; 		///< 撤单标志
Qty_def cancel_qty; 		///< 撤单数量
BSFlag2_def bs_flag2; 		///< 集中交易买卖类别
PBUID_def pbu_id; 		///< 投资者所属交易单元,交易席位
Amt_def total_value_traded; 		///< 成交金额,精确到分
UserInfo_def user_info; 		///< 用户私有信息
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
/*
 * @brief 默认构造函数
 */
  AgwOrderHHArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwOrderHHArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 新外围委托查询(宏汇专用)查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHHOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwOrderHHArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHHOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHHOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHHOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日委托汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryOrderCollectAll : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
Side_def side; 		///< 买卖方向
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryOrderCollectAll();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryOrderCollectAll& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryOrderCollectAll;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日委托汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwOrderCollectAllArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 币种
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
BSFlag2_def bs_flag2; 		///< 集中交易买卖类别
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Price_def order_price; 		///< 委托价格
Qty_def order_qty; 		///< 委托数量
Amt_def order_frz_amt; 		///< 委托金额
Qty_def last_qty; 		///< 成交数量
Qty_def cancel_qty; 		///< 撤单数量
Amt_def total_value_traded; 		///< 成交金额,精确到分
/*
 * @brief 默认构造函数
 */
  AgwOrderCollectAllArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 1 + 1 + 2 + 2 + 8 + 40 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwOrderCollectAllArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 当日委托汇总查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultOrderCollectAll : public AgwMsgExtQueryResultBase
{
public:
CustName_def cust_name; 		///< 客户姓名
Date8_def order_date; 		///< 委托日期
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托汇总记录总数
std::vector<AgwOrderCollectAllArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultOrderCollectAll();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultOrderCollectAll& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultOrderCollectAll;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 板块集中度查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryPlateConcentration : public AgwMsgExtQueryBase
{
public:
PlateID_def plate_id; 		///< 板块代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryPlateConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryPlateConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryPlateConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayPlateConcentration : public mp::RootPacket
{
public:
PlateID_def plate_id; 		///< 板块代码
PlateSymbol_def plate_symbol; 		///< 板块简称
CreditRatio_def keep_rate_begin; 		///< 维保控制起始值
CreditRatio_def keep_rate_end; 		///< 维保控制结束值
Amt_def debt_begin; 		///< 总负债起始值
Amt_def debt_end; 		///< 总负债结束值
CreditRatio_def ctrl_rate; 		///< 控制比例
/*
 * @brief 默认构造函数
 */
  AgwArrayPlateConcentration();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 40 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayPlateConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 板块集中度查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultPlateConcentration : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 板块集中度记录总数
std::vector<AgwArrayPlateConcentration> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultPlateConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultPlateConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultPlateConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 板块所属证券查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryPlateStkCode : public AgwMsgExtQueryBase
{
public:
PlateID_def plate_id; 		///< 板块代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryPlateStkCode();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryPlateStkCode& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryPlateStkCode;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayPlateStkCode : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
/*
 * @brief 默认构造函数
 */
  AgwArrayPlateStkCode();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayPlateStkCode& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 板块所属证券查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultPlateStkCode : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 板块集中度记录总数
std::vector<AgwArrayPlateStkCode> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultPlateStkCode();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultPlateStkCode& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultPlateStkCode;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自行平仓申请记录查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySelfCloseApply : public AgwMsgExtQueryBase
{
public:
Date8_def begin_date; 		///< 有效开始日期
Date8_def end_date; 		///< 有效结束日期
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySelfCloseApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySelfCloseApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySelfCloseApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自行平仓申请记录查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwSelfCloseApplyArray : public mp::RootPacket
{
public:
Date8_def apply_date; 		///< 申请日期
CloseRightStatus_def status; 		///< 权限状态
CheckStatus_def chk_status; 		///< 审批状态
Date8_def begin_date; 		///< 有效开始日期
Date8_def end_date; 		///< 有效结束日期
ApplyKind_def apply_kind; 		///< 自行申请标识
Date8_def chk_date; 		///< 审批日期
Time9_def chk_time; 		///< 审批时间
Remark_def chk_remark; 		///< 审批拒绝原因
/*
 * @brief 默认构造函数
 */
  AgwSelfCloseApplyArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 1 + 1 + 8 + 8 + 1 + 8 + 8 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSelfCloseApplyArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 自行平仓申请记录查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSelfCloseApply : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwSelfCloseApplyArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSelfCloseApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSelfCloseApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSelfCloseApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自行平仓申请条件查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySelfCloseApplyRight : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySelfCloseApplyRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySelfCloseApplyRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySelfCloseApplyRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 自行平仓申请条件查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSelfCloseApplyRight : public AgwMsgExtQueryResultBase
{
public:
CreditCtrl_def ok_flag; 		///< 允许申请标识
String_def remark; 		///< 备注（不允许申请的原因）
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSelfCloseApplyRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSelfCloseApplyRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSelfCloseApplyRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户自行平仓审查查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySelfCloseChk : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySelfCloseChk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySelfCloseChk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySelfCloseChk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户自行平仓审查查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSelfCloseChk : public AgwMsgExtQueryResultBase
{
public:
InvalidStatus_def status; 		///< 权限状态
QryChkStatus_def chk_status; 		///< 审批状态
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSelfCloseChk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSelfCloseChk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSelfCloseChk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 卖券还款类型查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySellRepayType : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
Qty_def qty; 		///< 数量
BusinessType_def business_type; 		///< 业务类型
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySellRepayType();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySellRepayType& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySellRepayType;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 卖券还款类型查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSellRepayType : public AgwMsgExtQueryResultBase
{
public:
MarketID_def market_id; 		///< 交易市场
SellRepaySource_def order_type; 		///< 卖券还款类型
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSellRepayType();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSellRepayType& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSellRepayType;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股票集中度查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryStkConcentration : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryStkConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryStkConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryStkConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwArrayStkConcentration : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
CreditRatio_def keep_rate_begin; 		///< 维保控制起始值
CreditRatio_def keep_rate_end; 		///< 维保控制结束值
Amt_def debt_begin; 		///< 总负债起始值
Amt_def debt_end; 		///< 总负债结束值
CreditRatio_def ctrl_rate; 		///< 控制比例
/*
 * @brief 默认构造函数
 */
  AgwArrayStkConcentration();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwArrayStkConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 股票集中度查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultStkConcentration : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 股票集中度记录总数
std::vector<AgwArrayStkConcentration> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultStkConcentration();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultStkConcentration& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultStkConcentration;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日成交汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTradeCollectAll : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTradeCollectAll();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTradeCollectAll& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTradeCollectAll;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日成交汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTradeCollectAllArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 币种
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
BSFlag2_def bs_flag2; 		///< 集中交易买卖类别
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Price_def last_px; 		///< 成交价格
Qty_def last_qty; 		///< 成交数量
Amt_def total_value_traded; 		///< 成交金额,精确到分
MatchType_def match_type; 		///< 成交类型
BankCode_def bank_code; 		///< 外部银行
BankID_def bank_id; 		///< 外部账户
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
Char_def grade; 		///< 层级信息（0-基础层，1-创新层，2-北交所）
/*
 * @brief 默认构造函数
 */
  AgwTradeCollectAllArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 1 + 1 + 2 + 2 + 8 + 40 + 8 + 8 + 8 + 1 + 5 + 32 + 64 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTradeCollectAllArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 当日成交汇总查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultTradeCollectAll : public AgwMsgExtQueryResultBase
{
public:
CustName_def cust_name; 		///< 客户姓名
Date8_def oper_date; 		///< 交易日期
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 当日成交汇总记录总数
std::vector<AgwTradeCollectAllArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultTradeCollectAll();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultTradeCollectAll& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultTradeCollectAll;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日资金流水查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditAssetDetails : public AgwMsgExtQueryBase
{
public:
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditAssetDetails();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditAssetDetails& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditAssetDetails;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日资金流水查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditAssetDetailsArray : public mp::RootPacket
{
public:
Date8_def biz_date; 		///< 系统日期
Sno_def details_sno; 		///< 资金流水号
Date8_def oper_date; 		///< 发生日期
Time9_def oper_time; 		///< 发生时间
DigestID_def digest_id; 		///< 业务摘要ID
FundDigestName_def digest_name; 		///< 业务摘要名称
Currency_def currency; 		///< 货币类型
AccountID_def account_id; 		///< 账户ID
MarketID_def market_id; 		///< 市场ID
Amt_def fund_effect; 		///< 资金发生数
Amt_def fund_bal; 		///< 资金本次余额
Qty_def stk_bal; 		///< 股份本次余额
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券简称
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
Price_def last_px; 		///< 成交价格
Qty_def last_qty; 		///< 成交数量
Amt_def total_value_traded; 		///< 成交金额
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
SeqNum_def agw_seq_id; 		///< 业务流水号,agw网关全局唯一
InstructionID_def instruction_id; 		///< 指令ID
ExecID_def match_code; 		///< 成交编号
BSFlag2_def bs_flag2; 		///< 集中交易买卖方向
RtgsContractID_def rtgs_contract_id; 		///< rtgs合同号
/*
 * @brief 默认构造函数
 */
  AgwCreditAssetDetailsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 11 + 8 + 8 + 4 + 128 + 4 + 12 + 2 + 8 + 8 + 8 + 8 + 40 + 1 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 32 + 2 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditAssetDetailsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
/*
 * @brief agw_seq_id接口
 */
    virtual SeqNum_def GetAgwSeqID() const { return agw_seq_id; }
};
/*
 * @brief 当日资金流水查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditAssetDetails : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwCreditAssetDetailsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditAssetDetails();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditAssetDetails& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditAssetDetails;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日负债流水查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditDebtsDetails : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 币种
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditDebtsDetails();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditDebtsDetails& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditDebtsDetails;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日负债流水查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditDebtsDetailsArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 币种
CreditDebtsType_def debt_type; 		///< 负债类型
DebtsEffectDirect_def effect_direct; 		///< 发生方向
RepayWay_def repay_way; 		///< 偿还方式
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Sno_def details_sno; 		///< 流水号
Date8_def relate_sys_date; 		///< 负债发生日期
Sno_def relate_sno; 		///< 委托号
MarketID_def market_id; 		///< 交易市场
Amt_def fund_effect; 		///< 负债资金发生数
Qty_def stk_effect; 		///< 负债股份发生数
/*
 * @brief 默认构造函数
 */
  AgwCreditDebtsDetailsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 1 + 1 + 1 + 8 + 40 + 11 + 8 + 11 + 2 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditDebtsDetailsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 当日负债流水查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditDebtsDetails : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwCreditDebtsDetailsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditDebtsDetails();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditDebtsDetails& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditDebtsDetails;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户锁券资质查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditLockRight : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditLockRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditLockRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditLockRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户锁券资质查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditLockRight : public AgwMsgExtQueryResultBase
{
public:
CustName_def cust_name; 		///< 客户姓名
RightStatus_def right_status; 		///< 资质状态
Date8_def apply_date; 		///< 申请日期
Remark_def remark; 		///< 备注
Sno_def link_sno; 		///< 关联序号
Remark_def link_remark; 		///< 关联注释
CheckStatus_def chk_status; 		///< 审批状态
OperID_def chk_oper; 		///< 审批柜员
Date8_def chk_date; 		///< 审批日期
Remark_def chk_remark; 		///< 复核备注
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditLockRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditLockRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditLockRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 两融最大可委托数查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditMaxOrderQty : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditMaxOrderQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditMaxOrderQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditMaxOrderQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 两融最大可委托数查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditMaxOrderQtyResult : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 账户ID
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
Qty_def max_order_qty; 		///< 最大可委托数
Qty_def theoretical_order_qty; 		///< 理论可委托数
Qty_def max_pay_qty; 		///< 买券还券应补数量
Amt_def max_pay_amt; 		///< 应偿还资金负债
Qty_def sep_rem_qty; 		///< 专户余券
Amt_def fund_credit_avl; 		///< 买券还券可用资金
Qty_def stk_debts_avl; 		///< 融券负债可用额度
Qty_def sg_max_stk_qty; 		///< 个股申购上限
String_def zero_reason; 		///< 可委托数为0原因描述
Bool_def limit_flag; 		///< 减持受限标识
Qty_def jj_max_limit_qty; 		///< 减持受限名单内的竞价卖出可用额度
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditMaxOrderQtyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditMaxOrderQtyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditMaxOrderQtyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 锁券合同查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditStkLock : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
LockStatusArray_def status; 		///< 锁券状态组合
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditStkLock();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditStkLock& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditStkLock;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 锁券合同查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwCreditStkLockArray : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 系统日期
AccountID_def account_id; 		///< 证券账户ID
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
Qty_def reserve_qty; 		///< 待分配数量
Qty_def stk_bal; 		///< 股份锁定总余额
Qty_def stk_avl; 		///< 股份可用数
Qty_def stk_used_qty; 		///< 已使用数量
Qty_def stk_sale; 		///< 当日股份卖出冻结
Qty_def stk_sale_frz; 		///< 当日卖出占用
Qty_def stk_repay; 		///< 日间偿还成交
LockType_def lock_type; 		///< 锁券类型
CreditRatio_def used_fee_rate; 		///< 锁券已使用费率
Amt_def used_fee; 		///< 锁券已使用费
Amt_def over_used_fee; 		///< 逾期锁券使用费
Amt_def puni_used_fee; 		///< 逾期锁券使用费罚息
CreditRatio_def unused_fee_rate; 		///< 锁券占用费率
Amt_def unused_fee; 		///< 锁券占用费
Amt_def over_unused_fee; 		///< 逾期锁券占用费
Amt_def puni_unused_fee; 		///< 逾期锁券占用费罚息
CreditRatio_def violate_fee_rate; 		///< 违约费率
Amt_def violate_fee; 		///< 违约费
Amt_def over_violate_fee; 		///< 逾期违约费
Amt_def puni_violate_fee; 		///< 逾期违约罚息
Date8_def begin_date; 		///< 生效日期
Date8_def close_date; 		///< 截止日期
FirstInMode_def first_in_mode; 		///< 首次进入模式
LockStatus_def status; 		///< 状态
SecuritySymbol_def security_symbol; 		///< 证券简称
/*
 * @brief 默认构造函数
 */
  AgwCreditStkLockArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 12 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 1 + 40;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCreditStkLockArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 锁券合同查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditStkLock : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 客户一级锁券记录总数
std::vector<AgwCreditStkLockArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditStkLock();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditStkLock& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditStkLock;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 锁券展期申请查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryLockDelayApply : public AgwMsgExtQueryBase
{
public:
Date8_def begin_date; 		///< 开始日期
Date8_def end_date; 		///< 结束日期
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
Date8_def sys_date; 		///< 锁券日期
Sno_def sno; 		///< 锁券流水号
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryLockDelayApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryLockDelayApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryLockDelayApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 锁券展期申请查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwLockDelayApplyArray : public mp::RootPacket
{
public:
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
AccountID_def account_id; 		///< 账户ID
Date8_def sys_date; 		///< 系统日期
Sno_def sno; 		///< 流水号
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
Date8_def old_end_date; 		///< 原结束日
CreditRatio_def old_used_fee_rate; 		///< 原使用费率
CreditRatio_def old_unused_fee_rate; 		///< 原占用费率
CreditRatio_def old_violate_fee_rate; 		///< 原违约费率
Qty_def old_left_qty; 		///< 原展期数量
BackStatus_def old_hs_status; 		///< 原状态
Date8_def app_end_date; 		///< 申请结束日
CreditRatio_def app_used_fee_rate; 		///< 申请使用费率
CreditRatio_def app_unused_fee_rate; 		///< 申请占用费率
Qty_def app_delay_qty; 		///< 申请展期数量
Date8_def chk_end_date; 		///< 审批结束日
CreditRatio_def chk_used_fee_rate; 		///< 审批使用费率
CreditRatio_def chk_unused_fee_rate; 		///< 审批占用费率
CreditRatio_def chk_violate_fee_rate; 		///< 审批违约费率
Qty_def chk_delay_qty; 		///< 审批展期数量
Qty_def back_qty; 		///< 处理日回收数量
Qty_def delay_qty; 		///< 展期数量
DealFlag_def deal_flag; 		///< 处理标识
Date8_def deal_date; 		///< 处理日期
Date8_def oper_date; 		///< 申请日期
Time9_def oper_time; 		///< 申请时间
OrderWay_def order_way; 		///< 操作方式
ClientFeatureCode_def client_feature_code; 		///< 网路地址
CheckStatus_def chk_status; 		///< 审批状态
OperID_def chk_oper; 		///< 审批柜员
Char64_def chk_remark; 		///< 审批备注
Date8_def chk_date; 		///< 审批物理日
Time9_def chk_time; 		///< 审批时间
Qty_def lock_qty; 		///< 锁券数量
Qty_def used_qty; 		///< 已使用数量
Qty_def back_qty1; 		///< 已回收数量
Price_def lock_price; 		///< 锁券价格
BackType_def back_type; 		///< 回收类型
Date8_def begin_date; 		///< 开始日期
Date8_def end_date; 		///< 结束日期
Date8_def real_date; 		///< 实际了结日期
Date8_def last_date; 		///< 最后计息日
BackStatus_def hs_status; 		///< 回收状态
ViolateStatus_def wy_status; 		///< 违约状态
SecuritySymbol_def security_symbol; 		///< 证券简称
OverDueDelayFlag_def over_due_delay_flag; 		///< 常规展期0，非常规展期1
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwLockDelayApplyArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwLockDelayApplyArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 锁券展期申请查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultLockDelayApply : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 客户一级锁券记录总数
std::vector<AgwLockDelayApplyArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultLockDelayApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultLockDelayApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultLockDelayApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 锁券子锁查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySubCreditStkLock : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
Date8_def begin_date; 		///< 开始日期
Date8_def end_date; 		///< 结束日期
BackStatusArray_def hs_status; 		///< 回收状态组合
SubFlagArray_def sub_flag; 		///< 子锁类型组合
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySubCreditStkLock();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySubCreditStkLock& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySubCreditStkLock;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 锁券子锁查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwSubCreditStkLockArray : public mp::RootPacket
{
public:
Date8_def sys_date; 		///< 系统日期
Sno_def sno; 		///< 流水号
MarketID_def market_id; 		///< 市场代码
AccountID_def account_id; 		///< 证券账户
SecurityID_def security_id; 		///< 证券代码
SourceType_def source_type; 		///< 证券来源
LockType_def lock_type; 		///< 锁券类型
CreditRatio_def used_fee_rate; 		///< 已使用的费率
CreditRatio_def unused_fee_rate; 		///< 锁券占用费率
CreditRatio_def violate_fee_rate; 		///< 违约费率
Amt_def used_fee; 		///< 使用费  --累计值
Amt_def unused_fee; 		///< 锁券占用费  --累计值
Amt_def over_unused_fee; 		///< 逾期锁券占用费  --累计值
Amt_def puni_unused_fee; 		///< 逾期锁券占用费罚息  --累计值
Amt_def pay_unused_fee; 		///< 锁券占用费  --已付
Amt_def pay_over_unused_fee; 		///< 逾期锁券占用费  --已付
Amt_def pay_puni_unused_fee; 		///< 逾期锁券占用费罚息 --已付
Amt_def unused_fee_unfrz; 		///< 锁券占用费  --日间偿还部分
Amt_def over_unused_fee_unfrz; 		///< 逾期锁券占用费  --日间偿还部分
Amt_def puni_unused_fee_unfrz; 		///< 逾期锁券占用费罚息 --日间偿还部分
Amt_def violate_fee; 		///< 违约费  --累计值
Amt_def over_violate_fee; 		///< 逾期违约费  --累计值
Amt_def puni_violate_fee; 		///< 逾期违约罚息  --累计值
Amt_def pay_violate_fee; 		///< 违约费  --已付
Amt_def pay_over_violate_fee; 		///< 逾期违约费  --已付
Amt_def pay_puni_violate_fee; 		///< 逾期违约罚息 --已付
Amt_def violate_fee_unfrz; 		///< 违约费  --日间偿还冻结
Amt_def over_violate_fee_unfrz; 		///< 逾期违约费  --日间偿还冻结
Amt_def puni_violate_fee_unfrz; 		///< 逾期违约罚息  --日间偿还冻结
Qty_def lock_qty; 		///< 分配数量
Qty_def sale_frz; 		///< 日间卖出冻结(指定头寸开仓，则委托记加，撤单记减，成交不管)
Qty_def used_qty; 		///< 已使用数量
Qty_def back_qty; 		///< 已回收数量
Price_def lock_price; 		///< 分配日收盘价
BackType_def back_type; 		///< 回收类型
Date8_def begin_date; 		///< 分配日期
Date8_def end_date; 		///< 到期日期
Date8_def real_date; 		///< 实际了结日期
Date8_def last_date; 		///< 最后计息日期
BackStatus_def hs_status; 		///< 回收状态
ViolateStatus_def wy_status; 		///< 违约状态
SubFlag_def sub_flag; 		///< 子锁类型
Date8_def link_date; 		///< 预锁则来自预分配记录，调拨则为源头子锁的日期
Sno_def link_sno; 		///< 预锁则来自预分配记录，调拨则为源头子锁的编号
PriceFlag_def price_flag; 		///< 分配当日是否按当日收盘价更新lockprice
SecuritySymbol_def security_symbol; 		///< 证券简称
Date8_def back_date; 		///< 归还日期
CheckStatus_def chk_status; 		///< 展期审批状态
/*
 * @brief 默认构造函数
 */
  AgwSubCreditStkLockArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 11 + 2 + 12 + 8 + 1 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 8 + 8 + 8 + 1 + 1 + 1 + 8 + 11 + 1 + 40 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSubCreditStkLockArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户子锁查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSubCreditStkLock : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 客户一级锁券记录总数
std::vector<AgwSubCreditStkLockArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSubCreditStkLock();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSubCreditStkLock& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSubCreditStkLock;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券指定合约卖券还款金额试算
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySellRepayAppointAmt : public AgwMsgExtQueryBase
{
public:
Date8_def contract_date; 		///< 合约委托日期
Sno_def contract_sno; 		///< 合约流水号
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySellRepayAppointAmt();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySellRepayAppointAmt& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySellRepayAppointAmt;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 融资融券指定合约卖券还款金额试算结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSellRepayAppointAmt : public AgwMsgExtQueryResultBase
{
public:
Amt_def total_fee; 		///< 总费用
Amt_def amt_pay; 		///< 该笔合约待偿还本金
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSellRepayAppointAmt();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSellRepayAppointAmt& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSellRepayAppointAmt;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股份成本重置请求
 */
class EXPORT_MACRO_DEFINE AgwMsgShareCostReset : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
PWD_def password; 		///< 密码
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
Price_def cost_price; 		///< 成本价格
Price_def profit_cost_price; 		///< 盈亏成本价格
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
Price_def hkd_profit_cost_price; 		///< 港币盈亏成本价格
/*
 * @brief 默认构造函数
 */
  AgwMsgShareCostReset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgShareCostReset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgShareCostReset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股份成本重置应答
 */
class EXPORT_MACRO_DEFINE AgwMsgShareCostResetResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
TimeStamp_def transact_time; 		///< 回报处理时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgShareCostResetResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgShareCostResetResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgShareCostResetResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户代码查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCustID : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCustID();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCustID& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCustID;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户代码查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustID : public AgwMsgExtQueryResultBase
{
public:
CustName_def cust_name; 		///< 客户姓名
MarketID_def market_id; 		///< 证券所属市场
BankCode_def bank_code; 		///< 银行代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustID();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustID& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustID;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 信用和普通账号信息组合
 */
class EXPORT_MACRO_DEFINE AgwAcctArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场
Currency_def currency; 		///< 货币类型
FundAccountID_def fund_account_id; 		///< 信用资金账户
AccountID_def account_id; 		///< 信用证券账户
FundAccountID_def pt_fund_account_id; 		///< 普通资金账户
AccountID_def pt_account_id; 		///< 普通证券账户
/*
 * @brief 默认构造函数
 */
  AgwAcctArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 4 + 16 + 12 + 16 + 12;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwAcctArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 普通信用账号关系查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCreditAcct : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCreditAcct();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCreditAcct& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCreditAcct;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 普通信用账号关系查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCreditAcct : public AgwMsgExtQueryResultBase
{
public:
Length_def data_length; 		///< 组合个数
std::vector<AgwAcctArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCreditAcct();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCreditAcct& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCreditAcct;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 退市整理期买入权限查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryDelisingPeriodBuyRight : public AgwMsgExtQueryBase
{
public:
BusinessRight_def right; 		///< 退整买入权限
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryDelisingPeriodBuyRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryDelisingPeriodBuyRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryDelisingPeriodBuyRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 退市整理期买入权限查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDelisingPeriodBuyRight : public AgwMsgExtQueryResultBase
{
public:
BusinessRight_def right; 		///< 退整买入权限
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDelisingPeriodBuyRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDelisingPeriodBuyRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDelisingPeriodBuyRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户风险分数和等级查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryCustRiskValue : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryCustRiskValue();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryCustRiskValue& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryCustRiskValue;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户风险分数和等级查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustRiskValue : public AgwMsgExtQueryResultBase
{
public:
CustRiskLevel_def risk_level; 		///< 用户风险承受等级
InvestPerid_def invest_perid; 		///< 投资期限
InvestVarieties_def invest_varieties; 		///< 投资种类集合
DeficitLevel_def deficit_level; 		///< 可承受亏损等级
SpecialRiskFlag_def special_risk_flag; 		///< 是否特别风险客户
ProfessionalKind_def professional_kind; 		///< 专业投资者类别
CustRiskName_def risk_name; 		///< 用户风险名称
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustRiskValue();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustRiskValue& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustRiskValue;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户股东代码查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryAccountId : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
AccountID_def inside_account_id; 		///< 股东内码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryAccountId();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryAccountId& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryAccountId;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户股东代码查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwAccountIdArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
CustID_def cust_id; 		///< 客户号
AccountID_def account_id; 		///< 普通证券账户ID
/*
 * @brief 默认构造函数
 */
  AgwAccountIdArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 16 + 12;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwAccountIdArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户股东代码查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAccountId : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwAccountIdArray> account_id_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAccountId();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAccountId& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAccountId;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户股东机构查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryCustBranchId : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
AccountID_def inside_account_id; 		///< 股东内码
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryCustBranchId();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryCustBranchId& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryCustBranchId;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户股东机构查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustBranchId : public AgwMsgExtQueryResultBase
{
public:
MarketID_def market_id; 		///< 市场ID
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustBranchId();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustBranchId& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustBranchId;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户股东信息查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryAccountInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryAccountInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryAccountInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryAccountInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户股东信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwAccountInfoArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
CustID_def cust_id; 		///< 客户号
AccountID_def account_id; 		///< 普通证券账户ID
Assignment_def assignment; 		///< 指定交易标志
FundAccountID_def fund_account_id; 		///< 资金账户ID
CustName_def cust_name; 		///< 客户姓名
Date8_def open_date; 		///< 开户日期
AccountSeq_def account_seq; 		///< 股东账号序列
AccountCreditFlag_def credit_flag; 		///< 信用账号标志
AccountRole_def account_role; 		///< 账户角色
BusinessRight_def repurchase_right; 		///< 回购权限
CustomerStatus_def status; 		///< 客户状态
/*
 * @brief 默认构造函数
 */
  AgwAccountInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 16 + 12 + 1 + 16 + 64 + 8 + 1 + 1 + 1 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwAccountInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户股东信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAccountInfo : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwAccountInfoArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAccountInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAccountInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAccountInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股东账户指定交易状态查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryAccountAssignment : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryAccountAssignment();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryAccountAssignment& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryAccountAssignment;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股东账户指定交易状态查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultAccountAssignment : public AgwMsgExtQueryResultBase
{
public:
Assignment_def assignment; 		///< 指定交易标志
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultAccountAssignment();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultAccountAssignment& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultAccountAssignment;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 中签资金冻结后放弃认购
 */
class EXPORT_MACRO_DEFINE AgwMsgExtGiveupNewStk : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
OrderWay_def order_way; 		///< 委托方式
UserInfo_def user_info; 		///< 用户私有信息
MarketID_def market_id; 		///< 证券账户所属市场
SecurityID_def security_id; 		///< 证券代码
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtGiveupNewStk();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtGiveupNewStk& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtGiveupNewStk;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 中签资金冻结后放弃认购应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtGiveupNewStkResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
Sno_def sno; 		///< 申请编号
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtGiveupNewStkResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtGiveupNewStkResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtGiveupNewStkResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 盯市订阅消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCustMtmSub : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
OrderWay_def order_way; 		///< 委托方式
UserInfo_def user_info; 		///< 用户私有信息
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCustMtmSub();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCustMtmSub& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCustMtmSub;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 盯市订阅消息应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCustMtmSubResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCustMtmSubResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCustMtmSubResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCustMtmSubResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 中签结果组合
 */
class EXPORT_MACRO_DEFINE AgwStkHitOrderArray : public mp::RootPacket
{
public:
Date8_def biz_date; 		///< 中签日期(业务日期)
QSNo_def sno; 		///< 流水号
QSDataType_def data_type; 		///< 清算数据类型 0-配号数据 1-中签数据 2-放弃数据
Currency_def currency; 		///< 货币代码
AccountID_def account_id; 		///< 股东账号
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
BusiType_def busi_type; 		///< 业务类型
SecuritySymbol_def stock_name; 		///< 订单类别
OrderID_def order_id; 		///< 合同序号
Date8_def order_date; 		///< 委托日期
Qty_def match_qty; 		///< 成交数量
Price_def match_price; 		///< 成交价格
MateNo_def mateno; 		///< 配号
QSNo_def ballotno; 		///< 中签号码
GiveupStatus_def giveup_flag; 		///< 清算放弃标识,0-放弃，1-不放弃
Qty_def hit_qty; 		///< 中签数量
Amt_def pay_amt; 		///< 应缴款金额
Date8_def pay_date; 		///< 缴款日期
GiveupStatus_def giveupstatus; 		///< 解冻标识, 0-已放弃并解冻, 1-未放弃
GiveupStatus_def giveupable; 		///< 是否允许放弃
/*
 * @brief 默认构造函数
 */
  AgwStkHitOrderArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 32 + 1 + 4 + 12 + 2 + 8 + 6 + 40 + 16 + 8 + 8 + 8 + 32 + 32 + 1 + 8 + 8 + 8 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwStkHitOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 中签查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryStkHit : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币代码
Char_def qry_flag; 		///< 查询标志
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryStkHit();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryStkHit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryStkHit;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 中签查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultStkHit : public AgwMsgExtQueryResultBase
{
public:
Length_def data_length; 		///< 组合个数
std::vector<AgwStkHitOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultStkHit();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultStkHit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultStkHit;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网络投票议案查询信息组合
 */
class EXPORT_MACRO_DEFINE AgwVotingProposalArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 投票代码
SecurityID_def base_security_id; 		///< 基础证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
Date8_def meeting_begin_date; 		///< 股东大会开始日
Date8_def meeting_end_date; 		///< 股东大会结束日
MeetingType_def meeting_type; 		///< 股东大会类型
MeetingDesc_def meeting_desc; 		///< 股东大会名称
GeneralMeetingSeq_def general_meeting_code; 		///< 股东大会编码
VotingProposal_def voting_proposal_code; 		///< 投票议案编号
VotingProposalDesc_def voting_prososal_desc; 		///< 投票议案名称
VotingProposalType_def voting_proposal_type; 		///< 议案类型
Int64_def cum_voting_chosen_num; 		///< 累计投票应选数
ShareHolderRole_def share_holder_role; 		///< 股东身份
ProposalRelation_def proposal_relation; 		///< 议案关系
ShareClass_def share_class; 		///< 股份类别
SecuritySymbol_def voting_symbol; 		///< 投票简称
SecurityFullSymbol_def security_full_symbol; 		///< 投票全称
Date8_def reg_date; 		///< 股权登记日
/*
 * @brief 默认构造函数
 */
  AgwVotingProposalArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 40 + 8 + 8 + 40 + 512 + 8 + 10 + 1024 + 1 + 8 + 50 + 100 + 20 + 40 + 64 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwVotingProposalArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 网络投票议案查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryVotingProposal : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def base_security_id; 		///< 基础证券代码
SecurityID_def security_id; 		///< 投票代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryVotingProposal();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryVotingProposal& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryVotingProposal;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网络投票议案查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultVotingProposal : public AgwMsgExtQueryResultBase
{
public:
Length_def data_length; 		///< 组合个数
std::vector<AgwVotingProposalArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultVotingProposal();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultVotingProposal& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultVotingProposal;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF是否需要签约查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryETFSign : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< ETF代码所属市场
SecurityID_def security_id; 		///< ETF代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryETFSign();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryETFSign& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryETFSign;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF是否需要签约查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultETFSign : public AgwMsgExtQueryResultBase
{
public:
Char_def need_sign; 		///< 是否需要签约,1需要,0不需要
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultETFSign();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultETFSign& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultETFSign;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 上市公司网络投票清单查询信息组合
 */
class EXPORT_MACRO_DEFINE AgwVotingListArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 投票代码
SecurityID_def base_security_id; 		///< 基础证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
Date8_def meeting_begin_date; 		///< 股东大会开始日
Date8_def meeting_end_date; 		///< 股东大会结束日
MeetingType_def meeting_type; 		///< 股东大会类型
MeetingDesc_def meeting_desc; 		///< 股东大会名称
GeneralMeetingSeq_def general_meeting_code; 		///< 股东大会编码
SecurityFullSymbol_def security_full_symbol; 		///< 投票长名称
Char_def has_voted; 		///< 是否已投票
/*
 * @brief 默认构造函数
 */
  AgwVotingListArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 40 + 8 + 8 + 40 + 512 + 8 + 64 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwVotingListArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 上市公司网络投票清单查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryVotingList : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
Char_def has_voted; 		///< 是否已投票
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryVotingList();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryVotingList& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryVotingList;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 上市公司网络投票清单查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultVotingList : public AgwMsgExtQueryResultBase
{
public:
Length_def data_length; 		///< 组合个数
std::vector<AgwVotingListArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultVotingList();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultVotingList& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultVotingList;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwReduceCustArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 股东账户
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
PBUID_def pbu_id; 		///< 投资者所属交易单元
Qty_def ms_qty; 		///< 竞价减持额度
Qty_def m0_qty; 		///< 大宗普通交易减持额度
Qty_def md_qty; 		///< 大宗特定(受限)股份减持额度
Qty_def ms_qty_frzn; 		///< 竞价减持额度-冻结部分
Qty_def m0_qty_frzn; 		///< 大宗普通交易减持额度-冻结部分
Qty_def md_qty_frzn; 		///< 大宗特定(受限)股份减持额度-冻结部分
Date8_def trade_day; 		///< 交易日
Qty_def total_qty; 		///< 证券余额
Qty_def frozen_qty; 		///< 证券冻结
Qty_def sz_stock1; 		///< 股份数量1
Qty_def stk_qty2_init; 		///< 初始股份数量2
Qty_def stk_qty3_init; 		///< 初始股份数量3
Qty_def stk_qty4_init; 		///< 初始股份数量4
Qty_def stk_qty5_init; 		///< 初始股份数量5
Qty_def stk_qty6_init; 		///< 初始股份数量6
Qty_def stk_qty2_frzn; 		///< 股份数量2冻结
Qty_def stk_qty3_frzn; 		///< 股份数量3冻结
Qty_def stk_qty4_frzn; 		///< 股份数量4冻结
Qty_def stk_qty5_frzn; 		///< 股份数量5冻结
Qty_def stk_qty6_frzn; 		///< 股份数量6冻结
Qty_def stk_qty2; 		///< 股份数量2
Qty_def stk_qty3; 		///< 股份数量3
Qty_def stk_qty4; 		///< 股份数量4
Qty_def stk_qty5; 		///< 股份数量5
Qty_def stk_qty6; 		///< 股份数量6
/*
 * @brief 默认构造函数
 */
  AgwReduceCustArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 6 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwReduceCustArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 大宗股份减持受限客户查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryReduceCust : public AgwMsgExtQueryBase
{
public:
PBUID_def pbu_id; 		///< 交易单元
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryReduceCust();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryReduceCust& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryReduceCust;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗股份减持受限客户查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultReduceCust : public AgwMsgExtQueryResultBase
{
public:
CustName_def cust_name; 		///< 客户名称
Length_def data_length; 		///< 组合个数
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwReduceCustArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultReduceCust();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultReduceCust& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultReduceCust;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF申赎成交查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgETFQueryOrderEx : public AgwMsgExtQueryBase
{
public:
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,同一个客户当天唯一,填0查所有
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
BusinessAbstractType_def business_abstract_type; 		///< 抽象业务编码：1.现货业务，2.经纪期权，3.两融业务
/*
 * @brief 默认构造函数
 */
  AgwMsgETFQueryOrderEx();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFQueryOrderEx& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFQueryOrderEx;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF成份股记录组
 */
class EXPORT_MACRO_DEFINE AgwETFShareRecordArray : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 证券所属市场
Qty_def qty; 		///< 成交数量或资金比例因子
Amt_def amt; 		///< 成交金额,N18(4)
Price_def price; 		///< 成交价格,N13(4)
ETFTradeReportType_def etf_trade_report_type; 		///< ETF成交回报类型
ExecID_def exec_id; 		///< 执行编号
Char_def margin_amt_type; 		///< 上交所现金替代资金类型
/*
 * @brief 默认构造函数
 */
  AgwETFShareRecordArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8 + 8 + 8 + 1 + 32 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwETFShareRecordArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwETFTradeOrderArray : public mp::RootPacket
{
public:
BusinessType_def business_type; 		///< 业务类型
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 证券账户
Side_def side; 		///< 买卖方向,填0查所有
OrdType_def ord_type; 		///< 订单类别
ExecType_def exec_type; 		///< 当前订单执行类型
ExecID_def exec_id; 		///< 执行编号
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
OrderID_def order_id; 		///< 交易所订单编号, 取值为数字
ClOrdID_def cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
TimeStamp_def transact_time; 		///< 交易所回报时间,格式为YYYY:MM:DD:HH:MM:SS.000
Price_def last_px; 		///< 成交价格,N13(4)
Qty_def last_qty; 		///< 成交数量(股票为股、基金为份、上海债券为手，其他为张)N15(2)
Amt_def total_value_traded; 		///< 成交金额,精确到分,N18(4)
Amt_def fee; 		///< 费用,N18(4)
Currency_def currency; 		///< 货币种类
UserInfo_def user_info; 		///< 用户私有信息
std::vector<AgwETFShareRecordArray> order_array; 		///< ETF成份股记录组
/*
 * @brief 默认构造函数
 */
  AgwETFTradeOrderArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwETFTradeOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 成交查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgETFQueryResultTradeOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 成交记录总数
std::vector<AgwETFTradeOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgETFQueryResultTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFQueryResultTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFQueryResultTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 成分股数组
 */
class EXPORT_MACRO_DEFINE AgwMsgETFConstituentStockUnit : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Qty_def qty; 		///< 成交数量(股票为股、基金为份、上海债券为手，其他为张)N15(2)
Amt_def amt; 		///< 成交金额,N18(4)
Amt_def price; 		///< 成交价格,N18(4)
ETFTradeReportType_def etf_trade_report_type; 		///< ETF成交回报类型
ExecID_def exec_id; 		///< 执行编号
/*
 * @brief 默认构造函数
 */
  AgwMsgETFConstituentStockUnit();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8 + 8 + 8 + 1 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFConstituentStockUnit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief ETF业务成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgETFRedemptionTradeDealER : public AgwMsgTradeExecutionReport
{
public:
std::vector<AgwMsgETFConstituentStockUnit> constituent_stock; 		///< 成分股与资金信息
/*
 * @brief 默认构造函数
 */
  AgwMsgETFRedemptionTradeDealER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFRedemptionTradeDealER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFRedemptionTradeDealER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 客户总资产查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryTotalFundAsset : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryTotalFundAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryTotalFundAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryTotalFundAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户股东信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTotalFundAssetArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 币种
Amt_def fund_bal; 		///< 资金余额
Amt_def fund_avl; 		///< 资金可用
Amt_def fund_asset_total_new; 		///< 总资产新
Amt_def fund_asset_total; 		///< 总资产
Amt_def fund_asset; 		///< 资金资产
Amt_def fund_asset_ggt; 		///< 港股通T+1日未交收资金
Amt_def fund_asset_stktotal; 		///< 证券总资产
Amt_def fund_asset_stk; 		///< 证券资产
Amt_def fund_asset_bond_total; 		///< 债券总资产
Amt_def fund_asset_bond; 		///< 债券资产
Amt_def fund_asset_rzgh; 		///< 国债正回购
Amt_def fund_asset_rqgh; 		///< 国债逆回购
Amt_def fund_asset_stkzy; 		///< 债券质押资产（回购资产）
Amt_def fund_asset_rzcs; 		///< 协议融资
Amt_def fund_asset_rqcs; 		///< 协议融券
Amt_def fund_asset_stkxyzy; 		///< 协议债券质押资产
Amt_def fund_asset_adjust; 		///< 待交收证券资产
Amt_def fund_asset_oftotal; 		///< 理财总资产
Amt_def fund_asset_of; 		///< 基金资产
Amt_def fund_asset_bjhgsh; 		///< 上海报价回购
Amt_def fund_asset_bjhgsz; 		///< 深圳报价回购
Amt_def fund_asset_ofuncome; 		///< 基金认申购定投未确认在途资金
Amt_def fund_asset_xjgj; 		///< 现金管家
Amt_def fund_asset_zfzz; 		///< 支付转账
Amt_def fund_asset_jdlsec; 		///< 君得利二号
Amt_def fund_asset_fjjj; 		///< 分级基金赎回日提前释放
Amt_def fund_asset_curdayrqgh; 		///< 融券购回当日到期
Amt_def fund_asset_curdayrzgh; 		///< 融资购回当日到期
Amt_def fund_asset_otc_end; 		///< OTC当前到期
Amt_def debts_total; 		///< 总负债
Amt_def zy_mtkvalue; 		///< 应收股票质押市值
Amt_def zy_debts; 		///< 股票质押负债
Amt_def en_mktvalue; 		///< 应收约定购回市值
Amt_def en_debts; 		///< 约定购回负债
Amt_def zywy_mktvalue; 		///< 股票质押违约卖出市值
Amt_def fund_asset_matchrqgh; 		///< 国债逆回购当日成交
Amt_def sum_profit_cost; 		///< 证券总成本
Amt_def sump_rofit_amt; 		///< 证券资产总浮动盈亏
Amt_def fund_asset_stkwxxj; 		///< 网下现金认购资产
Amt_def fund_asset_adjust_xsf; 		///< 新三板新股申购在途资金
Amt_def fund_asset_adjust_qt; 		///< 新股配债在途资金
Amt_def fund_asset_adjust_hgt; 		///< 港股通在途资金
Amt_def fund_asset_adjust_lof; 		///< lof在途资金
Amt_def fund_asset_stk_only; 		///< 股票资产
Amt_def fund_asset_adjust_etf; 		///< etf在途资金
Date8_def trade_date; 		///< 当前资产交易日期
/*
 * @brief 默认构造函数
 */
  AgwTotalFundAssetArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTotalFundAssetArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户总资产查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultTotalFundAsset : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwTotalFundAssetArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultTotalFundAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultTotalFundAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultTotalFundAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通参考汇率查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKSRefExchangeRate : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKSRefExchangeRate();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKSRefExchangeRate& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKSRefExchangeRate;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通参考汇率查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKSRefExchangeRate : public AgwMsgExtQueryResultBase
{
public:
MarketID_def market_id; 		///< 市场ID
ExchangeRate_def buy_exchange_rate; 		///< 买入参考汇率
FloatingRatio_def day_buy_rise_rate; 		///< 日间买入参考汇率浮动比例
FloatingRatio_def night_buy_rise_rate; 		///< 夜市买入参考汇率浮动比例
ExchangeRate_def sell_exchange_rate; 		///< 卖出参考汇率
FloatingRatio_def day_sale_rise_rate; 		///< 日间卖出参考汇率浮动比例
FloatingRatio_def night_sale_rise_rate; 		///< 夜市卖出参考汇率浮动比例
FloatingRatio_def mid_rate; 		///< 中间参考汇率
FloatingRatio_def sett_rate; 		///< 结算汇率
Date8_def sys_date; 		///< 更新日期
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKSRefExchangeRate();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKSRefExchangeRate& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKSRefExchangeRate;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通市场交易状态查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKSMarketTransStatus : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKSMarketTransStatus();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKSMarketTransStatus& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKSMarketTransStatus;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通市场交易状态查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKSMarketTransStatus : public AgwMsgExtQueryResultBase
{
public:
MarketID_def market_id; 		///< 市场ID
Char_def mkt_trd_status; 		///< 市场交易状态
Amt_def quota_bal; 		///< 每日初始额度
Amt_def quota_avl; 		///< 日中剩余额度
Char_def quota_status; 		///< 额度状态
Time_def upd_date; 		///< 更新日期，N17格式(YYYYmmDDHHMMSSsss)
String_def remark; 		///< 备注
Char_def quota_new_status; 		///< 新额度状态，1-额度用完，2-额度可用，3-额度充足
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKSMarketTransStatus();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKSMarketTransStatus& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKSMarketTransStatus;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通交易日历表查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKSTradeCalendar : public AgwMsgExtQueryBase
{
public:
Date8_def begin_date; 		///< 开始日期
Date8_def end_date; 		///< 结束日期
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKSTradeCalendar();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKSTradeCalendar& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKSTradeCalendar;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通交易日历组合
 */
class EXPORT_MACRO_DEFINE AgwHKSTradingCalendar : public mp::RootPacket
{
public:
Date8_def phy_date; 		///< 物理日期
Char_def hk_business_flag; 		///< 沪港通交易日标识，‘0’ 交易日 '1' 非交易日
Char_def hk_commit_flag; 		///< 沪港通交收日标识，‘0’ 交收日 '1' 非交收日
Currency_def money_type; 		///< 货币代码
Char_def business_flag; 		///< 交易日标识，‘0’ 交易日 '1' 非交易日
/*
 * @brief 默认构造函数
 */
  AgwHKSTradingCalendar();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 1 + 1 + 4 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwHKSTradingCalendar& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 外围沪港通交易日历表查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKSTradeCalendar : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwHKSTradingCalendar> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKSTradeCalendar();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKSTradeCalendar& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKSTradeCalendar;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通标的券查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKSUnderlyingSecurity : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKSUnderlyingSecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKSUnderlyingSecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKSUnderlyingSecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通标的券查询数组
 */
class EXPORT_MACRO_DEFINE AgwHKSUnderlyingSecurity : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
SecurityType_def security_type; 		///< 证券类别
Currency_def money_type; 		///< 货币代码
Char_def stop_flag; 		///< 停牌标志，F-正常、T-停牌
String_def reserve6; 		///< 字符串备用字段
/*
 * @brief 默认构造函数
 */
  AgwHKSUnderlyingSecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwHKSUnderlyingSecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 外围沪港通标的券查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKSUnderlyingSecurity : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwHKSUnderlyingSecurity> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKSUnderlyingSecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKSUnderlyingSecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKSUnderlyingSecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通证券交易状态查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKSSecurityStatus : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKSSecurityStatus();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKSSecurityStatus& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKSSecurityStatus;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通证券交易状态查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSecurityStatus : public AgwMsgExtQueryResultBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Char_def zs_trd_status; 		///< 整手交易状态，A' 买卖 'B' 只买 'S' 只卖 'N' 不可买卖
Char_def lg_trd_status; 		///< 零股交易状态，A' 买卖 'B' 只买 'S' 只卖 'N' 不可买卖
Char_def object_flag; 		///< 标的标志，0'-非标的证券 '1'-标的证券
Date8_def upd_date; 		///< 更新日期
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSecurityStatus();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSecurityStatus& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSecurityStatus;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通辅资金表资产查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKFundAsset : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKFundAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKFundAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKFundAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通辅资金表资产查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKFundAsset : public AgwMsgExtQueryResultBase
{
public:
Currency_def currency; 		///< 货币种类
Amt_def fund_uncome_avl; 		///< 在途可用资金
Amt_def fund_buy; 		///< 买入冻结
Amt_def fund_sale; 		///< 卖出解冻
Amt_def fund_uncome_buy; 		///< 在途买入
Amt_def fund_uncome_sale; 		///< 在途卖出
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKFundAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKFundAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKFundAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通最小价差查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKPriceRange : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 交易市场
Price_def price; 		///< 价格
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKPriceRange();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKPriceRange& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKPriceRange;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通最小价差查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwHKPriceRangeArray : public mp::RootPacket
{
public:
SecurityType_def security_type; 		///< 证券类型
Price_def begin_price; 		///< 区间起始价格
Price_def end_price; 		///< 区间终止价格
Price_def price_unit; 		///< 最小价差
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
/*
 * @brief 默认构造函数
 */
  AgwHKPriceRangeArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwHKPriceRangeArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 外围沪港通最小价差查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKPriceRange : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwHKPriceRangeArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKPriceRange();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKPriceRange& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKPriceRange;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围港股公司行为查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKCorporateActionRecord : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
ClOrdNO_def cl_ord_no; 		///< 申报编号
CorporateActionType_def corporate_action_type; 		///< 业务类型
HKOrderType_def hk_order_type; 		///< 申报类型
MarketID_def market_id; 		///< 市场代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKCorporateActionRecord();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKCorporateActionRecord& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKCorporateActionRecord;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围港股公司行为查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwHKCorporateActionRecordArray : public mp::RootPacket
{
public:
ClOrdNO_def cl_ord_no; 		///< 申报编号
Date8_def oper_date; 		///< 发生日期
MarketID_def market_id; 		///< 市场
ServiceType_def service_type; 		///< 服务类型
CorporateActionID_def corporate_action_id; 		///< 公司行为代码
CorporateActionType_def corporate_action_type; 		///< 公司行为业务类型
HKOrderType_def hk_order_type; 		///< 申报类型
AccountID_def account_id; 		///< 股东代码
SecurityID_def security_id; 		///< 证券代码
PBUID_def pbu_id; 		///< 交易单元
SecurityType_def security_type; 		///< 证券类型
Qty_def order_qty; 		///< 申报数量
Price_def order_price; 		///< 每股价格
Amt_def order_amt; 		///< 金额
ClearNO_def clear_no; 		///< 清算编号
OrdStatus_def order_status; 		///< 订单状态
ISIN_def isin; 		///< ISIN编码
String_def announcement_number; 		///< 公告编号
String_def proxy_form_number; 		///< 议案编号
String_def affirmative_votes; 		///< 赞成数量
String_def negative_votes; 		///< 反对数量
String_def abstention_votes; 		///< 弃权数量
Qty_def comment_qty; 		///< 备注仅上海HCX有意义
String_def acceptance_number; 		///< 受理编号
DateTime_def receive_time; 		///< 受理时间
Text_def result_code; 		///< 结果代码
String_def description; 		///< 处理结果说明
ClOrdNO_def org_cl_ord_no; 		///< 撤单原编号
Char_def rpt_status; 		///< 集中交易申报状态
SecuritySymbol_def security_symbol; 		///< 证券名称
/*
 * @brief 默认构造函数
 */
  AgwHKCorporateActionRecordArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwHKCorporateActionRecordArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 外围港股公司行为查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKCorporateActionRecord : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwHKCorporateActionRecordArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKCorporateActionRecord();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKCorporateActionRecord& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKCorporateActionRecord;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通市值查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKMarketValue : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKMarketValue();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKMarketValue& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKMarketValue;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围沪港通市值查询应答
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKMarketValue : public AgwMsgExtQueryResultBase
{
public:
Amt_def market_value_sh_hk; 		///< 港币计价的沪港通市值
Amt_def market_value_sz_hk; 		///< 港币计价的深港通市值
Amt_def market_value_hk; 		///< 港币计价的港股通总市值
Amt_def market_value_sh; 		///< 人民币计价的沪港通市值
Amt_def market_value_sz; 		///< 人民币计价的深港通市值
Amt_def market_value; 		///< 人民币计价的港股通总市值
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKMarketValue();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKMarketValue& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKMarketValue;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF成分股查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryComponentShare : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< ETF代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryComponentShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryComponentShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryComponentShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF成分股结果组合
 */
class EXPORT_MACRO_DEFINE AgwComponentShareArray : public mp::RootPacket
{
public:
SecurityID_def etf_security_id; 		///< ETF基金代码
MarketID_def etf_market_id; 		///< ETF基金市场ID
SecuritySymbol_def etf_security_symbol; 		///< ETF名称
SecurityID_def component_security_id; 		///< 成分股代码
MarketID_def component_market_id; 		///< 成分股市场ID
SecuritySymbol_def component_security_symbol; 		///< 成分股简称
SubstituteFlag_def replace_flag; 		///< 现金替代标志
PremiumRatio_def over_price_rate; 		///< 申购溢价比例
Qty_def component_share_qty; 		///< 成分股数量
Amt_def purchase_replace_amt; 		///< 申购现金替代金额
Amt_def redemption_replace_amt; 		///< 赎回现金替代金额
PRType_def etf_pr_type; 		///< 申赎类型
SecurityFullSymbol_def etf_security_full_symbol; 		///< ETF证券长名称
/*
 * @brief 默认构造函数
 */
  AgwComponentShareArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 40 + 8 + 2 + 40 + 1 + 4 + 8 + 8 + 8 + 1 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwComponentShareArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief ETF成分股查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultComponentShare : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwComponentShareArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultComponentShare();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultComponentShare& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultComponentShare;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF基本信息查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryETFInfo : public AgwMsgExtQueryBase
{
public:
SecurityID_def etf_security_id; 		///< ETF基金代码(填空格查所有)
ReportIndex_def query_index; 		///< 指定起始查询索引号, 将返回索引号以后的订单, 填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量, 填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryETFInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryETFInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryETFInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF成分股结果组合
 */
class EXPORT_MACRO_DEFINE AgwETFInfoArray : public mp::RootPacket
{
public:
MarketID_def etf_market_id; 		///< ETF基金市场ID
SecurityID_def etf_security_id; 		///< ETF基金代码
SecuritySymbol_def etf_security_symbol; 		///< ETF名称
SecurityID_def etf_security_id_0; 		///< 二级市场基金代码
SecurityID_def etf_security_id_1; 		///< 一级市场基金代码 
SecurityID_def etf_security_id_2; 		///< 申购赎回资金划拨代码 
Qty_def purchase_redemption_unit; 		///< 申赎最小份额
Amt_def e_cash_componet; 		///< 预估现金差额
MaxCashRatio_def max_cash_ratio; 		///< 最大现金替代比例
Bool_def is_purchase; 		///< 申购允许状态, True表示允许申购, False表示不允许申购
Bool_def is_redemption; 		///< 赎回允许状态, True表示允许赎回, False表示不允许赎回
NumInGroup_def record_num; 		///< 股票篮记录数
Date8_def trading_day; 		///< T日日期，格式YYYYMMDD 
Amt_def nav_per_cu; 		///< T-1日申购赎回基准单位净值
Amt_def nav; 		///< T-1日基金的单位净值
Qty_def creation_limit_user; 		///< 单帐户当天累计可申购的基金份额上限,为0表示没有限制 [0,999999999999]
Qty_def redemption_limit_user; 		///< 单帐户当天累计可赎回的基金份额上限,为0表示没有限制 [0,999999999999]
Qty_def net_creation_limit_user; 		///< 单个证券账户当天净申购的基金份额上限,为0表示没有限制[0,999999999999]
Qty_def net_redemption_limit_user; 		///< 单个证券账户当天净赎回的基金份额上限,为0表示没有限制[0,999999999999]
SecurityType_def security_type; 		///< ETF类型,1：本市场ETF 2：跨市场ETF 3：跨市场ETF（通过基金业务系统）4:实时申赎货币基金
Amt_def cash_componet; 		///< T-1日现金差额
SecurityFullSymbol_def security_full_symbol; 		///< 证券长名称
/*
 * @brief 默认构造函数
 */
  AgwETFInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 8 + 8 + 8 + 8 + 4 + 1 + 1 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 2 + 8 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwETFInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief ETF基本信息查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultETFInfo : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwETFInfoArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultETFInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultETFInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultETFInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗协议交易预委托
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradePreorder : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
PWD_def password; 		///< 密码
BSFlag2_def bs_flag2; 		///< 集中交易买卖类别
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
OrdType_def order_type; 		///< 订单类型
Price_def price; 		///< 委托价格
Qty_def order_qty; 		///< 申报数量(张)/ETF份额
TrdSubType_def trd_sub_type; 		///< 是否受限
Contactor_def contactor; 		///< 联系人
ContactInfo_def contact_info; 		///< 联系人信息
ConfirmID_def confirm_id; 		///< 约定号
PBUID_def counter_party_pbu_id; 		///< 对手方PBUID
CashMargin_def cash_margin; 		///< 信用标识
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
Bool_def after_hours_pricing_flag; 		///< 盘后定价标识
PriceProperty_def price_property; 		///< 价格类型
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradePreorder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradePreorder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradePreorder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗协议交易预委托应答
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradePreorderResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
TimeStamp_def transact_time; 		///< 回报处理时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
ClOrdID_def cl_ord_id; 		///< 报盘合同号
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradePreorderResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradePreorderResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradePreorderResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗交易预委托营业部权限查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryBlockTradePreorderBranchRight : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryBlockTradePreorderBranchRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryBlockTradePreorderBranchRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryBlockTradePreorderBranchRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗交易预委托营业部权限查询
 */
class EXPORT_MACRO_DEFINE BlockTradePreorderBranchRight : public mp::RootPacket
{
public:
BranchID_def branch_id; 		///< 营业部代码
Date8_def open_date; 		///< 授权日期
/*
 * @brief 默认构造函数
 */
  BlockTradePreorderBranchRight();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 10 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const BlockTradePreorderBranchRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 大宗交易预委托营业部权限查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultBlockTradePreorderBranchRight : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<BlockTradePreorderBranchRight> order_array; 		///< 大宗预委托营业部权限查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultBlockTradePreorderBranchRight();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultBlockTradePreorderBranchRight& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultBlockTradePreorderBranchRight;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗协议交易预委托撤销
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradePreorderCancel : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
PWD_def password; 		///< 密码
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
Remark_def remark; 		///< 备注
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradePreorderCancel();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradePreorderCancel& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradePreorderCancel;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗协议交易预委托撤销应答
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradePreorderCancelResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
TimeStamp_def transact_time; 		///< 回报处理时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradePreorderCancelResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradePreorderCancelResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradePreorderCancelResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗交易预委托查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryBlockTradePreorder : public AgwMsgExtQueryBase
{
public:
Date8_def begin_data; 		///< 开始日期
Date8_def end_data; 		///< 结束日期
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
ApplyStatusArray_def apply_status; 		///< 申请状态,填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryBlockTradePreorder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryBlockTradePreorder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryBlockTradePreorder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗预委托查询结果组合
 */
class EXPORT_MACRO_DEFINE PktBlockTradePreorderArray : public mp::RootPacket
{
public:
Date8_def apply_date; 		///< 申请日期
Sno_def apply_sno; 		///< 申请编号
Date8_def oper_date; 		///< 操作日期
Time9_def oper_time; 		///< 操作时间
BranchID_def branch_id; 		///< 营业部代码（券商编码）
CustID_def cust_id; 		///< 客户号
CustName_def cust_name; 		///< 客户姓名
FundAccountID_def fund_account_id; 		///< 资金账户ID
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 账户ID
SecurityID_def security_id; 		///< 合约编码
SecuritySymbol_def security_symbol; 		///< 证券简称
Price_def order_price; 		///< 委托价格
Qty_def order_qty; 		///< 委托数量
BSFlag2_def bs_flag2; 		///< 买卖类别
PBUID_def counter_party_pbu_id; 		///< 对手方PBUID
ConfirmID_def confirm_id; 		///< 约定号
Contactor_def contactor; 		///< 联系人
ContactInfo_def contact_info; 		///< 联系人信息
TrdSubType_def trd_sub_type; 		///< 是否受限
OperID_def chk_oper; 		///< 审批柜员
ApplyStatus_def apply_status; 		///< 申请状态
DealStatus_def deal_status; 		///< 执行状态
Date8_def order_date; 		///< 委托日期
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
String_def order_key_error_msg; 		///< 委托返回信息
String_def ext_str; 		///< 扩展字段存放clordid
/*
 * @brief 默认构造函数
 */
  PktBlockTradePreorderArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const PktBlockTradePreorderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 大宗交易预委托查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultBlockTradePreorder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<PktBlockTradePreorderArray> order_array; 		///< 大宗预委托查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultBlockTradePreorder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultBlockTradePreorder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultBlockTradePreorder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可转债持仓查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryConvertibleBonds : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,不传查全部
TradeType_def trade_type; 		///< 交易类型 22-转股换股，23-回售，不传查全部
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
SecurityID_def security_id; 		///< 证券ID,填空查全部
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryConvertibleBonds();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryConvertibleBonds& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryConvertibleBonds;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可转债持仓查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwConvertibleBondsArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 货币种类
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 股东账户
SecurityID_def security_id; 		///< 合约编码（申报代码）
SecuritySymbol_def security_symbol; 		///< 证券名称（申报证券名称）
Qty_def stk_bal; 		///< 证券余额
Qty_def stk_avl; 		///< 证券可用数
Qty_def leaves_qty; 		///< 市值持仓，对应411010中的stkqty
Amt_def buy_cost; 		///< 买入成本
Price_def cost_price; 		///< 成本价格
Amt_def mkt_val; 		///< 最新市值
Amt_def income; 		///< 买入浮动盈亏，对应411010中的profit
Price_def last_price; 		///< 最新价
Amt_def ticket_price; 		///< 证券面额
Qty_def qty_unit; 		///< 数量单位
Amt_def price_unit; 		///< 价格单位
Qty_def max_qty; 		///< 目标证券单笔最大委托数量
Char_def is_suspended; 		///< 目标证券停牌标志
TradeType_def trade_type; 		///< 交易类型 22-转股换股，23-回售，29-同时支持转股及回售
SecurityID_def target_security_id; 		///< 目标证券代码
SecuritySymbol_def target_security_symbol; 		///< 目标证券名称
SecurityID_def underlying_security_id; 		///< 正股代码（深圳市场）
SecuritySymbol_def underlying_security_symbol; 		///< 正股名称（深圳市场）
Price_def profit_price; 		///< 盈亏成本价格
Amt_def pro_income; 		///< 累计浮动盈亏,对应410503中的proincome和totalprofitcost
/*
 * @brief 默认构造函数
 */
  AgwConvertibleBondsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 2 + 12 + 8 + 40 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 1 + 8 + 40 + 8 + 40 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwConvertibleBondsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 可转债持仓查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultConvertibleBonds : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwConvertibleBondsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultConvertibleBonds();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultConvertibleBonds& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultConvertibleBonds;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转投资者受限证券查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdStkAccountLimit : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码,填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdStkAccountLimit();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdStkAccountLimit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdStkAccountLimit;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转投资者受限证券查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwThirdStkAccountLimitArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 交易市场
SecuritySymbol_def security_symbol; 		///< 证券简称
SecurityID_def security_id; 		///< 证券代码
AccountID_def account_id; 		///< 证券账户
/*
 * @brief 默认构造函数
 */
  AgwThirdStkAccountLimitArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 40 + 8 + 12;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdStkAccountLimitArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 股转投资者受限证券查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdStkAccountLimit : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwThirdStkAccountLimitArray> order_array; 		///< 股转投资者受限证券查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdStkAccountLimit();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdStkAccountLimit& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdStkAccountLimit;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转协议行情查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdAgreementTransferMd : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码,填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdAgreementTransferMd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdAgreementTransferMd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdAgreementTransferMd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转协议行情查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwThirdAgreementTransferMdArray : public mp::RootPacket
{
public:
SecurityID_def XYZQDM; 		///< 证券代码
PBUID_def XYJYDY; 		///< 交易单元
BSFlag2_def XYYWLB; 		///< 业务类别
Qty_def XYWTSL; 		///< 申报数量
Price_def XYWTJG; 		///< 申报价格
ConfirmID_def XYYDH; 		///< 成交约定号
TimeC6_def XYWTSJ; 		///< 申报时间
InvalidStatus_def XYJLZT; 		///< 记录状态
Char_def XYBYBZ; 		///< 备用标志
/*
 * @brief 默认构造函数
 */
  AgwThirdAgreementTransferMdArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 6 + 2 + 8 + 8 + 8 + 6 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdAgreementTransferMdArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 股转协议行情查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdAgreementTransferMd : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwThirdAgreementTransferMdArray> order_array; 		///< 股转协议行情查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdAgreementTransferMd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdAgreementTransferMd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdAgreementTransferMd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转特有持仓查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdStkAsset : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
ShareNature_def share_nature; 		///< 证券性质
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdStkAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdStkAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdStkAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转特有持仓查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwThirdStkAssetArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Currency_def currency; 		///< 货币种类
PBUID_def pbu_id; 		///< 席位代码
Qty_def stk_bal; 		///< 股份余额
Qty_def stk_avl; 		///< 股份可用
ShareNature_def share_nature; 		///< 股份性质
ShareNatureSymbol_def share_nature_symbol; 		///< 股份性质说明
Qty_def stk_buy; 		///< 股份买入解冻
Qty_def stk_sell; 		///< 股份卖出冻结
Qty_def stk_uncome_buy; 		///< 在途股份买入解冻
Qty_def stk_uncome_sell; 		///< 在途股份卖出冻结
/*
 * @brief 默认构造函数
 */
  AgwThirdStkAssetArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 40 + 4 + 6 + 8 + 8 + 2 + 40 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdStkAssetArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 股转特有持仓查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdStkAsset : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwThirdStkAssetArray> order_array; 		///< 股转特有持仓查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdStkAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdStkAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdStkAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 非公开转让申报信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdFgksbxx : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdFgksbxx();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdFgksbxx& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdFgksbxx;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 非公开转让申报信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwThirdFgksbxxArray : public mp::RootPacket
{
public:
SecurityID_def FGKZQDM; 		///< 证券代码
PBUID_def FGKJYDY; 		///< 交易单元
Char2_def FGKZQLB; 		///< 证券类别
Char2_def FGKSBLB; 		///< 申报类别
Qty_def FGKWTSL; 		///< 申报数量
Price_def FGKWTJG; 		///< 申报价格
Int64_def FGKYDH; 		///< 成交约定号
TimeC6_def FGKWTSJ; 		///< 申报时间
Char_def FGKJLZT; 		///< 记录状态
Char_def FGKBYBZ; 		///< 备用标志
/*
 * @brief 默认构造函数
 */
  AgwThirdFgksbxxArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 6 + 2 + 2 + 8 + 8 + 8 + 6 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdFgksbxxArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 非公开转让申报信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdFgksbxx : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwThirdFgksbxxArray> order_array; 		///< 非公开转让申报信息查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdFgksbxx();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdFgksbxx& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdFgksbxx;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 非公开转让成交信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdFgkcjxx : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdFgkcjxx();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdFgkcjxx& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdFgkcjxx;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 非公开转让成交信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwThirdFgkcjxxArray : public mp::RootPacket
{
public:
DealSno_def FGKCJXH; 		///< 成交序号
SecurityID_def FGKZQDM; 		///< 证券代码
SecuritySymbol_def FGKZQJC; 		///< 证券简称
Char2_def FGKZQLB; 		///< 证券类别
PBUID_def FGKBJYDY; 		///< 买入交易单元
PBUName_def FGKBDYMC; 		///< 买入单元名称
PBUID_def FGKSJYDY; 		///< 卖出交易单元
PBUName_def FGKSDYMC; 		///< 卖出单元名称
Qty_def FGKCJSL; 		///< 成交数量
Price_def FGKCJJG; 		///< 成交价格
TimeC6_def FGKCJSJ; 		///< 成交时间
Char_def FGKBYBZ; 		///< 备用标志
/*
 * @brief 默认构造函数
 */
  AgwThirdFgkcjxxArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 40 + 2 + 6 + 128 + 6 + 128 + 8 + 8 + 6 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdFgkcjxxArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 非公开转让成交信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdFgkcjxx : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwThirdFgkcjxxArray> order_array; 		///< 非公开转让成交信息查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdFgkcjxx();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdFgkcjxx& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdFgkcjxx;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief AGW新三板发行业务认购结果查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdIssueSubscribeResult : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def underlying_security_id; 		///< 正股证券代码
Date8_def order_date; 		///< 委托日期
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdIssueSubscribeResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdIssueSubscribeResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdIssueSubscribeResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板发行业务认购结果查询组合
 */
class EXPORT_MACRO_DEFINE AgwThirdIssueSubscribeResultArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecurityID_def underlying_security_id; 		///< 正股证券代码
Date8_def order_date; 		///< 委托日期
OrderID_def order_id; 		///< 合同序号
Date8_def subscribe_date; 		///< 成交日期，询价结果生成的日期
Qty_def subscribe_qty; 		///< 成交数量
Qty_def subscribe_qty2; 		///< 他数量2，为正数时表示网下投资者申购时的最大申购倍数
Price_def subscribe_price; 		///< 成交价格，禁止参与申购时为0
ThirdSubscribeResult_def subscribe_result; 		///< 认购结果 0：可以参与申购 1：禁止参与申购 2：必须参与申购
Date8_def insert_date; 		///< 插入日期
SecuritySymbol_def security_symbol; 		///< 证券简称
Sbyy_def sbyy; 		///< 失败原因
/*
 * @brief 默认构造函数
 */
  AgwThirdIssueSubscribeResultArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 8 + 8 + 16 + 8 + 8 + 8 + 8 + 1 + 8 + 40 + 2;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdIssueSubscribeResultArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief AGW新三板发行业务认购结果查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultThirdIssueSubscribeResult : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwThirdIssueSubscribeResultArray> order_array; 		///< 新三板发行业务认购结果查询组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultThirdIssueSubscribeResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultThirdIssueSubscribeResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultThirdIssueSubscribeResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转要约收购行情查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdTenderOfferSecurityInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场-109
SecurityID_def security_id; 		///< 证券代码-填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdTenderOfferSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdTenderOfferSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdTenderOfferSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 股转要约收购行情查询组合
 */
class EXPORT_MACRO_DEFINE AgwThirdTenderOfferSecurityInfoArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场-109
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
SecurityID_def underlying_security_id; 		///< 正股代码
Qty_def zr_unit; 		///< 转让单位
Qty_def sg_qty; 		///< 本次要约收购量
Date8_def yysg_begin_date; 		///< 要约收购开始日期
Date8_def yysg_end_date; 		///< 要约收购认购结束日期
Qty_def max_qty; 		///< 每笔限量
Price_def yysg_price; 		///< 要约收购价格
Qty_def buy_unit; 		///< 买数量单位
Qty_def sale_unit; 		///< 卖数量单位
Qty_def zxsb_unit; 		///< 最小申报数量
Char_def zrlx; 		///< 转让类型
Char_def tpbz; 		///< 停牌标志
Char_def yysd; 		///< 要约时段
Char_def grade; 		///< 层级信息（0-基础层，1-创新层，2-北交所）
/*
 * @brief 默认构造函数
 */
  AgwThirdTenderOfferSecurityInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 1 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdTenderOfferSecurityInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 股转要约收购行情查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdTenderOfferSecurityInfo : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwThirdTenderOfferSecurityInfoArray> order_array; 		///< 新三板发行业务认购结果查询组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdTenderOfferSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdTenderOfferSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdTenderOfferSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 要约收购最大可委托数量查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryThirdTenderOfferMaxOrderQty : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
Price_def price; 		///< 委托价格
ShareNature_def share_nature; 		///< 证券性质
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryThirdTenderOfferMaxOrderQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryThirdTenderOfferMaxOrderQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryThirdTenderOfferMaxOrderQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 要约收购最大可委托数量查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdTenderOfferMaxOrderQty : public AgwMsgExtQueryResultBase
{
public:
Qty_def max_order_qty; 		///< 最大可委托数量
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdTenderOfferMaxOrderQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdTenderOfferMaxOrderQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdTenderOfferMaxOrderQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗行情查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradeMdQuery : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 证券所属市场
BSFlag2_def bs_flag2; 		///< 买卖类别
Contactor_def contactor; 		///< 联系人
ConfirmID_def confirm_id; 		///< 约定号
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradeMdQuery();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradeMdQuery& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradeMdQuery;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 大宗交易行情
 */
class EXPORT_MACRO_DEFINE BlockTradeMarketData : public mp::RootPacket
{
public:
SeqNum_def apply_seq_no; 		///< 消息序号
SecurityID_def security_id; 		///< 合约编码
MarketID_def market_id; 		///< 证券所属市场
Qty_def order_qty; 		///< 委托数量
Price_def order_price; 		///< 委托价格
BSFlag2_def bs_flag2; 		///< 买卖类别
BSFlag2_def md_id; 		///< 指令类别
SerialNo_def serial_no; 		///< 申报合同序号
Contactor_def contactor; 		///< 联系人
ContactInfo_def contact_info; 		///< 联系人信息
DateTime_def transaction_time; 		///< 申报时间
ConfirmID_def confirm_id; 		///< 约定号
/*
 * @brief 默认构造函数
 */
  BlockTradeMarketData();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 2 + 8 + 8 + 2 + 2 + 32 + 12 + 30 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const BlockTradeMarketData& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 大宗交易行情查询回报
 */
class EXPORT_MACRO_DEFINE AgwMsgBlockTradeMdQueryResult : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<BlockTradeMarketData> block_trade_md_array; 		///< 大宗交易行情
/*
 * @brief 默认构造函数
 */
  AgwMsgBlockTradeMdQueryResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBlockTradeMdQueryResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBlockTradeMdQueryResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief AGW新三板发行证券信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdIssueSecurityInfo : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 证券所属市场
Char_def subscribe_purchase_status; 		///< 认申购状态
Date8_def order_date; 		///< 委托日期
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdIssueSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdIssueSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdIssueSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板发行业务证券信息查询组合
 */
class EXPORT_MACRO_DEFINE AgwThirdIssueSecurityInfoArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecurityID_def underlying_security_id; 		///< 正股证券代码
Date8_def subscribe_start_date; 		///< 认购起始时间
Date8_def subscribe_end_date; 		///< 认购结束时间
Qty_def subscribe_min_qty; 		///< 最小询价数量
Qty_def subscribe_max_qty; 		///< 最大询价数量
Price_def subscribe_min_price; 		///< 认购下限价格
Price_def subscribe_max_price; 		///< 认购上限价格
Qty_def subscribe_purchase_unit; 		///< 认申购单元
Date8_def purchase_date; 		///< 申购日期
Qty_def purchase_min_qty; 		///< 最小申购数量
Qty_def purchase_max_qty; 		///< 最大申购数量
Price_def purchase_min_price; 		///< 申购下限价格
Price_def purchase_max_price; 		///< 申购上限价格
Qty_def issued_total_qty; 		///< 总发行量
SecuritySymbol_def security_symbol; 		///< 证券简称
Char_def subscribe_purchase_status; 		///< 认申购状态
Date8_def update_date; 		///< 更新日期
Qty_def subscribe_confirm_min_qty; 		///< 询价确认最小申购数量
Qty_def subscribe_confirm_max_qty; 		///< 询价确认最大申购数量
ThirdSubscribeResult_def subscribe_confirm_result; 		///< 认购结果 0：可以参与申购 1：禁止参与申购 2：必须参与申购 3:未参与询价
OrdStatus_def order_status; 		///< 委托状态 0：未申报 1：已申报
IssueFlag_def fxfs; 		///< 发行方式，1-询价发行 2-定价发行 3-竞价发行
MktType_def mkt_type; 		///< 交易所证券类别
MktType_def mkt_sub_type; 		///< 交易所证券子类别
/*
 * @brief 默认构造函数
 */
  AgwThirdIssueSecurityInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 40 + 1 + 8 + 8 + 8 + 1 + 1 + 1 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwThirdIssueSecurityInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief AGW新三板发行证券信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryThirdIssueSecurityInfoResult : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwThirdIssueSecurityInfoArray> order_array; 		///< 新三板发行证券信息查询组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryThirdIssueSecurityInfoResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryThirdIssueSecurityInfoResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryThirdIssueSecurityInfoResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新股发行申购额度统一查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryUniIPOQty : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryUniIPOQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryUniIPOQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryUniIPOQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新股发行申购额度查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwUniIPOQtyArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 信用股东账号
PBUID_def seat; 		///< 席位代码
Qty_def max_qty; 		///< 申购额度
Qty_def max_qty1; 		///< 科创板申购额度
/*
 * @brief 默认构造函数
 */
  AgwUniIPOQtyArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 12 + 6 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwUniIPOQtyArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 新股发行申购额度查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultUniIPOQty : public AgwMsgExtQueryResultBase
{
public:
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwUniIPOQtyArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultUniIPOQty();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultUniIPOQty& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultUniIPOQty;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网络投票订单查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwNetVotingOrderArray : public mp::RootPacket
{
public:
ClOrdID_def cl_ord_id; 		///< 报盘合同号，客户报价消息编号
ClOrdNO_def cl_ord_no; 		///< 客户合同号,同一个客户资金账号当天唯一
TimeStamp_def transact_time; 		///< 客户委托时间
Currency_def currency; 		///< 货币种类
PBUID_def pbu_id; 		///< 投资者所属交易单元,交易席位
BSFlag2_def bs_flag2; 		///< 买卖类别
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
SecurityFullSymbol_def security_full_symbol; 		///< 证券长名称
GeneralMeetingSeq_def general_meeting_code; 		///< 股东大会编码
MeetingDesc_def meeting_desc; 		///< 股东大会名称
VotingProposal_def voting_proposal; 		///< 投票议案编号
VotingProposalDesc_def voting_prososal_desc; 		///< 投票议案名称
VotingProposalType_def voting_proposal_type; 		///< 议案类型
VotingPreference_def voting_preference; 		///< 投票意向
OrdStatus_def ord_status; 		///< 订单状态
Remark_def remark; 		///< 备注信息
Qty_def order_qty; 		///< 投票数量
/*
 * @brief 默认构造函数
 */
  AgwNetVotingOrderArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 10 + 8 + 8 + 4 + 6 + 2 + 2 + 8 + 40 + 64 + 8 + 512 + 10 + 1024 + 1 + 1 + 1 + 32 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwNetVotingOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 网络投票订单查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryNetVotingOrder : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
GeneralMeetingSeq_def general_meeting_code; 		///< 股东大会编码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryNetVotingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryNetVotingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryNetVotingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网络投票订单查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultNetVotingOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwNetVotingOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultNetVotingOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultNetVotingOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultNetVotingOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券协商成交委托
 */
class EXPORT_MACRO_DEFINE AgwMsgBondConsultTradeOrder : public AgwNewMsgOrder
{
public:
TradeReportType_def trade_report_type; 		///< 成交申报类型
TradeReportTransType_def trade_report_trans_type; 		///< 成交申报事务类别
ClOrdID_def trade_report_ref_id; 		///< 原客户成交申报编号
ConfirmID_def confirm_id; 		///< 约定号，一般取客户号去掉右边空格后的右边8位，客户号不足8位右补空格，由业务处理引擎生成
MemberID_def member_id; 		///< 发起方（本方）交易商代码
InvestorType_def investor_type; 		///< 发起方（本方）交易主体类型
InvestorID_def investor_id; 		///< 发起方（本方）交易主体代码
InvestorName_def investor_name; 		///< 发起方（本方）客户名称
TraderCode_def trader_code; 		///< 发起方（本方）交易员代码
MemberID_def counterparty_member_id; 		///< 对手方交易商代码
InvestorType_def counterparty_investor_type; 		///< 对手方交易主体类型
InvestorID_def counterparty_investor_id; 		///< 对手方交易主体代码
InvestorName_def counterparty_investor_name; 		///< 对手方客户名称
TraderCode_def counterparty_trader_code; 		///< 对手方交易员代码
SettlType_def settl_type; 		///< 结算方式
SettlPeriod_def settl_period; 		///< 结算周期
Price_def last_par_px; 		///< 成交价格二，预留
Bool_def accept_subterm; 		///< 是否同意补偿条款0-不同意1-同意（是和客户做交易非券商交易的补充条款）
Memo_def memo; 		///< 备注
/*
 * @brief 默认构造函数
 */
  AgwMsgBondConsultTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondConsultTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBondConsultTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券协商成交委托撤单委托
 */
class EXPORT_MACRO_DEFINE AgwMsgBondConsultTradeCancelOrder : public AgwMsgOrder
{
public:
TradeReportType_def trade_report_type; 		///< 成交申报类型
TradeReportTransType_def trade_report_trans_type; 		///< 成交申报事务类别
MemberID_def member_id; 		///< 本方交易商代码
InvestorType_def investor_type; 		///< 本方交易主体类型
InvestorID_def investor_id; 		///< 本方交易主体代码
InvestorName_def investor_name; 		///< 本方客户名称
TraderCode_def trader_code; 		///< 本方交易员代码
MemberID_def counterparty_member_id; 		///< 对手方交易商代码
InvestorType_def counterparty_investor_type; 		///< 对手方交易主体类型
InvestorID_def counterparty_investor_id; 		///< 对手方交易主体代码
InvestorName_def counterparty_investor_name; 		///< 对手方客户名称
TraderCode_def counterparty_trader_code; 		///< 对手方交易员代码
SettlType_def settl_type; 		///< 结算方式
SettlPeriod_def settl_period; 		///< 结算周期
Price_def last_par_px; 		///< 成交价格二,单位：元(精确到0.0001元)
Memo_def memo; 		///< 备注
/*
 * @brief 默认构造函数
 */
  AgwMsgBondConsultTradeCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondConsultTradeCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBondConsultTradeCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券协商成交委托成交回报
 */
class EXPORT_MACRO_DEFINE AgwMsgBondConsultTradeTER : public AgwMsgTradeExecutionReportSeqNum
{
public:
MemberID_def member_id; 		///< 本方交易商代码
InvestorType_def investor_type; 		///< 本方交易主体类型
InvestorID_def investor_id; 		///< 本方交易主体代码
InvestorName_def investor_name; 		///< 本方客户名称
TraderCode_def trader_code; 		///< 本方交易员代码
MemberID_def counterparty_member_id; 		///< 对手方交易商代码
InvestorType_def counterparty_investor_type; 		///< 对手方交易主体类型
InvestorID_def counterparty_investor_id; 		///< 对手方交易主体代码
InvestorName_def counterparty_investor_name; 		///< 对手方客户名称
TraderCode_def counterparty_trader_code; 		///< 对手方交易员代码
SettlType_def settl_type; 		///< 结算方式
SettlPeriod_def settl_period; 		///< 结算周期
CashMargin_def cash_margin; 		///< 信用标识
Memo_def memo; 		///< 备注
/*
 * @brief 默认构造函数
 */
  AgwMsgBondConsultTradeTER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondConsultTradeTER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgBondConsultTradeTER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券协商成交转发成交申报查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryOrderForwardInfo : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 证券所属市场必填
Side_def side; 		///< 买卖方向
OrdStatus_def ord_status; 		///< 订单当前申报的状态
MemberID_def counterparty_member_id; 		///< 对手方交易商代码
InvestorID_def counterparty_investor_id; 		///< 对手方交易主体代码
TraderCode_def counterparty_trader_code; 		///< 对手方交易员代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryOrderForwardInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryOrderForwardInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryOrderForwardInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券协商成交转发成交申报重复组信息
 */
class EXPORT_MACRO_DEFINE AgwOrderForwardInfoArray : public mp::RootPacket
{
public:
ConfirmID_def confirm_id; 		///< 约定号，一般取客户号去掉右边空格后的右边8位，客户号不足8位右补空格，由业务处理引擎生成
OrdStatus_def ord_status; 		///< 订单当前申报的状态
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
OwnerType_def owner_type; 		///< 订单所有者类型
TimeStamp_def transact_time; 		///< 订单处理时间
UserInfo_def user_info; 		///< 用户私有信息
ClOrdID_def trade_report_id; 		///< 客户成交申报编号
TradeReportType_def trade_report_type; 		///< 成交申报类型
TradeReportTransType_def trade_report_trans_type; 		///< 成交申报事务类别
TradeHandlingInstr_def trade_handling_instr; 		///< 成交申报模式
ClOrdID_def trade_report_ref_id; 		///< 原客户成交申报编号
Price_def last_px; 		///< 价格,单位：元(精确到0.0001元)
Qty_def last_qty; 		///< 数量
Side_def side; 		///< 买卖方向
PBUID_def pbu_id; 		///< 本方交易单元
ExecID_def exec_id; 		///< 执行编号
AccountID_def account_id; 		///< 本方证券账户
BranchID_def branch_id; 		///< 本方营业部代码
MemberID_def member_id; 		///< 本方交易商代码
InvestorType_def investor_type; 		///< 本方交易主体类型
InvestorID_def investor_id; 		///< 本方交易主体代码
InvestorName_def investor_name; 		///< 本方客户名称
TraderCode_def trader_code; 		///< 本方交易员代码
MemberID_def counterparty_member_id; 		///< 对手方交易商代码
InvestorType_def counterparty_investor_type; 		///< 对手方交易主体类型
InvestorID_def counterparty_investor_id; 		///< 对手方交易主体代码
InvestorName_def counterparty_investor_name; 		///< 对手方客户名称
TraderCode_def counterparty_trader_code; 		///< 对手方交易员代码
SettlType_def settl_type; 		///< 结算方式
SettlPeriod_def settl_period; 		///< 结算周期
CashMargin_def cash_margin; 		///< 信用标识
Memo_def memo; 		///< 备注
/*
 * @brief 默认构造函数
 */
  AgwOrderForwardInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 1 + 8 + 40 + 2 + 2 + 8 + 64 + 10 + 1 + 1 + 1 + 10 + 8 + 8 + 1 + 6 + 32 + 12 + 10 + 6 + 2 + 10 + 120 + 8 + 6 + 2 + 10 + 120 + 8 + 2 + 1 + 1 + 160;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwOrderForwardInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 债券协商成交转发成交申报查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultOrderForwardInfo : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 证券信息总数
std::vector<AgwOrderForwardInfoArray> order_array; 		///< 查询组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultOrderForwardInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultOrderForwardInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultOrderForwardInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 交易商信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryBondMember : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场编码
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryBondMember();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryBondMember& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryBondMember;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 交易商信息组合
 */
class EXPORT_MACRO_DEFINE AgwMsgBondMemberArray : public mp::RootPacket
{
public:
MemberID_def member_id; 		///< 交易商代码
String_def short_name; 		///< 交易商简称
String_def name; 		///< 交易商名称
/*
 * @brief 默认构造函数
 */
  AgwMsgBondMemberArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondMemberArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 交易员信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultBondMember : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwMsgBondMemberArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultBondMember();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultBondMember& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultBondMember;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 交易信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryTraderInvestor : public AgwMsgExtQueryBase
{
public:
MemberID_def member_id; 		///< 交易商代码
MarketID_def market_id; 		///< 市场编码
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryTraderInvestor();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryTraderInvestor& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryTraderInvestor;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 交易员信息组合
 */
class EXPORT_MACRO_DEFINE AgwMsgBondTraderArray : public mp::RootPacket
{
public:
TraderCode_def trader_code; 		///< 交易员代码
String_def trader_name; 		///< 交易员名称
Bool_def is_default_trader; 		///< 是否经纪业务缺省交易员
/*
 * @brief 默认构造函数
 */
  AgwMsgBondTraderArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondTraderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 交易主体信息组合
 */
class EXPORT_MACRO_DEFINE AgwMsgBondInvestorArray : public mp::RootPacket
{
public:
InvestorID_def investor_id; 		///< 交易主体代码
String_def investor_name; 		///< 交易主体代码名称
String_def investor_short_name; 		///< 交易主体代码简称
InvestorType_def investor_type; 		///< 交易主体类型
/*
 * @brief 默认构造函数
 */
  AgwMsgBondInvestorArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgBondInvestorArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 交易主体信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultTraderInvestor : public AgwMsgExtQueryResultBase
{
public:
MemberID_def member_id; 		///< 交易商代码
String_def short_name; 		///< 交易商简称
String_def name; 		///< 交易商名称
std::vector<AgwMsgBondTraderArray> trader_array; 		///< 查询结果组合
std::vector<AgwMsgBondInvestorArray> investor_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultTraderInvestor();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultTraderInvestor& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultTraderInvestor;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 0;}
};
/*
 * @brief 交易商信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryBondMember : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryBondMember();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryBondMember& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryBondMember;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 交易商信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultBondMember : public AgwMsgExtQueryResultBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultBondMember();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultBondMember& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultBondMember;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 交易信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryBondInfo : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryBondInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryBondInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryBondInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 交易信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultBondInfo : public AgwMsgExtQueryResultBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultBondInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultBondInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultBondInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板在途资产数据查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryThirdAdjustAsset : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场-固定填109
Currency_def currency; 		///< 货币类型，填空查全部
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryThirdAdjustAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryThirdAdjustAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryThirdAdjustAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF篮子股票买卖委托
 */
class EXPORT_MACRO_DEFINE AgwMsgETFBasketOrder : public AgwNewMsgOrder
{
public:
PriceKind_def price_kind; 		///< ETF篮子价格类型
TacticsKind_def tactics_kind; 		///< 策略类型
HitRate_def hit_rate; 		///< 命中率
OrderGroup_def order_group; 		///< 批量委托编号
ExtBankCode_def bank_code; 		///< 外部银行
Comments_def remark; 		///< 备注信息
Bool_def sell_all; 		///< 是否卖出全体现有成分股
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgETFBasketOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFBasketOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFBasketOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF篮子股票买卖订单执行报告
 */
class EXPORT_MACRO_DEFINE AgwMsgETFBasketTradeER : public AgwMsgTradeExecutionReportSeqNum
{
public:
OrderGroup_def order_group; 		///< 批量委托编号
Qty_def order_count; 		///< 发送委托笔数
/*
 * @brief 默认构造函数
 */
  AgwMsgETFBasketTradeER();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgETFBasketTradeER& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgETFBasketTradeER;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF网下认购（股票和现金）
 */
class EXPORT_MACRO_DEFINE AgwMsgOfflineETFSubscribe : public AgwNewMsgOrder
{
public:
SecurityID_def etf_security_id; 		///< etf代码
MarketID_def etf_market_id; 		///< etf市场
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgOfflineETFSubscribe();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOfflineETFSubscribe& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgOfflineETFSubscribe;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网下ETF认购撤销申请
 */
class EXPORT_MACRO_DEFINE AgwMsgOfflineETFSubscribeCancelOrder : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
String_def password; 		///< 用户密码
BranchID_def branch_id; 		///< 营业部代码
OrderWay_def order_way; 		///< 委托方式
Date8_def sys_date; 		///< 系统日期
ClOrdNO_def sno; 		///< 订单编号
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgOfflineETFSubscribeCancelOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOfflineETFSubscribeCancelOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgOfflineETFSubscribeCancelOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网下ETF认购撤销申请响应
 */
class EXPORT_MACRO_DEFINE AgwMsgOfflineETFSubscribeCancelAck : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
Date8_def sys_date; 		///< 系统日期
ClOrdNO_def sno; 		///< 订单编号
/*
 * @brief 默认构造函数
 */
  AgwMsgOfflineETFSubscribeCancelAck();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOfflineETFSubscribeCancelAck& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgOfflineETFSubscribeCancelAck;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网下ETF认购查询股票可认购数量
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryOfflineETFSubscribeQtyBySecurity : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def etf_security_id; 		///< etf证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryOfflineETFSubscribeQtyBySecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryOfflineETFSubscribeQtyBySecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryOfflineETFSubscribeQtyBySecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网下ETF认购查询股票可认购数量查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwMsgOfflineETFSubscribeQtyBySecurityArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Qty_def order_qty; 		///< 可认购数量
/*
 * @brief 默认构造函数
 */
  AgwMsgOfflineETFSubscribeQtyBySecurityArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOfflineETFSubscribeQtyBySecurityArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 网下ETF认购查询股票可认购数量查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryResultOfflineETFSubscribeQtyBySecurity : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
MarketID_def etf_market_id; 		///< 交易市场
SecurityID_def etf_security_id; 		///< etf证券代码
AccountID_def account_id; 		///< 账户ID
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
std::vector<AgwMsgOfflineETFSubscribeQtyBySecurityArray> order_array; 		///< 网下ETF认购查询股票可认购数量查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryResultOfflineETFSubscribeQtyBySecurity();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryResultOfflineETFSubscribeQtyBySecurity& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryResultOfflineETFSubscribeQtyBySecurity;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新三板在途资产数据查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThirdAdjustAsset : public AgwMsgExtQueryResultBase
{
public:
Amt_def adjust_amt; 		///< 在途资产
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThirdAdjustAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThirdAdjustAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThirdAdjustAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通证券行情信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryHKTMd : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryHKTMd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryHKTMd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryHKTMd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 港股通证券行情信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultHKTMd : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 账户ID
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
Price_def bid_price_limit_up; 		///< 买盘上限价
Price_def bid_price_limit_down; 		///< 买盘下限价
Price_def offer_price_limit_up; 		///< 卖盘上限价
Price_def offer_price_limit_down; 		///< 卖盘下限价
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultHKTMd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultHKTMd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultHKTMd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 要约收购人信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryOfferTenderer : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场代码-固定填102深圳市场
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryOfferTenderer();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryOfferTenderer& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryOfferTenderer;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 要约收购人信息
 */
class EXPORT_MACRO_DEFINE AgwMsgOfferTendererInfo : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Tenderer_def tenderer_id; 		///< 要约收购参与人编码
String_def tenderer_name; 		///< 收购人名称
Price_def offering_price; 		///< 要约收购价格
Date8_def start_date; 		///< 收购起始日
Date8_def end_date; 		///< 收购截止日
/*
 * @brief 默认构造函数
 */
  AgwMsgOfferTendererInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOfferTendererInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 要约收购人信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultOfferTenderer : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwMsgOfferTendererInfo> order_array; 		///< 要约收购人信息重复组
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultOfferTenderer();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultOfferTenderer& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultOfferTenderer;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 限售股信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwSaleLimitStkAssetArray : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户ID
CustName_def cust_name; 		///< 客户姓名
BranchID_def branch_id; 		///< 机构编码
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 市场ID
MarketDesc_def market_des; 		///< 市场信息描述
SecurityID_def security_id; 		///< 证券代码
Qty_def stk_bal; 		///< 股份余额
Qty_def stk_avl; 		///< 股份可用
ShareNature_def share_nature; 		///< 股份性质
SaleLimitSecurityType_def sale_limit_security_type; 		///< 证券类别
CirculationType_def circulation_type; 		///< 流通类型
RightType_def right_type; 		///< 权益类别
ListingYear_def listing_year; 		///< 挂牌年份
PBUID_def pbu_id; 		///< 席位代码
Price_def last_price; 		///< 最新价
Date8_def sys_date; 		///< 导入日期
/*
 * @brief 默认构造函数
 */
  AgwSaleLimitStkAssetArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 64 + 10 + 12 + 2 + 32 + 8 + 8 + 8 + 2 + 2 + 1 + 2 + 4 + 6 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSaleLimitStkAssetArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 限售股信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQuerySaleLimitStkAsset : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
ShareNature_def share_nature; 		///< 股份性质,填空查所有
CirculationType_def circulation_type; 		///< 流通类型,填空查所有
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQuerySaleLimitStkAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQuerySaleLimitStkAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQuerySaleLimitStkAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 限售股信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultSaleLimitStkAsset : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwSaleLimitStkAssetArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultSaleLimitStkAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultSaleLimitStkAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultSaleLimitStkAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网下ETF认购查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryETFCreation : public AgwMsgExtQueryBase
{
public:
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
AssetFlag_def asset_flag; 		///< 非撤单未申报网下现金标识
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryETFCreation();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryETFCreation& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryETFCreation;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 网下ETF认购查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryETFCreationArray : public mp::RootPacket
{
public:
ClOrdNO_def sno; 		///< 订单编号
Date8_def sys_date; 		///< 系统日期
Date8_def oper_date; 		///< 发生日期
Time9_def oper_time; 		///< 发生时间
Currency_def currency; 		///< 货币代码
AccountID_def account_id; 		///< 股东代码
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_name; 		///< 证券简称
SecurityFullSymbol_def security_full_symbol; 		///< 证券全称
Side_def side; 		///< 买卖方向
Qty_def order_qty; 		///< 证券发生数
Price_def order_price; 		///< 委托价格
Amt_def order_frz_amt; 		///< 委托冻结金额
OperWay_def oper_way; 		///< 操作方式
Char_def cancel_flag; 		///< 撤销标志
Char_def status; 		///< 处理状态
SecurityID_def frz_code; 		///< 股票认购冻结证券代码
SecuritySymbol_def frz_name; 		///< 股票认购冻结证券名称
Date8_def order_date; 		///< 委托日期
ClOrdNO_def order_sno; 		///< 委托序号
PBUID_def pbuid; 		///< 席位
FeeRate_def commission_rate; 		///< 股票认购费率
Date8_def match_date; 		///< 成交日期
ClOrdNO_def match_sno; 		///< 成交流水号
MarketID_def market_id_stk; 		///< 成分股市场
BSFlag2_def bsflag; 		///< 集中交易买卖类别
SecurityID_def of_code; 		///< 基准基金代码
SecuritySymbol_def of_name; 		///< 基准基金名称
Char_def input_bs; 		///< 输入买卖标志
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryETFCreationArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 8 + 8 + 4 + 12 + 2 + 8 + 40 + 64 + 1 + 8 + 8 + 8 + 1 + 1 + 1 + 8 + 40 + 8 + 8 + 6 + 8 + 8 + 8 + 2 + 2 + 8 + 40 + 1 + 2;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryETFCreationArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 网下ETF认购查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryResultETFCreation : public AgwMsgExtQueryResultBase
{
public:
CustomerKind_def cust_kind; 		///< 客户类别
CustomerGroup_def cust_group; 		///< 客户分组
CustName_def cust_name; 		///< 客户姓名
FundKind_def fund_kind; 		///< 资金类别
FundLevel_def fund_level; 		///< 资金级别
FundGroup_def fund_group; 		///< 资金分组
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwMsgExtQryETFCreationArray> order_array; 		///< 网下ETF认购查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryResultETFCreation();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryResultETFCreation& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryResultETFCreation;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 基金风险揭示签署
 */
class EXPORT_MACRO_DEFINE AgwMsgOfRiskSign : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
String_def password; 		///< 用户密码
BranchID_def branch_id; 		///< 营业部代码
/*
 * @brief 默认构造函数
 */
  AgwMsgOfRiskSign();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOfRiskSign& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgOfRiskSign;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 基金风险揭示签署结果
 */
class EXPORT_MACRO_DEFINE AgwMsgOfRiskSignAck : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
RejectReasonCode_def reject_reason_code; 		///< 拒绝原因码
UserInfo_def user_info; 		///< 用户私有信息
BranchID_def branch_id; 		///< 营业部代码
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgOfRiskSignAck();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgOfRiskSignAck& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgOfRiskSignAck;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 外围深圳场内LOF基金分红方式设置
 */
class EXPORT_MACRO_DEFINE AgwMsgLOFDividePolicySetOrder : public AgwNewMsgOrder
{
public:
DividePolicy_def dividend_policy; 		///< 分红方式
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgLOFDividePolicySetOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgLOFDividePolicySetOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgLOFDividePolicySetOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF客户股票篮查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryEtfStkAsset : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< ETF交易市场
Qty_def leaves_qty; 		///< 篮子数量
SecurityID_def etf_security_id; 		///< ETF代码
ReportIndex_def query_index; 		///< 指定起始查询索引号, 将返回索引号以后的订单, 填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量, 填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryEtfStkAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryEtfStkAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryEtfStkAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF客户股票篮查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwEtfStkAssetArray : public mp::RootPacket
{
public:
MarketID_def etf_market_id; 		///< ETF基金市场ID
SecurityID_def etf_security_id; 		///< ETF基金代码
SecuritySymbol_def etf_security_symbol; 		///< ETF名称
SecurityID_def component_security_id; 		///< 成分股代码
SecuritySymbol_def component_security_symbol; 		///< 成分股简称
Qty_def component_share_qty; 		///< 成分股数量
SubstituteFlag_def substitute_flag; 		///< 现金替代标志
PremiumRatio_def premium_ratio; 		///< 申购溢价比例
Amt_def purchase_cash_substitute; 		///< 申购现金替代金额
Qty_def stk_bal; 		///< 股份余额
Qty_def stk_avl; 		///< 股份可用
Qty_def stk_diff; 		///< 可申购数量
Qty_def stk_differentamount; 		///< 缺少数量
Qty_def stk_needamount; 		///< 需买数量
Char_def stk_stop_flag; 		///< 停牌标志，F-正常、T-停牌
Price_def stk_replace_price; 		///< 参考替代价格
/*
 * @brief 默认构造函数
 */
  AgwEtfStkAssetArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 40 + 8 + 1 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwEtfStkAssetArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief ETF客户股票篮查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultEtfStkAsset : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwEtfStkAssetArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultEtfStkAsset();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultEtfStkAsset& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultEtfStkAsset;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF查询客户当日批量委托
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryEtfBatchOrder : public AgwMsgExtQueryBase
{
public:
SecurityID_def etf_security_id; 		///< ETF基金代码
ReportIndex_def query_index; 		///< 指定起始查询索引号, 将返回索引号以后的订单, 填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量, 填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryEtfBatchOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryEtfBatchOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryEtfBatchOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief ETF查询客户当日批量委托结果组合
 */
class EXPORT_MACRO_DEFINE AgwEtfBatchOrderArray : public mp::RootPacket
{
public:
MarketID_def etf_market_id; 		///< ETF基金市场ID
SecurityID_def etf_security_id; 		///< ETF基金代码
FundAccountID_def fund_account_id; 		///< 资金账户ID
AccountID_def account_id; 		///< 股东代码
Date8_def order_date; 		///< 委托日期
OrderGroup_def order_group; 		///< 批量委托编号
Side_def side; 		///< 买卖方向
TimeStamp_def order_time; 		///< 委托时间
PriceKind_def price_kind; 		///< ETF篮子价格类型
Qty_def order_qty; 		///< 篮子数量
TacticsKind_def tactics_kind; 		///< 策略类型
HitRate_def hit_rate; 		///< 命中率
Qty_def order_count; 		///< 委托数量
Qty_def fail_count; 		///< 委托失败数量
ETFBasketCanCancelFlg_def can_cancel_flag; 		///< ETF篮子委托可撤单标志 0:存在可撤委托   1：全部成交或撤单，不存在可撤委托
/*
 * @brief 默认构造函数
 */
  AgwEtfBatchOrderArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 16 + 12 + 8 + 8 + 1 + 8 + 1 + 8 + 2 + 8 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwEtfBatchOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief ETF查询客户当日批量委托查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultEtfBatchOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwEtfBatchOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultEtfBatchOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultEtfBatchOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultEtfBatchOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可入质债券查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryPledgeBond : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 市场ID
Side_def side; 		///< 买卖方向,H-入库I-出库
Qty_def order_qty; 		///< 委托数量
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryPledgeBond();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryPledgeBond& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryPledgeBond;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可入质债券查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwPledgeBondArray : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部代码
IssuerNo_def issuer_no; 		///< 发行人编号
String_def issuer_full_name; 		///< 发行人全称
IssuerOutsideRating_def issuer_outside_rating; 		///< 发行人外部评级
Char_def bond_type; 		///< 债券类别 0-利率债，1-信用债
Char_def is_gtja_bmbzq; 		///< 是否为国泰君安白名单债券 0-否，1-是
CreditRatio_def credit_bond_concentration; 		///< 单一发行人信用类债券入库担保集中度
CreditRatio_def credit_bond_concentration_default; 		///< 单一发行人信用类债券入库担保集中度上限
/*
 * @brief 默认构造函数
 */
  AgwPledgeBondArray();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwPledgeBondArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 可入质债券查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryResultPledgeBond : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwPledgeBondArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryResultPledgeBond();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryResultPledgeBond& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryResultPledgeBond;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新外围委托查询(LOF基金分红委托专用)查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryLOFDividePolicyOrder : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码,填空查所有
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryLOFDividePolicyOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryLOFDividePolicyOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryLOFDividePolicyOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新外围委托查询(LOF基金分红委托专用)查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwMsgLOFDividePolicyOrderArray : public mp::RootPacket
{
public:
Date8_def order_date; 		///< 委托日期
SecurityID_def security_id; 		///< 证券代码
DividePolicy_def dividend_policy; 		///< LOF基金分红方式
DividePolicyName_def dividend_policy_name; 		///< LOF基金分红方式名称
/*
 * @brief 默认构造函数
 */
  AgwMsgLOFDividePolicyOrderArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 1 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgLOFDividePolicyOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 新外围委托查询(LOF基金分红委托专用)查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultLOFDividePolicyOrder : public AgwMsgExtQueryResultBase
{
public:
MarketID_def market_id; 		///< 市场ID
PBUID_def pbu_id; 		///< 席位
Length_def data_length; 		///< 组合个数
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwMsgLOFDividePolicyOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultLOFDividePolicyOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultLOFDividePolicyOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultLOFDividePolicyOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户证券卖出费用查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQuerySaleFee : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券市场
SecurityID_def security_id; 		///< 证券代码
Qty_def qty; 		///< 计算数量
Amt_def order_amt; 		///< 委托金额
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQuerySaleFee();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQuerySaleFee& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQuerySaleFee;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户证券卖出费用查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultSaleFee : public AgwMsgExtQueryResultBase
{
public:
Amt_def total_fee; 		///< 交易总费用
MarginRiskRate_def commission_rate; 		///< 佣金率
Amt_def service_fee; 		///< 投顾费
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultSaleFee();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultSaleFee& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultSaleFee;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券质押式回购实际占款天数查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryDaysOfRepoFundOccupation : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场，0查全部
SecurityID_def security_id; 		///< 证券代码，空查全部
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryDaysOfRepoFundOccupation();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryDaysOfRepoFundOccupation& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryDaysOfRepoFundOccupation;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 债券质押式回购实际占款天数查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwMsgDaysOfRepoFundOccupationArray : public mp::RootPacket
{
public:
Date8_def order_date; 		///< 交易日期,YYYYmmDD
MarketID_def market_id; 		///< 市场
SecurityID_def security_id; 		///< 证券代码
Days_def occupy_days; 		///< 占款天数
/*
 * @brief 默认构造函数
 */
  AgwMsgDaysOfRepoFundOccupationArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 2 + 8 + 4;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgDaysOfRepoFundOccupationArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 债券质押式回购实际占款天数查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryRstDaysOfRepoFundOccupation : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwMsgDaysOfRepoFundOccupationArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryRstDaysOfRepoFundOccupation();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryRstDaysOfRepoFundOccupation& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryRstDaysOfRepoFundOccupation;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户新标准券查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryCustSTDBonds : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场,0查全部
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryCustSTDBonds();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryCustSTDBonds& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryCustSTDBonds;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户新标准券查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwMsgCustSTDBondsArray : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号ID
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部ID
MarketID_def market_id; 		///< 市场ID
AccountID_def account_id; 		///< 股东账户
Qty_def bond_first; 		///< 一级质押标准券
Qty_def bond_second; 		///< 二级质押标准券
Qty_def bb_back; 		///< 标准券解冻
Qty_def bb_frz; 		///< 标准券冻结
Qty_def bb_avl_market; 		///< 一级标准券可用
Qty_def bb_avl_secu; 		///< 二级标准券可用
Rate_def stk_rate_used; 		///< 实际标准券使用率
Rate_def stk_rate; 		///< 标准券使用率上限
/*
 * @brief 默认构造函数
 */
  AgwMsgCustSTDBondsArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 16 + 10 + 2 + 12 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgCustSTDBondsArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户新标准券查询回报
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQryRstCustSTDBonds : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwMsgCustSTDBondsArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQryRstCustSTDBonds();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQryRstCustSTDBonds& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQryRstCustSTDBonds;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 未到期回购查询
 */
class EXPORT_MACRO_DEFINE AgmMsgExternalQueryUnexpiredRepo : public AgwMsgExtQueryBase
{
public:
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
Bool_def match_this_day; 		///< 当天交易日到期是否作为未到期返回 1-是 0-否
/*
 * @brief 默认构造函数
 */
  AgmMsgExternalQueryUnexpiredRepo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgmMsgExternalQueryUnexpiredRepo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgmMsgExternalQueryUnexpiredRepo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 未到期回购
 */
class EXPORT_MACRO_DEFINE AgwMsgUnexpiredRepoOrder : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
BranchID_def branch_id; 		///< 营业部代码
FundAccountID_def fund_account_id; 		///< 资金账户ID
AccountID_def account_id; 		///< 账户ID
Date8_def order_date; 		///< 委托日期
TimeStamp_def order_time; 		///< 委托时间
Char32_def sno; 		///< 流水号
PBUID_def pbu_id; 		///< 报盘席位号
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Currency_def currency; 		///< 币种
BSFlag2_def bsflag2; 		///< 买卖类别
Price_def order_price; 		///< 委托价格
Qty_def order_qty; 		///< 委托数量
Amt_def match_amt; 		///< 成交金额
Date8_def match_date; 		///< 成交日期
TradeType_def trade_type; 		///< 交易类型
SecurityType_def security_type; 		///< 证券类别
Date8_def bb_match_date; 		///< 回购到期日期
Amt_def bb_match_clear_amt; 		///< 回购到期金额
Price_def match_price; 		///< 成交价格
Days_def gh_days; 		///< 购回天数
ContractID_def order_id; 		///< 合同序号
/*
 * @brief 默认构造函数
 */
  AgwMsgUnexpiredRepoOrder();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 16 + 10 + 16 + 12 + 8 + 8 + 32 + 6 + 2 + 8 + 40 + 4 + 2 + 8 + 8 + 8 + 8 + 1 + 2 + 8 + 8 + 8 + 4 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgUnexpiredRepoOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 未到期回购查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultUnexpiredRepo : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwMsgUnexpiredRepoOrder> order_array; 		///< 未到期回购重复组
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultUnexpiredRepo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultUnexpiredRepo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultUnexpiredRepo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 当日逆回购成交查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryReverseRepoTrade : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
ClOrdNO_def order_sno; 		///< 客户订单编号,同一个客户当天唯一,填0查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryReverseRepoTrade();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryReverseRepoTrade& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryReverseRepoTrade;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 逆回购成交委托
 */
class EXPORT_MACRO_DEFINE AgwMsgReverseRepoTradeOrder : public mp::RootPacket
{
public:
Date8_def order_date; 		///< 委托日期
ClOrdNO_def order_sno; 		///< 委托序号
Currency_def currency; 		///< 币种
MarketID_def market_id; 		///< 市场代码
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
Days_def gh_days; 		///< 购回天数
Price_def order_price; 		///< 委托价格
Qty_def order_qty; 		///< 委托数量
Amt_def order_frozen_amt; 		///< 冻结金额
Qty_def match_qty; 		///< 成交数量
Amt_def match_amt; 		///< 成交金额
Qty_def cancel_qty; 		///< 撤单数量
OrdStatus_def order_status; 		///< 订单状态
Price_def avg_match_price; 		///< 平均成交价格
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< 营业部代码
AccountID_def account_id; 		///< 账户ID
/*
 * @brief 默认构造函数
 */
  AgwMsgReverseRepoTradeOrder();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 8 + 4 + 2 + 8 + 40 + 4 + 8 + 8 + 8 + 8 + 8 + 8 + 1 + 8 + 16 + 16 + 10 + 12;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgReverseRepoTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 当日逆回购成交查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultReverseRepoTrade : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwMsgReverseRepoTradeOrder> order_array; 		///< 逆回购成交委托重复组
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultReverseRepoTrade();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultReverseRepoTrade& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultReverseRepoTrade;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief PB专用订单查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryPBOrder : public AgwMsgExtQueryBase
{
public:
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,同一个客户当天唯一,填0查所有
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
Side_def side; 		///< 买卖方向,填0查所有
OrderQueryCondition_def order_query_condition; 		///< 委托查询条件
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryPBOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryPBOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryPBOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief PB订单查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultPBOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultPBOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultPBOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultPBOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief PB成交查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryPBTradeOrder : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,填0查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryPBTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryPBTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryPBTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief PB成交查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultPBTradeOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 成交记录总数
std::vector<AgwTradeOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultPBTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultPBTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultPBTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资产冻结包查询接口
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryAssetFrzPackage : public AgwMsgExtQueryBase
{
public:
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryAssetFrzPackage();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryAssetFrzPackage& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryAssetFrzPackage;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 资产冻结包查询结果
 */
class EXPORT_MACRO_DEFINE AgwAssetFrzPackageArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
Qty_def stk_frz_vip; 		///< 股份冻结数量
/*
 * @brief 默认构造函数
 */
  AgwAssetFrzPackageArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwAssetFrzPackageArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 资产冻结包查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAssetFrzPackage : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 客户一级锁券记录总数
std::vector<AgwAssetFrzPackageArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAssetFrzPackage();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAssetFrzPackage& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAssetFrzPackage;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可转债强赎证券信息查询接口
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryConvertibleBondForcedRedemption : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券ID
MarketID_def market_id; 		///< 交易市场
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryConvertibleBondForcedRedemption();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryConvertibleBondForcedRedemption& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryConvertibleBondForcedRedemption;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 可转债强赎证券信息结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultConvertibleBondForcedRedemption : public AgwMsgExtQueryResultBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
Date8_def full_redemption_date; 		///< 全部赎回日期
Date8_def last_transaction_date; 		///< 最后交易日期
Date8_def last_swap_date; 		///< 最后转股日期
ConvertibleBondPlateClass_def convertible_bond_plate_class; 		///< 可转债板块分类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultConvertibleBondForcedRedemption();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultConvertibleBondForcedRedemption& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultConvertibleBondForcedRedemption;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 沪市先行赔付信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryAdvanceRedressInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券ID
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryAdvanceRedressInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryAdvanceRedressInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryAdvanceRedressInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 沪市先行赔付信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwAdvanceRedressArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 证券账户
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
Qty_def redress_amt; 		///< 赔偿金额
RedressFlag_def redress_flag; 		///< 执行状态
Date8_def confirm_date; 		///< 确认日期
Date8_def account_date; 		///< 到账日期
Remark_def remark; 		///< 备注
/*
 * @brief 默认构造函数
 */
  AgwAdvanceRedressArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 40 + 8 + 1 + 8 + 8 + 32;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwAdvanceRedressArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 沪市先行赔付信息查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultAdvanceRedressInfo : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwAdvanceRedressArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultAdvanceRedressInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultAdvanceRedressInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultAdvanceRedressInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新国债逆回购持仓查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryNewReverseRepoPosition : public AgwMsgExtQueryBase
{
public:
Bool_def exp_flag; 		///< 是否查询今日到期合约
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryNewReverseRepoPosition();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryNewReverseRepoPosition& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryNewReverseRepoPosition;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 新国债逆回购持仓查询记录
 */
class EXPORT_MACRO_DEFINE AgwNewReverseRepoPositionArray : public mp::RootPacket
{
public:
Char32_def sno; 		///< 流水号
Date8_def match_date; 		///< 首次成交日期
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账号
BranchID_def branch_id; 		///< 营业部代码
MarketID_def market_id; 		///< 市场ID
AccountID_def account_id; 		///< 证券账户
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
Amt_def match_amt; 		///< 成交金额
Price_def match_price; 		///< 成交价格
Days_def gh_days; 		///< 回购天数
Bool_def exp_status; 		///< 是否今日到期
/*
 * @brief 默认构造函数
 */
  AgwNewReverseRepoPositionArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 32 + 8 + 16 + 16 + 10 + 2 + 12 + 8 + 40 + 8 + 8 + 4 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwNewReverseRepoPositionArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 新国债逆回购持仓查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultNewReverseRepoPosition : public AgwMsgExtQueryResultBase
{
public:
Length_def data_length; 		///< 组合个数
std::vector<AgwNewReverseRepoPositionArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultNewReverseRepoPosition();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultNewReverseRepoPosition& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultNewReverseRepoPosition;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 沪深要约收购行情查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTenderOfferSecurityInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTenderOfferSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTenderOfferSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTenderOfferSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 沪深要约收购行情查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTenderOfferArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券名称
Qty_def qty_unit; 		///< 数量单位
Qty_def qty_upper_limit; 		///< 申报数量上限
Qty_def qty_lower_limit; 		///< 申报数量下限
Price_def offer_price; 		///< 收购价
SecurityID_def subscription_id; 		///< 申购编码
/*
 * @brief 默认构造函数
 */
  AgwTenderOfferArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTenderOfferArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 沪深要约收购行情查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultTenderOfferSecurityInfo : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwTenderOfferArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultTenderOfferSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultTenderOfferSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultTenderOfferSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按市场成交汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTradeMarketCollect : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTradeMarketCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTradeMarketCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTradeMarketCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按市场成交汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTradeMarketCollectArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
Amt_def buy_total_value_traded; 		///< 买入成交总金额
Amt_def sell_total_value_traded; 		///< 卖出成交金额
Amt_def total_value_traded; 		///< 净买卖金额
Qty_def buy_total_qty_traded; 		///< 买入成交数量
Qty_def sell_total_qty_traded; 		///< 卖出成交数量
/*
 * @brief 默认构造函数
 */
  AgwTradeMarketCollectArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTradeMarketCollectArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 按市场成交汇总查询结果
 */
class EXPORT_MACRO_DEFINE AgwExtQueryResultTradeMarketCollect : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwTradeMarketCollectArray> order_array; 		///< 按市场成交汇总查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwExtQueryResultTradeMarketCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExtQueryResultTradeMarketCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExtQueryResultTradeMarketCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按证券成交汇总查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryTradeSecurityCollect : public AgwMsgExtQueryBase
{
public:
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryTradeSecurityCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryTradeSecurityCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryTradeSecurityCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 按证券成交汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwTradeSecurityCollectArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券ID
SecuritySymbol_def security_symbol; 		///< 证券简称
Amt_def buy_total_value_traded; 		///< 买入成交金额
Amt_def sell_total_value_traded; 		///< 卖出成交金额
Amt_def total_value_traded; 		///< 净买卖金额
Qty_def buy_total_qty_traded; 		///< 买入成交数量
Qty_def sell_total_qty_traded; 		///< 卖出成交数量
/*
 * @brief 默认构造函数
 */
  AgwTradeSecurityCollectArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 40 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwTradeSecurityCollectArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 按证券成交汇总查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwExtQueryResultTradeSecurityCollect : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwTradeSecurityCollectArray> order_array; 		///< 按证券成交汇总查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwExtQueryResultTradeSecurityCollect();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwExtQueryResultTradeSecurityCollect& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwExtQueryResultTradeSecurityCollect;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融多合约还款业务订单
 */
class EXPORT_MACRO_DEFINE AgwMsgDirectRepayMultiAppoint : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Bool_def is_account_partition; 		///< 账户是否按证券账户分区 1-是 0否
Currency_def currency; 		///< 货币种类
OrderWay_def order_way; 		///< 委托方式
String_def dd_contract_sno; 		///< 指定偿还负债流水号,逗号分隔
Amt_def repay_value; 		///< 还款金额
String_def remark; 		///< 备注
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgDirectRepayMultiAppoint();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgDirectRepayMultiAppoint& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgDirectRepayMultiAppoint;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融多合约还款业务应答
 */
class EXPORT_MACRO_DEFINE AgwMsgDirectRepayMultiAppointResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
Currency_def currency; 		///< 货币种类
Amt_def repay_value; 		///< 还款金额
Amt_def real_repay_value; 		///< 实际还款金额
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
/*
 * @brief 默认构造函数
 */
  AgwMsgDirectRepayMultiAppointResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgDirectRepayMultiAppointResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgDirectRepayMultiAppointResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融多合约展期申请
 */
class EXPORT_MACRO_DEFINE AgwMsgMultiDebtsDelayApply : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
String_def dd_sno; 		///< 合约流水号支持逗号分隔
SpecKinds_def spec_kinds; 		///< 扩展字段
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgMultiDebtsDelayApply();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMultiDebtsDelayApply& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgMultiDebtsDelayApply;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点两融多合约展期申请应答
 */
class EXPORT_MACRO_DEFINE AgwMsgMultiDebtsDelayApplyResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
Sno_def log_sno; 		///< 申请流水号
Uint8_def debts_total_num; 		///< 处理的合约数量
Uint8_def success_num; 		///< 成功数量
Uint8_def failure_num; 		///< 失败数量
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgMultiDebtsDelayApplyResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMultiDebtsDelayApplyResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgMultiDebtsDelayApplyResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 两融多合约展期申请(按到期天数)
 */
class EXPORT_MACRO_DEFINE AgwMsgMultiDebtsDelayApplyByDays : public mp::RootPacket
{
public:
TimeStamp_def time; 		///< 发起时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
PWD_def password; 		///< 密码
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
OrderWay_def order_way; 		///< 委托方式
Uint16_def expiry_days; 		///< 到期天数
OrderWayExt_def order_way_ext; 		///< 委托方式扩展
/*
 * @brief 默认构造函数
 */
  AgwMsgMultiDebtsDelayApplyByDays();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMultiDebtsDelayApplyByDays& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgMultiDebtsDelayApplyByDays;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 两融多合约展期申请(按到期天数)应答
 */
class EXPORT_MACRO_DEFINE AgwMsgMultiDebtsDelayApplyByDaysResult : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
AgwIndex_def index; 		///< 索引号
RejectReasonCode_def reject_reason_code; 		///< 订单拒绝原因
String_def reject_desc; 		///< 拒绝原因描述
TimeStamp_def transact_time; 		///< 回报时间
SeqNum_def client_seq_id; 		///< 用户系统消息序号
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
UserInfo_def user_info; 		///< 用户私有信息
Sno_def log_sno; 		///< 申请流水号
Uint32_def debts_total_num; 		///< 处理的合约数量
Uint32_def success_num; 		///< 成功数量
Uint32_def failure_num; 		///< 失败数量
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgMultiDebtsDelayApplyByDaysResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMultiDebtsDelayApplyByDaysResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgMultiDebtsDelayApplyByDaysResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点查询客户资金消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDFund : public AgwMsgExtQueryBase
{
public:
Currency_def currency; 		///< 货币种类
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点资金查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwDDFundArray : public mp::RootPacket
{
public:
Currency_def currency; 		///< 货币种类
FundSeq_def fund_seq; 		///< 主资金账号
Amt_def fund_last_bal; 		///< 昨日余额
Amt_def fund_bal; 		///< 资金余额
Amt_def fund_avl; 		///< 资金可用
Amt_def fund_asset; 		///< 资金资产
Amt_def stk_asset; 		///< 证券资产
Amt_def total_asset; 		///< 总资产
Amt_def fund_buy; 		///< 买入冻结
Amt_def max_draw; 		///< 可取金额
Amt_def settle_balance; 		///< 实时清算资金
Amt_def contract_posi_value; 		///< 合约持仓市值
Amt_def last_posi_value; 		///< 昨日持仓市值
/*
 * @brief 默认构造函数
 */
  AgwDDFundArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 4 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwDDFundArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点查询客户资金消息结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDFund : public AgwMsgExtQueryResultBase
{
public:
PartitionNo_def te_partition_no; 		///< 所属分区号
std::vector<AgwDDFundArray> order_array; 		///< 自研资金查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDFund();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDFund& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDFund;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 订单查询(自定义编号)
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDSpecialOrder : public AgwMsgExtQueryBase
{
public:
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,同一个客户当天唯一,填0查所有
ClOrdNO_def cust_cl_ord_no; 		///< 客户自定义订单编号,订单编号优先
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
Side_def side; 		///< 买卖方向,填0查所有
OrderQueryCondition_def order_query_condition; 		///< 委托查询条件
ClOrdNO_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
Bool_def flag; 		///< 是否剔除北交所和老三板
CancelOrderFlag_def cancel_order_flag; 		///< 撤单标志（表示该笔委托是否是撤单委托）
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDSpecialOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDSpecialOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDSpecialOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 委托查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwDDSpecOrderArray : public mp::RootPacket
{
public:
SecurityID_def security_id; 		///< 证券代码
SecuritySymbol_def security_symbol; 		///< 证券简称
MarketID_def market_id; 		///< 证券所属市场
AccountID_def account_id; 		///< 证券账户
Side_def side; 		///< 买卖方向
OrdType_def ord_type; 		///< 订单类别
OrdStatus_def ord_status; 		///< 原订单当前状态
TimeStamp_def transact_time; 		///< 客户委托时间
Price_def order_price; 		///< 委托价格
Price_def exec_price; 		///< 平均成交价格
Qty_def order_qty; 		///< 委托数量
Qty_def cum_qty; 		///< 累计成交数量
ClOrdNO_def cl_ord_no; 		///< 客户订单编号
ClOrdID_def cl_ord_id; 		///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClOrdNO_def orig_cl_ord_no; 		///< 原委托的客户订单编号
Amt_def frozen_trade_value; 		///< 冻结交易金额
Amt_def frozen_fee; 		///< 冻结费用
ExErrMsg_def ord_rej_reason; 		///< 交易所错误信息描述
Qty_def cancel_qty; 		///< 撤单数量
Amt_def total_value_traded; 		///< 成交金额,精确到分
CancelOrderFlag_def cancel_order_flag; 		///< 撤单标志（表示该笔委托是否是撤单委托）
/*
 * @brief 默认构造函数
 */
  AgwDDSpecOrderArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 40 + 2 + 12 + 1 + 1 + 1 + 8 + 8 + 8 + 8 + 8 + 8 + 10 + 8 + 8 + 8 + 8 + 128 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwDDSpecOrderArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 订单查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDSpecialOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwDDSpecOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDSpecialOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDSpecialOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDSpecialOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 证券信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwDDSecurityInfoArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
Currency_def currency; 		///< 货币类型
SecuritySymbol_def security_symbol; 		///< 证券名称
SecurityID_def security_id; 		///< 证券代码
SecurityType_def security_type; 		///< 证券类别
PriceTick_def price_tick; 		///< 证券价位
Price_def limit_up; 		///< 涨停价格
Price_def limit_down; 		///< 跌停价格
BondInterest_def bond_interest; 		///< 债券应计利息
Qty_def max_qty; 		///< 最大数量
Qty_def min_qty; 		///< 最小数量
Qty_def buy_tick; 		///< 买入最小单位
Qty_def sale_tick; 		///< 卖出最小单位
Price_def fixprice; 		///< 昨收盘价
Uint8_def pricing_method; 		///< 国债净价标志
/*
 * @brief 默认构造函数
 */
  AgwDDSecurityInfoArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 4 + 40 + 8 + 2 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwDDSecurityInfoArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 顶点全量证券信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryDDSecurityInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryDDSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryDDSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryDDSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 顶点全量证券信息查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultDDSecurityInfo : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 证券信息总数
Length_def data_length; 		///< 组合个数
std::vector<AgwDDSecurityInfoArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultDDSecurityInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultDDSecurityInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultDDSecurityInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief UTE回切NGTP消息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryUTERevertNGTP : public mp::RootPacket
{
public:
SenderCompID_def sender_comp_id; 		///< 发送方代码
TargetCompID_def target_comp_id; 		///< 接收方代码
String_def agw_user_name; 		///< agw网关用户名
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryUTERevertNGTP();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryUTERevertNGTP& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryUTERevertNGTP;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief UTE回切NGTP消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtUTERevertNGTPResult : public mp::RootPacket
{
public:
Bool_def revert_status; 		///< 回切状态0是未切1是已切
/*
 * @brief 默认构造函数
 */
  AgwMsgExtUTERevertNGTPResult();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtUTERevertNGTPResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtUTERevertNGTPResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief UTE单客户/UTE节点回切NGTP消息
 */
class EXPORT_MACRO_DEFINE AgwMsgSingleAccountRevertNGTP : public mp::RootPacket
{
public:
InstructionID_def instruction_id; 		///< 指令ID，同一登陆会话唯一
Uint8_def component_id; 		///< 组件类型
FundAccountID_def fund_account_id; 		///< 资金帐号
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户id
Bool_def ute_revert_switch; 		///< 回切开关
UTERevertScopeType_def ute_revert_scope_type; 		///< UTE回切范围类型,1=单客户回切,2=节点回切
UTEPartitionNo_def ute_partition_no; 		///< UTE分区号
PartitionNo_def cte_partition_no; 		///< CTE分区号
/*
 * @brief 默认构造函数
 */
  AgwMsgSingleAccountRevertNGTP();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 8 + 1 + 16 + 10 + 12 + 1 + 1 + 1 + 1;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgSingleAccountRevertNGTP& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgSingleAccountRevertNGTP;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief CRC风控试算查询请求
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCRCPreview : public AgwMsgExtQueryBase
{
public:
Char_def query_type; 		///< 查询类别1-早盘集合竞价虚假申报 2-涨跌停虚假申报 3-维持涨跌停限价买卖
RuleID_def rule_id; 		///< 规则ID
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
Side_def side; 		///< 买卖方向
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCRCPreview();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCRCPreview& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCRCPreview;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief CRC风控试算查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCRCPreview : public AgwMsgExtQueryResultBase
{
public:
Char_def query_type; 		///< 查询类别1-早盘集合竞价虚假申报 2-涨跌停虚假申报 3-维持涨跌停限价买卖
RuleID_def rule_id; 		///< 规则ID
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
Side_def side; 		///< 买卖方向
Char_def is_interrupted; 		///< 是否触发阻断0-否 1-是
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCRCPreview();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCRCPreview& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCRCPreview;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户单位时间委托金额查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryUnitTimeOrderAmt : public AgwMsgExtQueryBase
{
public:
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 证券所属市场
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
Int64_def statistical_time; 		///< 统计时间（秒）
Price_def order_price; 		///< 委托价格
Qty_def order_qty; 		///< 委托数量
OrdType_def order_type; 		///< 订单类别
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryUnitTimeOrderAmt();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryUnitTimeOrderAmt& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryUnitTimeOrderAmt;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户单位时间委托金额查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExternalQueryResultUnitTimeOrderAmt : public AgwMsgExtQueryResultBase
{
public:
AccountID_def account_id; 		///< 股东账户
SecurityID_def security_id; 		///< 证券代码
MarketID_def market_id; 		///< 证券所属市场
BusinessType_def business_type; 		///< 业务类型
Side_def side; 		///< 买卖方向
Amt_def order_amt; 		///< 已申报金额
Amt_def cur_order_amt; 		///< 已申报金额
Amt_def total_order_amt; 		///< 总委托金额（含当笔）
Time9_def begin_time; 		///< 统计起始时间HHMMSSsss
Time9_def end_time; 		///< 统计终止时间HHMMSSsss
TimeFlag_def time_flag; 		///< 统计时间段标识
/*
 * @brief 默认构造函数
 */
  AgwMsgExternalQueryResultUnitTimeOrderAmt();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExternalQueryResultUnitTimeOrderAmt& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExternalQueryResultUnitTimeOrderAmt;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 三方专用订单查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryThreePartyOrder : public AgwMsgExtQueryBase
{
public:
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,同一个客户当天唯一,填0查所有
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
Side_def side; 		///< 买卖方向,填0查所有
OrderQueryCondition_def order_query_condition; 		///< 委托查询条件
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
Bool_def flag; 		///< 是否剔除北交所和老三板
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryThreePartyOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryThreePartyOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryThreePartyOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 三方专用订单查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThreePartyOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 委托记录总数
std::vector<AgwOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThreePartyOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThreePartyOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThreePartyOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 三方专用成交查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryThreePartyTradeOrder : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
BusinessType_def business_type; 		///< 业务类型,填0查所有
ClOrdNO_def cl_ord_no; 		///< 客户订单编号,填0查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填0表示不指定
ReturnSeq_def return_seq; 		///< 返回顺序，按时间排序
Bool_def flag; 		///< 是否剔除北交所和老三板
Side_def side; 		///< 买卖方向,填0查所有
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryThreePartyTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryThreePartyTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryThreePartyTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 三方专用成交查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultThreePartyTradeOrder : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 成交记录总数
std::vector<AgwTradeOrderArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultThreePartyTradeOrder();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultThreePartyTradeOrder& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultThreePartyTradeOrder;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief G1客户股票风控查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryG1ShareControl : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 证券所属市场,填0查所有
SecurityID_def security_id; 		///< 证券代码,填空查所有
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryG1ShareControl();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryG1ShareControl& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryG1ShareControl;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief G1客户股票风控查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwG1ShareControlArray : public mp::RootPacket
{
public:
AccountID_def account_id; 		///< 账户ID
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
Qty_def max_qty; 		///< 最大数量
Uint8_def control_flag; 		///< 控制标识
Uint8_def end_flag; 		///< 结束标识
TimeStamp_def g1_set_time; 		///< G1设置时间
/*
 * @brief 默认构造函数
 */
  AgwG1ShareControlArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 12 + 2 + 8 + 8 + 1 + 1 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwG1ShareControlArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief G1客户股票风控查询结果
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultG1ShareControl : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 记录总数
Length_def data_length; 		///< 组合个数
std::vector<AgwG1ShareControlArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultG1ShareControl();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultG1ShareControl& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultG1ShareControl;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief G1客户股票风控设置
 */
class EXPORT_MACRO_DEFINE AgwMsgG1ShareControlSet : public mp::RootPacket
{
public:
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
ClientFeatureCode_def client_feature_code; 		///< 终端识别码
UserInfo_def user_info; 		///< 用户私有信息
PWD_def password; 		///< 密码
MarketID_def market_id; 		///< 交易市场
SecurityID_def security_id; 		///< 证券代码
Qty_def max_qty; 		///< 最大数量
Uint8_def control_flag; 		///< 控制标识
Uint8_def end_flag; 		///< 结束标识
/*
 * @brief 默认构造函数
 */
  AgwMsgG1ShareControlSet();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgG1ShareControlSet& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgG1ShareControlSet;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief G1客户股票风控设置应答
 */
class EXPORT_MACRO_DEFINE AgwMsgG1ShareControlSetResult : public mp::RootPacket
{
public:
TimeStamp_def set_time; 		///< 设置时间
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
BranchID_def branch_id; 		///< origid+营业部ID,各占5个字节
AccountID_def account_id; 		///< 账户ID
SeqNum_def client_seq_id; 		///< 用户系统消息序号
UserInfo_def user_info; 		///< 用户私有信息
String_def key_error_msg; 		///< 错误信息
String_def key_error_code; 		///< 错误代码串
KeyErrorLevel_def key_error_level; 		///< 错误级别
/*
 * @brief 默认构造函数
 */
  AgwMsgG1ShareControlSetResult();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgG1ShareControlSetResult& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgG1ShareControlSetResult;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 转股回售证券信息查询
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQuerySwapPutbackInfo : public AgwMsgExtQueryBase
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQuerySwapPutbackInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQuerySwapPutbackInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQuerySwapPutbackInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 转股回售证券信息查询结果组合
 */
class EXPORT_MACRO_DEFINE AgwSwapPutbackArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
Price_def swap_price; 		///< 转股价格
Price_def putback_price; 		///< 回售价格
/*
 * @brief 默认构造函数
 */
  AgwSwapPutbackArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwSwapPutbackArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 转股回售证券信息查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultSwapPutbackInfo : public AgwMsgExtQueryResultBase
{
public:
std::vector<AgwSwapPutbackArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultSwapPutbackInfo();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultSwapPutbackInfo& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultSwapPutbackInfo;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 盯市推送公共字段
 */
class EXPORT_MACRO_DEFINE AgwMsgMtmIndicatorPushBase : public mp::RootPacket
{
public:
AgwPartition_def partition; 		///< 分区号
BranchID_def branch_id; 		///< 营业部代码
CustID_def cust_id; 		///< 客户号
FundAccountID_def fund_account_id; 		///< 资金账户ID
UserInfo_def user_info; 		///< 用户私有信息
/*
 * @brief 默认构造函数
 */
  AgwMsgMtmIndicatorPushBase();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 1 + 10 + 16 + 16 + 64;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgMtmIndicatorPushBase& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 持仓集中度结果组合
 */
class EXPORT_MACRO_DEFINE AgwCustSecurityMtmIndicatorArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 市场ID
SecurityID_def security_id; 		///< 证券代码
CreditRatio_def stk_concen; 		///< 持仓集中度
CreditRatio_def funddebt_concen; 		///< 融资未偿还集中度
/*
 * @brief 默认构造函数
 */
  AgwCustSecurityMtmIndicatorArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCustSecurityMtmIndicatorArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 客户级券盯市指标推送
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCustSecurityMtmIndicatorPush : public AgwMsgMtmIndicatorPushBase
{
public:
AgwIndex_def index; 		///< 索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwCustSecurityMtmIndicatorArray> order_array; 		///< 持仓集中度结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCustSecurityMtmIndicatorPush();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCustSecurityMtmIndicatorPush& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCustSecurityMtmIndicatorPush;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户级盯市指标推送
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCustMtmIndicatorPush : public AgwMsgMtmIndicatorPushBase
{
public:
AgwIndex_def index; 		///< 索引号
CreditRatio_def keep_rate; 		///< 维保比
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCustMtmIndicatorPush();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 0 + 1 + 10 + 16 + 16 + 64 + 4 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCustMtmIndicatorPush& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCustMtmIndicatorPush;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 公司级券盯市指标结果组合
 */
class EXPORT_MACRO_DEFINE AgwCompanySecurityMtmIndicatorArray : public mp::RootPacket
{
public:
MarketID_def market_id; 		///< 证券所属市场
SecurityID_def security_id; 		///< 证券代码
CreditRatio_def stk_concen; 		///< 单券集中度
CreditRatio_def funddebt_concen; 		///< 融资未偿还集中度
/*
 * @brief 默认构造函数
 */
  AgwCompanySecurityMtmIndicatorArray();/*
 * @brief 消息大小
 */
    static inline size_t GetFixedSize()
    {
        return 0 + 2 + 8 + 8 + 8;
    }
/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwCompanySecurityMtmIndicatorArray& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
};
/*
 * @brief 公司级券盯市指标推送
 */
class EXPORT_MACRO_DEFINE AgwMsgExtCompanySecurityMtmIndicatorPush : public AgwMsgMtmIndicatorPushBase
{
public:
AgwIndex_def index; 		///< 索引号
TotalNum_def total_num; 		///< 记录总数
std::vector<AgwCompanySecurityMtmIndicatorArray> order_array; 		///< 公司级券盯市指标结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtCompanySecurityMtmIndicatorPush();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtCompanySecurityMtmIndicatorPush& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtCompanySecurityMtmIndicatorPush;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户级盯市指标查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCustStkInd : public AgwMsgExtQueryBase
{
public:
CreditRatio_def stk_concen_ratio; 		///< 单券集中度比例,填0按默认告警阈值查询
CreditRatio_def funddebt_concen_ratio; 		///< 融资未偿还集中度比例,填0按默认告警阈值查询
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCustStkInd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCustStkInd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCustStkInd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 客户级盯市指标查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCustStkInd : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwCustSecurityMtmIndicatorArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCustStkInd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCustStkInd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCustStkInd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 公司级盯市指标查询消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryCompStkInd : public AgwMsgExtQueryBase
{
public:
CreditRatio_def stk_concen_ratio; 		///< 单券集中度比例,填0按默认告警阈值查询
CreditRatio_def funddebt_concen_ratio; 		///< 融资未偿还集中度比例,填0按默认告警阈值查询
ReportIndex_def query_index; 		///< 起始查询索引号,填0表示从头开始查
TotalNum_def return_num; 		///< 指定查询返回数量,填 0表示不指定
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryCompStkInd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryCompStkInd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryCompStkInd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
/*
 * @brief 公司级盯市指标查询结果消息
 */
class EXPORT_MACRO_DEFINE AgwMsgExtQueryResultCompStkInd : public AgwMsgExtQueryResultBase
{
public:
ReportIndex_def last_index; 		///< 最后一笔查询结果的索引号
TotalNum_def total_num; 		///< 查询记录总数
std::vector<AgwCompanySecurityMtmIndicatorArray> order_array; 		///< 查询结果组合
/*
 * @brief 默认构造函数
 */
  AgwMsgExtQueryResultCompStkInd();/*
 * @brief 消息实际大小
 */
    virtual size_t GetSize() const;
/*
 * @brief 输出文本存到流中
 */
    virtual void DumpAsString(std::ostream& os) const;

/*
 * @brief 用默认值初始化成员
 */
    void FillDefaultValue();
/*
 * @brief 检查两个消息字段的值是否完全相同
 */
    bool Diff(const AgwMsgExtQueryResultCompStkInd& msg);
/*
 * @brief 从字节流读入消息
 */
    virtual error_code Decode(MessageDecoder& decoder);
/*
 * @brief 把消息写到字节流中
 */
    virtual error_code Encode(MessageEncoder& encoder) const;
    	virtual mp::MsgTypeNo_def GetTypeID() const { return kAgwMsgExtQueryResultCompStkInd;  }
      virtual mp::MsgVer_def GetMsgVer() const { return 2;}
};
}
}
#endif // ATP_GW_MESSAGES_AGW_EXTERNAL_H_

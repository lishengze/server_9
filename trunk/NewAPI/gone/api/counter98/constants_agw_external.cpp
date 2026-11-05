#include "constants_agw_external.h"
#include "types_agw_external.h"
namespace atp
{
namespace gw
{
int cc;
/*
 * @brief 成交申报模式
 */
const TradeHandlingInstr_def TradeHandlingInstr::kTransReport = '1';  ///< 成交报告
const TradeHandlingInstr_def TradeHandlingInstr::kProtocolMatch = '2';  ///< 协议配对
const TradeHandlingInstr_def TradeHandlingInstr::kTransRequest = '3';  ///< 成交请求
std::map<TradeHandlingInstr_def, std::string> TradeHandlingInstr::const_desc_g;
std::map<TradeHandlingInstr_def, std::string> TradeHandlingInstr::const_summary_g;
bool TradeHandlingInstr::init_desc_g = TradeHandlingInstr::InitDesc();
bool TradeHandlingInstr::init_summary_g = TradeHandlingInstr::InitSummary();
/*
 * @brief 结算方式
 */
#ifndef _MSC_VER
const SettlType_def SettlType::kMultLateralNetting;  ///< 多边净额
#endif
#ifndef _MSC_VER
const SettlType_def SettlType::kOneByOneFullAmount;  ///< 逐笔全额
#endif
std::map<SettlType_def, std::string> SettlType::const_desc_g;
std::map<SettlType_def, std::string> SettlType::const_summary_g;
bool SettlType::init_desc_g = SettlType::InitDesc();
bool SettlType::init_summary_g = SettlType::InitSummary();
/*
 * @brief 交易主体类型
 */
const InvestorType_def InvestorType::kSelf = {'0', '1', };  ///< 自营
const InvestorType_def InvestorType::kAsset = {'0', '2', };  ///< 资管
const InvestorType_def InvestorType::kInstitutional = {'0', '3', };  ///< 机构经纪
const InvestorType_def InvestorType::kPersonal = {'0', '4', };  ///< 个人经纪
std::map<InvestorType_def, std::string> InvestorType::const_desc_g;
std::map<InvestorType_def, std::string> InvestorType::const_summary_g;
bool InvestorType::init_desc_g = InvestorType::InitDesc();
bool InvestorType::init_summary_g = InvestorType::InitSummary();
/*
 * @brief 交易员
 */
const TraderCode_def TraderCode::kNULL = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', };  ///< 空
std::map<TraderCode_def, std::string> TraderCode::const_desc_g;
std::map<TraderCode_def, std::string> TraderCode::const_summary_g;
bool TraderCode::init_desc_g = TraderCode::InitDesc();
bool TraderCode::init_summary_g = TraderCode::InitSummary();
/*
 * @brief 成交申报事务类别
 */
#ifndef _MSC_VER
const TradeReportTransType_def TradeReportTransType::kNew;  ///< 普通申报
#endif
#ifndef _MSC_VER
const TradeReportTransType_def TradeReportTransType::kCancel;  ///< 主动撤单
#endif
#ifndef _MSC_VER
const TradeReportTransType_def TradeReportTransType::kReplace;  ///< 更新申报
#endif
std::map<TradeReportTransType_def, std::string> TradeReportTransType::const_desc_g;
std::map<TradeReportTransType_def, std::string> TradeReportTransType::const_summary_g;
bool TradeReportTransType::init_desc_g = TradeReportTransType::InitDesc();
bool TradeReportTransType::init_summary_g = TradeReportTransType::InitSummary();
/*
 * @brief 成交申报类型
 */
#ifndef _MSC_VER
const TradeReportType_def TradeReportType::kSubmitTransaction;  ///< 提交成交申报
#endif
#ifndef _MSC_VER
const TradeReportType_def TradeReportType::kAcceptTransaction;  ///< 接受成交申报
#endif
#ifndef _MSC_VER
const TradeReportType_def TradeReportType::kRefuseTransaction;  ///< 拒绝成交申报
#endif
std::map<TradeReportType_def, std::string> TradeReportType::const_desc_g;
std::map<TradeReportType_def, std::string> TradeReportType::const_summary_g;
bool TradeReportType::init_desc_g = TradeReportType::InitDesc();
bool TradeReportType::init_summary_g = TradeReportType::InitSummary();
/*
 * @brief 特殊业务类型标识
 */
const Char_def SpecBusiFlag::kNULL = ' ';  ///< 默认值
const Char_def SpecBusiFlag::kRightsIssue = '1';  ///< 配股
std::map<Char_def, std::string> SpecBusiFlag::const_desc_g;
std::map<Char_def, std::string> SpecBusiFlag::const_summary_g;
bool SpecBusiFlag::init_desc_g = SpecBusiFlag::InitDesc();
bool SpecBusiFlag::init_summary_g = SpecBusiFlag::InitSummary();
/*
 * @brief 网关业务拒绝代码
 */
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejSwitchOff;  ///< 网关开关关闭
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejAccountid;  ///< 股东账户拒绝
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejBusinessType;  ///< 业务类型错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejMarketID;  ///< 市场字段错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejSide;  ///< 买卖方向错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejOrdType;  ///< 订单类型错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejTimeInForce;  ///< 订单有效期错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejCashMargin;  ///< 信用标识错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejPositionEffect;  ///< 开平仓标识错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejCoveredOrUncovered;  ///< 备兑标识错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejDesignationTransType;  ///< 转托管注册类型错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejSecurityid;  ///< 证券代码错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejOrderQty;  ///< 订单申报数量错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejPrice;  ///< 价格错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejWhitList;  ///< IP限制错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejCurrency;  ///< 币种错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejTransferWay;  ///< 划转方式错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kQtyTypeError;  ///< 订单数量类型错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejGlobalRateLimit;  ///< 全局流速限制
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejIpMacRateLimit;  ///< 终端流速限制
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejCustIdNotInWhiteList;  ///< 客户号不在白名单
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejCustIdInBlackList;  ///< 该资金账号在黑名单内
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejFuncCodeNotFound;  ///< 功能号权限限制
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejMsgType;  ///< Rgw不处理委托
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejNeedFollowTheSessionProcess;  ///< 未按照三方接入会话流程进行接入
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejAlreadyLogin;  ///< 该客户号已经登录
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejNotLoggedin;  ///< 该客户号未登录
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejIPAddress;  ///< IP被禁止
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejT2SDK;  ///< 恒生API错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejFundAccountId;  ///< 恒生T2SDK FundAccountId错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejCheckCloseOutFlag;  ///< 了结标识错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejLegSideErr;  ///< 腿委托持仓类型错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejSecondaryOrderIdIsErr;  ///< 组合解除时，组合编号不能为空
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejNotExistThePartition;  ///< 无法在委托路由表中找到账户对应的分区号
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejNotExistTxEndpointPartitions;  ///< 账户无法在该分区的网关报送消息，总线管理服务中未配置网关到引擎的主题分区
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejCheckSZCirculationType;  ///< 深圳流通类型错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kCannotFindAccountID;  ///< 不具备股东账户权限
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kNoMsgTypeNoRight;  ///< 不具备功能号权限
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejStrategyIdIsErr;  ///< 构建组合时，组合策略编码不能为空
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejCustIDLimit;  ///< 客户号超过了流速上限
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejFuncCodeNotAllow;  ///< 客户功能号被禁止
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejRepayWay;  ///< 还款方式错误
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kTpaRejAGWFuncCodeNotAllow;  ///< AGW功能号被禁止
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFuncnoForbidden;  ///< 功能号被阻断
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessCustIDAndProductIDSpeedReject;  ///< 客户号和产品号流速超过
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessCustIDAndProductIDSumReject;  ///< 客户号和产品号流量超过
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessUserIDSpeedReject;  ///< 用户流速超过
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessUserIDSumReject;  ///< 用户流量超过
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessAgwSpeedReject;  ///< agw流速超过
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessAgwSumReject;  ///< agw流量超过
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessAgwSwitchReject;  ///< agw接入开关关闭
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessUserIDSwitchReject;  ///< 用户接入开关关闭
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kFlowAccessCustIDAndProductIDSwitchReject;  ///< 客户号和产品号接入开关关闭
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRiskControlNoConnectionReject;  ///< 风控无连接拒绝
#endif
#ifndef _MSC_VER
const RejectReasonCode_def AgwBizRejCode::kRejOther;  ///< 其他错误
#endif
std::map<RejectReasonCode_def, std::string> AgwBizRejCode::const_desc_g;
std::map<RejectReasonCode_def, std::string> AgwBizRejCode::const_summary_g;
bool AgwBizRejCode::init_desc_g = AgwBizRejCode::InitDesc();
bool AgwBizRejCode::init_summary_g = AgwBizRejCode::InitSummary();
/*
 * @brief 外部委托查询的委托状态条件
 */
#ifndef _MSC_VER
const OrderQueryCondition_def AgwOrderQueryCondition::kAll;  ///< 查询全部
#endif
#ifndef _MSC_VER
const OrderQueryCondition_def AgwOrderQueryCondition::kCancelAvailable;  ///< 查询可撤单委托
#endif
#ifndef _MSC_VER
const OrderQueryCondition_def AgwOrderQueryCondition::kTradeExecOrCanceled;  ///< 查询有成交或已经撤单委托
#endif
#ifndef _MSC_VER
const OrderQueryCondition_def AgwOrderQueryCondition::kUnavailableCancel;  ///< 查询不可撤单委托
#endif
std::map<OrderQueryCondition_def, std::string> AgwOrderQueryCondition::const_desc_g;
std::map<OrderQueryCondition_def, std::string> AgwOrderQueryCondition::const_summary_g;
bool AgwOrderQueryCondition::init_desc_g = AgwOrderQueryCondition::InitDesc();
bool AgwOrderQueryCondition::init_summary_g = AgwOrderQueryCondition::InitSummary();
/*
 * @brief 网关实时指令类型
 */
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kFundAdjust;  ///< 资金实时调整
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kSharePositionAdjust;  ///< 股份持仓实时调整
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kBusinessRightsControl;  ///< 实时限制交易
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kBatchBusinessRightsControl;  ///< 批量交易权限控制指令
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kModifyCashAuctionParams;  ///< 现货交易参数修改
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kModifyETFBusinessRule;  ///< ETF申赎业务规则修改
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kModifyShareDayTrading;  ///< 交易回转标记修改
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kCustPwdAdjust;  ///< 客户密码实时修改指令
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kAgwSpeedAdjust;  ///< 网关总流速总连接数调整
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kAgwTerminalAdjust;  ///< 网关终端信息调整
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kAgwBlackWhiteListAdjust;  ///< 网关黑白名单调整
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kAgwFuncNoListAdjust;  ///< 网关功能号调整
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kAgwAccessControlDeleteAdjust;  ///< 网关接入信息删除
#endif
#ifndef _MSC_VER
const InstructionType_def AgwInstructionType::kAgwAuthorizationAdjust;  ///< 网关授权信息调整
#endif
std::map<InstructionType_def, std::string> AgwInstructionType::const_desc_g;
std::map<InstructionType_def, std::string> AgwInstructionType::const_summary_g;
bool AgwInstructionType::init_desc_g = AgwInstructionType::InitDesc();
bool AgwInstructionType::init_summary_g = AgwInstructionType::InitSummary();
/*
 * @brief 网关查询指令类型
 */
#ifndef _MSC_VER
const InstructionType_def AgwQueryInstructionType::kFundDetailQuery;  ///< 资金流水明细查询
#endif
#ifndef _MSC_VER
const InstructionType_def AgwQueryInstructionType::kAccountShareDetailQuery;  ///< 股份流水明细查询
#endif
std::map<InstructionType_def, std::string> AgwQueryInstructionType::const_desc_g;
std::map<InstructionType_def, std::string> AgwQueryInstructionType::const_summary_g;
bool AgwQueryInstructionType::init_desc_g = AgwQueryInstructionType::InitDesc();
bool AgwQueryInstructionType::init_summary_g = AgwQueryInstructionType::InitSummary();
/*
 * @brief 业务类型
 */
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kAll;  ///< 所有类型
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCashAuction;  ///< 现货集中交易
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kPledge;  ///< 质押式回购交易
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kBondDistribution;  ///< 债券分销
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kOptionAuction;  ///< 期权集中交易
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kBlockTrade;  ///< 协议交易(大宗交易)
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kAfterHoursPricing;  ///< 盘后定价交易
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kRefinancing;  ///< 转融通
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kAssetTransfer;  ///< 资产管理计划份额转让
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCashPledgeRepo;  ///< 股票质押式回购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kAgreedRepo;  ///< 约定购回
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kPledgeQuoteRepo;  ///< 质押式报价回购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kETFRealTimePurchaseRedemption;  ///< ETF实时申购赎回
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kIssue;  ///< 网上发行认购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kSubscriptionRatio;  ///< 配股认购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kBondSellBack;  ///< 债券转股回售
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kOptionExercise;  ///< 期权行权
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kOpenFundPurchaseRedemption;  ///< 开放式基金申购赎回
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kTenderOffer;  ///< 要约收购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kPledgeRepoPledgeOrSign;  ///< 质押式回购质押解押
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kDesignation;  ///< 转托管
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kVote;  ///< 网络投票
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kDesignatedTransaction;  ///< 指定交易
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kWarrantsExercise;  ///< 权证行权
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kPasswordService;  ///< 密码服务
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kOpenMarginQuery;  ///< 保证金查询
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kOpenFundSetting;  ///< 开放式基金转托管、分红方式设置
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kTurnToDisposal;  ///< 转处置
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCouponSwap;  ///< 垫券还券
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kPayOff;  ///< 待清偿扣划
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kBondPledgeAgreementRepo;  ///< 债券质押式协议回购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kLevelFundRealTimeOpenOrMerge;  ///< 分级基金实时分拆合并
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCoverLockOrUnlock;  ///< 备兑锁定解锁
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kOptionCombination;  ///< 期权组合策略保证金
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kHKStocksThrough;  ///< 港股通
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kTibAfterHour;  ///< 科创板股票盘后定价交易业务
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kMarginTrade;  ///< 融资融券交易业务
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCreditAuction;  ///< 信用账户普通买入/普通卖出（卖券还款）交易业务
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kMarginTrading;  ///< 融资融券交易业务
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kDirectRepay;  ///< 直接还款
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCollateralInOrOut;  ///< 担保品划转
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kBuySecuRepay;  ///< 买券还券
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCurSecuRepay;  ///< 现券还券
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kCreditVote;  ///< 信用账户投票
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kDirectRepayAppoint;  ///< 指定合约还款
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kThirdAgreementTransfer;  ///< 股转协议转让
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kThirdTenderOffer;  ///< 股转要约收购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kThirdIssue;  ///< 股转发行
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kThirdCashAuction;  ///< 新三板集中交易(挂牌做市, 股转挂牌连续竞价, 挂牌集合竞价)
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kThirdBlockTrade;  ///< 股转大宗交易
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kThirdCashAuctionDelist;  ///< 新三板集中交易(两网公司及退市公司股票)
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kOfflineETFSubscribe;  ///< 网下ETF发行认购
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kLOFDividePolicySet;  ///< LOF基金分红方式设置
#endif
#ifndef _MSC_VER
const BusinessType_def AgwBusinessType::kThirdDelistAgreement;  ///< 股转摘牌协议转让
#endif
std::map<BusinessType_def, std::string> AgwBusinessType::const_desc_g;
std::map<BusinessType_def, std::string> AgwBusinessType::const_summary_g;
bool AgwBusinessType::init_desc_g = AgwBusinessType::InitDesc();
bool AgwBusinessType::init_summary_g = AgwBusinessType::InitSummary();
/*
 * @brief 货币种类
 */
const Currency_def AgwCurrency::kCNY = {'C', 'N', 'Y', ' ', };  ///< 人民币
const Currency_def AgwCurrency::kUSD = {'U', 'S', 'D', ' ', };  ///< 美元
const Currency_def AgwCurrency::kHKD = {'H', 'K', 'D', ' ', };  ///< 港币
std::map<Currency_def, std::string> AgwCurrency::const_desc_g;
std::map<Currency_def, std::string> AgwCurrency::const_summary_g;
bool AgwCurrency::init_desc_g = AgwCurrency::InitDesc();
bool AgwCurrency::init_summary_g = AgwCurrency::InitSummary();
/*
 * @brief 认沽认购
 */
const CallOrPut_def AgwCallOrPut::kCall = 'C';  ///< 认购
const CallOrPut_def AgwCallOrPut::kPut = 'P';  ///< 认沽
std::map<CallOrPut_def, std::string> AgwCallOrPut::const_desc_g;
std::map<CallOrPut_def, std::string> AgwCallOrPut::const_summary_g;
bool AgwCallOrPut::init_desc_g = AgwCallOrPut::InitDesc();
bool AgwCallOrPut::init_summary_g = AgwCallOrPut::InitSummary();
/*
 * @brief 信用标识
 */
const CashMargin_def AgwCashMargin::kCash = '1';  ///< 普通交易
const CashMargin_def AgwCashMargin::kOpen = '2';  ///< 融资融券开仓
const CashMargin_def AgwCashMargin::kClose = '3';  ///< 融资融券平仓
std::map<CashMargin_def, std::string> AgwCashMargin::const_desc_g;
std::map<CashMargin_def, std::string> AgwCashMargin::const_summary_g;
bool AgwCashMargin::init_desc_g = AgwCashMargin::InitDesc();
bool AgwCashMargin::init_summary_g = AgwCashMargin::InitSummary();
/*
 * @brief 备兑
 */
#ifndef _MSC_VER
const CoveredOrUncovered_def AgwCoveredUncovered::kCovered;  ///< 备兑仓
#endif
#ifndef _MSC_VER
const CoveredOrUncovered_def AgwCoveredUncovered::kUncovered;  ///< 普通仓
#endif
std::map<CoveredOrUncovered_def, std::string> AgwCoveredUncovered::const_desc_g;
std::map<CoveredOrUncovered_def, std::string> AgwCoveredUncovered::const_summary_g;
bool AgwCoveredUncovered::init_desc_g = AgwCoveredUncovered::InitDesc();
bool AgwCoveredUncovered::init_summary_g = AgwCoveredUncovered::InitSummary();
/*
 * @brief 转托管注册指令类型
 */
#ifndef _MSC_VER
const DesignationTransType_def AgwDesignationTransType::kNew;  ///< 新注册请求
#endif
#ifndef _MSC_VER
const DesignationTransType_def AgwDesignationTransType::kCancel;  ///< 注册撤销
#endif
std::map<DesignationTransType_def, std::string> AgwDesignationTransType::const_desc_g;
std::map<DesignationTransType_def, std::string> AgwDesignationTransType::const_summary_g;
bool AgwDesignationTransType::init_desc_g = AgwDesignationTransType::InitDesc();
bool AgwDesignationTransType::init_summary_g = AgwDesignationTransType::InitSummary();
/*
 * @brief 执行编号类型
 */
const ExecType_def AgwExecType::kNew = '0';  ///< 新订单
const ExecType_def AgwExecType::kInternal = '1';  ///< 内部执行
const ExecType_def AgwExecType::kCancelled = '4';  ///< 已撤销
const ExecType_def AgwExecType::kReject = '8';  ///< 已拒绝
const ExecType_def AgwExecType::kTrade = 'F';  ///< 已成交
const ExecType_def AgwExecType::kGold = 'J';  ///< 黄金交易所成交
std::map<ExecType_def, std::string> AgwExecType::const_desc_g;
std::map<ExecType_def, std::string> AgwExecType::const_summary_g;
bool AgwExecType::init_desc_g = AgwExecType::InitDesc();
bool AgwExecType::init_summary_g = AgwExecType::InitSummary();
/*
 * @brief 证券所属市场
 */
#ifndef _MSC_VER
const MarketID_def AgwMarketID::kNULL;  ///< 空
#endif
#ifndef _MSC_VER
const MarketID_def AgwMarketID::kShangHai;  ///< 上海
#endif
#ifndef _MSC_VER
const MarketID_def AgwMarketID::kShenZhen;  ///< 深圳
#endif
#ifndef _MSC_VER
const MarketID_def AgwMarketID::kHKTShangHai;  ///< 港股通-沪市
#endif
#ifndef _MSC_VER
const MarketID_def AgwMarketID::kHKTShenZhen;  ///< 港股通-深市
#endif
#ifndef _MSC_VER
const MarketID_def AgwMarketID::kThird;  ///< 北京
#endif
std::map<MarketID_def, std::string> AgwMarketID::const_desc_g;
std::map<MarketID_def, std::string> AgwMarketID::const_summary_g;
bool AgwMarketID::init_desc_g = AgwMarketID::InitDesc();
bool AgwMarketID::init_summary_g = AgwMarketID::InitSummary();
/*
 * @brief 订单状态
 */
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kNew;  ///< 已申报
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kPartiallyFilled;  ///< 部分成交
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kFilled;  ///< 全部成交
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kPartiallyFilledPartiallyCancelled;  ///< 部分成交部分撤销
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kCancelled;  ///< 已撤销
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kPartiallyCancelled;  ///< 部分撤销
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kReject;  ///< 已拒绝
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kUnSend;  ///< 未申报
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kSended;  ///< 正报
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kWaitCancelled;  ///< 待撤销
#endif
#ifndef _MSC_VER
const OrdStatus_def AgwOrdStatus::kPartiallyFilledWaitCancelled;  ///< 部成待撤
#endif
std::map<OrdStatus_def, std::string> AgwOrdStatus::const_desc_g;
std::map<OrdStatus_def, std::string> AgwOrdStatus::const_summary_g;
bool AgwOrdStatus::init_desc_g = AgwOrdStatus::InitDesc();
bool AgwOrdStatus::init_summary_g = AgwOrdStatus::InitSummary();
/*
 * @brief 订单类型
 */
const OrdType_def AgwOrdType::kMarket = '1';  ///< 市价
const OrdType_def AgwOrdType::kFixed = '2';  ///< 限价
const OrdType_def AgwOrdType::kLocalOptimal = 'U';  ///< 本方最优
const OrdType_def AgwOrdType::kTransferFixed = 'K';  ///< 市价剩余转限价,适用于上交所
std::map<OrdType_def, std::string> AgwOrdType::const_desc_g;
std::map<OrdType_def, std::string> AgwOrdType::const_summary_g;
bool AgwOrdType::init_desc_g = AgwOrdType::InitDesc();
bool AgwOrdType::init_summary_g = AgwOrdType::InitSummary();
/*
 * @brief 开平仓标识
 */
const PositionEffect_def AgwPositionEffect::kOpen = 'O';  ///< 开仓
const PositionEffect_def AgwPositionEffect::kClose = 'C';  ///< 平仓
std::map<PositionEffect_def, std::string> AgwPositionEffect::const_desc_g;
std::map<PositionEffect_def, std::string> AgwPositionEffect::const_summary_g;
bool AgwPositionEffect::init_desc_g = AgwPositionEffect::InitDesc();
bool AgwPositionEffect::init_summary_g = AgwPositionEffect::InitSummary();
/*
 * @brief 排序方法
 */
#ifndef _MSC_VER
const ReturnSeq_def AgwReturnSeq::kTimeOrder;  ///< 按时间正序
#endif
#ifndef _MSC_VER
const ReturnSeq_def AgwReturnSeq::kTimeOrderRe;  ///< 按时间倒序
#endif
std::map<ReturnSeq_def, std::string> AgwReturnSeq::const_desc_g;
std::map<ReturnSeq_def, std::string> AgwReturnSeq::const_summary_g;
bool AgwReturnSeq::init_desc_g = AgwReturnSeq::InitDesc();
bool AgwReturnSeq::init_summary_g = AgwReturnSeq::InitSummary();
/*
 * @brief 买卖方向
 */
const Side_def AgwSide::kAll = '0';  ///< 所有
const Side_def AgwSide::kBuy = '1';  ///< 买
const Side_def AgwSide::kSell = '2';  ///< 卖
const Side_def AgwSide::kRepo = '3';  ///< 正回购
const Side_def AgwSide::kAntiRepo = '4';  ///< 逆回购
const Side_def AgwSide::kSwap = '5';  ///< 转股
const Side_def AgwSide::kPutBack = '6';  ///< 回售
const Side_def AgwSide::kTenderOffer = '7';  ///< 预受要约
const Side_def AgwSide::kDisTenderOffer = '8';  ///< 解除预受要约
const Side_def AgwSide::kCall = '9';  ///< 认购
const Side_def AgwSide::kActivation = 'A';  ///< 密码激活
const Side_def AgwSide::kCancellation = 'C';  ///< 密码注销
const Side_def AgwSide::kPurchase = 'D';  ///< 申购
const Side_def AgwSide::kRedeem = 'E';  ///< 赎回
const Side_def AgwSide::kBorrow = 'G';  ///< 借入
const Side_def AgwSide::kLoan = 'F';  ///< 出借
const Side_def AgwSide::kDesignationTransfer = 'J';  ///< 转托管
const Side_def AgwSide::kRevertDesignationTransfer = 'K';  ///< 反向转托管
const Side_def AgwSide::kPhysicalPurchase = 'P';  ///< 实物申购
const Side_def AgwSide::kQuery = 'Q';  ///< 查询
const Side_def AgwSide::kPhysicalRedemption = 'R';  ///< 实物赎回
const Side_def AgwSide::kOpen = 'O';  ///< 分拆
const Side_def AgwSide::kMerge = 'M';  ///< 合并
const Side_def AgwSide::kPledgeIn = 'H';  ///< 入库
const Side_def AgwSide::kPledgeOut = 'I';  ///< 出库
const Side_def AgwSide::kLock = 'Z';  ///< 锁定
const Side_def AgwSide::kUnLock = 'Y';  ///< 解锁
const Side_def AgwSide::kDisposal = '@';  ///< 转处置
const Side_def AgwSide::kDisposalReturn = '%';  ///< 转处置返还
const Side_def AgwSide::kTransformIn = 'a';  ///< 转入
const Side_def AgwSide::kTransformOut = 'b';  ///< 转出
const Side_def AgwSide::kAddClose = 'c';  ///< 追加平仓
const Side_def AgwSide::kClearClose = 'd';  ///< 清偿平仓
const Side_def AgwSide::kAddForceSell = 'e';  ///< 追加强卖
const Side_def AgwSide::kClearForceSell = 'f';  ///< 清偿强卖
const Side_def AgwSide::kActivedSellRepay = 'g';  ///< 主动卖券还款
const Side_def AgwSide::kPutbackCancel = 'h';  ///< 回售撤销
const Side_def AgwSide::kCashCall = 'i';  ///< 网下现金认购
const Side_def AgwSide::kStockCall = 'j';  ///< 网下股票认购
const Side_def AgwSide::kConfirmTransBuy = 'k';  ///< 确认成交买
const Side_def AgwSide::kConfirmTransSell = 'l';  ///< 确认成交卖
std::map<Side_def, std::string> AgwSide::const_desc_g;
std::map<Side_def, std::string> AgwSide::const_summary_g;
bool AgwSide::init_desc_g = AgwSide::InitDesc();
bool AgwSide::init_summary_g = AgwSide::InitSummary();
/*
 * @brief 订单有效期类型,默认限价为GFD,市价为IOC
 */
const TimeInForce_def AgwTimeInForce::kGFD = '0';  ///< 当日有效
const TimeInForce_def AgwTimeInForce::kIOC = '3';  ///< 即时成交剩余撤销
const TimeInForce_def AgwTimeInForce::kFOK = '4';  ///< 即时全部成交或撤销,适用于上交所
const TimeInForce_def AgwTimeInForce::kAtCrossing = '9';  ///< 竞价限价盘,适用于深交所
std::map<TimeInForce_def, std::string> AgwTimeInForce::const_desc_g;
std::map<TimeInForce_def, std::string> AgwTimeInForce::const_summary_g;
bool AgwTimeInForce::init_desc_g = AgwTimeInForce::InitDesc();
bool AgwTimeInForce::init_summary_g = AgwTimeInForce::InitSummary();
/*
 * @brief 订单数量类型
 */
const LotType_def AgwLotType::kOddLot = '1';  ///< 零股订单
const LotType_def AgwLotType::kRoundLot = '2';  ///< 整手订单
std::map<LotType_def, std::string> AgwLotType::const_desc_g;
std::map<LotType_def, std::string> AgwLotType::const_summary_g;
bool AgwLotType::init_desc_g = AgwLotType::InitDesc();
bool AgwLotType::init_summary_g = AgwLotType::InitSummary();
/*
 * @brief 错误码类别
 */
#ifndef _MSC_VER
const RejectCodeType_def RejectCodeType::kNone;  ///< 所有
#endif
#ifndef _MSC_VER
const RejectCodeType_def RejectCodeType::kBusiness;  ///< 业务拒绝（包括AGW、ORS、COS拒绝）
#endif
#ifndef _MSC_VER
const RejectCodeType_def RejectCodeType::kExternalQuery;  ///< 外部查询拒绝
#endif
#ifndef _MSC_VER
const RejectCodeType_def RejectCodeType::kRealInstruction;  ///< 实时指令拒绝
#endif
#ifndef _MSC_VER
const RejectCodeType_def RejectCodeType::kExternalInstruction;  ///< 外部指令拒绝
#endif
#ifndef _MSC_VER
const RejectCodeType_def RejectCodeType::kExchange;  ///< 交易所
#endif
std::map<RejectCodeType_def, std::string> RejectCodeType::const_desc_g;
std::map<RejectCodeType_def, std::string> RejectCodeType::const_summary_g;
bool RejectCodeType::init_desc_g = RejectCodeType::InitDesc();
bool RejectCodeType::init_summary_g = RejectCodeType::InitSummary();
/*
 * @brief 持仓类型
 */
const LegSide_def AgwLegSide::kLongPosition = '1';  ///< 权利仓
const LegSide_def AgwLegSide::kShortPosition = '2';  ///< 义务仓
std::map<LegSide_def, std::string> AgwLegSide::const_desc_g;
std::map<LegSide_def, std::string> AgwLegSide::const_summary_g;
bool AgwLegSide::init_desc_g = AgwLegSide::InitDesc();
bool AgwLegSide::init_summary_g = AgwLegSide::InitSummary();
/*
 * @brief 信用方向
 */
const CreditDirect_def CreditDirect::kMarginBuy = '0';  ///< 融资买入
const CreditDirect_def CreditDirect::kMarginSell = '1';  ///< 融券卖出
std::map<CreditDirect_def, std::string> CreditDirect::const_desc_g;
std::map<CreditDirect_def, std::string> CreditDirect::const_summary_g;
bool CreditDirect::init_desc_g = CreditDirect::InitDesc();
bool CreditDirect::init_summary_g = CreditDirect::InitSummary();
/*
 * @brief 负债合约明细类型
 */
const DebtsDetailKind_def DebtsDetailKind::kLittleLoan = 'A';  ///< 小贷
const DebtsDetailKind_def DebtsDetailKind::kLockShare = 'B';  ///< 锁券
const DebtsDetailKind_def DebtsDetailKind::kSpec = '0';  ///< 专户
std::map<DebtsDetailKind_def, std::string> DebtsDetailKind::const_desc_g;
std::map<DebtsDetailKind_def, std::string> DebtsDetailKind::const_summary_g;
bool DebtsDetailKind::init_desc_g = DebtsDetailKind::InitDesc();
bool DebtsDetailKind::init_summary_g = DebtsDetailKind::InitSummary();
/*
 * @brief 查询方向
 */
const CreditDebtsQueryFlag_def CreditDebtsQueryFlag::kNULL = ' ';  ///< 空值
const CreditDebtsQueryFlag_def CreditDebtsQueryFlag::kDESC = '1';  ///< 倒序
const CreditDebtsQueryFlag_def CreditDebtsQueryFlag::kASC = '0';  ///< 顺序
std::map<CreditDebtsQueryFlag_def, std::string> CreditDebtsQueryFlag::const_desc_g;
std::map<CreditDebtsQueryFlag_def, std::string> CreditDebtsQueryFlag::const_summary_g;
bool CreditDebtsQueryFlag::init_desc_g = CreditDebtsQueryFlag::InitDesc();
bool CreditDebtsQueryFlag::init_summary_g = CreditDebtsQueryFlag::InitSummary();
/*
 * @brief 锁券状态
 */
const LockStatus_def LockStatus::kUnAssigned = '0';  ///< 有未分配(有未分配数量reserveqty>0)
const LockStatus_def LockStatus::kAssigned = '1';  ///< 已分配(reserveqty = 0)
const LockStatus_def LockStatus::kOutstanding = '2';  ///< 过期未偿还(不清除,如果没有产生费用则演变成3)
const LockStatus_def LockStatus::kRepaid = '3';  ///< 过期已偿还(记录删除或归历史)
std::map<LockStatus_def, std::string> LockStatus::const_desc_g;
std::map<LockStatus_def, std::string> LockStatus::const_summary_g;
bool LockStatus::init_desc_g = LockStatus::InitDesc();
bool LockStatus::init_summary_g = LockStatus::InitSummary();
/*
 * @brief 合同状态
 */
const ContractStatus_def ContractStatus::kNormal = '0';  ///< 正常
const ContractStatus_def ContractStatus::kNotStarted = '1';  ///< 未启用
const ContractStatus_def ContractStatus::kStoped = '2';  ///< 终止
std::map<ContractStatus_def, std::string> ContractStatus::const_desc_g;
std::map<ContractStatus_def, std::string> ContractStatus::const_summary_g;
bool ContractStatus::init_desc_g = ContractStatus::InitDesc();
bool ContractStatus::init_summary_g = ContractStatus::InitSummary();
/*
 * @brief 申请状态
 */
const ApplyStatus_def ApplyStatus::kNULL = ' ';  ///< 空值
const ApplyStatus_def ApplyStatus::kStar = '*';  ///< 星值
const ApplyStatus_def ApplyStatus::kNotApproved = '0';  ///< 未审批
const ApplyStatus_def ApplyStatus::kApproval = '1';  ///< 审批同意
const ApplyStatus_def ApplyStatus::kReject = '2';  ///< 审批拒绝
std::map<ApplyStatus_def, std::string> ApplyStatus::const_desc_g;
std::map<ApplyStatus_def, std::string> ApplyStatus::const_summary_g;
bool ApplyStatus::init_desc_g = ApplyStatus::InitDesc();
bool ApplyStatus::init_summary_g = ApplyStatus::InitSummary();
/*
 * @brief 新三板认购结果
 */
const ThirdSubscribeResult_def ThirdSubscribeResult::kMayPurchase = '0';  ///< 可以参与申购
const ThirdSubscribeResult_def ThirdSubscribeResult::kForbidPurchase = '1';  ///< 禁止参与申购
const ThirdSubscribeResult_def ThirdSubscribeResult::kMustPurchase = '2';  ///< 必须参与申购
const ThirdSubscribeResult_def ThirdSubscribeResult::kNotPurchase = '3';  ///< 客户未参与询价
std::map<ThirdSubscribeResult_def, std::string> ThirdSubscribeResult::const_desc_g;
std::map<ThirdSubscribeResult_def, std::string> ThirdSubscribeResult::const_summary_g;
bool ThirdSubscribeResult::init_desc_g = ThirdSubscribeResult::InitDesc();
bool ThirdSubscribeResult::init_summary_g = ThirdSubscribeResult::InitSummary();
/*
 * @brief 新三板发行方式
 */
const IssueFlag_def IssueFlag::kSubscribe = '1';  ///< 询价阶段
const IssueFlag_def IssueFlag::kPurchase = '2';  ///< 申购阶段
const IssueFlag_def IssueFlag::kAuction = '3';  ///< 竞价阶段
std::map<IssueFlag_def, std::string> IssueFlag::const_desc_g;
std::map<IssueFlag_def, std::string> IssueFlag::const_summary_g;
bool IssueFlag::init_desc_g = IssueFlag::InitDesc();
bool IssueFlag::init_summary_g = IssueFlag::InitSummary();
/*
 * @brief UTE回切范围类型
 */
#ifndef _MSC_VER
const UTERevertScopeType_def UTERevertScopeType::kNull;  ///< 空
#endif
#ifndef _MSC_VER
const UTERevertScopeType_def UTERevertScopeType::kSingleAccountRevert;  ///< 单客户回切
#endif
#ifndef _MSC_VER
const UTERevertScopeType_def UTERevertScopeType::kUTENodeRevert;  ///< 节点回切
#endif
std::map<UTERevertScopeType_def, std::string> UTERevertScopeType::const_desc_g;
std::map<UTERevertScopeType_def, std::string> UTERevertScopeType::const_summary_g;
bool UTERevertScopeType::init_desc_g = UTERevertScopeType::InitDesc();
bool UTERevertScopeType::init_summary_g = UTERevertScopeType::InitSummary();
}
}

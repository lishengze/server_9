#ifndef ATP_GW_CONSTANTS__H_
#define ATP_GW_CONSTANTS__H_
#include <map>
#include <string>
#include "export_macro.h"
#include "types_agw_external.h"
namespace atp
{
namespace gw
{
/*
 * @brief 成交申报模式
 */
struct EXPORT_MACRO_DEFINE TradeHandlingInstr
{
    static const TradeHandlingInstr_def kTransReport;  ///< 成交报告
    static const TradeHandlingInstr_def kProtocolMatch;  ///< 协议配对
    static const TradeHandlingInstr_def kTransRequest;  ///< 成交请求
/*
 * @brief msg check
 */
    static bool IsValid(const TradeHandlingInstr_def& value)
    {
        if (
            value != TradeHandlingInstr::kTransReport
            && value != TradeHandlingInstr::kProtocolMatch
            && value != TradeHandlingInstr::kTransRequest
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const TradeHandlingInstr_def& value)
    {
        std::map<TradeHandlingInstr_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[TradeHandlingInstr::kTransReport] = "成交报告";
        const_desc_g[TradeHandlingInstr::kProtocolMatch] = "协议配对";
        const_desc_g[TradeHandlingInstr::kTransRequest] = "成交请求";
        return true;
    }
    static std::string GetSummary(const TradeHandlingInstr_def& value)
    {
        std::map<TradeHandlingInstr_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[TradeHandlingInstr::kTransReport] = "";
        const_summary_g[TradeHandlingInstr::kProtocolMatch] = "";
        const_summary_g[TradeHandlingInstr::kTransRequest] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<TradeHandlingInstr_def, std::string> const_desc_g;
    static std::map<TradeHandlingInstr_def, std::string> const_summary_g;
};
/*
 * @brief 结算方式
 */
struct EXPORT_MACRO_DEFINE SettlType
{
    static const SettlType_def kMultLateralNetting = 103;  ///< 多边净额
    static const SettlType_def kOneByOneFullAmount = 104;  ///< 逐笔全额
/*
 * @brief msg check
 */
    static bool IsValid(const SettlType_def& value)
    {
        if (
            value != SettlType::kMultLateralNetting
            && value != SettlType::kOneByOneFullAmount
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const SettlType_def& value)
    {
        std::map<SettlType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[SettlType::kMultLateralNetting] = "多边净额";
        const_desc_g[SettlType::kOneByOneFullAmount] = "逐笔全额";
        return true;
    }
    static std::string GetSummary(const SettlType_def& value)
    {
        std::map<SettlType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[SettlType::kMultLateralNetting] = "";
        const_summary_g[SettlType::kOneByOneFullAmount] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<SettlType_def, std::string> const_desc_g;
    static std::map<SettlType_def, std::string> const_summary_g;
};
/*
 * @brief 交易主体类型
 */
struct EXPORT_MACRO_DEFINE InvestorType
{
    static const InvestorType_def kSelf;  ///< 自营
    static const InvestorType_def kAsset;  ///< 资管
    static const InvestorType_def kInstitutional;  ///< 机构经纪
    static const InvestorType_def kPersonal;  ///< 个人经纪
/*
 * @brief msg check
 */
    static bool IsValid(const InvestorType_def& value)
    {
        if (
            value != InvestorType::kSelf
            && value != InvestorType::kAsset
            && value != InvestorType::kInstitutional
            && value != InvestorType::kPersonal
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const InvestorType_def& value)
    {
        std::map<InvestorType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[InvestorType::kSelf] = "自营";
        const_desc_g[InvestorType::kAsset] = "资管";
        const_desc_g[InvestorType::kInstitutional] = "机构经纪";
        const_desc_g[InvestorType::kPersonal] = "个人经纪";
        return true;
    }
    static std::string GetSummary(const InvestorType_def& value)
    {
        std::map<InvestorType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[InvestorType::kSelf] = "";
        const_summary_g[InvestorType::kAsset] = "";
        const_summary_g[InvestorType::kInstitutional] = "";
        const_summary_g[InvestorType::kPersonal] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<InvestorType_def, std::string> const_desc_g;
    static std::map<InvestorType_def, std::string> const_summary_g;
};
/*
 * @brief 交易员
 */
struct EXPORT_MACRO_DEFINE TraderCode
{
    static const TraderCode_def kNULL;  ///< 空
/*
 * @brief msg check
 */
    static bool IsValid(const TraderCode_def& value)
    {
        if (
            value != TraderCode::kNULL
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const TraderCode_def& value)
    {
        std::map<TraderCode_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[TraderCode::kNULL] = "空";
        return true;
    }
    static std::string GetSummary(const TraderCode_def& value)
    {
        std::map<TraderCode_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[TraderCode::kNULL] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<TraderCode_def, std::string> const_desc_g;
    static std::map<TraderCode_def, std::string> const_summary_g;
};
/*
 * @brief 成交申报事务类别
 */
struct EXPORT_MACRO_DEFINE TradeReportTransType
{
    static const TradeReportTransType_def kNew = 0;  ///< 普通申报
    static const TradeReportTransType_def kCancel = 1;  ///< 主动撤单
    static const TradeReportTransType_def kReplace = 2;  ///< 更新申报
/*
 * @brief msg check
 */
    static bool IsValid(const TradeReportTransType_def& value)
    {
        if (
            value != TradeReportTransType::kNew
            && value != TradeReportTransType::kCancel
            && value != TradeReportTransType::kReplace
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const TradeReportTransType_def& value)
    {
        std::map<TradeReportTransType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[TradeReportTransType::kNew] = "普通申报";
        const_desc_g[TradeReportTransType::kCancel] = "主动撤单";
        const_desc_g[TradeReportTransType::kReplace] = "更新申报";
        return true;
    }
    static std::string GetSummary(const TradeReportTransType_def& value)
    {
        std::map<TradeReportTransType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[TradeReportTransType::kNew] = "";
        const_summary_g[TradeReportTransType::kCancel] = "";
        const_summary_g[TradeReportTransType::kReplace] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<TradeReportTransType_def, std::string> const_desc_g;
    static std::map<TradeReportTransType_def, std::string> const_summary_g;
};
/*
 * @brief 成交申报类型
 */
struct EXPORT_MACRO_DEFINE TradeReportType
{
    static const TradeReportType_def kSubmitTransaction = 0;  ///< 提交成交申报
    static const TradeReportType_def kAcceptTransaction = 2;  ///< 接受成交申报
    static const TradeReportType_def kRefuseTransaction = 3;  ///< 拒绝成交申报
/*
 * @brief msg check
 */
    static bool IsValid(const TradeReportType_def& value)
    {
        if (
            value != TradeReportType::kSubmitTransaction
            && value != TradeReportType::kAcceptTransaction
            && value != TradeReportType::kRefuseTransaction
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const TradeReportType_def& value)
    {
        std::map<TradeReportType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[TradeReportType::kSubmitTransaction] = "提交成交申报";
        const_desc_g[TradeReportType::kAcceptTransaction] = "接受成交申报";
        const_desc_g[TradeReportType::kRefuseTransaction] = "拒绝成交申报";
        return true;
    }
    static std::string GetSummary(const TradeReportType_def& value)
    {
        std::map<TradeReportType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[TradeReportType::kSubmitTransaction] = "";
        const_summary_g[TradeReportType::kAcceptTransaction] = "";
        const_summary_g[TradeReportType::kRefuseTransaction] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<TradeReportType_def, std::string> const_desc_g;
    static std::map<TradeReportType_def, std::string> const_summary_g;
};
/*
 * @brief 特殊业务类型标识
 */
struct EXPORT_MACRO_DEFINE SpecBusiFlag
{
    static const Char_def kNULL;  ///< 默认值
    static const Char_def kRightsIssue;  ///< 配股
/*
 * @brief msg check
 */
    static bool IsValid(const Char_def& value)
    {
        if (
            value != SpecBusiFlag::kNULL
            && value != SpecBusiFlag::kRightsIssue
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const Char_def& value)
    {
        std::map<Char_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[SpecBusiFlag::kNULL] = "默认值";
        const_desc_g[SpecBusiFlag::kRightsIssue] = "配股";
        return true;
    }
    static std::string GetSummary(const Char_def& value)
    {
        std::map<Char_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[SpecBusiFlag::kNULL] = "";
        const_summary_g[SpecBusiFlag::kRightsIssue] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<Char_def, std::string> const_desc_g;
    static std::map<Char_def, std::string> const_summary_g;
};
/*
 * @brief 网关业务拒绝代码
 */
struct EXPORT_MACRO_DEFINE AgwBizRejCode
{
    static const RejectReasonCode_def kRejSwitchOff = 101;  ///< 网关开关关闭
    static const RejectReasonCode_def kRejAccountid = 102;  ///< 股东账户拒绝
    static const RejectReasonCode_def kRejBusinessType = 103;  ///< 业务类型错误
    static const RejectReasonCode_def kRejMarketID = 104;  ///< 市场字段错误
    static const RejectReasonCode_def kRejSide = 105;  ///< 买卖方向错误
    static const RejectReasonCode_def kRejOrdType = 106;  ///< 订单类型错误
    static const RejectReasonCode_def kRejTimeInForce = 107;  ///< 订单有效期错误
    static const RejectReasonCode_def kRejCashMargin = 108;  ///< 信用标识错误
    static const RejectReasonCode_def kRejPositionEffect = 109;  ///< 开平仓标识错误
    static const RejectReasonCode_def kRejCoveredOrUncovered = 110;  ///< 备兑标识错误
    static const RejectReasonCode_def kRejDesignationTransType = 111;  ///< 转托管注册类型错误
    static const RejectReasonCode_def kRejSecurityid = 112;  ///< 证券代码错误
    static const RejectReasonCode_def kRejOrderQty = 113;  ///< 订单申报数量错误
    static const RejectReasonCode_def kRejPrice = 114;  ///< 价格错误
    static const RejectReasonCode_def kRejWhitList = 115;  ///< IP限制错误
    static const RejectReasonCode_def kRejCurrency = 116;  ///< 币种错误
    static const RejectReasonCode_def kRejTransferWay = 117;  ///< 划转方式错误
    static const RejectReasonCode_def kQtyTypeError = 118;  ///< 订单数量类型错误
    static const RejectReasonCode_def kTpaRejGlobalRateLimit = 119;  ///< 全局流速限制
    static const RejectReasonCode_def kTpaRejIpMacRateLimit = 120;  ///< 终端流速限制
    static const RejectReasonCode_def kTpaRejCustIdNotInWhiteList = 121;  ///< 客户号不在白名单
    static const RejectReasonCode_def kTpaRejCustIdInBlackList = 122;  ///< 该资金账号在黑名单内
    static const RejectReasonCode_def kTpaRejFuncCodeNotFound = 123;  ///< 功能号权限限制
    static const RejectReasonCode_def kTpaRejMsgType = 124;  ///< Rgw不处理委托
    static const RejectReasonCode_def kTpaRejNeedFollowTheSessionProcess = 125;  ///< 未按照三方接入会话流程进行接入
    static const RejectReasonCode_def kRejAlreadyLogin = 126;  ///< 该客户号已经登录
    static const RejectReasonCode_def kRejNotLoggedin = 127;  ///< 该客户号未登录
    static const RejectReasonCode_def kRejIPAddress = 128;  ///< IP被禁止
    static const RejectReasonCode_def kRejT2SDK = 200;  ///< 恒生API错误
    static const RejectReasonCode_def kRejFundAccountId = 201;  ///< 恒生T2SDK FundAccountId错误
    static const RejectReasonCode_def kRejCheckCloseOutFlag = 202;  ///< 了结标识错误
    static const RejectReasonCode_def kRejLegSideErr = 203;  ///< 腿委托持仓类型错误
    static const RejectReasonCode_def kRejSecondaryOrderIdIsErr = 204;  ///< 组合解除时，组合编号不能为空
    static const RejectReasonCode_def kRejNotExistThePartition = 205;  ///< 无法在委托路由表中找到账户对应的分区号
    static const RejectReasonCode_def kRejNotExistTxEndpointPartitions = 206;  ///< 账户无法在该分区的网关报送消息，总线管理服务中未配置网关到引擎的主题分区
    static const RejectReasonCode_def kRejCheckSZCirculationType = 208;  ///< 深圳流通类型错误
    static const RejectReasonCode_def kCannotFindAccountID = 209;  ///< 不具备股东账户权限
    static const RejectReasonCode_def kNoMsgTypeNoRight = 210;  ///< 不具备功能号权限
    static const RejectReasonCode_def kRejStrategyIdIsErr = 211;  ///< 构建组合时，组合策略编码不能为空
    static const RejectReasonCode_def kTpaRejCustIDLimit = 212;  ///< 客户号超过了流速上限
    static const RejectReasonCode_def kTpaRejFuncCodeNotAllow = 213;  ///< 客户功能号被禁止
    static const RejectReasonCode_def kRejRepayWay = 214;  ///< 还款方式错误
    static const RejectReasonCode_def kTpaRejAGWFuncCodeNotAllow = 215;  ///< AGW功能号被禁止
    static const RejectReasonCode_def kFuncnoForbidden = 225;  ///< 功能号被阻断
    static const RejectReasonCode_def kFlowAccessCustIDAndProductIDSpeedReject = 226;  ///< 客户号和产品号流速超过
    static const RejectReasonCode_def kFlowAccessCustIDAndProductIDSumReject = 227;  ///< 客户号和产品号流量超过
    static const RejectReasonCode_def kFlowAccessUserIDSpeedReject = 228;  ///< 用户流速超过
    static const RejectReasonCode_def kFlowAccessUserIDSumReject = 229;  ///< 用户流量超过
    static const RejectReasonCode_def kFlowAccessAgwSpeedReject = 230;  ///< agw流速超过
    static const RejectReasonCode_def kFlowAccessAgwSumReject = 231;  ///< agw流量超过
    static const RejectReasonCode_def kFlowAccessAgwSwitchReject = 232;  ///< agw接入开关关闭
    static const RejectReasonCode_def kFlowAccessUserIDSwitchReject = 233;  ///< 用户接入开关关闭
    static const RejectReasonCode_def kFlowAccessCustIDAndProductIDSwitchReject = 234;  ///< 客户号和产品号接入开关关闭
    static const RejectReasonCode_def kRiskControlNoConnectionReject = 235;  ///< 风控无连接拒绝
    static const RejectReasonCode_def kRejOther = 999;  ///< 其他错误
/*
 * @brief msg check
 */
    static bool IsValid(const RejectReasonCode_def& value)
    {
        if (
            value != AgwBizRejCode::kRejSwitchOff
            && value != AgwBizRejCode::kRejAccountid
            && value != AgwBizRejCode::kRejBusinessType
            && value != AgwBizRejCode::kRejMarketID
            && value != AgwBizRejCode::kRejSide
            && value != AgwBizRejCode::kRejOrdType
            && value != AgwBizRejCode::kRejTimeInForce
            && value != AgwBizRejCode::kRejCashMargin
            && value != AgwBizRejCode::kRejPositionEffect
            && value != AgwBizRejCode::kRejCoveredOrUncovered
            && value != AgwBizRejCode::kRejDesignationTransType
            && value != AgwBizRejCode::kRejSecurityid
            && value != AgwBizRejCode::kRejOrderQty
            && value != AgwBizRejCode::kRejPrice
            && value != AgwBizRejCode::kRejWhitList
            && value != AgwBizRejCode::kRejCurrency
            && value != AgwBizRejCode::kRejTransferWay
            && value != AgwBizRejCode::kQtyTypeError
            && value != AgwBizRejCode::kTpaRejGlobalRateLimit
            && value != AgwBizRejCode::kTpaRejIpMacRateLimit
            && value != AgwBizRejCode::kTpaRejCustIdNotInWhiteList
            && value != AgwBizRejCode::kTpaRejCustIdInBlackList
            && value != AgwBizRejCode::kTpaRejFuncCodeNotFound
            && value != AgwBizRejCode::kTpaRejMsgType
            && value != AgwBizRejCode::kTpaRejNeedFollowTheSessionProcess
            && value != AgwBizRejCode::kRejAlreadyLogin
            && value != AgwBizRejCode::kRejNotLoggedin
            && value != AgwBizRejCode::kRejIPAddress
            && value != AgwBizRejCode::kRejT2SDK
            && value != AgwBizRejCode::kRejFundAccountId
            && value != AgwBizRejCode::kRejCheckCloseOutFlag
            && value != AgwBizRejCode::kRejLegSideErr
            && value != AgwBizRejCode::kRejSecondaryOrderIdIsErr
            && value != AgwBizRejCode::kRejNotExistThePartition
            && value != AgwBizRejCode::kRejNotExistTxEndpointPartitions
            && value != AgwBizRejCode::kRejCheckSZCirculationType
            && value != AgwBizRejCode::kCannotFindAccountID
            && value != AgwBizRejCode::kNoMsgTypeNoRight
            && value != AgwBizRejCode::kRejStrategyIdIsErr
            && value != AgwBizRejCode::kTpaRejCustIDLimit
            && value != AgwBizRejCode::kTpaRejFuncCodeNotAllow
            && value != AgwBizRejCode::kRejRepayWay
            && value != AgwBizRejCode::kTpaRejAGWFuncCodeNotAllow
            && value != AgwBizRejCode::kFuncnoForbidden
            && value != AgwBizRejCode::kFlowAccessCustIDAndProductIDSpeedReject
            && value != AgwBizRejCode::kFlowAccessCustIDAndProductIDSumReject
            && value != AgwBizRejCode::kFlowAccessUserIDSpeedReject
            && value != AgwBizRejCode::kFlowAccessUserIDSumReject
            && value != AgwBizRejCode::kFlowAccessAgwSpeedReject
            && value != AgwBizRejCode::kFlowAccessAgwSumReject
            && value != AgwBizRejCode::kFlowAccessAgwSwitchReject
            && value != AgwBizRejCode::kFlowAccessUserIDSwitchReject
            && value != AgwBizRejCode::kFlowAccessCustIDAndProductIDSwitchReject
            && value != AgwBizRejCode::kRiskControlNoConnectionReject
            && value != AgwBizRejCode::kRejOther
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const RejectReasonCode_def& value)
    {
        std::map<RejectReasonCode_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwBizRejCode::kRejSwitchOff] = "网关开关关闭";
        const_desc_g[AgwBizRejCode::kRejAccountid] = "股东账户拒绝";
        const_desc_g[AgwBizRejCode::kRejBusinessType] = "业务类型错误";
        const_desc_g[AgwBizRejCode::kRejMarketID] = "市场字段错误";
        const_desc_g[AgwBizRejCode::kRejSide] = "买卖方向错误";
        const_desc_g[AgwBizRejCode::kRejOrdType] = "订单类型错误";
        const_desc_g[AgwBizRejCode::kRejTimeInForce] = "订单有效期错误";
        const_desc_g[AgwBizRejCode::kRejCashMargin] = "信用标识错误";
        const_desc_g[AgwBizRejCode::kRejPositionEffect] = "开平仓标识错误";
        const_desc_g[AgwBizRejCode::kRejCoveredOrUncovered] = "备兑标识错误";
        const_desc_g[AgwBizRejCode::kRejDesignationTransType] = "转托管注册类型错误";
        const_desc_g[AgwBizRejCode::kRejSecurityid] = "证券代码错误";
        const_desc_g[AgwBizRejCode::kRejOrderQty] = "订单申报数量错误";
        const_desc_g[AgwBizRejCode::kRejPrice] = "价格错误";
        const_desc_g[AgwBizRejCode::kRejWhitList] = "IP限制错误";
        const_desc_g[AgwBizRejCode::kRejCurrency] = "币种错误";
        const_desc_g[AgwBizRejCode::kRejTransferWay] = "划转方式错误";
        const_desc_g[AgwBizRejCode::kQtyTypeError] = "订单数量类型错误";
        const_desc_g[AgwBizRejCode::kTpaRejGlobalRateLimit] = "全局流速限制";
        const_desc_g[AgwBizRejCode::kTpaRejIpMacRateLimit] = "终端流速限制";
        const_desc_g[AgwBizRejCode::kTpaRejCustIdNotInWhiteList] = "客户号不在白名单";
        const_desc_g[AgwBizRejCode::kTpaRejCustIdInBlackList] = "该资金账号在黑名单内";
        const_desc_g[AgwBizRejCode::kTpaRejFuncCodeNotFound] = "功能号权限限制";
        const_desc_g[AgwBizRejCode::kTpaRejMsgType] = "Rgw不处理委托";
        const_desc_g[AgwBizRejCode::kTpaRejNeedFollowTheSessionProcess] = "未按照三方接入会话流程进行接入";
        const_desc_g[AgwBizRejCode::kRejAlreadyLogin] = "该客户号已经登录";
        const_desc_g[AgwBizRejCode::kRejNotLoggedin] = "该客户号未登录";
        const_desc_g[AgwBizRejCode::kRejIPAddress] = "IP被禁止";
        const_desc_g[AgwBizRejCode::kRejT2SDK] = "恒生API错误";
        const_desc_g[AgwBizRejCode::kRejFundAccountId] = "恒生T2SDK FundAccountId错误";
        const_desc_g[AgwBizRejCode::kRejCheckCloseOutFlag] = "了结标识错误";
        const_desc_g[AgwBizRejCode::kRejLegSideErr] = "腿委托持仓类型错误";
        const_desc_g[AgwBizRejCode::kRejSecondaryOrderIdIsErr] = "组合解除时，组合编号不能为空";
        const_desc_g[AgwBizRejCode::kRejNotExistThePartition] = "无法在委托路由表中找到账户对应的分区号";
        const_desc_g[AgwBizRejCode::kRejNotExistTxEndpointPartitions] = "账户无法在该分区的网关报送消息，总线管理服务中未配置网关到引擎的主题分区";
        const_desc_g[AgwBizRejCode::kRejCheckSZCirculationType] = "深圳流通类型错误";
        const_desc_g[AgwBizRejCode::kCannotFindAccountID] = "不具备股东账户权限";
        const_desc_g[AgwBizRejCode::kNoMsgTypeNoRight] = "不具备功能号权限";
        const_desc_g[AgwBizRejCode::kRejStrategyIdIsErr] = "构建组合时，组合策略编码不能为空";
        const_desc_g[AgwBizRejCode::kTpaRejCustIDLimit] = "客户号超过了流速上限";
        const_desc_g[AgwBizRejCode::kTpaRejFuncCodeNotAllow] = "客户功能号被禁止";
        const_desc_g[AgwBizRejCode::kRejRepayWay] = "还款方式错误";
        const_desc_g[AgwBizRejCode::kTpaRejAGWFuncCodeNotAllow] = "AGW功能号被禁止";
        const_desc_g[AgwBizRejCode::kFuncnoForbidden] = "功能号被阻断";
        const_desc_g[AgwBizRejCode::kFlowAccessCustIDAndProductIDSpeedReject] = "客户号和产品号流速超过";
        const_desc_g[AgwBizRejCode::kFlowAccessCustIDAndProductIDSumReject] = "客户号和产品号流量超过";
        const_desc_g[AgwBizRejCode::kFlowAccessUserIDSpeedReject] = "用户流速超过";
        const_desc_g[AgwBizRejCode::kFlowAccessUserIDSumReject] = "用户流量超过";
        const_desc_g[AgwBizRejCode::kFlowAccessAgwSpeedReject] = "agw流速超过";
        const_desc_g[AgwBizRejCode::kFlowAccessAgwSumReject] = "agw流量超过";
        const_desc_g[AgwBizRejCode::kFlowAccessAgwSwitchReject] = "agw接入开关关闭";
        const_desc_g[AgwBizRejCode::kFlowAccessUserIDSwitchReject] = "用户接入开关关闭";
        const_desc_g[AgwBizRejCode::kFlowAccessCustIDAndProductIDSwitchReject] = "客户号和产品号接入开关关闭";
        const_desc_g[AgwBizRejCode::kRiskControlNoConnectionReject] = "风控无连接拒绝";
        const_desc_g[AgwBizRejCode::kRejOther] = "其他错误";
        return true;
    }
    static std::string GetSummary(const RejectReasonCode_def& value)
    {
        std::map<RejectReasonCode_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwBizRejCode::kRejSwitchOff] = "";
        const_summary_g[AgwBizRejCode::kRejAccountid] = "";
        const_summary_g[AgwBizRejCode::kRejBusinessType] = "";
        const_summary_g[AgwBizRejCode::kRejMarketID] = "";
        const_summary_g[AgwBizRejCode::kRejSide] = "";
        const_summary_g[AgwBizRejCode::kRejOrdType] = "";
        const_summary_g[AgwBizRejCode::kRejTimeInForce] = "";
        const_summary_g[AgwBizRejCode::kRejCashMargin] = "";
        const_summary_g[AgwBizRejCode::kRejPositionEffect] = "";
        const_summary_g[AgwBizRejCode::kRejCoveredOrUncovered] = "";
        const_summary_g[AgwBizRejCode::kRejDesignationTransType] = "";
        const_summary_g[AgwBizRejCode::kRejSecurityid] = "";
        const_summary_g[AgwBizRejCode::kRejOrderQty] = "";
        const_summary_g[AgwBizRejCode::kRejPrice] = "";
        const_summary_g[AgwBizRejCode::kRejWhitList] = "";
        const_summary_g[AgwBizRejCode::kRejCurrency] = "";
        const_summary_g[AgwBizRejCode::kRejTransferWay] = "";
        const_summary_g[AgwBizRejCode::kQtyTypeError] = "";
        const_summary_g[AgwBizRejCode::kTpaRejGlobalRateLimit] = "";
        const_summary_g[AgwBizRejCode::kTpaRejIpMacRateLimit] = "";
        const_summary_g[AgwBizRejCode::kTpaRejCustIdNotInWhiteList] = "";
        const_summary_g[AgwBizRejCode::kTpaRejCustIdInBlackList] = "";
        const_summary_g[AgwBizRejCode::kTpaRejFuncCodeNotFound] = "";
        const_summary_g[AgwBizRejCode::kTpaRejMsgType] = "";
        const_summary_g[AgwBizRejCode::kTpaRejNeedFollowTheSessionProcess] = "";
        const_summary_g[AgwBizRejCode::kRejAlreadyLogin] = "";
        const_summary_g[AgwBizRejCode::kRejNotLoggedin] = "";
        const_summary_g[AgwBizRejCode::kRejIPAddress] = "";
        const_summary_g[AgwBizRejCode::kRejT2SDK] = "";
        const_summary_g[AgwBizRejCode::kRejFundAccountId] = "";
        const_summary_g[AgwBizRejCode::kRejCheckCloseOutFlag] = "";
        const_summary_g[AgwBizRejCode::kRejLegSideErr] = "";
        const_summary_g[AgwBizRejCode::kRejSecondaryOrderIdIsErr] = "";
        const_summary_g[AgwBizRejCode::kRejNotExistThePartition] = "";
        const_summary_g[AgwBizRejCode::kRejNotExistTxEndpointPartitions] = "";
        const_summary_g[AgwBizRejCode::kRejCheckSZCirculationType] = "";
        const_summary_g[AgwBizRejCode::kCannotFindAccountID] = "";
        const_summary_g[AgwBizRejCode::kNoMsgTypeNoRight] = "";
        const_summary_g[AgwBizRejCode::kRejStrategyIdIsErr] = "";
        const_summary_g[AgwBizRejCode::kTpaRejCustIDLimit] = "";
        const_summary_g[AgwBizRejCode::kTpaRejFuncCodeNotAllow] = "";
        const_summary_g[AgwBizRejCode::kRejRepayWay] = "";
        const_summary_g[AgwBizRejCode::kTpaRejAGWFuncCodeNotAllow] = "";
        const_summary_g[AgwBizRejCode::kFuncnoForbidden] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessCustIDAndProductIDSpeedReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessCustIDAndProductIDSumReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessUserIDSpeedReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessUserIDSumReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessAgwSpeedReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessAgwSumReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessAgwSwitchReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessUserIDSwitchReject] = "";
        const_summary_g[AgwBizRejCode::kFlowAccessCustIDAndProductIDSwitchReject] = "";
        const_summary_g[AgwBizRejCode::kRiskControlNoConnectionReject] = "";
        const_summary_g[AgwBizRejCode::kRejOther] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<RejectReasonCode_def, std::string> const_desc_g;
    static std::map<RejectReasonCode_def, std::string> const_summary_g;
};
/*
 * @brief 外部委托查询的委托状态条件
 */
struct EXPORT_MACRO_DEFINE AgwOrderQueryCondition
{
    static const OrderQueryCondition_def kAll = 0;  ///< 查询全部
    static const OrderQueryCondition_def kCancelAvailable = 1;  ///< 查询可撤单委托
    static const OrderQueryCondition_def kTradeExecOrCanceled = 2;  ///< 查询有成交或已经撤单委托
    static const OrderQueryCondition_def kUnavailableCancel = 3;  ///< 查询不可撤单委托
/*
 * @brief msg check
 */
    static bool IsValid(const OrderQueryCondition_def& value)
    {
        if (
            value != AgwOrderQueryCondition::kAll
            && value != AgwOrderQueryCondition::kCancelAvailable
            && value != AgwOrderQueryCondition::kTradeExecOrCanceled
            && value != AgwOrderQueryCondition::kUnavailableCancel
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const OrderQueryCondition_def& value)
    {
        std::map<OrderQueryCondition_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwOrderQueryCondition::kAll] = "查询全部";
        const_desc_g[AgwOrderQueryCondition::kCancelAvailable] = "查询可撤单委托";
        const_desc_g[AgwOrderQueryCondition::kTradeExecOrCanceled] = "查询有成交或已经撤单委托";
        const_desc_g[AgwOrderQueryCondition::kUnavailableCancel] = "查询不可撤单委托";
        return true;
    }
    static std::string GetSummary(const OrderQueryCondition_def& value)
    {
        std::map<OrderQueryCondition_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwOrderQueryCondition::kAll] = "";
        const_summary_g[AgwOrderQueryCondition::kCancelAvailable] = "";
        const_summary_g[AgwOrderQueryCondition::kTradeExecOrCanceled] = "";
        const_summary_g[AgwOrderQueryCondition::kUnavailableCancel] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<OrderQueryCondition_def, std::string> const_desc_g;
    static std::map<OrderQueryCondition_def, std::string> const_summary_g;
};
/*
 * @brief 网关实时指令类型
 */
struct EXPORT_MACRO_DEFINE AgwInstructionType
{
    static const InstructionType_def kFundAdjust = 11;  ///< 资金实时调整
    static const InstructionType_def kSharePositionAdjust = 12;  ///< 股份持仓实时调整
    static const InstructionType_def kBusinessRightsControl = 14;  ///< 实时限制交易
    static const InstructionType_def kBatchBusinessRightsControl = 21;  ///< 批量交易权限控制指令
    static const InstructionType_def kModifyCashAuctionParams = 23;  ///< 现货交易参数修改
    static const InstructionType_def kModifyETFBusinessRule = 24;  ///< ETF申赎业务规则修改
    static const InstructionType_def kModifyShareDayTrading = 26;  ///< 交易回转标记修改
    static const InstructionType_def kCustPwdAdjust = 35;  ///< 客户密码实时修改指令
    static const InstructionType_def kAgwSpeedAdjust = 41;  ///< 网关总流速总连接数调整
    static const InstructionType_def kAgwTerminalAdjust = 42;  ///< 网关终端信息调整
    static const InstructionType_def kAgwBlackWhiteListAdjust = 43;  ///< 网关黑白名单调整
    static const InstructionType_def kAgwFuncNoListAdjust = 44;  ///< 网关功能号调整
    static const InstructionType_def kAgwAccessControlDeleteAdjust = 45;  ///< 网关接入信息删除
    static const InstructionType_def kAgwAuthorizationAdjust = 49;  ///< 网关授权信息调整
/*
 * @brief msg check
 */
    static bool IsValid(const InstructionType_def& value)
    {
        if (
            value != AgwInstructionType::kFundAdjust
            && value != AgwInstructionType::kSharePositionAdjust
            && value != AgwInstructionType::kBusinessRightsControl
            && value != AgwInstructionType::kBatchBusinessRightsControl
            && value != AgwInstructionType::kModifyCashAuctionParams
            && value != AgwInstructionType::kModifyETFBusinessRule
            && value != AgwInstructionType::kModifyShareDayTrading
            && value != AgwInstructionType::kCustPwdAdjust
            && value != AgwInstructionType::kAgwSpeedAdjust
            && value != AgwInstructionType::kAgwTerminalAdjust
            && value != AgwInstructionType::kAgwBlackWhiteListAdjust
            && value != AgwInstructionType::kAgwFuncNoListAdjust
            && value != AgwInstructionType::kAgwAccessControlDeleteAdjust
            && value != AgwInstructionType::kAgwAuthorizationAdjust
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const InstructionType_def& value)
    {
        std::map<InstructionType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwInstructionType::kFundAdjust] = "资金实时调整";
        const_desc_g[AgwInstructionType::kSharePositionAdjust] = "股份持仓实时调整";
        const_desc_g[AgwInstructionType::kBusinessRightsControl] = "实时限制交易";
        const_desc_g[AgwInstructionType::kBatchBusinessRightsControl] = "批量交易权限控制指令";
        const_desc_g[AgwInstructionType::kModifyCashAuctionParams] = "现货交易参数修改";
        const_desc_g[AgwInstructionType::kModifyETFBusinessRule] = "ETF申赎业务规则修改";
        const_desc_g[AgwInstructionType::kModifyShareDayTrading] = "交易回转标记修改";
        const_desc_g[AgwInstructionType::kCustPwdAdjust] = "客户密码实时修改指令";
        const_desc_g[AgwInstructionType::kAgwSpeedAdjust] = "网关总流速总连接数调整";
        const_desc_g[AgwInstructionType::kAgwTerminalAdjust] = "网关终端信息调整";
        const_desc_g[AgwInstructionType::kAgwBlackWhiteListAdjust] = "网关黑白名单调整";
        const_desc_g[AgwInstructionType::kAgwFuncNoListAdjust] = "网关功能号调整";
        const_desc_g[AgwInstructionType::kAgwAccessControlDeleteAdjust] = "网关接入信息删除";
        const_desc_g[AgwInstructionType::kAgwAuthorizationAdjust] = "网关授权信息调整";
        return true;
    }
    static std::string GetSummary(const InstructionType_def& value)
    {
        std::map<InstructionType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwInstructionType::kFundAdjust] = "";
        const_summary_g[AgwInstructionType::kSharePositionAdjust] = "";
        const_summary_g[AgwInstructionType::kBusinessRightsControl] = "";
        const_summary_g[AgwInstructionType::kBatchBusinessRightsControl] = "";
        const_summary_g[AgwInstructionType::kModifyCashAuctionParams] = "";
        const_summary_g[AgwInstructionType::kModifyETFBusinessRule] = "";
        const_summary_g[AgwInstructionType::kModifyShareDayTrading] = "";
        const_summary_g[AgwInstructionType::kCustPwdAdjust] = "";
        const_summary_g[AgwInstructionType::kAgwSpeedAdjust] = "";
        const_summary_g[AgwInstructionType::kAgwTerminalAdjust] = "";
        const_summary_g[AgwInstructionType::kAgwBlackWhiteListAdjust] = "";
        const_summary_g[AgwInstructionType::kAgwFuncNoListAdjust] = "";
        const_summary_g[AgwInstructionType::kAgwAccessControlDeleteAdjust] = "";
        const_summary_g[AgwInstructionType::kAgwAuthorizationAdjust] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<InstructionType_def, std::string> const_desc_g;
    static std::map<InstructionType_def, std::string> const_summary_g;
};
/*
 * @brief 网关查询指令类型
 */
struct EXPORT_MACRO_DEFINE AgwQueryInstructionType
{
    static const InstructionType_def kFundDetailQuery = 34;  ///< 资金流水明细查询
    static const InstructionType_def kAccountShareDetailQuery = 35;  ///< 股份流水明细查询
/*
 * @brief msg check
 */
    static bool IsValid(const InstructionType_def& value)
    {
        if (
            value != AgwQueryInstructionType::kFundDetailQuery
            && value != AgwQueryInstructionType::kAccountShareDetailQuery
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const InstructionType_def& value)
    {
        std::map<InstructionType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwQueryInstructionType::kFundDetailQuery] = "资金流水明细查询";
        const_desc_g[AgwQueryInstructionType::kAccountShareDetailQuery] = "股份流水明细查询";
        return true;
    }
    static std::string GetSummary(const InstructionType_def& value)
    {
        std::map<InstructionType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwQueryInstructionType::kFundDetailQuery] = "";
        const_summary_g[AgwQueryInstructionType::kAccountShareDetailQuery] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<InstructionType_def, std::string> const_desc_g;
    static std::map<InstructionType_def, std::string> const_summary_g;
};
/*
 * @brief 业务类型
 */
struct EXPORT_MACRO_DEFINE AgwBusinessType
{
    static const BusinessType_def kAll = 0;  ///< 所有类型
    static const BusinessType_def kCashAuction = 1;  ///< 现货集中交易
    static const BusinessType_def kPledge = 2;  ///< 质押式回购交易
    static const BusinessType_def kBondDistribution = 3;  ///< 债券分销
    static const BusinessType_def kOptionAuction = 4;  ///< 期权集中交易
    static const BusinessType_def kBlockTrade = 5;  ///< 协议交易(大宗交易)
    static const BusinessType_def kAfterHoursPricing = 6;  ///< 盘后定价交易
    static const BusinessType_def kRefinancing = 7;  ///< 转融通
    static const BusinessType_def kAssetTransfer = 8;  ///< 资产管理计划份额转让
    static const BusinessType_def kCashPledgeRepo = 9;  ///< 股票质押式回购
    static const BusinessType_def kAgreedRepo = 10;  ///< 约定购回
    static const BusinessType_def kPledgeQuoteRepo = 11;  ///< 质押式报价回购
    static const BusinessType_def kETFRealTimePurchaseRedemption = 12;  ///< ETF实时申购赎回
    static const BusinessType_def kIssue = 13;  ///< 网上发行认购
    static const BusinessType_def kSubscriptionRatio = 14;  ///< 配股认购
    static const BusinessType_def kBondSellBack = 15;  ///< 债券转股回售
    static const BusinessType_def kOptionExercise = 16;  ///< 期权行权
    static const BusinessType_def kOpenFundPurchaseRedemption = 17;  ///< 开放式基金申购赎回
    static const BusinessType_def kTenderOffer = 18;  ///< 要约收购
    static const BusinessType_def kPledgeRepoPledgeOrSign = 19;  ///< 质押式回购质押解押
    static const BusinessType_def kDesignation = 20;  ///< 转托管
    static const BusinessType_def kVote = 21;  ///< 网络投票
    static const BusinessType_def kDesignatedTransaction = 22;  ///< 指定交易
    static const BusinessType_def kWarrantsExercise = 23;  ///< 权证行权
    static const BusinessType_def kPasswordService = 24;  ///< 密码服务
    static const BusinessType_def kOpenMarginQuery = 25;  ///< 保证金查询
    static const BusinessType_def kOpenFundSetting = 26;  ///< 开放式基金转托管、分红方式设置
    static const BusinessType_def kTurnToDisposal = 27;  ///< 转处置
    static const BusinessType_def kCouponSwap = 28;  ///< 垫券还券
    static const BusinessType_def kPayOff = 29;  ///< 待清偿扣划
    static const BusinessType_def kBondPledgeAgreementRepo = 30;  ///< 债券质押式协议回购
    static const BusinessType_def kLevelFundRealTimeOpenOrMerge = 31;  ///< 分级基金实时分拆合并
    static const BusinessType_def kCoverLockOrUnlock = 32;  ///< 备兑锁定解锁
    static const BusinessType_def kOptionCombination = 34;  ///< 期权组合策略保证金
    static const BusinessType_def kHKStocksThrough = 63;  ///< 港股通
    static const BusinessType_def kTibAfterHour = 97;  ///< 科创板股票盘后定价交易业务
    static const BusinessType_def kMarginTrade = 99;  ///< 融资融券交易业务
    static const BusinessType_def kCreditAuction = 200;  ///< 信用账户普通买入/普通卖出（卖券还款）交易业务
    static const BusinessType_def kMarginTrading = 201;  ///< 融资融券交易业务
    static const BusinessType_def kDirectRepay = 202;  ///< 直接还款
    static const BusinessType_def kCollateralInOrOut = 203;  ///< 担保品划转
    static const BusinessType_def kBuySecuRepay = 204;  ///< 买券还券
    static const BusinessType_def kCurSecuRepay = 205;  ///< 现券还券
    static const BusinessType_def kCreditVote = 207;  ///< 信用账户投票
    static const BusinessType_def kDirectRepayAppoint = 208;  ///< 指定合约还款
    static const BusinessType_def kThirdAgreementTransfer = 209;  ///< 股转协议转让
    static const BusinessType_def kThirdTenderOffer = 210;  ///< 股转要约收购
    static const BusinessType_def kThirdIssue = 211;  ///< 股转发行
    static const BusinessType_def kThirdCashAuction = 212;  ///< 新三板集中交易(挂牌做市, 股转挂牌连续竞价, 挂牌集合竞价)
    static const BusinessType_def kThirdBlockTrade = 213;  ///< 股转大宗交易
    static const BusinessType_def kThirdCashAuctionDelist = 214;  ///< 新三板集中交易(两网公司及退市公司股票)
    static const BusinessType_def kOfflineETFSubscribe = 215;  ///< 网下ETF发行认购
    static const BusinessType_def kLOFDividePolicySet = 217;  ///< LOF基金分红方式设置
    static const BusinessType_def kThirdDelistAgreement = 218;  ///< 股转摘牌协议转让
/*
 * @brief msg check
 */
    static bool IsValid(const BusinessType_def& value)
    {
        if (
            value != AgwBusinessType::kAll
            && value != AgwBusinessType::kCashAuction
            && value != AgwBusinessType::kPledge
            && value != AgwBusinessType::kBondDistribution
            && value != AgwBusinessType::kOptionAuction
            && value != AgwBusinessType::kBlockTrade
            && value != AgwBusinessType::kAfterHoursPricing
            && value != AgwBusinessType::kRefinancing
            && value != AgwBusinessType::kAssetTransfer
            && value != AgwBusinessType::kCashPledgeRepo
            && value != AgwBusinessType::kAgreedRepo
            && value != AgwBusinessType::kPledgeQuoteRepo
            && value != AgwBusinessType::kETFRealTimePurchaseRedemption
            && value != AgwBusinessType::kIssue
            && value != AgwBusinessType::kSubscriptionRatio
            && value != AgwBusinessType::kBondSellBack
            && value != AgwBusinessType::kOptionExercise
            && value != AgwBusinessType::kOpenFundPurchaseRedemption
            && value != AgwBusinessType::kTenderOffer
            && value != AgwBusinessType::kPledgeRepoPledgeOrSign
            && value != AgwBusinessType::kDesignation
            && value != AgwBusinessType::kVote
            && value != AgwBusinessType::kDesignatedTransaction
            && value != AgwBusinessType::kWarrantsExercise
            && value != AgwBusinessType::kPasswordService
            && value != AgwBusinessType::kOpenMarginQuery
            && value != AgwBusinessType::kOpenFundSetting
            && value != AgwBusinessType::kTurnToDisposal
            && value != AgwBusinessType::kCouponSwap
            && value != AgwBusinessType::kPayOff
            && value != AgwBusinessType::kBondPledgeAgreementRepo
            && value != AgwBusinessType::kLevelFundRealTimeOpenOrMerge
            && value != AgwBusinessType::kCoverLockOrUnlock
            && value != AgwBusinessType::kOptionCombination
            && value != AgwBusinessType::kHKStocksThrough
            && value != AgwBusinessType::kTibAfterHour
            && value != AgwBusinessType::kMarginTrade
            && value != AgwBusinessType::kCreditAuction
            && value != AgwBusinessType::kMarginTrading
            && value != AgwBusinessType::kDirectRepay
            && value != AgwBusinessType::kCollateralInOrOut
            && value != AgwBusinessType::kBuySecuRepay
            && value != AgwBusinessType::kCurSecuRepay
            && value != AgwBusinessType::kCreditVote
            && value != AgwBusinessType::kDirectRepayAppoint
            && value != AgwBusinessType::kThirdAgreementTransfer
            && value != AgwBusinessType::kThirdTenderOffer
            && value != AgwBusinessType::kThirdIssue
            && value != AgwBusinessType::kThirdCashAuction
            && value != AgwBusinessType::kThirdBlockTrade
            && value != AgwBusinessType::kThirdCashAuctionDelist
            && value != AgwBusinessType::kOfflineETFSubscribe
            && value != AgwBusinessType::kLOFDividePolicySet
            && value != AgwBusinessType::kThirdDelistAgreement
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const BusinessType_def& value)
    {
        std::map<BusinessType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwBusinessType::kAll] = "所有类型";
        const_desc_g[AgwBusinessType::kCashAuction] = "现货集中交易";
        const_desc_g[AgwBusinessType::kPledge] = "质押式回购交易";
        const_desc_g[AgwBusinessType::kBondDistribution] = "债券分销";
        const_desc_g[AgwBusinessType::kOptionAuction] = "期权集中交易";
        const_desc_g[AgwBusinessType::kBlockTrade] = "协议交易(大宗交易)";
        const_desc_g[AgwBusinessType::kAfterHoursPricing] = "盘后定价交易";
        const_desc_g[AgwBusinessType::kRefinancing] = "转融通";
        const_desc_g[AgwBusinessType::kAssetTransfer] = "资产管理计划份额转让";
        const_desc_g[AgwBusinessType::kCashPledgeRepo] = "股票质押式回购";
        const_desc_g[AgwBusinessType::kAgreedRepo] = "约定购回";
        const_desc_g[AgwBusinessType::kPledgeQuoteRepo] = "质押式报价回购";
        const_desc_g[AgwBusinessType::kETFRealTimePurchaseRedemption] = "ETF实时申购赎回";
        const_desc_g[AgwBusinessType::kIssue] = "网上发行认购";
        const_desc_g[AgwBusinessType::kSubscriptionRatio] = "配股认购";
        const_desc_g[AgwBusinessType::kBondSellBack] = "债券转股回售";
        const_desc_g[AgwBusinessType::kOptionExercise] = "期权行权";
        const_desc_g[AgwBusinessType::kOpenFundPurchaseRedemption] = "开放式基金申购赎回";
        const_desc_g[AgwBusinessType::kTenderOffer] = "要约收购";
        const_desc_g[AgwBusinessType::kPledgeRepoPledgeOrSign] = "质押式回购质押解押";
        const_desc_g[AgwBusinessType::kDesignation] = "转托管";
        const_desc_g[AgwBusinessType::kVote] = "网络投票";
        const_desc_g[AgwBusinessType::kDesignatedTransaction] = "指定交易";
        const_desc_g[AgwBusinessType::kWarrantsExercise] = "权证行权";
        const_desc_g[AgwBusinessType::kPasswordService] = "密码服务";
        const_desc_g[AgwBusinessType::kOpenMarginQuery] = "保证金查询";
        const_desc_g[AgwBusinessType::kOpenFundSetting] = "开放式基金转托管、分红方式设置";
        const_desc_g[AgwBusinessType::kTurnToDisposal] = "转处置";
        const_desc_g[AgwBusinessType::kCouponSwap] = "垫券还券";
        const_desc_g[AgwBusinessType::kPayOff] = "待清偿扣划";
        const_desc_g[AgwBusinessType::kBondPledgeAgreementRepo] = "债券质押式协议回购";
        const_desc_g[AgwBusinessType::kLevelFundRealTimeOpenOrMerge] = "分级基金实时分拆合并";
        const_desc_g[AgwBusinessType::kCoverLockOrUnlock] = "备兑锁定解锁";
        const_desc_g[AgwBusinessType::kOptionCombination] = "期权组合策略保证金";
        const_desc_g[AgwBusinessType::kHKStocksThrough] = "港股通";
        const_desc_g[AgwBusinessType::kTibAfterHour] = "科创板股票盘后定价交易业务";
        const_desc_g[AgwBusinessType::kMarginTrade] = "融资融券交易业务";
        const_desc_g[AgwBusinessType::kCreditAuction] = "信用账户普通买入/普通卖出（卖券还款）交易业务";
        const_desc_g[AgwBusinessType::kMarginTrading] = "融资融券交易业务";
        const_desc_g[AgwBusinessType::kDirectRepay] = "直接还款";
        const_desc_g[AgwBusinessType::kCollateralInOrOut] = "担保品划转";
        const_desc_g[AgwBusinessType::kBuySecuRepay] = "买券还券";
        const_desc_g[AgwBusinessType::kCurSecuRepay] = "现券还券";
        const_desc_g[AgwBusinessType::kCreditVote] = "信用账户投票";
        const_desc_g[AgwBusinessType::kDirectRepayAppoint] = "指定合约还款";
        const_desc_g[AgwBusinessType::kThirdAgreementTransfer] = "股转协议转让";
        const_desc_g[AgwBusinessType::kThirdTenderOffer] = "股转要约收购";
        const_desc_g[AgwBusinessType::kThirdIssue] = "股转发行";
        const_desc_g[AgwBusinessType::kThirdCashAuction] = "新三板集中交易(挂牌做市, 股转挂牌连续竞价, 挂牌集合竞价)";
        const_desc_g[AgwBusinessType::kThirdBlockTrade] = "股转大宗交易";
        const_desc_g[AgwBusinessType::kThirdCashAuctionDelist] = "新三板集中交易(两网公司及退市公司股票)";
        const_desc_g[AgwBusinessType::kOfflineETFSubscribe] = "网下ETF发行认购";
        const_desc_g[AgwBusinessType::kLOFDividePolicySet] = "LOF基金分红方式设置";
        const_desc_g[AgwBusinessType::kThirdDelistAgreement] = "股转摘牌协议转让";
        return true;
    }
    static std::string GetSummary(const BusinessType_def& value)
    {
        std::map<BusinessType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwBusinessType::kAll] = "";
        const_summary_g[AgwBusinessType::kCashAuction] = "";
        const_summary_g[AgwBusinessType::kPledge] = "";
        const_summary_g[AgwBusinessType::kBondDistribution] = "";
        const_summary_g[AgwBusinessType::kOptionAuction] = "";
        const_summary_g[AgwBusinessType::kBlockTrade] = "";
        const_summary_g[AgwBusinessType::kAfterHoursPricing] = "";
        const_summary_g[AgwBusinessType::kRefinancing] = "";
        const_summary_g[AgwBusinessType::kAssetTransfer] = "";
        const_summary_g[AgwBusinessType::kCashPledgeRepo] = "";
        const_summary_g[AgwBusinessType::kAgreedRepo] = "";
        const_summary_g[AgwBusinessType::kPledgeQuoteRepo] = "";
        const_summary_g[AgwBusinessType::kETFRealTimePurchaseRedemption] = "";
        const_summary_g[AgwBusinessType::kIssue] = "";
        const_summary_g[AgwBusinessType::kSubscriptionRatio] = "";
        const_summary_g[AgwBusinessType::kBondSellBack] = "";
        const_summary_g[AgwBusinessType::kOptionExercise] = "";
        const_summary_g[AgwBusinessType::kOpenFundPurchaseRedemption] = "";
        const_summary_g[AgwBusinessType::kTenderOffer] = "";
        const_summary_g[AgwBusinessType::kPledgeRepoPledgeOrSign] = "";
        const_summary_g[AgwBusinessType::kDesignation] = "";
        const_summary_g[AgwBusinessType::kVote] = "";
        const_summary_g[AgwBusinessType::kDesignatedTransaction] = "";
        const_summary_g[AgwBusinessType::kWarrantsExercise] = "";
        const_summary_g[AgwBusinessType::kPasswordService] = "";
        const_summary_g[AgwBusinessType::kOpenMarginQuery] = "";
        const_summary_g[AgwBusinessType::kOpenFundSetting] = "";
        const_summary_g[AgwBusinessType::kTurnToDisposal] = "";
        const_summary_g[AgwBusinessType::kCouponSwap] = "";
        const_summary_g[AgwBusinessType::kPayOff] = "";
        const_summary_g[AgwBusinessType::kBondPledgeAgreementRepo] = "";
        const_summary_g[AgwBusinessType::kLevelFundRealTimeOpenOrMerge] = "";
        const_summary_g[AgwBusinessType::kCoverLockOrUnlock] = "";
        const_summary_g[AgwBusinessType::kOptionCombination] = "";
        const_summary_g[AgwBusinessType::kHKStocksThrough] = "";
        const_summary_g[AgwBusinessType::kTibAfterHour] = "";
        const_summary_g[AgwBusinessType::kMarginTrade] = "";
        const_summary_g[AgwBusinessType::kCreditAuction] = "";
        const_summary_g[AgwBusinessType::kMarginTrading] = "";
        const_summary_g[AgwBusinessType::kDirectRepay] = "";
        const_summary_g[AgwBusinessType::kCollateralInOrOut] = "";
        const_summary_g[AgwBusinessType::kBuySecuRepay] = "";
        const_summary_g[AgwBusinessType::kCurSecuRepay] = "";
        const_summary_g[AgwBusinessType::kCreditVote] = "";
        const_summary_g[AgwBusinessType::kDirectRepayAppoint] = "";
        const_summary_g[AgwBusinessType::kThirdAgreementTransfer] = "";
        const_summary_g[AgwBusinessType::kThirdTenderOffer] = "";
        const_summary_g[AgwBusinessType::kThirdIssue] = "";
        const_summary_g[AgwBusinessType::kThirdCashAuction] = "";
        const_summary_g[AgwBusinessType::kThirdBlockTrade] = "";
        const_summary_g[AgwBusinessType::kThirdCashAuctionDelist] = "";
        const_summary_g[AgwBusinessType::kOfflineETFSubscribe] = "";
        const_summary_g[AgwBusinessType::kLOFDividePolicySet] = "";
        const_summary_g[AgwBusinessType::kThirdDelistAgreement] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<BusinessType_def, std::string> const_desc_g;
    static std::map<BusinessType_def, std::string> const_summary_g;
};
/*
 * @brief 货币种类
 */
struct EXPORT_MACRO_DEFINE AgwCurrency
{
    static const Currency_def kCNY;  ///< 人民币
    static const Currency_def kUSD;  ///< 美元
    static const Currency_def kHKD;  ///< 港币
/*
 * @brief msg check
 */
    static bool IsValid(const Currency_def& value)
    {
        if (
            value != AgwCurrency::kCNY
            && value != AgwCurrency::kUSD
            && value != AgwCurrency::kHKD
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const Currency_def& value)
    {
        std::map<Currency_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwCurrency::kCNY] = "人民币";
        const_desc_g[AgwCurrency::kUSD] = "美元";
        const_desc_g[AgwCurrency::kHKD] = "港币";
        return true;
    }
    static std::string GetSummary(const Currency_def& value)
    {
        std::map<Currency_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwCurrency::kCNY] = "";
        const_summary_g[AgwCurrency::kUSD] = "";
        const_summary_g[AgwCurrency::kHKD] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<Currency_def, std::string> const_desc_g;
    static std::map<Currency_def, std::string> const_summary_g;
};
/*
 * @brief 认沽认购
 */
struct EXPORT_MACRO_DEFINE AgwCallOrPut
{
    static const CallOrPut_def kCall;  ///< 认购
    static const CallOrPut_def kPut;  ///< 认沽
/*
 * @brief msg check
 */
    static bool IsValid(const CallOrPut_def& value)
    {
        if (
            value != AgwCallOrPut::kCall
            && value != AgwCallOrPut::kPut
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const CallOrPut_def& value)
    {
        std::map<CallOrPut_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwCallOrPut::kCall] = "认购";
        const_desc_g[AgwCallOrPut::kPut] = "认沽";
        return true;
    }
    static std::string GetSummary(const CallOrPut_def& value)
    {
        std::map<CallOrPut_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwCallOrPut::kCall] = "";
        const_summary_g[AgwCallOrPut::kPut] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<CallOrPut_def, std::string> const_desc_g;
    static std::map<CallOrPut_def, std::string> const_summary_g;
};
/*
 * @brief 信用标识
 */
struct EXPORT_MACRO_DEFINE AgwCashMargin
{
    static const CashMargin_def kCash;  ///< 普通交易
    static const CashMargin_def kOpen;  ///< 融资融券开仓
    static const CashMargin_def kClose;  ///< 融资融券平仓
/*
 * @brief msg check
 */
    static bool IsValid(const CashMargin_def& value)
    {
        if (
            value != AgwCashMargin::kCash
            && value != AgwCashMargin::kOpen
            && value != AgwCashMargin::kClose
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const CashMargin_def& value)
    {
        std::map<CashMargin_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwCashMargin::kCash] = "普通交易";
        const_desc_g[AgwCashMargin::kOpen] = "融资融券开仓";
        const_desc_g[AgwCashMargin::kClose] = "融资融券平仓";
        return true;
    }
    static std::string GetSummary(const CashMargin_def& value)
    {
        std::map<CashMargin_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwCashMargin::kCash] = "";
        const_summary_g[AgwCashMargin::kOpen] = "";
        const_summary_g[AgwCashMargin::kClose] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<CashMargin_def, std::string> const_desc_g;
    static std::map<CashMargin_def, std::string> const_summary_g;
};
/*
 * @brief 备兑
 */
struct EXPORT_MACRO_DEFINE AgwCoveredUncovered
{
    static const CoveredOrUncovered_def kCovered = 0;  ///< 备兑仓
    static const CoveredOrUncovered_def kUncovered = 1;  ///< 普通仓
/*
 * @brief msg check
 */
    static bool IsValid(const CoveredOrUncovered_def& value)
    {
        if (
            value != AgwCoveredUncovered::kCovered
            && value != AgwCoveredUncovered::kUncovered
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const CoveredOrUncovered_def& value)
    {
        std::map<CoveredOrUncovered_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwCoveredUncovered::kCovered] = "备兑仓";
        const_desc_g[AgwCoveredUncovered::kUncovered] = "普通仓";
        return true;
    }
    static std::string GetSummary(const CoveredOrUncovered_def& value)
    {
        std::map<CoveredOrUncovered_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwCoveredUncovered::kCovered] = "";
        const_summary_g[AgwCoveredUncovered::kUncovered] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<CoveredOrUncovered_def, std::string> const_desc_g;
    static std::map<CoveredOrUncovered_def, std::string> const_summary_g;
};
/*
 * @brief 转托管注册指令类型
 */
struct EXPORT_MACRO_DEFINE AgwDesignationTransType
{
    static const DesignationTransType_def kNew = 1;  ///< 新注册请求
    static const DesignationTransType_def kCancel = 3;  ///< 注册撤销
/*
 * @brief msg check
 */
    static bool IsValid(const DesignationTransType_def& value)
    {
        if (
            value != AgwDesignationTransType::kNew
            && value != AgwDesignationTransType::kCancel
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const DesignationTransType_def& value)
    {
        std::map<DesignationTransType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwDesignationTransType::kNew] = "新注册请求";
        const_desc_g[AgwDesignationTransType::kCancel] = "注册撤销";
        return true;
    }
    static std::string GetSummary(const DesignationTransType_def& value)
    {
        std::map<DesignationTransType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwDesignationTransType::kNew] = "";
        const_summary_g[AgwDesignationTransType::kCancel] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<DesignationTransType_def, std::string> const_desc_g;
    static std::map<DesignationTransType_def, std::string> const_summary_g;
};
/*
 * @brief 执行编号类型
 */
struct EXPORT_MACRO_DEFINE AgwExecType
{
    static const ExecType_def kNew;  ///< 新订单
    static const ExecType_def kInternal;  ///< 内部执行
    static const ExecType_def kCancelled;  ///< 已撤销
    static const ExecType_def kReject;  ///< 已拒绝
    static const ExecType_def kTrade;  ///< 已成交
    static const ExecType_def kGold;  ///< 黄金交易所成交
/*
 * @brief msg check
 */
    static bool IsValid(const ExecType_def& value)
    {
        if (
            value != AgwExecType::kNew
            && value != AgwExecType::kInternal
            && value != AgwExecType::kCancelled
            && value != AgwExecType::kReject
            && value != AgwExecType::kTrade
            && value != AgwExecType::kGold
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const ExecType_def& value)
    {
        std::map<ExecType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwExecType::kNew] = "新订单";
        const_desc_g[AgwExecType::kInternal] = "内部执行";
        const_desc_g[AgwExecType::kCancelled] = "已撤销";
        const_desc_g[AgwExecType::kReject] = "已拒绝";
        const_desc_g[AgwExecType::kTrade] = "已成交";
        const_desc_g[AgwExecType::kGold] = "黄金交易所成交";
        return true;
    }
    static std::string GetSummary(const ExecType_def& value)
    {
        std::map<ExecType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwExecType::kNew] = "";
        const_summary_g[AgwExecType::kInternal] = "";
        const_summary_g[AgwExecType::kCancelled] = "";
        const_summary_g[AgwExecType::kReject] = "";
        const_summary_g[AgwExecType::kTrade] = "";
        const_summary_g[AgwExecType::kGold] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<ExecType_def, std::string> const_desc_g;
    static std::map<ExecType_def, std::string> const_summary_g;
};
/*
 * @brief 证券所属市场
 */
struct EXPORT_MACRO_DEFINE AgwMarketID
{
    static const MarketID_def kNULL = 0;  ///< 空
    static const MarketID_def kShangHai = 101;  ///< 上海
    static const MarketID_def kShenZhen = 102;  ///< 深圳
    static const MarketID_def kHKTShangHai = 105;  ///< 港股通-沪市
    static const MarketID_def kHKTShenZhen = 106;  ///< 港股通-深市
    static const MarketID_def kThird = 109;  ///< 北京
/*
 * @brief msg check
 */
    static bool IsValid(const MarketID_def& value)
    {
        if (
            value != AgwMarketID::kNULL
            && value != AgwMarketID::kShangHai
            && value != AgwMarketID::kShenZhen
            && value != AgwMarketID::kHKTShangHai
            && value != AgwMarketID::kHKTShenZhen
            && value != AgwMarketID::kThird
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const MarketID_def& value)
    {
        std::map<MarketID_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwMarketID::kNULL] = "空";
        const_desc_g[AgwMarketID::kShangHai] = "上海";
        const_desc_g[AgwMarketID::kShenZhen] = "深圳";
        const_desc_g[AgwMarketID::kHKTShangHai] = "港股通-沪市";
        const_desc_g[AgwMarketID::kHKTShenZhen] = "港股通-深市";
        const_desc_g[AgwMarketID::kThird] = "北京";
        return true;
    }
    static std::string GetSummary(const MarketID_def& value)
    {
        std::map<MarketID_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwMarketID::kNULL] = "";
        const_summary_g[AgwMarketID::kShangHai] = "";
        const_summary_g[AgwMarketID::kShenZhen] = "";
        const_summary_g[AgwMarketID::kHKTShangHai] = "";
        const_summary_g[AgwMarketID::kHKTShenZhen] = "";
        const_summary_g[AgwMarketID::kThird] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<MarketID_def, std::string> const_desc_g;
    static std::map<MarketID_def, std::string> const_summary_g;
};
/*
 * @brief 订单状态
 */
struct EXPORT_MACRO_DEFINE AgwOrdStatus
{
    static const OrdStatus_def kNew = 0;  ///< 已申报
    static const OrdStatus_def kPartiallyFilled = 1;  ///< 部分成交
    static const OrdStatus_def kFilled = 2;  ///< 全部成交
    static const OrdStatus_def kPartiallyFilledPartiallyCancelled = 3;  ///< 部分成交部分撤销
    static const OrdStatus_def kCancelled = 4;  ///< 已撤销
    static const OrdStatus_def kPartiallyCancelled = 5;  ///< 部分撤销
    static const OrdStatus_def kReject = 8;  ///< 已拒绝
    static const OrdStatus_def kUnSend = 9;  ///< 未申报
    static const OrdStatus_def kSended = 10;  ///< 正报
    static const OrdStatus_def kWaitCancelled = 11;  ///< 待撤销
    static const OrdStatus_def kPartiallyFilledWaitCancelled = 12;  ///< 部成待撤
/*
 * @brief msg check
 */
    static bool IsValid(const OrdStatus_def& value)
    {
        if (
            value != AgwOrdStatus::kNew
            && value != AgwOrdStatus::kPartiallyFilled
            && value != AgwOrdStatus::kFilled
            && value != AgwOrdStatus::kPartiallyFilledPartiallyCancelled
            && value != AgwOrdStatus::kCancelled
            && value != AgwOrdStatus::kPartiallyCancelled
            && value != AgwOrdStatus::kReject
            && value != AgwOrdStatus::kUnSend
            && value != AgwOrdStatus::kSended
            && value != AgwOrdStatus::kWaitCancelled
            && value != AgwOrdStatus::kPartiallyFilledWaitCancelled
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const OrdStatus_def& value)
    {
        std::map<OrdStatus_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwOrdStatus::kNew] = "已申报";
        const_desc_g[AgwOrdStatus::kPartiallyFilled] = "部分成交";
        const_desc_g[AgwOrdStatus::kFilled] = "全部成交";
        const_desc_g[AgwOrdStatus::kPartiallyFilledPartiallyCancelled] = "部分成交部分撤销";
        const_desc_g[AgwOrdStatus::kCancelled] = "已撤销";
        const_desc_g[AgwOrdStatus::kPartiallyCancelled] = "部分撤销";
        const_desc_g[AgwOrdStatus::kReject] = "已拒绝";
        const_desc_g[AgwOrdStatus::kUnSend] = "未申报";
        const_desc_g[AgwOrdStatus::kSended] = "正报";
        const_desc_g[AgwOrdStatus::kWaitCancelled] = "待撤销";
        const_desc_g[AgwOrdStatus::kPartiallyFilledWaitCancelled] = "部成待撤";
        return true;
    }
    static std::string GetSummary(const OrdStatus_def& value)
    {
        std::map<OrdStatus_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwOrdStatus::kNew] = "";
        const_summary_g[AgwOrdStatus::kPartiallyFilled] = "";
        const_summary_g[AgwOrdStatus::kFilled] = "";
        const_summary_g[AgwOrdStatus::kPartiallyFilledPartiallyCancelled] = "";
        const_summary_g[AgwOrdStatus::kCancelled] = "";
        const_summary_g[AgwOrdStatus::kPartiallyCancelled] = "";
        const_summary_g[AgwOrdStatus::kReject] = "";
        const_summary_g[AgwOrdStatus::kUnSend] = "";
        const_summary_g[AgwOrdStatus::kSended] = "";
        const_summary_g[AgwOrdStatus::kWaitCancelled] = "";
        const_summary_g[AgwOrdStatus::kPartiallyFilledWaitCancelled] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<OrdStatus_def, std::string> const_desc_g;
    static std::map<OrdStatus_def, std::string> const_summary_g;
};
/*
 * @brief 订单类型
 */
struct EXPORT_MACRO_DEFINE AgwOrdType
{
    static const OrdType_def kMarket;  ///< 市价
    static const OrdType_def kFixed;  ///< 限价
    static const OrdType_def kLocalOptimal;  ///< 本方最优
    static const OrdType_def kTransferFixed;  ///< 市价剩余转限价,适用于上交所
/*
 * @brief msg check
 */
    static bool IsValid(const OrdType_def& value)
    {
        if (
            value != AgwOrdType::kMarket
            && value != AgwOrdType::kFixed
            && value != AgwOrdType::kLocalOptimal
            && value != AgwOrdType::kTransferFixed
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const OrdType_def& value)
    {
        std::map<OrdType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwOrdType::kMarket] = "市价";
        const_desc_g[AgwOrdType::kFixed] = "限价";
        const_desc_g[AgwOrdType::kLocalOptimal] = "本方最优";
        const_desc_g[AgwOrdType::kTransferFixed] = "市价剩余转限价,适用于上交所";
        return true;
    }
    static std::string GetSummary(const OrdType_def& value)
    {
        std::map<OrdType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwOrdType::kMarket] = "";
        const_summary_g[AgwOrdType::kFixed] = "";
        const_summary_g[AgwOrdType::kLocalOptimal] = "";
        const_summary_g[AgwOrdType::kTransferFixed] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<OrdType_def, std::string> const_desc_g;
    static std::map<OrdType_def, std::string> const_summary_g;
};
/*
 * @brief 开平仓标识
 */
struct EXPORT_MACRO_DEFINE AgwPositionEffect
{
    static const PositionEffect_def kOpen;  ///< 开仓
    static const PositionEffect_def kClose;  ///< 平仓
/*
 * @brief msg check
 */
    static bool IsValid(const PositionEffect_def& value)
    {
        if (
            value != AgwPositionEffect::kOpen
            && value != AgwPositionEffect::kClose
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const PositionEffect_def& value)
    {
        std::map<PositionEffect_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwPositionEffect::kOpen] = "开仓";
        const_desc_g[AgwPositionEffect::kClose] = "平仓";
        return true;
    }
    static std::string GetSummary(const PositionEffect_def& value)
    {
        std::map<PositionEffect_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwPositionEffect::kOpen] = "";
        const_summary_g[AgwPositionEffect::kClose] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<PositionEffect_def, std::string> const_desc_g;
    static std::map<PositionEffect_def, std::string> const_summary_g;
};
/*
 * @brief 排序方法
 */
struct EXPORT_MACRO_DEFINE AgwReturnSeq
{
    static const ReturnSeq_def kTimeOrder = 1;  ///< 按时间正序
    static const ReturnSeq_def kTimeOrderRe = 2;  ///< 按时间倒序
/*
 * @brief msg check
 */
    static bool IsValid(const ReturnSeq_def& value)
    {
        if (
            value != AgwReturnSeq::kTimeOrder
            && value != AgwReturnSeq::kTimeOrderRe
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const ReturnSeq_def& value)
    {
        std::map<ReturnSeq_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwReturnSeq::kTimeOrder] = "按时间正序";
        const_desc_g[AgwReturnSeq::kTimeOrderRe] = "按时间倒序";
        return true;
    }
    static std::string GetSummary(const ReturnSeq_def& value)
    {
        std::map<ReturnSeq_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwReturnSeq::kTimeOrder] = "";
        const_summary_g[AgwReturnSeq::kTimeOrderRe] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<ReturnSeq_def, std::string> const_desc_g;
    static std::map<ReturnSeq_def, std::string> const_summary_g;
};
/*
 * @brief 买卖方向
 */
struct EXPORT_MACRO_DEFINE AgwSide
{
    static const Side_def kAll;  ///< 所有
    static const Side_def kBuy;  ///< 买
    static const Side_def kSell;  ///< 卖
    static const Side_def kRepo;  ///< 正回购
    static const Side_def kAntiRepo;  ///< 逆回购
    static const Side_def kSwap;  ///< 转股
    static const Side_def kPutBack;  ///< 回售
    static const Side_def kTenderOffer;  ///< 预受要约
    static const Side_def kDisTenderOffer;  ///< 解除预受要约
    static const Side_def kCall;  ///< 认购
    static const Side_def kActivation;  ///< 密码激活
    static const Side_def kCancellation;  ///< 密码注销
    static const Side_def kPurchase;  ///< 申购
    static const Side_def kRedeem;  ///< 赎回
    static const Side_def kBorrow;  ///< 借入
    static const Side_def kLoan;  ///< 出借
    static const Side_def kDesignationTransfer;  ///< 转托管
    static const Side_def kRevertDesignationTransfer;  ///< 反向转托管
    static const Side_def kPhysicalPurchase;  ///< 实物申购
    static const Side_def kQuery;  ///< 查询
    static const Side_def kPhysicalRedemption;  ///< 实物赎回
    static const Side_def kOpen;  ///< 分拆
    static const Side_def kMerge;  ///< 合并
    static const Side_def kPledgeIn;  ///< 入库
    static const Side_def kPledgeOut;  ///< 出库
    static const Side_def kLock;  ///< 锁定
    static const Side_def kUnLock;  ///< 解锁
    static const Side_def kDisposal;  ///< 转处置
    static const Side_def kDisposalReturn;  ///< 转处置返还
    static const Side_def kTransformIn;  ///< 转入
    static const Side_def kTransformOut;  ///< 转出
    static const Side_def kAddClose;  ///< 追加平仓
    static const Side_def kClearClose;  ///< 清偿平仓
    static const Side_def kAddForceSell;  ///< 追加强卖
    static const Side_def kClearForceSell;  ///< 清偿强卖
    static const Side_def kActivedSellRepay;  ///< 主动卖券还款
    static const Side_def kPutbackCancel;  ///< 回售撤销
    static const Side_def kCashCall;  ///< 网下现金认购
    static const Side_def kStockCall;  ///< 网下股票认购
    static const Side_def kConfirmTransBuy;  ///< 确认成交买
    static const Side_def kConfirmTransSell;  ///< 确认成交卖
/*
 * @brief msg check
 */
    static bool IsValid(const Side_def& value)
    {
        if (
            value != AgwSide::kAll
            && value != AgwSide::kBuy
            && value != AgwSide::kSell
            && value != AgwSide::kRepo
            && value != AgwSide::kAntiRepo
            && value != AgwSide::kSwap
            && value != AgwSide::kPutBack
            && value != AgwSide::kTenderOffer
            && value != AgwSide::kDisTenderOffer
            && value != AgwSide::kCall
            && value != AgwSide::kActivation
            && value != AgwSide::kCancellation
            && value != AgwSide::kPurchase
            && value != AgwSide::kRedeem
            && value != AgwSide::kBorrow
            && value != AgwSide::kLoan
            && value != AgwSide::kDesignationTransfer
            && value != AgwSide::kRevertDesignationTransfer
            && value != AgwSide::kPhysicalPurchase
            && value != AgwSide::kQuery
            && value != AgwSide::kPhysicalRedemption
            && value != AgwSide::kOpen
            && value != AgwSide::kMerge
            && value != AgwSide::kPledgeIn
            && value != AgwSide::kPledgeOut
            && value != AgwSide::kLock
            && value != AgwSide::kUnLock
            && value != AgwSide::kDisposal
            && value != AgwSide::kDisposalReturn
            && value != AgwSide::kTransformIn
            && value != AgwSide::kTransformOut
            && value != AgwSide::kAddClose
            && value != AgwSide::kClearClose
            && value != AgwSide::kAddForceSell
            && value != AgwSide::kClearForceSell
            && value != AgwSide::kActivedSellRepay
            && value != AgwSide::kPutbackCancel
            && value != AgwSide::kCashCall
            && value != AgwSide::kStockCall
            && value != AgwSide::kConfirmTransBuy
            && value != AgwSide::kConfirmTransSell
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const Side_def& value)
    {
        std::map<Side_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwSide::kAll] = "所有";
        const_desc_g[AgwSide::kBuy] = "买";
        const_desc_g[AgwSide::kSell] = "卖";
        const_desc_g[AgwSide::kRepo] = "正回购";
        const_desc_g[AgwSide::kAntiRepo] = "逆回购";
        const_desc_g[AgwSide::kSwap] = "转股";
        const_desc_g[AgwSide::kPutBack] = "回售";
        const_desc_g[AgwSide::kTenderOffer] = "预受要约";
        const_desc_g[AgwSide::kDisTenderOffer] = "解除预受要约";
        const_desc_g[AgwSide::kCall] = "认购";
        const_desc_g[AgwSide::kActivation] = "密码激活";
        const_desc_g[AgwSide::kCancellation] = "密码注销";
        const_desc_g[AgwSide::kPurchase] = "申购";
        const_desc_g[AgwSide::kRedeem] = "赎回";
        const_desc_g[AgwSide::kBorrow] = "借入";
        const_desc_g[AgwSide::kLoan] = "出借";
        const_desc_g[AgwSide::kDesignationTransfer] = "转托管";
        const_desc_g[AgwSide::kRevertDesignationTransfer] = "反向转托管";
        const_desc_g[AgwSide::kPhysicalPurchase] = "实物申购";
        const_desc_g[AgwSide::kQuery] = "查询";
        const_desc_g[AgwSide::kPhysicalRedemption] = "实物赎回";
        const_desc_g[AgwSide::kOpen] = "分拆";
        const_desc_g[AgwSide::kMerge] = "合并";
        const_desc_g[AgwSide::kPledgeIn] = "入库";
        const_desc_g[AgwSide::kPledgeOut] = "出库";
        const_desc_g[AgwSide::kLock] = "锁定";
        const_desc_g[AgwSide::kUnLock] = "解锁";
        const_desc_g[AgwSide::kDisposal] = "转处置";
        const_desc_g[AgwSide::kDisposalReturn] = "转处置返还";
        const_desc_g[AgwSide::kTransformIn] = "转入";
        const_desc_g[AgwSide::kTransformOut] = "转出";
        const_desc_g[AgwSide::kAddClose] = "追加平仓";
        const_desc_g[AgwSide::kClearClose] = "清偿平仓";
        const_desc_g[AgwSide::kAddForceSell] = "追加强卖";
        const_desc_g[AgwSide::kClearForceSell] = "清偿强卖";
        const_desc_g[AgwSide::kActivedSellRepay] = "主动卖券还款";
        const_desc_g[AgwSide::kPutbackCancel] = "回售撤销";
        const_desc_g[AgwSide::kCashCall] = "网下现金认购";
        const_desc_g[AgwSide::kStockCall] = "网下股票认购";
        const_desc_g[AgwSide::kConfirmTransBuy] = "确认成交买";
        const_desc_g[AgwSide::kConfirmTransSell] = "确认成交卖";
        return true;
    }
    static std::string GetSummary(const Side_def& value)
    {
        std::map<Side_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwSide::kAll] = "";
        const_summary_g[AgwSide::kBuy] = "";
        const_summary_g[AgwSide::kSell] = "";
        const_summary_g[AgwSide::kRepo] = "";
        const_summary_g[AgwSide::kAntiRepo] = "";
        const_summary_g[AgwSide::kSwap] = "";
        const_summary_g[AgwSide::kPutBack] = "";
        const_summary_g[AgwSide::kTenderOffer] = "";
        const_summary_g[AgwSide::kDisTenderOffer] = "";
        const_summary_g[AgwSide::kCall] = "";
        const_summary_g[AgwSide::kActivation] = "";
        const_summary_g[AgwSide::kCancellation] = "";
        const_summary_g[AgwSide::kPurchase] = "";
        const_summary_g[AgwSide::kRedeem] = "";
        const_summary_g[AgwSide::kBorrow] = "";
        const_summary_g[AgwSide::kLoan] = "";
        const_summary_g[AgwSide::kDesignationTransfer] = "";
        const_summary_g[AgwSide::kRevertDesignationTransfer] = "";
        const_summary_g[AgwSide::kPhysicalPurchase] = "";
        const_summary_g[AgwSide::kQuery] = "";
        const_summary_g[AgwSide::kPhysicalRedemption] = "";
        const_summary_g[AgwSide::kOpen] = "";
        const_summary_g[AgwSide::kMerge] = "";
        const_summary_g[AgwSide::kPledgeIn] = "";
        const_summary_g[AgwSide::kPledgeOut] = "";
        const_summary_g[AgwSide::kLock] = "";
        const_summary_g[AgwSide::kUnLock] = "";
        const_summary_g[AgwSide::kDisposal] = "";
        const_summary_g[AgwSide::kDisposalReturn] = "";
        const_summary_g[AgwSide::kTransformIn] = "";
        const_summary_g[AgwSide::kTransformOut] = "";
        const_summary_g[AgwSide::kAddClose] = "";
        const_summary_g[AgwSide::kClearClose] = "";
        const_summary_g[AgwSide::kAddForceSell] = "";
        const_summary_g[AgwSide::kClearForceSell] = "";
        const_summary_g[AgwSide::kActivedSellRepay] = "";
        const_summary_g[AgwSide::kPutbackCancel] = "";
        const_summary_g[AgwSide::kCashCall] = "";
        const_summary_g[AgwSide::kStockCall] = "";
        const_summary_g[AgwSide::kConfirmTransBuy] = "";
        const_summary_g[AgwSide::kConfirmTransSell] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<Side_def, std::string> const_desc_g;
    static std::map<Side_def, std::string> const_summary_g;
};
/*
 * @brief 订单有效期类型,默认限价为GFD,市价为IOC
 */
struct EXPORT_MACRO_DEFINE AgwTimeInForce
{
    static const TimeInForce_def kGFD;  ///< 当日有效
    static const TimeInForce_def kIOC;  ///< 即时成交剩余撤销
    static const TimeInForce_def kFOK;  ///< 即时全部成交或撤销,适用于上交所
    static const TimeInForce_def kAtCrossing;  ///< 竞价限价盘,适用于深交所
/*
 * @brief msg check
 */
    static bool IsValid(const TimeInForce_def& value)
    {
        if (
            value != AgwTimeInForce::kGFD
            && value != AgwTimeInForce::kIOC
            && value != AgwTimeInForce::kFOK
            && value != AgwTimeInForce::kAtCrossing
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const TimeInForce_def& value)
    {
        std::map<TimeInForce_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwTimeInForce::kGFD] = "当日有效";
        const_desc_g[AgwTimeInForce::kIOC] = "即时成交剩余撤销";
        const_desc_g[AgwTimeInForce::kFOK] = "即时全部成交或撤销,适用于上交所";
        const_desc_g[AgwTimeInForce::kAtCrossing] = "竞价限价盘,适用于深交所";
        return true;
    }
    static std::string GetSummary(const TimeInForce_def& value)
    {
        std::map<TimeInForce_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwTimeInForce::kGFD] = "";
        const_summary_g[AgwTimeInForce::kIOC] = "";
        const_summary_g[AgwTimeInForce::kFOK] = "";
        const_summary_g[AgwTimeInForce::kAtCrossing] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<TimeInForce_def, std::string> const_desc_g;
    static std::map<TimeInForce_def, std::string> const_summary_g;
};
/*
 * @brief 订单数量类型
 */
struct EXPORT_MACRO_DEFINE AgwLotType
{
    static const LotType_def kOddLot;  ///< 零股订单
    static const LotType_def kRoundLot;  ///< 整手订单
/*
 * @brief msg check
 */
    static bool IsValid(const LotType_def& value)
    {
        if (
            value != AgwLotType::kOddLot
            && value != AgwLotType::kRoundLot
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const LotType_def& value)
    {
        std::map<LotType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwLotType::kOddLot] = "零股订单";
        const_desc_g[AgwLotType::kRoundLot] = "整手订单";
        return true;
    }
    static std::string GetSummary(const LotType_def& value)
    {
        std::map<LotType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwLotType::kOddLot] = "";
        const_summary_g[AgwLotType::kRoundLot] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<LotType_def, std::string> const_desc_g;
    static std::map<LotType_def, std::string> const_summary_g;
};
/*
 * @brief 错误码类别
 */
struct EXPORT_MACRO_DEFINE RejectCodeType
{
    static const RejectCodeType_def kNone = 0;  ///< 所有
    static const RejectCodeType_def kBusiness = 1;  ///< 业务拒绝（包括AGW、ORS、COS拒绝）
    static const RejectCodeType_def kExternalQuery = 2;  ///< 外部查询拒绝
    static const RejectCodeType_def kRealInstruction = 3;  ///< 实时指令拒绝
    static const RejectCodeType_def kExternalInstruction = 4;  ///< 外部指令拒绝
    static const RejectCodeType_def kExchange = 5;  ///< 交易所
/*
 * @brief msg check
 */
    static bool IsValid(const RejectCodeType_def& value)
    {
        if (
            value != RejectCodeType::kNone
            && value != RejectCodeType::kBusiness
            && value != RejectCodeType::kExternalQuery
            && value != RejectCodeType::kRealInstruction
            && value != RejectCodeType::kExternalInstruction
            && value != RejectCodeType::kExchange
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const RejectCodeType_def& value)
    {
        std::map<RejectCodeType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[RejectCodeType::kNone] = "所有";
        const_desc_g[RejectCodeType::kBusiness] = "业务拒绝（包括AGW、ORS、COS拒绝）";
        const_desc_g[RejectCodeType::kExternalQuery] = "外部查询拒绝";
        const_desc_g[RejectCodeType::kRealInstruction] = "实时指令拒绝";
        const_desc_g[RejectCodeType::kExternalInstruction] = "外部指令拒绝";
        const_desc_g[RejectCodeType::kExchange] = "交易所";
        return true;
    }
    static std::string GetSummary(const RejectCodeType_def& value)
    {
        std::map<RejectCodeType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[RejectCodeType::kNone] = "";
        const_summary_g[RejectCodeType::kBusiness] = "";
        const_summary_g[RejectCodeType::kExternalQuery] = "";
        const_summary_g[RejectCodeType::kRealInstruction] = "";
        const_summary_g[RejectCodeType::kExternalInstruction] = "";
        const_summary_g[RejectCodeType::kExchange] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<RejectCodeType_def, std::string> const_desc_g;
    static std::map<RejectCodeType_def, std::string> const_summary_g;
};
/*
 * @brief 持仓类型
 */
struct EXPORT_MACRO_DEFINE AgwLegSide
{
    static const LegSide_def kLongPosition;  ///< 权利仓
    static const LegSide_def kShortPosition;  ///< 义务仓
/*
 * @brief msg check
 */
    static bool IsValid(const LegSide_def& value)
    {
        if (
            value != AgwLegSide::kLongPosition
            && value != AgwLegSide::kShortPosition
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const LegSide_def& value)
    {
        std::map<LegSide_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[AgwLegSide::kLongPosition] = "权利仓";
        const_desc_g[AgwLegSide::kShortPosition] = "义务仓";
        return true;
    }
    static std::string GetSummary(const LegSide_def& value)
    {
        std::map<LegSide_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[AgwLegSide::kLongPosition] = "";
        const_summary_g[AgwLegSide::kShortPosition] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<LegSide_def, std::string> const_desc_g;
    static std::map<LegSide_def, std::string> const_summary_g;
};
/*
 * @brief 信用方向
 */
struct EXPORT_MACRO_DEFINE CreditDirect
{
    static const CreditDirect_def kMarginBuy;  ///< 融资买入
    static const CreditDirect_def kMarginSell;  ///< 融券卖出
/*
 * @brief msg check
 */
    static bool IsValid(const CreditDirect_def& value)
    {
        if (
            value != CreditDirect::kMarginBuy
            && value != CreditDirect::kMarginSell
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const CreditDirect_def& value)
    {
        std::map<CreditDirect_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[CreditDirect::kMarginBuy] = "融资买入";
        const_desc_g[CreditDirect::kMarginSell] = "融券卖出";
        return true;
    }
    static std::string GetSummary(const CreditDirect_def& value)
    {
        std::map<CreditDirect_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[CreditDirect::kMarginBuy] = "";
        const_summary_g[CreditDirect::kMarginSell] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<CreditDirect_def, std::string> const_desc_g;
    static std::map<CreditDirect_def, std::string> const_summary_g;
};
/*
 * @brief 负债合约明细类型
 */
struct EXPORT_MACRO_DEFINE DebtsDetailKind
{
    static const DebtsDetailKind_def kLittleLoan;  ///< 小贷
    static const DebtsDetailKind_def kLockShare;  ///< 锁券
    static const DebtsDetailKind_def kSpec;  ///< 专户
/*
 * @brief msg check
 */
    static bool IsValid(const DebtsDetailKind_def& value)
    {
        if (
            value != DebtsDetailKind::kLittleLoan
            && value != DebtsDetailKind::kLockShare
            && value != DebtsDetailKind::kSpec
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const DebtsDetailKind_def& value)
    {
        std::map<DebtsDetailKind_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[DebtsDetailKind::kLittleLoan] = "小贷";
        const_desc_g[DebtsDetailKind::kLockShare] = "锁券";
        const_desc_g[DebtsDetailKind::kSpec] = "专户";
        return true;
    }
    static std::string GetSummary(const DebtsDetailKind_def& value)
    {
        std::map<DebtsDetailKind_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[DebtsDetailKind::kLittleLoan] = "";
        const_summary_g[DebtsDetailKind::kLockShare] = "";
        const_summary_g[DebtsDetailKind::kSpec] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<DebtsDetailKind_def, std::string> const_desc_g;
    static std::map<DebtsDetailKind_def, std::string> const_summary_g;
};
/*
 * @brief 查询方向
 */
struct EXPORT_MACRO_DEFINE CreditDebtsQueryFlag
{
    static const CreditDebtsQueryFlag_def kNULL;  ///< 空值
    static const CreditDebtsQueryFlag_def kDESC;  ///< 倒序
    static const CreditDebtsQueryFlag_def kASC;  ///< 顺序
/*
 * @brief msg check
 */
    static bool IsValid(const CreditDebtsQueryFlag_def& value)
    {
        if (
            value != CreditDebtsQueryFlag::kNULL
            && value != CreditDebtsQueryFlag::kDESC
            && value != CreditDebtsQueryFlag::kASC
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const CreditDebtsQueryFlag_def& value)
    {
        std::map<CreditDebtsQueryFlag_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[CreditDebtsQueryFlag::kNULL] = "空值";
        const_desc_g[CreditDebtsQueryFlag::kDESC] = "倒序";
        const_desc_g[CreditDebtsQueryFlag::kASC] = "顺序";
        return true;
    }
    static std::string GetSummary(const CreditDebtsQueryFlag_def& value)
    {
        std::map<CreditDebtsQueryFlag_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[CreditDebtsQueryFlag::kNULL] = "";
        const_summary_g[CreditDebtsQueryFlag::kDESC] = "";
        const_summary_g[CreditDebtsQueryFlag::kASC] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<CreditDebtsQueryFlag_def, std::string> const_desc_g;
    static std::map<CreditDebtsQueryFlag_def, std::string> const_summary_g;
};
/*
 * @brief 锁券状态
 */
struct EXPORT_MACRO_DEFINE LockStatus
{
    static const LockStatus_def kUnAssigned;  ///< 有未分配(有未分配数量reserveqty>0)
    static const LockStatus_def kAssigned;  ///< 已分配(reserveqty = 0)
    static const LockStatus_def kOutstanding;  ///< 过期未偿还(不清除,如果没有产生费用则演变成3)
    static const LockStatus_def kRepaid;  ///< 过期已偿还(记录删除或归历史)
/*
 * @brief msg check
 */
    static bool IsValid(const LockStatus_def& value)
    {
        if (
            value != LockStatus::kUnAssigned
            && value != LockStatus::kAssigned
            && value != LockStatus::kOutstanding
            && value != LockStatus::kRepaid
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const LockStatus_def& value)
    {
        std::map<LockStatus_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[LockStatus::kUnAssigned] = "有未分配(有未分配数量reserveqty>0)";
        const_desc_g[LockStatus::kAssigned] = "已分配(reserveqty = 0)";
        const_desc_g[LockStatus::kOutstanding] = "过期未偿还(不清除,如果没有产生费用则演变成3)";
        const_desc_g[LockStatus::kRepaid] = "过期已偿还(记录删除或归历史)";
        return true;
    }
    static std::string GetSummary(const LockStatus_def& value)
    {
        std::map<LockStatus_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[LockStatus::kUnAssigned] = "";
        const_summary_g[LockStatus::kAssigned] = "";
        const_summary_g[LockStatus::kOutstanding] = "";
        const_summary_g[LockStatus::kRepaid] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<LockStatus_def, std::string> const_desc_g;
    static std::map<LockStatus_def, std::string> const_summary_g;
};
/*
 * @brief 合同状态
 */
struct EXPORT_MACRO_DEFINE ContractStatus
{
    static const ContractStatus_def kNormal;  ///< 正常
    static const ContractStatus_def kNotStarted;  ///< 未启用
    static const ContractStatus_def kStoped;  ///< 终止
/*
 * @brief msg check
 */
    static bool IsValid(const ContractStatus_def& value)
    {
        if (
            value != ContractStatus::kNormal
            && value != ContractStatus::kNotStarted
            && value != ContractStatus::kStoped
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const ContractStatus_def& value)
    {
        std::map<ContractStatus_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[ContractStatus::kNormal] = "正常";
        const_desc_g[ContractStatus::kNotStarted] = "未启用";
        const_desc_g[ContractStatus::kStoped] = "终止";
        return true;
    }
    static std::string GetSummary(const ContractStatus_def& value)
    {
        std::map<ContractStatus_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[ContractStatus::kNormal] = "";
        const_summary_g[ContractStatus::kNotStarted] = "";
        const_summary_g[ContractStatus::kStoped] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<ContractStatus_def, std::string> const_desc_g;
    static std::map<ContractStatus_def, std::string> const_summary_g;
};
/*
 * @brief 申请状态
 */
struct EXPORT_MACRO_DEFINE ApplyStatus
{
    static const ApplyStatus_def kNULL;  ///< 空值
    static const ApplyStatus_def kStar;  ///< 星值
    static const ApplyStatus_def kNotApproved;  ///< 未审批
    static const ApplyStatus_def kApproval;  ///< 审批同意
    static const ApplyStatus_def kReject;  ///< 审批拒绝
/*
 * @brief msg check
 */
    static bool IsValid(const ApplyStatus_def& value)
    {
        if (
            value != ApplyStatus::kNULL
            && value != ApplyStatus::kStar
            && value != ApplyStatus::kNotApproved
            && value != ApplyStatus::kApproval
            && value != ApplyStatus::kReject
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const ApplyStatus_def& value)
    {
        std::map<ApplyStatus_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[ApplyStatus::kNULL] = "空值";
        const_desc_g[ApplyStatus::kStar] = "星值";
        const_desc_g[ApplyStatus::kNotApproved] = "未审批";
        const_desc_g[ApplyStatus::kApproval] = "审批同意";
        const_desc_g[ApplyStatus::kReject] = "审批拒绝";
        return true;
    }
    static std::string GetSummary(const ApplyStatus_def& value)
    {
        std::map<ApplyStatus_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[ApplyStatus::kNULL] = "";
        const_summary_g[ApplyStatus::kStar] = "";
        const_summary_g[ApplyStatus::kNotApproved] = "";
        const_summary_g[ApplyStatus::kApproval] = "";
        const_summary_g[ApplyStatus::kReject] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<ApplyStatus_def, std::string> const_desc_g;
    static std::map<ApplyStatus_def, std::string> const_summary_g;
};
/*
 * @brief 新三板认购结果
 */
struct EXPORT_MACRO_DEFINE ThirdSubscribeResult
{
    static const ThirdSubscribeResult_def kMayPurchase;  ///< 可以参与申购
    static const ThirdSubscribeResult_def kForbidPurchase;  ///< 禁止参与申购
    static const ThirdSubscribeResult_def kMustPurchase;  ///< 必须参与申购
    static const ThirdSubscribeResult_def kNotPurchase;  ///< 客户未参与询价
/*
 * @brief msg check
 */
    static bool IsValid(const ThirdSubscribeResult_def& value)
    {
        if (
            value != ThirdSubscribeResult::kMayPurchase
            && value != ThirdSubscribeResult::kForbidPurchase
            && value != ThirdSubscribeResult::kMustPurchase
            && value != ThirdSubscribeResult::kNotPurchase
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const ThirdSubscribeResult_def& value)
    {
        std::map<ThirdSubscribeResult_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[ThirdSubscribeResult::kMayPurchase] = "可以参与申购";
        const_desc_g[ThirdSubscribeResult::kForbidPurchase] = "禁止参与申购";
        const_desc_g[ThirdSubscribeResult::kMustPurchase] = "必须参与申购";
        const_desc_g[ThirdSubscribeResult::kNotPurchase] = "客户未参与询价";
        return true;
    }
    static std::string GetSummary(const ThirdSubscribeResult_def& value)
    {
        std::map<ThirdSubscribeResult_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[ThirdSubscribeResult::kMayPurchase] = "";
        const_summary_g[ThirdSubscribeResult::kForbidPurchase] = "";
        const_summary_g[ThirdSubscribeResult::kMustPurchase] = "";
        const_summary_g[ThirdSubscribeResult::kNotPurchase] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<ThirdSubscribeResult_def, std::string> const_desc_g;
    static std::map<ThirdSubscribeResult_def, std::string> const_summary_g;
};
/*
 * @brief 新三板发行方式
 */
struct EXPORT_MACRO_DEFINE IssueFlag
{
    static const IssueFlag_def kSubscribe;  ///< 询价阶段
    static const IssueFlag_def kPurchase;  ///< 申购阶段
    static const IssueFlag_def kAuction;  ///< 竞价阶段
/*
 * @brief msg check
 */
    static bool IsValid(const IssueFlag_def& value)
    {
        if (
            value != IssueFlag::kSubscribe
            && value != IssueFlag::kPurchase
            && value != IssueFlag::kAuction
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const IssueFlag_def& value)
    {
        std::map<IssueFlag_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[IssueFlag::kSubscribe] = "询价阶段";
        const_desc_g[IssueFlag::kPurchase] = "申购阶段";
        const_desc_g[IssueFlag::kAuction] = "竞价阶段";
        return true;
    }
    static std::string GetSummary(const IssueFlag_def& value)
    {
        std::map<IssueFlag_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[IssueFlag::kSubscribe] = "";
        const_summary_g[IssueFlag::kPurchase] = "";
        const_summary_g[IssueFlag::kAuction] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<IssueFlag_def, std::string> const_desc_g;
    static std::map<IssueFlag_def, std::string> const_summary_g;
};
/*
 * @brief UTE回切范围类型
 */
struct EXPORT_MACRO_DEFINE UTERevertScopeType
{
    static const UTERevertScopeType_def kNull = 0;  ///< 空
    static const UTERevertScopeType_def kSingleAccountRevert = 1;  ///< 单客户回切
    static const UTERevertScopeType_def kUTENodeRevert = 2;  ///< 节点回切
/*
 * @brief msg check
 */
    static bool IsValid(const UTERevertScopeType_def& value)
    {
        if (
            value != UTERevertScopeType::kNull
            && value != UTERevertScopeType::kSingleAccountRevert
            && value != UTERevertScopeType::kUTENodeRevert
            )
        {
            return false;
        }
        return true;
    }
    static std::string GetDesc(const UTERevertScopeType_def& value)
    {
        std::map<UTERevertScopeType_def, std::string>::iterator iter =
            const_desc_g.find(value);
        if (iter != const_desc_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitDesc()
    {
        const_desc_g[UTERevertScopeType::kNull] = "空";
        const_desc_g[UTERevertScopeType::kSingleAccountRevert] = "单客户回切";
        const_desc_g[UTERevertScopeType::kUTENodeRevert] = "节点回切";
        return true;
    }
    static std::string GetSummary(const UTERevertScopeType_def& value)
    {
        std::map<UTERevertScopeType_def, std::string>::iterator iter =
            const_summary_g.find(value);
		   if (iter != const_summary_g.end())
        {
            return iter->second;
        }
        else
        {
            return "";
        }
    }
    static bool InitSummary()
    {
        const_summary_g[UTERevertScopeType::kNull] = "";
        const_summary_g[UTERevertScopeType::kSingleAccountRevert] = "";
        const_summary_g[UTERevertScopeType::kUTENodeRevert] = "";
        return true;
    }
    static bool init_desc_g;
    static bool init_summary_g;
    static std::map<UTERevertScopeType_def, std::string> const_desc_g;
    static std::map<UTERevertScopeType_def, std::string> const_summary_g;
};
}
}
#endif // ATP_GW_CONSTANTS__H_

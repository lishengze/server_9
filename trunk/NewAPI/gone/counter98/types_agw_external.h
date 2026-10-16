#ifndef ATP_GW_TYPES_AGW_EXTERNAL_H_
#define ATP_GW_TYPES_AGW_EXTERNAL_H_
#include <map>
#include <functional>
#include <array>
#include <stdint.h>
namespace atp
{
namespace gw
{
/*
 * @brief typedef arrea
 */
typedef std::array<char, 64> Description_def; ///< 描述
typedef uint32_t MsgType_def; ///< 消息编码
typedef uint32_t BodyLength_def; ///< 消息体长度
typedef uint32_t CheckSum_def; ///< 校验和
typedef std::array<char, 32> CompID_def; ///< 接收方或发送方代码
typedef std::array<char, 64> Comments_def; ///< 备注信息
typedef std::array<char, 32> DefaultAppVerID_def; ///< 协议版本
typedef int32_t HeartBtInt_def; ///< 心跳间隔
typedef std::array<char, 32> Password_def; ///< 密码
typedef std::array<char, 32> SenderCompID_def; ///< 发送方代码
typedef int32_t SessionStatus_def; ///< 会话状态
typedef std::array<char, 32> TargetCompID_def; ///< 接收方代码
typedef std::array<char, 32> TestReqID_def; ///< 测试请求信息
typedef std::array<char, 256> TextRes_def; ///< 文本信息
typedef uint32_t AgwNoPartition_def; ///< 分区记录个数
typedef uint8_t AgwPartition_def; ///< 分区号
typedef uint8_t APIType_def; ///< api厂商
typedef int32_t AgwIndex_def; ///< 索引号
typedef int64_t ClOrdNO_def; ///< 订单编号
typedef std::array<char, 12> AccountID_def; ///< 证券账户
typedef std::array<char, 6> AccountSubCode_def; ///< 合约账户子编码
typedef int64_t Amt_def; ///< 金额类型,N18(4)
typedef uint8_t Bool_def; ///< 布尔类型,1=True/Yes,0=False/No
typedef std::array<char, 50> BusinessRejectText_def; ///< 拒绝原因说明
typedef uint8_t BusinessType_def; ///< 业务编码
typedef uint32_t BodyLength_def; ///< 消息体长度
typedef std::array<char, 10> BranchID_def; ///< 营业部ID
typedef char CallOrPut_def; ///< 认沽认购
typedef char CashMargin_def; ///< 信用标识
typedef std::array<char, 4> Currency_def; ///< 货币种类
typedef std::string ClientFeatureCode_def; ///< 终端识别码,一般填mac地址
typedef uint8_t CoveredOrUncovered_def; ///< 备兑标识,衍生品交易填写
typedef std::array<char, 16> CustID_def; ///< 客户号
typedef uint32_t CheckSum_def; ///< 校验和
typedef std::array<char, 32> CompID_def; ///< 接收方或发送方代码
typedef char CreditCtrl_def; ///< 融资融券状态/标的标志
typedef int64_t Date8_def; ///< 日期,格式(YYYYmmDD)
typedef std::array<char, 64> Description_def; ///< 描述
typedef uint8_t DesignationTransType_def; ///< 转托管注册类型
typedef std::array<char, 32> DefaultAppVerID_def; ///< 协议版本
typedef int64_t ExchangeRate_def; ///< 参考汇率,N15(5)
typedef std::array<char, 32> ExecID_def; ///< 执行编号,单个交易日不重复
typedef char ExecType_def; ///< 执行报告类型
typedef uint8_t ETFTradeReportType_def; ///< ETF成交回报类型
typedef uint16_t ExternalInsErrorCode_def; ///< 外部指令拒绝原因代码
typedef int64_t FloatingRatio_def; ///< 浮动比例，N15（5）
typedef std::array<char, 16> FundAccountID_def; ///< 资金帐号ID
typedef uint64_t GeneralMeetingSeq_def; ///< 股东大会编码
typedef int32_t HeartBtInt_def; ///< 心跳间隔
typedef std::array<char, 32> InstanceID_def; ///< 实例ID
typedef int64_t InstructionID_def; ///< 指令ID
typedef uint16_t InstructionType_def; ///< 指令类型
typedef uint16_t InsRejectReasonCode_def; ///< 实时指令拒绝原因代码
typedef std::array<char, 32> OutsideInfo_def; ///< 外部信息
typedef std::array<char, 64> Handler_def; ///< 操作员类型
typedef std::array<char, 16> IP_def; ///< ip地址(IPv4)
typedef char LegSide_def; ///< 持仓类型
typedef uint32_t Length_def; ///< 长度,表示字节为单位的数据长度
typedef int32_t LinkNum_def; ///< 连接数
typedef int32_t LinkSpeed_def; ///< 连接流速(消息个数/每秒)
typedef std::string License_def; ///< license序列号
typedef uint8_t AccessMode_def; ///< 接入方式，默认填1即为API
typedef uint32_t NumInGroup_def; ///< 重复组数量
typedef uint16_t MarketID_def; ///< 市场编码
typedef uint32_t MsgType_def; ///< 消息编码
typedef uint16_t MaxPriceLevels_def; ///< 最多成交价位数
typedef char MarginCalKind_def; ///< 保证金比例计算方式
typedef uint32_t MsgTypeNo_def; ///< 消息编码
typedef std::array<char, 12> Mac_def; ///< mac地址
typedef std::array<char, 5> OrdRejReason_def; ///< 订单拒绝原因代码
typedef uint32_t ShExOrdRejReason_def; ///< 上交所订单拒绝原因代码
typedef uint8_t OrdStatus_def; ///< 订单状态
typedef char OrdType_def; ///< 订单类型
typedef uint8_t OrderQueryCondition_def; ///< 外部委托查询的委托状态条件
typedef std::array<char, 6> PBUID_def; ///< 申报交易单元
typedef uint8_t PBUType_def; ///< 交易单元类型
typedef char PositionEffect_def; ///< 开平仓标识
typedef uint16_t Port_def; ///< 连接的端口号
typedef int64_t Price_def; ///< 价格,N13(4)
typedef int8_t PermissonErrorCode_def; ///< 密码校验服务错误码
typedef char OrderWay_def; ///< 委托方式
typedef uint64_t PID_def; ///< 进程pid
typedef std::string PWD_def; ///< 客户登录密码
typedef std::array<char, 128> PWD_v3_def; ///< 客户登录密码
typedef char EnforceFlag_def; ///< 强制风险标识
typedef std::array<char, 32> Password_def; ///< AGW登录密码
typedef int64_t Qty_def; ///< 数量,N15(2)
typedef uint16_t QueryResultCode_def; ///< 查询结果代码
typedef int64_t ReportIndex_def; ///< 回报记录号,从1开始编号
typedef uint16_t RejectReasonCode_def; ///< 订单拒绝原因代码
typedef uint8_t ReturnSeq_def; ///< 排序方法
typedef uint16_t Result_def; ///< 处理结果
typedef std::array<char, 8> SecurityID_def; ///< 证券代码
typedef int32_t SpeedNum_def; ///< 流速，委托个数/s
typedef char Side_def; ///< 买卖方向
typedef std::string String_def; ///< 不定长字符串
typedef std::array<char, 16> SecondaryOrderID_def; ///< 组合编号
typedef std::array<char, 40> SecuritySymbol_def; ///< 证券简称
typedef std::array<char, 32> SenderCompID_def; ///< 发送方代码
typedef int32_t SessionStatus_def; ///< 会话状态
typedef int64_t SeqNum_def; ///< 消息序号
typedef int32_t SeqNumHeader_def; ///< 消息头消息序号,占位
typedef std::array<char, 8> Text_def; ///< 自由格式文本串,如无特别说明最多8个字符,超过部分将被自动截掉
typedef int64_t Time_def; ///< 时间
typedef char TimeInForce_def; ///< 订单有效期类型
typedef int64_t TimeStamp_def; ///< 本地时间戳
typedef int64_t TotalNum_def; ///< 记录总数
typedef uint8_t TransferWay_def; ///< 资金划转方式
typedef std::array<char, 6> Tenderer_def; ///< 要约收购参与人编码
typedef std::array<char, 32> TargetCompID_def; ///< 接收方代码
typedef std::array<char, 32> TestReqID_def; ///< 测试请求信息
typedef std::array<char, 256> TextRes_def; ///< 文本信息
typedef std::array<char, 64> UserInfo_def; ///< 用户私有信息
typedef std::array<char, 10> VotingProposal_def; ///< 投票议案编号
typedef char VotingPreference_def; ///< 投票意向
typedef std::array<char, 2> VotingSegment_def; ///< 分段统计段号（预留）
typedef uint32_t ValidationCode_def; ///< 密码激活校验号,正数
typedef std::array<char, 16> OrderID_def; ///< 交易所订单编号,跨交易日不重复,交易所填充
typedef std::array<char, 10> ClOrdID_def; ///< 申报合同号
typedef char Pwd_Type_def; ///< 密码加密类型
typedef uint16_t IPOType_def; ///< 配售类型
typedef char IsStkSource_def; ///< 是否券库清单
typedef std::array<char, 255> Distribution_def; ///< 发行方式
typedef uint32_t FundAccountAttr_def; ///< 资金账户属性
typedef std::array<char, 64> CustName_def; ///< 客户姓名
typedef std::array<char, 5> BankCode_def; ///< 银行代码
typedef char CancelFlag_def; ///< 撤单标志
typedef std::array<char, 2> BSFlag2_def; ///< 集中交易买卖类别
typedef std::array<char, 10> QuoteReqID_def; ///< 报价请求ID
typedef uint8_t QuoteStatus_def; ///< 报价状态
typedef char LotType_def; ///< 订单类型
typedef std::string String_def; ///< 变长字符消息定义
typedef std::array<char, 20> CorporateActionID_def; ///< 公司行为
typedef std::array<char, 2> EntitlementType_def; ///< 权益编号
typedef std::array<char, 10> EntitlementNumber_def; ///< 权益类别
typedef char ListingStatus_def; ///< 上市状态
typedef uint8_t RejectCodeType_def; ///< 错误码类别
typedef std::array<char, 2> ShareNegotiability_def; ///< 股份性质
typedef std::array<char, 6> PBUID_def; ///< 转出交易单元
typedef uint8_t PositionSide_def; ///< 持仓方向
typedef std::array<char, 25> MemberMarginAccount_def; ///< 公司保证金账号
typedef uint16_t SecurityType_def; ///< 证券类别
typedef uint8_t InterestRateProduct_def; ///< 利率产品
typedef uint32_t RollOverNumber_def; ///< 已展期次数
typedef uint16_t Reserver_def; ///< 版本号
typedef uint64_t DepositRatio_def; ///< 保证金比例
typedef std::array<char, 40> CashMarginSet_def; ///< 信用标识集合
typedef char CloseOutFlag_def; ///< 了结标志
typedef char QueryContents_def; ///< 查询内容
typedef char CirculationType_def; ///< 流通类型
typedef char RepayWay_def; ///< 还款方式
typedef uint8_t PartitionNo_def; ///< 分区号
typedef uint16_t CRDUnderlyingType_def; ///< 融资融券标的类型
typedef uint64_t CRDBusinessSwitch_def; ///< 融资融券业务开关
typedef int64_t CRDUnderlyingRate_def; ///< 融资融券标的折算率N13(4)
typedef uint8_t CreditRating_def; ///< 信用级别
typedef uint8_t CloseoutStatus_def; ///< 平仓状态
typedef int64_t MtnGuaranteeRatio_def; ///< 维持担保比例N7(4)
typedef std::array<char, 32> OpeningSerialNum_def; ///< 开仓流水号
typedef uint8_t CreditIdentification_def; ///< 信用标识
typedef uint8_t PriceProperty_def; ///< 价格类型
typedef uint8_t TrdSubType_def; ///< 是否受限股份
typedef std::array<char, 12> Contactor_def; ///< 联系人
typedef std::array<char, 30> ContactInfo_def; ///< 联系人信息
typedef std::array<char, 8> ConfirmID_def; ///< 约定号
typedef uint8_t Pricing_def; ///< 大宗交易定价申报类型
typedef char ConfirmFlag_def; ///< 客户确认标识
typedef int64_t MarginRiskRate_def; ///< 保证金风险率N(8)
typedef uint16_t VersionNo_def; ///< 版本号
typedef uint8_t AccountRole_def; ///< 账户角色
typedef int64_t PriceTick_def; ///< 价格档位,N13(4)
typedef uint8_t SecurityStatus_def; ///< 证券状态
typedef int64_t BondInterest_def; ///< 债券利息,N11(8)
typedef int64_t CreditRatio_def; ///< 信用比例,N12(8)
typedef std::array<char, 11> Sno_def; ///< 流水号
typedef char CreditStatus_def; ///< 信用状态
typedef char CreditDirect_def; ///< 信用方向
typedef char DebtsDetailKind_def; ///< 负债明细类型
typedef char CreditDebtsQueryFlag_def; ///< 查询方向
typedef char LifeStatus_def; ///< 合约状态
typedef char CreditFlag_def; ///< 资金帐号属性
typedef uint8_t KeyErrorLevel_def; ///< 错误级别
typedef std::array<char, 8> BackStatusArray_def; ///< 回收状态组合
typedef char BackType_def; ///< 回收类型
typedef char PriceFlag_def; ///< 分配当日是否按当日收盘价更新lockprice
typedef std::array<char, 10> SubFlagArray_def; ///< 子锁类型组合
typedef char SourceType_def; ///< 证券来源
typedef char LockStatus_def; ///< 锁券状态
typedef std::array<char, 8> LockStatusArray_def; ///< 锁券状态组合
typedef char LockType_def; ///< 锁券类型
typedef uint32_t PlateID_def; ///< 板块代码
typedef std::array<char, 40> PlateSymbol_def; ///< 板块简称
typedef char CreditParamKind_def; ///< 参数类别
typedef std::array<char, 40> CreditName_def; ///< 参数名称
typedef char CtrlType_def; ///< 集中度类型
typedef char FirstInMode_def; ///< 首次进入模式
typedef char BackStatus_def; ///< 回收状态
typedef char ViolateStatus_def; ///< 违约状态
typedef char SubFlag_def; ///< 子锁类型
typedef uint8_t FundSeq_def; ///< 资金序号
typedef char ContractStatus_def; ///< 合同状态
typedef uint16_t ContractLimitMonth_def; ///< 信用合同期限月数
typedef char ApplyStatus_def; ///< 申请状态
typedef std::array<char, 8> ApplyStatusArray_def; ///< 申请状态组合
typedef int64_t Time9_def; ///< 某天内的时间,N9格式(HHMMSSsss)
typedef std::array<char, 32> Remark_def; ///< 备注
typedef uint8_t DealFlag_def; ///< 处理标志
typedef char CheckStatus_def; ///< 审批状态
typedef char QryChkStatus_def; ///< 审批状态（查询用，字典不同）
typedef std::array<char, 16> OperID_def; ///< 复核柜员
typedef char TibFlag_def; ///< 是否为科创板股票
typedef char ReserveStatus_def; ///< 融券预约状态
typedef char RightStatus_def; ///< 锁券资质状态
typedef char CheckMod_def; ///< 审批选项
typedef char CloseRightStatus_def; ///< 客户平仓自行权限状态
typedef char ApplyKind_def; ///< 自行申请标识
typedef char InvalidStatus_def; ///< 有效状态
typedef char CreditDigest_def; ///< 信用交易摘要
typedef std::array<char, 40> CreditDigestName_def; ///< 信用交易摘要名称
typedef char MatchType_def; ///< 成交类型
typedef std::array<char, 4> ExtBankCode_def; ///< 外部银行
typedef std::array<char, 32> BankID_def; ///< 外部银行
typedef std::array<char, 8> SpecKinds_def; ///< 扩展字段
typedef std::array<char, 255> CfmAns_def; ///< 存放批量自动复核失败原因
typedef char CreditDebtsType_def; ///< 信用负债类型
typedef uint8_t DebtsEffectDirect_def; ///< 负债发生方向
typedef uint32_t DigestID_def; ///< 操作摘要类型
typedef std::array<char, 128> FundDigestName_def; ///< 资金摘要名称
typedef uint32_t Days_def; ///< 天数
typedef char SellRepaySource_def; ///< 卖券还款来源
typedef char HandFlag_def; ///< 股手标志
typedef std::array<char, 2> ConcenCreditID_def; ///< 集中度参数组标识
typedef std::array<char, 32> ContractID_def; ///< 合同编号
typedef char BusinessRight_def; ///< 投资者权限
typedef std::array<char, 32> CustRiskName_def; ///< 用户风险名称
typedef char AccountSeq_def; ///< 股东帐号序号
typedef char AccountCreditFlag_def; ///< 股东类别信用帐户标志
typedef char CustomerStatus_def; ///< 客户状态
typedef char CustomerKind_def; ///< 客户类型
typedef uint8_t CustRiskLevel_def; ///< 投资者风险级别
typedef uint8_t InvestPerid_def; ///< 投资期限
typedef uint64_t InvestVarieties_def; ///< 投资品种集合
typedef uint8_t DeficitLevel_def; ///< 可承受亏损等级
typedef uint8_t SpecialRiskFlag_def; ///< 是否特别风险客户
typedef uint8_t ProfessionalKind_def; ///< 专业投资者类别
typedef uint8_t Assignment_def; ///< 指定交易标志
typedef std::array<char, 6> BusiType_def; ///< 中签查询业务类型
typedef std::array<char, 32> QSNo_def; ///< 清算流水号sno
typedef std::array<char, 32> MateNo_def; ///< 中签配号
typedef char GiveupStatus_def; ///< 放弃认购标识
typedef std::array<char, 512> MeetingDesc_def; ///< 股东大会名称
typedef std::array<char, 100> ProposalRelation_def; ///< 议案关系
typedef std::array<char, 1024> VotingProposalDesc_def; ///< 投票议案名称
typedef std::array<char, 50> ShareHolderRole_def; ///< 股东身份
typedef std::array<char, 100> VotingName_def; ///< 议案名称
typedef std::array<char, 20> ShareClass_def; ///< 股份类别
typedef std::array<char, 40> MeetingType_def; ///< 股东大会类型
typedef uint8_t VotingProposalType_def; ///< 议案类型
typedef char Char_def; ///< char
typedef uint32_t RiseFallWidth_def; ///< 涨跌幅
typedef uint8_t BusinessAbstractType_def; ///< 抽象业务编码
typedef char QSDataType_def; ///< 数据类型 0-配号数据 1-中签数据 2-放弃数据
typedef char CancelOrderFlag_def; ///< 撤单标志(表示该笔委托是否为撤单委托)
typedef std::array<char, 8> MktType_def; ///< 交易所证券类别
typedef std::array<char, 128> ExErrMsg_def; ///< 交易所错误信息描述
typedef uint8_t CollectType_def; ///< 汇总方式
typedef uint16_t PolicyID_def; ///< 策略编号
typedef uint8_t SubstituteFlag_def; ///< 现金替代标志
typedef int32_t PremiumRatio_def; ///< 溢价比例,ETF只有当现金替代标志=1，3时才有效
typedef int8_t PRType_def; ///< 申赎类型
typedef int32_t MaxCashRatio_def; ///< ETF最大现金替代比例
typedef uint8_t TradeType_def; ///< 交易类型
typedef char DealStatus_def; ///< 执行状态
typedef std::array<char, 8> LabelID_def; ///< 标签编号
typedef std::array<char, 64> LabelName_def; ///< 标签名称
typedef std::array<char, 64> Char64_def; ///< 扩展字段64
typedef char TrdID_def; ///< 集中交易股份明细查询中的交易类别
typedef char OverDueDelayFlag_def; ///< 锁券展期是否逾期展期
typedef uint8_t AssetFlag_def; ///< 网下现金资金资产标识
typedef std::array<char, 2> ShareNature_def; ///< 证券性质
typedef std::array<char, 40> ShareNatureSymbol_def; ///< 证券性质说明
typedef char ThirdSubscribeResult_def; ///< 新三板认购结果，0：可以参与申购 1：禁止参与申购 2：必须参与申购
typedef char IssueType_def; ///< 新三板发行类型
typedef std::array<char, 6> TimeC6_def; ///< 某天内的时间,C6格式(HHMMSS)
typedef std::array<char, 32> SerialNo_def; ///< 申报合同序号
typedef int64_t DateTime_def; ///< 日期时间,N17格式(YYYYmmDDHHMMSSsss)
typedef int64_t DateTime_def; ///< 日期时间,N17格式(YYYYmmDDHHMMSSsss)
typedef std::array<char, 2> Char2_def; ///< C2
typedef uint64_t DealSno_def; ///< 成交序号
typedef std::array<char, 128> PBUName_def; ///< 营业部/交易单元名称
typedef char IssueFlag_def; ///< 新三板发行方式
typedef std::array<char, 64> SecurityFullSymbol_def; ///< 证券全称
typedef int64_t Int64_def; ///< Int64
typedef std::array<char, 32> CifAccount_def; ///< 一户通号
typedef char DealStatus_def; ///< 执行状态
typedef int64_t ExchangeRate_def; ///< 参考汇率,N15(5)
typedef char MarketTransStatus_def; ///< 市场交易状态
typedef char QuotaStatus_def; ///< 额度状态
typedef std::array<char, 4> CorporateActionType_def; ///< 公司行为类型
typedef std::array<char, 2> ServiceType_def; ///< 服务类别
typedef std::array<char, 3> HKOrderType_def; ///< 申报类型
typedef std::array<char, 5> ClearNO_def; ///< 清算编号
typedef std::array<char, 12> ISIN_def; ///< ISIN编码
typedef std::array<char, 32> MarketDesc_def; ///< 市场信息描述
typedef std::array<char, 2> SaleLimitSecurityType_def; ///< 限售股证券类别
typedef char CirculationType_def; ///< 流通类型
typedef std::array<char, 2> RightType_def; ///< 权益类别(可选)
typedef std::array<char, 4> ListingYear_def; ///< 挂牌年份(可选)
typedef char PriceKind_def; ///< ETF篮子价格类型
typedef std::array<char, 2> TacticsKind_def; ///< 策略类型
typedef int64_t HitRate_def; ///< 命中率
typedef int64_t OrderGroup_def; ///< 批量委托批号
typedef char CustomerGroup_def; ///< 客户分组
typedef char FundKind_def; ///< 资金分类
typedef char FundLevel_def; ///< 资金室号
typedef char FundGroup_def; ///< 资金分组
typedef char OperWay_def; ///< 操作方式
typedef int64_t FeeRate_def; ///< 费用比例,N14(8)
typedef char DividePolicy_def; ///< LOF基金分红方式
typedef std::array<char, 32> DividePolicyName_def; ///< LOF基金分红方式名称
typedef char ETFBasketCanCancelFlg_def; ///< ETF篮子委托可撤单标志 0:存在可撤委托   1：全部成交或撤单，不存在可撤委托
typedef std::array<char, 2> Sbyy_def; ///< 失败原因定义
typedef int64_t Rate_def; ///< 折算率,N15(4)
typedef std::array<char, 16> IssuerNo_def; ///< 发行人编号
typedef char IssuerOutsideRating_def; ///< 发行人外部评级
typedef std::array<char, 32> Char32_def; ///< 扩展字段32
typedef uint8_t TradeReportType_def; ///< 成交申报消息类型
typedef uint8_t TradeReportTransType_def; ///< 成交申报事务类别
typedef std::array<char, 6> MemberID_def; ///< 交易商代码
typedef std::array<char, 2> InvestorType_def; ///< 交易主体类型
typedef std::array<char, 10> InvestorID_def; ///< 交易主体代码
typedef std::array<char, 120> InvestorName_def; ///< 客户名称
typedef std::array<char, 8> TraderCode_def; ///< 交易员代码
typedef uint16_t SettlType_def; ///< 结算方式
typedef uint8_t SettlPeriod_def; ///< 结算周期
typedef std::array<char, 160> Memo_def; ///< 协商成交备注
typedef char TradeHandlingInstr_def; ///< 成交申报模式
typedef uint16_t OwnerType_def; ///< 所有者类型
typedef uint8_t TradingType_def; ///< 债券现券交易方式
typedef uint8_t TradeType_def; ///< 交易类型
typedef uint32_t TrdType_def; ///< 成交申报业务类别
typedef uint8_t TrdSubType_def; ///< 成交申报业务子类别
typedef char ShareTransDirect_def; ///< 股份划转方向
typedef char MarketTransDirect_def; ///< 市场划转方向
typedef int8_t ConvertibleBondPlateClass_def; ///< 可转债板块分类
typedef uint8_t StkLevel_def; ///< 证券品种
typedef char RedressFlag_def; ///< 赔付状态
typedef std::array<char, 2> OrderWayExt_def; ///< 扩展委托方式
typedef uint8_t CustCreditTag_def; ///< 信用客户个性化标签
typedef std::array<char, 32> RtgsContractID_def; ///< rtgs合同号
typedef uint8_t Uint8_def; ///< uint8
typedef uint16_t Uint16_def; ///< uint16
typedef uint32_t Uint32_def; ///< uint32
typedef uint16_t ServerID_def; ///< 集中交易核心号
typedef std::array<char, 16> TradeRights_def; ///< 交易权限
typedef char BondClass_def; ///< 债券分类
typedef char BondStatus_def; ///< 债券状态
typedef char TimeFlag_def; ///< 统计时间段标识
typedef uint8_t SpecialIPOType_def; ///< 特殊发行类型
typedef char SecurityClass_def; ///< 证券分类-用于持仓查询筛选
typedef uint32_t CustMtmPushFlag_def; ///< 客户盯市推送标志
typedef uint8_t UTEPartitionNo_def; ///< UTE分区号
typedef uint8_t UTERevertScopeType_def; ///< UTE回切范围类型,1=单客户回切,2=节点回切
typedef std::array<char, 11> RuleID_def; ///< 规则ID
typedef uint32_t MsgTypeNo_def; ///< 消息编码
typedef uint32_t NumInGroup_def; ///< 重复组数量
/*
 * @brief class declearation
 */
class AgwHeader;
class AgwMsgLogin;
class AgwMsgLogout;
class AgwMsgHeartBeat;
class AgwMsgTestRequest;
class AgwNoPartitionArray;
class AgwMsgSync;
class AgwMsgSyncFinish;
class AgwRejectReasonArray;
class AgwMsgRejectReasonCodeSync;
class AgwMsgRequestAccessData;
class AgwFuncCodeArray;
class AgwCustIDFuncCodeArray;
class AgwCustIDArray;
class AgwMsgReportAccessData;
class AgwMsgPushFuncCodeList;
class AgwDiskSerailNumberArray;
class AgwPortArray;
class AgwRealTimeSpeedArray;
class AgwMsgTerminalInfoCollection;
class AgwMsgRealTimeControlInstruction;
class AgwMsgRealTimeControlInstructionResult;
class AgwMsgRealTimeQueryInstruction;
class AgwMsgRealTimeQueryInstructionResult;
class AgwMsgBizRejection;
class AgwMsgCustLogoutReq;
class AgwMsgCustLogoutResp;
class AgwMsgCustLoginReq;
class AgwMsgCustLoginRespAccountUnit;
class AgwMsgCustLoginRespFundAccountUnit;
class AgwMsgCustLoginResp;
class AgwMsgExtFundTransfer;
class AgwMsgExtFundTransferResult;
class AgwMsgCustPasswdModify;
class AgwMsgCustPasswdModifyResult;
class AgwMsgCustAuthReq;
class AgwMsgCustAuthResp;
class AgwMsgSyncBiz;
class AgwMsgSyncBizFinish;
class AgwMsgVipCustLoginReq;
class AgwMsgOrder;
class AgwNewMsgOrder;
class AgwMsgCashAuctionOrder;
class AgwMsgTibAfterHourAuctionOrder;
class AgwMsgIssueOrder;
class AgwMsgThirdIssueOrder;
class AgwMsgRightsIssueOrder;
class AgwMsgWarrantExerciseOrder;
class AgwMsgHKStocksThroughAuctionOrder;
class AgwMsgHKCorporateActionCancelOrder;
class AgwMsgFDEPCancelOrder;
class AgwMsgCancelOrder;
class AgwMsgBatchCancelOrder;
class AgwMsgOrderReport;
class AgwMsgTradeExecutionReport;
class AgwMsgTradeExecutionReportSeqNum;
class AgwMsgOrderStatusAck;
class AgwMsgCashAuctionTradeER;
class AgwMsgTibAfterHourAuctionTradeER;
class AgwMsgRepoAuctionOrder;
class AgwMsgRepoAuctionTradeER;
class AgwMsgSwapPutbackOrder;
class AgwMsgLOFOrder;
class AgwMsgLOFTradeER;
class AgwMsgPledgeInOrOutOrder;
class AgwMsgPledgeInOrOutTradeER;
class AgwMsgPasswordServiceOrder;
class AgwMsgNetVotingOrder;
class AgwMsgTenderOfferOrder;
class AgwMsgETFRedemptionOrder;
class AgwMsgConstituentStockUnit;
class AgwMsgETFRedemptionTradeER;
class AgwMsgBondDistributionOrder;
class AgwMsgBondDistributionTradeER;
class AgwMsgStructuredFundOrder;
class AgwMsgStructuredFundTradeER;
class AgwMsgHKStocksThroughTradeER;
class AgwMsgHKVotingOrder;
class AgwMsgHKCorporateActionOrder;
class AgwMsgHKDesignationTransferOrder;
class AgwMsgDcomCancelOrder;
class AgwMsgCreditAuctionOrder;
class AgwMsgCreditAuctionTradeER;
class AgwMsgMarginTradingOrder;
class AgwMsgMarginTradingTradeER;
class AgwMsgCollateralInOrOutOrder;
class AgwMsgThirdCollateralInOrder;
class AgwMsgBuySecuRepayOrder;
class AgwMsgBuySecuRepayTradeER;
class AgwMsgCurSecuRepayOrder;
class AgwMsgSellRepayAppointOrder;
class AgwMsgSellRepayAppointTradeER;
class AgwMsgExtDirectRepay;
class AgwMsgExtDirectRepayResult;
class AgwMsgCreditVote;
class AgwMsgCreditVoteResult;
class AgwMsgExtDirectRepayAppoint;
class AgwMsgExtDirectRepayAppointResult;
class AgwMsgExtCreditDebtsDelayApply;
class AgwMsgExtCreditReserveStkApply;
class AgwMsgExtCreditReserveStkApplyResult;
class AgwMsgExtCreditDebtsDelayApplyResult;
class AgwMsgExtCreditReserveStkCancel;
class AgwMsgExtCreditReserveStkCancelResult;
class AgwMsgExtCustSelfCloseApply;
class AgwMsgExtCustSelfCloseApplyResult;
class AgwMsgExtLockDelayApply;
class AgwMsgExtLockDelayApplyResult;
class AgwMsgCashAuctionFPGAOrder;
class AgwMsgCashAuctionFPGARejectAck;
class AgwMsgMarginTradingExtOrder;
class AgwMsgCreditAuctionExtOrder;
class AgwUTEComponentShareFrozenInfo;
class AgwMsgETFRedemptionExtOrder;
class AgwMsgCancelOrderSync;
class AgwMsgExtQueryBase;
class AgwMsgExtQueryResultBase;
class AgwMsgExtQueryMaxOrderQty;
class AgwMsgNewExtQueryMaxOrderQty;
class AgwMsgExtQueryMaxOrderQtyResult;
class AgwMsgNewExtQueryMaxOrderQtyResultBase;
class AgwMsgNewExtQueryMaxOrderQtyResult;
class AgwMsgExtQueryTradeOrder;
class AgwTradeOrderArray;
class AgwMsgExtQueryResultTradeOrder;
class AgwMsgExtQueryFund;
class AgwMsgExtQueryResultFund;
class AgwAccountIDArray;
class AgwMsgExtQueryShare;
class AgwShareArray;
class AgwMsgExtQueryResultShare;
class AgwMsgExtQueryIPOQty;
class AgwIPOQtyArray;
class AgwMsgExtQueryResultIPOQty;
class AgwMsgExtQueryTIBIPOQty;
class AgwMsgExtQueryResultTIBIPOQty;
class AgwMsgExtQueryTradeCollect;
class AgwTradeCollectArray;
class AgwExtQueryResultTradeCollect;
class AgwMsgExtQueryOrder;
class AgwOrderArray;
class AgwMsgExtQueryResultOrder;
class AgwExternalInsTETransFundReq;
class AgwExternalInsTETransFundResp;
class AgwExternalInsTETransCreditFundReq;
class AgwExternalInsTETransCreditFundResp;
class AgwSecurityIDArray;
class AgwExternalInsTETransCreditShareReq;
class AgwExternalInsTETransCreditShareResp;
class AgwExternalInsTETransCreditMarginAvlReq;
class AgwExternalInsTETransCreditMarginAvlResp;
class AgwExternalInsTETransCreditLimitReq;
class AgwExternalInsTETransCreditLimitResp;
class AgwMsgExtQueryFundEx;
class AgwExtQueryFundArray;
class AgwMsgExtQueryResultFundEx;
class AgwMsgAfterHoursPricingOrder;
class AgwMsgBlockTradeIntentionOrder;
class AgwMsgBlockTradeTransactionOrder;
class AgwMsgBlockTradePricingOrder;
class AgwMsgBlockTradeCancelOrder;
class AgwMsgAfterHoursPricingTradeER;
class AgwMsgBlockTradeTransactionTradeER;
class AgwMsgBlockTradePricingTradeER;
class AgwMsgReduceHoldingQuoteTransferOrder;
class AgwMsgReduceHoldingQuoteTransferCancelOrder;
class AgwMsgThirdPricingOrder;
class AgwMsgThirdPricingTradeER;
class AgwMsgThirdTransactionOrder;
class AgwMsgThirdTransactionTradeER;
class AgwMsgThirdCashAuctionOrder;
class AgwMsgThirdCashAuctionTradeER;
class AgwMsgThirdCashAuctionDelistOrder;
class AgwMsgThirdCashAuctionDelistTradeER;
class AgwMsgThirdBlockTradeOrder;
class AgwMsgThirdBlockTradeTradeER;
class AgwMsgThirdTenderOfferOrder;
class AgwMsgThirdDelistAgreementOrder;
class AgwSecurityInfoArray;
class AgwMsgExtQuerySecurityInfo;
class AgwMsgExtQueryResultSecurityInfo;
class AgwMsgExtQueryMd;
class AgwMsgExtQueryRightsIssueInfo;
class AgwMsgExtQueryResultRightsIssueInfo;
class AgwMsgExtQueryResultMd;
class AgwMsgExtQueryIPOInfo;
class AgwMsgExtQueryBondPledge;
class AgwBondPledgeArray;
class AgwMsgExtQueryResultBondPledge;
class AgwMsgExtQuerySecuSeat;
class AgwMsgExtQueryResultSecuSeat;
class AgwIPOInfoArray;
class AgwMsgExtQueryResultIPOInfo;
class AgwMsgExtQueryFundMaxdrawCrm;
class AgwFundMaxdrawCrmArray;
class AgwMsgExtQueryResultFundMaxdrawCrm;
class AgwMsgExtQueryCustTotalAssets;
class AgwCustTotalAssetsArray;
class AgwMsgExtQueryResultCustTotalAssets;
class AgwMsgExtQueryAppAssets;
class AgwAppAssetsArray;
class AgwMsgExtQueryResultAppAssets;
class AgwMsgExtQuerySelfShare;
class AgwSelfShareArray;
class AgwMsgExtQueryResultSelfShare;
class AgwMsgExtQueryCreditSelfShare;
class AgwCreditSelfShareArray;
class AgwMsgExtQueryResultCreditSelfShare;
class AgwMsgExtQueryCreditMarginAvlTransFlow;
class AgwCreditMarginAvlTransFlowArray;
class AgwMsgExtQueryResultCreditMarginAvlTransFlow;
class AgwMsgExtQueryCreditLimitTransFlow;
class AgwCreditLimitTransFlowArray;
class AgwMsgExtQueryResultCreditLimitTransFlow;
class AgwMsgExtQueryAvlPrice;
class AgwMsgExtQueryResultAvlPrice;
class AgwMsgExtQuerySelfFund;
class AgwSelfFundArray;
class AgwMsgExtQueryResultSelfFund;
class AgwMsgExtQueryAssetDebts;
class AgwMsgExtQueryResultAssetDebts;
class AgwMsgExtQueryFundMaxdraw;
class AgwFundMaxdrawArray;
class AgwMsgExtQueryResultFundMaxdraw;
class AgwMsgExtQueryTargetStk;
class AgwTargetStkArray;
class AgwMsgExtQueryResultTargetStk;
class AgwMsgExtQueryCreditDebtsDetail;
class AgwCreditDebtsDetailArray;
class AgwMsgExtQueryResultCreditDebtsDetail;
class AgwMsgExtQueryCreditDebts;
class AgwCreditDebtsArray;
class AgwMsgExtQueryResultCreditDebts;
class AgwMsgExtQueryCreditDebtsPB;
class AgwMsgExtQueryResultCreditDebtsPB;
class AgwMsgExtQueryCreditDebtsQFII;
class AgwMsgExtQueryResultCreditDebtsQFII;
class AgwMsgExtQueryCreditDebtsBySecurity;
class AgwMsgExtQueryResultCreditDebtsBySecurity;
class AgwMsgExtQueryPledgeStk;
class AgwPledgeStkArray;
class AgwMsgExtQueryResultPledgeStk;
class AgwMsgExtQueryCreditVote;
class AgwCreditVoteArray;
class AgwMsgExtQueryResultCreditVote;
class AgwMsgExtQueryCreditVoteCount;
class AgwMsgExtQueryResultCreditVoteCount;
class AgwMsgExtQueryAccountContractFund;
class AgwMsgExtQueryResultAccountContractFund;
class AgwContractArray;
class AgwMsgExtQueryConcentration;
class AgwArrayConcentration;
class AgwMsgExtQueryResultConcentration;
class AgwMsgExtQueryConcentrationType;
class AgwMsgExtQueryResultConcentrationType;
class AgwMsgExtQueryCreditCollateralRatio;
class AgwMsgExtQueryResultCreditCollateralRatio;
class AgwMsgExtQueryCreditConcentration;
class AgwArrayCreditConcentration;
class AgwMsgExtQueryResultCreditConcentration;
class AgwMsgExtQueryCreditContract;
class AgwMsgExtQueryResultCreditContract;
class AgwMsgExtQueryCreditDebtsCollect;
class AgwCreditDebtsCollectArray;
class AgwMsgExtQueryResultCreditDebtsCollect;
class AgwMsgExtQueryCreditDebtsCollectNew;
class AgwCreditDebtsCollectNewArray;
class AgwMsgExtQueryResultCreditDebtsCollectNew;
class AgwMsgExtQueryCreditFundTransFlow;
class AgwCreditFundTransFlowArray;
class AgwMsgExtQueryResultCreditFundTransFlow;
class AgwMsgExtQueryCreditShareTransFlow;
class AgwCreditShareTransFlowArray;
class AgwMsgExtQueryResultCreditShareTransFlow;
class AgwMsgExtQueryLabelConcentration;
class AgwArrayLabelConcentration;
class AgwMsgExtQueryResultLabelConcentration;
class AgwMsgExtQueryLabelStkCode;
class AgwArrayLabelStkCode;
class AgwMsgExtQueryResultLabelStkCode;
class AgwMsgExtQueryZHShareTargetStk;
class AgwArrayZHShareTargetStk;
class AgwMsgExtQueryResultZHShareTargetStk;
class AgwMsgExtQueryCreditStkLockTargetStk;
class AgwArrayCreditStkLockTargetStk;
class AgwMsgExtQueryResultCreditStkLockTargetStk;
class AgwMsgExtQueryCustCreditRiskTag;
class AgwMsgExtQueryResultCustCreditRiskTag;
class AgwMsgExtQueryCustCreditRiskInfo;
class AgwMsgExtQueryResultCustCreditRiskInfo;
class AgwMsgExtQueryMarginSellQtySpecialAccount;
class AgwArrayMarginSellQtySpecialAccount;
class AgwMsgExtQueryResultMarginSellQtySpecialAccount;
class AgwMsgExtQueryCreditAvlCanBuySecurity;
class AgwArrayCreditAvlCanBuySecurity;
class AgwMsgExtQueryResultCreditAvlCanBuySecurity;
class AgwMsgExtQueryCreditCustomerlizedPledgeSecurity;
class AgwArrayCreditCustomerlizedPledgeSecurity;
class AgwMsgExtQueryResultCreditCustomerlizedPledgeSecurity;
class AgwMsgExtQueryCustomerlizedCreditCollateralRatio;
class AgwMsgExtQueryResultCustomerlizedCreditCollateralRatio;
class AgwMsgExtQueryDDCustInfo;
class AgwArrayDDCustInfo;
class AgwMsgExtQueryResultDDCustInfo;
class AgwMsgExtQueryDDShare;
class AgwArrayDDShare;
class AgwMsgExtQueryResultDDShare;
class AgwMsgExtQueryDDMarginStk;
class AgwArrayDDMarginStk;
class AgwMsgExtQueryResultDDMarginStk;
class AgwMsgExtQueryDDDebtsFlow;
class AgwArrayDDDebtsFlow;
class AgwMsgExtQueryResultDDDebtsFlow;
class AgwMsgExtQueryDDCreditAsset;
class AgwArrayDDCreditAsset;
class AgwMsgExtQueryResultDDCreditAsset;
class AgwMsgExtQueryDDDebtsDelayFlow;
class AgwArrayDDDebtsDelayFlow;
class AgwMsgExtQueryResultDDDebtsDelayFlow;
class AgwMsgExtQueryDDMarginSell;
class AgwArrayDDMarginSell;
class AgwMsgExtQueryResultDDMarginSell;
class AgwMsgExtQueryDDConcentration;
class AgwArrayDDConcentration;
class AgwMsgExtQueryResultDDConcentration;
class AgwMsgExtQueryDDCreditDebts;
class AgwArrayDDCreditDebts;
class AgwMsgExtQueryResultDDCreditDebts;
class AgwMsgExtQueryDDDelayDebts;
class AgwArrayDDDelayDebts;
class AgwMsgExtQueryResultDDDelayDebts;
class AgwMsgExtQueryCreditDebtsDelayApplyResult;
class AgwCreditDebtsDelayApplyResultArray;
class AgwMsgExtQueryResultCreditDebtsDelayApplyResult;
class AgwMsgExtQueryCreditPreMaturityDebts;
class AgwCreditPreMaturityDebtsArray;
class AgwMsgExtQueryResultCreditPreMaturityDebts;
class AgwMsgExtQueryCreditPreMaturityDebtsByDays;
class AgwMsgExtQueryResultCreditPreMaturityDebtsByDays;
class AgwMsgExtQueryCreditReserveStk;
class AgwCreditReserveStkArray;
class AgwMsgExtQueryResultCreditReserveStk;
class AgwMsgExtQueryCreditReserveStkTrd;
class AgwCreditReserveStkTrdArray;
class AgwMsgExtQueryResultCreditReserveStkTrd;
class AgwMsgExtQueryCreditTradeCollect;
class AgwCreditTradeCollectArray;
class AgwMsgExtQueryResultCreditTradeCollect;
class AgwMsgExtQueryCustMarginAvl;
class AgwMsgExtQueryResultCustMarginAvl;
class AgwMsgExtQueryHHOrder;
class AgwOrderHHArray;
class AgwMsgExtQueryResultHHOrder;
class AgwMsgExtQueryOrderCollectAll;
class AgwOrderCollectAllArray;
class AgwMsgExtQueryResultOrderCollectAll;
class AgwMsgExtQueryPlateConcentration;
class AgwArrayPlateConcentration;
class AgwMsgExtQueryResultPlateConcentration;
class AgwMsgExtQueryPlateStkCode;
class AgwArrayPlateStkCode;
class AgwMsgExtQueryResultPlateStkCode;
class AgwMsgExtQuerySelfCloseApply;
class AgwSelfCloseApplyArray;
class AgwMsgExtQueryResultSelfCloseApply;
class AgwMsgExtQuerySelfCloseApplyRight;
class AgwMsgExtQueryResultSelfCloseApplyRight;
class AgwMsgExtQuerySelfCloseChk;
class AgwMsgExtQueryResultSelfCloseChk;
class AgwMsgExtQuerySellRepayType;
class AgwMsgExtQueryResultSellRepayType;
class AgwMsgExtQueryStkConcentration;
class AgwArrayStkConcentration;
class AgwMsgExtQueryResultStkConcentration;
class AgwMsgExtQueryTradeCollectAll;
class AgwTradeCollectAllArray;
class AgwMsgExtQueryResultTradeCollectAll;
class AgwMsgExtQueryCreditAssetDetails;
class AgwCreditAssetDetailsArray;
class AgwMsgExtQueryResultCreditAssetDetails;
class AgwMsgExtQueryCreditDebtsDetails;
class AgwCreditDebtsDetailsArray;
class AgwMsgExtQueryResultCreditDebtsDetails;
class AgwMsgExtQueryCreditLockRight;
class AgwMsgExtQueryResultCreditLockRight;
class AgwMsgExtQueryCreditMaxOrderQty;
class AgwMsgExtQueryCreditMaxOrderQtyResult;
class AgwMsgExtQueryCreditStkLock;
class AgwCreditStkLockArray;
class AgwMsgExtQueryResultCreditStkLock;
class AgwMsgExtQueryLockDelayApply;
class AgwLockDelayApplyArray;
class AgwMsgExtQueryResultLockDelayApply;
class AgwMsgExtQuerySubCreditStkLock;
class AgwSubCreditStkLockArray;
class AgwMsgExtQueryResultSubCreditStkLock;
class AgwMsgExtQuerySellRepayAppointAmt;
class AgwMsgExtQueryResultSellRepayAppointAmt;
class AgwMsgShareCostReset;
class AgwMsgShareCostResetResult;
class AgwMsgExtQueryCustID;
class AgwMsgExtQueryResultCustID;
class AgwAcctArray;
class AgwMsgExtQueryCreditAcct;
class AgwMsgExtQueryResultCreditAcct;
class AgwMsgExternalQueryDelisingPeriodBuyRight;
class AgwMsgExtQueryResultDelisingPeriodBuyRight;
class AgwMsgExternalQueryCustRiskValue;
class AgwMsgExtQueryResultCustRiskValue;
class AgwMsgExternalQueryAccountId;
class AgwAccountIdArray;
class AgwMsgExtQueryResultAccountId;
class AgwMsgExternalQueryCustBranchId;
class AgwMsgExtQueryResultCustBranchId;
class AgwMsgExternalQueryAccountInfo;
class AgwAccountInfoArray;
class AgwMsgExtQueryResultAccountInfo;
class AgwMsgExternalQueryAccountAssignment;
class AgwMsgExternalQueryResultAccountAssignment;
class AgwMsgExtGiveupNewStk;
class AgwMsgExtGiveupNewStkResult;
class AgwMsgExtCustMtmSub;
class AgwMsgExtCustMtmSubResult;
class AgwStkHitOrderArray;
class AgwMsgExtQueryStkHit;
class AgwMsgExtQueryResultStkHit;
class AgwVotingProposalArray;
class AgwMsgExtQueryVotingProposal;
class AgwMsgExtQueryResultVotingProposal;
class AgwMsgExtQueryETFSign;
class AgwMsgExtQueryResultETFSign;
class AgwVotingListArray;
class AgwMsgExtQueryVotingList;
class AgwMsgExtQueryResultVotingList;
class AgwReduceCustArray;
class AgwMsgExtQueryReduceCust;
class AgwMsgExtQueryResultReduceCust;
class AgwMsgETFQueryOrderEx;
class AgwETFShareRecordArray;
class AgwETFTradeOrderArray;
class AgwMsgETFQueryResultTradeOrder;
class AgwMsgETFConstituentStockUnit;
class AgwMsgETFRedemptionTradeDealER;
class AgwMsgExternalQueryTotalFundAsset;
class AgwTotalFundAssetArray;
class AgwMsgExtQueryResultTotalFundAsset;
class AgwMsgExtQueryHKSRefExchangeRate;
class AgwMsgExtQueryResultHKSRefExchangeRate;
class AgwMsgExtQueryHKSMarketTransStatus;
class AgwMsgExtQueryResultHKSMarketTransStatus;
class AgwMsgExtQueryHKSTradeCalendar;
class AgwHKSTradingCalendar;
class AgwMsgExtQueryResultHKSTradeCalendar;
class AgwMsgExtQueryHKSUnderlyingSecurity;
class AgwHKSUnderlyingSecurity;
class AgwMsgExtQueryResultHKSUnderlyingSecurity;
class AgwMsgExtQueryHKSSecurityStatus;
class AgwMsgExtQueryResultSecurityStatus;
class AgwMsgExtQueryHKFundAsset;
class AgwMsgExtQueryResultHKFundAsset;
class AgwMsgExtQueryHKPriceRange;
class AgwHKPriceRangeArray;
class AgwMsgExtQueryResultHKPriceRange;
class AgwMsgExtQueryHKCorporateActionRecord;
class AgwHKCorporateActionRecordArray;
class AgwMsgExtQueryResultHKCorporateActionRecord;
class AgwMsgExtQueryHKMarketValue;
class AgwMsgExtQueryResultHKMarketValue;
class AgwMsgExtQueryComponentShare;
class AgwComponentShareArray;
class AgwMsgExtQueryResultComponentShare;
class AgwMsgExtQueryETFInfo;
class AgwETFInfoArray;
class AgwMsgExtQueryResultETFInfo;
class AgwMsgBlockTradePreorder;
class AgwMsgBlockTradePreorderResult;
class AgwMsgExternalQueryBlockTradePreorderBranchRight;
class BlockTradePreorderBranchRight;
class AgwMsgExtQueryResultBlockTradePreorderBranchRight;
class AgwMsgBlockTradePreorderCancel;
class AgwMsgBlockTradePreorderCancelResult;
class AgwMsgExternalQueryBlockTradePreorder;
class PktBlockTradePreorderArray;
class AgwMsgExtQueryResultBlockTradePreorder;
class AgwMsgExternalQueryConvertibleBonds;
class AgwConvertibleBondsArray;
class AgwMsgExtQueryResultConvertibleBonds;
class AgwMsgExternalQueryThirdStkAccountLimit;
class AgwThirdStkAccountLimitArray;
class AgwMsgExtQueryResultThirdStkAccountLimit;
class AgwMsgExternalQueryThirdAgreementTransferMd;
class AgwThirdAgreementTransferMdArray;
class AgwMsgExtQueryResultThirdAgreementTransferMd;
class AgwMsgExternalQueryThirdStkAsset;
class AgwThirdStkAssetArray;
class AgwMsgExtQueryResultThirdStkAsset;
class AgwMsgExternalQueryThirdFgksbxx;
class AgwThirdFgksbxxArray;
class AgwMsgExtQueryResultThirdFgksbxx;
class AgwMsgExternalQueryThirdFgkcjxx;
class AgwThirdFgkcjxxArray;
class AgwMsgExtQueryResultThirdFgkcjxx;
class AgwMsgExternalQueryThirdIssueSubscribeResult;
class AgwThirdIssueSubscribeResultArray;
class AgwMsgExternalQueryResultThirdIssueSubscribeResult;
class AgwMsgExternalQueryThirdTenderOfferSecurityInfo;
class AgwThirdTenderOfferSecurityInfoArray;
class AgwMsgExtQueryResultThirdTenderOfferSecurityInfo;
class AgwMsgExtQueryThirdTenderOfferMaxOrderQty;
class AgwMsgExtQueryResultThirdTenderOfferMaxOrderQty;
class AgwMsgBlockTradeMdQuery;
class BlockTradeMarketData;
class AgwMsgBlockTradeMdQueryResult;
class AgwMsgExternalQueryThirdIssueSecurityInfo;
class AgwThirdIssueSecurityInfoArray;
class AgwMsgExternalQueryThirdIssueSecurityInfoResult;
class AgwMsgExtQueryUniIPOQty;
class AgwUniIPOQtyArray;
class AgwMsgExtQueryResultUniIPOQty;
class AgwNetVotingOrderArray;
class AgwMsgExtQueryNetVotingOrder;
class AgwMsgExtQueryResultNetVotingOrder;
class AgwMsgBondConsultTradeOrder;
class AgwMsgBondConsultTradeCancelOrder;
class AgwMsgBondConsultTradeTER;
class AgwMsgExternalQueryOrderForwardInfo;
class AgwOrderForwardInfoArray;
class AgwMsgExternalQueryResultOrderForwardInfo;
class AgwMsgExternalQueryBondMember;
class AgwMsgBondMemberArray;
class AgwMsgExternalQueryResultBondMember;
class AgwMsgExternalQueryTraderInvestor;
class AgwMsgBondTraderArray;
class AgwMsgBondInvestorArray;
class AgwMsgExternalQueryResultTraderInvestor;
class AgwMsgExtQueryBondMember;
class AgwMsgExtQueryResultBondMember;
class AgwMsgExtQueryBondInfo;
class AgwMsgExtQueryResultBondInfo;
class AgwMsgExtQueryThirdAdjustAsset;
class AgwMsgETFBasketOrder;
class AgwMsgETFBasketTradeER;
class AgwMsgOfflineETFSubscribe;
class AgwMsgOfflineETFSubscribeCancelOrder;
class AgwMsgOfflineETFSubscribeCancelAck;
class AgwMsgExtQryOfflineETFSubscribeQtyBySecurity;
class AgwMsgOfflineETFSubscribeQtyBySecurityArray;
class AgwMsgExtQryResultOfflineETFSubscribeQtyBySecurity;
class AgwMsgExtQueryResultThirdAdjustAsset;
class AgwMsgExtQueryHKTMd;
class AgwMsgExtQueryResultHKTMd;
class AgwMsgExternalQueryOfferTenderer;
class AgwMsgOfferTendererInfo;
class AgwMsgExternalQueryResultOfferTenderer;
class AgwSaleLimitStkAssetArray;
class AgwMsgExternalQuerySaleLimitStkAsset;
class AgwMsgExternalQueryResultSaleLimitStkAsset;
class AgwMsgExtQryETFCreation;
class AgwMsgExtQryETFCreationArray;
class AgwMsgExtQryResultETFCreation;
class AgwMsgOfRiskSign;
class AgwMsgOfRiskSignAck;
class AgwMsgLOFDividePolicySetOrder;
class AgwMsgExtQueryEtfStkAsset;
class AgwEtfStkAssetArray;
class AgwMsgExtQueryResultEtfStkAsset;
class AgwMsgExtQueryEtfBatchOrder;
class AgwEtfBatchOrderArray;
class AgwMsgExtQueryResultEtfBatchOrder;
class AgwMsgExtQueryPledgeBond;
class AgwPledgeBondArray;
class AgwMsgExtQryResultPledgeBond;
class AgwMsgExternalQueryLOFDividePolicyOrder;
class AgwMsgLOFDividePolicyOrderArray;
class AgwMsgExternalQueryResultLOFDividePolicyOrder;
class AgwMsgExternalQuerySaleFee;
class AgwMsgExternalQueryResultSaleFee;
class AgwMsgExtQryDaysOfRepoFundOccupation;
class AgwMsgDaysOfRepoFundOccupationArray;
class AgwMsgExtQryRstDaysOfRepoFundOccupation;
class AgwMsgExtQryCustSTDBonds;
class AgwMsgCustSTDBondsArray;
class AgwMsgExtQryRstCustSTDBonds;
class AgmMsgExternalQueryUnexpiredRepo;
class AgwMsgUnexpiredRepoOrder;
class AgwMsgExternalQueryResultUnexpiredRepo;
class AgwMsgExternalQueryReverseRepoTrade;
class AgwMsgReverseRepoTradeOrder;
class AgwMsgExternalQueryResultReverseRepoTrade;
class AgwMsgExtQueryPBOrder;
class AgwMsgExtQueryResultPBOrder;
class AgwMsgExtQueryPBTradeOrder;
class AgwMsgExtQueryResultPBTradeOrder;
class AgwMsgExtQueryAssetFrzPackage;
class AgwAssetFrzPackageArray;
class AgwMsgExtQueryResultAssetFrzPackage;
class AgwMsgExtQueryConvertibleBondForcedRedemption;
class AgwMsgExtQueryResultConvertibleBondForcedRedemption;
class AgwMsgExtQueryAdvanceRedressInfo;
class AgwAdvanceRedressArray;
class AgwMsgExtQueryResultAdvanceRedressInfo;
class AgwMsgExtQueryNewReverseRepoPosition;
class AgwNewReverseRepoPositionArray;
class AgwMsgExtQueryResultNewReverseRepoPosition;
class AgwMsgExtQueryTenderOfferSecurityInfo;
class AgwTenderOfferArray;
class AgwMsgExtQueryResultTenderOfferSecurityInfo;
class AgwMsgExtQueryTradeMarketCollect;
class AgwTradeMarketCollectArray;
class AgwExtQueryResultTradeMarketCollect;
class AgwMsgExtQueryTradeSecurityCollect;
class AgwTradeSecurityCollectArray;
class AgwExtQueryResultTradeSecurityCollect;
class AgwMsgDirectRepayMultiAppoint;
class AgwMsgDirectRepayMultiAppointResult;
class AgwMsgMultiDebtsDelayApply;
class AgwMsgMultiDebtsDelayApplyResult;
class AgwMsgMultiDebtsDelayApplyByDays;
class AgwMsgMultiDebtsDelayApplyByDaysResult;
class AgwMsgExtQueryDDFund;
class AgwDDFundArray;
class AgwMsgExtQueryResultDDFund;
class AgwMsgExtQueryDDSpecialOrder;
class AgwDDSpecOrderArray;
class AgwMsgExtQueryResultDDSpecialOrder;
class AgwDDSecurityInfoArray;
class AgwMsgExtQueryDDSecurityInfo;
class AgwMsgExtQueryResultDDSecurityInfo;
class AgwMsgExtQueryUTERevertNGTP;
class AgwMsgExtUTERevertNGTPResult;
class AgwMsgSingleAccountRevertNGTP;
class AgwMsgExtQueryCRCPreview;
class AgwMsgExtQueryResultCRCPreview;
class AgwMsgExternalQueryUnitTimeOrderAmt;
class AgwMsgExternalQueryResultUnitTimeOrderAmt;
class AgwMsgExtQueryThreePartyOrder;
class AgwMsgExtQueryResultThreePartyOrder;
class AgwMsgExtQueryThreePartyTradeOrder;
class AgwMsgExtQueryResultThreePartyTradeOrder;
class AgwMsgExtQueryG1ShareControl;
class AgwG1ShareControlArray;
class AgwMsgExtQueryResultG1ShareControl;
class AgwMsgG1ShareControlSet;
class AgwMsgG1ShareControlSetResult;
class AgwMsgExtQuerySwapPutbackInfo;
class AgwSwapPutbackArray;
class AgwMsgExtQueryResultSwapPutbackInfo;
class AgwMsgMtmIndicatorPushBase;
class AgwCustSecurityMtmIndicatorArray;
class AgwMsgExtCustSecurityMtmIndicatorPush;
class AgwMsgExtCustMtmIndicatorPush;
class AgwCompanySecurityMtmIndicatorArray;
class AgwMsgExtCompanySecurityMtmIndicatorPush;
class AgwMsgExtQueryCustStkInd;
class AgwMsgExtQueryResultCustStkInd;
class AgwMsgExtQueryCompStkInd;
class AgwMsgExtQueryResultCompStkInd;
}
}
#endif // ATP_GW_TYPES_AGW_EXTERNAL_H_
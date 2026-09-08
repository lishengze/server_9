# struct_req.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/struct_req.h`
> 作用：定义个微（GW）柜台协议的**请求/委托**消息体（非加速接口）。

## 概述

本文件定义了与个微柜台通信的各类**委托请求**消息体结构体，覆盖股票、回购、大宗、ETF、LOF、信用、融资融券、港股通、新三板（股转）、担保品划转等业务。文件头部还包含了个微协议的业务类型枚举注释（`<Value>` 定义）。

## 业务类型枚举（文件头部注释）

| 业务类型 | 值 | 描述 |
|---------|----|------|
| Pledge | 2 | 质押式回购交易 |
| BlockTrade | 5 | 协议交易(大宗交易) |
| AfterHoursPricing | 6 | 盘后定价交易(大宗交易) |
| Issue | 13 | 网上发行认购 |
| RightsIssue | 14 | 配股认购 |
| BondSellBack | 15 | 债券转股回售 |
| OpenFundPurchaseRedemption | 17 | 开放式基金申购赎回 |
| TenderOffer | 18 | 要约收购 |
| Designation | 20 | 转托管 |
| Vote | 21 | 网络投票 |
| HKDesignationTransfer | 61 | 港股通转托管 |
| HKCorporateAction | 62 | 港股通公司行为 |
| HKStocksThrough | 63 | 港股通交易 |
| HKVoting | 64 | 港股通投票 |
| TibAfterHour | 97 | 科创板股票盘后定价交易 |
| CreditAuction | 200 | 信用账户普通买入/普通卖出 |
| MarginTrading | 201 | 融资买入/融券卖出 |
| DirectRepay | 202 | 直接还款 |
| CollateralInOrOut | 203 | 担保品转入/转出 |
| BuySecuRepay | 204 | 买券还券 |
| CurSecuRepay | 205 | 现券还券 |
| RemainSecTransform | 206 | 余券划转 |
| CreditVote | 207 | 信用账户投票 |
| DirectRepayAppoint | 208 | 指定直接还款 |
| ThirdAgreementTransfer | 209 | 股转协议转让 |
| ThirdTenderOffer | 210 | 股转要约回购 |
| ThirdIssue | 211 | 股转网上发行 |
| ThirdCashAuction | 212 | 新三板集中交易 |
| ThirdBlockTrade | 213 | 股转大宗交易 |
| ThirdCashAuctionDelist | 214 | 新三板两网公司及退市公司股票 |
| OfflineETFSubscribe | 215 | 网下ETF发行认购 |

## 委托请求类清单

### 股票/基金类

| 类 | 业务 | pktno |
|----|------|-------|
| `AgwMsgRepoAuctionOrder` | 质押式回购集中竞价 | 100201 |
| `AgwMsgBlockTradeIntentionOrder` | 协议交易(大宗) | - |
| `AgwMsgAfterHoursPricingOrder` | 盘后定价交易 | - |
| `AgwMsgIssueOrder` | 网上发行认购 | - |
| `AgwMsgRightsIssueOrder` | 配股认购 | - |
| `AgwMsgSwapPutbackOrder` | 债券转股回售 | - |
| `AgwMsgLOFOrder` | LOF基金 | - |
| `AgwMsgTenderOfferOrder` | 要约收购 | - |
| `AgwMsgNetVotingOrder` | 网络投票 | - |
| `AgwMsgOfflineETFSubscribe` | 网下ETF发行认购 | - |

### 港股通类

| 类 | 业务 |
|----|------|
| `AgwMsgHKCorporateActionOrder` | 港股通公司行为 |
| `AgwMsgHKStocksThroughAuctionOrder` | 港股通交易 |
| `AgwMsgHKVotingOrder` | 港股通投票 |
| `AgwMsgTibAfterHourAuctionOrder` | 科创板盘后定价 |

### 信用/融资融券类

| 类 | 业务 |
|----|------|
| `AgwMsgCreditAuctionOrder` | 信用账户普通买卖 |
| `AgwMsgMarginTradingOrder` | 融资买入/融券卖出 |
| `AgwMsgCollateralInOrOutOrder` | 担保品转入/转出 |
| `AgwMsgBuySecuRepayOrder` | 买券还券 |
| `AgwMsgCurSecuRepayOrder` | 现券还券 |
| `AgwMsgCreditVote` | 信用账户投票 |
| `AgwMsgExtDirectRepayAppoint` | 指定直接还款 |
| `AgwMsgExtDirectRepay` | 直接还款 |

### 新三板（股转）类

| 类 | 业务 |
|----|------|
| `AgwMsgThirdPricingOrder` | 股转协议转让 |
| `AgwMsgThirdTenderOfferOrder` | 股转要约回购 |
| `AgwMsgThirdIssueOrder` | 股转网上发行 |
| `AgwMsgThirdCashAuctionOrder` | 新三板集中交易 |
| `AgwMsgThirdBlockTradeOrder` | 股转大宗交易 |
| `AgwMsgThirdCashAuctionDelistOrder` | 新三板两网退市 |

### 资金划转类

| 类 | 业务 | pktno |
|----|------|-------|
| `AgwExternalInsTETransFundReq` | TE间按证券账户划拨资金 | 810001 |
| `AgwExternalInsTETransCreditFundReq` | 信用账户TE间划拨资金 | 820001 |
| `AgwExternalInsTETransCreditMarginAvlReq` | 信用账户TE间划拨保证金余额 | 820004 |

## 委托请求结构示例（AgwMsgRepoAuctionOrder）

```cpp
class AgwMsgRepoAuctionOrder {
  uint8_t business_type;                // 业务类型
  std::array<char,8> security_id;       // 证券代码
  uint16_t market_id;                   // 市场ID
  int64_t client_order_time;            // 委托时间
  std::array<char,16> cust_id;          // 客户号
  std::array<char,16> fund_account_id;  // 资金帐号
  std::array<char,12> account_id;       // 账户id
  char side;                            // 买卖方向
  char order_type;                      // 订单类型
  int64_t price;                        // 委托价格
  int64_t order_qty;                    // 申报数量(张)/ETF份额
  int64_t client_seq_id;                // 用户系统消息序号
  std::string client_feature_code;      // 终端识别码
  char order_way;                       // 委托方式
  std::array<char,64> user_info;        // 用户私有信息
  std::string password;                 // 密码
  std::array<char,10> branch_id;        // origid+营业部ID
  uint16_t policy_id;                   // 策略编号
  char enforce_flag;                    // 强制风险标识
  int64_t stop_px;                      // 止损价
  int64_t min_qty;                      // 最低成交数量
  uint16_t max_price_levels;            // 最多成交价位数
  // ... 各业务可能有更多专有字段
};
```

## 设计要点
1. 本文件是 **个微柜台请求协议** 定义，与 `common_struct.h`、`struct_ans.h` 共同构成个微协议全集。
2. 所有委托结构都包含公共字段：`business_type / security_id / market_id / client_order_time / cust_id / fund_account_id / account_id / side / order_type / price / order_qty / client_seq_id / client_feature_code / order_way / user_info / password / branch_id / policy_id`。
3. 部分字段使用 `std::string`（如 `client_feature_code` / `password`），序列化需处理动态长度。
4. 这些结构体对应 `gw_counter_direct` 的 `build_*_msg` 待实现部分，以及非加速接口（`order_trade_type.h` 之外）的协议映射。
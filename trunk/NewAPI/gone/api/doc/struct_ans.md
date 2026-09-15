# struct_ans.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/struct_ans.h`
> 作用：定义个微（GW）柜台协议的**应答/回报**消息体（非加速接口）。

## 概述

本文件定义了与个微柜台通信的各类**应答与回报**消息体结构体，覆盖股票、信用、融资融券、ETF、港股通、新三板（股转）等业务的成交回报和执行报告。这些结构体是 **个微柜台真实协议** 的 C++ 映射，作为 `gw_counter_direct` 实现回报解析时的参考。

## 公共基类

### AgwMsgOrderReport（报告公共字段）
```cpp
class AgwMsgOrderReport {
  uint8_t partition;                  // 分区号
  int32_t index;                      // 索引号
  uint8_t business_type;              // 业务类型
  int64_t client_order_no;            // 客户订单号
  std::array<char,8> security_id;     // 证券代码
  uint16_t market_id;                 // 市场ID
  char exec_type;                     // 当前委托执行类型
  uint8_t order_status;               // 当前申报的状态
  std::array<char,16> cust_id;        // 客户号ID
  std::array<char,16> fund_account_id;// 资金账户ID
  std::array<char,12> account_id;     // 投资者帐号
  int64_t price;                      // 委托价格
  int64_t order_qty;                  // 委托数量
  int64_t leaves_qty;                 // 未成交数量
  int64_t cum_qty;                    // 累计成交数量
  char side;                          // 买卖方向
  int64_t transact_time;              // 回报时间
  std::array<char,64> user_info;      // 用户私有信息
  std::array<char,16> order_id;       // 交易所订单编号
  std::array<char,10> cl_ord_id;      // 申报合同号
  std::array<char,10> branch_id;      // origid+营业部ID
  std::string key_error_msg;          // 错误信息
  std::string key_error_code;         // 错误代码串
  uint8_t key_error_level;            // 错误级别
};
```
> 这是所有回报消息的公共字段基类，派生类在其基础上扩展。

## 报告类

| 类 | 说明 | 扩展字段 |
|----|------|---------|
| `AgwMsgTradeExecutionReport` | 成交执行报告（继承基类） | exec_id, last_px, last_qty, total_value_traded, fee |
| `AgwMsgTradeExecutionReportSeqNum` | 带序号的成交执行报告 | 同基类字段（冗余展开） |

## 成交回报类（TradeER）

| 类 | 业务 | 说明 |
|----|------|------|
| `AgwMsgRepoAuctionTradeER` | 质押式回购 | 集中竞价回购成交回报 |
| `AgwMsgAfterHoursPricingTradeER` | 盘后定价 | 大宗交易盘后定价成交回报 |
| `AgwMsgLOFTradeER` | LOF | LOF基金成交回报 |
| `AgwMsgCreditAuctionTradeER` | 信用账户 | 信用普通买入/卖出成交回报 |
| `AgwMsgMarginTradingTradeER` | 融资融券 | 融资买入/融券卖出成交回报 |
| `AgwMsgBuySecuRepayTradeER` | 买券还券 | 买券还券成交回报 |
| `AgwMsgThirdPricingTradeER` | 股转协议转让 | 协议转让成交回报 |
| `AgwMsgThirdCashAuctionTradeER` | 新三板集中交易 | 挂牌做市/连续竞价成交回报 |
| `AgwMsgThirdBlockTradeTradeER` | 股转大宗 | 股转大宗交易成交回报 |
| `AgwMsgThirdCashAuctionDelistTradeER` | 新三板两网退市 | 两网公司及退市公司成交回报 |

## 结果/应答类

| 类 | 说明 |
|----|------|
| `AgwMsgCreditVoteResult` | 信用账户投票结果 |
| `AgwMsgExtDirectRepayAppointResult` | 指定直接还款结果 |
| `AgwMsgExtDirectRepayResult` | 直接还款结果 |
| `AgwExternalInsTETransFundResp` | TE间按证券账户划拨资金应答 |
| `AgwExternalInsTETransCreditFundResp` | 信用账户TE间划拨资金应答 |
| `AgwExternalInsTETransCreditMarginAvlResp` | 信用账户TE间划拨保证金余额应答 |

## 设计要点
1. 本文件是 **个微柜台应答协议** 定义，与 `common_struct.h`（登录/委托/普通回报）互补。
2. 大部分成交回报类结构相似（分区/索引/订单/证券/报价/数量/时间/费用），差异在于业务专有字段。
3. `AgwMsgOrderReport` 作为公共基类，减少重复字段定义。
4. 这些结构体对应 `gw_counter_direct` 的回报解析（`deal_order_rtn` / `deal_trade_rtn` / `deal_cancel_rsp`）待实现部分。
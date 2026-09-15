# common_struct.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/common_struct.h`
> 作用：定义个微（GW）柜台协议的数据结构。

## 概述

本文件定义了与个微柜台（GW）通信的协议消息体结构体，包括登录、委托、撤单、成交推送等消息。这些结构体是 **个微柜台真实协议** 的 C++ 映射，当前作为 `gw_counter_direct` 实现真实协议时的参考。

## 登录相关

### AgwMsgLogin（链接登录）
```cpp
class AgwMsgLogin {
  std::array<char,32> sender_comp_id;     // 发送方代码
  std::array<char,32> target_comp_id;     // 接收方代码
  int32_t heart_bt_int;                   // 心跳间隔时间
  std::array<char,32> password;           // 用户登录密码
  std::array<char,32> default_app_ver_id; // 协议版本
  uint8_t spec_cust_flag;                 // 特殊客户标识，正常为0
  std::array<char,256> trade_pwd;         // 客户交易密码-特殊用户使用
};
```

### AgwMsgCustLoginReq（客户登录请求）
```cpp
class AgwMsgCustLoginReq {
  int64_t time;                           // 发起验证时间
  int64_t client_seq_id;                  // 用户系统消息序号
  char order_way;                         // 委托方式
  char pwd_type;                          // 密码加密类型
  std::array<char,16> cust_id;            // 客户号
  std::array<char,16> fund_account_id;    // 资金账户ID
  std::array<char,10> branch_id;          // origid+营业部ID
  std::array<char,12> account_id;         // 账户ID
  std::array<char,64> user_info;          // 用户私有信息
  std::array<char,2> order_way_ext;       // 委托方式扩展
  std::array<char,128> password;          // 客户密码
  std::array<char,1024> client_feature_code; // 终端识别码
  std::array<char,1024> extra_data;       // 自定义json字符串
};
```

### LogOnReq（个微登录请求）
```cpp
class LogOnReq {
  std::array<char,16> fund_account_id;    // 客户资金账号
  std::array<char,10> branch_id;          // 分支机构代码
  std::array<char,12> account_id;         // 股东账号
  std::array<char,16> cust_id;            // 客户号（暂不要求传入）
  int64_t client_seq_id;                  // 用户私有报单号
  int64_t agw_seq_id;                     // AGW序号
  uint32_t heart_bt_int;                  // 心跳间隔
  std::array<char,100> password;          // 密码
  std::array<char,1024> client_feature_code; // 终端识别码
  std::array<char,32> agw_user;           // AGW用户
};
```

## 委托相关

### NewAgwMsgCashAuctionOrder（现货委托 100101）
```cpp
class NewAgwMsgCashAuctionOrder {
  char side;                              // 买卖方向
  char order_type;                        // 市价限价标记
  int64_t order_price;                    // 委托价格
  int64_t order_qty;                      // 委托数量
  int64_t stop_px;                        // 止损价
  int64_t client_seq_id;                  // 用户私有报单号
  uint8_t tgw_id;                         // TGW编号
  uint16_t policy_id;                     // 佣金编号
  std::array<char,16> fund_account_id;    // 客户资金账号
  std::array<char,10> branch_id;          // 分支机构代码
  std::array<char,12> account_id;         // 股东账号
  std::array<char,8> security_id;         // 证券代码
  uint16_t market_id;                     // 市场ID
};
```

## 回报相关

### AgwMsgOrderStatusAck（委托应答 290001）
主要字段：`partition / index / business_type / order_no / security_id / market_id / exec_type / order_status / cust_id / fund_account_id / account_id / price / order_qty / leaves_qty / cum_qty / cancel_qty / side / transact_time / order_id / cl_ord_id / branch_id / client_seq_id / orig_cli_ord_no / frozen_trade_value / frozen_fee / reject_reason_code / ord_type / time_in_force / position_effect / covered_or_uncovered / account_sub_code`。

### AgwMsgCancelOrder（撤单 190001）
主要字段：`business_type / security_id / market_id / client_order_time / cust_id / fund_account_id / account_id / side / order_type / price / order_qty / client_seq_id / client_feature_code / order_way / user_info / branch_id / policy_id / orig_cli_ord_no / order_way_ext / orig_client_seq_id`。

### AgwMsgCashAuctionTradeER（成交推送 210115）
主要字段：`partition / index / business_type / client_order_no / security_id / market_id / exec_type / order_status / cust_id / fund_account_id / account_id / price / order_qty / leaves_qty / cum_qty / side / transact_time / order_id / cl_ord_id / branch_id / exec_id / last_px / last_qty / total_value_traded / fee / client_seq_id / cash_margin`。

## ETF 相关

### AgwMsgETFRedemptionOrder（ETF申赎委托 101201）
与现货委托类似，增加：`sh_account_id / sz_account_id / password / enforce_flag`。

### AgwMsgConstituentStockUnit（ETF成分股单元）
```cpp
class AgwMsgConstituentStockUnit {
  std::array<char,8> security_id;     // 证券代码
  uint16_t market_id;                 // 市场ID
  int64_t qty;                        // 成交数量
  int64_t amt;                        // 成交金额
  int64_t price;                      // 成交价格
  uint8_t etf_trade_report_type;      // ETF成交回报类型
  std::array<char,32> exec_id;        // 执行编号
  char margin_amt_type;               // 上交所现金替代资金类型
};
```

### AgwMsgETFRedemptionTradeER（ETF申赎成交回报 2012015）
包含 `std::vector<AgwMsgConstituentStockUnit> constituent_stock` 成分股数组。

## 信用账户相关

### AgwMsgCreditAuctionOrder（信用普通买入/卖出 7100701）
增加信用专有字段：`stop_px / min_qty / max_price_levels / time_in_force`。

### AgwMsgCreditAuctionTradeER（信用成交回报 7200715）
与普通成交回报结构相似。

### AgwMsgMarginTradingOrder（融资买入/融券卖出 7101701）
与信用普通委托结构相似。

### AgwMsgMarginTradingTradeER（融资融券成交回报 7201715）
与信用成交回报结构相似。

## 设计要点
1. 本文件定义的是 **个微柜台协议消息体**（GW 协议），与 g1 协议（fpga 使用）和 c98 协议（98 柜台使用）不同。
2. 当前 `gw_counter_direct` 的 `build_*_msg` 和回报解析全部留空，待依据本文件中的正式结构体实现。
3. 部分字段使用了 `std::string` / `std::vector`（如 `client_feature_code`、`constituent_stock`），序列化时需注意处理动态长度。
4. 消息 ID 注释标注了对应的业务代码（如 `100101` 为现货委托）。
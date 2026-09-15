# order_trade_type.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/order_trade_type.h`
> 作用：定义 API 层的请求/应答/回报数据结构（加速接口）。

## 概述

本文件定义了 API 对外暴露的核心业务数据结构，包括登录、委托、撤单、各类查询的请求，以及委托回报、成交回报、资金/持仓信息等应答。这些是 **加速接口**（走极速柜台）的数据结构。

## 登录

### LoginReq（登录请求）
```cpp
class LoginReq {
  int64_t client_req_no;                      // 客户请求号
  std::array<char,16> cust_id;                // 客户号
  std::array<char,16> fund_account_id;        // 客户资金账号
  std::array<char,12> account_id;             // 客户股东账号
  std::array<char,10> branch_id;              // 分支机构代码
  std::array<char,2> order_way_ext;           // 客户委托方式
  std::array<char,256> password;              // 密码
  std::array<char,64> user_info;              // 用户私有信息
  std::array<char,1024> client_feature_code;  // 客户终端信息
};
```
> 注意：LoginReq **没有 version 字段**。登录消息中的 version 是 SDK 在组包时自动填入 `g1_msg_ver` 常量的（详见 question.md 2.5 节）。

### LoginAns（登录应答）
```cpp
class LoginAns {
  int64_t client_req_no;                // 客户请求号
  std::array<char,16> cust_id;          // 客户号
  std::array<char,16> fund_account_id;  // 客户资金账号
  std::array<char,12> account_id;       // 客户股东账号
  std::array<char,10> branch_id;        // 分支机构代码
  int16_t market_type;                  // 市场
  int32_t err_code;                     // 错误码
  std::array<char,124> err_msg;         // 错误信息
  int64_t login_time;                   // 登陆时间 HHMMSSmmm
};
```

## 委托与撤单

### OrderReq（委托请求）
```cpp
class OrderReq {
  std::array<char,16> fund_account_id; // 客户资金账号
  std::array<char,10> branch_id;       // 分支机构代码
  char side;                           // 买卖方向
  char order_type;                     // 市价限价标记
  uint16_t policy_id;                  // 策略佣金ID
  uint16_t tgw_id;                     // TGW编号
  std::array<char,8> security_id;      // 证券代码
  int64_t order_price;                 // 委托价格，放大10000
  int64_t order_qty;                   // 委托数量，不放大100
  int64_t stop_price;                  // 止损价
  int64_t client_seq_id;               // 用户私有报单号
};
```

### CancelReq（撤单请求）
```cpp
class CancelReq {
  int64_t client_req_no;                // 客户请求号
  std::array<char,16> fund_account_id;  // 客户资金账号
  std::array<char,10> branch_id;        // 分支机构代码
  int64_t order_sys_no;                 // 柜台原始报单编号
  int64_t client_seq_id;                // 用户私有报单号(预留)
};
```

### CancelRsp（撤单响应）
```cpp
class CancelRsp {
  int64_t client_req_no;                // 客户请求号
  std::array<char,16> cust_id;          // 客户号
  std::array<char,16> fund_account_id;  // 客户资金账号
  std::array<char,12> account_id;       // 客户股东账号
  std::array<char,10> branch_id;        // 分支机构代码
  int16_t market_type;                  // 市场
  int64_t order_sys_no;                 // 柜台原始报单编号
  int64_t client_seq_id;                // 用户私有报单号
  int32_t err_code;                     // 撤单失败错误码
  int32_t rej_api;                      // 是否为api错误(1=api层拒绝)
};
```

## 委托回报与成交回报

### OrderRtn（委托回报）
委托信息字段：`cust_id / fund_account_id / account_id / branch_id / side / order_type / order_status / policy_id / market_type / security_id / order_price / order_qty / client_seq_id / rtn_type / err_code / order_sys_no / frozen_amount / fee / trade_qty / cancel_qty / order_time / update_time`。
> `order_price` 放大10000，`order_qty` 不放大100。

### TradeRtn（成交回报）
包含委托信息（同 OrderRtn）+ 成交信息：`exec_time / exec_id / exec_price / exec_qty / exec_amount / exec_fee`。

### TradeInfo（成交查询应答单条）
委托信息 + 成交信息，用于成交查询应答数组。

## 查询请求

| 类 | 说明 | 关键字段 |
|----|------|---------|
| `OrderQueryReq` | 委托查询 | client_req_no, order_sys_no, user_seq_id |
| `OrderBatchQueryReq` | 委托批量查询 | order_sys_no_begin/end（0=全部） |
| `TradeQueryReq` | 成交查询 | client_req_no, order_sys_no, user_seq_id |
| `TradeBatchQueryReq` | 成交批量查询 | exec_id_begin/end（0=全部） |
| `FundQueryReq` | 资金查询 | client_req_no, fund_account_id, branch_id |
| `PositionQueryReq` | 持仓查询 | client_req_no, security_id（空=全部） |

## 查询应答数据

### CustFundInfo（资金信息）
```cpp
class CustFundInfo {
  int64_t client_req_no;                // 客户请求号
  std::array<char,16> cust_id;          // 客户号
  std::array<char,16> fund_account_id;  // 客户资金账号
  std::array<char,10> branch_id;        // 分支机构代码
  int16_t market_type;                  // 市场
  int64_t avail_amount;                 // 可用资金
  int64_t token_amount;                 // 可取资金
};
```

### CustPositionInfo（持仓信息）
```cpp
class CustPositionInfo {
  int64_t client_req_no;                // 客户请求号
  std::array<char,16> cust_id;          // 客户号
  std::array<char,16> fund_account_id;  // 客户资金账号
  std::array<char,12> account_id;       // 客户股东账号
  std::array<char,10> branch_id;        // 分支机构代码
  int16_t market_type;                  // 市场
  std::array<char,8> security_id;       // 证券代码
  int64_t avail_qty;                    // 可用数量
  int64_t buy_done_qty;                 // 买入成交数量
  int64_t sell_done_qty;                // 卖出成交数量
};
```

## 订单状态与回报类型宏

### 订单状态（ORDER_STATE_*）
| 宏 | 值 | 含义 |
|----|----|------|
| `ORDER_STATE_ORDER_IDLE` | 0 | 订单空闲 |
| `ORDER_STATE_ORDER_PEND` | 1 | 订单待报 |
| `ORDER_STATE_ORDER_NEW` | 2 | 订单已报 |
| `ORDER_STATE_DONE_PART` | 3 | 订单部分成交 |
| `ORDER_STATE_DONE_FULL` | 4 | 订单全部成交 |
| `ORDER_STATE_CANCEL_ING` | 5 | 已报待撤 |
| `ORDER_STATE_PART_CANCEL_ING` | 6 | 部成待撤 |
| `ORDER_STATE_CANCEL_ALL` | 7 | 全部撤销 |
| `ORDER_STATE_PART_CANCEL` | 8 | 部成部撤 |
| `ORDER_STATE_DISCARD` | 9 | 内部废单 |

### 回报类型（RSP_TYPE_*）
| 宏 | 值 | 含义 |
|----|----|------|
| `RSP_TYPE_COUNTER_RSP` | 1 | 柜台响应 |
| `RSP_TYPE_EXCHANGE_RSP` | 2 | 交易所响应 |
| `RSP_TYPE_ORDER_TRADE` | 3 | 委托成交 |
| `RSP_TYPE_ORDER_DISCARD` | 4 | 委托废单 |
| `RSP_TYPE_CANCEL_RSP` | 5 | 撤单响应 |
| `RSP_TYPE_CANCEL_TRADE` | 6 | 撤单回报 |

## 设计要点
1. 所有字段用 `std::array` 定长存储，保证内存布局稳定、可跨线程拷贝。
2. 价格放大 10000、数量不放大，避免浮点误差。
3. 请求/应答结构体与柜台协议消息体（g1/c98）分离，柜台层负责转换。
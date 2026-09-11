
# FTE API 数据结构补齐分析

> 本文档分析 FTE 旧接口数据结构（`gw_external_message.h` / `gw_ex_msg_flat.h`）与新 API 数据结构（`trade_order_type.h`）之间的字段映射关系，识别双方缺失字段，并结合 FTE 业务流程说明各字段的作用和必要性。

---

## 一、对应关系总览

| 方向 | 旧接口 (FTE)       | 新 API        | 说明         |
| ---- | ------------------ | ------------- | ------------ |
| 请求 | `LogOnReq`       | `LoginReq`  | 登录请求     |
| 请求 | `TradeOrderReq`  | `OrderReq`  | 委托请求     |
| 请求 | `CancelOrderReq` | `CancelReq` | 撤单请求     |
| 响应 | `LogOnAns`       | `LoginAns`  | 登录应答     |
| 响应 | `TradeOrderER`   | `OrderRtn`  | 委托响应回报 |
| 响应 | `TradeOrderER`   | `CancelRsp` | 撤单响应回报 |
| 响应 | `TradeOrderER`   | `TradeRtn`  | 成交回报     |

> 注意：FTE 的 `TradeOrderER` 是**统一执行报告**，通过 `exec_type` 区分委托响应、撤单响应、成交回报三种场景。新 API 则拆分为三个独立结构体。

---

## 二、请求方向：新 API → FTE 转换

### 2.1 LoginReq → LogOnReq

#### 字段映射表

| 新 API (LoginReq)       | FTE (LogOnReq)                       | 类型/长度            | 映射说明                                                                              |
| ----------------------- | ------------------------------------ | -------------------- | ------------------------------------------------------------------------------------- |
| `client_req_no`       | ❌ 无直接对应字段                    | int64                | **缺失**。新 API 的请求号，用于请求-应答配对。FTE 使用 `client_seq_id` 做配对 |
| `cust_id`             | `trade_order_user.cust_id`         | char[16]             | 客户号。api 缓存                                                                      |
| `fund_account_id`     | `trade_order_user.fund_account_id` | char[16]             | ✅ 直接映射                                                                           |
| `account_id`          | `trade_order_user.account_id`      | char[12]             | ✅ 直接映射 api 缓存                                                                  |
| `branch_id`           | `trade_order_user.branch_id`       | char[10]             | ✅ 直接映射                                                                           |
| `order_way_ext`       | ❌ 无对应字段                        | char[2]              | **缺失**。客户委托方式，API缓存                                                 |
| `password`            | `password`                         | char[256]→char[100] | ⚠️ FTE 的`UTEPassword_def` 仅 100 字节，截断风险                                  |
| `user_info`           | ❌ 无对应字段                        | char[64]             | **缺失**。API 缓存                                                              |
| `client_feature_code` | `client_feature_code`              | char[1024]           | ✅ 直接映射                                                                           |

#### FTE 有但新 API 没有的字段

| FTE 字段          | 类型     | 业务作用                                     | 使用位置                                                | 是否必须                                  |
| ----------------- | -------- | -------------------------------------------- | ------------------------------------------------------- | ----------------------------------------- |
| `heart_bt_int`  | uint32_t | 心跳间隔（秒），控制客户端与服务端的心跳频率 | `tcpserver_handler::on_message` 中心跳超时检测        | **否**，可由系统配置默认值          |
| `agw_user`      | char[32] | 统一接入网关用户信息，标识网关登录身份       | `DealGatewayLogon()` 网关登录校验                     | **否**，仅统一接入场景需要          |
| `client_seq_id` | int64    | 客户端消息序号，用于请求-应答配对            | 贯穿整个订单处理流程，`FTE_SAVE_CLIENT_SEQ_ID` 宏保存 | **是**，可用 `client_req_no` 替代 |
| `agw_seq_id`    | int64    | 网关消息序号，统一接入网关分配               | 网关消息去重和排序                                      | **否**，仅统一接入场景              |

#### 补齐建议

```
LoginReq → LogOnReq 转换伪代码：

logon_req.trade_order_user.fund_account_id = login_req.fund_account_id
logon_req.trade_order_user.branch_id      = login_req.branch_id
logon_req.trade_order_user.account_id     = login_req.account_id
logon_req.trade_order_user.cust_id        = login_req.cust_id       // 或留空由FTE回填
logon_req.trade_order_user.client_seq_id  = login_req.client_req_no // 用client_req_no作为序列号
logon_req.trade_order_user.agw_seq_id     = 0                      // 非统一接入填0
logon_req.heart_bt_int                    = HEARTBEAT_DEFAULT       // 配置默认值
logon_req.password                        = login_req.password      // 注意截断
logon_req.client_feature_code             = login_req.client_feature_code
logon_req.agw_user                        = ""                      // 非统一接入填空
```

---

### 2.2 OrderReq → TradeOrderReq

#### 字段映射表

| 新 API (OrderReq)   | FTE (TradeOrderReq)                  | 类型/长度 | 映射说明                                                                   |
| ------------------- | ------------------------------------ | --------- | -------------------------------------------------------------------------- |
| `fund_account_id` | `trade_order_user.fund_account_id` | char[16]  | ✅ 直接映射                                                                |
| `branch_id`       | `trade_order_user.branch_id`       | char[10]  | ✅ 直接映射                                                                |
| `side`            | `trade_order_info.side`            | char      | ✅ 直接映射。FTE: '1'=买, '2'=卖, 'D'=申购, 'E'=赎回                       |
| `order_type`      | `trade_order_info.order_type`      | char      | ✅ 直接映射。FTE: '1'=市价, '2'=限价, 'U'=本方最优                         |
| `policy_id`       | ❌ 无对应字段                        | uint16_t  | **缺失**。策略佣金ID，FTE 在 `FeeInfo` 中根据证券+客户自动匹配佣金 |
| `tgw_id`          | ❌ 无对应字段                        | uint16_t  | **缺失**。TGW 编号，FTE 内部通过 `offer_way` 路由到不同网关        |
| `security_id`     | `trade_order_info.security_id`     | char[8]   | ✅ 直接映射                                                                |
| `order_price`     | `trade_order_info.order_price`     | int64     | ✅ 直接映射。FTE 价格精度 N13(4)，新 API 放大10000，需确认精度匹配         |
| `order_qty`       | `trade_order_info.order_qty`       | int64     | ✅ 直接映射。FTE 数量精度 N15(2)，新API 不放大100                          |
| `stop_price`      | `trade_order_info.stop_px`         | int64     | ✅ 直接映射                                                                |
| `client_seq_id`   | `trade_order_user.client_seq_id`   | int64     | ✅ 直接映射                                                                |

#### FTE 有但新 API 没有的字段

| FTE 字段       | 类型     | 业务作用                                            | 使用位置                                                                               | 是否必须                    |
| -------------- | -------- | --------------------------------------------------- | -------------------------------------------------------------------------------------- | --------------------------- |
| `account_id` | char[12] | **股东账户**，交易所报盘必需字段              | `SendOrderToExch()` 中填充到交易所协议结构体                                         | **是**，API 缓存补齐  |
| `cust_id`    | char[16] | **客户号**，登录时由 FTE 根据资金账户信息回填 | DSE 同步时使用，`external_order_ptr->trade_order_user.cust_id = fund_data->cust_id_` | **是**，API 缓存补齐  |
| `agw_seq_id` | int64    | 网关消息序号                                        | 统一接入场景的消息排序和去重                                                           | **否**，非统一接入填0 |
| `market_id`  | uint16   | **市场代码**，区分上海(101)/深圳(102)         | 贯穿订单处理全流程：校验、路由、报盘                                                   | **是**，客户端补齐    |

#### 补齐建议

```
OrderReq → TradeOrderReq 转换伪代码：

trade_order_req.trade_order_user.fund_account_id = order_req.fund_account_id
trade_order_req.trade_order_user.branch_id      = order_req.branch_id
trade_order_req.trade_order_user.account_id     = ? // 需要从登录会话中获取
trade_order_req.trade_order_user.cust_id        = ? // 需要从登录会话中获取
trade_order_req.trade_order_user.client_seq_id  = order_req.client_seq_id
trade_order_req.trade_order_user.agw_seq_id     = 0

trade_order_req.trade_order_info.security_id    = order_req.security_id
trade_order_req.trade_order_info.market_id      = ? // 需要从登录会话或证券信息获取
trade_order_req.trade_order_info.side           = order_req.side
trade_order_req.trade_order_info.order_type     = order_req.order_type
trade_order_req.trade_order_info.order_qty      = order_req.order_qty
trade_order_req.trade_order_info.order_price    = order_req.order_price
trade_order_req.trade_order_info.stop_px        = order_req.stop_price
```

**关键问题**：新 API 的 `OrderReq` 缺少 `account_id`（股东账户）和 `market_id`（市场代码）。这两个字段在 FTE 中都是**必需字段**：

- `market_id` 决定了路由到深交所还是上交所的网关
- `account_id` 是交易所报盘协议中的必填字段

建议在新 API 的 `OrderReq` 中增加 `account_id` 和 `market_id`，或在登录应答中返回客户的 `account_id` 列表和关联的市场。

---

### 2.3 CancelReq → CancelOrderReq

#### 字段映射表

| 新 API (CancelReq)  | FTE (CancelOrderReq)                 | 类型/长度 | 映射说明                                                                      |
| ------------------- | ------------------------------------ | --------- | ----------------------------------------------------------------------------- |
| `client_req_no`   | ❌ 无直接对应字段                    | int64     | **缺失**。请求号，FTE 使用 `orig_client_seq_id` 配对                  |
| `fund_account_id` | `trade_order_user.fund_account_id` | char[16]  | ✅ 直接映射                                                                   |
| `branch_id`       | `trade_order_user.branch_id`       | char[10]  | ✅ 直接映射                                                                   |
| `order_sys_no`    | ❌ 无直接对应字段                    | int64     | **缺失**。柜台原始报单编号，FTE 使用 `orig_clordno`（合同号）定位原单 |
| `client_seq_id`   | `trade_order_user.client_seq_id`   | int64     | ✅ 直接映射                                                                   |

#### FTE 有但新 API 没有的字段

| FTE 字段               | 类型     | 业务作用                               | 使用位置                                                    | 是否必须                       |
| ---------------------- | -------- | -------------------------------------- | ----------------------------------------------------------- | ------------------------------ |
| `account_id`         | char[12] | 股东账户，用于校验                     | `DealCancelOrderReq()` 中权限校验                         | **是**，api缓存获取      |
| `cust_id`            | char[16] | 客户号                                 | 权限校验和 DSE 同步                                         | **是**，api 缓存获取     |
| `agw_seq_id`         | int64    | 网关序号                               | 统一接入场景                                                | **否**                   |
| `orig_client_seq_id` | int64    | **原单客户端序号**，用于定位原单 | `DealCancelOrderReq()` 中通过 `GetOrderByNO()` 查找原单 | **是**，撤单必须指定原单 |
| `orig_clordno`       | int64    | **原单合同号**，FTE 内部订单编号 | `OrderManager::GetOrderByNO()` 查找原单                   | **是**，撤单必须指定原单 |

#### 补齐建议

```
CancelReq → CancelOrderReq 转换伪代码：

cancel_req.trade_order_user.fund_account_id = cancel_req.fund_account_id
cancel_req.trade_order_user.branch_id      = cancel_req.branch_id
cancel_req.trade_order_user.account_id     = ? // 从登录会话获取
cancel_req.trade_order_user.cust_id        = ? // 从登录会话获取
cancel_req.trade_order_user.client_seq_id  = cancel_req.client_req_no
cancel_req.trade_order_user.agw_seq_id     = 0

// 关键：需要将 order_sys_no 转换为 FTE 的 orig_clordno
// FTE 内部使用 clordno（int64 自增编号）定位订单，不是用 order_sys_no（交易所编号）
cancel_req.cancel_order_info.orig_clordno  = ? // 需要从 OrderRtn 的 order_sys_no 反向映射
cancel_req.cancel_order_info.orig_client_seq_id = ? // 需要从原单的 client_seq_id 获取
```

**关键问题**：新 API 使用 `order_sys_no`（柜台原始报单编号）定位原单，但 FTE 内部使用 `clordno`（自增的客户订单编号）定位原单。`order_sys_no` 对应 FTE 的 `order_id`（交易所订单编号），两者不是同一个编号。

**解决方案**：需要在撤单请求中传入 FTE 的 `clordno`（即新 API 中需要增加 `clordno` 字段），或者在 `OrderRtn` 响应中返回 `clordno` 与 `order_sys_no` 的对应关系，由 API 层维护映射表。

---

## 三、响应方向：FTE → 新 API 转换

### 3.1 LogOnAns → LoginAns

#### 字段映射表

| FTE (LogOnAns)                       | 新 API (LoginAns)   | 类型/长度         | 映射说明                                                |
| ------------------------------------ | ------------------- | ----------------- | ------------------------------------------------------- |
| `trade_order_user.cust_id`         | `cust_id`         | char[16]          | ✅ 直接映射                                             |
| `trade_order_user.fund_account_id` | `fund_account_id` | char[16]          | ✅ 直接映射                                             |
| `trade_order_user.account_id`      | `account_id`      | char[12]          | ✅ 直接映射                                             |
| `trade_order_user.branch_id`       | `branch_id`       | char[10]          | ✅ 直接映射                                             |
| `trade_order_user.client_seq_id`   | `client_req_no`   | int64             | ⚠️ FTE 的 client_seq_id 映射为新 API 的 client_req_no |
| `error_code`                       | `err_code`        | uint32_t→int32_t | ⚠️ 类型不同，需注意符号扩展                           |
| ❌ 无对应字段                        | `market_type`     | int16             | **缺失**，新 API 需要市场信息 -- 待确认           |
| ❌ 无对应字段                        | `err_msg`         | char[124]         | **缺失**，新 API 需要错误描述文本                 |
| ❌ 无对应字段                        | `login_time`      | int64             | **缺失**，新 API 需要登录时间                     |
| `session_status`                   | ❌ 无对应           | int32_t           | FTE 会话状态，新 API 不需要                             |

#### 补齐建议

```
LogOnAns → LoginAns 转换伪代码：

login_ans.client_req_no     = logon_ans.trade_order_user.client_seq_id
login_ans.cust_id           = logon_ans.trade_order_user.cust_id
login_ans.fund_account_id   = logon_ans.trade_order_user.fund_account_id
login_ans.account_id        = logon_ans.trade_order_user.account_id
login_ans.branch_id         = logon_ans.trade_order_user.branch_id
login_ans.market_type       = ? // FTE 无此字段，需根据登录请求的 market_id 或配置填充
login_ans.err_code          = logon_ans.error_code
login_ans.err_msg           = GetErrorText(logon_ans.error_code) // 根据错误码生成
login_ans.login_time        = current_timestamp()                // 取当前时间
```

**缺失字段补齐方案**：

- `market_type`：FTE 按市场分区（每个 FTE 实例只处理一个市场），可在 API 层根据客户登录时指定的市场填充
- `err_msg`：API 层维护错误码→错误文本映射表
- `login_time`：取当前系统时间，格式 HHMMSSmmm

---

### 3.2 TradeOrderER → OrderRtn（委托响应/回报）

`TradeOrderER` 是 FTE 的统一执行报告结构体，通过 `exec_type` 区分不同回报类型。当 `exec_type` 为 `'0'`(New) 或 `'8'`(Reject) 时，映射为 `OrderRtn`。

#### 字段映射表

| FTE (TradeOrderER.OrdERInfo)  | 新 API (OrderRtn) | 类型/长度         | 映射说明                                                                      |
| ----------------------------- | ----------------- | ----------------- | ----------------------------------------------------------------------------- |
| `side`                      | `side`          | char              | ✅ 直接映射                                                                   |
| `ord_type`                  | `order_type`    | char              | ✅ 直接映射                                                                   |
| `ord_status`                | `order_status`  | uint8_t→uint16_t | ⚠️ FTE 内部状态码需映射为新 API 的`ORDER_STATE_*` 常量                    |
| ❌ 无对应字段                 | `policy_id`     | uint16_t          | **缺失**，策略佣金ID，FTE 内部使用 `FeeInfo` 管理 -- 根据请求是否补充 |
| `market_id`                 | `market_type`   | uint16_t→int16_t | ⚠️ FTE: 101=上海,102=深圳；新 API 需映射                                    |
| ❌ 无对应字段                 | `reserved`      | int16_t           | 保留字段，填0                                                                 |
| `security_id`               | `security_id`   | char[8]           | ✅ 直接映射                                                                   |
| `price`                     | `order_price`   | int64             | ✅ 直接映射                                                                   |
| `order_qty`                 | `order_qty`     | int64             | ✅ 直接映射                                                                   |
| `client_seq_id`             | `client_seq_id` | int64             | ✅ 直接映射                                                                   |
| `exec_type`                 | `rtn_type`      | char→int32_t     | ⚠️ FTE exec_type 需映射为新 API 的`RSP_TYPE_*` 常量                       |
| `ord_rej_reason` / `code` | `err_code`      | uint16_t→int32_t | ⚠️ FTE 有交易所错误码和内部错误码两个字段                                   |
| `order_id`                  | `order_sys_no`  | char[16]→int64   | ⚠️ FTE 的 order_id 是字符串，新 API 是 int64                                |
| `frozen_trade_value`        | `frozen_amount` | int64             | ✅ 直接映射                                                                   |
| `frozen_fee`                | `fee`           | int64             | ⚠️ FTE 的 frozen_fee 是冻结费用，新 API 的 fee 是累计费用                   |
| `cum_qty`                   | `trade_qty`     | int64             | ✅ 直接映射                                                                   |
| ❌ 无对应字段                 | `cancel_qty`    | int64             | **缺失**，撤单成交数量                                                  |
| `transact_time`             | `order_time`    | int64             | ✅ 直接映射                                                                   |
| ❌ 无对应字段                 | `update_time`   | int64             | **缺失**，更新时间                                                      |
| ❌ 无对应字段                 | `trade_amount`  | int64             | **缺失**（注释掉了），累计成交金额 -- 不需要                            |
| `total_value_traded`        | ❌ 无对应         | int64             | FTE 成交金额，新 API 无此字段                                                 |
| `leaves_qty`                | ❌ 无对应         | int64             | FTE 剩余数量                                                                  |
| `user_info`                 | ❌ 无对应         | char[64]          | FTE 用户私有信息                                                              |
| `clordid`                   | ❌ 无对应         | char[10]          | FTE 申报合同号                                                                |
| `orig_clordid`              | ❌ 无对应         | char[10]          | FTE 原申报合同号                                                              |
| `cash_margin`               | ❌ 无对应         | char              | FTE 信用标识                                                                  |
| `cancel_flag`               | ❌ 无对应         | char              | FTE 撤单标志                                                                  |
| `clordno`                   | ❌ 无对应         | int64             | FTE 内部订单编号                                                              |
| `orig_clordno`              | ❌ 无对应         | int64             | FTE 原单编号                                                                  |
| `business_type`             | ❌ 无对应         | uint8_t           | FTE 业务类型（现货/ETF/两融）                                                 |
| `fee`                       | ❌ 无对应         | int64             | FTE 单笔成交费用（在委托响应中为0）                                           |
| `last_px`                   | ❌ 无对应         | int64             | FTE 最新成交价（在委托响应中为0）                                             |
| `last_qty`                  | ❌ 无对应         | int64             | FTE 最新成交量（在委托响应中为0）                                             |

#### 状态码映射

FTE `ord_status` 到新 API `order_status` 映射：

| FTE ord_status                      | 含义        | 新 API ORDER_STATE_*           |
| ----------------------------------- | ----------- | ------------------------------ |
| `OrdStatus::kNull` (0)            | 初始状态    | `ORDER_STATE_ORDER_IDLE` (0) |
| `OrdStatus::kSended` (1)          | 已发送/正报 | `ORDER_STATE_ORDER_NEW` (2)  |
| `OrdStatus::kPartiallyFilled` (2) | 部分成交    | `ORDER_STATE_DONE_PART` (3)  |
| `OrdStatus::kFilled` (3)          | 全部成交    | `ORDER_STATE_DONE_FULL` (4)  |
| `OrdStatus::kPendingCancel` (4)   | 待撤        | `ORDER_STATE_CANCEL_ING` (5) |
| `OrdStatus::kCancelled` (5)       | 已撤销      | `ORDER_STATE_CANCEL_ALL` (7) |
| `OrdStatus::kReject` (8)          | 已拒绝      | `ORDER_STATE_DISCARD` (9)    |

FTE `exec_type` 到新 API `rtn_type` 映射：

| FTE exec_type       | 含义   | 新 API RSP_TYPE_*              |
| ------------------- | ------ | ------------------------------ |
| `'0'` (New)       | 新订单 | `RSP_TYPE_COUNTER_RSP` (1)   |
| `'8'` (Reject)    | 拒绝   | `RSP_TYPE_ORDER_DISCARD` (4) |
| `'4'` (Cancelled) | 已撤销 | `RSP_TYPE_CANCEL_RSP` (5)    |
| `'F'` (Trade)     | 成交   | `RSP_TYPE_ORDER_TRADE` (3)   |

---

### 3.3 TradeOrderER → CancelRsp（撤单响应）

当 `exec_type` 为 `'4'`(Cancelled) 时，映射为 `CancelRsp`。

#### 字段映射表

| FTE (TradeOrderER)                          | 新 API (CancelRsp)  | 映射说明           |
| ------------------------------------------- | ------------------- | ------------------ |
| `trade_order_user.cust_id`                | `cust_id`         | ✅ 直接映射        |
| `trade_order_user.fund_account_id`        | `fund_account_id` | ✅ 直接映射        |
| `trade_order_user.account_id`             | `account_id`      | ✅ 直接映射        |
| `trade_order_user.branch_id`              | `branch_id`       | ✅ 直接映射        |
| `trade_order_user.client_seq_id`          | `client_req_no`   | ⚠️ 映射为请求号  |
| `order_er_info.market_id`                 | `market_type`     | ⚠️ 市场代码映射  |
| `order_er_info.order_id`                  | `order_sys_no`    | ⚠️ 字符串→int64 |
| `order_er_info.client_seq_id`             | `client_seq_id`   | ✅ 直接映射        |
| `order_er_info.code` / `ord_rej_reason` | `err_code`        | ⚠️ 错误码映射    |
| ❌ 无对应字段                               | `rej_api`         | int32_t            |
| ❌ 无对应字段                               | `order_sys_no`    | int64              |

---

### 3.4 TradeOrderER → TradeRtn（成交回报）

当 `exec_type` 为 `'F'`(Trade) 时，映射为 `TradeRtn`。

#### 字段映射表

| FTE (TradeOrderER)                   | 新 API (TradeRtn)   | 映射说明                                               |
| ------------------------------------ | ------------------- | ------------------------------------------------------ |
| `trade_order_user.cust_id`         | `cust_id`         | ✅                                                     |
| `trade_order_user.fund_account_id` | `fund_account_id` | ✅                                                     |
| `trade_order_user.account_id`      | `account_id`      | ✅                                                     |
| `trade_order_user.branch_id`       | `branch_id`       | ✅                                                     |
| `side`                             | `side`            | ✅                                                     |
| `ord_type`                         | `order_type`      | ✅                                                     |
| `ord_status`                       | `order_status`    | ⚠️ 状态码映射                                        |
| `market_id`                        | `market_type`     | ⚠️ 市场代码映射                                      |
| `security_id`                      | `security_id`     | ✅                                                     |
| `price`                            | `order_price`     | ✅                                                     |
| `order_qty`                        | `order_qty`       | ✅                                                     |
| `client_seq_id`                    | `client_seq_id`   | ✅                                                     |
| `order_id`                         | `order_sys_no`    | ⚠️ 字符串→int64                                     |
| `frozen_trade_value`               | `frozen_amount`   | ✅                                                     |
| `frozen_fee` + `fee`             | `fee`             | ⚠️ FTE 分开冻结费用和成交费用，新 API 合并为累计费用 |
| `cum_qty`                          | `trade_qty`       | ✅                                                     |
| ❌ 无对应字段                        | `cancel_qty`      | **缺失 -- **                                          |
| `transact_time`                    | `order_time`      | ✅                                                     |
| ❌ 无对应字段                        | `exec_time`       | **缺失**，成交时间                               |
| `exec_id`                          | `exec_id`         | ⚠️ FTE: char[16], 新API: char[32]                    |
| `last_px`                          | `exec_price`      | ✅ 最新成交价                                          |
| `last_qty`                         | `exec_qty`        | ✅ 最新成交量                                          |
| ❌ 无对应字段                        | `exec_amount`     | **缺失**，成交金额                               |
| `fee`                              | `exec_fee`        | ✅ 单笔成交费用                                        |
| ❌ 无对应字段                        | `policy_id`       | **缺失**                                         |
| ❌ 无对应字段                        | `reserved`        | **缺失**                                         |
| ❌ 无对应字段                        | `update_time`     | **缺失**                                         |
| `total_value_traded`               | ❌ 无对应           | FTE 累计成交金额，可映射到`exec_amount`              |

---

## 四、缺失字段汇总与补齐方案

### 4.1 请求方向：新 API 缺少的字段（需新增）

| 所属结构      | 缺少字段                  | 业务作用                 | 补齐方案                                                                 |
| ------------- | ------------------------- | ------------------------ | ------------------------------------------------------------------------ |
| `OrderReq`  | `account_id` (char[12]) | 股东账户，交易所报盘必需 | **建议新增**。或从登录会话中获取默认账户                           |
| `OrderReq`  | `market_id` (uint16)    | 市场代码，决定路由       | **建议新增**。或从 security_id 前两位推断                          |
| `CancelReq` | `account_id` (char[12]) | 股东账户                 | **建议新增**。或从登录会话获取                                     |
| `CancelReq` | `orig_clordno` (int64)  | FTE 内部订单编号         | **建议新增**。或在 `OrderRtn` 中返回 `clordno`，API 层维护映射 |

### 4.2 请求方向：FTE 有但新 API 没有的字段（API 层补齐）

| FTE 字段                      | 补齐方式                              |
| ----------------------------- | ------------------------------------- |
| `heart_bt_int`              | 使用系统配置默认值（如 30 秒）        |
| `agw_user` / `agw_seq_id` | 非统一接入场景填 0/空                 |
| `cust_id`                   | 登录后由 FTE 自动回填，API 层无需处理 |

### 4.3 响应方向：新 API 缺少的字段（FTE 可补齐）

| 新 API 字段              | FTE 对应数据           | 补齐方式                                |
| ------------------------ | ---------------------- | --------------------------------------- |
| `LoginAns.market_type` | 登录时的市场分区       | 从 FTE 实例的`partition_market_` 获取 |
| `LoginAns.err_msg`     | 错误码                 | API 层维护错误码→文本映射表            |
| `LoginAns.login_time`  | 当前时间               | 取`GET_GLOBAL_TIMESTAMP()`            |
| `OrderRtn.policy_id`   | FTE 内部 FeeInfo       | 暂填0，后续根据 FeeInfo 补齐            |
| `OrderRtn.cancel_qty`  | 撤单数量               | 从`orig_clordno` 对应的原单获取       |
| `OrderRtn.update_time` | 交易所更新时间         | 从交易所回报中获取                      |
| `TradeRtn.exec_time`   | 成交时间               | 从`last_exec_id` 关联的成交时间获取   |
| `TradeRtn.exec_amount` | `total_value_traded` | 直接映射`last_px * last_qty`          |
| `CancelRsp.rej_api`    | 错误来源               | 0=FTE内部拒绝, 1=交易所拒绝             |

### 4.4 响应方向：FTE 有但新 API 不需要的字段

| FTE 字段                            | 说明                                                  |
| ----------------------------------- | ----------------------------------------------------- |
| `clordid` (char[10])              | FTE 内部申报合同号，新 API 无需暴露                   |
| `clordno` (int64)                 | FTE 内部订单编号，新 API 使用`order_sys_no`         |
| `orig_clordid` / `orig_clordno` | 原单编号，新 API 无需暴露                             |
| `cash_margin` (char)              | 信用标识，仅两融业务使用                              |
| `cancel_flag` (char)              | 撤单标志，新 API 通过`rtn_type` 区分                |
| `leaves_qty` (int64)              | 剩余数量，新 API 可通过`order_qty - trade_qty` 计算 |
| `user_info` (char[64])            | 用户私有信息，新 API 无需暴露                         |
| `business_type` (uint8_t)         | FTE 业务类型，新 API 通过其他字段判断                 |

---

## 五、关键问题与风险点

### 5.1 撤单定位方式不一致（P0）

**问题**：新 API 使用 `order_sys_no`（柜台原始报单编号，对应 FTE 的 `order_id`）定位原单，但 FTE 内部使用 `clordno`（自增的客户订单编号）定位原单。两者是不同编号体系。

**影响**：撤单请求无法直接转换为 FTE 的撤单请求。

**解决方案**：

- 方案A：在 `OrderRtn` 和 `TradeRtn` 中同时返回 `order_sys_no` 和 `clordno`，API 层维护 `order_sys_no → clordno` 映射
- 方案B：在 `CancelReq` 中增加 `clordno` 字段
- **推荐方案A**，对客户端透明

### 5.2 股东账户缺失（P1）

**问题**：新 API 的 `OrderReq` 和 `CancelReq` 均缺少 `account_id`（股东账户），但该字段是 FTE 报盘协议的必需字段。

**影响**：无法直接调用 FTE 的报盘接口。

**解决方案**：

- 在 `OrderReq` 中增加 `account_id` 字段
- 或在登录应答中返回客户的所有股东账户，下单时由 API 层根据 `security_id` 和 `market_id` 自动选择

### 5.3 市场代码缺失（P1）

**问题**：新 API 的 `OrderReq` 缺少 `market_id`，但 FTE 需要根据市场代码路由到不同网关。

**影响**：无法确定订单发往上海还是深圳。

**解决方案**：

- 在 `OrderReq` 中增加 `market_id` 字段
- 或根据 `security_id` 前两位推断市场（6xxxxx=上海, 0xxxxx/3xxxxx=深圳）

### 5.4 价格精度对齐（P2）

**问题**：

- FTE 价格精度：`Price_def` 为 `int64_t`，注释 N13(4)，即放大 **10000** 倍
- 新 API 价格精度：`order_price` 为 `int64_t`，注释"放大 **10000**"
- 两者一致，无需转换

**数量精度**：

- FTE 数量精度：`Qty_def` 为 `int64_t`，注释 N15(2)，即放大 **100** 倍
- 新 API 数量精度：`order_qty` 为 `int64_t`，注释"不放大100"
- **两者不一致**，新 API 传入数量时需要乘以 100

### 5.5 密码长度截断（P2）

**问题**：新 API 的 `password` 为 `char[256]`，但 FTE 的 `UTEPassword_def` 仅为 `char[100]`。

**影响**：密码超过 100 字节时被截断，可能导致登录失败。

**解决方案**：API 层截断前先告警，或建议客户端限制密码长度。

### 5.6 exec_id 长度不一致（P2）

**问题**：FTE 的 `ExecID_def` 为 `char[16]`，但新 API `TradeRtn.exec_id` 为 `char[32]`。

**影响**：FTE 的 16 字节 exec_id 可直接填充到新 API 的 32 字节字段中，剩余补零。

---

## 六、转换逻辑总结

### 请求转换器（New API → FTE）

```
LoginReq → LogOnReq:
  - 直接映射: fund_account_id, branch_id, account_id, cust_id, password, client_feature_code
  - client_req_no → client_seq_id
  - 补充默认值: heart_bt_int=30, agw_user="", agw_seq_id=0

OrderReq → TradeOrderReq:
  - 直接映射: fund_account_id, branch_id, side, order_type, security_id, order_price, order_qty, stop_price, client_seq_id
  - 需补充: account_id(从会话), cust_id(从会话), market_id(从证券代码推断), agw_seq_id=0
  - 注意: order_qty 需乘以100(FTE精度)

CancelReq → CancelOrderReq:
  - 直接映射: fund_account_id, branch_id, client_seq_id
  - 需补充: account_id(从会话), cust_id(从会话), agw_seq_id=0
  - 关键: order_sys_no → orig_clordno(需映射表)
```

### 响应转换器（FTE → New API）

```
LogOnAns → LoginAns:
  - 直接映射: cust_id, fund_account_id, account_id, branch_id
  - client_seq_id → client_req_no
  - error_code → err_code
  - 补充: market_type(从分区), err_msg(从映射表), login_time(当前时间)

TradeOrderER → OrderRtn (exec_type='0'/'8'):
  - 直接映射: side, order_type, security_id, order_price, order_qty, client_seq_id
  - ord_status → order_status(状态映射)
  - exec_type → rtn_type(类型映射)
  - order_id → order_sys_no(字符串转int64)
  - frozen_trade_value → frozen_amount
  - cum_qty → trade_qty
  - transact_time → order_time
  - 补充: policy_id=0, reserved=0, cancel_qty=0, update_time=transact_time

TradeOrderER → TradeRtn (exec_type='F'):
  - 相同字段映射同 OrderRtn
  - 额外: exec_id, last_px→exec_price, last_qty→exec_qty, fee→exec_fee
  - total_value_traded → exec_amount
  - 补充: exec_time=transact_time, cancel_qty=0, update_time=transact_time

TradeOrderER → CancelRsp (exec_type='4'):
  - 相同字段映射同 OrderRtn
  - code/ord_rej_reason → err_code
  - 补充: rej_api=0
```

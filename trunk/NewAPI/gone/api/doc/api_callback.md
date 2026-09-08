# api_callback.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/api_callback.h`
> 作用：定义 API 回调接口类，是柜台回报 → 客户的统一出口。

## 概述

本文件定义了客户回调接口 `api_callback`，以及回调中用到的辅助结构体（`StreamInfo`、`QueryAnsCtl`）、错误事件枚举（`err_event_type`）和几个常量。所有柜台（98 / fpga / gw）的回报最终都收敛到 `callback_manager` 的 `on_*` 接口，再由 `callback_manager` 调用本类对应的虚方法通知客户。

## 常量与辅助结构

| 名称 | 类型 | 说明 |
|------|------|------|
| `STREAM_API_COUNTER` | `constexpr int32_t = 100` | 标识流信息来自 API 层（此时 stream_seq=0） |
| `MAX_ERR_DESC_LEN` | `constexpr int32_t = 256` | 错误描述最大长度（含结尾 `\0`） |

### StreamInfo（流信息）
```cpp
struct StreamInfo {
  int32_t counter_type; // 柜台类型, STREAM_API_COUNTER=api, 此时流序号=0
  int64_t stream_seq;   // 流序号（同步后台可靠流水号，用于去重/断点续传）
};
```
用于委托回报 / 成交推送 / 撤单响应回调，标识推送来源和流序号。

### QueryAnsCtl（批量查询应答控制）
```cpp
struct QueryAnsCtl {
  int32_t count;         // 本次回调中的数据条数
  int32_t is_last;       // 本次查询是否结束（true=所有数据已返回）
  int64_t client_req_no; // 客户请求号（对应查询请求中的 client_req_no）
};
```
用于委托 / 成交 / 持仓查询应答回调，支持分页返回。

### err_event_type（错误事件类型）
```cpp
enum class err_event_type : int32_t {
  none_type = 0,         // 无效类型
  agw_user_login = 1,    // agw 用户登录处理错误
  fast_user_offline = 2, // 极速柜台用户下线
  get_sec_info = 3       // 获取证券信息错误
};
```
用于 `on_error` 回调标识错误来源。

## 类 api_callback

客户可选择性地实现关心的回调方法，所有方法均有默认空实现（不实现 = 不回调），避免被迫实现全部方法。

### 回调方法一览

| 方法 | 参数 | 触发时机 |
|------|------|---------|
| `on_login` | `const LoginAns &ans` | login() 完整登录流程（98agw→98账户→极速柜台）最终结果 |
| `on_order_rtn` | `si, rtn` | 委托回报 |
| `on_trade_rtn` | `si, rtn` | 成交推送 |
| `on_cancel_rsp` | `si, rsp` | 撤单响应（柜台废单） |
| `on_order_query_ans` | `ans_arr, ctl` | 委托查询应答 |
| `on_trade_query_ans` | `ans_arr, ctl` | 成交查询应答 |
| `on_fund_query_ans` | `info` | 资金查询应答 |
| `on_position_query_ans` | `ans_arr, ctl` | 持仓查询应答（数组 + 分页控制） |
| `on_link_status` | `counter_type, link_type, status` | 链接状态变化（0=断线, 1=连接成功） |
| `on_error` | `event_type, err_code, err_desc` | 通用错误 |

### on_link_status 参数说明
- `counter_type`：柜台类型，0=98柜台，1=极速柜台
- `link_type`：链接类型，0=网关链接，1=极速交易链接
- `status`：0=已断线，1=连接成功

### 双模式回调
`callback_manager` 会按 `callback_mode` 配置分派：
- **direct**：直接同步调用（在 IO/引擎线程中执行）
- **queued**：打包入无锁队列，由独立回调线程 `do_work()` 取出后调用

## 设计要点
1. 所有柜台回报统一收敛到本接口，对客户暴露单一回调入口。
2. 默认空实现保证客户只实现关心的回调，降低接入成本。
3. 回调方法均为虚函数，客户通过继承 `api_callback` 并覆写实现。
# callback_manager 文档

> 源文件：`trunk/NewAPI/gone/api/src/callback_manager.h` + `callback_manager.cpp`
> 作用：回调管理类，统一分发柜台回报给客户回调接口。

## 概述

`callback_manager` 继承 `simple_thread`，使用 `que_mth_buf` 无锁队列实现队列模式回调。所有柜台回报收敛到本类的 `on_*` 接口，内部按 `callback_mode` 决定是**直接同步调用**还是**入队后由回调线程调用**。

## 回调事件类型（cb_event_type）

| 枚举值 | 值 | 对应 on_* 接口 |
|--------|----|---------------|
| `login` | 0 | on_login |
| `order_rtn` | 1 | on_order_rtn |
| `trade_rtn` | 2 | on_trade_rtn |
| `cancel_rsp` | 3 | on_cancel_rsp |
| `order_query` | 4 | on_order_query_ans |
| `trade_query` | 5 | on_trade_query_ans |
| `fund_query` | 6 | on_fund_query_ans |
| `position_query` | 7 | on_position_query_ans |
| `link_status` | 8 | on_link_status |
| `error` | 9 | on_error |

## 回调事件头 cb_event_head

```cpp
struct cb_event_head {
  cb_event_type type;   // 事件类型（对应具体 on_* 分发逻辑）
  int32_t data_len;     // 事件数据长度(不含头)
};
```

## 单写/多写模式

- **socket_shared 模式**：所有回调入队来自同一线程（mgmt_engine），使用单写接口。
- **socket_single / tcpdirect 模式**：多线程入队，使用多写接口（`_mth` 版本）。

```cpp
FORCE_INLINE int64_t write_get(char *&data, int32_t len) {
  if (0 == single_writer_) return cb_queue_.write_get_mth(data, len);
  else                    return cb_queue_.write_get(data, len);
}
FORCE_INLINE void write_cmt(int64_t pos, int32_t len) {
  if (0 == single_writer_) cb_queue_.write_cmt_mth(pos, len);
  else                    cb_queue_.write_cmt(pos, len);
}
```

## 初始化与生命周期

```cpp
int32_t init(lb_common::lb_log *log, api_callback *cb, callback_mode mode,
             int32_t cpu_affinity, int32_t queue_size_mb, int32_t wait_ms,
             int32_t single_writer);
int32_t start();  // 启动回调线程（仅队列模式）
void stop();      // 停止回调线程
```

## 回调接口（柜台/引擎调用）

| 方法 | 参数 | 说明 |
|------|------|------|
| `on_login` | `ans` | 登录结果回调 |
| `on_order_rtn` | `si, rtn` | 委托回报 |
| `on_trade_rtn` | `si, rtn` | 成交推送 |
| `on_cancel_rsp` | `si, rsp` | 撤单响应（柜台废单） |
| `on_order_query_ans` | `ans_arr, ctl` | 委托查询应答（数组，由柜台分配不释放） |
| `on_trade_query_ans` | `ans_arr, ctl` | 成交查询应答 |
| `on_fund_query_ans` | `info` | 资金查询应答 |
| `on_position_query_ans` | `ans_arr, ctl` | 持仓查询应答 |
| `on_link_status` | `counter_type, link_type, status` | 链接状态变化 |
| `on_error` | `event_type, err_code, err_desc` | 通用错误 |

## 双模式分发逻辑

以 `on_order_rtn` 为例：
- **direct 模式**：直接同步调用 `user_callback_->on_order_rtn(si, rtn)`（在 IO/引擎线程执行）。
- **queued 模式**：
  1. 打包 `cb_event_head{type=order_rtn}` + `StreamInfo` + `OrderRtn` 写入无锁队列（`write_get`/`write_cmt`）。
  2. `trigger()` 唤醒回调线程。
  3. 回调线程 `do_work()` 从队列 `read_get` 读取，按 `head->type` 分发到对应 `user_callback_->on_*`。

## 线程函数

```cpp
void do_work() override;   // 消费回调队列并分发
bool need_work() override; // 是否有待处理工作
```

## 设计要点
1. **统一出口**：所有柜台回报统一收敛到 `callback_manager`，再分发到 `api_callback`。
2. **direct/queued 双模式**：direct 保证最低延迟；queued 用独立线程隔离，避免阻塞 IO 线程。
3. **单写/多写自适应**：根据引擎模式选择单写或多写队列接口，提升性能。
4. 查询应答数组由柜台分配，`callback_manager` 不负责释放，避免所有权混乱。
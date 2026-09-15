# idle_engine 文档

> 源文件：`trunk/NewAPI/gone/api/src/idle_engine.h` + `idle_engine.cpp`
> 作用：空引擎（占位），用于 socket_shared 模式下的极速引擎占位。

## 概述

`idle_engine<TFastCounter>` 是空引擎，仅在 `speed_link_type == socket_shared` 时存在。内部仅持有多 socket 引擎的发送队列指针，**无链接、无线程**。行为（post_send_event / send_queue）全部转发到多 socket 引擎。

## 设计目的

在 `api_impl` 模板实例化极速引擎时作为**占位**。`api_impl` 会调用其接口，但实际运行时不会调用 `get_queue` 和 `get_out_op`（因为 socket_shared 模式下极速业务走 multi 引擎）。

## 接口

| 方法 | 说明 |
|------|------|
| `init(cfg, tfst, log)` | 初始化（签名与其他引擎统一） |
| `add_timer_poll(th)` | no-op，返回 0 |
| `start()` | no-op（资源由 multi 引擎管理） |
| `stop()` | no-op |
| `get_queue()` | 返回 nullptr |
| `get_out_op()` | 返回 nullptr |

## 关键成员

```cpp
TFastCounter *counter_;  // 极速柜台指针 (init 阶段注入)
lb_common::lb_log *log_; // 日志指针
```

## 设计要点
1. **接口统一**：与 `tcpdirect_engine` / `single_socket_engine` 接口完全一致，便于模板选型。
2. **无资源**：无链接、无线程，资源全部由 multi 引擎管理。
3. **占位用途**：socket_shared 模式下，极速业务链接由 multi 引擎承担，本引擎仅作占位。
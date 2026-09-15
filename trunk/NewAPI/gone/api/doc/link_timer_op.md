# link_timer_op 文档

> 源文件：`trunk/NewAPI/gone/api/src/link_timer_op.h` + `link_timer_op.cpp`
> 作用：链接定时事件类，驱动链接的心跳发送、心跳超时检测、重连决策。

## 概述

`link_timer_op<TLink, TEngine>` 是链接定时事件类，由工作引擎持有，知道 link + engine 两个指针。`deal_event` 中调用 link 的检查函数，根据结果向 engine 投递事件。

> 定时器不再由 link 内部持有，link 也不再有自己的 `on_timer()` 方法。

## 模板参数

- `TLink`：链接类型
- `TEngine`：该 link 所属工作引擎类型

## 接口

| 方法 | 说明 |
|------|------|
| `link_timer_op()` | 构造函数，创建 timerfd |
| `~link_timer_op()` | 析构函数，关闭 timerfd |
| `get_fd()` | 获取 timerfd 文件描述符 |
| `get_interval()` | 获取定时器间隔（秒） |
| `init_timer(link, engine, interval)` | 绑定 link + engine + interval（engine 在 init 阶段调用） |
| `add_timer_poll(th)` | 将 timerfd 以自动定时循环方式加入线程 epoll（水平触发） |
| `close()` | 停止定时器 |
| `delive_link_event(event_type, event_data)` | 投递链接事件 |
| `deal_event()` | 定时器触发回调：读 timerfd → 决策 → 调 link 检查函数，投递事件到 engine |

## 定时触发逻辑（deal_event）

```
timerfd 触发 → 读 timerfd
  → 调 link_.check_heart_send()     // 需要发心跳？ → 投递 SEND_HEART 事件
  → 调 link_.check_heart_timeout()  // 心跳超时？ → 投递 LINK_CLOSE 事件
  → 调 link_.check_reconnect(o_need_switch) // 需要重连？ → 投递 LINK_CONNECT 事件
  → 投递事件到 engine
```

## 关键成员

```cpp
int32 timer_fd_;        // timerfd 文件描述符，重复循环定时
int32 interval_;        // 定时循环间隔(秒)
TLink *link_;           // 链接指针
TEngine *engine_;       // link 所属工作引擎指针
```

## 事件回调

| 方法 | 说明 |
|------|------|
| `deal_error()` | 错误回调（timerfd 自身出错几乎不会发生，保留） |
| `deal_close()` | 关闭回调 |

## 设计要点
1. **职责分离**：定时器独立于链接，由引擎统一持有和管理。
2. **双模板**：同时知道 link 和 engine，便于双向交互。
3. **统一驱动**：心跳发送、超时检测、重连决策统一由定时器循环驱动。
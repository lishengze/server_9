# multi_socket_engine 文档

> 源文件：`trunk/NewAPI/gone/api/src/multi_socket_engine.h` + `multi_socket_engine.cpp`
> 作用：多 socket 引擎（模型 c），管理 98 柜台链接和极速柜台网关链接。

## 概述

`multi_socket_engine<TFastCounter>` 是多 socket 引擎，简化为 **2 槽设计**：

| 槽位 | 链接 | 用途 |
|------|------|------|
| 槽 0 | `g98_link_` | 98 柜台链接（恒存在） |
| 槽 1 | `fast_gw_link_` | 极速柜台网关链接（按 TFastCounter 不同语义） |

模板参数 `TFastCounter` 硬编码消息路由（无适配器、无虚基类）。

## 槽位语义（按 TFastCounter）

| TFastCounter | fast_gw_link_ 用途 |
|--------------|-------------------|
| `gw_direct` | ❌ 闲置（个微在 fast_engine） |
| `fpga_direct` | fpga GW 链接（登录/证券信息） |
| `fpga_gateway` | fpga GW 链接（登录/证券信息/业务） |

## 双重角色

1. **多个链接的工作引擎**：98 + fpga_gw/个微/闲置都在本线程。
2. **极速引擎的 timerfd 注册目标**：即控制平面 epoll。

## 内部类 eng_link_op

```cpp
class eng_link_op : public link_engine_outop {
  multi_socket_engine<TFastCounter> *owner_;
  void deal_heart_msg_ans(int16 link_type) override;
  void trigger_send() override;
  void deal_close_link(int16 link_type, int32 err_code) override;
};
```

## 接口

| 方法 | 说明 |
|------|------|
| `init(cfg, tfst, log, c98)` | 初始化（注入极速柜台 + 98 柜台指针） |
| `connect_98agw()` | 建立 98 链接 |
| `start()` | 启动引擎线程（含 epoll 线程） |
| `stop()` | 停止引擎线程 |
| `get_queue()` | 获取发送队列 |
| `get_out_op()` | 获取链接操作接口 |
| `get_thread()` | 获取 epoll 线程 |

## 事件处理

```cpp
void deal_event() override;              // epoll 事件处理
void deal_cust_login(const acc_login_event_info &pmlog);  // 客户登录
void deal_cust98_login(const acc_login_event_info &pmlog); // 98 客户登录
void deal_agw98_login();                  // agw 登录
```

## 关键成员

```cpp
int32 send_poll_num_;               // 发送轮询批量数
int32 recv_poll_num_;               // 接收轮询批量数
que_mth_buf send_queue_;            // 发送队列（柜台类/极速引擎共享）
event_wake queue_wake_;             // 发送队列触发事件
aio_socket_link<counter98> g98_link_;                          // 98 链接
link_timer_op<aio_socket_link<counter98>, multi_socket_engine> g98_link_timer_;
aio_socket_link<TFastCounter> fast_gw_link_;                   // 极速网关链接
link_timer_op<aio_socket_link<TFastCounter>, multi_socket_engine> fast_gw_link_timer_;
TFastCounter *counter_;             // 极速柜台指针
counter98 *counter98_;              // 98 柜台指针
mthread epoll_th_;                  // epoll 线程
eng_link_op link_outop_;
```

## 设计要点
1. **2 槽简化**：98 + 极速网关，按 TFastCounter 硬编码路由。
2. **控制平面**：承担极速引擎 timerfd 的注册目标。
3. **统一收发队列**：柜台类和极速引擎共享发送队列。
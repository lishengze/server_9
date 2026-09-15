# tcpdirect_engine 文档

> 源文件：`trunk/NewAPI/gone/api/src/tcpdirect_engine.h` + `tcpdirect_engine.cpp`
> 作用：TCPDirect 极速引擎（模型 a），仅在 tcpdirect 模式下使用。

## 概述

`tcpdirect_engine<TFastCounter>` 是 TCPDirect 极速引擎，由一个 `tcpdir_link`（stack 自包含）+ 一个 `link_timer_op` + 一个发送队列 + 一条专属线程组成。与 `single_socket_engine` 结构对称，但使用 `tcpdir_link`（基于 Solarflare TCPDirect）。仅在 `speed_link_type == tcpdirect` 模式下使用。

## 内部类 eng_link_op

```cpp
class eng_link_op : public link_engine_outop {
  tcpdirect_engine<TFastCounter> *owner_;
  void deal_heart_msg_ans(int16 link_type) override;
  void trigger_send() override {};    // 死轮询模式，无需触发
  void deal_close_link(int16 link_type, int32 err_code) override;
};
```

## 接口

| 方法 | 说明 |
|------|------|
| `init(cfg, tfst, log)` | 初始化 |
| `add_timer_poll(th)` | 将链接定时器注册到目标线程 epoll |
| `start()` | 启动引擎线程 |
| `stop()` | 停止引擎线程 |
| `get_queue()` | 获取发送队列 |
| `get_out_op()` | 获取链接操作接口 |

## 线程函数

```cpp
void do_work() override;    // 极速引擎主循环（从 send_queue_ 取事件分发到 link/柜台）
bool need_work() override { return true; }  // 恒为 true，极速引擎不休眠
```

## 事件处理

```cpp
void deal_cust_login(const acc_login_event_info &pmlog);   // 客户登录
void deal_fpga_core_connect(const fpga_core_connect_info &pmlog); // fpga core 链接
```

## 关键成员

```cpp
int32 send_poll_num_;                        // 发送轮询批量数
int32 recv_poll_num_;                        // 接收轮询批量数
que_mth_buf send_queue_;                     // 发送队列（柜台/api_impl 共享）
tcpdir_link<TFastCounter> link_;             // 极速交易链接（stack 自包含）
TFastCounter *counter_;                      // 极速柜台指针
lb_common::lb_log *log_;                     // 日志指针
link_timer_op<tcpdir_link<TFastCounter>, tcpdirect_engine> timer_op_; // 链接定时器
eng_link_op link_outop_;
```

## do_work 分发逻辑

```
do_work() 从 send_queue_ 取 link_send_event
  → 先调 link_.deal_recv() 驱动 TCPDirect 接收
  → 然后处理事件:
    → case ACCOUNT_LOGIN: deal_cust_login
    → case FPGA_CORE_CONNECT: deal_fpga_core_connect
    → case SEND_MSG: link_.send_msg(evt->data, evt->data_len)
    → case SEND_HEART / LINK_CLOSE / LINK_CONNECT: 对应链接操作
```

## 设计要点
1. **Solarflare TCPDirect**：使用专用网卡硬件实现极低延迟网络通信。
2. **单链接专属线程**：一个极速交易链接 + 一条专属线程。
3. **死轮询**：`need_work()` 恒为 true，引擎不休眠，保证最低延迟。
4. **结构对称**：与 single_socket_engine 结构对称，仅 IO 模型不同。
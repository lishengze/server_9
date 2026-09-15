# tcpdir_link 文档

> 源文件：`trunk/NewAPI/gone/api/src/tcpdir_link.h` + `tcpdir_link.cpp`
> 作用：基于 tcpdir_ch 的 Solarflare TCPDirect 链接（组合模式，薄包装）。

## 概述

`tcpdir_link<TCounter>` 是基于 Solarflare TCPDirect 技术的链接包装。与 `aio_socket_link` 结构对称，但 IO 模型不同：不使用 mthread epoll（Solarflare 自带 zf_mux_wait，由 tcpdir_poll 驱动），单一地址（无备地址），自带 `tcpdir_stack`。

## 与 aio_socket_link 关键差异

| 特性 | aio_socket_link | tcpdir_link |
|------|----------------|-------------|
| IO 模型 | aio_tcp + mthread epoll | Solarflare TCPDirect (zf_mux_wait) |
| 地址 | 主/备双地址 | 单一地址 |
| 驱动方式 | epoll 事件驱动 | 引擎线程循环调 `ch_.loop_deal_recv()` |
| stack | 无 | 自带 `tcpdir_stack` |
| 适用场景 | socket_single | tcpdirect |

## 内部类 msg_cb

```cpp
class msg_cb : public lb_common::tcpdir_msg_cb {
  tcpdir_link<TCounter> *owner_;
  int32 deal_msg(tcpdir_ch *pch, char *pbuf, int32 len) override;
};
```
tcpdir_ch 收到完整消息时回调，转发到 `counter.deal_recv_msg`。

## 接口

| 方法 | 说明 |
|------|------|
| `init(tlog, tfst, solarflare_iface, link_type, once_recv_len, check_interval, max_same_addr_fails)` | 链接初始化（含 stack init） |
| `set_remote(remote)` | 设置地址（单一地址） |
| `connect(recv_pool_num, need_switch, recv_th)` | 重连/连接（need_switch 仅占位，不支持切换） |
| `close_ch(err)` | 关闭链接 |
| `send_msg(buf, len)` | 发送数据（转发到 ch_.send_msg_fc） |
| `deal_recv()` | 处理接收（引擎 do_work 循环调用） |
| `get_link_type()` | 获取链接类型 |
| `is_work() / is_close() / is_free()` | 链接状态查询 |
| `get_heart_interval()` | 获取心跳间隔 |
| `deal_heart_ans()` | 处理心跳应答 |
| `check_heart_send()` | 检查是否需要发送心跳 |
| `check_heart_timeout()` | 检查心跳是否超时 |
| `check_reconnect(o_need_switch)` | 检查是否需要重连（o_need_switch 恒为 0） |

## 通道事件回调（tcpdir_ch_op 虚函数）

| 方法 | 触发时机 |
|------|---------|
| `deal_ch_connect` | 通道连接成功 |
| `deal_ch_closing` | 通道正在关闭 |
| `deal_ch_closed` | 通道已关闭 |
| `deal_ch_error` | 通道错误 |

## 关键成员

```cpp
tcpdir_ch ch_;                    // 底层 TCPDirect 通道 (owned)
tcpdir_stack stack_;              // TCPDirect stack (owned, 自包含)
msg_cb msg_cb_;                   // 消息接收回调
int16 link_type_;                 // 链接类型 (LINK_TYPE_*)
uint16 check_interval_;           // 心跳和重连间隔(秒)
int32 once_recv_len_;             // 一次 recv 的输入长度
TCounter *counter_;               // 所属柜台指针
reconnect_ctl reconn_;            // 重连控制器
csock_addr addrs_;                // 单一地址
lb_common::lb_log *log_;          // 日志指针
```

## 设计要点
1. Solarflare TCPDirect 技术实现极低延迟的网络通信，适用于 HFT 场景。
2. 组合而非继承（tcpdir_ch 不可继承），自身作为 ch 事件回调。
3. 引擎线程通过循环调 `ch_.loop_deal_recv()` 驱动接收，无 epoll 中断开销。
4. 单一地址，不支持主备切换。
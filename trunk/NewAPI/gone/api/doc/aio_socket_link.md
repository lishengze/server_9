# aio_socket_link 文档

> 源文件：`trunk/NewAPI/gone/api/src/aio_socket_link.h` + `aio_socket_link.cpp`
> 作用：基于 aio_tcp 的非阻塞 TCP 链接（组合模式，薄包装）。

## 概述

`aio_socket_link<TCounter>` 是 API 的网络链接层，将底层 `aio_tcp` 非阻塞 TCP 通道包装成柜台可用的链接对象。采用**组合而非继承**（`aio_tcp` 是 final 类），自身作为 `aio_ch_op<tcp_buf_ch>` 通道事件回调，并持有内嵌 `msg_cb` 作为消息回调。

## 设计原则

1. **组合而非继承**：`aio_tcp` 是 final，不能继承，故持有 `ch_`（aio_tcp 实例）组合使用。
2. **自身是 `aio_ch_op<tcp_buf_ch>`**：作为通道事件回调（错误/关闭/连接等）。
3. **持有内嵌 `msg_cb`**：作为消息回调，收到完整消息时转发给柜台。
4. **链接生命周期**：`init → set_remote → start_recv(connect+start) → ... → close_ch`。

## 数据流

```
mthread epoll → aio_tcp::deal_event → loop_deal_recv
  → msg_cb::deal_msg(this, aio_msg)
  → counter.deal_recv_msg(msg.pmsg, msg.msglen, link_type)
```

## 地址管理

- `addrs_[0]` = primary，`addrs_[1]` = secondary
- fast 单 socket / shared 只使用 `[0]`；98 / fpga_gw 使用 `[0]+[1]`
- 故障切换通过 `check_reconnect(bool &o_need_switch)` 决策，内部切换 `active_idx_`

## 类结构

### 内部类 msg_cb
```cpp
class msg_cb : public lb_common::ch_recv_cb<aio_socket_ch_t> {
  aio_socket_link<TCounter> *owner_;   // 外层 link 指针
  int32 deal_msg(aio_socket_ch_t *pch, lb_common::aio_msg &msg) override;
};
```
aio_tcp 收到完整消息时回调，转发到 `counter.deal_recv_msg`。

### 主要方法

| 方法 | 说明 |
|------|------|
| `init(tlog, tfst, link_type, once_recv_len, check_interval, max_same_addr_fails)` | 链接初始化，注入日志、柜台指针、链接类型等 |
| `set_remote(remote)` | 设置主地址（最多调用两次，传不同地址），必须在 start_recv 前调用 |
| `connect(recv_pool_num, need_switch, recv_th)` | 重连/连接，need_switch=1 切换地址 |
| `close_ch(err)` | 关闭链接 |
| `send_msg(buf, len)` | 发送数据（转发到 `ch_.send_msg_fc`） |
| `deal_recv()` | 手动驱动接收（无 mthread 时用） |
| `get_link_type()` | 获取链接类型 |
| `is_work() / is_close() / is_free()` | 链接状态查询 |
| `get_heart_interval()` | 获取心跳间隔 |
| `deal_heart_ans()` | 处理心跳应答 |
| `check_heart_send()` | 检查是否需要发送心跳 |
| `check_heart_timeout()` | 检查心跳是否超时 |
| `check_reconnect(o_need_switch)` | 检查是否需要重连，输出是否切换地址 |

### 模板参数
- `TCounter`：柜台类型，通过 `counter_.deal_recv_msg` 接收数据。

## 通道事件回调（aio_ch_op 虚函数）

| 方法 | 触发时机 |
|------|---------|
| `deal_ch_error` | 通道错误 |
| `deal_ch_closing` | 通道正在关闭 |
| `deal_recv_stop` | 接收停止 |
| `deal_ch_closed` | 通道已关闭 |
| `deal_ch_connect` | 通道连接成功 |

## 关键成员

```cpp
aio_socket_ch_t ch_;                  // 底层非阻塞 TCP 通道 (owned)
msg_cb msg_cb_;                       // 消息接收回调
TCounter *counter_;                   // 所属柜台指针
int16 link_type_;                     // 链接类型 (LINK_TYPE_*)
lb_common::reconnect_ctl reconn_;     // 重连控制器
int32 check_interval_;                // 心跳和重连间隔(秒)
int16 active_idx_;                    // 0=primary, 1=secondary
lb_common::csock_addr addrs_[2];      // [0]=primary, [1]=secondary
```

## 设计要点
1. 链接类型通过 `link_type_` 标识（LINK_TYPE_98 / SPEED_TRADE / SPEED_GW），与 `api_event_msg.h` 中的常量对应。
2. 心跳与重连统一由 `link_timer_op` 通过 `check_*` 系列方法驱动。
3. 多地址支持主备切换，提高可用性。
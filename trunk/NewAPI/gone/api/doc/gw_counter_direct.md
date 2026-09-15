# gw_counter_direct 文档

> 源文件：`trunk/NewAPI/gone/api/src/gw_counter_direct.h` + `gw_counter_direct.cpp`
> 作用：个微软件极速柜台（直连模式）协议对象。
> 当前状态：**FTE TCP Binary 协议已完整实现**（登录/委托/撤单/回报/心跳/ETF），并完成性能优化（Task 8）。

## 概述

`gw_counter_direct` 负责个微软件极速柜台（仅直连模式）单 TCP 链接的消息组包/解析。**个微柜台无查询接口**，与 fpga_counter_gateway 一致（无 deal_order_query 等）。

## 协议状态（已实现）

- **协议**：FTE TCP Binary（`gw_message::*` 结构体，大端字节序）。
- **报文格式**：`[PktNewHeader 8B | 消息体 | 校验和 4B]`。
- **消息头**：`PktNewHeader`（msg_id + msg_len，`ByteSwap32` 大端）。
- **校验和**：`GenerateSzCheckSum` 对 [头+体] 逐字节求和 `%256`，`ByteSwap32` 后追加。
- **消息类型**：
  - 上行：`1001` 登录 / `1003` 委托 / `1004` 撤单 / `1010` ETF / `3` 心跳
  - 下行：`2001` 登录应答 / `2003` 委托回报 / `2004` 撤单应答 / `2005` 成交回报 / `2010` ETF 成交回报 / `9` 拒绝
- **字节序注意**：FTE 使用**主机字节序**（`HostToNetwork` 为 no-op），但消息头 `PktNewHeader` 用 `ByteSwap32` 显式大端交换。

## 链接与队列

```cpp
void init_trade(que_mth_buf *que, link_engine_outop *link_outop);  // 极速业务发送队列
void init_gateway(que_mth_buf *que, link_engine_outop *heart_op) {} // 个微无网关链接，空实现
```

## 业务接口（api instance 调用）

| 方法 | 说明 |
|------|------|
| `deal_order_req(req)` | 处理委托，若离线返回错误让上层路由到 98 |
| `deal_etf_order_req(req)` | 处理 ETF，不支持则路由到 98 |
| `deal_cancel_req(req)` | 处理撤单 |

## 引擎/链接接口

| 方法 | 说明 |
|------|------|
| `deal_recv_msg(buf, len, link_type)` | 接收消息处理（个微必为 LINK_TYPE_SPEED_TRADE），按 msg_id 分发 |
| `deal_send_error(...)` | 处理发送失败 |
| `deal_cust_login(req, o_buf, buf_len)` | 处理账户登录事件（构造 LogOnReq） |
| `ans_cust_login(req, err_ret, err_msg)` | 引擎同步阶段失败回调 |
| `build_heart_msg(o_buf, buf_len)` | 构造心跳消息（msg_id=3） |
| `can_link_connect(link_type)` | SPEED_TRADE 可建链 |
| `deal_link_connect(link_type, have_switch)` | 链接成功回调（个微不支持地址切换，忽略 have_switch） |
| `deal_link_close(link_type)` | 链接关闭回调（重置链接态与登录态） |

## 消息构建（FTE 协议，已实现）

| 方法 | 说明 |
|------|------|
| `build_order_msg(req, o_buf)` | 构造委托消息（PktNewHeader + TradeOrderReq + 校验和） |
| `build_etf_order_msg(req, o_buf)` | 构造 ETF 委托消息（msg_id=1010，体同 TradeOrderReq） |
| `build_cancel_msg(req, o_buf)` | 构造撤单消息（PktNewHeader + CancelOrderReq + 校验和） |
| `build_login_msg(info, o_buf, buf_len)` | 构造账户登录消息（PktNewHeader + LogOnReq + 校验和） |
| `build_login_rtn(...)` | 构造登录应答（多个重载） |

### 性能优化（Task 8，方案 B/C/D）

三个 build 函数采用**直接序列化 + 单趟校验和**：
- 直接在 `o_buf` 按字段顺序序列化，**无中间 `body` 对象**（方案 D）。
- 边写边累加校验和，**消除第二次 O(n) 遍历**（方案 C）。
- 字符串字段用 `cksum_copy_pad()` 一次遍历完成「拷贝 + 空格填充 + 校验和累加」，替代原 `memcpy + space_pad` 双重遍历（方案 B）。
- 辅助函数：`cksum_put` / `cksum_write` / `cksum_copy_pad` / `cksum_be32`（消息头）/ `cksum_net64/32/16`（body 字段，`HostToNetwork` no-op）/ `cksum_finish`。
- 字段顺序与 `TradeOrderReq::encode` / `CancelOrderReq::encode` 严格一致，回归测试 4/4 通过。

## 应答解析（FTE 协议）

| 方法 | 说明 |
|------|------|
| `deal_log_ans(body, body_len)` | 处理登录应答（LogOnAns），成功置 login_state=2 |
| `deal_order_rtn(body, body_len)` | 处理委托回报（exec_type='0'/'8'） |
| `deal_trade_rtn(body, body_len)` | 处理成交回报（exec_type='F'） |
| `deal_cancel_rsp(body, body_len)` | 处理撤单回报（exec_type='4'） |
| `deal_etf_trade_rtn(body, body_len)` | 处理 ETF 成交回报（TradeOrderER + ConstituentStock[]） |
| `deal_reject_msg(body, body_len)` | 处理拒绝消息（RejectMsg） |
| `build_api_order_rej(req, err_code, o_rtn, o_stream)` | 构造委托拒单 |
| `build_api_cancel_rej(req, err_code, o_rtn, o_stream)` | 构造撤单拒单 |

## 状态字典映射

| 方法 | 说明 |
|------|------|
| `map_ord_status(fte_status)` | FTE ord_status(0-8) → NewAPI ORDER_STATE_*(0-9)。例：(2)→DONE_PART(3)，(3)→DONE_FULL(4) |
| `map_exec_type(exec_type)` | FTE exec_type('0'/'4'/'8'/'F') → RSP_TYPE_*。例：('F')→ORDER_TRADE(3)，('0')→COUNTER_RSP(1) |
| `map_market_id(fte_market_id)` | FTE 市场ID → NewAPI 市场ID |

## 队列操作

```cpp
// 申请队列内存 (头 + take_len 字节 payload), 失败时 o_buf=NULL, 返回 <0
int64 take_req_que_mem(char *&o_buf, int32 take_len) {
  int64 pos = trade_send_queue_->write_get_mth(data, take_len + sizeof(link_send_event));
  link_send_event *evt = (link_send_event*)data;
  evt->link_type = LINK_TYPE_SPEED_TRADE;
  evt->type = LINK_EVENT_TYPE_SEND_MSG;
  evt->data_len = take_len;
  return pos;
}
// 提交队列并触发发送（注释掉 trigger_send，因 fast_engine 是死轮询模式）
void cmt_req_que_mem(int64 get_pos, int32 take_len) {
  trade_send_queue_->write_cmt_mth(get_pos, take_len + sizeof(link_send_event));
  //trade_eng_op_->trigger_send();
}
```

## GwSessionCache（会话缓存）

全局单例（`gw_session_cache.h`），以 `fund_account_id` 为主键：
- 登录时 `create_session()` 缓存会话字段（account_id/cust_id 等）。
- 委托/撤单时 `get_session()` 补充 body 中的 account_id/cust_id。
- 维护 `order_sys_no ↔ {clordno, client_seq_id}` 映射，供撤单反查原单。
- 撤单定位：`get_clordno()` / `get_orig_client_seq_id()`。

## 关键成员

```cpp
int16 trade_link_connect_ = 0;   // 极速链接状态（跨线程，atomic_load16/store16 访问）
int16 login_state = 0;           // 用户登陆状态: 0-未登陆, 1-登陆中, 2-登陆成功（atomic 访问）
int16 market_type = 0;           // 市场
int16 heart_interval = 5;        // 心跳间隔
que_mth_buf *trade_send_queue_;  // 极速柜台发送队列
callback_manager *cb_mgr_;       // 回调管理器
int64 session_seq_ = 0;          // 会话消息序号（atomic_fetch_add64/load64 访问）
lb_log *log_;                    // 日志
link_engine_outop *trade_eng_op_;// 极速引擎导出的链接相关操作
```

**原子化（方案 F）**：跨线程状态变量统一用 matomic.h 的原子操作访问，消除数据竞争：
- `trade_link_connect_` / `login_state`：`atomic_load16` / `atomic_store16`
- `session_seq_`：`atomic_fetch_add64(&session_seq_, 1) + 1`（保持 `++` 语义）

## 日志策略（方案 E）

高频日志已降级为 `debug_log`（生产可关闭）：
- `deal_recv_msg` 每笔回报日志（含 hexbuf 构造）
- `deal_trade_rtn` 入口日志

## 待完成任务

1. **断线重登**：链接断开后 login_state 重置已实现，但断线自动重登逻辑待完善。
2. **登录异常重试**：登录失败后的重试机制未实现。
3. **deal_send_error**：发送失败处理逻辑待完善。

## 设计要点

1. **FTE TCP Binary 协议**：消息头 `PktNewHeader` 大端，body 用主机字节序，校验和对 [头+体] 求和 %256。
2. **无网关链接**：`init_gateway` 空实现，个微只有单条极速业务链接。
3. **委托降级**：离线/不支持时返回错误码，上层路由到 98。
4. **会话缓存**：GwSessionCache 全局单例，撤单通过 order_sys_no 反查原单。
5. **性能优化**：直接序列化 + 单趟校验和 + 原子化状态 + 日志降噪。
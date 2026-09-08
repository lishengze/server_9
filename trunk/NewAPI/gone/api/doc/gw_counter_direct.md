# gw_counter_direct 文档

> 源文件：`trunk/NewAPI/gone/api/src/gw_counter_direct.h` + `gw_counter_direct.cpp`
> 作用：个微软件极速柜台（直连模式）协议对象。

## 概述

`gw_counter_direct` 负责个微软件极速柜台（仅直连模式）单 TCP 链接的消息组包/解析。**个微柜台无查询接口**，与 fpga_counter_gateway 一致（无 deal_order_query 等）。

## 协议状态

- 个微真实协议是外部定义，当前未提供。
- **临时替代方案**：消息头使用 `g1_msg_head`，登录消息体使用 `login_req/login_ans`。
- 委托/撤单等业务消息体无法替代，暂留空实现 + `// todo` 标注。

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
| `deal_recv_msg(buf, len, link_type)` | 接收消息处理（个微必为 LINK_TYPE_SPEED_TRADE） |
| `deal_send_error(...)` | 处理发送失败 |
| `deal_cust_login(req, o_buf, buf_len)` | 处理账户登录事件 |
| `ans_cust_login(req, err_ret, err_msg)` | 引擎同步阶段失败回调 |
| `build_heart_msg(o_buf, buf_len)` | 构造心跳消息 |
| `can_link_connect(link_type)` | SPEED_TRADE 可建链 |
| `deal_link_connect(link_type, have_switch)` | 链接成功回调（个微不支持地址切换，忽略 have_switch） |
| `deal_link_close(link_type)` | 链接关闭回调 |

## 消息构建

| 方法 | 状态 |
|------|------|
| `build_order_msg` | **留空**（个微真实协议未知） |
| `build_etf_order_msg` | **留空** |
| `build_cancel_msg` | **留空** |
| `build_login_msg(info, log_type, o_req)` | 临时用 g1 login_req 替代 |
| `build_login_rtn(...)` | 构造登录应答（多个重载） |

## 应答解析

| 方法 | 状态 |
|------|------|
| `deal_log_ans(msg)` | 临时用 g1 login_ans 替代 |
| `build_api_order_rej(msg, err_code, o_rtn, o_stream)` | 构造委托拒单（复用 fpga 逻辑，临时用 g1 头） |
| `build_api_cancel_rej(msg, err_code, o_rtn, o_stream)` | 构造撤单拒单 |

## 队列操作

```cpp
// 申请队列内存 (头 + take_len 字节 payload)
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

## 关键成员

```cpp
int16 trade_link_connect_ = 0;   // 极速链接状态
int16 login_state = 0;           // 用户登陆状态: 0-未登陆, 1-登陆中, 2-登陆成功
int16 market_type = 0;           // 市场
int16 heart_interval = 5;        // 心跳间隔
que_mth_buf *trade_send_queue_;  // 极速柜台发送队列
callback_manager *cb_mgr_;       // 回调管理器
// todo : 定义个微柜台缓存结构，添加缓存对象
```

## 待完成任务
1. **build_order_msg / build_etf_order_msg / build_cancel_msg 留空**：需依据正式个微协议实现。
2. **deal_recv_msg 只分发登录/心跳**：委托/成交/撤单回报全部 default 跳过。
3. **build_api_order_rej / build_api_cancel_rej 留空**：需解析原始消息。
4. **缓存结构未定义**：`gw_counter_direct.h:173` 有 `//todo`。
5. **链接状态管理**：链接建立后是否重登、断开后 login_state 重置待完善。

## 设计要点
1. **临时用 g1 协议替代**：登录/心跳可工作，业务消息体待正式协议。
2. **无网关链接**：`init_gateway` 空实现，个微只有单条极速业务链接。
3. **委托降级**：离线/不支持时返回错误码，上层路由到 98。
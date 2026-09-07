# 问题分析：fast_ 初始化/链接/登录 与 回报回调链路

> 分析对象：`trunk/NewAPI/gone/api/`
> 关联源码：`api_instance.h/.cpp`、`single_socket_engine.cpp`、`multi_socket_engine.cpp`、
> `counter98.cpp`、`fpga_counter_base.cpp`、`fpga_counter_direct.cpp`、`gw_counter_direct.cpp`、`callback_manager.cpp`

---

## 问题一：api_impl 中的 `TFastCounter fast_` 如何初始化、与柜台链接、登录？

### 1.1 初始化（`api_impl::init`，api_instance.cpp:134）

`fast_` 是模板成员，类型由 5 种配置决定（`gw_counter_direct` / `fpga_counter_direct` / `fpga_counter_gateway`）。
初始化分三个阶段：

```cpp
// ① 柜台对象自身初始化：设置 cb_mgr_、log_，拷贝配置
ret = fast_.init(cfg, &cb_mgr_, &log_);

// ② 极速引擎初始化：引擎内部持有 counter_ 指针（TFastCounter*）
ret = fast_engine_.init(cfg, &fast_, &log_);

// ③ 注入"柜台 → 引擎"的双向引用（关键步骤）
if (fpga_gateway) {
    fast_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
} else {
    fast_.init_trade(fast_engine_.get_queue(), fast_engine_.get_out_op()); // 业务发送队列
}
fast_.init_gateway(multi_engine_.get_queue(), multi_engine_.get_out_op()); // GW 发送队列
```

**关键点**：`init_trade` / `init_gateway` 把引擎的**无锁发送队列**（`que_mth_buf*`）和**链接操作接口**（`link_engine_outop*`）注入柜台。此后柜台组包后通过 `take_req_que_mem` + `cmt_req_que_mem` 入队，引擎线程消费队列并实际发送。柜台**不直接碰 socket**，所有网络操作都经引擎。

- `fast_engine_`（single_socket_engine / tcpdirect_engine）：极速**业务**链接（直连核心，socket_single 模式）
- `multi_engine_`（multi_socket_engine）：控制平面，管理 98 链接（槽0）+ 极速 GW 链接（槽1）

### 1.2 与柜台链接（`api_impl::start`，api_instance.cpp:193）

极速链接是**按需建立（lazy）**的，start 阶段只做线程与定时器准备：

```cpp
// ② 把极速链接的心跳/重连 timerfd 注册到 multi 引擎的 epoll 线程（控制平面）
fast_engine_.add_timer_poll(multi_engine_.get_thread());

// ③ 98 链接同步建链（仅建链）
multi_engine_.connect_98agw();

// ④ multi 引擎异步启动（接管 98 + 极速GW 的 IO）
multi_engine_.start();

// ⑤ fast 引擎异步启动（接管极速业务链接 IO）
fast_engine_.start();

// ⑥ 同步 agw 登录（必须最后，此时 multi 线程已运行可收应答）
c98_.deal_agw_login();
```

真正的极速链接在**账户登录时**建立，分柜台类型：

| 柜台类型 | 链接建立时机与方式 |
|---------|-------------------|
| **gw_direct（个微）** | `single_socket_engine::deal_cust_login`：若 link 空闲则 `link_.connect()` 直连核心（不支持切换）|
| **fpga_direct（FPGA直连）** | 先建 GW 链接（`multi_engine` 槽1）→ GW 登录应答回填 trade_ip/port → `deal_log_ans` → `delive_fpga_connect` 触发 **FPGA core 链接** |
| **fpga_gateway（FPGA网关）** | 只走 GW 链接（`LINK_TYPE_SPEED_GW`），业务也走此链接，无独立 core 链接 |

### 1.3 登录流程（完整链路）

登录请求由 **98 柜台统一入口** 发起，再级联触发极速柜台登录：

```
用户 login() → api_impl::login → c98_.deal_login_req(req)
  └─ ① 校验 agw 已登录（agw_login_state==2）
  └─ ② LoginReq → acc_login_event_info（build_cust_login_event）
  └─ ③ 投 ACCOUNT_LOGIN 事件到 multi 引擎队列（LINK_TYPE_98）
        multi_engine::deal_event → LINK_EVENT_TYPE_ACCOUNT_LOGIN
          ├─ link_type==98  → deal_cust98_login
          │    → c98_.deal_cust_login()：需登录则构建98登录消息发送；否则 delive_fast_counter_login()
          │    → 98 登录应答 deal_cust_login_ans → 成功后 delive_fast_counter_login(info)
          └─ link_type!=98  → deal_cust_login（极速柜台登录）
                → 建立极速链接（见 1.2）
                → counter_->deal_cust_login() 构建极速登录消息并发送
```

极速柜台登录应答回来后：

- **gw_direct**：`deal_recv_msg → deal_log_ans` → 成功 `login_state=2` → `cb_mgr_->on_login(ans)`
- **fpga_direct**：`deal_recv_msg → G1_MSG_LOGIN_ANS → deal_log_ans` → 保存 trade_ip/port → `delive_fpga_connect` 建 core 链接 → 证券信息获取完成后 `cb_mgr_->on_login`
- **fpga_gateway**：`deal_recv_msg → deal_log_ans` → 直接 `cb_mgr_->on_login`（无 core 同步步骤）

**登录回调统一出口**：三种柜台最终都通过 `cb_mgr_->on_login(ans)` 通知客户。

---

## 问题二：不同柜台处理器收到回报后，如何一步步回调给客户？

回报回调分**两层**：链路层（网络→柜台）→ 柜台层（解析→callback_manager→客户）。

### 2.1 链路层：网络数据 → 柜台 `deal_recv_msg`

```
网络收包 → aio_tcp / tcpdir_ch 异步 IO
  → link 的 msg_cb（ch_recv_cb / tcpdir_msg_cb）
  → counter_.deal_recv_msg(msg.pmsg, msg.msglen, link_type)
```

- `aio_socket_link.cpp:30`：`return owner_->counter_->deal_recv_msg(msg.pmsg, msg.msglen, owner_->link_type_);`
- `tcpdir_link.cpp:33`：`return owner_->counter_->deal_recv_msg(pbuf, len, owner_->link_type_);`

`link_type` 标明来源链接（LINK_TYPE_98 / LINK_TYPE_SPEED_GW / LINK_TYPE_SPEED_TRADE）。

### 2.2 柜台层：`deal_recv_msg` 解析并按消息类型分发

各柜台 `deal_recv_msg` 用 while 循环按消息头**逐个解析多个完整消息**（兼容粘包/半包），
按 `msg_id` 分发到具体处理函数：

| 柜台 | 消息头 | 分发示例 |
|------|--------|---------|
| **counter98** | `c98_msg_head_tmp` | AGW_LOGIN_ANS→`deal_agwuser_login_ans`；ACC_LOGIN_ANS→`deal_cust_login_ans`；HEART_ANS→心跳 |
| **fpga_counter_direct** | `g1_msg_head` | ORDER_RTN→`deal_order_rtn`；TRADE_RTN→`deal_trade_rtn`；CANCEL_RSP→`deal_cancel_rsp`；LOGIN_ANS→`deal_log_ans`；OFFLINE_PUSH→`deal_fpag_state`；GW_REJ→`deal_gw_rej` |
| **fpga_counter_gateway** | `g1_msg_head` | 同上，但按客户哈希（`clients_.find`）找到对应 `fpga_cust_info` 再处理 |
| **gw_counter_direct** | `g1_msg_head`（临时） | LOGIN_ANS→`deal_log_ans` 等 |

> 未知消息默认跳过（不返回错误、不关链接），保证旧 API 对新增消息的兼容性。

### 2.3 回调分发：`cb_mgr_` → 客户

以 FPGA 委托回报为例（`fpga_counter_base.cpp:175` `deal_order_rtn`）：

```
deal_order_rtn(head, cust, counter_type)
  → 校验长度、按 g1 order_rtn 解析
  → 构造 API 层 OrderRtn + StreamInfo（含 counter_type, stream_seq 等）
  → cb_mgr_->on_order_rtn(si_out, out)          // fpga_counter_base.cpp:230
```

`callback_manager::on_order_rtn`（callback_manager.cpp:98）按模式分发：

- **direct 模式**：直接同步调用 `user_callback_->on_order_rtn(si, rtn)`（在 IO/引擎线程中执行）
- **queued 模式**：
  1. 把 `cb_event_head{type=order_rtn, data_len}` + `StreamInfo` + `OrderRtn` 打包写入**无锁回调队列**（`write_get`/`write_cmt`）
  2. `trigger()` 唤醒回调线程
  3. 回调线程 `do_work()`（callback_manager.cpp:360）从队列 `read_get` 读取，按 `head->type` 分发：
     `case order_rtn: user_callback_->on_order_rtn(si, rtn)`（callback_manager.cpp:373-378）

### 2.4 回报回调统一出口（三种柜台对比）

| 回报类型 | 柜台处理函数 | callback_manager 接口 | 客户回调 |
|---------|-------------|----------------------|---------|
| 委托回报 | `deal_order_rtn` | `on_order_rtn(si, rtn)` | `on_order_rtn` |
| 成交回报 | `deal_trade_rtn` | `on_trade_rtn(si, rtn)` | `on_trade_rtn` |
| 撤单回报 | `deal_cancel_rsp` | `on_cancel_rsp(si, rsp)` | `on_cancel_rsp` |
| 登录应答 | `deal_log_ans` / `deal_cust_login_ans` | `on_login(ans)` | `on_login` |
| 链接状态 | `deal_link_connect/close` | `on_link_status(...)` | `on_link_status` |
| 错误/离线 | `deal_fpag_state` 等 | `on_error(...)` | `on_error` |

**三种柜台在回报链路上的差异**：
- **counter98**：单客户、无缓存，`deal_recv_msg` 直接分发处理（查询应答也在此）
- **fpga_counter_direct**：单客户（`client_info_`），core 链接回报直接处理
- **fpga_counter_gateway**：多客户，`deal_recv_msg` 先用客户哈希（`clients_.find`）定位对应 `fpga_cust_info`，再调用**基类** `deal_order_rtn/trade_rtn/cancel_rsp`（基类公共解析逻辑复用）
- **gw_counter_direct**：单客户，临时用 g1 协议分发（真实协议待实现）

**完整链路示例（FPGA 直连委托回报）**：
```
网络 → aio_tcp 异步收包
  → aio_socket_link::msg_cb::deal_msg
  → fpga_counter_direct::deal_recv_msg(buf, len, LINK_TYPE_SPEED_TRADE)
  → case G1_MSG_ORDER_RTN → deal_order_rtn(head, client_info_, fpga_direct)
  → 解析 g1 order_rtn → 构造 OrderRtn + StreamInfo
  → cb_mgr_->on_order_rtn(si, rtn)
  → [direct] user_callback_->on_order_rtn(si, rtn)
    或 [queued] 入队 → 回调线程 do_work → user_callback_->on_order_rtn(si, rtn)
```

---

## 附：核心设计要点

1. **柜台与引擎解耦**：柜台懂协议、无线程；引擎懂传输、带线程。通过无锁队列（`que_mth_buf`）跨线程通信。
2. **统一回调出口**：所有柜台回报最终收敛到 `callback_manager` 的 `on_*` 接口，对客户暴露单一 `api_callback` 虚接口。
3. **direct / queued 双模式**：direct 保证最低延迟（IO 线程直接调）；queued 用独立回调线程 + 无锁队列隔离，避免阻塞 IO 线程。
4. **登录级联**：98 柜台统一入口，成功后把 `acc_login_event_info` 级联投递给极速柜台，实现"配置化选择登录柜台"。
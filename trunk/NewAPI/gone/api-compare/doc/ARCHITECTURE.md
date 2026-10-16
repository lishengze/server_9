# API 架构设计文档

> **基线依据**：[api/src/](../src/) 当前代码实现
> **配套文档**：[REQUIREMENTS.md](REQUIREMENTS.md)（业务需求，登录体系权威）｜ [LOGIN_COMPLIANCE.md](LOGIN_COMPLIANCE.md)（登录需求实现状态矩阵）

---

## 1. 架构总览

### 1.1 三层架构
```
┌─────────────────────────────────────────────────────────────────┐
│                  用户 API 接口层 (Public API)                     │
│   api_interface (虚基类) + api_impl<TF, TE> (5 个模板实例化)     │
└──────────────────────────┬──────────────────────────────────────┘
                           │ (组合)
┌──────────────────────────┴──────────────────────────────────────┐
│                  引擎层 (Engine Layer)                            │
│  ┌──────────────────────┐  ┌────────────────────────────┐       │
│  │ multi_socket_engine<TF> │  │ fast_engine (单socket/tcpdirect)│   │
│  │ (2 槽链接, epoll 线程) │  │ (1 槽链接, 业务 epoll 线程)  │       │
│  │ slot 0: g98_link_       │  │ link_: 极速业务链接           │       │
│  │ slot 1: fast_gw_link_   │  │                              │       │
│  └──────────────────────┘  └────────────────────────────┘       │
└──────────────────────────┬──────────────────────────────────────┘
                           │ (回调链接)
┌──────────────────────────┴──────────────────────────────────────┐
│                  柜台层 (Counter Layer)                          │
│  ┌─────────┐  ┌──────────────┐  ┌────────────┐  ┌──────────┐   │
│  │counter98 │  │fpga_counter_  │  │fpga_counter_│  │gw_counter_│   │
│  │ (非模板) │  │ direct       │  │ gateway    │  │ direct    │   │
│  │ (98 协议)│  │ (g1 协议)    │  │ (g1 协议)   │  │ (个微协议) │   │
│  └─────────┘  └──────────────┘  └────────────┘  └──────────┘   │
│   can_link_connect()      全部实现:                               │
│   deal_link_connect(lt, hs)                                      │
└──────────────────────────┬──────────────────────────────────────┘
                           │ (组合)
┌──────────────────────────┴──────────────────────────────────────┐
│                  链接层 (Link Layer)                             │
│  ┌────────────────────┐  ┌────────────────────┐               │
│  │ aio_socket_link<T>  │  │ tcpdir_link<T>     │               │
│  │ (aio_tcp, 内核 socket,│  │ (Solarflare TCPDirect,│           │
│  │  主+备地址)         │  │  单地址)           │               │
│  │ 跟踪 have_switch    │  │ have_switch=0      │               │
│  └────────────────────┘  └────────────────────┘               │
│  ┌─────────────────────────────────────────────┐               │
│  │ link_timer_op<TLink, TEngine> (timerfd 驱动) │               │
│  └─────────────────────────────────────────────┘               │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────┴──────────────────────────────────────┐
│                  回调层 (Callback Layer)                         │
│  callback_manager (direct 同步直调 / queued 队列+线程)              │
└─────────────────────────────────────────────────────────────────┘
```

**链接接口约定**：
- `can_link_connect(int16 link_type) -> bool`：引擎在重连事件分发时调用，返回该类型链接当前是否可发起 connect
- `deal_link_connect(int16 link_type, int32 have_switch) -> int32`：链接 connect 成功后由 link 调用，柜台可在此处做切换后处理（如 98 重新发起 agw 登录）
- `have_switch` 语义：仅当 `link_timer_op` 投出 `need_switch=1` 且 link 实际切到备地址时为 1（见 §7.2）

### 1.2 模板实例化（5 种）
| 模板参数 | 极速柜台 | 极速引擎 | 备注 |
|---|---|---|---|
| `api_impl<gw_counter_direct, single_socket_engine<gw_counter_direct>>` | 个微 | single_socket | C1 配置 |
| `api_impl<gw_counter_direct, tcpdirect_engine<gw_counter_direct>>` | 个微 | tcpdirect | C2 配置 |
| `api_impl<fpga_counter_direct, single_socket_engine<fpga_counter_direct>>` | fpga 直连 | single_socket | C3 配置 |
| `api_impl<fpga_counter_direct, tcpdirect_engine<fpga_counter_direct>>` | fpga 直连 | tcpdirect | C4 配置 |
| `api_impl<fpga_counter_gateway, idle_engine<fpga_counter_gateway>>` | fpga 网关 | **idle (占位)** | C5 配置（shared） |

---

## 2. 模块依赖图

```
                    api_interface (虚)
                          △
                          │ 继承
                          │
            ┌─────────────┴─────────────┐
            │                            │
            │ api_impl<TF, TE>           │
            │ (模板, 5 个实例化)         │
            │                            │
            │ 持有: TF, TE, c98, cb_mgr   │
            │ 持有: multi_engine          │
            │                            │
            └──┬────┬────┬────┬──────────┘
               │    │    │    │
       ┌───────┘    │    │    └────────┐
       │            │    │             │
       ▼            ▼    ▼             ▼
   TF (柜台)   fast_engine  multi_engine  cb_mgr
       │            │    │             │
       │            │    │             │
       │       ┌────┘    │             │
       │       │         │             │
       │       ▼         │             ▼
       │   aio_socket_link / tcpdir_link   api_callback (用户)
       │   link_timer_op
       │
       └─ 注入 queue + link_out_op

       multi_engine 内部:
       ┌────────────────────────────┐
       │ g98_link_ (counter98)        │
       │ fast_gw_link_ (TF, fpga 模式)│
       │ epoll_th_                    │
       │ send_queue_                  │
       └────────────────────────────┘
```

**关键依赖**：
- `api_impl` 是 **唯一** 持有所有子模块的实体（计数器、引擎、回调管理器）
- 链接（`aio_socket_link` / `tcpdir_link`）由 **引擎** 持有，不被 api_impl 冗余保存
- `link_out_op`（`link_engine_outop`）是引擎注入到柜台的回调接口
- 发送队列（`que_mth_buf`）是引擎与柜台共享的（柜台 `deal_*` 写入，引擎线程消费）

---

## 3. 关键数据流

### 3.1 业务发送数据流（以买卖委托为例）
```
用户
  │ order_insert(req)
  ▼
api_impl
  │ fast_.deal_order_req(req)
  ▼
fpga_counter_direct (TF)
  │ 校验 trade_link_connect_, login_state, fpga_state
  │ get_sec_index(security_id)
  │ 计算 take_len = sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(order_req)
  │ take_req_que_mem(data, take_len) → 写入 send_queue_  [从 fast_engine 注入]
  │ build_order_msg(req, cust, sec_index, head) → 写入 data 段
  │ cmt_req_que_mem(pos, take_len)
  │ 失败 → 降级到 c98_.deal_order_req(req)
  ▼
fast_engine 线程（do_work）
  │ 消费 send_queue_
  │ evt->type == LINK_EVENT_TYPE_SEND_MSG
  │ link_.send_msg(evt->data, evt->data_len)
  │ 失败 → counter.deal_send_error(...)
  ▼
aio_socket_link
  │ aio_tcp.send_msg
  ▼
[网络] → FPGA 柜台
```

### 3.2 应答接收数据流
```
[网络] → FPGA 柜台
  │ FPGA → 业务消息
  ▼
aio_tcp::loop_deal_recv
  │ (epoll 线程: multi 或 fast 的 mthread)
  ▼
aio_socket_link::msg_cb::deal_msg
  │ counter.deal_recv_msg(pmsg, msglen, link_type)
  ▼
fpga_counter_*.deal_recv_msg
  │ 循环解析消息
  │ switch (head->msg_id) {
  │   G1_MSG_ORDER_RTN: deal_order_rtn
  │   G1_MSG_TRADE_RTN: deal_trade_rtn
  │   G1_MSG_CANCEL_RSP: deal_cancel_rsp
  │   G1_MSG_HEART_ANS: eng_op.deal_heart_msg_ans
  │   G1_MSG_LOGIN_ANS: deal_log_ans
  │   ...
  │ }
  ▼
fpga_counter_base::deal_order_rtn
  │ 构造 OrderRtn
  │ cb_mgr_->on_order_rtn(si, out)
  ▼
callback_manager
  │ direct: user_callback_->on_order_rtn
  │ queued: 入 cb_queue_, 唤醒回调线程
  ▼
用户 api_callback
```

### 3.3 登录数据流（分级跳过 + S1 并发门）
```
用户
  │ login(LoginReq)
  ▼
api_impl
  │ 1. c98_.deal_agw_login()   [分级步骤①: agw 已成功 → 跳过(同步幂等)]
  │ 2. c98_.deal_login_req(req) [分级步骤②: 98 账户]
  ▼
counter98::deal_login_req
  │ val==LOGIN_STATE_OK? ──是──→ delive_fast_counter_login [跳过 98 步骤, 直达柜台步骤③]
  │否
  │ 投事件 (ACCOUNT_LOGIN, LINK_TYPE_98, info) → multi 线程 → g98_link 发送
  │   (98 步骤无在途缓存, 需求约定: 成功后永不再次登录;
  │    并发重复消息由柜台步骤③ S1 合并、98 迟到失败忽略 val==OK 消化)
  ▼
[网络] → 98 应答 → counter98::deal_cust_login_ans
  │ err==0: val→OK + delive_fast_counter_login (柜台步骤③; 每个成功应答均投递,
  │         重复事件由柜台状态机 S1/S2 幂等消化)
  │         err!=0: 回调失败(用户请求) / 迟到失败忽略
  ▼
柜台步骤③ fpga_counter_*::deal_cust_login (引擎线程, 三场景 §6.2)
  │ S2: 登录成功(不看链路) → 幂等回调成功, 不发包
  │ S1: 在途==USER → 合并不发
  │ 柜台不可用降级: fpga_state!=1(2-故障/3-故障切换柜台) → 本地降级成功回调, 不发包 (状态位不变、清在途)
  │ 首个: 在途无 → CAS 占位 USER → 组包发送 (gateway 新客户无对象直发,
  │       对象在首个成功应答时入库)
  ▼
[网络] → fpga/个微 login_ans → deal_log_ans
  │ 成功: save_client_info(state→OK) → [direct: delive_fpga_connect]
  │       → ans_login_ok: 在途==USER 消费→on_login 成功; RELOGIN/迟到→静默
  │ 失败: ans_login_err: USER/RELOGIN→清状态(→NONE)+回调; 无在途迟到→忽略; 不关链
  ▼
用户 api_callback: on_login(LoginAns)
```
> 登出数据流见 §6.5（本地同步操作，无网络交互）。

---

## 4. 模块详细设计

### 4.1 api_interface + api_impl

#### api_interface
- 纯虚基类，定义 18 个虚方法（1 login + 1 logout + 5 insert/cancel + 12 query）+ start/stop + 静态工厂
- 编译期多态：5 个 api_impl 模板实例化，无运行时 virtual 调度开销

#### api_impl<TF, TE>
- 模板参数：`TF` = 极速柜台类（3 种之一），`TE` = 极速引擎类（2 种之一 + idle）
- 成员：
  - `TF fast_`：极速柜台实例
  - `TE fast_engine_`：极速引擎实例（fpga_gateway 时为 idle）
  - `multi_socket_engine<TF> multi_engine_`：2 槽控制平面
  - `counter98 c98_`：98 柜台实例（非模板）
  - `callback_manager cb_mgr_`：回调管理器
  - `lb_common::lb_log log_`：日志
  - `int32 have_start`：启动状态机（0/1/2/3，3=已停止不可重启）
- 业务流程（`order_insert`）模板：
  ```cpp
  template <class TF, class TE> int32 api_impl<TF, TE>::order_insert(const OrderReq &req) {
    int32 ret = fast_.deal_order_req(req);
    if (ret == LBAPI_ERR_COUNTER_OFFLINE || ret == LBAPI_ERR_UNSUPPORTED_OP) {
      // 降级到 98
      return c98_.deal_order_req(req);
    }
    return ret;
  }
  ```
- `login(req)`（分级编排，§3.3）：
  1. `c98_.deal_agw_login()` —— 分级步骤①（agw 已成功 → 同步幂等跳过；agwuser 会话与 98 链接绑定，断链复位、重连由 have_agw_ok_ 自动重发）
  2. `c98_.deal_login_req(req)` —— 分级步骤②（98 val==OK → 跳过直达柜台步骤③；S1 在途合并；否则投 98 登录事件，柜台步骤③由 98 应答/跳过路径驱动）
- `logout(req)`（§6.5；两层编排，**柜台层先行**）：
  1. `fast_.deal_logout_req(req)` —— 柜台层门控（`log_state_ctl.to_logout`）：用户在途 → 返回 `STATE_LIMITED`；direct 账户不匹配（已绑定时）→ `NO_CUST`；重连在途 → 允许（置 LOGOUT 清在途位，迟到重登应答按 LOGOUT 忽略）；未登录/已登出/offline 降级/未绑定 → 幂等成功；置 LOGOUT 成功时按模式关链（direct 关 core / 个微不关链·多账户共用 / gateway 不关 GW 链）——**拒绝时 98 状态尚未变更**
  2. `c98_.deal_logout_req(req)` —— 98 层门控：cust_login_map 无成员（从未成功登录过）→ 返回 `NOT_LOG_CUST`（柜台登出成功 ⇒ 必经 98 登录成功、map 必有成员，此分支仅防御）；有成员（含已登出）→ val→LOGOUT（成员不移除）
  3. api_instance 本地 `build_logout_rtn`（编排层职责，LogoutAns 构造不下沉柜台）+ `cb_mgr_.on_logout(ans)` 同步回调成功
- 启动状态机：`have_start` 通过 CAS 抢占
  ```cpp
  template <class TF, class TE> int32 api_impl<TF, TE>::start() {
    while (true) {
      int32 t = atomic_load32(&have_start);
      if (t == 0) { if (atomic_cas32(&have_start, &t, 1)) break; }
      else if (t == 1) { sleep_us(300); }
      else { return LBAPI_OK; }  // t == 2: 已启动
    }
    // ... 启动各子模块 ...
    atomic_store32(&have_start, 2);
    return LBAPI_OK;
  }
  ```

### 4.2 multi_socket_engine<TF>

#### 角色
- **控制平面 epoll 线程**，同时管理两个槽的链接
- **极速引擎定时器的注册目标**（fast_engine 的 timerfd 注册到 multi 的 epoll_th）

#### 成员
- `aio_socket_link<counter98> g98_link_`（槽 0，恒存在）
- `link_timer_op<aio_socket_link<counter98>, multi_socket_engine> g98_link_timer_`（98 心跳/重连）
- `aio_socket_link<TF> fast_gw_link_`（槽 1，fpga 模式启用）
- `link_timer_op<aio_socket_link<TF>, multi_socket_engine> fast_gw_link_timer_`
- `lb_common::que_mth_buf send_queue_`：发送队列
- `lb_common::event_wake queue_wake_`：触发事件
- `lb_common::mthread epoll_th_`：epoll 线程
- `TF *counter_`：注入的极速柜台
- `counter98 *counter98_`：注入的 98 柜台
- `eng_link_op link_outop_`：注入到柜台的 out_op

#### epoll_th 监听的事件
1. `queue_wake_.get_fd()`：发送队列触发事件（业务侧 wake）
2. `g98_link_timer_.get_fd()`：98 心跳/重连 timer
3. `fast_gw_link_timer_.get_fd()`：极速GW 心跳/重连 timer（fpga 模式）
4. `g98_link_.get_fd()`：98 链接 IO 事件
5. `fast_gw_link_.get_fd()`：极速GW 链接 IO 事件

#### deal_event 事件分发（switch on evt->type）
| 事件类型 | 处理 |
|---|---|
| `LINK_EVENT_TYPE_SEND_MSG` | 根据 `link_type` 路由到 `g98_link_` 或 `fast_gw_link_` |
| `LINK_EVENT_TYPE_SEND_HEART` | 调 `counter.build_heart_msg` / `counter98.build_heart_msg` |
| `LINK_EVENT_TYPE_LINK_CLOSE` | `g98_link_.close_ch` / `fast_gw_link_.close_ch` |
| `LINK_EVENT_TYPE_LINK_CONNECT` | `g98_link_.connect` / `fast_gw_link_.connect` |
| `LINK_EVENT_TYPE_ACCOUNT_LOGIN` | `link_type != 98` → `deal_cust_login`；`== 98` → `deal_cust98_login` |
| `LINK_EVENT_TYPE_FPGA_CORE_CONNECT` | 占位（fpga core 由 fpga_counter 同步处理） |
| `LINK_EVENT_TYPE_AGWUSER_LOGIN` | `deal_agw98_login` |

#### 启动序列
```cpp
multi_socket_engine::start() {
  epoll_th_.add_poll_event(*this);            // 自身 fd（queue_wake）
  g98_link_timer_.add_timer_poll(&epoll_th_);  // 98 定时器
  if (fpga) {
    fast_gw_link_timer_.add_timer_poll(&epoll_th_);  // 极速GW 定时器
  }
  epoll_th_.run();                            // 启动 epoll 线程
  queue_wake_.wake();                         // 触发首次消费
}
```

#### 同步 connect_98agw
`api_impl::start()` 阶段同步调用，阻塞到 98 链接 WORKING：
```cpp
multi_socket_engine::connect_98agw() {
  if (g98_link_.is_work()) return 0;
  if (g98_link_.is_free()) return g98_link_.connect(recv_poll_num_, 0, &epoll_th_);
  return LBAPI_ERR_STATE_LIMITED;
}
```

### 4.3 fast_engine (single_socket / tcpdirect / idle)

#### 共同接口
- `init(cfg, tfst, log)`：配置队列 + 链接 + 定时器
- `start()`：启动业务 epoll 线程
- `stop()`：start 失败回退（关 timerfd + 关闭链接 + 停止线程，未启动为 no-op，保持可重新 start）
- `close()`：用户主动停止（不可再 start）
- `get_queue()`：返回发送队列（柜台注入用）
- `get_out_op()`：返回 link_out_op（柜台注入用）
- `add_timer_poll(mthread*)`：由 api_impl.start 调用，将 timerfd 注册到 mthread（对 fast_engine 是 multi 线程，对 idle 是 no-op）

#### single_socket_engine 内部
- 持 1 个 `aio_socket_link<TF>` 链接
- 持 1 个 `link_timer_op<aio_socket_link<TF>, single_socket_engine>`
- 持 1 个发送队列
- 自身是 `lb_common::simple_thread`（1 条业务 epoll 线程）

#### tcpdirect_engine 内部
- 持 1 个 `tcpdir_link<TF>` 链接（Solarflare TCPDirect）
- 持 1 个 `link_timer_op<tcpdir_link<TF>, tcpdirect_engine>`
- 持 1 个发送队列
- 自身是 `lb_common::simple_thread`
- 主循环是 `ch_.loop_deal_recv()`（Solarflare 自带 zf_mux_wait）

#### idle_engine（占位空实现）
- **无链接、无线程、无队列**（get_queue() 返回 nullptr）
- 仅持有 `counter_` 和 `log_` 指针
- `start()` / `stop()` / `add_timer_poll()` 全部 no-op
- **存在原因**：保持 5 种 `api_impl<TF, TE>` 模板实例化时的 `TE` 类型统一
- **实际工作**：fpga_gateway 模式下，所有业务由 `multi_engine` 槽 1 的 `fast_gw_link` 处理（api_impl 对 fpga_gateway 直接注入 multi 引擎队列），idle_engine 完全不工作

### 4.4 link_timer_op

- 模板参数：`TLink`（链接类型）+ `TEngine`（所属引擎）
- 创建 timerfd（CLOCK_MONOTONIC, 非阻塞）
- `init_timer(link, engine, interval)` 绑定 link + engine + 周期
- `add_timer_poll(mthread*)` 加入 epoll，开始定时循环
- `deal_event()`：读 timerfd → 决策：
  1. `link.check_heart_timeout()` → true → 投 `LINK_CLOSE` 事件（心跳超时关链，走断线重连）
  2. `link.check_heart_send()` → true → 投 `SEND_HEART` 事件
  3. `link.check_reconnect(&need_switch)` → 投 `LINK_CONNECT` 事件（从未成功连接过的链接 timer 亦周期重试并计数，双地址同址计满 max_fails 后切备，见 §7.2）
- `close()`：关闭 timerfd

### 4.5 aio_socket_link / tcpdir_link

#### 共同
- 组合模式（`aio_tcp` 和 `tcpdir_ch` 都不可继承，link 薄包装）
- 实现 `aio_ch_op<tcp_buf_ch>` / `tcpdir_ch_op` 接口
- 内嵌 `msg_cb`（`ch_recv_cb`）→ 直接转发到 `counter.deal_recv_msg`
- 心跳 / 重连由 link_timer_op 驱动

#### aio_socket_link
- 内部状态机：`IDLE → CONNECTING → WORKING → CLOSING → CLOSED`
- IO 由 multi / fast 引擎的 mthread 线程驱动（epoll）
- `set_remote(addr)` 可调 1-2 次（主+备）：永远保存最新地址，最多两个且不重复，已满淘汰最旧保留最新两个；98 / fpga_gw / 个微 fast 链接用主+备，fpga direct core 链接仅单地址（地址来自 GW 登录应答）
- `connect(need_switch, mthread*)`：异步连接
- `check_reconnect(&o_need_switch)`：检查是否需要重连 + 切地址（timer 线程仅原子读 addr_valid_num，与 set_remote 线程无读写竞争）

#### tcpdir_link
- 自带 tcpdir_stack（Solarflare 网卡 stack）
- `set_remote` 单一地址（先写 ip 再原子发布 port）
- `connect` 需先 init stack
- IO 由 fast_engine 线程 `ch_.loop_deal_recv()` 驱动（先 `stack_.poll_stack()` 驱动 reactor）

### 4.6 counter 层

#### 登录状态与在途标记（核心数据结构）
- **状态字典**（[api_event_msg.h](src/api_event_msg.h)）：`LOGIN_STATE_NONE=0 / LOGOUT=1 / OK=2`
  - 载体：`fpga_cust_info.login_state`（经 log_state_ctl）、`gw_counter_direct.log_state`、98 `cust_login_map_` 的 val
  - **实现约定：行为判定一律 `==LOGIN_STATE_OK / !=LOGIN_STATE_OK`**，不对 0/1 写行为分支（登出设置点/日志除外）——1=已登出 为纯审计信息
- **在途位标志**（账户级，与状态位组合编码在同一原子字 `log_state_ctl`）：`LOGIN_PENDING_USER=0x10`（用户在途）/ `LOGIN_PENDING_RELOGIN=0x20`（重登在途）—— **仅维护在柜台客户对象上**（direct/gateway 的 `fpga_cust_info`、个微柜台自身）；**98 步骤不设在途缓存**（账户一旦成功登录永不再次登录，无在途可互斥；并发首次登录的重复消息由柜台 S1 合并与 98 迟到失败忽略消化）
- `fpga_cust_info` 关键字段：`fpga_state`（独立故障维度 1-正常/2-故障/3-故障切换柜台，2/3 同为不可用态，**不触碰登录状态**，变化经 on_trade_counter 回调）、`log_state`（log_state_ctl 单原子字：低 2 位登录状态 + 0x10/0x20 在途位）

#### 链接适配 API（统一约定，模板多态）
```cpp
bool can_link_connect(int16 link_type);                        // 引擎重连前询问
int32 deal_link_connect(int16 link_type, int32 have_switch);   // connect 成功回调
void deal_link_close(int16 link_type);                         // 链接关闭回调
int32_t deal_logout_req(const LogoutReq &req);                 // api_impl 登出编排调用 (内部 log_state_ctl.to_logout 门控)
```
`can_link_connect` 实现：
| 柜台 | 行为 |
|---|---|
| `counter98` | 仅 `LINK_TYPE_98` 返回 true |
| `gw_counter_direct` | 仅 `LINK_TYPE_SPEED_TRADE` 返回 true |
| `fpga_counter_direct` | TRADE：`login_state==LOGIN_STATE_OK && fpga_state==1`（§6.4）；GW：恒 true |
| `fpga_counter_gateway` | 仅 `LINK_TYPE_SPEED_GW` 返回 true |

`deal_link_connect` 实现（重连矩阵，详见 §6.4）：
| 柜台 | 行为 |
|---|---|
| `counter98` | `trade_link_connect_=1`；曾成功登录过（have_agw_ok_）→ CAS 占位在途后重投 AGWUSER_LOGIN（与是否切换地址无关，§6.4） |
| `gw_counter_direct` | **需求6**：`login_state==OK` → `delive_relogin_event`（S3 占位 RELOGIN，缓存登录事件投递） |
| `fpga_counter_direct` | GW：无条件重发证券信息 + `is_logined() && fpga_state==1` → `delive_login_event` 直投（柜台 to_login 内部分支控制 S3）；TRADE：仅置 `trade_link_connect=1` |
| `fpga_counter_gateway` | GW：无条件重发证券信息 + 遍历客户（`is_logined() && fpga_state==1`）重登；"至少一个客户重登"以投递成功为准，**无应登客户不关链**、应登>0 且全投递失败才关链重来；**have_switch=1（地址切换）时任一客户投递失败即关链重来** |

`deal_link_close` 实现（解耦不变量 + 断链清除在途）：
| 柜台 | 行为 |
|---|---|
| 全部 | **不清除任何登录状态**；**清除该链接上的全部登录在途标记**（98 步骤无在途缓存、无可清；direct：单客户；gateway：遍历 clients_vec_；个微：自身）；在途请求作废、无登录回调（经 on_link_status 感知） |

#### counter98（非模板，固定类）
- 状态：`agw_login_state`（0/1/2，同步等待状态机；**断链复位**——会话与 98 链接绑定，曾成功标志 `have_agw_ok_` 保持并驱动重连重发）+ `trade_link_connect_`
- **98 账户登录状态表**：`cust_login_map_`（fund+branch → val=LOGIN_STATE_*），成员永不因登出移除；`have_cust_login`=val==OK（分级跳过）、`mark_cust_login_ok`=→OK、`deal_logout_req`（map 无成员 → NOT_LOG_CUST；有成员 → val→LOGOUT）
- **98 步骤无在途缓存**：账户一旦成功登录永不再次登录；每个 98 成功应答均投递柜台登录事件，重复事件由柜台 S1 合并与 S2 幂等消化（重复成功应答可能再次回调成功，见 LOGIN_COMPLIANCE 附注）
- 登录：`deal_agw_login`（同步等待、幂等、CAS 抢占在途）+ `deal_login_req`（分级跳过 + 投递）+ `deal_cust_login_ans`
- 登出支持：`deal_logout_req`（98 层门控：map 无成员 → NOT_LOG_CUST；有成员 → val→LOGOUT）/ `have_cust_login`（LogoutAns 构造在 api_impl 编排层，不在本类）
- 业务/协议/应答解析（真实 ATP AGW 协议 vendor 库 `counter98/`；出站 ver=vendor GetMsgVer()（会话类 0/业务类 2）、入站 `(msg_type,msg_ver)` 组合键精确分发（心跳分发前单独处理）；agw 登录成功后回报同步（AgwMsgSync/SyncFinish + 内存断点续传，RUL-26）；12 类查询（6 基础 + 6 扩展，RUL-26④）；`check_req_precond` 生效 LINK_DISCONNECTED / NOT_LOG_AGW / NOT_LOG_CUST 三前置）

#### fpga_counter_base（模板派生类的基类）
- 公共状态：`trade_link_connect`、`sec_state_`（0/1/2）、`session_id`（登录应答回填）
- 公共登录机制：
  - `log_state.ans_login_ok()`：成功应答回调判定——仅 在途==USER 回调一次；RELOGIN/迟到 静默（状态置 OK、清在途位）
  - `log_state.ans_login_err()`：失败应答统一处理——USER/RELOGIN → 清状态(→NONE)+回调（RELOGIN 曾成功客户重登被拒视为异常，不再自动重登）；LOGOUT/无在途迟到 → 忽略；登录失败一律不关链
  - `ans_cust_login`：引擎同步失败（connect/组包/发送）→ `clear_pending()`（USER 状态位本为 NONE；RELOGIN 保持成功态）+ 回调失败 + **关链**（链路不可用例外；重连后 RELOGIN 重试）
  - 内部重登投递：`delive_login_event` 直投（不做状态控制）——在途/资格控制统一到柜台 `deal_cust_login` 的 `log_state_ctl.to_login` 内部分支（置 RELOGIN、状态保持）
  - `deal_gw_rej` / `build_api_*_rej`：证券代码直接取协议体回显的 `security_id`（secs_ 仅剩发送方向带锁查找）
- 公共消息构造/解析：session_id 携带、session_seq_no 去重（去重内部化，RUL-32/33 后 StreamInfo=柜台类型+partno(板卡号)+stream_seq(fpga 恒 0)）
- 证券代码映射：`sec_map_`（hash，发送方向带锁）+ `secs_`（vector）

#### fpga_counter_direct（继承 fpga_counter_base）
- 持单 `client_info_`；`deal_cust_login` 三场景（§6.2）+ FPGA 下线本地降级 + 换账户拒绝（绑定过账户后异账户 → NOT_LOG_CUST + 失败回调，不发后台）
- `deal_log_ans`：成功 save + core 队满仅日志（不回调不关链，timer 重拨兜底）+ pending 驱动回调；失败 take 分支（§6.3）
- `deal_fpag_state`：仅改 `fpga_state`（不复位登录状态），每次更新经 on_trade_counter 回调（含恢复）；状态 2/3 时关 core 链接；恢复推送后 core 自动重拨闭环
- `deal_logout_req`：已绑定时账户不匹配 → `NO_CUST`（未绑定账户不拒绝，幂等成功——与 gateway 对称）；to_logout 门控（用户在途拒绝 / 未登录幂等成功）+ 置 LOGOUT 时关 core（不影响 GW 链）

#### fpga_counter_gateway（继承 fpga_counter_base）
- 持多客户存储：`client_map_`（fund+branch → *cust）+ `clients_`（board+user → *cust）+ `clients_vec_`（回收/重登遍历）
- `deal_log_ans`：**board/user 变化视为异常**——已入库对象收到不一致的 (board,user) → fatal 日志 + on_login 失败（UNSUPPORTED_TYPE）+ `ans_login_err 消费在途位`（避免在途滞留卡死后续 login），不更新映射（fgw 契约：board/user 当日不变）；未入库客户的首个成功应答为正常分配（创建对象 + 插入 clients_ / client_map_ 索引，先 clients_ 后 client_map_，失败路径回收）
- `deal_fpag_state`：仅改 `fpga_state`；重登资格门（fpga_state==1）天然跳过柜台不可用客户
- `deal_logout_req`：对象未入库（offline 降级等）→ 幂等成功；to_logout 门控 + 置 LOGOUT；共享 GW 链不关闭

#### gw_counter_direct（独立类，非模板）
- 多账户：每资金账户一条 `GwLoginInfo`（gw_login_cache.h，内嵌 `log_gw` log_state_ctl；凭据暂存/应答回填见该文件头契约）
- `deal_cust_login` 三场景（§6.2，多账户按条目状态机）；`deal_link_connect` **需求6 重连自动重登**（`collect_relogin_events` 收集已登录条目批量投递，S3）
- `deal_logout_req`：to_logout 门控 + 置 LOGOUT；**多账户共用链路不关链**；当前未校验请求账户与绑定账户一致（异账户 logout 会登出绑定账户，见 LOGIN_COMPLIANCE 已知偏差）
- 协议占位（g1 头 + login_req/login_ans 临时替代，业务消息留空 todo）

---

## 5. 启动时序详解（critical path）

```
api_impl::start()
  │
  ├─[1] cb_mgr_.start()
  │     └─ 队列模式：启动回调线程
  │
  ├─[2] fast_engine_.add_timer_poll(multi_engine_.get_thread())
  │     └─ 极速业务心跳/重连 timerfd 加入 multi 控制平面 epoll
  │
  ├─[3] multi_engine_.connect_98agw()           [同步阻塞]
  │     └─ g98_link_.connect(recv_poll_num_, 0, &epoll_th_)
  │        └─ 98 链接 IDLE → CONNECTING → WORKING（仅建链，不收发）
  │
  ├─[4] multi_engine_.start()                   [异步]
  │     ├─ epoll_th_.add_poll_event(*this)     // queue_wake
  │     ├─ g98_link_timer_.add_timer_poll(&epoll_th_)
  │     ├─ [fpga] fast_gw_link_timer_.add_timer_poll(&epoll_th_)
  │     └─ epoll_th_.run()                     // 启动 epoll 线程
  │
  └─[5] fast_engine_.start()                     [异步]
        └─ simple_thread.run() 启动业务 epoll 线程
```

**关键时序约束**：
- [1][2] 必须先于 [3]：先准备好回调线程 + 业务 timerfd 注册
- [3] 必须先于 [4]：98 链接要建立好，multi.start 的 epoll 才有 fd 可监听
- [4] 必须先于 [5]：fast_engine.add_timer_poll 已把业务 timerfd 注册到 multi 的 epoll_th
- 任一步失败 → 统一回滚（fast→multi→cb 逆序 stop，未启动为 no-op）并复位 have_start，允许原地重试

**agw 登录不在 start() 中**——agwuser 登录随首次用户 `login()` 的分级步骤①同步发起（`c98_.deal_agw_login()`，幂等，已成功则跳过；见 §6.1/§6.2）。原因：
- agw 登录依赖 multi 引擎线程消费 `AGWUSER_LOGIN` 事件并接收应答，start 末步与 login 首步在时序上等价；
- 需求 7（分级跳过）要求 agwuser 登录成为登录流程的可跳过步骤，而非独立的启动动作。

---

## 6. 登录时序详解

> 执行者约定：**[用户]** 用户线程 ｜ **[multi]** multi 引擎 epoll 线程 ｜ **[fast]** fast 引擎业务线程。

### 6.0 登录状态模型（核心不变量）

| 状态 | 载体 | 写入 | 清除 |
|---|---|---|---|
| agwuser 登录状态 | counter98 `agw_login_state`（0/1/2，同步等待状态机） | agw 登录/重登成功 | 用户登出不触及；**断链复位**（会话与链路绑定，曾成功标志 have_agw_ok_ 保持）；重登被柜明确拒绝时复位+关链（后台契约下正常不发生，§6.4） |
| 客户 98 登录状态 | `cust_login_map_` val（LOGIN_STATE_*，成员永不因登出移除） | 98 登录成功（→OK） | 仅用户主动登出（→LOGOUT） |
| 客户柜台登录状态 | `fpga_cust_info.log_state` / 个微 `log_state`（log_state_ctl 低 2 位，LOGIN_STATE_*） | 柜台登录成功（→OK） | 仅用户主动登出（→LOGOUT）；内部重登被拒 → 清状态(→NONE)+回调+不再重登 |
| fpga 下线状态 | `fpga_cust_info.fpga_state`（1-正常/2-故障） | FPGA 状态推送 | FPGA 状态推送（独立故障维度，不触碰登录状态） |
| 登录在途位 | **仅柜台客户对象**（log_state_ctl 的 0x10=USER / 0x20=RELOGIN 位；98 步骤无在途缓存） | 三场景占位（§6.2） | 应答消费 / 断链清除 |

**不变量**（REQUIREMENTS §3.2.0）：
- 登录成功状态仅能被用户主动登出改变为已登出（或内部重登被拒终局 → NONE）；**链接断开/重连/建立一律不清除客户登录状态，仅清除在途位**（agwuser 例外：会话与 98 链路绑定，断链复位，§6.1）；
- 登录状态 ≠ 链路可用性：交易请求同时校验两者（§6.6）；
- 登录状态判定一律 `==LOGIN_STATE_OK / !=LOGIN_STATE_OK`。

### 6.1 agwuser 登录（分级步骤①，同步）
```
[用户] login() → c98_.deal_agw_login()
  ├─ 状态==OK → 同步幂等返回（跳过）
  ├─ 状态==在途 → 等待首者结果（超时返回,在途请求仍会完成并置终态）
  └─ 占位(0→1) → 投 AGWUSER_LOGIN 事件 → [multi] 发送
       └─ AGW_LOGIN_ANS → 成功标志(err=0/state=2/have_agw_ok_)后置, 先 send_agw_sync()
                         (按配置回报同步,RUL-26④/27/28; 快速登录/无配置跳过; 需同步而
                         发送失败=agw 登录失败: 置真实错误码+复位在途(1→0)+关 98 链,
                         成功标志不置位→重连不自动重登,用户重新 login; 下行回报按分区
                         去重+推进断点(RUL-30,≤已存值丢弃); SyncFinish 到达
                         on_event(report_sync_finish)) /
                         失败: 复位+设错误码+关 98 链 (从未成功则等用户重新 login)
  (断链: state 复位(会话随链路失效, 在途则设断链错误码解锁同步等待方);
   重连: have_agw_ok_==1 → CAS 占位(0→1)重发——与用户线程 CAS 抢占互斥,
         先占者投递后到者跳过; 成功前 98 业务按 NOT_LOG_AGW 拒绝)
```
- 98 链接重连：曾成功登录过（have_agw_ok_）→ 重投 agwuser 登录（刷新会话，与是否切换地址无关），成功前 98 业务按 NOT_LOG_AGW 本地拒绝（§6.4）。
- 重登窗口期业务（查询/降级）放行、由后台明确拒绝（REQUIREMENTS §6.2 后台契约）。

### 6.2 分级登录与三场景（用户 login()，REQUIREMENTS §3.2.2）
```
[用户] login(req)
  ① agwuser 登录 (§6.1, 已成功跳过)
  ② 98 客户登录: c98_.deal_login_req
       val==OK → 跳过, 直达③
       否则投 98 登录事件 → [multi] 发送 → ACC_LOGIN_ANS
         成功: val→OK + 转③;  失败: 回调失败 (迟到失败 val==OK 忽略)
         (98 步骤无在途缓存: 成功后永不再次登录;
          并发重复消息由柜台步骤③ S1 合并与迟到失败忽略消化)
  ③ 柜台客户登录 (fpga/个微 deal_cust_login, 引擎线程):
       S2 幂等 / S1 合并或首个 / 下线降级 / 内部重登分支 (见下)
```
**柜台步骤三场景**（log_state_ctl 在途位；REQUIREMENTS §3.2.2）：
| 场景 | 条件 | 动作 |
|---|---|---|
| S2 幂等 | 用户发起 且 登录成功（**不看链路状态**） | 立即回调成功，不发包（断线窗口同样幂等） |
| S1 合并 | 在途==USER（同账户并发） | 不发包，共享首应答回调一次（按资金账号+分支机构归因） |
| 下线降级 | 用户发起、非成功态 且 `fpga_state!=1`（2/3） | 本地降级成功回调（98 成功即成功），不发包、状态位不变、清在途；恢复后用户重新 login |
| S1 首个 | 在途无 | 用户发起 → `to_login` CAS 占位 USER（状态位清 NONE）→ 组包发送（gateway 新客户无对象直接发送，对象在首个成功应答时入库）；内部重登事件经 `to_login` 内部分支置 RELOGIN（状态保持；非成功态静默终止） |

- **换账户**（direct/个微）：绑定过账户（含已登出/在途）后异账户 login → 柜台步骤同步拒绝（NOT_LOG_CUST + 失败回调，不向极速柜台发包）；98 步骤先行，异账户的 98 登录已发出（98 map 可能留下 OK 幽灵成员，无行为影响，显式登出可清理）。
- **新客户并发**（gateway）：客户对象在**首个成功应答**时入库（生命周期不变）；入库前的并发首登缺少互斥载体，重复柜台登录事件由状态机 S1/S2 幂等消化（重复成功应答可能再次回调成功，见 LOGIN_COMPLIANCE 附注）。

### 6.3 登录应答与回调（按发起方界定）
| 应答 | 处理 |
|---|---|
| 柜台登录成功 | save（state→OK）→ [direct: 发起 core，失败仅日志 timer 兜底] → `ans_login_ok`：在途==USER 消费 → **on_login 成功**；RELOGIN/迟到 → 静默 |
| 柜台登录失败，在途==USER（用户发起） | 状态→NONE + **on_login 失败**（不关链） |
| 柜台登录失败，在途==RELOGIN（内部重登被拒） | **状态→NONE** + **on_login 失败** + **不再自动重登、不关链**（曾成功客户重登被拒视为异常，交由用户处理） |
| 无在途迟到应答 | 忽略（在途已被首应答/断链清除） |
| 引擎同步失败（connect/组包/发送） | `ans_cust_login`→clear_pending（USER：状态位本为 NONE；RELOGIN：保持成功态）+ on_login 失败 + **关链**（链路不可用例外；重连后 RELOGIN 重试） |

**回调例外**（REQUIREMENTS §3.2.3）：①同账户并发合并仅回调一次（98 重复成功应答/gateway 新客户并发场景不严格成立，见 LOGIN_COMPLIANCE 附注）；②断链作废的在途用户登录无登录回调（经 on_link_status 感知）。

### 6.4 断线重连与重新登录矩阵（REQUIREMENTS §3.2.5）
| 链接 | 断链时 | 重连后 |
|---|---|---|
| 98 网关链接 | agwuser 状态复位（会话失效，在途则设断链错误码）；客户状态不清（98 步骤无在途缓存） | 曾成功登录过（have_agw_ok_）→ 重发 agwuser 登录（成功前业务按 NOT_LOG_AGW 拒绝）；已成功 98 客户**不重登** |
| fpga GW 链接（direct/gateway） | 不清客户状态；清在途 | 无条件重取证券信息；`is_logined() && fpga_state==1` 客户重登（S3 占位 RELOGIN，状态保持）；gateway"至少一个客户重登"以**投递成功**为准，无应登客户不关链、应登>0 全投递失败才关链重来；**have_switch=1 时任一投递失败即关链重来** |
| fpga core 链接（direct） | 置 trade_link_connect=0（不动状态/在途——登录消息不走 core） | `login_state==OK && fpga_state==1` 方可重连（can_link_connect 门）；无登录动作 |
| 个微链接 | 不清状态；清在途 | `login_state==OK` → `delive_relogin_event` 自动重登（需求6，S3；被拒按终局：清状态+回调+不再重登） |

**FPGA 下线推送**：仅改 `fpga_state`（+on_event+direct 关 core）；登录状态保持 → **已登录**用户 login() 幂等回成功（98 降级）、委托按 COUNTER_OFFLINE 降级 98；恢复推送后 core 自动重拨 + GW 重连自动重登，闭环恢复。

### 6.5 用户主动登出（REQUIREMENTS §3.2.4；柜台层先行）
```
[用户] logout(req) → api_impl::logout (柜台层先行, 拒绝时 98 状态不变)
  ├─ fast_.deal_logout_req (柜台层, log_state_ctl.to_logout):
  │    用户在途 → 返回 STATE_LIMITED (同步失败, 无回调)
  │    direct 账户不匹配(已绑定) → 返回 NO_CUST (同步失败, 无回调)
  │    重连在途 → 允许登出 (置 LOGOUT 清在途位, 迟到重登应答按 LOGOUT 忽略)
  │    未登录/已登出/offline 降级/未绑定 → 幂等成功
  │    置 LOGOUT 成功 → direct: 关 fpga core 链接 (不影响 GW 链) / gateway: 不关共享 GW 链 / 个微: 不关链接 (多账户共用)
  ├─ c98_.deal_logout_req (98 层): map 无成员(从未成功登录) → 返回 NOT_LOG_CUST (仅防御:
  │    柜台登出成功 ⇒ 必经 98 登录成功, map 必有成员); 有成员(含已登出) → val→LOGOUT (成员不移除)
  └─ on_logout 成功回调 (本地同步操作, 不发协议消息)
```
登出后：客户信息/索引保留（不退出"曾成功"集合）；重登资格由登录状态门控（登出客户不参与自动重登）；重新 login 走完整分级流程。

### 6.6 业务门控（登录状态 × 链接状态，REQUIREMENTS §3.2.6）
- 委托/撤单必须同时满足：对应柜台**客户登录状态==OK** + 对应**链接状态**；任一不满足按错误码拒绝。
- **降级矩阵**：仅 FPGA 下线（COUNTER_OFFLINE）与柜台不支持（UNSUPPORTED_OP）降级 98；**链接断开硬失败**（LINK_DISCONNECTED，不降级）。
- 98 侧业务（查询/降级）登录前置：链接 + agw 登录本地校验；"账户登录成功过"检查为 todo，正式 98 协议到位后统一（见 LOGIN_COMPLIANCE）。

---

## 7. 链接管理详解

### 7.1 链接层级关系
```
[极速引擎] 持有 [业务链接] + [业务timer]
[多socket引擎] 持有 [98链接] + [98timer] + [极速GW链接] + [极速GWtimer]
[业务timer] 和 [极速GW timer] 都注册到 [多socket引擎的 epoll 线程]
```

**关键设计**：
- 极速业务链接的心跳由控制平面（multi 引擎）驱动，不是由业务线程驱动
- 这样可以保证心跳事件统一处理，不会因为业务线程阻塞而漏处理
- epoll 线程是唯一处理 timerfd 的线程

### 7.2 心跳 / 重连 决策
```
timerfd 触发 (每 check_interval 秒, check_interval = heartbeat_interval / 2)
  │
  ├─ link.check_heart_timeout() → true → 投递 LINK_CLOSE 事件
  │                                    (关闭链接, 转入断线重连流程, 见 §6.3-6.6)
  ├─ link.check_heart_send()  → true  → 投递 SEND_HEART 事件
  │        (发送周期 = heartbeat_interval, timer 以其一半频率检查)
  └─ link.check_reconnect(&need_switch)
        ├─ 链接不是 FREE 状态 / 无地址 → false (不重连)
        ├─ 单地址 (valid_num==1):
        │     reconnect_ctl.check_reconnect —— 从未连上与曾连上均计数:
        │     从未连上每周期一次(计满 max_fails 停止重试); 曾连上受间隔 + max_fails 上限
        │     → true 时投 LINK_CONNECT(0) 事件 (重连当前地址)
        └─ 双地址 (valid_num==2):
              从未连上与曾连上统一计数决策 (首拨主地址):
              计数 < max_fails → true, need_switch=0 (重试当前地址);
              计满 max_fails → 重置计数后再查 → true, need_switch=1
              (切换另一地址; max_fails=0 无限同址不切, 两阶段语义一致)
              → 投 LINK_CONNECT(need_switch) 事件
```

### 7.3 链接事件投递
```
link_timer_op::delive_link_event(event_type, event_data):
  ├─ 通过 engine_.get_queue() 获取引擎发送队列
  ├─ 申请 queue 内存: total_len = sizeof(link_send_event) + 24
  ├─ link_send_event.evt:
  │   link_type = link_->get_link_type()
  │   type = event_type
  │   data_len = 24
  └─ 写入 data: 根据 event_type 填 link_close_event_info 或 link_connect_event_info
```

### 7.4 链接 connect 回调链（have_switch 透传）

```
timerfd 触发 link_timer_op::check_reconnect → 投 LINK_CONNECT(need_switch)
  │
  ▼
[引擎线程] deal_event 消费 LINK_CONNECT 事件
  │  调 counter_->can_link_connect(evt->link_type)
  │   └─ false → 跳过 (fpga 柜台可能因未登录拒绝 trade 链接重连)
  │
  │  调 link_.connect(recv_poll_num_, pcon->need_switch, &epoll_th_)
  ▼
[链接层] aio_socket_link::connect / tcpdir_link::connect
  ├─ have_switch 计算 (仅 aio_socket_link, tcpdir 永远 0):
  │   need_switch != 0 且 addr_valid_num == 2 → active_idx_ 翻转 (备→主也算) + have_switch=1
  │
  ├─ ch_.connect_ch(当前活动地址) —— 单次同步尝试
  │   (失败不自动试另一地址; 备地址经 timer 的 need_switch=1 路径生效, 见 §7.2)
  │
  └─ 成功 → counter_->deal_link_connect(link_type_, have_switch)
              │
              ▼
[柜台层] counter::deal_link_connect(link_type, have_switch)
  ├─ trade_link_connect_ = 1
  ├─ on_link_status(..., 1)
  └─ 柜台特定动作:
      ├─ counter98: have_agw_ok_==1 → CAS 占位后重投 AGWUSER_LOGIN 事件 (与 have_switch 无关)
      ├─ gw_direct: have_switch 忽略
      └─ fpga_*: have_switch 仅影响 gateway 重登失败策略 (§6.4: 切换时任一投递失败即关链)
```

### 7.5 链接状态变化回调
```
link 断链 [aio: deal_recv_stop 回调 / tcpdir: deal_ch_closing 回调]
  ├─ counter.deal_link_close(link_type)
  │   ├─ trade_link_connect_ = 0
  │   ├─ cb_mgr_->on_link_status(..., 0)
  │   └─ 清除在途位 (clear_pending, 不清登录状态 —— 解耦不变量;
  │      agwuser 例外: 98 链路断开复位 agw_login_state, 会话随链路失效)
  └─ 返回
```

### 7.6 极速GW 链接 vs 极速业务链接

| 角色 | 极速GW (multi 槽 1) | 极速业务 (fast 引擎) |
|---|---|---|
| link_type | `LINK_TYPE_SPEED_GW` | `LINK_TYPE_SPEED_TRADE` |
| 用途 | 登录 + 证券信息 + 业务（fpga_gateway） | 业务（fpga_direct=core, gw=业务） |
| 线程 | multi epoll_th | fast 业务线程 |
| 触发器 | multi 持有 | fast 持有（但注册到 multi） |
| 适用模式 | fpga_direct / fpga_gateway | fpga_direct / fpga_gateway（无）/ gw_direct |
| 支持备地址 | ✅ (aio_socket_link) | ✅ |
| have_switch 透传 | ✅ | ✅ |

---

## 8. 关键时序约束与并发安全

### 8.1 启动状态机
- `have_start` 四态：0-未启动 / 1-启动中 / 2-已启动 / 3-已停止（不可重启）
- CAS 抢占启动权；start 失败统一回滚后复位为 0（允许原地重试）；stop 后为 3，重新 start 返回 STATE_LIMITED
- 并发 start 不会重复启动；stop 与 start 经 CAS 互斥

### 8.2 agw 登录状态机（CAS 保护）
- `agw_login_state` 三态：0/1/2（同步等待状态机；断链复位——会话与 98 链路绑定，曾成功标志 have_agw_ok_ 保持）
- 0→1 转换用 `atomic_cas16`（deal_agw_login 用户线程与 deal_link_connect 重连重发线程均经 CAS 抢占，先占者投递）；deal_agw_login 阶段同步占用，deal_agwuser_login_ans 阶段释放
- 业务发送（deal_order_req 等）必须 agw_state==OK

### 8.3 登录状态与在途标记（核心）
- **登录状态**（`log_state_ctl` 低 2 位 / 98 map val）：原子 32-bit CAS 读写；**写者**＝登录成功应答（→OK）、登出（→LOGOUT）、重登被拒终局（→NONE）；**读者**＝业务门控（用户线程）、重登门（引擎线程）——判定一律 `==OK / !=OK`
- **柜台登录控制**：`log_state_ctl` 单原子字（状态位+在途位组合编码），`to_login`/`to_logout`/`ans_login_ok`/`ans_login_err`/`clear_pending` 全部 CAS 循环，跨"用户线程事件投递 + 引擎线程应答/断链"并发安全；**98 步骤无在途缓存**
- pending 与登录状态正交：在途不降级登录状态，断链清在途不清状态

### 8.4 队列安全
- `send_queue_` 是 `que_mth_buf`（多写多读线程安全）
- 柜台线程调 `write_get_mth`（多写接口）；引擎线程调 `read_get`（单读接口）；无锁
- multi 引擎消费循环以 `queue_dealing_` 原子标志 + 复查补唤醒覆盖"清 dealing 前入队"的丢唤醒窗口

### 8.5 链接生命周期同步
- 极速引擎定时器必须在 multi 引擎启动**后**注册（timerfd 注册到 multi 的 epoll_th）
- api_impl::start 顺序：**cb_mgr → fast_engine.add_timer_poll → multi.connect_98agw → multi.start → fast_engine.start**（agw 登录随首次 login()，§5）
- 停止顺序：multi.close → fast_engine.close → cb_mgr.close（先停生产者，避免 multi 在途写入已关闭的 fast 队列）

---

## 9. 文件清单与依赖

### 9.1 头文件（api/include/ + api/src/）
| 文件 | 角色 |
|---|---|
| `api_interface.h` | 用户 API 虚基类 |
| `api_callback.h` | 回调接口 + StreamInfo/QueryAnsCtl/err_event_type |
| `api_config.h` | 配置虚基类 + 27 个配置名常量 + 5 种类型 |
| `api_config_impl.h` | api_config 实现类（属性表 + 便捷 getter） |
| `api_errno.h` | 39 个错误码 + api_strerror 声明 |
| `order_trade_type.h` | 业务类型（OrderReq/OrderRtn/CancelReq/CancelRsp/LoginReq/LoginAns/6 个 Query*） |
| `api_instance.h` | api_impl 模板 + 显式实例化 |
| `api_event_msg.h` | 链接类型常量 + 7 种 link_send_event 类型 + 4 种 event_info + log_state_ctl 状态机（LOGIN_STATE/PENDING 字典与六方法） |
| `callback_manager.h` | 回调管理 + 11 种 cb_event_type（含 logout） |
| `aio_socket_link.h` | aio_tcp 链接包装 |
| `tcpdir_link.h` | TCPDirect 链接包装 |
| `link_timer_op.h` | 链接 timerfd 定时器 |
| `counter98.h` | 98 柜台类（非模板） |
| `fpga_counter_base.h` | FPGA 柜台基类（公共状态/消息/解析） |
| `fpga_counter_direct.h` | FPGA 直连柜台 |
| `fpga_counter_gateway.h` | FPGA 网关柜台 |
| `gw_counter_direct.h` | 个微直连柜台（独立类） |
| `idle_engine.h` | 占位极速引擎（fpga_gateway 用） |
| `multi_socket_engine.h` | 2 槽多 socket 引擎 |
| `single_socket_engine.h` | 单 socket 极速引擎 |
| `tcpdirect_engine.h` | TCPDirect 极速引擎 |

### 9.2 实现文件（api/src/）
- 16 个 .cpp 文件（与 .h 一一对应，api_config.cpp 对应 api_config.h/api_config_impl.h）
- 5 个 `api_impl` 模板显式实例化在 `api_instance.cpp` 末尾
- 链接 / timer / 引擎的模板显式实例化在各自成员函数定义文件末尾（显式实例化必须置于定义所在编译单元，否则 .so 运行期符号缺失）

### 9.3 外部依赖（include/ + 公共库）
- `g1msghead.h` / `g1trademsg.h`：FPGA g1 协议（外部定义，完整）
- `counter98/`（api 内 vendor 目录）：98 ATP AGW 协议库（消息/编解码/常量，已接线，RUL-25 起 c98msg_tmp.h 占位头废弃删除）
- `lb_common`：基础库（aio_tcp / tcpdir_ch / mthread / que_mth_buf / simple_thread / log / reconnect_ctl / event_wake / hash_map_mth / str_copy_format 等）

---

## 10. 关键设计决策与权衡

### 10.1 idle_engine 占位而非删除
- 原因：保持 `api_impl<TF, TE>` 模板签名统一，5 种配置实例化一致
- 代价：fpga_gateway 模式多一个无用模板实例
- 收益：模板代码无需 if-else 分支，工厂逻辑清晰

### 10.2 极速业务 timerfd 由 multi 引擎驱动
- 决策：业务线程只管收发，心跳/重连由控制平面统一处理
- 收益：业务线程阻塞不漏心跳
- 代价：timerfd 跨线程（但 epoll 线程安全）

### 10.3 链接是 OWNED 关系
- 链接（aio_socket_link / tcpdir_link）由引擎持有，不被 api_impl 冗余保存
- 收益：避免双重释放，生命周期清晰
- 代价：链接状态机由引擎调用 deal_ch_* 回调，柜台不能直接操作链接

### 10.4 模板特化多态（无 virtual）
- 决策：fpga_counter_base 用模板而非 virtual 基类
- 收益：编译期多态，零运行时开销
- 代价：派生类同名方法独立实现，无 override 检查

### 10.5 业务降级
- 决策：仅极速柜台返回 `COUNTER_OFFLINE`（FPGA 下线）/ `UNSUPPORTED_OP`（不支持）时自动改投 98；**链接断开（LINK_DISCONNECTED）为硬失败，不降级**
- 收益：fpga 下线/不支持时 98 接管；链路状态语义清晰（断链≠柜台离线）
- 代价：断链期间业务失败（登录状态保持，链路恢复即自动可用）

### 10.6 回调分发线程模型
- 决策：queued 模式下全部回调事件经 cb_queue_ 由**单一回调线程串行分发**（同账户回调有序，用户回调无需考虑多线程并发，仅可重入）；direct 模式下回调在产生事件的线程（multi/fast 引擎线程、用户线程）**直接并发调用**
- 收益：queued 模式简化用户并发模型；direct 模式零队列延迟
- 代价：queued 模式队列满时事件丢弃（仅记错误日志，不保证不丢，见 api_config.h 回调模式注释）；direct 模式用户回调需自行处理多线程并发

### 10.7 counter98 非模板
- 决策：98 柜台为独立具体类（不模板化）
- 原因：98 业务（查询/降级）固定，仅一个实例
- 代价：未来若要支持 98 多种配置需重新模板化

### 10.8 have_switch 语义：caller 显式切
- 决策：`have_switch=1` 当且仅当 `need_switch != 0` 且双地址有效（valid_num==2），此时 `active_idx_` 翻转（备→主的切回同样置 1）
- `need_switch=0` 的重连恒为 `have_switch=0`；connect 内部无"主地址失败自动试备"路径（见 §7.2/§7.4）
- 该标志用于日志与诊断（98 agw 重发与 have_switch 无关，为无条件触发；gateway 重登仅在 have_switch=1 时收紧失败策略，见 §6.4）

### 10.9 链接适配 API 模板约定
- 决策：4 个柜台都实现 `can_link_connect(int16 link_type)` 和 `deal_link_connect(int16 link_type, int32 have_switch)` 接口
- 不通过 virtual，而是模板多态（与 10.4 一致）
- 引擎在 `LINK_EVENT_TYPE_LINK_CONNECT` 分发时统一调用 `can_link_connect`，避免每个 engine 重复判断
- 收益：link 与 counter 的协议边界清晰；新增柜台时只需实现这两个接口
- 代价：仍是无 override 检查，靠文档 + 编译期多态保证一致性

### 10.10 可靠消息流水去重
- 决策：g1 协议 `order_rtn` / `trade_rtn` / `cancel_rsp` 增加 `session_seq_no` 字段
- 语义：>0 表示可靠消息（按此字段去重），=0 表示非可靠（不去重）
- fpga 柜台基类按 `body->session_seq_no` 做去重（保留最大流水号，仅内部去重不外露），`StreamInfo` = 柜台类型 + partno(板卡号) + stream_seq 恒 0（RUL-32/33）
- 收益：去重判定单点内聚；RUL-32/33 后流身份经 counter_type+partno(板卡号)外露（98 侧另有 stream_seq=分区内记录号），fpga 去重不再外露。
- 代价：每次 RTN 处理需多 1 次比较 + 赋值

### 10.11 session_id 字段透传
- 决策：g1 消息头增加 `board_no` / `user_id` / `session_id`（uint32）字段
- `fpga_counter_base.session_id` 登录应答时从 `login_ans.session_id` 回填
- 业务消息（订单/撤单/证券信息/心跳）头中携带 session_id
- 收益：后台可按 session_id 路由/校验消息；链接地址切换后若 session_id 失效，柜台可检测
- 代价：消息头由 12 字节扩为 16 字节（+33%）

### 10.12 登录状态与链接生命周期解耦（REQUIREMENTS §3.2.0）
- 决策：登录成功状态仅能被用户主动登出（或内部重登被拒终局 →NONE）改变；链接断开/重连只清除登录在途位，不清客户登录状态（agwuser 例外：会话与 98 链接绑定，断链复位）
- 收益：状态权威唯一（柜台明确信号），GW/core 断链期间交易按链路状态独立门控；避免断链误清状态引发的状态耦合缺陷族（断链误清状态→交易误拒→重连死锁）
- 代价：登录状态与会话实际有效性可能短暂不一致（由后台明确拒绝兜底，窗口期原则）

### 10.13 在途位标记 + 三场景
- 决策：在途语义全部由在途位（USER=0x10/RELOGIN=0x20，与状态位同字编码）表达，登录状态不设"在途"值；S1 并发互斥合并 / S2 幂等（成功即幂等，不看链路）/ S3 内部重登（CAS 占位）
- 收益：一个原子变量同时解决并发互斥（后台"同链接两次登录拒绝"契约）、回调归属（USER 消费才回调）；状态字段保持两态语义纯净
- 代价：已成功用户 login 一律幂等回成功（断线窗口内不发起真登录，会话刷新由重连内部重登承担）

### 10.14 状态字典统一与判定约定
- 决策：`LOGIN_STATE_NONE/LOGOUT/OK`（0/1/2）跨字段统一（fpga_cust_info / 个微 / 98 map val）；行为判定一律 `==LOGIN_STATE_OK / !=LOGIN_STATE_OK`，1=已登出 为纯审计信息
- 收益：在途位与状态字段各自独立取值不冲突；"从未登录/已登出"无行为差异由判定约定从构造上保证
- 代价：无（约定靠评审保证）

### 10.15 登出为纯本地操作（两层编排、柜台层先行）
- 决策：logout 不向柜台发协议消息——柜台层 to_logout 先行（用户在途拒绝 / 重连在途允许 / 未登录幂等成功 / 置 LOGOUT 时按模式关链：direct 关 core / 个微关链 / gateway 不关共享 GW 链）+ 98 层 map 无成员拒绝（NOT_LOG_CUST，仅防御）/ 有成员 val→LOGOUT（成员均不移除）
- 收益：无协议依赖（协议未正式化即可用）、无异步失败路径；客户信息保留（重新 login 无需重建索引）；柜台层先行保证拒绝路径零状态变更
- 代价：fgw 侧不感知登出（客户资源由 fgw 自身会话机制回收）

### 10.16 回调按发起方界定
- 决策：是否回调仅取决于 pending（=用户在途）与应答结果；内部重登成功静默、失败清状态+回调+不再自动重登
- 收益：单一判定源；重登资格（state==OK）与回调归属（pending）解耦
- 代价：合并/断链作废两类场景无逐请求回调（文档化例外）

### 10.17 换账户拒绝
- 决策：direct/个微绑定过账户（曾成功/在途，含已登出）后异账户 login：柜台步骤同步拒绝（NOT_LOG_CUST + 失败回调，不向极速柜台发包）；换号 = 销毁实例重建；gateway 多客户共存不受限
- 收益：fgw 侧"同连接不可换号"不变式由 API 侧保证
- 代价：98 步骤先于柜台判定——异账户的 98 登录已发出并成功（98 map 留下 OK 幽灵成员，无行为影响；显式登出可清理）

---

## 11. g1 协议结构与字段约定

> 完整定义见 [include/g1msghead.h](../../include/g1msghead.h) 与 [include/g1trademsg.h](../../include/g1trademsg.h)

### 11.1 消息头 `g1_msg_head` (16 字节)

| 字段 | 类型 | 用途 |
|---|---|---|
| `msg_id` | uint32 | 消息号（见 §11.3 消息号字典） |
| `msg_len` | uint32 | 不含头, 包体长度 |
| `board_no` | uint16 | 板卡号 |
| `user_id` | uint16 | 用户 id |
| `session_id` | uint32 | 会话 id（登录应答回填，业务消息携带） |

### 11.2 长度常量

| 常量 | 值 | 说明 |
|---|---|---|
| `G1_MSG_MAX_LEN` | 32700 | 单消息最大长度 |
| `G1_SECURITYID_MAXLEN` | 8 | 证券代码 |
| `G1_FUNDACCOUNTID_LEN` | 16 | 客户资金 |
| `G1_CUSTID_LEN` | 16 | 客户代码 |
| `G1_BRANCHID_LEN` | 12 | 客户营业部 |
| `G1_HOLDERACC_LEN` | 12 | 股东账户 |
| `G1_CUST_END_LEN` | 1024 | 客户终端信息 |
| `G1_SESSION_LEN` | 32 | 客户会话信息 |
| `G1_VERSION_LEN` | 64 | 版本信息 |
| `G1_IPADDR_LEN` | 16 | IP 地址 |
| `G1_ERRMSG_LEN` | 128 | 错误信息 |
| `G1_EXECID_LEN` | 16 | 成交编号 |

### 11.3 消息号字典

| 常量 | 值 | 方向 | 用途 |
|---|---|---|---|
| `G1_MSG_ORDER_REQ` | 1001 | api→后台 | 委托请求 |
| `G1_MSG_CANCEL_REQ` | 1002 | api→后台 | 撤单请求 |
| `G1_MSG_HEART_REQ` | 1003 | api→后台 | 心跳请求 |
| `G1_MSG_SEC_INFO_REQ` | 1004 | api→后台 | 证券信息请求 |
| `G1_MSG_LOGIN_REQ` | 1005 | api→后台 | 登录请求 |
| `G1_MSG_ORDER_RTN` | 2001 | 后台→api | 委托回报 |
| `G1_MSG_TRADE_RTN` | 2002 | 后台→api | 成交推送 |
| `G1_MSG_CANCEL_RSP` | 2003 | 后台→api | 撤单响应 |
| `G1_MSG_HEART_ANS` | 2004 | 后台→api | 心跳响应 |
| `G1_MSG_SEC_INFO_ANS` | 2005 | 后台→api | 证券信息推送 |
| `G1_MSG_LOGIN_ANS` | 2006 | 后台→api | 登录响应 |
| `G1_MSG_OFFLINE_PUSH` | 2007 | 后台→api | 板卡客户状态推送 |
| `G1_MSG_GW_REJ` | 2008 | 后台→api | 网关路由拒绝（头 + g1_gw_rej_head + 被拒的 order_req/cancel_req） |

### 11.4 业务体关键字段约定

| 结构体 | 字段 | 说明 |
|---|---|---|
| `login_req` / `login_ans` / `sec_info_req` / `sec_push_head` | `cust_req_no` | 客户私有请求号 |
| `order_req` / `order_rtn` / `trade_rtn` / `cancel_rsp` | `cust_req_no` | 客户私有请求号 |
| `cancel_req` | `cust_req_no` | 客户私有请求号（撤单自身） |
| `cancel_req` | `org_cust_req_no` | 原客户私有请求号（被撤委托） |

### 11.5 可靠消息流水

`order_rtn` / `trade_rtn` / `cancel_rsp` 带 `int64_t session_seq_no`：
- `>0`：可靠消息，fpga 柜台基类按此字段去重（保留最大流水号）
- `=0`：非可靠消息，不去重

RUL-32/33 后 `StreamInfo` = 柜台类型（counter_type 枚举值）+ partno（板卡号；98 为回报分区号，个微无意义填 0）+ `stream_seq`（98=分区内回报记录号 index，与 partno 组成回报去重位置；fpga 的 `session_seq_no` 去重完全内部化恒填 0）。

### 11.6 `login_ans` 关键字段

| 字段 | 类型 | 用途 |
|---|---|---|
| `cust_req_no` | int64 | 客户私有请求号 |
| `user_id` | uint16 | 用户 ID 索引（log_type=2 时返回 -1） |
| `board_no` | uint16 | 板卡号 |
| `session_id` | uint32 | 会话号（fpga 柜台回填到基类 `session_id`） |
| `trade_ip` | char[16] | 交易 IP（fpga_direct 用于建立 core 链接） |
| `trade_port` | int32 | 交易端口 |
| `log_type` | int32 | 1=用户，2=网关 |
| `err_code` | int32 | 错误码 |
| `login_time` | int64 | 登录时间，HHMMSSmmm (与 API LoginAns.login_time 同口径) |

---

## 12. 演进路径（已识别但未实施）

### 12.1 协议实现
- 98 协议真实字段（c98_*_req / c98_*_ans 完整定义）
- 个微协议真实字段（gw_order_req / gw_cancel_req / gw_query_req）
- 6 类查询的 98 应答解析
- 98 业务"账户登录成功过"前置检查（check_req_precond 的 have_cust_login，当前为注释 todo）

### 12.2 业务补全
- `build_order_msg` / `build_etf_order_msg` / `build_cancel_msg` 等 98/个微留空实现
- `deal_recv_msg` 中 6 类查询应答分发
- `deal_send_error` 完整 OrderRtn/CancelRsp 构造（98/个微）
- 个微登出请求账户校验（与个微协议落地一并处理）

### 12.3 性能优化
- 0-copy 收发（当前 dispatch_zero_copy = 0）
- 链接地址池（仅 1-2 固定地址）
- 业务线程池（当前每用户 1 引擎线程）

### 12.4 测试
- 单元测试（柜台协议解析、链接状态机）
- 集成测试（5 种配置端到端）
- 压力测试（队列满 / 重连风暴 / 协议超时）
- **登录体系专项**：S1 并发合并、断链作废在途（98/GW/core/个微四链路）、登出（direct 关 core / 个微关链 / gateway 不关链，含 offline 降级账户与在途拒绝）、下线→恢复闭环（core 自动重拨）、异账户登录拒绝（direct+个微）、gateway board/user 变化异常路径（fgw 重启重分配）、98 重复成功应答的重复回调行为评估

### 12.5 fgw 侧联动确认（联测）
- 后台契约确认：①曾成功客户在重连后的重登不拒绝（拒绝仅源于同链接重复登录）——API 侧内部重登被拒按"清状态+回调+不再重登"终局处理（不关链、无熔断）；②fgw 重启后 board/user 当日不变（变化时 API 按异常回调失败，见 §4.6 gateway）

---

**对应代码基线**：[api/src/](../src/)（libg1api.so/libg1api_sf.so 编译通过（构建方法见 BUILD.md）；个微柜台已知问题（登出未校验账户等）按约定暂不修复，见 LOGIN_COMPLIANCE 附注）
**配套文档**：[REQUIREMENTS.md](REQUIREMENTS.md)（业务需求权威）｜ [LOGIN_COMPLIANCE.md](LOGIN_COMPLIANCE.md)（实现状态矩阵）

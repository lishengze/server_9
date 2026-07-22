# API 架构设计文档 (v2.1 基线)

> **基线日期**：2026-06-17  
> **基线依据**：[api/src/](../src/) 当前代码实现  
> **配套文档**：[REQUIREMENTS.md](REQUIREMENTS.md)  
> **v2.0→v2.1 关键变更**：
> - g1 协议头改版：4 字节对齐消息号/长度，新增 `board_no`/`user_id`/`session_id` 字段（[include/g1msghead.h](../../include/g1msghead.h)）
> - g1 业务体重命名：`user_seq_no` → `cust_req_no`（统一）；`client_seq_id` → `cust_req_no`，撤单 `client_seq_id` 改名 `org_cust_req_no`（[include/g1trademsg.h](../../include/g1trademsg.h)）
> - 引入可靠消息流水 `session_seq_no`（order_rtn/trade_rtn/cancel_rsp）→ 恢复后台消息去重
> - fpga 柜台基类增加 `session_id` 成员（[api/src/fpga_counter_base.h](src/fpga_counter_base.h)）：登录应答回填，业务消息头携带
> - 链接 API 改造：`can_connect_trade()` → `can_link_connect(int16 link_type)`；`deal_link_connect(link_type)` → `deal_link_connect(link_type, have_switch)`，并基于 `have_switch` 触发 98 地址切换后 agw 重新登录
> - `acc_login_event_info.client_req_no` → `cust_req_no`（[api/src/api_event_msg.h](src/api_event_msg.h)）
> - `fpga_cust_info.user_seq_no` → `cust_req_no`

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

**链接接口约定（v2.1 新版）**：
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

### 3.3 登录数据流
```
用户
  │ login(LoginReq)
  ▼
api_impl
  │ c98_.deal_login_req(req)
  ▼
counter98
  │ 校验 agw_login_state == 2
  │ build_cust_login_event(req, info) [info.session = agw_session]
  │ 投事件到 gw_send_queue_: (LINK_EVENT_TYPE_ACCOUNT_LOGIN, LINK_TYPE_98, info)
  ▼
multi_engine 线程
  │ case LINK_EVENT_TYPE_ACCOUNT_LOGIN:
  │   if (link_type != LINK_TYPE_98) {  // fpga 极速柜台
  │     deal_cust_login(*pmlog) → fast_gw_link.connect + send
  │   } else {  // 98 账户
  │     deal_cust98_login(*pmlog) → g98_link.send(c98 登录请求)
  │   }
  ▼
[网络] → 98 柜台
  │ 98 接收 c98 登录请求
  │ 98 处理 → 返回 ACC_LOGIN_ANS
  ▼
counter98::deal_recv_msg
  │ case C98_MSG_ACC_LOGIN_ANS: deal_cust_login_ans
  │   if (err == 0):
  │     build_fast_counter_login_event → delive_fast_counter_login
  │     → 投事件 (LINK_EVENT_TYPE_ACCOUNT_LOGIN, LINK_TYPE_SPEED_TRADE/GW, info)
  │   else:
  │     build_cust_login_rtn → cb_mgr_->on_login
  ▼
multi_engine 线程
  │ 消费 fpga 登录事件
  │ 路由到 fast_gw_link 或业务线程
  ▼
fpga_counter_*::deal_log_ans (login_ans)
  │ 解析 login_ans (session, user_id, board_no, trade_ip/port)
  │ 保存到 client_info_ / clients_
  │ fpga_direct 同步触发 fpga core 链接
  │ cb_mgr_->on_login(ans)
  ▼
用户 api_callback
  │ on_login(LoginAns) ← 登录成功
```

---

## 4. 模块详细设计

### 4.1 api_interface + api_impl

#### api_interface
- 纯虚基类，定义 11 个虚方法（1 login + 5 insert/cancel + 6 query）+ start/stop + 静态工厂
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
  - `int32_t have_start`：启动状态机（0/1/2）
- 业务流程（`order_insert`）模板：
  ```cpp
  template <class TF, class TE> int32 api_impl<TF, TE>::order_insert(const OrderReq &req) {
    int32 ret = fast_.deal_order_req(req);
    if (ret == LBAPI_ERR_COUNTER_OFFLINE || ret == LBAPI_ERR_UNSUPPORTED_OP) {
      // D22: 降级到 98
      return c98_.deal_order_req(req);
    }
    return ret;
  }
  ```
- `login(req)`：直接转发到 `c98_.deal_login_req(req)`（登录流程由 98 驱动）
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
- `stop()`：关闭 timerfd + 关闭链接 + 停止线程
- `get_queue()`：返回发送队列（柜台注入用）
- `get_out_op()`：返回 link_out_op（柜台注入用）
- `add_timer_poll(mthread*)`：由 api_impl.start 调用，将 timerfd 注册到 mthread（对 fast_engine 是自身线程，对 idle 是 no-op）

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

#### idle_engine（v2 新增：占位空实现）
- **无链接、无线程、无队列**（get_queue() 返回 nullptr）
- 仅持有 `counter_` 和 `log_` 指针
- `start()` / `stop()` / `add_timer_poll()` 全部 no-op
- **存在原因**：保持 5 种 `api_impl<TF, TE>` 模板实例化时的 `TE` 类型统一
- **实际工作**：fpga_gateway 模式下，所有业务由 `multi_engine` 槽 1 的 `fast_gw_link` 处理，idle_engine 完全不工作

### 4.4 link_timer_op

- 模板参数：`TLink`（链接类型）+ `TEngine`（所属引擎）
- 创建 timerfd（CLOCK_MONOTONIC, 非阻塞）
- `init_timer(link, engine, interval)` 绑定 link + engine + 周期
- `add_timer_poll(mthread*)` 加入 epoll，开始定时循环
- `deal_event()`：读 timerfd → 决策：
  1. `link.check_heart_send()` → 投 `SEND_HEART` 事件
  2. `link.check_heart_timeout()` → 投 `SEND_HEART` 事件 + 记超时
  3. `link.check_reconnect(&need_switch)` → 投 `LINK_CONNECT` 事件
- `close()`：关闭 timerfd

### 4.5 aio_socket_link / tcpdir_link

#### 共同
- 组合模式（`aio_tcp` 和 `tcpdir_ch` 都不可继承，link 薄包装）
- 实现 `aio_ch_op<tcp_buf_ch>` / `tcpdir_ch_op` 接口
- 内嵌 `msg_cb`（`ch_recv_cb`）→ 直接转发到 `counter.deal_recv_msg`
- 持主+备地址（aio_socket），tcpdir_link 单地址
- 心跳 / 重连由 link_timer_op 驱动

#### aio_socket_link
- 内部状态机：`IDLE → CONNECTING → WORKING → CLOSING → CLOSED`
- IO 由 multi / fast 引擎的 mthread 线程驱动（epoll）
- `set_remote(addr)` 可调 1-2 次（主+备）
- `connect(need_switch, mthread*)`：异步连接
- `check_reconnect(&o_need_switch)`：检查是否需要重连 + 切地址

#### tcpdir_link
- 自带 tcpdir_stack（Solarflare 网卡 stack）
- `set_remote_ex` 接收但只保留 1 个地址
- `connect` 需先 init stack
- IO 由 fast_engine 线程 `ch_.loop_deal_recv()` 驱动

### 4.6 counter 层

#### 链接适配 API（v2.1 统一约定）
所有柜台类都实现以下两个虚接口约定（不通过 virtual，而是模板多态）：
```cpp
// 引擎在重连事件中调用, 询问该类型链接当前是否可发起 connect
bool can_link_connect(int16 link_type);

// 链接 connect 成功后, 由 link 层回调, 通知柜台做后续动作
// have_switch = 1 表示本次连接实际发生地址切换 (主→备)
int32 deal_link_connect(int16 link_type, int32 have_switch);
```
柜台对 `can_link_connect` 的实现：
| 柜台 | 行为 |
|---|---|
| `counter98` | 仅 `LINK_TYPE_98` 返回 true |
| `gw_counter_direct` | 仅 `LINK_TYPE_SPEED_TRADE` 返回 true |
| `fpga_counter_direct` | `LINK_TYPE_SPEED_TRADE` 要求 `login_state==2`；其他 type 恒 true |
| `fpga_counter_gateway` | 仅 `LINK_TYPE_SPEED_GW` 返回 true |

柜台对 `deal_link_connect(link_type, have_switch)` 的实现：
| 柜台 | have_switch=0 行为 | have_switch=1 额外动作 |
|---|---|---|
| `counter98` | 标记 `trade_link_connect_=1` + `on_link_status(0,0,1)` | 重新投 `LINK_EVENT_TYPE_AGWUSER_LOGIN` 事件，触发 agw 重新登录（参考 §6.3） |
| `gw_counter_direct` | 标记 + `on_link_status` | 不支持地址切换，参数忽略 |
| `fpga_counter_direct` | 标记 + 视 `link_type` 决定是否发证券信息请求 | `// todo : 链接地址切换, 是否重新登陆` |
| `fpga_counter_gateway` | 标记 + 若 `sec_state_==0` 发证券信息请求 + `on_link_status` | 同上 todo |

#### counter98（非模板，固定类）
- 状态：`agw_login_state`（0/1/2）+ `trade_link_connect_`（0/1）
- 队列：`gw_send_queue_`（98 业务）+ `trade_send_queue_`（极速业务）
- 业务：`deal_order_req` / `deal_etf_order_req` / `deal_bse_order_req` / `deal_cancel_req` / `deal_*_query` 10 个
- 协议：`c98msg_tmp.h` 占位（真实协议待实现）
- 登录：`deal_agw_login`（同步等待）+ `deal_login_req`（账户登录入口）+ `deal_cust_login_ans`（账户登录应答处理）
- 应答解析：`deal_recv_msg`（msg_id switch 分发到 AGW_LOGIN_ANS / ACC_LOGIN_ANS / HEART_ANS）
- 地址切换自愈：`deal_link_connect(98, 1)` → 重新发起 agw 登录

#### fpga_counter_base（模板派生类的基类）
- 公共状态：`trade_link_connect`、`sec_state_`（0/1/2）、`session_id`（**v2.1 新增**，登录应答回填）
- 公共消息构造：`build_order_msg` / `build_cancel_msg` / `build_login_msg`（log_type 参数化）
  - **v2.1**：`build_order_msg` / `build_cancel_msg` / `build_sec_info_req_msg` / `build_heart_msg` 均在头中携带 `session_id`
  - `build_login_msg` 自身不携带（登录成功后从应答回填）
- 公共应答解析：`deal_order_rtn` / `deal_trade_rtn` / `deal_cancel_rsp` / `deal_sec_info_ans` / `build_login_rtn`（多个重载）
  - **v2.1**：依据 body 的 `session_seq_no` 做可靠消息去重（>0 时去重，0 表示非可靠）
  - **v2.1**：`StreamInfo.stream_seq = rtn->session_seq_no`（与下游去重接口一致）
- 证券代码映射：`sec_map_`（hash）+ `secs_`（vector）
- 客户信息结构 `fpga_cust_info`（v2.1 重命名 `user_seq_no → cust_req_no`）

#### fpga_counter_direct（继承 fpga_counter_base，模板 TFastCounter 不用）
- 持单 `client_info_`（fpga_cust_info）
- `can_link_connect(LINK_TYPE_SPEED_TRADE) = client_info_.login_state == 2`
- `can_link_connect(LINK_TYPE_SPEED_GW) = true`（GW 始终允许连）
- `deal_cust_login` → 保存 trade_ip/port → 同步触发 fpga core 链接 → 触发 on_link_status
- v2.1：`fpga_connect_session_` 概念被 `session_id` 替代（直接由基类管理）

#### fpga_counter_gateway（继承 fpga_counter_base）
- 持多客户存储：`client_map_acc`（fund+branch → *fpga_cust_info）+ `clients_`（board+user → *fpga_cust_info）
- `login_cache_`：缓存等待证券信息的 login_ans
- `deal_log_ans`：保存 cust_info 到 hash，缓存或回调
- `deal_fpag_state`：处理 FPGA 客户状态推送
- v2.1：网关模式下没有独立 trade 链接，GW 链接 (`LINK_TYPE_SPEED_GW`) 兼作业务链接

#### gw_counter_direct（独立类，非模板）
- 协议待实现：build_*_msg 全部留空 + `// todo`
- 临时替代：build_login_msg 用 g1 login_req，build_heart_msg 用 g1_msg_head
- 接收消息：deal_recv_msg 用 g1 msg_id 分发
- v2.1：`init_gateway` 为 no-op（个微无 GW 槽）

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
  ├─[5] fast_engine_.start()                     [异步]
  │     └─ simple_thread.run() 启动业务 epoll 线程
  │
  └─[6] c98_.deal_agw_login()                    [同步阻塞, 最后一步]
        ├─ 构造 agw 登录消息 (build_agw_login_msg)
        ├─ 入 gw_send_queue_ (LINK_EVENT_TYPE_AGWUSER_LOGIN)
        ├─ gw_eng_op_->trigger_send() 唤醒 multi 线程
        ├─ multi.deal_event → deal_agw98_login → g98_link.send_msg
        ├─ 98 返回 AGW_LOGIN_ANS → counter98.deal_agwuser_login_ans
        ├─ agw_login_state ← 2 (成功) / 0 (失败)
        └─ 同步循环 sleep_us(100) + 状态检查，最长 agw_user_login_timeout 秒
```

**关键时序约束**：
- [1][2] 必须先于 [3]：先准备好回调线程 + 业务 timerfd 注册
- [3] 必须先于 [4]：98 链接要建立好，multi.start 的 epoll 才有 fd 可监听
- [4] 必须先于 [5]：fast_engine.add_timer_poll 已把业务 timerfd 注册到 multi 的 epoll_th
- [4] [5] 必须先于 [6]：**agw 登录必须最后**，因为依赖 multi 引擎线程已起来才能接收 `AGW_LOGIN_ANS` 应答
- [6] 失败 → 整体 start 返回错误，但 [4][5] 已启动的引擎不会自动停，需要用户调用 `stop()` 清理

**为什么 agw 登录必须在最后**：
- agw 登录流程：c98 构造消息 → 写入 `gw_send_queue_`（= multi 引擎的 send_queue_）→ `trigger_send()` 唤醒 → 等待 `agw_login_state=2`
- 如果 multi 引擎线程未起，入队消息不会被消费，agw 登录状态永远不更新，`deal_agw_login` 在超时后失败
- 因此 **multi.start 必须在 c98.deal_agw_login 之前**，把 agw 登录推到启动流程最末

---

## 6. 登录时序详解

### 6.1 agw 登录（start 阶段同步）
```
[3] c98_.deal_agw_login()
  ├─ 原子状态: 0→1 (CAS)
  ├─ 构造 agw 登录请求 (c98_msg_head_tmp + c98_agw_login_req)
  ├─ 入 gw_send_queue_ (LINK_EVENT_TYPE_AGWUSER_LOGIN, LINK_TYPE_98)
  ├─ gw_eng_op_->trigger_send()
  ├─ 同步循环: 100us 间隔检查 agw_login_state
  ├─ [异步] 98 收到 AGW_LOGIN_ANS → deal_agwuser_login_ans
  │   ├─ 成功: agw_login_state ← 2, 保存 agw_session
  │   └─ 失败: agw_login_state ← 0
  └─ 状态 = 2 → return LBAPI_OK
     状态 = 0 → return LBAPI_ERR_LOGIN_FAIL
     超时 (agw_user_login_timeout) → return LBAPI_ERR_LOGIN_TIMEOUT
```

### 6.2 账户登录（用户调用 login(req)）
```
[用户] login(req)
  │
  ▼
api_impl::login(req)
  │ → c98_.deal_login_req(req)
  ▼
[1] counter98::deal_login_req
  ├─ 校验 agw_login_state == 2 (否则 NOT_LOG_AGW)
  ├─ build_cust_login_event(req, info)
  │   └─ info.session ← agw_session (从 agw 登录获得)
  ├─ 入 gw_send_queue_ (LINK_EVENT_TYPE_ACCOUNT_LOGIN, LINK_TYPE_98)
  ├─ gw_eng_op_->trigger_send()
  └─ return LBAPI_OK  [用户立即返回，结果通过 on_login 通知]
  │
  ▼
[2] multi_engine.deal_event 消费
  │ case LINK_EVENT_TYPE_ACCOUNT_LOGIN
  │   link_type == LINK_TYPE_98 → deal_cust98_login
  │     ├─ c98.deal_cust_login(req, o_buf, buf_len)
  │     │   └─ 构造 c98_msg_head_tmp + c98_acc_login_req
  │     ├─ g98_link.send_msg
  │     └─ return 消息长度
  ▼
[3] 98 处理 c98 登录 → 返回 ACC_LOGIN_ANS
  │
  ▼
[4] counter98::deal_recv_msg
  │ case C98_MSG_ACC_LOGIN_ANS
  │ → deal_cust_login_ans
  │   if err != 0:
  │     build_cust_login_rtn → cb_mgr_->on_login(failure)
  │   if err == 0:
  │     build_fast_counter_login_event → 构造 fast login 事件
  │     delive_fast_counter_login(info)
  │       └─ gw 模式：入 trade_send_queue_ (LINK_TYPE_SPEED_TRADE)
  │          fpga 模式：入 gw_send_queue_ (LINK_TYPE_SPEED_GW)
  ▼
[5a] gw 路径 (single_socket_engine.deal_event)
  │ 业务线程：消费 trade_send_queue_
  │ case LINK_EVENT_TYPE_ACCOUNT_LOGIN
  │   link_type == LINK_TYPE_SPEED_TRADE → deal_cust_login
  │     ├─ fast_gw_link.connect (若 is_free)
  │     ├─ counter.deal_cust_login → build login msg (g1 协议)
  │     └─ fast_gw_link.send_msg
  ▼
[5b] fpga 路径 (multi_engine.deal_event)
  │ 多 socket 引擎：消费 gw_send_queue_ (fpga 用此队列)
  │ case LINK_EVENT_TYPE_ACCOUNT_LOGIN
  │   link_type == LINK_TYPE_SPEED_GW → deal_cust_login
  │     ├─ fast_gw_link.connect (若 is_free)
  │     ├─ counter.deal_cust_login → build login msg (g1 协议)
  │     └─ fast_gw_link.send_msg
  ▼
[6] 极速柜台处理登录 → 返回 LOGIN_ANS
  │ g1 协议 login_ans
  ▼
[7] 极速柜台 deal_recv_msg
  │ case G1_MSG_LOGIN_ANS → deal_log_ans
  │   ├─ 解析 login_ans (session, user_id, board_no, trade_ip/port)
  │   ├─ 保存到 client_info_ / clients_
  │   ├─ fpga_direct: 同步触发 fpga core 链接
  │   │   ├─ counter.delive_fpga_connect()
  │   │   ├─ core_link_.connect
  │   │   └─ on_link_status 通知用户
  │   └─ cb_mgr_->on_login(LoginAns)
  ▼
[用户] on_login(LoginAns) ← 收到最终登录结果
```

**关键不变量**：
- 5 步骤全部异步，用户 `login()` 立即返回
- 任意中间失败 → `ans_cust_login(pmlog, err_code, err_msg)` → `cb_mgr_->on_login(failure)` 通知
- 多次 `login()` 调用：每个用户独立登录，agw 仅一次
- 极速柜台离线：fpga_direct 模式下 core 链接失败 → close 业务链接 + on_link_status + on_login 失败

### 6.3 98 柜台地址切换自愈（v2.1 新增）
```
[1] 98 链接物理断开 (主地址 reconn 计数达上限)
    link_timer_op.check_reconnect → 投 LINK_CONNECT(1) 事件
    │
    ▼
[2] multi_engine 消费 LINK_CONNECT(1)
    │ 调 counter98.can_link_connect(LINK_TYPE_98) → true
    │ 调 g98_link.connect(recv_poll_num_, 1, &epoll_th_)
    │   ├─ 切到备地址, active_idx_=1
    │   ├─ 内部 ch.connect_ch(备地址)
    │   └─ 成功 → have_switch = 1 (need_switch=1 + 实际切到备)
    │   调 counter98.deal_link_connect(LINK_TYPE_98, 1)
    ▼
[3] counter98.deal_link_connect(98, 1)
    │ trade_link_connect_ = 1
    │ on_link_status(0, 0, 1)
    │ have_switch=1 → 重新投 AGWUSER_LOGIN 事件到 98 队列
    │   ├─ 入 gw_send_queue_: (LINK_EVENT_TYPE_AGWUSER_LOGIN, LINK_TYPE_98)
    │   └─ trigger_send() 唤醒 multi 线程
    ▼
[4] multi_engine deal_event → AGWUSER_LOGIN
    │ deal_agw98_login() → c98.build_agw_login_msg → g98_link.send
    ▼
[5] 98 agw 登录成功 → counter98.deal_agwuser_login_ans
    │ agw_login_state ← 2
    │ // todo : 检查已经登陆的用户, 重新发起账户登陆事件
    │   (目前未实现"已登录用户重登"机制, 需上层用户在收到
    │    on_link_status 后自行调 login())
    ▼
[6] 账户登录成功 → counter98.deal_cust_login_ans
    │ // todo : 存储已经成功登陆的用户
    │   (供未来地址切换后自动重登使用)
    │ 正常路径: 触发极速柜台登录
```

**v2.1 决策**：98 柜台是唯一实现了"地址切换自愈"的柜台。
- 个微柜台不支持地址切换, `have_switch` 始终忽略
- FPGA 柜台暂不处理（`// todo`），fpga 的地址切换会要求重新发业务但 session_id 可能失效

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
timerfd 触发 (每 check_interval 秒)
  │
  ├─ link.check_heart_send()  → true  → 投递 SEND_HEART 事件
  ├─ link.check_heart_timeout() → true → 投递 SEND_HEART + 记超时
  └─ link.check_reconnect(&need_switch)
        ├─ 链接不是 FREE 状态 → false (不重连)
        ├─ 无地址 → false
        ├─ 主地址 reconn 计数 >= max_fails → true, need_switch=0
        │   └─ 投递 LINK_CONNECT(0) 事件 (重连主地址)
        └─ 备地址有效 && 重置计数后仍到阈值 → true, need_switch=1
            └─ 投递 LINK_CONNECT(1) 事件 (切换备地址)
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

### 7.4 链接 connect 回调链（v2.1：have_switch 透传）

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
  │   have_switch = (need_switch != 0 && addr_valid_num == 2 && active_idx_ == 1) ? 1 : 0
  │   ─ 即"caller 显式请求切" + "实际切到备地址" 二者皆满足才算 1
  │
  ├─ 主地址 ch_.connect_ch(...)
  ├─ 若失败 && need_switch==0 && 有备地址:
  │   └─ 自动切到备地址再连一次 (have_switch 仍为 0, 因为非 caller 显式切)
  │
  └─ 成功 → counter_->deal_link_connect(link_type_, have_switch)
              │
              ▼
[柜台层] counter::deal_link_connect(link_type, have_switch)
  ├─ trade_link_connect_ = 1
  ├─ on_link_status(..., 1)
  └─ 柜台特定动作:
      ├─ counter98: have_switch=1 → 重新投 AGWUSER_LOGIN 事件
      ├─ gw_direct: have_switch 忽略
      └─ fpga_*: // todo 地址切换是否重新登录
```

### 7.5 链接状态变化回调
```
link.deal_ch_close() [aio_tcp 回调]
  ├─ counter.deal_link_close(link_type)
  │   ├─ trade_link_connect_ = 0
  │   ├─ cb_mgr_->on_link_status(1, 0, 0)
  │   └─ 重置 login_state = 0 (如果适用)
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
- `have_start` 三态：0/1/2
- CAS 抢占启动权
- 并发 start 不会重复启动

### 8.2 agw 登录状态机（CAS 保护）
- `agw_login_state` 三态：0/1/2
- 0→1 转换用 `atomic_cas16`
- deal_agw_login 阶段同步占用，deal_agwuser_login_ans 阶段释放
- 业务发送（deal_order_req 等）必须 agw_state==2

### 8.3 队列安全
- `send_queue_` 是 `que_mth_buf`（多写多读线程安全）
- 柜台线程调 `write_get_mth`（多写接口）
- 引擎线程调 `read_get`（单读接口）
- 柜台/引擎共享同一队列，无锁

### 8.4 链接生命周期同步
- 极速引擎定时器必须在 multi 引擎启动**后**注册（因为 timerfd 注册到 multi 的 epoll_th）
- api_impl::start 顺序：**cb_mgr → fast_engine.add_timer_poll → multi.connect_98agw → multi.start → fast_engine.start → c98.deal_agw_login**
- **关键约束**：`c98.deal_agw_login` 必须放在最后一步（依赖 multi 线程已起来才能接收 AGW_LOGIN_ANS 应答）
- 反序停止：fast_engine.stop → multi.stop → cb_mgr.stop

---

## 9. 文件清单与依赖

### 9.1 头文件（api/include/ + api/src/）
| 文件 | 角色 |
|---|---|
| `api_interface.h` | 用户 API 虚基类 |
| `api_callback.h` | 回调接口 + StreamInfo/QueryAnsCtl/err_event_type |
| `api_config.h` | 配置虚基类 + 27 个配置名常量 + 5 种类型 |
| `api_errno.h` | 39 个错误码 + api_strerror 声明 |
| `order_trade_type.h` | 业务类型（OrderReq/OrderRtn/CancelReq/CancelRsp/LoginReq/LoginAns/6 个 Query*） |
| `api_instance.h` | api_impl 模板 + 显式实例化 |
| `api_event_msg.h` | 链接类型常量 + 7 种 link_send_event 类型 + 4 种 event_info |
| `callback_manager.h` | 回调管理 + 10 种 cb_event_type |
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
- 16 个 .cpp 文件（与 .h 一一对应）
- 5 个 `api_impl` 模板显式实例化在 `api_instance.cpp` 末尾
- 链接 / timer 的模板显式实例化在 `multi_socket_engine.cpp` / `single_socket_engine.cpp` / `tcpdirect_engine.cpp` 末尾

### 9.3 外部依赖（include/ + 公共库）
- `g1msghead.h` / `g1trademsg.h`：FPGA g1 协议（外部定义，完整）
- `c98msg_tmp.h`：98 协议占位（外部定义缺失，待补）
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

### 10.5 业务降级（D22）
- 决策：极速柜台返回 `COUNTER_OFFLINE` / `UNSUPPORTED_OP` 时自动改投 98
- 收益：99 路径覆盖（fpga 故障 / 离线 → 98 接管）
- 代价：业务延迟增加（98 路径多一跳）

### 10.6 单写回调优化（fpga_gateway）
- 决策：fpga_gateway 模式 `single_writer = 1`，所有回调来自同一 multi 引擎线程
- 收益：避免多线程入队冲突
- 代价：fpga_gateway 模式仅支持 single_writer

### 10.7 counter98 非模板
- 决策：98 柜台为独立具体类（不模板化）
- 原因：98 业务（查询/降级）固定，仅一个实例
- 代价：未来若要支持 98 多种配置需重新模板化

### 10.8 have_switch 语义：caller 显式切 + 实际切（v2.1）
- 决策：`have_switch=1` 当且仅当 `need_switch=1`（caller 显式请求切）且 link 实际切到备地址
- 内部自动重试（caller 传 `need_switch=0`，主地址失败自动尝试备地址）**不**算 have_switch
- 收益：caller 可区分"主动切"和"自动重试"两种语义；柜台可对主动切做更重的处理（如 98 重新登录）
- 代价：链接层需多跟踪 `active_idx_` 与 caller 意图的差异

### 10.9 链接适配 API 模板约定（v2.1）
- 决策：4 个柜台都实现 `can_link_connect(int16 link_type)` 和 `deal_link_connect(int16 link_type, int32 have_switch)` 接口
- 不通过 virtual，而是模板多态（与现有 4.4 一致）
- 引擎在 `LINK_EVENT_TYPE_LINK_CONNECT` 分发时统一调用 `can_link_connect`，避免每个 engine 重复判断
- 收益：link 与 counter 的协议边界清晰；新增柜台时只需实现这两个接口
- 代价：仍是无 override 检查，靠文档 + 编译期多态保证一致性

### 10.10 可靠消息流水去重（v2.1）
- 决策：g1 协议 `order_rtn` / `trade_rtn` / `cancel_rsp` 增加 `session_seq_no` 字段
- 语义：>0 表示可靠消息（按此字段去重），=0 表示非可靠（不去重）
- fpga 柜台基类按 `body->session_seq_no` 做去重（保留最大流水号），`StreamInfo.stream_seq` 同步
- 收益：后台重复推送不会在 API 层被处理两次；上层可按 stream_seq 判断消息新旧
- 代价：每次 RTN 处理需多 1 次比较 + 赋值

### 10.11 session_id 字段透传（v2.1）
- 决策：g1 消息头增加 `board_no` / `user_id` / `session_id`（uint32）字段
- `fpga_counter_base.session_id` 登录应答时从 `login_ans.session_id` 回填
- 业务消息（订单/撤单/证券信息/心跳）头中携带 session_id
- 收益：后台可按 session_id 路由/校验消息；链接地址切换后若 session_id 失效，柜台可检测
- 代价：消息头由 12 字节扩为 16 字节（+33%）

---



## 11. g1 协议结构与字段约定（v2.1）

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
| `G1_MSG_MAX_LEN` | 2000 | 单消息最大长度 |
| `G1_SECURITYID_MAXLEN` | 8 | 证券代码 |
| `G1_FUNDACCOUNTID_LEN` | 16 | 客户资金 |
| `G1_CUSTID_LEN` | 16 | 客户代码 |
| `G1_BRANCHID_LEN` | 12 | 客户营业部（v2.1 由 8 改为 12） |
| `G1_HOLDERACC_LEN` | 12 | 股东账户（v2.1 新增） |
| `G1_CUST_END_LEN` | 1024 | 客户终端信息 |
| `G1_SESSION_LEN` | 32 | 客户会话信息 |
| `G1_VERSION_LEN` | 64 | 版本信息 |
| `G1_IPADDR_LEN` | 16 | IP 地址 |
| `G1_ERRMSG_LEN` | 64 | 错误信息 |
| `G1_EXECID_LEN` | 32 | 成交编号（v2.1 由 16 改为 32） |

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
| `G1_MSG_CANCEL_RSP` | 2003 | 后台→api | 撤单响应（v2.1 新增） |
| `G1_MSG_HEART_ANS` | 2004 | 后台→api | 心跳响应 |
| `G1_MSG_SEC_INFO_ANS` | 2005 | 后台→api | 证券信息推送 |
| `G1_MSG_LOGIN_ANS` | 2006 | 后台→api | 登录响应 |
| `G1_MSG_OFFLINE_PUSH` | 2007 | 后台→api | 板卡客户状态推送 |

### 11.4 业务体字段重命名（v2.1）

| 结构体 | 旧字段 | 新字段 | 说明 |
|---|---|---|---|
| `login_req` / `login_ans` / `sec_info_req` / `sec_push_head` | `user_seq_no` | `cust_req_no` | 客户私有请求号 |
| `order_req` / `order_rtn` / `trade_rtn` / `cancel_rsp` | `client_seq_id` | `cust_req_no` | 客户私有请求号 |
| `cancel_req` | `user_seq_no` | `cust_req_no` | 客户私有请求号（撤单自身） |
| `cancel_req` | `client_seq_id` | `org_cust_req_no` | 原客户私有请求号（被撤委托） |

### 11.5 可靠消息流水（v2.1）

`order_rtn` / `trade_rtn` / `cancel_rsp` 增加 `int64_t session_seq_no`：
- `>0`：可靠消息，fpga 柜台基类按此字段去重（保留最大流水号）
- `=0`：非可靠消息，不去重

`StreamInfo.stream_seq` 同步为 `session_seq_no`，供上层 `api_callback.on_order_rtn` / `on_trade_rtn` / `on_cancel_rsp` 用户判断消息新旧。

### 11.6 `login_ans` 关键字段

| 字段 | 类型 | 用途 |
|---|---|---|
| `cust_req_no` | int64 | 客户私有请求号 |
| `user_id` | uint16 | 用户 ID 索引（log_type=2 时返回 -1） |
| `board_no` | uint16 | 板卡号 |
| `session_id` | uint32 | 会话号（v2.1 fpga 柜台回填到基类 `session_id`） |
| `trade_ip` | char[16] | 交易 IP（fpga_direct 用于建立 core 链接） |
| `trade_port` | int32 | 交易端口 |
| `log_type` | int32 | 1=用户，2=网关 |
| `err_code` | int32 | 错误码 |
| `login_time` | int64 | 登录时间，秒 |

---

## 12. 演进路径（已识别但未实施）

### 12.1 协议实现
- 98 协议真实字段（c98_*_req / c98_*_ans 完整定义）
- 个微协议真实字段（gw_order_req / gw_cancel_req / gw_query_req）
- 6 类查询的 98 应答解析

### 12.2 业务补全
- `build_order_msg` / `build_etf_order_msg` / `build_cancel_msg` 等留空实现
- `deal_recv_msg` 中 6 类查询应答分发
- `deal_send_error` 完整 OrderRtn/CancelRsp 构造

### 12.3 FPGA 柜台地址切换自愈
- 当前 fpga 柜台 `deal_link_connect(have_switch=1)` 仅 `// todo` 占位
- 需考虑：切换后 fpga 柜台 session_id 是否仍有效？是否需重新发登录？
- 可参考 98 柜台自愈实现

### 12.4 98 已登录用户重登机制
- `deal_agwuser_login_ans` 中 `// todo : 检查已经登陆的用户, 重新发起账户登陆事件`
- `deal_cust_login_ans` 中 `// todo : 存储已经成功登陆的用户`
- 需要 98 柜台增加 `logged_in_users_` 容器跟踪当前已登录用户，agw 重新登录成功后批量重发

### 12.5 性能优化
- 0-copy 收发（当前 dispatch_zero_copy = 0）
- 链接地址池（仅 1-2 固定地址）
- 业务线程池（当前每用户 1 引擎线程）

### 12.6 测试
- 单元测试（柜台协议解析、链接状态机）
- 集成测试（5 种配置端到端）
- 压力测试（队列满 / 重连风暴 / 协议超时）

---

**文档版本**：v2.1  
**对应代码基线**：HEAD 提交 `3cbdc73` + 后续重构（g1 协议改版、have_switch、session_id、cust_req_no 重命名）  
**配套文档**：[REQUIREMENTS.md](REQUIREMENTS.md)

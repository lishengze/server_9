# Counter 和 Engine 层的关系与各自作用

## 一、架构概述

本文档分析了 `trunk/NewAPI/gone/api` 下的核心代码，重点解析 **Counter（柜台层）** 和 **Engine（引擎层）** 的职责关系。这是一个交易 API 客户端库，采用三层架构：用户 API 接口层 → 引擎层 → 柜台层 → 链接层。

```
┌─────────────────────────────────────────────────┐
│                  用户 API 接口层                   │
│         api_interface + api_impl<TF, TE>        │
└──────────────────┬──────────────────────────────┘
                   │ (组合)
┌──────────────────┴──────────────────────────────┐
│                 引擎层 (Engine Layer)             │
│  ┌─────────────────┐  ┌────────────────────┐   │
│  │ multi_engine    │  │ fast_engine        │   │
│  │ (控制平面 epoll) │  │ (业务平面线程)     │   │
│  └─────────────────┘  └────────────────────┘   │
└──────────────────┬──────────────────────────────┘
                   │ (回调链接)
┌──────────────────┴──────────────────────────────┐
│                 柜台层 (Counter Layer)           │
│  ┌────────────┐ ┌────────────────────────────┐ │
│  │ counter98  │ │ fpga_counter_ 系列类       │ │
│  │ (98 协议)   │ │ (g1 协议 / 个微协议)        │ │
│  └────────────┘ └────────────────────────────┘ │
└─────────────────────────────────────────────────┘
```

## 二、Counter 层（柜台层）—— 管"说什么"：协议 + 业务语义

### 2.1 Counter 成员分类

| 类名                                                           | 协议类型 | 特点说明                                                                            |
| -------------------------------------------------------------- | -------- | ----------------------------------------------------------------------------------- |
| [`counter98`](gone/api/src/counter98.h)                       | 98 协议  | 非模板固定类，恒存在，负责查询/降级业务/登录驱动                                    |
| [`fpga_counter_direct`](gone/api/src/fpga_counter_direct.h)   | g1 协议  | FPGA 直连模式，单客户，继承[`fpga_counter_base`](gone/api/src/fpga_counter_base.h) |
| [`fpga_counter_gateway`](gone/api/src/fpga_counter_gateway.h) | g1 协议  | FPGA 网关模式，多客户（双 hash 索引）                                               |
| [`gw_counter_direct`](gone/api/src/gw_counter_direct.h)       | 个微协议 | 个微直连（协议未实现，当前留空）                                                    |

### 2.2 Counter 的核心职责

**Counter 是一个无线程的纯协议处理对象**，主要职责包括：

#### 1. 业务入口校验与分发

- 方法：`deal_order_req()` / `deal_cancel_req()` / `deal_*_query()` / `deal_login_req()`
- 校验状态：登录态 (`login_state`)、链接状态 (`trade_link_connect_`)、证券信息就绪态 (`sec_state_`)、FPGA 客户状态 (`fpga_state`)

#### 2. 消息组包（发送路径）

- 方法：`build_order_msg()` / `build_cancel_msg()` / `build_login_msg()` / `build_heart_msg()`
- 对 g1 协议：构造消息头携带 `session_id`（会话 ID）、`board_no`（板卡号）、`user_id`（用户 ID）
- 写引擎的发送队列：通过 [take_req_que_mem()](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/counter98.h#L140-L153) 把 `link_send_event` + 消息体写入无锁队列，然后 `trigger_send()` 唤醒引擎

#### 3. 消息解析（接收路径）

- 方法：[`deal_recv_msg()`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/counter98.h#L87-L87) 按 `msg_id` switch 分发
  - `G1_MSG_ORDER_RTN` → 委托回报
  - `G1_MSG_TRADE_RTN` → 成交推送
  - `G1_MSG_CANCEL_RSP` → 撤单响应
  - `G1_MSG_LOGIN_ANS` → 登录应答
  - `G1_MSG_HEART_ANS` → 心跳响应
  - `G1_MSG_OFFLINE_PUSH` → 板卡状态推送

#### 4. 状态与数据管理

- 状态机：登录状态机（`agw_login_state`: 0 未登录→1 登录中→2 成功）
- 证券映射：`sec_map_` 证券代码 → 索引映射（hash 表 + vector 存储）
- 客户信息：`client_info_`（单客户）/ `clients_`（多客户的 hash 索引）

#### 5. 可靠消息去重

- 依据 [`session_seq_no`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/common/include/hash_map_mth.h#L16) 字段去重订单回报（保留最大流水号）
- v2.1 新增：`order_rtn/trade_rtn/cancel_rsp` 增加可靠消息流水字段

#### 6. 回调用户

- 解析出回报后调用 [`cb_mgr_`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/common/include/hash_map_mth.h#L282) 通知用户

### 2.3 Counter 的关键数据结构

```cpp
// FPGA 客户信息 (单客户/多客户都用此结构)
struct fpga_cust_info {
  int32_t fpga_state;     // FPGA 中状态
  int16_t login_state;    // 登录状态：0-未登录，1-登录中，2-已登录
  uint16_t user_id;       // 用户索引 ID（登录后分配）
  uint16_t board_no;      // FPGA 编号（登录后分配）
  uint32_t session_id;    // 会话 ID（v2.1: per-customer）
  char trade_ip[G1_IPADDR_LEN];  // 交易 IP（登录后从 server 获得）
  int32_t trade_port;     // 交易端口
};
```

---

## 三、Engine 层（引擎层）—— 管"怎么传"：线程 + IO+ 链接生命周期

### 3.1 Engine 成员分类

| 类名                                                           | 角色               | 持有的链接/资源                                                                                                                                                                                                                                                                                                                                                                                               |
| -------------------------------------------------------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [`multi_socket_engine`](gone/api/src/multi_socket_engine.h)   | **控制平面** | 槽 0:[`g98_link_`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.h#L109) (98 链接)槽 1: [`fast_gw_link_`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.h#L114) (极速 GW 链接)+ 2 个 [`link_timer_op`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/link_timer_op.h#L24) (定时器)+ 1 条 epoll 线程 |
| [`single_socket_engine`](gone/api/src/single_socket_engine.h) | **业务平面** | 1 条[`aio_socket_link`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/aio_socket_link.h#L30)+ 发送队列 + 1 条业务线程                                                                                                                                                                                                                                                                   |
| [`tcpdirect_engine`](gone/api/src/tcpdirect_engine.h)         | **业务平面** | 1 条[`tcpdir_link`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/tcpdir_link.h#L33)(Solarflare 内核旁路)+ 发送队列 + 1 条业务线程                                                                                                                                                                                                                                                      |
| [`idle_engine`](gone/api/src/idle_engine.h)                   | 占位空实现         | 无链接无线程，仅为模板签名统一                                                                                                                                                                                                                                                                                                                                                                                |

### 3.2 Engine 的核心职责

**Engine 是一个带线程的传输调度器**，不懂业务语义，只负责事件分发：

#### 1. 持有并管理链接

- 链接的所有权在 Engine，api_impl 不再冗余保存（避免双重释放）
- [`aio_socket_link`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/aio_socket_link.h#L30) (普通 socket): 支持主备地址切换
- [`tcpdir_link`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/tcpdir_link.h#L33) (Solarflare TCPDirect): 单地址，内核旁路高性能

#### 2. 运行 IO 线程

- [`multi`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.h#L119): `mthread` epoll 监听 5 类 fd
  - 发送队列触发 fd (eventfd)
  - 98 链接 timerfd (心跳/重连)
  - 极速 GW timerfd (fpga 模式)
  - 98 链接 fd
  - 极速 GW 链接 fd
- [`fast`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/single_socket_engine.h#L33): `simple_thread` 循环消费 send_queue

#### 3. 消费发送队列（事件分发）

[`deal_event()`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.cpp#L206-L302) 或 [`do_work()`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/single_socket_engine.h#L68) 消费 [`send_queue_`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.h#L104)，按 [`link_send_event::type`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L59) 分发：

| event type                                                                                                                 | 处理方式                                                                                                                                 |
| -------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| [`LINK_EVENT_TYPE_SEND_MSG`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L24)      | 根据`link_type`路由到对应链接发送 (`link_.send_msg()`)                                                                               |
| [`LINK_EVENT_TYPE_SEND_HEART`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L25)    | 回调柜台的`build_heart_msg()` 生成心跳包再发                                                                                           |
| [`LINK_EVENT_TYPE_LINK_CLOSE`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L26)    | 关闭链接`close_ch()`                                                                                                                   |
| [`LINK_EVENT_TYPE_LINK_CONNECT`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L27)  | 先问柜台[`can_link_connect()`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/counter98.h#L116-L121),允许则建立连接 |
| [`LINK_EVENT_TYPE_ACCOUNT_LOGIN`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L28) | 回调柜台[`deal_cust_login()`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/counter98.h#L106-L106) 生成登录包再发  |

#### 4. 心跳/重连决策

[`link_timer_op`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/link_timer_op.h#L24) timerfd 定期检查：

- `link.check_heart_send()` → 投 SEND_HEART 事件
- `link.check_heart_timeout()` → 超时标记+SEND_HEART
- `link.check_reconnect(&need_switch)` → 主地址失败次数达阈值切备地址，透传 `have_switch`

#### 5. 发送失败处理

发送消息失败时回掉柜台 [`counter.deal_send_error()`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/counter98.h#L90-L90)，让柜台构造拒单回报通知用户。

---

## 四、Counter 和 Engine 的关系

### 4.1 核心设计原则：职责正交 + 双向依赖注入 + 编译期绑定

#### 1. 接线者：api_impl

[`api_impl<TF, TE>`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_instance.h#L41) 是唯一同时持有柜台和引擎的实体：

```cpp
template <class TFastCounter, class TEngine>
class api_impl : public api_interface {
  TFastCounter fast_;              // 极速柜台
  TEngine fast_engine_;            // 极速引擎
  multi_socket_engine<TFastCounter> multi_engine_;  // 控制平面
  counter98 c98_;                  // 98 柜台
  callback_manager cb_mgr_;        // 回调管理器
};
```

**init() 阶段的双向注入**（[见 api_instance.cpp#L125-L181](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_instance.cpp#L125-L181)）：

- **引擎 → 柜台**：`fast_.init(cfg, &cb_mgr_, &log_)` 初始化柜台
- **引擎资源 ← 柜台**：`fast_.init_trade(engine.get_queue(), engine.get_out_op())` 把引擎的**发送队列**和 **link_engine_outop** 注入柜台

### 4.2 发送路径（用户线程 → 引擎线程）

```
用户线程: 
  api_impl::order_insert()
  → fast_.deal_order_req(req)  [柜台：校验状态 + 组包]
  → take_req_que_mem(data, len) 写 engine.send_queue_  [que_mth_buf 无锁队列]
  → trigger_send() wake engine
  
Engine 线程 (deal_event):
  read_get from send_queue_
  → link_.send_msg(evt->data, evt->data_len)  [通过链接发送]
  → network
  失败 → counter.deal_send_error(...) 构造拒单回报
```

**关键点**：

- 柜台的 `deal_*_req` 被**用户线程**调用（写队列端，`write_get_mth` 多写接口）
- 通过无锁队列 `que_mth_buf`（[`lb_common::que_mth_buf`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/common/include/que_mth_buf.h)）解耦
- `trigger_send()` 使用 eventfd 唤醒引擎

### 4.3 接收路径（引擎线程内回调）

```
网络收包 → aio_tcp::loop_deal_recv (epoll 线程)
  → aio_socket_link::msg_cb::deal_msg
  → counter.deal_recv_msg(pmsg, msglen, link_type)  [柜台解析]
  → switch(msg_id) dispatch 分发消息类型
  → 更新状态 (login_state/sec_state/session_seq_no)
  → deal_order_rtn() / deal_trade_rtn()
  → cb_mgr_->on_order_rtn(out)  [回调用户]
```

**关键点**：

- 柜台的 `deal_recv_msg` 被**引擎线程**回调（接收路径）
- 无锁操作：不持有任何锁，只用原子变量（[`matomic.h`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/common/include/matomic.h)）

### 4.4 接口契约：编译期模板多态（零虚函数开销）

柜台对 Engine 暴露一组约定接口，**全部通过模板参数编译期绑定**：

```cpp
// Counter 必须实现的接口（模板多态，非虚函数）
class counter_interface {
public:
  int32 deal_recv_msg(const char* buf, int32 len, int16 link_type);
  void deal_send_error(char* msg_buf, int32 msg_len, int16 link_type, int32 err_ret);
  int32 build_heart_msg(char* o_buf, int32 buf_len);
  bool can_link_connect(int16 link_type);
  int32 deal_link_connect(int16 link_type, int32 have_switch);
  void deal_link_close(int16 link_type);
  int32 deal_cust_login(const acc_login_event_info& req, char* o_buf, int32 buf_len);
};
```

这是低延迟交易 API 的关键设计（架构文档 §10.4/§10.9），**没有运行时虚函数开销**。

反向 Engine 给 Counter 用的是普通虚基类 [`link_engine_outop`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L95-L102)：

```cpp
class link_engine_outop {
public:
  virtual void deal_heart_msg_ans(int16 link_type) {}
  virtual void trigger_send() {}
  virtual void deal_close_link(int16 link_type, int32 err_code) {}
};
```

### 4.5 控制平面 vs 业务平面分工

#### multi_engine（控制平面，恒在）

- **恒在组件**：槽 0 [`g98_link_`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.h#L109) (98) + 槽 1 [`fast_gw_link_`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.h#L114) (fpga GW)
- **心跳管理**：**所有链接的心跳/重连 timerfd 都注册到它的 epoll 线程** —— 即使业务线程阻塞也不会漏心跳（架构文档 §10.2）
- **登录驱动**：负责 agw 登录、账户登录、fpga core 连接的协调

#### fast_engine（业务平面，按需存在）

- **仅处理**极速业务链接的收发（下单、撤单、回报等）
- **fpga_gateway 模式下退化**为 [`idle_engine`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/idle_engine.h#L34)（占位空实现），业务全部走 multi 槽 1
- **三种实例化**：[`single_socket_engine`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/single_socket_engine.h#L33)（C1/C3）、[`tcpdirect_engine`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/tcpdirect_engine.h#L33)（C2/C4）、`idle_engine`（C5）

### 4.6 业务降级：两层联动容错

架构文档 D22 决策：业务发送失败自动降级

```cpp
// api_impl<TF,TE>::order_insert
int32 ret = fast_.deal_order_req(req);
if (ret == LBAPI_ERR_COUNTER_OFFLINE || ret == LBAPI_ERR_UNSUPPORTED_OP) {
  return c98_.deal_order_req(req);  // 降级到 98 柜台
}
return ret;
```

**场景**：

- FPGA 柜台离线 (`COUNTER_OFFLINE`) → 自动切换到 98 柜台
- 不支持的业务 (`UNSUPPORTED_OP`) → 98 柜台接管

### 4.7 登录流程：由 counter98 驱动的复杂协作

1. **用户调用**：`	::login(req)` → `c98_.deal_login_req(req)`
2. **98 柜台校验**：检查 `agw_login_state == 2`（必须先 agw 登录）
3. **构造账户登录事件**：入队 [`LINK_EVENT_TYPE_ACCOUNT_LOGIN`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_event_msg.h#L28) 到 98 队列
4. **multi 引擎消费**：区分 `link_type != 98` → 走 fpga 路径；`== 98` → 直接发 98 链路
5. **极速柜台登录应答**：解析 `login_ans`，回填 `trade_ip/port`
6. **fpga_direct 模式同步建立 core 链接**：`delive_fpga_connect()`
7. **最终回调用户**：`cb_mgr_->on_login(LoginAns)`

**关键约束**：`c98_.deal_agw_login()` **必须在最后执行**（架构文档 §5），否则 `AGW_LOGIN_ANS` 应答无法被接收。

---

## 五、启动时序（关键路径）

### 5.1 完整的启动序列

```
api_impl::start()
  ├─[1] cb_mgr_.start()                        // 回调线程
  ├─[2] fast_engine_.add_timer_poll(multi_th)  // 业务 timerfd 加入 multi epoll
  ├─[3] multi_engine_.connect_98agw()          // 同步阻塞，98 链接 IDLE→WORKING
  ├─[4] multi_engine_.start()                  // 异步启动 epoll 线程
  ├─[5] fast_engine_.start()                   // 异步启动业务线程
  └─[6] c98_.deal_agw_login()                  // 同步阻塞 agw 登录
```

### 5.2 时序约束解释

- **[1][2]** 必须先于 `[3]`：先准备好回调线程 + 业务 timerfd 注册
- **[3]** 必须先于 `[4]`：98 链接要建立好，multi.start 的 epoll 才有 fd 可监听
- **[4]** 必须先于 `[5]`：fast_engine.add_timer_poll 已把业务 timerfd 注册到 multi 的 epoll_th
- **[4][5]** 必须先于 `[6]`：**agw 登录必须最后**，依赖 multi 引擎线程已起来才能接收 `AGW_LOGIN_ANS` 应答
- **[6] 失败**：返回错误，但 `[4][5]` 已启动的引擎不会自动停，需用户调用 `stop()` 清理

---

## 六、五种配置模板实例化

根据 [`speed_counter_type`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_config.h#L35) + [`speed_link_type`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_config.h#L34)，工厂函数创建 5 种组合：

| 配置编号 | 柜台类型 (TF)                                                                                                             | 引擎类型 (TE)                                                                                                                                 | 适用场景             |
| -------- | ------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- | -------------------- |
| C1       | [`gw_counter_direct`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/gw_counter_direct.h#L45)        | [`single_socket_engine<gw_counter_direct>`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/single_socket_engine.h#L33)   | 个微软件，单 socket  |
| C2       | [`gw_counter_direct`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/gw_counter_direct.h#L45)        | [`tcpdirect_engine<gw_counter_direct>`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/tcpdirect_engine.h#L33)           | 个微软件，TCPDirect  |
| C3       | [`fpga_counter_direct`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/fpga_counter_direct.h#L36)    | [`single_socket_engine<fpga_counter_direct>`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/single_socket_engine.h#L33) | FPGA 直连，单 socket |
| C4       | [`fpga_counter_direct`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/fpga_counter_direct.h#L36)    | [`tcpdirect_engine<fpga_counter_direct>`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/tcpdirect_engine.h#L33)         | FPGA 直连，TCPDirect |
| C5       | [`fpga_counter_gateway`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/fpga_counter_gateway.h#L150) | [`idle_engine<fpga_counter_gateway>`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/idle_engine.h#L34)                  | FPGA 网关，共享模式  |

**显式实例化**（[`api_instance.cpp#L324-L329`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/api_instance.cpp#L324-L329)）保证这 5 种组合被编译进静态库。

---

## 七、关键技术决策与设计权衡

### 7.1 职责分离（架构文档 §10.1-10.3）

- **Counter**：懂业务协议的**无线程对象**，负责组包/解包/状态机
- **Engine**：不懂业务的**传输调度器**，负责 epoll 线程/链接管理/心跳重连
- **收益**：高内聚低耦合，业务逻辑变更不影响 IO 层

### 7.2 模板多态而非虚函数（架构文档 §10.4）

- 柜台对 Engine 的接口全是编译期绑定（`template<TF, TE>`），**零虚函数开销**
- Engine 对 Counter 的反向调用用普通虚基类 `link_engine_outop`（单向虚函数，性能影响可忽略）
- **收益**：低延迟（热点路径无虚表跳转）

### 7.3 无锁队列解耦（架构文档 §8.3）

- 唯一同步原语是 [`que_mth_buf`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/common/include/que_mth_buf.h)（多写单读无锁环）
- 柜台写队列（用户线程）、引擎读队列（IO 线程），**不持有任何互斥锁**
- **收益**：高并发无锁竞争

### 7.4 控制平面集中管理心跳（架构文档 §10.2）

- 所有链接的 timerfd 都注册到 `multi` 的 epoll 线程
- **收益**：业务线程阻塞不漏心跳，统一调度简化逻辑

### 7.5 链接所有权归属 Engine（架构文档 §10.3）

- 链接（`aio_socket_link` / `tcpdir_link`）由 Engine 持有，api_impl 不冗余保存
- **收益**：避免双重释放，生命周期清晰（遵循 RAII）

### 7.6 Idle 引擎占位（架构文档 §10.1）

- FPGA 网关模式不用独立业务线程，用 `idle_engine` 占位
- **收益**：保持 5 种 `api_impl<TF, TE>` 模板签名统一，代码清晰

### 7.7 业务降级容错（架构文档 §D22）

- FPGA 柜台离线时自动降级到 98 柜台
- **收益**：99% 业务可用性（极端故障仍有降级方案）

---

## 八、总结

### 8.1 Counter vs Engine 一句话对比

| 维度                 | Counter（柜台层）               | Engine（引擎层）               |
| -------------------- | ------------------------------- | ------------------------------ |
| **角色**       | 懂业务协议的**大脑**      | 不懂语义的**骨架**       |
| **职责**       | "说什么"：组包/解包/状态机      | "怎么传"：线程/epoll/链接/心跳 |
| **是否有线程** | 否，纯对象                      | 是，至少 1 条 epoll 线程       |
| **核心数据**   | 登录态、证券映射、客户信息      | 链接指针、发送队列、timerfd    |
| **如何唤醒**   | 被用户线程/引擎线程调用         | 通过 eventfd 被用户线程唤醒    |
| **如何回调**   | 提供虚函数供 Engine 调用        | 提供虚函数供 Counter 回调      |
| **典型错误**   | COUNTER_OFFLINE、UNSUPPORTED_OP | LINK_TIMEOUT、RECONN_MAX       |

### 8.2 关系的本质

**"柜台生产事件，引擎消费事件"**：

- Counter 把业务消息打包成 `link_send_event` 写 Engine 的无锁队列
- Engine 消费队列，按类型路由到链接发送
- 两层之间用队列解耦，用编译期模板绑定消除运行时开销

### 8.3 设计目标

这套设计的核心目标是**低延迟 + 高可用 + 高扩展性**：

- 低延迟：编译期绑定 + 无锁队列 + 零拷贝收发（待优化）
- 高可用：业务降级 + 链接重连自愈（98 柜台）
- 高扩展性：模板特化 + 5 种配置 + 易于新增柜台类型

---

## 九、相关文档索引

- **架构设计**: [`trunk/NewAPI/gone/api/doc/ARCHITECTURE.md`](gone/api/doc/ARCHITECTURE.md)
- **需求规格**: [`trunk/NewAPI/gone/api/doc/REQUIREMENTS.md`](gone/api/doc/REQUIREMENTS.md)
- **协议定义**:
  - g1 协议：[`trunk/NewAPI/include/g1msghead.h`](gone/include/g1msghead.h)、[`trunk/NewAPI/include/g1trademsg.h`](gone/include/g1trademsg.h)
  - 98 协议：[`trunk/NewAPI/gone/api/src/c98msg_tmp.h`](gone/api/src/c98msg_tmp.h)（占位）
- **公共库**: [`trunk/NewAPI/common/include/`](gone/common/include/)

---

## 十、counter98 双队列设计分析

### 10.1 两个队列的定义与初始化

在 `counter98.h` 中（第 218-220 行）定义了两个独立的发送队列：

```cpp
lb_common::que_mth_buf *gw_send_queue_ = nullptr;    ///< 98柜台发送队列
lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列
```

在 `counter98.cpp` 的 `init_trade` 和 `init_gateway` 方法中（第 44-51 行）：

```cpp
void init_trade(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {
  trade_send_queue_ = que;      // 极速交易队列
  trade_eng_op_ = link_outop;
}
void init_gateway(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {
  gw_send_queue_ = que;         // 98网关队列
  gw_eng_op_ = link_outop;
}
```

### 10.2 两个队列的作用

#### **gw_send_queue_（98柜台发送队列）**

- **用途**：处理与 **98 AGW 网关** 的所有通信
- **发送的消息类型**：
  - AGW 用户登录（`LINK_EVENT_TYPE_AGWUSER_LOGIN`）- 第 573-584 行
  - 客户账户登录（`LINK_EVENT_TYPE_ACCOUNT_LOGIN`）- 第 679-692 行
  - 所有业务委托和查询（通过 `take_req_que_mem` 和 `cmt_req_que_mem`）- 第 140-157 行
  - 心跳消息
- **对应的链接类型**：`LINK_TYPE_98`

#### **trade_send_queue_（极速柜台发送队列）**

- **用途**：处理与 **极速柜台** 的直接通信（绕过 98 AGW）
- **发送的消息类型**：
  - 极速柜台的客户登录事件 - 第 755-801 行的 `delive_fast_counter_login` 方法
- **对应的链接类型**：
  - 当 `fast_counter_type_ == counter_type::gw_direct` 时，使用 `LINK_TYPE_SPEED_TRADE`
  - 否则使用 `LINK_TYPE_SPEED_GW`

### 10.3 为什么需要双队列？

从代码第 758-767 行的注释可以看出原因：

```cpp
//note : 这里对极速柜台做了特殊处理，因为个微和fpga柜台登陆的不一致。
```

**核心原因**：

1. **协议差异**：98 AGW 网关和极速柜台使用不同的通信协议
2. **链路分离**：
   - 98 AGW 链路：处理 AGW 用户登录、账户登录、普通业务委托
   - 极速链路：直接连接极速柜台，绕过 AGW，用于低延迟交易
3. **并行处理**：两个队列可以独立发送，互不阻塞，提高吞吐量
4. **降级策略**：当 98 AGW 不可用时，可以直接通过极速柜台发送

### 10.4 实际使用场景

在 `delive_fast_counter_login` 方法中（第 755-801 行）：

```cpp
if (fast_counter_type_ == counter_type::gw_direct) {
  tlink_type = LINK_TYPE_SPEED_TRADE;
  tq = trade_send_queue_;      // 使用极速队列
} else {
  tlink_type = LINK_TYPE_SPEED_GW;
  tq = gw_send_queue_;         // 使用98队列
}
```

这说明：

- 如果极速柜台是 **直连模式**（`gw_direct`），使用 `trade_send_queue_`
- 如果极速柜台是 **网关模式**，复用 `gw_send_queue_`

### 10.5 队列使用总结

| 队列                  | 作用             | 对应链路                  | 主要消息                          |
| --------------------- | ---------------- | ------------------------- | --------------------------------- |
| `gw_send_queue_`    | 98 AGW 网关通信  | `LINK_TYPE_98`          | AGW登录、账户登录、业务委托、查询 |
| `trade_send_queue_` | 极速柜台直连通信 | `LINK_TYPE_SPEED_TRADE` | 极速柜台登录、低延迟交易          |

这种设计实现了 **双通道冗余** 和 **协议隔离**，既保证了与 98 AGW 的兼容性，又支持极速柜台的低延迟需求。

---

## 十一、counter98 双队列是否同时使用分析

### 11.1 配置决定唯一模式

从 `api_instance.cpp` 第 55-113 行可以看到，系统在启动时会根据配置选择**唯一的一种**极速柜台类型：

```cpp
counter_type ct = cfg->get_fast_counter_type();
```

然后在 `counter98` 初始化时保存这个配置（第 41 行）：

```cpp
fast_counter_type_ = cfg.get_fast_counter_type();
```

### 11.2 运行时只会走一个分支

在 `delive_fast_counter_login` 方法中（第 761-767 行）：

```cpp
if (fast_counter_type_ == counter_type::gw_direct) {
  tlink_type = LINK_TYPE_SPEED_TRADE;
  tq = trade_send_queue_;      // 只使用极速队列
} else {
  tlink_type = LINK_TYPE_SPEED_GW;
  tq = gw_send_queue_;         // 只使用98队列
}
```

这是一个 **if-else** 分支，**运行时只会执行其中一个**。

### 11.3 队列初始化也是按需分配

从 `api_instance.cpp` 第 168-176 行：

```cpp
if (speed_counter_type_ == counter_type::fpga_gateway) {
  c98_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
  fast_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
} else {
  c98_.init_trade(fast_engine_.get_queue(), fast_engine_.get_out_op());
  fast_.init_trade(fast_engine_.get_queue(), fast_engine_.get_out_op());
}
fast_.init_gateway(multi_engine_.get_queue(), multi_engine_.get_out_op());
c98_.init_gateway(multi_engine_.get_queue(), multi_engine_.get_out_op());
```

可以看到：

- **所有模式**都会初始化 `gw_send_queue_`（通过 `init_gateway`）
- **只有直连模式**（`gw_direct` / `fpga_direct`）才会真正使用 `trade_send_queue_`

### 11.4 为什么定义了两个队列但只用一个？

这是为了**代码统一**和**未来扩展**：

1. **统一接口**：无论哪种模式，`counter98` 都有相同的成员变量和方法
2. **简化逻辑**：不需要条件判断来初始化不同的成员
3. **未来兼容**：如果将来需要支持模式切换或混合模式，代码结构已经就绪

### 11.5 两种模式互斥性总结

| 配置模式         | 使用的队列            | 使用的链接类型            | 是否同时使用两个队列 |
| ---------------- | --------------------- | ------------------------- | -------------------- |
| `gw_direct`    | `trade_send_queue_` | `LINK_TYPE_SPEED_TRADE` | ❌ 否                |
| `fpga_direct`  | `gw_send_queue_`    | `LINK_TYPE_SPEED_GW`    | ❌ 否                |
| `fpga_gateway` | `gw_send_queue_`    | `LINK_TYPE_SPEED_GW`    | ❌ 否                |

**答案**：两种模式**完全互斥**，运行时只会使用其中一个队列。定义两个队列是为了代码结构的统一性和未来扩展的可能性。

---

## 十二、消息回调流程详解

### 12.1 整体架构

消息回调采用**两层设计**：

1. **第一层**：柜台层（counter98/fpga_counter_direct/gw_counter_direct）→ 调用 `callback_manager` 的接口
2. **第二层**：`callback_manager` → 调用用户实现的 `api_callback` 接口

```
网络收包 → Engine线程 
  → Counter.deal_recv_msg() 解析消息
  → callback_manager.on_xxx() 
  → [直接模式] 直接调用 user_callback_->on_xxx()
  → [队列模式] 入队 → 回调线程消费 → user_callback_->on_xxx()
```

### 12.2 两种回调模式

从 [`callback_manager.h`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/callback_manager.h#L41-L42) 第 41-42 行可以看到，`callback_manager` 继承自 `simple_thread`，支持两种回调模式：

#### **模式1：直接回调模式（callback_mode::direct）**

**流程**：

```cpp
// callback_manager.cpp 第 72-76 行
void callback_manager::on_login(const LoginAns &ans) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_login(ans);  // 直接调用用户回调
    return;
  }
  // ... 队列模式逻辑
}
```

**特点**：

- **同步调用**：在 Engine 线程中直接调用用户回调函数
- **零延迟**：无队列开销，无线程切换
- **阻塞风险**：如果用户回调函数执行时间长，会阻塞 Engine 线程，影响后续消息处理
- **适用场景**：用户回调逻辑简单、执行快速（如只设置标志位、写日志）

#### **模式2：队列回调模式（callback_mode::queued）**

**流程**：

```cpp
// callback_manager.cpp 第 77-96 行
void callback_manager::on_login(const LoginAns &ans) {
  // 1. 计算事件长度
  int32_t data_len = sizeof(LoginAns);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  
  // 2. 从回调队列分配内存
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    // 队列满，记录错误日志并返回
    error_log(tlh) << "cb_queue enqueue fail..." << end_log;
    return;
  }
  
  // 3. 构造回调事件
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::login;
  head->data_len = data_len;
  *reinterpret_cast<LoginAns *>(data + sizeof(cb_event_head)) = ans;
  
  // 4. 提交事件并唤醒回调线程
  write_cmt(pos, total_len);
  trigger();  // 唤醒回调线程
}
```

**回调线程消费**（第 360-432 行）：

```cpp
void callback_manager::do_work() {
  char *data;
  while (cb_queue_.read_get(data) > 0) {
    // 1. 读取事件头
    const cb_event_head *head = reinterpret_cast<const cb_event_head *>(data);
    const char *event_data = data + sizeof(cb_event_head);
  
    // 2. 按事件类型分发
    switch (head->type) {
    case cb_event_type::login: {
      const LoginAns &ans = *reinterpret_cast<const LoginAns *>(event_data);
      user_callback_->on_login(ans);  // 调用用户回调
      break;
    }
    case cb_event_type::order_rtn: {
      // ... 委托回报分发
      break;
    }
    // ... 其他事件类型
    }
  
    // 3. 提交读取，释放队列空间
    cb_queue_.read_cmt(static_cast<int32_t>(sizeof(cb_event_head) + head->data_len));
  }
}
```

**特点**：

- **异步调用**：Engine 线程只负责入队，回调线程负责消费
- **解耦**：用户回调执行时间不影响 Engine 线程
- **有延迟**：队列开销 + 线程切换延迟
- **适用场景**：用户回调逻辑复杂、执行时间长（如数据库操作、复杂业务逻辑）

### 12.3 完整回调流程示例：委托回报

以 **委托回报（order_rtn）** 为例，展示完整的回调流程：

#### **步骤1：网络收包 → Engine 线程**

```cpp
// multi_socket_engine.cpp 或 single_socket_engine.cpp
// aio_tcp 收到数据后回调
aio_socket_link::msg_cb::deal_msg(pmsg, msglen, link_type)
  → counter.deal_recv_msg(pmsg, msglen, link_type)
```

#### **步骤2：Counter 解析消息**

以 [`fpga_counter_direct.cpp`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/fpga_counter_direct.cpp#L265-L273) 第 265-273 行为例：

```cpp
// fpga_counter_direct::deal_recv_msg
switch (head->msg_id) {
case G1_MSG_ORDER_RTN: {
  // 1. 解析委托回报消息
  OrderRtn rtn;
  StreamInfo stream;
  build_api_order_rtn(head, *cust, rtn, stream);  // 构造回调数据
  
  // 2. 调用 callback_manager 的回调接口
  cb_mgr_->on_order_rtn(stream, rtn);
  break;
}
```

#### **步骤3：callback_manager 处理**

**直接模式**（第 98-102 行）：

```cpp
void callback_manager::on_order_rtn(const StreamInfo &si, const OrderRtn &rtn) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_order_rtn(si, rtn);  // 直接调用用户回调
    return;
  }
  // ... 队列模式
}
```

**队列模式**（第 103-130 行）：

```cpp
void callback_manager::on_order_rtn(const StreamInfo &si, const OrderRtn &rtn) {
  // 1. 计算事件长度
  int32_t data_len = sizeof(StreamInfo) + sizeof(OrderRtn);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  
  // 2. 入队
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    error_log(tlh) << "cb_queue enqueue fail,event=order_rtn..." << end_log;
    return;
  }
  
  // 3. 构造回调事件
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::order_rtn;
  head->data_len = data_len;
  *reinterpret_cast<StreamInfo *>(data + sizeof(cb_event_head)) = si;
  *reinterpret_cast<OrderRtn *>(data + sizeof(cb_event_head) + sizeof(StreamInfo)) = rtn;
  
  // 4. 提交并唤醒回调线程
  write_cmt(pos, total_len);
  trigger();  // 唤醒回调线程
}
```

#### **步骤4：回调线程消费（仅队列模式）**

```cpp
// callback_manager::do_work() 第 373-378 行
case cb_event_type::order_rtn: {
  const StreamInfo &si = *reinterpret_cast<const StreamInfo *>(event_data);
  const OrderRtn &rtn = *reinterpret_cast<const OrderRtn *>(event_data + sizeof(StreamInfo));
  user_callback_->on_order_rtn(si, rtn);  // 调用用户回调
  break;
}
```

#### **步骤5：用户回调函数**

用户实现的回调函数（在用户线程中执行）：

```cpp
class MyCallback : public api_callback {
public:
  void on_order_rtn(const StreamInfo &si, const OrderRtn &rtn) override {
    // 用户业务逻辑
    printf("委托回报: 客户号=%s, 证券=%s, 状态=%d\n", 
           rtn.cust_id.data(), rtn.security_id.data(), rtn.order_status);
  }
};
```

### 12.4 回调事件类型

从 [`callback_manager.h`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/callback_manager.h#L14-L25) 第 14-25 行，支持 10 种回调事件类型：

| 事件类型           | 说明         | 触发场景                              |
| ------------------ | ------------ | ------------------------------------- |
| `login`          | 登录结果     | 98 AGW → 98 账户 → 极速柜台登录完成 |
| `order_rtn`      | 委托回报     | 柜台返回委托状态变化                  |
| `trade_rtn`      | 成交推送     | 柜台推送成交信息                      |
| `cancel_rsp`     | 撤单响应     | 撤单请求被拒绝或成功                  |
| `order_query`    | 委托查询应答 | 查询委托列表                          |
| `trade_query`    | 成交查询应答 | 查询成交列表                          |
| `fund_query`     | 资金查询应答 | 查询客户资金                          |
| `position_query` | 持仓查询应答 | 查询客户持仓                          |
| `link_status`    | 链接状态变化 | 链接建立/断开                         |
| `error`          | 通用错误     | 各类错误事件                          |

### 12.5 关键设计要点

#### **1. 单写/多写模式优化**

从 [`callback_manager.h`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/callback_manager.h#L121-L133) 第 121-133 行：

```cpp
FORCE_INLINE int64_t write_get(char *&data, int32_t len) {
  if (0 == single_writer_)
    return cb_queue_.write_get_mth(data, len);  // 多写接口（多线程安全）
  else
    return cb_queue_.write_get(data, len);      // 单写接口（无锁，更快）
}
```

- **socket_shared 模式**：所有回调来自同一线程（multi_engine），使用单写接口
- **socket_single/tcpdirect 模式**：多线程入队，使用多写接口（`mth`）

#### **2. 队列满时的处理**

队列满时不会阻塞，而是记录错误日志并丢弃事件：

```cpp
if (pos <= 0) {
  error_log(tlh) << "cb_queue enqueue fail,event=order_rtn..." << end_log;
  return;  // 丢弃事件，不阻塞
}
```

#### **3. 零拷贝设计**

回调数据直接写入队列内存，无需额外拷贝：

```cpp
char *data;
int64_t pos = write_get(data, total_len);  // 获取队列内存指针
*reinterpret_cast<OrderRtn *>(data + offset) = rtn;  // 直接写入
write_cmt(pos, total_len);  // 提交
```

### 12.6 总结

消息回调的核心流程：

```
1. 网络收包 → Engine 线程
2. Engine 调用 Counter.deal_recv_msg() 解析消息
3. Counter 解析出回报后调用 callback_manager.on_xxx()
4. callback_manager 根据模式选择：
   - 直接模式：直接调用 user_callback_->on_xxx()（Engine 线程）
   - 队列模式：入队 → trigger() 唤醒回调线程 → do_work() 消费 → user_callback_->on_xxx()（回调线程）
5. 用户回调函数执行
```

**设计优势**：

- **灵活性**：支持直接/队列两种模式，适应不同性能需求
- **低延迟**：直接模式零开销，队列模式无锁队列
- **高可用**：队列满时丢弃而非阻塞，保证系统稳定性
- **易扩展**：新增回调类型只需添加事件枚举和分发逻辑

---

## 十三、aio_socket_link 及相关链接组件详解

### 13.1 aio_socket_link 概述

[`aio_socket_link`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/aio_socket_link.h#L45) 是基于 `aio_tcp` 的非阻塞 TCP 链接组件，采用**组合模式**（Composition）而非继承，为交易系统提供可靠的底层通信通道。

#### **核心设计原则**

从 [`aio_socket_link.h`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/aio_socket_link.h#L1-L17) 第 1-17 行可以看到其设计理念：

```cpp
// 设计原则:
//   1. 组合而非继承 (aio_tcp 是 final, 不能继承)
//   2. 自身是 aio_ch_op<tcp_buf_ch>, 作为 ch 事件回调
//   3. 持有内嵌 msg_cb (ch_recv_cb<tcp_buf_ch>), 作为消息回调
//   4. 拥有 ch_ (aio_tcp) 实例, 通过 ch_.connect_ch/start_ch 完成 IO
//   5. 链接生命周期: init -> set_remote -> start_recv (connect+start) -> ... -> close_ch
```

### 13.2 核心数据结构

```cpp
template <class TCounter> class aio_socket_link : public lb_common::aio_ch_op<aio_socket_ch_t> {
private:
  aio_socket_ch_t ch_;              ///< 底层非阻塞 TCP 通道 (owned, 唯一)
  msg_cb msg_cb_;                   ///< 消息接收回调 (owned, 由 ch_ 调用)
  TCounter *counter_;               ///< 所属柜台指针 (init 阶段注入)
  int16 link_type_;                 ///< 链接类型 (LINK_TYPE_*)
  uint16 switch_reconn_num;         ///< 重连失败次数，切换地址
  int32 once_recv_len_;             ///< 一次调用recv 的输入长度
  lb_common::reconnect_ctl reconn_; ///< 重连控制器
  int32 check_interval_;            ///< 心跳和重连间隔(秒)
  int16 active_idx_;                ///< 0=primary, 1=secondary
  int16 addr_valid_num;             ///< 有效地址数 (1=仅主, 2=主备)
  lb_common::csock_addr addrs_[2];  ///< [0]=primary, [1]=secondary
  lb_common::lb_log *log_;          ///< 日志指针
};
```

### 13.3 地址管理：主备切换机制

#### **双地址设计**

```cpp
// aio_socket_link.h 第 14-17 行
// 地址管理:
//   addrs_[0] = primary, addrs_[1] = secondary
//   fast 单 socket / shared 等只用 [0]; 98 / fpga_gw 用 [0]+[1]
//   故障切换通过 check_reconnect(bool &o_need_switch) 决策, 内部切换 active_idx_
```

#### **地址设置流程**

```cpp
// aio_socket_link.cpp 第 69-82 行
int32 aio_socket_link<TCounter>::set_remote(const lb_common::csock_addr &remote) {
  if (addr_valid_num < 2) {
    addrs_[addr_valid_num] = remote;
    addr_valid_num++;
    if (addr_valid_num == 1) {
      reconn_.init(0);  // 单地址，不切换
    } else {
      reconn_.init(switch_reconn_num);  // 双地址，启用切换
    }
    return 0;
  } else {
    return LBAPI_ERR_CFG_INVALID;  // 最多 2 个地址
  }
}
```

#### **故障切换逻辑**

```cpp
// aio_socket_link.h 第 128-144 行
bool check_reconnect(int32 &o_need_switch) {
  o_need_switch = 0;
  if (!ch_.is_free() || addrs_[0].port <= 0 || addrs_[0].ip[0] == '\0') {
    return false;
  }
  if (reconn_.check_reconnect(static_cast<uint16>(check_interval_))) {
    return true;  // 主地址重连
  }
  if (addr_valid_num == 2) {
    reconn_.reset_reconnect_count();
    if (reconn_.check_reconnect(static_cast<uint16>(check_interval_))) {
      o_need_switch = 1;  // 切换到备地址
      return true;
    }
  }
  return false;
}
```

### 13.4 消息接收流程

#### **数据流**

```cpp
// aio_socket_link.h 第 10-12 行
// 数据流:
//   mthread epoll -> aio_tcp::deal_event -> loop_deal_recv ->
//     msg_cb::deal_msg(this, aio_msg) -> counter.deal_recv_msg(msg.pmsg, msg.msglen, link_type)
```

#### **消息回调实现**

```cpp
// aio_socket_link.cpp 第 25-31 行
template <class TCounter>
int32 aio_socket_link<TCounter>::msg_cb::deal_msg(aio_socket_ch_t *pch, lb_common::aio_msg &msg) {
  // 直接将消息内容转给 counter 处理
  // NOTE: aio_tcp 的 deal_recv 会在 msg 处理完后自动 cmt_buf (zero_copy=0 时)
  return owner_->counter_->deal_recv_msg(msg.pmsg, msg.msglen, owner_->link_type_);
}
```

**关键点**：

- `aio_tcp` 负责 TCP 层面的数据接收（epoll 驱动）
- `loop_deal_recv` 循环调用直到没有更多数据
- 每收到一个完整消息，回调 `msg_cb::deal_msg`
- `msg_cb` 直接转发给 `counter_->deal_recv_msg` 进行协议解析

### 13.5 链接生命周期

#### **1. 初始化阶段（init）**

```cpp
// aio_socket_link.cpp 第 48-66 行
int32 aio_socket_link<TCounter>::init(lb_common::lb_log *tlog, TCounter *tfst, int16 link_type, 
                                      int32 once_recv_len, int32 check_interval, 
                                      int32 max_same_addr_fails) {
  ch_.set_heart_interval(check_interval);
  counter_ = tfst;
  link_type_ = link_type;
  switch_reconn_num = static_cast<uint16>(max_same_addr_fails);
  once_recv_len_ = once_recv_len;
  check_interval_ = check_interval;
  active_idx_ = 0;
  addr_valid_num = 0;
  reconn_.init(0);
  log_ = tlog;
  msg_cb_.owner_ = this;
  return 0;
}
```

#### **2. 设置地址（set_remote）**

```cpp
// 最多调用两次，分别设置主地址和备地址
link.set_remote(primary_addr);    // [0] primary
link.set_remote(secondary_addr);  // [1] secondary
```

#### **3. 建立连接（connect）**

```cpp
// aio_socket_link.cpp 第 108-196 行
int32 aio_socket_link<TCounter>::connect(int32 recv_pool_num, int32 need_switch, lb_common::mthread *recv_th) {
  // 1. 状态检查
  if (ch_.is_work()) {
    return 0;  // 已经连接
  } else if (!ch_.is_free()) {
    return LBAPI_ERR_LINK_CONNECT_REPEAT;  // 状态异常
  }
  
  // 2. 地址切换（如果需要）
  if (need_switch != 0 && addr_valid_num == 2) {
    active_idx_ = 1 - active_idx_;  // 主备切换
  }
  
  // 3. 构造连接参数
  lb_common::aio_attr buf_attr;
  lb_common::channel_attr ch_attr;
  build_aio_attr_(buf_attr, ch_attr, recv_pool_num);
  
  // 4. 调用 aio_tcp 建立连接
  int32 ret = ch_.connect_ch(buf_attr, ch_attr, &msg_cb_, this, recv_th,
                             const_cast<lb_common::csock_addr *>(&active_addr_()), nullptr, 0);
  
  // 5. 主地址失败自动尝试备地址
  if (ret < 0 && need_switch == 0 && addr_valid_num == 2) {
    active_idx_ = 1 - active_idx_;
    if (addrs_[active_idx_].ip[0] != '\0' && addrs_[active_idx_].port > 0) {
      ret = ch_.connect_ch(...);
    }
  }
  
  // 6. 连接成功处理
  if (ret == 1) {
    reconn_.set_connected();
    if (NULL != recv_th) {
      ret = ch_.start_ch();  // 启动接收
    }
    ret = counter_->deal_link_connect(link_type_, have_switch);  // 通知柜台
    return 0;
  }
  
  return LBAPI_ERR_LINK_CONNECT;
}
```

#### **4. 连接参数构造**

```cpp
// aio_socket_link.cpp 第 86-105 行
void aio_socket_link<TCounter>::build_aio_attr_(lb_common::aio_attr &buf_attr, 
                                                lb_common::channel_attr &ch_attr,
                                                int32 recv_pool_num) {
  // aio_attr: 接收循环 + 心跳 + 用户数据
  buf_attr.onerecvtimes = recv_pool_num;     // 一次 epoll 唤醒最多 recv 次数
  buf_attr.oncerecvlen = once_recv_len_;     // 单次接收长度
  buf_attr.maxmsglen = 1 * 1024 * 1024;      // 最大消息长度 1MB
  buf_attr.buf_size = 2 * 1024 * 1024;       // 接收缓冲 2MB
  buf_attr.dispatch_zero_copy = 0;           // 关闭零拷贝（简化处理）
  buf_attr.heartinterval = check_interval_;  // 心跳间隔
  buf_attr.puserdata = reinterpret_cast<void *>(this);
  
  // channel_attr: 系统 socket 选项
  ch_attr.family = 1;                        // IPv4
  ch_attr.recvsockbuflen = 1 * 1024 * 1024;  // 接收 socket 缓冲 1MB
  ch_attr.sendsockbuflen = 1 * 1024 * 1024;  // 发送 socket 缓冲 1MB
  ch_attr.tcpdelayack = 1;                   // 禁用 Nagle（低延迟）
  ch_attr.sendrecvtime = 5;                  // 收发超时 5 秒
}
```

#### **5. 关闭链接（close_ch）**

```cpp
// aio_socket_link.cpp 第 198-199 行
template <class TCounter> 
void aio_socket_link<TCounter>::close_ch(int32 err) { 
  ch_.close_ch(err); 
}
```

#### **6. 链接事件回调（aio_ch_op 实现）**

```cpp
// aio_socket_link.cpp 第 201-243 行
void deal_ch_error(aio_socket_ch_t *pch, int32 err_type, int32 err_code) {
  error_log(tlh) << "get socket link error to close..." << end_log;
  ch_.close_ch(err_code);  // 出错关闭
}

void deal_ch_closing(aio_socket_ch_t *pch, int32 errcode) {
  warning_log(tlh) << "socket link is closing..." << end_log;
}

void deal_recv_stop(aio_socket_ch_t *pch) {
  info_log(tlh) << "socket link recv stop..." << end_log;
}

void deal_ch_closed(aio_socket_ch_t *pch, int32 errcode) {
  // 物理关闭: 通知 counter, 由 link_timer_op 驱动重连
  counter_->deal_link_close(link_type_);
  reconn_.set_disconnected();
  info_log(tlh) << "socket link closed end..." << end_log;
}

void deal_ch_connect(aio_socket_ch_t *pch, lb_common::csock_addr &localaddr) {
  info_log(tlh) << "socket link connected..." << end_log;
}
```

### 13.6 定时器检查接口

供 `link_timer_op` 调用的检查接口：

```cpp
// aio_socket_link.h 第 112-144 行

/// 检查是否需要发送心跳
FORCE_INLINE bool check_heart_send() {
  if (ch_.is_work())
    return ch_.check_heart_send();
  return false;
}

/// 检查心跳是否超时
FORCE_INLINE bool check_heart_timeout() {
  if (ch_.is_work())
    return ch_.check_heart_timeout();
  return false;
}

/// 检查是否需要重连
bool check_reconnect(int32 &o_need_switch) {
  // ... 实现见 13.3
}
```

### 13.7 发送数据

```cpp
// aio_socket_link.h 第 89-90 行
/// 发送数据
FORCE_INLINE int32 send_msg(char *buf, int32 len) { 
  return ch_.send_msg_fc(buf, len); 
}
```

---

## 十四、tcpdir_link：Solarflare TCPDirect 链接

### 14.1 tcpdir_link 概述

[`tcpdir_link`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/tcpdir_link.h#L42) 是基于 Solarflare TCPDirect 技术的高性能链接组件，通过内核旁路（Kernel Bypass）实现超低延迟通信。

#### **与 aio_socket_link 的关键差异**

从 [`tcpdir_link.h`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/tcpdir_link.h#L14-L18) 第 14-18 行可以看到：

```cpp
// 与 aio_socket_link 关键差异:
//   - 不使用 mthread epoll (Solarflare TCPDirect 自带 zf_mux_wait, 由 tcpdir_poll 驱动)
//   - 单一地址 (无备地址), set_remote_ex 为 no-op
//   - 自带 tcpdir_stack (Solarflare 网卡 stack), 需在 init 阶段 init_stack
//   - 引擎线程通过循环调 ch_.loop_deal_recv() 驱动接收
```

### 14.2 核心数据结构

```cpp
template <class TFastCounter> class tcpdir_link : public lb_common::tcpdir_ch_op {
private:
  lb_common::tcpdir_ch ch_;         ///< 底层 TCPDirect 通道 (owned)
  lb_common::tcpdir_stack stack_;   ///< TCPDirect stack (owned, 自包含)
  msg_cb msg_cb_;                   ///< 消息接收回调 (owned, 由 ch_ 调用)
  int16 link_type_;                 ///< 链接类型 (LINK_TYPE_*)
  uint16 check_interval_;           ///< 心跳和重连间隔(秒)
  int32 once_recv_len_;             ///< 一次调用recv 的输入长度
  TFastCounter *counter_;           ///< 所属柜台指针
  lb_common::reconnect_ctl reconn_; ///< 重连控制器
  lb_common::csock_addr addrs_;     ///< 单一地址（无备地址）
  lb_common::lb_log *log_;          ///< 日志指针
};
```

### 14.3 关键特性对比

| 特性               | aio_socket_link             | tcpdir_link                          |
| ------------------ | --------------------------- | ------------------------------------ |
| **IO 模型**  | 标准 socket + mthread epoll | Solarflare TCPDirect + zf_mux_wait   |
| **驱动方式** | epoll 事件驱动              | 引擎线程循环调用`loop_deal_recv()` |
| **地址支持** | 双地址（主备切换）          | 单地址（无备地址）                   |
| **内核旁路** | ❌ 否                       | ✅ 是                                |
| **延迟**     | 普通（微秒级）              | 超低（亚微秒级）                     |
| **适用场景** | 98 链路、FPGA 网关链路      | 极速交易链路（FPGA 直连）            |
| **网卡要求** | 普通网卡                    | Solarflare 网卡                      |

### 14.4 数据流

```cpp
// tcpdir_link.h 第 10-12 行
// 数据流:
//   引擎 do_work -> ch_.loop_deal_recv() -> msg_cb.deal_msg(this, buf, len) ->
//     counter.deal_recv_msg(buf, len, link_type)
```

**关键点**：

- 不使用 epoll，由引擎线程主动轮询
- `tcpdir_poll` 驱动 Solarflare 网卡的事件等待
- 绕过内核协议栈，直接访问网卡 DMA 区域

### 14.5 初始化与连接

```cpp
// tcpdir_link.cpp（实现文件）
int32 init(lb_common::lb_log *tlog, TFastCounter *tfst, const char *solarflare_iface, 
           int16 link_type, int32 once_recv_len, int32 check_interval, 
           int32 max_same_addr_fails);

int32 set_remote(const lb_common::csock_addr &remote);  // 单地址

int32 connect(int32 recv_pool_num, int32 need_switch = 0, 
              lb_common::mthread *recv_th = NULL);
```

**初始化流程**：

1. 调用 `tcpdir_stack::init_stack()` 初始化 Solarflare 网卡 stack
2. 设置链接参数（心跳间隔、接收长度等）
3. 设置单一远程地址
4. 建立 TCPDirect 连接

---

## 十五、link_timer_op：链接定时事件管理

### 15.1 设计目标

[`link_timer_op`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/link_timer_op.h#L39) 负责管理链接的心跳、超时检测和重连决策，是链接可靠性的关键保障组件。

#### **核心设计理念**

从 [`link_timer_op.h`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/link_timer_op.h#L1-L7) 第 1-7 行可以看到：

```cpp
// 链接定时事件类
//
// 模板参数: TLink (链接类型) + TEngine (该 link 所属工作引擎)
// 由工作引擎持有, 知道 link + engine 两个指针.
// deal_event 中调 link 的检查函数, 根据结果向 engine 投递事件.
//
// 不再由 link 内部持有, link 也不再有 on_timer() 方法.
```

### 15.2 核心数据结构

```cpp
template <class TLink, class TEngine> class link_timer_op : public lb_common::epoll_event_op {
private:
  int32 timer_fd_;  ///< timerfd 文件描述符,重复循环定时
  int32 interval_;  ///< 定时循环间隔(秒, 由 init_timer 注入)
  TLink *link_;     ///< 链接指针
  TEngine *engine_; ///< link 所属工作引擎指针
};
```

### 15.3 定时器触发流程

#### **1. 定时器初始化**

```cpp
// link_timer_op.cpp（实现文件）
int32 init_timer(TLink *link, TEngine *engine, int32 interval) {
  link_ = link;
  engine_ = engine;
  interval_ = interval;
  
  // 创建 timerfd
  timer_fd_ = timerfd_create(CLOCK_MONOTONIC, 0);
  
  // 设置定时循环
  struct itimerspec its;
  its.it_value.tv_sec = interval;      // 首次触发
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = interval;   // 后续循环
  its.it_interval.tv_nsec = 0;
  timerfd_settime(timer_fd_, 0, &its, NULL);
  
  return 0;
}
```

#### **2. 注册到 epoll**

```cpp
// link_timer_op.cpp
int32 add_timer_poll(lb_common::mthread *th) {
  // 将 timerfd 加入 epoll，水平触发
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;  // 可读 + 边缘触发
  ev.data.ptr = this;
  return th->add_fd(timer_fd_, ev);
}
```

#### **3. 定时器触发处理（deal_event）**

```cpp
// link_timer_op.cpp
void deal_event() {
  // 1. 读取 timerfd（清除触发状态）
  uint64 exp;
  read(timer_fd_, &exp, sizeof(exp));
  
  // 2. 检查心跳发送
  if (link_->check_heart_send()) {
    delive_link_event(LINK_EVENT_TYPE_SEND_HEART, 0);
  }
  
  // 3. 检查心跳超时
  if (link_->check_heart_timeout()) {
    // 超时处理逻辑（可能触发重连）
  }
  
  // 4. 检查是否需要重连
  int32 need_switch = 0;
  if (link_->check_reconnect(need_switch)) {
    delive_link_event(LINK_EVENT_TYPE_LINK_CONNECT, need_switch);
  }
}
```

#### **4. 事件投递到引擎**

```cpp
// link_timer_op.cpp
int32 delive_link_event(int16 event_type, int32 event_data) {
  // 构造 link_send_event
  int32 total_len = sizeof(link_send_event);
  char *data = nullptr;
  int64 pos = engine_->get_queue()->write_get_mth(data, total_len);
  if (pos <= 0) {
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }
  
  link_send_event *evt = reinterpret_cast<link_send_event *>(data);
  evt->link_type = link_->get_link_type();
  evt->type = event_type;
  evt->data_len = 0;
  
  engine_->get_queue()->write_cmt_mth(pos, total_len);
  engine_->trigger_send();  // 唤醒引擎线程
  
  return 0;
}
```

### 15.4 在 Engine 中的使用

从 [`multi_socket_engine.cpp`](file:///home/lsz/dev/code/work/api_trunk/trunk/NewAPI/gone/api/src/multi_socket_engine.cpp#L167-L181) 第 167-181 行可以看到：

```cpp
// 98 链接定时器
ret = g98_link_timer_.add_timer_poll(&epoll_th_);
if (ret < 0) {
  error_log(tlh) << "add 98 timer to thread epoll error..." << end_log;
  return LBAPI_ERR_ADD_TIMER;
}

// FPGA 网关链接定时器（仅 FPGA 模式）
int32 t_fast_counter = counter_->get_counter_type();
if (t_fast_counter == static_cast<int32_t>(counter_type::fpga_direct) ||
    t_fast_counter == static_cast<int32_t>(counter_type::fpga_gateway)) {
  ret = fast_gw_link_timer_.add_timer_poll(&epoll_th_);
  if (ret < 0) {
    error_log(tlh) << "add fast gateway timer to thread epoll error..." << end_log;
    return LBAPI_ERR_ADD_TIMER;
  }
}
```

### 15.5 定时器检查项

`link_timer_op` 定期检查以下三项：

| 检查项             | 方法                                    | 触发事件                         | 说明                     |
| ------------------ | --------------------------------------- | -------------------------------- | ------------------------ |
| **心跳发送** | `link_->check_heart_send()`           | `LINK_EVENT_TYPE_SEND_HEART`   | 定期发送心跳保持连接     |
| **心跳超时** | `link_->check_heart_timeout()`        | -                                | 超时未收到响应，标记异常 |
| **重连决策** | `link_->check_reconnect(need_switch)` | `LINK_EVENT_TYPE_LINK_CONNECT` | 断线重连，可能切换地址   |

---

## 十六、链接组件总结

### 16.1 组件关系图

```
┌─────────────────────────────────────────────────────────┐
│                    Engine Layer                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │multi_engine  │  │single_engine │  │tcpdirect_    │ │
│  │(epoll 线程)  │  │(单 socket)   │  │engine        │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘ │
│         │                 │                 │          │
│         ▼                 ▼                 ▼          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │link_timer_op │  │link_timer_op │  │link_timer_op │ │
│  │(98 定时器)   │  │(极速定时器)  │  │(TCPDirect 定时)│ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘ │
│         │                 │                 │          │
│         ▼                 ▼                 ▼          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │aio_socket_   │  │aio_socket_   │  │tcpdir_link   │ │
│  │link (98)     │  │link (极速)   │  │(TCPDirect)   │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘ │
│         │                 │                 │          │
│         ▼                 ▼                 ▼          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │aio_tcp       │  │aio_tcp       │  │tcpdir_ch     │ │
│  │(非阻塞 TCP)  │  │(非阻塞 TCP)  │  │(内核旁路)    │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### 16.2 组件职责对比

| 组件                      | 职责           | 特点                                 | 适用场景                   |
| ------------------------- | -------------- | ------------------------------------ | -------------------------- |
| **aio_socket_link** | 标准 TCP 链接  | 双地址主备切换、epoll 驱动、普通延迟 | 98 AGW 链路、FPGA 网关链路 |
| **tcpdir_link**     | TCPDirect 链接 | 单地址、内核旁路、超低延迟           | FPGA 直连极速交易链路      |
| **link_timer_op**   | 定时器管理     | timerfd、心跳/超时/重连检测          | 所有链接类型               |
| **aio_tcp**         | 底层非阻塞 TCP | final 类、组合模式、接收缓冲管理     | aio_socket_link 内部       |
| **tcpdir_ch**       | TCPDirect 通道 | Solarflare 专用、DMA 直接访问        | tcpdir_link 内部           |

### 16.3 设计优势

1. **组合优于继承**：`aio_tcp` 是 final 类，通过组合模式复用功能
2. **模板多态**：编译期绑定柜台类型，零虚函数开销
3. **职责分离**：链接管理（aio_socket_link）与定时管理（link_timer_op）分离
4. **高可用设计**：双地址主备切换、自动重连、心跳保活
5. **性能分层**：普通链接（aio_socket_link）与极速链接（tcpdir_link）并存

---

**文档版本**：v1.3
**更新日期**：2026-07-30
**分析基线**：HEAD + 后续重构（g1 协议改版、v2.1 规范）
**新增内容**：第十三章 aio_socket_link 组件详解、第十四章 tcpdir_link 组件详解、第十五章 link_timer_op 定时器管理、第十六章链接组件总结

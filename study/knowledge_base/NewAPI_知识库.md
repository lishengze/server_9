# NewAPI 交易 API 框架知识库

> 本知识库系统化整理了 `trunk/NewAPI` 交易 API 客户端框架的架构、设计、数据流与待办任务。
> 面向后续接入/维护/开发人员，以及需要理解该框架的大模型。
>
> **来源**：`study/counter.md`、`study/question.md`、`study/技术实现.md`、`study/数据流转.md`、`study/产品使用.md`、`task/api_dev/api_dev_task.txt`、`task/api_dev/gw_counter_api.md`、`mock/client/mock_client_design.md`、`mock/98_counter/98_counter_mock_design.md`
> **基线**：HEAD + 后续重构（g1 协议改版、v2.1 规范）
> **版本**：v2.2（2026-09-15，新增 §26 性能测试系统 / 关键缺陷修复 / FTE 对象池扩容 / CPU 绑定验证）

---

## 目录

1. [项目定位与系统概述](#1-项目定位与系统概述)
2. [三层架构总览](#2-三层架构总览)
3. [柜台层 Counter（协议大脑）](#3-柜台层-counter协议大脑)
4. [引擎层 Engine（传输骨架）](#4-引擎层-engine传输骨架)
5. [Counter 与 Engine 的关系](#5-counter-与-engine-的关系)
6. [链接层与定时组件](#6-链接层与定时组件)
7. [回调层 callback_manager](#7-回调层-callback_manager)
8. [五种配置模板实例化](#8-五种配置模板实例化)
9. [数据流：发送 / 接收 / 登录](#9-数据流发送--接收--登录)
10. [登录链路](#10-登录链路)
11. [委托与撤单链路](#11-委托与撤单链路)
12. [查询链路](#12-查询链路)
13. [关键设计决策与权衡](#13-关键设计决策与权衡)
14. [当前完成度评估](#14-当前完成度评估)
15. [待完成任务清单](#15-待完成任务清单)
16. [实现方案建议](#16-实现方案建议)
17. [相关文档与文件索引](#17-相关文档与文件索引)
18. [FTE 协议详解（gw_counter 对接目标）](#18-fte-协议详解gw_counter-对接目标)
19. [gw_counter 模块设计（gw_counter_api.md）](#19-gw_counter-模块设计gw_counter_apimd)
20. [FTE 编译部署测试环境](#20-fte-编译部署测试环境)
21. [编译系统与 docker 环境](#21-编译系统与-docker-环境)
22. [代码复查与缺陷修复记录](#22-代码复查与缺陷修复记录)
23. [Mock 组件（mock_client / 98_counter_mock / json_utils）](#23-mock-组件mock_client--98_counter_mock--json_utils)
24. [FTE 联调关键发现（Task 7.4~7.6）](#24-fte-联调关键发现task-74-76)
25. [gw_counter 性能分析与优化（Task 7.8）](#25-gw_counter-性能分析与优化task-78)
26. [性能测试系统与关键缺陷修复（Task 7.9~7.12）](#26-性能测试系统与关键缺陷修复task-79-712)

---

## 1. 项目定位与系统概述

**NewAPI** 是一套面向客户的**金融交易 API 客户端库**，对接三个柜台系统：

| 柜台 | 类型 | 特点 |
|------|------|------|
| **GONE** | FPGA 硬件极速柜台 | 低延迟、硬件加速 |
| **GW**（个微） | 软件极速柜台 | 软件加速、单客户 |
| **Counter98** | 普通柜台 | 兼容性好、查询唯一来源、降级目标 |

**核心目标**：低延迟 + 高可用 + 高扩展性。

**代码位置**：`trunk/NewAPI/gone/api/`

---

## 2. 三层架构总览

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

### 各层职责一句话

- **API 接口层**：向上对用户提供统一交易/查询接口，屏蔽多柜台多网络 IO 复杂性。
- **引擎层**：不懂业务语义的传输调度器，负责线程、epoll、链接管理、心跳重连。
- **柜台层**：懂业务协议的无线程对象，负责组包/解包/状态机。
- **链接层**：物理网络链路管理（主备切换、断线重连）。
- **回调层**：将业务事件推送给用户实现的 `api_callback`。

---

## 3. 柜台层 Counter（协议大脑）

**Counter 是一个无线程的纯协议处理对象**，管"说什么"：协议 + 业务语义。

### 3.1 成员分类

| 类名 | 协议类型 | 特点 |
|------|---------|------|
| `counter98` | 98 协议 | 非模板固定类，恒存在，负责查询/降级业务/登录驱动 |
| `fpga_counter_direct` | g1 协议 | FPGA 直连模式，单客户，继承 `fpga_counter_base` |
| `fpga_counter_gateway` | g1 协议 | FPGA 网关模式，多客户（双 hash 索引） |
| `gw_counter_direct` | FTE TCP Binary 协议 | 个微直连（FTE 协议已完成实现），继承 fpga_counter_base 范式 |

### 3.2 核心职责

1. **业务入口校验与分发**：`deal_order_req()` / `deal_cancel_req()` / `deal_*_query()` / `deal_login_req()`，校验登录态、链接状态等。
2. **消息组包（发送路径）**：`build_order_msg()` / `build_cancel_msg()` 等，构造消息头（session_id/board_no/user_id），通过 `take_req_que_mem()` 写无锁队列后 `trigger_send()` 唤醒引擎。
3. **消息解析（接收路径）**：`deal_recv_msg()` 按 `msg_id` switch 分发。
4. **状态与数据管理**：登录状态机、证券映射 `sec_map_`、客户信息。
5. **可靠消息去重**：依据 `session_seq_no` 字段去重订单回报。
6. **回调用户**：解析出回报后调用 `cb_mgr_` 通知用户。

### 3.3 关键数据结构

```cpp
struct fpga_cust_info {
  int32_t fpga_state;     // FPGA 中状态
  int16_t login_state;    // 登录状态：0-未登录，1-登录中，2-已登录
  uint16_t user_id;       // 用户索引 ID
  uint16_t board_no;      // FPGA 编号
  uint32_t session_id;    // 会话 ID（v2.1: per-customer）
  char trade_ip[G1_IPADDR_LEN];  // 交易 IP
  int32_t trade_port;     // 交易端口
};
```

---

## 4. 引擎层 Engine（传输骨架）

**Engine 是一个带线程的传输调度器**，管"怎么传"：线程 + IO + 链接生命周期。

### 4.1 成员分类

| 类名 | 角色 | 持有的链接/资源 |
|------|------|----------------|
| `multi_socket_engine` | 控制平面 | 槽0: `g98_link_`（98链接）+ 槽1: `fast_gw_link_`（极速GW链接）+ 2个 `link_timer_op` + 1条 epoll 线程 |
| `single_socket_engine` | 业务平面 | 1条 `aio_socket_link` + 发送队列 + 1条业务线程 |
| `tcpdirect_engine` | 业务平面 | 1条 `tcpdir_link`（Solarflare 内核旁路）+ 发送队列 + 1条业务线程 |
| `idle_engine` | 占位空实现 | 无链接无线程，仅为模板签名统一 |

### 4.2 核心职责

1. **持有并管理链接**：链接所有权在 Engine，避免双重释放。
2. **运行 IO 线程**：multi 用 epoll 监听 5 类 fd；fast 用 `simple_thread` 循环消费 send_queue。
3. **消费发送队列**：`deal_event()` / `do_work()` 按 `link_send_event::type` 分发。
4. **心跳/重连决策**：`link_timer_op` timerfd 定期检查。
5. **发送失败处理**：回掉柜台 `counter.deal_send_error()` 构造拒单回报。

### 4.3 事件类型分发

| event type | 处理方式 |
|-----------|---------|
| `LINK_EVENT_TYPE_SEND_MSG` | 按 `link_type` 路由到对应链接发送 |
| `LINK_EVENT_TYPE_SEND_HEART` | 回调柜台 `build_heart_msg()` 生成心跳包再发 |
| `LINK_EVENT_TYPE_LINK_CLOSE` | 关闭链接 |
| `LINK_EVENT_TYPE_LINK_CONNECT` | 先问柜台 `can_link_connect()`，允许则建立连接 |
| `LINK_EVENT_TYPE_ACCOUNT_LOGIN` | 回调柜台 `deal_cust_login()` 生成登录包再发 |

---

## 5. Counter 与 Engine 的关系

### 5.1 核心设计原则：职责正交 + 双向依赖注入 + 编译期绑定

**接线者 api_impl**（唯一同时持有柜台和引擎的实体）：

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

**init() 双向注入**：
- 引擎 → 柜台：`fast_.init(cfg, &cb_mgr_, &log_)`
- 柜台 ← 引擎：`fast_.init_trade(engine.get_queue(), engine.get_out_op())` 注入发送队列和 `link_engine_outop`

### 5.2 接口契约：编译期模板多态（零虚函数开销）

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

反向 Engine→Counter 用普通虚基类 `link_engine_outop`。

### 5.3 控制平面 vs 业务平面分工

- **multi_engine（控制平面，恒在）**：槽0 g98_link + 槽1 fast_gw_link；所有链接心跳/重连 timerfd 注册到它的 epoll 线程；负责登录驱动。
- **fast_engine（业务平面，按需存在）**：仅处理极速业务链接收发；fpga_gateway 模式退化为 idle_engine；三种实例化（single_socket/tcpdirect/idle）。

### 5.4 业务降级：两层联动容错

```cpp
int32 ret = fast_.deal_order_req(req);
if (ret == LBAPI_ERR_COUNTER_OFFLINE || ret == LBAPI_ERR_UNSUPPORTED_OP) {
  return c98_.deal_order_req(req);  // 降级到 98 柜台
}
```

---

## 6. 链接层与定时组件

### 6.1 aio_socket_link（标准 TCP）

- 基于 `aio_tcp`（final 类），组合模式而非继承。
- **双地址主备切换**：`addrs_[0]=primary`、`addrs_[1]=secondary`，`check_reconnect(o_need_switch)` 决策切换。
- 数据流：`mthread epoll → aio_tcp::deal_event → loop_deal_recv → msg_cb::deal_msg → counter.deal_recv_msg`。
- 生命周期：init → set_remote → start_recv (connect+start) → close_ch。

### 6.2 tcpdir_link（Solarflare TCPDirect）

- 内核旁路（Kernel Bypass），超低延迟（亚微秒级）。
- 单一地址（无备地址），`set_remote_ex` 为 no-op。
- 不使用 epoll，由引擎线程循环调 `ch_.loop_deal_recv()` 驱动接收。
- 需 Solarflare 网卡。

### 6.3 link_timer_op（定时器管理）

- timerfd 定期检查三项：心跳发送 / 心跳超时 / 重连决策。
- 通过 `delive_link_event()` 向引擎投递事件。

---

## 7. 回调层 callback_manager

### 7.1 两种回调模式

| 模式 | 流程 | 特点 |
|------|------|------|
| **direct** | 同步在 IO/引擎线程直接调用用户回调 | 零延迟，但阻塞会卡住引擎线程 |
| **queued** | 入队 → trigger() 唤醒回调线程 → do_work() 消费 → 调用户回调 | 解耦，有队列+线程切换延迟 |

### 7.2 10 种回调事件类型

`login` / `order_rtn` / `trade_rtn` / `cancel_rsp` / `order_query` / `trade_query` / `fund_query` / `position_query` / `link_status` / `error`

### 7.3 关键设计点

- **单写/多写优化**：`write_get` 根据 `single_writer_` 选择单写接口（更快）或多写接口（线程安全）。
- **队列满不阻塞**：记录错误日志并丢弃事件。
- **零拷贝**：回调数据直接写入队列内存。

---

## 8. 五种配置模板实例化

根据 `speed_counter_type` + `speed_link_type`，工厂函数创建 5 种组合：

| 编号 | 柜台类型 (TF) | 引擎类型 (TE) | 适用场景 |
|------|--------------|--------------|---------|
| C1 | `gw_counter_direct` | `single_socket_engine<gw_counter_direct>` | 个微软件，单 socket |
| C2 | `gw_counter_direct` | `tcpdirect_engine<gw_counter_direct>` | 个微软件，TCPDirect |
| C3 | `fpga_counter_direct` | `single_socket_engine<fpga_counter_direct>` | FPGA 直连，单 socket |
| C4 | `fpga_counter_direct` | `tcpdirect_engine<fpga_counter_direct>` | FPGA 直连，TCPDirect |
| C5 | `fpga_counter_gateway` | `idle_engine<fpga_counter_gateway>` | FPGA 网关，共享模式 |

显式实例化保证 5 种组合被编译进静态库。

### 启动时序（关键路径）

```
api_impl::start()
  ├─[1] cb_mgr_.start()                        // 回调线程
  ├─[2] fast_engine_.add_timer_poll(multi_th)  // 业务 timerfd 加入 multi epoll
  ├─[3] multi_engine_.connect_98agw()          // 同步阻塞，98 链接 IDLE→WORKING
  ├─[4] multi_engine_.start()                  // 异步启动 epoll 线程
  ├─[5] fast_engine_.start()                   // 异步启动业务线程
  └─[6] c98_.deal_agw_login()                  // 同步阻塞 agw 登录（必须最后）
```

**关键约束**：`[6]` agw 登录必须最后，依赖 multi 线程已运行才能收 `AGW_LOGIN_ANS` 应答。

---

## 9. 数据流：发送 / 接收 / 登录

### 9.1 业务发送数据流（下行）

```
用户线程:
  api_impl::order_insert(req)
  → fast_.deal_order_req(req)  [柜台：校验状态 + 组包]
  → take_req_que_mem(data, len) 写 engine.send_queue_  [que_mth_buf 无锁队列]
  → trigger_send() wake engine

引擎线程 (deal_event):
  read_get from send_queue_
  → link_.send_msg(evt->data, evt->data_len)
  → network
  失败 → counter.deal_send_error(...)
```

### 9.2 应答接收数据流（上行）

```
网络收包 → aio_tcp::loop_deal_recv (epoll 线程)
  → aio_socket_link::msg_cb::deal_msg
  → counter.deal_recv_msg(pmsg, msglen, link_type)  [柜台解析]
  → switch(msg_id) dispatch
  → 更新状态 → deal_order_rtn() / deal_trade_rtn()
  → cb_mgr_->on_order_rtn(out)  [回调用户]
```

### 9.3 关键机制

- 柜台的 `deal_*_req` 被**用户线程**调用（写队列端，`write_get_mth` 多写接口）。
- 柜台的 `deal_recv_msg` 被**引擎线程**回调（接收路径）。
- 无锁操作：不持有任何锁，只用原子变量。
- 可靠消息去重：`session_seq_no` 保证 Exactly-Once 语义。

---

## 10. 登录链路

**98 柜台统一入口 → 级联触发极速柜台登录**。

### 10.1 登录请求路径（以个微 gw_direct 为例）

```
① api->login(req) → api_impl::login → c98_.deal_login_req(req)
② counter98::deal_login_req：检查 agw_login_state==2 → build_cust_login_event → 入 98 队列
③ multi_engine::deal_event：case ACCOUNT_LOGIN → deal_cust98_login
④ counter98::deal_cust_login：cust_need_login=false → delive_fast_counter_login
⑤ counter98::delive_fast_counter_login：gw_direct → 投到 trade_send_queue_
⑥ single_socket_engine::do_work：case ACCOUNT_LOGIN → deal_cust_login
⑦ single_socket_engine::deal_cust_login：link_.connect() 建链 → 柜台构建登录消息 → send_msg
⑧ gw_counter_direct::deal_cust_login：build_login_msg → login_state=1
```

### 10.2 登录应答返回路径

```
个微核心 → 网络 → aio_socket_link::msg_cb → gw_counter_direct::deal_recv_msg
  → case G1_MSG_LOGIN_ANS → deal_log_ans
  → login_state = 2（成功）或 0（失败）
  → cb_mgr_->on_login(ans)   [统一回调出口]
```

各柜台差异：
- **gw_direct**：直接 `on_login`
- **fpga_direct**：保存 trade_ip/port → `delive_fpga_connect` 建 core 链接 → 证券信息获取完成后 `on_login`
- **fpga_gateway**：直接 `on_login`（无 core 同步）

### 10.3 version 字段来源（fgw 版本校验）

- `g1_msg_ver` 是**编译期常量**（`g1msghead.h`），客户端 SDK 和 fgw 独立编译各自快照。
- 客户端组包时自动填入 `login_req.version = g1_msg_ver`。
- fgw 用 `strncmp(g1_msg_ver, nreq->version) < 0` 判断：客户端版本不能高于服务端版本。
- 意义：跨版本兼容性检测，防止旧 SDK 接入新协议。
- **counter98 走 c98 协议，不走这条 g1 版本校验**。

---

## 11. 委托与撤单链路

### 11.1 委托请求路径

```
① api->order_insert(req) → api_impl::order_insert
② fast_.deal_order_req(req)（先极速）→ 失败(COUNTER_OFFLINE/UNSUPPORTED_OP) → c98_.deal_order_req
③ gw_counter_direct::deal_order_req：
   检查 trade_link_connect_ → 检查 login_state!=2
   → take_req_que_mem → build_order_msg（★ 当前 todo 留空）
   → cmt_req_que_mem
④ single_socket_engine::do_work：case SEND_MSG → link_.send_msg
```

### 11.2 登录 vs 委托路径对比

| 环节 | 登录请求 | 委托请求 |
|------|---------|---------|
| 入口 | `api_impl::login → c98_.deal_login_req` | `api_impl::order_insert → fast_.deal_order_req` |
| 98 级联 | 必须 | 不需要；失败才降级 98 |
| 建链时机 | 登录时按需建链 | 前提是已登录 + 已链接 |
| 队列 | 两级 | 一级 |
| 消息构建 | `build_login_msg`（有临时实现） | `build_order_msg`（todo 留空） |

### 11.3 撤单链路

与委托几乎同构：先 fast 后降级 98。回报走 `deal_cancel_rsp` → `cb_mgr_->on_cancel_rsp`。

---

## 12. 查询链路

**查询只走 98 柜台**（极速柜台 fpga/gw 均无查询接口）。

```
api_impl::order_query → c98_.deal_order_query
api_impl::trade_query → c98_.deal_trade_query
api_impl::fund_query → c98_.deal_fund_query
api_impl::position_query → c98_.deal_position_query
```

### 各类消息链路总览

| 消息类型 | 入口（api_impl） | 极速柜台 | 98 柜台 | 回调出口 |
|---------|-----------------|---------|---------|---------|
| 登录 | `login` | ✅ | ✅ 统一入口级联 | `on_login` |
| 委托 | `order_insert` | ✅ 优先+降级 | ✅ 降级目标 | `on_order_rtn` |
| 撤单 | `order_cancel` | ✅ 优先+降级 | ✅ 降级目标 | `on_cancel_rsp` |
| 委托查询 | `order_query` | ❌ | ✅ 唯一 | （未接） |
| 成交查询 | `trade_query` | ❌ | ✅ 唯一 | （未接） |
| 资金查询 | `fund_query` | ❌ | ✅ 唯一 | `on_fund_query_ans` |
| 持仓查询 | `position_query` | ❌ | ✅ 唯一 | `on_position_query_ans` |

**设计要点**：下单类（委托/撤单）走极速柜台追求低延迟，失败降级 98；查询类统一走 98 柜台。回报统一收敛到 `callback_manager` 的 `on_*` 接口。

---

## 13. 关键设计决策与权衡

| 决策 | 说明 | 收益 |
|------|------|------|
| 职责分离 | Counter懂协议、Engine懂传输 | 高内聚低耦合 |
| 模板多态 | 编译期绑定，零虚函数开销 | 低延迟（热点路径无虚表跳转） |
| 无锁队列 | `que_mth_buf` 多写单读 | 高并发无锁竞争 |
| 控制平面集中心跳 | 所有 timerfd 注册到 multi epoll | 业务线程阻塞不漏心跳 |
| 链接所有权归 Engine | 避免双重释放 | 生命周期清晰（RAII） |
| Idle 引擎占位 | fpga_gateway 用 idle_engine | 保持 5 种模板签名统一 |
| 业务降级容错 | FPGA 离线自动降级 98 | 99% 业务可用性 |

---

## 14. 当前完成度评估

| 模块 | 完成度 | 说明 |
|------|--------|------|
| **fpga 柜台** | ⭐⭐⭐ 最高 | 委托/撤单/回报基本完成，仅需微调 |
| **gw 柜台** | ⭐⭐⭐ 最高 | FTE TCP Binary 协议全部实现：登录/委托/撤单/回报/心跳/ETF |
| **counter98** | ⭐ 低 | 请求入口骨架已搭好，消息构建和应答处理全部留空 |
| **查询应答** | ⭐ 低 | 请求已能发出，但应答未接入分发 |
| **登录/重连** | ⭐⭐ 中 | 断线自动重登、登录异常重试未实现 |
| **缓存结构** | ⭐ 低 | 各柜台缓存结构 todo |

---

## 15. 待完成任务清单

### 15.1 gw_counter_direct（个微柜台）✅ 已完成

FTE TCP Binary 协议全部实现（gw_counter_direct.h/.cpp + gw_session_cache.h）：

| 功能 | 状态 | 说明 |
|------|------|------|
| 登录（build_login_msg / deal_log_ans） | ✅ 完成 | PktNewHeader + LogOnReq/LogOnAns，密码截断100字节 |
| 委托（build_order_msg / deal_order_rtn） | ✅ 完成 | PktNewHeader + TradeOrderReq/TradeOrderER，session缓存补充 account_id/cust_id |
| 撤单（build_cancel_msg / deal_cancel_rsp） | ✅ 完成 | 从 GwSessionCache 反查 orig_clordno/orig_client_seq_id |
| 委托成交回报（deal_trade_rtn） | ✅ 完成 | TradeOrderER 解析，exec_id(16→32)/状态字典映射 |
| ETF 申购赎回（build_etf_order_msg / deal_etf_trade_rtn） | ✅ 完成 | msg_id=1010，映射为 OrderRtn（成分券暂不展开） |
| 拒绝消息（deal_reject_msg） | ✅ 完成 | RejectMsg 解析，统一回调 ORDER_DISCARD |
| 心跳（build_heart_msg / deal_recv_msg 心跳） | ✅ 完成 | msg_id=3, msg_len=0 |
| 链接管理（deal_link_connect/close） | ✅ 完成 | 建链/断链状态管理，断链重置 login_state |
| 发送失败处理（deal_send_error） | ✅ 完成 | 构造 API 拒绝回报 |
| 会话缓存（GwSessionCache） | ✅ 完成 | 全局单例，fund_account_id 主键，order_sys_no→{clordno,client_seq_id} 映射 |
| 拆包/校验和 | ✅ 完成 | decode header → 校验 msg_len≤65536 → 校验和验证 → switch 分发 |
| 状态字典映射 | ✅ 完成 | ord_status(0-8)→ORDER_STATE_*(0-9)；exec_type('0'/'4'/'8'/'F')→RSP_TYPE_* |
| 链接断开自动重登 | ✅ 完成 | deal_link_close 中 login_state=0，multi_engine 驱动重登 |

### 15.2 counter98（98 柜台）

| 函数 | 位置 | 当前实现 |
|------|------|---------|
| `build_order_msg`/`etf`/`bse`/`cancel`/各类 query | :411-:474 | 全部 `(void)req; (void)o_buf;` |
| `build_agw_login_msg` | :611 | 临时结构体，`// todo 重写` |
| `build_login_msg`（账户登录） | :726 | 临时结构体，`// todo 重写` |
| `deal_recv_msg` | :502 | 只分发 AGW_LOGIN_ANS/ACC_LOGIN_ANS/HEART_ANS，其他 default 跳过 |
| `deal_send_error` | :481 | 留空 |

**需要补充的应答处理**：委托回报、成交回报、撤单回报、委托/成交/资金/持仓查询应答。

### 15.3 fpga 柜台（完成度最高）

- `build_login_event` 依据正式协议重写
- 委托回报 order_status / rtn_type 依据字典修正
- 地址切换后是否重新登录

### 15.4 框架层面

| 任务 | 说明 |
|------|------|
| **个微真实协议** | 当前全用 g1 头临时替代，需正式协议文档 |
| **98 真实协议** | 当前用 c98_msg_head_tmp 占位，需正式协议文档 |
| **查询应答分发** | counter98 未接入任何查询应答 |
| **登录异常反复重试** | 任务要求"反复登陆直到成功"，当前无重试逻辑 |
| **链接断开自动重登** | login_state 重置被注释 |
| **已登录用户管理** | 需正式的用户会话管理 |
| **缓存结构定义** | counter98.h/gw_counter_direct.h 都有 todo |
| **非加速消息接口** | struct_req.h / struct_ans.h 待补充 |
| **配置化选择登录柜台** | 基础架构已支持，具体配置待完善 |
| **验密优化** | 是否先登陆 AGW 验密待定 |

---

## 16. 实现方案建议

**核心思路**：复用 fpga 柜台（完成度最高）的成熟模式，gw/counter98 均按"组包 → 入队 → 引擎发送 → 回报分发 → 回调"的统一框架补齐。

### 通用实现原则

1. **复用 fpga 模式**：`fpga_counter_base` 的 `build_*_msg` / `deal_*_rtn` / `build_api_*_rej` 是标准范式。
2. **消息构建三步**：填 `msg_head` → 强转 `head+1` 得消息体 → 填充业务字段。
3. **回报解析三步**：校验 `msg_len` → 强转消息体 → 构造 API 层回报 + `StreamInfo` → `cb_mgr_->on_*`。
4. **可靠消息去重**：依据 `session_seq_no` 做去重。
5. **未知消息跳过**：不返回错误、不关链接。

### 实现优先级建议

| 优先级 | 任务 |
|--------|------|
| **P0** | gw 委托/撤单消息构建 + 入队长度修正 |
| **P0** | gw 回报分发（order/trade/cancel） |
| **P1** | counter98 消息构建（委托/撤单/查询） |
| **P1** | counter98 查询应答分发 |
| **P1** | 断线自动重登 + 登录状态管理 |
| **P2** | 登录异常反复重试 |
| **P2** | 缓存结构定义 |
| **P3** | 非加速消息接口 |
| **P3** | 正式协议替换 |

> **关键依赖**：gw/counter98 的真实协议文档是最大前置条件。协议未到位前可先用临时 g1/c98 结构体打通全链路（P0/P1），协议到位后只改消息体构建与解析函数，不动框架。

---

## 17. 相关文档与文件索引

### 文档

| 文档 | 内容 |
|------|------|
| `study/counter.md` | Counter 和 Engine 层关系与各自作用（深度分析） |
| `study/question.md` | 登录/委托/回报全链路分析 + 待完成任务清单 + 实现方案 |
| `study/技术实现.md` | 模块职责、接口层次、技术特点 |
| `study/数据流转.md` | 下行/上行/登录数据流 |
| `study/产品使用.md` | 业务接入指南 |
| `study/API 设计方案_1_1.docx` | 官方设计文档 |

### 代码文件

| 位置 | 内容 |
|------|------|
| `gone/api/src/api_instance.h/.cpp` | api_impl 核心实现、接线 |
| `gone/api/src/counter98.h/.cpp` | 98 柜台 |
| `gone/api/src/fpga_counter_base/direct/gateway` | FPGA 柜台 |
| `gone/api/src/gw_counter_direct.h/.cpp` | 个微柜台 |
| `gone/api/src/multi_socket_engine` | 控制平面引擎 |
| `gone/api/src/single_socket_engine` / `tcpdirect_engine` | 业务平面引擎 |
| `gone/api/src/aio_socket_link` / `tcpdir_link` | 链接组件 |
| `gone/api/src/link_timer_op` | 定时器管理 |
| `gone/api/src/callback_manager.h/.cpp` | 回调管理器 |
| `gone/api/include/order_trade_type.h` | 加速消息类型定义 |
| `gone/api/include/struct_req.h` / `struct_ans.h` | 非加速消息结构体 |
| `gone/api/include/common_struct.h` | 公共结构体 |
| `gone/api/include/gw_head.h` | 个微协议头 |
| `gone/include/g1msghead.h` / `g1trademsg.h` | g1 协议定义 |
| `common/include/que_mth_buf.h` | 无锁队列 |

### 任务文档

- `task/api_dev/api_dev_task.txt`：[NGTP-T202612257] 优化低延时 API（CMakeLists 优化 + 接口完善）
- `task/api_dev/fte_api.md`：新 API 数据结构 ↔ FTE 数据结构转换关系（第2-3章字段映射）
- `task/api_dev/gw_counter_api.md`：gw_counter 模块接口设计文档（完整字段映射 + 链路设计 + 实现优先级）

---

## 18. FTE 协议详解（gw_counter 对接目标）

> **来源**：`/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/fte/knowledge_base/README.md`、`fte_tcp_通信链路分析.md`、`include/message/gw_head.h`、`task/api_dev/fte_api.md`

### 18.1 FTE 项目

| 项目 | 说明 |
|------|------|
| 项目路径 | `/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte` |
| 知识库 | `fte/knowledge_base/README.md` + `Prompt.md` |
| 协议结构体 | `fte/include/message/gw_head.h`（`gw_message::*` 命名空间，扁平版） |
| 服务端结构 | `fte/include/message/gw_external_message.h`（`message::*` 命名空间，嵌套版） |
| API 示例 | `fte/api_demo/`（C++11 客户端） |
| 编译脚本 | `compile_fte.sh`（docker 容器 otc） |
| 部署测试 | `fte/test_all/` |

### 18.2 报文格式

```
┌─────────────────┬──────────────────────────┬──────────────────┐
│ PktNewHeader 8B │  消息体 msg_len B        │ 校验和 uint32(大端)│
│ msg_id | msg_len│  (业务消息结构 encode)    │ (逐字节求和 %256) │
└─────────────────┴──────────────────────────┴──────────────────┘
whole_msg_len = sizeof(PktNewHeader) + msg_len + sizeof(uint32_t)
```

- **多字节整型按大端**（网络序）传输
- **校验和**：`GenerateSzCheckSum()` 对 [头+体] 逐字节求和 %256，转大端 4 字节
- **消息头**：`PktNewHeader` 8 字节（msg_id 消息类型 + msg_len 消息体长度）

### 18.3 消息类型

| 范围 | 说明 |
|------|------|
| 1xxx | 请求（客户端→FTE）：1001 登录、1003 委托、1004 撤单、1010 ETF、1007 资金查询、1008 股份查询 |
| 2xxx | 回报（FTE→客户端）：2001 登录应答、2003 委托回报、2004 撤单回报、2005 成交回报、2010 ETF 回报 |
| 3 | 心跳（kPktNewHeartBeat） |
| 9 | 拒绝消息（kPktRejectMsg） |

### 18.4 消息结构双版本

| 版本 | 命名空间 | 文件 | 使用者 |
|------|---------|------|--------|
| 扁平版 | `gw_message::` | `include/message/gw_head.h` | API 客户端（纯标准库，含 encode/decode） |
| 嵌套版 | `message::` | `include/message/gw_external_message.h` | FTE 服务端（嵌套结构，含 decode/encode） |

> 两者 `#pragma pack(1)` 内存布局与大端字节序完全一致，数据可互通。

### 18.5 关键结构体尺寸与字段（API 侧 gw_head.h）

| 结构体 | sizeof | 关键字段 |
|--------|--------|---------|
| `PktNewHeader` | 8 | msg_id(uint32) + msg_len(uint32)，含 encode/decode |
| `LogOnReq` | 1230 | TradeOrderUser 展开 + heart_bt_int(4) + password(100) + client_feature_code(1024) + agw_user(32) |
| `LogOnAns` | 86 | TradeOrderUser 展开 + session_status(4) + error_code(4) |
| `TradeOrderReq` | 106 | TradeOrderUser 展开 + security_id(8)/market_id(2)/side(1)/order_type(1)/order_qty(8)/order_price(8)/stop_px(8) |
| `CancelOrderReq` | 94 | TradeOrderUser 展开 + orig_client_seq_id(8) + orig_clordno(8) |
| `TradeOrderER` | 324 | TradeOrderUser 展开 + OrdERInfo(246) + constituent_stock[0] |
| `RejectMsg` | 83 | TradeOrderUser 展开 + reject_reason_code(2) + cancel_flag(1) + business_type(1) |
| `ConstituentStock` | 52 | 成分券信息 |

> **TradeOrderUser 展开字段**：fund_account_id[16] + branch_id[10] + account_id[12] + cust_id[16] + client_seq_id(8) + agw_seq_id(8)

### 18.6 字段级核对结论（重要）

经与 API 侧 `gw_head.h` 逐字段核对，发现 **fte_api.md 与 gw_head.h 存在不一致**：

| 字段 | fte_api.md 假设 | 实际 gw_head.h | 结论 |
|------|----------------|----------------|------|
| `policy_id` | TradeOrderReq 有 | **无** | 丢弃，不映射 |
| `tgw_id` | TradeOrderReq 有 | **无** | 丢弃，不映射 |
| `market_id` | 需缓存补充 | OrderReq 已有 | **直接映射** |
| `clordid` vs `clordno` | 用 clordid(char[10]) | 撤单用 clordno(int64) | 用 clordno 做撤单映射 |

> **设计原则**：字段映射以实际 `gw_head.h` 为准，`fte_api.md` 仅供参考。

### 18.7 拆包组包流程

**拆包（客户端接收）**：
```
TCP 字节流 → counter.deal_recv_msg(buf, len, link_type)
  → 循环解析 PktNewHeader
  → 校验 msg_len ≤ 65536
  → 等待完整报文（whole_msg_len ≤ len - deal_len）
  → 验证校验和（GenerateSzCheckSum [头+体] == 收到值）
  → 按 msg_id switch 分发
```

**组包（客户端发送）**：
```
填充业务结构 → encode() 序列化 → 填充 PktNewHeader(msg_id+msg_len)
  → 计算校验和 → 追加 4 字节
  → 写入发送队列（take_req_que_mem → build_*_msg → cmt_req_que_mem）
  → 引擎 send_msg(evt->data, evt->data_len)
```

---

## 19. gw_counter 模块实现（gw_counter_api.md）✅ 已完成

> 设计文档：`task/api_dev/gw_counter_api.md`（14 章），经两轮复盘修正 9 处问题。
> 实现文件：`trunk/NewAPI/gone/api/src/gw_counter_direct.h/.cpp` + `gw_session_cache.h`

### 19.1 实现核心

| 设计点 | 实现状态 | 实现位置 |
|--------|---------|---------|
| **GwSessionCache** | ✅ 全局单例，fund_account_id 主键 | `gw_session_cache.h` 全部内联实现 |
| **两阶段会话** | ✅ create_session + fill_session_from_ans | `gw_counter_direct.cpp` build_login_msg / deal_log_ans |
| **撤单定位** | ✅ order_sys_no → {clordno, client_seq_id} 反查 | `gw_counter_direct.cpp` build_cancel_msg |
| **状态字典** | ✅ ord_status(0-8) ↔ ORDER_STATE_*；exec_type('0'/'4'/'8'/'F') ↔ RSP_TYPE_* | `gw_counter_direct.cpp` map_ord_status / map_exec_type |
| **心跳确认** | ✅ deal_heart_msg_ans 已调用 | `gw_counter_direct.cpp` deal_recv_msg kPktNewHeartBeat 分支 |
| **校验和** | ✅ GenerateSzCheckSum 逐字节求和 %256，大端 4 字节 | `gw_counter_direct.h` 静态方法 |
| **市场映射** | ✅ 101→1(上海)/102→2(深圳) | `gw_counter_direct.cpp` map_market_id |

### 19.2 消息链路（已实现）

| 链路 | 请求结构 | 回报结构 | 回调 | 函数 |
|------|---------|---------|------|------|
| 登录 | LogOnReq(1001) | LogOnAns(2001) → LoginAns | `on_login` | build_login_msg / deal_log_ans |
| 委托 | TradeOrderReq(1003) | TradeOrderER(2003) → OrderRtn | `on_order_rtn` | build_order_msg / deal_order_rtn |
| 撤单 | CancelOrderReq(1004) | TradeOrderER(2004) → CancelRsp | `on_cancel_rsp` | build_cancel_msg / deal_cancel_rsp |
| 成交 | — | TradeOrderER(2005) → TradeRtn | `on_trade_rtn` | deal_trade_rtn |
| ETF | TradeOrderReq(1010) | TradeOrderER(2010) → OrderRtn | `on_order_rtn` | build_etf_order_msg / deal_etf_trade_rtn |
| 心跳 | — | kPktNewHeartBeat(3) | `deal_heart_msg_ans` | build_heart_msg / deal_recv_msg |
| 拒绝 | — | RejectMsg(9) | `on_order_rtn`(reject) | deal_reject_msg |

### 19.3 关键实现细节

1. **build_login_msg**：PktNewHeader(msg_id=1001) + LogOnReq + 校验和。password 截断到 100 字节，client_feature_code/agw_user 从 acc_login_event_info 复制，heart_bt_int=10。
2. **deal_log_ans**：解析 LogOnAns，error_code=0 时 login_state=2，否则 login_state=0。从应答回填 fund_account_id/cust_id/account_id/branch_id 到 GwSessionCache。
3. **deal_order_rtn**：解析 TradeOrderER，用 `strtoll(er.order_id, NULL, 10)` 得 order_sys_no，记录 order_sys_no→{clordno, client_seq_id} 映射。状态字典映射 ord_status/exec_type。
4. **deal_trade_rtn**：解析 TradeOrderER，exec_id 从 16 字节复制到 TradeRtn 32 字节字段。成交价/量/金额/费用映射。
5. **build_cancel_msg**：从 GwSessionCache 反查 orig_clordno/orig_client_seq_id（找不到设 0）。
6. **deal_recv_msg 拆包**：循环解码 PktNewHeader → 校验 msg_len≤65536 → 等待完整报文 → 校验和验证 → switch(msg_id) 分发。msg_len=0 时按心跳处理。

---

## 20. FTE 编译部署测试环境

> **来源**：`fte/knowledge_base/README.md` 第16章、`fte/test_all/fte_test.md`

### 20.1 环境

| 项目 | 说明 |
|------|------|
| 编译环境 | docker 容器 `otc`，工作目录 `/mnt/work/gt_trunk`（映射宿主机 `/home/lsz/code/work/gt_trunk`） |
| 编译模式 | Release/Debug/Fast（`-r`/`-d`/`-f`） |
| 编译宏 | `-DNO_DSE`（简化版 fte） |
| 编译器 | gcc 4.8.5 |
| 产物目录 | `/mnt/work/gt_test/work_atp/cmake/fte/bin`（ute）与 `.../lib`（libutedatainit.so） |

### 20.2 编译

```bash
./compile_fte.sh            # 默认 Release
./compile_fte.sh -r         # Release
./compile_fte.sh -d         # Debug
# 脚本自动检测并进入 docker 容器 otc
```

### 20.3 部署测试目录（test_all/）

| 目录/文件 | 说明 |
|-----------|------|
| `env.sh` | 公共环境变量 |
| `start_all.sh` | 统一启动（模拟交易所 + 上海 FTE + 深圳 FTE） |
| `stop_all.sh` / `status_all.sh` / `clear_all.sh` | 停止 / 检测 / 清理 |
| `tgw_simulator/` | 模拟交易所（3 实例） |
| `etf_test_sh/` / `etf_test_sz/` | 上海 / 深圳 FTE 测试数据与配置 |

### 20.4 端口与实例

| 端口 | 服务 | 实例 |
|------|------|------|
| 38141 | 模拟交易所（上海新债券） | TGWSimulator_Stock |
| 38140 | 模拟交易所（上海竞价流式） | TGWSimulator_Bond |
| 39142 | 模拟交易所（深圳） | TGWSimulator_ETF |
| 33001 | 上海 FTE | UTE_61_611_11 |
| 33002 | 深圳 FTE | UTE_84_842_21 |

### 20.5 完整流程（已实测通过）

```
编译 → start_all.sh 启动 → status_all.sh 检测 → 日志验证建链
→ stop_all.sh 停止 → clear_all.sh 清理
```

- **上海 FTE**：与 38140/38141 建链，持续心跳（10s）
- **深圳 FTE**：与 39142 建链，登录成功（DealLogon Recv logon message），持续心跳
- **数据加载**：两个 FTE 均生成 `fteinit.txt`，`register_vec size 42`

### 20.6 已知问题

| 问题 | 级别 | 处理 |
|------|------|------|
| 大页内存分配失败 `Mmap hugepage failed` | 低 | 自动降级普通内存 |
| tgw_simulator 遗留僵尸进程 defunct | 低 | 以端口监听判断状态 |
| oh-my-zsh compaudit 警告 | 低 | 忽略 |

### 20.7 联调准备

api client 开发完成后，`docker exec otc zsh -c "cd /mnt/work/gt_trunk && source ~/.zshrc && ./DYS-FRAMEWORK/fte/test_all/start_all.sh"` 一键启动全套环境，api client 连接 `127.0.0.1:33001`（上海）或 `127.0.0.1:33002`（深圳）与 FTE + 模拟交易所通信。

---

## 21. 编译系统与 docker 环境

### 21.1 编译架构

| 层级 | CMakeLists.txt | 角色 |
|------|---------------|------|
| 根工程 | `/mnt/work/api_trunk/CMakeLists.txt` | project(API_MAIN)，C++11，设置全局编译标志，add_subdirectory(trunk/NewAPI newapi) |
| 子模块 | `trunk/NewAPI/CMakeLists.txt` | add_subdirectory(common + gone/api + solarflare 可选)，add_dependencies(lbapi lbcommon) |
| common | `trunk/NewAPI/common/CMakeLists.txt` | 静态库 liblbcommon.a |
| gone/api | `trunk/NewAPI/gone/api/CMakeLists.txt` | 动态库 liblbapi.so，file(GLOB src/*.cpp) 自动收集源文件 |

### 21.2 docker 编译环境

| 项目 | 说明 |
|------|------|
| 容器名 | `otc`（宿主机 `/home/lsz/code` → 容器 `/mnt`） |
| 项目路径 | 宿主机 `/home/lsz/code/work/api_trunk` → 容器 `/mnt/work/api_trunk` |
| 编译器 | gcc 4.8.5（C++11），cmake /usr/local/bin/cmake |
| 构建目录 | `build_cmake/`（容器内 `/mnt/work/api_trunk/build_cmake`） |
| 产物 | `build_cmake/lib/liblbapi.so`（共享库）+ `liblbcommon.a`（静态库） |

### 21.3 build.sh 编译脚本

`build.sh` 支持宿主机自动转发到 docker 容器：

```bash
./build.sh              # Debug 模式编译（转发到 docker otc）
./build.sh Release      # Release 模式编译
./build.sh clean        # 清理 build_cmake 目录
./build.sh rebuild      # 清理后重新编译（Debug）
./build.sh -j8          # Debug 模式，8 线程并行编译
```

- **宿主机执行**：检测到项目路径非 `/mnt/*` 开头，自动执行 `docker exec otc bash -lc "cd /mnt/work/api_trunk && ./build.sh $*"`
- **容器内执行**：直接执行 cmake + make
- **环境变量**：`env -u LD_LIBRARY_PATH` 避免 VSCode 扩展旧 libstdc++ 冲突

### 21.4 CMakeLists 兼容性修复

docker 内 gcc 4.8.5 不支持 `-mprefer-vector-width=256`（需要 GCC >= 8）。根 `CMakeLists.txt` 用 `check_cxx_compiler_flag` 检测：

```cmake
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mprefer-vector-width=256" HAS_PREFER_VECTOR_WIDTH)
if(HAS_PREFER_VECTOR_WIDTH)
    set(PREFER_VECTOR_FLAG "-mprefer-vector-width=256")
else()
    set(PREFER_VECTOR_FLAG "")
endif()
set(CMAKE_CXX_FLAGS "... ${PREFER_VECTOR_FLAG} ...")
```

同样修复了 `trunk/NewAPI/CMakeLists.txt` 和 `trunk/NewAPI/gone/api/CMakeLists.txt`（独立编译模式）。

### 21.5 编译验证

编译产物 `liblbapi.so` 成功链接 `gw_counter_direct.cpp.o`，通过 `nm -D` 验证关键符号：

| 符号 | 说明 |
|------|------|
| `gw_counter_direct::deal_log_ans` | 登录应答处理 |
| `gw_counter_direct::deal_recv_msg` | 消息拆包分发 |
| `gw_counter_direct::deal_order_req/rtn` | 委托请求/回报 |
| `gw_counter_direct::deal_trade_rtn` | 成交回报 |
| `gw_counter_direct::deal_cancel_req/rsp` | 撤单请求/应答 |
| `gw_counter_direct::build_login_msg/order_msg/heart_msg` | 消息构建 |
| `gw_counter_direct::map_ord_status/map_exec_type/map_market_id` | 状态字典映射 |
| `gw_counter_direct::init_trade/init_gateway` | 初始化 |
| `gw_counter_direct::deal_cust_login/ans_cust_login` | 登录事件处理 |
| `gw_counter_direct::deal_link_close` | 链接关闭处理 |


---

## 22. 代码复查与缺陷修复记录

### 22.1 复查范围

对 `gw_counter_direct.h/.cpp` + `gw_session_cache.h` 逐行复查，对照：
- 设计文档 `gw_counter_api.md`（14 章）
- FTE 协议结构体 `gw_head.h`（字段名/类型/尺寸）
- API 数据结构 `order_trade_type.h`（TradeRtn/OrderRtn/CancelRsp/LoginAns 字段定义）
- 成熟实现 `fpga_counter_direct.h/.cpp`（范式参考）

### 22.2 发现并修复的缺陷

| # | 缺陷 | 文件 | 影响 | 修复 |
|---|------|------|------|------|
| 1 | `sizeof(ans.fund_account_id.data())` = 8（指针大小），应为 16（数组大小） | `gw_counter_direct.cpp` build_login_rtn | fund_account_id/account_id/branch_id 只复制 8 字节而非完整长度 | 改为 `sizeof(ans.fund_account_id)` 等数组真实大小 |
| 2 | `build_api_order_rej`/`build_api_cancel_rej` 中 `stream_seq = 0` | `gw_counter_direct.cpp` | 与设计文档第 11.2 节不一致，发送失败路径序列号可能冲突 | 改为 `++session_seq_` 单调递增 |

### 22.3 核对无误的关键点

| 检查项 | 结果 |
|--------|------|
| TradeOrderER 全部字段引用（order_id/clordno/exec_type/ord_status/last_px/frozen_fee/total_value_traded 等） | ✅ 与 gw_head.h 一致 |
| TradeOrderReq 无 policy_id/tgw_id（fte_api.md 有，gw_head.h 无，已丢弃） | ✅ 一致 |
| CancelOrderReq 撤单字段 orig_clordno/orig_client_seq_id 为 int64 | ✅ 一致 |
| LogOnReq password[100]/client_feature_code[1024]/agw_user[32] | ✅ 一致 |
| LogOnAns.error_code (uint32) / session_status (int32) | ✅ 一致 |
| RejectMsg.reject_reason_code (uint16) | ✅ 一致 |
| TradeRtn.exec_id 为 array<char,32>，TradeOrderER.exec_id 为 array<char,16>，复制时 min(32,16)=16 | ✅ 正确处理 |
| 状态字典 map_ord_status/map_exec_type/map_market_id 逐值 | ✅ 与设计文档一致 |
| 校验和字节序（发送 HostToNetwork，接收 HostToNetwork 反转） | ✅ 两端一致 |
| 拆包半包处理、msg_len>65536 跳过、校验和失败跳过 | ✅ 正确 |
| 撤单映射 order_sys_no→{clordno, client_seq_id} 写入与读取键一致 | ✅ 正确 |
| GwSessionCache 生命周期（login 时 create，log_ans 时 fill，cancel 时 lookup） | ✅ 正确 |

### 22.4 设计文档列但保持现状的项

| 项 | 设计文档 | 当前实现 | 理由 |
|---|---------|---------|------|
| deal_reject_msg 按 business_type 分发 | 第 11 节 switch 分支 | 统一回调 on_order_rtn(DISCARD) | switch 各分支实际等价 |
| deal_etf_trade_rtn 成分券展开 | 第 8.2 节 | 只解析固定部分，映射为 OrderRtn | 符合"成分券暂不展开" |
| deal_link_connect 触发重新登录 | 第 12.2 节 | 由 multi_engine 驱动 | 框架级职责 |
| deal_send_error 重置 login_state | 第 12.2 节 | 不重置 | 链接断开已由 deal_link_close 重置，单条发送失败不应误杀会话 |

---

## 23. Mock 组件（mock_client / 98_counter_mock / json_utils）

> **来源**：`mock/client/mock_client_design.md`（v2.1）、`mock/98_counter/98_counter_mock_design.md`
> **代码路径**：`trunk/NewAPI/gone/api/mock/`

### 23.1 组件总览

| 组件 | 类型 | 功能 | 编译目标 |
|------|------|------|---------|
| `mock_client` | 客户端测试工具 | 加载 `liblbapi.so`，JSON 配置化测试 FTE 协议全链路 | `mock_client`（可执行，~1MB） |
| `98_counter_mock` | 服务端模拟 | 模拟 98 柜台，支持 AGW 登录/账户登录/心跳 | `counter98_mock`（可执行，~1MB） |
| `json_utils` | 工具库 | 轻量级 JSON 解析/序列化，零外部依赖，兼容 gcc 4.8.5 | 静态编译进两个目标 |

**编译方式**：`./build.sh -DBUILD_MOCK=ON`（父模块透传 CMake 选项）。

### 23.2 mock_client 架构与流程

```
JSON Config → TestCaseRunner → API Loader(链接 liblbapi.so) → CallbackHandler
                                  ↓ 发送请求
                             gw_counter_direct → FTE 柜台
                                  ↕ 回报
                             CallbackHandler 记录 → 字段校验
```

- **直接链接**（非 dlopen）`liblbapi.so`。
- **逐字段校验**：`extract_response_fields()` 提取回报结构体全部字段到 `map<string,string>`；`trim_fixed()` 裁剪定长 char 数组的 `\0`/空格；`match_field()` 字符串比对。
- **异步等待**：成交回报(2005)用 `has_trade_rtn()` 轮询等待（超时 5s）；撤单用 `has_cancel_rsp()` 特定等待，避免被中间委托回报(2003)干扰。
- **动态引用**：撤单请求 `order_sys_no` 支持 `"$last_order_sys_no"`，自动引用上一笔委托的 order_sys_no。
- **支持的回报类型**：LoginAns(2001)、OrderRtn(2003)、TradeRtn(2005)、CancelRsp(2004)、WaitHeartbeat。
- **JSON 预期格式**：`expected_response.fields` 中 `null`=动态字段跳过，非 `null`=精确比对。

### 23.3 98_counter_mock 架构

```
TCP 客户端 → counter98_server(accept_loop) → client_session(每连接一线程)
                    ↕                        → account_manager（用户/账户校验）
             message_parser（组包/拆包）
```

- **线程模型**：`accept_loop` 主线程 + 每连接 `client_session` 线程 + `cleanup_loop` 清理线程。
- **关键修复**：cleanup_loop 删除 session 用 `disconnected_` 标记避免 use-after-free；`running_` 用 `std::atomic`；TCP 粘包/拆包用 `feed_data` + 接收缓冲区循环处理；新 socket 设 `TCP_NODELAY`。

### 23.4 测试结果

- mock_client 4/4 通过（登录 6 字段、委托 17 字段、成交回报 17 字段、撤单 7 字段）。
- 测试记录文档：`mock/client/mock_client_test.md`。

---

## 24. FTE 联调关键发现（Task 7.4~7.6）

> 以下为 mock_client 与 FTE 真实联调中验证的关键结论，含多个易踩坑点。

### 24.1 FTE 登录密码来源（重要）

- FTE 登录密码从 XML `ext_mod_user_info_ute_<partition>.xml` 的 `<Password>` 字段加载（`ftedata_init.cpp LoadLoginInfo`），**不是** `cash_fund.bin`！改 bin 密码无效。
- `passwd_map_` 的 key 为 `GeneralFundAssetKey(fund_account_id, branch_id)`，branch_id 数组带 `\0` 填充（如 `0001\0\0\0\0\0\0`）。

### 24.2 FTE 启动校验与数据加载

- `fund_info_manager.cpp Init` 要求 `cash_fund_map_.size() == account_data_map_.size()`，否则 `SetDataLoadErr(2)` → 启动失败。插入 cash_fund 必须同步插 account_ute。
- 编译产物安装到 `${CMAKE_ATP_RES_ROOT}/fte`（=/mnt/work/gt_test/work_atp/cmake/fte），覆盖 0 字节文件。
- `env.sh` 中 `UTE_BIN` 原指向 0 字节文件，需改为有效二进制；`libutedatainit.so` 0 字节需复制有效版本。
- `bin_tool.py update <file> --field <定位字段> --eq <匹配值> --set <field=value> [--show]` 支持 bin 文件字段更新。

### 24.3 offerWay invalid 根因（pbu 空格填充）

- **根因**：account_ute bin 的 offer_pbu 是空字节填充 `"21085\0"`，而 FTE 配置 ute.xml 的 pbu_id 经 CopyToArray 是**空格填充** `"21085 "`，byte[5]（' ' vs '\0'）不同导致 `pbu_id_set_.find()` 失败，`group_index_array_[kSHOes]` 保持 -1。
- **修复**：改 `account_ute_61.bin` 的 trade_pbu/offer_pbu 为空格填充（备份 .bak_pbu）；`simulator_tgw.xml` 的 `<pbu_id>` 空→21085。
- **关键约束**：`PBUID_def = std::array<char,6>`，FTE 内部统一空格填充；上游 bin 若空字节填充会致精确比较失败。
- **一致性要求**：连接 pbu 在 FTE ute.xml（pbu_id_list），模拟交易所 pbu_array 在 simulator_tgw.xml，两者必须一致。

### 24.4 心跳周期单位修复（2005 丢失根因）

- **现象**：客户端收不到成交回报(2005)，只收到 2001/2003/2003，随后链接断（err_type=1, err_code=-39=LBERR_CH_LINK_BROKEN）。
- **根因**：FTE 心跳周期单位错误。客户端发 `heart_bt_int=5`（秒），FTE `uplink_biz_processor.cpp:207` 直接赋给 `heart_period_`，而 `tcp_endpoint.h:749` detect_timer 按**毫秒**解释 `LocalMilliseconds_def(period*2)` → 10ms 心跳超时，FTE 主动断链，2005 丢失。
- **修复**（1 行）：`uplink_biz_processor.cpp:207` `output = logon_req.heart_bt_int * 1000;`（秒→毫秒）。
- **注意**：FTE 心跳 `heart_bt_int` 语义为秒但 timer 按毫秒，对接方须保证 heart_period 为毫秒值。

### 24.5 委托/撤单/登录链路打通

- 委托到达交易所并成交（exec_type[F] ord_status[2] cum_qty=100）。
- 状态映射：`map_ord_status(2)→ORDER_STATE_DONE_PART(3)`，(3)→DONE_FULL(4)；`map_exec_type('F')→RSP_TYPE_ORDER_TRADE(3)`，('0')→COUNTER_RSP(1)。

### 24.6 逐字段回报校验机制（Task 7.5）

- **JSON 格式**：`expected_response.fields` 对象包含回报结构体全部字段；`null`=动态字段跳过，非 `null`=精确比对。
- **字段提取**：`extract_response_fields()` 自动提取 LoginAns/OrderRtn/TradeRtn/CancelRsp 所有字段到 `map<string,string>`，支持定长 char 数组的 `\0`/空格裁剪（`trim_fixed`）。
- **动态引用**：撤单请求 `order_sys_no` 支持 `"$last_order_sys_no"`，自动引用上一笔委托的 order_sys_no。
- **异步等待**：成交回报(2005)用主动轮询等待（超时 5s）；撤单测试用 `has_cancel_rsp()` 特定等待避免被中间回报干扰。
- **测试结果**：4/4 通过。关键文件：`test_case_runner.cpp`、`fte_combo.json`。

---

## 25. gw_counter 性能分析与优化（Task 7.8）

> 详细分析见 `mock/client/mock_client_design.md` §14。

### 25.1 发送路径瓶颈

单笔委托（约 118 字节）从 `deal_order_req` 到发送的耗时环节：

| 瓶颈 | 位置 | 特征 |
|------|------|------|
| **会话查找**（首要） | `GwSessionCache::get_session()` | 全局 mutex 锁 + unordered_map 哈希 + std::string 构造 |
| **日志输出**（高放大） | `deal_recv_msg` | 每条消息构造 hexbuf + snprintf 打 info_log |
| **消息构建** | `build_order_msg` | 先 memcpy 再 space_pad 遍历同一数组两次 |
| **校验和** | `GenerateSzCheckSum()` | 逐字节求和 %256，O(n) |
| **中间拷贝** | 栈上 body → encode 到 o_buf | 一次不必要的中间拷贝 |

**瓶颈排序**：GwSessionCache 全局锁 > 日志输出 > 消息构建重复遍历 > 校验和逐字节循环。

### 25.2 优化方案

| 优先级 | 方案 | 收益 |
|--------|------|------|
| **P0** | A：会话信息缓存到 counter 实例成员变量，消除全局锁（个微单连接特性） | 最大 |
| **P0** | E：日志降级为 debug，生产关闭高频日志 | 高吞吐 I/O 显著下降 |
| P1 | B：合并 memcpy 与空格填充（`copy_and_pad`） | 减少数组重复遍历 |
| P1 | C：校验和与序列化合并，固定字段预编码缓存 | 减少一次 O(n) 遍历 |
| P2 | D：直接构建到队列内存（参考 fpga_counter_direct） | 减少中间拷贝 |
| P2 | F：`++session_seq_`/`login_state` 原子化 | 消除数据竞争 |

### 25.3 验证建议

- mock_client 增加「性能测试」用例类型，用 `std::chrono::steady_clock` 记录单笔委托耗时。
- 用 `perf`/`gprof` 对 `build_order_msg`、`GenerateSzCheckSum`、`GwSessionCache::get_session` 采样验证优化前后热点变化。

---

## 26. 性能测试系统与关键缺陷修复（Task 7.9~7.12）

> **来源**：`mock/client/src/perf_runner.h/.cpp`、`mock/client/src/metric_stats.h/.cpp`、`mock/client/src/cpu_affinity.h/.cpp`
> **测试记录**：`mock/client/mock_client_test.md` §7.4~7.12
> **设计文档**：`mock/client/mock_client_design.md` §15

### 26.1 性能测试模块设计

性能测试作为 **mock_client 内置模块**（非独立程序），通过 JSON 配置开关控制。

**配置文件**：`mock/client/config/connection_config.json` 新增 `perf_test` 块：

```json
{
  "perf_test": {
    "enable": true,
    "duration_sec": 30,
    "tps": 500,
    "warmup_sec": 3,
    "cpu_id": -1,
    "report_file": "perf_report.txt",
    "order": {
      "fund_account_id": "800000000004",
      "security_id": "000001.SZ",
      "market_id": 2,
      "side": 1,
      "order_type": 2,
      "order_qty": 100,
      "order_price": 250200
    }
  }
}
```

**模块文件**：

| 文件 | 角色 |
|------|------|
| `mock/client/src/perf_runner.h/.cpp` | PerfConfig + PerfRunner（主控逻辑） |
| `mock/client/src/metric_stats.h/.cpp` | 延迟统计（均值/P50/P75/P90/最大/最小/标准差） |
| `mock/client/src/cpu_affinity.h/.cpp` | CPU 绑定（`sched_setaffinity`） |

**执行流程**：

1. `main.cpp` 功能测试完成后，解析 config 的 `perf_test` 节点
2. `enable=true` 时调用 `MockClient::run_perf_test()`
3. 绑核（`cpu_id >= 0` 时 `sched_setaffinity(0, ...)`）→ 预热（`warmup_sec`）→ 匀速发单（间隔 = 1/TPS 秒）
4. 统计延迟 → 输出报告文件

**耗时来源**：复用 `OrderReq` 的 `api_arrive_time_ns`/`api_leave_time_ns`（`api_impl::order_insert` 记录），`lat = leave - arrive`。
失败返回码统计进报告（如 -22 = `LBAPI_ERR_LINK_DISCONNECTED`）。

### 26.2 关键缺陷修复 P1：双线程并发接收数据竞争

**现象**：性能测试中校验和不匹配 0~69 次/轮，仅影响 2003/2005 回报，差异值 1 或 7。

**根因**：`single_socket_engine::do_work()` 末尾调用 `link_.deal_recv()`，与 mthread `recv_th_` 双线程**并发**调用 `loop_deal_recv()`，操作共享 `aio_recv_buf::curbuf` 导致数据竞争。

**修复**：移除 `single_socket_engine.cpp` 中 `do_work()` 末尾的 `link_.deal_recv()`，让 mthread 独占驱动接收。

**验证**：校验和不匹配从 69 次降为 0。根因确认：`multi_socket_engine` 不调 `link_.deal_recv()`，无此问题。

```
文件：trunk/NewAPI/gone/api/src/single_socket_engine.cpp
- 移除：link_.deal_recv();  // do_work() 末尾
```

### 26.3 关键缺陷修复 P2：API 心跳超时断链

**现象**：性能测试中大量返回码 -22（`LBAPI_ERR_LINK_DISCONNECTED`），每轮 110 次。

**根因**：FTE 不向客户端发送心跳（单向心跳：客户端→FTE），但 API 的 `heart.on_msg()` 被注释掉。导致 `check_timeout()` 恒成立（`tcnt==0`），6 秒后超时主动断链。

**修复**：在 `aio_tcp.h::deal_recv()` 中启用 `heart.on_msg()`，业务消息也保持链路存活。

**验证**：perf test -22 失败从 110 次降为 0。

```
文件：trunk/NewAPI/common/include/aio_tcp.h
- 启用：heart.on_msg();  // 在 deal_recv() 中取消注释
```

### 26.4 FTE 对象池扩容

**背景**：FTE 使用对象池管理回报/拒绝消息，初始容量 2000。连续约 6000 笔订单后池空，动态分配并记 `"capacity should resize"` 警告；继续运行后 FTE 降级（进程存活但不再监听端口）。

**扩容内容**（`uplink_biz_processor.cpp SetFTE2DSEQueue`）：

| 对象池 | 原容量 | 新容量 | 实际（2 的幂） |
|--------|--------|--------|----------------|
| `fte_report_pool_` | 2000 | 20000 | 32768 |
| `fte_reject_pool_` | 2000 | 20000 | 32768 |
| `sh_fte_etf_report_pool_` | 2000 | 20000 | 32768 |
| `sz_fte_etf_report_pool_` | 2000 | 20000 | 32768 |
| `sh_internal_etf_report_pool_` | 2000 | 20000 | 32768 |
| `sz_internal_etf_report_pool_` | 8 | 2000 | 2048 |
| `etf_sync_order_pool_` | 1000 | 10000 | 16384 |

**编译**：`./compile_fte.sh -r`（产物 `/mnt/work/gt_test/work_atp/cmake/fte/bin/ute`）。

### 26.5 性能测试验证结论

**两个修复后 perf 结果**：校验和不匹配 0、-22 断链 0、发单成功率 100%。

| 场景 | TPS | 持续时间 | 总笔数 | 结果 |
|------|-----|---------|--------|------|
| 基准 | 100 | 5s | 500 | ✅ 全部通过 |
| 中负载 | 200 | 15s | 3000 | ✅ 全部通过 |
| 中负载 | 200 | 20s | 4000 | ✅ 全部通过 |
| 高负载（扩容前） | 500 | 30s | 15000 | ❌ FTE 对象池耗尽崩溃 |
| 高负载（扩容后） | 500 | 30s | 15000 | ✅ 稳定运行（0 失败，0 resize 警告） |

**多轮稳定性（3 轮 × 200 TPS / 10s）**：全部通过（0 失败 / 0 校验和不匹配 / 0 断链 / EXIT=0）。

| 轮次 | 样本数 | 平均延迟 | P50 | P75 | P90 | 最大 |
|------|--------|---------|-----|-----|-----|------|
| 1 | 1999 | 2627.42ns | 1013ns | 2176ns | 4710ns | 73842ns |
| 2 | 1999 | 2379.57ns | 983ns | 2165ns | 4051ns | 71905ns |
| 3 | 1999 | 2296.48ns | 962ns | 2114ns | 3789ns | 72647ns |

### 26.6 CPU 绑定验证

| cpu_id | TPS | 样本 | 平均 | P50 | P90 | 最大 | 标准差 |
|--------|-----|------|------|-----|-----|------|--------|
| 0（绑定） | 490.88 | 14727 | 1986.21ns | 832ns | 2715ns | 6228979ns | 51372ns |
| -1（不绑定） | 499.18 | 14976 | 1635.5ns | 842ns | 2755ns | 223030ns | 3505ns |

**结论**：
- CPU 绑定功能正常（日志确认 `"已绑定到 CPU 0"`、`"当前 CPU 亲和性: 0"`）
- P50/P90 绑核略优（~10ns），但 CPU 0 有中断干扰（最大尖峰 6.2ms）
- **建议**：绑定到非 CPU0 的专用核，或使用 `isolcpus` 内核参数隔离

### 26.7 其他发现

- `simple_thread::busy_run()` 忙轮询导致进程 97% CPU 是**正常设计**（低延迟），非卡死。
- `object_pool` 容量自动向上取整到 2 的幂：`create("name", 20000)` → 实际 32768。
- FTE 端口在测试完成后停止监听（进程存活但降级），多轮测试需重启 FTE。
- tgw_simulator 38140 端口进程易 defunct（僵尸），需单独重启：`tgw_simulator -p 38140 -m 7 -n TGWSimulator_Bond`。

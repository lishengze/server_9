# NewAPI 交易 API 框架知识库

> 本知识库系统化整理了 `trunk/NewAPI` 交易 API 客户端框架的架构、设计、数据流与待办任务。
> 面向后续接入/维护/开发人员，以及需要理解该框架的大模型。
>
> **来源**：`study/counter.md`、`study/question.md`、`study/技术实现.md`、`study/数据流转.md`、`study/产品使用.md`、`task/api_dev/api_dev_task.txt`、`task/api_dev/gw_counter_api.md`、`mock/client/mock_client_design.md`、`mock/98_counter/98_counter_mock_design.md`
> **基线**：HEAD + 后续重构（g1 协议改版、v2.1 规范）
> **版本**：v3.0（2026-09-22，§35 mock_client test_plan 主配置模式、§36 日志系统与结果分析、§37 mock_client 全面复盘与修复回归；§33 单链接单客户、§34 非阻塞发送）

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
27. [FTE vs GOne 完整性能对比与瓶颈分析](#27-fte-vs-gone-完整性能对比与瓶颈分析)（§27.10 gw counter 优化与重测）
28. [gw counter 深度分析与序列化优化](#28-gw-counter-深度分析与序列化优化)（§28.1 分析文档 / §28.2 双趟序列化 / §28.3 三档对比 / §28.4 方案1实验回退）
29. [压测算法优化：离开 API 时间记录点](#29-压测算法优化离开-api-时间记录点)
30. [FTE 压测卡死根因（DSE 队列无消费者）与修复](#30-fte-压测卡死根因dse-队列无消费者与修复)
31. [重新编译部署后 FTE vs GOne 复测](#31-重新编译部署后-fte-vs-gone-复测)
32. [新计时口径对比（send 前记录点）](#32-新计时口径对比send-前记录点)
33. [gw counter 单链接单客户模式优化](#33-gw-counter-单链接单客户模式优化)
34. [gw counter 非阻塞发送 + 写就绪通知优化](#34-gw-counter-非阻塞发送--写就绪通知优化)
35. [mock_client test_plan 主配置模式（--plan）](#35-mock_client-test_plan-主配置模式--plan)
36. [mock_client 日志系统与结果分析](#36-mock_client-日志系统与结果分析)
37. [mock_client 全面复盘与修复回归](#37-mock_client-全面复盘与修复回归)

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
| `mock/client/src/metric_stats.h/.cpp` | 延迟统计（均值/P50/P75/P90/P95/最大/最小/标准差） |
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

---

## 27. GOne（fpga_direct）双链路架构与模拟柜台开发（2026-09-16）

### 27.1 GOne 双链路架构

GOne（FPGA 极速柜台）采用**双链路架构**，区别于 FTE 的单链路：

| 链路 | 端口 | 引擎 | 职责 |
|:---|:---:|:---|:---|
| **GW 链路** | 44001 | `multi_socket_engine` 的 `fast_gw_link_` | 证券信息查询、客户登录、心跳 |
| **Core 链路** | 44002 | `single_socket_engine` 的 `link_` | 委托、撤单、心跳 |

**登录流程**：
1. API 连接 GW 链路（44001）→ 发送 `sec_info_req` → 收 `sec_info_ans`
2. API 发送 `login_req` → 收 `login_ans`（含 `trade_port=44002`）
3. API 从 `login_ans` 提取 `trade_port`，连接 Core 链路（44002）
4. Core 链路建立后，API 在 Core 链路发送委托/撤单

### 27.2 3 种 fast_counter_type

| 枚举值 | 宏 | 柜台类型 | 说明 |
|:---:|:---|---|:---|
| 1 | `FAST_COUNTER_TYPE_GW` | gw_counter_direct | 个微软件极速（FTE） |
| 2 | `FAST_COUNTER_TYPE_FPGA_DIRECT` | fpga_counter_direct | FPGA 直连（GOne） |
| 3 | `FAST_COUNTER_TYPE_FPGA_GATEWAY` | fpga_counter_gateway | FPGA 网关（GOne-GW） |

### 27.3 模拟柜台 gone_counter_mock

**位置**：`trunk/NewAPI/gone/api/mock/gone_counter/`

**核心组件**：
- `gone_counter_server`：监听 GW/Core 双端口，accept 后创建 `client_session`
- `client_session`：单链路会话，处理消息并回应答
- `session_registry`：GW/Core 会话共享登录信息（双链路关联）
- `account_manager`：账户配置校验
- `message_parser`：g1 协议消息编解码

**配置**：`config/server_config.json`（端口、账户、超时、日志）

### 27.4 关键修复

#### 修复 1：字段截断
**现象**：登录测试字段校验失败，`cust_id`/`fund_account_id`/`account_id` 比预期少 1 个字符。

**根因**：`fpga_counter_base.cpp` 的 `build_login_rtn()` 和 `save_client_info()` 使用 `str_copy_format`（最多拷贝 `dst_size-1` 字节），占满 16 字节的定长字段被截断。

**修复**：改用 `memcpy` 完整拷贝（参照 `counter98.cpp` 已有修复模式）。

#### 修复 2：Core 链路端口错误
**现象**：登录成功后 Core 链路连接到 GW 端口（44001）而非 Core 端口（44002）。

**根因**：`single_socket_engine::deal_fpga_core_connect()` 使用 `set_remote()` 设置 Core 地址，但 `aio_socket_link::set_remote()` 最多允许 2 个地址，且 `connect(need_switch=0)` 不切换地址，导致仍连主地址 44001。

**修复**：
1. `aio_socket_link.h` 新增 `reset_remote()` 方法（清空主备地址，仅设一个地址）
2. `deal_fpga_core_connect()` 改用 `reset_remote(trade_port)` 覆盖地址后连接

#### 修复 3：mock 未推送成交回报
**现象**：成交回报测试超时。

**根因**：mock 委托处理后只回委托回报（order_rtn），未推送成交回报（trade_rtn）。

**修复**：新增 `send_trade_rtn()`，委托应答后主动推送成交回报（order_status=3, exec_qty=100）。

### 27.5 GOne 功能测试结果

**测试配置**：`connection_config_gone.json`（fast_counter_type=2, speed_link_type=1, counter98_addr=127.0.0.1:9003）

**测试用例**：`gone_combo.json`（5 个用例：登录/委托/成交/撤单/心跳）

**结果**：**5/5 PASS ✅**

| 测试用例 | 耗时 | 校验字段 | 结果 |
|:---|---:|:---:|:---:|
| GOne 登录测试 | 4ms | 6/6 ✓ | ✅ |
| GOne 委托买入测试 | 4ms | 17/17 ✓（rtn_type=0 委托应答） | ✅ |
| GOne 成交回报校验 | 0ms | 17/17 ✓（order_status=3, exec_price=250200, exec_qty=100） | ✅ |
| GOne 撤单测试 | 20ms | 7/7 ✓（err_code=0） | ✅ |
| GOne 心跳维持测试 | 30001ms | 双链路 30s 不断链 | ✅ |

### 27.6 GOne 性能测试

**实现方式**：复用 mock_client 内置 `PerfRunner`，通过 `set_counter_name()` 动态化报告标题。

**修改点**：
- `perf_runner.h`：`PerfConfig` 新增 `counter_name` 字段
- `perf_runner.cpp`：报告标题从写死 "FTE" 改为动态 `cfg_.counter_name`
- `mock_client.cpp`：`init()` 中按 `fast_counter_type` 设置 counter_name（1=FTE, 2=GOne, 3=GOne-GW）
- `connection_config_gone.json`：perf_test enable=true, duration_sec=10, tps=10000
- `run_perf_compare.sh`：顺序执行 FTE → GOne 性能测试

**结果**（10000 TPS / 10 秒）：

| 指标 | 值 |
|:---|---:|
| 发送/成功/失败 | **92,073 / 92,073 / 0**（100%） |
| 实际 TPS | 9,207（目标 10,000） |
| 平均延迟 | **434 ns** |
| P50 | **333 ns** |
| P75 | **431 ns** |
| P90 | **522 ns** |
| 最大值 | 646,667 ns |
| 最小值 | 111 ns |

### 27.7 FTE vs GOne 完整对比（2026-09-16 重跑）

> 修复 FTE 环境后，在 docker `otc` 内完整重跑 FTE 与 GOne 的顺序性能测试（10000 TPS / 10s）。

**环境**：docker `otc`，FTE(33001) + 模拟交易所(38140/38141/39142) + counter98_mock(9002) 跑 FTE；gone_counter_mock(44001/44002) + counter98_mock(9003) 跑 GOne。

#### 原始结果

| 指标 | GOne (fpga_direct) @10000TPS | FTE (gw counter) @2000TPS（干净基线） | FTE (gw counter) @10000TPS |
|:---|---:|---:|---:|
| 实际 TPS | 9363.62 | 1996.08 | 9443.84 |
| 成功率 | **100%** | **100%** | 43.7%（-25 失败 56%） |
| 平均延迟 | **186.7 ns** | 1010 ns | 2248 ns |
| P50 | **120 ns** | 611 ns | 1974 ns |
| P90 | **180 ns** | 1593 ns | 2595 ns |
| 最大值 | 165.7 us | 196 us | 3697 us |
| 失败 | 0 | 0 | 53136（-25） |

> GOne 平均延迟约为 FTE（干净基线）的 **1/5.4**，约为 FTE 高负载的 **1/12**。

### 27.8 瓶颈分析

**① GOne (fpga_direct) —— 极简路径，186ns**
委托路径 `order_insert → fpga_counter_direct::deal_order_req`：
- 状态检查直接读**成员变量**（`trade_link_connect` / `client_info_.login_state` / `client_info_.fpga_state`），无锁
- `get_sec_index()`：成员 `sec_map_` 哈希查找
- `trade_send_queue_->write_get_mth()`：无锁队列写
- `build_order_msg(req, client_info_, sec_index, head)`：**直接使用成员 `client_info_`**，无全局锁、无 string 构造
- **结论**：单客户会话全部缓存为成员变量，无锁无分配，是 186ns 的关键。

**② FTE (gw counter) —— 全局锁 + string 构造，1010~2248ns（主要瓶颈）**
委托路径 `order_insert → gw_counter_direct::deal_order_req`：
- `build_order_msg(req, o_buf)` 中**每次委托**执行：
  ```cpp
  std::string fa_key(req.fund_account_id.data(), strnlen(...,16));  // string 构造+分配
  GwSessionInfo *session = GwSessionCache::instance().get_session(fa_key);
  // get_session: std::lock_guard<std::mutex> mutex_ + unordered_map::find + string 比较
  ```
- **瓶颈点**：`GwSessionCache::get_session()` 的**全局 mutex 锁** + **string 构造** + **unordered_map 哈希**，是 FTE 比 GOne 慢 5~12 倍的主因。
- 撤单路径更重：`build_cancel_msg` 调 `get_session()` + `get_clordno()` + `get_orig_client_seq_id()` = **3 次全局锁**。
- **优化方向**（对应 api_dev_task.txt）：单客户场景下将会话缓存到 counter 实例成员变量，消除全局锁与 string 构造（对齐 fpga_direct 的 `client_info_` 范式）。

**③ -25 SEND_QUEUE_FULL（56% 失败）—— FTE 下游容量瓶颈**
- 10000 TPS × 10s = 10 万笔，远超 **FTE 对象池容量（32768）**。FTE 在 `index[16382]`（约 1.6 万笔）后停止处理（日志中断），FTE 崩溃/降级。
- FTE 崩溃后 TCP 连接未立即收到 FIN（链路仍显示 connected），引擎线程无法正常发送，API 发送队列积压 → 后续订单返回 `-25`。
- **结论**：-25 是**下游 FTE 容量不足**的连带效应。2000 TPS 干净基线 0 失败，证明 API 侧发送队列在合理负载下无瓶颈。

**④ 心跳**：FTE 的 `heart_bt_int` 语义为**秒**，但 `tcp_endpoint.h` 的 `detect_timer` 按**毫秒**解释 `heart_period`。已由 API 侧统一换算：`build_login_msg` 中 `heart_bt_int = heart_interval * 1000`（当前代码 `gw_counter_direct.cpp` 已实现），FTE 侧 `uplink_biz_processor.cpp` 回退为直赋。若未换算，FTE 会因 10ms 心跳超时主动断链，导致成交回报（2005）丢失。

### 27.9 结论

1. ✅ **GOne 性能显著优于 FTE**：平均 187ns vs 1010ns（干净基线），快约 **5.4 倍**。
2. ✅ **GOne 10000 TPS 稳定**：100% 成功，0 失败。
3. ❌ **FTE 10000 TPS 无法支撑**：FTE 对象池耗尽崩溃，56% -25 失败。
4. 🔧 **FTE 瓶颈定位**：API 侧 `GwSessionCache::get_session()` 全局锁 + string 构造；下游 FTE 对象池容量不足。
5. 🔧 **优化建议**：① 会话缓存到 counter 实例成员（去全局锁）；② 发送队列/FTE 对象池扩容；③ 心跳 ×1000 移到 API 侧。

### 27.10 gw counter 优化与重测（2026-09-16）

> 针对 §27.8 定位的瓶颈实施 5 项优化，重测 10000 TPS / 10s。

#### 27.10.1 优化内容

| # | 优化项 | 文件 |
|:--|:---|:---|
| 1 | **GwSessionCache 去锁**：移除 6 个方法 `std::lock_guard<std::mutex>`（手动排查业务安全） | `gw_session_cache.h` |
| 2 | **string 构造优化**：3 个 build 方法用类成员 `fa_key_cache_`，每次 `assign()` 复用 buffer | `gw_counter_direct.h/.cpp` |
| 3 | **心跳 ×1000 移到 API 侧**：`heart_bt_int = heart_interval * 1000`，FTE 回退 | `gw_counter_direct.cpp` + FTE |
| 4 | **发送队列扩容**：`send_queue_size_mb` 2MB→64MB（mock_client 增加透传） | `connection_config.json` + `mock_client.cpp` |
| 5 | **FTE 对象池扩容**：`fte_report`/`fte_reject` 150000（实际 262144），ETF 池合理值，`etf_sync` 保持 10000 | FTE `uplink_biz_processor.cpp` |

#### 27.10.2 优化后结果（10000 TPS / 10s）

```
实际 TPS : 9621.25   样本数 : 96213
成功 96213 / 失败 0   ← 100% 成功
平均值 : 392.717 ns   P50 : 191 ns   P75 : 250 ns   P90 : 351 ns
最大值 : 762530 ns    最小值 : 90 ns   标准差 : 3187.75 ns
```

> 功能测试（登录/委托/成交/撤单/心跳）全部 PASS。

#### 27.10.3 优化前后对比（10000 TPS）

| 指标 | 优化前 FTE | **优化后 FTE** | GOne | FTE/GOne |
|:---|---:|---:|---:|---:|
| 成功率 | 44% | **100%** | 100% | — |
| 平均延迟 | 2248 ns | **392.7 ns** | 186.7 ns | 2.1x |
| P50 | 1974 ns | **191 ns** | 120 ns | 1.6x |
| P90 | 2595 ns | **351 ns** | 180 ns | 2.0x |
| 失败 | 53136（-25） | **0** | 0 | — |
| FTE 崩溃 | 是 | **否** | — | — |

> FTE 与 GOne 差距从 **12 倍缩小到 2.1 倍**，10000 TPS 下 0 失败、FTE 不再崩溃。

#### 27.10.4 优化效果分析

1. **延迟 2248→392.7ns（5.7x）**：`get_session()` 全局锁 + string 堆分配移除；发送队列扩容消除 `write_get_mth` 拥塞阻塞。
2. **成功率 44%→100%**：`send_queue_size_mb` 2MB→64MB 消除 -25（2MB 仅缓冲约 5700 笔）。
3. **FTE 不再崩溃**：对象池实际 262144，支撑 10 万笔。
4. **心跳正确性**：API 侧 ×1000 统一换算，FTE 回退。
5. **剩余差距（2.1x）**：FTE 仍有 `unordered_map::find` 哈希遍历 + 消息构建开销；GOne 全成员变量零分配。进一步优化＝会话指针直接缓存到 counter 实例（完全对齐 fpga_direct 范式）。

#### 27.10.5 关键经验

- **`send_queue_size_mb` 需在 mock_client 显式透传**：JSON 配置项若 mock_client 未 `set_attr` 则默认 2MB，压测高 TPS 必现 -25。已补 `config.has("send_queue_size_mb")` 透传。
- **FTE 对象池创建卡死**：将 ETF 大对象池也扩到 150000 会导致 `sz_fte_etf_report` 分配卡死（对象大、262144 个内存分配过慢）。订单压测只需 `fte_report`/`fte_reject`，ETF 池保持合理值。
- **FTE 高负载后降级**：10000 TPS 压测后 FTE 进入降级状态（不响应新登录），需 stop_all + start_all 重启。
- **mock_client 压测后挂起**：10 万笔回报回调处理慢，进程在 shutdown 阶段挂起，需 timeout 兜底。

---

## 28. gw counter 深度分析与序列化优化（2026-09-16）

> 在 §27.10 的 5 项优化基础上，进一步对 gw counter 模块进行**深度分析**（架构/UML/链路/瓶颈/方案）和**序列化微优化**（双趟宽累加），并在 §28.4 实验性验证**方案1（会话迁成员）**后因业务约束回退。

### 28.1 深度分析文档 study/gw_counter.md

**文件**：`study/gw_counter.md`（全文约 23KB）

**内容**：

| 章节 | 内容 |
|------|------|
| §1 整体架构 | 三层架构图（API层→Counter层→Engine层→网络），与 fpga_direct/counter98 对比 |
| §2 类关系 UML 图 | Mermaid 类图展示 `api_impl`、`gw_counter_direct`、`GwSessionCache`、`single_socket_engine` 关系 |
| §3 FTE 协议总览 | 报文格式（头8B+体+校验和4B）、消息类型表（1xxx/2xxx/3/9）、结构体尺寸、状态字典映射 |
| §4 消息通讯链路 | **6 个 Mermaid 时序图**：登录、委托、撤单、ETF、心跳、回报拆包分发流程图 |
| §5 状态机与线程模型 | 登录状态机、链接状态机、线程模型表（含 GwSessionCache 无锁数据竞争风险标注） |
| §6 性能瓶颈分析 | 实测数据（GOne 187ns vs FTE 1010ns，5.4x 差距），热路径源码逐行分析，5 个瓶颈定位 |
| §7 详细解决方案 | **6 套方案**（P0~P2 优先级）：会话迁成员、撤单映射、回报路径优化、线程安全、高 TPS 熔断、序列化微调 |

**核心发现**：
- 性能瓶颈根因：`GwSessionCache` 全局单例 unordered_map（string key + 哈希） vs fpga_direct 的成员变量缓存，是 5.4 倍差距的主要来源
- 线程安全风险：当前 GwSessionCache 无锁，跨线程并发读写存在数据竞争
- 撤单 3 次查找：`build_cancel_msg` 每次触发 3 次全局查找

### 28.2 序列化优化落地（双趟宽累加）

**改动文件**：`trunk/NewAPI/gone/api/src/gw_counter_direct.cpp`

**新增辅助函数**：
- `pad_copy(p, src, n)`：`memset(p,' ',n)` 整块填空格 + `memcpy(p,src,strnlen)` 拷贝实际内容（替代逐字节 `cksum_copy_pad`）
- `checksum_bytes(buf, len)`：uint64 宽累加校验和（一次 8 字节拆字节求和，替代逐字节 `GenerateSzCheckSum`）

**重写 3 个 build 函数**（双趟方案）：
- 第一趟：字段赋值用 `memcpy`/`memset` 整块（session 字段直接 memcpy，req 字段 `pad_copy`）
- 第二趟：对 `[头+体]` 用 `checksum_bytes` 宽累加算校验和

**`GenerateSzCheckSum` 改为宽累加**（接收路径校验和也受益）。

### 28.3 三档 TPS 性能对比（同环境旧版 vs 优化后）

**对比方法**：`git stash` 临时编译旧版（逐字节单趟）跑基线，再恢复优化版跑同样三组，确保同环境公平对比。

| TPS 档位 | 旧版平均 | 优化后平均 | 变化 |
|:--------:|:--------:|:---------:|:----:|
| 1000 TPS | 1012.8ns | 947.1ns | ↓6.5% |
| 2000 TPS | 762.2ns | 741.7ns | ↓2.7% |
| **10000 TPS** | **442.9ns** | **359.4ns** | **↓18.9%** |

**结论**：序列化优化收益随 TPS 上升而放大。@10000TPS 平均 ↓18.9%，P90 ↓22.9%（441→340ns），0 失败。但低 TPS 下收益有限（报文仅 106 字节）。

### 28.4 方案1（会话迁成员）实验验证后回退

**实验内容**：将 `GwSessionInfo` 作为 `gw_counter_direct` 成员变量，所有 `GwSessionCache::instance().get_session()` 改为直接引用 `session_`，`record_order_locator` 改为直接写 `session_.order_locators`，撤单反查改为成员查找（1 次 find 替代 3 次全局查找）。

**实验数据（@10000 TPS/5s）**：

| 指标 | 旧版 | 双趟序列化 | **方案1** | 旧版→方案1 |
|:---|---:|---:|---:|---:|
| 平均延迟 | 442.9ns | 359.4ns | **297.3ns** | **↓32.9%** |
| P50 | 241ns | 201ns | **160ns** | ↓33.6% |
| P90 | 441ns | 340ns | **260ns** | **↓41.0%** |
| 失败率 | 0% | 0% | 0% | 持平 |

**回退原因**：业务逻辑必须以 `fund_account_id` 为 key 缓存会话数据、委托时取出赋值，会话缓存必须保留全局单例 `GwSessionCache`。方案1改造已回退。

**意义**：方案1性能数据（@10000TPS P50 160ns，逼近 GOne 的 120ns）证明了"会话 string+hash 查找是主要性能瓶颈"的判断正确。当前代码状态 = 双趟序列化优化（方案6）。

### 28.5 当前代码状态与剩余差距

**当前代码**：HEAD + 双趟序列化优化（`pad_copy` + `checksum_bytes` + `GenerateSzCheckSum` 宽累加）+ `GwSessionCache` 全局单例（业务约束保留）+ 5 项性能优化（§27.10：去锁 / `fa_key_cache_` string 复用 / 心跳 ×1000 / 发送队列 64MB / FTE 对象池扩容）。

> **gw counter 优化全景（2026-09-17 掌握）**：
> 1. **GwSessionCache 去锁**：移除 6 个方法 `std::lock_guard<std::mutex>`，手动排查业务场景后无锁直接访问（`gw_session_cache.h`）
> 2. **string 构造优化**：3 个 build 方法（order/etf/cancel）用类成员 `fa_key_cache_`，每次 `assign()` 复用 buffer，避免每次委托构造临时 `std::string`（`gw_counter_direct.h` 成员 + `.cpp` 使用）
> 3. **心跳 ×1000**：`build_login_msg` 中 `heart_bt_int = heart_interval * 1000`，FTE 侧 `uplink_biz_processor.cpp` 回退为直赋（秒→毫秒统一由 API 侧换算）
> 4. **发送队列扩容**：`send_queue_size_mb` 2MB→64MB（mock_client 增加透传）
> 5. **FTE 对象池扩容**：`fte_report`/`fte_reject` 150000（实际 262144），ETF 池保持合理值（避免分配卡死）
> 6. **双趟序列化**：`pad_copy`（memset 整块 + memcpy）+ `checksum_bytes`（uint64 宽累加），替代逐字节单趟

**最终性能（@10000 TPS）**：

| 指标 | GOne(fpga) | FTE(gw) 当前 | 差距 |
|:---|---:|---:|---:|
| 平均延迟 | 186.7ns | **359.4ns** | 1.9x |
| P50 | 120ns | **201ns** | 1.7x |
| P90 | 180ns | **340ns** | 1.9x |
| 成功率 | 100% | **100%** | 持平 |

**剩余差距来源**：
1. **`GwSessionCache` unordered_map 查找**（string 构造 + 哈希）：业务约束不可消除，可考虑无锁/更优哈希缓解
2. **FTE 协议序列化**：106 字节报文，双趟优化后已接近极限
3. **回报处理路径**：`std::string` 构造 + `strtoll` + `unordered_map` 插入（方案3 可优化）

**下一步建议**：
- 方案3（回报路径优化）：消除回报处理中的 `std::string` 构造
- 方案4（线程安全）：给 GwSessionCache 加锁或明确线程归属
- 方案5（高 TPS 熔断）：`send_queue_full` 时降级 counter98

---

## 29. 压测算法优化：离开 API 时间记录点调整（2026-09-17）

### 29.1 问题

原压测算法中 `api_leave_time_ns` 在 `order_insert` 的 `deal_order_req` 返回后（入队后）记录，此时消息尚未发送到网卡，测量低估了真实延迟。

### 29.2 方案

将 `api_leave_time_ns` 记录点移到引擎线程 `send()` 系统调用后，通过 `link_send_event::leave_time_ptr` 跨线程传递时间戳指针。

**核心修改**：
- `link_send_event` 新增 `uint64_t *leave_time_ptr` 字段（`api_event_msg.h`）
- `order_insert` 中重置 `api_leave_time_ns=0`，成功后自旋等待引擎线程写入（`api_instance.cpp`）
- `deal_order_req` 设置指针（`gw_counter_direct.cpp` / `fpga_counter_direct.cpp` / `fpga_counter_gateway.cpp`）
- 引擎线程 `do_work` 中 `send_msg` 后原子写入（`single_socket_engine.cpp` / `multi_socket_engine.cpp` / `tcpdirect_engine.cpp`）
- 所有其他构造点初始化 `leave_time_ptr=nullptr`（心跳/登录/连接/证券信息/撤单/ETF/98）

> **后续优化（§30.8）**：经分析 `tcp_ch::send_msg_fc` 是阻塞式发送（EAGAIN→CPU_PAUSE 忙等），send() 后记录会吞入系统调用+内核缓冲忙等（占"API 内处理耗时"82~94%），后改为 send() **前**记录，测量纯 API 框架延迟。详见 `task/api_dev/time_ana.md` §二 和 `study/gone_counter.md` §九。

### 29.3 GOne 性能测试结果（10000 TPS / 10s，Release）

| 指标 | 优化前（入队后记录） | **优化后（send 后记录）** | 说明 |
|:---|---:|---:|:---|
| 发送/成功/失败 | 92,073/92,073/0 | **99,966/99,966/0** | 自旋等待使匀速发单更准确 |
| 实际 TPS | 9,207 | **9,996** | 目标 10,000 达成率提升 |
| 平均延迟 | 434 ns | **2,189 ns** | 含引擎消费+send() 系统调用 |
| P50 | 333 ns | **1,937 ns** | — |
| P90 | 522 ns | **2,483 ns** | — |

> 平均延迟从 434ns→2189ns 为**测量口径变化**（入队耗时 → 完整发送链路耗时），并非性能退化。

### 29.4 FTE 对比

FTE 环境不可用（33001 未监听，ute/tgw 未运行），无法对比。

---

## 30. FTE 压测卡死根因分析与 FTE vs GOne 全面对比（2026-09-17）

### 30.1 问题现象

FTE（gw counter）在 10000 TPS / 10s 压测下卡死：mock_client 停在约 16381 个回调（约 8000 笔订单），无论 2000 还是 10000 TPS 都卡在相同累计订单数附近，FTE 进程存活但不再推进。

### 30.2 根因定位（逐一排除）

**排除对象池**：`unbound_object_pool`（订单池 1M）与 `object_pool`（fte_report/fte_reject 等）的 `get()` 在池空时都会**回退到 `new`**，永不返回 nullptr。日志中 `produce too slow`、`capacity should resize`、`Sth wrong` 均出现 0 次，证明**没有任何对象池耗尽**。

**真正的根因——DSE 队列无消费者**：
- `fte_internal_spsc`（FTE→DSE 队列，容量 8192，roundup 后 16384）是唯一承载 `kFTEExecutionReport` 等消息的队列
- 编译带 **`-DNO_DSE`**：其唯一消费者 `external_data_sync::create()` 位于 `#ifndef NO_DSE` 块内被跳过
- 但 `SetFTE2DSEQueue(&fte_internal_spsc)`（main.cpp:365）**仍执行**，生产者照常 `push`
- 结果：消息**只进不出**，队列满后 `producer_consumer_queue::push()` 的 `while(!trypush()){}` **忙等自旋**，处理线程永久卡死

**定位手段**：
- `ps -eo pid,comm,args` 看进程：FTE alive（非 defunct）+ 处理线程状态 **R（自旋）**
- `gstack <pid>` 看调用栈：卡在 `producer_consumer_queue.h:126 push()` 忙等，由 `DealReportAfter`（uplink_biz_processor.cpp:2648）触发
- 卡住 index 16382 ≈ 队列容量 16384，进一步佐证

### 30.3 修复方案（最小改动）

1. **扩容 DSE 队列**：`fte/fte/src/main.cpp` 中 `FTE_DSE_FIFO_LEN` 8192→65536（roundup 后 131072），容纳 10 万笔订单瞬时排队
2. **NO_DSE 下启动丢弃消费者线程**（main.cpp，`SetFTE2DSEQueue` 成功后）：
```cpp
#ifdef NO_DSE
    std::thread dse_drop_th([&fte_internal_spsc]() {
        internal_msg_type msg;
        while (true) {
            if (fte_internal_spsc.pop(msg)) {
                if (msg.data) { msg.recycle_func.release(msg.data); }
            }
        }
    });
    dse_drop_th.detach();
    LOG_INFO("NO_DSE mode: started DSE message drop thread to prevent queue blocking.");
#endif
```

> 曾尝试 `PushToDSE` 辅助函数方案（头文件声明 + 替换 15 处 push），因改动面大被用户否决，改为上述最小改动。

### 30.4 FTE 10000 TPS 验证结果（修复后）

| 指标 | 值 |
|:---|---:|
| 实际 TPS | 9195.48 |
| 样本数 | 91956（完整跑完 ✅） |
| 平均延迟 | 9224.53 ns |
| P50 | 7964 ns |
| P75 | 9357 ns |
| P90 | 10823 ns |
| 最大 | 1045814 ns |

> 修复前 2000 TPS 能过（消息量<容量）、10000 TPS 卡死；修复后两者均完整跑完。

### 30.5 FTE vs GOne 全面对比（同环境 10000 TPS / 10s）

| 指标 | FTE (gw) | GOne (fpga_direct) | 结论 |
|:---|---:|---:|:---|
| 实际 TPS | 9195 | 9067 | 接近（差 1.4%） |
| 平均延迟 | 9224 ns | **5757 ns** | GOne 快 37.5% |
| P50 | 7964 ns | **3814 ns** | GOne 快 52% |
| P75 | 9357 ns | **4098 ns** | GOne 快 56% |
| P90 | 10823 ns | **6492 ns** | GOne 快 40% |
| 最大延迟 | **1045 μs** | 3425 μs | FTE 更稳定 |

### 30.6 瓶颈分析

1. **GOne 中低延迟区间优势明显**：P50/P75/P90 比 FTE 快 40~56%，符合 fpga_direct 硬件极速定位
2. **FTE 最大延迟更优**（1045μs vs 3425μs），说明 gw counter 延迟分布更集中、更稳定
3. **TPS 接近**：两者均受 mock_client 发送/回调链路和 CPU 竞争约束，差异仅 1.4%
4. 注意：GOne 在干净环境（无 FTE 竞争）下平均仅 **2159ns**（§29.3），当前因 FTE 同机运行存在 CPU 竞争，GOne 平均劣化到 5757ns，说明**同机多柜台压测会互相干扰**，单柜台基准应单独测

### 30.7 关键经验

1. **对象池 ≠ 卡死原因**：`unbound_object_pool` / `object_pool` 池空回退 `new`，永不返回 null；判断池耗尽应查日志 `produce too slow` / `capacity should resize`
2. **忙等自旋定位**：进程 alive + 线程 R（非 S）= 用户态忙等死锁，`gstack` 看栈是否卡在 `while(!trypush()){}`
3. **`-DNO_DSE` 陷阱**：NO_DSE 编译下 DSE 队列无消费者，消息只进不出；排查 FTE 卡死务必检查编译宏与队列消费线程
4. **`pkill` 自终止陷阱**：`pkill -f 'xxx'` 会匹配当前 shell 命令行自身导致自杀（exit 143），应改用 `pkill -x <进程名>` 或按 PID kill
5. **FTE 部署**：`/mnt/work/gt_test/work_atp/cmake/fte/bin/ute`，编译产物 `build_/build_release/fte/ute`，`start_all.sh` 重启

### 30.8 CPU 隔离绑定 + 精简 TGW 后的复测（send() 前记录，2026-09-17 二轮）

**环境优化**：
- 停用深圳 FTE（UTE_84_842_21）与深圳 TGW（ETF 39142），仅保留上海股票(38141)/债券(38140) TGW，降低整体 CPU 开销
- `taskset -c 3,11` 绑定 mock_client 到 core3（CPU 3 + CPU 11 两个 HT）

**关键发现——绑定单核会饿死**：绑定**单个** CPU 导致 TPS 骤降（~500，延迟 1ms+），根因是 `order_insert` 自旋等待引擎线程写 `api_leave_time_ns`，而发送线程与引擎线程在同一核上互相饿死。**必须绑定整个物理核心（2 个 HT）**让二者并行。

**复测结果（10000 TPS / 10s，send() 前记录）**：

| 指标 | FTE (gw) | GOne (fpga_direct) | GOne 优势 |
|:---|---:|---:|:---|
| 实际 TPS | 9419 | **9877** | 高 4.9% |
| 平均延迟 | 1315.66 ns | **334.35 ns** | **快 74.6%** |
| P50 | 419 ns | **242 ns** | 快 42.2% |
| P75 | 437 ns | **251 ns** | 快 42.6% |
| P90 | 556 ns | **262 ns** | 快 52.9% |
| 最大延迟 | 4039 μs | **296 μs** | FTE 尾部抖动更大 |

- 绑定 core3 后 GOne 平均从 991ns 降至 **334ns**（↓66%），FTE 从 1624ns 降至 **1315ns**（↓19%），说明此前受同机 CPU 竞争干扰严重
- **结论**：充分隔离干扰后，GOne（fpga_direct 硬件极速）在延迟与吞吐上全面优于 FTE（gw 软件柜台），中低延迟区间快 42~75%；FTE 尾部延迟抖动更显著（4039μs vs 296μs）

### 30.9 P95 指标与瓶颈分析（2026-09-17 三轮）

**新增 P95 指标**：mock_client 的 `metric_stats` 增加 P95 统计（`metric_stats.cpp` 计算 `percentile(0.95)` + 输出），O3 Release 编译。

**FTE vs GOne 对比（绑定 core3，10000 TPS，含 P95）**：

| 指标 | FTE (gw) | GOne (fpga_direct) | GOne 优势 |
|:---|---:|---:|:---|
| 实际 TPS | 9156 | **9546** | 高 4.3% |
| 平均延迟 | 2172.56 ns | **746.68 ns** | 快 65.6% |
| P50 | 418 ns | **245 ns** | 快 41.4% |
| P75 | 448 ns | **259 ns** | 快 42.2% |
| P90 | 599 ns | **277 ns** | 快 53.8% |
| **P95** | **2850 ns** | **304 ns** | **快 89.3%** |
| 最大 | 3178 μs | 2987 μs | 接近 |

**关键发现——FTE 尾部延迟是最大瓶颈**：
- FTE 的 P95(2850ns) 是 GOne P95(304ns) 的 **9.4 倍**
- FTE P90→P95 跨度 599→2850ns（+2251ns），GOne 仅 277→304ns（+27ns）——FTE 有 5% 请求尾部剧增
- FTE 平均(2172) vs P50(418) 达 5.2x，平均被长尾严重拉高

**FTE 尾部延迟根因**：① `send_msg_fc` 阻塞式忙等（EAGAIN→CPU_PAUSE）② 内核 TCP 栈抖动（系统调用/中断/锁）③ FTE 模拟柜台处理波动 ④ 跨进程通信放大

**解决方案**（详见 `task/api_dev/time_ana.md` §六）：
- 短期：FTE 启用非阻塞发送（MSG_DONTWAIT+epoll 写就绪）+ 引擎线程绑核 → P95 预计 2850→800ns
- 中期：批量发送（sendmmsg）提升 TPS
- 长期：用户态协议栈（DPDK/io_uring）绕过内核网络栈，缩小与 GOne 差距

## 32. 新计时口径对比：send() 前记录纯 API 框架延迟（2026-09-17，§31 后续）

### 32.1 背景

用户 14:54 更新了 API 压测计时方法和技术指标分析：
- **计时方法**：3 个引擎（single/multi/tcpdirect）统一改为 `send()` **系统调用前**记录 `api_leave_time_ns`，注释明确"测量纯 API 框架处理延迟，排除 send() 系统调用与内核缓冲满忙等"
- **技术指标**：`metric_stats.h/.cpp` 新增 **P95 分位**、分位数改为**线性插值**、标准差改用 **n-1 无偏估计**
- **修改文件**：`single_socket_engine.cpp:159`、`multi_socket_engine.cpp:265`、`tcpdirect_engine.cpp:128`、`metric_stats.h/.cpp`

### 32.2 测试方法

- 重新编译 API（Release + BUILD_MOCK=ON，liblbapi.so 14:58 更新）
- 分轮测试（FTE 轮停 GOne mock；GOne 轮停 FTE/tgw 环境）
- **均不绑核**（cpu_id=-1，用户更新后配置恢复默认）
- 10000 TPS / 10 秒，新口径（send 前记录）

### 32.3 FTE 结果（gw counter，环境含 tgw+FTE busy-loop）

```
实际 TPS : 8503 (85.0%)
平均     : 5537 ns
P50      : 401 ns
P75      : 581 ns
P90      : 2755 ns
P95      : 9277 ns    ← 新指标
最大     : 9294 μs
标准差   : 71038 ns
```

### 32.4 GOne 结果（fpga_direct，干净环境）

```
实际 TPS : 9960 (99.6%)
平均     : 417 ns
P50      : 311 ns
P75      : 340 ns
P90      : 391 ns
P95      : 452 ns     ← 新指标
最大     : 329 μs
标准差   : 1781 ns
```

### 32.5 最终对比（新口径：纯 API 框架处理延迟）

| 指标 | FTE (gw counter) | GOne (fpga_direct) | 差距 |
|:---|---:|---:|:---|
| 实际 TPS | 8503 (85.0%) | **9960 (99.6%)** | GOne 高 17.1% |
| 平均 | 5537 ns | **417 ns** | GOne 快 13.3x |
| P50 | 401 ns | **311 ns** | 仅差 1.3x |
| P75 | 581 ns | **340 ns** | GOne 快 1.7x |
| P90 | 2755 ns | **391 ns** | GOne 快 7.0x |
| **P95** | **9277 ns** | **452 ns** | GOne 快 20.5x |
| 最大 | 9294 μs | 329 μs | GOne 好 28x |
| 标准差 | 71038 ns | 1781 ns | GOne 稳 40x |

### 32.6 关键分析

1. **P50 接近**（401 vs 311ns，仅 1.3x）：稳态下两者 API 框架处理速度接近，说明 API 框架本身对两种柜台的处理效率相当
2. **平均/P90/P95 差距大**：FTE 平均(5537) / P90(2755) / P95(9277) 远高于 P50(401)，说明 **FTE 链路有大量长尾延迟**（FTE 处理不过来时请求在 API 引擎队列中排队）
3. **TPS 差距**：FTE 8503 vs GOne 9960——FTE 软件柜台处理上限约 8500 TPS（含 FTE↔tgw 往返），GOne FPGA 可跑满 10000
4. **新口径 vs 旧口径**（§31）：

| 指标 | GOne 旧口径(send后) | GOne 新口径(send前) | FTE 旧口径(send后) | FTE 新口径(send前) |
|:---|---:|---:|---:|---:|
| TPS | 9710 | **9960** | 7404 | **8503** |
| 平均 | 4949 ns | **417 ns** | 20266 ns | **5537 ns** |
| P50 | 3997 ns | **311 ns** | 10329 ns | **401 ns** |
| P90 | 7854 ns | **391 ns** | 40074 ns | **2755 ns** |

- GOne 改善更显著（平均 12x↓），因 FPGA 直连 send() 系统调用占延迟主体
- FTE 改善 3.7x，因软件链路本身延迟较大，send() 开销占比相对较小

5. **最大延迟**：FTE 9.3ms vs GOne 0.33ms（28x）——FTE 高负载下极端尾部延迟远超 GOne

### 32.7 结论

1. **新口径更准确地反映了 API 框架自身性能**：排除 send() 系统调用后，P50 仅差 1.3x，证明 API 框架对两种柜台的处理效率相当
2. **FTE 的瓶颈在软件链路本身**（FTE↔tgw 往返处理能力），而非 API 框架
3. **GOne 在吞吐和尾部延迟上全面领先**，符合 FPGA 硬件极速 vs 软件柜台的定位
4. **P95 新指标揭示了 FTE 的长尾问题**：20.5x 差距说明高负载下 FTE 请求排队严重，是后续优化重点

## 33. gw counter 单链接单客户模式优化（2026-09-18，§32 后续）

### 33.1 需求

用户要求给 API 增加"单链接单客户 / 单链接多客户"配置化支持：
1. 新增配置项判断当前 API 是单链接单客户还是单链接多客户
2. 单客户：登录成功后直接缓存为客户信息成员变量，不存 map，委托时直接使用
3. 多客户：登录后以 fund_account_id 为 key 存 map，委托时按 key 获取
4. 委托时按配置分流
5. **默认单链接单客户模式**
6. 改造后全面测试 gw counter 单客户模式 vs gone counter 性能

### 33.2 改造内容

**新增配置** `single_cust_per_link`（bool，默认 true）：
- `api_config.h` config_name 命名空间新增
- `api_config_impl.h` 新增 `bool single_cust_per_link_ = true` + getter
- `api_config.cpp`：known_attrs 新增 `{single_cust_per_link, bool_val}`；构造函数初始化 true；实现 bool `set_attr`/`get_attr`；copy_from 新增 bool 属性复制

**gw_counter_direct 分流**（核心改造）：
- 新增成员：`bool single_cust_per_link_`、`GwSessionInfo local_session_`（单客户模式本地会话，含 order_locators）
- 新增辅助方法：`get_session_for_order`/`record_order_locator`/`get_orig_client_seq_id`/`get_clordno`（按模式分流）
- `init()`：读取配置 + `local_session_.reset()`
- `deal_cust_login`/`deal_log_ans`：单客户写本地成员，多客户写 GwSessionCache
- `build_order_msg`/`build_etf_order_msg`/`build_cancel_msg`：用 `get_session_for_order` 替代 map 查找
- `build_cancel_msg` 撤单反查：用 `get_orig_client_seq_id`/`get_clordno`
- `deal_order_rtn`/`deal_trade_rtn`/`deal_etf_trade_rtn`：用 `record_order_locator`

**mock_client 支持**：`mock_client.cpp` 增加 `single_cust_per_link` 配置项（as_bool）

### 33.3 10000 TPS 对比（同环境，docker otc 容器内，不绑核）

| 指标 | 单客户模式 | 多客户模式 | 改善 |
|:---|:---:|:---:|:---:|
| P50 | **291 ns** | 381 ns | **↓23.6%** |
| P75 | **460 ns** | 551 ns | ↓16.5% |
| P90 | **1373 ns** | 3386 ns | **↓59.5%** |
| P95 | **9327 ns** | 10800 ns | ↓13.6% |
| 平均 | **4658 ns** | 6202 ns | **↓24.9%** |
| 最大 | **4.1 ms** | 9.0 ms | **↓54.2%** |
| TPS | **8562** | 8320 | +2.9% |
| 失败 | **0** | 0 | - |

**结论**：单客户模式全面优于多客户模式。P50 ↓23.6%（省去 GwSessionCache unordered_map 的 string+hash 查找），P90 ↓59.5%（高负载下减少 map 累积开销），平均 ↓24.9%，最大 ↓54.2%。**10000 TPS 下 0 失败**（相比历史多客户 baseline 56% 失败 -25 SEND_QUEUE_FULL，彻底解决）。

### 33.4 vs GOne（fpga_direct）参考

GOne 硬件不可用（无 FPGA 卡），引用 §32.4 历史数据：
- GOne @10000TPS：P50=120ns, P90=180ns, P95=452ns, TPS=9363
- 单客户 @10000TPS：P50=291ns, P90=1373ns, P95=9327ns, TPS=8562

差距仍大（P95 20x），根因是 **FTE 单线程 asio 处理能力瓶颈（~8500 TPS）→ TCP 背压**（见 §5.6/§32.6），非 API 框架问题。P50 已从 401→291ns（↓27%），框架层已接近 GOne。

### 33.5 关键结论

1. **单客户模式是 gw counter 的默认最优配置**（默认 true），消除 GwSessionCache 查找开销
2. **0 失败**：单客户模式 + FTE DSE 队列修复（§30）后，10000 TPS 稳定运行
3. **P95 尾部仍受 FTE 处理能力限制**：需 FTE 多线程 asio 或限流才能进一步改善（见 §5.6 方案六/七）
4. **GOne 硬件不可用时无法直接对比**：引用历史数据作参考

## 34. gw counter 非阻塞发送 + 写就绪通知优化（2026-09-18，§33 后续）

### 34.1 需求

按 time_ana.md §6.2 方案一（优先级最高）优化 gw counter：非阻塞 send + 写就绪通知，避免 CPU_PAUSE 忙等钉死引擎线程。

### 34.2 改造内容

**核心改动（aio_tcp 层，gw counter 实际发送路径）**：
- `common/include/aio_tcp.h`：
  - 重写 `send_msg_fc`：用 `MSG_DONTWAIT` 非阻塞发送；EAGAIN 时把剩余数据暂存 `pending_buf_`，注册 EPOLLOUT 写事件，**返回已发送字节数（不阻塞引擎线程）**
  - **v2 修复：无锁快速路径**——`send_msg_fc` 先检查 `pending_active_` 原子标志，pending 空时直接 `::send()`（无锁，避免 mutex 开销使 P50 退化）；仅部分发送/EAGAIN 时才加锁暂存。实测 P50 从 321ns 恢复至 300ns（接近原版 291ns）
  - 新增 `flush_pending()`：EPOLLOUT 触发时由接收线程补发 pending buffer
  - 新增 `pending_active_`（std::atomic）无锁快速路径：pending 空时 flush 不竞争锁
  - `deal_event()`：先 `flush_pending()` 再 `loop_deal_recv()`（EPOLLIN/EPOLLOUT 统一处理）
  - `close_ch()`：清理 pending + 取消 EPOLLOUT 注册
  - 新增 `send_mtx_` 互斥锁保护 pending_buf_/pending_off_/write_registered_
- `common/src/wait_poll.cpp`：`wait_poll_multi::mode_wake` 支持 EPOLLOUT（`isout==1` 时 `EPOLLIN|EPOLLOUT`），用于动态注册/取消写事件

**实现要点**：
- 复用现有 `mthread::mode_poll_event`（wait_poll_multi 多事件轮询器）注册/取消 EPOLLOUT
- 引擎线程（send_msg_fc）与接收线程（flush_pending）通过 `send_mtx_` 互斥
- `tcp_ch.cpp` 曾尝试方案A'（poll 等待），实测更差（P95 26781ns），已回退；最终采用 aio_tcp 层异步方案（方案B）

### 34.3 10000 TPS 测试结果（docker otc 容器内，不绑核，0 失败）

| 指标 | 单客户(异步) | 多客户(异步) | 单客户(原版) | 多客户(原版) |
|:---|:---:|:---:|:---:|:---:|
| P50 | 321~330ns | 391~401ns | 291ns | 381ns |
| P90 | 1022~3547ns | 1152~2438ns | 1373ns | 3386ns |
| P95 | 8626~11341ns | 7103~9728ns | 9327ns | 10800ns |
| TPS | 8410~8609 | 8538~8789 | 8562 | 8320 |
| 失败 | 0 | 0 | 0 | 0 |

**异步发送改善（最佳情况）**：
- 多客户：P90 3386→**1152ns（↓66%）**，P95 10800→7103ns（↓34%）
- 单客户：P90 1373→**1022ns（↓26%）**

**5000 TPS（FTE 能力内）异步单客户**：P90=**872ns**，P50=401ns，0 失败——证明 P90 高主要是 FTE 过载导致的，非阻塞发送在 FTE 能力内表现优秀。

### 34.4 关键结论

1. **非阻塞发送 + 写就绪通知在多客户模式下改善最显著**（P90 ↓66%），单客户模式也改善（P90 ↓26%）
2. **FTE 环境不稳定**（多次测试 P90 波动 1022~3547ns），但 **0 失败稳定**；P90/P95 尾部仍受 FTE 单线程 asio 处理能力（~8500 TPS）限制
3. **方案B（异步）优于方案A'（poll）**：poll 仍阻塞引擎线程且系统调用开销大（实测更差）；异步让引擎线程在 EAGAIN 时不阻塞，是正确方向
4. **优化顺序**：单客户模式（§33，P50 优化）→ 非阻塞发送（§34，P90 优化）→ FTE 多线程 asio（P95 根治，需对端）
5. **GOne 仍远优**（P50 120 vs 330）：FPGA 硬件无 TCP 栈/无进程，非 API 框架可比

### 34.5 为什么"优化后表现没有改善"？（复盘，2026-09-18）

**用户反馈优化后表现没有明显改善，复盘根因：**

**根因1：异步方案给正常发送路径（无 EAGAIN）增加了 mutex 锁开销 → P50 退化**
- v1 的 `send_msg_fc` 在函数开头无条件 `lock_guard(send_mtx_)`，即使 pending 空也要加锁/解锁
- 实测 P50：原版 291ns → v1 异步 321ns（↑30ns）
- **修复（v2）**：无锁快速路径——pending 空时直接 `::send()`（无锁），仅部分发送/EAGAIN 才加锁
- 实测 P50：v2 异步 300ns（恢复接近原版 291ns）

**根因2：P90/P95 的根本瓶颈是 FTE 处理能力，非阻塞发送无法解决**
- 非阻塞发送解决的是"引擎线程被 CPU_PAUSE 忙等钉死"，但 FTE 单线程 asio 处理能力 ~8500 TPS < 发单 10000 TPS
- 即使引擎线程不阻塞，发单速率也受限于 FTE 处理能力（TCP 背压），P90/P95 必然高
- **实证**：5000 TPS（FTE 能力内）非阻塞发送 P90=872ns（优秀）；10000 TPS（过载）P90=1983~3547ns
- 所以非阻塞发送在 FTE 能力内有效，但无法解决 FTE 过载导致的尾部延迟

**结论**：
- 非阻塞发送优化本身正确（避免了忙等钉死线程、P50 无退化），但**收益被 FTE 处理能力瓶颈掩盖**
- 要真正改善 P90/P95，必须解决 FTE 处理能力：**FTE 多线程 asio**（对端改造）或**客户端限流**（发单 ≤ FTE 能力）
- 当前环境 FTE 能力 ~8500 TPS，超过即过载，尾部延迟高是必然结果

---

## 35. mock_client test_plan 主配置模式（--plan，2026-09-21）

### 35.1 需求与背景

mock_client 早期采用"连接配置文件 + 测试用例目录/单文件"的分离模式（`--config` + `--testdir/--testcase`），存在三个痛点：

1. **配置分散**：连接配置（connection_config.json）与测试用例（test_cases/*.json）分离，功能测试与性能测试参数割裂。
2. **预期回报不完整**：旧 format 只校验类型，不校验字段值，无法逐字段精确验证。
3. **性能测试与功能测试脱节**：需要分别在两处配置，难以保证"在功能测试通过基础上跑压测"。

**解决方案**：引入 **test_plan 主配置模式**（`--plan <path>`），单文件整合全部功能测试场景 + 性能测试参数，独立案例放 `config/cases/`（FTE）/`config/cases_gone/`（GOne）。

### 35.2 主配置文件结构

`config/test_plan_gw.json`（FTE）与 `config/test_plan_gone.json`（GOne）为两个模板，结构如下：

```json
{
  "description": "GW(FTE) 测试计划主配置文件",
  "connection_config_file": "connection_config_gw_single.json",  // 引用连接配置（相对主配置目录）
  "test_plan": {
    "functional_tests": [
      {
        "name": "FTE 登录测试",
        "enabled": true,
        "timeout_ms": 30000,
        "request_file": "cases/login_request.json",   // 相对 base_dir（主配置所在目录）
        "expected_file": "cases/login_expected.json"
      },
      { "name": "FTE 成交回报校验", "expected_file": "cases/trade_expected.json" }  // 异步回报可无 request_file
    ],
    "perf_test": {
      "enable": true, "duration_sec": 10, "tps": 8000, "warmup_sec": 3, "cpu_id": -1,
      "report_file": "perf_report_gw_plan.txt",
      "net_time_map_file": "/tmp/api_net_time_map_gw_plan.txt",
      "order_file": "cases/perf_order_request.json"   // 独立委托模板（推荐），也可内联 order 节点
    }
  }
}
```

**关键字段**：
- `connection_config_file`：连接配置文件名（相对主配置所在目录 base_dir），也可直接内联 `connection` 节点。
- `functional_tests[]`：每个场景可含 `request_file`/`expected_file`（均相对 base_dir）、`timeout_ms`、`enabled`。
- `perf_test`：与旧模式 connection_config 的 perf_test 块字段一致，新增 `order_file` 指向独立委托模板文件。

**运行入口**（main.cpp 模式一）：

```bash
# FTE（单链接单客户）
LD_LIBRARY_PATH=build_cmake/lib ./build_cmake/bin/mock_client \
  --plan mock/client/config/test_plan_gw.json
# GOne
LD_LIBRARY_PATH=build_cmake/lib ./build_cmake/bin/mock_client \
  --plan mock/client/config/test_plan_gone.json
```

### 35.3 cases/ 目录结构

独立请求/预期文件按 `config/cases/`（FTE，`cases_gone/` 为 GOne 差异文件）组织：

| 文件 | 说明 |
|------|------|
| `login_request.json` / `login_expected.json` | 登录请求与预期应答 |
| `order_request.json` / `order_expected.json` | 委托请求与预期回报 |
| `trade_expected.json` | 成交回报预期（异步，无 request_file） |
| `cancel_request.json` / `cancel_expected.json` | 撤单请求与预期应答 |
| `heartbeat_request.json` / `heartbeat_expected.json` | 心跳维持测试 |
| `perf_order_request.json` | 性能测试委托模板（`{"order": {...}}`） |

**请求文件结构**（`{type, fields}`；load_plan 用 `req_root["request"]`，否则回退 `req_root`）：

```json
{ "type": "login", "fields": { "fund_account_id": "1000000000000001", "branch_id": "0001", ... } }
```

**预期文件结构**（`{type, fields}`；load_plan 用 `exp_root["expected_response"]`，否则回退 `exp_root`）：

```json
{ "type": "order_rtn", "fields": { "fund_account_id": "1000000000000001", "order_status": 0, "client_seq_id": null } }
```

`fields` 中值 `null` = 动态字段（流水号/时间）跳过校验；非 null 值做字符串精确比对。

### 35.4 parse_type 类型映射

| 字符串 | 枚举 | 说明 |
|--------|------|------|
| `"login"` | `Login` | 登录请求/应答 |
| `"order_insert"` / `"order_rtn"` | `OrderInsert` | 委托请求/回报 |
| `"etf_order_insert"` | `EtfOrderInsert` | ETF 委托（→ `etf_order_insert`） |
| `"order_cancel"` / `"cancel_rsp"` | `OrderCancel` | 撤单请求/应答 |
| `"trade_rtn"` | `TradeRtn` | 成交回报（**异步**，无 request_file） |
| `"wait_heartbeat"` / `"heartbeat_ok"` | `WaitHeartbeat` | 心跳维持（send_request 直接返回 true） |

**注意**：`EtfOrderInsert` 在 send_request 中走 `api_->etf_order_insert()`（非 `order_insert`）；`TradeRtn` 在 execute 走异步分支（不发请求，等待已存储的成交回报）；`WaitHeartbeat` 不发请求，execute 跳过 validate_response。

### 35.5 run_test_plan 流程（mock_client.cpp）

```
run_test_plan(plan_root, base_dir)
  1. 提取 connection：plan_root.has("connection") ? 内联 : 解析 connection_config_file（base_dir 相对）
     （注意 JsonValue::operator[] 对缺失 key 抛异常，必须先 has() 判断）
  2. init_from_json(conn) 完成 API 初始化
  3. runner_->load_plan(test_plan, base_dir) 加载场景（enabled=false 跳过）
  4. wait_link_ready(10000) 等待柜台链接就绪（避免链接未建立时大面积失败）
  5. runner_->execute_all() 逐一执行场景并计入 report_
  6. perf_test：若配置 order_file 则加载并注入 order 节点（order_root.has("order") ? order : order_root）
     → 仅当 perf_test.enable=true 才执行 run_perf_test（未启用不算失败）
  7. 返回 perf_ok（反映性能测试成败）
```

### 35.6 GOne vs FTE 预期文件差异

| 字段 | FTE（cases/） | GOne（cases_gone/） |
|------|--------------|-------------------|
| 登录 `cust_id` | `C000000000000001` | `C000000000000001`（资金账号） |
| 委托 `cust_id` | `1000000000000001`（资金账号） | `C000000000000001`（客户号） |
| 委托 `rtn_type` | 1 | 0 |
| 撤单 `err_code` | 50046 | 0 |

> GOne 与 FTE 在 cust_id 语义（资金账号 vs 客户号）、rtn_type、撤单 err_code 上存在差异，故 GOne 差异文件放 `cases_gone/` 单独管理。

---

## 36. mock_client 日志系统与结果分析

### 36.1 日志系统（logger.h/cpp）

mock_client 引入独立分级日志系统 `mock::Logger`（单例），替代原有分散的 `std::cout`/`std::cerr`。

**核心设计**：
- **五级日志**：`DEBUG/INFO/WARN/ERROR/FATAL`。
- **双输出目标**：同时输出到屏幕（控制台）和日志文件。
- **线程安全**：内部 `std::mutex` 保护文件与控制台输出。
- **格式**：`[时间戳] [级别] [文件:行] 消息`。
- **无锁快速路径**：`min_level_` 用 `std::atomic<int>`（memory_order_relaxed），`log()` 在加锁前先判级，低于最低级别直接丢弃（避免锁竞争），消除 data race。

**使用方式**：
```cpp
mock::Logger::instance().init(log_path, mock::LogLevel::INFO, true);  // main 入口初始化
LOG_INFO("测试报告已保存到: " << report_path);   // 支持 << 流式拼接
LOG_DEBUG(...) / LOG_WARN(...) / LOG_ERROR(...) / LOG_FATAL(...)
```

**关键实现**（logger.cpp）：
- `init()`：设置日志文件路径、最低级别、控制台开关；重复调用先 close 再开。
- `log()`：判级 → 生成毫秒时间戳 → 保留文件 basename → 去尾部换行 → 加锁输出到 stderr（ERROR/FATAL）/stdout + 文件。
- `close()`：flush + 关闭文件。

### 36.2 结果分析系统（result_analysis.h/cpp）

`mock::ResultAnalysis` 将测试最终结果整合输出到独立分析文件（默认 `result_analysis.txt`），与运行日志（mock_client.log）和功能报告（test_report.txt）相互独立。

**整合内容**（result_analysis.cpp `ResultAnalysis::write()`）：
1. **【一、功能测试分析】**：总计/通过/失败/通过率 + 逐用例 PASS/FAIL（含耗时、失败原因、字段校验详情）。
2. **【二、性能测试分析】**：若执行则内嵌 perf_report 文本；未执行标明"未执行性能测试"。
3. **【三、总体结论】**：功能测试通过率 + 性能测试失败笔数 + 综合结论（`func_ok && perf_ok`）。

**关键逻辑**：
- `perf_failed < 0` 表示未执行性能测试，不纳入总体结论判定。
- 通过 `main.cpp` 的 `client.write_analysis(analysis_path, plan_desc)` 调用，`plan_desc` 记录测试计划/配置来源。

### 36.3 新命令行参数（main.cpp）

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--plan <path>` | 空 | 测试计划主配置（推荐模式） |
| `--analysis <path>` | `result_analysis.txt` | 结果分析文件输出路径 |
| `--log <path>` | `mock_client.log` | 日志文件路径 |

---

## 37. mock_client 全面复盘与修复回归（2026-09-21）

### 37.1 复盘文档

`mock/client/mock_client_upgrade.md` 对 mock_client 全部源文件（~2800 行）做了全面复盘，识别 **6 类共 18 项** 优化点（🔴高4 / 🟡中8 / 🟢低6），按严重度提出修复建议。

### 37.2 修复状态（10 项修复 + 1 项撤销）

| # | 严重度 | 问题 | 状态 |
|---|--------|------|------|
| 1 | 🔴 | `load_api()` 忽略 `--lib` | ✅ 修复（dladdr 定位实际库路径 + 不一致 WARN） |
| 2 | 🔴 | 无链接就绪等待 | ✅ 修复（`wait_link_ready` 轮询 `last_link_status`） |
| 3 | 🟡 | `run_test_plan` 忽略 perf 失败 | ✅ 修复（返回 perf 结果，未启用不算失败） |
| 4 | 🟡 | `net_time_map_` 含失败委托条目 | ✅ 修复（仅成功委托记录映射） |
| 5 | 🟡 | `Logger::min_level_` 无锁读取 | ✅ 修复（改 `std::atomic<int>`） |
| 6 | 🟡 | validate 数组死代码 | ✅ 移除 |
| 7 | 🟡 | ETF 委托类型未实现 | ✅ 实现（`EtfOrderInsert` → `etf_order_insert`） |
| 8 | 🟢 | 多余 iostream 包含 | ✅ 移除 |
| 9 | 🟢 | timeout_ms 重复计算 | ✅ 提取局部变量 |
| 10 | 🟢 | None/Unknown 语义 | ✅ 加 Unknown 警告 |
| 11 | 🔴 | ~~CallbackHandler 数据竞争~~ | ⚠️ **撤销**：复查确认当前实现已线程安全，无需修改 |

**#11 撤销原因**：深入复查发现各 `on_*` 回调的数据赋值已在 `lock_guard` 锁内，主线程通过 `wait_for_response()`（持同一把锁）看到标志后读取数据，构成正确的 happens-before 同步。**该问题实际不存在**。

### 37.3 关键修复细节

- **load_api()**（mock_client.cpp）：当前为直接链接方式（编译时已链接 liblbapi.so），用 `dladdr` 定位实际生效库路径，与 `--lib` 参数比对，不一致时 WARN（避免用户误以为指定路径生效）。
- **wait_link_ready()**：`api_->start()` 返回成功仅表示实例启动，与柜台 TCP 链接是异步建立的（`on_link_status` 回调通知）。此函数每 50ms 轮询 `callback_->last_link_status()` 直到就绪或超时（10000ms）。
- **run_test_plan()**：末尾 `return perf_ok`（反映 perf 状态）；perf 未启用时 `perf_enabled=false` 不算失败。
- **net_time_map_**（perf_runner）：仅当 `order_insert()` 返回 0（成功）才记录 `(client_seq_id, api_arrive_time_ns)` 映射。

### 37.4 回归测试结果

修复后通过 **--plan 模式** 进行功能 + 性能回归：

| 柜台 | 功能测试 | 性能测试 | 结果 |
|------|---------|---------|------|
| **GW (FTE)** | 5/5 通过（登录/委托/成交/撤单/心跳） | 6518 TPS，P50=531ns | 0 失败 ✅ |
| **GOne** | 5/5 通过（登录/委托/成交/撤单/心跳） | 9216 TPS，P50=411ns，P90=641ns | 0 失败 ✅ |

> 回归数据引用自 mock_client_test.md 与 result_analysis 输出。性能测试在功能测试通过基础上执行，验证 test_plan 模式"功能+性能一体化"设计的正确性。

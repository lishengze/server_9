# NewAPI 交易 API 框架知识库

> 本知识库系统化整理了 `trunk/NewAPI` 交易 API 客户端框架的架构、设计、数据流与待办任务。
> 面向后续接入/维护/开发人员，以及需要理解该框架的大模型。
>
> **来源**：`study/counter.md`、`study/question.md`、`study/技术实现.md`、`study/数据流转.md`、`study/产品使用.md`、`task/api_dev/api_dev_task.txt`
> **基线**：HEAD + 后续重构（g1 协议改版、v2.1 规范）

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
| `gw_counter_direct` | 个微协议 | 个微直连（协议未实现，当前留空） |

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
| **gw 柜台** | ⭐ 低 | 协议骨架已搭好，业务消息体全部留空 |
| **counter98** | ⭐ 低 | 请求入口骨架已搭好，消息构建和应答处理全部留空 |
| **查询应答** | ⭐ 低 | 请求已能发出，但应答未接入分发 |
| **登录/重连** | ⭐⭐ 中 | 断线自动重登、登录异常重试未实现 |
| **缓存结构** | ⭐ 低 | 各柜台缓存结构 todo |

---

## 15. 待完成任务清单

### 15.1 gw_counter_direct（个微柜台）

| 函数/位置 | 当前实现 | 需要完成 |
|-----------|---------|---------|
| `build_order_msg`（:154） | `(void)req; (void)o_buf;` | 依据正式个微协议构造消息 |
| `build_etf_order_msg`（:161） | 同上 | ETF 申购赎回 |
| `build_cancel_msg`（:168） | 同上 | 撤单 |
| `deal_order_req`（:53） | `take_len=sizeof(g1_msg_head)` 占位 | 计算消息体长度、填充 build |
| `deal_recv_msg`（:305） | 只实现 LOGIN_ANS/HEART_ANS | ORDER_RTN/TRADE_RTN/CANCEL_RSP 全 default 跳过 |
| `build_api_order_rej`（:369） | 只填部分字段 | 依据正式协议 |
| 链接状态管理（:449/:460） | 空 / login_state 重置被注释 | 建链后重新登录、断线重置登录态 |

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
# 网卡抓包时间分析设计方案（api_net_time）

> 版本：v1.0
> 日期：2026-09-18
> 目标：测量委托请求从**进入 API**（`api_arrive_time_ns`）到**真正从网卡发出**的时间差，统计相关指标。
> 归属：trunk/NewAPI/gone/api/mock/client（NewAPI 框架性能测试）
> 参考：旧 API `DYS-FRAMEWORK/api/gw_api_boost/ute_mock_client/api_net_time_design.md`（已验证的网卡转包方案）

---

## 1. 背景与目标

### 1.1 背景

NewAPI 框架（gw_counter_direct → FTE）已实现 API 内部打点（见 `perf_runner.cpp`）：

| 字段 | 记录时机 | 时钟 |
|------|---------|------|
| `api_arrive_time_ns` | `api_impl::order_insert()` 入口（请求刚进入 API） | CLOCK_MONOTONIC |
| `api_leave_time_ns` | 引擎线程在 `::send()` 前写入（send-before） | CLOCK_MONOTONIC |

已有指标：
- **框架处理耗时** = `api_leave_time_ns - api_arrive_time_ns`（入队 + 引擎消费，纯 API 逻辑）

但该指标止步于 `::send()` **调用前**。`::send()` 返回只表示数据已写入**内核 TCP 发送缓冲区**，**并不等于数据已从网卡实际发出**。数据从内核发送缓冲到真正从网卡发出，还经过：TCP 协议栈处理、IP 分片、网卡驱动入队、网卡硬件发送。

### 1.2 目标

**测量委托包从"进入 API"到"真正从网卡发出"的端到端时间差**：

```
lat = 网卡发出时间戳 - api_arrive_time_ns
```

该指标覆盖了 API 处理 + ::send() + 内核协议栈 + 网卡排队的完整链路，比"框架处理耗时"更贴近真实网络发送延迟，可用于：
- 验证 API 内部打点（send-before）是否准确反映真实发送时刻
- 定位内核协议栈 / 网卡排队引入的额外延迟
- 对比单客户/多客户、不同 TPS 下的真实发送延迟

---

## 2. 现有打点机制回顾

- 打点全部在 API 内部（`api_instance.cpp:301` 记录 `api_arrive_time_ns`，引擎线程写 `api_leave_time_ns`）。
- 委托请求经 `gw_counter_direct::build_order_msg()` 编码为网络包：
  ```
  [PktNewHeader 8B | TradeOrderReq 106B | 校验和 4B]  （共 118 字节）
  ```
- 网络包只包含 `TradeOrderReq`（FTE 协议结构），**不含** `api_arrive_time_ns`（该字段只在 API 层 `OrderReq` 对象中）。

### 2.1 关键结构定义

**PktNewHeader**（8 字节，前 4 字节大端）：

| 字段 | 类型 | 偏移 | 说明 |
|------|------|------|------|
| msg_id | uint32 BE | 0 | 消息号，委托请求 = 1003（`kPktOrderReq`） |
| msg_len | uint32 BE | 4 | 消息体长度 = sizeof(TradeOrderReq) = 106 |

**TradeOrderReq**（106 字节，`gw_head.h` L733，`build_order_msg` 手工序列化）：

| 字段 | 类型 | 偏移 | 说明 |
|------|------|------|------|
| fund_account_id | char[16] | 0 | |
| branch_id | char[10] | 16 | |
| account_id | char[12] | 26 | 会话补充 |
| cust_id | char[16] | 38 | 会话补充 |
| **client_seq_id** | int64 | **54** | mock_client 设为递增整数（perf_runner `build_order`） |
| agw_seq_id | int64 | 62 | 恒为 0 |
| security_id | char[8] | 70 | |
| market_id | uint16 | 78 | |
| side | char | 80 | |
| order_type | char | 81 | |
| order_qty | int64 | 82 | |
| order_price | int64 | 90 | |
| stop_px | int64 | 98 | |

---

## 3. 核心难点分析

### 3.1 难点一：网络包不含 `api_arrive_time_ns`

`api_arrive_time_ns` 只在 API 层 `OrderReq` 对象中，**不会进入网络包**（`TradeOrderReq` 是 FTE 协议结构）。

**不能修改 `TradeOrderReq` 结构**加该字段——因为 FTE 服务端用旧结构解析，包变长会导致解析错位或校验失败，破坏协议兼容性。

### 3.2 难点二：跨进程时钟对齐

抓包程序是独立进程，它的"网卡发出时间戳"与 mock_client 的 `api_arrive_time_ns` 需要**可比较**。

### 3.3 难点三：TCP 流式解析

委托包在 TCP 流中传输，需处理**粘包/半包**（拆包组包）。

### 3.4 难点四：网卡发出时间戳精度

抓包时间戳是内核发送路径打点（软件时间戳），非网卡硬件时间戳（需 PTP 硬件支持）。

---

## 4. 方案选型与原理

### 4.1 关联方案：client_seq_id 关联（不改协议）

由于网络包不含 `api_arrive_time_ns`，采用 **client_seq_id 关联**：

1. **mock_client**：发送委托时，将 `client_seq_id → api_arrive_time_ns` 映射缓存到内存，测试结束后写出到映射文件。
2. **抓包程序**：抓取发出的委托包，解析出 `client_seq_id`，从映射文件查得对应的 `api_arrive_time_ns`。
3. **计算**：`网卡发出时间戳 - api_arrive_time_ns`，统计指标。

**为什么可行**：
- `client_seq_id` 在 `TradeOrderReq` 偏移 54 处（网络包偏移 62），可稳定解析。
- mock_client 的 `client_seq_id = seq`（递增整数），perf_runner 在 `order_insert` 返回后读取 `req.client_seq_id` 与 `req.api_arrive_time_ns`。

### 4.2 字节序确认（关键）

**当前项目 `build_order_msg` 直接 `memcpy(p, &req.client_seq_id, 8)`（主机序直通，小端）**，未调用 `TradeOrderReq::encode()`（encode 会转大端，但 build_order_msg 未使用它）。

因此委托包中 `client_seq_id` 为 **x86 host 字节序（小端）**，与旧 API 一致。**抓包程序须用 `le64()`（小端）读取**。`PktNewHeader` 的 `msg_id`/`msg_len` 仍为大端（`ByteSwap32`）。

### 4.3 时钟对齐方案：统一使用 CLOCK_MONOTONIC

| 组件 | 时钟 | 说明 |
|------|------|------|
| mock_client `api_arrive_time_ns` | `perf_now_ns()`（CLOCK_MONOTONIC） | `api_event_msg.h:14` |
| mock_client `client_seq_id` | 递增整数 | 与 `api_arrive_time_ns` 同记录时刻 |
| 抓包程序网卡时间戳 | CLOCK_MONOTONIC | AF_PACKET + `PACKET_TIMESTAMP_MONOTONIC` |

三者都在同一台机器上，基于同一单调时钟基准（开机以来的单调时间），**可直接相减**，无需跨进程时钟同步。

> 注意：libpcap 默认时间戳是 CLOCK_REALTIME（墙上时钟），不可直接比较。必须显式设置抓包时间戳为单调时钟。容器/旧内核可能不支持 `PACKET_TIMESTAMP_MONOTONIC`，需启动时校准 `offset = CLOCK_REALTIME − CLOCK_MONOTONIC`。

### 4.4 抓包方案：AF_PACKET（原生 Linux，不依赖 libpcap 头文件）

| 方案 | 优点 | 缺点 |
|------|------|------|
| **AF_PACKET socket**（选用） | 原生接口，无第三方头文件依赖；可直接设置 `PACKET_TIMESTAMP_MONOTONIC`；代码可控 | 需 root 权限；需自行解析链路层/IP/TCP |
| libpcap | API 简单，有 BPF 过滤 | 缺开发头文件（`pcap.h`），需额外安装 libpcap-dev |

**选用 AF_PACKET** 的原因：
1. 系统有 libpcap 运行时库但**缺头文件**，避免额外安装依赖。
2. AF_PACKET 可直接用 `setsockopt(SOL_PACKET, PACKET_TIMESTAMP, PACKET_TIMESTAMP_MONOTONIC)` 获取单调时间戳。
3. 回环测试（`127.0.0.1:33001`）在 `lo` 接口，AF_PACKET 完全可控。

**抓包时间戳语义**：AF_PACKET 抓到的发出包，时间戳是内核在发送路径上打的时间点（接近网卡实际发出时刻）。回环接口 `lo` 上即为数据写入回环的时刻。

---

## 5. 架构设计

```
┌─────────────────────────┐
│      mock_client        │
│  ┌───────────────────┐  │
│  │  perf_runner       │  │
│  │  order_insert()    │  │
│  │   ↓                │  │
│  │  API 记录 api_arrive_time_ns │
│  │   ↓                │  │
│  │  ::send()          │  │ 数据写入内核
│  └─────────┬─────────┘  │
│            │            │
│  记录 (client_seq_id,   │
│   api_arrive_time_ns)   │
│  测试结束写出映射文件     │
└────────────┼────────────┘
             │ 映射文件
             ▼
┌─────────────────────────┐        ┌──────────────────────┐
│   api_net_time_capture   │ ◄───── │  lo 接口（AF_PACKET） │
│  抓包 → 解析委托包        │        │  127.0.0.1:33001 发出 │
│  → 提取 client_seq_id    │        └──────────────────────┘
│  → 关联 api_arrive_time  │
│  → 计算 网卡发出-arrive   │
│  → 统计指标输出           │
└─────────────────────────┘
```

### 5.1 组件清单

| 组件 | 文件 | 职责 |
|------|------|------|
| mock_client 改造 | `src/perf_runner.h/.cpp` | 记录 `(client_seq_id, api_arrive_time_ns)` 映射，测试结束写出 |
| 抓包分析程序 | `api_net_time_capture.cpp` | 抓包、解析、关联、统计 |
| 共享结构 | `api_net_time_common.h` | 解析常量、映射文件格式 |

---

## 6. 数据结构与协议解析

### 6.1 映射文件格式

```
# 每行: <client_seq_id> <api_arrive_time_ns>
1 1234567890123456789
2 1234567890123456789
...
```

- 文本格式，方便调试。
- 由 mock_client 在测试结束后写出（不引入实时 I/O 开销，避免影响性能测试）。
- 抓包程序测试结束后读取。

### 6.2 委托包应用数据解析

```
应用数据 = [PktNewHeader 8B | TradeOrderReq 106B | 校验和 4B]  （共 118 字节）
           └─ msg_id=1003 判定为委托请求 ─┘
client_seq_id 位于应用数据偏移 = 8(PktNewHeader) + 54(TradeOrderReq内) = 62，8 字节小端 int64
```

### 6.3 TCP 流重组

- 跟踪 TCP 连接（4 元组：src_ip/src_port/dst_ip/dst_port，端口含 33001）。
- 按 TCP seq 号累计字节流。
- 从字节流中按 118 字节定长切分完整委托包（PktNewHeader.msg_len 可校验）。
- 处理粘包（一个 TCP 段含多个包）与半包（一个包跨多个 TCP 段）。

---

## 7. 模块详细设计

### 7.1 mock_client 改造（perf_runner）

**新增成员**：
```cpp
std::vector<std::pair<int64_t, uint64_t>> net_time_map_;  // (client_seq_id, api_arrive_time_ns)
```

**修改 `run_benchmark()`**：
- 在 `order_insert` 返回后，读取 `req.client_seq_id` 与 `req.api_arrive_time_ns`，`emplace_back` 到 `net_time_map_`。

**新增方法**：测试结束后将 `net_time_map_` 写出到映射文件（路径由配置 `net_time_map_file` 或默认 `/tmp/api_net_time_map.txt`）。

**配置**：`perf_test` 块新增 `net_time_map_file`（可选，缺省不写）。

### 7.2 抓包程序（api_net_time_capture.cpp）

**命令行参数**：
```bash
api_net_time_capture --iface lo --port 33001 --map /tmp/api_net_time_map.txt [--duration 10] [--report report.txt]
```

**流程**：
1. `socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))`，bind 到 `lo`。
2. `setsockopt(SOL_PACKET, PACKET_TIMESTAMP, PACKET_TIMESTAMP_MONOTONIC)` + `setsockopt(SOL_SOCKET, SO_TIMESTAMPNS)`。
3. 循环 `recvmsg`，解析链路层（以太网 14B / SLL 16B / loopback 0B）→ IPv4 → TCP → 应用数据。
4. 过滤：目的端口 33001（发出的委托包），TCP 载荷非空。
5. TCP 流重组，提取完整委托包（msg_id=1003）。
6. 解析 `client_seq_id`（小端 le64，偏移 62），记录 `(client_seq_id, 网卡发出时间戳)`。
7. 测试结束后读取映射文件，关联计算 `lat = 网卡时间 - arrive`。
8. 统计均值/P50/P75/P90/P95/Max/Min/标准差，输出报告。

### 7.3 统计输出

```
========== 网卡抓包时间分析报告 ==========
抓包接口   : lo
目标端口   : 33001
抓包时长   : 10 秒
捕获委托包 : 5000
关联成功   : 5000
关联失败   : 0
---------------- 延迟指标（网卡发出时间 - api_arrive_time_ns, 纳秒）----------------
样本数     : 5000
平均值     : ...
P50/P75/P90/P95/Max/Min/标准差 ...
==========================================
```

---

## 8. 时钟对齐详细说明

- mock_client `api_arrive_time_ns` 基于 `perf_now_ns()`（CLOCK_MONOTONIC）。
- 抓包程序用 `PACKET_TIMESTAMP_MONOTONIC` 使 `recvmsg` 时间戳为 CLOCK_MONOTONIC。
- 同一机器上 CLOCK_MONOTONIC 基准一致，两值直接可比。
- **容器/旧内核兜底**：若 `PACKET_TIMESTAMP_MONOTONIC` 未生效，`SO_TIMESTAMPNS` 返回 CLOCK_REALTIME。启动时计算 `offset = CLOCK_REALTIME − CLOCK_MONOTONIC`；对每个包，若时间戳在启动时 monotonic ± 1 天内则视为 monotonic，否则视为 realtime 并减去 offset 转到 monotonic。
- 延迟 = 网卡发出时间戳 - api_arrive_time_ns，恒为正（网卡发出必然晚于 API 进入）。

---

## 9. 实现步骤

1. **mock_client 改造**：`perf_runner` 记录 `(client_seq_id, api_arrive_time_ns)` 映射，测试结束写出。
2. **开发抓包程序**：`api_net_time_capture.cpp`（AF_PACKET + 协议解析 + TCP 重组 + 关联统计）。
3. **编译**：抓包程序用 g++ 单独编译（纯 C++11，复用 `metric_stats.cpp`，无需 libpcap 头文件）。
4. **集成测试**：
   - 启动 FTE + tgw_simulator + counter98_mock。
   - 启动抓包程序（sudo，lo 接口，33001 端口）。
   - 运行 mock_client 性能测试（功能测试先完成 FTE 登录）。
   - 抓包程序输出报告，与已有 API 内延迟指标（框架处理耗时）对比。

---

## 10. 测试方案

| 用例 | 步骤 | 预期 |
|------|------|------|
| 功能验证 | 100 TPS 跑 5 秒，验证抓包关联成功数 = 发送数 | 关联成功率 100% |
| 延迟对比 | 对比 `网卡发出-arrive` 与 `框架处理耗时` | 前者 ≥ 后者（含内核协议栈+网卡排队） |
| 高 TPS | 5000 TPS 跑 5 秒 | 无丢包，关联稳定 |
| 异常处理 | 抓包程序先启动/后启动 | 后启动时关联失败数增加，给出提示 |

### 10.1 预期结论

- `网卡发出时间 - api_arrive_time_ns` 应略大于 `框架处理耗时`（`api_leave_time_ns - api_arrive_time_ns`），差额 ≈ 内核协议栈处理 + 网卡排队时间。
- 该指标更真实反映"委托从进入 API 到真正发出"的端到端延迟。

---

## 11. 风险与应对

| 风险 | 影响 | 应对 |
|------|------|------|
| 抓包程序需 root | 非 root 无法抓包 | 用 sudo 运行；文档说明 |
| 回环接口 lo 时间戳为软件时间戳 | 非硬件级精度 | 用于相对对比足够；如需硬件时间戳需网卡 PTP 支持 |
| TCP 粘包/半包 | 解析错位 | 严格按 seq 重组 + msg_len 校验 + 畸形包丢弃 |
| 高 TPS 下抓包丢包 | 关联成功率下降 | 设置更大 socket 接收缓冲（SO_RCVBUF）；必要时用 PACKET_RX_RING |
| 映射文件 I/O 影响性能 | 污染性能测试 | 映射文件测试结束才写出，测试期间零 I/O |
| 字节序误判 | 关联失败 | client_seq_id 用 le64（build_order_msg 小端直通），msg_id/msg_len 用 be32 |

---

## 12. 交付物

| 文件 | 说明 |
|------|------|
| `api_net_time_design.md` | 本设计方案 |
| `api_net_time_common.h` | 共享常量与结构 |
| `api_net_time_capture.cpp` | 抓包分析程序 |
| `src/perf_runner.h/.cpp`（改造） | mock_client 时间映射记录 |
| 测试报告 | 集成测试结果 |

---

## 13. 与旧 API 方案的差异（适配要点）

| 项 | 旧 API（ute_mock_client） | 当前 NewAPI | 适配 |
|------|------|------|------|
| 委托包结构 | [PktNewHeader 8B \| TradeOrderReq 106B \| 校验和 4B] | 相同 | 复用 |
| msg_id | 1003 | 1003（kPktOrderReq） | 复用 |
| client_seq_id 字节序 | 小端（memcpy host） | **小端（build_order_msg memcpy host）** | 复用 le64 |
| client_seq_id 网络偏移 | 62 | 62 | 复用 |
| api_arrive_time_ns 时钟 | steady_clock | CLOCK_MONOTONIC（perf_now_ns） | 兼容 |
| 统计结构 | mock::MetricStats | perf::MetricStats | 改用 perf::MetricStats |
| 关联 key | client_seq_id = perf_now_ns() | client_seq_id = 递增整数 | 兼容（仍是唯一 key） |
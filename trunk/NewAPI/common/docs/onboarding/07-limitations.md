---
purpose: LIMITATIONS
audience: AI agents and humans
source-of-truth: yes (comprehensive list of what's NOT supported)
last-reviewed: 2026-08-31
---

# Limitations：lbcommon 不支持什么

> 本文件汇总 lbcommon **明确不支持 / 未实现 / 不适用** 的功能。AI 在选型/实现前**必须**先读，避免浪费时间。
> 每条都给出"如果你需要 X 时怎么办"。

## 1. 网络协议

### 1.1 IPv6

> **不支持**：`CHANNEL_FAMILY_IPV6` 已定义但**未实现**。`tcp_ch.cpp` 显式校验 IPv6 返回 `LBERR_ARGV_WRONG`。

**强制**：所有 `channel_attr.family` 必须设为 `CHANNEL_FAMILY_IPV4`。

**如果业务需要 IPv6**：
- 把 IPv6 地址映射为 IPv4（不推荐）
- 在业务层加 NAT64 转换
- 提需求给 lbcommon 维护者

详见 [网络.md §1.1](../使用范式/网络.md#11-协议层comm_sock)。

### 1.2 TCP 协议级特性

| 特性 | 状态 | 替代 |
|------|------|------|
| TLS/SSL | 不内置 | 业务层用 OpenSSL/BoringSSL |
| HTTP/HTTPS | 不内置 | 业务层用 HTTP 库 |
| WebSocket | 不内置 | 业务层实现 |
| 多播/广播 TCP | 不适用 | TCP 是单播；多播用 UDP |

## 2. 异常

> **不支持**：lbcommon **不使用 C++ 异常**（`throw`/`catch`）。所有错误走错误码（`LBERR_*` 负值）。

**强制**：业务代码：
- 不要 `throw` 异常（lbcommon 接口不捕获，会直接 `terminate()`）
- 所有 lbcommon API 调用**必须**判返回值 `< 0`
- 业务内部错误码体系建议与 lberr 风格一致（负值 + 字符串消息）

如果业务内部使用异常：**用 try/catch 围栏在 lbcommon 调用边界**，转换为业务内部错误码。

详见 [00-AI-QUICKSTART §4](./00-AI-QUICKSTART.md#4-契约三元式速查)。

## 3. 操作系统

### 3.1 Windows / macOS

> **不支持**：`CMakeLists.txt` 显式 `set(CMAKE_SYSTEM_NAME Linux)` + `-D__linux__` + `-D_LINUX -DLINUX`。
> lbcommon 用 Linux epoll/eventfd/timerfd 等系统调用，无法移植。

**如果业务需要跨平台**：
- 业务层做平台抽象（libuv / Boost.Asio / 自实现）
- lbcommon 仅 Linux 路径使用

### 3.2 Linux 发行版

| 系统调用 | 最低内核要求 | 备注 |
|----------|--------------|------|
| `epoll` | 2.6 | Linux 标配 |
| `eventfd` | 2.6.27 | 2008 年后所有发行版 |
| `timerfd` | 2.6.25 | 2008 年后所有发行版 |
| `pthread_*` | 2.6 | glibc ≥2.4 |
| `CLOCK_MONOTONIC` | 2.6 | 2003 年后所有发行版 |

实际目标：**glibc ≥ 2.17**（CentOS 7 基准）。

### 3.3 root 权限

| 操作 | 是否需要 root | 备注 |
|------|---------------|------|
| 普通 TCP/UDP bind | 否 | 端口 < 1024 需要 root |
| `pthread_attr_setaffinity_np` + SCHED_RR | 是（root 或 CAP_SYS_NICE）| [线程.md §4.3](../使用范式/线程.md#43-绑核) |
| 大页（HugePage）| 是 | 业务可选用；lbcommon `map_shm` 支持 |

## 4. 共享内存场景

### 4.1 仅单进程写

> **强制约束**：`shm_block_pool` 与 `que_proc_buf` / `que_spw_fixed` **仅支持单进程写**。

**写进程 = 创建者**（`create_mem=1`, `is_create=1`）。

**读进程**：
- 只能 `init(create_mem=0)` + `addr2area` / `addr2block` / `addr2data` **只读**寻址
- 不可调 `get/take/back/release`（编译期不报错，运行期破坏共享内存链表）

详见 [内存池.md §5.4](../使用范式/内存池.md#54-读进程操作边界重要)。

### 4.2 ABI 约束

> **强制约束**：跨进程通信时，**所有进程必须用同一份 lbcommon 头文件**编译。

`shm_data_addr` 字段顺序是 ABI 敏感的（`area_id → data_id → block_id`）。不同版本 lbcommon 混用会跨进程寻址错乱。

详见 [内存池.md §5.3](../使用范式/内存池.md#53-三级寻址-shm_data_addr)。

### 4.3 崩溃恢复语义

| 队列 | 崩溃恢复 |
|------|----------|
| `que_proc_buf` | **丢数据**（崩溃块标记 `op_pid=PROC_BUF_PID_INVALID(-11)` 并跳过）|
| `que_spw_fixed` | 无崩溃检测（崩溃由外部保证）|

**业务必须容忍** `que_proc_buf` 崩溃恢复的少量数据丢失。

### 4.4 SHM 名长度

`shm_name` 大小假设类总大小 256 字节，依赖 vector/cmutex 实际大小。**跨平台可能编译失败**（详见 [内存池.md §9.10](../使用范式/内存池.md#9-避坑)）。

## 5. 编译与平台

### 5.1 C++ 标准

> **必须**：C++17。

lbcommon 用到 inline variable、structured binding、`std::atomic` 部分特性。

### 5.2 `-march` 对齐

> **必须**：业务项目与 lbcommon 用相同 `-march`（推荐 `-march=native`）。

lbcommon 内联 SIMD 函数（`_mm_*`）按特定指令集生成。不同 `-march` 编译会导致 illegal instruction 或 UB。

详见 [02-integration-guide §5](./02-integration-guide.md#5-编译选项对齐)。

### 5.3 优化级别

> **必须**：`Release` 必须 `-O2` 或 `-O3`。

`atomic_*` 内置函数在 `-O0` 下未优化，会大幅降低并发性能。

### 5.4 编译器

| 编译器 | 支持 | 备注 |
|--------|------|------|
| GCC ≥ 7 | ✅ | 主测 |
| Clang ≥ 5 | ✅ | clangd 可直接用 `compile_commands.json` |
| ICC / MSVC | ❌ | 不测 |

## 6. 架构限制

### 6.1 没有 `main()` / 没有 demo

> **不适用**：本仓库不是独立可运行工程——没有 `main()`、没有单元测试、没有官方 demo。

所有源文件最终被业务项目链接。**业务代码自己写 `main()`**。

### 6.2 没有 CHANGELOG

> **不适用**：当前未明确版本化策略。每次 commit 是独立的。

如需变更追踪：依赖 git log + `.repo_memory/`（AI cold-start）。

### 6.3 没有 API 自动生成文档（Doxygen 等）

> **不适用**：当前 47 个 header-only / inline 头文件规模，Doxygen 收益不明确。

业务代码读 API 直接读 `../common/include/*.h`。

### 6.4 没有包管理器（Conan / vcpkg / apt）

> **不适用**：lbcommon 是个人项目，不发布包。**业务项目用 CMake `add_subdirectory` 集成**（详见 [02-integration-guide §2](./02-integration-guide.md#2-cmake-模板业务项目)）。

### 6.5 没有公开仓库

> **不适用**：当前为本地仓库，无 GitHub/GitLab 公开。

## 7. 已知 bug / 残留

> 以下是代码内已知但**未修复**或**业务须自行规避**的问题：

| Bug 编号 | 描述 | 业务规避 |
|---------|------|----------|
| Bug-PO-7 | `dync_pool::take` 失败时 `free_num` 下溢（已修）| 用 `get_used()` 判断不用 `free_num` |
| Bug-PO-57 | `shmem_block_c1h` 重复 back 链表成环（已修）| 不手动改 flag |
| Bug-PO-58 | `dync_pool::take` 失败时 `o_block` 未设 NULL（已修）| 必查返回值 |
| §3 §无修复 | `dync_pool` 持有线程崩溃后状态不一致 → 永久阻塞 | 业务保证线程正常退出 |
| §无编号 | `cmutex_proc` robust 仅在 `_POSIX_THREAD_ROBUST_MUTEXES` 定义时生效 | 锁初始化时检查返回值 |
| §无编号 | `shmem_pool::shm_name` 256 字节依赖 vector/cmutex 实际大小 | 跨平台时关注 |

## 8. 不在 lbcommon 范围

| 业务需求 | 推荐替代 |
|----------|----------|
| HTTP 服务器 | 用 C++ HTTP 库（cpp-httplib / Crow / Drogon）|
| TLS/SSL | OpenSSL / BoringSSL |
| JSON 解析 | nlohmann/json / rapidjson |
| 数据库 | libpq / mysqlclient / sqlite3 |
| 序列化 | protobuf / flatbuffers |
| 配置热加载 | 自己写（lbcommon 提供 ini_reader / csv_reader）|
| 协程 | std::coroutine（C++20）或 libco |
| 跨平台 | 把 lbcommon 包在平台抽象层后 |

## 9. 已废弃 / 不推荐

| 项 | 替代 | 原因 |
|----|------|------|
| Solarflare TCPDirect | 当前**注释掉**（`CMakeLists.txt` L62-67）| 按需打开 |
| `simple_thread` 高并发场景 | `mthread` | `simple_thread` 仅适合单任务循环 |
| `aio_recv_que` 做请求应答 | `aio_recv_buf` / `aio_recv_pool` | 共享队列无法关联响应 |
| 多线程+`cmutex` 保护 `send_msg` | 事件投递 / 发送线程+队列 | EAGAIN 重试长持锁 |

## 10. 已知生态与集成

| 项 | 状态 |
|----|------|
| 与 OpenSSL 协同 | 可，业务层包 |
| 与 nlohmann/json 协同 | 可，业务层包 |
| 与 spdlog 协同 | 业务层可换掉 `lb_log`，但失去 lbcommon 异步日志特性 |
| 与 gRPC / Thrift | 业务层包 |
| 测试框架 | **无内置**——业务自选 gtest / catch2 |

## 11. 下一步

- 决策树：[04-decision-trees.md](./04-decision-trees.md)
- 坑点：[06-pitfalls-index.md](./06-pitfalls-index.md)
- 业务模板：[05-business-recipes.md](./05-business-recipes.md)
- 仓库导航：[../README.md](../README.md)

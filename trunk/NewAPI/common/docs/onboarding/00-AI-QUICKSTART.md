---
purpose: AI-QUICKSTART
audience: AI agents developing business code on lbcommon
source-of-truth: yes (for entry decisions and iron rules only; API details in 使用范式/*.md)
last-reviewed: 2026-08-31
---

# lbcommon AI 入口（30 秒读完）

> 你是 AI，正在为基于 `lbcommon` 的业务项目编写代码。本文件是你**进入项目后第一份必读**。
> 严格按本文件的路由表与铁律行动。**违反铁律 = 数据损坏或对象永不释放**。

## 1. 必读清单（按顺序）

| # | 文件 | 用途 | 期望耗时 |
|---|------|------|----------|
| 1 | [01-getting-started.md](./01-getting-started.md) | 30 行 hello TCP server，从 clone 到跑通 | 5 min |
| 2 | [02-integration-guide.md](./02-integration-guide.md) | CMake `add_subdirectory(./common)`、链接标志、命名空间 | 3 min |
| 3 | [03-glossary.md](./03-glossary.md) | 必须知道的术语（src_stat_ref、swr/mth/proc/spw、_buf/_fixed 等）| 5 min |
| 4 | [04-decision-trees.md](./04-decision-trees.md) | 选哪个组件（队列/锁/内存池/线程/网络）| 按需 |
| 5 | [07-limitations.md](./07-limitations.md) | **不要尝试这些事**（不支持的清单）| 3 min |

读完 1-3 + 5 即可动手；4 与 06 在选型/排错时按需查。

## 2. 路由表（按业务意图直达入口）

| 业务场景 | 入口文件 | 关键类/函数 |
|----------|----------|-------------|
| 写 TCP 服务器 | [05-business-recipes.md §1](./05-business-recipes.md#1-网络服务器组装) | `aio_tcp` + `mthread` + `que_mth_buf` |
| TCP 客户端长连接（带心跳/重连）| [05-business-recipes.md §3](./05-business-recipes.md#3-重连心跳组装) | `aio_tcp` + `heart_manage` + `reconnect_ctl` + `timer_thread` |
| 跨进程共享数据（单写多读）| [05-business-recipes.md §2](./05-business-recipes.md#2-跨进程共享组装) | `shm_block_pool` + `que_proc_buf` 或 `que_spw_fixed` |
| 高频线程间消息 | [../使用范式/队列.md §2](../使用范式/队列.md#2-选型决策树) | `que_mth_buf` 或 `que_swr_buf` |
| 多线程任务池 | [../使用范式/线程.md §1](../使用范式/线程.md#1-选型决策树) | `mthread_pool` |
| 定时任务（周期/单次）| [../使用范式/线程.md §5](../使用范式/线程.md#5-timer_thread--timer_order_list-定时器) | `timer_thread` |
| 跨进程键值索引（启动加载）| [../使用范式/哈希与数据结构.md §2](../使用范式/哈希与数据结构.md#2-选型决策树) | `hash_vec_pos` + `vec_fix_mem` |
| UDP 单播/组播 | [../使用范式/网络.md §1](../使用范式/网络.md#11-协议层comm_sock) | `aio_udp` |
| 进程级优雅退出 | [../使用范式/线程.md §6](../使用范式/线程.md#6-生命周期统一) | `install_signal_hand` |

## 3. 铁律（违反即数据损坏或引用泄漏）

| # | 铁律 | 违反后果 | 详见 |
|---|------|----------|------|
| 1 | `aio_tcp::send_msg` **非线程安全**；多线程并发 send 会让两条消息字节流交错拼接 | 对端收到乱序废包 | [网络.md §发送模式](../使用范式/网络.md#发送模式与线程安全) |
| 2 | `delive_ch_event(needadd=1)` 必须配 `end_ch_event(1)`；漏配 → 对象永不 `deal_after_closed` | 引用泄漏 | [网络.md §5](../使用范式/网络.md#5-tcp_ch-引用计数state_machineh-的-src_stat_ref) |
| 3 | `deal_ch_closed`（不是 `deal_ch_closing`）才释放对象资源 | use-after-free | [网络.md §5](../使用范式/网络.md#5-tcp_ch-引用计数state_machineh-的-src_stat_ref) |
| 4 | `que_swr_buf` / `que_mth_buf` **不存数据长度**；读者必须自备长度知识 | 后续所有消息读错位 | [队列.md §1](../使用范式/队列.md#1-命名约定选型前提) |
| 5 | `atomic_lock` **不支持递归**；同线程二次 lock = 死锁 | 永久阻塞 | [锁与同步.md §2](../使用范式/锁与同步.md#2-mlock7-锁选型矩阵--raii-守卫) |
| 6 | `shmem_pool` **仅单进程写**；读进程只能 `addr2data` 只读访问，不可调 `get/release/take/back` | 破坏共享内存链表 | [内存池.md §5.4](../使用范式/内存池.md#54-读进程操作边界重要) |
| 7 | 队列 `init(que_size)` 必须传 **2 的幂**；`mask = tnum - 1` 用位与寻址 | 数据错乱 | [队列.md §5.1](../使用范式/队列.md#51-2-的幂--mask所有队列通用) |
| 8 | 所有定长队列 `<T>` 与 `fix_pool<T>` / `dync_pool<N,T>` 的 T 必须 **POD**（无虚函数/无复杂构造析构）| `data2it`/`pos2blk` 用 `offsetof` 反算地址，T 有 vtable 偏移错位 | [内存池.md §6](../使用范式/内存池.md#6-pod-限制强制) |
| 9 | `aio_tcp` 的 `heart.on_msg()` **在 `deal_recv` 内被注释**——必须在 `deal_msg` 回调里**手动**调 `on_com_msg()` / `on_heart_msg()` | 心跳 3 倍超时误判 | [网络.md §6](../使用范式/网络.md#6-heart_manage-心跳) |
| 10 | `aio_tcp::connect_ch` 异步连接失败**不走** `deal_ch_closing`；直接 `deal_after_closed` → `deal_ch_closed` | 用户在 `deal_ch_closed` 必须能处理"连接失败" errcode | [网络.md §2/§9](../使用范式/网络.md#2-aio_tcp-完整生命周期) |
| 11 | 析构函数**不自动停**运行中的线程（`mthread` / `timer_thread`）；必须先 `to_stop()` + `join()` | pthread 句柄泄漏 + use-after-free | [线程.md §6](../使用范式/线程.md#6-生命周期统一) |
| 12 | 弱 CAS（`*_weak`）**必须包在循环里**且有退出条件 | 死循环或错过更新 | [锁与同步.md §1.3](../使用范式/锁与同步.md#13-强-cas-vs-弱-cas关键选型) |

## 4. 契约三元式速查

`lbcommon` 的所有 API 返回值统一语义：

| 返回值 | 含义 | 处理方式 |
|--------|------|----------|
| `> 0`（位置/长度）| 成功（位置或字节数）| 直接使用 |
| `0` | "空/满"语义（队列无数据、池耗尽等）| 业务侧自行重试或放弃 |
| `< 0` | 错误码（`LBERR_*`，范围 `-1 ~ -39`）| 必查 `perror_lb(o_errmsg, ret)` 取人类可读消息 |

`aio_tcp::connect_ch` 特殊三态（**唯一例外**）：

| 返回值 | 含义 | 后续动作 |
|--------|------|----------|
| `0` | 成功（同步或异步均已开始）| 等回调 `deal_ch_connect`（同步路径已回调）|
| `< 0` | 失败 | 清理 + 报错 |

> `tcp_ch::connect_ch` 底层三态是 `1=同步成功 / 0=异步进行中 / <0=失败`，但 `aio_tcp::connect_ch` 已归一化为 `0=成功`。**业务代码用 aio_tcp 时只看 `ret==0` 与 `ret<0`**。

## 5. 错误码范围速查（完整定义见 `include/comm_errno.h`）

| 范围 | 类别 | 典型码 |
|------|------|--------|
| `-1 ~ -10` | 基础（参数/属性/对象生命周期）| `LBERR_ARGV_WRONG(-1)`、`LBERR_NOT_SUPPORT(-2)`、`LBERR_MEM_ALLOC_FAIL(-3)`、`LBERR_OBJ_NOT_HAVE(-13)` |
| `-11 ~ -20` | 对象状态/读写 | `LBERR_OBJ_CLOSE_FAIL(-11)`、`LBERR_OBJ_IS_EMPTY(-18)`、`LBERR_OBJ_IS_FULL(-19)` |
| `-21 ~ -27` | 数量/时间 | `LBERR_OBJ_NUM_LIMIT(-21)`、`LBERR_TIME_WAIT_OUT(-27)` |
| `-28 ~ -29` | 时间/共享内存 | `LBERR_TIME_TAKE_FAIL(-28)`、`LBERR_SHM_MAP_FAIL(-29)` |
| `-30 ~ -33` | 锁/引用 | `LBERR_LOCK_NOT_CONSIST(-30)`、`LBERR_LOCK_TAKE_FAIL(-32)`、`LBERR_REF_ADD_FAIL(-33)` |
| `-34 ~ -39` | 通道 | `LBERR_CH_CONNECT_FAIL(-34)`、`LBERR_CH_LINK_BROKEN(-39)` |

## 6. 不要做这些事

> **不支持**：`CHANNEL_FAMILY_IPV6`（仅定义未实现） / Windows / macOS / 异常（throw/catch） / `main()` demo（库内无 main）。
> 详见 [07-limitations.md](./07-limitations.md)。

## 7. 出错怎么办

1. 看返回码，定位范围（按 §5）
2. 调 `perror_lb(o_errmsg, ret)` 取人类可读消息
3. 查 [06-pitfalls-index.md](./06-pitfalls-index.md) 是否命中已知坑
4. 查对应 [使用范式/*.md](../使用范式/) 详解
5. 如仍不解，**在 common 仓库内**通过 `/memorax-code` 查历史经验

## 8. 给业务项目 AI 的路径提示

业务项目仓库（与 `common/` 同级，CMake `add_subdirectory(./common)`）中的 AI 应通过相对路径访问本文档：

```
../common/docs/onboarding/00-AI-QUICKSTART.md   ← 本文件
../common/docs/onboarding/01-getting-started.md
../common/docs/onboarding/03-glossary.md
../common/include/aio_tcp.h                       ← 直接读头文件获取精确接口
../common/使用范式/网络.md
```

**注意**：memorax 是 git-repository 作用域，业务项目 AI 默认看不到 common 的 memorax 条目——只能读 committed 文档。

## 9. 配套结构

| 路径 | 内容 |
|------|------|
| `docs/onboarding/` | 本目录（AI 入口）|
| `docs/使用范式/*.md` | 6 个组级深度手册（队列/线程/内存池/网络/锁同步/哈希数据结构）|
| `docs/架构范式.md` | 3 个跨组系统组装模式（网络服务器/跨进程共享/重连心跳）|
| `docs/README.md` | 仓库导航（含场景速查 + 命名规范 + 分层）|
| `include/*.h` | 47 个头文件（header-heavy / inline 优先）|
| `.repo_memory/`（gitignored）| 本仓库 AI 的 cold-start 页（不在业务项目可见）|

## 10. 延伸阅读

- 选型 → [04-decision-trees.md](./04-decision-trees.md)
- 坑点 → [06-pitfalls-index.md](./06-pitfalls-index.md)
- 不支持项 → [07-limitations.md](./07-limitations.md)
- 跨组组装 → [../架构范式.md](../架构范式.md)
- 仓库导航 → [../README.md](../README.md)

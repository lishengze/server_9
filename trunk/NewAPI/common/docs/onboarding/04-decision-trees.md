---
purpose: DECISION-TREES
audience: AI agents selecting the right lbcommon component for a business need
source-of-truth: yes (single-page index; deep rationale in 使用范式/*.md)
last-reviewed: 2026-08-31
---

# Decision Trees：按业务意图选组件

> 本文件是**单页索引**，把 6 个 使用范式 的选型决策树合并呈现。
> 每个决策末尾给出 `→ <类名>`，深度的"为什么"链接到对应 [使用范式/*.md](../使用范式/) 章节。

## 1. 队列选型

**问题**：跨进程？并发度？数据形态？

```
跨进程？
├─ 否（单进程）
│   ├─ 写/读各 1 线程？
│   │   ├─ 变长 → que_swr_buf
│   │   └─ 定长 → que_swr_fixed<T>
│   └─ 多线程并发
│       ├─ 变长 → que_mth_buf
│       └─ 定长 → que_mth_fixed<T>
└─ 是（共享内存）
    ├─ 变长 + 多写多读 + 需崩溃恢复 → que_proc_buf
    └─ 定长 + 单进程写 + 多进程读 → que_spw_fixed<T>
```

| | 变长 | 定长 |
|---|---|---|
| 单写单读 | `que_swr_buf` | `que_swr_fixed<T>` |
| 多线程并发 | `que_mth_buf` | `que_mth_fixed<T>` |
| 多进程 | `que_proc_buf` | `que_spw_fixed<T>` |

**关键约束**：
- 所有队列 `init(que_size)` **必须传 2 的幂**（内部 mask=tnum-1 位与寻址）
- `_buf`（变长）**不存数据长度**，读者必须自备长度知识
- `T` 必须 POD（定长）
- 多线程并发**只能用 `_mth` 后缀接口**

详见 [队列.md §2](../使用范式/队列.md#2-选型决策树)。

## 2. 锁选型

**问题**：进程范围？互斥/读写？临界区长度？

| 锁 | 进程 | 类型 | 自旋/睡眠 | 关键特性 |
|---|---|---|---|---|
| `atomic_lock` | 单 | 互斥 | 自旋 | CAS；30000 自旋后 CPU_PAUSE；**不支持递归** |
| `atomic_rwlock` | 单 | 读写 | 自旋 | **写优先**（写等待阻塞新读）|
| `cspinlock` | 单 | 互斥 | 自旋 | pthread_spin；构造即 init |
| `cmutex` | 单 | 互斥 | 睡眠 | 临界区可较长；可配递归 |
| `rwlock` | 单 | 读写 | 睡眠 | **写优先** |
| `cmutex_proc` | **多** | 互斥 | 睡眠 | 必须放共享内存；robust 崩溃恢复 |
| `swlock_proc` | **多** | 读写（写优先）| 睡眠 | 必须放共享内存；`reinit()` 崩溃恢复；带超时 |

**决策规则**：
1. **临界区极短（几行）→ 自旋**（`atomic_lock` / `atomic_rwlock` / `cspinlock`）
2. **临界区较长（IO/系统调用）→ 睡眠**（`cmutex` / `rwlock`）
3. **跨进程共享内存 → 必须放 SHM**（`cmutex_proc` / `swlock_proc`）
4. **必须 RAII → `clock_guard<T>` 包装**

详见 [锁与同步.md §2](../使用范式/锁与同步.md#2-mlock7-锁选型矩阵--raii-守卫)。

## 3. 内存池选型

**问题**：跨进程？容量固定？线程模型？

```
跨进程共享？
├─ 是 → shm_block_pool + shmem_block_c1h<D>
│        （共享内存，三级寻址 area→block→data，**仅单进程写**）
└─ 否 → 容量固定可预估？
         ├─ 固定 → fix_pool<T>（最轻量）
         └─ 动态增减 → dync_pool<N,T> + dync_pool_c1h<N,T>
                      （主级动态扩展 + 线程私有缓存）
```

线程模型（统一）：

| 线程模型 | 获取 | 释放 |
|---|---|---|
| 单线程 | `get` | `back` |
| 多线程 | `take` | `release` |
| 私有缓存 c1h | `get/take` (走主级)| `back/release`（块级归还策略）|

**关键约束**：
- T 必须 POD（`data2it`/`pos2blk` 用 `offsetof` 反算地址）
- 多线程用 `release(pos)` 才有重复释放保护；`release(buf)` 无
- `dync_pool` 的 N 建议 `8*n-2`（如 62=8*8-2, 126），上限 126
- `shmem_pool` 读进程**只**能 `addr2data` 只读访问；不可调 `get/release/take/back`

详见 [内存池.md §2](../使用范式/内存池.md#2-选型决策树)。

## 4. 线程选型

**问题**：要定时器？要线程循环？事件队列驱动？

```
要定时器？
├─ 是 → timer_thread（timerfd + epoll，封装好；后台线程自动跑）
└─ 否 → 要线程？
    ├─ 单任务循环 → simple_thread（继承 do_work / need_work）
    │   ├─ CPU 密集/快响应 → 忙轮询（busyround==0）
    │   └─ I/O 密集/省 CPU → 等待触发（busyround>0 + eventfd）
    ├─ 多生产者事件投递 + 单线程消费 → mthread（epoll + que_mth_fixed<event_info>）
    ├─ 多线程按负载分配 → mthread_pool（持 N 个 mthread）
    └─ 裸 pthread（不要循环框架）→ thread_comm::run_thread / wait_thread
```

**模式要点**：
- `simple_thread`：`busyround==0` 忙轮询；`busyround>0` 等待触发（互斥）
- `mthread`：`waitms>0 || eventq_size<32` 走 epoll；否则纯忙轮询
- `timer_thread`：最小粒度 10us，插入 O(n)（几百个可接受，上千考虑分层时间轮）

详见 [线程.md §1](../使用范式/线程.md#1-选型决策树)。

## 5. 网络/AIO 选型

**问题**：协议？缓存策略？连接模式？

### 5.1 协议层

| 需求 | 类 |
|------|-----|
| TCP 客户端/服务端 | `tcp_ch` / `aio_tcp` |
| UDP 单播/组播/广播 | `udp_ch` / `aio_udp` |
| TCP 监听端 | `tcp_listen_ch` |

**强制**：`channel_attr.family = CHANNEL_FAMILY_IPV4`（IPv6 已定义但未实现）。

### 5.2 异步框架 + 3 缓存策略

| 缓存类 RC | 共享底座 | 预定义 (TCP) | 预定义 (UDP) | 适用 |
|---|---|---|---|---|
| `aio_recv_buf` | 无（每通道独占）| `tcp_buf_ch` | `udp_buf_ch` | 少量连接，最简 |
| `aio_recv_pool<N>` | `fix_pool` | `tcp_mtu_pool_ch` | `udp_mtu_pool_ch` | 高并发多连接，共享池 |
| `aio_recv_que` | `que_swr_buf` | `tcp_que_ch` | `udp_que_ch` | 多通道单线程消费，**非请求应答** |

**关键约束**：
- `aio_recv_que` 不适合请求应答（共享队列消息与通道分离，无法关联响应到连接）
- `dispatch_zero_copy=1` 时**必须**调 `dispatch_deal_end`，否则缓存泄漏

详见 [网络.md §1](../使用范式/网络.md#12-异步框架层--3-缓存策略)。

### 5.3 发送模式（多线程串行化）

| 模式 | 做法 | send_msg 串行化保证 | 适用 |
|---|---|---|---|
| 1. 单线程直发 | 用户保证单线程 | 单线程天然 | 最简，deal_msg 内应答 |
| 2. 发送线程+私有队列（**推荐**）| 每线程私有队列 | 单发送线程消费串行 | 高并发 |
| 3. 事件封装投递（**推荐**）| 发送封装 event_op 投递 | deal_event 单线程 | 跨线程拉回接收线程 |
| 4. 多线程+用户锁 | 用户加 cmutex | 串行 | 最差；EAGAIN 长持锁 |

**铁律**：`aio_tcp::send_msg` 非线程安全，多线程并发会让消息字节流**交错拼接**。

详见 [网络.md §发送模式](../使用范式/网络.md#发送模式与线程安全)。

## 6. 哈希与数据结构选型

```
哈希表（key→value 检索）？
├─ 动态扩展、多线程 → hash_map_mth
├─ 启动固定、只增不删、跨进程 → hash_vec_pos + vec_fix_mem
└─ 整型递增流序号、单线程 → ring_hash

有序容器？
└─ 几百个对象从大到小（如行情挡位）→ ring_window

可扩展数组？
└─ POD 对象 + 多线程 + 自动扩容 → smth_vec
```

详见 [哈希与数据结构.md §2](../使用范式/哈希与数据结构.md#2-选型决策树)。

## 7. 跨组组装

更复杂的业务需要**跨组**拼装。直接读 [架构范式.md](../架构范式.md)：

| 范式 | 用途 | 关键组件 |
|------|------|----------|
| 1. 网络服务器 | epoll 高性能 TCP 服务 | `mthread` + `aio_tcp` + `que_mth_buf` |
| 2. 跨进程共享 | 单写多读进程间通信 | `shmem_pool` + `que_proc_buf` 或 `que_spw_fixed` |
| 3. 重连心跳 | 长连接活性维持 | `aio_tcp` + `heart_manage` + `reconnect_ctl` + `timer_thread` |

**详细可粘贴代码模板**见 [05-business-recipes.md](./05-business-recipes.md)。

## 8. 决策终点速查

| 业务意图 | 直接选 |
|----------|--------|
| 单线程生产者-消费者 | `que_swr_buf` / `que_swr_fixed<T>` |
| 多线程任务投递 | `mthread` + `que_mth_fixed<event_info>` |
| 高并发 TCP 服务器 | `aio_tcp<aio_recv_pool>` + `fix_pool` + `mthread_pool` |
| TCP 客户端带心跳 | `aio_tcp` + `heart_manage` + `reconnect_ctl` |
| 跨进程共享（单写多读）| `shm_block_pool` + `que_spw_fixed<T>` |
| 跨进程变长消息 | `que_proc_buf` |
| 启动加载键值索引（SHM）| `hash_vec_pos` + `vec_fix_mem` |
| 行情挡位（几百排序对象）| `ring_window` |
| 进程级优雅退出 | `install_signal_hand` |
| 周期性定时任务 | `timer_thread` |

## 9. 下一步

- 怎么拼：[05-business-recipes.md](./05-business-recipes.md)
- 坑点：[06-pitfalls-index.md](./06-pitfalls-index.md)
- 不支持：[07-limitations.md](./07-limitations.md)

---
purpose: PITFALLS-INDEX
audience: AI agents and humans encountering bugs or unusual behavior
source-of-truth: yes (single-page index; deep details in 使用范式/*.md "避坑" sections)
last-reviewed: 2026-08-31
---

# Pitfalls Index：跨模块坑点单页索引

> lbcommon 6 个组都有各自"避坑"节，本文件把它们合并成**单页索引**——按"症状"或"业务场景"分组。
> 完整原理链接到对应 [使用范式/*.md](../使用范式/) 章节。

## 1. 网络/AIO 坑点

| # | 症状 / 触发场景 | 原因 | 正确做法 | 详见 |
|---|----------------|------|----------|------|
| N1 | 多线程 send 后对端收到乱序废包 | `send_msg` 非线程安全，字节流交错 | 4 种串行化模式（推荐事件投递）| [网络.md §发送模式](../使用范式/网络.md#发送模式与线程安全) |
| N2 | `deal_ch_closed` 不触发，对象永不释放 | `delive_ch_event(needadd=1)` 未配 `end_ch_event(1)` | 严格 1:1 配对 | [网络.md §5](../使用范式/网络.md#5-tcp_ch-引用计数state_machineh-的-src_stat_ref) |
| N3 | 连接断了但应用不释放对象 | 在 `deal_ch_closing` 释放了 | **必须**在 `deal_ch_closed`（引用归零）才释放 | 同上 |
| N4 | 心跳超时误判（业务正常但被踢） | `deal_recv` 内 `heart.on_msg()` 被注释，业务没手动调 `on_com_msg()` | `deal_msg` 回调里手动 `on_com_msg()` | [网络.md §6](../使用范式/网络.md#6-heart_manage-心跳) |
| N5 | 异步连接失败但收不到回调 | 异步失败不走 `deal_ch_closing`，走 `deal_after_closed` → `deal_ch_closed` | `deal_ch_closed` 必须能处理"连接失败" errcode | [网络.md §2/§9](../使用范式/网络.md#9-避坑) |
| N6 | 响应发不出去 / 关联不到原连接 | 用了 `aio_recv_que` 但想做请求应答 | 改用 `aio_recv_buf` 或 `aio_recv_pool` | [网络.md §1.2](../使用范式/网络.md#12-异步框架层--3-缓存策略) |
| N7 | 缓存泄漏 | `dispatch_zero_copy=1` 但没调 `dispatch_deal_end` | 零拷贝分发后必须调 | [网络.md §4](../使用范式/网络.md#4-3-种缓存策略--dispatch_zero_copy) |
| N8 | `connect_ch` 同步/异步返回值混淆 | 底层三态 `1=同步/0=异步/<0=失败` | `aio_tcp` 已归一为 `0=成功/<0=失败`；只看 `ret==0` | [网络.md §2](../使用范式/网络.md#2-aio_tcp-完整生命周期) |
| N9 | 业务重启后客户端无法重连 | `m_happen_connected==0`（首次连接门）| 业务侧手动管理首次连接流程 | [网络.md §7](../使用范式/网络.md#7-reconnect_ctl-重连) |
| N10 | 心跳 `msg_count` 跨线程 → 数据竞争 | `on_msg` 非原子 | `on_msg` 必须在同一线程调用 | [网络.md §6](../使用范式/网络.md#6-heart_manage-心跳) |

## 2. 队列坑点

| # | 症状 / 触发场景 | 原因 | 正确做法 | 详见 |
|---|----------------|------|----------|------|
| Q1 | `init(1023)` 后数据错乱 | 容量非 2 的幂，`mask=tnum-1` 位与寻址失效 | **必须**传 2 的幂 | [队列.md §5.1](../使用范式/队列.md#51-2-的幂--mask所有队列通用) |
| Q2 | 读出来的数据错位 / 长度不对 | 变长队列（`*_buf`）**不存数据长度** | 读者必须自备长度编解码；或用 `que_proc_buf::read_cmt()`（无参版读 head 的 `mem_len`）| [队列.md §1/§3.2](../使用范式/队列.md#3-write-read-配对规则) |
| Q3 | 队列假满 / 卡死 | `write_get` 后忘了 `write_cmt` | 严格 `write_get → write_cmt` 配对 | [队列.md §8.4](../使用范式/队列.md#8-避坑) |
| Q4 | 假满（读侧）| `read_get` 后忘了 `read_cmt` | `read_cmt(len)` 提交对应长度 | 同上 |
| Q5 | 多线程下数据损坏 | 用了无后缀接口（如 `write_get`）| 多线程并发**只能**用 `_mth` 后缀接口（`write_get_mth`/`write_cmt_mth`）| [队列.md §6](../使用范式/队列.md#6-线程安全边界) |
| Q6 | CPU 飙高 / 上下文切换抖动 | `write_cmt_mth`/`pop_mth` 内层 `QUE_LOOP_BUSY_COUNT`(50000) 忙等 | 设计取舍；高并发考虑业务降速 | [队列.md §8.5](../使用范式/队列.md#8-避坑) |
| Q7 | 定长队列实际容量 = `tnum-1` 而非 `tnum` | 保留 1 槽防满覆盖 | `init(1024)` 实际可用 1023 | [队列.md §5.3](../使用范式/队列.md#53-满判定) |
| Q8 | 跨进程崩溃后队列数据全丢 | `que_proc_buf` 崩溃恢复丢弃 `discard_pos` 到新 `wrcmt` 之间数据 | 业务须容忍；`que_spw_fixed` 无崩溃检测（崩溃由外部保证）| [队列.md §4.2](../使用范式/队列.md#42-op_pid-崩溃检测que_proc_buf-独有) |
| Q9 | `reset_pos`/`close` 后数据错乱 | 在还有读写活动时调 | 必须无任何读写时调 | [队列.md §8.8](../使用范式/队列.md#8-避坑) |

## 3. 锁与同步坑点

| # | 症状 / 触发场景 | 原因 | 正确做法 | 详见 |
|---|----------------|------|----------|------|
| L1 | 自旋锁卡死 | `atomic_lock` 同线程二次 lock | `atomic_lock` **不支持递归**——单进单出 | [锁与同步.md §2](../使用范式/锁与同步.md#2-mlock7-锁选型矩阵--raii-守卫) |
| L2 | `cas_weak` 死循环 / 错过更新 | 弱 CAS 可能"值相等也返回 false" | 弱 CAS **必须**包在循环里且有退出条件 | [锁与同步.md §1.3](../使用范式/锁与同步.md#13-强-cas-vs-弱-cas关键选型) |
| L3 | 自旋锁持锁期间 IO/系统调用 → CPU 100% | `atomic_lock`/`atomic_rwlock`/`spin_state_lock` 满自旋 | 临界区必须**极短**；长临界区用 `cmutex`/`rwlock` | [锁与同步.md §7.6](../使用范式/锁与同步.md#7-避坑) |
| L4 | 多锁死锁 | 加锁顺序不一致 | 全局统一加锁顺序（源码不强制）| [锁与同步.md §7.4](../使用范式/锁与同步.md#7-避坑) |
| L5 | 读者延迟大 | `atomic_rwlock`/`rwlock`/`swlock_proc` **写优先**（写等待阻塞新读）| 设计取舍；写极少时可换 `pthread_rwlockattr` | [锁与同步.md §2](../使用范式/锁与同步.md#2-mlock7-锁选型矩阵--raii-守卫) |
| L6 | 跨进程锁崩溃后不可用 | `cmutex_proc` robust 仅在 `_POSIX_THREAD_ROBUST_MUTEXES` 定义时生效 | 必须检查 lock 返回码；`EOWNERDEAD` 调 `pthread_mutex_consistent` | [锁与同步.md §2/§7.7](../使用范式/锁与同步.md#7-避坑) |
| L7 | 数据竞争（原子 ≠ 线程安全）| 单变量原子，多变量组合仍需锁/屏障 | 写端 `atomic_write_fence` + 读端 `atomic_read_fence` 必须配对 | [锁与同步.md §1/§7.1](../使用范式/锁与同步.md#7-避坑) |
| L8 | 多进程 `eventfd` 用错模式 | 信号量模式 vs 计数器模式 | 信号量 `init(1)`：逐项唤醒；计数器 `init(0)`：单次通知 | [锁与同步.md §5](../使用范式/锁与同步.md#5-wait_wakeeventfd-封装event_wake) |
| L9 | `wait_poll_multi::remove_wake` 期间收到 fd 事件 | 延迟删除 | 子类需容忍"已标记删除但仍收事件" | [锁与同步.md §4.3](../使用范式/锁与同步.md#43-wait_poll_multi多事件) |

## 4. 内存池坑点

| # | 症状 / 触发场景 | 原因 | 正确做法 | 详见 |
|---|----------------|------|----------|------|
| P1 | 偏移错位 / 段错误 | T 有 vtable / 复杂构造析构 | T **必须** POD（无虚函数 / 无复杂构造析构）| [内存池.md §6](../使用范式/内存池.md#6-pod-限制强制) |
| P2 | 链表成环 / 数据损坏 | 多线程下用 `release(buf)` 重复释放 | 多线程**必须**用 `release(pos)`（有重复释放检查）| [内存池.md §3.2](../使用范式/内存池.md#32-重复释放检查的不对称性关键避坑) |
| P3 | 调用 `get/take` 后读 `o_block` 段错误 | 失败时 `o_block` 未设或为 NULL | 必查返回值 `ret<0`；不可读 `o_block` | [内存池.md §10](../使用范式/内存池.md#10-错误码与失败语义) |
| P4 | `init` 不生效（静默返回 0）| `fix_pool`/`dync_pool`/`hash_map_mth` 重复 init 静默成功 | **必须**先 `close()` 才能真正重置 | [内存池.md §10](../使用范式/内存池.md#10-错误码与失败语义) |
| P5 | 共享内存链表被破坏 | 读进程调 `get/take/release/back`（编译期不报错）| 读进程**只**能 `addr2data` 只读访问 | [内存池.md §5.4](../使用范式/内存池.md#54-读进程操作边界重要) |
| P6 | 跨进程寻址错乱 | 不同进程用了不同版本 lbcommon 头文件 | 所有进程用**同一份**头文件（`shm_data_addr` ABI 敏感）| [内存池.md §5.3](../使用范式/内存池.md#53-三级寻址-shm_data_addr) |
| P7 | `dync_pool` 永久阻塞 | take 路径假设持有线程必走完释放，线程崩溃后状态不一致 | 业务保证线程正常退出；目前无修复方案 | [内存池.md §9.8](../使用范式/内存池.md#9-避坑) |
| P8 | `dync_pool<N,T>` 索引越界 | N > 126（int8 留负值空间）| N 建议 `8*n-2`（如 62, 126），上限 126 | [内存池.md §7](../使用范式/内存池.md#7-模板参数-ndync_pool-int8-n-t) |

## 5. 线程/定时器坑点

| # | 症状 / 触发场景 | 原因 | 正确做法 | 详见 |
|---|----------------|------|----------|------|
| T1 | pthread 句柄泄漏 / UAF | 析构未自动停线程 | **必须**显式 `to_stop()` + `join()`（pool 用 `destroy()`）| [线程.md §6](../使用范式/线程.md#6-生命周期统一) |
| T2 | 事件队列卡死 | `get_event` 后忘 `cmt_event` / `deal_queue` 忘 `read_cmt` | 严格配对 | [线程.md §8.3](../使用范式/线程.md#8-避坑) |
| T3 | `simple_thread` 两种模式互调 | `busyround==0` vs `>0` | 两者互斥；等待触发下 `loopround<10` 抬到 10 | [线程.md §8.6](../使用范式/线程.md#8-避坑) |
| T4 | CPU 100% / 不休眠 | `mthread` 用纯忙轮询（`waitms<=0 && eventq_size>=32`）| 省 CPU 必须 `waitms>0` | [线程.md §3.4](../使用范式/线程.md#34-模式判定_f_mthread_func) |
| T5 | 定时不准确 | 最小 10us 粒度 + timerfd 调度 | 10us 软上限；上千定时器考虑分层时间轮 | [线程.md §8.4-§8.5](../使用范式/线程.md#8-避坑) |
| T6 | 绑核失败 | `init_set_cpu` 同时设 SCHED_RR 最高优先级 | 需 root / CAP_SYS_NICE | [线程.md §8.8](../使用范式/线程.md#8-避坑) |
| T7 | 投递事件到错误的线程 | `assign_thread` fallback 到 `ths[0]`（无 running 时）| 投递前 `is_running()` 自查 | [线程.md §8.9](../使用范式/线程.md#8-避坑) |
| T8 | SIGTERM 不能优雅退出 | 没调 `install_signal_hand` | 主线程早调 `install_signal_hand()` | [线程.md §6](../使用范式/线程.md#6-生命周期统一) |
| T9 | `event_info.buf[56]` / `time_event_info.buf[48]` 字段传非 POD | memcpy 传递 | buf 必须 POD | [线程.md §7](../使用范式/线程.md#7-回调基类meventh) |

## 6. 哈希/数据结构坑点

| # | 症状 / 触发场景 | 原因 | 正确做法 | 详见 |
|---|----------------|------|----------|------|
| H1 | `hash_map_mth` 拖慢写吞吐 | 遍历持 `exp_lock` 读锁阻塞 rehash 扩展 | 业务热路径**不要遍历**；遍历用于低频运维 | [哈希与数据结构.md §9](../使用范式/哈希与数据结构.md#9-迭代遍历) |
| H2 | `hash_vec_pos` 重复 key / 竞态 | `insert`（无锁）与 `insert_mth`（加锁）混用 | 单线程只 `insert`，多线程只 `insert_mth`，**不要混用** | [哈希与数据结构.md §4.1](../使用范式/哈希与数据结构.md#41-hash_vec_posinsert单线程-vs-insert_mth多线程-不可混用) |
| H3 | `hash_vec_pos` 满了 | 容量固定，满则 `LBERR_OBJ_IS_FULL` | **只增不删**场景；满则需换 `hash_map_mth` 或重新分配 | [哈希与数据结构.md §10.3](../使用范式/哈希与数据结构.md#11-避坑) |
| H4 | `ring_window<N,...>` 编译失败 | N 不是 2 的幂 | N **必须** 2 的幂（`static_assert` 强制）| [哈希与数据结构.md §7](../使用范式/哈希与数据结构.md#7-ring_window-有序窗口) |
| H5 | `smth_vec<T>::write()` 报 "no member named 'save_no'" | T 没实现 `save_no(int32)` | T **必须** POD + 实现 `save_no(int32)`（迟绑定契约，编译期不报错）| [哈希与数据结构.md §8](../使用范式/哈希与数据结构.md#8-smth_vec-平滑扩展数组) |
| H6 | `smth_vec` 索引冲突 | `pre_no` 前缀与位置重叠 | `pre_no>0` 时必须 `pre_no >= max_size` | [哈希与数据结构.md §10.6](../使用范式/哈希与数据结构.md#11-避坑) |
| H7 | `hash_map_mth` 存大对象 | value 拷贝开销 | value 尽量小；大的或非 POD 存指针 | [哈希与数据结构.md §10.7](../使用范式/哈希与数据结构.md#11-避坑) |
| H8 | `ring_hash::erase` 后节点丢失 | `ring_hash` 不管理节点内存 | 节点由外部缓存池分配；erase 返回节点指针 | [哈希与数据结构.md §10.8](../使用范式/哈希与数据结构.md#11-避坑) |
| H9 | `find(need_lock=0)` 崩溃或脏读 | 违反无锁读契约：扩容触发（delete 旧篮子数组）或并发 erase/clear | 无锁读仅限"构造完只读"或"并发插入且**元素数<篮子数/2 不扩容、只插不删**"；其余用默认 need_lock=1 | [哈希与数据结构.md §4.2](../使用范式/哈希与数据结构.md#42-hash_map_mthfind-的-need_lock-参数) |

## 7. 通用坑点

| # | 症状 / 触发场景 | 原因 | 正确做法 | 详见 |
|---|----------------|------|----------|------|
| G1 | 编译期 `Illegal instruction` | 业务项目 `-march` 与 lbcommon 不一致 | 业务项目加 `-march=native` 与 lbcommon 对齐 | [02 §5](./02-integration-guide.md#5-编译选项对齐) |
| G2 | undefined reference to pthread_create | 链 lib 时丢 pthread | 不要绕过 `target_link_libraries(lbcommon)` 直接链 `.a` | [02 §9](./02-integration-guide.md#9-故障排查) |
| G3 | `fatal error: aio_tcp.h: No such file` | include 路径未传 | 检查 `target_link_libraries(... lbcommon)` | 同上 |
| G4 | 异常中断 / 程序崩溃 | lbcommon **不支持异常** | 业务代码不要 `throw` | [07-limitations §2](./07-limitations.md#2-异常不支持) |
| G5 | IPv6 不可用 | IPv6 已定义但未实现 | 用 IPv4 | [07-limitations §1](./07-limitations.md#1-ipv6) |
| G6 | 跨进程 ABI 不一致 | 不同进程用了不同 lbcommon 头文件 | 所有进程用**同一份**头文件 | [02 §4.3](./02-integration-guide.md#43-abi-约束共享内存场景) |
| G7 | 多个业务模块都 link lbcommon 但状态错乱 | 静态库多份副本 | 默认 OK（每个可执行文件自带）；改 SHARED 时注意版本一致 | [02 §4](./02-integration-guide.md#4-链接与-abi-注意) |

## 8. 紧急排错流程

```
1. 看返回码（按 00-AI-QUICKSTART §5 定位范围）
2. perror_lb(o_errmsg, ret) 查人类可读消息
3. 在本文件按症状搜关键字
4. 查对应 使用范式/*.md 的"避坑"小节
5. 在 common 仓库内 /memorax-code 搜历史教训
6. 仍不解 → 直接读 include/*.h 头文件 + 对应 .cpp 实现
```

## 9. 下一步

- 不支持项：[07-limitations.md](./07-limitations.md)
- 决策树：[04-decision-trees.md](./04-decision-trees.md)
- 仓库导航：[../README.md](../README.md)

---
purpose: GLOSSARY
audience: AI agents and humans reading lbcommon source / docs
source-of-truth: yes
last-reviewed: 2026-08-31
---

# Glossary：lbcommon 专用术语表

> lbcommon 有大量自创命名和缩写。第一次遇到时读本表。
> 术语按"主题"分组；同义词/相关词互相链接。

## 1. 命名空间与代码风格

| 术语 | 含义 | 详见 |
|------|------|------|
| `lb_common` | lbcommon 强制命名空间，所有公共类型都在内 | [README §命名空间](../README.md#命名空间) |
| 全小写下划线命名 | 类/结构体/函数/变量全部 `snake_case`，**无 `_t`/`_s` 后缀**（与 POSIX 区分） | [README §命名规范](../README.md#命名规范) |
| `o_` 前缀 | 输出参数（`o_errmsg`, `o_netaddr`）| [README §变量命名](../README.md#变量命名) |
| `i_` 前缀 | 输入参数（少用）| 同上 |
| `p` 前缀 | 指针（`pch`, `pmsg`, `pbuf`）| 同上 |
| 模板参数单大写字母 | `T`, `N`, `K`, `V`, `CH`, `MT` | 同上 |
| `LBERR_xxx` | 错误码宏（**负值**，范围 `-1 ~ -39`）| [00-AI-QUICKSTART §5](./00-AI-QUICKSTART.md#5-错误码范围速查完整定义见-includecomm_errnoh) |
| `lb_log` / `lb_log_hand` | 日志主类 + 每线程私有句柄（流式 API）| `mlog.h` |

## 2. 队列族命名编码（关键！）

类名按「**写读拓扑 + 数据形态**」编码：

| 段 | 含义 | 例 |
|----|------|-----|
| `swr` | 单写单读（Single-Write-Single-Read）| `que_swr_buf`、`que_swr_fixed<T>` |
| `mth` | 单进程多线程并发（Multi-Thread）| `que_mth_buf`、`que_mth_fixed<T>` |
| `proc` | 多进程变长（Multi-Process）| `que_proc_buf` |
| `spw` | 单进程写 + 多进程读（Single-Process-Write）| `que_spw_fixed<T>` |
| `_buf` | 变长数据（不存长度！）| `que_swr_buf` |
| `_fixed<T>` | 定长数据，POD 模板 | `que_mth_fixed<T>` |

接口后缀：

| 后缀 | 含义 |
|------|------|
| 无后缀 | 单线程版（基础接口）|
| `_mth` | 多线程版（CAS + 忙等）|
| `_mp` | 多进程版（仅 `que_proc_buf`，含崩溃检测）|

详见 [队列.md §1](../使用范式/队列.md#1-命名约定选型前提)。

## 3. 锁族命名

| 类 | 含义 |
|----|------|
| `atomic_lock` | CAS 自旋互斥锁（单进程，**不支持递归**）|
| `atomic_rwlock` | CAS 自旋读写锁（单进程，**写优先**）|
| `cspinlock` | pthread_spin 自旋锁（单进程）|
| `cmutex` | pthread 睡眠互斥锁（单进程，可配递归）|
| `rwlock` | pthread 读写锁（单进程，**写优先**）|
| `cmutex_proc` | **多进程** pthread mutex + robust（必须放共享内存）|
| `swlock_proc` | **多进程** sem_t 双信号量读写锁（必须放共享内存）|
| `clock_guard<T>` | RAII 模板（构造 lock，析构 unlock）|

详见 [锁与同步.md §2](../使用范式/锁与同步.md#2-mlock7-锁选型矩阵--raii-守卫)。

## 4. 网络/AIO 关键术语

| 术语 | 含义 | 详见 |
|------|------|------|
| `aio_tcp<B, RC>` | AIO TCP 模板类，B=缓存类型，RC=接收缓存策略 | `aio_tcp.h` |
| `tcp_buf_ch` | `aio_tcp<char, aio_recv_buf>` 预定义——每通道独占独立缓存（最简）| [网络.md §1](../使用范式/网络.md#12-异步框架层--3-缓存策略) |
| `tcp_mtu_pool_ch` | `aio_tcp<aio_mtu_pool, aio_recv_mtu_pool>` 预定义——共享 `fix_pool` 缓存 | 同上 |
| `tcp_que_ch` | `aio_tcp<que_swr_buf, aio_recv_que>` 预定义——共享 `que_swr_buf` 缓存（**零拷贝，但仅适合流式广播**）| 同上 |
| `aio_attr` | AIO 配置结构（`onerecvtimes`、`maxmsglen`、`buf_size`、`dispatch_zero_copy`、`heartinterval`）| `comm_aio.h` |
| `aio_msg` | AIO 消息（`pmsg` + `msglen` + `buf_addr`）| `comm_aio.h` |
| `channel_attr` | 通道属性（`family`、缓冲大小等）| `comm_sock.h` |
| `CHANNEL_FAMILY_IPV4` | IPv4 地址族（**唯一支持**；IPv6 已定义但未实现）| [网络.md §1.1](../使用范式/网络.md#11-协议层comm_sock) |
| `csock_addr` | 套接字地址封装（IP + port）| `comm_sock.h` |
| `deal_msg` | 纯虚消息回调，返回 `>0=处理长度 / =0=未处理 / <0=错误` | `comm_aio.h` §`ch_recv_cb` |
| `deal_ch_error / closing / closed / connect` | 通道生命周期回调（`aio_ch_op` 默认空实现）| `comm_aio.h` |
| `deal_ch_closing` | 首次关闭成功回调**一次**——"不再可见" | [网络.md §5](../使用范式/网络.md#5-tcp_ch-引用计数state_machineh-的-src_stat_ref) |
| `deal_ch_closed` | 引用归零后回调——**真正释放资源** | 同上 |
| `dispatch_zero_copy` | 接收消息分发到其他线程处理时是否零拷贝（`aio_attr` 字段）| [网络.md §4](../使用范式/网络.md#4-3-种缓存策略--dispatch_zero_copy) |
| `dispatch_deal_end` | 零拷贝时，分发线程处理完通知缓存可复用 | 同上 |
| `onload` | Solarflare kernel-bypass 加速（当前**注释掉**，需手动开）| [网络.md §发送模式](../使用范式/网络.md#发送模式与线程安全) |

## 5. 引用计数与状态机（`state_machine.h`）

| 术语 | 含义 |
|------|------|
| `src_stat_ref` | "状态 + 引用计数合一" 结构，单 `int64` 高位状态 + 低 48 位计数 |
| `to_init / end_init` | CLOSED → INITING → INITED 转移 |
| `set_work / stop_work` | INITED ↔ WORK |
| `to_close` | WORK → CLOSING；ref==0 直接 → CLOSED |
| `add_ref / sub_ref` | 引用 +1/-1；sub_ref 归零 + 已 to_close → 自动转 CLOSED |
| `is_work / is_close / is_free` | 状态查询（WORK / ≤CLOSING / CLOSED）|
| `src_async_ctl` | 异步事件投递控制（最多一个异步事件在飞）|
| `spin_state_lock` | 单 `int32` 自旋状态锁（主状态 + SEND/RECV 附加位）|

状态机：`CLOSED → INITING → INITED → WORK → CLOSING → CLOSED`

详见 [锁与同步.md §3](../使用范式/锁与同步.md#3-state_machine状态机-3-个类)。

## 6. 内存池族

| 术语 | 含义 |
|------|------|
| `fix_pool<T>` | 固定容量池，空闲链表管理（最轻量）|
| `dync_pool<N,T>` + `dync_pool_c1h<N,T>` | 动态扩展两级池 + 线程私有缓存 |
| `shm_block_pool` + `shmem_block_c1h<D>` | 共享内存三级寻址池（area→block→data，**仅单进程写**）|
| `shm_data_addr` | 三级寻址结构（`area_id` + `data_id` + `block_id`），与 `int64 pos` 组成 `shm_addr_unit` 联合体（可原子跨进程传递）|
| `addr2data` | 用 `shm_data_addr` 在**只读**模式下找到数据指针 |
| `get / back` | **单线程** 获取/归还 |
| `take / release` | **多线程** 获取/归还（内部 atomic_lock）|
| `release(pos)` vs `release(buf)` | `release(pos)` 有重复释放检查；`release(buf)` 无（多线程安全用前者）|
| `c1h` | "Cache Level 1 Heread" / 每线程私有缓存句柄 |

详见 [内存池.md §1-§4](../使用范式/内存池.md)。

## 7. 线程组

| 术语 | 含义 |
|------|------|
| `simple_thread` | 单线程封装（`do_work` + `need_work` 虚函数；忙轮询/等待触发两模式）|
| `mthread` | epoll + 事件队列线程（多生产者投递，单线程消费）|
| `mthread_pool` | 持 N 个 mthread，按负载或轮询分配 |
| `timer_thread` | timerfd + epoll 定时器线程 |
| `timer_order_list` | 有序链表定时器（最小 10us，插入 O(n)）|
| `event_op` | 纯虚事件回调基类 `void deal(char* pbuf)` |
| `event_info` | `{ event_op *op; char buf[56]; }`（buf 必须 POD）|
| `time_event_op` | 纯虚定时器回调 `void deal_timer(is_period, timer, pbuf)` |
| `time_event_info` | `{ op, isperiod, timer, char buf[48]; }`（POD）|
| `epoll_event_op` | epoll fd 事件回调基类（`deal_event/deal_error/deal_close/get_fd/set_event`）|
| `wait_poll_one / wait_poll_multi` | 单/多事件 epoll 封装 |
| `event_wake` | eventfd 封装（信号量/计数器两模式）|
| `eventfd` | Linux 内核事件通知 fd（`event_wake` 底层）|
| `do_work / need_work` | simple_thread 虚函数（必 override / 默认 FALSE）|
| `to_stop() / join()` | 显式停止线程（**析构不自动调**——必须显式调）|
| `g_signal_stopctl` | 全局信号停止标志（`install_signal_hand` 设置 SIGTERM/SIGQUIT 处理）|

详见 [线程.md](../使用范式/线程.md)。

## 8. 哈希与数据结构

| 术语 | 含义 |
|------|------|
| `hash_str` | 变长字符串哈希函数（`\0` 结尾，~32 字节）|
| `hash_fm8<N>` | 定长键哈希（N 是 8 的倍数 ≤64）|
| `hash_fm4<N>` | 定长键哈希（4 字节对齐，N%8==0 实际 4 字节步进）|
| `hash_map_mth<N,K,V,HASH>` | 动态可扩展多线程哈希表（增量 rehash，**不建议遍历**）|
| `hash_vec_pos<K,V,HASH>` | 固定哈希向量 + 固定内存（**支持共享内存**，只增不删）|
| `vec_fix_mem<T>` | 固定内存数组（与 `hash_vec_pos` 联合用）|
| `ring_hash<D>` | 流序号取模哈希环（单线程，外部管理节点内存）|
| `ring_window<N,K,V,CMP>` | 有序环形窗口（**从大到小**，N 必须 2 的幂）|
| `ring_range_buf` | 多窗口二级索引（只增不减）|
| `smth_vec<T>` | 平滑扩展数组（**T 必须 POD 且实现 `save_no(int32)`**——迟绑定契约）|

详见 [哈希与数据结构.md](../使用范式/哈希与数据结构.md)。

## 9. 心跳与重连

| 术语 | 含义 |
|------|------|
| `heart_manage` | 心跳超时检测：`check_timeout` = `(now - last_heart >= interval*3) && (msg_count==0)` |
| `heart.on_msg()` | **在 `deal_recv` 内被注释**——必须业务在 `deal_msg` 回调里手动调 `on_com_msg()`/`on_heart_msg()` |
| `heart.on_heart()` | 收到心跳应答时调（清 msg_count）|
| `heart.check_send()` | 距 last_send ≥ interval 返回 true，发送心跳 |
| `reconnect_ctl` | 断线重连控制（`max_reconnect=0`=无限；首次连接门 `m_happen_connected==1` 才重连）|
| `set_connected / set_disconnected` | 状态转换（前者强制 m_happen_connected=1，后者**无条件**记录 m_disconnect_time）|
| `m_happen_connected` | 首次连接门标志——从未成功连接过则**不重连** |
| `m_disconnect_time` | 上次断线时间（首次断线 0 时不等间隔立即重连）|

详见 [网络.md §6/§7](../使用范式/网络.md#6-heart_manage-心跳)。

## 10. 内存与对齐

| 术语 | 含义 |
|------|------|
| `CACHE_ALIGN_SIZE` | 64 字节缓存行对齐（所有热路径结构体用 `alignas(64)` 防伪共享）|
| `SIMD_ALIGN_SIZE` | x86_64=32B / ARM=16B SIMD 对齐 |
| `ALIGN_UP / ALIGN_OFF / IS_ALIGN` | 对齐工具宏 |
| `CPU_PAUSE` | 自旋优化提示（x86 `pause`，aarch64 `yield`，riscv64 `nop`）|
| `likely() / unlikely()` | 分支预测提示宏 |
| `FORCE_INLINE` | 强制内联（`__attribute__((always_inline))` 等）|
| `POD` | "Plain Old Data"——无虚函数/无复杂构造析构的类型（lbcommon 三池 + 定长队列的硬性约束）|
| `offsetof` | 编译期偏移宏（`data2it`/`pos2blk` 反算地址的根因——见 [内存池.md §6](../使用范式/内存池.md#6-pod-限制强制)）|
| `view_ptr` 等 | `comm_utils` 提供的 SIMD 加速 memcpy/compare 清零 |
| `str_copy_format / str_format` | `comm_utils` 字符串复制/格式化（高性能版：截断时写满 `dst_size` 字节，**不保证 NUL 终止**）|
| `str_copy_safe / str_format_safe` | `comm_utils` 安全版（**保证 NUL 终止**：最多写 `dst_size-1` 字符；`str_copy_safe` 需显式传 `src_size`）|

详见 `comm_sys.h` + [内存池.md §6](../使用范式/内存池.md#6-pod-限制强制)。

## 11. 错误码语义三元式（统一约定）

| 返回值 | 含义 | 处理 |
|--------|------|------|
| `> 0`（位置/长度）| 成功 | 直接使用 |
| `0` | "空/满"/"未就绪" | 业务侧自行重试或放弃 |
| `< 0` | `LBERR_*` 错误码 | 必查 `perror_lb(o_errmsg, ret)` |

**唯一例外**：`aio_tcp::connect_ch` 已把底层三态 `1/0/<0` 归一为 `0/<0`；同步与异步均通过回调 `deal_ch_connect` 通知。

## 12. 跨仓库与记忆

| 术语 | 含义 |
|------|------|
| `memorax-cli` | 用户级持久编码记忆工具（跨 session）|
| `workspace` | memorax 工作空间绑定（当前 = `common`）|
| `scope=git-repository` | memorax 数据按 git 仓库身份隔离——业务项目 AI 默认看不到 common 的 memorax |
| `.repo_memory/`（gitignored）| 本仓库 AI 的 cold-start 页（PROFILE.md + architecture-and-layers.md 等）|
| `committed docs` | 跨仓库 AI 接入的唯一可靠通道——业务项目 `add_subdirectory` 时自动带入 |
| `/memorax-code add` | Claude Code 主动写入 memorax 的命令（路由到对应操作）|

详见 [00-AI-QUICKSTART §8](./00-AI-QUICKSTART.md#8-给业务项目-ai-的路径提示)。

## 13. 下一步

- 选哪个组件：[04-decision-trees.md](./04-decision-trees.md)
- 怎么拼：[05-business-recipes.md](./05-business-recipes.md)
- 坑点：[06-pitfalls-index.md](./06-pitfalls-index.md)
- 不支持：[07-limitations.md](./07-limitations.md)

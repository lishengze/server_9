# 底层基础库知识导航

> 本知识库为 C/C++ 底层基础库提供"导航级"文档，配合 RAG 检索 + 直读源码的工作流使用。
> README「分类详解」节提供单分类的语义导航与源码地图（不罗列完整 API 签名）；深度选型/配对/避坑见使用范式。
>
> **三层文档分工**：README「分类详解」= 单分类导航（语义+源码地图）；`使用范式/*.md` = 组级深度手册（选型+配对+避坑）；`架构范式.md` = 跨组系统组装。

## 模块总览

| 编号 | 分类 | 核心能力 | 头文件数 | 使用范式 |
|------|------|----------|----------|----------|
| 01 | 基础类型与错误 | 定宽类型、内存对齐、CPU 优化宏、统一错误码 | 2 | — |
| 02 | 通用工具 | SIMD 内存操作、时间函数、对齐内存、共享内存、C 字符串、动态库、排序 | 4 | — |
| 03 | 文件解析 | INI 配置读取、CSV 读写 | 2 | — |
| 04 | 环形队列 | 单写单读/多写多读/多进程、变长/固定大小环形缓冲区族 | 7 | [队列.md](使用范式/队列.md) |
| 05 | 并发与同步 | 原子操作、内存屏障、自旋锁/互斥锁/读写锁、状态机、epoll、eventfd | 5 | [锁与同步.md](使用范式/锁与同步.md) |
| 06 | 内存池 | 固定/动态/共享内存三级池体系 | 3 | [内存池.md](使用范式/内存池.md) |
| 07 | 特殊数据结构 | 哈希函数/表/向量、哈希环、有序窗口、平滑扩展数组 | 6 | [哈希与数据结构.md](使用范式/哈希与数据结构.md) |
| 08 | 异步事件与线程 | 事件/定时器回调、线程封装、epoll+事件队列线程、线程池、定时器线程 | 6 | [线程.md](使用范式/线程.md) |
| 09 | 日志 | 同步/异步日志、流式 API、按日期/行数切换文件 | 1 | — |
| 10 | 网络异步IO | 套接字工具、TCP/UDP 通道、AIO 框架(三种缓存策略)、心跳管理 | 10 | [网络.md](使用范式/网络.md) |

## 场景速查（跨组业务需求 → 推荐类）

| 业务场景 | 推荐类（跨组）| 关键配对/约束 | 详见 |
|---|---|---|---|
| 单进程无锁生产者-消费者（变长消息）| que_swr_buf | 单写单读；2 的幂；读者须知长度 | 队列.md §3 |
| 多线程任务投递 + 单线程消费 | mthread + que_mth_fixed<event_info> | eventq 投递；cmt_event 必须调 | 线程.md §3 + 队列.md |
| 高并发 TCP 服务器（多连接共享内存池）| aio_tcp<aio_recv_pool> + fix_pool | take/release(pos) 配对 | 网络.md §4 + 内存池.md |
| 跨进程共享配置/状态（单写多读）| shm_block_pool + que_spw_fixed | 写进程=创建者；其他只读 shm_data_addr | 内存池.md §5 + 队列.md §4 |
| 启动时一次性加载键值索引（共享内存）| hash_vec_pos + vec_fix_mem | 只增不删；POD | 哈希与数据结构.md §6 |
| 长连接心跳 + 断线重连 | aio_tcp + heart_manage + reconnect_ctl | on_msg 须手动调；首次连接门 | 网络.md §6/§7 + 架构范式.md §3 |
| 定时任务（周期/单次）| timer_thread | 最小 10us；插入 O(n) | 线程.md §5 |
| TCP 多线程发送安全 | aio_tcp + link_send_que（发送线程=接收线程）| send_msg 非线程安全；私有队列串行 | 网络.md §发送模式 |


## 命名规范

### 命名空间

- 基础库代码位于 `lb_common` 命名空间内

### 类型命名

| 类别 | 规范 | 示例 |
|------|------|------|
| 类名 | 全小写+下划线 | `atomic_lock`, `que_mth_buf`, `lb_log` |
| 结构体名 | 全小写+下划线 | `csock_addr`, `aio_attr`, `shm_data_addr` |
| 模板类名 | 同类名规范 | `que_mth_fixed<T>`, `dync_pool<N,T>` |
| typedef别名 | 全小写+下划线 | `sock_fd`, `f_thread_comfunc` |
| 模板结构体 | 同结构体规范 | `que_fixed_it<V>`, `fix_pool_it<MT>` |
| **无** `_t`/`_s` 后缀 | — | 与POSIX保留区分 |

### 函数命名

| 类别 | 规范 | 示例 |
|------|------|------|
| 成员函数 | 全小写+下划线 | `lock()`, `write_log()`, `deal_msg()` |
| 虚函数 | 同上，无前缀 | `do_work()`, `need_work()` |
| 静态函数 | 同上 | `uint64_to_hex()`, `send_udp()` |
| 内联辅助 | 同上 | `itostr()`, `is_support()` |

### 变量命名

| 类别 | 规范 | 示例 |
|------|------|------|
| 成员变量 | 全小写+下划线，**无前缀** | `mlock`, `epollfd`, `wrpos` |
| 局部变量 | 全小写+下划线 | `msg_len`, `ret_code` |
| 输出参数 | `o_` 前缀 | `o_errmsg`, `o_netaddr`, `o_errcode` |
| 输入参数 | `i_` 前缀（少量使用） | `i_ipaddr`, `ipos` |
| 指针变量 | `p` 前缀 | `pbuf`, `pmsg`, `pch` |
| 模板参数 | 单大写字母或简短大写 | `T`, `N`, `K`, `V`, `CH`, `MT` |

### 宏命名

| 类别 | 规范 | 示例 |
|------|------|------|
| 常量宏 | 全大写+下划线 | `CACHE_ALIGN_SIZE`, `SPIN_LOCK_BUSY_COUNT` |
| 错误码宏 | `LBERR_` 前缀 | `LBERR_MEM_ALLOC_FAIL`, `LBERR_CH_LINK_BROKEN` |
| 日志宏 | 小写+下划线 | `debug_log()`, `info_log()`, `end_log` |
| 编译器提示宏 | 全大写 | `FORCE_INLINE`, `likely()`, `unlikely()` |
| 头文件守卫 | `#pragma once` | — |

## 分层说明

| 层级 | 名称 | 包含分类 | 职责 |
|------|------|----------|------|
| L0 | 零依赖层 | 01 | 全库基石：定宽类型、对齐宏、错误码。被所有上层模块直接包含 |
| L1 | 基础工具层 | 02 | 无状态工具函数：SIMD、时间、内存、字符串、动态库。依赖 L0 |
| L2 | 核心机制层 | 04, 05 | 并发原语和队列：原子操作、锁、状态机、epoll、环形队列。依赖 L0 |
| L3 | 数据管理层 | 03, 06, 07 | 结构化数据存取：文件解析、内存池、哈希/窗口/数组。依赖 L0~L2 |
| L4 | 运行框架层 | 08, 09 | 线程与日志：事件驱动线程、线程池、定时器、异步日志。依赖 L0~L3 |
| L5 | 网络通信层 | 10 | AIO 网络框架：TCP/UDP 通道、异步收发、心跳。依赖 L0~L4 |

## 关键依赖路径

- **日志(09)** ← 通用工具(02) + 环形队列(04) + 并发同步(05) + 异步线程(08)
- **网络异步IO(10)** ← 并发同步(05) + 异步线程(08) + 环形队列(04) + 内存池(06)
- **环形队列(04)** ← 并发同步(05, matomic.h)
- **特殊数据结构(07)** ← 并发同步(05) + 通用工具(02)
- **内存池(06)** ← 并发同步(05) + 通用工具(02)

## 使用方式

1. **定位模块**：根据业务意图触发词，在 README「分类详解」节对应分类小节中找到语义导航与源码地图
2. **找到源码**：根据源码地图中的头文件，直接读取 `include/` 头文件获取精确接口
3. **理解套路**：参考对应使用范式文档的配对规则，理解初始化→操作→销毁的完整生命周期
4. **避开陷阱**：阅读使用范式文档的避坑节，重点关注线程安全声明、资源归属和性能限制

## 分类详解

> 本节为单分类级导航入口。每分类含「触发时机」（业务意图触发词）与「源码地图」（头文件→类/职责）。
> 深度选型、配对规则、避坑指南见对应的 [使用范式/*.md](使用范式)。

### 01 基础类型与错误

**触发时机**
- 当需要跨平台确保 int32/int64 等定宽类型时
- 当需要对齐内存到缓存行/SIMD 边界时
- 当需要分支预测优化(likely/unlikely)或 CPU 自旋暂停时
- 当需要缓存预取指令(L1/L2/L3)时
- 当需要统一错误码与错误消息转换时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| comm_sys.h | 定宽类型/对齐宏/CPU优化宏 | 跨平台定宽类型别名、缓存行/SIMD 对齐宏、likely/unlikely/CPU_PAUSE、缓存预取宏 |
| comm_errno.h | lb_comm_err / perror_lb | 统一错误码(-1~-39，6 大类)与错误消息转换 |

### 02 通用工具

**触发时机**
- 当需要 SIMD 加速的内存拷贝/比较/清零时
- 当需要纳秒级单调时间或 RDTSC 时间戳时
- 当需要对齐内存分配(aligned_malloc/free)时
- 当需要映射共享内存(支持大页)时
- 当需要按分隔符拆分字符串或去除空白时
- 当需要运行时动态加载 .so 并获取函数指针时
- 当需要对 vector 做轻量排序时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| mutils.h | comm_utils(静态类) | SIMD 内存操作、时间函数、对齐内存、共享内存映射、文件操作 |
| cstr_utils.h | cstr_utils(静态类) | 就地修改的 C 字符串 split/trim/lower |
| lib_loader.h | lib_loader | dlopen/dlsym/dlclose 封装(禁止拷贝，析构自动 close) |
| sort_comm.h | sort_cmp_*/lb_sort_func | 比较器 + 希尔排序(直接修改 vector，非稳定) |

### 03 文件解析

**触发时机**
- 当需要解析 INI 格式配置文件(节+键值对)时
- 当需要读取 CSV 数据文件(首行列名)时
- 当需要写入 CSV 日志/数据文件时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| ini_file.h | ini_reader | INI 解析，支持节/10 种类型值/列表读取(链表+map，非线程安全) |
| csv_file.h | csv_reader / csv_writer | CSV 读写：reader 64KB 块缓冲；writer cmutex 线程安全 |

### 04 环形队列

**触发时机**
- 当需要无锁的单写单读数据缓冲时
- 当需要多线程并发读写队列时
- 当需要跨进程通信队列(共享内存)时
- 当需要固定大小的对象队列(零拷贝)时
- 当需要可变长度的消息队列时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| que_comm.h | que_fixed_it\<V\> / 公共宏 | 队列公共定义(QUE_USER_DATA_LEN/LOOP_BUSY_COUNT)与定长项模板 |
| que_swr_buf.h | que_swr_buf | 单写单读变长无锁队列(最简模型，2 的幂) |
| que_swr_fixed.h | que_swr_fixed\<T\> | 单写单读固定大小队列(POD，原子操作，无锁) |
| que_mth_fixed.h | que_mth_fixed\<T\> | 多线程固定大小队列(CAS + 忙等待) |
| que_mth_buf.h | que_mth_buf | 多读多写变长队列(原子 CAS，变长 POD，单/多线程双接口) |
| que_spw_fixed.h | que_spw_fixed\<T\> | 单进程多写 + 多进程读固定大小队列(共享内存，写进程须为创建者) |
| que_proc_buf.h | que_proc_buf | 多进程变长队列(共享内存 + op_pid 崩溃恢复) |

> 选型/配对/避坑详见 [使用范式/队列.md](使用范式/队列.md)

### 05 并发与同步

**触发时机**
- 当需要跨平台原子操作(load/store/cas/fetch_add)时
- 当需要内存屏障(acquire/release/seq_cst)时
- 当需要轻量自旋锁或系统互斥锁时
- 当需要读写锁(写优先)时
- 当需要多进程共享内存锁时
- 当需要管理对象初始化/工作/关闭状态转换时
- 当需要自旋状态锁（带发送/接收附加操作状态）时
- 当需要 epoll 事件轮询(单事件/多事件)时
- 当需要线程间 eventfd 唤醒时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| matomic.h | 原子操作宏 | 5 种内存屏障、原子 load/store/exchange/CAS/fetch_add(16/32/64/ptr) |
| mlock.h | atomic_lock/atomic_rwlock/cspinlock/cmutex/rwlock/cmutex_proc/swlock_proc/clock_guard | CAS 自旋锁/读写锁、pthread 锁族、多进程锁、RAII 守卫 |
| state_machine.h | src_stat_ref / src_async_ctl / spin_state_lock | 资源状态引用(CLOSED→INITING→INITED→WORK→CLOSING)、异步控制、自旋状态锁(IDLE/INITING/WORKING/CLOSING + SEND/RECV) |
| wait_poll.h | wait_poll_one / wait_poll_multi | 单/多事件 epoll 轮询(multi 内置 atomic_lock 删除保护) |
| wait_wake.h | event_wake | eventfd 封装，支持信号量模式 |

> 选型/配对/避坑详见 [使用范式/锁与同步.md](使用范式/锁与同步.md)

### 06 内存池

**触发时机**
- 当需要固定容量的对象缓存池(如连接池、session 池)时
- 当需要动态扩展的对象池(如消息块池)时
- 当需要跨进程共享的内存池(如共享数据区)时
- 当需要线程私有的本地缓存减少锁竞争时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| fix_pool.h | fix_pool\<T\> | 固定容量内存池，空闲链表管理，支持原子锁多线程 |
| dync_pool.h | dync_pool\<N,T\> / dync_pool_c1h\<N,T\> | 动态扩展两级池(主池 + 线程私有句柄) |
| shmem_pool.h | shm_block_pool / shmem_block_c1h\<D\> | 共享内存三级寻址池(area→block→data + 私有句柄) |

> 选型/配对/避坑详见 [使用范式/内存池.md](使用范式/内存池.md)

### 07 特殊数据结构

**触发时机**
- 当需要高性能字符串/定长键哈希函数时
- 当需要多线程安全的可扩展哈希表(增量 rehash)时
- 当需要固定内存+共享内存的哈希映射时
- 当需要按流序号索引在途数据时
- 当需要从大到小排序的少量对象窗口(如行情挡位)时
- 当需要多线程安全的可扩展数组(POD)时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| hash_comm.h | hash_str / hash_fm8\<N\> / hash_fm4\<N\> | 变长字符串与定长键哈希函数 |
| hash_map_mth.h | hash_map_mth\<N,K,V,HASH\> | 多线程安全哈希表，增量 rehash，非对称锁 |
| hash_vec.h | hash_vec_pos\<K,V,HASH\> / vec_fix_mem\<T\> | 固定哈希向量 + 固定内存数组(常联合用于启动加载索引) |
| ring_hash.h | ring_hash\<D\> | 流序号哈希环(单线程，外部管理节点内存) |
| ring_window.h | ring_window\<N,K,V,CMP\> / ring_range_buf | 有序环形窗口(二级数组，从大到小) |
| smth_vec.h | smth_vec\<T\> | 读写锁 + SIMD 扩容的可扩展数组(POD) |

> 选型/配对/避坑详见 [使用范式/哈希与数据结构.md](使用范式/哈希与数据结构.md)

### 08 异步事件与线程

**触发时机**
- 当需要定义事件处理回调(event_op/time_event_op)时
- 当需要管理定时器(单次/周期性)时
- 当需要创建工作线程(忙轮询或事件触发)时
- 当需要基于 epoll+事件队列的多线程框架时
- 当需要定时器线程(timerfd 驱动)时
- 当需要线程池(按负载分配)时
- 当需要屏蔽线程信号或安装信号处理器时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| mevent.h | event_op / event_info / time_event_op / time_event_info | 事件回调基类+载体、定时器回调基类+载体 |
| simple_thread.h | simple_thread | 单线程封装，忙轮询/epoll+eventfd 触发两模式(do_work/need_work) |
| mthread.h | mthread / mthread_pool | epoll+事件队列线程 + 线程池(按负载/轮询分配) |
| time_thread.h | timer_thread | timerfd+epoll 定时器线程(委托 timer_order_list) |
| time_event.h | timer_order_list | 有序链表定时器管理器(最小 10us，插入 O(n)) |
| thread_comm.h | run_thread/wait_thread/mask_thread_signal/install_signal_hand | 全局线程工具函数(运行/等待/屏蔽信号/安装处理器) |

> 选型/配对/避坑详见 [使用范式/线程.md](使用范式/线程.md)

### 09 日志

**触发时机**
- 当需要高性能异步日志(异步入队+后台线程写盘)时
- 当需要每线程独立日志句柄(无锁格式化)时
- 当需要按日期/行数自动切换日志文件时
- 当需要流式日志输出(info_log(hand) << "msg" << num << end_log)时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| mlog.h | lb_log / lb_log_hand | 日志核心类(继承 simple_thread，同步/异步) + 线程私有句柄(流式 API) |

### 10 网络异步IO

**触发时机**
- 当需要创建 TCP/UDP 套接字(单播/组播/广播)时
- 当需要基于 epoll 的异步 TCP/UDP 通道时
- 当需要 AIO 异步接收框架(独立缓存/内存池/队列)时
- 当需要心跳超时检测时
- 当需要断线重连策略控制（重连次数、间隔、状态管理）时
- 当需要零拷贝消息分发时

**源码地图**

| 头文件 | 类/组件 | 职责 |
|--------|---------|------|
| comm_sock.h | sock_utils / csock_addr / channel_attr | 套接字创建/配置/收发静态类 + 地址/通道属性结构体 |
| comm_aio.h | aio_attr / aio_msg / ch_recv_cb / aio_ch_op | AIO 公共定义与回调基类 |
| tcp_ch.h | tcp_ch / tcp_listen_ch | TCP 通道 + 监听通道，完整连接生命周期 + 引用计数 |
| udp_ch.h | udp_ch / udp_ch_op | UDP 通道(单播/组播/广播)+ 回调接口，引用计数与轮询接收 |
| aio_tcp.h | aio_tcp\<B,RC\> + tcp_buf_ch/tcp_mtu_pool_ch/tcp_que_ch | AIO TCP 框架，模板化缓冲区+接收缓存策略 |
| aio_udp.h | aio_udp\<B,RC\> + udp_buf_ch/udp_mtu_pool_ch/udp_que_ch | AIO UDP 框架，三种缓存策略预定义 |
| aio_recv_buf.h | aio_recv_buf | AIO 通道私有独立缓存(单线程接收，临时缓存+分片+提交) |
| aio_recv_pool.h | aio_recv_pool\<N\> / aio_mtu_pool / aio_recv_mtu_pool | AIO 通道共享固定内存池接收缓存(多通道多线程共享) |
| aio_recv_que.h | aio_recv_que | AIO 通道共享单写单读队列接收缓存(多通道单线程共享，零拷贝分发) |
| heart_manage.h | heart_manage | 心跳超时检测(3 倍间隔 + 无业务消息 = 异常) |
| reconnect_ctl.h | reconnect_ctl | 断线重连控制(最大次数/间隔检查/连接状态跟踪) |

> 选型/配对/避坑详见 [使用范式/网络.md](使用范式/网络.md)

## 文件清单

```
docs/
├── README.md                          (本文件：总览 + 场景速查 + 分类详解 + 命名/分层)
├── 架构范式.md                        (跨组系统组装)
└── 使用范式/                          (组级深度手册)
    ├── 队列.md
    ├── 内存池.md
    ├── 线程.md
    ├── 网络.md
    ├── 哈希与数据结构.md
    └── 锁与同步.md
```

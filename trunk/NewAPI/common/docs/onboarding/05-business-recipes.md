---
purpose: BUSINESS-RECIPES
audience: AI agents and humans implementing cross-group assembly patterns
source-of-truth: yes (for the 3 templates; deep rationale in 架构范式.md + 各使用范式)
last-reviewed: 2026-08-31
---

# Business Recipes：3 个跨组组装可粘贴模板

> 把 lbcommon 多个组件拼成完整业务系统的最小骨架。每个模板可直接抄到业务项目里改业务逻辑。
> 详细原理见 [架构范式.md](../架构范式.md) 与各 [使用范式/*.md](../使用范式/)。

## 1. 网络服务器组装（mthread + aio_tcp + que_mth_buf）

**场景**：高并发 TCP 服务器，epoll 多事件驱动，业务消息跨线程分发。

### 1.1 架构图

```
业务客户端 ──TCP──> [tcp_listen_ch] → accept → [aio_tcp:rx]
                                              │
                                              ├─ deal_msg ──> [业务处理]
                                              │
                                       [recv thread = mthread]
                                              │
                                    ┌─────────┴─────────┐
                                    │                   │
                          delive_ch_event       que_mth_buf
                          (投递业务事件)         (大消息跨线程)
                                    ↓                   ↓
                              [业务线程池]         [消费线程]
```

### 1.2 模板代码（业务事件模式）

```cpp
#include <aio_tcp.h>
#include <mthread.h>
#include <que_mth_buf.h>
#include <thread_comm.h>

using namespace lb_common;

// 1) 业务事件（投递到接收线程，单线程执行 send_msg）
struct biz_event {
  int32 msg_type;
  char  payload[48];
};
class biz_ev_op : public event_op {
  tcp_buf_ch *mch;        // 业务持有 aio_tcp 指针
public:
  biz_ev_op(tcp_buf_ch *p) : mch(p) {}
  void deal(char *pbuf) override {
    auto *ev = reinterpret_cast<biz_event *>(pbuf);
    // 此处 deal 在 mthread 单线程执行，send_msg 安全
    char reply[64];
    int n = snprintf(reply, sizeof(reply), "ack:%d\n", ev->msg_type);
    int32 ret = mch->send_msg(reply, n);
    if (ret < 0) { /* 处理错误 */ }
  }
};

// 2) 消息回调（接收线程触发；deal_msg 后投业务事件）
class srv_recv_cb : public ch_recv_cb<tcp_buf_ch> {
  mthread *th;            // 持有接收线程指针（用于 delive_ch_event）
public:
  srv_recv_cb(mthread *t) : th(t) {}
  int32 deal_msg(tcp_buf_ch *pch, aio_msg &msg) override {
    // 必须手动调 on_com_msg（heart.on_msg 在 deal_recv 内被注释！）
    pch->on_com_msg();
    // 把业务处理封成事件投回接收线程（避免多线程 send_msg）
    biz_ev_op *op = new biz_ev_op(pch);
    biz_event ev{msg.msglen, {}};
    memcpy(ev.payload, msg.pmsg, min(msg.msglen, 48));
    int32 ret = pch->delive_recv_event(op, &ev, sizeof(ev), /*needadd=*/1);
    if (ret < 0) { delete op; return msg.msglen; }   // 投递失败回退
    return msg.msglen;
  }
};

// 3) 通道生命周期
class srv_ch_op : public aio_ch_op<tcp_buf_ch> {
public:
  void deal_ch_closing(tcp_buf_ch *pch, int32 err) override {
    // "不再可见"：从全局 map/索引移除
    // pch->user_data = your_handle
  }
  void deal_ch_closed(tcp_buf_ch *pch, int32 err) override {
    // 引用归零，**真正释放对象**
    auto *handle = (your_handle_t *)pch->get_user_data();
    delete handle; delete pch;
  }
};

// 4) 监听端回调（接受新连接）
class srv_listen_op : public tcp_listen_op {
public:
  void deal_listen_accept(tcp_listen_ch *pl, csock_addr &caddr, int32 fd) override {
    auto *pch = new tcp_buf_ch();
    aio_attr battr{}; battr.maxmsglen = 4096; battr.buf_size = 4096;
    channel_attr cattr{}; cattr.family = CHANNEL_FAMILY_IPV4;
    if (pch->accept_ch(battr, cattr, &g_rcb, &g_cop, fd) < 0) {
      delete pch; return;
    }
    pch->start_ch(g_recv_th);
  }
};
```

### 1.3 关键约束

- **必须**手动 `on_com_msg()` / `on_heart_msg()`（[网络.md §6](../使用范式/网络.md#6-heart_manage-心跳)）
- **必须**配对 `delive_ch_event(needadd=1)` ↔ `end_ch_event(1)`
- **必须**在 `deal_ch_closed` 才 `delete pch`（不是 `deal_ch_closing`）
- 监听端回调在监听线程触发；客户端连接后立即 `start_ch` 加入 `g_recv_th`

### 1.4 发送串行化（event_op 投递）

**铁律**：`send_msg` 非线程安全。多线程发送必须封成 `event_op` 投到 recv thread：

```cpp
// 任何线程要发送时：
biz_ev_op *op = new biz_ev_op(pch);
int32 ret = pch->delive_recv_event(op, &biz_payload, sizeof(biz_payload), 1);
// op 的 deal() 在 recv_th 单线程执行 → send_msg 安全
```

或用发送线程 + 私有队列（[网络.md §发送模式](../使用范式/网络.md#发送模式与线程安全) §模式 2）。

### 1.5 延伸阅读

- 详细：[架构范式.md §范式 1](../架构范式.md#范式-1网络服务器组装mthread--aio_tcp--que_mth_buf)
- 缓存策略：[网络.md §1](../使用范式/网络.md#12-异步框架层--3-缓存策略)
- 引用计数：[网络.md §5](../使用范式/网络.md#5-tcp_ch-引用计数state_machineh-的-src_stat_ref)

---

## 2. 跨进程共享组装（shmem_pool + que_proc_buf 或 que_spw_fixed）

**场景**：进程间共享内存通信，**单进程写 + 多进程读**（如行情分发、主备共享状态）。

### 2.1 架构图

```
                  ┌──────────────────────────────────┐
                  │       共享内存 (SHM)              │
                  │   shm_block_pool + que_proc_buf  │
                  └──────────────────────────────────┘
                       ↑ 写              ↓ 读
                  ┌─────────┐       ┌─────────────┐
                  │ 写进程  │       │ 多个读进程   │
                  │(创建者) │       │(打开者)      │
                  └─────────┘       └─────────────┘
```

### 2.2 写进程（创建者）

```cpp
#include <shmem_pool.h>
#include <que_proc_buf.h>

using namespace lb_common;

struct msg_data {      // **必须 POD**
  int32 key;
  char  value[60];
};

// 创建者：唯一负责写
class shm_writer {
  shm_block_pool pool;
  que_proc_buf    q;
  bool inited = false;
public:
  int32 init(const char *shm_name) {
    int32 ret = pool.init(shm_name, /*init_block_num*/16,
                          /*max_area_num*/4, /*max_block_num*/256,
                          /*data_size*/sizeof(msg_data),
                          /*is_continue*/0, /*create_mem*/1);
    if (ret < 0) return ret;
    ret = q.init(shm_name, /*que_size*/1024, /*align_byte*/64,
                 /*is_continue*/0, /*is_create*/1);
    if (ret < 0) { pool.close(); return ret; }
    inited = true;
    return 0;
  }
  int32 send(const msg_data &m) {
    // 1) 从池申请数据块
    int64 pos;
    int32 ret = pool.take_data((void*&)pos, ...);  // 详见 shmem_pool.h API
    if (ret < 0) return ret;
    // 2) 写入数据
    msg_data *p = pool.addr2data(addr);
    *p = m;
    // 3) 通过队列投递 shm_data_addr
    ret = q.write_mp(&addr, sizeof(addr));
    return ret;
  }
  void close() {
    if (!inited) return;
    q.close(1);       // **is_create=1**
    pool.close();
    inited = false;
  }
};
```

### 2.3 读进程（打开者）

```cpp
class shm_reader {
  shm_block_pool pool;
  que_proc_buf    q;
public:
  int32 init(const char *shm_name) {
    int32 ret = pool.init(shm_name, 16, 4, 256,
                          sizeof(msg_data), /*is_continue*/0,
                          /*create_mem*/0);   // **打开者**
    if (ret < 0) return ret;
    ret = q.init(shm_name, 1024, 64, /*is_continue*/0, /*is_create*/0);
    return ret;
  }
  int32 recv(msg_data *o_msg) {
    shm_data_addr addr;
    int64 ret = q.read_mp(&addr, sizeof(addr));
    if (ret <= 0) return ret;
    // **只读**访问数据——不调 get/release/take/back
    msg_data *p = pool.addr2data(addr);
    *o_msg = *p;
    return ret;
  }
  void close() {
    q.close(0);       // **is_create=0**
    pool.close();
  }
};
```

### 2.4 关键约束

- **写进程 = 创建者**（`create_mem=1`, `is_create=1`）；读进程 `create_mem=0`, `is_create=0`
- 读进程**只能**调 `addr2data` 只读访问；不可调 `get/release/take/back`（会破坏共享内存链表，编译期不报错）
- 所有进程必须用**同一份 lbcommon 头文件**编译（`shm_data_addr` 字段 ABI 一致）
- 启动顺序：创建者先 `init` 完成；打开者自旋等 `mask!=0`（队列）或 `state==1`（pool）
- `que_proc_buf` 崩溃恢复**丢数据**（业务须容忍）；`que_spw_fixed` 无崩溃检测（崩溃由外部保证）

### 2.5 延伸阅读

- 详细：[架构范式.md §范式 2](../架构范式.md#范式-2跨进程共享组装shmem_pool--que_proc_buf)
- 共享内存语义：[内存池.md §5](../使用范式/内存池.md#5-shmem_pool-共享内存语义)
- 队列崩溃恢复：[队列.md §4.2](../使用范式/队列.md#42-op_pid-崩溃检测que_proc_buf-独有)
- 读进程操作边界：[内存池.md §5.4](../使用范式/内存池.md#54-读进程操作边界重要)

---

## 3. 重连心跳组装（aio_tcp + heart_manage + reconnect_ctl + timer_thread）

**场景**：维持长连接活性（行情订阅、交易前置、推送通道）。定时器线程查心跳/重连，接收线程处理连接事件。

### 3.1 架构图

```
[timer_thread]
  ├─ check_reconnect (1Hz)   ──┐
  ├─ check_heart_send (1Hz)   │
  └─ check_heart_timeout      │
                              ↓
                    (投递 LINK_CONNECT / LINK_CLOSE)
                              ↓
[mthread 接收线程]
  ├─ deal_connect_ch ───> start_ch + login
  ├─ deal_close_ch   ───> set_disconnected
  ├─ deal_msg
  │   ├─ 心跳应答 → on_heart_msg
  │   └─ 业务消息 → on_com_msg
  └─ send heartbeat (check_heart_send)
```

### 3.2 模板代码

```cpp
#include <aio_tcp.h>
#include <heart_manage.h>
#include <reconnect_ctl.h>
#include <time_thread.h>
#include <mthread.h>

using namespace lb_common;

enum LinkEvType { LINK_CONNECT = 1, LINK_CLOSE = 2 };
struct link_event { int32 type; int32 err; };

class link_ev_op : public event_op {
  tcp_mtu_pool_ch *mch;
  reconnect_ctl   *rc;
  mthread         *th;
public:
  link_ev_op(tcp_mtu_pool_ch *p, reconnect_ctl *r, mthread *t)
    : mch(p), rc(r), th(t) {}
  void deal(char *pbuf) override {
    auto *e = reinterpret_cast<link_event *>(pbuf);
    if (e->type == LINK_CONNECT) {
      // 在 recv thread 调 connect_ch（与 deal_msg 不竞争 ch 状态）
      csock_addr remote; remote.set_addr("x.x.x.x", 9000);
      channel_attr cattr{}; cattr.family = CHANNEL_FAMILY_IPV4;
      aio_attr battr{}; battr.maxmsglen = 4096; battr.buf_size = 4096;
      battr.heartinterval = 5;     // 5 秒心跳
      if (mch->connect_ch(battr, cattr, /*rcb*/nullptr, /*cop*/nullptr,
                           /*pconnectth*/th, &remote) == 0) {
        mch->start_ch(/*precvth*/th);
      }
    } else {  // LINK_CLOSE
      mch->close_ch(e->err);
    }
  }
};

class heart_timer_op : public time_event_op {
  tcp_mtu_pool_ch *mch;
  reconnect_ctl   *rc;
  mthread         *th;
public:
  void deal_timer(int32 is_period, int32 timer, char *pbuf) override {
    // 1) 重连（仅未连接 + 已发生过连接）
    if (rc->check_reconnect(2)) {  // 2 秒间隔
      link_ev_op *op = new link_ev_op(mch, rc, th);
      link_event ev{LINK_CONNECT, 0};
      th->delive_ch_event(...);   // 投递到接收线程
      // 注：needadd=1 时配对 end_ch_event(1)
    }
    // 2) 心跳超时（仅已连接）
    if (mch->is_work() && mch->check_heart_timeout()) {
      link_ev_op *op = new link_ev_op(mch, rc, th);
      link_event ev{LINK_CLOSE, LBERR_TIME_WAIT_OUT};
      th->delive_ch_event(...);
    }
    // 3) 心跳发送（仅已连接）
    if (mch->is_work() && mch->check_heart_send()) {
      // 投递心跳发送事件到接收线程
    }
  }
};
```

### 3.3 关键约束

- **业务消息必须在 deal_msg 手动调** `on_com_msg()`（[网络.md §6](../使用范式/网络.md#6-heart_manage-心跳)）
- **首次连接门**：`m_happen_connected==0` 时 `check_reconnect` 返回 false——首次连接失败需业务另行处理
- **`set_disconnected` 在 `deal_ch_closed` 调**（不是 `deal_ch_closing`）
- **不在定时器线程直接调** `connect_ch`/`close_ch`——会与接收线程 `deal_msg` 竞争 ch 状态
- 心跳 `msg_count` 非原子——`on_msg` 不可跨线程（同线程约束）

### 3.4 生命周期

```
启动顺序：mthread_pool.run() → timer_thread.run()
关闭顺序：timer_thread.to_stop()+join() →
          mthread.to_stop()+join() → delete ch
```

### 3.5 延伸阅读

- 详细：[架构范式.md §范式 3](../架构范式.md#范式-3重连心跳组装reconnect_ctl--heart_manage--aio_tcp)
- 心跳：[网络.md §6](../使用范式/网络.md#6-heart_manage-心跳)
- 重连：[网络.md §7](../使用范式/网络.md#7-reconnect_ctl-重连)
- 定时器：[线程.md §5](../使用范式/线程.md#5-timer_thread--timer_order_list-定时器)

---

## 4. 不属于"跨组组装"但常用的组合

### 4.1 多线程任务池（mthread_pool）

```cpp
#include <mthread.h>
using namespace lb_common;

mthread_pool pool;
pool.init(/*N*/4, /*maxpoll*/1024, /*eventq_size*/256,
          /*eventq_busy*/6, /*waitms*/10);
pool.run();

// 投递任务
mthread *th = nullptr;
pool.assign_thread(th);                  // 按负载选线程
event_info ev{...};
th->write_mth_or_get_event(ev);           // 投递事件

// 关闭
pool.destroy();   // 逐个 to_stop + join + delete[]
```

### 4.2 进程级优雅退出

```cpp
#include <thread_comm.h>
using namespace lb_common;

int main() {
  install_signal_hand();    // SIGTERM/SIGQUIT → g_signal_stopctl
  // 所有线程循环查 g_signal_stopctl 自动退出
  // ...
  return 0;
}
```

## 5. 下一步

- 选哪个组件：[04-decision-trees.md](./04-decision-trees.md)
- 坑点：[06-pitfalls-index.md](./06-pitfalls-index.md)
- 不支持项：[07-limitations.md](./07-limitations.md)
- 仓库导航：[../README.md](../README.md)

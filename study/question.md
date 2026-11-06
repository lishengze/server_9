# 问题分析：NewAPI 登录 / 委托 / 回报 全链路

> 分析对象：`trunk/NewAPI/gone/api/` + `trunk/NewAPI/gone/fgw/`
> 关联源码：`api_instance.h/.cpp`、`single_socket_engine.cpp`、`multi_socket_engine.cpp`、
> `counter98.cpp`、`fpga_counter_base.cpp`、`fpga_counter_direct.cpp`、`gw_counter_direct.cpp`、
> `callback_manager.cpp`、`fgw/src/api_engine.cpp`、`include/g1trademsg.h`、`g1msghead.h`、`c98msg_tmp.h`

---

# 一、总体架构与核心设计要点

1. **柜台与引擎解耦**：柜台懂协议、无线程；引擎懂传输、带线程。通过无锁队列（`que_mth_buf`）跨线程通信。
2. **统一回调出口**：所有柜台回报最终收敛到 `callback_manager` 的 `on_*` 接口，对客户暴露单一 `api_callback` 虚接口。
3. **direct / queued 双模式**：direct 保证最低延迟（IO 线程直接调）；queued 用独立回调线程 + 无锁队列隔离，避免阻塞 IO 线程。
4. **登录级联**：98 柜台统一入口，成功后把 `acc_login_event_info` 级联投递给极速柜台，实现"配置化选择登录柜台"。
5. **队列统一模式**：所有跨线程通信（柜台 → 引擎）都通过 `que_mth_buf` 无锁队列 + `link_send_event` 头：

```cpp
link_send_event{                       // 队列节点头部
    int16 link_type,                   // 目标链接类型（SPEED_TRADE / SPEED_GW / 98）
    int16 type,                        // 事件类型（SEND_MSG / HEART / LOGIN / CONNECT / CLOSE）
    int32 data_len,                    // 负载长度
    char data[0]                       // 柔性数组：业务消息体 / 事件信息
}
              ↓
引擎线程 do_work() 消费队列 → switch(type)
  → case SEND_MSG:  link_.send_msg(data, data_len)
  → case ACCOUNT_LOGIN: deal_cust_login(reinterpret_cast<acc_login_event_info*>(data))
  → case HEART:  build_heart_msg → send
  → case CONNECT: link_.connect(...)
  → case CLOSE:  link_.close_ch(...)
```

**关键的"柜台 → 引擎"接口函数（位于各柜台类中）**：
- `take_req_que_mem(data, len)`：申请队列内存，填充 `link_send_event` 头
- `cmt_req_que_mem(pos, len)`：提交队列，触发引擎消费
- 这两个函数是模板/内联的，零额外开销。柜台组包后直接写入引擎的无锁队列，引擎线程在 `do_work()` 中消费并发送。

---

# 二、登录链路

## 2.1 柜台初始化（`api_impl::init`，api_instance.cpp:134）

`fast_` 是模板成员，类型由 5 种配置决定（`gw_counter_direct` / `fpga_counter_direct` / `fpga_counter_gateway`）。
初始化分三个阶段：

```cpp
// ① 柜台对象自身初始化：设置 cb_mgr_、log_，拷贝配置
ret = fast_.init(cfg, &cb_mgr_, &log_);

// ② 极速引擎初始化：引擎内部持有 counter_ 指针（TFastCounter*）
ret = fast_engine_.init(cfg, &fast_, &log_);

// ③ 注入"柜台 → 引擎"的双向引用（关键步骤）
if (fpga_gateway) {
    fast_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
} else {
    fast_.init_trade(fast_engine_.get_queue(), fast_engine_.get_out_op()); // 业务发送队列
}
fast_.init_gateway(multi_engine_.get_queue(), multi_engine_.get_out_op()); // GW 发送队列
```

**关键点**：`init_trade` / `init_gateway` 把引擎的**无锁发送队列**（`que_mth_buf*`）和**链接操作接口**（`link_engine_outop*`）注入柜台。此后柜台组包后通过 `take_req_que_mem` + `cmt_req_que_mem` 入队，引擎线程消费队列并实际发送。柜台**不直接碰 socket**，所有网络操作都经引擎。

- `fast_engine_`（single_socket_engine / tcpdirect_engine）：极速**业务**链接（直连核心，socket_single 模式）
- `multi_engine_`（multi_socket_engine）：控制平面，管理 98 链接（槽0）+ 极速 GW 链接（槽1）

## 2.2 与柜台链接（`api_impl::start`，api_instance.cpp:193）

极速链接是**按需建立（lazy）**的，start 阶段只做线程与定时器准备：

```cpp
// ② 把极速链接的心跳/重连 timerfd 注册到 multi 引擎的 epoll 线程（控制平面）
fast_engine_.add_timer_poll(multi_engine_.get_thread());

// ③ 98 链接同步建链（仅建链）
multi_engine_.connect_98agw();

// ④ multi 引擎异步启动（接管 98 + 极速GW 的 IO）
multi_engine_.start();

// ⑤ fast 引擎异步启动（接管极速业务链接 IO）
fast_engine_.start();

// ⑥ 同步 agw 登录（必须最后，此时 multi 线程已运行可收应答）
c98_.deal_agw_login();
```

真正的极速链接在**账户登录时**建立，分柜台类型：

| 柜台类型                           | 链接建立时机与方式                                                                                                                                |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| **gw_direct（个微）**        | `single_socket_engine::deal_cust_login`：若 link 空闲则 `link_.connect()` 直连核心（不支持切换）                                              |
| **fpga_direct（FPGA直连）**  | 先建 GW 链接（`multi_engine` 槽1）→ GW 登录应答回填 trade_ip/port → `deal_log_ans` → `delive_fpga_connect` 触发 **FPGA core 链接** |
| **fpga_gateway（FPGA网关）** | 只走 GW 链接（`LINK_TYPE_SPEED_GW`），业务也走此链接，无独立 core 链接                                                                          |

## 2.3 登录请求发送路径（LoginReq，以个微 gw_direct 为例）

> 追踪从客户入口到个微柜台发送的完整路径，说明"无锁队列 + 引擎线程消费 + 链接发送"的通用模式。

登录请求由 **98 柜台统一入口** 发起，再级联触发极速柜台登录。客户调用 `api->login(req)`，经过 10 个步骤到达个微核心：

```cpp
① 客户入口 → api_interface::login(req)
   └── 虚基类接口，最终调用 api_impl::login

② api_impl::login(req)                                         [api_instance.cpp:271]
   └── return c98_.deal_login_req(req);                         // 委托给 98 柜台统一入口

③ counter98::deal_login_req(req)                                [counter98.cpp:662]
   ├── 检查 agw_login_state==2（agw 已登录）
   ├── build_cust_login_event(req, info)                        // LoginReq → acc_login_event_info
   ├── 构造 link_send_event{link_type=LINK_TYPE_98, type=ACCOUNT_LOGIN, data_len=sizeof(info)}
   ├── gw_send_queue_->write_get_mth(data, total_len)           // 写入 multi 引擎的发送队列
   ├── gw_send_queue_->write_cmt_mth(pos, total_len)            // 提交
   └── gw_eng_op_->trigger_send()                               // 唤醒 multi 引擎线程

④ multi_engine::deal_event() 消费队列                           [multi_socket_engine.cpp:206]
   ├── read_get 取出 link_send_event
   ├── case ACCOUNT_LOGIN:
   │   └── evt->link_type == LINK_TYPE_98 → deal_cust98_login(*pmlog)
   └── read_cmt 释放

⑤ multi_engine::deal_cust98_login(pmlog)                       [multi_socket_engine.cpp:356]
   ├── counter98_->deal_cust_login(pmlog, log_buf, sizeof(log_buf))
   └── 返回 0 或消息长度

⑥ counter98::deal_cust_login(pmlog, log_buf, buf_len)          [counter98.cpp:717]
   ├── 当前 cust_need_login=false（98 账户无需额外登录）
   └── return delive_fast_counter_login(req);                   // 级联投递给极速柜台

⑦ counter98::delive_fast_counter_login(info)                   [counter98.cpp:755]
   ├── fast_counter_type_ == gw_direct
   │   ├── tlink_type = LINK_TYPE_SPEED_TRADE
   │   └── tq = trade_send_queue_                               // ★ 个微登录投到 fast_engine 的发送队列
   ├── 构造 link_send_event{link_type=SPEED_TRADE, type=ACCOUNT_LOGIN, data_len=sizeof(info)}
   ├── trade_send_queue_->write_get_mth → write_cmt_mth         // 写入 fast_engine 发送队列
   └── 注意：个微不调 trigger_send（fast_engine 是 simple_thread 死轮询模式，自动轮询）

⑧ single_socket_engine::do_work() 消费队列                     [single_socket_engine.cpp:118]
   ├── read_get 取出 link_send_event
   ├── case ACCOUNT_LOGIN → deal_cust_login(*pmlog)
   └── read_cmt 释放

⑨ single_socket_engine::deal_cust_login(pmlog)                 [single_socket_engine.cpp:185]
   ├── 只支持 gw_direct（非 gw 则返回）
   ├── link_.is_free() → link_.connect(recv_poll_num_, 0, NULL) // ★ 建立个微 TCP 链接（直连核心）
   ├── counter_->deal_cust_login(pmlog, log_buf, sizeof(log_buf)) // 柜台构建登录消息
   ├── 返回 tlen > 0 → link_.send_msg(log_buf, tlen)            // ★ 发送给个微核心
   └── 返回 <0 → ans_cust_login 回调失败；返回 0 → 已登录跳过

⑩ gw_counter_direct::deal_cust_login(pmlog, log_buf, buf_len)  [gw_counter_direct.cpp:254]
   ├── login_state==2 → 已登录，直接 cb_mgr_->on_login 返回 0
   ├── login_state!=2 → build_login_msg(req, 1, head)           // 构造 g1_msg_head + login_req
   │   └── msg_id=G1_MSG_LOGIN_REQ, msg_len=sizeof(login_req)
   ├── login_state = 1                                          // 进入"登录中"状态
   └── 返回 msg_len
```

## 2.4 登录应答返回路径

```cpp
个微核心 → 网络 → aio_socket_link::msg_cb → gw_counter_direct::deal_recv_msg
  → case G1_MSG_LOGIN_ANS → deal_log_ans
  → login_state = 2（成功）或 0（失败）
  → cb_mgr_->on_login(ans)                                      // 回调客户
```

各柜台登录应答后的差异：

- **gw_direct**：`deal_recv_msg → deal_log_ans` → 成功 `login_state=2` → `cb_mgr_->on_login(ans)`
- **fpga_direct**：`deal_recv_msg → G1_MSG_LOGIN_ANS → deal_log_ans` → 保存 trade_ip/port → `delive_fpga_connect` 建 core 链接 → 证券信息获取完成后 `cb_mgr_->on_login`
- **fpga_gateway**：`deal_recv_msg → deal_log_ans` → 直接 `cb_mgr_->on_login`（无 core 同步步骤）

**登录回调统一出口**：三种柜台最终都通过 `cb_mgr_->on_login(ans)` 通知客户。

## 2.5 登录消息中的 version 字段来源（fgw 版本校验）

### 2.5.1 读取位置

`api_engine::deal_login_req(api_link *link, g1_msg_head *head)` [api_engine.cpp:378/395]：

```cpp
login_req *nreq = reinterpret_cast<login_req *>(head + 1);  // 指向 g1_msg_head 之后的消息体
if (std::strncmp(g1_msg_ver, nreq->version, std::strlen(g1_msg_ver)) < 0) {
    send_login_error(link, head, FGW_ERR_MSG_VER, "not support msg version");
}
```

`nreq` 是消息体（`login_req`），`nreq->version` 就是**客户端随登录请求发来的版本字段**。

### 2.5.2 version 字段的结构体定义（确实存在）

- **g1 协议 `login_req`**（g1trademsg.h:35）：
  ```cpp
  int32_t req_connect_id;                     //网关填写，api 设为0
  char version[G1_VERSION_LEN];               //版本号
  ```
- **c98 协议 `c98_agw_login_req`**（c98msg_tmp.h:46）：
  ```cpp
  char agw_user_password[256]; //agw 用户密码
  char version[64];            //版本号
  ```
- **c98 协议 `c98_acc_login_req`**（c98msg_tmp.h:70）：
  ```cpp
  char order_way[2];        //委托通道类型
  char version[64];         //版本号
  ```

> **特别澄清**：API 层的 `LoginReq`（`order_trade_type.h:8`，客户 `api->login(req)` 传入的类型）**确实没有 version 字段**，它只有 `client_req_no`、`cust_id`、`fund_account_id` 等业务参数。客户无需、也无法传入版本号。
>
> g1 与 c98 的登录消息体（`login_req` / `c98_agw_login_req`）**都显式包含 version 字段**，但这个字段**不是来自客户请求**，而是 SDK 在组包时自动填充的。

### 2.5.3 谁填充了它 —— 客户端 `build_login_msg`

客户端在构造登录消息时，把全局版本常量 `g1_msg_ver` 复制进 `login_req.version`：

- **`fpga_counter_base.cpp:583-584`**：
  ```cpp
  std::memset(body->version, 0, sizeof(body->version));
  std::strncpy(body->version, g1_msg_ver, strlen(g1_msg_ver));
  ```
- **`gw_counter_direct.cpp:202-203`**（个微直连，同样逻辑）：
  ```cpp
  std::memset(body->version, 0, sizeof(body->version));
  std::strncpy(body->version, g1_msg_ver, std::strlen(g1_msg_ver));
  ```

### 2.5.4 `g1_msg_ver` 定义在哪

**`g1msghead.h:6`**：
```cpp
static const char *g1_msg_ver = "1.0.0";
```
注意：这个常量**客户端（api）和 fgw 共用同一个头文件**，两端拿到的是同一个值。

### 2.5.5 校验逻辑的含义

fgw 用自己端的 `g1_msg_ver` 与客户端发来的 `nreq->version` 比较：

```cpp
if (std::strncmp(g1_msg_ver, nreq->version, std::strlen(g1_msg_ver)) < 0)
```
`strncmp(a, b) < 0` 表示 `a < b`，即**服务端版本 < 客户端版本**时拒绝。也就是说：客户端版本不能高于服务端版本（超前客户端可能用了服务端还不支持的协议扩展）。

#### 为什么比较有意义？——编译期版本快照

关键理解：`g1_msg_ver` 是**编译期常量**，客户端 SDK 和 fgw 是**独立编译、独立发布**的，各自把 `g1_msg_ver` 的值"快照"进自己的二进制：

```
开发时（同版本）：
  SDK 编译 → g1_msg_ver = "1.0.0" 快照进 SDK 二进制
  fgw 编译 → g1_msg_ver = "1.0.0" 快照进 fgw 二进制
  → 比较相等 → 校验通过（所以开发时看不出意义）

跨版本部署：
  SDK v1.0（旧）二进制仍带 "1.0.0"
  fgw v1.1（新）编译时 g1_msg_ver 改为 "1.1.0"
  → strncmp("1.1.0", "1.0.0") < 0 不成立 → 拒绝接入
```

**所以比较的不是"两个运行时相同的常量"，而是"两个不同二进制各自编译时的版本快照"**。这个校验防止不兼容的旧 SDK 接入升级后的新协议。

#### version 完整填充链路总结

```
客户 api->login(LoginReq req)         // LoginReq 无 version
  → counter98::deal_login_req
  → build_cust_login_event(req, info) // LoginReq → acc_login_event_info（仍无 version）
  → delive_fast_counter_login(info)   // 级联投递
  → gw_counter_direct::deal_cust_login
  → build_login_msg(info, 1, head)    // ★ version 在此处自动填入
       body->version = g1_msg_ver     // SDK 内部常量，与客户请求无关
  → 发送给 fgw
  → fgw 解析 nreq->version，与自己的 g1_msg_ver 比较
```

**结论**：version 不是客户传的，是 SDK 在组包时自动填的编译期版本常量。比较的意义是**跨版本兼容性检测**。

### 2.5.6 特别说明 counter98

`counter98.cpp` 走的是 **c98 协议**（`c98_msg_head_tmp + c98_agw_login_req`），不是 g1 协议：

- `build_agw_login_msg`（counter98.cpp:611）中 `c98_agw_login_req.version` **保留未填充**（第629行 `// version 字段保留`）。
- 所以 `g1_msg_ver` 版本校验路径只适用于走 g1 协议的 fpga / gw 柜台，**counter98 不走这条校验**。

---

# 三、委托链路（OrderReq）

## 3.1 委托请求路径

客户调用 `api->order_insert(req)`，路径比登录短得多（无需建链，无需 98 级联）：

```cpp
① 客户入口 → api_interface::order_insert(req)
   └── 虚基类接口，最终调用 api_impl::order_insert

② api_impl::order_insert(req)                                  [api_instance.cpp:277]
   ├── int32 ret = fast_.deal_order_req(req);                   // 先尝试个微柜台
   ├── if (COUNTER_OFFLINE || UNSUPPORTED_OP)                   // 失败则降级 98
   │   └── return c98_.deal_order_req(req);
   └── return ret;

③ gw_counter_direct::deal_order_req(req)                       [gw_counter_direct.cpp:53]
   ├── 检查 trade_link_connect_==0 → 返回 LINK_DISCONNECTED    // 链接未建立
   ├── 检查 login_state!=2 → 返回 NOT_LOG_CUST                 // 未登录
   ├── take_req_que_mem(data, take_len)                         // [gw_counter_direct.h:118]
   │   ├── trade_send_queue_->write_get_mth(data, take_len + sizeof(link_send_event))
   │   ├── 填充 link_send_event{
   │   │       link_type = LINK_TYPE_SPEED_TRADE,
   │   │       type     = LINK_EVENT_TYPE_SEND_MSG,
   │   │       data_len = take_len
   │   │   }
   │   └── 返回 pos（队列位置）
   ├── build_order_msg(req, data + sizeof(link_send_event))     // 构造个微委托消息体
   │   └── ★ 当前 todo 留空（个微真实协议未知，临时用 g1_msg_head 占位）
   └── cmt_req_que_mem(pos, take_len)                           // [gw_counter_direct.h:133]
       └── trade_send_queue_->write_cmt_mth(pos, take_len + sizeof(link_send_event))
           // 注意：注释掉了 trade_eng_op_->trigger_send()
           // 因为 fast_engine 是死轮询模式，自动轮询无需触发

④ single_socket_engine::do_work() 消费队列                     [single_socket_engine.cpp:118]
   ├── read_get 取出 link_send_event
   ├── case LINK_EVENT_TYPE_SEND_MSG:
   │   └── link_.send_msg(evt->data, evt->data_len)             // ★ 直接发送给个微核心
   │       └── 失败 → counter_->deal_send_error(..., ret)       // 回调客户拒绝回报
   └── read_cmt 释放
```

## 3.2 登录 vs 委托 路径对比

| 环节 | 登录请求 | 委托请求 |
|------|---------|---------|
| **入口** | `api_impl::login → c98_.deal_login_req` | `api_impl::order_insert → fast_.deal_order_req` |
| **98 级联** | 必须：先 agw 登录 → 98 账户登录 → 极速柜台登录 | 不需要；失败才降级 98 |
| **建链时机** | 登录时按需建链（`link_.connect`） | 前提是已登录（`login_state==2`）+ 已链接 |
| **队列** | 两级：gw_send_queue_(multi) → trade_send_queue_(fast) | 一级：trade_send_queue_(fast) |
| **引擎消费** | multi_engine::deal_event → single_socket_engine::do_work | single_socket_engine::do_work |
| **消息构建** | `build_login_msg`（有临时实现） | `build_order_msg`（todo 留空） |
| **发送** | `link_.send_msg`（aio_socket_link） | `link_.send_msg`（aio_socket_link） |
| **降级路径** | 98 登录失败 → `cb_mgr_->on_login(err)` | 个微离线 → `c98_.deal_order_req` |

---

# 四、回报回调链路

> 不同柜台处理器收到回报后，如何一步步回调给客户。回报回调分**两层**：链路层（网络→柜台）→ 柜台层（解析→callback_manager→客户）。

## 4.1 链路层：网络数据 → 柜台 `deal_recv_msg`

```cpp
网络收包 → aio_tcp / tcpdir_ch 异步 IO
  → link 的 msg_cb（ch_recv_cb / tcpdir_msg_cb）
  → counter_.deal_recv_msg(msg.pmsg, msg.msglen, link_type)
```

- `aio_socket_link.cpp:30`：`return owner_->counter_->deal_recv_msg(msg.pmsg, msg.msglen, owner_->link_type_);`
- `tcpdir_link.cpp:33`：`return owner_->counter_->deal_recv_msg(pbuf, len, owner_->link_type_);`

`link_type` 标明来源链接（LINK_TYPE_98 / LINK_TYPE_SPEED_GW / LINK_TYPE_SPEED_TRADE）。

## 4.2 柜台层：`deal_recv_msg` 解析并按消息类型分发

各柜台 `deal_recv_msg` 用 while 循环按消息头**逐个解析多个完整消息**（兼容粘包/半包），
按 `msg_id` 分发到具体处理函数：

| 柜台                           | 消息头                  | 分发示例                                                                                                                                                                               |
| ------------------------------ | ----------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **counter98**            | `c98_msg_head_tmp`    | AGW_LOGIN_ANS→`deal_agwuser_login_ans`；ACC_LOGIN_ANS→`deal_cust_login_ans`；HEART_ANS→心跳                                                                                     |
| **fpga_counter_direct**  | `g1_msg_head`         | ORDER_RTN→`deal_order_rtn`；TRADE_RTN→`deal_trade_rtn`；CANCEL_RSP→`deal_cancel_rsp`；LOGIN_ANS→`deal_log_ans`；OFFLINE_PUSH→`deal_fpag_state`；GW_REJ→`deal_gw_rej` |
| **fpga_counter_gateway** | `g1_msg_head`         | 同上，但按客户哈希（`clients_.find`）找到对应 `fpga_cust_info` 再处理                                                                                                              |
| **gw_counter_direct**    | `g1_msg_head`（临时） | LOGIN_ANS→`deal_log_ans` 等                                                                                                                                                         |

> 未知消息默认跳过（不返回错误、不关链接），保证旧 API 对新增消息的兼容性。

## 4.3 回调分发：`cb_mgr_` → 客户

以 FPGA 委托回报为例（`fpga_counter_base.cpp:175` `deal_order_rtn`）：

```cpp
deal_order_rtn(head, cust, counter_type)
  → 校验长度、按 g1 order_rtn 解析
  → 构造 API 层 OrderRtn + StreamInfo（含 counter_type, stream_seq 等）
  → cb_mgr_->on_order_rtn(si_out, out)          // fpga_counter_base.cpp:230
```

`callback_manager::on_order_rtn`（callback_manager.cpp:98）按模式分发：

- **direct 模式**：直接同步调用 `user_callback_->on_order_rtn(si, rtn)`（在 IO/引擎线程中执行）
- **queued 模式**：
  1. 把 `cb_event_head{type=order_rtn, data_len}` + `StreamInfo` + `OrderRtn` 打包写入**无锁回调队列**（`write_get`/`write_cmt`）
  2. `trigger()` 唤醒回调线程
  3. 回调线程 `do_work()`（callback_manager.cpp:360）从队列 `read_get` 读取，按 `head->type` 分发：
     `case order_rtn: user_callback_->on_order_rtn(si, rtn)`（callback_manager.cpp:373-378）

## 4.4 回报回调统一出口（三种柜台对比）

| 回报类型  | 柜台处理函数                               | callback_manager 接口      | 客户回调           |
| --------- | ------------------------------------------ | -------------------------- | ------------------ |
| 委托回报  | `deal_order_rtn`                         | `on_order_rtn(si, rtn)`  | `on_order_rtn`   |
| 成交回报  | `deal_trade_rtn`                         | `on_trade_rtn(si, rtn)`  | `on_trade_rtn`   |
| 撤单回报  | `deal_cancel_rsp`                        | `on_cancel_rsp(si, rsp)` | `on_cancel_rsp`  |
| 登录应答  | `deal_log_ans` / `deal_cust_login_ans` | `on_login(ans)`          | `on_login`       |
| 链接状态  | `deal_link_connect/close`                | `on_link_status(...)`    | `on_link_status` |
| 错误/离线 | `deal_fpag_state` 等                     | `on_error(...)`          | `on_error`       |

**三种柜台在回报链路上的差异**：

- **counter98**：单客户、无缓存，`deal_recv_msg` 直接分发处理（查询应答也在此）
- **fpga_counter_direct**：单客户（`client_info_`），core 链接回报直接处理
- **fpga_counter_gateway**：多客户，`deal_recv_msg` 先用客户哈希（`clients_.find`）定位对应 `fpga_cust_info`，再调用**基类** `deal_order_rtn/trade_rtn/cancel_rsp`（基类公共解析逻辑复用）
- **gw_counter_direct**：单客户，临时用 g1 协议分发（真实协议待实现）

**完整链路示例（FPGA 直连委托回报）**：

```cpp
网络 → aio_tcp 异步收包
  → aio_socket_link::msg_cb::deal_msg
  → fpga_counter_direct::deal_recv_msg(buf, len, LINK_TYPE_SPEED_TRADE)
  → case G1_MSG_ORDER_RTN → deal_order_rtn(head, client_info_, fpga_direct)
  → 解析 g1 order_rtn → 构造 OrderRtn + StreamInfo
  → cb_mgr_->on_order_rtn(si, rtn)
  → [direct] user_callback_->on_order_rtn(si, rtn)
    或 [queued] 入队 → 回调线程 do_work → user_callback_->on_order_rtn(si, rtn)
```

---

# 五、撤单与查询链路

> 与登录/委托并列的另外两类消息：**撤单**（CancelReq）和**查询**（Order/Trade/Fund/Position Query）。
> 关键差异：撤单和委托一样走"极速柜台优先 + 98 降级"；**查询只走 98 柜台**（极速柜台 fpga/gw 均无查询接口）。

## 5.1 撤单链路（CancelReq）

### 5.1.1 撤单请求路径（极速柜台优先）

客户调用 `api->order_cancel(req)`，与委托几乎同构（先 fast 后降级 98）：

```cpp
① 客户入口 → api_interface::order_cancel(req)
   └── 虚基类接口，最终调用 api_impl::order_cancel

② api_impl::order_cancel(req)                                  [api_instance.cpp:301]
   ├── int32 ret = fast_.deal_cancel_req(req);                  // 先尝试极速柜台
   ├── if (ret == COUNTER_OFFLINE || UNSUPPORTED_OP)            // 失败则降级 98
   │   └── return c98_.deal_cancel_req(req);
   └── return ret;

③ fpga_counter_direct::deal_cancel_req(req)                    [fpga_counter_direct.cpp:117]
   ├── 检查 trade_link_connect==0 → LINK_DISCONNECTED
   ├── 检查 client_info_.login_state!=2 → NOT_LOG_CUST
   ├── 检查 client_info_.fpga_state==2 → COUNTER_OFFLINE        // 客户状态异常触发降级
   ├── trade_send_queue_->write_get_mth(data, total_len)        // 入队
   ├── 填充 link_send_event{link_type=SPEED_TRADE, type=SEND_MSG, data_len}
   ├── build_cancel_msg(req, client_info_, head)                // 构造 g1 cancel_req
   └── trade_send_queue_->write_cmt_mth(pos, total_len)

④ single_socket_engine::do_work() 消费队列
   └── case SEND_MSG → link_.send_msg(evt->data, evt->data_len) // 发送给核心
```

### 5.1.2 撤单回报回调（fpga `deal_cancel_rsp`）

核心返回撤单应答后，走与委托回报相同的两层回调：

```cpp
网络 → aio_socket_link → fpga_counter_direct::deal_recv_msg
  → case G1_MSG_CANCEL_RSP → deal_cancel_rsp(head, cust, counter_type)  [fpga_counter_base.cpp:292]
  → 校验长度、按 g1 cancel_rsp 解析
  → 依据 session_seq_no 做可靠消息去重（重复/过期包直接丢弃）
  → 构造 API 层 CancelRsp + StreamInfo（含 counter_type, stream_seq）
  → cb_mgr_->on_cancel_rsp(si_out, out)                         // fpga_counter_base.cpp:327
  → [direct] user_callback_->on_cancel_rsp(si, rsp)
    或 [queued] 入队 → 回调线程 do_work → user_callback_->on_cancel_rsp(si, rsp)
```

> 另外，网关路由拒绝（`G1_MSG_GW_REJ`）也会走到 `on_cancel_rsp`：当被拒绝的请求是 cancel_req 时，`deal_gw_rej` 构造 `CancelRsp` 后同样 `cb_mgr_->on_cancel_rsp`（fpga_counter_base.cpp:394）。

### 5.1.3 counter98 撤单（降级路径）

```cpp
③' counter98::deal_cancel_req(req)                              [counter98.cpp:175]
   ├── 检查 trade_link_connect_==0 → LINK_DISCONNECTED
   ├── 检查 agw_login_state!=2 → NOT_LOG_AGW
   ├── take_req_que_mem(data, take_len)                         // 98 发送队列入队
   ├── build_cancel_msg(req, data + sizeof(link_send_event))    // 构造 c98 cancel_req
   └── cmt_req_que_mem(pos, take_len)
```

## 5.2 查询链路（Order / Trade / Fund / Position Query）

### 5.2.1 查询只走 98 柜台

`api_instance.cpp:310-327` 中四类查询**全部直接委托 98 柜台**，无极速路径、无降级：

```cpp
template <class TF, class TE> int32 api_impl<TF, TE>::order_query(const OrderQueryReq &req)   { return c98_.deal_order_query(req); }
template <class TF, class TE> int32 api_impl<TF, TE>::trade_query(const TradeQueryReq &req)   { return c98_.deal_trade_query(req); }
template <class TF, class TE> int32 api_impl<TF, TE>::fund_query(const FundQueryReq &req)     { return c98_.deal_fund_query(req); }
template <class TF, class TE> int32 api_impl<TF, TE>::position_query(const PositionQueryReq&req) { return c98_.deal_position_query(req); }
```

> 原因：极速柜台（fpga / gw）**无查询接口**（`gw_counter_direct.h` 注释明确"个微柜台无查询接口, 与 fpga_counter_gateway 一致"）。

### 5.2.2 查询请求路径（以 counter98 为例）

四类查询结构一致，均为"入队 + 构建 c98 查询消息"：

```cpp
① 客户 → api->fund_query(FundQueryReq)  →  api_impl::fund_query → c98_.deal_fund_query(req)
② counter98::deal_fund_query(req)                               [counter98.cpp:342]
   ├── 检查 trade_link_connect_==0 → LINK_DISCONNECTED
   ├── 检查 agw_login_state!=2 → NOT_LOG_AGW
   ├── take_req_que_mem(data, take_len)                         // 98 发送队列入队
   ├── build_fund_query_msg(req, data + sizeof(link_send_event))// 构造 c98 查询消息
   └── cmt_req_que_mem(pos, take_len)
```

### 5.2.3 查询应答回调（接口已定义）

回调接口（api_callback.h）：

- `on_fund_query_ans(const CustFundInfo &info)`                     // 资金查询应答
- `on_position_query_ans(const CustPositionInfo *ans_arr, const QueryAnsCtl &ctl)` // 持仓查询应答（数组 + 分页控制）

回调管理器的分发逻辑与其他回报一致（direct 直接调 / queued 入队后 `do_work` 按 type 分发，callback_manager.cpp:404-412）。

### 5.2.4 当前实现状态

> **注意**：`counter98::deal_recv_msg`（counter98.cpp:523）目前只分发 `C98_MSG_AGW_LOGIN_ANS` / `C98_MSG_ACC_LOGIN_ANS` / `C98_MSG_HEART_ANS` 三类，**查询应答（fund/position 等）尚未接入分发**（`default` 分支直接跳过）。即查询请求已能发出，但**应答回调和查询消息构建多为 todo / 待正式协议重写**，属于未完成部分。

## 5.3 各类消息链路总览

| 消息类型 | 入口（api_impl） | 极速柜台 | 98 柜台 | 回调出口 |
|---------|-----------------|---------|---------|---------|
| **登录** | `login` | ✅ build_login_msg | ✅ 统一入口级联 | `on_login` |
| **委托** | `order_insert` | ✅ 优先 + 降级 | ✅ 降级目标 | `on_order_rtn` |
| **撤单** | `order_cancel` | ✅ 优先 + 降级 | ✅ 降级目标 | `on_cancel_rsp` |
| **委托查询** | `order_query` | ❌ 无 | ✅ 唯一 | （未接） |
| **成交查询** | `trade_query` | ❌ 无 | ✅ 唯一 | （未接） |
| **资金查询** | `fund_query` | ❌ 无 | ✅ 唯一 | `on_fund_query_ans` |
| **持仓查询** | `position_query` | ❌ 无 | ✅ 唯一 | `on_position_query_ans` |

> 设计要点：**下单类（委托/撤单）走极速柜台追求低延迟，失败降级 98；查询类因极速柜台不提供，统一走 98 柜台**。回报回调统一收敛到 `callback_manager` 的 `on_*` 接口，对客户暴露单一 `api_callback` 虚接口。

---

# 六、待完成任务清单

> 基于代码中 `// todo` 标记 + 框架逻辑推断，梳理 gw_counter_direct、counter98、fpga 各柜台及整体框架的未完成任务。

## 6.1 gw_counter_direct（个微柜台）

个微柜台当前处于**协议骨架已搭好、业务消息体全部留空**的阶段。

### 6.1.1 业务消息构建（全部留空）

| 函数 | 位置 | 当前实现 | 需要完成 |
|------|------|---------|---------|
| `build_order_msg` | gw_counter_direct.cpp:154 | `(void)req; (void)o_buf;` | 依据正式个微协议，构造 g1_msg_head + 个微委托消息体 |
| `build_etf_order_msg` | gw_counter_direct.cpp:161 | `(void)req; (void)o_buf;` | 同上，ETF 申购赎回 |
| `build_cancel_msg` | gw_counter_direct.cpp:168 | `(void)req; (void)o_buf;` | 同上，撤单 |

### 6.1.2 请求处理函数（消息体占位）

- **`deal_order_req`**（gw_counter_direct.cpp:53）：`take_len = sizeof(g1_msg_head)` 仅占位，需要依据正式协议计算消息体长度、填充 `build_order_msg`
- **`deal_etf_order_req`**（gw_counter_direct.cpp:86）：同上
- **`deal_cancel_req`**（gw_counter_direct.cpp:119）：同上

### 6.1.3 回报消息分发（全部跳过）

`deal_recv_msg`（gw_counter_direct.cpp:305）当前只实现了 `G1_MSG_LOGIN_ANS` 和 `G1_MSG_HEART_ANS`，以下回报类型**全部 default 跳过**：

| msg_id | 对应回报 | 需要完成 |
|--------|---------|---------|
| `G1_MSG_ORDER_RTN` | 委托回报 | 解析个微协议委托回报 → `cb_mgr_->on_order_rtn` |
| `G1_MSG_TRADE_RTN` | 成交回报 | 解析个微协议成交回报 → `cb_mgr_->on_trade_rtn` |
| `G1_MSG_CANCEL_RSP` | 撤单回报 | 解析个微协议撤单回报 → `cb_mgr_->on_cancel_rsp` |
| 其他 | 未知消息 | 待补充 |

### 6.1.4 回报解析辅助函数（留空）

| 函数 | 位置 | 当前实现 |
|------|------|---------|
| `build_api_order_rej` | gw_counter_direct.cpp:369 | 只填了 market_type/err_code/rtn_type，`(void)msg` 忽略原始消息 |
| `build_api_cancel_rej` | gw_counter_direct.cpp:382 | 同上 |
| `deal_send_error` | gw_counter_direct.cpp:394 | 基于 g1 头简单分发，回报体构建调用上述留空函数 |

### 6.1.5 链接状态管理（待完善）

| 问题 | 位置 | 当前行为 | 需要完成 |
|------|------|---------|---------|
| 链接建立后是否重新登录 | gw_counter_direct.cpp:449 | 空 | 建立链接后判断 login_state，若为 1（登录中）则等待，若为 0 则重新发起登录 |
| 链接断开时登录状态重置 | gw_counter_direct.cpp:460 | `// login_state = 0;` 被注释 | 断线后重置 login_state，触发自动重登 |

### 6.1.6 缓存结构

`gw_counter_direct.h:173`：`//todo : 定义个微柜台缓存结构，添加缓存对象` — 需要定义个微柜台特有的缓存（如会话信息、客户信息等）。

## 6.2 counter98（98 柜台）

98 柜台当前处于**请求入口骨架已搭好、所有消息构建和应答处理全部留空**的阶段。

### 6.2.1 业务消息构建（全部留空）

| 函数 | 位置 | 当前实现 |
|------|------|---------|
| `build_order_msg` | counter98.cpp:411 | `(void)req; (void)o_buf;` |
| `build_etf_order_msg` | counter98.cpp:418 | `(void)req; (void)o_buf;` |
| `build_bse_order_msg` | counter98.cpp:425 | `(void)req; (void)o_buf;` |
| `build_cancel_msg` | counter98.cpp:432 | `(void)req; (void)o_buf;` |
| `build_order_query_msg` | counter98.cpp:439 | `(void)req; (void)o_buf;` |
| `build_order_batch_query_msg` | counter98.cpp:446 | `(void)req; (void)o_buf;` |
| `build_trade_query_msg` | counter98.cpp:453 | `(void)req; (void)o_buf;` |
| `build_trade_batch_query_msg` | counter98.cpp:460 | `(void)req; (void)o_buf;` |
| `build_fund_query_msg` | counter98.cpp:467 | `(void)req; (void)o_buf;` |
| `build_position_query_msg` | counter98.cpp:474 | `(void)req; (void)o_buf;` |
| `build_agw_login_msg` | counter98.cpp:611 | 临时用 c98_msg_head_tmp + c98_agw_login_req，`// todo : 依据正式协议重写` |
| `build_login_msg`（账户登录） | counter98.cpp:726 | 临时用 c98_msg_head_tmp + c98_acc_login_req，`// todo : 依据正式协议重写` |

### 6.2.2 请求处理函数（检查逻辑待完善）

所有 `deal_xxx` 函数的检查逻辑当前都是临时实现：

```cpp
// todo : 需要依据正式缓存和柜台规则，重新实现检查
if (agw_login_state != 2) { ... return NOT_LOG_AGW; }
```

涉及的函数：`deal_order_req`、`deal_etf_order_req`、`deal_bse_order_req`、`deal_cancel_req`、`deal_order_query`、`deal_order_batch_query`、`deal_trade_query`、`deal_fund_query`、`deal_position_query`。

### 6.2.3 消息分发与应答处理（大量留空）

**`deal_recv_msg`**（counter98.cpp:502）当前只分发：

| msg_id | 处理函数 | 状态 |
|--------|---------|------|
| `C98_MSG_AGW_LOGIN_ANS` | `deal_agwuser_login_ans` | 临时实现，`// todo : 依据正式协议重写` |
| `C98_MSG_ACC_LOGIN_ANS` | `deal_cust_login_ans` | 临时实现，`// todo : 依据正式协议重写` |
| `C98_MSG_HEART_ANS` | `deal_heart_msg_ans` | 已实现 |
| 其他所有消息 | `default` 跳过 | **未实现** |

**需要补充的应答处理**：

| 消息类型 | 需要做的事 |
|---------|-----------|
| 委托回报（ORDER_RTN） | 解析 c98 委托回报 → `cb_mgr_->on_order_rtn` |
| 成交回报（TRADE_RTN） | 解析 c98 成交回报 → `cb_mgr_->on_trade_rtn` |
| 撤单回报（CANCEL_RSP） | 解析 c98 撤单回报 → `cb_mgr_->on_cancel_rsp` |
| 委托查询应答（ORDER_QUERY_ANS） | 解析 → `on_order_query_ans`（接口待确认） |
| 成交查询应答（TRADE_QUERY_ANS） | 解析 → 客户回调 |
| 资金查询应答（FUND_QUERY_ANS） | 解析 → `cb_mgr_->on_fund_query_ans` |
| 持仓查询应答（POSITION_QUERY_ANS） | 解析 → `cb_mgr_->on_position_query_ans` |

### 6.2.4 登录相关（待完善）

| 问题 | 位置 | 需要完成 |
|------|------|---------|
| `deal_cust_login` 是否真正需要登录 | counter98.cpp:717 | 当前 `cust_need_login = false` 硬编码，需依据正式规则判断 |
| agw 登录超时后处理 | counter98.cpp:604 | 超时后是否需要登出重试 |
| 已登录用户检查 | counter98.cpp:653 | `// todo : 检查已经登陆的用户, 重新发起账户登陆事件` |
| 客户登录状态管理 | counter98.cpp:834/844 | 成功/失败时的状态存储和处理 |
| `build_fast_counter_login_event` | counter98.cpp:853 | 从 c98 登录应答构建极速柜台登录事件，`// todo : 依据正式协议重写` |
| `build_cust_login_rtn` | counter98.cpp:868 | 从 c98 登录应答构建 API 层 LoginAns，`// todo : 依据正式协议重写` |
| 登录过程中的断线处理 | counter98.cpp:934 | `// todo 处理登陆过程中的断线` |

### 6.2.5 其他

| 项目 | 位置 | 说明 |
|------|------|------|
| 缓存结构 | counter98.h:222 | `//todo : 定义 98 柜台缓存结构，添加缓存对象` |
| `deal_send_error` | counter98.cpp:481 | 留空，需要构造 OrderRtn/CancelRsp 回调客户 |
| agw 登录 event 数据 | counter98.cpp:572 | `char *data = nullptr; // TODO -- 赋值登陆信息;` |

## 6.3 fpga 柜台（fpga_counter_base / fpga_counter_direct / fpga_counter_gateway）

fpga 柜台是三个柜台中完成度最高的，但仍有一些待办：

| 问题 | 位置 | 说明 |
|------|------|------|
| `build_login_event` 依据正式协议重写 | fpga_counter_base.cpp:588 | 当前 `o_info.cust_req_no = 0;` |
| 委托回报 order_status 依据字典修正 | fpga_counter_base.cpp:123 | `// todo : 依据委托状态字典变动而修改` |
| 委托回报 rtn_type 依据字典修正 | fpga_counter_base.cpp:138 | `// todo : 依据回报类型字典变动而修改` |
| 地址切换后是否重新登录 | fpga_counter_gateway.cpp:592 | `// todo : 链接地址切换, 是否重新登陆` |

## 6.4 框架层面待完成任务

### 6.4.1 协议实现

| 任务 | 说明 |
|------|------|
| **个微（GW）真实协议** | 当前全部用 g1 消息头 + g1 login_req 临时替代，委托/撤单/回报等消息体全部留空。需要获取正式个微协议文档后实现 |
| **98 真实协议** | 当前用 `c98_msg_head_tmp` + 临时结构体（`c98_agw_login_req` 等）占位，所有 build_xxx_msg 和 deal_recv_msg 分发都是 todo。需要获取正式 98 协议文档后实现 |
| **查询应答分发** | counter98 的 `deal_recv_msg` 未接入任何查询应答（fund/position/trade/order），查询请求已能发出但应答无法处理 |

### 6.4.2 登录与重连

| 任务 | 说明 |
|------|------|
| **登录异常反复重试** | 任务文档要求"登陆任何一个柜台失败，反复登陆直到成功"。当前 `deal_cust_login` 失败后直接返回错误，无重试逻辑 |
| **链接断开后自动重登** | gw_counter_direct 中 `login_state` 重置被注释；fpga 柜台类似。需要统一的断线重登机制 |
| **已登录用户的管理** | counter98 中已有 `// todo : 存储已经成功登陆的用户`，需要正式的用户会话管理 |
| **地址切换后重登** | fpga_gateway 的地址切换 todo |

### 6.4.3 缓存与状态

| 任务 | 说明 |
|------|------|
| **柜台缓存结构定义** | counter98.h 和 gw_counter_direct.h 都有 `//todo` 标记，需要定义各柜台特有的缓存 |
| **状态字典对齐** | fpga 柜台的 order_status / rtn_type 需要与正式字典表对齐 |

### 6.4.4 非加速消息接口

任务文档（`api_dev_task.txt`）提到需要完善非加速消息接口：
- `struct_req.h` 和 `struct_ans.h` 中定义了非加速请求/应答结构体
- 当前框架主要实现了加速接口（order_trade_type.h），非加速接口需要补充

### 6.4.5 配置化

| 任务 | 说明 |
|------|------|
| 配置化选择登录柜台 | 基础架构已支持模板化选择，但具体配置项和逻辑可能待完善 |
| 验密优化 | 任务文档提到"验密优化待定：是否登陆后，先登陆到 AGW 进行验密" |

---

# 七、代码实现方案

> 针对第六章的待办任务给出可落地的实现方案。核心思路：**复用 fpga 柜台（完成度最高）的成熟模式**，gw/counter98 均按"组包 → 入队 → 引擎发送 → 回报分发 → 回调"的统一框架补齐。真实协议未知处先用现有 g1/c98 临时结构体实现，并标注正式协议替换点。

## 7.1 通用实现原则

1. **复用 fpga 模式**：`fpga_counter_base` 的 `build_*_msg` / `deal_*_rtn` / `build_api_*_rej` 是标准范式，gw/counter98 照此实现。
2. **消息构建三步**：填 `msg_head`（msg_id/msg_len/user_id/board_no/session_id）→ 强转 `head+1` 得消息体 → 填充业务字段。
3. **回报解析三步**：校验 `msg_len` → 强转消息体 → 构造 API 层回报 + `StreamInfo` → `cb_mgr_->on_*`。
4. **可靠消息去重**：依据 `session_seq_no`（g1）或对应序号字段做去重，避免重复回报。
5. **未知消息跳过**：不返回错误、不关链接，保证旧 API 兼容新消息。

## 7.2 gw_counter_direct：委托/撤单消息构建

> gw 是单客户，需在类中维护客户信息（如 `gw_cust_info`，参考 fpga 的 `client_info_`）。当前临时用 g1 协议，正式个微协议替换消息体即可。

**实现 `build_order_msg`（替换 gw_counter_direct.cpp:154 的留空实现）**：

```cpp
// 构造个微委托消息 (g1_msg_head + order_req) —— 临时用 g1 协议, 正式协议替换消息体
void gw_counter_direct::build_order_msg(const OrderReq &req, char *o_buf) {
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
  head->msg_id = G1_MSG_ORDER_REQ;
  head->msg_len = sizeof(order_req);
  head->user_id = client_info_.user_id;      // 需新增客户信息结构
  head->board_no = client_info_.board_no;
  head->session_id = client_info_.session_id;

  order_req *body = reinterpret_cast<order_req *>(head + 1);
  body->user_id = client_info_.user_id;
  body->board_no = client_info_.board_no;
  body->sec_index = sec_index;               // 需维护证券代码→索引映射
  body->side = req.side;
  body->order_type = req.order_type;
  body->order_price = req.order_price;
  body->order_qty = req.order_qty;
  body->cust_req_no = req.client_seq_id;
  body->stop_price = req.stop_price;
  body->tgw_id = req.tgw_id;
  body->policy_id = req.policy_id;
  body->order_way[0] = client_info_.order_way_ext[0];
  body->order_way[1] = client_info_.order_way_ext[1];
  body->reserve = 0;
}
```

**同时更新 `deal_order_req`（gw_counter_direct.cpp:53）**，把 `take_len` 从 `sizeof(g1_msg_head)` 改为完整消息长度：

```cpp
// todo 替换为: 依据正式个微协议计算
int32 take_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(order_req));
char *data = nullptr;
int64 pos = trade_send_queue_->write_get_mth(data, take_len + sizeof(link_send_event));
if (unlikely(pos <= 0)) { return LBAPI_ERR_SEND_QUEUE_FULL; }

link_send_event *evt = reinterpret_cast<link_send_event *>(data);
evt->link_type = LINK_TYPE_SPEED_TRADE;
evt->type = LINK_EVENT_TYPE_SEND_MSG;
evt->data_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(order_req));

build_order_msg(req, evt->data);
trade_send_queue_->write_cmt_mth(pos, take_len + sizeof(link_send_event));
return LBAPI_OK;
```

**`build_cancel_msg`（gw_counter_direct.cpp:168）同理**：

```cpp
void gw_counter_direct::build_cancel_msg(const CancelReq &req, char *o_buf) {
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
  head->msg_id = G1_MSG_CANCEL_REQ;
  head->msg_len = sizeof(cancel_req);
  head->user_id = client_info_.user_id;
  head->board_no = client_info_.board_no;
  head->session_id = client_info_.session_id;

  cancel_req *body = reinterpret_cast<cancel_req *>(head + 1);
  body->cust_req_no = req.client_req_no;
  body->order_sys_no = req.order_sys_no;
  body->user_id = client_info_.user_id;
  body->board_no = client_info_.board_no;
  body->reserve = 0;
  body->org_cust_req_no = req.client_seq_id;
}
```

## 7.3 gw_counter_direct：回报解析与分发

**在 `deal_recv_msg`（gw_counter_direct.cpp:305）中补齐回报分发**：

```cpp
case G1_MSG_ORDER_RTN: {
  deal_order_rtn(head, client_info_);        // 新增, 见下
  break;
}
case G1_MSG_TRADE_RTN: {
  deal_trade_rtn(head, client_info_);        // 新增
  break;
}
case G1_MSG_CANCEL_RSP: {
  deal_cancel_rsp(head, client_info_);       // 新增
  break;
}
```

**实现 `deal_order_rtn`（复用 fpga 模式）**：

```cpp
void gw_counter_direct::deal_order_rtn(const g1_msg_head *msg, const gw_cust_info &cust) {
  if (unlikely(msg->msg_len < sizeof(order_rtn))) { error_log(...); return; }
  const order_rtn *rtn = reinterpret_cast<const order_rtn *>(msg + 1);

  // 可靠消息去重
  if (rtn->session_seq_no > 0) {
    if (unlikely(rtn->session_seq_no <= session_seq_)) return;
    session_seq_ = rtn->session_seq_no;
  }

  StreamInfo si_out;
  si_out.counter_type = get_counter_type();
  si_out.stream_seq = rtn->session_seq_no;

  alignas(32) OrderRtn out;
  std::memcpy(out.cust_id.data(), cust.cust_id, sizeof(out.cust_id));
  std::memcpy(out.fund_account_id.data(), cust.fund_account_id, sizeof(out.fund_account_id));
  std::memcpy(out.account_id.data(), cust.holder_acc, sizeof(out.account_id));
  std::memcpy(out.branch_id.data(), cust.branch_id, sizeof(out.branch_id));
  out.market_type = market_type;
  out.order_sys_no = rtn->order_sys_no;
  out.client_seq_id = rtn->cust_req_no;
  out.order_price = rtn->order_price;
  out.order_qty = rtn->order_qty;
  out.order_status = static_cast<uint16>(rtn->order_status);
  out.rtn_type = static_cast<uint16>(rtn->rtn_type);
  out.err_code = rtn->err_code;
  // sec_index -> security_id
  if (rtn->sec_index < secs_.size()) {
    std::memcpy(out.security_id.data(), secs_[rtn->sec_index].security_id, sizeof(out.security_id));
  }

  cb_mgr_->on_order_rtn(si_out, out);        // 统一回调出口
}
```

> `deal_trade_rtn` / `deal_cancel_rsp` 结构完全一致，只是解析 `trade_rtn` / `cancel_rsp` 消息体并调用 `cb_mgr_->on_trade_rtn` / `on_cancel_rsp`。可完全照搬 `fpga_counter_base.cpp:234/292`。

## 7.4 counter98：消息构建（委托/撤单/查询）

> counter98 所有 `build_*_msg` 留空。以委托为例，基于临时 `c98_msg_head_tmp + c98_order_req` 实现，正式协议替换即可：

```cpp
void counter98::build_order_msg(const OrderReq &req, char *o_buf) {
  c98_msg_head_tmp *head = reinterpret_cast<c98_msg_head_tmp *>(o_buf);
  head->msg_id = C98_MSG_ORDER_REQ;
  head->msg_len = sizeof(c98_order_req);
  head->seq_no = next_seq_no_++;            // 需维护自增序号

  c98_order_req *body = reinterpret_cast<c98_order_req *>(o_buf + sizeof(c98_msg_head_tmp));
  std::memset(body, 0, sizeof(*body));
  body->client_req_no = req.client_seq_id;
  std::memcpy(body->fund_account_id, req.fund_account_id.data(), sizeof(body->fund_account_id));
  std::memcpy(body->branch_id, req.branch_id.data(), sizeof(body->branch_id));
  std::memcpy(body->security_id, req.security_id.data(), sizeof(body->security_id));
  body->side = req.side;
  body->order_type = req.order_type;
  body->order_price = req.order_price;
  body->order_qty = req.order_qty;
}
```

**撤单**：

```cpp
void counter98::build_cancel_msg(const CancelReq &req, char *o_buf) {
  c98_msg_head_tmp *head = reinterpret_cast<c98_msg_head_tmp *>(o_buf);
  head->msg_id = C98_MSG_CANCEL_REQ;
  head->msg_len = sizeof(c98_cancel_req);
  head->seq_no = next_seq_no_++;

  c98_cancel_req *body = reinterpret_cast<c98_cancel_req *>(o_buf + sizeof(c98_msg_head_tmp));
  std::memset(body, 0, sizeof(*body));
  body->client_req_no = req.client_req_no;
  body->order_sys_no = req.order_sys_no;
}
```

**查询**（四类结构一致）：

```cpp
void counter98::build_fund_query_msg(const FundQueryReq &req, char *o_buf) {
  c98_msg_head_tmp *head = reinterpret_cast<c98_msg_head_tmp *>(o_buf);
  head->msg_id = C98_MSG_FUND_QUERY_REQ;
  head->msg_len = sizeof(c98_query_req);
  head->seq_no = next_seq_no_++;

  c98_query_req *body = reinterpret_cast<c98_query_req *>(o_buf + sizeof(c98_msg_head_tmp));
  std::memset(body, 0, sizeof(*body));
  body->client_req_no = req.client_req_no;
  std::memcpy(body->fund_account_id, req.fund_account_id.data(), sizeof(body->fund_account_id));
  std::memcpy(body->branch_id, req.branch_id.data(), sizeof(body->branch_id));
}
```

## 7.5 counter98：查询应答分发

**在 `deal_recv_msg` 的 switch（counter98.cpp:523）中新增查询应答 case**：

```cpp
case C98_MSG_FUND_QUERY_ANS: {
  deal_fund_query_ans(head);                // 新增
  break;
}
case C98_MSG_POSITION_QUERY_ANS: {
  deal_position_query_ans(head);            // 新增
  break;
}
case C98_MSG_TRADE_QUERY_ANS: {
  deal_trade_query_ans(head);               // 新增
  break;
}
case C98_MSG_ORDER_QUERY_ANS: {
  deal_order_query_ans(head);               // 新增
  break;
}
```

**实现资金查询应答（参考 fpga 回报解析 + 回调）**：

```cpp
void counter98::deal_fund_query_ans(const c98_msg_head_tmp *msg) {
  if (unlikely(msg->msg_len < sizeof(c98_fund_query_ans))) { error_log(...); return; }
  const c98_fund_query_ans *ans = reinterpret_cast<const c98_fund_query_ans *>(msg + 1);

  CustFundInfo info;
  std::memset(&info, 0, sizeof(info));
  info.total_asset = ans->total_asset;
  info.available = ans->available;
  info.freeze = ans->freeze;
  // ... 其余字段按正式协议填充

  cb_mgr_->on_fund_query_ans(info);         // 统一回调出口
}
```

**持仓查询应答（数组 + 分页控制）**：

```cpp
void counter98::deal_position_query_ans(const c98_msg_head_tmp *msg) {
  // 依据正式协议解析多条持仓
  const c98_position_query_ans *ans = reinterpret_cast<const c98_position_query_ans *>(msg + 1);

  alignas(32) CustPositionInfo arr[C98_POSITION_MAX];
  int32 cnt = 0;
  for (int32 i = 0; i < ans->count && i < C98_POSITION_MAX; ++i) {
    // 逐条填充 arr[cnt++]
  }

  QueryAnsCtl ctl;
  ctl.total = ans->total;
  ctl.cur = ans->cur_page;
  ctl.total_page = ans->total_page;

  cb_mgr_->on_position_query_ans(arr, ctl);
}
```

## 7.6 断线重连与登录重试

### 7.6.1 断线自动重登

**gw_counter_direct**（当前 `deal_link_close` 中 `login_state = 0` 被注释）：

```cpp
void gw_counter_direct::deal_link_close(int16 link_type) {
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    trade_link_connect_ = 0;
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(get_counter_type(), 0, 0);
    }
    // ★ 取消注释: 断线重置登录状态, 触发自动重登
    if (login_state == 2) {
      login_state = 0;
      // 投递重登事件到引擎队列, 由引擎线程重建链接并重登
      // 参考 delive_fast_counter_login 的入队方式
    }
  }
}
```

**`deal_link_connect` 中的重登判断**：

```cpp
int32 gw_counter_direct::deal_link_connect(int16 link_type, int32 have_switch) {
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    trade_link_connect_ = 1;
    if (cb_mgr_ != nullptr) {
      cb_mgr_->on_link_status(get_counter_type(), 0, 1);
    }
    // ★ 链接已建立但尚未登录 → 重新发起登录
    if (login_state == 0) {
      // 构造 acc_login_event_info 投递 ACCOUNT_LOGIN 事件
      // 复用登录链路(第二章 2.3)
    }
  }
  return 0;
}
```

### 7.6.2 登录异常反复重试

在 `deal_log_ans` 失败分支加入重试（带次数限制，避免死循环）：

```cpp
void gw_counter_direct::deal_log_ans(const login_ans &ans) {
  if (ans.err_code == 0) {
    login_state = 2;
    cb_mgr_->on_login(build_login_rtn(ans));
  } else {
    // ★ 登录失败重试 (任务要求: 反复登陆直到成功)
    if (login_retry_cnt_ < MAX_LOGIN_RETRY) {
      login_retry_cnt_++;
      // 投递 ACCOUNT_LOGIN 事件, 重新发起登录
      // 可加退避延时 (如 100ms * retry_cnt)
    } else {
      login_state = 0;
      cb_mgr_->on_error(err_event_type::cust_login, ans.err_code, "login fail after retry");
    }
  }
}
```

## 7.7 实现优先级建议

| 优先级 | 任务 | 说明 |
|--------|------|------|
| **P0** | gw 委托/撤单消息构建 + 入队长度修正 | 否则 gw 无法真正下单，只发空头 |
| **P0** | gw 回报分发（order/trade/cancel） | 否则 gw 收不到任何业务回报 |
| **P1** | counter98 消息构建（委托/撤单/查询） | 98 是查询唯一来源，也是降级目标 |
| **P1** | counter98 查询应答分发 | 否则查询请求发出后无应答 |
| **P1** | 断线自动重登 + 登录状态管理 | 影响稳定性 |
| **P2** | 登录异常反复重试 | 任务明确要求 |
| **P2** | 缓存结构定义 | 支撑状态管理 |
| **P3** | 非加速消息接口 | 待 struct_req/ans 明确 |
| **P3** | 正式协议替换 | 需协议文档，替换消息体即可 |

> **关键依赖**：gw/counter98 的真实协议文档是最大前置条件。当前所有 `build_*_msg` 和回报解析都依赖协议字段定义。**在协议文档未到位前，可先用上述 g1/c98 临时结构体打通全链路（P0/P1），协议到位后只改消息体构建与解析函数，不动框架**。
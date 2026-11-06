# gw_counter_direct 模块接口设计文档

> **目的**：为 `trunk/NewAPI/gone/api/src/gw_counter_direct.cpp/h` 模块补齐与 FTE 柜台通信所需的全部功能。
> **协议基础**：FTE TCP 二进制协议（`gw_message::*` 结构体，`#pragma pack(1)`，大端字节序）
> **转换依据**：`task/api_dev/fte_api.md` 第 2-3 章字段映射关系
> **参考代码**：`/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/fte/include/message/gw_head.h`（FTE 协议结构体）
> **当前状态**：gw_counter_direct 使用 g1 协议临时替代，登录/心跳有临时实现，委托/撤单/ETF 全部留空

---

## 目录

1. [模块定位与架构](#1-模块定位与架构)
2. [FTE 协议总览](#2-fte-协议总览)
3. [全局会话缓存（GwSessionCache）](#3-全局会话缓存gwsessioncache)
4. [消息转换总纲](#4-消息转换总纲)
5. [登录链路（LoginReq ↔ LogOnReq ↔ LogOnAns ↔ LoginAns）](#5-登录链路)
6. [委托链路（OrderReq ↔ TradeOrderReq ↔ TradeOrderER ↔ OrderRtn）](#6-委托链路)
7. [撤单链路（CancelReq ↔ CancelOrderReq ↔ TradeOrderER ↔ CancelRsp）](#7-撤单链路)
8. [ETF 委托链路](#8-etf-委托链路)
9. [成交回报链路（TradeOrderER → TradeRtn）](#9-成交回报链路)
10. [心跳链路](#10-心跳链路)
11. [错误处理（RejectMsg + deal_send_error）](#11-错误处理)
12. [链接状态与登录状态管理](#12-链接状态与登录状态管理)
13. [改动清单](#13-改动清单)
14. [实现优先级与依赖](#14-实现优先级与依赖)

---

## 1. 模块定位与架构

### 1.1 在 NewAPI 框架中的位置

```
api_impl<gw_counter_direct, single_socket_engine<gw_counter_direct>>
  │
  ├─ fast_ = gw_counter_direct       ← 本模块，无线程的协议处理对象
  ├─ fast_engine_ = single_socket    ← 业务引擎，管理 TCP 链接
  └─ multi_engine_                   ← 控制平面，管理登录/心跳/重连
```

**gw_counter_direct 职责**：
- 将 `lb_api::*`（NewAPI 数据结构）转换为 `gw_message::*`（FTE 协议结构体）
- 将 `gw_message::*`（FTE 回报）转换回 `lb_api::*`（NewAPI 回调数据结构）
- 管理登录状态机 + 链接状态
- 维护会话缓存（`GwSessionCache`）

### 1.2 与 FTE 的 TCP 通信层级

```
NewAPI 用户层 (api_interface)
        ↓ lb_api::* 结构体
gw_counter_direct (本模块)
        ↓ gw_message::* 结构体 + encode() 序列化
single_socket_engine::send_queue_ (无锁队列)
        ↓ link_send_event 事件
aio_socket_link → TCP → FTE 服务端
        ↓
FTE 服务端处理 → 回报 → TCP
        ↓
aio_socket_link::msg_cb::deal_msg
        ↓ 字节流
gw_counter_direct::deal_recv_msg (本模块)
        ↓ 反序列化 + 字段转换
callback_manager::on_*()
        ↓
用户回调
```

---

## 2. FTE 协议总览

### 2.1 报文格式

```
┌─────────────────┬──────────────────────────────┬──────────────┐
│ PktNewHeader 8B │  消息体 msg_len B            │ 校验和 uint32│
│ msg_id | msg_len│  (gw_message::* encode 产物)   │  (大端)      │
└─────────────────┴──────────────────────────────┴──────────────┘
whole_msg_len = sizeof(PktNewHeader) + msg_len + sizeof(uint32_t)
```

- **字节序**：多字节整型（≥2 字节）按大端（网络序）传输
- **校验和**：`GenerateSzCheckSum` 逐字节求和 %256，转大端追加 4 字节
- **消息类型**：请求 1xxx / 回报 2xxx / 心跳 3 / 拒绝 9

### 2.2 gw_counter_direct 涉及的消息类型

| 方向 | 消息号 | 结构体 | 业务 | 当前状态 |
|------|--------|--------|------|---------|
| 请求→FTE | 1001 | `gw_message::LogOnReq` | 登录 | 临时实现（g1 头） |
| 请求→FTE | 1003 | `gw_message::TradeOrderReq` | 现货委托 | ❌ 留空 |
| 请求→FTE | 1004 | `gw_message::CancelOrderReq` | 撤单 | ❌ 留空 |
| 请求→FTE | 1010 | `gw_message::TradeOrderReq` | ETF 申赎 | ❌ 留空 |
| 回报←FTE | 2001 | `gw_message::LogOnAns` | 登录应答 | 临时实现（g1） |
| 回报←FTE | 2003 | `gw_message::TradeOrderER` | 委托回报 | ❌ 留空 |
| 回报←FTE | 2004 | `gw_message::TradeOrderER` | 撤单回报 | ❌ 留空 |
| 回报←FTE | 2005 | `gw_message::TradeOrderER` | 成交回报 | ❌ 留空 |
| 回报←FTE | 2010 | `gw_message::TradeOrderER` | ETF 成交回报 | ❌ 留空 |
| 回报←FTE | 9 | `gw_message::RejectMsg` | 拒绝回报 | ❌ 未处理 |
| 心跳 | 3 | 空包体 | 心跳 | 临时实现（g1） |

### 2.3 关键结构体尺寸

| 结构体 | sizeof | 说明 |
|--------|--------|------|
| `gw_message::PktNewHeader` | 8 | msg_id(uint32) + msg_len(uint32) |
| `gw_message::LogOnReq` | 1230 | TradeOrderUser(78) + heart_bt_int(4) + password(100) + feature_code(1024) + agw_user(32) |
| `gw_message::LogOnAns` | 86 | TradeOrderUser(78) + session_status(4) + error_code(4) |
| `gw_message::TradeOrderReq` | 106 | TradeOrderUser(78) + TradeOrderInfo(28) |
| `gw_message::CancelOrderReq` | 94 | TradeOrderUser(78) + CancelOrderInfo(16) |
| `gw_message::TradeOrderER` | 324 | TradeOrderUser(78) + OrdERInfo(246) + constituent_stock[0] |
| `gw_message::RejectMsg` | 83 | TradeOrderUser(78) + reject_reason_code(2) + cancel_flag(1) + business_type(1) |
| `gw_message::ConstituentStock` | 52 | 成分券信息 |

> **注意**：TradeOrderER 固定部分 sizeof=324，含成分券时总长 = 324 + 52 * no_security

---

## 3. 全局会话缓存（GwSessionCache）

### 3.1 设计

在登录成功后，创建一个全局单例的会话缓存，以 `fund_account_id` 为主键，存储客户信息。

> **重要修正**：登录请求在 NewAPI 内部实际以 `acc_login_event_info`（`api_event_msg.h`）传递，**不是** `LoginReq`。会话缓存的创建应基于 `acc_login_event_info`（在 `deal_cust_login` 时），`cust_id`/`account_id` 在收到 `LogOnAns` 时回填。

```cpp
/// 客户会话信息
struct GwSessionInfo {
  std::array<char, 16> fund_account_id;  // 资金账号（主键）
  std::array<char, 16> cust_id;          // 客户号（从 LogOnAns 回填）
  std::array<char, 12> account_id;       // 股东账户（从 LogOnAns 回填）
  std::array<char, 10> branch_id;        // 分支机构代码（从 acc_login_event_info）
  std::array<char, 2> order_way_ext;     // 客户委托方式（从 acc_login_event_info）
  std::array<char, 64> user_info;        // 用户私有信息（从 acc_login_event_info）
  int16_t market_type;                   // 市场类型（配置获取，用于 LoginAns）
  
  /// 原单映射表（为撤单提供定位信息）：
  ///   order_sys_no(FTE order_id) → { clordno, client_seq_id }
  ///  - clordno      用于 CancelOrderReq.orig_clordno（FTE 内部定位原单）
  ///  - client_seq_id 用于 CancelOrderReq.orig_client_seq_id（原委托请求号）
  std::unordered_map<int64_t, OrderLocator> order_locators;
};

/// 原单定位信息
struct OrderLocator {
  int64_t clordno;        ///< FTE 内部订单编号（orig_clordno 用）
  int64_t client_seq_id;  ///< 原委托的 client_seq_id（orig_client_seq_id 用）
};

/// 全局会话缓存管理器（单例）
class GwSessionCache {
public:
  static GwSessionCache& instance();
  
  /// 登录请求发出时创建/更新会话（基于 acc_login_event_info）
  void create_session(const acc_login_event_info& login_req);
  
  /// 登录应答到达时回填 cust_id / account_id（基于 LogOnAns）
  void fill_session_from_ans(const gw_message::LogOnAns& ans);
  
  /// 获取会话信息（找不到返回 nullptr）
  GwSessionInfo* get_session(const std::string& fund_account_id);
  
  /// 记录原单定位信息（收到 OrderRtn/TradeRtn 时记录）
  void record_order_locator(const std::string& fund_account_id,
                            int64_t order_sys_no, int64_t clordno, int64_t client_seq_id);
  
  /// 根据 order_sys_no 查找 clordno（找不到返回 0）
  int64_t get_clordno(const std::string& fund_account_id, int64_t order_sys_no);
  
  /// 根据 order_sys_no 查找原委托 client_seq_id（找不到返回 0）
  int64_t get_orig_client_seq_id(const std::string& fund_account_id, int64_t order_sys_no);
  
private:
  std::mutex mutex_;
  std::unordered_map<std::string, GwSessionInfo> sessions_;
};
```

### 3.2 缓存字段来源与用途

| 字段 | 来源 | 用于 |
|------|------|------|
| `fund_account_id` | `acc_login_event_info`（`deal_cust_login` 时） | 主键 |
| `cust_id` | `LogOnAns` 回填（`deal_log_ans` 时） | OrderReq/CancelReq 组包时补充 |
| `account_id` | `LogOnAns` 回填（`deal_log_ans` 时） | OrderReq/CancelReq 组包时补充 |
| `branch_id` | `acc_login_event_info` | 业务请求映射 |
| `order_way_ext` | `acc_login_event_info` | 暂未使用（预留） |
| `user_info` | `acc_login_event_info` | 暂未使用（预留） |
| `market_type` | 配置获取 | `LoginAns.market_type` 填充 |
| `order_sys_no→{clordno, client_seq_id}` | 回报时记录（`deal_order_rtn`/`deal_trade_rtn`） | CancelReq 撤单时 `orig_clordno` + `orig_client_seq_id` 映射 |

---

## 4. 消息转换总纲

### 4.1 转换规则

依据 `fte_api.md` 第 2-3 章：

| 规则 | 说明 |
|------|------|
| **直接映射** | 字段名/类型一致，直接赋值 |
| **选择映射** | 字段名或类型不同，需转换（如 `client_req_no→client_seq_id`） |
| **FTE 有，NewAPI 无** | 丢弃（请求方向）或由 API 层补充默认值（回报方向） |
| **NewAPI 有，FTE 无** | 回报方向设为默认值（0/空字符串） |
| **缓存补充** | 从 `GwSessionCache` 获取 `account_id`/`cust_id`/`market_type` |

### 4.2 组包通用流程（发送到 FTE）

```cpp
// 1. 计算消息体长度
int32 body_len = sizeof(gw_message::TradeOrderReq); // 或其他结构体
int32 take_len = sizeof(gw_message::PktNewHeader) + body_len + sizeof(uint32_t); // 头+体+校验和

// 2. 申请队列内存（含 link_send_event 头）
char* data = nullptr;
int64 pos = take_req_que_mem(data, take_len);

// 3. 构造消息头
gw_message::PktNewHeader header;
header.msg_id = gw_message::kPktOrderReq;
header.msg_len = body_len;

// 4. 序列化到 data + sizeof(link_send_event) 位置
char* buf = data + sizeof(link_send_event);
size_t off = header.encode(buf, take_len);
// 填充消息体...
// off += body.encode(buf + off, take_len - off);
// 填充校验和...
// uint32_t cks = GenerateSzCheckSum(buf, off);
// ...

// 5. 提交队列
cmt_req_que_mem(pos, take_len);
```

### 4.3 拆包通用流程（从 FTE 接收）

参考 `deal_recv_msg` 现有框架，改为按 FTE 协议解析：

```cpp
int32 gw_counter_direct::deal_recv_msg(const char* buf, int32 len, int16 link_type) {
  int32 deal_len = 0;
  while (len - deal_len >= (int32)sizeof(gw_message::PktNewHeader)) {
    // 1. 解析消息头
    gw_message::PktNewHeader header;
    if (!header.decode(buf + deal_len, len - deal_len)) break;
    
    // 消息长度上限校验（参考 FTE 服务端：msg_len > 65536 拒绝）
    if (header.msg_len > 65536) {
      deal_len += sizeof(gw_message::PktNewHeader); // 跳过非法头
      continue;
    }
    
    int32 whole_msg_len = sizeof(gw_message::PktNewHeader) + header.msg_len + sizeof(uint32_t);
    if (whole_msg_len > len - deal_len) break; // 半包，等待更多数据
    
    // 2. 校验校验和（验证 [头+体] 的校验和）
    uint32_t recv_cks = 0;
    memcpy(&recv_cks, buf + deal_len + sizeof(gw_message::PktNewHeader) + header.msg_len, 4);
    uint32_t calc_cks = GenerateSzCheckSum(buf + deal_len, sizeof(gw_message::PktNewHeader) + header.msg_len);
    if (recv_cks != calc_cks) {
      deal_len += whole_msg_len; // 校验和失败，跳过
      continue;
    }
    
    // 3. 按 msg_id 分发
    const char* body = buf + deal_len + sizeof(gw_message::PktNewHeader);
    switch (header.msg_id) {
      case gw_message::kPktLoginAns:     deal_log_ans(body, header.msg_len); break;
      case gw_message::kPktOrderAns:     deal_order_rtn(body, header.msg_len); break;
      case gw_message::kPktCancelOrderAns: deal_cancel_rsp(body, header.msg_len); break;
      case gw_message::kPktOrderMatch:   deal_trade_rtn(body, header.msg_len); break;
      case gw_message::kPktEtfOrderMatch: deal_etf_trade_rtn(body, header.msg_len); break;
      case gw_message::kPktRejectMsg:    deal_reject_msg(body, header.msg_len); break;
      case gw_message::kPktNewHeartBeat:
        // FTE 心跳无消息体，确认心跳（避免 aio_tcp 心跳超时误判）
        if (trade_eng_op_) trade_eng_op_->deal_heart_msg_ans(link_type);
        break;
      default: /* 未知消息，跳过 */ break;
    }
    
    deal_len += whole_msg_len;
  }
  return deal_len;
}
```

---

## 5. 登录链路

### 5.1 请求方向：LoginReq → LogOnReq

**当前**：使用 g1 协议头 + `login_req` 体。  
**改为**：使用 `gw_message::LogOnReq` + `gw_message::PktNewHeader`。

**字段映射**：

| NewAPI (LoginReq) | FTE (LogOnReq) | 映射 | 说明 |
|-------------------|----------------|------|------|
| `client_req_no` | `client_seq_id` | 选择映射 | 请求-应答配对 |
| `cust_id` | `cust_id` | 直接映射 | 从 LoginReq 传入 |
| `fund_account_id` | `fund_account_id` | 直接映射 | |
| `account_id` | `account_id` | 直接映射 | |
| `branch_id` | `branch_id` | 直接映射 | |
| `order_way_ext` | ❌ 无 | 丢弃 | 缓存到 GwSessionCache |
| `password` | `password` | 直接映射 | ⚠️ 截断至 100 字节 |
| `user_info` | ❌ 无 | 丢弃 | 缓存到 GwSessionCache |
| `client_feature_code` | `client_feature_code` | 直接映射 | |
| ❌ | `heart_bt_int` | 补充默认值 | 配置默认值（如 30） |
| ❌ | `agw_user` | 补充空字符串 | 非统一接入 |
| ❌ | `agw_seq_id` | 补充 0 | 非统一接入 |

**组包伪代码**（注意：FTE `LogOnReq` **没有** `log_type` 字段，签名应去掉该参数）：

```cpp
// deal_cust_login: 入口（引擎调用），返回完整 FTE 报文长度（含头+校验和）
// 成功返回报文长度，0-不需重复登录，<0 出错
int32 gw_counter_direct::deal_cust_login(const acc_login_event_info &info, char *o_buf, int32 buf_len) {
  if (login_state == 2) {
    // 已登录：直接回调成功，返回 0 表示不需重复登录
    LoginAns ans = build_login_rtn(info, 0, NULL);
    cb_mgr_->on_login(ans);
    return 0;
  }
  int32 msg_len = static_cast<int32>(sizeof(gw_message::PktNewHeader) + sizeof(gw_message::LogOnReq) + sizeof(uint32_t));
  if (buf_len < msg_len) return LBAPI_ERR_MSG_LEN;
  
  // 先创建会话缓存（存 order_way_ext/user_info 等，供后续业务使用）
  GwSessionCache::instance().create_session(info);
  
  build_login_msg(info, o_buf, buf_len);
  login_state = 1; // 进入登录中
  return msg_len;
}

// build_login_msg: 构造 FTE 登录报文（PktNewHeader + LogOnReq + 校验和）
void gw_counter_direct::build_login_msg(const acc_login_event_info &info, char* o_buf, int32 buf_len) {
  // 构造 PktNewHeader + LogOnReq
  gw_message::PktNewHeader header;
  header.msg_id = gw_message::kPktLoginReq;
  header.msg_len = sizeof(gw_message::LogOnReq);
  
  gw_message::LogOnReq body;
  body.reset();
  
  // TradeOrderUser 字段
  memcpy(body.fund_account_id.data(), info.fund_account_id, sizeof(body.fund_account_id));
  memcpy(body.branch_id.data(), info.branch_id, sizeof(body.branch_id));
  memcpy(body.account_id.data(), info.account_id, sizeof(body.account_id));
  memcpy(body.cust_id.data(), info.cust_id, sizeof(body.cust_id)); // 可留空，由 FTE 回填
  body.client_seq_id = info.cust_req_no;
  body.agw_seq_id = 0;
  
  // LogOnReq 特有字段
  body.heart_bt_int = static_cast<uint32_t>(heart_interval);
  // password: 注意截断到 100 字节（acc_login_event_info.password 为 256）
  size_t pwd_len = strnlen(info.password, sizeof(info.password));
  pwd_len = std::min(pwd_len, sizeof(body.password) - 1);
  memcpy(body.password.data(), info.password, pwd_len);
  // client_feature_code
  memcpy(body.client_feature_code.data(), info.client_feature_code, sizeof(body.client_feature_code));
  // agw_user 填空（非统一接入）
  body.agw_user.fill(' ');
  
  // 序列化到 o_buf（PktNewHeader + LogOnReq）
  size_t off = header.encode(o_buf, buf_len);
  body.encode(o_buf + off, buf_len - off);
  // 追加校验和
  uint32_t calc_cks = GenerateSzCheckSum(o_buf, off);
  uint32_t be_cks = detail::HostToNetwork(calc_cks);
  memcpy(o_buf + off, &be_cks, 4);
}
```

### 5.2 应答方向：LogOnAns → LoginAns

**当前**：使用 g1 `login_ans` 解析。  
**改为**：使用 `gw_message::LogOnAns::decode()`。

**字段映射**：

| FTE (LogOnAns) | NewAPI (LoginAns) | 映射 | 说明 |
|----------------|-------------------|------|------|
| `cust_id` | `cust_id` | 直接映射 | |
| `fund_account_id` | `fund_account_id` | 直接映射 | |
| `account_id` | `account_id` | 直接映射 | |
| `branch_id` | `branch_id` | 直接映射 | |
| `client_seq_id` | `client_req_no` | 选择映射 | |
| `error_code` | `err_code` | 选择映射 | uint32→int32 |
| ❌ | `market_type` | 补充默认值 | 从配置获取 |
| ❌ | `err_msg` | 补充空字符串 | 或从错误码映射表获取 |
| ❌ | `login_time` | 补充当前时间 | |

**关键处理**：

```cpp
void gw_counter_direct::deal_log_ans(const char* body, int32 body_len) {
  gw_message::LogOnAns ans;
  ans.reset();
  if (!ans.decode(body, body_len)) {
    // 解码失败
    return;
  }
  
  // 回填会话缓存（cust_id / account_id 由 FTE 在应答中回填）
  GwSessionCache::instance().fill_session_from_ans(ans);
  
  // build_login_rtn 重载改造：acc_login_event_info 版 + gw_message::LogOnAns 版
  LoginAns login_ans = build_login_rtn(ans);
  
  if (ans.error_code == 0) {
    login_state = 2; // 登录成功
  } else {
    login_state = 0; // 登录失败
  }
  
  cb_mgr_->on_login(login_ans);
}

// ans_cust_login: 登录失败回调（引擎在连接/组包失败时调用），保留现有实现
void gw_counter_direct::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg) {
  login_state = 0;
  LoginAns ans = build_login_rtn(req, err_ret, err_msg);
  cb_mgr_->on_login(ans);
}
```

---

## 6. 委托链路

### 6.1 请求方向：OrderReq → TradeOrderReq

**当前**：`build_order_msg` 留空，`take_len` 仅 `sizeof(g1_msg_head)`。  
**改为**：使用 `gw_message::TradeOrderReq`。

**字段映射**：

| NewAPI (OrderReq) | FTE (TradeOrderReq) | 映射 | 说明 |
|-------------------|---------------------|------|------|
| `fund_account_id` | `fund_account_id` | 直接映射 | |
| `branch_id` | `branch_id` | 直接映射 | |
| ❌ | `account_id` | **缓存补充** | 从 GwSessionCache 获取 |
| ❌ | `cust_id` | **缓存补充** | 从 GwSessionCache 获取 |
| `client_seq_id` | `client_seq_id` | 直接映射 | |
| ❌ | `agw_seq_id` | 补充 0 | |
| `security_id` | `security_id` | 直接映射 | |
| `market_id` | `market_id` | **直接映射** | ✅ `OrderReq` 已有 `market_id`(uint16)，无需缓存补充 |
| `side` | `side` | 直接映射 | |
| `order_type` | `order_type` | 直接映射 | |
| `order_qty` | `order_qty` | **直接映射** | ⚠️ 注意精度：NewAPI 不放大，FTE N15(2) 需确认 |
| `order_price` | `order_price` | 直接映射 | 双方均放大 10000 |
| `stop_price` | `stop_px` | 直接映射 | |
| `policy_id` | ❌ | **丢弃** | ⚠️ API 侧 `TradeOrderReq` 无此字段（fte_api.md §2.2 有，但实际 `gw_head.h` 已移除） |
| `tgw_id` | ❌ | **丢弃** | ⚠️ 同上，实际 `TradeOrderReq` 无此字段 |

> **⚠️ fte_api.md 与 gw_head.h 不一致**：fte_api.md §2.2 的映射表包含 `policy_id`/`tgw_id`，但实际 `trunk/NewAPI/gone/api/include/gw_head.h` 的 `TradeOrderReq`（TradeOrderInfo 展开）只有 `security_id/market_id/side/order_type/order_qty/order_price/stop_px`，**没有** `policy_id`/`tgw_id`。设计文档以实际 `gw_head.h` 为准。

**组包伪代码**：

```cpp
void gw_counter_direct::build_order_msg(const OrderReq &req, char *o_buf) {
  // 获取会话缓存（补充 account_id / cust_id）
  GwSessionInfo* session = GwSessionCache::instance().get_session(
    std::string(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), 16)));
  
  gw_message::PktNewHeader header;
  header.msg_id = gw_message::kPktOrderReq;
  header.msg_len = sizeof(gw_message::TradeOrderReq);
  
  gw_message::TradeOrderReq body;
  body.reset();
  
  // TradeOrderUser 字段
  memcpy(body.fund_account_id.data(), req.fund_account_id.data(), sizeof(body.fund_account_id));
  memcpy(body.branch_id.data(), req.branch_id.data(), sizeof(body.branch_id));
  if (session) {
    memcpy(body.account_id.data(), session->account_id.data(), sizeof(body.account_id));
    memcpy(body.cust_id.data(), session->cust_id.data(), sizeof(body.cust_id));
  }
  body.client_seq_id = req.client_seq_id;
  body.agw_seq_id = 0;
  
  // TradeOrderInfo 字段（注意：无 policy_id/tgw_id，直接丢弃）
  memcpy(body.security_id.data(), req.security_id.data(), sizeof(body.security_id));
  body.market_id = req.market_id;   // 直接映射
  body.side = req.side;
  body.order_type = req.order_type;
  body.order_qty = req.order_qty;
  body.order_price = req.order_price;
  body.stop_px = req.stop_price;
  
  // 序列化（PktNewHeader + TradeOrderReq + 校验和）
  size_t off = header.encode(o_buf, sizeof(header) + sizeof(body) + 4);
  body.encode(o_buf + off, sizeof(body));
  uint32_t calc_cks = GenerateSzCheckSum(o_buf, off);
  uint32_t be_cks = detail::HostToNetwork(calc_cks);
  memcpy(o_buf + off, &be_cks, 4);
}
```

### 6.2 应答方向：TradeOrderER → OrderRtn（exec_type='0'/'8'）

**当前**：`deal_recv_msg` 中 `G1_MSG_ORDER_RTN`/`G1_MSG_TRADE_RTN`/`G1_MSG_CANCEL_RSP` 全部 default 跳过。  
**改为**：按 FTE 协议解析 `gw_message::TradeOrderER`。

**字段映射**（详见 fte_api.md §3.2）：

| FTE (TradeOrderER) | NewAPI (OrderRtn) | 映射 | 说明 |
|--------------------|-------------------|------|------|
| `cust_id` | `cust_id` | 直接映射 | |
| `fund_account_id` | `fund_account_id` | 直接映射 | |
| `account_id` | `account_id` | 直接映射 | |
| `branch_id` | `branch_id` | 直接映射 | |
| `side` | `side` | 直接映射 | |
| `ord_type` | `order_type` | 直接映射 | |
| `ord_status` | `order_status` | **状态字典映射** | 见下方 |
| ❌ | `policy_id` | 设为 0 | |
| `market_id` | `market_type` | 选择映射 | 101→1(上海), 102→2(深圳) |
| `security_id` | `security_id` | 直接映射 | |
| `price` | `order_price` | 直接映射 | |
| `order_qty` | `order_qty` | 直接映射 | |
| `client_seq_id` | `client_seq_id` | 直接映射 | |
| `exec_type` | `rtn_type` | **类型字典映射** | 见下方 |
| `ord_rej_reason`/`code` | `err_code` | 选择映射 | |
| `order_id` | `order_sys_no` | **字符串→int64** | 转换后缓存映射关系 |
| `clordno` | ❌ | **缓存补充** | int64 内部编号，用于撤单 `orig_clordno` 映射（记录到 GwSessionCache） |
| `frozen_trade_value` | `frozen_amount` | 直接映射 | |
| `frozen_fee` | `fee` | 选择映射 | 设 0（新 API fee 为累计费用） |
| `cum_qty` | `trade_qty` | 直接映射 | |
| ❌ | `cancel_qty` | 设为 0 | |
| `transact_time` | `order_time` | 直接映射 | |
| ❌ | `update_time` | 设为 0 | |

> **⚠️ `clordid` vs `clordno`**：`TradeOrderER` 有两个相关字段——`clordid`(char[10], 客户端订单编号字符串) 和 `clordno`(int64, FTE 内部订单编号)。撤单 `CancelOrderReq.orig_clordno` 是 int64，故映射/缓存用 `clordno`(int64)。（fte_api.md §3.2 写的是 `clordid`，实际以 `gw_head.h` 为准用 `clordno`。）

**状态字典映射**：

| FTE ord_status | 含义 | NewAPI order_status |
|---------------|------|-------------------|
| 0 (kNull) | 初始 | ORDER_STATE_ORDER_IDLE (0) |
| 1 (kSended) | 已发送 | ORDER_STATE_ORDER_NEW (2) |
| 2 (kPartiallyFilled) | 部分成交 | ORDER_STATE_DONE_PART (3) |
| 3 (kFilled) | 全部成交 | ORDER_STATE_DONE_FULL (4) |
| 4 (kPendingCancel) | 待撤 | ORDER_STATE_CANCEL_ING (5) |
| 5 (kCancelled) | 已撤销 | ORDER_STATE_CANCEL_ALL (7) |
| 8 (kReject) | 已拒绝 | ORDER_STATE_DISCARD (9) |

**回报类型字典映射**：

| FTE exec_type | 含义 | NewAPI rtn_type |
|--------------|------|----------------|
| '0' (New) | 新订单 | RSP_TYPE_COUNTER_RSP (1) |
| '8' (Reject) | 拒绝 | RSP_TYPE_ORDER_DISCARD (4) |
| '4' (Cancelled) | 已撤销 | RSP_TYPE_CANCEL_RSP (5) |
| 'F' (Trade) | 成交 | RSP_TYPE_ORDER_TRADE (3) |

**关键处理**：

```cpp
void gw_counter_direct::deal_order_rtn(const char* body, int32 body_len) {
  gw_message::TradeOrderER er;
  er.reset();
  if (!er.decode(body, body_len)) return;
  
  // 记录 order_id → {clordno, client_seq_id} 映射（用于撤单）
  std::string fa_id(er.fund_account_id.data(), strnlen(er.fund_account_id.data(), 16));
  GwSessionCache::instance().record_order_locator(fa_id,
    strtoll(er.order_id.data(), nullptr, 10), er.clordno, er.client_seq_id);
  
  // 构造 OrderRtn
  OrderRtn rtn;
  memset(&rtn, 0, sizeof(rtn));
  memcpy(rtn.cust_id.data(), er.cust_id.data(), sizeof(rtn.cust_id));
  memcpy(rtn.fund_account_id.data(), er.fund_account_id.data(), sizeof(rtn.fund_account_id));
  memcpy(rtn.account_id.data(), er.account_id.data(), sizeof(rtn.account_id));
  memcpy(rtn.branch_id.data(), er.branch_id.data(), sizeof(rtn.branch_id));
  rtn.side = er.side;
  rtn.order_type = er.ord_type;
  rtn.order_status = map_ord_status(er.ord_status);
  rtn.market_type = map_market_id(er.market_id);
  memcpy(rtn.security_id.data(), er.security_id.data(), sizeof(rtn.security_id));
  rtn.order_price = er.price;
  rtn.order_qty = er.order_qty;
  rtn.client_seq_id = er.client_seq_id;
  rtn.rtn_type = map_exec_type(er.exec_type);
  rtn.err_code = (er.ord_rej_reason != 0) ? er.ord_rej_reason : er.code;
  rtn.order_sys_no = strtoll(er.order_id.data(), nullptr, 10);
  rtn.frozen_amount = er.frozen_trade_value;
  rtn.fee = 0; // 委托响应中费用为 0
  rtn.trade_qty = er.cum_qty;
  rtn.cancel_qty = 0;
  rtn.order_time = er.transact_time;
  rtn.update_time = er.transact_time;
  
  StreamInfo stream;
  stream.counter_type = get_counter_type();
  stream.stream_seq = ++session_seq_;
  
  cb_mgr_->on_order_rtn(stream, rtn);
}
```

---

## 7. 撤单链路

### 7.1 请求方向：CancelReq → CancelOrderReq

**当前**：`build_cancel_msg` 留空。  
**改为**：使用 `gw_message::CancelOrderReq`。

**关键问题**：NewAPI 使用 `order_sys_no`（柜台委托号）定位原单，但 FTE 使用 `orig_clordno`（客户订单编号）。需要从 `GwSessionCache` 的映射表中查找。

**字段映射**：

| NewAPI (CancelReq) | FTE (CancelOrderReq) | 映射 | 说明 |
|--------------------|----------------------|------|------|
| `fund_account_id` | `fund_account_id` | 直接映射 | |
| `branch_id` | `branch_id` | 直接映射 | |
| ❌ | `account_id` | **缓存补充** | 从 GwSessionCache 获取 |
| ❌ | `cust_id` | **缓存补充** | 从 GwSessionCache 获取 |
| `client_req_no` | `client_seq_id` | 选择映射 | 撤单请求自身的配对号 |
| ❌ | `agw_seq_id` | 补充 0 | |
| `order_sys_no` | `orig_clordno` | **映射查找** | 通过 GwSessionCache 的 order_sys_no→clordno 映射 |
| ❌（原委托 client_seq_id） | `orig_client_seq_id` | **映射查找** | 通过 GwSessionCache 的 order_sys_no→client_seq_id 映射；找不到设 0 |

> **修正说明**：`orig_client_seq_id` 是**原委托**的 `client_seq_id`，不是撤单请求自身的 `client_req_no`。`CancelReq` 只提供 `order_sys_no`，故需从映射表反查原委托的 `client_seq_id`。

### 7.2 应答方向：TradeOrderER → CancelRsp（exec_type='4'）

**字段映射**（详见 fte_api.md §3.3）：

| FTE (TradeOrderER) | NewAPI (CancelRsp) | 映射 |
|--------------------|--------------------|------|
| `cust_id` | `cust_id` | 直接映射 |
| `fund_account_id` | `fund_account_id` | 直接映射 |
| `account_id` | `account_id` | 直接映射 |
| `branch_id` | `branch_id` | 直接映射 |
| `client_seq_id` | `client_req_no` | 选择映射 |
| `market_id` | `market_type` | 选择映射 |
| `order_id` | `order_sys_no` | 字符串→int64 |
| `client_seq_id` | `client_seq_id` | 直接映射 |
| `code`/`ord_rej_reason` | `err_code` | 选择映射 |
| ❌ | `rej_api` | 设为 0 |
| `orig_clordno` | `order_sys_no` | 选择映射 |

---

## 8. ETF 委托链路

### 8.1 请求方向

与普通委托基本一致，区别：
- `header.msg_id = gw_message::kPktETFReq`（1010）
- 复用 `gw_message::TradeOrderReq` 结构体
- `side` 字段：`'D'`=申购, `'E'`=赎回

### 8.2 应答方向

ETF 成交回报（`kPktEtfOrderMatch`, 2010）：
- 消息体 = `TradeOrderER` 固定部分 + `ConstituentStock[no_security]`
- 使用 `TradeOrderER::decode(data, len, no_security, stocks)` 重载解析成分券
- 映射为 `OrderRtn`（与普通委托回报相同），成分券信息暂不展开（如需可增加 ETF 专用结构体）

---

## 9. 成交回报链路

**消息号**：`kPktOrderMatch`（2005）  
**结构体**：`gw_message::TradeOrderER`  
**exec_type**：`'F'`（Trade）

**字段映射**（详见 fte_api.md §3.4）：

| FTE (TradeOrderER) | NewAPI (TradeRtn) | 映射 |
|--------------------|-------------------|------|
| `cust_id`→`fund_account_id`→`account_id`→`branch_id` | 同上 | 直接映射 |
| `side`/`ord_type`/`ord_status`/`market_id`/`security_id` | 同上 | 直接映射 |
| `price`/`order_qty`/`client_seq_id` | `order_price`/`order_qty`/`client_seq_id` | 直接映射 |
| `order_id` | `order_sys_no` | 字符串→int64 |
| `frozen_trade_value` | `frozen_amount` | 直接映射 |
| `frozen_fee` + `fee` | `fee` | 选择映射（合并） |
| `cum_qty` | `trade_qty` | 直接映射 |
| ❌ | `cancel_qty` | 设为 0 |
| `transact_time` | `order_time` | 直接映射 |
| ❌ | `exec_time` | 设为 0 |
| `exec_id` | `exec_id` | char[16]→char[32] |
| `last_px` | `exec_price` | 直接映射 |
| `last_qty` | `exec_qty` | 直接映射 |
| ❌ | `exec_amount` | 设为 0（可用 total_value_traded 映射） |
| `fee` | `exec_fee` | 直接映射 |
| ❌ | `policy_id`/`reserved`/`update_time` | 设为 0 |

---

## 10. 心跳链路

### 10.1 发送心跳

**当前**：使用 g1 头 `G1_MSG_HEART_REQ`。  
**改为**：使用 FTE 心跳格式（空包体，仅 8 字节头 + 4 字节校验和）。

```cpp
int32 gw_counter_direct::build_heart_msg(char *o_buf, int32 buf_len) {
  int32 msg_len = sizeof(gw_message::PktNewHeader) + sizeof(uint32_t); // 头 + 校验和
  if (buf_len < msg_len) return -1;
  
  gw_message::PktNewHeader header;
  header.msg_id = gw_message::kPktNewHeartBeat;
  header.msg_len = 0;
  
  size_t off = header.encode(o_buf, buf_len);
  // 校验和：对 [头] 求和 %256
  uint32_t cks = GenerateSzCheckSum(o_buf, off);
  uint32_t be_cks = detail::HostToNetwork(cks);
  memcpy(o_buf + off, &be_cks, 4);
  
  return msg_len;
}
```

### 10.2 接收心跳

FTE 心跳 `kPktNewHeartBeat(3)` 无消息体，在 `deal_recv_msg` 中识别后：
1. 调用 `trade_eng_op_->deal_heart_msg_ans(link_type)` 确认心跳（**必须**，否则 `aio_tcp` 的心跳超时检测会误判断链）
2. 无需其他处理（FTE 心跳双向，无需回发）

---

## 11. 错误处理

### 11.1 RejectMsg 处理（消息号 9）

当 FTE 校验失败时返回 `RejectMsg`，需解析并映射为对应的 `OrderRtn` 或 `CancelRsp`。

```cpp
void gw_counter_direct::deal_reject_msg(const char* body, int32 body_len) {
  gw_message::RejectMsg rej;
  rej.reset();
  if (!rej.decode(body, body_len)) return;
  
  // 根据 business_type 判断业务类型，分发到不同回调
  StreamInfo stream;
  stream.counter_type = get_counter_type();
  stream.stream_seq = ++session_seq_;
  
  switch (rej.business_type) {
    case 0: // 现货（默认）
    case 1: // ETF
    case 2: // 两融
    default: {
      OrderRtn rtn;
      memset(&rtn, 0, sizeof(rtn));
      memcpy(rtn.cust_id.data(), rej.cust_id.data(), sizeof(rtn.cust_id));
      memcpy(rtn.fund_account_id.data(), rej.fund_account_id.data(), sizeof(rtn.fund_account_id));
      memcpy(rtn.account_id.data(), rej.account_id.data(), sizeof(rtn.account_id));
      memcpy(rtn.branch_id.data(), rej.branch_id.data(), sizeof(rtn.branch_id));
      rtn.client_seq_id = rej.client_seq_id;
      rtn.err_code = rej.reject_reason_code;
      rtn.rtn_type = RSP_TYPE_ORDER_DISCARD;
      rtn.order_status = ORDER_STATE_DISCARD;
      cb_mgr_->on_order_rtn(stream, rtn);
      break;
    }
  }
}
```

### 11.2 deal_send_error 处理

**当前**：按 g1 msg_id 分发。  
**改为**：按 FTE msg_id 分发。

```cpp
void gw_counter_direct::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret) {
  if (msg_buf == nullptr || cb_mgr_ == nullptr || msg_len < (int32)sizeof(gw_message::PktNewHeader)) return;
  
  gw_message::PktNewHeader header;
  if (!header.decode(msg_buf, msg_len)) return;
  
  switch (header.msg_id) {
  case gw_message::kPktOrderReq:
  case gw_message::kPktETFReq: {
    // 解析 TradeOrderReq 获取用户信息
    gw_message::TradeOrderReq req;
    req.reset();
    if (req.decode(msg_buf + sizeof(gw_message::PktNewHeader), header.msg_len)) {
      OrderRtn rtn;
      memset(&rtn, 0, sizeof(rtn));
      memcpy(rtn.fund_account_id.data(), req.fund_account_id.data(), sizeof(rtn.fund_account_id));
      memcpy(rtn.branch_id.data(), req.branch_id.data(), sizeof(rtn.branch_id));
      rtn.client_seq_id = req.client_seq_id;
      rtn.err_code = err_ret;
      rtn.rtn_type = RSP_TYPE_ORDER_DISCARD;
      rtn.order_status = ORDER_STATE_DISCARD;
      StreamInfo stream;
      stream.counter_type = get_counter_type();
      stream.stream_seq = ++session_seq_;
      cb_mgr_->on_order_rtn(stream, rtn);
    }
    break;
  }
  case gw_message::kPktCancelOrderReq: {
    // 解析 CancelOrderReq
    // ...
    break;
  }
  default:
    break;
  }
}
```

---

## 12. 链接状态与登录状态管理

### 12.1 状态机

```
                    deal_link_connect
  ┌─────────┐  ───────────────────▶  ┌──────────┐
  │ 断开态   │                        │ 已链接态  │
  │ link=0  │◀───────────────────    │ link=1   │
  └─────────┘  deal_link_close       └────┬─────┘
                                          │ deal_cust_login → build_login_msg → 发送 1001
                                          ▼
                                    ┌──────────┐
                                    │ 登录中    │  login_state=1
                                    └────┬─────┘
                                         │ deal_log_ans(err_code==0)
                                         ▼
                                    ┌──────────┐
                                    │ 已登录    │  login_state=2 → 可发业务
                                    └──────────┘
```

### 12.2 关键修改点

| 当前问题 | 修改方案 |
|---------|---------|
| `deal_link_close` 中 `login_state = 0` 被注释 | **取消注释**：链接断开时重置 login_state=0 |
| `deal_link_connect` 中 `// todo : 链接建立，是否重新登陆？` | **实现**：login_state!=2 时触发重新登录（由 multi_engine 驱动） |
| `deal_send_error` 中 login_state 未重置 | **实现**：发送失败时重置 login_state=0 |
| 登录失败未重试 | **实现**：由 multi_engine 的定时器驱动重试 |

---

## 13. 改动清单

### 13.1 gw_counter_direct.h 改动

| 改动 | 说明 |
|------|------|
| 新增 `#include "gw_head.h"` | 引入 FTE 协议结构体 |
| 新增 `#include "gw_session_cache.h"` | 引入会话缓存 |
| 新增 `int32_t deal_log_ans(const char* body, int32 body_len)` | FTE 版登录应答处理 |
| 新增 `void deal_order_rtn(const char* body, int32 body_len)` | 委托回报处理 |
| 新增 `void deal_trade_rtn(const char* body, int32 body_len)` | 成交回报处理 |
| 新增 `void deal_cancel_rsp(const char* body, int32 body_len)` | 撤单回报处理 |
| 新增 `void deal_etf_trade_rtn(const char* body, int32 body_len)` | ETF 成交回报处理 |
| 新增 `void deal_reject_msg(const char* body, int32 body_len)` | 拒绝消息处理 |
| 新增 `int32_t map_ord_status(uint8_t fte_status)` | 状态字典映射 |
| 新增 `int32_t map_exec_type(char exec_type)` | 回报类型映射 |
| 新增 `int16_t map_market_id(uint16_t fte_market_id)` | 市场代码映射 |
| 新增 `uint32_t GenerateSzCheckSum(const char* buf, uint32_t len)` | 校验和计算 |
| 修改 `build_order_msg` 签名 | 改为使用 FTE 协议 |
| 修改 `build_cancel_msg` 签名 | 同上 |
| 修改 `build_etf_order_msg` 签名 | 同上 |
| 修改 `build_login_msg` 签名 | 去掉 `log_type` 参数，改用 FTE 协议 |
| 修改 `build_heart_msg` | 改为 FTE 格式 |
| 修改 `deal_recv_msg` | 改为 FTE msg_id 分发 + 长度/校验和校验 + 心跳确认 |
| 修改 `deal_send_error` | 改为 FTE msg_id 分发 |
| 修改 `deal_log_ans` | 改为 FTE 版 + 回填会话缓存 |
| 修改 `build_login_rtn` 重载 | 改为 `acc_login_event_info` 版 + `gw_message::LogOnAns` 版 |
| 保留 `ans_cust_login` | 登录失败回调（引擎调用），仅内部字段映射改用 FTE 返回 |
| 取消注释 `login_state = 0` | 链接断开时重置登录态 |

### 13.2 新增文件

| 文件 | 说明 |
|------|------|
| `gw_session_cache.h/.cpp` | 全局会话缓存管理器 |
| `gw_protocol_utils.h` | FTE 协议工具函数（校验和、组包/拆包辅助） |

### 13.3 gw_counter_direct.cpp 改动

所有 `build_*_msg` 函数从留空实现改为完整的 FTE 协议组包。  
`deal_recv_msg` 从 g1 switch 改为 FTE msg_id switch。  
新增 `deal_order_rtn`/`deal_trade_rtn`/`deal_cancel_rsp`/`deal_reject_msg` 等回报处理函数。

---

## 14. 实现优先级与依赖

### P0（核心功能，打通委托/撤单/回报链路）

| 任务 | 依赖 |
|------|------|
| 1. `build_order_msg` 实现（OrderReq→TradeOrderReq） | GwSessionCache |
| 2. `build_cancel_msg` 实现（CancelReq→CancelOrderReq） | GwSessionCache + 映射表 |
| 3. `deal_recv_msg` 改为 FTE 分发 | 无 |
| 4. `deal_order_rtn` 实现（TradeOrderER→OrderRtn） | 无 |
| 5. `deal_cancel_rsp` 实现（TradeOrderER→CancelRsp） | 无 |
| 6. `deal_trade_rtn` 实现（TradeOrderER→TradeRtn） | 无 |

### P1（登录/会话/心跳）

| 任务 | 依赖 |
|------|------|
| 7. `build_login_msg` 改为 FTE LogOnReq | 无 |
| 8. `deal_log_ans` 改为 FTE LogOnAns | 无 |
| 9. `GwSessionCache` 实现 | 无 |
| 10. `build_heart_msg` 改为 FTE 格式 | 无 |
| 11. 链接断开重置 login_state | 无 |

### P2（错误处理/ETF/完善）

| 任务 | 依赖 |
|------|------|
| 12. `deal_reject_msg` 实现 | P0 完成后 |
| 13. `deal_send_error` 改为 FTE 格式 | P0 完成后 |
| 14. ETF 委托/回报处理 | P0 完成后 |
| 15. 订单映射表维护（order_sys_no→clordno） | P1 完成后 |
| 16. 登录异常重试 | P1 完成后 |

---

## 附录：关键代码索引

| 关注点 | 文件 |
|--------|------|
| FTE 协议结构体 | `/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/fte/include/message/gw_head.h` |
| FTE 消息类型常量 | `gw_head.h` L24-41 |
| FTE 校验和算法 | `src/tech/util.cpp` `GenerateSzCheckSum` L68-90 |
| FTE 组包示例 | `uplink_biz_processor.cpp` `PackageNew` L2707-2723 |
| FTE 拆包示例 | `tcpserver_handler.cpp` `on_message` L39-101 |
| 新 API 数据结构 | `trunk/NewAPI/gone/api/include/order_trade_type.h` |
| 新 API 事件结构 | `trunk/NewAPI/gone/api/src/api_event_msg.h` |
| 字段转换关系 | `task/api_dev/fte_api.md` |
| 当前 gw_counter 实现 | `trunk/NewAPI/gone/api/src/gw_counter_direct.cpp/h` |
| FTE 知识库 | `/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/fte/knowledge_base/README.md` |
| FTE TCP 通信分析 | `/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/fte_tcp_通信链路分析.md` |
| API 客户端示例 | `/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/api_demo/` |
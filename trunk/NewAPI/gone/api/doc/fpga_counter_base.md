# fpga_counter_base 文档

> 源文件：`trunk/NewAPI/gone/api/src/fpga_counter_base.h` + `fpga_counter_base.cpp`
> 作用：FPGA 柜台公共基类，被 fpga_counter_direct（直连，单客户）和 fpga_counter_gateway（网关，多客户）继承。

## 概述

`fpga_counter_base` 是 FPGA 柜台三个实现（direct/gateway）的公共基类，提供公共协议状态、证券代码映射、公共消息构建、公共消息解析等能力。**多态通过模板特化实现（无 virtual 钩子）**，派生类的 `deal_recv_msg` 必须显式调用基类的 protected 方法。

## 公共结构体

### fpga_cust_info（FPGA 客户信息）
```cpp
struct fpga_cust_info {
  // 热路径: 每次委托/撤单必用
  int32_t fpga_state;              // 该客户在 fpga 中状态
  int16_t login_state;             // 登陆状态: 0-未登陆, 1-登陆中, 2-已登陆
  uint16_t user_id;                // 用户索引ID(登录后分配)
  uint16_t board_no;               // 所在 FPGA 编号(登录后分配)
  uint32_t session_id;             // fdm board 映射的会话 ID
  char order_way_ext[2];           // 客户委托方式
  int32_t trade_port;              // 交易接口端口(登录后分配)
  char trade_ip[G1_IPADDR_LEN];    // 交易接口地址IP(登录后分配)
  // 冷路径: 仅登录/管理使用
  char cust_id[G1_CUSTID_LEN];     // 客户号
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 客户资金账号
  char branch_id[G1_BRANCHID_LEN]; // 分支机构代码
  char holder_acc[G1_HOLDERACC_LEN]; // 客户股东账号
  int64_t cust_req_no;             // 客户私有请求号
  char end_code[G1_CUST_END_LEN];  // 终端信息
};
```

### fpga_sec_info / fpga_sec_key / fpga_sec_hash（证券信息与哈希）
- `fpga_sec_info`：证券代码、市场、sec_index、买数量单位。
- `fpga_sec_key`：8 字节定长证券代码，按 64-bit 块复制/比较，避免逐字节。
- `fpga_sec_hash`：按前 8 字节做 64-bit 哈希。

## 类成员

```cpp
int32 trade_link_connect = 0;   // 交易通道是否链接
int32 sec_state_ = 0;           // 证券信息获取状态: 0-未进行, 1-进行中, 2-成功
int16 market_type = 0;          // 市场
int16 heart_interval = 5;       // 心跳间隔
hash_map_mth<1020, fpga_sec_key, uint16_t, fpga_sec_hash> sec_map_; // 证券代码→sec_index
std::vector<fpga_sec_info> secs_;  // 证券信息数组(按 sec_index 索引)
callback_manager *cb_mgr_;      // 回调管理器
int64 session_seq_;             // 会话消息序号
char agw_user[32];
```

## 公共接口

### 对外接口（engine 调用）
```cpp
int32 build_heart_msg(char *o_buf, int32 buf_len);  // 构造心跳消息
int64 get_session_seq_no() const;                   // 原子读取会话序号
```

### 消息构建（公共协议层）
| 方法 | 说明 |
|------|------|
| `build_order_msg(req, cust, sec_index, o_req)` | 构造委托消息（g1_msg_head + order_req） |
| `build_cancel_msg(req, cust, o_req)` | 构造撤单消息 |
| `build_sec_info_req_msg(cust_req_no, o_req)` | 构造证券信息请求 |
| `build_login_msg(info, log_type, o_req)` | 构造登录消息 |
| `build_login_rtn(...)` | 构造登录应答（多个重载） |
| `build_login_event(cust, o_info)` | 构造登录事件 |

### 消息解析（公共协议层）
| 方法 | 说明 |
|------|------|
| `deal_order_rtn(msg, cust, counter_type)` | 解析委托回报 → on_order_rtn |
| `deal_trade_rtn(msg, cust, counter_type)` | 解析成交回报 → on_trade_rtn |
| `deal_cancel_rsp(msg, cust, counter_type)` | 解析撤单回报 → on_cancel_rsp |
| `deal_sec_info_ans(msg)` | 解析证券信息应答 |
| `deal_gw_rej(msg, cust, counter_type)` | 处理网关路由拒绝（G1_MSG_GW_REJ） |

### 拒单构造
| 方法 | 说明 |
|------|------|
| `build_api_order_rej(msg, cust, err_code, o_rtn, o_stream)` | 构造委托拒单响应 |
| `build_api_cancel_rej(msg, cust, err_code, o_rtn, o_stream)` | 构造撤单拒单响应 |
| `build_api_gw_rej_order(body, rej, cust, counter_type, o_rtn, o_stream)` | 从 order_req + rej head 构造 GW 拒单 |
| `build_api_gw_rej_cancel(body, rej, cust, counter_type, o_rsp, o_stream)` | 从 cancel_req + rej head 构造 GW 拒撤单 |

### 登录与客户管理
| 方法 | 说明 |
|------|------|
| `delive_cust_login(cust, que, link_op)` | GW 链接重连后重新投递已登录客户的登录事件 |
| `save_client_info(msg, o_cust)` | 从登录应答保存客户信息 |
| `get_sec_index(security_id, sec_index)` | 查询证券 sec_index |

## 设计要点
1. **模板特化多态**：无 virtual 钩子，派生类显式调用基类 protected 方法，避免虚函数开销。
2. **热路径/冷路径分离**：`fpga_cust_info` 中委托/撤单必用的字段放前面，提升缓存命中。
3. **证券代码哈希映射**：`sec_map_` 实现证券代码 → sec_index 的 O(1) 查询。
4. **统一回调出口**：所有回报解析最终调用 `cb_mgr_->on_*`。
5. 完成度最高的柜台，是 gw/counter98 实现的参考模板。
# fpga_counter_gateway 文档

> 源文件：`trunk/NewAPI/gone/api/src/fpga_counter_gateway.h` + `fpga_counter_gateway.cpp`
> 作用：FPGA 网关模式柜台（多客户），继承 fpga_counter_base。

## 概述

`fpga_counter_gateway` 是 FPGA 网关模式柜台，多客户、所有业务走同一条 fpga_gw 链接。与 `fpga_counter_direct` 的主要差异（D26）：
- 多客户存储（`client_map_` + `clients_`，按 user_id 索引）
- 无 fpga core 链接，委托/撤单/回报全部走 fpga_gw 链接
- 登录类型 log_type=2（网关代多客户登录）
- login_ans 成功后直接 `cb_mgr_->on_login`（无 core 同步 connect 步骤）
- 证券信息获取期间到达的成功 login_ans 先缓存，待 `sec_state_==2` 后统一回调

## 多客户哈希结构

### fpga_fundacc_key（资金账号 + 分支机构哈希键）
```cpp
struct fpga_fundacc_key {
  char fund_account_id[16];  // 客户资金账号
  char branch_id[16];        // 分支机构代码
};
```
按 64-bit 块复制/比较，避免逐字节。

### fpga_fundacc_index（board_no + user_id 索引）
```cpp
struct fpga_fundacc_index {
  uint16 board_no;
  uint16 user_id;
};
```

### 哈希映射类型
```cpp
using fpga_client_map_acc   = hash_map_mth<60, fpga_fundacc_key, fpga_cust_info*, fpga_fundacc_hash>;
using fpga_client_map_index = hash_map_mth<60, fpga_fundacc_index, fpga_cust_info*, fpga_fundacc_index_hash>;
```

## 链接与队列

```cpp
void init_trade(que_mth_buf *que, link_engine_outop *link_outop) {} // 网关无独立 trade 链接，no-op
void init_gateway(que_mth_buf *que, link_engine_outop *link_outop); // 注入 gw 链接队列与回调
```

## 业务接口（api instance 调用）

| 方法 | 说明 |
|------|------|
| `deal_order_req(req)` | 处理委托，业务也走 gw_send_queue_ |
| `deal_etf_order_req(req)` | fpga 不支持，返回错误路由到 98 |
| `deal_cancel_req(req)` | 处理撤单 |

## 引擎/链接接口

| 方法 | 说明 |
|------|------|
| `deal_recv_msg(buf, len, link_type)` | 接收消息处理（网关模式业务也走 SPEED_GW） |
| `deal_send_error(...)` | 处理发送失败 |
| `deal_cust_login(req, o_buf, buf_len)` | 处理账户登录事件 |
| `ans_cust_login(req, err_ret, err_msg)` | 账户登录同步失败回调 |
| `can_link_connect(link_type)` | SPEED_GW 可建链 |
| `deal_link_connect(link_type, have_switch)` | 链接成功回调 |
| `deal_link_close(link_type)` | 链接关闭回调 |

## 派生类内部方法

| 方法 | 说明 |
|------|------|
| `deal_fpag_state(msg)` | 处理 FPGA 用户状态消息 |
| `deal_log_ans(msg)` | 处理登录应答（无 core 链接，按 sec_state_ 决定是否直接 on_login） |
| `check_ans_log(err_code)` | 证券信息获取完成/失败后，对缓存的 login_ans 统一处理 |
| `get_client_info(o_info, branch_id, fund_account_id)` | 查询单个客户信息（查 client_map_） |

## 关键成员

```cpp
que_mth_buf *gw_send_queue_;        // gw 链接发送队列
link_engine_outop *gw_eng_op_;      // gw 链接引擎操作
fpga_client_map_acc client_map_;    // 资金账号+分支机构 → user_id+board_no（发送查询，needlock=0）
fpga_client_map_index clients_;     // boardno+userid → fpga_cust_info 指针（接收查询，needlock=1）
std::vector<fpga_cust_info*> clients_vec_; // 仅用于回收
std::vector<login_ans> login_cache_;        // 缓存登录成功但证券信息未就绪的 login_ans
```

## 登录流程（网关模式）

```
GW 登录应答 → deal_log_ans
  → 若 sec_state_==2：直接 cb_mgr_->on_login
  → 若证券信息获取中：缓存到 login_cache_
  → 证券信息完成 → check_ans_log → 统一回调并清空缓存
```

## 设计要点
1. **多客户**：通过两个哈希映射（fundacc→index、index→cust_info）实现双索引查询。
2. **单链接**：所有业务走同一条 fpga_gw 链接，无独立 core 链接。
3. **登录缓存**：证券信息未就绪时缓存 login_ans，就绪后统一回调。
4. **委托降级**：客户状态异常或 ETF 不支持时返回错误，上层路由到 98。
# counter98 文档

> 源文件：`trunk/NewAPI/gone/api/src/counter98.h` + `counter98.cpp`
> 作用：98 柜台协议对象，负责 98 柜台的组包、解析、状态管理、查询应答处理。

## 概述

`counter98` 是 98 柜台协议对象。柜台对外只提供业务 API 接口对应的发送函数，不暴露 build 消息函数。业务发送函数内部完成**组包 + 入队**（`que_mth_buf::write_get_mth`）。

## 链接与队列

通过 `init_trade` / `init_gateway` 注入引擎的发送队列和操作接口：

```cpp
void init_trade(que_mth_buf *que, link_engine_outop *link_outop);   // 极速柜台发送队列
void init_gateway(que_mth_buf *que, link_engine_outop *link_outop); // 98/网关发送队列
```

## 状态管理

```cpp
int16 trade_link_connect_ = 0;  // 98 链接状态: 0-未链接/断开, 1-已链接
int16 agw_login_state = 0;      // agw 用户登陆状态: 0-未登陆, 1-登陆中, 2-登陆成功
int64 session_seq_ = 0;         // 会话消息序号 (原子访问)
char agw_session[32];           // agw 用户登陆成功返回的会话号
```

## 业务接口（api instance 调用）

| 方法 | 说明 |
|------|------|
| `deal_order_req(req)` | 处理买卖委托 |
| `deal_etf_order_req(req)` | 处理 ETF 申购赎回委托 |
| `deal_bse_order_req(req)` | 处理北交所委托 |
| `deal_cancel_req(req)` | 处理委托撤单 |
| `deal_order_query(req)` | 处理客户委托查询 |
| `deal_order_batch_query(req)` | 处理客户委托批量查询 |
| `deal_trade_query(req)` | 处理客户成交查询 |
| `deal_trade_batch_query(req)` | 处理客户成交批量查询 |
| `deal_fund_query(req)` | 处理客户资金查询 |
| `deal_position_query(req)` | 处理客户持仓查询 |
| `deal_login_req(req)` | 处理登录请求（98 统一登录入口） |

## 引擎/链接接口

| 方法 | 说明 |
|------|------|
| `deal_recv_msg(buf, len, link_type)` | 接收消息处理，按消息头逐个解析 |
| `deal_send_error(msg_buf, msg_len, link_type, err_ret)` | 处理发送失败 |
| `deal_agw_login()` | 发起 agw 用户登录，同步等待结果 |
| `build_agw_login_msg(o_buf, buf_len)` | 构造 agw 登录消息 |
| `ans_agwuser_login(err_code, err_msg)` | 处理 agw 登录应答 |
| `deal_cust_login(req, o_buf, buf_len)` | 处理账户登录事件 |
| `ans_cust_login(req, err_ret, err_msg)` | 处理账户登录应答 |
| `build_heart_msg(o_buf, buf_len)` | 构造心跳消息 |
| `can_link_connect(link_type)` | 判断链接是否可建立 |
| `deal_link_connect(link_type, have_switch)` | 链接成功回调 |
| `deal_link_close(link_type)` | 链接关闭回调 |

## 消息构建（组包）

98 真实协议已实现（counter98_queue_fix 修复：`send_req_msg()` 按精确长度预留/拷贝/提交，消除预留空隙错位问题）：

| 方法 | 消息 |
|------|------|
| `build_order_msg` | 98 委托消息 |
| `build_etf_order_msg` | 98 ETF 申购赎回 |
| `build_bse_order_msg` | 98 北交所委托 |
| `build_cancel_msg` | 98 撤单 |
| `build_order_query_msg` | 98 委托查询 |
| `build_order_batch_query_msg` | 98 委托批量查询 |
| `build_trade_query_msg` | 98 成交查询 |
| `build_trade_batch_query_msg` | 98 成交批量查询 |
| `build_fund_query_msg` | 98 资金查询 |
| `build_position_query_msg` | 98 持仓查询 |
| `build_login_msg` | 98 账户登录 |

> 组包统一走 `send_req_msg()`（`counter98.h`）：先组包到临时缓冲确定 `msg_len`，再按精确长度 `sizeof+msg_len` 预留/拷贝/提交，reader 按 `sizeof+data_len` 消费，两者一致避免错位。

## 应答处理

| 方法 | 说明 |
|------|------|
| `deal_agwuser_login_ans(msg)` | 处理 agw 登录应答 |
| `deal_cust_login_ans(msg)` | 处理账户登录应答 |
| `build_cust_login_rtn(msg, o_ans)` | 从 c98 登录应答构建 API 层 LoginAns |
| `build_fast_counter_login_event(ans, o_info)` | 从 c98 登录应答构建极速柜台登录事件 |
| `build_cust_login_event(req, o_info)` | 从 LoginReq 构建账户登录事件 |
| `delive_fast_counter_login(info)` | 级联投递极速柜台登录 |

## 关键流程

### 登录（98 统一入口）
```
api->login(req) → counter98::deal_login_req
  → build_cust_login_event(req, info)      // LoginReq → acc_login_event_info
  → 入队 ACCOUNT_LOGIN 事件 → multi_engine::deal_cust98_login
  → counter98::deal_cust_login             // 凭据暂存至 C98SessionCache.create_session
  → delive_fast_counter_login(info)        // 级联投递极速柜台
```

### 登录成功回报（缓存客户数据）
```
counter98::deal_cust_login_ans(msg)        // 98 登录应答
  → session_cache_.get_session(fa_key)     // 按 fund_account_id 定位条目
  → session_cache_.fill_session_login_ok(cust_id, branch_id, account_id)  // 回填身份字段
  → delive_fast_counter_login(it->second)  // 级联投递极速柜台登录
```

## 账户登录缓存（C98SessionCache）

**本柜台专属实例缓存**（`c98_session_cache.h`，`counter98` 成员，非全局单例），以 `fund_account_id` 为主键：

- 数据条目复用 `GwSessionInfo`（字段宽度与 98 协议 build_*_msg 目标字段一致）。
- **登录成功回报时缓存**（`deal_cust_login_ans`）：回填 cust_id/branch_id/account_id（值不变不写，RUL-15）。
- **凭据暂存**（`deal_cust_login` → `create_session`）：缓存 order_way_ext/user_info/password 等。
- **组包读取**：`get_session()` 按 fund_account_id 查找，补充 body 中 account_id/cust_id 等。
- **断链清理**：`clear_all()`（链接关闭时全量清除）。

## 待完成任务
1. **缓存条目生命周期**：C98SessionCache 条目永不删除，仅链接关闭时 `clear_all()`；断线自动重登（`collect_relogin_events`）触发接入待完善。
2. **登录状态管理**：`cust_need_login` 已默认 `true`（账户需登录）；超时处理、已登录用户管理待完善。
3. **deal_send_error 留空**：需构造 OrderRtn/CancelRsp 回调客户。

## 近期修复记录
- **`cust_need_login` 默认 true**（`counter98.cpp`）：此前硬编码 `false` 跳过 98 账户登录，已修正为 `true` 使账户登录正常发起。
- **移除重复的 `memcpy(body->session, ...)`**（`counter98.cpp`）：登录应答构建中重复拷贝 session 字段，已清理。

## 设计要点
1. 柜台对上层只暴露业务 API 接口，隐藏组包细节。
2. 所有跨线程通信通过无锁队列（`que_mth_buf`）+ `link_send_event` 头完成。
3. 98 柜台是登录统一入口和查询唯一通道。
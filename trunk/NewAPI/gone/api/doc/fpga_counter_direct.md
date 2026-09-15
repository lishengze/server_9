# fpga_counter_direct 文档

> 源文件：`trunk/NewAPI/gone/api/src/fpga_counter_direct.h` + `fpga_counter_direct.cpp`
> 作用：FPGA 直连模式柜台（单客户），继承 fpga_counter_base。

## 概述

`fpga_counter_direct` 是 FPGA 直连模式柜台，单实例、单客户。唯一额外存储是 `fpga_cust_info client_info_`。**GW 登录成功后才可建立 fpga trade 链接**（trade_ip/port 由 GW 登录应答回填到 `client_info_`）。多态通过模板特化实现（无 virtual 钩子）。

## 链接与队列

```cpp
void init_trade(que_mth_buf *que, link_engine_outop *link_outop);  // 极速业务发送队列
void init_gateway(que_mth_buf *que, link_engine_outop *link_outop);// GW 链接发送队列
```

## 业务接口（api instance 调用）

| 方法 | 说明 |
|------|------|
| `deal_order_req(req)` | 处理委托，若 fpga 客户状态不正常返回离线错误，让上层路由到 98 |
| `deal_etf_order_req(req)` | fpga 不支持，返回不支持错误，让上层路由到 98 |
| `deal_cancel_req(req)` | 处理撤单 |

## 引擎/链接接口

| 方法 | 说明 |
|------|------|
| `deal_recv_msg(buf, len, link_type)` | 接收消息处理（GW 或 core 链接） |
| `deal_send_error(...)` | 处理发送失败 |
| `deal_cust_login(req, o_buf, buf_len)` | 处理账户登录事件 |
| `ans_cust_login(req, err_ret, err_msg)` | 引擎同步阶段失败回调 |
| `can_link_connect(link_type)` | SPEED_TRADE 需 login_state==2 才可建链 |
| `deal_link_connect(link_type, have_switch)` | 链接成功回调 |
| `deal_link_close(link_type)` | 链接关闭回调 |

## 派生类内部方法

| 方法 | 说明 |
|------|------|
| `deal_fpag_state(msg)` | 处理 FPGA 用户状态消息 |
| `deal_log_ans(msg)` | 处理 FPGA 账户登录应答（成功则保存 trade_ip/port，触发 fpga core 链接） |
| `delive_fpga_connect()` | 触发 fpga core 链接 |
| `check_ans_log(err_code)` | 证券信息获取完成后检查登录状态 |

## 登录流程

```
GW 登录应答 → deal_log_ans
  → 保存 trade_ip/port 到 client_info_
  → delive_fpga_connect() 触发 fpga core 链接
  → 证券信息获取完成 → check_ans_log → cb_mgr_->on_login
```

## 关键成员

```cpp
fpga_cust_info client_info_{};      // 单一客户
que_mth_buf *trade_send_queue_;     // 极速柜台发送队列
que_mth_buf *gw_send_queue_;        // GW 链接发送队列
link_engine_outop *gw_eng_op_;      // GW 引擎操作
link_engine_outop *trade_eng_op_;   // 极速引擎操作
```

## 设计要点
1. **单客户**：只存一个 `client_info_`，无客户哈希表。
2. **两级链接**：先 GW 登录，再建 core 链接（基于登录应答的 trade_ip/port）。
3. **委托降级**：fpga 客户状态异常或 ETF 不支持时返回错误码，上层路由到 98。
4. 完成度最高的柜台之一，是 gw/counter98 的实现参考。
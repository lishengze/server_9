# api_instance 文档

> 源文件：`trunk/NewAPI/gone/api/src/api_instance.h` + `api_instance.cpp`
> 作用：API 实例类，唯一按配置选型的实体，持有所有子模块。

## 概述

`api_impl<TFastCounter, TEngine>` 继承 `api_interface`，是 API 的核心实现类。它按配置（fast_counter_type / speed_link_type）选型实例化，持有极速柜台、极速引擎、98 柜台、回调管理器、日志等所有子模块。link 由对应 engine 内部持有，`api_impl` 不冗余保存 link 指针。

## 模板参数

- `TFastCounter`：极速柜台类（`gw_counter_direct` / `fpga_counter_direct` / `fpga_counter_gateway`）
- `TEngine`：极速引擎类型（`tcpdirect_engine` / `single_socket_engine` / shared_engine）

## 类成员

```cpp
speed_link_type speed_link_type_;   // 极速链接类型
counter_type speed_counter_type_;   // 极速柜台类型
TFastCounter fast_;                 // 极速柜台
TEngine fast_engine_;               // 极速引擎
multi_socket_engine<TFastCounter> multi_engine_; // 2槽: g98 + second_link_
counter98 c98_;                     // 98 柜台
callback_manager cb_mgr_;           // 回调管理器
int32_t have_start;                 // 是否启动
lb_common::lb_log log_;             // 日志
```

## 初始化（init）

初始化分三个阶段：
1. **柜台对象自身初始化**：`fast_.init(cfg, &cb_mgr_, &log_)`，设置回调管理器、日志，拷贝配置。
2. **极速引擎初始化**：`fast_engine_.init(cfg, &fast_, &log_)`，引擎内部持有柜台指针。
3. **注入"柜台→引擎"双向引用**：
   - fpga_gateway：`fast_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op())`
   - 其他：`fast_.init_trade(fast_engine_.get_queue(), fast_engine_.get_out_op())`
   - `fast_.init_gateway(multi_engine_.get_queue(), multi_engine_.get_out_op())`

## 业务接口（实现 api_interface）

| 接口 | 委托方式 |
|------|---------|
| `login(req)` | `c98_.deal_login_req(req)`（98 统一入口，级联极速柜台） |
| `order_insert(req)` | `fast_.deal_order_req` 优先，失败降级 `c98_.deal_order_req` |
| `etf_order_insert(req)` | `fast_.deal_etf_order_req` 优先，失败降级 `c98_.deal_etf_order_req` |
| `bse_order_insert(req)` | `fast_.deal_bse_order_req` 优先，失败降级 `c98_.deal_bse_order_req` |
| `order_cancel(req)` | `fast_.deal_cancel_req` 优先，失败降级 `c98_.deal_cancel_req` |
| `order_query(req)` | `c98_.deal_order_query(req)`（只走 98） |
| `order_batch_query(req)` | `c98_.deal_order_batch_query(req)` |
| `trade_query(req)` | `c98_.deal_trade_query(req)` |
| `trade_batch_query(req)` | `c98_.deal_trade_batch_query(req)` |
| `fund_query(req)` | `c98_.deal_fund_query(req)` |
| `position_query(req)` | `c98_.deal_position_query(req)` |

> 委托/撤单走极速柜台（低延迟），失败降级 98；查询类只走 98 柜台（极速柜台无查询接口）。

## 启动（start）

```cpp
// 极速链接心跳/重连 timerfd 注册到 multi 引擎 epoll 线程
fast_engine_.add_timer_poll(multi_engine_.get_thread());
// 98 链接同步建链
multi_engine_.connect_98agw();
// multi 引擎异步启动（接管 98 + 极速GW 的 IO）
multi_engine_.start();
// fast 引擎异步启动（接管极速业务链接 IO）
fast_engine_.start();
// 同步 agw 登录（最后，此时 multi 线程已运行可收应答）
c98_.deal_agw_login();
```

## 停止（stop）

停止所有引擎线程和链接，释放资源。

## 设计要点
1. **配置选型**：通过模板参数在编译期确定柜台和引擎类型，零运行时开销。
2. **模块持有**：单一实体持有所有子模块，生命周期统一管理。
3. **委托降级**：下单类操作极速优先 + 98 降级。
4. **查询统一走 98**：极速柜台无查询接口。
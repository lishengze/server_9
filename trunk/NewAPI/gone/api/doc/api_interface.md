# api_interface.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/api_interface.h`
> 作用：定义 API 接口虚基类，是客户调用 API 的入口。

## 概述

本文件定义了 `api_interface` 虚基类，客户通过它完成登录、委托、撤单、查询等所有操作。同时提供了创建/释放实例的静态工厂函数。

## 使用示例

```cpp
// 1. 创建配置
api_config* cfg = api_config::create_config();
cfg->set_attr(config_name::fast_counter_type, static_cast<int32_t>(counter_type::fpga_direct));
cfg->set_attr(config_name::counter98_addr, net_addr{"192.168.1.1", 9000});
cfg->set_attr(config_name::speed_counter_addr, net_addr{"10.0.0.1", 8000});
cfg->set_attr(config_name::agw98_user, "my_agw_user");
cfg->set_attr(config_name::agw98_user_password, "my_password");
cfg->set_attr(config_name::api_instance_name, "my_api");

// 2. 创建实例
api_interface* api = nullptr;
int32_t ret = api_interface::create_instance(api, *cfg, &my_callback);
api_config::destroy_config(cfg);  // 创建成功后释放配置
if (ret != LBAPI_OK) { /* 处理错误 */ }

// 3. 使用
api->start();
api->login(req);
api->order_insert(order_req);
// ...

// 4. 停止并释放
api->stop();
api->release_instance(api);
```

## 类 api_interface

### 登录

```cpp
virtual int32_t login(const LoginReq &req) = 0;
```
自动完成完整登录流程：
1. 同步连接 98 柜台，发起 agw 登录（使用配置的 98agw_user/98agw_user_password）
2. agw 登录成功后，发起 98 柜台账户登录（使用 req 中的账户信息）
3. 98 账户登录成功后（回调中），级联发起极速柜台登录
4. 极速柜台登录成功后，通过 `on_login` 回调通知用户

任一步骤失败，通过 `on_login` 回调通知用户失败原因。

### 委托与撤单

| 方法 | 说明 | 目标柜台 |
|------|------|---------|
| `order_insert(req)` | 买卖委托 | 极速柜台优先，失败降级 98 |
| `etf_order_insert(req)` | ETF 申购赎回 | 极速柜台优先，失败降级 98 |
| `bse_order_insert(req)` | 北交所委托 | 极速柜台优先，失败降级 98 |
| `order_cancel(req)` | 委托撤单 | 极速柜台优先，失败降级 98 |

### 查询（全部走 98 柜台）

| 方法 | 说明 |
|------|------|
| `order_query(req)` | 客户委托查询 |
| `order_batch_query(req)` | 客户委托批量查询 |
| `trade_query(req)` | 客户成交查询 |
| `trade_batch_query(req)` | 客户成交批量查询 |
| `fund_query(req)` | 客户资金查询 |
| `position_query(req)` | 客户持仓查询 |

### 生命周期

| 方法 | 说明 |
|------|------|
| `start()` | 启动所有链接和线程 |
| `stop()` | 停止所有链接和线程 |

### 静态工厂函数

```cpp
// 创建 API 实例
// 内部自动完成：配置校验 → 模块初始化 → 返回可用实例
// @param o_api  输出实例指针，需由调用方通过 release() 释放
// @param config 配置属性
// @param cb     用户回调接口
// @return api_errno, LBAPI_OK=成功
static int32_t create_instance(api_interface *&o_api, api_config &config, api_callback *cb);

// 释放 API 实例
static void release_instance(api_interface *api);
```

## 设计要点
1. **登录级联**：login 方法自动完成 98agw → 98账户 → 极速柜台的三级登录，对客户透明。
2. **委托降级**：委托/撤单先尝试极速柜台（追求低延迟），失败（柜台离线/不支持）自动降级 98 柜台。
3. **查询统一走 98**：查询类操作全部直接委托 98 柜台，无极速路径。
4. **工厂模式**：`create_instance` 隐藏内部实现类（`api_impl`）的模板实例化细节。
# api_config.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/api_config.h`
> 作用：定义 API 配置属性接口和相关枚举类型。

## 概述

本文件定义了 API 配置相关的枚举类型（市场类型、柜台类型、链接类型、回调模式）和配置属性虚接口类 `api_config`。用户通过 `set_attr` / `get_attr` 系列重载函数设置/读取配置属性，属性名使用 `config_name` 命名空间中的常量。

## 枚举类型

### market_type_t（市场类型）

| 枚举值 | 值 | 说明 |
|--------|----|------|
| `SH_MARKET` | 1 | 上交所 |
| `SZ_MARKET` | 2 | 深交所（支持北交所） |
| `BSE_MARKET` | 3 | 北交所（仅用于bse_order_insert中的市场标识，api实例不直接配置此值） |

### counter_type（柜台类型）

| 枚举值 | 值 | 说明 |
|--------|----|------|
| `fixed_98` | 0 | 98柜台标识（默认占位值，不可作为fast_counter_type最终配置值） |
| `gw_direct` | 1 | 个微软件极速柜台（直连模式） |
| `fpga_direct` | 2 | FPGA 极速柜台（直连模式，单客户） |
| `fpga_gateway` | 3 | FPGA 极速柜台（网关模式，多客户） |

### speed_link_type（极速链接类型）

| 枚举值 | 值 | 适用柜台 |
|--------|----|---------|
| `socket_shared` | 0 | 仅 fpga_gateway |
| `socket_single` | 1 | gw_direct / fpga_direct |
| `tcpdirect` | 2 | gw_direct / fpga_direct（Solarflare TCPDirect加速） |

### callback_mode（回调模式）

| 枚举值 | 值 | 说明 |
|--------|----|------|
| `direct` | 0 | 多线程直接回调（IO/引擎线程中直接调客户回调） |
| `queued` | 1 | 回调队列+回调线程（独立线程消费队列后回调，隔离IO线程） |

## 辅助结构

### net_addr（网络地址）
```cpp
struct net_addr {
  char ip[56];
  int32_t port;
};
```

## 配置属性（config_name 命名空间）

| 属性名常量 | 类型 | 默认值 | 必须设置 | 说明 |
|-----------|------|-------|---------|------|
| `fast_counter_type` | int32 | fixed_98 | **是** | 须设为 gw_direct/fpga_direct/fpga_gateway 之一 |
| `market_type` | int8 | 0 | **是** | 1=上海, 2=深交所/北交所 |
| `speed_link_type_name` | int32 | socket_single | 否 | 极速链接类型 |
| `solarflare_iface` | string | 空 | 否 | tcpdirect模式时必须设置 |
| `speed_counter_addr` | net_addr | 空 | **是** | 极速柜台地址 |
| `speed_counter_addr_bak` | net_addr | 空 | 否 | 极速柜台备用地址 |
| `counter98_addr` | net_addr | 空 | **是** | 98柜台地址 |
| `counter98_addr_bak` | net_addr | 空 | 否 | 98柜台备用地址 |
| `callback_mode_name` | int32 | direct | 否 | 回调模式 |
| `send_poll_num` | int32 | 6 | 否 | 发送轮询数 |
| `recv_poll_num` | int32 | 2 | 否 | 接收轮询数 |
| `heartbeat_interval` | int32 | 5 | 否 | 心跳间隔(秒) |
| `max_reconnect_count` | int32 | 0(无限) | 否 | 最大重连次数 |
| `speed_engine_cpu` | int32 | -1(不绑定) | 否 | 极速引擎CPU绑定 |
| `mgmt_engine_cpu` | int32 | -1(不绑定) | 否 | 管理引擎CPU绑定 |
| `callback_thread_cpu` | int32 | -1(不绑定) | 否 | 回调线程CPU绑定 |
| `send_queue_size_mb` | int32 | 2 | 否 | 发送队列大小(MB) |
| `callback_queue_size_mb` | int32 | 8 | 否 | 回调队列大小(MB) |
| `callback_wait_ms` | int32 | 10 | 否 | 回调等待(ms, 0=死轮询) |
| `multi_io_wait_ms` | int32 | 100 | 否 | epoll_wait超时(ms) |
| `98agw_user` | string | 空 | **是** | AGW用户 |
| `98agw_user_password` | string | 空 | **是** | AGW用户密码 |
| `agw_user_login_timeout` | int32 | 10(秒) | 否 | AGW登录超时 |
| `log_queue_size_mb` | int32 | 8(0=同步) | 否 | 日志队列大小 |
| `log_level` | int32 | 1(通知) | 否 | 日志级别 |
| `api_instance_name` | string | 空 | **是** | API实例名 |
| `log_output_dir` | string | ./api_log | 否 | 日志输出目录 |

## 类 api_config

虚接口类，通过 `set_attr` / `get_attr` 系列重载函数访问配置属性。

### 工厂函数

```cpp
static api_config *create_config();               // 创建默认配置实例
static void destroy_config(api_config *cfg);      // 释放配置实例
```

### 设置属性（按类型重载）
```cpp
int32_t set_attr(const char *attr_name, int8_t attr_val);
int32_t set_attr(const char *attr_name, int32_t attr_val);
int32_t set_attr(const char *attr_name, const char *attr_val);
int32_t set_attr(const char *attr_name, const net_addr &attr_val);
// ... 还有 int16/int64/bool/string 版本（部分为预留接口）
```

### 获取属性（按类型重载）
```cpp
int32_t get_attr(const char *attr_name, int32_t &o_val) const;
int32_t get_attr(const char *attr_name, char *o_val, int32_t buf_len) const;
int32_t get_attr(const char *attr_name, net_addr &o_val) const;
// ... 对应 set_attr 的各类型版本
```

### 其他方法
```cpp
int32_t validate() const;           // 配置校验
int32_t copy_from(const api_config &src);  // 从源配置复制所有属性
```

## 使用示例
```cpp
api_config* cfg = api_config::create_config();
cfg->set_attr(config_name::fast_counter_type, static_cast<int32_t>(counter_type::fpga_direct));
cfg->set_attr(config_name::counter98_addr, net_addr{"192.168.1.1", 9000});
cfg->set_attr(config_name::speed_counter_addr, net_addr{"10.0.0.1", 8000});
cfg->set_attr(config_name::agw98_user, "my_agw_user");
cfg->set_attr(config_name::agw98_user_password, "my_password");
cfg->set_attr(config_name::api_instance_name, "my_api");

// create_instance 后 config 不再使用
api_interface* api = nullptr;
api_interface::create_instance(api, *cfg, &my_callback);
api_config::destroy_config(cfg);
```

## 设计要点
1. 新增配置项只需在 `config_name` 命名空间添加常量 + 实现类添加成员，接口不变。
2. `create_instance` 成功后配置被分发到内部模块，config 对象不再使用。
3. `int16/int64/bool` 类型的 set_attr/get_attr 为预留接口，当前返回 `LBAPI_ERR_UNSUPPORTED`。
4. `run_mode` 概念已合并到 `counter_type`，无独立配置项。
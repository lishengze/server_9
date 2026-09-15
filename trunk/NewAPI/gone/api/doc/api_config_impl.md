# api_config_impl 文档

> 源文件：`trunk/NewAPI/gone/api/src/api_config_impl.h` + `api_config.cpp`
> 作用：api_config 虚接口的具体实现类，内部持有所有配置属性值。

## 概述

`api_config_impl` 继承自 `api_config`，是配置属性的具体实现。内部持有所有配置属性的成员变量（均有默认值），实现 `set_attr` / `get_attr` 虚接口，并提供便捷的 `get_*` 方法供内部模块直接使用（非虚、无类型间接）。

## 属性期望类型枚举 attr_type

```cpp
enum class attr_type : int32_t {
  int16_val = 0,
  int32_val = 1,
  int64_val = 2,
  bool_val = 3,
  string_val = 4,
  addr_val = 5,
  int8_val = 6
};
```
用于 `check_attr_type` 内部类型校验，确保 `set_attr` 传入的类型与属性期望类型一致。

## 虚接口实现

完整实现 `api_config` 的所有纯虚方法，包括各类型重载的 `set_attr` / `get_attr`、`validate()`、`copy_from()`。

## 便捷 Getter 方法（非虚，内部模块使用）

| 方法 | 返回类型 | 说明 |
|------|---------|------|
| `get_fast_counter_type()` | `counter_type` | 极速柜台类型 |
| `get_market_type()` | `market_type_t` | 市场类型 |
| `get_speed_link_type()` | `speed_link_type` | 极速链接类型 |
| `get_solarflare_iface()` | `const char*` | Solarflare 网卡名 |
| `get_speed_counter_addr()` | `net_addr` | 极速柜台主地址 |
| `get_speed_counter_addr_bak()` | `net_addr` | 极速柜台备地址 |
| `get_counter98_addr()` | `net_addr` | 98 柜台地址 |
| `get_counter98_addr_bak()` | `net_addr` | 98 柜台备地址 |
| `get_callback_mode()` | `callback_mode` | 回调模式 |
| `get_send_poll_num()` | `int32_t` | 发送轮询批量数 |
| `get_recv_poll_num()` | `int32_t` | 接收轮询批量数 |
| `get_heartbeat_interval()` | `int32_t` | 心跳间隔(秒) |
| `get_max_reconnect_count()` | `int32_t` | 最大重连次数 |
| `get_speed_engine_cpu()` | `int32_t` | 极速引擎 CPU 亲和 |
| `get_mgmt_engine_cpu()` | `int32_t` | 管理引擎 CPU 亲和 |
| `get_callback_thread_cpu()` | `int32_t` | 回调线程 CPU 亲和 |
| `get_send_queue_size_mb()` | `int32_t` | 发送队列大小(MB) |
| `get_callback_queue_size_mb()` | `int32_t` | 回调队列大小(MB) |
| `get_callback_wait_ms()` | `int32_t` | 回调等待(ms) |
| `get_multi_io_wait_ms()` | `int32_t` | epoll_wait 超时(ms) |
| `get_agw98_user()` | `const char*` | AGW 用户名 |
| `get_agw98_user_password()` | `const char*` | AGW 用户密码 |
| `get_agw_user_login_timeout()` | `int32_t` | AGW 登录超时(秒) |
| `get_log_queue_size_mb()` | `int32_t` | 日志队列大小(MB) |
| `get_log_level()` | `int32_t` | 日志级别 |
| `get_api_instance_name()` | `const char*` | API 实例名称 |
| `get_log_output_dir()` | `const char*` | 日志输出目录 |

## 成员变量（均有默认值）

```cpp
counter_type fast_counter_type_;        // 默认: fixed_98
market_type_t market_type_;             // 默认: 0
speed_link_type speed_link_type_;       // 默认: socket_single
char solarflare_iface_[64];             // 默认: 空
net_addr speed_counter_addr_;           // 默认: 空
net_addr speed_counter_addr_bak_;       // 默认: 空
net_addr counter98_addr_;               // 默认: 空
net_addr counter98_addr_bak_;           // 默认: 空
callback_mode callback_mode_;           // 默认: direct
int32_t send_poll_num_;                 // 默认: 6
int32_t recv_poll_num_;                 // 默认: 2
int32_t heartbeat_interval_;            // 默认: 5
int32_t max_reconnect_count_;           // 默认: 0(无限)
int32_t speed_engine_cpu_;              // 默认: -1(不绑定)
int32_t mgmt_engine_cpu_;               // 默认: -1
int32_t callback_thread_cpu_;           // 默认: -1
int32_t send_queue_size_mb_;            // 默认: 2
int32_t callback_queue_size_mb_;        // 默认: 8
int32_t callback_wait_ms_;              // 默认: 10
int32_t multi_io_wait_ms_;              // 默认: 100
char agw98_user_[32];                   // 默认: 空
char agw98_user_password_[256];         // 默认: 空
int32_t agw_user_login_timeout_;        // 默认: 10
int32_t log_queue_size_mb_;             // 默认: 8
int32_t log_level_;                     // 默认: 1(通知)
char api_instance_name_[64];            // 默认: 空
char log_output_dir_[256];              // 默认: ./api_log
```

## 设计要点
1. **类型安全**：`check_attr_type` 确保 set_attr 传入类型与属性期望类型一致，避免运行时类型错误。
2. **便捷 getter**：内部模块通过非虚 getter 直接读取，避免虚函数调用开销。
3. **统一默认值**：所有属性有合理默认值，减少配置负担。
4. **新增配置项**：只需在 `api_config_impl` 添加成员 + setter/getter + config_name 常量即可。
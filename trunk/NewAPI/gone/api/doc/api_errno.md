# api_errno.h 文档

> 源文件：`trunk/NewAPI/gone/api/include/api_errno.h`
> 作用：定义 API 错误码枚举和错误信息获取函数。

## 概述

本文件定义了 API 模块内部使用的错误码枚举 `api_errno`，以及获取错误描述信息的函数 `api_strerror`。0 表示成功，负值表示错误。

## 枚举 api_errno

| 错误码 | 值 | 说明 |
|--------|----|------|
| `LBAPI_OK` | 0 | 成功 |
| `LBAPI_ERR_INVALID_PARAM` | -1 | 无效参数 |
| `LBAPI_ERR_UNSUPPORTED_TYPE` | -2 | 不支持的类型 |
| `LBAPI_ERR_UNSUPPORTED_OP` | -3 | 不支持的操作 |
| `LBAPI_ERR_STATE_LIMITED` | -4 | 状态错误（init/start 调用时序不对） |
| `LBAPI_ERR_CFG_INVALID` | -5 | 配置错误 |
| `LBAPI_ERR_TRANS_TYPE` | -6 | 类型转换错误 |
| `LBAPI_ERR_UNSUPPORT_LINK` | -7 | 不支持的链接 |
| `LBAPI_ERR_UNSUPPORT_COUNTER` | -8 | 不支持的柜台 |
| `LBAPI_ERR_LOG_INIT` | -9 | 日志初始化失败 |
| `LBAPI_ERR_QUEUE_INIT` | -10 | 队列初始化失败 |
| `LBAPI_ERR_THREAD_INIT` | -11 | 线程初始化失败 |
| `LBAPI_ERR_TIMER_INIT` | -12 | 定时器初始化失败 |
| `LBAPI_ERR_ATTR_INIT` | -13 | 资源属性初始化失败 |
| `LBAPI_ERR_LINK_INIT` | -14 | 链接对象初始化失败 |
| `LBAPI_ERR_COUNTER_INIT` | -15 | 柜台对象初始化失败 |
| `LBAPI_ERR_THRAD_START` | -16 | 启动线程失败 |
| `LBAPI_ERR_LINK_ADDR` | -17 | 链接地址错误 |
| `LBAPI_ERR_LINK_CONNECT` | -18 | 建立连接错误 |
| `LBAPI_ERR_NO_SOLARFLARE` | -19 | 无 solarflare 网卡 |
| `LBAPI_ERR_LINK_CONNECT_REPEAT` | -20 | 重复链接 |
| `LBAPI_ERR_LINK_START_RECV` | -21 | 启动链接接收失败 |
| `LBAPI_ERR_LINK_DISCONNECTED` | -22 | 物理断线 |
| `LBAPI_ERR_LINK_HEART_TIMEOUT` | -23 | 心跳超时 |
| `LBAPI_ERR_COUNTER_OFFLINE` | -24 | 柜台客户状态不正常 |
| `LBAPI_ERR_SEND_QUEUE_FULL` | -25 | 发送队列已满 |
| `LBAPI_ERR_MSG_LEN` | -26 | 消息长度错误 |
| `LBAPI_ERR_MSG_TYPE` | -27 | 消息类型错误 |
| `LBAPI_ERR_BUILD_MSG` | -28 | 构建消息失败 |
| `LBAPI_ERR_SEND_MSG` | -29 | 发送消息失败 |
| `LBAPI_ERR_LOGIN_FAIL` | -30 | 登陆失败 |
| `LBAPI_ERR_LOGIN_TIMEOUT` | -31 | 登陆超时 |
| `LBAPI_ERR_ADD_TIMER` | -32 | 添加定时事件失败 |
| `LBAPI_ERR_ADD_WAKEFD` | -33 | 添加触发事件失败 |
| `LBAPI_ERR_NOT_LOG_AGW` | -34 | 未登陆 agw 用户 |
| `LBAPI_ERR_NOT_LOG_CUST` | -35 | 柜台投资者未登陆 |
| `LBAPI_ERR_NO_SEC` | -36 | 证券信息不存在 |
| `LBAPI_ERR_ALLOC_MEM` | -37 | 获取内存失败 |
| `LBAPI_ERR_NO_CUST` | -38 | 客户信息不存在 |
| `LBAPI_ERR_INSERT_MAP` | -39 | 插入 map 失败 |

## 函数 api_strerror

```cpp
const char *api_strerror(int32_t err);
```
获取错误码对应的错误描述字符串（静态存储，无需释放）。

## 设计要点
1. 错误码覆盖初始化、链接、登录、消息、队列、配置等各环节。
2. `LBAPI_ERR_COUNTER_OFFLINE`（-24）用于极速柜台返回给 `api_impl`，触发降级到 98 柜台。
3. 错误码按功能分组：-1~-8 参数/配置错误，-9~-16 初始化错误，-17~-23 链接错误，-24~-25 柜台/队列错误，-26~-29 消息错误，-30~-31 登录错误，-32~-39 其他。
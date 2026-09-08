# api_event_msg.h 文档

> 源文件：`trunk/NewAPI/gone/api/src/api_event_msg.h`
> 作用：定义引擎消息队列的事件类型、链接类型常量和事件信息结构体。

## 概述

本文件是 API 内部引擎通信的"消息协议"定义，包括：链接事件类型宏、链接类型常量、链接发送事件结构、账户登录事件、链接关闭/连接事件等。

## 链接事件类型宏

| 宏 | 值 | 说明 |
|----|----|------|
| `LINK_EVENT_TYPE_SEND_MSG` | 0 | 发送消息 |
| `LINK_EVENT_TYPE_SEND_HEART` | 1 | 发送心跳 |
| `LINK_EVENT_TYPE_LINK_CLOSE` | 2 | 链接关闭 |
| `LINK_EVENT_TYPE_LINK_CONNECT` | 3 | 连接/重连 |
| `LINK_EVENT_TYPE_ACCOUNT_LOGIN` | 4 | 账户登录请求 |
| `LINK_EVENT_TYPE_FPGA_CORE_CONNECT` | 5 | 发起 fpga core 链接（fast_engine 同步 connect） |
| `LINK_EVENT_TYPE_AGWUSER_LOGIN` | 6 | agw 用户登录请求 |

> 注意：`FPGA_CORE_OK / FPGA_CORE_FAIL` 故意未定义——fast_engine 同步 connect 后直接同步返回结果，无需异步事件回传。

## 链接类型常量

| 常量 | 值 | 用途 |
|------|----|------|
| `LINK_TYPE_98` | 0 | 98 柜台链接（唯一） |
| `LINK_TYPE_SPEED_TRADE` | 1 | 极速交易链接（个微业务 / fpga core） |
| `LINK_TYPE_SPEED_GW` | 2 | 极速网关链接（fpga 模式专用） |
| `LINK_TYPE_MAX` | 3 | 链接类型总数占位 |

> **命名约定**：`link_type` 表达"什么类型的链接"，物理槽位是 engine 内部细节；引擎内部维护 type→slot 映射。

## 链接发送事件 link_send_event

```cpp
struct link_send_event {
  int16 link_type; // 目标链接类型 (LINK_TYPE_98/SPEED_TRADE/SPEED_GW)
  int16 type;      // 事件类型 (LINK_EVENT_TYPE_*)
  int32 data_len;  // 数据长度 (心跳/关闭/重连/链接事件时为0)
  char data[0];    // 数据柔性数组
};
```
柜台组包后通过 `take_req_que_mem`/`cmt_req_que_mem` 写入引擎队列，引擎 `do_work()` 消费。

## 账户登录事件 acc_login_event_info

```cpp
struct acc_login_event_info {
  int64_t cust_req_no;            // 客户私有请求号
  char cust_id[16];               // 客户号
  char fund_account_id[16];       // 客户资金账号
  char branch_id[10];             // 分支机构代码
  char account_id[12];            // 客户股东账号
  char order_way_ext[2];          // 客户委托方式
  char session[32];               // agw 用户登录返回的会话号
  char password[256];             // 密码
  char user_info[64];             // 用户私有信息
  char client_feature_code[1024]; // 客户终端信息
};
```
由 `counter98::deal_log_req` 转换得到，投递到对应引擎触发极速柜台登录。

## 其他事件结构

| 结构 | 字段 | 用途 |
|------|------|------|
| `link_close_event_info` | err_code, filled | 链接关闭事件 |
| `link_connect_event_info` | need_switch, filled | 链接连接/重连事件，need_switch=切备地址 |
| `fpga_core_connect_info` | trade_port, trade_ip | fpga core 链接信息（从 fpga login_ans 提取） |

## 链接引擎操作接口 link_engine_outop

```cpp
class link_engine_outop {
public:
  virtual void deal_heart_msg_ans(int16 link_type) {};  // 处理心跳应答
  virtual void trigger_send() {};                        // 触发发送（唤醒引擎线程）
  virtual void deal_close_link(int16 link_type, int32 err_code) {}; // 关闭链接
};
```
柜台通过此接口与引擎交互（触发发送、关闭链接等）。

## 设计要点
1. 事件类型 + 链接类型 + 柔性数组构成统一的消息队列协议，支持跨线程通信。
2. `link_type` 按逻辑角色分类，物理槽位对柜台透明。
3. `link_send_event` 的 `data[0]` 柔性数组支持任意长度的业务消息。
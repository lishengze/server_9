#pragma once

#include "comm_sys.h"

#include "g1msghead.h"
#include "order_trade_type.h"

#include <cstdint>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

using lb_common::int16;
using lb_common::int32;

#define LINK_EVENT_TYPE_SEND_MSG      0 ///< 发送消息
#define LINK_EVENT_TYPE_SEND_HEART    1 ///< 发送心跳
#define LINK_EVENT_TYPE_LINK_CLOSE    2 ///< 链接关闭
#define LINK_EVENT_TYPE_LINK_CONNECT  3 ///< 连接/重连
#define LINK_EVENT_TYPE_ACCOUNT_LOGIN 4 ///< 账户登陆请求
// FPGA_CORE_OK / FPGA_CORE_FAIL 故意未定义：fast_engine 同步 connect 后直接
// 同步返回结果，无需异步事件回传
#define LINK_EVENT_TYPE_FPGA_CORE_CONNECT 5 ///< 发起 fpga core 链接（fast_engine 同步 connect）
#define LINK_EVENT_TYPE_AGWUSER_LOGIN     6 ///< agw用户登陆请求

/// 链接类型常量（按逻辑角色分类，不再用 idx）
///
/// 命名约定：
///   - link_type 表达"什么类型的链接"
///   - 物理槽位是 engine 内部细节，外部用 link_type
///   - 引擎内部维护 type→slot 映射
///
/// 三种类型（互不重复值）：
///   - LINK_TYPE_98          : 98 柜台链接（唯一）
///   - LINK_TYPE_SPEED_TRADE : 极速交易链接
///                           · 个微：个微业务链接（fast_engine 或 multi 槽 1）
///                           · fpga：fpga core 链接（fast_engine，fpga direct 模式专用）
///   - LINK_TYPE_SPEED_GW    : 极速网关链接（fpga 模式专用）
///                           · fpga direct：fpga GW 链接（登录/证券信息）
///                           · fpga gateway：fpga GW 链接（登录/证券信息/业务）
///
/// 命名风格参考 fpga 柜台类（fpga_counter_direct / fpga_counter_gateway）
constexpr int16_t LINK_TYPE_98 = 0;          ///< 98 柜台链接
constexpr int16_t LINK_TYPE_SPEED_TRADE = 1; ///< 极速交易链接
constexpr int16_t LINK_TYPE_SPEED_GW = 2;    ///< 极速网关链接
constexpr int16_t LINK_TYPE_MAX = 3;         ///< 链接类型总数占位

/// 链接发送事件 (柔性数组, 写入时按 total = sizeof(link_send_event) + data_len 申请)
struct link_send_event {
  int16 link_type; ///< 目标链接类型 (LINK_TYPE_98 / LINK_TYPE_SPEED_TRADE / LINK_TYPE_SPEED_GW)
  int16 type;      ///< 事件类型
  int32 data_len;  ///< 数据长度 (心跳/关闭/重连/链接事件时为 0)
  char data[0];    ///< 数据柔性数组
};

/// 账户登录事件信息 (从 c98.deal_log_req 转换得到, 投递到对应引擎)
struct acc_login_event_info {
  int64_t cust_req_no;            ///< 客户私有请求号
  char cust_id[16];               ///< 客户号
  char fund_account_id[16];       ///< 客户资金账号
  char branch_id[10];             ///< 分支机构代码
  char account_id[12];            ///< 客户股东账号
  char order_way_ext[2];          ///< 客户委托方式
  char session[32];               ///< agw 用户登陆返回的会话号
  char password[256];             ///< 密码
  char user_info[64];             ///< 用户私有信息
  char client_feature_code[1024]; ///< 客户终端信息
};

/// 链接关闭事件信息
struct link_close_event_info {
  int32 err_code; ///< 错误码
  int32 filled;   ///< 占位对齐
};

/// 链接连接/重连事件信息
struct link_connect_event_info {
  int32 need_switch; ///< 是否切到备地址 (1=切, 0=不切)
  int32 filled;      ///< 占位对齐
};

struct fpga_core_connect_info {
  int32_t trade_port;           ///< core 链接端口（从 fpga login_ans 提取）
  char trade_ip[G1_IPADDR_LEN]; ///< core 链接 IP（从 fpga login_ans 提取）
};

class link_engine_outop {
public:
  virtual void deal_heart_msg_ans(int16 link_type) {};
  virtual void trigger_send() {};
  virtual void deal_close_link(int16 link_type, int32 err_code) {};
};

} // namespace lb_api
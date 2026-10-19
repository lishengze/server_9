#pragma once

#include "comm_sys.h"
#include "matomic.h"

#include "g1msghead.h"
#include "order_trade_type.h"
#include "api_errno.h"

#include <cstdint>
#include <cstring>
#include <time.h>

namespace lb_api {

/// 性能测试辅助：获取当前单调时钟纳秒（低开销，clock_gettime 约 10-30ns）
inline uint64_t perf_now_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

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

/// 客户登录状态字典 (fpga_cust_info.login_state / counter98 cust_login_map_ val / gw_counter_direct.login_state)
static constexpr int32_t LOGIN_STATE_NONE = 0;   ///< 未登录(从未)
static constexpr int32_t LOGIN_STATE_LOGOUT = 1; ///< 已登出(曾成功后登出)
static constexpr int32_t LOGIN_STATE_OK = 2;     ///< 登录成功(保持至用户主动登出; 单客户模式绑定过账户后不支持换账户登录)
static constexpr int32_t LOGIN_STATE_MASK = 3;   ///< LOGIN_STATE_LOGOUT | LOGIN_STATE_OK

/// 账户登录在途标记字典 (log_state_ctl 位标志, 与登录状态位组合编码在同一原子字内)
static constexpr int32_t LOGIN_PENDING_USER = 0x10;    ///< 用户在途(应答成功回调一次, 失败清状态回调)
static constexpr int32_t LOGIN_PENDING_RELOGIN = 0x20; ///< 重连(内部)重登在途(成功静默; 失败清状态+回调+不再自动重登, 不关链)
static constexpr int32_t LOGIN_PENDING_MASK = 0x30;    ///< LOGIN_PENDING_USER | LOGIN_PENDING_RELOGIN

/// 登录控制状态机
struct log_state_ctl {
  int32_t log_state;

  /// 用户登出，检查设置 logout
  /// 返回 ：1-设置成功 ， 0-原来就未登录，<0 - 存在在途请求
  int32_t to_logout() {
    do {
      int32_t t = lb_common::atomic_load32(&log_state);
      if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER)
        return LBAPI_ERR_STATE_LIMITED;
      if ((t & LOGIN_STATE_MASK) != LOGIN_STATE_OK)
        return 0;
      if (lb_common::atomic_cas32(&log_state, &t, LOGIN_STATE_LOGOUT))
        return 1;
    } while (true);
  }

  /// 是否为初始
  bool is_free() {
    int32_t t = lb_common::atomic_load32(&log_state);
    return t == 0;
  }

  /// 是否登录成功
  bool is_logined() {
    int32_t t = lb_common::atomic_load32(&log_state);
    return ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK);
  }

  /// 处理登录事件，检查设置
  /// 该函数保证同一客户只有一个在途登录请求到后台
  /// o_havelog , 返回是否为登录状态 (仅返回 0 时有意义: 1=幂等回调用户登录成功)
  /// 返回 1- 发到后台 ， 0-终止
  int32_t to_login(int32_t &o_havelog, int32_t is_user_login) {
    do {
      int32_t t = lb_common::atomic_load32(&log_state);
      if (is_user_login == 1) {
        if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK) {
          o_havelog = 1;
          return 0;
        }
        if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER) {
          o_havelog = 0;
          return 0;
        }
        if ((t & LOGIN_PENDING_MASK) == 0) {
          if (lb_common::atomic_cas32(&log_state, &t, LOGIN_PENDING_USER)) {
            o_havelog = 0;
            return 1;
          }
        } else {
          if (lb_common::atomic_cas32(&log_state, &t, LOGIN_PENDING_USER)) {
            o_havelog = 0;
            return 0;
          }
        }
      } else {
        if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK) {
          o_havelog = 1;
        } else {
          o_havelog = 0;
          return 0;
        }
        if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER) {
          o_havelog = 0;
          return 0;
        }
        int32_t n = ((t & LOGIN_STATE_MASK) | LOGIN_PENDING_RELOGIN);
        if (lb_common::atomic_cas32(&log_state, &t, n)) {
          return 1;
        }
      }
    } while (true);
  }

  /// 链接关闭或异步处理失败, 清除pending
  /// 返回原来的在途登录：0-无在途，1-用户在途，2-重登在途
  int32_t clear_pending() {
    do {
      int32_t t = lb_common::atomic_load32(&log_state);
      if ((t & LOGIN_PENDING_MASK) == 0) return 0;
      int32_t ret = ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER) ? 1 : 2;
      int32_t n = (t & LOGIN_STATE_MASK);
      if (lb_common::atomic_cas32(&log_state, &t, n)) return ret;
    } while (true);
  }

  /// 后台登录应答成功，设置状态
  /// 返回：1- 回调用户成功，0- 不回调用户
  int32_t ans_login_ok() {
    do {
      int32_t t = lb_common::atomic_load32(&log_state);
      int32_t ret = ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER);
      if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_LOGOUT) return ret;
      if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK) ret = 0;
      if (lb_common::atomic_cas32(&log_state, &t, LOGIN_STATE_OK)) return ret;
    } while (true);
  }

  /// 后台登录应答失败，设置状态
  /// 返回：1- 回调用户失败，0- 不回调用户
  int32_t ans_login_err() {
    do {
      int32_t t = lb_common::atomic_load32(&log_state);
      if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_LOGOUT) return 0;
      if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER) {
        if (lb_common::atomic_cas32(&log_state, &t, 0)) return 1;
      } else if ((t & LOGIN_PENDING_MASK) == 0) {
        return 0;
      } else {
        if (lb_common::atomic_cas32(&log_state, &t, 0)) return 1;
      }
    } while (true);
  }
};

/// 链接发送事件 (柔性数组, 写入时按 total = sizeof(link_send_event) + data_len 申请)
struct link_send_event {
  int16 link_type; ///< 目标链接类型 (LINK_TYPE_98 / LINK_TYPE_SPEED_TRADE / LINK_TYPE_SPEED_GW)
  int16 type;      ///< 事件类型
  int32 data_len;  ///< 数据长度 (心跳/关闭/重连/链接事件时为 0)
  uint64_t *leave_time_ptr; ///< 发送完成时间戳写入指针（性能测试用，指向 OrderReq::api_leave_time_ns；非委托事件为 nullptr）
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
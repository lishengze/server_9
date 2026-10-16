#ifndef G1DICTDEFINE_H
#define G1DICTDEFINE_H

#include <cstdint>

// g1dictdefine - G1 协议整型字典（api / fgw / fwm 公共）
//
// DB char(1) 列（DataStatus/FPGAStatus/ForceReloadData/...）读入 char 后
// 按 (ch - '0') 转整型再与本字典比较。

// ============================================================================
// fpga 用户状态（g1trademsg.h fpga_user_state.state；fgw customer.onboard_state）
// ============================================================================
static constexpr int32_t G1_FPGA_USER_STATE_NORMAL = 1;  ///< 正常
static constexpr int32_t G1_FPGA_USER_STATE_FAULT = 2;   ///< 故障
static constexpr int32_t G1_FPGA_USER_STATE_SWITCH = 3;  ///< 故障切换柜台

// ============================================================================
// t_FpgaCounterStatus.FPGAStatus（DB char(1)：'1'-FPGA 正常，'2'-FPGA 异常）
// ============================================================================
static constexpr int32_t G1_FPGA_STATUS_NORMAL = 1; ///< FPGA 正常
static constexpr int32_t G1_FPGA_STATUS_FAULT = 2;  ///< FPGA 异常

// ============================================================================
// t_FpgaCounterStatus.DataStatus 上场数据状态（DB char(1)）
// ============================================================================
static constexpr int32_t G1_DATA_STATUS_NOT_UP = 0;   ///< 未上场
static constexpr int32_t G1_DATA_STATUS_UPLOADED = 1; ///< 上场完成
static constexpr int32_t G1_DATA_STATUS_FAILED = 2;   ///< 上场失败

// ============================================================================
// t_FpgaCounterStatus.ForceReloadData 强制重新上场数据（DB char(1)）
// ============================================================================
static constexpr int32_t G1_FORCE_RELOAD_NO = 0;   ///< 否
static constexpr int32_t G1_FORCE_RELOAD_BASE = 1; ///< 仅更新证券和客户基础信息
static constexpr int32_t G1_FORCE_RELOAD_ALL = 2;  ///< 全部数据

// ============================================================================
// 用户登录 log_type（g1trademsg.h login_req/ans.log_type）
// ============================================================================
static constexpr int32_t G1_LOG_TYPE_USER = 1;    ///< 用户
static constexpr int32_t G1_LOG_TYPE_GATEWAY = 2; ///< 网关

#endif // G1DICTDEFINE_H

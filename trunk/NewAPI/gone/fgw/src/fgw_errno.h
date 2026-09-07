// fgw_errno - fgw 内部错误码
//
// Step 5 决策：仅保留错误码定义；其他常量/类型移到所属模块
// 命名：FGW_OK / FGW_ERR_XXX

#pragma once

namespace lb_fgw {

// ============================================================================
// fgw 内部错误码（业务可观察的错误回执）
// ============================================================================
static constexpr int FGW_OK = 0;
static constexpr int FGW_ERR_BOARD_NA = -1001;      ///< 业务上行找不到板卡（回 G1_MSG_GW_REJ）
static constexpr int FGW_ERR_NOT_LOGGED = -1002;    ///< 板卡未完成网关登录（回 G1_MSG_GW_REJ）
static constexpr int FGW_ERR_CUST_NA = -1003;       ///< 客户不存在
static constexpr int FGW_ERR_RECV_DOWN = -1004;     ///< 业务下行找不到 api 接收方（丢弃 + 日志）
static constexpr int FGW_ERR_BUF_FULL = -1005;      ///< 内部队列满
static constexpr int FGW_ERR_PARAM = -1006;         ///< 参数错误
static constexpr int FGW_ERR_LINK_STATE = -1007;    ///< 链接状态限制操作
static constexpr int FGW_ERR_HEART_TIMEOUT = -1008; ///< 链接心跳超时
static constexpr int FGW_ERR_ALLOC_MEM = -1009;     ///< 申请内存失败
static constexpr int FGW_ERR_MSG_LEN = -1010;       ///< 数据长度错误
static constexpr int FGW_ERR_MSG_VER = -1011;       ///< 数据版本不支持
static constexpr int FGW_ERR_ADD_AGWSESS = -1012;   ///< 添加 agwuser sesssion 失败
static constexpr int FGW_ERR_DUP_EXIST = -1013;     ///< 键已存在，拒绝重复插入
static constexpr int FGW_ERR_AGWSESS_NA = -1014;    ///< agwuser sesssion 不存在
static constexpr int FGW_ERR_LINK_NA = -1015;       ///< 链接不存在
static constexpr int FGW_ERR_LINK_SEND = -1016;     ///< 链接发送失败
static constexpr int FGW_ERR_CUST_OFFLINE = -1017;  ///< 板卡上用户状态故障
} // namespace lb_fgw
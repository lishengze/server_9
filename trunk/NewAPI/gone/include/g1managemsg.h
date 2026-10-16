#ifndef G1MANAGEMSG_H
#define G1MANAGEMSG_H

#include "g1msghead.h"
#include "g1comdefine.h"

/*管理 web 网关与后台 fdm 间的消息的通用结构：
g1_msg_head + g1_mng_msg_head + body 。

g1_msg_head 中的 msg_id, msg_len , board_no 是必填的，user_id 依赖是否为客户相关消息，session_id 暂未使用。
board_no ，user_id web 前端不设置，web 网关对于请求必须查询内部缓存信息来填写再传给后台。
*/

#define G1_EXCHANGE_OFFERNO_LEN 12 // 交易所报盘委托的报盘合同号长度
#define G1_EXCHANGE_PBU_LEN 8      // 交易所PBU 长度
#define G1_EXCHANGE_SENDCMP_LEN 32 // 交易所网关登录的发送方代码长度
#define G1_EXCHANGE_PASSWD_LEN 32  // 交易所网关登录的密码长度(对齐 t_ExchangeGateInfo.LogPasswd char(32); 原值16, 改前全仓零引用)
#define G1_EXCHANGE_BRANCH_LEN 8   // 交易所报盘的营业部长度
#define G1_OFFERNO_PREFIX_LEN 3    // 板卡报单编号前三位长度(t_BoardAttr.OfferNoPrefix3)
#define G1_RULE_LEN 24             // 验资验券规则长度(t_FundHoldChange.Rule)

/*管理消息头*/
struct g1_mng_msg_head
{
  uint32_t req_connect_id; // 来自网关请求的连接id，后台 fdm 填写，用于在fpga 应答找回网关链接
  uint32_t req_no;         // 请求号，管理 web 网关生成，后台应答带回
};

#define G1_MNGMSG_SEC_AUTH_REQ 1101u      ///< 证券买卖权限修改请求
#define G1_MNGMSG_SEC_AUTH_ANS 2101u      ///< 证券买卖权限修改应答
#define G1_MNGMSG_USER_SEC_AUTH_REQ 1102u ///< 用户证券买卖权限修改请求
#define G1_MNGMSG_USER_SEC_AUTH_ANS 2102u ///< 用户证券买卖权限修改应答

// 证券买卖权限修改请求，web 网关发给后台 fdm,再到 fpga 核心内
struct sys_sec_auth_req
{
  char security_id[G1_SECURITYID_MAXLEN]; // 证券代码
  uint16_t board_no;                      // 指定板卡
  char bid_flag;                          // 是否可买
  char ask_flag;                          // 是否可卖
  int32_t sec_index;                      // 证券索引，web 网关填写，再传给后台。
};
// 证券买卖权限修改应答
struct sys_sec_auth_ans
{
  char security_id[G1_SECURITYID_MAXLEN]; // 证券代码
  uint16_t board_no;                      // 指定板卡
  char bid_flag;                          // 是否可买
  char ask_flag;                          // 是否可卖
  int32_t err_code;                       // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// 用户证券买卖权限修改请求，web 网关发给后台 fdm,再到 fpga 核心内
struct user_sec_auth_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  char bid_flag;                              // 是否可买
  char ask_flag;                              // 是否可卖
  uint16_t user_id;                           // 用户 id ，web 网关填写，再传给后台。
  int32_t sec_index;                          // 证券索引，web 网关填写，再传给后台。
};
// 用户证券买卖权限修改应答
struct user_sec_auth_ans
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  char bid_flag;                              // 是否可买
  char ask_flag;                              // 是否可卖
  uint16_t user_id;                           // 用户 id
  int32_t err_code;                           // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

#define G1_MNGMSG_USER_INFO_FIND_REQ 1103u ///< 用户信息查询请求
#define G1_MNGMSG_USER_INFO_FIND_ANS 2103u ///< 用户信息查询应答
#define G1_MNGMSG_USER_INFO_SET_REQ 1104u  ///< 用户信息设置请求
#define G1_MNGMSG_USER_INFO_SET_ANS 2104u  ///< 用户信息设置应答

// 用户信息查询请求，web 网关发给后台 fdm,再到 fpga 核心内
struct user_info_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id，web 网关填写，再传给后台。
  uint16_t board_no;                          // 板卡号，web 网关填写，再传给后台。
  int32_t reserved;                           // 填充未使用
};
// 用户描述属性信息
struct gone_user_attr
{
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  uint16_t user_id;                           // 用户 id
  uint16_t board_no;                          // 板卡号
  char bid_flag;                              // 是否可买
  char ask_flag;                              // 是否可卖
  char assignment;                            // 指定交易状态
  char acc_status;                            // 股东账户状态
  char account_id[G1_HOLDERACC_LEN];          // 股东账户
  char offer_branch_id[G1_BRANCHID_LEN];      // 报盘营业部代码
  char pbu_id[G1_EXCHANGE_PBU_LEN];           // 席位
};
// 用户信息查询应答
struct user_info_find_ans
{
  gone_user_attr info;
  int32_t err_code; // 错误代码
  int32_t reserved; // 填充未使用
  char err_msg[G1_ERRMSG_LEN];
};

// 用户信息设置请求，web 网关发给后台 fdm,再到 fpga 核心内
struct user_info_set_req
{
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  uint16_t user_id;                           // 用户 id，web 网关填写，再传给后台。
  uint16_t board_no;                          // 板卡号，web 网关填写，再传给后台。
  char bid_flag;                              // 是否可买
  char ask_flag;                              // 是否可卖
  char assignment;                            // 指定交易状态,保留暂不使用，web 前端不展示
  char acc_status;                            // 股东账户状态,保留暂不使用，web 前端不展示
  char offer_branch_id[G1_BRANCHID_LEN];      // 报盘营业部代码，填空表示不修改
  char pbu_id[G1_EXCHANGE_PBU_LEN];           // 席位，填空表示不修改
};
// 用户信息设置应答
struct user_info_set_ans
{
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  uint16_t user_id;                           // 用户 id
  uint16_t board_no;                          // 板卡号
  int32_t err_code;                           // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

#define G1_MNGMSG_USER_FUND_FIND_REQ 1105u ///< 用户资金查询请求
#define G1_MNGMSG_USER_FUND_FIND_ANS 2105u ///< 用户资金查询应答
#define G1_MNGMSG_USER_FUND_SET_REQ 1106u  ///< 用户资金设置请求
#define G1_MNGMSG_USER_FUND_SET_ANS 2106u  ///< 用户资金设置应答

// 用户资金查询请求，web 网关发给后台 fdm,再到 fpga 核心内
struct user_fund_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id，web 网关填写，再传给后台。
  uint16_t board_no;                          // 板卡号，web 网关填写，再传给后台。
  int32_t reserved;                           // 填充未使用
};
// 用户资金查询应答
struct user_fund_find_ans
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id
  uint16_t board_no;                          // 板卡号
  int32_t err_code;                           // 错误代码
  int64_t avail;                              // 可用资金，放大10000
  int64_t t0;                                 // t0 资金，放大10000
  int64_t t1;                                 // t1 资金，放大10000
  int64_t t2;                                 // t2 资金，放大10000
  int64_t t3;                                 // t3 资金，放大10000
  int64_t tn;                                 // tn 资金，放大10000
  char err_msg[G1_ERRMSG_LEN];
};

// 用户资金设置请求，web 网关发给后台 fdm,再到 fpga 核心内
struct user_fund_set_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id，web 网关填写，再传给后台。
  uint16_t board_no;                          // 板卡号，web 网关填写，再传给后台。
  int32_t reserved;                           // 填充未使用
  int64_t t0_change;                          // t0 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t1_change;                          // t1 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t2_change;                          // t2 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t3_change;                          // t3 资金变动金额，放大10000,正数增加，负数扣减
  int64_t tn_change;                          // tn 资金变动金额，放大10000,正数增加，负数扣减
};
// 用户资金设置应答
struct user_fund_set_ans
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id
  uint16_t board_no;                          // 板卡号
  int32_t err_code;                           // 错误代码
  int64_t avail;                              // 可用资金，放大10000
  int64_t t0;                                 // t0 资金，放大10000
  int64_t t1;                                 // t1 资金，放大10000
  int64_t t2;                                 // t2 资金，放大10000
  int64_t t3;                                 // t3 资金，放大10000
  int64_t tn;                                 // tn 资金，放大10000
  char err_msg[G1_ERRMSG_LEN];
};

#define G1_MNGMSG_USER_HOLD_FIND_REQ 1107u ///< 用户持仓查询请求
#define G1_MNGMSG_USER_HOLD_FIND_ANS 2107u ///< 用户持仓查询应答
#define G1_MNGMSG_USER_HOLD_SET_REQ 1108u  ///< 用户持仓设置请求
#define G1_MNGMSG_USER_HOLD_SET_ANS 2108u  ///< 用户持仓设置应答

// 用户持仓查询请求，web 网关发给后台 fdm,再到 fpga 核心内
struct user_hold_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id，web 网关填写，再传给后台。
  uint16_t board_no;                          // 板卡号，web 网关填写，再传给后台。
  int32_t sec_index;                          // 证券索引，web 网关填写，再传给后台。
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
};
// 用户持仓查询应答
struct user_hold_find_ans
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id
  uint16_t board_no;                          // 板卡号
  int32_t sec_index;                          // 证券索引
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  int64_t avail_qty;                          // 可卖数量
  int64_t init_qty;                           // 日初剩余数量
  int64_t day_buy_qty;                        // 买入成交剩余数量
  int64_t day_pr_qty;                         // ETF申赎成交剩余数量
  int64_t day_ppr_qty;                        // 当日实物ETF申赎成交剩余数量
  int32_t err_code;                           // 错误代码
  int32_t reserved;                           // 填充未使用
  char err_msg[G1_ERRMSG_LEN];
};

// 用户持仓设置请求，web 网关发给后台 fdm,再到 fpga 核心内
struct user_hold_set_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id，web 网关填写，再传给后台。
  uint16_t board_no;                          // 板卡号，web 网关填写，再传给后台。
  int32_t sec_index;                          // 证券索引，web 网关填写，再传给后台。
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  int32_t need_sync;                          // 是否要同步,0-无变动，1-变动（可正负），2-证券不存在
  int32_t reserved;                           // 填充未使用
  int64_t init_change;                        // 日初剩余数量变动
  int64_t day_buy_change;                     // 买入成交剩余数量变动
  int64_t day_pr_change;                      // ETF申赎成交剩余数量变动
  int64_t day_ppr_change;                     // 当日实物ETF申赎成交剩余数量变动
};
// 用户持仓设置应答
struct user_hold_set_ans
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id
  uint16_t board_no;                          // 板卡号
  int32_t sec_index;                          // 证券索引
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  int64_t avail_qty;                          // 可卖数量
  int64_t init_qty;                           // 日初剩余数量
  int64_t day_buy_qty;                        // 买入成交剩余数量
  int64_t day_pr_qty;                         // ETF申赎成交剩余数量
  int64_t day_ppr_qty;                        // 当日实物ETF申赎成交剩余数量
  int32_t err_code;                           // 错误代码
  int32_t reserved;                           // 填充未使用
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 盘中实时查询修改交易所 tgw 地址 ----------
#define G1_MNGMSG_TGW_ADDR_SET_REQ 1111u  ///< 交易所tgw网关地址修改请求
#define G1_MNGMSG_TGW_ADDR_SET_ANS 2111u  ///< 交易所tgw网关地址修改应答
#define G1_MNGMSG_TGW_ADDR_FIND_REQ 1112u ///< 交易所tgw网关地址查询请求
#define G1_MNGMSG_TGW_ADDR_FIND_ANS 2112u ///< 交易所tgw网关地址查询应答
// 交易所tgw 网关地址查询请求，web 网关发给后台 fdm
struct tgw_addr_find_req
{
  uint16_t board_no; ///< FPGA 板卡号,必填,web 前端指定
  int16_t reserved;  // 填充未使用
  int32_t tgw_id;    ///< 板卡内的 tgw 编号,0-全部，web 前端指定
};
// 交易所tgw 网关地址信息
struct tgw_addr_info
{
  uint16_t board_no;              // 板卡号，web 前端指定
  int16_t market_type;            // 市场, 保留未使用
  int32_t tgw_id;                 // 板卡内的 tgw 编号
  int32_t new_tgw_port;           // tgw 监听端口
  int32_t new_tgw_bond_port;      // tgw 的债券监听端口（上交所有效）
  char new_tgw_ip[G1_IPADDR_LEN]; // tgw 监听IP
};
// 交易所tgw 网关地址查询应答
struct tgw_addr_find_ans
{
  uint32_t cur_num; // 当前包返回数量
  int32_t err_code; // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  tgw_addr_info infos[0];
};

// 交易所tgw 网关地址修改请求，web 网关发给后台 fdm
struct tgw_addr_set_req
{
  uint16_t board_no;              // 板卡号，web 前端指定
  int16_t market_type;            // 市场, 保留未使用
  int32_t tgw_id;                 // 板卡内的 tgw 编号
  int32_t new_tgw_port;           // tgw 监听端口
  int32_t new_tgw_bond_port;      // tgw 的债券监听端口（上交所有效）
  char new_tgw_ip[G1_IPADDR_LEN]; // tgw 监听IP
};
// 交易所tgw 网关地址修改应答
struct tgw_addr_set_ans
{
  uint16_t board_no;   // 板卡号，web 前端指定
  int16_t market_type; // 市场
  int32_t tgw_id;      // 板卡内的 tgw 编号
  int32_t err_code;    // 错误代码
  int32_t reserved;    // 填充未使用
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 板卡客户自动补单 ----------
#define G1_MNGMSG_RECOVERY_SWITCH_REQ 1113u ///< 自动补单完成通知客户切换至低延时
#define G1_MNGMSG_RECOVERY_SWITCH_ANS 2113u ///< 自动补单完成通知客户切换至低延时
#define G1_MNGMSG_RECOVERY_END_REQ 1114u    ///< 自动补单结束通知低延时请求
#define G1_MNGMSG_RECOVERY_END_ANS 2114u    ///< 自动补单结束通知低延时应答
#define G1_MNGMSG_ORDER_RECOVERY_REQ 1115u  ///< 自动补单请求
#define G1_MNGMSG_ORDER_RECOVERY_ANS 2115u  ///< 自动补单应答
#define G1_MNGMSG_ORDER_RECOVERY_SYNC 2116u ///< 补单的委托信息推送
// web 前端应支持按客户补单和对板卡上所有客户的补单，但后台仅支持按客户补单。
// 所以对板卡上所有客户的补单，前端应先查出板卡上的所有客户并展示在前端，然后顺序一个个客户发起补单，成功一个客户标识成功一个，直到所有客户成功.
// 当收到自动补单应答成功后，提示用户发起自动补单结束通知低延时请求。
// 也就是需要前端要能做这个流程控制和断点允许用户继续控制。
// 自动补单结束通知低延时成功后，管理员应检查低延时确认是否收到补单完成，若收到，则可接着发起补单完成通知客户切换至低延时的消息。

// 补单完成通知客户切换至低延时的消息请求，web 网关发给后台 fdm
// 若账户信息不空，则为客户；账户信息空，板卡号必须有效，表示板卡上所有客户
struct order_recovery_switch_req
{
  uint16_t board_no;                          // 板卡号，web 前端指定
  uint16_t user_id;                           // 用户 id
  int32_t reserved;                           // 保留未使用
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
};
// 自动补单结束通知低延时应答
struct order_recovery_switch_ans
{
  uint16_t board_no;                          // 板卡号
  uint16_t user_id;                           // 用户 id
  int32_t err_code;                           // 错误代码
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char err_msg[G1_ERRMSG_LEN];
};

// 自动补单结束通知低延时请求，web 网关发给后台 fdm
// 若账户信息不空，则为客户补单完成通知低延时柜台；账户信息空，板卡号必须有效，表示板卡补单完成通知低延时柜台
struct order_recovery_end_req
{
  uint16_t board_no;                          // 板卡号，web 前端指定
  uint16_t user_id;                           // 用户 id
  int32_t reserved;                           // 保留未使用
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
};
// 自动补单结束通知低延时应答
struct order_recovery_end_ans
{
  uint16_t board_no;                          // 板卡号
  uint16_t user_id;                           // 用户 id
  int32_t err_code;                           // 错误代码
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char err_msg[G1_ERRMSG_LEN];
};

// 自动补单请求，web 网关发给后台 fdm
struct order_recovery_req
{
  uint16_t board_no;                          // 板卡号，web 前端指定
  uint16_t user_id;                           // 用户 id
  int32_t begin_time;                         // 补单起始时间，选填，时分秒，web 前端指定
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
};
// 补单的委托信息
struct recovery_order_info
{
  char branch_id[G1_BRANCHID_LEN];              // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN];   // 资金账号
  char security_id[G1_SECURITYID_MAXLEN];       // 证券代码
  int16_t market_type;                          // 市场
  char side;                                    // 买卖标记
  int8_t reserved;                              // 保留未使用
  char order_offer_no[G1_EXCHANGE_OFFERNO_LEN]; // 客户订单编号
  int64_t order_qty;                            // 委托数量
  int64_t order_price;                          // 委托价格，放大10000
  int64_t exec_time;                            // 交易时间
};
// 补单的委托信息推送, fdm 将成功补充的每笔委托分批发给 web 网关，转给 web 前端展示。
struct recovery_order_sync
{
  int32_t total_num;             // 需要补单的总委托数，必填
  int32_t cur_num;               // 本次推送消息中的委托数量，必填，(0,50]
  uint16_t board_no;             // 板卡号，web 前端指定
  int16_t is_last;               // 结束标志：1-结束，0-未结束，必填
  int32_t reserved;              // 填充未使用
  recovery_order_info orders[0]; // 委托，必填
};
// 自动补单应答，fdm 成功补单完成或失败时，发送应答给 web 网关，转给 web 前端。
// 该消息也标识着本次补单请求，后台处理结束，不再会收到该请求的补单的委托信息推送。
struct order_recovery_ans
{
  uint16_t board_no;                          // 板卡号，web 前端指定
  uint16_t user_id;                           // 用户 id
  int32_t last_order_time;                    // 补单最后一笔成功的时间，时分秒
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  int32_t success_num;                        // 补单成功笔数
  int32_t err_code;                           // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

#define G1_MNGMSG_ORDER_RATE_MNG_REQ 1117u  ///< 用户委托笔数风控管理请求，web 网关设置数据库表
#define G1_MNGMSG_ORDER_RATE_MNG_ANS 2117u  ///< 用户委托笔数风控管理应答
#define G1_MNGMSG_ORDER_RATE_FIND_REQ 1118u ///< 用户委托笔数风控查询请求，web 网关查询数据库表
#define G1_MNGMSG_ORDER_RATE_FIND_ANS 2118u ///< 用户委托笔数风控查询应答
#define G1_MNGMSG_ORDER_RATE_SET_REQ 1119u  ///< 用户委托笔数风控实时设置请求，web 网关发给后台 fdm
#define G1_MNGMSG_ORDER_RATE_SET_ANS 2119u  ///< 用户委托笔数风控实时设置应答

// 查询用户委托笔数风控请求，web 网关查询数据库表
//   1. 账户非空，表示按账户查询，此时 board_no 被忽略
//   2. 账户空，board_no 必须有效，表示查板卡上所有客户
//   3. 账户空，board_no = 0，视为错误
struct order_rate_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  int16_t market_type;                        // 市场, 保留未使用
  uint16_t board_no;                          // 板卡号,web 前端指定
};
// 用户委托笔数风控信息
struct user_order_rate_info
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  int16_t market_type;                        // 市场
  int8_t set_stock_rate;                      // 0-否，1-是
  int8_t set_fund_rate;                       // 0-否，1-是
  int8_t set_bond_rate;                       // 0-否，1-是
  int8_t set_stock_num;                       // 0-否，1-是
  int8_t set_fund_num;                        // 0-否，1-是
  int8_t set_bond_num;                        // 0-否，1-是
  int32_t block_order_rate;                   // 每秒委托笔数大于此值阻断，0-表示不控制
  int32_t block_order_num;                    // 最大委托笔数大于此值阻断，0-表示不控制
};
// 查询用户委托笔数风控应答
struct order_rate_find_ans
{
  uint16_t board_no; // 板卡号
  uint16_t cur_num;  // 返回客户委托风控条目数量
  int32_t err_code;  // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  user_order_rate_info user_infos[0];
};

// 添加，修改，删除用户委托笔数风控请求，web 网关设置数据库表
struct order_rate_mng_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  int32_t op_type;                            // 操作类型：1-添加，2-修改，3-删除
  int16_t market_type;                        // 市场, 保留未使用
  int8_t set_stock_rate;                      // 0-否，1-是
  int8_t set_fund_rate;                       // 0-否，1-是
  int8_t set_bond_rate;                       // 0-否，1-是
  int8_t set_stock_num;                       // 0-否，1-是
  int8_t set_fund_num;                        // 0-否，1-是
  int8_t set_bond_num;                        // 0-否，1-是
  int32_t block_order_num;                    // 最大委托笔数大于此值阻断，0-表示不控制
  int32_t block_order_rate;                   // 每秒委托笔数大于此值阻断，0-表示不控制
};
// 添加，修改，删除用户委托笔数风控应答
struct order_rate_mng_ans
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  int32_t op_type;                            // 操作类型：1-添加，2-修改，3-删除
  int32_t err_code;                           // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// 实时添加修改用户委托笔数风控设置请求，web 网关发给后台 fdm,再到 fpga 核心内
//  fdm 需要将新的设置落库
struct order_rate_set_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t user_id;                           // 用户 id，web 网关填写，再传给后台。
  uint16_t board_no;                          // 板卡号，web 网关填写，再传给后台。
  int16_t market_type;                        // 市场, 保留未使用，web 网关补充
  int8_t set_stock_rate;                      // 0-否，1-是
  int8_t set_fund_rate;                       // 0-否，1-是
  int8_t set_bond_rate;                       // 0-否，1-是
  int8_t set_stock_num;                       // 0-否，1-是
  int8_t set_fund_num;                        // 0-否，1-是
  int8_t set_bond_num;                        // 0-否，1-是
  int32_t block_order_rate;                   // 每秒委托笔数大于此值阻断，0-表示不控制
  int32_t block_order_num;                    // 最大委托笔数大于此值阻断，0-表示不控制
};
// 实时添加修改用户委托笔数风控设置应答
struct order_rate_set_ans
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t board_no;                          // 板卡号
  uint16_t user_id;                           // 用户 id
  int32_t err_code;                           // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 板卡上客户列表 ----------
#define G1_MNGMSG_BOARD_USER_QRY_REQ 1121u ///< 板卡客户查询请求
#define G1_MNGMSG_BOARD_USER_QRY_ANS 2121u ///< 板卡客户查询应答

// 板卡客户查询请求，web 网关直接查询内存缓存
struct board_user_qry_req
{
  uint16_t board_no;   ///< FPGA 板卡号，web 前端指定
  int16_t market_type; // 市场, 保留未使用
};
// 板卡客户信息
struct gone_user_base
{
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  uint16_t user_id;                           // 用户 id
  uint16_t board_no;                          // 板卡号
  int16_t market_type;                        // 市场
  int16_t reserved;                           // 填充未使用
};
// 板卡客户查询应答
struct board_user_qry_ans
{
  int32_t cur_num;  // 客户个数
  int32_t err_code; // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  gone_user_base users[0]; // 用户标识信息 todo 改为客户信息数组
};

// ---------- 用户费率信息表 t_UserFeeInfo ----------
#define G1_MNGMSG_USER_FEE_FIND_REQ 1122u ///< 用户费率查询请求
#define G1_MNGMSG_USER_FEE_FIND_ANS 2122u ///< 用户费率查询应答

// 用户费率查询请求，web 网关查询数据库表（当日）：
// 账号必填按客户查；证券类别 0-全部类别
struct user_fee_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  int16_t market_type;                        // 市场，web 前端传入
  int16_t reserved;                           // 填充未使用
  int32_t security_type;                      // 证券类别，0-全部类别
};
// 用户费率信息（当日）
struct user_fee_info
{
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  uint16_t board_no;                          // 板卡号
  int16_t market_type;                        // 市场
  int32_t security_type;                      // 证券类别
  int32_t bid_fee_rate;                       // 买入费率，放大10000
  int32_t ask_fee_rate;                       // 卖出费率，放大10000
};
// 用户费率查询应答
struct user_fee_find_ans
{
  uint32_t cur_num; // 当前包返回数量
  int32_t err_code; // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  user_fee_info infos[0];
};

// ---------- 板卡状态信息表 t_FpgaCounterStatus ----------
#define G1_MNGMSG_BOARD_STATUS_FIND_REQ 1130u ///< 板卡状态查询请求
#define G1_MNGMSG_BOARD_STATUS_FIND_ANS 2130u ///< 板卡状态查询应答
#define G1_MNGMSG_BOARD_STATUS_SET_REQ 1131u  ///< 板卡状态设置请求
#define G1_MNGMSG_BOARD_STATUS_SET_ANS 2131u  ///< 板卡状态设置应答
#define G1_MNGMSG_BOARD_OFFLINE_SET_REQ 1132u ///< 板卡下线设置请求
#define G1_MNGMSG_BOARD_OFFLINE_SET_ANS 2132u ///< 板卡下线设置应答
// 板卡状态查询请求，web 网关查询数据库表，板卡号为 0 表示全部板卡
struct board_status_find_req
{
  uint16_t board_no; ///< FPGA 板卡号，web 前端指定，0-全部
  uint16_t reserved; // 填充未使用
};
// 板卡状态信息（当日）
struct board_status_info
{
  uint16_t board_no;              // 板卡号
  int16_t market_type;            // 市场
  int32_t fgw_port;               // FGW 网关端口
  char fgw_ip[G1_IPADDR_LEN];     // FGW 网关 IP 地址
  char manager_ip[G1_IPADDR_LEN]; // 管理端 IP 地址（后台 fdm 监听）
  int32_t manager_port;           // 管理端端口（后台 fdm 监听）
  char data_status;               // 上场数据状态：0-未上场，1-上场完成，2-上场失败
  char force_reload_data;         // 是否强制重新上场数据：0-否，1-仅更新证券和客户基础信息，2-全部数据
  char business_status;           // 业务状态：0-未发生过业务，1-已发生过业务
  char fpga_status;               // 1-FPGA 正常，2-FPGA 异常
  int32_t data_upload_time;       // 数据上场完成时间，时分秒
  int32_t fpga_offline_time;      // fpga 异常时间，时分秒
};
// 板卡状态查询应答
struct board_status_find_ans
{
  uint32_t cur_num; // 当前包返回数量
  int32_t err_code; // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  board_status_info infos[0];
};
// 板卡状态设置请求，设置 ForceReloadData 和 business_status 字段（强制板卡重新上场数据）
struct board_status_set_req
{
  uint16_t board_no;      ///< FPGA 板卡号，web 前端指定
  char force_reload_data; // 0-否，1-仅更新证券和客户基础信息，2-全部数据
  char business_status;   // 业务状态：0-未发生过业务，1-已发生过业务
};
// 板卡状态设置应答
struct board_status_set_ans
{
  uint16_t board_no; // 板卡号
  uint16_t reserved; // 填充未使用
  int32_t err_code;  // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};
// 板卡下线设置请求，设置 fpga_status
struct board_offline_set_req
{
  uint16_t board_no;  ///< FPGA 板卡号，web 前端指定
  int16_t is_offline; ///< 1-下线
};
// 板卡下线设置应答
struct board_offline_set_ans
{
  uint16_t board_no;  // 板卡号
  int16_t is_offline; ///< 1-下线
  int32_t err_code;   // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 板卡属性表 t_BoardAttr ----------
#define G1_MNGMSG_BOARD_ATTR_FIND_REQ 1133u ///< 板卡属性查询请求
#define G1_MNGMSG_BOARD_ATTR_FIND_ANS 2133u ///< 板卡属性查询应答
#define G1_MNGMSG_BOARD_ATTR_SET_REQ 1134u  ///< 板卡属性设置请求
#define G1_MNGMSG_BOARD_ATTR_SET_ANS 2134u  ///< 板卡属性设置应答

// 板卡属性查询请求，web 网关查询数据库表，板卡号为 0 表示全部板卡
struct board_attr_find_req
{
  uint16_t board_no; ///< FPGA 板卡号，web 前端指定，0-全部
  uint16_t reserved; // 填充未使用
};
// 板卡属性信息
struct board_attr_info
{
  uint16_t board_no;                               // 板卡号
  int16_t market_type;                             // 市场
  int32_t max_user_num;                            // 最大客户数
  int32_t max_conn_num;                            // 最大连接数
  int32_t max_security_num;                        // 最大证券数
  int32_t update_date;                             // 更新日期
  int32_t update_time;                             // 更新时间
  char offer_no_prefix[G1_OFFERNO_PREFIX_LEN + 1]; // 报单编号前三位
};
// 板卡属性查询应答
struct board_attr_find_ans
{
  uint32_t cur_num; // 当前包返回数量
  int32_t err_code; // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  board_attr_info infos[0];
};
// 板卡属性设置请求，web 网关设置数据库表。单行键表（按板卡号）
struct board_attr_set_req
{
  int32_t op_type;                                 // 操作类型：1-添加，2-修改，3-删除
  uint16_t board_no;                               // 板卡号
  int16_t market_type;                             // 市场
  int32_t max_user_num;                            // 最大客户数
  int32_t max_conn_num;                            // 最大连接数
  int32_t max_security_num;                        // 最大证券数
  char offer_no_prefix[G1_OFFERNO_PREFIX_LEN + 1]; // 报单编号前三位
};
// 板卡属性设置应答
struct board_attr_set_ans
{
  uint16_t board_no;   // 板卡号
  int16_t market_type; // 市场
  int32_t err_code;    // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 交易所网关配置信息表 t_ExchangeGateInfo ----------
#define G1_MNGMSG_EXCH_GW_CFG_FIND_REQ 1135u ///< 交易所网关配置查询请求
#define G1_MNGMSG_EXCH_GW_CFG_FIND_ANS 2135u ///< 交易所网关配置查询应答
#define G1_MNGMSG_EXCH_GW_CFG_SET_REQ 1136u  ///< 交易所网关配置设置请求
#define G1_MNGMSG_EXCH_GW_CFG_SET_ANS 2136u  ///< 交易所网关配置设置应答

// 交易所网关配置查询请求，web 网关查询数据库表
// 网关 ID 为 0 表示该板卡全部网关
struct exch_tgw_cfg_find_req
{
  uint16_t board_no;      ///< FPGA 板卡号，web 前端指定
  int16_t reserved;       // 填充未使用
  int32_t exchange_gw_id; ///< 交易所网关 ID，0-全部
};
// 交易所网关配置信息
struct exch_tgw_cfg_info
{
  uint16_t board_no;                         // 板卡号
  int16_t market_type;                       // 市场
  int32_t exchange_gw_id;                    // 交易所网关 ID
  int32_t sh_bond_port;                      // 沪债端口
  int32_t port;                              // 交易端口
  char ip[G1_IPADDR_LEN];                    // IP 地址
  int32_t heart_interval;                    // 心跳间隔
  int32_t update_date;                       // 更新日期
  int32_t update_time;                       // 更新时间
  int32_t reserved;                          // 填充未使用
  char log_comp_id[G1_EXCHANGE_SENDCMP_LEN]; // 交易所网关登录的发送方代码
  char log_passwd[G1_EXCHANGE_PASSWD_LEN];   // 交易所网关登录的密码（页面呈现须脱敏）
};
// 交易所网关配置查询应答
struct exch_tgw_cfg_find_ans
{
  uint32_t cur_num; // 当前包返回数量
  int32_t err_code; // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  exch_tgw_cfg_info infos[0];
};
// 交易所网关配置设置请求，web 网关设置数据库表
struct exch_tgw_cfg_set_req
{
  int32_t op_type;                           // 操作类型：1-添加，2-修改，3-删除
  uint16_t board_no;                         // 板卡号
  int16_t market_type;                       // 市场
  int32_t exchange_gw_id;                    // 交易所网关 ID
  int32_t sh_bond_port;                      // 沪债端口
  int32_t port;                              // 交易端口
  int32_t heart_interval;                    // 心跳间隔
  char ip[G1_IPADDR_LEN];                    // IP 地址
  char log_comp_id[G1_EXCHANGE_SENDCMP_LEN]; // 交易所网关登录的发送方代码
  char log_passwd[G1_EXCHANGE_PASSWD_LEN];   // 交易所网关登录的密码（页面呈现须脱敏）
};
// 交易所网关配置设置应答
struct exch_tgw_cfg_set_ans
{
  int32_t op_type;        // 操作类型
  uint16_t board_no;      // 板卡号
  int16_t market_type;    // 市场
  int32_t exchange_gw_id; // 交易所网关 ID
  int32_t err_code;       // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 交易所席位与网关映射配置表 t_PBUGateInfo ----------
#define G1_MNGMSG_PBU_GW_MAP_FIND_REQ 1137u ///< 席位与网关映射查询请求
#define G1_MNGMSG_PBU_GW_MAP_FIND_ANS 2137u ///< 席位与网关映射查询应答
#define G1_MNGMSG_PBU_GW_MAP_SET_REQ 1138u  ///< 席位与网关映射设置请求
#define G1_MNGMSG_PBU_GW_MAP_SET_ANS 2138u  ///< 席位与网关映射设置应答

// 席位与网关映射查询请求，web 网关查询数据库表
struct pbu_gw_map_find_req
{
  uint16_t board_no; ///< FPGA 板卡号，web 前端指定
  uint16_t reserved; // 填充未使用
};
// 席位与网关映射信息
struct pbu_gw_map_info
{
  uint16_t board_no;                // 板卡号
  int16_t market_type;              // 市场
  int32_t exchange_gw_id;           // 交易所网关 ID
  char pbu_id[G1_EXCHANGE_PBU_LEN]; // 席位
};
// 席位与网关映射查询应答
struct pbu_gw_map_find_ans
{
  uint32_t cur_num; // 当前包返回数量
  int32_t err_code; // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  pbu_gw_map_info infos[0];
};
// 席位与网关映射设置请求，web 网关设置数据库表
struct pbu_gw_map_set_req
{
  int16_t op_type;                  // 操作类型：1-添加，2-修改，3-删除
  uint16_t board_no;                // 板卡号
  int32_t exchange_gw_id;           // 交易所网关 ID
  char pbu_id[G1_EXCHANGE_PBU_LEN]; // 席位
};
// 席位与网关映射设置应答
struct pbu_gw_map_set_ans
{
  int16_t op_type;   // 操作类型
  uint16_t board_no; // 板卡号
  int32_t err_code;  // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 系统配置信息表 t_SysTradeCfg ----------
#define G1_MNGMSG_SYS_CFG_FIND_REQ 1139u ///< 系统配置查询请求
#define G1_MNGMSG_SYS_CFG_FIND_ANS 2139u ///< 系统配置查询应答
#define G1_MNGMSG_SYS_CFG_SET_REQ 1140u  ///< 系统配置设置请求
#define G1_MNGMSG_SYS_CFG_SET_ANS 2140u  ///< 系统配置设置应答

// 系统配置查询请求，web 网关查询数据库表
struct sys_cfg_find_req
{
  int16_t market_type; ///< 市场，web 前端指定
  int16_t reserved;    // 填充未使用
};
// 系统配置信息
struct sys_cfg_info
{
  int16_t market_type; // 市场
  int16_t reserved;    // 填充未使用
  int32_t min_fee_amt; // 最小费用金额，放大10000
  int32_t update_date; // 更新日期
  int32_t update_time; // 更新时间
};
// 系统配置查询应答
struct sys_cfg_find_ans
{
  int16_t market_type; // 市场
  int16_t reserved;    // 填充未使用
  int32_t min_fee_amt; // 最小费用金额，放大10000
  int32_t update_date; // 更新日期
  int32_t update_time; // 更新时间
  int32_t err_code;    // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};
// 系统配置设置请求，web 网关设置数据库表。单行键表（按市场）
struct sys_cfg_set_req
{
  int16_t op_type;     // 操作类型：1-添加，2-修改
  int16_t market_type; // 市场
  int32_t min_fee_amt; // 最小费用金额，放大10000
};
// 系统配置设置应答
struct sys_cfg_set_ans
{
  int16_t op_type;     // 操作类型：1-添加，2-修改
  int16_t market_type; // 市场
  int32_t min_fee_amt; // 最小费用金额，放大10000
  int32_t err_code;    // 错误代码
  char err_msg[G1_ERRMSG_LEN];
};

// ---------- 客户登录流水表 t_TradeLogin ----------
#define G1_MNGMSG_TRADE_LOGIN_FIND_REQ 1141u ///< 客户登录流水查询请求
#define G1_MNGMSG_TRADE_LOGIN_FIND_ANS 2141u ///< 客户登录流水查询应答

// 客户登录流水查询请求，web 网关查询数据库表（当日）
//   1. 账户非空，表示按账户查询，此时 board_no 被忽略
//   2. 账户空，board_no 必须有效，表示查板卡上全部客户
//   3. 账户空，board_no = 0，视为错误
// 分页查询，每页返回条目不可大于50
struct trade_login_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t board_no;                          // 板卡号，web 前端指定
  uint16_t per_page_its;                      // 每页返回条目数量,不可大于50
  uint32_t page_no;                           // 第几页
};
// 客户登录流水信息（当日）
struct trade_login_info
{
  char cust_id[G1_CUSTID_LEN];                // 客户号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char agw_session[G1_SESSION_LEN];           // AGW 会话信息
  char end_code[G1_CUST_END_LEN];             // 终端信息
  char order_way[G1_ORDERWAY_LEN];            // 委托通道类型
  int16_t log_type;                           // 1-用户，2-网关
  int16_t market_type;                        // 市场
  int32_t heart_beat_int;                     // 心跳时间
  int32_t session_id;                         // 会话号
  int32_t proto_type;                         // 通信协议
  char trade_ip[G1_IPADDR_LEN];               // 交易 IP，非直连为空
  int32_t trade_port;                         // 交易端口，非直连为 0
  uint16_t board_no;                          // 板卡号
  int16_t reserved;                           // 填充未使用
  char version[G1_VERSION_LEN];               // 后台版本号
  int32_t login_time;                         // 登录时间，秒
  int32_t err_code;                           // 错误码
  char err_msg[G1_ERRMSG_LEN];                // 错误信息
};
// 客户登录流水查询应答
struct trade_login_find_ans
{
  int32_t is_last;       // 是否结束
  uint16_t cur_num;      // 当前包返回数量
  uint16_t per_page_its; // 每页返回条目数量
  uint32_t page_no;      // 第几页
  int32_t err_code;      // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  trade_login_info infos[0];
};

// ---------- 客户直连会话连接分配表 t_UserDirectSession ----------
#define G1_MNGMSG_USER_DIRECT_SESSION_FIND_REQ 1142u ///< 客户直连会话查询请求
#define G1_MNGMSG_USER_DIRECT_SESSION_FIND_ANS 2142u ///< 客户直连会话查询应答

// 客户直连会话查询请求，web 网关查询数据库表（当日）：
//   1. 账户非空，表示按账户查询，此时 board_no 被忽略
//   2. 账户空，board_no 必须有效，表示查板卡上全部客户
//   3. 账户空，board_no = 0，视为错误
// 分页查询，每页返回条目不可大于50
struct user_direct_session_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  uint16_t board_no;                          // 板卡号，web 前端指定
  uint16_t per_page_its;                      // 每页返回条目数量,不可大于50
  uint32_t page_no;                           // 第几页
};
// 客户直连会话连接信息（当日）
struct user_direct_session_info
{
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  char agw_session[G1_SESSION_LEN];           // AGW 会话信息
  int32_t session_id;                         // 内部会话号
  int32_t cid;                                // 连接 ID
  uint16_t board_no;                          // 板卡号
  char addr_status;                           // 地址状态
  char reserved;                              // 填充未使用
  int32_t trade_port;                         // 交易端口
  char trade_ip[G1_IPADDR_LEN];               // 交易 IP
};
// 客户直连会话查询应答
struct user_direct_session_find_ans
{
  int32_t is_last;       // 是否结束
  uint16_t cur_num;      // 当前包返回数量
  uint16_t per_page_its; // 每页返回条目数量
  uint32_t page_no;      // 第几页
  int32_t err_code;      // 错误代码
  char err_msg[G1_ERRMSG_LEN];
  user_direct_session_info infos[0];
};

// ---------- 委托流水表 t_OrderTick ----------
#define G1_MNGMSG_ORDER_TICK_FIND_REQ 1143u ///< 委托流水分页查询请求
#define G1_MNGMSG_ORDER_TICK_FIND_ANS 2143u ///< 委托流水分页查询应答

// 委托流水分页查询请求，web 网关查询数据库表（当日）：
//   1. 柜台委托号非 0，按系统委托号直查（单笔，其余过滤条件忽略）
//   2. 柜台委托号为 0，按客户查（账号必填），证券代码空表示全部证券
// 分页查询，每页返回条目不可大于50
struct order_tick_find_req
{
  int64_t order_sys_no;                       ///< 柜台委托号，非 0-按委托号直查
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; ///< 资金账号
  char branch_id[G1_BRANCHID_LEN];            ///< 分支机构
  char security_id[G1_SECURITYID_MAXLEN];     ///< 证券代码，空-全部
  int16_t market_type;                        ///< 市场，web 前端传入
  uint16_t per_page_its;                      ///< 每页返回条目数量,不可大于50
  uint32_t page_no;                           ///< 第几页
};
// 委托流水信息（当日，t_OrderTick 全字段镜像，不含交易日）
struct order_tick_info
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN];   // 资金账号
  char branch_id[G1_BRANCHID_LEN];              // 分支机构
  uint16_t user_id;                             // 用户索引
  uint16_t board_no;                            // 板卡号
  uint16_t sec_index;                           // 证券索引
  char side;                                    // 买卖方向
  char order_type;                              // 市价限价标记
  char security_id[G1_SECURITYID_MAXLEN];       // 证券代码
  int64_t order_qty;                            // 委托数量
  int64_t order_price;                          // 委托价格，放大10000
  int64_t cust_req_no;                          // 客户私有请求号（= user_seq_no）
  uint16_t policy_id;                           // 策略佣金ID，fpga不用存
  int16_t need_to_offer;                        // 是否需要低延时报盘
  int32_t err_code;                             // 错误码
  int64_t order_sys_no;                         // 柜台委托号
  int64_t order_time;                           // 委托时间
  int64_t frozen_fund;                          // 冻结资金，放大10000
  int64_t frozen_fee;                           // 冻结费用，放大10000
  int64_t day_ppr_change;                       // 当日实物 ETF 申赎成交剩余数量变动
  int64_t day_pr_change;                        // ETF 申赎成交剩余数量变动
  int64_t day_buy_change;                       // 买入成交剩余数量变动
  int64_t init_change;                          // 日初剩余数量变动
  int32_t tgw_rej_code;                         // TGW 拒绝错误码
  char order_offer_no[G1_EXCHANGE_OFFERNO_LEN]; // 委托报盘合同号
  char exch_order_id[G1_EXCHANGE_ORDERID_LEN];  // 交易所订单编号
  int64_t report_index;                         // 回报索引
  int64_t report_time;                          // 回报时间
  int32_t tgw_id;                               // 用户指定 TGW 编号
  int32_t platform_id;                          // 平台 ID
  int32_t partition_no;                         // 分区号
  int32_t offer_tgw_id;                         // 报单的 TGW 编号
  int64_t tick_count;                           // Tick 计数
};
// 委托流水分页查询应答
struct order_tick_find_ans
{
  int32_t is_last;       ///< 1-已到末页，0-还有后续
  uint16_t cur_num;      ///< 当前包返回数量
  uint16_t per_page_its; ///< 每页返回条目数量
  uint32_t page_no;      ///< 第几页
  int32_t err_code;      ///< 错误代码
  char err_msg[G1_ERRMSG_LEN];
  order_tick_info infos[0];
};

// ---------- 成交流水表 t_Trade ----------
#define G1_MNGMSG_TRADE_FIND_REQ 1144u ///< 成交流水分页查询请求
#define G1_MNGMSG_TRADE_FIND_ANS 2144u ///< 成交流水分页查询应答

// 成交流水分页查询请求，web 网关查询数据库表（当日），过滤规则同 1143
//   1. 柜台委托号非 0，按系统委托号直查（单笔，其余过滤条件忽略）
//   2. 柜台委托号为 0，按客户查（账号必填），证券代码空表示全部证券
// 分页查询，每页返回条目不可大于50
struct trade_find_req
{
  int64_t order_sys_no;                       ///< 柜台委托号，非 0-按委托号直查
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; ///< 资金账号
  char branch_id[G1_BRANCHID_LEN];            ///< 分支机构
  char security_id[G1_SECURITYID_MAXLEN];     ///< 证券代码，空-全部
  int16_t market_type;                        ///< 市场，web 前端传入
  uint16_t per_page_its;                      ///< 每页返回条目数量,不可大于50
  uint32_t page_no;                           ///< 第几页
};
// 成交流水信息（当日，t_Trade 全字段镜像，不含交易日）
struct trade_info
{
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  uint16_t user_id;                           // 用户索引
  uint16_t board_no;                          // 板卡号
  uint16_t sec_index;                         // 证券索引
  char side;                                  // 买卖方向
  char order_type;                            // 市价限价标记
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  int64_t order_qty;                          // 委托数量
  int64_t order_price;                        // 委托价格，放大10000
  int64_t cust_req_no;                        // 客户私有请求号
  int32_t orde_status;                        // 委托状态
  int32_t rtn_type;                           // 回报类型 (AgwExecType 字典: '0'委托确认/'4'撤单成交/'8'废单/'F'成交, 见 g1trademsg.h G1_RSP_TYPE_*)
  int64_t order_sys_no;                       // 柜台委托号
  char exec_id[G1_EXECID_LEN];                // 交易所成交编号
  int64_t exec_price;                         // 成交价格，放大10000
  int64_t exec_qty;                           // 成交数量
  int64_t exec_amount;                        // 成交金额，放大10000
  int64_t exec_fee;                           // 单笔成交费用，放大10000
  int64_t trade_amount;                       // 累计成交金额，放大10000
  int64_t trade_qty;                          // 累计成交数量
  int64_t cancel_qty;                         // 撤单成交数量
  int64_t total_fee;                          // 累计费用，放大10000
  int64_t exec_time;                          // 成交时间，取交易所时间
  int32_t err_code;                           // 错误码
  int32_t partition_no;                       // 分区号
  int64_t report_index;                       // 回报索引
  int64_t report_time;                        // 回报时间
  int64_t session_seq_no;                     // 后台会话可靠消息流水号
};
// 成交流水分页查询应答
struct trade_find_ans
{
  int32_t is_last;       ///< 1-已到末页，0-还有后续
  uint16_t cur_num;      ///< 当前包返回数量
  uint16_t per_page_its; ///< 每页返回条目数量
  uint32_t page_no;      ///< 第几页
  int32_t err_code;      ///< 错误代码
  char err_msg[G1_ERRMSG_LEN];
  trade_info infos[0];
};

// ---------- 验资验券表 t_FundHoldChange ----------
#define G1_MNGMSG_FUND_HOLD_CHANGE_FIND_REQ 1145u ///< 验资验券分页查询请求
#define G1_MNGMSG_FUND_HOLD_CHANGE_FIND_ANS 2145u ///< 验资验券分页查询应答

// 验资验券分页查询请求，web 网关查询数据库表（当日）：
// 账号必填按客户查，证券代码空表示全部证券
// 分页查询，每页返回条目不可大于50
struct fund_hold_change_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; ///< 资金账号
  char branch_id[G1_BRANCHID_LEN];            ///< 分支机构
  char security_id[G1_SECURITYID_MAXLEN];     ///< 证券代码，空-全部
  int16_t market_type;                        ///< 市场，web 前端传入
  uint16_t per_page_its;                      ///< 每页返回条目数量,不可大于50
  uint32_t page_no;                           ///< 第几页
};
// 验资验券信息（当日，t_FundHoldChange 全字段镜像，不含交易日）
struct fund_hold_change_info
{
  int64_t req_no;                             // 请求号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  int32_t fund_type;                          // 资金类型
  int32_t fund_flag;                          // 资金标志
  int64_t change_amt;                         // 资金变化，放大10000
  int32_t hold_type;                          // 持仓变化类型
  int16_t market_type;                        // 市场
  uint16_t security_index;                    // 证券索引
  int64_t qty_change;                         // 持仓数量变化
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  char rule[G1_RULE_LEN];                     // 规则
  int32_t err_code;                           // 错误代码
  int32_t reserved;                           // 保留未使用
  int64_t t0_change;                          // t0 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t1_change;                          // t1 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t2_change;                          // t2 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t3_change;                          // t3 资金变动金额，放大10000,正数增加，负数扣减
  int64_t tn_change;                          // tn 资金变动金额，放大10000,正数增加，负数扣减
  int64_t init_change;                        // 日初剩余数量变动
  int64_t day_buy_change;                     // 买入成交剩余数量变动
  int64_t day_pr_change;                      // ETF申赎成交剩余数量变动
  int64_t day_ppr_change;                     // 当日实物ETF申赎成交剩余数量变动
};
// 验资验券分页查询应答
struct fund_hold_change_find_ans
{
  int32_t is_last;       ///< 1-已到末页，0-还有后续
  uint16_t cur_num;      ///< 当前包返回数量
  uint16_t per_page_its; ///< 每页返回条目数量
  uint32_t page_no;      ///< 第几页
  int32_t err_code;      ///< 错误代码
  char err_msg[G1_ERRMSG_LEN];
  fund_hold_change_info infos[0];
};

// ---------- 资金股份变动同步表 t_FundHoldSync ----------
#define G1_MNGMSG_FUND_HOLD_SYNC_FIND_REQ 1146u ///< 资金股份变动同步分页查询请求
#define G1_MNGMSG_FUND_HOLD_SYNC_FIND_ANS 2146u ///< 资金股份变动同步分页查询应答

// 资金股份变动同步分页查询请求，web 网关查询数据库表（当日），过滤规则同 1145
// 账号必填按客户查，证券代码空表示全部证券
// 分页查询，每页返回条目不可大于50
struct fund_hold_sync_find_req
{
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; ///< 资金账号
  char branch_id[G1_BRANCHID_LEN];            ///< 分支机构
  char security_id[G1_SECURITYID_MAXLEN];     ///< 证券代码，空-全部
  int16_t market_type;                        ///< 市场，web 前端传入
  uint16_t per_page_its;                      ///< 每页返回条目数量,不可大于50
  uint32_t page_no;                           ///< 第几页
};
// 资金股份变动同步信息（当日，t_FundHoldSync 全字段镜像，不含交易日）
struct fund_hold_sync_info
{
  int64_t req_no;                             // 请求号
  char branch_id[G1_BRANCHID_LEN];            // 分支机构
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; // 资金账号
  int32_t need_sync;                          // 是否要同步
  int16_t market_type;                        // 市场
  uint16_t security_index;                    // 证券索引
  char security_id[G1_SECURITYID_MAXLEN];     // 证券代码
  int64_t t0_change;                          // t0 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t1_change;                          // t1 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t2_change;                          // t2 资金变动金额，放大10000,正数增加，负数扣减
  int64_t t3_change;                          // t3 资金变动金额，放大10000,正数增加，负数扣减
  int64_t tn_change;                          // tn 资金变动金额，放大10000,正数增加，负数扣减
  int64_t init_change;                        // 日初剩余数量变动
  int64_t day_buy_change;                     // 买入成交剩余数量变动
  int64_t day_pr_change;                      // ETF申赎成交剩余数量变动
  int64_t day_ppr_change;                     // 当日实物ETF申赎成交剩余数量变动
};
// 资金股份变动同步分页查询应答
struct fund_hold_sync_find_ans
{
  int32_t is_last;       ///< 1-已到末页，0-还有后续
  uint16_t cur_num;      ///< 当前包返回数量
  uint16_t per_page_its; ///< 每页返回条目数量
  uint32_t page_no;      ///< 第几页
  int32_t err_code;      ///< 错误代码
  char err_msg[G1_ERRMSG_LEN];
  fund_hold_sync_info infos[0];
};

#endif // G1MANAGEMSG_H

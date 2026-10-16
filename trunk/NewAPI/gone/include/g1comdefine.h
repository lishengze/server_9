#ifndef G1COMDEFINE_H
#define G1COMDEFINE_H

// 版本格式定义：<协议兼容性>.<业务功能扩展>.<bug修复>
static constexpr char g1_msg_ver[] = "1.0.0";

// 市场ID
#define G1_MARKET_SH 101 // 上交所市场id
#define G1_MARKET_SZ 102 // 深交所市场id

// 消息长度上限 (单消息最大长度, 修改需谨慎 — 协议版本兼容)
#define G1_MSG_MAX_LEN 32700

// 长度定义
#define G1_SECURITYID_MAXLEN 8     // 证券代码长度
#define G1_BRANCHID_LEN 12         // 客户营业部长度
#define G1_FUNDACCOUNTID_LEN 16    // 客户资金长度
#define G1_CUSTID_LEN 16           // 客户代码长度
#define G1_HOLDERACC_LEN 12        // 客户股东账户长度
#define G1_CUST_PASSWORD_LEN 256   // 客户密码长度
#define G1_CUST_END_LEN 1024       // 客户终端信息长度
#define G1_SESSION_LEN 32          // 客户会话信息长度
#define G1_VERSION_LEN 64          // 版本信息长度
#define G1_IPADDR_LEN 16           // ip地址长度
#define G1_ERRMSG_LEN 128          // 错误信息长度
#define G1_EXECID_LEN 16           // 成交编号长度
#define G1_EXCHANGE_OFFERNO_LEN 12 // 交易所报盘委托的报盘合同号长度
#define G1_ORDERWAY_LEN 2          // 委托通道类型长度(t_TradeLogin.OrderWay)
#define G1_EXCHANGE_ORDERID_LEN 16 // 交易所订单编号长度(t_OrderTick.ExchOrderID)

#endif // G1COMDEFINE_H

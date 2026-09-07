#ifndef G1COMDEFINE_H
#define G1COMDEFINE_H

//消息长度上限 (单消息最大长度, 修改需谨慎 — 协议版本兼容)
#define G1_MSG_MAX_LEN 2000

//长度定义
#define G1_SECURITYID_MAXLEN    8    //证券代码长度
#define G1_BRANCHID_LEN         12   //客户营业部长度
#define G1_FUNDACCOUNTID_LEN    16   //客户资金长度
#define G1_CUSTID_LEN           16   //客户代码长度
#define G1_HOLDERACC_LEN        12   //客户股东账户长度
#define G1_CUST_PASSWORD_LEN    64   //客户密码长度
#define G1_CUST_END_LEN         1024 //客户终端信息长度
#define G1_SESSION_LEN          32   //客户会话信息长度
#define G1_VERSION_LEN          64   //版本信息长度
#define G1_IPADDR_LEN           16   //ip地址长度
#define G1_ERRMSG_LEN           64   //错误信息长度
#define G1_EXECID_LEN           16   //成交编号长度
#define G1_EXCHANGE_OFFERNO_LEN 12   //交易所报盘委托的报盘合同号长度
#define G1_EXCHANGE_PBU_LEN     8    //交易所PBU 长度
#define G1_EXCHANGE_SENDCMP_LEN 32   //交易所网关登录的发送方代码长度
#define G1_EXCHANGE_PASSWD_LEN  16   //交易所网关登录的密码长度
#define G1_EXCHANGE_BRANCH_LEN  8    //交易所报盘的营业部长度

#endif //G1COMDEFINE_H

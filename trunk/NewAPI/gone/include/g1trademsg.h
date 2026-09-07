#ifndef G1TRADEMSG_H
#define G1TRADEMSG_H
#include "g1msghead.h"

// api 请求消息
#define G1_MSG_ORDER_REQ    1001u ///< 委托请求
#define G1_MSG_CANCEL_REQ   1002u ///< 撤单请求
#define G1_MSG_HEART_REQ    1003u ///< 心跳请求
#define G1_MSG_SEC_INFO_REQ 1004u ///< 证券信息请求
#define G1_MSG_LOGIN_REQ    1005u ///< 登录请求
// 后台应答推送消息
#define G1_MSG_ORDER_RTN    2001u ///< 委托回报
#define G1_MSG_TRADE_RTN    2002u ///< 成交推送
#define G1_MSG_CANCEL_RSP   2003u ///< 撤单响应
#define G1_MSG_HEART_ANS    2004u ///< 心跳响应
#define G1_MSG_SEC_INFO_ANS 2005u ///< 证券信息推送
#define G1_MSG_LOGIN_ANS    2006u ///< 登录响应
#define G1_MSG_OFFLINE_PUSH 2007u ///< 板卡客户状态推送
#define G1_MSG_GW_REJ       2008u ///< 网关路由拒绝

// 登录消息
struct login_req {
  int64_t cust_req_no;                        ///< 客户私有请求号
  char cust_id[G1_CUSTID_LEN];                //客户号
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; //资金账号，前16字节有效
  char branch_id[G1_BRANCHID_LEN];            //分支机构,前4字节有效
  char holder_acc[G1_HOLDERACC_LEN];          //股东账户
  char session[G1_SESSION_LEN];               //会话信息
  char end_code[G1_CUST_END_LEN];             //终端信息
  int16_t market_type;                        //市场
  char order_way[2];                          //委托通道类型
  int32_t log_type;                           //1-用户，2-网关
  int32_t heart_bt_int;                       //心跳时间
  int32_t req_connect_id;                     //网关填写，api 设为0
  char version[G1_VERSION_LEN];               //版本号
};

struct login_ans {
  int64_t cust_req_no;                        ///< 客户私有请求号
  char cust_id[G1_CUSTID_LEN];                //客户号
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; //资金账号，前16字节有效
  char branch_id[G1_BRANCHID_LEN];            //分支机构,前4字节有效
  char holder_acc[G1_HOLDERACC_LEN];          //股东账户
  char session[G1_SESSION_LEN];               //会话信息
  char end_code[G1_CUST_END_LEN];             //终端信息
  uint16_t user_id;                           //用户ID索引，LogType=2时，无意义，返回-1
  uint16_t board_no;                          //板卡号
  int16_t proto_type;                         //协议，1-TCP,2-udp；目前固定1
  char order_way[2];                          //委托通道类型
  int32_t req_connect_id;                     //网关填写，fdm 带回
  int32_t log_type;                           //1-用户，2-网关
  uint32_t session_id;                        //会话号
  int32_t trade_port;                         //交易端口
  char trade_ip[G1_IPADDR_LEN];               //交易IP
  int32_t err_code;                           //错误码
  int32_t reserved;                           //保留填充
  char err_msg[G1_ERRMSG_LEN];                //错误信息
  char version[G1_VERSION_LEN];               //后台版本号
  int64_t login_time;                         //登录时间，秒
};

//获取证券信息请求
struct sec_info_req {
  int64_t cust_req_no; ///< 客户私有请求号
};

struct sec_push_head {
  int64_t cust_req_no; ///< 客户私有请求号
  uint16_t total_num;
  uint16_t cur_num;
  int32_t err_code;
};

struct sec_push_info {
  char security_id[G1_SECURITYID_MAXLEN];
  int16_t market_type;   //市场
  uint16_t sec_index;    //证券索引
  int32_t buy_qty_unit;  //买数量交易单位，不放大
  int32_t sell_qty_unit; //卖数量交易单位，不放大
  int32_t reserved;      //填充未使用
  int64_t price_unit;    //价格变动单位，放大10000
};

struct sec_info_ans {
  sec_push_head head;
  sec_push_info info[];
};

// 板卡客户状态消息
struct fpga_user_state {
  int32_t cur_time;                           //通知时间，秒
  uint16_t board_no;                          //板卡号
  int16_t market_type;                        //市场
  uint16_t user_id;                           //用户索引
  int16_t state;                              //状态，1-正常，2-故障
  char branch_id[G1_BRANCHID_LEN];            //分支机构,前4字节有效
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; //资金账号，前16字节有效
};

struct order_req {
  uint16_t user_id;    //用户索引
  uint16_t board_no;   //板卡号
  uint16_t sec_index;  //证券索引
  char side;           //买卖方向
  char order_type;     //市价限价标记
  int64_t order_price; //委托价格，放大10000
  int64_t order_qty;   //委托数量，不放大100
  int64_t cust_req_no; ///< 客户私有请求号
  int64_t stop_price;  //止损价
  uint16_t tgw_id;     //TGW编号
  uint16_t policy_id;  //策略佣金ID
  char order_way[2];   //委托方式
  int16_t reserve;     //预留
};

struct order_rtn {
  uint16_t user_id;    //用户索引
  uint16_t board_no;   //板卡号
  uint16_t sec_index;  //证券索引
  char side;           //买卖方向
  char order_type;     //市价限价标记
  int64_t order_price; //放大10000
  int64_t order_qty;   //不放大100
  int64_t cust_req_no; ///< 客户私有请求号

  uint8_t order_status;
  uint8_t rtn_type; //回报类型，字典待定：委托应答、委托回报、委托废单、撤单应答、撤单成交、撤单废单
  uint16_t policy_id; //策略佣金ID，fpga不用存
  int32_t err_code;   //错误码

  int64_t order_sys_no;   //柜台委托号
  int64_t frozen_amount;  //冻结金额
  int64_t trade_amount;   //累计成交金额
  int64_t trade_qty;      //累计成交数量
  int64_t fee;            // 累计费用（含冻结）
  int64_t cancel_qty;     //撤单数量
  int64_t order_time;     //委托时间，HHMMSSmmm，看交易所什么类型，取交易所时间
  int64_t update_time;    //更新时间，FPGA不用存，取交易所时间
  int64_t session_seq_no; ///< 后台会话可靠消息流水号(0-非可靠消息)
};

struct cancel_req {
  int64_t cust_req_no;     ///< 客户私有请求号
  int64_t order_sys_no;    //柜台原始报单编号
  uint16_t user_id;        //用户索引
  uint16_t board_no;       //板卡号
  int32_t reserve;         //预留
  int64_t org_cust_req_no; ///< 原客户私有请求号(被撤委托的)
};

//一般用于撤单废单响应，如中间路径出错返回
struct cancel_rsp {
  int64_t cust_req_no;     ///< 客户私有请求号
  int64_t order_sys_no;    //柜台原始报单编号
  uint16_t user_id;        //用户索引
  uint16_t board_no;       //板卡号
  int32_t err_code;        //错误代码
  int64_t org_cust_req_no; ///< 原客户私有请求号(被撤委托的)
  int64_t session_seq_no;  ///< 后台会话可靠消息流水号(0-非可靠消息)
};

struct trade_rtn {
  // 委托信息
  uint16_t user_id;     //用户索引
  uint16_t board_no;    //板卡号
  uint16_t sec_index;   //证券索引
  char side;            //买卖方向
  char order_type;      //市价限价标记
  int64_t order_price;  //放大10000
  int64_t order_qty;    //不放大100
  int64_t cust_req_no;  // 客户私有请求号
  uint8_t order_status; //委托状态
  uint8_t rtn_type; //回报类型，字典待定：委托应答、委托回报、委托废单、撤单应答、撤单成交、撤单废单
  uint16_t policy_id; //策略佣金ID，fpga不用存
  int32_t err_code;   //错误码

  int64_t order_sys_no;  //柜台委托号
  int64_t frozen_amount; //冻结金额
  int64_t trade_amount;  //累计成交金额
  int64_t trade_qty;     //累计成交数量
  int64_t cancel_qty;    //撤单成交数量
  int64_t fee;           //累计费用（含冻结）
  int64_t order_time;    //委托时间，HHMMSSmmm，看交易所什么类型，取交易所时间
  int64_t exec_time;     //成交时间，FPGA不用存，取交易所时间

  // 成交信息
  char exec_id[G1_EXECID_LEN]; // 交易所成交编号
  int64_t exec_price;          // 成交价格
  int64_t exec_qty;            // 成交数量
  int64_t exec_amount;         // 成交金额
  int64_t exec_fee;            // 单笔成交费用

  int64_t session_seq_no; ///< 后台会话可靠消息流水号(0-非可靠消息)
};

// 网关路由拒绝消息扩展头
struct g1_gw_rej_head {
  uint32_t rej_msg_id;         //被拒绝的消息号
  int32_t err_code;            //拒绝错误码
  char err_msg[G1_ERRMSG_LEN]; //错误信息
};

#endif //G1TRADEMSG_H

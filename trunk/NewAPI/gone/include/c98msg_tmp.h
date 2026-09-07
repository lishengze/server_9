#ifndef C98_TMP_MSGHEAD_H
#define C98_TMP_MSGHEAD_H

#include <cstdint>

/*
这是个 98 柜台消息定义头文件，假的，临时模拟定义，方便登陆流程的实现
*/
static const char *c98_msg_ver = "1.0.0";

// api 请求消息
#define C98_MSG_AGW_LOGIN_REQ      10001u ///< agw 登陆请求
#define C98_MSG_ACC_LOGIN_REQ      10002u ///< 投资者账户登陆请求
#define C98_MSG_HEART_REQ          10003u ///< 心跳请求
#define C98_MSG_ORDER_REQ          10004u ///< 委托请求 (98 模式下走 c98 委托)
#define C98_MSG_CANCEL_REQ         10005u ///< 撤单请求
#define C98_MSG_ORDER_QUERY_REQ    10006u ///< 委托查询请求
#define C98_MSG_TRADE_QUERY_REQ    10007u ///< 成交查询请求
#define C98_MSG_FUND_QUERY_REQ     10008u ///< 资金查询请求
#define C98_MSG_POSITION_QUERY_REQ 10009u ///< 持仓查询请求

// 后台应答推送消息
#define C98_MSG_AGW_LOGIN_ANS      20001u ///< agw 登陆响应
#define C98_MSG_ACC_LOGIN_ANS      20002u ///< 投资者账户登陆响应
#define C98_MSG_HEART_ANS          20003u ///< 心跳响应
#define C98_MSG_ORDER_ANS          20004u ///< 委托响应
#define C98_MSG_CANCEL_ANS         20005u ///< 撤单响应
#define C98_MSG_ORDER_QUERY_ANS    20006u ///< 委托查询响应
#define C98_MSG_TRADE_QUERY_ANS    20007u ///< 成交查询响应
#define C98_MSG_FUND_QUERY_ANS     20008u ///< 资金查询响应
#define C98_MSG_POSITION_QUERY_ANS 20009u ///< 持仓查询响应

#define C98_MSG_MAX_LEN 4096

struct c98_msg_head_tmp {
  uint32_t msg_id;  //消息号
  uint32_t msg_len; //不含头,包体长度
  int64_t seq_no;   //流消息号，可靠性预留，默认0
};

// AGW登录消息
struct c98_agw_login_req {
  int64_t client_req_no;       ///< 客户私有请求号
  char agw_user[32];           //agw 用户
  char agw_user_password[256]; //agw 用户密码
  char version[64];            //版本号
};

struct c98_agw_login_ans {
  int64_t client_req_no; ///< 客户私有请求号
  char agw_user[32];     //agw 用户
  char session[32];      //会话信息
  int32_t err_code;      //错误码
  char err_msg[128];     //错误信息
};

// 投资者登录消息
struct c98_acc_login_req {
  int64_t client_req_no;    ///< 客户私有请求号
  char cust_id[16];         //客户号
  char fund_account_id[16]; //资金账号，前16字节有效
  char branch_id[10];       //分支机构,前4字节有效
  char account_id[12];      ///< 客户股东账号
  char password[256];       //客户密码
  char session[32];         //会话信息
  char end_code[1024];      //终端信息
  int16_t market_type;      //市场
  uint16_t heart_bt_int;    //心跳时间
  char order_way[2];        //委托通道类型
  char version[64];         //版本号
};

struct c98_acc_login_ans {
  int64_t client_req_no;    ///< 客户请求号
  char cust_id[16];         //客户号
  char fund_account_id[16]; //资金账号，前16字节有效
  char branch_id[10];       //分支机构,前4字节有效
  char account_id[12];      ///< 客户股东账号
  char password[256];       //客户密码
  char session[32];         //会话信息
  char user_info[64];       //用户信息
  char end_code[1024];      //终端信息
  int16_t market_type;      //市场
  uint16_t heart_bt_int;    //心跳时间
  char order_way_ext[2];    //委托通道类型
  int32_t err_code;         //错误码
  char err_msg[128];        //错误信息
  int64_t login_time;
};

// 委托请求 (98 协议体)
struct c98_order_req {
  char fund_account_id[16]; //资金账号
  char branch_id[10];       //分支机构
  char security_id[8];      //证券代码
  char side;                //买卖方向
  char order_type;          //市价限价
  int64_t order_price;      //委托价格,放大10000
  int64_t order_qty;        //委托数量
  int64_t stop_price;       //止损价
  int64_t client_seq_id;    //用户私有报单号
  char order_way[2];        //委托方式
  char reserve[16];         //预留
};

// 撤单请求 (98 协议体)
struct c98_cancel_req {
  int64_t client_seq_id;    //用户私有报单号
  char fund_account_id[16]; //资金账号
  char branch_id[10];       //分支机构
  int64_t order_sys_no;     //柜台原始报单编号
};

// 委托/成交通用查询请求 (98 协议体)
struct c98_query_req {
  int64_t client_req_no;    //客户请求号
  char cust_id[16];         //客户号
  char fund_account_id[16]; //资金账号
  char branch_id[10];       //分支机构
  char market_type;         //市场
  int64_t begin_no;         //起始号(0=全部)
  int64_t end_no;           //结束号(0=无上限)
};

// 资金查询请求 (98 协议体)
struct c98_fund_query_req {
  int64_t client_req_no;    //客户请求号
  char cust_id[16];         //客户号
  char fund_account_id[16]; //资金账号
  char branch_id[10];       //分支机构
  char market_type;         //市场
  char reserve[16];         //预留
};

// 持仓查询请求 (98 协议体)
struct c98_position_query_req {
  int64_t client_req_no;    //客户请求号
  char cust_id[16];         //客户号
  char fund_account_id[16]; //资金账号
  char branch_id[10];       //分支机构
  char security_id[8];      //证券代码(空=所有)
  char reserve[16];         //预留
};

#endif

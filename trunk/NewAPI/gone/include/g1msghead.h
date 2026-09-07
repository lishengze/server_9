#ifndef G1MSGHEAD_H
#define G1MSGHEAD_H
#include "g1comdefine.h"
#include <cstdint>

static const char *g1_msg_ver = "1.0.0";

struct g1_msg_head {
  uint32_t msg_id;     //消息号
  uint32_t msg_len;    //不含头,包体长度
  uint16_t board_no;   //板卡号
  uint16_t user_id;    //用户 id
  uint32_t session_id; //会话id
};

/*管理消息的辅助头*/
struct g1_mng_msg_head {
  char fund_account_id[G1_FUNDACCOUNTID_LEN]; //资金账号，前16字节有效
  char branch_id[G1_BRANCHID_LEN];            //分支机构,前4字节有效
  uint32_t req_connect_id;                    //网关请求来自的连接id，网关填写
};

#endif //G1MSGHEAD_H

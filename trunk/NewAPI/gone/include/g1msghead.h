#ifndef G1MSGHEAD_H
#define G1MSGHEAD_H
#include "g1comdefine.h"
#include <cstdint>

struct g1_msg_head
{
  uint32_t msg_id;     // 消息号
  uint32_t msg_len;    // 不含头,包体长度
  uint16_t board_no;   // 板卡号
  uint16_t user_id;    // 用户 id
  uint32_t session_id; // 会话id
};

#endif // G1MSGHEAD_H

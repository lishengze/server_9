#include "message_parser.h"
#include <cstdio>

namespace mock_gone {

bool MessageParser::parse_header(const char* data, size_t len, g1_msg_head& head) {
    if (len < sizeof(g1_msg_head)) {
        return false;
    }
    std::memcpy(&head, data, sizeof(g1_msg_head));
    return true;
}

const char* MessageParser::get_body(const char* data, size_t len, const g1_msg_head& head) {
    size_t total_len = sizeof(g1_msg_head) + head.msg_len;
    if (len < total_len) {
        return nullptr;
    }
    return data + sizeof(g1_msg_head);
}

int MessageParser::build_header(char* buf, size_t cap, uint32_t msg_id, uint32_t msg_len,
                                 uint16_t board_no, uint16_t user_id, uint32_t session_id) {
    if (cap < sizeof(g1_msg_head)) {
        return -1;
    }
    g1_msg_head head;
    head.msg_id = msg_id;
    head.msg_len = msg_len;
    head.board_no = board_no;
    head.user_id = user_id;
    head.session_id = session_id;
    std::memcpy(buf, &head, sizeof(g1_msg_head));
    return static_cast<int>(sizeof(g1_msg_head));
}

int MessageParser::build_message(char* buf, size_t cap, uint32_t msg_id,
                                  const void* body, uint32_t body_len,
                                  uint16_t board_no, uint16_t user_id, uint32_t session_id) {
    size_t total_len = sizeof(g1_msg_head) + body_len;
    if (cap < total_len) {
        return -1;
    }
    int hlen = build_header(buf, cap, msg_id, body_len, board_no, user_id, session_id);
    if (hlen < 0) return -1;
    if (body != nullptr && body_len > 0) {
        std::memcpy(buf + hlen, body, body_len);
    }
    return static_cast<int>(total_len);
}

std::string MessageParser::msg_type_name(uint32_t msg_id) {
    switch (msg_id) {
    case G1_MSG_ORDER_REQ:    return "ORDER_REQ(1001)";
    case G1_MSG_CANCEL_REQ:   return "CANCEL_REQ(1002)";
    case G1_MSG_HEART_REQ:    return "HEART_REQ(1003)";
    case G1_MSG_SEC_INFO_REQ: return "SEC_INFO_REQ(1004)";
    case G1_MSG_LOGIN_REQ:    return "LOGIN_REQ(1005)";
    case G1_MSG_ORDER_RTN:    return "ORDER_RTN(2001)";
    case G1_MSG_TRADE_RTN:    return "TRADE_RTN(2002)";
    case G1_MSG_CANCEL_RSP:   return "CANCEL_RSP(2003)";
    case G1_MSG_HEART_ANS:    return "HEART_ANS(2004)";
    case G1_MSG_SEC_INFO_ANS: return "SEC_INFO_ANS(2005)";
    case G1_MSG_LOGIN_ANS:    return "LOGIN_ANS(2006)";
    case G1_MSG_OFFLINE_PUSH: return "OFFLINE_PUSH(2007)";
    case G1_MSG_GW_REJ:       return "GW_REJ(2008)";
    default: {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "UNKNOWN(%u)", msg_id);
        return std::string(buf);
    }
    }
}

} // namespace mock_gone

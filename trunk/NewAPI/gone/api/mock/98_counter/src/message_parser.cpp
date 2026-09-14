#include "message_parser.h"

namespace mock_98 {

bool MessageParser::parse_header(const char* data, size_t len, c98_msg_head_tmp& head) {
    if (len < sizeof(c98_msg_head_tmp)) return false;
    std::memcpy(&head, data, sizeof(c98_msg_head_tmp));
    return true;
}

const char* MessageParser::get_body(const char* data, size_t len, const c98_msg_head_tmp& head) {
    size_t total = sizeof(c98_msg_head_tmp) + head.msg_len;
    if (len < total) return nullptr;
    return data + sizeof(c98_msg_head_tmp);
}

int MessageParser::build_header(char* buf, size_t cap, uint32_t msg_id, uint32_t msg_len, int64_t seq_no) {
    if (cap < sizeof(c98_msg_head_tmp)) return -1;
    c98_msg_head_tmp head;
    head.msg_id = msg_id;
    head.msg_len = msg_len;
    head.seq_no = seq_no;
    std::memcpy(buf, &head, sizeof(c98_msg_head_tmp));
    return static_cast<int>(sizeof(c98_msg_head_tmp));
}

int MessageParser::build_message(char* buf, size_t cap, uint32_t msg_id,
                                  const void* body, uint32_t body_len, int64_t seq_no) {
    size_t total = sizeof(c98_msg_head_tmp) + body_len;
    if (cap < total) return -1;

    int n = build_header(buf, cap, msg_id, body_len, seq_no);
    if (n < 0) return -1;

    if (body && body_len > 0) {
        std::memcpy(buf + sizeof(c98_msg_head_tmp), body, body_len);
    }
    return static_cast<int>(total);
}

std::string MessageParser::msg_type_name(uint32_t msg_id) {
    switch (msg_id) {
        case C98_MSG_AGW_LOGIN_REQ: return "AGW_LOGIN_REQ(10001)";
        case C98_MSG_ACC_LOGIN_REQ: return "ACC_LOGIN_REQ(10002)";
        case C98_MSG_HEART_REQ:     return "HEART_REQ(10003)";
        case C98_MSG_ORDER_REQ:     return "ORDER_REQ(10004)";
        case C98_MSG_CANCEL_REQ:    return "CANCEL_REQ(10005)";
        case C98_MSG_AGW_LOGIN_ANS: return "AGW_LOGIN_ANS(20001)";
        case C98_MSG_ACC_LOGIN_ANS: return "ACC_LOGIN_ANS(20002)";
        case C98_MSG_HEART_ANS:     return "HEART_ANS(20003)";
        default:                    return "UNKNOWN(" + std::to_string(msg_id) + ")";
    }
}

} // namespace mock_98

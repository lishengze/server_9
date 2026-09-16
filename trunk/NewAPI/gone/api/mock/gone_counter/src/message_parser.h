#ifndef MOCK_GONE_MESSAGE_PARSER_H
#define MOCK_GONE_MESSAGE_PARSER_H

#include "g1msghead.h"
#include "g1trademsg.h"
#include <cstdint>
#include <cstring>
#include <string>

namespace mock_gone {

/// g1 协议消息解析工具类
class MessageParser {
public:
    /// 解析消息头
    static bool parse_header(const char* data, size_t len, g1_msg_head& head);

    /// 获取消息体指针
    static const char* get_body(const char* data, size_t len, const g1_msg_head& head);

    /// 构建消息头到缓冲区
    static int build_header(char* buf, size_t cap, uint32_t msg_id, uint32_t msg_len,
                            uint16_t board_no, uint16_t user_id, uint32_t session_id);

    /// 构建完整消息（头 + 体）
    static int build_message(char* buf, size_t cap, uint32_t msg_id,
                             const void* body, uint32_t body_len,
                             uint16_t board_no, uint16_t user_id, uint32_t session_id);

    /// 获取消息类型名称
    static std::string msg_type_name(uint32_t msg_id);
};

} // namespace mock_gone

#endif // MOCK_GONE_MESSAGE_PARSER_H

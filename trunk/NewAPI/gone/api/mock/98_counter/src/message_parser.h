#ifndef MOCK_98_MESSAGE_PARSER_H
#define MOCK_98_MESSAGE_PARSER_H

#include "c98msg_tmp.h"
#include <cstdint>
#include <cstring>
#include <string>

namespace mock_98 {

/// 消息解析工具类
class MessageParser {
public:
    /// 解析消息头
    static bool parse_header(const char* data, size_t len, c98_msg_head_tmp& head);

    /// 获取消息体指针
    static const char* get_body(const char* data, size_t len, const c98_msg_head_tmp& head);

    /// 构建消息头到缓冲区
    static int build_header(char* buf, size_t cap, uint32_t msg_id, uint32_t msg_len, int64_t seq_no);

    /// 构建完整消息（头 + 体）
    static int build_message(char* buf, size_t cap, uint32_t msg_id,
                             const void* body, uint32_t body_len, int64_t seq_no);

    /// 获取消息类型名称
    static std::string msg_type_name(uint32_t msg_id);
};

} // namespace mock_98

#endif // MOCK_98_MESSAGE_PARSER_H

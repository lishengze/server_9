#ifndef MOCK_98_MESSAGE_PARSER_H
#define MOCK_98_MESSAGE_PARSER_H

#include <cstdint>
#include <cstring>
#include "agw_protocol.h"

// ============================================================
// 消息解析器: 处理 AGW 线格式的拆包/组包
// 线格式: [raw_agw_header 16B] [body 变长] [check_sum 4B]
// ============================================================

/// 解析消息头, 从 buf 中读取 16B header
/// @return true=成功, false=数据不足
inline bool parse_agw_header(const char *buf, int32_t len, raw_agw_header &hdr) {
  if (len < AGW_HEADER_SIZE)
    return false;
  std::memcpy(&hdr, buf, sizeof(hdr));
  return true;
}

/// 计算完整消息总长度 (header + body + checksum)
inline int32_t agw_msg_total_len(const raw_agw_header &hdr) {
  return AGW_HEADER_SIZE + static_cast<int32_t>(hdr.body_length) + AGW_CHECKSUM_SIZE;
}

/// 校验 CheckSum
inline bool verify_agw_checksum(const char *buf, const raw_agw_header &hdr) {
  int32_t body_len = static_cast<int32_t>(hdr.body_length);
  // CheckSum 在 header + body 之后
  const char *cs_ptr = buf + AGW_HEADER_SIZE + body_len;
  uint32_t cs_in_msg = 0;
  std::memcpy(&cs_in_msg, cs_ptr, sizeof(cs_in_msg));
  cs_in_msg = mp::NetworkToHost(cs_in_msg);
  uint32_t cs_calc = calc_check_sum(buf, static_cast<uint32_t>(AGW_HEADER_SIZE + body_len));
  return cs_in_msg == cs_calc;
}

#endif // MOCK_98_MESSAGE_PARSER_H

#ifndef MOCK_98_AGW_PROTOCOL_H
#define MOCK_98_AGW_PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <string>

// lbcommon: 字节序转换
#include "comm_sys.h"

// counter98 协议头文件
#include "lb_counter98_protocol.h"
#include "message_encoder.h"
#include "message_decoder.h"
#include "serialize_buffer.h"

// ============================================================
// AGW 线格式: [raw_agw_header 16B] [body 变长] [check_sum 4B 网络字节序]
// body_length 仅含 Body, 不含 CheckSum; CheckSum 覆盖 Header + Body
// ============================================================

static constexpr int32_t AGW_HEADER_SIZE = 16;
static constexpr int32_t AGW_CHECKSUM_SIZE = 4;

struct raw_agw_header {
  uint32_t msg_type;
  uint16_t msg_ver;
  uint16_t reserver;
  int32_t  seq_num;    ///< 协议头序号 (98柜台未使用, 保持为0)
  uint32_t body_length;
};
static_assert(sizeof(raw_agw_header) == AGW_HEADER_SIZE, "raw_agw_header must be 16 bytes");

// ---- 辅助: 计算 CheckSum (对 Header + Body 逐字节累加, 取 % 256) ----
inline uint32_t calc_check_sum(const char *data, uint32_t len) {
  uint32_t sum = 0;
  for (uint32_t i = 0; i < len; ++i) {
    sum += static_cast<uint32_t>(static_cast<unsigned char>(data[i]));
  }
  return sum % 256;
}

// ---- 辅助: 写 AgwHeader + Body + CheckSum 到 o_buf ----
// 返回消息总长度(含 Header + Body + CheckSum), <0 出错
inline int32_t write_agw_msg(char *o_buf, uint32_t msg_type, uint16_t msg_ver,
                              const char *body, uint32_t body_len) {
  raw_agw_header hdr{};
  hdr.msg_type = msg_type;
  hdr.msg_ver = msg_ver;
  hdr.reserver = 0;
  hdr.body_length = body_len;
  std::memcpy(o_buf, &hdr, sizeof(hdr));
  if (body_len > 0 && body != nullptr) {
    std::memcpy(o_buf + sizeof(hdr), body, body_len);
  }
  uint32_t check_sum = mp::HostToNetwork(
      calc_check_sum(o_buf, static_cast<uint32_t>(sizeof(hdr) + body_len)));
  std::memcpy(o_buf + sizeof(hdr) + body_len, &check_sum, sizeof(check_sum));
  return static_cast<int32_t>(sizeof(hdr) + body_len + sizeof(uint32_t));
}

// ---- 辅助: 编码消息体到 SerializeBuffer ----
template <typename MsgType>
inline bool encode_body(const MsgType &msg, lb_counter98::SerializeBuffer &buf) {
  lb_counter98::MessageEncoder encoder(buf);
  auto ec = msg.Encode(encoder);
  return ec == lb_counter98::kSuccess;
}

// ---- 辅助: 从 buffer 解码消息体 ----
template <typename MsgType>
inline bool decode_body(MsgType &msg, const char *body, int32_t body_len) {
  lb_counter98::MessageDecoder decoder(body, body_len);
  auto ec = msg.Decode(decoder);
  return ec == lb_counter98::kSuccess;
}

// ---- 辅助: 消息类型名称 (用于日志) ----
inline const char *agw_msg_type_name(uint32_t msg_type) {
  switch (msg_type) {
  case lb_counter98::kAgwMsgLogin:                       return "Login";
  case lb_counter98::kAgwMsgCustLoginReq:                return "CustLoginReq";
  case lb_counter98::kAgwMsgCustLoginResp:               return "CustLoginResp";
  case lb_counter98::kAgwMsgHeartBeat:                   return "HeartBeat";
  case lb_counter98::kAgwMsgCashAuctionOrder:            return "CashAuctionOrder";
  case lb_counter98::kAgwMsgCashAuctionTradeER:          return "CashAuctionTradeER";
  case lb_counter98::kAgwMsgCancelOrder:                 return "CancelOrder";
  case lb_counter98::kAgwMsgOrderStatusAck:              return "OrderStatusAck";
  case lb_counter98::kAgwMsgExtQueryOrder:               return "ExtQueryOrder";
  case lb_counter98::kAgwMsgExtQueryResultOrder:         return "ExtQueryResultOrder";
  case lb_counter98::kAgwMsgExtQueryTradeOrder:          return "ExtQueryTradeOrder";
  case lb_counter98::kAgwMsgExtQueryResultTradeOrder:    return "ExtQueryResultTradeOrder";
  case lb_counter98::kAgwMsgExtQueryFund:                return "ExtQueryFund";
  case lb_counter98::kAgwMsgExtQueryResultFund:          return "ExtQueryResultFund";
  case lb_counter98::kAgwMsgExtQuerySelfShare:           return "ExtQuerySelfShare";
  case lb_counter98::kAgwMsgExtQueryResultSelfShare:     return "ExtQueryResultSelfShare";
  case lb_counter98::kAgwMsgExtQueryMaxOrderQty:         return "ExtQueryMaxOrderQty";
  case lb_counter98::kAgwMsgExtQueryMaxOrderQtyResult:   return "ExtQueryMaxOrderQtyResult";
  case lb_counter98::kAgwMsgExternalQueryTotalFundAsset: return "ExternalQueryTotalFundAsset";
  case lb_counter98::kAgwMsgExtQueryResultTotalFundAsset:return "ExtQueryResultTotalFundAsset";
  case lb_counter98::kAgwMsgExtQueryFundMaxdraw:         return "ExtQueryFundMaxdraw";
  case lb_counter98::kAgwMsgExtQueryResultFundMaxdraw:   return "ExtQueryResultFundMaxdraw";
  case lb_counter98::kAgwMsgExtQueryCustMarginAvl:       return "ExtQueryCustMarginAvl";
  case lb_counter98::kAgwMsgExtQueryResultCustMarginAvl: return "ExtQueryResultCustMarginAvl";
  case lb_counter98::kAgwMsgExtQueryAssetDebts:          return "ExtQueryAssetDebts";
  case lb_counter98::kAgwMsgExtQueryResultAssetDebts:    return "ExtQueryResultAssetDebts";
  case lb_counter98::kAgwMsgExtQueryCreditMaxOrderQty:   return "ExtQueryCreditMaxOrderQty";
  case lb_counter98::kAgwMsgExtQueryCreditMaxOrderQtyResult: return "ExtQueryCreditMaxOrderQtyResult";
  default: return "Unknown";
  }
}

#endif // MOCK_98_AGW_PROTOCOL_H

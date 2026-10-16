// api_net_time_common.h - 网卡抓包时间分析：共享常量与结构
// 被 mock_client（perf_runner）和 api_net_time_capture 两个组件复用
//
// 适配：NewAPI 框架（gw_counter_direct → FTE）
//   - 委托包 = [PktNewHeader 8B | TradeOrderReq 106B | 校验和 4B] = 118B
//   - msg_id = 1003（kPktOrderReq），msg_len = 106
//   - client_seq_id 网络偏移 62，小端（build_order_msg 直接 memcpy host 字节序）
//   - 详见 api_net_time_design.md
#ifndef API_NET_TIME_COMMON_H
#define API_NET_TIME_COMMON_H

#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <fstream>

namespace apinet {

// ---- FTE 委托包协议常量 ----
static const uint32_t kPktOrderReq        = 1003;   // 委托请求消息号（kPktOrderReq）
static const size_t   kPktHeaderLen       = 8;      // PktNewHeader 长度
static const size_t   kTradeOrderReqLen   = 106;    // TradeOrderReq 长度
static const size_t   kChecksumLen        = 4;      // 尾部校验和长度
static const size_t   kOrderPktLen        = kPktHeaderLen + kTradeOrderReqLen + kChecksumLen; // 118
static const size_t   kClientSeqIdOffsetInReq = 54; // client_seq_id 在 TradeOrderReq 内的偏移
static const size_t   kClientSeqIdOffset  = kPktHeaderLen + kClientSeqIdOffsetInReq;          // 62

// 大端读取
inline uint16_t be16(const uint8_t* p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}
inline uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
inline int64_t be64(const uint8_t* p) {
    return ((int64_t)be32(p) << 32) | (int64_t)be32(p + 4);
}
// 小端读取
inline int64_t le64(const uint8_t* p) {
    return ((int64_t)p[0]) | ((int64_t)p[1] << 8) | ((int64_t)p[2] << 16) | ((int64_t)p[3] << 24)
         | ((int64_t)p[4] << 32) | ((int64_t)p[5] << 40) | ((int64_t)p[6] << 48) | ((int64_t)p[7] << 56);
}
inline uint16_t le16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
inline uint32_t le32(const uint8_t* p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// ---- GOne (FPGA) 委托包协议常量 ----
// GOne 包 = [g1_msg_head 16B | order_req 48B] = 64B，无校验和
//   g1_msg_head: msg_id(uint32 LE) msg_len(uint32 LE) board_no(uint16) user_id(uint16) session_id(uint32)
//   order_req:   cust_req_no(int64 LE) 在 order_req 内偏移 24
//   msg_id = G1_MSG_ORDER_REQ = 1001
static const uint32_t kG1MsgOrderReq      = 1001;   // GOne 委托请求消息号
static const size_t   kG1MsgHeadLen       = 16;     // g1_msg_head 长度
static const size_t   kOrderReqLen        = 48;     // order_req 长度
static const size_t   kGonePktLen         = kG1MsgHeadLen + kOrderReqLen; // 64
static const size_t   kGoneClientSeqIdOffsetInReq = 24; // cust_req_no 在 order_req 内偏移
static const size_t   kGoneClientSeqIdOffset = kG1MsgHeadLen + kGoneClientSeqIdOffsetInReq; // 40

// 协议类型
enum class ProtoType { Gw, Gone };

// 从完整 GOne 委托包（应用数据开头）提取 client_seq_id（cust_req_no，小端）
inline int64_t extract_gone_client_seq_id(const uint8_t* p) {
    return le64(p + kGoneClientSeqIdOffset);
}

// 从完整委托包（应用数据开头）提取 client_seq_id。
// 注意：build_order_msg 直接 memcpy host 字节序（小端），故 client_seq_id 在网络包中为小端（x86 host）。
inline int64_t extract_client_seq_id(const uint8_t* p) {
    return le64(p + kClientSeqIdOffset);
}

// ---- 关联映射表：client_seq_id -> api_arrive_time_ns ----
using SeqArriveMap = std::unordered_map<int64_t, uint64_t>;

// 从映射文件加载（每行: <client_seq_id> <api_arrive_time_ns>，空白分隔）
inline bool load_map_file(const std::string& path, SeqArriveMap& m) {
    std::ifstream ifs(path.c_str());
    if (!ifs.is_open()) return false;
    int64_t seq = 0;
    uint64_t arrive = 0;
    while (ifs >> seq >> arrive) m[seq] = arrive;
    return true;
}

// 写出映射文件（每行: <client_seq_id> <api_arrive_time_ns>）
inline bool write_map_file(const std::string& path,
                           const std::vector<std::pair<int64_t, uint64_t>>& vec) {
    std::ofstream ofs(path.c_str());
    if (!ofs.is_open()) return false;
    for (auto& kv : vec)
        ofs << kv.first << " " << kv.second << "\n";
    return true;
}

} // namespace apinet

#endif // API_NET_TIME_COMMON_H
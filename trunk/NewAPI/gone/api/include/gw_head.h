#pragma once

// ============================================================================
// gw_head.h
// FTE 对外接口数据结构（扁平化、纯标准库依赖版本）
//
// ███████████████████████████████████████████████████████████████████████████
// 一、文件定位
//   本头文件是"新 TCP API"客户端使用的数据头文件，定义了与 FTE 服务端通信所需的
//   全部请求/回报消息结构。所有字段已展开为内置类型，仅依赖 C++ 标准库，
//   不依赖项目内部任何头文件。
//
// 二、与 FTE 服务端的一致性
//   - 本文件（命名空间 gw_message::*）与服务端所用 gw_external_message.h
//     （命名空间 message::*）的 #pragma pack(1) 内存布局、大端字节序约定完全一致，
//     数据可互通（如 sizeof(LogOnReq)=1230、sizeof(TradeOrderER)=324 等已实测一致）。
//   - 字节序约定：多字节整型(>=2字节)按大端(网络序)传输；字符数组(std::array<char,N>)
//     按原字节序传输。
//
// 三、序列化接口说明（encode=发送 / decode=接收）
//   每个消息结构均提供以下两个接口：
//     size_t encode(char* dest, size_t cap) const
//       将对象序列化为网络字节序(大端)字节流写入 dest。
//       参数：dest 输出缓冲区；cap 缓冲区容量。
//       返回：实际写入字节数；若 cap < sizeof(本结构) 返回 0（表示失败）。
//       【注意】encode 不修改源对象，可安全复用同一对象多次发送。
//     bool decode(const char* data, size_t len)
//       从网络字节序(大端)字节流 data 反序列化到本对象。
//       参数：data 报文体起始地址；len 报文体长度(通常取 PktNewHeader::msg_len)。
//       返回：成功 true；len < sizeof(本结构) 返回 false。
//       【注意】decode 前建议先调用 reset() 清空，避免残留脏数据。
//
// 四、报文整体格式（与 FTE 服务端一致）
//     ┌─────────────────┬──────────────────────────┬──────────────┐
//     │ PktNewHeader 8B │  消息体 msg_len B        │ 校验和 uint32│
//     │ msg_id|msg_len  │  (本文件结构 encode 产物) │  (大端)      │
//     └─────────────────┴──────────────────────────┴──────────────┘
//     - msg_id ：消息类型（见下方消息类型常量）。
//     - msg_len：消息体字节数（不含头、不含校验和）。
//     - 校验和 ：对 [头+体] 逐字节求和 %256，再转大端追加 4 字节。
//
// 五、消息类型常量（请求 1xxx / 回报 2xxx / 心跳 3 / 拒绝 9）
//     请求(客户端→FTE)：kPktLoginReq(1001)  kPktLogoutReq(1002)
//                        kPktOrderReq(1003)  kPktCancelOrderReq(1004)
//                        kPktMarginTradingReq(1005)  kPktCreditAuctionReq(1006)
//                        kPktGatewayLoginReq(1009)  kPktETFReq(1010)
//     回报(FTE→客户端)：kPktLoginAns(2001)  kPktLogoutAns(2002)
//                        kPktOrderAns(2003)  kPktCancelOrderAns(2004)
//                        kPktOrderMatch(2005)  kPktGatewayLoginAns(2009)
//                        kPktEtfOrderMatch(2010)  kPktRejectMsg(9)
//     心跳：kPktNewHeartBeat(3)
//
// 六、结构体与消息类型对照
//     请求结构：LogOnReq(1001)  GatewayLogOnReq(1009)  LogOutReq(1002)
//               TradeOrderReq(1003/1005/1006/1010)  CancelOrderReq(1004)
//     回报结构：LogOnAns(2001)  GatewayLogOnAns(2009)  LogOutAns(2002)
//               TradeOrderER(2003/2004/2005/2010)  RejectMsg(9)
//     PktNewHeader：所有消息共用的 8 字节头。
//     ConstituentStock：ETF 成分股，作为 TradeOrderER 的数组元素（不展开）。
//
// 七、使用示例
//   （一）发送委托请求（组包）
//     gw_message::TradeOrderReq req; req.reset();
//     memcpy(req.fund_account_id.data(), "1000000000000001", 16);
//     req.market_id = 1;  req.side = '1';  req.order_type = '2';
//     req.order_qty = 100;  req.order_price = 1000;
//     char pkt[sizeof(gw_message::PktNewHeader) + sizeof(gw_message::TradeOrderReq) + 4];
//     gw_message::PktNewHeader hdr;
//     hdr.msg_id  = gw_message::kPktOrderReq;
//     hdr.msg_len = sizeof(gw_message::TradeOrderReq);
//     size_t off = hdr.encode(pkt, sizeof(pkt));            // 写 8 字节头
//     off += req.encode(pkt + off, sizeof(pkt) - off);      // 写消息体
//     // 追加校验和：对 [头+体] 求和%256 转大端 4 字节，然后 send(pkt, off+4)
//
//   （二）接收回报（拆包，已收到完整报文 pkt/len）
//     gw_message::PktNewHeader hdr;
//     if (!hdr.decode(pkt, len)) return;                    // 解 8 字节头
//     // 校验报文长度与校验和（略）...
//     switch (hdr.msg_id) {
//       case gw_message::kPktOrderAns: {                    // 委托回报
//         gw_message::TradeOrderER rtn; rtn.reset();
//         if (rtn.decode(pkt + 8, hdr.msg_len)) { /* 处理 */ }
//         break;
//       }
//       case gw_message::kPktEtfOrderMatch: {               // ETF 成交回报(含成分券)
//         gw_message::TradeOrderER rtn; rtn.reset();
//         std::vector<gw_message::ConstituentStock> stk(rtn.no_security);
//         rtn.decode(pkt + 8, hdr.msg_len, rtn.no_security, stk.data());
//         break;
//       }
//       case gw_message::kPktRejectMsg: {                   // 拒绝回报
//         gw_message::RejectMsg rj; rj.reset();
//         if (rj.decode(pkt + 8, hdr.msg_len)) { /* 处理 */ }
//         break;
//       }
//     }
//
// 八、注意事项
//   - 字符数组字段需用 std::array<char,N> 方式赋值（如 memcpy/fill），勿直接字符串赋值。
//   - decode 前建议 reset()；encode 不修改源对象。
//   - TradeOrderER 固定部分 sizeof=324 字节；含成分券时总长 =
//     sizeof(TradeOrderER) + sizeof(ConstituentStock) * no_security。
//   - 心跳消息(kPktNewHeartBeat)无消息体，仅 8 字节头 + 4 字节校验和。
// ============================================================================

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>

namespace gw_message
{

// 消息类型常量
const uint32_t kPktNewHeartBeat = 3;
const uint32_t kPktLoginReq = 1001;
const uint32_t kPktLoginAns = 2001;
const uint32_t kPktLogoutReq = 1002;
const uint32_t kPktLogoutAns = 2002;
const uint32_t kPktOrderReq = 1003;
const uint32_t kPktCancelOrderReq = 1004;
const uint32_t kPktMarginTradingReq = 1005;
const uint32_t kPktCreditAuctionReq = 1006;
const uint32_t kPktETFReq = 1010;
const uint32_t kPktOrderAns = 2003;
const uint32_t kPktCancelOrderAns = 2004;
const uint32_t kPktOrderMatch = 2005;
const uint32_t kPktGatewayLoginReq = 1009;
const uint32_t kPktGatewayLoginAns = 2009;
const uint32_t kPktEtfOrderMatch = 2010;
const uint32_t kPktRejectMsg = 9;
const uint32_t kPktStrategyExit = 999999;

// ============================================================================
// 字节序转换辅助（API 端序列化使用，纯标准库实现，不依赖 endian_util.h）
// ============================================================================
namespace detail
{

// 判断当前平台是否为小端（x86/ARM 通常为小端）
inline bool IsLittleEndian()
{
    const uint16_t one = 1;
    return *reinterpret_cast<const uint8_t*>(&one) == 1;
}

inline uint16_t ByteSwap16(uint16_t v) { return static_cast<uint16_t>((v >> 8) | (v << 8)); }
inline uint32_t ByteSwap32(uint32_t v)
{
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
         | ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
}
inline uint64_t ByteSwap64(uint64_t v)
{
    return ((v & 0x00000000000000FFull) << 56) | ((v & 0x000000000000FF00ull) << 40)
         | ((v & 0x0000000000FF0000ull) << 24) | ((v & 0x00000000FF000000ull) << 8)
         | ((v & 0x000000FF00000000ull) >> 8) | ((v & 0x0000FF0000000000ull) >> 24)
         | ((v & 0x00FF000000000000ull) >> 40) | ((v & 0xFF00000000000000ull) >> 56);
}

// 主机序 -> 大端（网络序）。大端平台保持不变，小端平台做字节反转。
// ByteSwap 为自反操作，故 大端->主机序 复用同一函数。
//
// 注意：FTE 服务器（ute 二进制）在 x86 上编译时未定义 FTE_BIG_ENDIAN，
// 其 message::* 的 decode/encode 通过 FTE_MEMCOPY / FTE_CODEC_SWITCH 宏
// 直接按主机字节序（小端）原始 memcpy，不做任何字节序转换。
// 因此 API 端也必须按主机字节序（小端）发送/解析，这里统一改为直通(no-op)，
// 否则数值字段（client_seq_id/order_qty/market_id 等）会因字节交换而错位。
inline uint16_t HostToNetwork(uint16_t v) { return v; }
inline uint32_t HostToNetwork(uint32_t v) { return v; }
inline uint64_t HostToNetwork(uint64_t v) { return v; }

// 按类型宽度选择字节序转换（int16/int32/int64 与对应无符号类型位模式一致）
template <typename T>
inline T HostToNetworkT(T v)
{
    if (sizeof(T) == 2) return static_cast<T>(HostToNetwork(static_cast<uint16_t>(v)));
    if (sizeof(T) == 4) return static_cast<T>(HostToNetwork(static_cast<uint32_t>(v)));
    if (sizeof(T) == 8) return static_cast<T>(HostToNetwork(static_cast<uint64_t>(v)));
    return v;   // 单字节无需转换
}

} // namespace detail

// 辅助函数：将 std::array<char, N> 转换为 std::string
template <size_t Size>
inline std::string AsString(const std::array<char, Size>& from)
{
    return std::string(&from[0], Size);
}

#pragma pack(push)
#pragma pack(1)

// PktNewHeader: 消息头，独立类，不被嵌套
class PktNewHeader
{
public:
    uint32_t msg_id;
    uint32_t msg_len;

    void reset()
    {
        msg_id = 0;
        msg_len = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "msg_id: " << msg_id << "\n"
           << "msg_len: " << msg_len << "\n";
        return os.str();
    }

    /**
     * @brief 将消息头序列化为网络字节序(大端)字节流。
     *
     * 用于组包：在发送任何消息前，先把 msg_id / msg_len 编码为 8 字节大端头，
     * 作为整个报文的开头。msg_len 为消息体字节数（不含头、不含校验和）。
     *
     * @param dest 输出缓冲区（至少 8 字节）。
     * @param cap  缓冲区容量。
     * @return 写入字节数（固定 8）；若 cap < 8 返回 0 表示失败。
     */
    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(PktNewHeader)) return 0;
        // FTE 服务器 PktNewHeader::decode 用 DECODE_BINARY -> set_host_value -> NetworkToHost(be32toh)，
        // 即消息头按大端(网络序)解析，故这里必须显式字节交换，不能依赖 no-op 的 HostToNetwork。
        size_t off = 0;
        uint32_t be = detail::ByteSwap32(msg_id);  memcpy(dest + off, &be, 4); off += 4;
        be = detail::ByteSwap32(msg_len);          memcpy(dest + off, &be, 4); off += 4;
        return off;
    }

    /**
     * @brief 从网络字节序(大端)字节流反序列化消息头。
     *
     * 用于拆包：收到报文后先解析前 8 字节，得到 msg_id（消息类型）与
     * msg_len（消息体长度），据此判断消息体数据范围与后续处理分支。
     *
     * @param data 报文起始地址（至少 8 字节）。
     * @param len  可用字节数。
     * @return 成功 true；len < 8 返回 false。
     */
    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(PktNewHeader)) return false;
        // FTE 服务器按大端(网络序)发送/解析消息头，故这里必须显式字节交换。
        size_t off = 0;
        uint32_t be;
        memcpy(&be, data + off, 4); msg_id = detail::ByteSwap32(be); off += 4;
        memcpy(&be, data + off, 4); msg_len = detail::ByteSwap32(be); off += 4;
        return true;
    }
};

// ConstituentStock: 成分股信息，作为数组元素使用（不展开）
class ConstituentStock
{
public:
    uint16_t market_id;                         // 市场
    std::array<char, 8> security_id;            // 成分股证券代码
    int64_t order_qty;                          // 股份交付数量
    int64_t subst_cash;                         // 现金替代金额
    int64_t price;                              // 成交价格
    uint8_t etf_trade_report_type;              // ETF成交回报类型
    std::array<char, 16> exec_id;               // 执行编号
    char margin_amt_type;                       // 上交所现金替代资金类型

    void reset()
    {
        market_id = 0;
        security_id.fill(' ');
        order_qty = 0;
        subst_cash = 0;
        price = 0;
        etf_trade_report_type = 0;
        exec_id.fill(' ');
        margin_amt_type = ' ';
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "market_id: " << static_cast<int>(market_id) << "\n"
           << "security_id: " << AsString(security_id) << "\n"
           << "order_qty: " << order_qty << "\n"
           << "subst_cash: " << subst_cash << "\n"
           << "price: " << price << "\n"
           << "etf_trade_report_type: " << static_cast<int>(etf_trade_report_type) << "\n"
           << "exec_id: " << AsString(exec_id) << "\n"
           << "margin_amt_type: " << margin_amt_type << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(ConstituentStock)) return 0;
        size_t off = 0;
        uint16_t be16 = detail::HostToNetwork(market_id); memcpy(dest + off, &be16, 2); off += 2;
        memcpy(dest + off, security_id.data(), 8); off += 8;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(order_qty)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(subst_cash)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(price));     memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, &etf_trade_report_type, 1); off += 1;
        memcpy(dest + off, exec_id.data(), 16); off += 16;
        memcpy(dest + off, &margin_amt_type, 1); off += 1;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(ConstituentStock)) return false;
        size_t off = 0;
        uint16_t be16;
        memcpy(&be16, data + off, 2); market_id = detail::HostToNetwork(be16); off += 2;
        memcpy(security_id.data(), data + off, 8); off += 8;
        uint64_t be64;
        memcpy(&be64, data + off, 8); order_qty = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); subst_cash = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); price = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&etf_trade_report_type, data + off, 1); off += 1;
        memcpy(exec_id.data(), data + off, 16); off += 16;
        memcpy(&margin_amt_type, data + off, 1); off += 1;
        return true;
    }
};

// LogOnReq: 登录请求（TradeOrderUser 已展开）
class LogOnReq
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    uint32_t heart_bt_int;
    std::array<char, 100> password;
    std::array<char, 1024> client_feature_code;
    std::array<char, 32> agw_user;

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        heart_bt_int = 0;
        password.fill(' ');
        client_feature_code.fill(' ');
        agw_user.fill(' ');
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "heart_bt_int: " << heart_bt_int << "\n"
           << "password: " << AsString(password) << "\n"
           << "client_feature_code: " << AsString(client_feature_code) << "\n"
           << "agw_user: " << AsString(agw_user) << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(LogOnReq)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));    memcpy(dest + off, &be64, 8); off += 8;
        uint32_t be32 = detail::HostToNetwork(heart_bt_int); memcpy(dest + off, &be32, 4); off += 4;
        memcpy(dest + off, password.data(), 100); off += 100;
        memcpy(dest + off, client_feature_code.data(), 1024); off += 1024;
        memcpy(dest + off, agw_user.data(), 32); off += 32;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(LogOnReq)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        uint32_t be32;
        memcpy(&be32, data + off, 4); heart_bt_int = detail::HostToNetwork(be32); off += 4;
        memcpy(password.data(), data + off, 100); off += 100;
        memcpy(client_feature_code.data(), data + off, 1024); off += 1024;
        memcpy(agw_user.data(), data + off, 32); off += 32;
        return true;
    }
};

// GatewayLogOnReq: 网关登录请求（无嵌套对象）
class GatewayLogOnReq
{
public:
    uint32_t heart_bt_int;
    std::array<char, 64> password;
    std::array<char, 64> agw_user;

    void reset()
    {
        heart_bt_int = 0;
        password.fill(' ');
        agw_user.fill(' ');
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "heart_bt_int: " << heart_bt_int << "\n"
           << "password: " << AsString(password) << "\n"
           << "agw_user: " << AsString(agw_user) << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(GatewayLogOnReq)) return 0;
        size_t off = 0;
        uint32_t be32 = detail::HostToNetwork(heart_bt_int); memcpy(dest + off, &be32, 4); off += 4;
        memcpy(dest + off, password.data(), 64); off += 64;
        memcpy(dest + off, agw_user.data(), 64); off += 64;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(GatewayLogOnReq)) return false;
        size_t off = 0;
        uint32_t be32;
        memcpy(&be32, data + off, 4); heart_bt_int = detail::HostToNetwork(be32); off += 4;
        memcpy(password.data(), data + off, 64); off += 64;
        memcpy(agw_user.data(), data + off, 64); off += 64;
        return true;
    }
};

// LogOnAns: 登录应答（TradeOrderUser 已展开）
class LogOnAns
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    int32_t session_status;
    uint32_t error_code;

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        session_status = 0;
        error_code = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "session_status: " << session_status << "\n"
           << "error_code: " << error_code << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(LogOnAns)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));    memcpy(dest + off, &be64, 8); off += 8;
        uint32_t be32;
        be32 = detail::HostToNetwork(static_cast<uint32_t>(session_status)); memcpy(dest + off, &be32, 4); off += 4;
        be32 = detail::HostToNetwork(error_code); memcpy(dest + off, &be32, 4); off += 4;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(LogOnAns)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        uint32_t be32;
        memcpy(&be32, data + off, 4); session_status = static_cast<int32_t>(detail::HostToNetwork(be32)); off += 4;
        memcpy(&be32, data + off, 4); error_code = detail::HostToNetwork(be32); off += 4;
        return true;
    }
};

// GatewayLogOnAns: 网关登录应答（无嵌套对象）
class GatewayLogOnAns
{
public:
    std::array<char, 64> agw_user;
    int32_t session_status;
    uint32_t error_code;

    void reset()
    {
        agw_user.fill(' ');
        session_status = 0;
        error_code = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "agw_user: " << AsString(agw_user) << "\n"
           << "session_status: " << session_status << "\n"
           << "error_code: " << error_code << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(GatewayLogOnAns)) return 0;
        size_t off = 0;
        memcpy(dest + off, agw_user.data(), 64); off += 64;
        uint32_t be32;
        be32 = detail::HostToNetwork(static_cast<uint32_t>(session_status)); memcpy(dest + off, &be32, 4); off += 4;
        be32 = detail::HostToNetwork(error_code); memcpy(dest + off, &be32, 4); off += 4;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(GatewayLogOnAns)) return false;
        size_t off = 0;
        memcpy(agw_user.data(), data + off, 64); off += 64;
        uint32_t be32;
        memcpy(&be32, data + off, 4); session_status = static_cast<int32_t>(detail::HostToNetwork(be32)); off += 4;
        memcpy(&be32, data + off, 4); error_code = detail::HostToNetwork(be32); off += 4;
        return true;
    }
};

// LogOutReq: 登出请求（TradeOrderUser 已展开）
class LogOutReq
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    std::array<char, 100> password;

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        password.fill(' ');
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "password: " << AsString(password) << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(LogOutReq)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));    memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, password.data(), 100); off += 100;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(LogOutReq)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(password.data(), data + off, 100); off += 100;
        return true;
    }
};

// LogOutAns: 登出应答（TradeOrderUser 已展开）
class LogOutAns
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    int32_t session_status;
    uint32_t error_code;

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        session_status = 0;
        error_code = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "session_status: " << session_status << "\n"
           << "error_code: " << error_code << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(LogOutAns)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));    memcpy(dest + off, &be64, 8); off += 8;
        uint32_t be32;
        be32 = detail::HostToNetwork(static_cast<uint32_t>(session_status)); memcpy(dest + off, &be32, 4); off += 4;
        be32 = detail::HostToNetwork(error_code); memcpy(dest + off, &be32, 4); off += 4;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(LogOutAns)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        uint32_t be32;
        memcpy(&be32, data + off, 4); session_status = static_cast<int32_t>(detail::HostToNetwork(be32)); off += 4;
        memcpy(&be32, data + off, 4); error_code = detail::HostToNetwork(be32); off += 4;
        return true;
    }
};

// TradeOrderReq: 委托请求（TradeOrderUser、TradeOrderInfo 已展开）
class TradeOrderReq
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    // TradeOrderInfo 展开开始
    std::array<char, 8> security_id;
    uint16_t market_id;                 // 市场ID
    char side;
    char order_type;
    int64_t order_qty;
    int64_t order_price;
    int64_t stop_px;
    // TradeOrderInfo 展开结束

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        security_id.fill(' ');
        market_id = 0;
        side = ' ';
        order_type = ' ';
        order_qty = 0;
        order_price = 0;
        stop_px = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "security_id: " << AsString(security_id) << "\n"
           << "market_id: " << static_cast<int>(market_id) << "\n"
           << "side: " << side << "\n"
           << "order_type: " << order_type << "\n"
           << "order_qty: " << order_qty << "\n"
           << "order_price: " << order_price << "\n"
           << "stop_px: " << stop_px << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(TradeOrderReq)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));    memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, security_id.data(), 8); off += 8;
        uint16_t be16 = detail::HostToNetwork(market_id); memcpy(dest + off, &be16, 2); off += 2;
        memcpy(dest + off, &side, 1); off += 1;
        memcpy(dest + off, &order_type, 1); off += 1;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(order_qty));  memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(order_price)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(stop_px));    memcpy(dest + off, &be64, 8); off += 8;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(TradeOrderReq)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(security_id.data(), data + off, 8); off += 8;
        uint16_t be16;
        memcpy(&be16, data + off, 2); market_id = detail::HostToNetwork(be16); off += 2;
        memcpy(&side, data + off, 1); off += 1;
        memcpy(&order_type, data + off, 1); off += 1;
        memcpy(&be64, data + off, 8); order_qty = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); order_price = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); stop_px = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        return true;
    }
};

// CancelOrderReq: 撤单请求（TradeOrderUser、CancelOrderInfo 已展开）
class CancelOrderReq
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    // CancelOrderInfo 展开开始
    int64_t orig_client_seq_id;
    int64_t orig_clordno;
    // CancelOrderInfo 展开结束

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        orig_client_seq_id = 0;
        orig_clordno = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "orig_client_seq_id: " << orig_client_seq_id << "\n"
           << "orig_clordno: " << orig_clordno << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(CancelOrderReq)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id));      memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));         memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(orig_client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(orig_clordno));       memcpy(dest + off, &be64, 8); off += 8;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(CancelOrderReq)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); orig_client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); orig_clordno = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        return true;
    }
};

// TradeOrderER: 委托执行报告（TradeOrderUser、OrdERInfo 已展开）
// 注意：constituent_stock[0] 是零长度数组（成分券），不展开，也不计入 sizeof(TradeOrderER)
class TradeOrderER
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    // OrdERInfo 展开开始
    std::array<char, 16> order_id;
    std::array<char, 10> clordid;
    std::array<char, 8> security_id;
    uint16_t market_id;
    char exec_type;
    uint8_t ord_status;
    int64_t price;
    int64_t order_qty;
    int64_t leaves_qty;
    int64_t cum_qty;
    char side;
    int64_t transact_time;
    std::array<char, 64> user_info;
    std::array<char, 16> exec_id;
    std::array<char, 10> orig_clordid;
    char ord_type;
    uint16_t ord_rej_reason;            // 交易所错误码
    char time_in_force;
    int64_t last_px;
    int64_t last_qty;
    char cash_margin;
    char cancel_flag;
    int64_t clordno;
    int64_t orig_clordno;
    int64_t index;
    uint16_t code;                      // 内部错误码
    int64_t frozen_trade_value;         // 冻结交易金额
    int64_t frozen_fee;                 // 冻结费用
    int64_t fee;                        // 单笔成交费用
    int64_t total_value_traded;         // 成交金额
    uint8_t business_type;              // 业务类型
    uint32_t no_security;               // 成分券数量
    ConstituentStock constituent_stock[0];  // 成分股数组（数组，不展开）
    // OrdERInfo 展开结束

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        order_id.fill(' ');
        clordid.fill(' ');
        security_id.fill(' ');
        market_id = 0;
        exec_type = ' ';
        ord_status = 0;
        price = 0;
        order_qty = 0;
        leaves_qty = 0;
        cum_qty = 0;
        side = ' ';
        transact_time = 0;
        user_info.fill(' ');
        exec_id.fill(' ');
        orig_clordid.fill(' ');
        ord_type = ' ';
        ord_rej_reason = 0;
        time_in_force = ' ';
        last_px = 0;
        last_qty = 0;
        cash_margin = ' ';
        cancel_flag = ' ';
        clordno = 0;
        orig_clordno = 0;
        index = 0;
        code = 0;
        frozen_trade_value = 0;
        frozen_fee = 0;
        fee = 0;
        total_value_traded = 0;
        business_type = 0;
        no_security = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "order_id: " << AsString(order_id) << "\n"
           << "clordid: " << AsString(clordid) << "\n"
           << "security_id: " << AsString(security_id) << "\n"
           << "market_id: " << static_cast<int>(market_id) << "\n"
           << "exec_type: " << exec_type << "\n"
           << "ord_status: " << static_cast<uint16_t>(ord_status) << "\n"
           << "price: " << price << "\n"
           << "order_qty: " << order_qty << "\n"
           << "leaves_qty: " << leaves_qty << "\n"
           << "cum_qty: " << cum_qty << "\n"
           << "side: " << side << "\n"
           << "transact_time: " << transact_time << "\n"
           << "user_info: " << AsString(user_info) << "\n"
           << "exec_id: " << AsString(exec_id) << "\n"
           << "orig_clordid: " << AsString(orig_clordid) << "\n"
           << "ord_type: " << ord_type << "\n"
           << "ord_rej_reason: " << ord_rej_reason << "\n"
           << "time_in_force: " << time_in_force << "\n"
           << "last_px: " << last_px << "\n"
           << "last_qty: " << last_qty << "\n"
           << "cash_margin: " << cash_margin << "\n"
           << "cancel_flag: " << cancel_flag << "\n"
           << "clordno: " << clordno << "\n"
           << "orig_clordno: " << orig_clordno << "\n"
           << "index: " << index << "\n"
           << "code: " << code << "\n"
           << "frozen_trade_value: " << frozen_trade_value << "\n"
           << "frozen_fee: " << frozen_fee << "\n"
           << "fee: " << fee << "\n"
           << "total_value_traded: " << total_value_traded << "\n"
           << "business_type: " << static_cast<uint16_t>(business_type) << "\n"
           << "no_security: " << no_security << "\n";
        return os.str();
    }

    // 序列化固定部分（不含成分券数组），返回 sizeof(TradeOrderER)
    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(TradeOrderER)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));    memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, order_id.data(), 16); off += 16;
        memcpy(dest + off, clordid.data(), 10); off += 10;
        memcpy(dest + off, security_id.data(), 8); off += 8;
        uint16_t be16 = detail::HostToNetwork(market_id); memcpy(dest + off, &be16, 2); off += 2;
        memcpy(dest + off, &exec_type, 1); off += 1;
        memcpy(dest + off, &ord_status, 1); off += 1;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(price));       memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(order_qty));   memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(leaves_qty));  memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(cum_qty));     memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, &side, 1); off += 1;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(transact_time)); memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, user_info.data(), 64); off += 64;
        memcpy(dest + off, exec_id.data(), 16); off += 16;
        memcpy(dest + off, orig_clordid.data(), 10); off += 10;
        memcpy(dest + off, &ord_type, 1); off += 1;
        be16 = detail::HostToNetwork(ord_rej_reason); memcpy(dest + off, &be16, 2); off += 2;
        memcpy(dest + off, &time_in_force, 1); off += 1;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(last_px));  memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(last_qty)); memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, &cash_margin, 1); off += 1;
        memcpy(dest + off, &cancel_flag, 1); off += 1;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(clordno));       memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(orig_clordno));  memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(index));         memcpy(dest + off, &be64, 8); off += 8;
        be16 = detail::HostToNetwork(code); memcpy(dest + off, &be16, 2); off += 2;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(frozen_trade_value)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(frozen_fee));         memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(fee));                memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(total_value_traded)); memcpy(dest + off, &be64, 8); off += 8;
        memcpy(dest + off, &business_type, 1); off += 1;
        uint32_t be32 = detail::HostToNetwork(no_security); memcpy(dest + off, &be32, 4); off += 4;
        return off;
    }

    /**
     * @brief 序列化固定部分 + 成分券数组（ETF 申赎回报专用）。
     *
     * 当消息为 kPktEtfOrderMatch(2010) 且含成分券时使用。总长为
     * sizeof(TradeOrderER) + sizeof(ConstituentStock) * no_security。
     *
     * @param dest        输出缓冲区。
     * @param cap         缓冲区容量（需 >= 上述总长）。
     * @param no_security 成分券数量。
     * @param stocks      成分券数组（长度 >= no_security）。
     * @return 写入总字节数；容量不足返回 0。
     */
    size_t encode(char* dest, size_t cap, uint32_t no_security, const ConstituentStock* stocks) const
    {
        size_t fixed = sizeof(TradeOrderER);
        size_t total = fixed + sizeof(ConstituentStock) * no_security;
        if (cap < total) return 0;
        size_t n = encode(dest, cap);
        if (n == 0) return 0;
        for (uint32_t i = 0; i < no_security; ++i)
        {
            if (stocks[i].encode(dest + fixed + i * sizeof(ConstituentStock), sizeof(ConstituentStock)) == 0)
                return 0;
        }
        return total;
    }

    // 反序列化固定部分（不含成分券数组）
    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(TradeOrderER)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(order_id.data(), data + off, 16); off += 16;
        memcpy(clordid.data(), data + off, 10); off += 10;
        memcpy(security_id.data(), data + off, 8); off += 8;
        uint16_t be16;
        memcpy(&be16, data + off, 2); market_id = detail::HostToNetwork(be16); off += 2;
        memcpy(&exec_type, data + off, 1); off += 1;
        memcpy(&ord_status, data + off, 1); off += 1;
        memcpy(&be64, data + off, 8); price = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); order_qty = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); leaves_qty = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); cum_qty = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&side, data + off, 1); off += 1;
        memcpy(&be64, data + off, 8); transact_time = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(user_info.data(), data + off, 64); off += 64;
        memcpy(exec_id.data(), data + off, 16); off += 16;
        memcpy(orig_clordid.data(), data + off, 10); off += 10;
        memcpy(&ord_type, data + off, 1); off += 1;
        memcpy(&be16, data + off, 2); ord_rej_reason = detail::HostToNetwork(be16); off += 2;
        memcpy(&time_in_force, data + off, 1); off += 1;
        memcpy(&be64, data + off, 8); last_px = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); last_qty = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&cash_margin, data + off, 1); off += 1;
        memcpy(&cancel_flag, data + off, 1); off += 1;
        memcpy(&be64, data + off, 8); clordno = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); orig_clordno = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); index = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be16, data + off, 2); code = detail::HostToNetwork(be16); off += 2;
        memcpy(&be64, data + off, 8); frozen_trade_value = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); frozen_fee = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); fee = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); total_value_traded = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&business_type, data + off, 1); off += 1;
        uint32_t be32;
        memcpy(&be32, data + off, 4); no_security = detail::HostToNetwork(be32); off += 4;
        return true;
    }

    /**
     * @brief 反序列化固定部分 + 成分券数组（ETF 回报专用）。
     *
     * 用于接收 kPktEtfOrderMatch(2010) 回报。调用前可先用无成分券的重载
     * decode(data, len) 解析出 no_security，再据此分配 stocks 数组后调用本接口。
     *
     * @param data        报文数据（大端）。
     * @param len         可用字节数（需 >= 固定部分 + 成分券总长）。
     * @param no_security 成分券数量。
     * @param stocks      输出成分券数组（长度 >= no_security）。
     * @return 成功 true；长度不足或成分券解码失败返回 false。
     */
    bool decode(const char* data, size_t len, uint32_t no_security, ConstituentStock* stocks)
    {
        size_t fixed = sizeof(TradeOrderER);
        if (len < fixed + sizeof(ConstituentStock) * no_security) return false;
        if (!decode(data, len)) return false;
        for (uint32_t i = 0; i < no_security; ++i)
        {
            if (!stocks[i].decode(data + fixed + i * sizeof(ConstituentStock), sizeof(ConstituentStock)))
                return false;
        }
        return true;
    }
};

// RejectMsg: 拒绝返回消息（TradeOrderUser 已展开）
class RejectMsg
{
public:
    // TradeOrderUser 展开开始
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;       // 暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    // TradeOrderUser 展开结束
    uint16_t reject_reason_code;
    char cancel_flag;
    uint8_t business_type;              // 业务类型

    void reset()
    {
        fund_account_id.fill(' ');
        branch_id.fill(' ');
        account_id.fill(' ');
        cust_id.fill(' ');
        client_seq_id = 0;
        agw_seq_id = 0;
        reject_reason_code = 0;
        cancel_flag = ' ';
        business_type = 0;
    }

    std::string dump()
    {
        std::ostringstream os;
        os << "fund_account_id: " << AsString(fund_account_id) << "\n"
           << "branch_id: " << AsString(branch_id) << "\n"
           << "account_id: " << AsString(account_id) << "\n"
           << "cust_id: " << AsString(cust_id) << "\n"
           << "client_seq_id: " << client_seq_id << "\n"
           << "agw_seq_id: " << agw_seq_id << "\n"
           << "reject_reason_code: " << reject_reason_code << "\n"
           << "cancel_flag: " << cancel_flag << "\n"
           << "business_type: " << static_cast<uint16_t>(business_type) << "\n";
        return os.str();
    }

    size_t encode(char* dest, size_t cap) const
    {
        if (cap < sizeof(RejectMsg)) return 0;
        size_t off = 0;
        memcpy(dest + off, fund_account_id.data(), 16); off += 16;
        memcpy(dest + off, branch_id.data(), 10); off += 10;
        memcpy(dest + off, account_id.data(), 12); off += 12;
        memcpy(dest + off, cust_id.data(), 16); off += 16;
        uint64_t be64;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(client_seq_id)); memcpy(dest + off, &be64, 8); off += 8;
        be64 = detail::HostToNetwork(static_cast<uint64_t>(agw_seq_id));    memcpy(dest + off, &be64, 8); off += 8;
        uint16_t be16 = detail::HostToNetwork(reject_reason_code); memcpy(dest + off, &be16, 2); off += 2;
        memcpy(dest + off, &cancel_flag, 1); off += 1;
        memcpy(dest + off, &business_type, 1); off += 1;
        return off;
    }

    bool decode(const char* data, size_t len)
    {
        if (len < sizeof(RejectMsg)) return false;
        size_t off = 0;
        memcpy(fund_account_id.data(), data + off, 16); off += 16;
        memcpy(branch_id.data(), data + off, 10); off += 10;
        memcpy(account_id.data(), data + off, 12); off += 12;
        memcpy(cust_id.data(), data + off, 16); off += 16;
        uint64_t be64;
        memcpy(&be64, data + off, 8); client_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        memcpy(&be64, data + off, 8); agw_seq_id = static_cast<int64_t>(detail::HostToNetwork(be64)); off += 8;
        uint16_t be16;
        memcpy(&be16, data + off, 2); reject_reason_code = detail::HostToNetwork(be16); off += 2;
        memcpy(&cancel_flag, data + off, 1); off += 1;
        memcpy(&business_type, data + off, 1); off += 1;
        return true;
    }
};

#pragma pack(pop)

} // namespace gw_message
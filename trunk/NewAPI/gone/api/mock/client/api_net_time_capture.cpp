// api_net_time_capture.cpp - 网卡抓包时间分析程序（NewAPI 框架）
//
// 功能：抓取指定接口上发往特定端口的委托请求包，解析出 client_seq_id，
//       关联 mock_client 产出的 (client_seq_id -> api_arrive_time_ns) 映射文件，
//       计算「网卡发出时间戳 - api_arrive_time_ns」的端到端延迟，并统计指标。
//
// 实现：AF_PACKET socket（原生 Linux，不依赖 libpcap 头文件）
//       + PACKET_TIMESTAMP_MONOTONIC（单调时钟，与 mock_client 的 CLOCK_MONOTONIC 同基准）
//       + IPv4/TCP/应用层解析 + TCP 流重组（处理粘包/半包）
//
// 运行：需 root 权限（AF_PACKET SOCK_RAW）
//   sudo ./api_net_time_capture --iface lo --port 33001 --map /tmp/api_net_time_map.txt
//
// 用法:
//   api_net_time_capture [--iface <接口>] [--port <端口>] [--map <映射文件>]
//                        [--duration <秒>] [--report <报告文件>] [--max-packets <N>]
//
// 适配：NewAPI 委托包 client_seq_id 为小端（build_order_msg memcpy host 字节序），
//       网络偏移 62。详见 api_net_time_design.md。
#include "api_net_time_common.h"
#include "metric_stats.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>

#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

using perf::MetricStats;

// 兼容较旧内核头文件：PACKET_TIMESTAMP_MONOTONIC 在 linux/if_packet.h 中为 2
#ifndef PACKET_TIMESTAMP_MONOTONIC
#define PACKET_TIMESTAMP_MONOTONIC 2
#endif

namespace {

// ---- 命令行参数 ----
struct Options {
    std::string iface      = "lo";
    uint16_t    port       = 33001;                 // 抓取发往该端口的包（FTE 监听端口）
    std::string map_file   = "/tmp/api_net_time_map.txt";
    double      duration   = 0.0;                   // 抓包时长（秒），0 = 无限直到 Ctrl-C
    std::string report_file = "api_net_time_report.txt";
    int64_t     max_packets = -1;                   // 最大处理包数，-1 = 不限
    apinet::ProtoType proto = apinet::ProtoType::Gw; // 协议类型：gw（FTE）或 gone（FPGA）
};

void usage(const char* prog) {
    fprintf(stderr,
        "用法: %s [--iface <接口>] [--port <端口>] [--map <映射文件>]\n"
        "         [--duration <秒>] [--report <报告文件>] [--max-packets <N>]\n"
        "         [--proto <gw|gone>]\n"
        "  --proto gw   解析 FTE 委托包（PktNewHeader+TradeOrderReq，msg_id=1003，偏移62）默认\n"
        "  --proto gone 解析 GOne 委托包（g1_msg_head+order_req，msg_id=1001，偏移40）\n"
        "示例: sudo %s --iface lo --port 33001 --map /tmp/api_net_time_map.txt --duration 10\n"
        "      sudo %s --iface lo --port 44001 --map /tmp/gone_map.txt --proto gone --duration 10\n",
        prog, prog, prog);
}

bool parse_args(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--iface" && i + 1 < argc)       opt.iface = argv[++i];
        else if (a == "--port" && i + 1 < argc)   opt.port = (uint16_t)atoi(argv[++i]);
        else if (a == "--map" && i + 1 < argc)    opt.map_file = argv[++i];
        else if (a == "--duration" && i + 1 < argc) opt.duration = atof(argv[++i]);
        else if (a == "--report" && i + 1 < argc) opt.report_file = argv[++i];
        else if (a == "--max-packets" && i + 1 < argc) opt.max_packets = atoll(argv[++i]);
        else if (a == "--proto" && i + 1 < argc) {
            std::string p = argv[++i];
            if (p == "gw") opt.proto = apinet::ProtoType::Gw;
            else if (p == "gone") opt.proto = apinet::ProtoType::Gone;
            else { fprintf(stderr, "未知协议: %s（应为 gw 或 gone）\n", p.c_str()); return false; }
        }
        else if (a == "-h" || a == "--help") { usage(argv[0]); return false; }
        else { fprintf(stderr, "未知参数: %s\n", a.c_str()); usage(argv[0]); return false; }
    }
    return true;
}

// ---- TCP 流重组 ----
struct StreamKey {
    uint32_t src_ip, dst_ip;
    uint16_t src_port, dst_port;
    bool operator<(const StreamKey& o) const {
        if (src_ip != o.src_ip) return src_ip < o.src_ip;
        if (dst_ip != o.dst_ip) return dst_ip < o.dst_ip;
        if (src_port != o.src_port) return src_port < o.src_port;
        return dst_port < o.dst_port;
    }
};

struct TcpStream {
    uint32_t src_ip = 0, dst_ip = 0;
    uint16_t src_port = 0, dst_port = 0;
    uint32_t next_seq = 0;          // 期望的下一个 seq
    bool initialized = false;
    std::vector<uint8_t> buf;       // 已按 seq 顺序累积的应用字节流
};

// 启发式定位 IPv4 头在帧内的偏移
//   - 直接 IP（loopback 无链路层头）
//   - 以太网头 14 字节（data[12..13]=0x0800）
//   - Linux cooked SLL 头 16 字节（data[14..15]=0x0800）
int find_ip_offset(const uint8_t* data, int len) {
    if (len >= 20 && (data[0] >> 4) == 4) return 0;                 // loopback
    if (len >= 34 && data[12] == 0x08 && data[13] == 0x00) return 14; // 以太网
    if (len >= 36 && data[14] == 0x08 && data[15] == 0x00) return 16; // SLL
    return -1;
}

} // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parse_args(argc, argv, opt)) return 1;

    // 1. 打开 AF_PACKET socket
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) { perror("socket(AF_PACKET)"); return 1; }

    // 2. 时间戳设为单调时钟（与 mock_client 的 CLOCK_MONOTONIC 同基准，可直接相减）
    int ts_type = PACKET_TIMESTAMP_MONOTONIC;
    if (setsockopt(fd, SOL_PACKET, PACKET_TIMESTAMP, &ts_type, sizeof(ts_type)) < 0) {
        perror("setsockopt(PACKET_TIMESTAMP)");   // 非致命：默认时间戳亦可，后续校准
    }
    // 必须在 socket 上启用 SO_TIMESTAMPNS，recvmsg 才会通过 CMSG 返回时间戳
    int ts_on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPNS, &ts_on, sizeof(ts_on)) < 0) {
        perror("setsockopt(SO_TIMESTAMPNS)");
    }

    // 3. 增大接收缓冲，降低高 TPS 下丢包概率
    int rcvbuf = 16 * 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    // 4. 设置接收超时（1 秒），让 recvmsg 循环能定期检查 duration 超时
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 4. bind 到指定接口
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex  = if_nametoindex(opt.iface.c_str());
    if (sll.sll_ifindex == 0) { perror("if_nametoindex"); close(fd); return 1; }
    if (bind(fd, (struct sockaddr*)&sll, sizeof(sll)) < 0) {
        perror("bind"); close(fd); return 1;
    }

    // 校准时钟：容器/旧内核可能不支持 PACKET_TIMESTAMP_MONOTONIC，net_time 可能为
    // CLOCK_REALTIME。计算 offset = realtime - monotonic，把 net_time 统一转到 CLOCK_MONOTONIC，
    // 与 mock_client 的 perf_now_ns()（CLOCK_MONOTONIC）对齐。
    struct timespec mono_ts, real_ts;
    clock_gettime(CLOCK_MONOTONIC, &mono_ts);
    clock_gettime(CLOCK_REALTIME, &real_ts);
    int64_t real_to_mono_offset =
        ((int64_t)real_ts.tv_sec * 1000000000LL + real_ts.tv_nsec) -
        ((int64_t)mono_ts.tv_sec * 1000000000LL + mono_ts.tv_nsec);

    printf("[api_net_time_capture] 开始抓包: iface=%s port=%d map=%s duration=%.1fs "
           "clock_offset_realtime_minus_monotonic=%lldns\n",
           opt.iface.c_str(), opt.port, opt.map_file.c_str(), opt.duration,
           (long long)real_to_mono_offset);
    fflush(stdout);

    // ---- 抓包主循环 ----
    std::map<StreamKey, TcpStream> streams;
    std::vector<std::pair<int64_t, uint64_t>> captures; // (client_seq_id, 网卡发出时间戳 ns)
    uint64_t total_pkts = 0, to_port_pkts = 0, order_pkts = 0;
    int64_t processed = 0;

    auto start = std::chrono::steady_clock::now();
    bool timed_out = false;

    char buf[65536];
    struct iovec iov;
    struct msghdr msg;
    char cbuf[CMSG_SPACE(sizeof(struct timespec))];

    while (!timed_out) {
        if (opt.max_packets >= 0 && processed >= opt.max_packets) break;
        if (opt.duration > 0) {
            auto now = std::chrono::steady_clock::now();
            double el = std::chrono::duration_cast<std::chrono::duration<double>>(now - start).count();
            if (el >= opt.duration) break;
        }

        iov.iov_base = buf;
        iov.iov_len  = sizeof(buf);
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov      = &iov;
        msg.msg_iovlen   = 1;
        msg.msg_control  = cbuf;
        msg.msg_controllen = sizeof(cbuf);

        ssize_t n = recvmsg(fd, &msg, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;  // SO_RCVTIMEO 超时，继续以检查 duration
            perror("recvmsg"); break;
        }
        processed++;

        // 时间戳：优先用 PACKET_TIMESTAMP_MONOTONIC；若未生效（旧内核返回 realtime），
        // 用启动时校准的 offset 统一转到 CLOCK_MONOTONIC，与 mock_client 对齐。
        struct timespec ts = {0, 0};
        for (struct cmsghdr* c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
            if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SO_TIMESTAMPNS) {
                ts = *(struct timespec*)CMSG_DATA(c);
            }
        }
        int64_t raw_net_ns = (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
        // 判断时钟源：若 raw_net_ns 在启动时 monotonic 附近（±1 天内），则是 monotonic；
        // 否则为 realtime，需减去启动时校准的 offset 转到 monotonic。
        int64_t now_mono_ns =
            (int64_t)mono_ts.tv_sec * 1000000000LL + (int64_t)mono_ts.tv_nsec;
        const int64_t kDayNs = 86400LL * 1000000000LL;
        uint64_t net_time_ns;
        if (raw_net_ns > now_mono_ns - kDayNs && raw_net_ns < now_mono_ns + kDayNs) {
            net_time_ns = (uint64_t)raw_net_ns;                 // 已是 monotonic
        } else {
            net_time_ns = (uint64_t)(raw_net_ns - real_to_mono_offset); // realtime -> monotonic
        }

        const uint8_t* data = (const uint8_t*)buf;
        int len = (int)n;
        total_pkts++;

        // 定位 IPv4 头
        int ip_off = find_ip_offset(data, len);
        if (ip_off < 0 || ip_off + 20 > len) continue;
        const uint8_t* ip = data + ip_off;
        if ((ip[0] >> 4) != 4) continue;          // 仅 IPv4
        int ihl = (ip[0] & 0x0F) * 4;
        if (ip[9] != 6) continue;                 // 仅 TCP
        if (ip_off + ihl + 20 > len) continue;

        uint32_t src_ip = apinet::be32(ip + 12);
        uint32_t dst_ip = apinet::be32(ip + 16);
        const uint8_t* tcp = ip + ihl;
        uint16_t src_port = apinet::be16(tcp);
        uint16_t dst_port = apinet::be16(tcp + 2);
        uint32_t seg_seq  = apinet::be32(tcp + 4);
        int tcp_off = ((tcp[12] >> 4) & 0x0F) * 4;
        int app_off = ip_off + ihl + tcp_off;
        if (app_off > len) continue;
        int app_len = len - app_off;

        // 只处理发往目标端口（mock_client -> FTE）且带载荷的包
        if (dst_port != opt.port) continue;
        to_port_pkts++;
        if (app_len <= 0) continue;

        // TCP 流重组（按 4 元组 + seq 累计）
        StreamKey key{src_ip, dst_ip, src_port, dst_port};
        TcpStream& st = streams[key];
        if (!st.initialized) {
            st.initialized = true;
            st.src_ip = src_ip; st.dst_ip = dst_ip;
            st.src_port = src_port; st.dst_port = dst_port;
            st.next_seq = seg_seq;
        }
        if (seg_seq == st.next_seq) {
            st.buf.insert(st.buf.end(), data + app_off, data + app_off + app_len);
            st.next_seq += (uint32_t)app_len;
        } else if (seg_seq < st.next_seq) {
            continue;   // 重传/重叠，忽略（简化）
        } else {
            continue;   // 乱序到达，丢弃（简化）
        }

        // 从流中切分完整委托包（按协议解析）
        while (st.buf.size() >= 8) {
            size_t pkt_len = 0;
            int64_t cseq = 0;
            bool is_order = false;

            if (opt.proto == apinet::ProtoType::Gw) {
                // gw (FTE) 协议：PktNewHeader 大端 + TradeOrderReq + 校验和
                uint32_t msg_id  = apinet::be32(st.buf.data());
                uint32_t msg_len = apinet::be32(st.buf.data() + 4);
                pkt_len = apinet::kPktHeaderLen + msg_len + apinet::kChecksumLen;
                if (msg_len != apinet::kTradeOrderReqLen || pkt_len > st.buf.size()) {
                    if (st.buf.size() > 8192) st.buf.clear();
                    break;
                }
                if (msg_id == apinet::kPktOrderReq) {
                    cseq = apinet::extract_client_seq_id(st.buf.data());
                    is_order = true;
                }
            } else {
                // GOne 协议：g1_msg_head 小端 + order_req，无校验和
                uint32_t msg_id  = apinet::le32(st.buf.data());
                uint32_t msg_len = apinet::le32(st.buf.data() + 4);
                pkt_len = apinet::kG1MsgHeadLen + msg_len;
                if (msg_len != apinet::kOrderReqLen || pkt_len > st.buf.size()) {
                    if (st.buf.size() > 8192) st.buf.clear();
                    break;
                }
                if (msg_id == apinet::kG1MsgOrderReq) {
                    cseq = apinet::extract_gone_client_seq_id(st.buf.data());
                    is_order = true;
                }
            }

            if (is_order) {
                captures.emplace_back(cseq, net_time_ns);
                order_pkts++;
            }
            st.buf.erase(st.buf.begin(), st.buf.begin() + pkt_len);
        }
    }

    close(fd);

    // ---- 关联映射文件，计算延迟 ----
    apinet::SeqArriveMap seq_map;
    if (!apinet::load_map_file(opt.map_file, seq_map)) {
        fprintf(stderr, "[api_net_time_capture] 警告: 无法加载映射文件 %s\n", opt.map_file.c_str());
    }

    std::vector<uint64_t> lats;
    size_t matched = 0, unmatched = 0;
    for (auto& c : captures) {
        auto it = seq_map.find(c.first);
        if (it != seq_map.end() && c.second >= it->second) {
            lats.push_back(c.second - it->second);
            matched++;
        } else {
            unmatched++;
        }
    }

    MetricStats ms;
    ms.compute(lats);

    // ---- 输出报告 ----
    std::string report;
    {
        char line[1024];
        snprintf(line, sizeof(line),
            "========== 网卡抓包时间分析报告 ==========\n"
            "协议类型   : %s\n"
            "抓包接口   : %s\n"
            "目标端口   : %u\n"
            "抓包时长   : %.1f 秒\n"
            "处理包数   : %lld\n"
            "发往目标端口包数: %lld\n"
            "捕获委托包 : %lld\n"
            "映射文件   : %s\n"
            "关联成功   : %zu\n"
            "关联失败   : %zu\n"
            "---------------- 延迟指标（网卡发出时间 - api_arrive_time_ns, 纳秒）----------------\n"
            "样本数     : %zu\n"
            "平均值     : %.1f\n"
            "P50        : %llu\n"
            "P75        : %llu\n"
            "P90        : %llu\n"
            "P95        : %llu\n"
            "最大值     : %llu\n"
            "最小值     : %llu\n"
            "标准差     : %.1f\n"
            "==========================================\n",
            opt.proto == apinet::ProtoType::Gw ? "gw (FTE)" : "gone (FPGA)",
            opt.iface.c_str(), opt.port, opt.duration,
            (long long)processed, (long long)to_port_pkts, (long long)order_pkts,
            opt.map_file.c_str(), matched, unmatched,
            ms.count, ms.mean,
            (unsigned long long)ms.p50, (unsigned long long)ms.p75,
            (unsigned long long)ms.p90, (unsigned long long)ms.p95,
            (unsigned long long)ms.max, (unsigned long long)ms.min, ms.stddev);
        report = line;
    }

    printf("%s", report.c_str());
    fflush(stdout);

    FILE* fp = fopen(opt.report_file.c_str(), "w");
    if (fp) { fputs(report.c_str(), fp); fclose(fp); printf("报告已保存到: %s\n", opt.report_file.c_str()); }
    else fprintf(stderr, "无法打开报告文件: %s\n", opt.report_file.c_str());

    return 0;
}
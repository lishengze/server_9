#ifndef MOCK_98_CLIENT_SESSION_H
#define MOCK_98_CLIENT_SESSION_H

#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <ctime>

#include "agw_protocol.h"
#include "account_manager.h"

namespace mock_98 {

/// 客户端会话
/// 处理 AGW 登录 / 账户登录 / 心跳 / 委托 / 撤单 / 各类查询
/// 使用 Archforce AGW 协议 (raw_agw_header + body + checksum)
class ClientSession {
public:
    ClientSession(int fd, AccountManager* acct_mgr);
    ~ClientSession();

    /// 获取套接字
    int fd() const { return fd_; }

    /// 处理接收数据（支持 TCP 粘包/拆包，内部维护接收缓冲区）
    /// @return 0=正常, -1=连接关闭, -2=协议错误
    int feed_data(const char* data, size_t len);

    /// 关闭会话
    void close();

    /// 标记为已断开（由客户端线程调用）
    void mark_disconnected() { disconnected_ = true; }

    /// 检查是否已断开
    bool is_disconnected() const { return disconnected_; }

    /// 检查是否心跳超时
    bool is_timeout(int timeout_sec) const;

private:
    /// 处理单条完整消息
    void handle_message(const raw_agw_header &hdr, const char *body, int32_t body_len);

    /// 发送数据
    void send_data(const char *data, int32_t len);

    /// 发送 AGW 消息 (header + body + checksum)
    void send_agw_msg(uint32_t msg_type, uint16_t msg_ver, const char *body, uint32_t body_len);

    /// 发送 AGW 消息 (从 SerializeBuffer)
    void send_agw_msg(uint32_t msg_type, uint16_t msg_ver, const lb_counter98::SerializeBuffer &buf);

    // ---- 各消息类型处理器 ----
    void handle_agw_login(const char *body, int32_t body_len);
    void handle_cust_login(const char *body, int32_t body_len);
    void handle_heartbeat(const char *body, int32_t body_len);
    void handle_order(const char *body, int32_t body_len);
    void handle_cancel(const char *body, int32_t body_len);
    void handle_query_order(const char *body, int32_t body_len);
    void handle_query_trade(const char *body, int32_t body_len);
    void handle_query_fund(const char *body, int32_t body_len);
    void handle_query_position(const char *body, int32_t body_len);
    void handle_query_max_order_qty(const char *body, int32_t body_len);
    void handle_query_total_fund(const char *body, int32_t body_len);
    void handle_query_fund_maxdraw(const char *body, int32_t body_len);
    void handle_query_cust_margin_avl(const char *body, int32_t body_len);
    void handle_query_asset_debts(const char *body, int32_t body_len);
    void handle_query_credit_max_order_qty(const char *body, int32_t body_len);

    // ---- 连接管理 ----
    int fd_;
    AccountManager* acct_mgr_;
    time_t last_heartbeat_;
    bool disconnected_;
    bool closed_;
    bool agw_logged_in_;
    std::string agw_session_;               ///< AGW 登录后的 session
    std::set<std::string> logged_accounts_; ///< 已登录的资金账号

    // TCP 接收缓冲区（处理粘包/拆包）
    std::vector<char> recv_buf_;
    static const size_t MAX_RECV_BUF = 65536;
};

} // namespace mock_98

#endif // MOCK_98_CLIENT_SESSION_H

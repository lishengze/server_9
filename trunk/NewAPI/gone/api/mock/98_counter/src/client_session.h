#ifndef MOCK_98_CLIENT_SESSION_H
#define MOCK_98_CLIENT_SESSION_H

#include "message_parser.h"
#include "account_manager.h"
#include <string>
#include <ctime>
#include <cstdint>

namespace mock_98 {

/// 客户端会话
class ClientSession {
public:
    ClientSession(int fd, AccountManager* acct_mgr);
    ~ClientSession();

    /// 获取套接字
    int fd() const { return fd_; }

    /// 处理接收数据（支持 TCP 粘包/拆包，内部维护接收缓冲区）
    /// @return 0=正常, -1=连接关闭, -2=协议错误
    int feed_data(const char* data, size_t len);

    /// 处理单条完整消息（由 feed_data 内部调用）
    int handle_message(const char* data, size_t len);

    /// 检查是否心跳超时
    bool is_timeout(int timeout_sec) const;

    /// 关闭会话
    void close();

    /// 标记为已断开（由客户端线程调用）
    void mark_disconnected() { disconnected_ = true; }

    /// 检查是否已断开
    bool is_disconnected() const { return disconnected_; }

    /// 是否已登录
    bool is_agw_logged_in() const { return agw_logged_in_; }
    bool is_account_logged_in() const { return account_logged_in_; }

    /// 获取会话信息
    std::string session_id() const { return session_id_; }
    std::string fund_account_id() const { return fund_account_id_; }

private:
    /// 处理 AGW 登录请求
    int handle_agw_login(const c98_agw_login_req* req);

    /// 处理账户登录请求
    int handle_acc_login(const c98_acc_login_req* req);

    /// 处理心跳
    int handle_heartbeat();

    /// 发送消息
    bool send_message(uint32_t msg_id, const void* body, uint32_t body_len);

    /// 发送 AGW 登录应答
    bool send_agw_login_ans(int64_t client_req_no, int32_t err_code, const char* err_msg);

    /// 发送账户登录应答
    bool send_acc_login_ans(const c98_acc_login_req* req, int32_t err_code, const char* err_msg);

    int fd_;
    AccountManager* acct_mgr_;
    time_t last_heartbeat_;
    bool agw_logged_in_;
    bool account_logged_in_;
    bool disconnected_;
    std::string session_id_;
    std::string agw_user_;
    std::string fund_account_id_;

    // TCP 接收缓冲区（处理粘包/拆包）
    std::vector<char> recv_buf_;
    static const size_t MAX_RECV_BUF = 65536;
};

} // namespace mock_98

#endif // MOCK_98_CLIENT_SESSION_H

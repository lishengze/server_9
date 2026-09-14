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

    /// 处理接收到的消息
    /// @return 0=正常, -1=连接关闭, -2=协议错误
    int handle_data(const char* data, size_t len);

    /// 检查是否心跳超时
    bool is_timeout(int timeout_sec) const;

    /// 关闭会话
    void close();

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
    std::string session_id_;
    std::string agw_user_;
    std::string fund_account_id_;
};

} // namespace mock_98

#endif // MOCK_98_CLIENT_SESSION_H

#include "client_session.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <algorithm>

namespace mock_98 {

ClientSession::ClientSession(int fd, AccountManager* acct_mgr)
    : fd_(fd)
    , acct_mgr_(acct_mgr)
    , last_heartbeat_(std::time(nullptr))
    , agw_logged_in_(false)
    , account_logged_in_(false)
    , disconnected_(false)
{
}

ClientSession::~ClientSession() {
    close();
}

int ClientSession::feed_data(const char* data, size_t len) {
    if (!data || len == 0) return 0;

    // 防止缓冲区溢出
    if (recv_buf_.size() + len > MAX_RECV_BUF) {
        std::cerr << "[Session] 接收缓冲区溢出, 清空缓冲区" << std::endl;
        recv_buf_.clear();
        return -2;
    }

    // 追加到接收缓冲区
    recv_buf_.insert(recv_buf_.end(), data, data + len);

    // 循环处理缓冲区中所有完整消息
    while (recv_buf_.size() >= sizeof(c98_msg_head_tmp)) {
        // 解析消息头
        c98_msg_head_tmp head;
        std::memcpy(&head, recv_buf_.data(), sizeof(c98_msg_head_tmp));

        size_t msg_total = sizeof(c98_msg_head_tmp) + head.msg_len;
        if (recv_buf_.size() < msg_total) {
            // 消息体尚未收全，等待更多数据
            break;
        }

        // 处理完整消息
        int ret = handle_message(recv_buf_.data(), msg_total);
        if (ret < 0) {
            recv_buf_.clear();
            return ret;
        }

        // 从缓冲区移除已处理的消息
        recv_buf_.erase(recv_buf_.begin(), recv_buf_.begin() + msg_total);
    }

    return 0;
}

int ClientSession::handle_message(const char* data, size_t len) {
    if (len < sizeof(c98_msg_head_tmp)) {
        std::cerr << "[Session] 数据长度不足: " << len << std::endl;
        return -2;
    }

    c98_msg_head_tmp head;
    if (!MessageParser::parse_header(data, len, head)) {
        std::cerr << "[Session] 消息头解析失败" << std::endl;
        return -2;
    }

    size_t expected = sizeof(c98_msg_head_tmp) + head.msg_len;
    if (len < expected) {
        std::cerr << "[Session] 消息体不完整: 需要 " << expected
                  << ", 实际 " << len << std::endl;
        return -2;
    }

    const char* body = MessageParser::get_body(data, len, head);
    std::cout << "[Session] 收到消息: " << MessageParser::msg_type_name(head.msg_id)
              << ", msg_len=" << head.msg_len << ", seq_no=" << head.seq_no << std::endl;

    switch (head.msg_id) {
        case C98_MSG_AGW_LOGIN_REQ:
            if (body && head.msg_len >= sizeof(c98_agw_login_req)) {
                return handle_agw_login(reinterpret_cast<const c98_agw_login_req*>(body));
            }
            break;

        case C98_MSG_ACC_LOGIN_REQ:
            if (body && head.msg_len >= sizeof(c98_acc_login_req)) {
                return handle_acc_login(reinterpret_cast<const c98_acc_login_req*>(body));
            }
            break;

        case C98_MSG_HEART_REQ:
            return handle_heartbeat();

        default:
            std::cout << "[Session] 忽略未知消息: " << head.msg_id << std::endl;
            return 0;
    }

    return 0;
}

bool ClientSession::is_timeout(int timeout_sec) const {
    return (std::time(nullptr) - last_heartbeat_) > timeout_sec;
}

void ClientSession::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    disconnected_ = true;
    std::cout << "[Session] 会话已关闭"
              << ", agw_user=" << agw_user_
              << ", fund_account=" << fund_account_id_ << std::endl;
}

int ClientSession::handle_agw_login(const c98_agw_login_req* req) {
    std::string user(req->agw_user, strnlen(req->agw_user, sizeof(req->agw_user)));
    std::string password(req->agw_user_password, strnlen(req->agw_user_password, sizeof(req->agw_user_password)));

    std::cout << "[Session] AGW 登录请求: user=" << user
              << ", client_req_no=" << req->client_req_no << std::endl;

    if (acct_mgr_->verify_agw_user(user, password)) {
        agw_user_ = user;
        session_id_ = acct_mgr_->generate_session_id();
        agw_logged_in_ = true;
        last_heartbeat_ = std::time(nullptr);

        std::cout << "[Session] AGW 登录成功, session=" << session_id_ << std::endl;
        send_agw_login_ans(req->client_req_no, 0, "OK");
    } else {
        std::cout << "[Session] AGW 登录失败: 用户名或密码错误" << std::endl;
        send_agw_login_ans(req->client_req_no, 1001, "Invalid AGW user or password");
    }

    return 0;
}

int ClientSession::handle_acc_login(const c98_acc_login_req* req) {
    if (!agw_logged_in_) {
        std::cout << "[Session] 账户登录失败: AGW 未登录" << std::endl;
        send_acc_login_ans(req, 1002, "AGW not logged in");
        return 0;
    }

    std::string fund_account(req->fund_account_id, strnlen(req->fund_account_id, sizeof(req->fund_account_id)));
    std::string password(req->password, strnlen(req->password, sizeof(req->password)));

    std::cout << "[Session] 账户登录请求: fund_account=" << fund_account
              << ", client_req_no=" << req->client_req_no << std::endl;

    if (acct_mgr_->verify_account(fund_account, password)) {
        fund_account_id_ = fund_account;
        account_logged_in_ = true;
        last_heartbeat_ = std::time(nullptr);

        std::cout << "[Session] 账户登录成功: " << fund_account << std::endl;
        send_acc_login_ans(req, 0, "OK");
    } else {
        std::cout << "[Session] 账户登录失败: 账号或密码错误" << std::endl;
        send_acc_login_ans(req, 1003, "Invalid account or password");
    }

    return 0;
}

int ClientSession::handle_heartbeat() {
    last_heartbeat_ = std::time(nullptr);
    send_message(C98_MSG_HEART_ANS, nullptr, 0);
    return 0;
}

bool ClientSession::send_message(uint32_t msg_id, const void* body, uint32_t body_len) {
    size_t total = sizeof(c98_msg_head_tmp) + body_len;
    char* buf = new char[total];

    int n = MessageParser::build_message(buf, total, msg_id, body, body_len, 0);
    if (n < 0) {
        delete[] buf;
        return false;
    }

    ssize_t sent = ::send(fd_, buf, total, 0);
    delete[] buf;

    if (sent < 0 || (size_t)sent != total) {
        std::cerr << "[Session] 发送消息失败: msg_id=" << msg_id << std::endl;
        return false;
    }

    std::cout << "[Session] 发送消息: " << MessageParser::msg_type_name(msg_id)
              << ", len=" << total << std::endl;
    return true;
}

bool ClientSession::send_agw_login_ans(int64_t client_req_no, int32_t err_code, const char* err_msg) {
    c98_agw_login_ans ans;
    std::memset(&ans, 0, sizeof(ans));

    ans.client_req_no = client_req_no;
    std::memcpy(ans.agw_user, agw_user_.c_str(),
                std::min(agw_user_.size(), sizeof(ans.agw_user)));
    std::memcpy(ans.session, session_id_.c_str(),
                std::min(session_id_.size(), sizeof(ans.session)));
    ans.err_code = err_code;
    if (err_msg) {
        std::memcpy(ans.err_msg, err_msg, std::min(strlen(err_msg), sizeof(ans.err_msg) - 1));
    }

    return send_message(C98_MSG_AGW_LOGIN_ANS, &ans, sizeof(ans));
}

bool ClientSession::send_acc_login_ans(const c98_acc_login_req* req, int32_t err_code, const char* err_msg) {
    c98_acc_login_ans ans;
    std::memset(&ans, 0, sizeof(ans));

    ans.client_req_no = req->client_req_no;
    std::memcpy(ans.cust_id, req->cust_id, sizeof(ans.cust_id));
    std::memcpy(ans.fund_account_id, req->fund_account_id, sizeof(ans.fund_account_id));
    std::memcpy(ans.branch_id, req->branch_id, sizeof(ans.branch_id));
    std::memcpy(ans.account_id, req->account_id, sizeof(ans.account_id));
    std::memcpy(ans.password, req->password, sizeof(ans.password));
    std::memcpy(ans.session, session_id_.c_str(),
                std::min(session_id_.size(), sizeof(ans.session)));
    std::memcpy(ans.end_code, req->end_code, sizeof(ans.end_code));
    ans.market_type = req->market_type;
    ans.heart_bt_int = req->heart_bt_int;
    std::memcpy(ans.order_way_ext, req->order_way, sizeof(ans.order_way_ext));
    ans.err_code = err_code;
    if (err_msg) {
        std::memcpy(ans.err_msg, err_msg, std::min(strlen(err_msg), sizeof(ans.err_msg) - 1));
    }
    ans.login_time = static_cast<int32_t>(std::time(nullptr));

    return send_message(C98_MSG_ACC_LOGIN_ANS, &ans, sizeof(ans));
}

} // namespace mock_98

#ifndef MOCK_GONE_CLIENT_SESSION_H
#define MOCK_GONE_CLIENT_SESSION_H

#include "message_parser.h"
#include "account_manager.h"
#include <string>
#include <vector>
#include <ctime>
#include <cstdint>

namespace mock_gone {

/// 链路类型：GW = 网关链接（处理 sec_info/login/heart），CORE = 核心交易链接（处理 order/cancel/heart）
enum class LinkType { GW = 0, CORE = 1 };

/// 客户端会话
class ClientSession {
public:
    ClientSession(int fd, AccountManager* acct_mgr, LinkType link_type, int core_port);
    ~ClientSession();

    /// 获取套接字
    int fd() const { return fd_; }

    /// 获取链路类型
    LinkType link_type() const { return link_type_; }

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

    /// 是否已登录（仅 GW 链路有效）
    bool is_logged_in() const { return logged_in_; }

    /// 获取登录信息
    uint16_t user_id() const { return user_id_; }
    uint16_t board_no() const { return board_no_; }
    uint32_t session_id() const { return session_id_; }
    std::string fund_account_id() const { return fund_account_id_; }

private:
    /// 处理证券信息请求（仅 GW 链路）
    int handle_sec_info_req(const g1_msg_head* head);

    /// 处理登录请求（仅 GW 链路）
    int handle_login_req(const g1_msg_head* head);

    /// 处理心跳请求（GW/CORE 链路均需处理）
    int handle_heart_req(const g1_msg_head* head);

    /// 处理委托请求（仅 CORE 链路）
    int handle_order_req(const g1_msg_head* head);

    /// 处理撤单请求（仅 CORE 链路）
    int handle_cancel_req(const g1_msg_head* head);

    /// 发送消息
    bool send_message(uint32_t msg_id, const void* body, uint32_t body_len,
                      uint16_t board_no, uint16_t user_id, uint32_t session_id);

    /// 发送心跳应答
    bool send_heart_ans();

    /// 发送证券信息应答
    bool send_sec_info_ans(int64_t cust_req_no);

    /// 发送登录应答
    bool send_login_ans(const login_req* req, uint32_t err_code, const char* err_msg);

    /// 发送委托回报
    bool send_order_rtn(const order_req* req);

    /// 发送成交回报（委托后主动推送，模拟成交）
    bool send_trade_rtn(const order_req* req);

    /// 发送撤单响应
    bool send_cancel_rsp(const cancel_req* req);

    int fd_;
    AccountManager* acct_mgr_;
    LinkType link_type_;    ///< 链路类型（GW 或 CORE）
    int core_port_;         ///< Core 链路端口（GW 链路 login_ans 的 trade_port 填此值）
    time_t last_heartbeat_;
    bool logged_in_;
    bool disconnected_;
    std::string fund_account_id_;

    // 登录后分配的信息
    uint16_t user_id_;
    uint16_t board_no_;
    uint32_t session_id_;

    // 委托计数器（生成 order_sys_no）
    int64_t order_counter_;

    // TCP 接收缓冲区（处理粘包/拆包）
    std::vector<char> recv_buf_;
    static const size_t MAX_RECV_BUF = 65536;
};

} // namespace mock_gone

#endif // MOCK_GONE_CLIENT_SESSION_H

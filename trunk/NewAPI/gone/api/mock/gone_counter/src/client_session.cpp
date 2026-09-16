#include "client_session.h"
#include "session_registry.h"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

namespace mock_gone {

ClientSession::ClientSession(int fd, AccountManager* acct_mgr, LinkType link_type, int core_port)
    : fd_(fd)
    , acct_mgr_(acct_mgr)
    , link_type_(link_type)
    , core_port_(core_port)
    , last_heartbeat_(std::time(nullptr))
    , logged_in_(false)
    , disconnected_(false)
    , user_id_(0)
    , board_no_(0)
    , session_id_(0)
    , order_counter_(10000)
{
}

ClientSession::~ClientSession() {
    close();
}

int ClientSession::feed_data(const char* data, size_t len) {
    // 追加到接收缓冲区
    recv_buf_.insert(recv_buf_.end(), data, data + len);
    if (recv_buf_.size() > MAX_RECV_BUF) {
        std::cerr << "[Session] 接收缓冲区溢出, fd=" << fd_ << std::endl;
        return -2;
    }

    // 循环解析完整消息
    size_t offset = 0;
    while (recv_buf_.size() - offset >= sizeof(g1_msg_head)) {
        g1_msg_head head;
        std::memcpy(&head, recv_buf_.data() + offset, sizeof(g1_msg_head));

        size_t total_len = sizeof(g1_msg_head) + head.msg_len;
        if (recv_buf_.size() - offset < total_len) {
            break; // 消息不完整，等待更多数据
        }

        // 处理完整消息
        int ret = handle_message(recv_buf_.data() + offset, total_len);
        if (ret < 0) {
            return ret;
        }

        offset += total_len;
    }

    // 移除已处理的数据
    if (offset > 0) {
        recv_buf_.erase(recv_buf_.begin(), recv_buf_.begin() + offset);
    }

    return 0;
}

int ClientSession::handle_message(const char* data, size_t len) {
    g1_msg_head head;
    std::memcpy(&head, data, sizeof(g1_msg_head));

    std::string link_name = (link_type_ == LinkType::GW) ? "GW" : "CORE";
    std::cout << "[Session][" << link_name << "] 收到消息: " << MessageParser::msg_type_name(head.msg_id)
              << ", msg_len=" << head.msg_len << ", fd=" << fd_ << std::endl;

    // 注意：传递 data（原始缓冲区指针）而非 &head（栈上临时变量），
    // 因为处理函数中 head+1 需要指向消息体数据
    const g1_msg_head* msg_head = reinterpret_cast<const g1_msg_head*>(data);

    switch (head.msg_id) {
    case G1_MSG_SEC_INFO_REQ:
        if (link_type_ != LinkType::GW) {
            std::cerr << "[Session][" << link_name << "] 错误: GW 消息 sec_info 出现在 Core 链路" << std::endl;
            return 0;
        }
        return handle_sec_info_req(msg_head);
    case G1_MSG_LOGIN_REQ:
        if (link_type_ != LinkType::GW) {
            std::cerr << "[Session][" << link_name << "] 错误: GW 消息 login 出现在 Core 链路" << std::endl;
            return 0;
        }
        return handle_login_req(msg_head);
    case G1_MSG_HEART_REQ:
        return handle_heart_req(msg_head);
    case G1_MSG_ORDER_REQ:
        if (link_type_ != LinkType::CORE) {
            std::cerr << "[Session][" << link_name << "] 错误: Core 消息 order 出现在 GW 链路" << std::endl;
            return 0;
        }
        return handle_order_req(msg_head);
    case G1_MSG_CANCEL_REQ:
        if (link_type_ != LinkType::CORE) {
            std::cerr << "[Session][" << link_name << "] 错误: Core 消息 cancel 出现在 GW 链路" << std::endl;
            return 0;
        }
        return handle_cancel_req(msg_head);
    default:
        std::cout << "[Session][" << link_name << "] 未知消息类型: " << head.msg_id << ", fd=" << fd_ << std::endl;
        return 0; // 跳过未知消息
    }
}

bool ClientSession::is_timeout(int timeout_sec) const {
    if (!logged_in_) return false; // 未登录不检查心跳超时
    return std::difftime(std::time(nullptr), last_heartbeat_) > timeout_sec;
}

void ClientSession::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

// ---- 消息处理 ----

int ClientSession::handle_sec_info_req(const g1_msg_head* head) {
    const sec_info_req* req = reinterpret_cast<const sec_info_req*>(head + 1);
    std::cout << "[Session] 证券信息请求: cust_req_no=" << req->cust_req_no << ", fd=" << fd_ << std::endl;

    if (!send_sec_info_ans(req->cust_req_no)) {
        return -1;
    }
    return 0;
}

int ClientSession::handle_login_req(const g1_msg_head* head) {
    const login_req* req = reinterpret_cast<const login_req*>(head + 1);
    std::string fund_account_id(req->fund_account_id, strnlen(req->fund_account_id, sizeof(req->fund_account_id)));

    // DEBUG: dump login_req 前 48 字节
    const unsigned char* raw = reinterpret_cast<const unsigned char*>(req);
    std::cerr << "[DEBUG] login_req raw[0:48]: ";
    for (int i = 0; i < 48; ++i) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%02x ", raw[i]);
        std::cerr << buf;
    }
    std::cerr << std::endl;
    std::cerr << "[DEBUG] login_req raw[0:48] ascii: ";
    for (int i = 0; i < 48; ++i) {
        char c = (raw[i] >= 32 && raw[i] < 127) ? static_cast<char>(raw[i]) : '.';
        std::cerr << c;
    }
    std::cerr << std::endl;

    std::cout << "[Session][GW] 登录请求: fund_account=" << fund_account_id
              << ", market_type=" << req->market_type
              << ", heart_bt_int=" << req->heart_bt_int
              << ", log_type=" << req->log_type
              << ", fd=" << fd_ << std::endl;

    // 验证账户
    if (!acct_mgr_->verify_account(fund_account_id, "")) {
        // 不校验密码（模拟柜台简化），只检查账户是否存在
        AccountConfig info;
        if (!acct_mgr_->get_account_info(fund_account_id, info)) {
            std::cerr << "[Session][GW] 登录失败: 账户不存在, fund_account=" << fund_account_id << std::endl;
            send_login_ans(req, 1001, "account not found");
            return 0;
        }
    }

    // 分配登录信息
    user_id_ = 1;  // 固定 user_id
    board_no_ = 1; // 固定 board_no
    session_id_ = acct_mgr_->generate_session_id();
    fund_account_id_ = fund_account_id;
    logged_in_ = true;
    last_heartbeat_ = std::time(nullptr);

    // 注册会话到全局注册表（Core 链路业务处理时查询）
    SessionInfo info;
    info.fund_account_id = fund_account_id_;
    info.branch_id = std::string(req->branch_id, strnlen(req->branch_id, sizeof(req->branch_id)));
    info.user_id = user_id_;
    info.board_no = board_no_;
    info.session_id = session_id_;
    info.logged_in = true;
    SessionRegistry::instance().register_session(fund_account_id_, info);

    std::cout << "[Session][GW] 登录成功: fund_account=" << fund_account_id
              << ", user_id=" << user_id_ << ", board_no=" << board_no_
              << ", session_id=" << session_id_ << std::endl;

    if (!send_login_ans(req, 0, nullptr)) {
        return -1;
    }
    return 0;
}

int ClientSession::handle_heart_req(const g1_msg_head* head) {
    (void)head;
    last_heartbeat_ = std::time(nullptr);

    if (!send_heart_ans()) {
        return -1;
    }
    return 0;
}

int ClientSession::handle_order_req(const g1_msg_head* head) {
    const order_req* req = reinterpret_cast<const order_req*>(head + 1);

    // Core 链路：从消息头提取 user_id/board_no/session_id
    user_id_ = head->user_id;
    board_no_ = head->board_no;
    session_id_ = head->session_id;

    std::cout << "[Session][CORE] 委托请求: cust_req_no=" << req->cust_req_no
              << ", sec_index=" << req->sec_index
              << ", side=" << (int)req->side
              << ", price=" << req->order_price
              << ", qty=" << req->order_qty
              << ", user_id=" << user_id_ << ", board_no=" << board_no_
              << ", fd=" << fd_ << std::endl;

    if (!send_order_rtn(req)) {
        return -1;
    }

    // 模拟成交：委托后主动推送一笔成交回报（trade_rtn）
    if (!send_trade_rtn(req)) {
        return -1;
    }
    return 0;
}

int ClientSession::handle_cancel_req(const g1_msg_head* head) {
    const cancel_req* req = reinterpret_cast<const cancel_req*>(head + 1);

    // Core 链路：从消息头提取 user_id/board_no/session_id
    user_id_ = head->user_id;
    board_no_ = head->board_no;
    session_id_ = head->session_id;

    std::cout << "[Session][CORE] 撤单请求: cust_req_no=" << req->cust_req_no
              << ", order_sys_no=" << req->order_sys_no
              << ", org_cust_req_no=" << req->org_cust_req_no
              << ", user_id=" << user_id_ << ", board_no=" << board_no_
              << ", fd=" << fd_ << std::endl;

    if (!send_cancel_rsp(req)) {
        return -1;
    }
    return 0;
}

// ---- 消息发送 ----

bool ClientSession::send_message(uint32_t msg_id, const void* body, uint32_t body_len,
                                  uint16_t board_no, uint16_t user_id, uint32_t session_id) {
    char buf[4096];
    int total_len = MessageParser::build_message(buf, sizeof(buf), msg_id, body, body_len,
                                                  board_no, user_id, session_id);
    if (total_len < 0) {
        std::cerr << "[Session] 构建消息失败, msg_id=" << msg_id << std::endl;
        return false;
    }

    ssize_t n = ::send(fd_, buf, total_len, 0);
    if (n <= 0) {
        std::cerr << "[Session] 发送消息失败, msg_id=" << msg_id << ", fd=" << fd_ << std::endl;
        return false;
    }

    std::cout << "[Session] 发送消息: " << MessageParser::msg_type_name(msg_id)
              << ", body_len=" << body_len << ", fd=" << fd_ << std::endl;
    return true;
}

bool ClientSession::send_heart_ans() {
    // 心跳应答：仅消息头，无消息体
    return send_message(G1_MSG_HEART_ANS, nullptr, 0, 0, 0, 0);
}

bool ClientSession::send_sec_info_ans(int64_t cust_req_no) {
    // 构建 sec_push_head + sec_push_info
    // 使用 stack 数组构建完整消息体
    char body_buf[sizeof(sec_push_head) + sizeof(sec_push_info)];
    std::memset(body_buf, 0, sizeof(body_buf));

    sec_push_head* push_head = reinterpret_cast<sec_push_head*>(body_buf);
    push_head->cust_req_no = cust_req_no;
    push_head->total_num = 1;
    push_head->cur_num = 1;
    push_head->err_code = 0;

    sec_push_info* info = reinterpret_cast<sec_push_info*>(push_head + 1);
    std::strncpy(info->security_id, "600007", sizeof(info->security_id));
    info->market_type = 1;  // 上海
    info->sec_index = 1;    // 证券索引
    info->buy_qty_unit = 100;
    info->sell_qty_unit = 100;
    info->reserved = 0;
    info->price_unit = 2502000000LL; // 价格变动单位，放大10000

    return send_message(G1_MSG_SEC_INFO_ANS, body_buf, sizeof(body_buf), 0, 0, 0);
}

bool ClientSession::send_login_ans(const login_req* req, uint32_t err_code, const char* err_msg) {
    login_ans ans;
    std::memset(&ans, 0, sizeof(ans));

    ans.cust_req_no = req->cust_req_no;
    std::memcpy(ans.cust_id, req->cust_id, sizeof(ans.cust_id));
    std::memcpy(ans.fund_account_id, req->fund_account_id, sizeof(ans.fund_account_id));
    std::memcpy(ans.branch_id, req->branch_id, sizeof(ans.branch_id));
    std::memcpy(ans.holder_acc, req->holder_acc, sizeof(ans.holder_acc));
    std::memcpy(ans.session, req->session, sizeof(ans.session));
    std::memcpy(ans.end_code, req->end_code, sizeof(ans.end_code));

    if (err_code == 0) {
        ans.user_id = user_id_;
        ans.board_no = board_no_;
        ans.proto_type = 1; // TCP
        std::memcpy(ans.order_way, req->order_way, sizeof(ans.order_way));
        ans.req_connect_id = 0;
        ans.log_type = req->log_type;
        ans.session_id = session_id_;
        ans.trade_port = core_port_;
        std::strncpy(ans.trade_ip, "127.0.0.1", sizeof(ans.trade_ip));
        ans.err_code = 0;
        ans.reserved = 0;
        std::memset(ans.err_msg, 0, sizeof(ans.err_msg));
        std::strncpy(ans.version, req->version, sizeof(ans.version));
        ans.login_time = static_cast<int64_t>(std::time(nullptr));
    } else {
        ans.user_id = 0;
        ans.board_no = 0;
        ans.proto_type = 1;
        std::memset(ans.order_way, 0, sizeof(ans.order_way));
        ans.req_connect_id = 0;
        ans.log_type = req->log_type;
        ans.session_id = 0;
        ans.trade_port = 0;
        std::memset(ans.trade_ip, 0, sizeof(ans.trade_ip));
        ans.err_code = static_cast<int32_t>(err_code);
        ans.reserved = 0;
        if (err_msg != nullptr) {
            std::strncpy(ans.err_msg, err_msg, sizeof(ans.err_msg) - 1);
        } else {
            std::memset(ans.err_msg, 0, sizeof(ans.err_msg));
        }
        std::memset(ans.version, 0, sizeof(ans.version));
        ans.login_time = static_cast<int64_t>(std::time(nullptr));
    }

    return send_message(G1_MSG_LOGIN_ANS, &ans, sizeof(ans), ans.board_no, ans.user_id, ans.session_id);
}

bool ClientSession::send_order_rtn(const order_req* req) {
    order_rtn rtn;
    std::memset(&rtn, 0, sizeof(rtn));

    rtn.user_id = req->user_id;
    rtn.board_no = req->board_no;
    rtn.sec_index = req->sec_index;
    rtn.side = req->side;
    rtn.order_type = req->order_type;
    rtn.order_price = req->order_price;
    rtn.order_qty = req->order_qty;
    rtn.cust_req_no = req->cust_req_no;

    rtn.order_status = 0; // 已报
    rtn.rtn_type = 0;     // 委托应答
    rtn.policy_id = req->policy_id;
    rtn.err_code = 0;

    rtn.order_sys_no = ++order_counter_;
    rtn.frozen_amount = req->order_price * req->order_qty / 10000;
    rtn.trade_amount = 0;
    rtn.trade_qty = 0;
    rtn.fee = 0;
    rtn.cancel_qty = 0;
    rtn.order_time = static_cast<int64_t>(std::time(nullptr)) * 1000000; // 模拟时间
    rtn.update_time = rtn.order_time;
    rtn.session_seq_no = 0;

    return send_message(G1_MSG_ORDER_RTN, &rtn, sizeof(rtn), rtn.board_no, rtn.user_id, session_id_);
}

bool ClientSession::send_trade_rtn(const order_req* req) {
    trade_rtn rtn;
    std::memset(&rtn, 0, sizeof(rtn));

    // 委托信息（与 order_rtn 保持一致）
    rtn.user_id = req->user_id;
    rtn.board_no = req->board_no;
    rtn.sec_index = req->sec_index;
    rtn.side = req->side;
    rtn.order_type = req->order_type;
    rtn.order_price = req->order_price;
    rtn.order_qty = req->order_qty;
    rtn.cust_req_no = req->cust_req_no;

    rtn.order_status = 3; // 已成交
    rtn.rtn_type = 1;     // 成交回报
    rtn.policy_id = req->policy_id;
    rtn.err_code = 0;

    rtn.order_sys_no = order_counter_; // 与上笔委托回报一致
    rtn.frozen_amount = req->order_price * req->order_qty / 10000;
    rtn.trade_amount = req->order_price * req->order_qty / 10000;
    rtn.trade_qty = req->order_qty;
    rtn.cancel_qty = 0;
    rtn.fee = 0;
    rtn.order_time = static_cast<int64_t>(std::time(nullptr)) * 1000000;
    rtn.exec_time = rtn.order_time;

    // 成交信息
    std::strncpy(rtn.exec_id, "T0000000001", sizeof(rtn.exec_id));
    rtn.exec_price = req->order_price;
    rtn.exec_qty = req->order_qty;
    rtn.exec_amount = req->order_price * req->order_qty / 10000;
    rtn.exec_fee = 0;
    rtn.session_seq_no = 0;

    return send_message(G1_MSG_TRADE_RTN, &rtn, sizeof(rtn), rtn.board_no, rtn.user_id, session_id_);
}

bool ClientSession::send_cancel_rsp(const cancel_req* req) {
    cancel_rsp rsp;
    std::memset(&rsp, 0, sizeof(rsp));

    rsp.cust_req_no = req->cust_req_no;
    rsp.order_sys_no = req->order_sys_no;
    rsp.user_id = req->user_id;
    rsp.board_no = req->board_no;
    rsp.err_code = 0; // 撤单成功
    rsp.org_cust_req_no = req->org_cust_req_no;
    rsp.session_seq_no = 0;

    return send_message(G1_MSG_CANCEL_RSP, &rsp, sizeof(rsp), rsp.board_no, rsp.user_id, session_id_);
}

} // namespace mock_gone

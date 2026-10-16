#include "client_session.h"

#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <sys/socket.h>

// ============================================================
// ClientSession 实现
// 使用 Archforce AGW 协议处理所有消息
// ============================================================

namespace mock_98 {

ClientSession::ClientSession(int fd, AccountManager* acct_mgr)
    : fd_(fd)
    , acct_mgr_(acct_mgr)
    , last_heartbeat_(time(nullptr))
    , disconnected_(false)
    , closed_(false)
    , agw_logged_in_(false)
{
}

ClientSession::~ClientSession() {
    close();
}

void ClientSession::close() {
    if (closed_) return;
    closed_ = true;
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool ClientSession::is_timeout(int timeout_sec) const {
    return (time(nullptr) - last_heartbeat_) > timeout_sec;
}

// ============================================================
// 数据收发
// ============================================================

int ClientSession::feed_data(const char* data, size_t len) {
    last_heartbeat_ = time(nullptr);

    // 追加到接收缓冲区
    if (recv_buf_.size() + len > MAX_RECV_BUF) {
        fprintf(stderr, "recv buffer overflow, closing\n");
        return -2;
    }
    recv_buf_.insert(recv_buf_.end(), data, data + len);

    // 处理完整消息
    size_t offset = 0;
    while (offset + AGW_HEADER_SIZE <= recv_buf_.size()) {
        raw_agw_header hdr;
        std::memcpy(&hdr, recv_buf_.data() + offset, sizeof(hdr));
        int32_t msg_total = sizeof(hdr) + static_cast<int32_t>(hdr.body_length) + static_cast<int32_t>(sizeof(uint32_t));
        if (offset + static_cast<size_t>(msg_total) > recv_buf_.size()) {
            break; // 数据不完整, 等下次
        }
        const char *body = recv_buf_.data() + offset + AGW_HEADER_SIZE;
        int32_t body_len = static_cast<int32_t>(hdr.body_length);
        handle_message(hdr, body, body_len);
        offset += msg_total;
    }

    // 移除已处理数据
    if (offset > 0) {
        recv_buf_.erase(recv_buf_.begin(), recv_buf_.begin() + offset);
    }

    return 0;
}

void ClientSession::send_data(const char *data, int32_t len) {
    if (closed_) return;
    ssize_t n = ::send(fd_, data, len, MSG_NOSIGNAL);
    if (n != len) {
        fprintf(stderr, "send failed: sent=%zd, expected=%d\n", n, len);
    }
}

void ClientSession::send_agw_msg(uint32_t msg_type, uint16_t msg_ver,
                                  const char *body, uint32_t body_len) {
    char buf[4096];
    int32_t total = write_agw_msg(buf, msg_type, msg_ver, body, body_len);
    if (total > 0) {
        send_data(buf, total);
    }
}

void ClientSession::send_agw_msg(uint32_t msg_type, uint16_t msg_ver,
                                  const lb_counter98::SerializeBuffer &buf) {
    send_agw_msg(msg_type, msg_ver,
                 reinterpret_cast<const char *>(buf.Begin()),
                 static_cast<uint32_t>(buf.Size()));
}

// ============================================================
// 消息分发
// ============================================================

void ClientSession::handle_message(const raw_agw_header &hdr,
                                    const char *body, int32_t body_len) {
    printf("recv msg: %s (type=%u, ver=%u, body_len=%d)\n",
           agw_msg_type_name(hdr.msg_type), hdr.msg_type, hdr.msg_ver, body_len);

    switch (hdr.msg_type) {
    // ---- 登录/心跳 ----
    case lb_counter98::kAgwMsgLogin:                  handle_agw_login(body, body_len); break;
    case lb_counter98::kAgwMsgCustLoginReq:           handle_cust_login(body, body_len); break;
    case lb_counter98::kAgwMsgHeartBeat:              handle_heartbeat(body, body_len); break;
    // ---- 委托/撤单 ----
    case lb_counter98::kAgwMsgCashAuctionOrder:       handle_order(body, body_len); break;
    case lb_counter98::kAgwMsgCancelOrder:            handle_cancel(body, body_len); break;
    // ---- 查询 ----
    case lb_counter98::kAgwMsgExtQueryOrder:          handle_query_order(body, body_len); break;
    case lb_counter98::kAgwMsgExtQueryTradeOrder:     handle_query_trade(body, body_len); break;
    case lb_counter98::kAgwMsgExtQueryFund:           handle_query_fund(body, body_len); break;
    case lb_counter98::kAgwMsgExtQuerySelfShare:      handle_query_position(body, body_len); break;
    case lb_counter98::kAgwMsgExtQueryMaxOrderQty:    handle_query_max_order_qty(body, body_len); break;
    case lb_counter98::kAgwMsgExternalQueryTotalFundAsset: handle_query_total_fund(body, body_len); break;
    case lb_counter98::kAgwMsgExtQueryFundMaxdraw:    handle_query_fund_maxdraw(body, body_len); break;
    case lb_counter98::kAgwMsgExtQueryCustMarginAvl:  handle_query_cust_margin_avl(body, body_len); break;
    case lb_counter98::kAgwMsgExtQueryAssetDebts:     handle_query_asset_debts(body, body_len); break;
    case lb_counter98::kAgwMsgExtQueryCreditMaxOrderQty: handle_query_credit_max_order_qty(body, body_len); break;
    default:
        printf("unknown msg_type=%u, ignored\n", hdr.msg_type);
        break;
    }
}

// ============================================================
// 登录/心跳
// ============================================================

void ClientSession::handle_agw_login(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgLogin req;
    if (!decode_body(req, body, body_len)) {
        fprintf(stderr, "AgwUserLogin decode failed\n");
        return;
    }

    std::string agw_user(req.sender_comp_id.data(), strnlen(req.sender_comp_id.data(), req.sender_comp_id.size()));
    std::string agw_pwd(req.password.data(), strnlen(req.password.data(), req.password.size()));

    bool ok = acct_mgr_->verify_agw_user(agw_user, agw_pwd);

    lb_counter98::AgwMsgLogin ans;
    ans.heart_bt_int = req.heart_bt_int;
    ans.spec_cust_flag = req.spec_cust_flag;
    if (ok) {
        agw_logged_in_ = true;
        agw_session_ = "mock_session_" + std::to_string(fd_);
        // sender_comp_id 在应答中携带 session (counter98.cpp deal_agwuser_login_ans 读取此字段)
        std::memset(ans.sender_comp_id.data(), ' ', ans.sender_comp_id.size());
        std::memcpy(ans.sender_comp_id.data(), agw_session_.c_str(),
                    std::min(agw_session_.size(), ans.sender_comp_id.size()));
        printf("AGW login success: user=%s\n", agw_user.c_str());
    } else {
        fprintf(stderr, "AGW login failed: user=%s\n", agw_user.c_str());
    }

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgLogin, 0, buf);
    }
}

void ClientSession::handle_cust_login(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgCustLoginReq req;
    if (!decode_body(req, body, body_len)) {
        fprintf(stderr, "CustLogin decode failed\n");
        return;
    }

    std::string fund_account(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()));
    std::string password(req.password);

    bool ok = acct_mgr_->verify_account(fund_account, password);

    lb_counter98::AgwMsgCustLoginResp ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.pwd_ciphertext = password;
    if (ok) {
        // 填充 fund_account_array (counter98.cpp deal_cust_login_ans 读取此字段)
        atp::gw::AgwMsgCustLoginRespFundAccountUnit fa_unit;
        fa_unit.fund_account_id = req.fund_account_id;
        fa_unit.branch_id = req.branch_id;
        ans.fund_account_array.push_back(fa_unit);
        ans.key_error_code = "";
        ans.key_error_msg = "";
        logged_accounts_.insert(fund_account);
        printf("CustLogin success: fund_account=%s\n", fund_account.c_str());
    } else {
        ans.key_error_code = "1";
        ans.key_error_msg = "invalid fund account or password";
        fprintf(stderr, "CustLogin failed: fund_account=%s\n", fund_account.c_str());
    }

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgCustLoginResp, 2, buf);
    }
}

void ClientSession::handle_heartbeat(const char *body, int32_t body_len) {
    (void)body;
    (void)body_len;
    // 心跳: 直接回送空心跳
    send_agw_msg(lb_counter98::kAgwMsgHeartBeat, 2, nullptr, 0);
}

// ============================================================
// 委托/撤单
// ============================================================

void ClientSession::handle_order(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgCashAuctionOrder req;
    if (!decode_body(req, body, body_len)) {
        fprintf(stderr, "CashAuctionOrder decode failed\n");
        return;
    }

    std::string fund_account(req.fund_account_id.data(), strnlen(req.fund_account_id.data(), req.fund_account_id.size()));
    printf("Order: fund_account=%s, security=%s, side=%c, qty=%lld, price=%lld, client_seq_id=%lld\n",
           fund_account.c_str(),
           std::string(req.security_id.data(), strnlen(req.security_id.data(), req.security_id.size())).c_str(),
           req.side, (long long)req.order_qty, (long long)req.price, (long long)req.client_seq_id);

    // 构造委托确认回报
    lb_counter98::AgwMsgCashAuctionTradeER er;
    er.client_seq_id = req.client_seq_id;
    er.cust_id = req.cust_id;
    er.fund_account_id = req.fund_account_id;
    er.branch_id = req.branch_id;
    er.account_id = req.account_id;
    er.security_id = req.security_id;
    er.side = req.side;
    er.price = req.price;
    er.order_qty = req.order_qty;
    er.market_id = req.market_id;
    {
        std::string id_str = std::to_string(req.client_seq_id);
        std::memset(er.order_id.data(), 0, er.order_id.size());
        std::memcpy(er.order_id.data(), id_str.c_str(), std::min(id_str.size(), er.order_id.size()));
    } // 模拟柜台编号
    er.order_status = '2';               // 2=已报
    er.key_error_code = "";
    er.key_error_msg = "";

    lb_counter98::SerializeBuffer buf;
    if (encode_body(er, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgCashAuctionTradeER, 2, buf);
    }
}

void ClientSession::handle_cancel(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgCancelOrder req;
    if (!decode_body(req, body, body_len)) {
        fprintf(stderr, "CancelOrder decode failed\n");
        return;
    }

    printf("Cancel: client_seq_id=%lld, orig_cli_ord_no=%lld\n",
           (long long)req.client_seq_id, (long long)req.orig_cli_ord_no);

    // 构造撤单确认回报 (撤单应答使用 OrderStatusAck, msg_type 同撤单请求)
    lb_counter98::AgwMsgOrderStatusAck er;
    er.client_seq_id = req.client_seq_id;
    er.cust_id = req.cust_id;
    er.fund_account_id = req.fund_account_id;
    er.branch_id = req.branch_id;
    er.account_id = req.account_id;
    er.orig_cli_ord_no = req.orig_cli_ord_no;
    {
        std::string id_str = std::to_string(req.orig_cli_ord_no);
        std::memset(er.order_id.data(), 0, er.order_id.size());
        std::memcpy(er.order_id.data(), id_str.c_str(), std::min(id_str.size(), er.order_id.size()));
    }
    er.order_status = '2'; // 2=已报
    er.key_error_code = "";
    er.key_error_msg = "";

    lb_counter98::SerializeBuffer buf;
    if (encode_body(er, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgCancelOrder, 2, buf);
    }
}

// ============================================================
// 查询处理 (返回模拟数据)
// ============================================================

void ClientSession::handle_query_order(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryOrder req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryOrder decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultOrder ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    ans.total_num = 1;


    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultOrder, 2, buf);
    }
    printf("QueryOrder: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_trade(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryTradeOrder req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryTrade decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultTradeOrder ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    ans.total_num = 0;

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultTradeOrder, 2, buf);
    }
    printf("QueryTrade: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_fund(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryFund req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryFund decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultFund ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    // 模拟资金数据
    ans.leaves_value = 100000000;    // 资金余额: 1亿
    ans.available_tall = 80000000;   // 可用资金: 8千万
    ans.frozen_all = 20000000;       // 冻结资金: 2千万

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultFund, 2, buf);
    }
    printf("QueryFund: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_position(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQuerySelfShare req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryPosition decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultSelfShare ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    ans.total_num = 0;

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultSelfShare, 2, buf);
    }
    printf("QueryPosition: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_max_order_qty(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryMaxOrderQty req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryMaxOrderQty decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryMaxOrderQtyResult ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    ans.max_order_qty = 100000; // 模拟最大可委托数

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryMaxOrderQtyResult, 2, buf);
    }
    printf("QueryMaxOrderQty: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_total_fund(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExternalQueryTotalFundAsset req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryTotalFund decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultTotalFundAsset ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultTotalFundAsset, 2, buf);
    }
    printf("QueryTotalFund: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_fund_maxdraw(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryFundMaxdraw req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryFundMaxdraw decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultFundMaxdraw ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultFundMaxdraw, 2, buf);
    }
    printf("QueryFundMaxdraw: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_cust_margin_avl(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryCustMarginAvl req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryCustMarginAvl decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultCustMarginAvl ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    // 模拟可用保证金数据
    ans.margin_avl = 50000000;      // 保证金可用余额
    ans.credit_bal = 100000000;     // 授信额度
    ans.credit_avl = 80000000;      // 可用额度
    ans.margin_rate = 50;           // 保证金比例
    ans.fund_avl = 30000000;        // 资金可用数

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultCustMarginAvl, 2, buf);
    }
    printf("QueryCustMarginAvl: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_asset_debts(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryAssetDebts req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryAssetDebts decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryResultAssetDebts ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.currency = req.currency;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    // 模拟资产负债数据
    ans.asset = 300000000;          // 总资产
    ans.liability = 100000000;      // 总负债
    ans.fund_bal = 100000000;       // 资金余额
    ans.fund_avl = 80000000;        // 资金可用数
    ans.keep_rate = 300;            // 维持担保比例 300%
    ans.credit_status = '0';        // 信用状态正常

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryResultAssetDebts, 2, buf);
    }
    printf("QueryAssetDebts: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

void ClientSession::handle_query_credit_max_order_qty(const char *body, int32_t body_len) {
    lb_counter98::AgwMsgExtQueryCreditMaxOrderQty req;
    if (!decode_body(req, body, body_len)) { fprintf(stderr, "ExtQueryCreditMaxOrderQty decode failed\n"); return; }

    lb_counter98::AgwMsgExtQueryCreditMaxOrderQtyResult ans;
    ans.client_seq_id = req.client_seq_id;
    ans.cust_id = req.cust_id;
    ans.fund_account_id = req.fund_account_id;
    ans.branch_id = req.branch_id;
    ans.key_error_code = "";
    ans.key_error_msg = "";
    // 模拟两融最大可委托数
    ans.max_order_qty = 50000;        // 最大可委托数
    ans.theoretical_order_qty = 60000; // 理论可委托数
    ans.fund_credit_avl = 30000000;   // 买券还券可用资金
    ans.stk_debts_avl = 20000000;     // 融券负债可用额度

    lb_counter98::SerializeBuffer buf;
    if (encode_body(ans, buf)) {
        send_agw_msg(lb_counter98::kAgwMsgExtQueryCreditMaxOrderQtyResult, 2, buf);
    }
    printf("QueryCreditMaxOrderQty: client_seq_id=%lld\n", (long long)req.client_seq_id);
}

} // namespace mock_98

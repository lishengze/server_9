#include "callback_handler.h"
#include <iostream>

namespace mock {

CallbackHandler::CallbackHandler()
    : response_received_(false)
    , trade_rtn_received_(false)
    , cancel_rsp_received_(false)
    , last_link_status_(0)
    , last_link_type_(0)
    , last_counter_type_(0)
    , last_error_code_(0)
{
}

void CallbackHandler::on_login(const lb_api::LoginAns& ans) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_login_ans_ = ans;
    response_received_ = true;
    cv_.notify_one();
    std::cout << "[Callback] on_login: err_code=" << ans.err_code
              << ", market_type=" << ans.market_type << std::endl;
}

void CallbackHandler::on_order_rtn(const lb_api::StreamInfo& si, const lb_api::OrderRtn& rtn) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_order_rtn_ = rtn;
    response_received_ = true;
    cv_.notify_one();
    std::cout << "[Callback] on_order_rtn: order_status=" << (int)rtn.order_status
              << ", rtn_type=" << rtn.rtn_type
              << ", order_qty=" << rtn.order_qty
              << ", trade_qty=" << rtn.trade_qty << std::endl;
}

void CallbackHandler::on_trade_rtn(const lb_api::StreamInfo& si, const lb_api::TradeRtn& rtn) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_trade_rtn_ = rtn;
    response_received_ = true;
    trade_rtn_received_ = true;
    cv_.notify_one();
    std::cout << "[Callback] on_trade_rtn: exec_price=" << rtn.exec_price
              << ", exec_qty=" << rtn.exec_qty
              << ", exec_id=" << rtn.exec_id.data() << std::endl;
}

void CallbackHandler::on_cancel_rsp(const lb_api::StreamInfo& si, const lb_api::CancelRsp& rsp) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_cancel_rsp_ = rsp;
    response_received_ = true;
    cancel_rsp_received_ = true;
    cv_.notify_one();
    std::cout << "[Callback] on_cancel_rsp: err_code=" << rsp.err_code
              << ", order_sys_no=" << rsp.order_sys_no << std::endl;
}

void CallbackHandler::on_link_status(int32_t counter_type, int32_t link_type, int32_t status) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_counter_type_ = counter_type;
    last_link_type_ = link_type;
    last_link_status_ = status;
    std::cout << "[Callback] on_link_status: counter_type=" << counter_type
              << ", link_type=" << link_type
              << ", status=" << status << std::endl;
}

void CallbackHandler::on_error(lb_api::err_event_type event_type, int32_t err_code, const char* err_desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_code_ = err_code;
    last_error_desc_ = err_desc ? err_desc : "";
    std::cerr << "[Callback] on_error: event_type=" << (int)event_type
              << ", err_code=" << err_code
              << ", err_desc=" << (err_desc ? err_desc : "") << std::endl;
}

bool CallbackHandler::wait_for_response(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this]{ return response_received_; });
}

void CallbackHandler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    response_received_ = false;
    cancel_rsp_received_ = false;
    trade_rtn_received_ = false;
}

} // namespace mock

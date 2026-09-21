// callback_handler.cpp - CallbackHandler 回调处理器实现
//
// 职责：继承 api_callback 虚接口，接收 liblbapi.so 发出的各类回报，
//   并将回报数据记录到成员变量 + 通过条件变量通知等待中的测试线程。
//
// 线程模型：
//   - 回报回调由 API 内部线程（engine / link）调用
//   - 测试执行线程通过 wait_for_response() / has_xxx() 等待并查询
//   - 所有共享状态统一用 mutex_ 保护，条件变量 cv_ 用于唤醒等待者
//
// 关键设计：reset() 在每次发送新请求前调用，清空所有 received_ 标志位，
//   确保「上一个请求的回报」不会干扰「当前请求的校验」。

#include "callback_handler.h"
#include "logger.h"
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

// on_login: 登录应答回调
// 记录 LoginAns，置位 response_received_ 并唤醒等待线程。
void CallbackHandler::on_login(const lb_api::LoginAns& ans) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_login_ans_ = ans;
    response_received_ = true;
    cv_.notify_one();
    LOG_INFO("[Callback] on_login: err_code=" << ans.err_code
              << ", market_type=" << ans.market_type);
}

// on_order_rtn: 委托回报回调
// 记录 OrderRtn，置位 response_received_。注意：委托回报(2003)可能是
// 委托状态变更(New/Reject)或撤单中间回报，不单独置 trade/cancel 标志。
void CallbackHandler::on_order_rtn(const lb_api::StreamInfo& si, const lb_api::OrderRtn& rtn) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_order_rtn_ = rtn;
    response_received_ = true;
    cv_.notify_one();
    LOG_INFO("[Callback] on_order_rtn: order_status=" << (int)rtn.order_status
              << ", rtn_type=" << rtn.rtn_type
              << ", order_qty=" << rtn.order_qty
              << ", trade_qty=" << rtn.trade_qty);
}

// on_trade_rtn: 成交回报回调
// 记录 TradeRtn，置位 response_received_ 和 trade_rtn_received_。
// trade_rtn_received_ 供成交回报(2005)测试用例的专用等待使用。
void CallbackHandler::on_trade_rtn(const lb_api::StreamInfo& si, const lb_api::TradeRtn& rtn) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_trade_rtn_ = rtn;
    response_received_ = true;
    trade_rtn_received_ = true;
    cv_.notify_one();
    LOG_INFO("[Callback] on_trade_rtn: exec_price=" << rtn.exec_price
              << ", exec_qty=" << rtn.exec_qty
              << ", exec_id=" << rtn.exec_id.data());
}

// on_cancel_rsp: 撤单应答回调
// 记录 CancelRsp，置位 response_received_ 和 cancel_rsp_received_。
// cancel_rsp_received_ 供撤单测试用例专用等待，避免被中间委托回报(2003)干扰。
void CallbackHandler::on_cancel_rsp(const lb_api::StreamInfo& si, const lb_api::CancelRsp& rsp) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_cancel_rsp_ = rsp;
    response_received_ = true;
    cancel_rsp_received_ = true;
    cv_.notify_one();
    LOG_INFO("[Callback] on_cancel_rsp: err_code=" << rsp.err_code
              << ", order_sys_no=" << rsp.order_sys_no);
}

// on_link_status: 链接状态回调
// 记录柜台类型、链接类型和状态值（0=断开, 1=连接）。
void CallbackHandler::on_link_status(int32_t counter_type, int32_t link_type, int32_t status) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_counter_type_ = counter_type;
    last_link_type_ = link_type;
    last_link_status_ = status;
    LOG_INFO("[Callback] on_link_status: counter_type=" << counter_type
              << ", link_type=" << link_type
              << ", status=" << status);
}

// on_error: 错误事件回调
// 记录错误码和错误描述，仅打印，不置位任何 received_ 标志。
void CallbackHandler::on_error(lb_api::err_event_type event_type, int32_t err_code, const char* err_desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_code_ = err_code;
    last_error_desc_ = err_desc ? err_desc : "";
    LOG_ERROR("[Callback] on_error: event_type=" << (int)event_type
              << ", err_code=" << err_code
              << ", err_desc=" << (err_desc ? err_desc : ""));
}

// wait_for_response: 等待任意回报到达
// 使用条件变量 cv_ 阻塞等待，直到 response_received_ 为 true 或超时。
// 返回 true 表示已收到回报，false 表示超时。
bool CallbackHandler::wait_for_response(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this]{ return response_received_; });
}

// reset: 重置接收状态（每次发送新请求前调用）
// 清空 response_received_ / trade_rtn_received_ / cancel_rsp_received_，
// 确保新请求的回报不会被上一次请求的遗留标志干扰。
void CallbackHandler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    response_received_ = false;
    cancel_rsp_received_ = false;
    trade_rtn_received_ = false;
}

} // namespace mock

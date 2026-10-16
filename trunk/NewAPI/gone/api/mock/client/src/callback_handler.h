#ifndef MOCK_CLIENT_CALLBACK_HANDLER_H
#define MOCK_CLIENT_CALLBACK_HANDLER_H

#include "api_interface.h"
#include "api_callback.h"
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>

namespace mock {

/// 回调处理器：接收 API 回报，支持同步等待
class CallbackHandler : public lb_api::api_callback {
public:
    CallbackHandler();

    // ========== api_callback 虚方法实现 ==========
    void on_login(const lb_api::LoginAns& ans) override;
    void on_order_rtn(const lb_api::StreamInfo& si, const lb_api::OrderRtn& rtn) override;
    void on_trade_rtn(const lb_api::StreamInfo& si, const lb_api::TradeRtn& rtn) override;
    void on_cancel_rsp(const lb_api::StreamInfo& si, const lb_api::CancelRsp& rsp) override;
    void on_link_status(int32_t counter_type, int32_t link_type, int32_t status) override;
    void on_error(lb_api::err_event_type event_type, int32_t err_code, const char* err_desc) override;

    // ========== 查询回调实现 ==========
    void on_order_query_ans(const lb_api::OrderRtn* ans_arr, const lb_api::QueryAnsCtl& ctl) override;
    void on_trade_query_ans(const lb_api::TradeInfo* ans_arr, const lb_api::QueryAnsCtl& ctl) override;
    void on_fund_query_ans(const lb_api::CustFundInfo& info) override;
    void on_position_query_ans(const lb_api::CustPositionInfo* ans_arr, const lb_api::QueryAnsCtl& ctl) override;
    void on_max_order_qty_query_ans(const lb_api::CustMaxOrderQtyInfo& info) override;
    void on_total_fund_asset_query_ans(const lb_api::CustTotalFundAssetInfo* ans_arr, const lb_api::QueryAnsCtl& ctl) override;
    void on_fund_maxdraw_query_ans(const lb_api::CustFundMaxdrawInfo* ans_arr, const lb_api::QueryAnsCtl& ctl) override;
    void on_cust_margin_avl_query_ans(const lb_api::CustMarginAvlInfo& info) override;
    void on_asset_debts_query_ans(const lb_api::AssetDebtsInfo& info) override;
    void on_credit_max_order_qty_query_ans(const lb_api::CreditMaxOrderQtyInfo& info) override;

    // ========== 等待方法 ==========
    /// 等待回报到达，超时返回 false
    bool wait_for_response(int timeout_ms);

    /// 重置状态（准备接收下一个回报）
    void reset();

    // ========== 查询方法 ==========
    bool has_response() const { return response_received_; }
    bool has_trade_rtn() const { return trade_rtn_received_; }
    bool has_cancel_rsp() const { return cancel_rsp_received_; }
    const lb_api::LoginAns& last_login_ans() const { return last_login_ans_; }
    const lb_api::OrderRtn& last_order_rtn() const { return last_order_rtn_; }
    const lb_api::TradeRtn& last_trade_rtn() const { return last_trade_rtn_; }
    const lb_api::CancelRsp& last_cancel_rsp() const { return last_cancel_rsp_; }
    int32_t last_link_status() const { return last_link_status_; }
    int32_t last_link_type() const { return last_link_type_; }
    int32_t last_counter_type() const { return last_counter_type_; }
    /// 业务链接（LINK_TYPE_SPEED_TRADE=1，委托/撤单走此链接）是否已就绪
    bool trade_link_ready() const { return last_trade_link_status_ != 0; }
    std::string last_error_desc() const { return last_error_desc_; }

    // ========== 查询结果 getter ==========
    const std::vector<lb_api::OrderRtn>& order_query_results() const { return order_query_results_; }
    const std::vector<lb_api::TradeInfo>& trade_query_results() const { return trade_query_results_; }
    const lb_api::CustFundInfo& fund_query_result() const { return fund_query_result_; }
    const std::vector<lb_api::CustPositionInfo>& position_query_results() const { return position_query_results_; }
    const lb_api::CustMaxOrderQtyInfo& max_order_qty_result() const { return max_order_qty_result_; }
    const std::vector<lb_api::CustTotalFundAssetInfo>& total_fund_asset_results() const { return total_fund_asset_results_; }
    const std::vector<lb_api::CustFundMaxdrawInfo>& fund_maxdraw_results() const { return fund_maxdraw_results_; }
    const lb_api::CustMarginAvlInfo& cust_margin_avl_result() const { return cust_margin_avl_result_; }
    const lb_api::AssetDebtsInfo& asset_debts_result() const { return asset_debts_result_; }
    const lb_api::CreditMaxOrderQtyInfo& credit_max_order_qty_result() const { return credit_max_order_qty_result_; }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool response_received_;
    bool trade_rtn_received_;
    bool cancel_rsp_received_;

    // 回报数据
    lb_api::LoginAns last_login_ans_;
    lb_api::OrderRtn last_order_rtn_;
    lb_api::TradeRtn last_trade_rtn_;
    lb_api::CancelRsp last_cancel_rsp_;
    int32_t last_link_status_;
    int32_t last_link_type_;
    int32_t last_counter_type_;
    int32_t last_trade_link_status_;   ///< 业务链接(SPEED_TRADE)状态 0=断开 1=连接
    int32_t last_error_code_;
    std::string last_error_desc_;

    // 查询结果存储
    std::vector<lb_api::OrderRtn> order_query_results_;
    std::vector<lb_api::TradeInfo> trade_query_results_;
    lb_api::CustFundInfo fund_query_result_;
    std::vector<lb_api::CustPositionInfo> position_query_results_;
    lb_api::CustMaxOrderQtyInfo max_order_qty_result_;
    std::vector<lb_api::CustTotalFundAssetInfo> total_fund_asset_results_;
    std::vector<lb_api::CustFundMaxdrawInfo> fund_maxdraw_results_;
    lb_api::CustMarginAvlInfo cust_margin_avl_result_;
    lb_api::AssetDebtsInfo asset_debts_result_;
    lb_api::CreditMaxOrderQtyInfo credit_max_order_qty_result_;
};

} // namespace mock

#endif // MOCK_CLIENT_CALLBACK_HANDLER_H

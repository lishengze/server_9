#pragma once

#include "api_callback.h"
#include <vector>

/// 离线测试的用户回调记录器。
/// 现实对象：无
class order_test_callback final : public lb_api::api_callback
{
public:
  void on_order_rtn(const lb_api::StreamInfo &stream, const lb_api::OrderRtn &order) override;
  void on_trade_rtn(const lb_api::StreamInfo &stream, const lb_api::TradeRtn &trade) override;
  void on_order_query_ans(const lb_api::OrderRtn *orders, const lb_api::QueryAnsCtl &ctl) override;
  void on_trade_query_ans(const lb_api::TradeInfo *trades, const lb_api::QueryAnsCtl &ctl) override;
  void on_event(lb_api::api_event_type type, int32_t code, const char *message) override;

  std::vector<lb_api::OrderRtn> orders;
  std::vector<lb_api::TradeRtn> trades;
  std::vector<lb_api::OrderRtn> queried_orders;
  std::vector<lb_api::TradeInfo> queried_trades;
  std::vector<int32_t> errors;
};

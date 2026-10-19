#include "callback_manager.h"
#include "api_errno.h"
#include "order_trade_type.h"
#include <cstdint>
#include <cstring>

namespace lb_api {

callback_manager::callback_manager()
    : mode_(callback_mode::direct), single_writer_(false), user_callback_(nullptr), log_(nullptr) {}

callback_manager::~callback_manager() { cb_queue_.close(); }

int32_t callback_manager::init(lb_common::lb_log *log, api_callback *cb, callback_mode mode, int32_t cpu_affinity,
                               int32_t queue_size_mb, int32_t wait_ms, int32_t single_writer) {
  user_callback_ = cb;
  mode_ = mode;
  single_writer_ = single_writer;
  log_ = log;

  lb_common::lb_log_hand tlh(log_);
  int64_t queue_bytes = (queue_size_mb > 0 ? queue_size_mb : 8);
  queue_bytes = queue_bytes * 1024 * 1024;
  int32_t have_que = 0;
  if (mode_ == callback_mode::queued) {
    have_que = 1;
    int64_t buf_size = lb_common::que_mth_buf::need_buf_size(queue_bytes);
    int32_t ret = cb_queue_.init(buf_size);
    if (ret != 0) {
      error_log(tlh) << "init callback queue error,ret=" << ret << end_log;
      return LBAPI_ERR_QUEUE_INIT;
    }
    // wait_ms=0 → 死轮询(init_th busyround=0); wait_ms>0 → epoll触发模式
    int32_t busyround = (wait_ms == 0) ? 0 : 10000;
    ret = init_th(busyround, cpu_affinity, wait_ms);
    if (ret != 0) {
      error_log(tlh) << "init callback thread error,ret=" << ret << end_log;
      return LBAPI_ERR_THREAD_INIT;
    }
  }

  info_log(tlh) << "init callback manager ok,have_que=" << have_que << ",single_writer=" << single_writer_
                << ",queue_size=" << queue_bytes << ",wait_ms=" << wait_ms << end_log;
  return 0;
}

int32_t callback_manager::start() {
  lb_common::lb_log_hand tlh(log_);

  if (mode_ == callback_mode::queued) {
    int32_t ret = run();
    if (ret != 0) {
      error_log(tlh) << "run callback manager thread error,ret=" << ret << end_log;
      return LBAPI_ERR_THRAD_START;
    }
  }

  info_log(tlh) << "start callback manager ok" << end_log;
  return 0;
}

void callback_manager::stop() {
  if (mode_ == callback_mode::queued) {
    trigger();
    join();
  }
}

// close: 完整销毁 —— 停回调线程 + 关回调队列, 析构时调用
void callback_manager::close() {
  if (mode_ == callback_mode::queued) {
    trigger();
    join();
    cb_queue_.close();
  }
}

// ---- 回调接口 ----

void callback_manager::on_login(const LoginAns &ans) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_login(ans);
    return;
  }
  int32_t data_len = sizeof(LoginAns);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=login,ret=" << pos << ",client_req_no=" << ans.client_req_no
                   << ",cust_id=" << ans.cust_id.data() << ",fund_account_id=" << ans.fund_account_id.data()
                   << ",branch_id=" << ans.branch_id.data() << ",market_type=" << static_cast<int32_t>(ans.market_type)
                   << ",err_code=" << ans.err_code << ",err_msg=" << ans.err_msg.data()
                   << ",login_time=" << ans.login_time << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::login;
  head->data_len = data_len;
  *reinterpret_cast<LoginAns *>(data + sizeof(cb_event_head)) = ans;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_logout(const LogoutAns &ans) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_logout(ans);
    return;
  }
  int32_t data_len = sizeof(LogoutAns);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=logout,ret=" << pos << ",client_req_no=" << ans.client_req_no
                   << ",fund_account_id=" << ans.fund_account_id.data() << ",err_code=" << ans.err_code << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::logout;
  head->data_len = data_len;
  *reinterpret_cast<LogoutAns *>(data + sizeof(cb_event_head)) = ans;
  write_cmt(pos, total_len);
  trigger();
}

// on_trade_counter 有效载荷缓冲宽度 (来源 fpga 消息字段, 规范形 C 串)
static constexpr int32_t CB_BRANCH_ID_LEN = 12;   // G1_BRANCHID_LEN
static constexpr int32_t CB_FUND_ACCOUNT_ID_LEN = 16; // G1_FUNDACCOUNTID_LEN

void callback_manager::on_trade_counter(int32_t counter_type, const char *branch_id, const char *fund_account_id,
                                        int32_t status) {
  if (mode_ == callback_mode::direct) {
    // 字段兜底为空串: 直接回调模式不得把 NULL 传给用户 (api_callback 契约为字符串)
    user_callback_->on_trade_counter(counter_type, branch_id != nullptr ? branch_id : "",
                                     fund_account_id != nullptr ? fund_account_id : "", status);
    return;
  }
  // 有效载荷为 2 个 int32 (counter_type/status) + branch_id/fund_account_id 定长缓冲
  int32_t data_len = sizeof(int32_t) * 2 + CB_BRANCH_ID_LEN + CB_FUND_ACCOUNT_ID_LEN;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=trade_counter,ret=" << pos << ",counter_type=" << counter_type
                   << ",branch_id=" << (branch_id ? branch_id : "")
                   << ",fund_account_id=" << (fund_account_id ? fund_account_id : "") << ",status=" << status << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::trade_counter;
  head->data_len = data_len;
  char *payload = data + sizeof(cb_event_head);
  std::memset(payload, 0, data_len);
  *reinterpret_cast<int32_t *>(payload) = counter_type;
  *reinterpret_cast<int32_t *>(payload + sizeof(int32_t)) = status;
  char *branch_dst = payload + sizeof(int32_t) * 2;
  char *fund_dst = branch_dst + CB_BRANCH_ID_LEN;
  if (nullptr != branch_id)
    std::strncpy(branch_dst, branch_id, CB_BRANCH_ID_LEN - 1);
  if (nullptr != fund_account_id)
    std::strncpy(fund_dst, fund_account_id, CB_FUND_ACCOUNT_ID_LEN - 1);
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_order_rtn(const StreamInfo &si, const OrderRtn &rtn) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_order_rtn(si, rtn);
    return;
  }
  int32_t data_len = sizeof(StreamInfo) + sizeof(OrderRtn);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=order_rtn,ret=" << pos << ",counter_type=" << si.counter_type
                   << ",seq=" << si.stream_seq << ",cust_id=" << rtn.cust_id.data()
                   << ",fund_account_id=" << rtn.fund_account_id.data() << ",branch_id=" << rtn.branch_id.data()
                   << ",side=" << rtn.side << ",order_type=" << rtn.order_type << ",policy_id=" << rtn.policy_id
                   << ",order_status=" << static_cast<int32_t>(rtn.order_status)
                   << ",market_type=" << static_cast<int32_t>(rtn.market_type)
                   << ",security_id=" << rtn.security_id.data() << ",order_price=" << rtn.order_price
                   << ",order_qty=" << rtn.order_qty << ",client_seq_id=" << rtn.client_seq_id
                   << ",rtn_type=" << rtn.rtn_type << ",err_code=" << rtn.err_code
                   << ",order_sys_no=" << rtn.order_sys_no << ",frozen_amount=" << rtn.frozen_amount
                   << ",fee=" << rtn.fee << ",trade_qty=" << rtn.trade_qty << ",cancel_qty=" << rtn.cancel_qty
                   << ",order_time=" << rtn.order_time << ",update_time=" << rtn.update_time << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::order_rtn;
  head->data_len = data_len;
  *reinterpret_cast<StreamInfo *>(data + sizeof(cb_event_head)) = si;
  *reinterpret_cast<OrderRtn *>(data + sizeof(cb_event_head) + sizeof(StreamInfo)) = rtn;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_trade_rtn(const StreamInfo &si, const TradeRtn &rtn) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_trade_rtn(si, rtn);
    return;
  }
  int32_t data_len = sizeof(StreamInfo) + sizeof(TradeRtn);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=trade_rtn,ret=" << pos << ",counter_type=" << si.counter_type
                   << ",seq=" << si.stream_seq << ",cust_id=" << rtn.cust_id.data()
                   << ",fund_account_id=" << rtn.fund_account_id.data() << ",branch_id=" << rtn.branch_id.data()
                   << ",side=" << rtn.side << ",order_type=" << rtn.order_type << ",policy_id=" << rtn.policy_id
                   << ",order_status=" << static_cast<int32_t>(rtn.order_status)
                   << ",market_type=" << static_cast<int32_t>(rtn.market_type)
                   << ",security_id=" << rtn.security_id.data() << ",order_price=" << rtn.order_price
                   << ",order_qty=" << rtn.order_qty << ",client_seq_id=" << rtn.client_seq_id
                   << ",order_sys_no=" << rtn.order_sys_no << ",frozen_amount=" << rtn.frozen_amount
                   << ",fee=" << rtn.fee << ",trade_qty=" << rtn.trade_qty << ",cancel_qty=" << rtn.cancel_qty
                   << ",order_time=" << rtn.order_time << ",exec_time=" << rtn.exec_time
                   << ",exec_id=" << rtn.exec_id.data() << ",exec_price=" << rtn.exec_price
                   << ",exec_qty=" << rtn.exec_qty << ",exec_amount=" << rtn.exec_amount << ",exec_fee=" << rtn.exec_fee
                   << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::trade_rtn;
  head->data_len = data_len;
  *reinterpret_cast<StreamInfo *>(data + sizeof(cb_event_head)) = si;
  *reinterpret_cast<TradeRtn *>(data + sizeof(cb_event_head) + sizeof(StreamInfo)) = rtn;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_cancel_rsp(const StreamInfo &si, const CancelRsp &rsp) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_cancel_rsp(si, rsp);
    return;
  }
  int32_t data_len = sizeof(StreamInfo) + sizeof(CancelRsp);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=cancel_rsp,ret=" << pos << ",counter_type=" << si.counter_type
                   << ",seq=" << si.stream_seq << ",client_req_no=" << rsp.client_req_no
                   << ",cust_id=" << rsp.cust_id.data() << ",fund_account_id=" << rsp.fund_account_id.data()
                   << ",branch_id=" << rsp.branch_id.data() << ",client_seq_id=" << rsp.client_seq_id
                   << ",order_sys_no=" << rsp.order_sys_no << ",err_code=" << rsp.err_code << ",rej_api=" << rsp.rej_api
                   << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::cancel_rsp;
  head->data_len = data_len;
  *reinterpret_cast<StreamInfo *>(data + sizeof(cb_event_head)) = si;
  *reinterpret_cast<CancelRsp *>(data + sizeof(cb_event_head) + sizeof(StreamInfo)) = rsp;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_order_query_ans(const OrderRtn *ans_arr, const QueryAnsCtl &ctl) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_order_query_ans(ans_arr, ctl);
    return;
  }
  int32_t arr_bytes = ctl.count * sizeof(OrderRtn);
  int32_t data_len = sizeof(QueryAnsCtl) + arr_bytes;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=order_query,ret=" << pos << ",client_req_no=" << ctl.client_req_no
                   << ",count=" << ctl.count << ",is_last=" << ctl.is_last << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::order_query;
  head->data_len = data_len;
  *reinterpret_cast<QueryAnsCtl *>(data + sizeof(cb_event_head)) = ctl;
  if (arr_bytes > 0)
    std::memcpy(data + sizeof(cb_event_head) + sizeof(QueryAnsCtl), ans_arr, arr_bytes);
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_trade_query_ans(const TradeInfo *ans_arr, const QueryAnsCtl &ctl) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_trade_query_ans(ans_arr, ctl);
    return;
  }
  int32_t arr_bytes = ctl.count * sizeof(TradeInfo);
  int32_t data_len = sizeof(QueryAnsCtl) + arr_bytes;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=trade_query,ret=" << pos << ",client_req_no=" << ctl.client_req_no
                   << ",count=" << ctl.count << ",is_last=" << ctl.is_last << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::trade_query;
  head->data_len = data_len;
  *reinterpret_cast<QueryAnsCtl *>(data + sizeof(cb_event_head)) = ctl;
  if (arr_bytes > 0)
    std::memcpy(data + sizeof(cb_event_head) + sizeof(QueryAnsCtl), ans_arr, arr_bytes);
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_fund_query_ans(const CustFundInfo &info) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_fund_query_ans(info);
    return;
  }
  int32_t data_len = sizeof(CustFundInfo);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=fund_query,ret=" << pos << ",cust_id=" << info.cust_id.data()
                   << ",fund_account_id=" << info.fund_account_id.data() << ",branch_id=" << info.branch_id.data()
                   << ",avail_amount=" << info.avail_amount << ",token_amount=" << info.token_amount << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::fund_query;
  head->data_len = data_len;
  *reinterpret_cast<CustFundInfo *>(data + sizeof(cb_event_head)) = info;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_position_query_ans(const CustPositionInfo *ans_arr, const QueryAnsCtl &ctl) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_position_query_ans(ans_arr, ctl);
    return;
  }
  int32_t arr_bytes = ctl.count * sizeof(CustPositionInfo);
  int32_t data_len = sizeof(QueryAnsCtl) + arr_bytes;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=position_query,ret=" << pos << ",client_req_no=" << ctl.client_req_no
                   << ",count=" << ctl.count << ",is_last=" << ctl.is_last << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::position_query;
  head->data_len = data_len;
  *reinterpret_cast<QueryAnsCtl *>(data + sizeof(cb_event_head)) = ctl;
  if (arr_bytes > 0)
    std::memcpy(data + sizeof(cb_event_head) + sizeof(QueryAnsCtl), ans_arr, arr_bytes);
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_max_order_qty_query_ans(const CustMaxOrderQtyInfo &info) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_max_order_qty_query_ans(info);
    return;
  }
  int32_t data_len = sizeof(CustMaxOrderQtyInfo);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=max_order_qty_query,ret=" << pos << ",client_req_no=" << info.client_req_no
                   << ",fund_account_id=" << info.fund_account_id.data() << ",branch_id=" << info.branch_id.data() << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::max_order_qty_query;
  head->data_len = data_len;
  *reinterpret_cast<CustMaxOrderQtyInfo *>(data + sizeof(cb_event_head)) = info;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_total_fund_asset_query_ans(const CustTotalFundAssetInfo *ans_arr, const QueryAnsCtl &ctl) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_total_fund_asset_query_ans(ans_arr, ctl);
    return;
  }
  int32_t arr_bytes = ctl.count * sizeof(CustTotalFundAssetInfo);
  int32_t data_len = sizeof(QueryAnsCtl) + arr_bytes;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=total_fund_asset_query,ret=" << pos << ",client_req_no=" << ctl.client_req_no
                   << ",count=" << ctl.count << ",is_last=" << ctl.is_last << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::total_fund_asset_query;
  head->data_len = data_len;
  *reinterpret_cast<QueryAnsCtl *>(data + sizeof(cb_event_head)) = ctl;
  if (arr_bytes > 0)
    std::memcpy(data + sizeof(cb_event_head) + sizeof(QueryAnsCtl), ans_arr, arr_bytes);
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_fund_maxdraw_query_ans(const CustFundMaxdrawInfo *ans_arr, const QueryAnsCtl &ctl) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_fund_maxdraw_query_ans(ans_arr, ctl);
    return;
  }
  int32_t arr_bytes = ctl.count * sizeof(CustFundMaxdrawInfo);
  int32_t data_len = sizeof(QueryAnsCtl) + arr_bytes;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=fund_maxdraw_query,ret=" << pos << ",client_req_no=" << ctl.client_req_no
                   << ",count=" << ctl.count << ",is_last=" << ctl.is_last << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::fund_maxdraw_query;
  head->data_len = data_len;
  *reinterpret_cast<QueryAnsCtl *>(data + sizeof(cb_event_head)) = ctl;
  if (arr_bytes > 0)
    std::memcpy(data + sizeof(cb_event_head) + sizeof(QueryAnsCtl), ans_arr, arr_bytes);
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_cust_margin_avl_query_ans(const CustMarginAvlInfo &info) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_cust_margin_avl_query_ans(info);
    return;
  }
  int32_t data_len = sizeof(CustMarginAvlInfo);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=cust_margin_avl_query,ret=" << pos << ",client_req_no=" << info.client_req_no
                   << ",fund_account_id=" << info.fund_account_id.data() << ",branch_id=" << info.branch_id.data() << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::cust_margin_avl_query;
  head->data_len = data_len;
  *reinterpret_cast<CustMarginAvlInfo *>(data + sizeof(cb_event_head)) = info;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_asset_debts_query_ans(const AssetDebtsInfo &info) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_asset_debts_query_ans(info);
    return;
  }
  int32_t data_len = sizeof(AssetDebtsInfo);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=asset_debts_query,ret=" << pos << ",client_req_no=" << info.client_req_no
                   << ",fund_account_id=" << info.fund_account_id.data() << ",branch_id=" << info.branch_id.data() << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::asset_debts_query;
  head->data_len = data_len;
  *reinterpret_cast<AssetDebtsInfo *>(data + sizeof(cb_event_head)) = info;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_credit_max_order_qty_query_ans(const CreditMaxOrderQtyInfo &info) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_credit_max_order_qty_query_ans(info);
    return;
  }
  int32_t data_len = sizeof(CreditMaxOrderQtyInfo);
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=credit_max_order_qty_query,ret=" << pos << ",client_req_no=" << info.client_req_no
                   << ",fund_account_id=" << info.fund_account_id.data() << ",branch_id=" << info.branch_id.data() << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::credit_max_order_qty_query;
  head->data_len = data_len;
  *reinterpret_cast<CreditMaxOrderQtyInfo *>(data + sizeof(cb_event_head)) = info;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_link_status(int32_t counter_type, int32_t link_type, int32_t status) {
  if (mode_ == callback_mode::direct) {
    user_callback_->on_link_status(counter_type, link_type, status);
    return;
  }
  int32_t data_len = sizeof(int32_t) * 4;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=link_status,ret=" << pos << ",counter_type=" << counter_type
                   << ",status=" << status << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::link_status;
  head->data_len = data_len;
  *reinterpret_cast<int32_t *>(data + sizeof(cb_event_head)) = counter_type;
  *reinterpret_cast<int32_t *>(data + sizeof(cb_event_head) + sizeof(int32_t)) = link_type;
  *reinterpret_cast<int32_t *>(data + sizeof(cb_event_head) + sizeof(int32_t) + sizeof(int32_t)) = status;
  write_cmt(pos, total_len);
  trigger();
}

void callback_manager::on_error(err_event_type event_type, int32_t err_code, const char *err_desc) {
  if (mode_ == callback_mode::direct) {
    char terr_msg[2] = {0, 0};
    user_callback_->on_error(event_type, err_code, terr_msg);
    return;
  }
  // 错误描述固定MAX_ERR_DESC_LEN字节
  int32_t data_len = sizeof(int32_t) * 2 + MAX_ERR_DESC_LEN;
  int32_t total_len = sizeof(cb_event_head) + data_len;
  char *data;
  int64_t pos = write_get(data, total_len);
  if (pos <= 0) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "cb_queue enqueue fail,event=error,ret=" << pos
                   << ",event_type=" << static_cast<int32_t>(event_type) << ",err_code=" << err_code
                   << ",err_desc=" << (err_desc ? err_desc : "") << end_log;
    return;
  }
  cb_event_head *head = reinterpret_cast<cb_event_head *>(data);
  head->type = cb_event_type::error;
  head->data_len = data_len;
  *reinterpret_cast<int32_t *>(data + sizeof(cb_event_head)) = static_cast<int32_t>(event_type);
  *reinterpret_cast<int32_t *>(data + sizeof(cb_event_head) + sizeof(int32_t)) = err_code;
  char *desc_dst = data + sizeof(cb_event_head) + sizeof(int32_t) * 2;
  std::memset(desc_dst, 0, MAX_ERR_DESC_LEN);
  if (nullptr != err_desc)
    std::strncpy(desc_dst, err_desc, MAX_ERR_DESC_LEN - 1);
  write_cmt(pos, total_len);
  trigger();
}

// ---- simple_thread虚函数实现 ----

bool callback_manager::need_work() { return cb_queue_.get_used() > 0; }

// ---- 队列读取与分发 ----

void callback_manager::do_work() {
  char *data;
  while (cb_queue_.read_get(data) > 0) {

    const cb_event_head *head = reinterpret_cast<const cb_event_head *>(data);
    const char *event_data = data + sizeof(cb_event_head);

    switch (head->type) {
    case cb_event_type::login: {
      const LoginAns &ans = *reinterpret_cast<const LoginAns *>(event_data);
      user_callback_->on_login(ans);
      break;
    }
    case cb_event_type::logout: {
      const LogoutAns &ans = *reinterpret_cast<const LogoutAns *>(event_data);
      user_callback_->on_logout(ans);
      break;
    }
    case cb_event_type::trade_counter: {
      int32_t counter_type = *reinterpret_cast<const int32_t *>(event_data);
      int32_t status = *reinterpret_cast<const int32_t *>(event_data + sizeof(int32_t));
      const char *branch_id = event_data + sizeof(int32_t) * 2;
      const char *fund_account_id = branch_id + CB_BRANCH_ID_LEN;
      user_callback_->on_trade_counter(counter_type, branch_id, fund_account_id, status);
      break;
    }
    case cb_event_type::order_rtn: {
      const StreamInfo &si = *reinterpret_cast<const StreamInfo *>(event_data);
      const OrderRtn &rtn = *reinterpret_cast<const OrderRtn *>(event_data + sizeof(StreamInfo));
      user_callback_->on_order_rtn(si, rtn);
      break;
    }
    case cb_event_type::trade_rtn: {
      const StreamInfo &si = *reinterpret_cast<const StreamInfo *>(event_data);
      const TradeRtn &rtn = *reinterpret_cast<const TradeRtn *>(event_data + sizeof(StreamInfo));
      user_callback_->on_trade_rtn(si, rtn);
      break;
    }
    case cb_event_type::cancel_rsp: {
      const StreamInfo &si = *reinterpret_cast<const StreamInfo *>(event_data);
      const CancelRsp &rtn = *reinterpret_cast<const CancelRsp *>(event_data + sizeof(StreamInfo));
      user_callback_->on_cancel_rsp(si, rtn);
      break;
    }
    case cb_event_type::order_query: {
      const QueryAnsCtl &ctl = *reinterpret_cast<const QueryAnsCtl *>(event_data);
      const OrderRtn *arr = reinterpret_cast<const OrderRtn *>(event_data + sizeof(QueryAnsCtl));
      user_callback_->on_order_query_ans(arr, ctl);
      break;
    }
    case cb_event_type::trade_query: {
      const QueryAnsCtl &ctl = *reinterpret_cast<const QueryAnsCtl *>(event_data);
      const TradeInfo *arr = reinterpret_cast<const TradeInfo *>(event_data + sizeof(QueryAnsCtl));
      user_callback_->on_trade_query_ans(arr, ctl);
      break;
    }
    case cb_event_type::fund_query: {
      const CustFundInfo &info = *reinterpret_cast<const CustFundInfo *>(event_data);
      user_callback_->on_fund_query_ans(info);
      break;
    }
    case cb_event_type::position_query: {
      const QueryAnsCtl &ctl = *reinterpret_cast<const QueryAnsCtl *>(event_data);
      const CustPositionInfo *arr = reinterpret_cast<const CustPositionInfo *>(event_data + sizeof(QueryAnsCtl));
      user_callback_->on_position_query_ans(arr, ctl);
      break;
    }
    case cb_event_type::max_order_qty_query: {
      const CustMaxOrderQtyInfo &info = *reinterpret_cast<const CustMaxOrderQtyInfo *>(event_data);
      user_callback_->on_max_order_qty_query_ans(info);
      break;
    }
    case cb_event_type::total_fund_asset_query: {
      const QueryAnsCtl &ctl = *reinterpret_cast<const QueryAnsCtl *>(event_data);
      const CustTotalFundAssetInfo *arr = reinterpret_cast<const CustTotalFundAssetInfo *>(event_data + sizeof(QueryAnsCtl));
      user_callback_->on_total_fund_asset_query_ans(arr, ctl);
      break;
    }
    case cb_event_type::fund_maxdraw_query: {
      const QueryAnsCtl &ctl = *reinterpret_cast<const QueryAnsCtl *>(event_data);
      const CustFundMaxdrawInfo *arr = reinterpret_cast<const CustFundMaxdrawInfo *>(event_data + sizeof(QueryAnsCtl));
      user_callback_->on_fund_maxdraw_query_ans(arr, ctl);
      break;
    }
    case cb_event_type::cust_margin_avl_query: {
      const CustMarginAvlInfo &info = *reinterpret_cast<const CustMarginAvlInfo *>(event_data);
      user_callback_->on_cust_margin_avl_query_ans(info);
      break;
    }
    case cb_event_type::asset_debts_query: {
      const AssetDebtsInfo &info = *reinterpret_cast<const AssetDebtsInfo *>(event_data);
      user_callback_->on_asset_debts_query_ans(info);
      break;
    }
    case cb_event_type::credit_max_order_qty_query: {
      const CreditMaxOrderQtyInfo &info = *reinterpret_cast<const CreditMaxOrderQtyInfo *>(event_data);
      user_callback_->on_credit_max_order_qty_query_ans(info);
      break;
    }
    case cb_event_type::link_status: {
      int32_t counter_type = *reinterpret_cast<const int32_t *>(event_data);
      int32_t link_type = *reinterpret_cast<const int32_t *>(event_data + sizeof(int32_t));
      int32_t status = *reinterpret_cast<const int32_t *>(event_data + sizeof(int32_t) * 2);
      user_callback_->on_link_status(counter_type, link_type, status);
      break;
    }
    case cb_event_type::error: {
      int32_t et = *reinterpret_cast<const int32_t *>(event_data);
      int32_t ec = *reinterpret_cast<const int32_t *>(event_data + sizeof(int32_t));
      const char *desc = event_data + sizeof(int32_t) * 2;
      user_callback_->on_error(static_cast<err_event_type>(et), ec, desc);
      break;
    }
    }

    cb_queue_.read_cmt(static_cast<int32_t>(sizeof(cb_event_head) + head->data_len));
  }
}

} // namespace lb_api

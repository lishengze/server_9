// fdm_board - FDM 板卡信息（聚合根）实现
//
// Step 8 S6：板卡初始化 + 客户填充（按 user_id dense 索引）

#include "fdm_board.h"
#include "fgw_errno.h"

namespace lb_fgw {

// ---- fill_customers：load_custs 后填充客户列表 ----
// user_id 板内 0 连续（需求 §8）→ customers_[user_id] dense 索引
void fdm_board::fill_customers(std::vector<customer> &custs) {
  uint32_t ts = custs.size();
  for (customer &c : custs) {
    if (c.user_id_ >= ts && c.fund_account_id_[0] != '\0') {
      ts = c.user_id_ + 1;
    }
  }
  if (ts > customers_.size()) {
    customers_.resize(ts);
  }
  for (customer &c : custs) {
    if (c.fund_account_id_[0] != '\0') {
      lb_common::comm_utils::str_format(c.cust_id_, sizeof(c.cust_id_));
      lb_common::comm_utils::str_format(c.branch_id_, sizeof(c.branch_id_));
      lb_common::comm_utils::str_format(c.fund_account_id_, sizeof(c.fund_account_id_));
      lb_common::comm_utils::str_format(c.holder_acc_, sizeof(c.holder_acc_));
      c.onboard_state = 1;
      customers_[c.user_id_] = c;
    }
  }
}

int32_t fdm_board::add_fdm_session(uint32_t session_id, api_link *ln_api) {
  if (!ln_api->add_ref())
    return FGW_ERR_LINK_STATE;

  if (likely(session_id < fdm_sessions_.size())) {
    fdm_session &tc = fdm_sessions_[session_id];
    if (tc.is_load()) {
      if (nullptr != tc.ln_api_) {
        tc.ln_api_->sub_ref();
        tc.ln_api_ = nullptr;
      }
      tc.ln_api_ = ln_api;
      tc.ln_fdm_ = &link_;
      return 0;
    }
    tc.board_no_ = board_no_;
    tc.session_id_ = session_id;
    tc._pad_ = 0;
    tc.ln_api_ = ln_api;
    tc.ln_fdm_ = &link_;
    return 0;
  }

  fdm_sessions_.resize(session_id + 1);
  fdm_session &tc = fdm_sessions_[session_id];
  tc.board_no_ = board_no_;
  tc.session_id_ = session_id;
  tc._pad_ = 0;
  tc.ln_api_ = ln_api;
  tc.ln_fdm_ = &link_;
  return 0;
}

void fdm_board::detach_fdm_session_link(uint32_t session_id, api_link *ln_api) {
  fdm_session *fdm_ses = find_fdm_session(session_id);
  if (nullptr == fdm_ses)
    return;
  if (fdm_ses->ln_api_ != ln_api)
    return;
  ln_api->sub_ref();
}

void fdm_board::dispatch_onboard_state(g1_msg_head *head) {
  for (const auto &fs : fdm_sessions_) {
    if (fs.ln_api_ == nullptr)
      continue;
    fs.ln_api_->push_onboard_state(head);
  }
}

bool fdm_board::get_addr(lb_common::csock_addr &o_addr) {
  ilock.lock();
  if (load_status_ == BOARD_LOADED) {
    o_addr = remote_;
    ilock.unlock();
    return true;
  }
  ilock.unlock();
  return false;
}
void fdm_board::load_done(const char *ip, int32_t port, int32_t load_time) {
  ilock.lock();
  load_time_ = load_time;
  std::memset(&remote_, 0, sizeof(remote_));
  std::strncpy(remote_.ip, ip, sizeof(remote_.ip));
  remote_.port = port;
  load_status_ = BOARD_LOADED;
  ilock.unlock();
}
bool fdm_board::check_load_reset(int32_t new_load_time) {
  ilock.lock();
  if (load_status_ == BOARD_LOADED && load_time_ != 0 && load_time_ < new_load_time) {
    load_status_ = BOARD_LOAD_NOT;
    ilock.unlock();
    return true;
  }
  ilock.unlock();
  return false;
}

} // namespace lb_fgw

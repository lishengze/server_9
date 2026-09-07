// data_repo - 公共数据存储实现
//
// Step 8 S6：跨模块共享索引的增删查（hash_map_mth 自带表级自旋锁）
// R-13：data_engine 拥有，其他模块裸指针只读访问

#include "data_repo.h"

#include <cstdint>
#include <cstring>

#include "fgw_errno.h"
#include "g1trademsg.h"
#include "mlock.h"
#include "mutils.h"

namespace lb_fgw {

// ---- init：初始化 3 个哈希表 ----
int32_t data_repo::init() {
  int32_t ret = fdms_.init(256);
  if (ret < 0)
    return ret;
  ret = agwusers_.init(1024);
  if (ret < 0)
    return ret;
  return cust_map_.init(1024);
}

// ---- fdms_（board_no → fdm_board*）板卡路由 ----

int32_t data_repo::add_fdm(uint16_t board_no, fdm_board *board) {
  // 先查找是否已存在
  fdm_board *exist = nullptr;
  int32_t ret = fdms_.find(exist, board_no, 1);
  if (ret == 0 && exist != nullptr)
    return FGW_ERR_DUP_EXIST; // 板卡号已存在，拒绝重复插入

  ret = fdms_.insert(board, board_no);
  if (ret < 0)
    return ret;
  lb_common::clock_guard<lb_common::cmutex> lk(vec_lock_);
  fdm_vec_.push_back(board);
  return 0;
}

void data_repo::get_fdm_boards(std::vector<fdm_board *> &o_boards) {
  lb_common::clock_guard<lb_common::cmutex> lk(vec_lock_);
  o_boards.assign(fdm_vec_.begin(), fdm_vec_.end());
}

// ---- agwusers_（agw_user_key → agw_user*）登录中转 ----
agw_user *data_repo::add_agwuser_link(const agw_user_key &key, api_link *link) {
  // 查找是否已存在同 key 的 agw_user
  agw_user *exist = nullptr;
  int32_t ret = agwusers_.find(exist, key, 1);
  if (ret == 0 && exist != nullptr) {
    ret = exist->attach_api_link(link);
    if (ret == 0)
      return exist;
    else
      return nullptr;
  }

  // 新建 agw_user
  agw_user *user = new (std::nothrow) agw_user();
  if (user == nullptr)
    return nullptr;
  ret = user->init(key.user_name, link);
  if (ret < 0) {
    delete user;
    return nullptr;
  }

  ret = agwusers_.insert(user, key);
  if (ret < 0) {
    delete user;
    return nullptr;
  }

  lb_common::clock_guard<lb_common::cmutex> lk(vec_lock_);
  agwuser_vec_.push_back(user);
  return user;
}

int32_t agw_user::init(const char *agw_user_name, api_link *new_link) {
  lb_common::comm_utils::str_copy_format(user_name_, agw_user_name, sizeof(user_name_));
  if (new_link->add_ref()) {
    link_ = new_link;
  } else {
    link_ = nullptr;
    return FGW_ERR_LINK_STATE;
  }
  fdm_ses.reserve(16);
  return 0;
}
int32_t agw_user::attach_api_link(api_link *new_link) {
  lb_common::clock_guard<lb_common::cmutex> lk(mlock_);
  if (link_ == new_link)
    return 0;
  if (link_ != nullptr) {
    link_->sub_ref();
    link_ = nullptr;
  }

  // 已存在：更新 link 绑定
  if (new_link->add_ref()) {
    link_ = new_link;
    return 0;
  }
  return FGW_ERR_LINK_STATE;
}
void agw_user::add_fdm_session(uint16_t board, uint32_t session) {
  lb_common::clock_guard<lb_common::cmutex> lk(mlock_);
  for (board_session_id &fs : fdm_ses) {
    if (fs.board_no_ == board && fs.session_id_ == session)
      return;
  }
  board_session_id ts;
  ts.board_no_ = board;
  ts.session_id_ = session;
  fdm_ses.push_back(ts);
}
bool agw_user::detach_api_link(std::vector<board_session_id> &o_ses, api_link *cmp_link) {
  o_ses.clear();
  lb_common::clock_guard<lb_common::cmutex> lk(mlock_);
  if (link_ != cmp_link)
    return false;
  for (board_session_id &fs : fdm_ses) {
    if (fs.board_no_ == 0)
      continue;
    o_ses.push_back(fs);
  }
  link_ = nullptr;
  return true;
}

void data_repo::detach_agwuser_link(api_link *link) {
  if (link == nullptr)
    return;

  std::vector<board_session_id> tses;
  agw_user *found = nullptr;
  vec_lock_.lock();
  for (auto *user : agwuser_vec_) {
    if (user->detach_api_link(tses, link)) {
      found = user;
      break;
    }
  }
  vec_lock_.unlock();

  if (found == nullptr)
    return;
  for (board_session_id &fs : tses) {
    if (fs.board_no_ == 0)
      continue;
    fdm_board *pboard = nullptr;
    int32_t ret = find_fdm(pboard, fs.board_no_);
    if (ret < 0 || nullptr == pboard)
      continue;
    api_link_deref_data ev;
    ev.board = pboard;
    ev.session_id = fs.session_id_;
    ev.expected = link;
    pboard->get_link()->post_recv_event(&deref_op_, &ev, sizeof(ev), 0);
  }

  link->sub_ref(); // 释放 add_agwuser_link 持的引用
}

// ---- cust_map_（fundacc_key → board_user_id）客户索引 ----

int32_t data_repo::add_cust(const fgw_fundacc_key &key, const board_user_id &id) {
  // 先查找是否已存在
  board_user_id exist;
  int32_t ret = cust_map_.find(exist, key, 1);
  if (ret == 0)
    return 0; // 客户已存在，拒绝重复插入

  board_user_id tmp = id;
  return cust_map_.insert(tmp, key);
}

// ---- secs_ 证券信息 ----
void data_repo::add_sec(std::vector<sec_info> &db_secs) {
  uint32_t ts = db_secs.size();
  for (auto &c : db_secs) {
    if (c.sec_index_ >= ts && c.security_id_[0] != '\0') {
      ts = c.sec_index_ + 1;
    }
  }
  if (ts > secs.size()) {
    secs.resize(ts);
  }

  lb_common::clock_guard<lb_common::cmutex> lk(vec_lock_);
  for (auto &c : db_secs) {
    if (c.security_id_[0] != '\0') {
      lb_common::comm_utils::str_format(c.security_id_, sizeof(c.security_id_));
      secs[c.sec_index_] = c;
    }
  }
}

int32_t data_repo::patch_secs_msg(sec_push_head *buf_head, int32_t buf_len, int32_t start_idx) {
  if (buf_head == nullptr || buf_len < (int32_t)(sizeof(sec_push_head) + sizeof(sec_push_info)))
    return FGW_ERR_PARAM;

  lb_common::clock_guard<lb_common::cmutex> lk(vec_lock_);
  uint16_t total = (uint16_t)secs.size();
  if (start_idx >= total) {
    buf_head->total_num = 0;
    buf_head->cur_num = 0;
    buf_head->err_code = 0;
    return 0;
  }

  int32_t body_len = buf_len - (int32_t)sizeof(sec_push_head);
  int32_t max_info = body_len / (int32_t)sizeof(sec_push_info);
  int32_t remaining = total - start_idx;
  int32_t cur_num = 0; //(remaining < max_info) ? remaining : max_info;

  sec_push_info *info = reinterpret_cast<sec_push_info *>(buf_head + 1);
  int32_t i = start_idx;
  while (i < (int32_t)(secs.size())) {
    const sec_info &src = secs[i];
    if (src.sec_index_ < 0 || src.security_id_[0] == '\0') {
      i++;
      continue;
    }

    sec_push_info &dst = info[i];
    std::memcpy(dst.security_id, src.security_id_, sizeof(dst.security_id));
    dst.market_type = src.market_type_;
    dst.sec_index = src.sec_index_;
    dst.buy_qty_unit = src.buy_qty_unit_;
    dst.sell_qty_unit = src.sell_qty_unit_;
    dst.reserved = 0;
    dst.price_unit = src.price_unit_;
    cur_num++;
    if (cur_num >= max_info)
      break;
  }

  if (i < (int32_t)(secs.size())) {
    buf_head->total_num = (uint16_t)remaining;
    buf_head->cur_num = (uint16_t)cur_num;
    buf_head->err_code = 0;
  } else {
    buf_head->total_num = (uint16_t)cur_num;
    buf_head->cur_num = (uint16_t)cur_num;
    buf_head->err_code = 0;
  }

  return i - start_idx;
}

} // namespace lb_fgw

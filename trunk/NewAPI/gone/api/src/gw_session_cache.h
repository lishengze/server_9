// gw_session_cache - 个微柜台全局会话缓存
//
// 全局单例，以 fund_account_id 为主键，缓存客户会话信息。
// 登录请求发出时（deal_cust_login）创建会话，登录应答到达时（deal_log_ans）回填 cust_id/account_id。
// 同时维护 order_sys_no → {clordno, client_seq_id} 映射，供撤单定位原单。

#pragma once

#include "api_event_msg.h"
#include "gw_head.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>

namespace lb_api {

/// 原单定位信息（撤单时定位 FTE 原单用）
struct OrderLocator {
  int64_t clordno = 0;         ///< FTE 内部订单编号（orig_clordno 用）
  int64_t client_seq_id = 0;   ///< 原委托的 client_seq_id（orig_client_seq_id 用）
};

/// 客户会话信息
struct GwSessionInfo {
  std::array<char, 16> fund_account_id;  ///< 资金账号（主键）
  std::array<char, 16> cust_id;          ///< 客户号（从 LogOnAns 回填）
  std::array<char, 12> account_id;       ///< 股东账户（从 LogOnAns 回填）
  std::array<char, 10> branch_id;        ///< 分支机构代码
  std::array<char, 2> order_way_ext;     ///< 客户委托方式
  std::array<char, 64> user_info;        ///< 用户私有信息
  int16_t market_type = 0;               ///< 市场类型

  /// 原单定位映射表：order_sys_no → {clordno, client_seq_id}
  std::unordered_map<int64_t, OrderLocator> order_locators;

  void reset() {
    fund_account_id.fill(' ');
    cust_id.fill(' ');
    account_id.fill(' ');
    branch_id.fill(' ');
    order_way_ext.fill(' ');
    user_info.fill(' ');
    market_type = 0;
    order_locators.clear();
  }
};

/// 全局会话缓存管理器（单例）
class GwSessionCache {
public:
  static GwSessionCache& instance();

  /// 登录请求发出时创建/更新会话（基于 acc_login_event_info）
  void create_session(const acc_login_event_info& login_req);

  /// 登录应答到达时回填 cust_id / account_id（基于 LogOnAns）
  void fill_session_from_ans(const gw_message::LogOnAns& ans);

  /// 获取会话信息（找不到返回 nullptr）
  GwSessionInfo* get_session(const std::string& fund_account_id);

  /// 记录原单定位信息（收到 OrderRtn/TradeRtn 时记录）
  void record_order_locator(const std::string& fund_account_id,
                            int64_t order_sys_no, int64_t clordno, int64_t client_seq_id);

  /// 根据 order_sys_no 查找 clordno（找不到返回 0）
  int64_t get_clordno(const std::string& fund_account_id, int64_t order_sys_no);

  /// 根据 order_sys_no 查找原委托 client_seq_id（找不到返回 0）
  int64_t get_orig_client_seq_id(const std::string& fund_account_id, int64_t order_sys_no);

private:
  GwSessionCache() = default;
  ~GwSessionCache() = default;
  GwSessionCache(const GwSessionCache&) = delete;
  GwSessionCache& operator=(const GwSessionCache&) = delete;

  /// 从 fund_account_id 提取字符串键
  static std::string fa_key(const std::array<char, 16>& fa);

  std::unordered_map<std::string, GwSessionInfo> sessions_;
};

// ====== 内联实现 ======

inline GwSessionCache& GwSessionCache::instance() {
  static GwSessionCache cache;
  return cache;
}

inline std::string GwSessionCache::fa_key(const std::array<char, 16>& fa) {
  return std::string(fa.data(), strnlen(fa.data(), 16));
}

inline void GwSessionCache::create_session(const acc_login_event_info& login_req) {
  std::string key(login_req.fund_account_id, strnlen(login_req.fund_account_id, 16));
  GwSessionInfo& info = sessions_[key];
  info.reset();
  memcpy(info.fund_account_id.data(), login_req.fund_account_id, sizeof(info.fund_account_id));
  memcpy(info.branch_id.data(), login_req.branch_id, sizeof(info.branch_id));
  memcpy(info.order_way_ext.data(), login_req.order_way_ext, sizeof(info.order_way_ext));
  memcpy(info.user_info.data(), login_req.user_info, sizeof(info.user_info));
}

inline void GwSessionCache::fill_session_from_ans(const gw_message::LogOnAns& ans) {
  std::string key(ans.fund_account_id.data(), strnlen(ans.fund_account_id.data(), 16));
  auto it = sessions_.find(key);
  if (it == sessions_.end()) return;
  memcpy(it->second.cust_id.data(), ans.cust_id.data(), sizeof(it->second.cust_id));
  memcpy(it->second.account_id.data(), ans.account_id.data(), sizeof(it->second.account_id));
}

inline GwSessionInfo* GwSessionCache::get_session(const std::string& fund_account_id) {
  auto it = sessions_.find(fund_account_id);
  if (it != sessions_.end()) return &it->second;
  return nullptr;
}

inline void GwSessionCache::record_order_locator(const std::string& fund_account_id,
                                                  int64_t order_sys_no, int64_t clordno,
                                                  int64_t client_seq_id) {
  auto it = sessions_.find(fund_account_id);
  if (it == sessions_.end()) return;
  OrderLocator loc;
  loc.clordno = clordno;
  loc.client_seq_id = client_seq_id;
  it->second.order_locators[order_sys_no] = loc;
}

inline int64_t GwSessionCache::get_clordno(const std::string& fund_account_id, int64_t order_sys_no) {
  auto it = sessions_.find(fund_account_id);
  if (it == sessions_.end()) return 0;
  auto loc_it = it->second.order_locators.find(order_sys_no);
  if (loc_it != it->second.order_locators.end()) return loc_it->second.clordno;
  return 0;
}

inline int64_t GwSessionCache::get_orig_client_seq_id(const std::string& fund_account_id,
                                                       int64_t order_sys_no) {
  auto it = sessions_.find(fund_account_id);
  if (it == sessions_.end()) return 0;
  auto loc_it = it->second.order_locators.find(order_sys_no);
  if (loc_it != it->second.order_locators.end()) return loc_it->second.client_seq_id;
  return 0;
}

} // namespace lb_api
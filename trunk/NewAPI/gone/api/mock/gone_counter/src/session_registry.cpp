#include "session_registry.h"

SessionRegistry &SessionRegistry::instance() {
  static SessionRegistry registry;
  return registry;
}

void SessionRegistry::register_session(const std::string &fund_account_id, const SessionInfo &info) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_[fund_account_id] = info;
}

SessionInfo SessionRegistry::get_session(const std::string &fund_account_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sessions_.find(fund_account_id);
  if (it != sessions_.end()) {
    return it->second;
  }
  return SessionInfo{};
}

bool SessionRegistry::is_logged_in(const std::string &fund_account_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sessions_.find(fund_account_id);
  return it != sessions_.end() && it->second.logged_in;
}

void SessionRegistry::remove_session(const std::string &fund_account_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_.erase(fund_account_id);
}

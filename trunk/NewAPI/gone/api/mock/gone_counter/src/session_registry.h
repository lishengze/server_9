#ifndef SESSION_REGISTRY_H
#define SESSION_REGISTRY_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

/// 会话信息：GW 链路登录成功后注册，Core 链路处理业务时查询
struct SessionInfo {
  std::string fund_account_id; ///< 资金账号
  std::string branch_id;       ///< 分支机构
  uint16_t user_id = 0;        ///< 用户 ID 索引
  uint16_t board_no = 0;       ///< 板卡号
  uint32_t session_id = 0;     ///< 会话号
  bool logged_in = false;      ///< 是否已登录
};

/// 全局会话注册表（单例）
/// GW 链路登录成功后写入，Core 链路收到业务消息时按 fund_account_id 查询
class SessionRegistry {
public:
  static SessionRegistry &instance();

  void register_session(const std::string &fund_account_id, const SessionInfo &info);
  SessionInfo get_session(const std::string &fund_account_id);
  bool is_logged_in(const std::string &fund_account_id);
  void remove_session(const std::string &fund_account_id);

private:
  SessionRegistry() = default;
  ~SessionRegistry() = default;
  SessionRegistry(const SessionRegistry &) = delete;
  SessionRegistry &operator=(const SessionRegistry &) = delete;

  std::unordered_map<std::string, SessionInfo> sessions_;
  std::mutex mutex_;
};

#endif // SESSION_REGISTRY_H

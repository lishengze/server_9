#ifndef MOCK_98_ACCOUNT_MANAGER_H
#define MOCK_98_ACCOUNT_MANAGER_H

#include "json_utils.h"
#include <string>
#include <vector>
#include <cstdint>

namespace mock_98 {

/// AGW 用户配置
struct AgwUserConfig {
    std::string user;
    std::string password;
    std::string description;
};

/// 账户配置
struct AccountConfig {
    std::string fund_account_id;
    std::string password;
    std::string cust_id;
    std::string account_id;
    std::string branch_id;
    int market_type;
    std::string description;
};

/// 账户管理器
class AccountManager {
public:
    AccountManager();

    /// 从 JSON 文件加载配置
    bool load_config(const std::string& path);

    /// 验证 AGW 用户
    bool verify_agw_user(const std::string& user, const std::string& password);

    /// 验证账户
    bool verify_account(const std::string& fund_account_id, const std::string& password);

    /// 获取账户信息
    bool get_account_info(const std::string& fund_account_id, AccountConfig& info) const;

    /// 生成会话 ID
    std::string generate_session_id();

    /// 获取 AGW 用户列表
    const std::vector<AgwUserConfig>& agw_users() const { return agw_users_; }

    /// 获取账户列表
    const std::vector<AccountConfig>& accounts() const { return accounts_; }

private:
    std::vector<AgwUserConfig> agw_users_;
    std::vector<AccountConfig> accounts_;
    int64_t session_counter_;
};

} // namespace mock_98

#endif // MOCK_98_ACCOUNT_MANAGER_H

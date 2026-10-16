#ifndef MOCK_GONE_ACCOUNT_MANAGER_H
#define MOCK_GONE_ACCOUNT_MANAGER_H

#include "json_utils.h"
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

namespace mock_gone {

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

    /// 验证账户（g1 登录时使用 fund_account_id + password）
    bool verify_account(const std::string& fund_account_id, const std::string& password);

    /// 获取账户信息
    bool get_account_info(const std::string& fund_account_id, AccountConfig& info) const;

    /// 生成会话号（session_id，自增）
    uint32_t generate_session_id();

    /// 获取账户列表
    const std::vector<AccountConfig>& accounts() const { return accounts_; }

private:
    std::vector<AccountConfig> accounts_;
    uint32_t session_counter_;
    mutable std::mutex mutex_;
};

} // namespace mock_gone

#endif // MOCK_GONE_ACCOUNT_MANAGER_H

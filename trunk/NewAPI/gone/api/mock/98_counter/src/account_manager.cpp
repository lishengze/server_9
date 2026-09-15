// account_manager.cpp - AccountManager 账户管理器实现
//
// 职责：管理 98 柜台模拟服务端的 AGW 用户和资金账户配置。
//   从 JSON 配置文件加载用户/账户列表，提供验证和查询接口。
//   generate_session_id() 使用时间戳+自增计数器生成唯一会话 ID。

#include "account_manager.h"
#include <sstream>
#include <ctime>

namespace mock_98 {

AccountManager::AccountManager()
    : session_counter_(0)
{
}

// load_config: 从 JSON 文件加载 AGW 用户和资金账户配置
// 配置结构：
//   { "agw_users": [{"user":"...", "password":"...", "description":"..."}],
//     "accounts": [{"fund_account_id":"...", "password":"...", ...}] }
bool AccountManager::load_config(const std::string& path) {
    try {
        mock::JsonValue root = mock::JsonParser::parse_file(path);

        // 加载 AGW 用户
        mock::JsonValue users = root["agw_users"];
        for (size_t i = 0; i < users.size(); i++) {
            AgwUserConfig cfg;
            cfg.user = users[i]["user"].as_string();
            cfg.password = users[i]["password"].as_string();
            cfg.description = users[i]["description"].as_string();
            agw_users_.push_back(cfg);
        }

        // 加载账户
        mock::JsonValue accts = root["accounts"];
        for (size_t i = 0; i < accts.size(); i++) {
            AccountConfig cfg;
            cfg.fund_account_id = accts[i]["fund_account_id"].as_string();
            cfg.password = accts[i]["password"].as_string();
            cfg.cust_id = accts[i]["cust_id"].as_string();
            cfg.account_id = accts[i]["account_id"].as_string();
            cfg.branch_id = accts[i]["branch_id"].as_string();
            cfg.market_type = accts[i]["market_type"].as_int();
            cfg.description = accts[i]["description"].as_string();
            accounts_.push_back(cfg);
        }

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// verify_agw_user: 校验 AGW 用户（用户名 + 密码同时匹配）
bool AccountManager::verify_agw_user(const std::string& user, const std::string& password) {
    for (size_t i = 0; i < agw_users_.size(); i++) {
        if (agw_users_[i].user == user && agw_users_[i].password == password) {
            return true;
        }
    }
    return false;
}

// verify_account: 校验资金账户（资金账号 + 密码同时匹配）
bool AccountManager::verify_account(const std::string& fund_account_id, const std::string& password) {
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].fund_account_id == fund_account_id &&
            accounts_[i].password == password) {
            return true;
        }
    }
    return false;
}

bool AccountManager::get_account_info(const std::string& fund_account_id, AccountConfig& info) const {
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].fund_account_id == fund_account_id) {
            info = accounts_[i];
            return true;
        }
    }
    return false;
}

std::string AccountManager::generate_session_id() {
    std::lock_guard<std::mutex> lock(mutex_);
    session_counter_++;
    std::ostringstream oss;
    oss << "SESS" << std::time(nullptr) << "_" << session_counter_;
    return oss.str();
}

} // namespace mock_98

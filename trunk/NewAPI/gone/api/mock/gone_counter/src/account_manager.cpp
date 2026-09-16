#include "account_manager.h"
#include <iostream>
#include <cstring>

namespace mock_gone {

AccountManager::AccountManager() : session_counter_(1000) {}

bool AccountManager::load_config(const std::string& path) {
    try {
        mock::JsonValue root = mock::JsonParser::parse_file(path);

        // 加载账户
        if (root.has("accounts")) {
            mock::JsonValue arr = root["accounts"];
            for (size_t i = 0; i < arr.size(); i++) {
                mock::JsonValue item = arr[i];
                AccountConfig acct;
                acct.fund_account_id = item["fund_account_id"].as_string();
                acct.password = item["password"].as_string();
                acct.cust_id = item["cust_id"].as_string();
                acct.account_id = item["account_id"].as_string();
                acct.branch_id = item["branch_id"].as_string();
                acct.market_type = item["market_type"].as_int();
                acct.description = item.has("description") ? item["description"].as_string() : "";
                accounts_.push_back(acct);
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AccountManager] 加载配置异常: " << e.what() << std::endl;
        return false;
    }
}

bool AccountManager::verify_account(const std::string& fund_account_id, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].fund_account_id == fund_account_id) {
            return accounts_[i].password == password;
        }
    }
    return false;
}

bool AccountManager::get_account_info(const std::string& fund_account_id, AccountConfig& info) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].fund_account_id == fund_account_id) {
            info = accounts_[i];
            return true;
        }
    }
    return false;
}

uint32_t AccountManager::generate_session_id() {
    std::lock_guard<std::mutex> lock(mutex_);
    return ++session_counter_;
}

} // namespace mock_gone

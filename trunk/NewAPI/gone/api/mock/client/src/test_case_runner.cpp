#include "test_case_runner.h"
#include "api_interface.h"
#include "api_config.h"
#include "order_trade_type.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <dirent.h>

namespace mock {

TestCaseRunner::TestCaseRunner(lb_api::api_interface* api, CallbackHandler* handler)
    : api_(api), handler_(handler)
{
}

TestCaseType TestCaseRunner::parse_type(const std::string& type_str) {
    if (type_str == "login") return TestCaseType::Login;
    if (type_str == "order_insert" || type_str == "order_rtn") return TestCaseType::OrderInsert;
    if (type_str == "etf_order_insert" || type_str == "trade_rtn") return TestCaseType::EtfOrderInsert;
    if (type_str == "order_cancel" || type_str == "cancel_rsp") return TestCaseType::OrderCancel;
    if (type_str == "wait_heartbeat" || type_str == "heartbeat_ok") return TestCaseType::WaitHeartbeat;
    return TestCaseType::Unknown;
}

bool TestCaseRunner::load_test_case(const std::string& path) {
    try {
        JsonValue root = JsonParser::parse_file(path);
        JsonValue tc = root["test_case"];

        TestCase test_case;
        test_case.name = tc["name"].as_string();
        test_case.description = tc["description"].as_string();
        test_case.counter_type = tc["counter_type"].as_string();
        test_case.timeout_ms = tc["timeout_ms"].as_int();

        // 解析请求
        JsonValue req = root["request"];
        test_case.request_type = parse_type(req["type"].as_string());
        test_case.request_fields = req["fields"];

        // 解析预期回报
        JsonValue exp = root["expected_response"];
        test_case.response_type = parse_type(exp["type"].as_string());

        // 解析预期字段
        JsonValue fields = exp["fields"];
        JsonValue validate = exp["validate"];
        for (size_t i = 0; i < validate.size(); i++) {
            std::string field_name = validate[i].as_string();
            FieldMatch fm;
            fm.field_name = field_name;
            fm.expected_value = fields[field_name];
            fm.required = true;
            test_case.expected_fields.push_back(fm);
        }

        test_cases_.push_back(test_case);
        std::cout << "[Loader] 加载测试用例: " << test_case.name
                  << " (type=" << req["type"].as_string() << ")" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Loader] 加载失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

int TestCaseRunner::load_test_dir(const std::string& dir) {
    int count = 0;
    DIR* dp = opendir(dir.c_str());
    if (!dp) {
        std::cerr << "[Loader] 无法打开目录: " << dir << std::endl;
        return 0;
    }
    struct dirent* entry;
    while ((entry = readdir(dp)) != nullptr) {
        std::string name(entry->d_name);
        if (name.size() > 5 && name.substr(name.size() - 5) == ".json") {
            std::string full_path = dir + "/" + name;
            if (load_test_case(full_path)) count++;
        }
    }
    closedir(dp);
    return count;
}

std::vector<TestResult> TestCaseRunner::execute_all() {
    std::vector<TestResult> results;
    for (size_t i = 0; i < test_cases_.size(); i++) {
        std::cout << "\n========== 执行测试用例 [" << (i+1) << "/"
                  << test_cases_.size() << "]: "
                  << test_cases_[i].name << " ==========" << std::endl;
        TestResult r = execute(test_cases_[i]);
        results.push_back(r);
        std::cout << ">> 结果: " << (r.passed ? "通过" : "失败")
                  << " (" << r.elapsed_ms << "ms)"
                  << (r.passed ? "" : " - " + r.fail_reason)
                  << std::endl;
    }
    return results;
}

TestResult TestCaseRunner::execute(const TestCase& tc) {
    TestResult result;
    result.case_name = tc.name;
    result.passed = false;

    auto start = std::chrono::steady_clock::now();

    // 发送请求
    if (!send_request(tc)) {
        result.fail_reason = "发送请求失败";
        auto end = std::chrono::steady_clock::now();
        result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        return result;
    }

    // 等待回报
    if (tc.response_type == TestCaseType::WaitHeartbeat) {
        // 心跳测试：等待指定时间，检查链接状态
        int wait_sec = tc.request_fields["wait_seconds"].as_int();
        std::cout << "[Runner] 等待 " << wait_sec << " 秒验证心跳..." << std::endl;
        for (int i = 0; i < wait_sec; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!handler_->last_link_status()) {
                result.fail_reason = "心跳中断 (链接断开)";
                auto end = std::chrono::steady_clock::now();
                result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                return result;
            }
        }
        result.passed = true;
        result.match_details.push_back("心跳维持正常");
    } else {
        if (!handler_->wait_for_response(tc.timeout_ms)) {
            result.fail_reason = "等待回报超时 (" + std::to_string(tc.timeout_ms) + "ms)";
            auto end = std::chrono::steady_clock::now();
            result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            return result;
        }
        // 验证回报
        if (!validate_response(tc, result.match_details)) {
            result.fail_reason = "字段验证失败";
        } else {
            result.passed = true;
        }
    }

    auto end = std::chrono::steady_clock::now();
    result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;
}

bool TestCaseRunner::send_request(const TestCase& tc) {
    handler_->reset();

    try {
        switch (tc.request_type) {
            case TestCaseType::Login: {
                lb_api::LoginReq req;
                std::memset(&req, 0, sizeof(req));
                req.client_req_no = tc.request_fields["client_req_no"].as_int();
                std::string val;

                val = tc.request_fields["fund_account_id"].as_string();
                std::memcpy(req.fund_account_id.data(), val.c_str(),
                            std::min(val.size(), req.fund_account_id.size()));

                val = tc.request_fields["branch_id"].as_string();
                std::memcpy(req.branch_id.data(), val.c_str(),
                            std::min(val.size(), req.branch_id.size()));

                val = tc.request_fields["account_id"].as_string();
                std::memcpy(req.account_id.data(), val.c_str(),
                            std::min(val.size(), req.account_id.size()));

                val = tc.request_fields["cust_id"].as_string();
                std::memcpy(req.cust_id.data(), val.c_str(),
                            std::min(val.size(), req.cust_id.size()));

                val = tc.request_fields["password"].as_string();
                std::memcpy(req.password.data(), val.c_str(),
                            std::min(val.size(), req.password.size()));

                val = tc.request_fields["order_way_ext"].as_string();
                std::memcpy(req.order_way_ext.data(), val.c_str(),
                            std::min(val.size(), req.order_way_ext.size()));

                val = tc.request_fields["user_info"].as_string();
                std::memcpy(req.user_info.data(), val.c_str(),
                            std::min(val.size(), req.user_info.size()));

                int32_t ret = api_->login(req);
                std::cout << "[Runner] login() 返回: " << ret << std::endl;
                return ret == 0;
            }

            case TestCaseType::OrderInsert: {
                lb_api::OrderReq req;
                std::memset(&req, 0, sizeof(req));

                std::string val = tc.request_fields["fund_account_id"].as_string();
                std::memcpy(req.fund_account_id.data(), val.c_str(),
                            std::min(val.size(), req.fund_account_id.size()));

                val = tc.request_fields["branch_id"].as_string();
                std::memcpy(req.branch_id.data(), val.c_str(),
                            std::min(val.size(), req.branch_id.size()));

                std::string side_str = tc.request_fields["side"].as_string();
                req.side = side_str.empty() ? 0 : side_str[0];
                std::string order_type_str = tc.request_fields["order_type"].as_string();
                req.order_type = order_type_str.empty() ? 0 : order_type_str[0];
                req.policy_id = tc.request_fields["policy_id"].as_int();
                req.tgw_id = tc.request_fields["tgw_id"].as_int();

                val = tc.request_fields["security_id"].as_string();
                std::memcpy(req.security_id.data(), val.c_str(),
                            std::min(val.size(), req.security_id.size()));

                req.order_price = tc.request_fields["order_price"].as_int();
                req.order_qty = tc.request_fields["order_qty"].as_int();
                req.stop_price = tc.request_fields["stop_price"].as_int();
                req.client_seq_id = tc.request_fields["client_seq_id"].as_int();
                req.market_id = tc.request_fields["market_id"].as_int();

                int32_t ret = api_->order_insert(req);
                std::cout << "[Runner] order_insert() 返回: " << ret << std::endl;
                return ret == 0;
            }

            case TestCaseType::OrderCancel: {
                lb_api::CancelReq req;
                std::memset(&req, 0, sizeof(req));
                req.client_req_no = tc.request_fields["client_req_no"].as_int();

                std::string val = tc.request_fields["fund_account_id"].as_string();
                std::memcpy(req.fund_account_id.data(), val.c_str(),
                            std::min(val.size(), req.fund_account_id.size()));

                val = tc.request_fields["branch_id"].as_string();
                std::memcpy(req.branch_id.data(), val.c_str(),
                            std::min(val.size(), req.branch_id.size()));

                req.order_sys_no = tc.request_fields["order_sys_no"].as_int();
                req.client_seq_id = tc.request_fields["client_seq_id"].as_int();

                int32_t ret = api_->order_cancel(req);
                std::cout << "[Runner] order_cancel() 返回: " << ret << std::endl;
                return ret == 0;
            }

            case TestCaseType::WaitHeartbeat:
                return true; // 心跳测试不需要发送请求

            default:
                std::cerr << "[Runner] 不支持请求类型: " << (int)tc.request_type << std::endl;
                return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Runner] 发送请求异常: " << e.what() << std::endl;
        return false;
    }
}

bool TestCaseRunner::validate_response(const TestCase& tc, std::vector<std::string>& details) {
    if (!handler_->has_response()) {
        details.push_back("无回报数据");
        return false;
    }

    bool all_match = true;
    for (size_t i = 0; i < tc.expected_fields.size(); i++) {
        const FieldMatch& fm = tc.expected_fields[i];
        std::string detail;

        // 根据响应类型获取字段值
        std::string actual_value;
        bool field_found = false;

        switch (tc.response_type) {
            case TestCaseType::Login: {
                const auto& ans = handler_->last_login_ans();
                if (fm.field_name == "err_code") {
                    actual_value = std::to_string(ans.err_code);
                    field_found = true;
                } else if (fm.field_name == "market_type") {
                    actual_value = std::to_string(ans.market_type);
                    field_found = true;
                } else if (fm.field_name == "cust_id") {
                    actual_value = std::string(ans.cust_id.data());
                    field_found = true;
                } else if (fm.field_name == "fund_account_id") {
                    actual_value = std::string(ans.fund_account_id.data());
                    field_found = true;
                }
                break;
            }
            case TestCaseType::OrderInsert: {
                const auto& rtn = handler_->last_order_rtn();
                if (fm.field_name == "order_status") {
                    actual_value = std::to_string((int)rtn.order_status);
                    field_found = true;
                } else if (fm.field_name == "rtn_type") {
                    actual_value = std::to_string(rtn.rtn_type);
                    field_found = true;
                } else if (fm.field_name == "order_qty") {
                    actual_value = std::to_string(rtn.order_qty);
                    field_found = true;
                } else if (fm.field_name == "trade_qty") {
                    actual_value = std::to_string(rtn.trade_qty);
                    field_found = true;
                } else if (fm.field_name == "side") {
                    actual_value = std::string(1, rtn.side);
                    field_found = true;
                } else if (fm.field_name == "order_price") {
                    actual_value = std::to_string(rtn.order_price);
                    field_found = true;
                }
                break;
            }
            case TestCaseType::OrderCancel: {
                const auto& rsp = handler_->last_cancel_rsp();
                if (fm.field_name == "err_code") {
                    actual_value = std::to_string(rsp.err_code);
                    field_found = true;
                } else if (fm.field_name == "order_sys_no") {
                    actual_value = std::to_string(rsp.order_sys_no);
                    field_found = true;
                } else if (fm.field_name == "client_seq_id") {
                    actual_value = std::to_string(rsp.client_seq_id);
                    field_found = true;
                }
                break;
            }
            default:
                break;
        }

        if (!field_found) {
            detail = "字段 '" + fm.field_name + "' 未找到";
            all_match = false;
        } else {
            all_match = match_field(fm.field_name, fm.expected_value, actual_value, detail) && all_match;
        }
        details.push_back(detail);
    }

    return all_match;
}

bool TestCaseRunner::match_field(const std::string& field_name, const JsonValue& expected,
                                  const std::string& actual_value, std::string& detail) {
    std::string expected_str;
    if (expected.is_int()) {
        expected_str = std::to_string(expected.as_int());
    } else if (expected.is_string()) {
        expected_str = expected.as_string();
    } else if (expected.is_bool()) {
        expected_str = expected.as_bool() ? "true" : "false";
    } else {
        expected_str = expected.to_string();
    }

    bool match = (actual_value == expected_str);
    detail = "字段 '" + field_name + "': 预期=" + expected_str + ", 实际=" + actual_value
             + (match ? " ✓" : " ✗");
    return match;
}

} // namespace mock

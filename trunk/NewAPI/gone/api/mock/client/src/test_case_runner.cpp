#include "test_case_runner.h"
#include "api_interface.h"
#include "api_config.h"
#include "order_trade_type.h"
#include "api_event_msg.h"  // LINK_TYPE_SPEED_TRADE
#include "logger.h"
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
    if (type_str == "etf_order_insert") return TestCaseType::EtfOrderInsert;
    if (type_str == "order_cancel" || type_str == "cancel_rsp") return TestCaseType::OrderCancel;
    if (type_str == "trade_rtn") return TestCaseType::TradeRtn;
    if (type_str == "wait_heartbeat" || type_str == "heartbeat_ok") return TestCaseType::WaitHeartbeat;
    return TestCaseType::Unknown;
}

bool TestCaseRunner::load_test_case(const std::string& path) {
    try {
        JsonValue root = JsonParser::parse_file(path);

        // 支持 test_cases 数组（多个用例顺序执行）
        JsonValue arr = root["test_cases"];
        if (arr.is_array() && arr.size() > 0) {
            for (size_t i = 0; i < arr.size(); i++) {
                if (!load_single_case(arr[i])) return false;
            }
            return true;
        }
        // 兼容单个 test_case
        return load_single_case(root);
    } catch (const std::exception& e) {
        LOG_ERROR("[Loader] 加载失败: " << path << " - " << e.what());
        return false;
    }
}

bool TestCaseRunner::load_single_case(const JsonValue& root) {
    try {
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

        // Unknown 类型：提示配置错误（Unknown 会跳过校验/发送，易被误认为通过）
        if (test_case.request_type == TestCaseType::Unknown) {
            LOG_WARN("[Loader] 用例 '" << test_case.name
                      << "' 的请求类型未知: '" << req["type"].as_string() << "'");
        }
        if (test_case.response_type == TestCaseType::Unknown) {
            LOG_WARN("[Loader] 用例 '" << test_case.name
                      << "' 的预期回报类型未知: '" << exp["type"].as_string() << "'");
        }

        // 解析预期字段（fields 对象的所有键；值为 null 表示动态字段跳过校验）
        JsonValue fields = exp["fields"];
        if (fields.is_object()) {
            std::vector<std::string> keys = fields.keys();
            for (size_t i = 0; i < keys.size(); i++) {
                FieldMatch fm;
                fm.field_name = keys[i];
                fm.expected_value = fields[keys[i]];
                fm.required = !fields[keys[i]].is_null();
                test_case.expected_fields.push_back(fm);
            }
        } else {
            LOG_WARN("[Loader] 用例 '" << test_case.name
                      << "' 缺少 expected_response.fields 对象，预期字段校验为空");
        }

        test_cases_.push_back(test_case);
        LOG_INFO("[Loader] 加载测试用例: " << test_case.name
                  << " (type=" << req["type"].as_string() << ")");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("[Loader] 加载失败: " << e.what());
        return false;
    }
}

int TestCaseRunner::load_test_dir(const std::string& dir) {
    int count = 0;
    DIR* dp = opendir(dir.c_str());
    if (!dp) {
        LOG_ERROR("[Loader] 无法打开目录: " << dir);
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

int TestCaseRunner::load_plan(const JsonValue& plan_node, const std::string& base_dir) {
    int count = 0;
    try {
        JsonValue ft = plan_node["functional_tests"];
        if (!ft.is_array()) {
            LOG_ERROR("[Loader] test_plan.functional_tests 缺失或非法, 跳过计划加载");
            return 0;
        }
        for (size_t i = 0; i < ft.size(); i++) {
            JsonValue item = ft[i];
            if (!item.is_object()) continue;

            // 场景开关：enabled=false 跳过
            if (item.has("enabled") && !item["enabled"].as_bool()) {
                std::string name = item.has("name") ? item["name"].as_string() : "unnamed";
                LOG_INFO("[Loader] 跳过场景(未启用): " << name);
                continue;
            }

            std::string name = item.has("name") ? item["name"].as_string() : ("scenario_" + std::to_string(i));
            std::string description = item.has("description") ? item["description"].as_string() : "";
            std::string req_file = item.has("request_file") ? item["request_file"].as_string() : "";
            std::string exp_file = item.has("expected_file") ? item["expected_file"].as_string() : "";
            int timeout_ms = item.has("timeout_ms") ? static_cast<int>(item["timeout_ms"].as_int()) : 5000;

            std::string req_path = req_file.empty() ? "" : (base_dir + "/" + req_file);
            std::string exp_path = exp_file.empty() ? "" : (base_dir + "/" + exp_file);

            TestCase test_case;
            test_case.name = name;
            test_case.description = description;
            test_case.counter_type = "plan";
            test_case.timeout_ms = timeout_ms;

            // 解析请求（独立文件，结构: { "type": ..., "fields": {...} }）
            if (!req_path.empty()) {
                JsonValue req_root = JsonParser::parse_file(req_path);
                JsonValue req = req_root["request"];
                if (req.is_null()) req = req_root;  // 兼容直接以 type/fields 为顶层
                test_case.request_type = parse_type(req["type"].as_string());
                test_case.request_fields = req["fields"];
                if (test_case.request_type == TestCaseType::Unknown) {
                    LOG_WARN("[Loader] 场景 '" << name
                              << "' 的请求类型未知: '" << req["type"].as_string() << "'");
                }
            } else {
                test_case.request_type = TestCaseType::None;
            }

            // 解析预期回报（独立文件，空文件/无字段时仅验证响应类型）
            if (!exp_path.empty()) {
                JsonValue exp_root = JsonParser::parse_file(exp_path);
                JsonValue exp = exp_root["expected_response"];
                if (exp.is_null()) exp = exp_root;
                test_case.response_type = parse_type(exp["type"].as_string());
                if (test_case.response_type == TestCaseType::Unknown) {
                    LOG_WARN("[Loader] 场景 '" << name
                              << "' 的预期回报类型未知: '" << exp["type"].as_string() << "'");
                }

                JsonValue fields = exp["fields"];
                if (fields.is_object()) {
                    std::vector<std::string> fkeys = fields.keys();
                    for (size_t j = 0; j < fkeys.size(); j++) {
                        FieldMatch fm;
                        fm.field_name = fkeys[j];
                        fm.expected_value = fields[fkeys[j]];
                        // null = 动态字段（流水号/时间）跳过校验；非 null 必须校验
                        fm.required = !fields[fkeys[j]].is_null();
                        test_case.expected_fields.push_back(fm);
                    }
                }
            } else {
                test_case.response_type = TestCaseType::None;
            }

            test_cases_.push_back(test_case);
            count++;
            LOG_INFO("[Loader] 加载计划场景: " << name
                      << " (req=" << (req_file.empty() ? "-" : req_file)
                      << ", exp=" << (exp_file.empty() ? "-" : exp_file) << ")");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("[Loader] load_plan 异常: " << e.what());
    }
    return count;
}

std::vector<TestResult> TestCaseRunner::execute_all() {
    std::vector<TestResult> results;
    for (size_t i = 0; i < test_cases_.size(); i++) {
        LOG_INFO("========== 执行测试用例 [" << (i+1) << "/"
                  << test_cases_.size() << "]: "
                  << test_cases_[i].name << " ==========");
        TestResult r = execute(test_cases_[i]);
        results.push_back(r);
        LOG_INFO(">> 结果: " << (r.passed ? "通过" : "失败")
                  << " (" << r.elapsed_ms << "ms)"
                  << (r.passed ? "" : " - " + r.fail_reason));
    }
    return results;
}

TestResult TestCaseRunner::execute(const TestCase& tc) {
    TestResult result;
    result.case_name = tc.name;
    result.passed = false;

    auto start = std::chrono::steady_clock::now();
    // 统一超时：用例未指定时默认 5000ms
    int64_t timeout_ms = tc.timeout_ms > 0 ? tc.timeout_ms : 5000;

    // 发送请求
    if (tc.response_type == TestCaseType::TradeRtn) {
        // 成交回报(2005)是异步回报：无需发送新请求，等待并校验已存储的成交回报
        LOG_INFO("[Runner] 等待并校验异步成交回报(2005)...");
        auto wait_start = std::chrono::steady_clock::now();
        while (!handler_->has_trade_rtn()) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - wait_start).count() > timeout_ms) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        if (handler_->has_trade_rtn()) {
            if (!validate_response(tc, result.match_details)) {
                result.fail_reason = "字段验证失败";
            } else {
                result.passed = true;
            }
        } else {
            result.fail_reason = "未收到成交回报(2005)";
        }
        auto end = std::chrono::steady_clock::now();
        result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        return result;
    }

    // 委托/撤单需走业务链接(SPEED_TRADE)。GOne 的 Core 链接在登录应答后异步建立，
    // 发送前等待业务链接就绪，避免 trade_link_connect==0 导致发送失败。
    if (tc.request_type == TestCaseType::OrderInsert ||
        tc.request_type == TestCaseType::EtfOrderInsert ||
        tc.request_type == TestCaseType::OrderCancel) {
        if (!wait_trade_link_ready(timeout_ms)) {
            result.fail_reason = "业务链接未就绪";
            auto end = std::chrono::steady_clock::now();
            result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            return result;
        }
    }

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
        LOG_INFO("[Runner] 等待 " << wait_sec << " 秒验证心跳...");
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
        if (tc.response_type == TestCaseType::OrderCancel) {
            // 撤单应答可能被中间的其他回报(2003)干扰，需等待 cancel_rsp 特定响应
            auto wait_start = std::chrono::steady_clock::now();
            while (!handler_->has_cancel_rsp()) {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - wait_start).count() > timeout_ms) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            if (!handler_->has_cancel_rsp()) {
                result.fail_reason = "未收到撤单应答";
                auto end = std::chrono::steady_clock::now();
                result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                return result;
            }
        } else if (!handler_->wait_for_response(timeout_ms)) {
            result.fail_reason = "等待回报超时 (" + std::to_string(timeout_ms) + "ms)";
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

// wait_trade_link_ready: 等待业务链接(lb_api::LINK_TYPE_SPEED_TRADE)就绪
// 委托/撤单走业务链接。GOne(fpga_direct) 的 Core 链接在 GW 登录应答后异步建立，
// 若在建立前发送委托会因 trade_link_connect==0 返回 LINK_DISCONNECTED(-22)。
// 轮询 handler_->trade_link_ready() 直到就绪或超时。
bool TestCaseRunner::wait_trade_link_ready(int timeout_ms) {
    if (handler_->trade_link_ready()) {
        return true;
    }
    LOG_INFO("[Runner] 等待业务链接(SPEED_TRADE)就绪, 超时 " << timeout_ms << "ms...");
    auto start = std::chrono::steady_clock::now();
    while (!handler_->trade_link_ready()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count() > timeout_ms) {
            LOG_WARN("[Runner] 等待业务链接就绪超时 (" << timeout_ms << "ms)");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    LOG_INFO("[Runner] 业务链接已就绪");
    return true;
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
                LOG_DEBUG("login fund_account_id val=[" << val << "] len=" << val.size()
                          << " req.size=" << req.fund_account_id.size());
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
                LOG_INFO("[Runner] login() 返回: " << ret);
                return ret == 0;
            }

            case TestCaseType::OrderInsert:
            case TestCaseType::EtfOrderInsert: {
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
                req.market_type = tc.request_fields["market_type"].as_int();

                // 普通委托走 order_insert，ETF 委托走 etf_order_insert
                bool is_etf = (tc.request_type == TestCaseType::EtfOrderInsert);
                int32_t ret = is_etf ? api_->etf_order_insert(req) : api_->order_insert(req);
                LOG_INFO("[Runner] " << (is_etf ? "etf_order_insert" : "order_insert")
                          << "() 返回: " << ret);
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

                // order_sys_no 支持 "$last_order_sys_no" 动态引用上一笔委托回报的 order_sys_no
                JsonValue osn = tc.request_fields["order_sys_no"];
                int64_t order_sys_no = 0;
                if (osn.is_string() && osn.as_string() == "$last_order_sys_no") {
                    order_sys_no = handler_->last_order_rtn().order_sys_no;
                } else {
                    order_sys_no = osn.as_int();
                }
                req.order_sys_no = order_sys_no;
                req.client_seq_id = tc.request_fields["client_seq_id"].as_int();

                int32_t ret = api_->order_cancel(req);
                LOG_INFO("[Runner] order_cancel() 返回: " << ret);
                return ret == 0;
            }

            case TestCaseType::WaitHeartbeat:
                return true; // 心跳测试不需要发送请求

            default:
                LOG_ERROR("[Runner] 不支持请求类型: " << (int)tc.request_type);
                return false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("[Runner] 发送请求异常: " << e.what());
        return false;
    }
}

bool TestCaseRunner::validate_response(const TestCase& tc, std::vector<std::string>& details) {
    if (!handler_->has_response()) {
        details.push_back("无回报数据");
        return false;
    }

    // 提取回报全部字段
    std::map<std::string, std::string> actual;
    extract_response_fields(tc.response_type, actual);

    bool all_match = true;
    int checked = 0;
    int skipped = 0;
    for (size_t i = 0; i < tc.expected_fields.size(); i++) {
        const FieldMatch& fm = tc.expected_fields[i];
        std::string detail;

        // 值为 null 的动态字段：跳过校验
        if (fm.expected_value.is_null()) {
            skipped++;
            continue;
        }
        checked++;

        auto it = actual.find(fm.field_name);
        if (it == actual.end()) {
            detail = "字段 '" + fm.field_name + "' 未找到";
            all_match = false;
        } else {
            all_match = match_field(fm.field_name, fm.expected_value, it->second, detail) && all_match;
        }
        details.push_back(detail);
    }
    details.push_back("[Summary] 校验字段数=" + std::to_string(checked)
                      + ", 跳过动态字段数=" + std::to_string(skipped));
    return all_match;
}

std::string TestCaseRunner::trim_fixed(const char* data, size_t len) {
    std::string s(data, len);
    // 去掉末尾的空格和 \0 填充（用 std::string 构造集合，避免 C 字符串截断）
    size_t e = s.find_last_not_of(std::string(" \0", 2));
    if (e == std::string::npos) return "";
    return s.substr(0, e + 1);
}

void TestCaseRunner::extract_response_fields(TestCaseType type, std::map<std::string, std::string>& out) {
    switch (type) {
    case TestCaseType::Login: {
        const auto& ans = handler_->last_login_ans();
        out["client_req_no"] = std::to_string(ans.client_req_no);
        out["cust_id"] = trim_fixed(ans.cust_id.data(), ans.cust_id.size());
        out["fund_account_id"] = trim_fixed(ans.fund_account_id.data(), ans.fund_account_id.size());
        out["account_id"] = trim_fixed(ans.account_id.data(), ans.account_id.size());
        out["branch_id"] = trim_fixed(ans.branch_id.data(), ans.branch_id.size());
        out["market_type"] = std::to_string(ans.market_type);
        out["err_code"] = std::to_string(ans.err_code);
        out["err_msg"] = trim_fixed(ans.err_msg.data(), ans.err_msg.size());
        out["login_time"] = std::to_string(ans.login_time);
        break;
    }
    case TestCaseType::OrderInsert:
    case TestCaseType::EtfOrderInsert: {
        const auto& rtn = handler_->last_order_rtn();
        out["cust_id"] = trim_fixed(rtn.cust_id.data(), rtn.cust_id.size());
        out["fund_account_id"] = trim_fixed(rtn.fund_account_id.data(), rtn.fund_account_id.size());
        out["account_id"] = trim_fixed(rtn.account_id.data(), rtn.account_id.size());
        out["branch_id"] = trim_fixed(rtn.branch_id.data(), rtn.branch_id.size());
        out["side"] = std::string(1, rtn.side);
        out["order_type"] = std::string(1, rtn.order_type);
        out["order_status"] = std::to_string((int)rtn.order_status);
        out["policy_id"] = std::to_string(rtn.policy_id);
        out["market_type"] = std::to_string(rtn.market_type);
        out["reserved"] = std::to_string(rtn.reserved);
        out["security_id"] = trim_fixed(rtn.security_id.data(), rtn.security_id.size());
        out["order_price"] = std::to_string(rtn.order_price);
        out["order_qty"] = std::to_string(rtn.order_qty);
        out["client_seq_id"] = std::to_string(rtn.client_seq_id);
        out["rtn_type"] = std::to_string(rtn.rtn_type);
        out["err_code"] = std::to_string(rtn.err_code);
        out["order_sys_no"] = std::to_string(rtn.order_sys_no);
        out["frozen_amount"] = std::to_string(rtn.frozen_amount);
        out["fee"] = std::to_string(rtn.fee);
        out["trade_qty"] = std::to_string(rtn.trade_qty);
        out["cancel_qty"] = std::to_string(rtn.cancel_qty);
        out["order_time"] = std::to_string(rtn.order_time);
        out["update_time"] = std::to_string(rtn.update_time);
        break;
    }
    case TestCaseType::OrderCancel: {
        const auto& rsp = handler_->last_cancel_rsp();
        out["client_req_no"] = std::to_string(rsp.client_req_no);
        out["cust_id"] = trim_fixed(rsp.cust_id.data(), rsp.cust_id.size());
        out["fund_account_id"] = trim_fixed(rsp.fund_account_id.data(), rsp.fund_account_id.size());
        out["account_id"] = trim_fixed(rsp.account_id.data(), rsp.account_id.size());
        out["branch_id"] = trim_fixed(rsp.branch_id.data(), rsp.branch_id.size());
        out["market_type"] = std::to_string(rsp.market_type);
        out["order_sys_no"] = std::to_string(rsp.order_sys_no);
        out["client_seq_id"] = std::to_string(rsp.client_seq_id);
        out["err_code"] = std::to_string(rsp.err_code);
        out["rej_api"] = std::to_string(rsp.rej_api);
        break;
    }
    case TestCaseType::TradeRtn: {
        const auto& rtn = handler_->last_trade_rtn();
        out["cust_id"] = trim_fixed(rtn.cust_id.data(), rtn.cust_id.size());
        out["fund_account_id"] = trim_fixed(rtn.fund_account_id.data(), rtn.fund_account_id.size());
        out["account_id"] = trim_fixed(rtn.account_id.data(), rtn.account_id.size());
        out["branch_id"] = trim_fixed(rtn.branch_id.data(), rtn.branch_id.size());
        out["side"] = std::string(1, rtn.side);
        out["order_type"] = std::string(1, rtn.order_type);
        out["order_status"] = std::to_string((int)rtn.order_status);
        out["policy_id"] = std::to_string(rtn.policy_id);
        out["market_type"] = std::to_string(rtn.market_type);
        out["reserved"] = std::to_string(rtn.reserved);
        out["security_id"] = trim_fixed(rtn.security_id.data(), rtn.security_id.size());
        out["order_price"] = std::to_string(rtn.order_price);
        out["order_qty"] = std::to_string(rtn.order_qty);
        out["client_seq_id"] = std::to_string(rtn.client_seq_id);
        out["order_sys_no"] = std::to_string(rtn.order_sys_no);
        out["frozen_amount"] = std::to_string(rtn.frozen_amount);
        out["fee"] = std::to_string(rtn.fee);
        out["trade_qty"] = std::to_string(rtn.trade_qty);
        out["cancel_qty"] = std::to_string(rtn.cancel_qty);
        out["order_time"] = std::to_string(rtn.order_time);
        // 成交特有字段
        out["exec_time"] = std::to_string(rtn.exec_time);
        out["exec_id"] = trim_fixed(rtn.exec_id.data(), rtn.exec_id.size());
        out["exec_price"] = std::to_string(rtn.exec_price);
        out["exec_qty"] = std::to_string(rtn.exec_qty);
        out["exec_amount"] = std::to_string(rtn.exec_amount);
        out["exec_fee"] = std::to_string(rtn.exec_fee);
        break;
    }
    default:
        break;
    }
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

#ifndef MOCK_CLIENT_TEST_CASE_RUNNER_H
#define MOCK_CLIENT_TEST_CASE_RUNNER_H

#include "json_utils.h"
#include "callback_handler.h"
#include <string>
#include <vector>
#include <map>

// 前向声明
namespace lb_api {
class api_interface;
struct LoginReq;
struct OrderReq;
struct CancelReq;
}

namespace mock {

/// 测试用例类型
enum class TestCaseType {
    Login,
    OrderInsert,
    EtfOrderInsert,
    OrderCancel,
    TradeRtn,
    WaitHeartbeat,
    Unknown
};

/// 字段匹配规则
struct FieldMatch {
    std::string field_name;
    JsonValue expected_value;
    bool required;
};

/// 测试用例定义
struct TestCase {
    std::string name;
    std::string description;
    std::string counter_type;
    int timeout_ms;

    TestCaseType request_type;
    JsonValue request_fields;

    TestCaseType response_type;
    std::vector<FieldMatch> expected_fields;
};

/// 测试结果
struct TestResult {
    std::string case_name;
    bool passed;
    std::string fail_reason;
    int64_t elapsed_ms;
    std::vector<std::string> match_details;
};

/// 测试用例执行器
class TestCaseRunner {
public:
    TestCaseRunner(lb_api::api_interface* api, CallbackHandler* handler);

    /// 加载单个测试用例 JSON 文件
    bool load_test_case(const std::string& path);

    /// 从 JSON 节点解析单个测试用例
    bool load_single_case(const JsonValue& root);

    /// 加载目录下所有测试用例 JSON 文件
    int load_test_dir(const std::string& dir);

    /// 执行所有已加载的测试用例
    std::vector<TestResult> execute_all();

    /// 获取测试用例数量
    size_t count() const { return test_cases_.size(); }

private:
    /// 解析测试用例类型
    TestCaseType parse_type(const std::string& type_str);

    /// 执行单个测试用例
    TestResult execute(const TestCase& tc);

    /// 构造请求并发送
    bool send_request(const TestCase& tc);

    /// 验证回报
    bool validate_response(const TestCase& tc, std::vector<std::string>& details);

    /// 字段比对
    bool match_field(const std::string& field_name, const JsonValue& expected,
                     const std::string& actual_value, std::string& detail);

    /// 提取回报所有字段到 map（用于逐字段比对）
    void extract_response_fields(TestCaseType type, std::map<std::string, std::string>& out);

    /// 裁剪定长 char 数组的空格和 \0 填充
    std::string trim_fixed(const char* data, size_t len);

    lb_api::api_interface* api_;
    CallbackHandler* handler_;
    std::vector<TestCase> test_cases_;
};

} // namespace mock

#endif // MOCK_CLIENT_TEST_CASE_RUNNER_H

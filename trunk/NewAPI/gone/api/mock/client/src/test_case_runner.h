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
    None,       // 无请求（如仅等待异步回报）或无预期字段
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

    /// 从主测试计划加载用例（场景列表，每个场景引用独立的 request_file / expected_file）
    /// @param plan_node test_plan 节点（含 functional_tests 数组：
    ///                  [{name, enabled, timeout_ms, request_file, expected_file}...]）
    /// @param base_dir  相对路径基准目录（通常为主配置文件所在目录）
    /// @return 成功加载的用例数
    int load_plan(const JsonValue& plan_node, const std::string& base_dir);

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

    /// 等待业务链接（LINK_TYPE_SPEED_TRADE）就绪
    /// 用于委托/撤单测试：GOne 的 Core 链接在登录应答后异步建立，
    /// 发送业务请求前需确保业务链接已就绪（避免 trade_link_connect==0 发送失败）
    bool wait_trade_link_ready(int timeout_ms);

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

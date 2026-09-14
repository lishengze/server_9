#ifndef MOCK_CLIENT_H
#define MOCK_CLIENT_H

#include "json_utils.h"
#include "test_case_runner.h"
#include "test_report.h"
#include "callback_handler.h"
#include <string>

namespace lb_api {
class api_interface;
}

namespace mock {

/// mock_client 主类
class MockClient {
public:
    MockClient();
    ~MockClient();

    /// 加载 API 动态库（直接链接 liblbapi.so）
    bool load_api(const std::string& lib_path);

    /// 初始化（加载连接配置，创建 API 实例）
    bool init(const std::string& config_path);

    /// 运行单个测试用例
    TestResult run_test(const std::string& testcase_path);

    /// 运行目录下所有测试用例
    std::vector<TestResult> run_all_tests(const std::string& test_dir);

    /// 获取测试报告
    const TestReport& report() const { return report_; }

    /// 关闭并释放资源
    void shutdown();

private:
    lb_api::api_interface* api_;
    CallbackHandler* callback_;
    TestCaseRunner* runner_;
    TestReport report_;
};

} // namespace mock

#endif // MOCK_CLIENT_H

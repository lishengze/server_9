// mock_client.h - MockClient 主类声明
//
// 职责：作为 mock_client 测试程序的顶层门面，封装被测 API（liblbapi.so）的
//   加载、初始化、测试执行与资源释放，并向外部暴露测试报告。
//   内部协调：CallbackHandler（接收回报）、TestCaseRunner（执行用例）、
//   TestReport（汇总结果）。

#ifndef MOCK_CLIENT_H
#define MOCK_CLIENT_H

#include "json_utils.h"
#include "test_case_runner.h"
#include "test_report.h"
#include "callback_handler.h"
#include "perf_runner.h"
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

    /// 运行性能测试（配置开启时）
    /// @param perf_node  connection_config.json 中的 "perf_test" 配置节点
    /// @return 是否成功执行
    bool run_perf_test(const JsonValue& perf_node);

    /// 获取测试报告
    const TestReport& report() const { return report_; }

    /// 关闭并释放资源
    void shutdown();

private:
    lb_api::api_interface* api_;
    CallbackHandler* callback_;
    TestCaseRunner* runner_;
    PerfRunner* perf_runner_;
    TestReport report_;
};

} // namespace mock

#endif // MOCK_CLIENT_H

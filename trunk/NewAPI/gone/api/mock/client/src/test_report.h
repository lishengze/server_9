#ifndef MOCK_CLIENT_TEST_REPORT_H
#define MOCK_CLIENT_TEST_REPORT_H

#include "test_case_runner.h"
#include <string>
#include <vector>

namespace mock {

/// 测试报告
class TestReport {
public:
    TestReport();

    /// 添加测试结果
    void add_result(const TestResult& result);

    /// 打印报告到控制台
    void print() const;

    /// 保存报告到文件
    bool save(const std::string& path) const;

    /// 获取统计
    int total() const { return total_; }
    int passed() const { return passed_; }
    int failed() const { return failed_; }

private:
    std::vector<TestResult> results_;
    int total_;
    int passed_;
    int failed_;
};

} // namespace mock

#endif // MOCK_CLIENT_TEST_REPORT_H

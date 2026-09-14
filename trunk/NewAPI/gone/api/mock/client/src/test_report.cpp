#include "test_report.h"
#include <iostream>
#include <fstream>

namespace mock {

TestReport::TestReport()
    : total_(0), passed_(0), failed_(0)
{
}

void TestReport::add_result(const TestResult& result) {
    results_.push_back(result);
    total_++;
    if (result.passed) passed_++;
    else failed_++;
}

void TestReport::print() const {
    std::cout << "\n========================================\n";
    std::cout << "          测试报告\n";
    std::cout << "========================================\n";
    std::cout << "总计: " << total_ << " | 通过: " << passed_
              << " | 失败: " << failed_ << std::endl;
    std::cout << "----------------------------------------\n";

    for (size_t i = 0; i < results_.size(); i++) {
        const TestResult& r = results_[i];
        std::cout << (r.passed ? "[PASS] " : "[FAIL] ")
                  << r.case_name
                  << " (" << r.elapsed_ms << "ms)" << std::endl;
        if (!r.passed) {
            std::cout << "       原因: " << r.fail_reason << std::endl;
        }
        for (size_t j = 0; j < r.match_details.size(); j++) {
            std::cout << "       " << r.match_details[j] << std::endl;
        }
    }
    std::cout << "========================================\n";
}

bool TestReport::save(const std::string& path) const {
    std::ofstream file(path.c_str());
    if (!file.is_open()) {
        std::cerr << "无法写入报告文件: " << path << std::endl;
        return false;
    }

    file << "测试报告\n";
    file << "总计: " << total_ << " 通过: " << passed_ << " 失败: " << failed_ << "\n\n";
    for (size_t i = 0; i < results_.size(); i++) {
        const TestResult& r = results_[i];
        file << (r.passed ? "[PASS] " : "[FAIL] ")
             << r.case_name << " (" << r.elapsed_ms << "ms)\n";
        if (!r.passed) {
            file << "  原因: " << r.fail_reason << "\n";
        }
        for (size_t j = 0; j < r.match_details.size(); j++) {
            file << "  " << r.match_details[j] << "\n";
        }
    }
    file.close();
    return true;
}

} // namespace mock

// test_report.cpp - TestReport 测试报告实现
//
// 职责：收集测试结果（TestResult），提供控制台打印和文件保存功能。
//   报告格式包含：总计/通过/失败统计、每个用例的 PASS/FAIL 状态、
//   耗时和字段校验详情。

#include "test_report.h"
#include "logger.h"
#include <iostream>
#include <fstream>

namespace mock {

TestReport::TestReport()
    : total_(0), passed_(0), failed_(0)
{
}

// add_result: 添加单个测试结果，更新统计计数
void TestReport::add_result(const TestResult& result) {
    results_.push_back(result);
    total_++;
    if (result.passed) passed_++;
    else failed_++;
}

// print: 打印汇总报告到控制台
// 输出统计行，随后逐个用例打印 PASS/FAIL、耗时、失败原因和字段校验详情。
void TestReport::print() const {
    LOG_INFO("");
    LOG_INFO("========================================");
    LOG_INFO("          测试报告");
    LOG_INFO("========================================");
    LOG_INFO("总计: " << total_ << " | 通过: " << passed_
              << " | 失败: " << failed_);
    LOG_INFO("----------------------------------------");

    for (size_t i = 0; i < results_.size(); i++) {
        const TestResult& r = results_[i];
        LOG_INFO((r.passed ? "[PASS] " : "[FAIL] ")
                  << r.case_name
                  << " (" << r.elapsed_ms << "ms)");
        if (!r.passed) {
            LOG_INFO("       原因: " << r.fail_reason);
        }
        for (size_t j = 0; j < r.match_details.size(); j++) {
            LOG_INFO("       " << r.match_details[j]);
        }
    }
    LOG_INFO("========================================");
}

// save: 将详细报告写入文件
// 与 print() 内容一致，写入指定路径；打开失败时打印错误并返回 false。
bool TestReport::save(const std::string& path) const {
    std::ofstream file(path.c_str());
    if (!file.is_open()) {
        LOG_ERROR("无法写入报告文件: " << path);
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

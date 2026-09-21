// result_analysis.cpp - 测试结果分析实现

#include "result_analysis.h"
#include "logger.h"
#include <fstream>
#include <ctime>
#include <chrono>
#include <cstdio>
#include <sstream>

namespace mock {

bool ResultAnalysis::write(const std::string& path,
                           const TestReport& report,
                           const std::string& perf_report,
                           long long perf_failed,
                           const std::string& plan_desc) {
    std::ofstream ofs(path.c_str());
    if (!ofs.is_open()) {
        LOG_ERROR("无法写入结果分析文件: " << path);
        return false;
    }

    // 时间戳
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmv;
    ::localtime_r(&tt, &tmv);
    char ts[32];
    std::snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d",
                  tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                  tmv.tm_hour, tmv.tm_min, tmv.tm_sec);

    int total = report.total();
    int passed = report.passed();
    int failed = report.failed();
    double rate = total > 0 ? (100.0 * passed / total) : 0.0;

    ofs << "========================================\n";
    ofs << "        mock_client 测试结果分析\n";
    ofs << "========================================\n";
    ofs << "生成时间   : " << ts << "\n";
    if (!plan_desc.empty()) {
        ofs << "测试计划   : " << plan_desc << "\n";
    }
    ofs << "\n";

    // 一、功能测试分析
    ofs << "【一、功能测试分析】\n";
    ofs << "----------------------------------------\n";
    ofs << "总计: " << total << " | 通过: " << passed << " | 失败: " << failed
        << " | 通过率: " << rate << "%\n";
    ofs << "----------------------------------------\n";
    const std::vector<TestResult>& results = report.results();
    for (size_t i = 0; i < results.size(); i++) {
        const TestResult& r = results[i];
        ofs << (r.passed ? "[PASS] " : "[FAIL] ")
            << r.case_name << " (" << r.elapsed_ms << "ms)\n";
        if (!r.passed) {
            ofs << "        原因: " << r.fail_reason << "\n";
        }
        for (size_t j = 0; j < r.match_details.size(); j++) {
            ofs << "        " << r.match_details[j] << "\n";
        }
    }
    ofs << "----------------------------------------\n";
    ofs << "\n";

    // 二、性能测试分析
    ofs << "【二、性能测试分析】\n";
    ofs << "----------------------------------------\n";
    if (perf_report.empty()) {
        ofs << "未执行性能测试（perf_test.enable=false 或未配置）\n";
    } else {
        // 去掉首尾多余空行，保持缩进
        ofs << perf_report;
        if (perf_report[perf_report.size() - 1] != '\n') {
            ofs << "\n";
        }
    }
    ofs << "----------------------------------------\n";
    ofs << "\n";

    // 三、总体结论
    ofs << "【三、总体结论】\n";
    ofs << "----------------------------------------\n";
    bool func_ok = (failed == 0);
    bool perf_ok = (perf_failed < 0) || (perf_failed == 0);
    ofs << "功能测试 : " << passed << "/" << total << " 通过"
        << (func_ok ? " ✓" : " ✗（存在失败用例）") << "\n";
    if (perf_failed >= 0) {
        ofs << "性能测试 : 失败 " << perf_failed << " 笔"
            << (perf_ok ? " ✓" : " ✗（存在失败请求）") << "\n";
    }
    ofs << "结论     : " << ((func_ok && perf_ok) ? "测试全部通过 ✓" : "存在失败，请检查分析") << "\n";
    ofs << "----------------------------------------\n";
    ofs << "========================================\n";

    ofs.close();
    LOG_INFO("测试结果分析已写入: " << path);
    return true;
}

} // namespace mock
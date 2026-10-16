// result_analysis.h - 测试结果分析
//
// 职责：将测试最终结果分析输出到独立的结果分析文件。
//   整合：功能测试报告（逐用例 PASS/FAIL + 字段校验详情）+
//         性能测试报告（若执行）+ 总体结论（通过率 / 失败判定）。
//   与运行日志（mock_client.log）和功能报告（test_report.txt）相互独立。

#ifndef MOCK_CLIENT_RESULT_ANALYSIS_H
#define MOCK_CLIENT_RESULT_ANALYSIS_H

#include "test_report.h"
#include <string>

namespace mock {

/// 测试结果分析
class ResultAnalysis {
public:
    /// 写入结果分析文件
    /// @param path        分析文件输出路径
    /// @param report      功能测试报告
    /// @param perf_report 性能测试报告文本（可为空，表示未执行）
    /// @param perf_failed 性能测试失败笔数（用于总体结论；未执行传 -1）
    /// @param plan_desc   测试计划/配置描述（可为空）
    /// @return 是否成功
    static bool write(const std::string& path,
                      const TestReport& report,
                      const std::string& perf_report,
                      long long perf_failed,
                      const std::string& plan_desc);
};

} // namespace mock

#endif // MOCK_CLIENT_RESULT_ANALYSIS_H
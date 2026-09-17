// metric_stats.h - 性能指标统计
//
// 职责：对收集到的耗时样本（纳秒）计算统计指标：
//   平均值 / P50 / P75 / P90 / P95 / 最大值 / 最小值 / 标准差
// 供性能测试程序输出到报告文件。

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace perf {

/// 性能统计结果
struct MetricStats {
  size_t count = 0;      ///< 样本数
  uint64_t total = 0;    ///< 总耗时（ns）
  double mean = 0.0;     ///< 平均值（ns）
  uint64_t p50 = 0;      ///< 50 分位（ns）
  uint64_t p75 = 0;      ///< 75 分位（ns）
  uint64_t p90 = 0;      ///< 90 分位（ns）
  uint64_t p95 = 0;      ///< 95 分位（ns）
  uint64_t max = 0;      ///< 最大值（ns）
  uint64_t min = 0;      ///< 最小值（ns）
  double stddev = 0.0;   ///< 标准差（ns）

  /// 基于样本计算全部指标（对样本做排序副本，不改原数据）
  void compute(const std::vector<uint64_t>& samples);

  /// 格式化为可读文本（含指标名与单位 ns）
  std::string to_string() const;
};

} // namespace perf
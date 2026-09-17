// metric_stats.cpp - 性能指标统计实现

#include "metric_stats.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace perf {

void MetricStats::compute(const std::vector<uint64_t>& samples) {
  count = samples.size();
  if (count == 0) {
    return;
  }

  // 排序副本（不改原数据）
  std::vector<uint64_t> s = samples;
  std::sort(s.begin(), s.end());

  min = s.front();
  max = s.back();

  total = 0;
  for (uint64_t v : s) {
    total += v;
  }
  mean = static_cast<double>(total) / static_cast<double>(count);

  // 分位数：线性插值（兼容样本数非 (p*(n-1)) 整数的情形）
  auto percentile = [&](double p) -> uint64_t {
    if (s.empty()) return 0;
    double idx = static_cast<double>(s.size() - 1) * p;
    size_t lo = static_cast<size_t>(idx);
    size_t hi = std::min(lo + 1, s.size() - 1);
    double frac = idx - static_cast<double>(lo);
    return static_cast<uint64_t>(static_cast<double>(s[lo]) * (1.0 - frac) +
                                 static_cast<double>(s[hi]) * frac);
  };
  p50 = percentile(0.50);
  p75 = percentile(0.75);
  p90 = percentile(0.90);
  p95 = percentile(0.95);

  // 样本标准差（n-1 无偏估计）
  double sum_sq = 0.0;
  for (uint64_t v : s) {
    double d = static_cast<double>(v) - mean;
    sum_sq += d * d;
  }
  stddev = (count > 1) ? std::sqrt(sum_sq / static_cast<double>(count - 1)) : 0.0;
}

std::string MetricStats::to_string() const {
  std::ostringstream oss;
  oss << "样本数    : " << count << "\n"
      << "总耗时    : " << total << " ns\n"
      << "平均值    : " << mean << " ns\n"
      << "P50 (50%) : " << p50 << " ns\n"
      << "P75 (75%) : " << p75 << " ns\n"
      << "P90 (90%) : " << p90 << " ns\n"
      << "P95 (95%) : " << p95 << " ns\n"
      << "最大值    : " << max << " ns\n"
      << "最小值    : " << min << " ns\n"
      << "标准差    : " << stddev << " ns\n";
  return oss.str();
}

} // namespace perf
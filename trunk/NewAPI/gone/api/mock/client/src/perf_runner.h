// perf_runner.h - 性能测试模块（mock_client 内置）
//
// 职责：在功能测试（登录、委托等）通过的基础上，对 FTE 柜台委托请求通路做
//   性能测试：匀速发单（间隔 = 1/TPS 秒）、收集每笔请求在 api 内的处理耗时
//   （api_leave_time_ns - api_arrive_time_ns）、计算统计指标并输出到报告文件。
//
// 使用方式：在 connection_config.json 的 "perf_test" 配置块中设置
//   enable=true 及测试参数，mock_client 在功能测试完成后自动执行性能测试。

#ifndef MOCK_CLIENT_PERF_RUNNER_H
#define MOCK_CLIENT_PERF_RUNNER_H

#include "json_utils.h"
#include "metric_stats.h"
#include "cpu_affinity.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace lb_api {
class api_interface;
struct OrderReq;
} // namespace lb_api

namespace mock {

/// 性能测试配置（解析自 connection_config.json 的 "perf_test" 块）
struct PerfConfig {
    bool enable = false;        ///< 是否开启性能测试
    int32_t duration_sec = 10;  ///< 测试时长（秒），默认 10
    int32_t tps = 100;          ///< 发单速率（笔/秒），匀速发单，间隔 = 1/TPS 秒
    int32_t warmup_sec = 3;     ///< 预热/绑核等待时间（秒），确保绑核生效后再发单
    int32_t cpu_id = -1;        ///< CPU 绑定目标，-1 表示不绑定
    std::string report_file = "perf_report.txt"; ///< 报告输出文件

    // 委托模板（用于构造性能测试的委托请求）
    std::string fund_account_id;
    std::string branch_id;
    std::string password;
    char side = '1';
    char order_type = '2';
    std::string security_id;
    int64_t order_price = 0;
    int64_t order_qty = 0;
    int64_t stop_price = 0;
    int32_t order_market_type = 1;

    /// 从 JSON 节点加载 perf_test 配置
    bool load(const JsonValue& node);
};

/// 性能测试执行器
class PerfRunner {
public:
    explicit PerfRunner(lb_api::api_interface* api);

    /// 加载性能测试配置
    bool load_config(const JsonValue& perf_node);

    /// 是否开启性能测试
    bool enabled() const { return cfg_.enable; }

    /// 执行性能测试（须在登录成功后调用，复用当前已登录的 api 实例）
    /// @return 是否成功完成
    bool run();

    /// 生成完整报告文本（含参数 + 指标）
    std::string report_text() const;

private:
    /// 构造一笔委托请求（使用配置模板 + 递增 client_seq_id）
    static void build_order(const PerfConfig& cfg, int64_t seq, lb_api::OrderReq& req);

    /// 匀速发单，收集每笔请求耗时样本
    void run_benchmark(std::vector<uint64_t>& latencies);

    lb_api::api_interface* api_;
    PerfConfig cfg_;
    perf::MetricStats stats_;   ///< 统计结果
    size_t sent_ = 0;           ///< 发送笔数
    size_t ok_ = 0;             ///< 成功笔数
    std::map<int32_t, size_t> fail_codes_; ///< 失败返回码统计
    double actual_tps_ = 0.0;   ///< 实际 TPS
    double test_time_sec_ = 0.0;///< 实际测试时长（秒）
    std::string cpu_bind_desc_; ///< CPU 绑定描述
};

} // namespace mock

#endif // MOCK_CLIENT_PERF_RUNNER_H
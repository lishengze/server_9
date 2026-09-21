// perf_runner.cpp - 性能测试模块实现
//
// 流程：
//   1. load_config() 从 connection_config.json 的 "perf_test" 块加载配置
//   2. run() 执行：绑核 → 预热 → 匀速发单 → 统计 → 输出报告
//   3. report_text() 生成完整报告文本

#include "perf_runner.h"
#include "api_interface.h"
#include "order_trade_type.h"
#include "logger.h"

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace mock {

// ==================== PerfConfig ====================

bool PerfConfig::load(const JsonValue& node) {
    try {
        enable = node["enable"].as_bool();
        duration_sec = static_cast<int32_t>(node["duration_sec"].as_int());
        tps = static_cast<int32_t>(node["tps"].as_int());
        warmup_sec = static_cast<int32_t>(node["warmup_sec"].as_int());
        cpu_id = static_cast<int32_t>(node["cpu_id"].as_int());
        report_file = node["report_file"].as_string();
        net_time_map_file = node["net_time_map_file"].as_string();

        // 委托模板（可选，缺省使用默认值）
        JsonValue od = node["order"];
        if (!od.is_null()) {
            fund_account_id = od["fund_account_id"].as_string();
            branch_id = od["branch_id"].as_string();
            password = od["password"].as_string();

            std::string s = od["side"].as_string();
            side = s.empty() ? '1' : s[0];
            s = od["order_type"].as_string();
            order_type = s.empty() ? '2' : s[0];

            security_id = od["security_id"].as_string();
            order_price = od["order_price"].as_int();
            order_qty = od["order_qty"].as_int();
            stop_price = od["stop_price"].as_int();
            order_market_type = static_cast<int32_t>(od["market_type"].as_int());
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("[PerfConfig] 加载失败: " << e.what());
        return false;
    }
}

// ==================== PerfRunner ====================

PerfRunner::PerfRunner(lb_api::api_interface* api)
    : api_(api)
{
}

bool PerfRunner::load_config(const JsonValue& perf_node) {
    return cfg_.load(perf_node);
}

// 构造一笔委托请求
void PerfRunner::build_order(const PerfConfig& cfg, int64_t seq, lb_api::OrderReq& req) {
    std::memset(&req, 0, sizeof(req));
    std::memcpy(req.fund_account_id.data(), cfg.fund_account_id.c_str(),
                std::min(cfg.fund_account_id.size(), req.fund_account_id.size()));
    std::memcpy(req.branch_id.data(), cfg.branch_id.c_str(),
                std::min(cfg.branch_id.size(), req.branch_id.size()));
    req.side = cfg.side;
    req.order_type = cfg.order_type;
    std::memcpy(req.security_id.data(), cfg.security_id.c_str(),
                std::min(cfg.security_id.size(), req.security_id.size()));
    req.order_price = cfg.order_price;
    req.order_qty = cfg.order_qty;
    req.stop_price = cfg.stop_price;
    req.client_seq_id = seq;
    req.market_type = static_cast<uint16_t>(cfg.order_market_type);
}

// 匀速发单
void PerfRunner::run_benchmark(std::vector<uint64_t>& latencies) {
    if (cfg_.tps <= 0) {
        LOG_ERROR("[PerfRunner] TPS 必须 > 0");
        return;
    }

    const double interval_us = 1000000.0 / static_cast<double>(cfg_.tps);
    const auto interval = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::duration<double, std::micro>(interval_us));

    const auto end_time = std::chrono::steady_clock::now() +
                          std::chrono::seconds(cfg_.duration_sec);
    const auto start_time = std::chrono::steady_clock::now();

    int64_t seq = 1;
    sent_ = 0;
    ok_ = 0;
    fail_codes_.clear();
    net_time_map_.clear();
    net_time_map_.reserve(static_cast<size_t>(cfg_.tps) * static_cast<size_t>(cfg_.duration_sec));

    auto next_send = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() < end_time) {
        lb_api::OrderReq req;
        build_order(cfg_, seq++, req);

        int32_t ret = api_->order_insert(req);
        ++sent_;
        if (ret == 0) {
            ++ok_;
        } else {
            ++fail_codes_[ret];
        }

        // 计算 api 内耗时（纳秒）
        uint64_t lat = req.api_leave_time_ns - req.api_arrive_time_ns;
        latencies.push_back(lat);

        // 记录 (client_seq_id, api_arrive_time_ns) 映射，供网卡抓包程序关联分析
        net_time_map_.emplace_back(req.client_seq_id, req.api_arrive_time_ns);

        // 匀速等待
        next_send += interval;
        auto now = std::chrono::steady_clock::now();
        if (now < next_send) {
            std::this_thread::sleep_for(next_send - now);
        } else {
            next_send = now + interval;
        }
    }

    auto end = std::chrono::steady_clock::now();
    test_time_sec_ = std::chrono::duration<double>(end - start_time).count();
    actual_tps_ = static_cast<double>(sent_) / test_time_sec_;
}

bool PerfRunner::run() {
    if (!api_) {
        LOG_ERROR("[PerfRunner] API 实例为空");
        return false;
    }

    LOG_INFO("========== 性能测试开始 ==========");

    // 1. CPU 绑定
    if (cfg_.cpu_id >= 0) {
        if (!perf::cpu_affinity::bind_cpu(cfg_.cpu_id)) {
            LOG_ERROR("[PerfRunner] CPU 绑定失败: cpu_id=" << cfg_.cpu_id);
            cpu_bind_desc_ = "绑定失败";
        } else {
            cpu_bind_desc_ = "CPU " + std::to_string(cfg_.cpu_id);
            LOG_INFO("[PerfRunner] 已绑定到 " << cpu_bind_desc_);
        }
    } else {
        cpu_bind_desc_ = "不绑定";
    }

    // 查询当前 CPU 亲和性（日志确认）
    std::string cpu_aff = perf::cpu_affinity::get_current_cpu();
    LOG_INFO("[PerfRunner] 当前 CPU 亲和性: " << cpu_aff);

    // 2. 预热等待（确保绑核生效）
    if (cfg_.warmup_sec > 0) {
        LOG_INFO("[PerfRunner] 预热等待 " << cfg_.warmup_sec << " 秒...");
        std::this_thread::sleep_for(std::chrono::seconds(cfg_.warmup_sec));
    }

    // 3. 匀速发单
    LOG_INFO("[PerfRunner] 开始发单: duration=" << cfg_.duration_sec
              << "s, TPS=" << cfg_.tps);

    std::vector<uint64_t> latencies;
    run_benchmark(latencies);

    // 4. 统计
    stats_.compute(latencies);
    LOG_INFO("[PerfRunner] 发单完成: 发送=" << sent_ << " 成功=" << ok_
              << " 失败=" << (sent_ - ok_));

    // 5. 输出报告
    std::string report = report_text();
    LOG_INFO(report);

    if (!cfg_.report_file.empty()) {
        std::ofstream ofs(cfg_.report_file);
        if (ofs) {
            ofs << report;
            ofs.close();
            LOG_INFO("[PerfRunner] 报告已保存到: " << cfg_.report_file);
        } else {
            LOG_ERROR("[PerfRunner] 无法写入报告: " << cfg_.report_file);
        }
    }

    // 6. 写出网卡抓包关联映射文件（client_seq_id -> api_arrive_time_ns）
    if (!cfg_.net_time_map_file.empty()) {
        if (apinet::write_map_file(cfg_.net_time_map_file, net_time_map_)) {
            LOG_INFO("[PerfRunner] 网卡抓包映射已写出: " << cfg_.net_time_map_file
                      << " (" << net_time_map_.size() << " 条)");
        } else {
            LOG_ERROR("[PerfRunner] 无法写出网卡抓包映射: " << cfg_.net_time_map_file);
        }
    }

    return true;
}

std::string PerfRunner::report_text() const {
    std::ostringstream oss;
    oss << "\n========== " << cfg_.counter_name << " 委托通路性能测试报告 ==========\n"
        << "测试时间 : " << test_time_sec_ << " 秒\n"
        << "目标 TPS : " << cfg_.tps << "\n"
        << "实际 TPS : " << actual_tps_ << "\n"
        << "样本数   : " << stats_.count << "\n"
        << "CPU 绑定 : " << cpu_bind_desc_ << "\n"
        << "\n"
        << "------- API 内处理耗时（纳秒）-------\n"
        << stats_.to_string();

    // 失败统计
    if (!fail_codes_.empty()) {
        oss << "------- 失败返回码统计 -------\n";
        for (auto& kv : fail_codes_) {
            oss << "  返回码 " << kv.first << " : " << kv.second << " 次\n";
        }
    }

    oss << "============================================\n";
    return oss.str();
}

} // namespace mock
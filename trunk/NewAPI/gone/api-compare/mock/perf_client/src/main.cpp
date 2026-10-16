// main.cpp - FTE 委托通路性能测试主程序
//
// 用法：
//   perf_client --config perf_config.json [--duration 10] [--tps 1000] [--cpu 0] [--report out.txt]
//
// 流程：
//   1. 解析命令行参数（可选覆盖 JSON 配置）
//   2. CPU 绑定（sched_setaffinity，通用化）
//   3. warmup 等待（确保绑核生效）
//   4. 初始化 API + 登录 FTE
//   5. 匀速发单（间隔 = 1/TPS 秒），收集每笔 api 内耗时
//   6. 分析指标（均值/P50/P75/P90/最大/最小/标准差）
//   7. 输出报告到控制台 + 文件

#include "cpu_affinity.h"
#include "metric_stats.h"
#include "perf_client.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

// 简单命令行参数解析
static const char* get_arg(int argc, char* argv[], const char* key) {
  for (int i = 1; i < argc - 1; ++i) {
    if (std::strcmp(argv[i], key) == 0) {
      return argv[i + 1];
    }
  }
  return nullptr;
}

static bool has_flag(int argc, char* argv[], const char* flag) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], flag) == 0) {
      return true;
    }
  }
  return false;
}

static void print_usage(const char* prog) {
  std::cout << "用法: " << prog << " --config <path> [选项]\n"
            << "选项:\n"
            << "  --config <path>     JSON 配置文件路径（必填）\n"
            << "  --duration <sec>    测试时长（秒，默认 10）\n"
            << "  --tps <n>           每秒发单量（默认 1000）\n"
            << "  --cpu <id>          绑定的 CPU 编号（-1 不绑定）\n"
            << "  --warmup <sec>      启动后等待秒数（默认 3）\n"
            << "  --report <path>     报告输出文件\n"
            << "  --help              显示此帮助\n";
}

int main(int argc, char* argv[]) {
  if (has_flag(argc, argv, "--help")) {
    print_usage(argv[0]);
    return 0;
  }

  const char* config_path = get_arg(argc, argv, "--config");
  if (!config_path) {
    std::cerr << "[ERROR] 必须指定 --config" << std::endl;
    print_usage(argv[0]);
    return 1;
  }

  // 加载配置
  perf::PerfConfig cfg;
  if (!cfg.load(config_path)) {
    std::cerr << "[ERROR] 配置文件加载失败: " << config_path << std::endl;
    return 1;
  }

  // 命令行覆盖
  const char* cli_duration = get_arg(argc, argv, "--duration");
  if (cli_duration) cfg.test_duration_sec = std::atoi(cli_duration);
  const char* cli_tps = get_arg(argc, argv, "--tps");
  if (cli_tps) cfg.tps = std::atoi(cli_tps);
  const char* cli_cpu = get_arg(argc, argv, "--cpu");
  if (cli_cpu) cfg.cpu_id = std::atoi(cli_cpu);
  const char* cli_warmup = get_arg(argc, argv, "--warmup");
  if (cli_warmup) cfg.warmup_sec = std::atoi(cli_warmup);
  const char* cli_report = get_arg(argc, argv, "--report");
  if (cli_report) cfg.report_file = cli_report;

  // 打印测试参数
  std::cout << "\n========== 性能测试参数 ==========\n"
            << "配置文件 : " << config_path << "\n"
            << "测试时长 : " << cfg.test_duration_sec << " 秒\n"
            << "TPS      : " << cfg.tps << "\n"
            << "CPU 绑定 : " << (cfg.cpu_id >= 0 ? std::to_string(cfg.cpu_id) : "不绑定") << "\n"
            << "Warmup   : " << cfg.warmup_sec << " 秒\n"
            << "报告文件 : " << (cfg.report_file.empty() ? "无" : cfg.report_file) << "\n"
            << "==================================\n\n";

  // 1. CPU 绑定（通用化）
  if (cfg.cpu_id >= 0) {
    if (!perf::cpu_affinity::bind_cpu(cfg.cpu_id)) {
      std::cerr << "[WARN] CPU 绑定失败（可能需要 root 权限）" << std::endl;
    } else {
      std::string cpus = perf::cpu_affinity::get_current_cpu();
      std::cout << "[CPU] 已绑定到 CPU " << cfg.cpu_id << " 当前允许: " << cpus << std::endl;
    }
  }

  // 2. warmup 等待（确保绑核生效）
  if (cfg.warmup_sec > 0) {
    std::cout << "[Warmup] 等待 " << cfg.warmup_sec << " 秒（确保绑核生效）..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(cfg.warmup_sec));
  }

  // 3. 初始化 + 登录
  perf::PerfClient client;
  if (!client.init(config_path)) {
    std::cerr << "[ERROR] 初始化失败" << std::endl;
    return 1;
  }
  if (!client.login()) {
    std::cerr << "[ERROR] 登录失败" << std::endl;
    client.shutdown();
    return 1;
  }
  std::cout << "[OK] 登录成功，开始性能测试..." << std::endl;

  // 4. 匀速发单
  std::vector<uint64_t> latencies;
  latencies.reserve(static_cast<size_t>(cfg.test_duration_sec * cfg.tps) + 1024);

  auto test_start = std::chrono::steady_clock::now();
  perf::run_benchmark(client, cfg, latencies);
  auto test_end = std::chrono::steady_clock::now();
  double real_sec = std::chrono::duration<double>(test_end - test_start).count();

  // 5. 计算指标
  perf::MetricStats stats;
  stats.compute(latencies);

  // 6. 输出报告
  std::ostringstream report;
  report << "========== FTE 委托通路性能测试报告 ==========\n"
         << "测试时间 : " << real_sec << " 秒\n"
         << "目标 TPS : " << cfg.tps << "\n"
         << "实际 TPS : " << (latencies.size() / real_sec) << "\n"
         << "样本数   : " << latencies.size() << "\n"
         << "CPU 绑定 : " << (cfg.cpu_id >= 0 ? std::to_string(cfg.cpu_id) : "不绑定") << "\n\n"
         << "------- API 内处理耗时（纳秒）-------\n"
         << stats.to_string() << "\n";

  std::cout << report.str();

  // 保存到文件
  if (!cfg.report_file.empty()) {
    std::ofstream ofs(cfg.report_file.c_str());
    if (ofs) {
      ofs << report.str();
      ofs.close();
      std::cout << "[OK] 报告已保存到: " << cfg.report_file << std::endl;
    } else {
      std::cerr << "[WARN] 无法写入报告文件: " << cfg.report_file << std::endl;
    }
  }

  // 7. 清理
  client.shutdown();
  std::cout << "[OK] 性能测试完成" << std::endl;
  return 0;
}
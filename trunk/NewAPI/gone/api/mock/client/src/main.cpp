// main.cpp - mock_client 入口
//
// 职责：解析命令行参数，创建 MockClient 并协调测试执行流程。
//   支持两种模式：
//     模式一（推荐）：--plan <path>  测试计划主配置文件
//     模式二（兼容）：--config + --testdir/--testcase  旧模式
//
// 日志系统（Logger）在 main 入口初始化，所有组件通过 LOG_* 宏统一输出。
// 测试结果分析（ResultAnalysis）在测试完成后写入独立分析文件。

#include "mock_client.h"
#include "logger.h"
#include "json_utils.h"
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <cerrno>

/// 确保输出目录存在（不存在则递归创建），用于 result/ 等输出目录
void ensure_output_dir(const std::string& path) {
    if (path.empty()) return;
    // 取路径的目录部分（去掉最后一个 '/' 之后的内容）
    std::string dir = path;
    size_t slash = dir.find_last_of("/\\");
    if (slash == std::string::npos) return;  // 无目录部分，无需创建
    dir = dir.substr(0, slash);
    if (dir.empty() || dir == ".") return;
    if (::mkdir(dir.c_str(), 0755) == 0) {
        LOG_INFO("已创建输出目录: " << dir);
    } else if (errno != EEXIST) {
        LOG_WARN("创建输出目录失败: " << dir << " (errno=" << errno << ")");
    }
}

/// 打印帮助信息
void print_usage(const char* prog) {
    LOG_INFO("用法: " << prog << " [选项]");
    LOG_INFO("选项:");
    LOG_INFO("  --plan <path>        测试计划主配置文件路径 (推荐, 内容见 test_plan_*.json)");
    LOG_INFO("  --config <path>      连接配置文件路径 (兼容旧模式, 默认: config/connection_config.json)");
    LOG_INFO("  --lib <path>         liblbapi.so 路径 (默认: ../../build_cmake/lib/liblbapi.so)");
    LOG_INFO("  --testcase <path>    单个测试用例 JSON 文件路径");
    LOG_INFO("  --testdir <path>     测试用例目录 (默认: config/test_cases)");
    LOG_INFO("  --report <path>      测试报告输出路径 (默认: result/test_report.txt)");
    LOG_INFO("  --log <path>         日志文件路径 (默认: result/mock_client.log)");
    LOG_INFO("  --analysis <path>    结果分析文件路径 (默认: result/result_analysis.txt)");
    LOG_INFO("  --help               打印帮助信息");
}

int main(int argc, char* argv[]) {
    // 默认参数
    std::string plan_path;
    std::string config_path = "config/connection_config.json";
    std::string lib_path = "../../build_cmake/lib/liblbapi.so";
    std::string testcase_path;
    std::string test_dir = "config/test_cases";
    std::string report_path = "result/test_report.txt";
    std::string log_path = "result/mock_client.log";
    std::string analysis_path = "result/result_analysis.txt";

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--plan") == 0 && i + 1 < argc) {
            plan_path = argv[++i];
        } else if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_path = argv[++i];
        } else if (std::strcmp(argv[i], "--lib") == 0 && i + 1 < argc) {
            lib_path = argv[++i];
        } else if (std::strcmp(argv[i], "--testcase") == 0 && i + 1 < argc) {
            testcase_path = argv[++i];
        } else if (std::strcmp(argv[i], "--testdir") == 0 && i + 1 < argc) {
            test_dir = argv[++i];
        } else if (std::strcmp(argv[i], "--report") == 0 && i + 1 < argc) {
            report_path = argv[++i];
        } else if (std::strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            log_path = argv[++i];
        } else if (std::strcmp(argv[i], "--analysis") == 0 && i + 1 < argc) {
            analysis_path = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            LOG_ERROR("未知选项: " << argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // 确保输出目录存在（result/ 等）
    ensure_output_dir(report_path);
    ensure_output_dir(log_path);
    ensure_output_dir(analysis_path);

    // 初始化日志系统（INFO 级别，同时输出到屏幕和日志文件）
    mock::Logger::instance().init(log_path, mock::LogLevel::INFO, true);
    LOG_INFO("========================================");
    LOG_INFO("      mock_client - API 测试客户端");
    LOG_INFO("========================================");
    if (!plan_path.empty()) {
        LOG_INFO("测试计划: " << plan_path);
    } else {
        LOG_INFO("配置文件: " << config_path);
    }
    LOG_INFO("动态库:   " << lib_path);
    if (!testcase_path.empty()) {
        LOG_INFO("测试用例: " << testcase_path);
    }
    LOG_INFO("报告输出: " << report_path);
    LOG_INFO("日志文件: " << log_path);
    LOG_INFO("分析文件: " << analysis_path);

    // 创建 MockClient
    mock::MockClient client;

    // 加载 API 动态库
    if (!client.load_api(lib_path)) {
        LOG_ERROR("加载 API 动态库失败");
        return 1;
    }

    bool ok = false;
    std::string plan_desc;

    // 模式一：测试计划主配置（test_plan.json），推荐
    if (!plan_path.empty()) {
        try {
            using mock::JsonParser;
            using mock::JsonValue;
            JsonValue plan_root = JsonParser::parse_file(plan_path);

            // 计算 base_dir：主配置文件所在目录（request_file/expected_file 相对该目录）
            std::string base_dir = ".";
            size_t slash = plan_path.find_last_of("/\\");
            if (slash != std::string::npos) {
                base_dir = plan_path.substr(0, slash);
            }

            ok = client.run_test_plan(plan_root, base_dir);
            if (!ok) {
                LOG_ERROR("测试计划执行失败");
                return 1;
            }
            plan_desc = plan_path;
        } catch (const std::exception& e) {
            LOG_ERROR("测试计划解析失败: " << e.what());
            return 1;
        }
    } else {
        // 模式二：旧模式（连接配置 + 测试用例目录/单文件）
        // 初始化
        if (!client.init(config_path)) {
            LOG_ERROR("初始化失败");
            return 1;
        }

        // 运行测试
        if (!testcase_path.empty()) {
            // 运行单个测试用例
            mock::TestResult r = client.run_test(testcase_path);
            client.report().print();
        } else {
            // 运行目录下所有测试用例
            client.run_all_tests(test_dir);
            client.report().print();
        }
        ok = true;

        // 性能测试（可选，通过 connection_config.json 的 perf_test.enable 控制）
        try {
            using mock::JsonParser;
            using mock::JsonValue;
            JsonValue config = JsonParser::parse_file(config_path);
            JsonValue perf_node = config["perf_test"];
            if (!perf_node.is_null()) {
                client.run_perf_test(perf_node);
            } else {
                LOG_INFO("配置中未找到 perf_test 节点，跳过性能测试");
            }
        } catch (const std::exception& e) {
            LOG_ERROR("性能测试配置解析失败: " << e.what());
        }
        plan_desc = config_path;
    }

    // 保存报告
    client.report().save(report_path);
    LOG_INFO("测试报告已保存到: " << report_path);

    // 写入测试结果分析文件
    client.write_analysis(analysis_path, plan_desc);

    // 等待异步回报（如成交回报）到达后处理，再关闭
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 关闭
    client.shutdown();

    return ok ? 0 : 1;
}
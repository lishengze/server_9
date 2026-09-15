#include "mock_client.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

/// 打印帮助信息
void print_usage(const char* prog) {
    std::cout << "用法: " << prog << " [选项]\n"
              << "选项:\n"
              << "  --config <path>       连接配置文件路径 (默认: config/connection_config.json)\n"
              << "  --lib <path>          liblbapi.so 路径 (默认: ../../build_cmake/lib/liblbapi.so)\n"
              << "  --testcase <path>     单个测试用例 JSON 文件路径\n"
              << "  --testdir <path>      测试用例目录 (默认: config/test_cases)\n"
              << "  --report <path>       测试报告输出路径 (默认: test_report.txt)\n"
              << "  --help                打印帮助信息\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // 默认参数
    std::string config_path = "config/connection_config.json";
    std::string lib_path = "../../build_cmake/lib/liblbapi.so";
    std::string testcase_path;
    std::string test_dir = "config/test_cases";
    std::string report_path = "test_report.txt";

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_path = argv[++i];
        } else if (std::strcmp(argv[i], "--lib") == 0 && i + 1 < argc) {
            lib_path = argv[++i];
        } else if (std::strcmp(argv[i], "--testcase") == 0 && i + 1 < argc) {
            testcase_path = argv[++i];
        } else if (std::strcmp(argv[i], "--testdir") == 0 && i + 1 < argc) {
            test_dir = argv[++i];
        } else if (std::strcmp(argv[i], "--report") == 0 && i + 1 < argc) {
            report_path = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "未知选项: " << argv[i] << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    std::cout << "========================================\n";
    std::cout << "      mock_client - API 测试客户端\n";
    std::cout << "========================================\n";
    std::cout << "配置文件: " << config_path << std::endl;
    std::cout << "动态库:   " << lib_path << std::endl;
    if (!testcase_path.empty()) {
        std::cout << "测试用例: " << testcase_path << std::endl;
    } else {
        std::cout << "测试目录: " << test_dir << std::endl;
    }
    std::cout << "报告输出: " << report_path << std::endl;
    std::cout << std::endl;

    // 创建 MockClient
    mock::MockClient client;

    // 加载 API 动态库
    if (!client.load_api(lib_path)) {
        std::cerr << "加载 API 动态库失败" << std::endl;
        return 1;
    }

    // 初始化
    if (!client.init(config_path)) {
        std::cerr << "初始化失败" << std::endl;
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

    // 保存报告
    client.report().save(report_path);
    std::cout << "测试报告已保存到: " << report_path << std::endl;

    // 等待异步回报（如成交回报）到达后处理，再关闭
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 关闭
    client.shutdown();

    return 0;
}

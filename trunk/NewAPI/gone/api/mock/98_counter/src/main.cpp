#include "counter98_server.h"
#include <iostream>
#include <cstring>
#include <csignal>
#include <thread>
#include <chrono>

// 全局服务指针，用于信号处理
static mock_98::Counter98Server* g_server = nullptr;

/// 信号处理函数
void signal_handler(int sig) {
    std::cout << "\n[Main] 收到信号 " << sig << ", 正在停止服务..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

/// 打印帮助信息
void print_usage(const char* prog) {
    std::cout << "用法: " << prog << " [选项]\n"
              << "选项:\n"
              << "  --config <path>  配置文件路径 (默认: config/server_config.json)\n"
              << "  --port <port>    监听端口 (覆盖配置文件)\n"
              << "  --help           打印帮助信息\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string config_path = "config/server_config.json";
    int override_port = 0;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_path = argv[++i];
        } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            override_port = std::atoi(argv[++i]);
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
    std::cout << "   98_counter_mock - 98 柜台模拟服务\n";
    std::cout << "========================================\n";
    std::cout << "配置文件: " << config_path << std::endl;
    std::cout << std::endl;

    // 创建服务
    mock_98::Counter98Server server;
    g_server = &server;

    // 加载配置
    if (!server.load_config(config_path)) {
        std::cerr << "[Main] 加载配置失败" << std::endl;
        return 1;
    }

    // 覆盖端口
    if (override_port > 0) {
        std::cout << "[Main] 覆盖端口: " << override_port << std::endl;
        server.set_port(override_port);
    }

    // 注册信号处理
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 启动服务
    if (!server.start()) {
        std::cerr << "[Main] 启动服务失败" << std::endl;
        return 1;
    }

    // 主线程等待服务结束
    while (server.is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    g_server = nullptr;
    std::cout << "[Main] 服务已退出" << std::endl;
    return 0;
}

/// GOne 模拟柜台 - 入口
///
/// 功能：
///   模拟 GOne（FPGA）柜台服务端，支持 g1 协议
///   接收链接、证券信息请求、登录、心跳、委托、撤单
///
/// 使用方式：
///   ./gone_counter_mock --config config/server_config.json

#include "gone_counter_server.h"
#include <iostream>
#include <csignal>
#include <cstring>

namespace mock_gone {

static GoneCounterServer* g_server = nullptr;

void signal_handler(int sig) {
    std::cout << "\n[Main] 收到信号 " << sig << ", 正在关闭..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

} // namespace mock_gone

int main(int argc, char* argv[]) {
    std::string config_path = "config/server_config.json";

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_path = argv[i + 1];
            i++;
        }
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  GOne 模拟柜台 (gone_counter_mock)" << std::endl;
    std::cout << "  配置文件: " << config_path << std::endl;
    std::cout << "========================================" << std::endl;

    // 注册信号处理
    std::signal(SIGINT, mock_gone::signal_handler);
    std::signal(SIGTERM, mock_gone::signal_handler);

    // 创建并初始化服务器
    mock_gone::GoneCounterServer server;
    mock_gone::g_server = &server;

    if (!server.init(config_path)) {
        std::cerr << "[Main] 初始化失败" << std::endl;
        return 1;
    }

    if (!server.start()) {
        std::cerr << "[Main] 启动失败" << std::endl;
        return 1;
    }

    // 等待服务结束
    server.wait();

    std::cout << "[Main] GOne 模拟柜台已退出" << std::endl;
    return 0;
}

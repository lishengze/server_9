#ifndef MOCK_98_COUNTER98_SERVER_H
#define MOCK_98_COUNTER98_SERVER_H

#include "client_session.h"
#include "account_manager.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>

namespace mock_98 {

/// 服务端配置
struct ServerConfig {
    int listen_port;
    int heartbeat_timeout;
    std::string log_file;
    std::string log_level;
};

/// 98 柜台模拟服务端
class Counter98Server {
public:
    Counter98Server();
    ~Counter98Server();

    /// 从 JSON 文件加载配置
    bool load_config(const std::string& path);

    /// 启动服务
    bool start();

    /// 停止服务
    void stop();

    /// 是否正在运行
    bool is_running() const { return running_; }

private:
    /// 接收连接的主循环（运行在独立线程）
    void accept_loop();

    /// 清理超时会话
    void cleanup_loop();

    /// 移除会话
    void remove_session(int fd);

    ServerConfig config_;
    AccountManager acct_mgr_;
    int server_fd_;
    bool running_;

    // 会话管理
    std::vector<ClientSession*> sessions_;
    mutable std::mutex sessions_mutex_;

    // 线程
    std::thread* accept_thread_;
    std::thread* cleanup_thread_;
};

} // namespace mock_98

#endif // MOCK_98_COUNTER98_SERVER_H

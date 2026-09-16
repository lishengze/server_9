#ifndef MOCK_GONE_COUNTER_SERVER_H
#define MOCK_GONE_COUNTER_SERVER_H

#include "client_session.h"
#include "account_manager.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>

namespace mock_gone {

/// GOne 模拟柜台服务端
class GoneCounterServer {
public:
    GoneCounterServer();
    ~GoneCounterServer();

    /// 初始化配置
    bool init(const std::string& config_path);

    /// 启动服务
    bool start();

    /// 停止服务
    void stop();

    /// 等待服务结束
    void wait();

private:
    /// GW 链路监听线程（sec_info/login/heart）
    void gw_accept_thread();

    /// Core 链路监听线程（order/cancel/heart）
    void core_accept_thread();

    /// 通用 accept 循环
    void accept_loop(int listen_fd, LinkType link_type);

    /// 客户端处理线程
    void client_thread(ClientSession* session);

    /// 清理线程（检查心跳超时）
    void cleanup_thread();

    int gw_listen_fd_;
    int core_listen_fd_;
    int gw_port_;
    int core_port_;
    int heartbeat_timeout_;
    bool running_;

    AccountManager acct_mgr_;
    std::vector<ClientSession*> sessions_;
    std::mutex sessions_mutex_;

    std::thread gw_accept_thr_;
    std::thread core_accept_thr_;
    std::thread cleanup_thr_;
    std::vector<std::thread> client_threads_;
    std::mutex threads_mutex_;
};

} // namespace mock_gone

#endif // MOCK_GONE_COUNTER_SERVER_H

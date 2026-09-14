#include "counter98_server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <algorithm>

namespace mock_98 {

Counter98Server::Counter98Server()
    : server_fd_(-1)
    , running_(false)
    , accept_thread_(nullptr)
    , cleanup_thread_(nullptr)
{
    config_.listen_port = 9001;
    config_.heartbeat_timeout = 30;
}

Counter98Server::~Counter98Server() {
    stop();
}

bool Counter98Server::load_config(const std::string& path) {
    try {
        mock::JsonValue root = mock::JsonParser::parse_file(path);

        // 加载服务端配置
        mock::JsonValue server = root["server"];
        config_.listen_port = server["listen_port"].as_int();
        config_.heartbeat_timeout = server["heartbeat_timeout"].as_int();
        if (server.has("log_file")) {
            config_.log_file = server["log_file"].as_string();
        }
        if (server.has("log_level")) {
            config_.log_level = server["log_level"].as_string();
        }

        // 加载账户
        if (!acct_mgr_.load_config(path)) {
            std::cerr << "[Server] 加载账户配置失败" << std::endl;
            return false;
        }

        std::cout << "[Server] 配置加载成功: port=" << config_.listen_port
                  << ", agw_users=" << acct_mgr_.agw_users().size()
                  << ", accounts=" << acct_mgr_.accounts().size() << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Server] 加载配置异常: " << e.what() << std::endl;
        return false;
    }
}

bool Counter98Server::start() {
    if (running_) {
        std::cout << "[Server] 服务已在运行中" << std::endl;
        return true;
    }

    // 创建 socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[Server] 创建 socket 失败" << std::endl;
        return false;
    }

    // 设置 SO_REUSEADDR
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[Server] setsockopt(SO_REUSEADDR) 失败" << std::endl;
        ::close(server_fd_);
        return false;
    }

    // 绑定地址
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config_.listen_port);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[Server] bind 失败: port=" << config_.listen_port << std::endl;
        ::close(server_fd_);
        return false;
    }

    // 监听
    if (listen(server_fd_, 10) < 0) {
        std::cerr << "[Server] listen 失败" << std::endl;
        ::close(server_fd_);
        return false;
    }

    running_ = true;
    std::cout << "[Server] 98 柜台模拟服务已启动, 监听端口: " << config_.listen_port << std::endl;

    // 启动接收线程
    accept_thread_ = new std::thread(&Counter98Server::accept_loop, this);

    // 启动清理线程
    cleanup_thread_ = new std::thread(&Counter98Server::cleanup_loop, this);

    return true;
}

void Counter98Server::stop() {
    running_ = false;

    // 关闭 server socket 以中断 accept
    if (server_fd_ >= 0) {
        ::close(server_fd_);
        server_fd_ = -1;
    }

    // 等待线程结束
    if (accept_thread_ && accept_thread_->joinable()) {
        accept_thread_->join();
    }
    delete accept_thread_;
    accept_thread_ = nullptr;

    if (cleanup_thread_ && cleanup_thread_->joinable()) {
        cleanup_thread_->join();
    }
    delete cleanup_thread_;
    cleanup_thread_ = nullptr;

    // 先关闭所有客户端 fd，让阻塞的 recv() 返回错误，线程退出
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (size_t i = 0; i < sessions_.size(); i++) {
            sessions_[i]->close();
        }
    }
    // 等待客户端线程退出
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 清理所有会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (size_t i = 0; i < sessions_.size(); i++) {
            delete sessions_[i];
        }
        sessions_.clear();
    }

    std::cout << "[Server] 服务已停止" << std::endl;
}

void Counter98Server::accept_loop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "[Server] accept 失败" << std::endl;
            }
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "[Server] 新连接: " << client_ip << ":" << ntohs(client_addr.sin_port)
                  << ", fd=" << client_fd << std::endl;

        // 设置 TCP_NODELAY，及时发送响应
        int opt = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

        // 创建会话
        ClientSession* session = new ClientSession(client_fd, &acct_mgr_);
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.push_back(session);
        }

        // 启动接收线程（每个客户端一个线程）
        std::thread([session, this]() {
            char buf[4096];
            while (this->running_) {
                ssize_t n = recv(session->fd(), buf, sizeof(buf), 0);
                if (n <= 0) {
                    // 连接关闭或错误：关闭 fd 并标记断开，由 cleanup_loop 清理
                    std::cout << "[Server] 客户端断开: fd=" << session->fd() << std::endl;
                    session->close();
                    session->mark_disconnected();
                    break;
                }
                int ret = session->feed_data(buf, (size_t)n);
                if (ret < 0) {
                    std::cout << "[Server] 协议错误, 关闭会话: fd=" << session->fd() << std::endl;
                    session->close();
                    session->mark_disconnected();
                    break;
                }
            }
        }).detach();
    }
}

void Counter98Server::cleanup_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (size_t i = 0; i < sessions_.size(); ) {
            bool need_cleanup = sessions_[i]->is_disconnected() ||
                                sessions_[i]->is_timeout(config_.heartbeat_timeout);
            if (need_cleanup) {
                std::cout << "[Server] 清理会话: fd=" << sessions_[i]->fd()
                          << (sessions_[i]->is_disconnected() ? " (已断开)" : " (心跳超时)")
                          << std::endl;
                delete sessions_[i];
                sessions_.erase(sessions_.begin() + i);
            } else {
                i++;
            }
        }
    }
}

} // namespace mock_98

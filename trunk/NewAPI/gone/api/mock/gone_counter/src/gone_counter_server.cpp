#include "gone_counter_server.h"
#include "json_utils.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <ctime>

namespace mock_gone {

GoneCounterServer::GoneCounterServer()
    : gw_listen_fd_(-1)
    , core_listen_fd_(-1)
    , gw_port_(44001)
    , core_port_(44002)
    , heartbeat_timeout_(30)
    , running_(false)
{
}

GoneCounterServer::~GoneCounterServer() {
    stop();
}

bool GoneCounterServer::init(const std::string& config_path) {
    // 加载账户配置
    if (!acct_mgr_.load_config(config_path)) {
        std::cerr << "[Server] 加载账户配置失败: " << config_path << std::endl;
        return false;
    }

    // 读取服务端配置
    try {
        mock::JsonValue root = mock::JsonParser::parse_file(config_path);
        if (root.has("server")) {
            mock::JsonValue svr = root["server"];
            if (svr.has("gw_port")) {
                gw_port_ = svr["gw_port"].as_int();
            }
            if (svr.has("core_port")) {
                core_port_ = svr["core_port"].as_int();
            }
            if (svr.has("heartbeat_timeout")) {
                heartbeat_timeout_ = svr["heartbeat_timeout"].as_int();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Server] 解析配置文件异常: " << e.what() << std::endl;
        return false;
    }

    std::cout << "[Server] 配置加载完成: gw_port=" << gw_port_
              << ", core_port=" << core_port_
              << ", heartbeat_timeout=" << heartbeat_timeout_ << "s"
              << ", accounts=" << acct_mgr_.accounts().size() << std::endl;
    return true;
}

/// 创建并绑定监听套接字
static int create_listen_socket(int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        std::cerr << "[Server] 创建 socket 失败: " << strerror(errno) << std::endl;
        return -1;
    }

    // 允许地址重用
    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定地址
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[Server] bind 失败: port=" << port << ", " << strerror(errno) << std::endl;
        ::close(fd);
        return -1;
    }

    // 监听
    if (::listen(fd, 128) < 0) {
        std::cerr << "[Server] listen 失败: " << strerror(errno) << std::endl;
        ::close(fd);
        return -1;
    }
    return fd;
}

bool GoneCounterServer::start() {
    // 创建 GW 链路监听套接字
    gw_listen_fd_ = create_listen_socket(gw_port_);
    if (gw_listen_fd_ < 0) {
        return false;
    }

    // 创建 Core 链路监听套接字
    core_listen_fd_ = create_listen_socket(core_port_);
    if (core_listen_fd_ < 0) {
        ::close(gw_listen_fd_);
        gw_listen_fd_ = -1;
        return false;
    }

    running_ = true;

    // 启动 GW 链路 accept 线程
    gw_accept_thr_ = std::thread(&GoneCounterServer::gw_accept_thread, this);

    // 启动 Core 链路 accept 线程
    core_accept_thr_ = std::thread(&GoneCounterServer::core_accept_thread, this);

    // 启动清理线程
    cleanup_thr_ = std::thread(&GoneCounterServer::cleanup_thread, this);

    std::cout << "[Server] GOne 模拟柜台启动成功, GW 端口: " << gw_port_
              << ", Core 端口: " << core_port_ << std::endl;
    return true;
}

void GoneCounterServer::stop() {
    running_ = false;

    // 关闭监听套接字（唤醒 accept）
    if (gw_listen_fd_ >= 0) {
        ::close(gw_listen_fd_);
        gw_listen_fd_ = -1;
    }
    if (core_listen_fd_ >= 0) {
        ::close(core_listen_fd_);
        core_listen_fd_ = -1;
    }

    // 关闭所有客户端会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (size_t i = 0; i < sessions_.size(); i++) {
            sessions_[i]->close();
        }
    }

    // 等待线程结束
    if (gw_accept_thr_.joinable()) gw_accept_thr_.join();
    if (core_accept_thr_.joinable()) core_accept_thr_.join();
    if (cleanup_thr_.joinable()) cleanup_thr_.join();

    // 等待客户端线程结束
    {
        std::lock_guard<std::mutex> lock(threads_mutex_);
        for (size_t i = 0; i < client_threads_.size(); i++) {
            if (client_threads_[i].joinable()) client_threads_[i].join();
        }
        client_threads_.clear();
    }

    // 清理会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (size_t i = 0; i < sessions_.size(); i++) {
            delete sessions_[i];
        }
        sessions_.clear();
    }
}

void GoneCounterServer::wait() {
    if (gw_accept_thr_.joinable()) gw_accept_thr_.join();
    if (core_accept_thr_.joinable()) core_accept_thr_.join();
}

void GoneCounterServer::gw_accept_thread() {
    accept_loop(gw_listen_fd_, LinkType::GW);
}

void GoneCounterServer::core_accept_thread() {
    accept_loop(core_listen_fd_, LinkType::CORE);
}

void GoneCounterServer::accept_loop(int listen_fd, LinkType link_type) {
    std::string link_name = (link_type == LinkType::GW) ? "GW" : "CORE";
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = ::accept4(listen_fd, reinterpret_cast<struct sockaddr*>(&client_addr),
                                  &addr_len, SOCK_NONBLOCK);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 无新连接，sleep 短暂等待
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (!running_) break;
            std::cerr << "[Server][" << link_name << "] accept 失败: " << strerror(errno) << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        int client_port = ntohs(client_addr.sin_port);
        std::cout << "[Server][" << link_name << "] 新连接: " << ip_str << ":" << client_port
                  << ", fd=" << client_fd << std::endl;

        // 创建会话（GW 链路登录应答中 trade_port 填 core_port_）
        ClientSession* session = new ClientSession(client_fd, &acct_mgr_, link_type, core_port_);
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.push_back(session);
        }

        // 启动客户端处理线程
        {
            std::lock_guard<std::mutex> lock(threads_mutex_);
            client_threads_.emplace_back(&GoneCounterServer::client_thread, this, session);
        }
    }
}

void GoneCounterServer::client_thread(ClientSession* session) {
    char buf[8192];
    while (running_ && !session->is_disconnected()) {
        ssize_t n = ::recv(session->fd(), buf, sizeof(buf), 0);
        if (n > 0) {
            int ret = session->feed_data(buf, static_cast<size_t>(n));
            if (ret < 0) {
                std::cout << "[Server] 会话处理错误, ret=" << ret << ", fd=" << session->fd() << std::endl;
                break;
            }
        } else if (n == 0) {
            std::cout << "[Server] 连接关闭, fd=" << session->fd() << std::endl;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞模式下无数据，短暂等待
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (!running_) break;
            std::cerr << "[Server] recv 错误: " << strerror(errno) << ", fd=" << session->fd() << std::endl;
            break;
        }
    }

    session->mark_disconnected();
    session->close();
}

void GoneCounterServer::cleanup_thread() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        if (!running_) break;

        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (size_t i = 0; i < sessions_.size(); ) {
            if (sessions_[i]->is_disconnected() || sessions_[i]->is_timeout(heartbeat_timeout_)) {
                if (sessions_[i]->is_timeout(heartbeat_timeout_)) {
                    std::cout << "[Server] 心跳超时, 关闭会话, fd=" << sessions_[i]->fd() << std::endl;
                }
                sessions_[i]->close();
                delete sessions_[i];
                sessions_.erase(sessions_.begin() + i);
            } else {
                i++;
            }
        }
    }
}

} // namespace mock_gone

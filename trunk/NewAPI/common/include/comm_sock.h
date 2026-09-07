#pragma once

/**
 * @file comm_sock.h
 * @brief 通用套接字通信模块
 *
 * @details 提供UDP和TCP套接字的创建、配置、数据收发等基础功能。
 * 支持阻塞和非阻塞模式，包含套接字属性设置和错误处理。
 *
 * 主要特性：
 * - UDP/TCP套接字创建和配置
 * - 阻塞和非阻塞数据收发
 * - 套接字缓冲区设置
 * - 超时设置
 * - 组播支持
 * - 网卡接口查询
 */

#include "comm_sys.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

namespace lb_common {

/** @brief 网卡接口名称最大长度 */
#define CHANNEL_ETH_NAME_LEN 64
/** @brief IP地址字符串最大长度 */
#define CHANNEL_IP_LEN 16
/** @brief IPv4协议族 */
#define CHANNEL_FAMILY_IPV4 AF_INET
/** @brief IPv6协议族 */
#define CHANNEL_FAMILY_IPV6 AF_INET6
/** @brief MAC层MTU长度 */
#define CHANNEL_MAC_MTU_LEN 1480 // 1488,1500

/** @brief 套接字文件描述符类型 */
typedef int32 sock_fd;

/**
 * @brief 网络地址结构
 *
 * 存储IP地址和端口号信息的简单结构。
 */
struct csock_addr {
  char ip[CHANNEL_IP_LEN]; ///< IP地址字符串
  int32 port;              ///< 端口号
};

/**
 * @brief 套接字属性配置
 *
 * 用于配置套接字的各种属性参数。
 */
struct channel_attr {
  int32 family;         ///< 协议族，只支持 CHANNEL_FAMILY_IPV4
  int32 recvsockbuflen; ///< 系统套接字的接收缓存大小
  int32 sendsockbuflen; ///< 系统套接字的发送缓存大小
  int32 tcpdelayack;    ///< TCP禁用Nagle算法标志，1=禁用减少小包延迟
  int32 sendrecvtime;   ///< 收发超时时间（秒），默认5秒
  int32 localloop;      ///< 是否支持本地回环，UDP组播使用，1=支持
};

/**
 * @brief 套接字工具类
 *
 * 提供UDP和TCP套接字的创建、配置、数据收发等静态方法。
 * 支持阻塞和非阻塞模式，包含完整的错误处理机制。
 */
class sock_utils {
public:
  /**
   * @brief 发送UDP数据（网络地址结构）
   *
   * 向指定的网络地址发送UDP数据包。
   *
   * @param[in] fd 套接字文件描述符
   * @param[in] rem_net 远程网络地址结构
   * @param[in] buf 发送数据缓冲区
   * @param[in] len 数据长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  static int32 send_udp(sock_fd fd, struct sockaddr_in &rem_net, char *buf, int32 len);

  /**
   * @brief 发送UDP数据（网络地址结构，强制模式）
   *
   * 向指定的网络地址发送UDP数据包，遇到EAGAIN等错误时重试。
   *
   * @param[in] fd 套接字文件描述符
   * @param[in] rem_net 远程网络地址结构
   * @param[in] buf 发送数据缓冲区
   * @param[in] len 数据长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  static int32 send_udp_fc(sock_fd fd, struct sockaddr_in &rem_net, char *buf, int32 len);

  /**
   * @brief 发送UDP数据（自定义地址结构）
   *
   * 向指定的IP地址和端口发送UDP数据包。
   *
   * @param[in] fd 套接字文件描述符
   * @param[in] premote 远程地址指针
   * @param[in] buf 发送数据缓冲区
   * @param[in] len 数据长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  static int32 send_udp(sock_fd fd, csock_addr *premote, char *buf, int32 len);

  /**
   * @brief 发送UDP数据（自定义地址结构，强制模式）
   *
   * 向指定的IP地址和端口发送UDP数据包，遇到EAGAIN等错误时重试。
   *
   * @param[in] fd 套接字文件描述符
   * @param[in] premote 远程地址指针
   * @param[in] buf 发送数据缓冲区
   * @param[in] len 数据长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  static int32 send_udp_fc(sock_fd fd, csock_addr *premote, char *buf, int32 len);

  /**
   * @brief 接收UDP数据
   *
   * 从UDP套接字接收数据。
   *
   * @param[in] fd 套接字文件描述符
   * @param[out] buf 接收数据缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   */
  static int32 recv_udp(sock_fd fd, char *buf, int32 len);

  /**
   * @brief 接收UDP数据（强制模式）
   *
   * 从UDP套接字接收数据，遇到EAGAIN等错误时重试。
   *
   * @param[in] fd 套接字文件描述符
   * @param[out] buf 接收数据缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   */
  static int32 recv_udp_fc(sock_fd fd, char *buf, int32 len);

  /**
   * @brief 发送TCP数据
   *
   * 向TCP套接字发送数据，支持部分发送。
   *
   * @param[in] fd 套接字文件描述符
   * @param[in] buf 发送数据缓冲区
   * @param[in] len 数据长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  static int32 send_tcp(sock_fd fd, char *buf, int32 len);

  /**
   * @brief 发送TCP数据（强制模式）
   *
   * 向TCP套接字发送数据，遇到EAGAIN等错误时重试。
   *
   * @param[in] fd 套接字文件描述符
   * @param[in] buf 发送数据缓冲区
   * @param[in] len 数据长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  static int32 send_tcp_fc(sock_fd fd, char *buf, int32 len);

  /**
   * @brief 接收TCP数据
   *
   * 从TCP套接字接收数据，支持部分接收。
   *
   * @param[in] fd 套接字文件描述符
   * @param[out] buf 接收数据缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   */
  static int32 recv_tcp(sock_fd fd, char *buf, int32 len);

  /**
   * @brief 接收TCP数据（强制模式）
   *
   * 从TCP套接字接收数据，遇到EAGAIN等错误时重试。
   *
   * @param[in] fd 套接字文件描述符
   * @param[out] buf 接收数据缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   */
  static int32 recv_tcp_fc(sock_fd fd, char *buf, int32 len);

  /**
   * @brief 地址转换
   *
   * 将自定义地址结构转换为网络地址结构。
   *
   * @param[in] i_ipaddr 输入的自定义地址结构
   * @param[out] o_netaddr 输出的网络地址结构
   */
  static void addr_to_net(csock_addr &i_ipaddr, struct sockaddr_in &o_netaddr);

  /**
   * @brief 设置非阻塞模式
   *
   * 将套接字设置为非阻塞模式。
   *
   * @param[in] fd 套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 set_non_block(sock_fd fd);

  /**
   * @brief 设置套接字缓冲区
   *
   * 设置套接字的发送和接收缓冲区大小。
   *
   * @param[in] fd 套接字文件描述符
   * @param[in] sendbuflen 发送缓冲区大小，0表示不设置
   * @param[in] recvbuflen 接收缓冲区大小，0表示不设置
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 set_sock_buf(sock_fd fd, int32 sendbuflen, int32 recvbuflen);

  /**
   * @brief 创建组播发送套接字
   *
   * 创建用于发送组播数据的UDP套接字。
   *
   * @param[in] attr 套接字属性配置
   * @param[in] premote 组播地址
   * @param[in] plocal 本地地址（可为NULL）
   * @param[out] o_netaddr 输出的网络地址结构
   * @param[out] o_sock 输出的套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 create_mudp_send_sock(channel_attr &attr, csock_addr *premote, csock_addr *plocal,
                                     struct sockaddr_in &o_netaddr, sock_fd &o_sock);

  /**
   * @brief 创建组播接收套接字
   *
   * 创建用于接收组播数据的UDP套接字。
   *
   * @param[in] attr 套接字属性配置
   * @param[in] premote 组播地址
   * @param[in] plocal 本地地址（可为NULL）
   * @param[out] o_netaddr 输出的网络地址结构
   * @param[out] o_sock 输出的套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 create_mudp_recv_sock(channel_attr &attr, csock_addr *premote, csock_addr *plocal,
                                     struct sockaddr_in &o_netaddr, sock_fd &o_sock);

  /**
   * @brief 创建UDP发送套接字
   *
   * 创建用于发送UDP数据的套接字（非组播）。
   *
   * @param[in] attr 套接字属性配置
   * @param[in] premote 远程地址
   * @param[out] o_netaddr 输出的网络地址结构
   * @param[out] o_sock 输出的套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 create_udp_send_sock(channel_attr &attr, csock_addr *premote, struct sockaddr_in &o_netaddr,
                                    sock_fd &o_sock);

  /**
   * @brief 创建UDP接收套接字
   *
   * 创建用于接收UDP数据的套接字（非组播）。
   *
   * @param[in] attr 套接字属性配置
   * @param[in] plocal 本地地址
   * @param[out] o_netaddr 输出的网络地址结构
   * @param[out] o_sock 输出的套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 create_udp_recv_sock(channel_attr &attr, csock_addr *plocal, struct sockaddr_in &o_netaddr,
                                    sock_fd &o_sock);

  /**
   * @brief 创建TCP连接套接字
   *
   * 创建TCP客户端套接字并尝试连接到服务器。
   *
   * @param[in] async_connect 是否异步模式，1=异步，0=同步
   * @param[in] attr 套接字属性配置
   * @param[in] premote 服务器地址
   * @param[in] plocal 本地地址（可为NULL）
   * @param[out] o_sock 输出的套接字文件描述符
   * @return 1 成功链接，0 异步链接中，<0 失败返回错误码
   */
  static int32 create_tcp_connect_sock(int32 async_connect, channel_attr &attr, csock_addr *premote, csock_addr *plocal,
                                       sock_fd &o_sock);

  /**
   * @brief 创建TCP监听套接字
   *
   * 创建TCP服务器监听套接字。
   *
   * @param[in] attr 套接字属性配置
   * @param[in] plocal 监听地址
   * @param[out] o_sock 输出的套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 create_tcp_listen_sock(channel_attr &attr, csock_addr *plocal, sock_fd &o_sock);

  /**
   * @brief 获取套接字错误
   *
   * 获取套接字的错误状态。
   *
   * @param[in] fd 套接字文件描述符
   * @param[out] o_errcode 输出的错误码
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 get_sock_error(sock_fd fd, int32 &o_errcode);

  /**
   * @brief 获取网卡名称
   *
   * 根据IP地址查找对应的网卡接口名称。
   *
   * @param[out] o_name 输出的网卡名称缓冲区
   * @param[in] namelen 缓冲区长度
   * @param[in] ethip IP地址字符串
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 get_eth_name(char *o_name, int32 namelen, const char *ethip);

  /**
   * @brief 获取网卡名称
   *
   * 根据IP地址查找对应的网卡接口名称。
   *
   * @param[out] o_ip 输出的网卡IPv4地址缓冲区
   * @param[in] ip_len 缓冲区长度
   * @param[in] eth_name 网卡名称
   * @return int32 成功返回0，失败返回错误码
   */
  static int32 get_eth_ip4(char *o_ip, int32 ip_len, const char *eth_name);
};

} // namespace lb_common

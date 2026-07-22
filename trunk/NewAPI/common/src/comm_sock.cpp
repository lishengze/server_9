#include "comm_sock.h"
#include "comm_errno.h"
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <ifaddrs.h>
#include <netinet/in.h>

/**
 * @file comm_sock.cpp
 * @brief 通用套接字通信模块实现
 *
 * 实现UDP和TCP套接字的创建、配置、数据收发等基础功能。
 * 支持阻塞和非阻塞模式，包含套接字属性设置和错误处理。
 */

namespace lb_common {

int32 sock_utils::send_udp(sock_fd fd, struct sockaddr_in &rem_net, char *buf, int32 len) {
  int32 ret;
  do {
    ret = sendto(fd, buf, len, 0, (struct sockaddr *)&(rem_net), sizeof(sockaddr_in));
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        ret = 0;
      else if (errno == EINTR)
        continue;
      else
        ret = LBERR_OBJ_WRITE_FAIL;
    }
    return ret;
  } while (1);
}
int32 sock_utils::send_udp_fc(sock_fd fd, struct sockaddr_in &rem_net, char *buf, int32 len) {
  int32 ret;
  do {
    ret = sendto(fd, buf, len, 0, (struct sockaddr *)&(rem_net), sizeof(sockaddr_in));
    if (unlikely(ret <= 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        CPU_PAUSE();
        continue;
      } else
        ret = LBERR_OBJ_WRITE_FAIL;
    }
    return ret;
  } while (1);
}

int32 sock_utils::send_udp(sock_fd fd, csock_addr *premote, char *buf, int32 len) {
  struct sockaddr_in remoteaddr;
  remoteaddr.sin_family = CHANNEL_FAMILY_IPV4;
  remoteaddr.sin_addr.s_addr = inet_addr(premote->ip);
  remoteaddr.sin_port = htons(premote->port);
  int32 ret;
  do {
    ret = sendto(fd, buf, len, 0, (struct sockaddr *)&(remoteaddr), sizeof(sockaddr_in));
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        ret = 0;
      else if (errno == EINTR)
        continue;
      else
        ret = LBERR_OBJ_WRITE_FAIL;
    }
    return ret;
  } while (1);
}
int32 sock_utils::send_udp_fc(sock_fd fd, csock_addr *premote, char *buf, int32 len) {
  struct sockaddr_in remoteaddr;
  remoteaddr.sin_family = CHANNEL_FAMILY_IPV4;
  remoteaddr.sin_addr.s_addr = inet_addr(premote->ip);
  remoteaddr.sin_port = htons(premote->port);
  int32 ret;
  do {
    ret = sendto(fd, buf, len, 0, (struct sockaddr *)&(remoteaddr), sizeof(sockaddr_in));
    if (unlikely(ret <= 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        CPU_PAUSE();
        continue;
      } else
        ret = LBERR_OBJ_WRITE_FAIL;
    }
    return ret;
  } while (1);
}
int32 sock_utils::recv_udp(sock_fd fd, char *buf, int32 len) {
  int ret;
  do {
    ret = recvfrom(fd, buf, len, 0, NULL, NULL);
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        ret = 0;
      } else if (errno == EINTR) {
        continue;
      } else
        ret = LBERR_OBJ_READ_FAIL;
    }
    return ret;
  } while (1);
}
int32 sock_utils::recv_udp_fc(sock_fd fd, char *buf, int32 len) {
  int ret;
  do {
    ret = recvfrom(fd, buf, len, 0, NULL, NULL);
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        CPU_PAUSE();
        continue;
      } else
        ret = LBERR_OBJ_READ_FAIL;
    }
    return ret;
  } while (1);
}
int32 sock_utils::send_tcp(sock_fd fd, char *buf, int32 len) {
  int32 ret = 0;
  int32 wlen = 0;
  do {
    ret = send(fd, buf + wlen, len - wlen, 0);
    if (likely(ret == len - wlen)) {
      return len;
    } else if (ret >= 0) {
      wlen += ret;
      continue;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return wlen;
      } else if (errno == EINTR) {
        continue;
      }
      return LBERR_OBJ_WRITE_FAIL;
    }
  } while (1);
}
int32 sock_utils::send_tcp_fc(sock_fd fd, char *buf, int32 len) {
  int32 ret = 0;
  int32 wlen = 0;
  do {
    ret = send(fd, buf + wlen, len - wlen, 0);
    if (likely(ret == len - wlen)) {
      return len;
    } else if (ret >= 0) {
      wlen += ret;
      continue;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        CPU_PAUSE();
        continue;
      }
      return LBERR_OBJ_WRITE_FAIL;
    }
  } while (1);
}
int32 sock_utils::recv_tcp(sock_fd fd, char *buf, int32 len) {
  int32 result;
  int32 rlen = 0;
  do {
    result = recv(fd, buf + rlen, len - rlen, 0);
    if (likely(result == len - rlen)) {
      return len;
    } else if (result >= 0) {
      rlen += result;
      continue;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return rlen;
      } else if (errno == EINTR) {
        continue;
      }
      return LBERR_OBJ_READ_FAIL;
    }
  } while (1);
}
int32 sock_utils::recv_tcp_fc(sock_fd fd, char *buf, int32 len) {
  int32 result;
  int32 rlen = 0;
  do {
    result = recv(fd, buf + rlen, len - rlen, 0);
    if (likely(result == len - rlen)) {
      return len;
    } else if (result >= 0) {
      rlen += result;
      continue;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        CPU_PAUSE();
        continue;
      }
      return LBERR_OBJ_READ_FAIL;
    }
  } while (1);
}

void sock_utils::addr_to_net(csock_addr &i_ipaddr, struct sockaddr_in &o_netaddr) {
  o_netaddr.sin_addr.s_addr = inet_addr(i_ipaddr.ip);
  o_netaddr.sin_port = htons(i_ipaddr.port);
}
int32 sock_utils::set_non_block(sock_fd fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
    return LBERR_ATTR_SET_FAIL;
  }
  return 0;
}
int32 sock_utils::set_sock_buf(sock_fd fd, int32 sendbuflen, int32 recvbuflen) {
  int32 buflen = 0;
  int32 len = sizeof(buflen);
  if (sendbuflen > 0) {
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&buflen, (socklen_t *)&len);
    buflen = sendbuflen > buflen ? sendbuflen : buflen;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&buflen, sizeof(buflen)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
  }
  if (recvbuflen > 0) {
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&buflen, (socklen_t *)&len);
    buflen = recvbuflen > buflen ? recvbuflen : buflen;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&buflen, sizeof(buflen)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
  }
  return 0;
}
int32 sock_utils::create_mudp_send_sock(channel_attr &attr, csock_addr *premote, csock_addr *plocal,
                                        struct sockaddr_in &o_netaddr, sock_fd &o_sock) {
  int32 ret = 0;
  int32 sock_opt = 1;
  if (NULL == premote || attr.family != CHANNEL_FAMILY_IPV4) {
    return LBERR_ARGV_WRONG;
  }
  if ('\0' == premote->ip[0] || premote->port <= 0) {
    return LBERR_ARGV_WRONG;
  }

  o_netaddr.sin_family = attr.family;
  o_netaddr.sin_addr.s_addr = inet_addr(premote->ip);
  o_netaddr.sin_port = htons(premote->port);

  int32 tfd = socket(attr.family, SOCK_DGRAM, IPPROTO_UDP);
  if (tfd == -1) {
    return LBERR_OBJ_OPEN_FAIL;
  }

  if ((ret = set_sock_buf(tfd, attr.sendsockbuflen, attr.recvsockbuflen)) < 0) {
    close(tfd);
    return ret;
  }

  if (attr.sendrecvtime >= 1) {
    struct timeval send_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
  }

  if ((ret = set_non_block(tfd)) < 0) {
    close(tfd);
    return ret;
  }

  if (attr.localloop == 1)
    sock_opt = 1;
  else
    sock_opt = 0; // 1 can loop,0 not loop
  if (setsockopt(tfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&sock_opt, sizeof(sock_opt)) != 0) {
    close(tfd);
    return LBERR_ATTR_SET_FAIL;
  }

  if (NULL != plocal && '\0' != plocal->ip[0]) {
    struct in_addr taddr = {0};
    taddr.s_addr = inet_addr(plocal->ip);
    if (setsockopt(tfd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&taddr, sizeof(taddr)) != 0) {
      close(tfd);
      return LBERR_ATTR_SET_FAIL;
    }
  }

  o_sock = tfd;
  return 0;
}
int32 sock_utils::create_mudp_recv_sock(channel_attr &attr, csock_addr *premote, csock_addr *plocal,
                                        struct sockaddr_in &o_netaddr, sock_fd &o_sock) {
  int32 ret = 0;
  int32 sock_opt = 1;
  struct sockaddr_in baddr;
  int32 addrlen = sizeof(baddr);
  struct ip_mreq multiaddr;

  if (NULL == premote || attr.family != CHANNEL_FAMILY_IPV4) {
    return LBERR_ARGV_WRONG;
  }
  if ('\0' == premote->ip[0] || premote->port <= 0) {
    return LBERR_ARGV_WRONG;
  }

  o_netaddr.sin_family = attr.family;
  o_netaddr.sin_addr.s_addr = inet_addr(premote->ip);
  o_netaddr.sin_port = htons(premote->port);

  int32 tfd = socket(attr.family, SOCK_DGRAM, IPPROTO_UDP);
  if (tfd == -1) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  if ((ret = set_sock_buf(tfd, attr.sendsockbuflen, attr.recvsockbuflen)) < 0) {
    close(tfd);
    return ret;
  }
  sock_opt = 1;
  if (setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt, sizeof(sock_opt)) == -1) {
    close(tfd);
    return LBERR_ATTR_SET_FAIL;
  }

  if (attr.sendrecvtime >= 1) {
    struct timeval recv_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
    struct timeval send_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
  }

  baddr.sin_family = attr.family;
  baddr.sin_port = htons(premote->port);
  baddr.sin_addr.s_addr = inet_addr(premote->ip);
  if (NULL == plocal || '\0' == plocal->ip[0]) {
    // baddr.sin_addr.s_addr = htonl(INADDR_ANY);
    multiaddr.imr_interface.s_addr = htonl(INADDR_ANY);
  } else {
    // baddr.sin_addr.s_addr = inet_addr(plocal->ip);
    multiaddr.imr_interface.s_addr = inet_addr(plocal->ip);
  }
  multiaddr.imr_multiaddr.s_addr = inet_addr((premote->ip));
  if (-1 == bind(tfd, (struct sockaddr *)&baddr, addrlen)) {
    close(tfd);
    return LBERR_CH_BIND_FAIL;
  }
  if (-1 == setsockopt(tfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multiaddr, sizeof(multiaddr))) {
    close(tfd);
    return LBERR_ATTR_SET_FAIL;
  }

  if (NULL != plocal && '\0' != plocal->ip[0]) {
    struct in_addr taddr = {0};
    taddr.s_addr = inet_addr(plocal->ip);
    setsockopt(tfd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&taddr, sizeof(taddr));
  }
  if ((ret = set_non_block(tfd)) < 0) {
    close(tfd);
    return ret;
  }
  o_sock = tfd;
  return 0;
}
int32 sock_utils::create_udp_send_sock(channel_attr &attr, csock_addr *premote, struct sockaddr_in &o_netaddr,
                                       sock_fd &o_sock) {
  int32 ret = 0;
  if (attr.family != CHANNEL_FAMILY_IPV4) {
    return LBERR_ARGV_WRONG;
  }
  if ('\0' == premote->ip[0] || premote->port <= 0) {
    return LBERR_ARGV_WRONG;
  }
  struct sockaddr_in remoteaddr;
  if (NULL != premote) {
    remoteaddr.sin_family = attr.family;
    remoteaddr.sin_port = htons(premote->port);
    remoteaddr.sin_addr.s_addr = inet_addr(premote->ip);
  }

  o_netaddr.sin_family = attr.family;
  o_netaddr.sin_addr.s_addr = inet_addr(premote->ip);
  o_netaddr.sin_port = htons(premote->port);

  int32 tfd = socket(attr.family, SOCK_DGRAM, IPPROTO_UDP);
  if (tfd == -1) {
    return LBERR_OBJ_OPEN_FAIL;
  }

  if ((ret = set_sock_buf(tfd, attr.sendsockbuflen, attr.recvsockbuflen)) < 0) {
    close(tfd);
    return ret;
  }

  if (attr.sendrecvtime >= 1) {
    struct timeval send_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
  }

  if (NULL != premote) {
    if (connect(tfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr)) < 0) {
      close(tfd);
      return LBERR_CH_CONNECT_FAIL;
    }
  }

  if ((ret = set_non_block(tfd)) < 0) {
    close(tfd);
    return ret;
  }

  o_sock = tfd;
  return 0;
}
int32 sock_utils::create_udp_recv_sock(channel_attr &attr, csock_addr *plocal, struct sockaddr_in &o_netaddr,
                                       sock_fd &o_sock) {
  int32 ret = 0;
  int32 sock_opt = 1;
  struct sockaddr_in baddr;
  int32 addrlen = sizeof(baddr);
  if (NULL == plocal || attr.family != CHANNEL_FAMILY_IPV4) {
    return LBERR_ARGV_WRONG;
  }
  if ('\0' == plocal->ip[0] || plocal->port <= 0) {
    return LBERR_ARGV_WRONG;
  }

  o_netaddr.sin_family = attr.family;
  o_netaddr.sin_addr.s_addr = inet_addr(plocal->ip);
  o_netaddr.sin_port = htons(plocal->port);

  int32 tfd = socket(attr.family, SOCK_DGRAM, IPPROTO_UDP);
  if (tfd == -1) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  if ((ret = set_sock_buf(tfd, attr.sendsockbuflen, attr.recvsockbuflen)) < 0) {
    close(tfd);
    return ret;
  }
  sock_opt = 1;
  if (setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt, sizeof(sock_opt)) != 0) {
    close(tfd);
    return LBERR_ATTR_SET_FAIL;
  }
  if (attr.sendrecvtime >= 1) {
    struct timeval recv_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
    struct timeval send_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
  }
  baddr.sin_family = attr.family;
  baddr.sin_port = htons(plocal->port);
  baddr.sin_addr.s_addr = inet_addr(plocal->ip);
  if (bind(tfd, (struct sockaddr *)&baddr, addrlen) < 0) {
    close(tfd);
    return LBERR_CH_BIND_FAIL;
  }
  if ((ret = set_non_block(tfd)) < 0) {
    close(tfd);
    return ret;
  }
  o_sock = tfd;
  return 0;
}
int32 sock_utils::create_tcp_connect_sock(int32 async_connect, channel_attr &attr, csock_addr *premote,
                                          csock_addr *plocal, sock_fd &o_sock) {
  int32 ret = 0;
  int32 sock_opt = 1;
  struct sockaddr_in remoteaddr;
  struct sockaddr_in baddr;
  int32 addrlen = sizeof(baddr);
  if (NULL == premote || attr.family != CHANNEL_FAMILY_IPV4)
    return LBERR_ARGV_WRONG;
  if ('\0' == premote->ip[0] || premote->port <= 0)
    return LBERR_ARGV_WRONG;
  struct linger so_linger;

  int32 tfd = socket(attr.family, SOCK_STREAM, IPPROTO_TCP);
  if (tfd == -1) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  baddr.sin_family = attr.family;
  if (NULL != plocal && '\0' != plocal->ip[0] && plocal->port > 0) {
    baddr.sin_port = htons(plocal->port);
    baddr.sin_addr.s_addr = inet_addr(plocal->ip);
    if (-1 == bind(tfd, (struct sockaddr *)&baddr, addrlen)) {
      close(tfd);
      return LBERR_CH_BIND_FAIL;
    }
  }
  if ((ret = set_sock_buf(tfd, attr.sendsockbuflen, attr.recvsockbuflen)) < 0) {
    close(tfd);
    return ret;
  }
  if (attr.tcpdelayack == 1) {
    sock_opt = 1;
    if (setsockopt(tfd, IPPROTO_TCP, TCP_NODELAY, (char *)&sock_opt, sizeof(sock_opt)) == -1) {
      close(tfd);
      return LBERR_ATTR_SET_FAIL;
    }
  }
  if (attr.sendrecvtime >= 1) {
    struct timeval recv_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
    struct timeval send_timeout {
      attr.sendrecvtime, 0
    };
    if (setsockopt(tfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) == -1) {
      return LBERR_ATTR_SET_FAIL;
    }
  }

  if (async_connect == 1) {
    if ((ret = set_non_block(tfd)) < 0) {
      close(tfd);
      return ret;
    }
  }
  so_linger.l_onoff = 1;
  so_linger.l_linger = 0;
  if (setsockopt(tfd, SOL_SOCKET, SO_LINGER, (char *)&so_linger, sizeof(so_linger)) != 0) {
    close(tfd);
    return LBERR_ATTR_SET_FAIL;
  }

  remoteaddr.sin_family = attr.family;
  remoteaddr.sin_port = htons(premote->port);
  remoteaddr.sin_addr.s_addr = inet_addr(premote->ip);
  while (1) {
    ret = connect(tfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr));
    if (ret < 0) {
      if (async_connect == 0) {
        close(tfd);
        return LBERR_CH_CONNECT_FAIL;
      } else if (errno == EINPROGRESS || errno == EAGAIN) {
        ret = 0;
      } else if (errno == EINTR) {
        continue;
      } else {
        close(tfd);
        return LBERR_CH_CONNECT_FAIL;
      }
    } else {
      if (async_connect == 0) {
        if ((ret = set_non_block(tfd)) < 0) {
          close(tfd);
          return ret;
        }
      }
      ret = 1;
    }
    break;
  }
  o_sock = tfd;
  return ret;
}
int32 sock_utils::create_tcp_listen_sock(channel_attr &attr, csock_addr *plocal, sock_fd &o_sock) {
  int32 ret = 0;
  if (NULL == plocal || attr.family != CHANNEL_FAMILY_IPV4)
    return LBERR_ARGV_WRONG;
  if ('\0' == plocal->ip[0] || plocal->port <= 0)
    return LBERR_ARGV_WRONG;
  int32 sock_opt = 1;
  struct sockaddr_in baddr;
  int32 addrlen = sizeof(baddr);

  int32 tfd = socket(attr.family, SOCK_STREAM, IPPROTO_TCP);
  if (tfd == -1) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  if ((ret = set_non_block(tfd)) < 0) {
    close(tfd);
    return ret;
  }
  if ((ret = set_sock_buf(tfd, attr.sendsockbuflen, attr.recvsockbuflen)) < 0) {
    close(tfd);
    return ret;
  }
  sock_opt = 1;
  if (setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt, sizeof(sock_opt)) == -1) {
    close(tfd);
    return LBERR_ATTR_SET_FAIL;
  }
  if (setsockopt(tfd, IPPROTO_TCP, TCP_NODELAY, (char *)&sock_opt, sizeof(sock_opt)) == -1) {
    close(tfd);
    return LBERR_ATTR_SET_FAIL;
  }
  baddr.sin_family = attr.family;
  baddr.sin_port = htons(plocal->port);
  baddr.sin_addr.s_addr = inet_addr(plocal->ip);
  if (-1 == bind(tfd, (struct sockaddr *)&baddr, addrlen)) {
    close(tfd);
    return LBERR_CH_BIND_FAIL;
  }
  if (listen(tfd, 64) == -1) {
    close(tfd);
    return LBERR_CH_LISTEN_FAIL;
  }
  o_sock = tfd;
  return 0;
}

int32 sock_utils::get_sock_error(sock_fd fd, int32 &o_errcode) {
  int32 terr = 0;
  uint32 tlen = sizeof(terr);
  int32 ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)(&terr), (socklen_t *)(&tlen));
  o_errcode = terr;
  return ret;
}

int32 sock_utils::get_eth_name(char *o_name, int32 namelen, const char *ethip) {
  if (NULL == ethip || o_name == NULL || namelen <= 0)
    return LBERR_ARGV_WRONG;
  if (ethip[0] == '\0')
    return LBERR_ARGV_WRONG;

  std::memset(o_name, 0, namelen);
  struct ifaddrs *ifaddr;
  struct ifaddrs *ifa;
  char taddrip[CHANNEL_IP_LEN];
  void *tpaddr;
  int32 tlen = std::strlen(ethip);
  int32 ret = LBERR_OBJ_NOT_HAVE;
  if (getifaddrs(&ifaddr) == -1) {
    return LBERR_ATTR_GET_FAIL;
  }
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    int32 family = ifa->ifa_addr->sa_family;
    // 只处理 IPv4 和 IPv6
    if (family != AF_INET) // && family != AF_INET6)
      continue;

    std::memset(taddrip, 0, CHANNEL_IP_LEN);
    tpaddr = (void *)(&(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
    inet_ntop(AF_INET, tpaddr, taddrip, CHANNEL_IP_LEN);

    if (std::strncmp(ethip, taddrip, tlen) == 0) {
      tlen = std::strlen(ifa->ifa_name);
      if (tlen >= namelen) {
        freeifaddrs(ifaddr);
        return LBERR_OBJ_NUM_LIMIT;
      }
      std::strncpy(o_name, ifa->ifa_name, tlen);
      ret = 0;
      break;
    }
  }
  freeifaddrs(ifaddr);
  return ret;
}

int32 sock_utils::get_eth_ip4(char *o_ip, int32 ip_len, const char *eth_name) {
  if (NULL == o_ip || eth_name == NULL || ip_len <= 16)
    return LBERR_ARGV_WRONG;
  if (eth_name[0] == '\0')
    return LBERR_ARGV_WRONG;

  std::memset(o_ip, 0, ip_len);
  struct ifaddrs *ifaddr;
  struct ifaddrs *ifa;
  void *tpaddr;
  int32 tlen = std::strlen(eth_name);
  int32 ret = LBERR_OBJ_NOT_HAVE;
  if (getifaddrs(&ifaddr) == -1) {
    return LBERR_ATTR_GET_FAIL;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;
    int32 family = ifa->ifa_addr->sa_family;
    // 只处理 IPv4 和 IPv6
    if (family != AF_INET) // && family != AF_INET6)
      continue;

    // 匹配网卡名和地址族
    if (std::strncmp(ifa->ifa_name, eth_name, tlen) == 0) {
      tpaddr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;

      if (NULL != tpaddr) {
        inet_ntop(family, tpaddr, o_ip, ip_len);
        ret = 0;
        break;
      }
    }
  }

  freeifaddrs(ifaddr);
  return ret;
}

} // namespace lb_common

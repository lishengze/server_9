#include "tcp_ch.h"
#include "comm_errno.h"
#include <cstring>

/**
 * @file tcp_ch.cpp
 * @brief TCP通道通信模块实现
 *
 * 实现TCP连接和监听通道的管理功能，支持同步和异步连接模式。
 * 包含连接状态管理、数据收发、事件处理等完整功能。
 */

namespace lb_common
{

  int32 tcp_ch::send_msg(char *pmsg, int32 len)
  {
    int32 trytimes = 100;
    int32 ret;
    int32 wlen = 0;
    while (is_work() && trytimes > 0)
    {
      ret = send(sysfd, pmsg + wlen, len - wlen, 0); // MSG_DONTWAIT);
      if (likely(ret == len - wlen))
      {
        return len;
      }
      else if (ret > 0)
      {
        wlen += ret;
        continue;
      }
      else if (ret == 0)
      {
        // TCP send 在仍有待发数据时返回 0 属异常（通常对端已关闭）。
        // 原代码 `ret >= 0` 落入 wlen += 0; continue; → 死循环。与 recv_msg 对齐。
        return LBERR_CH_LINK_BROKEN;
      }
      else
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          if (wlen == 0)
            return 0;
          trytimes--;
          continue;
        }
        else if (errno == EINTR)
        {
          continue;
        }
        else
          return LBERR_OBJ_WRITE_FAIL;
      }
    }
    return LBERR_OBJ_STATE_LIMIT;
  }

  int32 tcp_ch::send_msg_fc(char *pmsg, int32 len)
  {
    int32 trytimes = 100;
    int32 ret = 0;
    int32 wlen = 0;
    while (is_work() && trytimes > 0)
    {
      ret = send(sysfd, pmsg + wlen, len - wlen, 0); // MSG_DONTWAIT);
      if (likely(ret == len - wlen))
      {
        return len;
      }
      else if (ret > 0)
      {
        wlen += ret;
        continue;
      }
      else if (ret == 0)
      {
        // 见 send_msg：ret == 0 继续重试会死循环。
        return LBERR_CH_LINK_BROKEN;
      }
      else
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
          trytimes--;
          CPU_PAUSE();
          continue;
        }
        else
          return LBERR_OBJ_WRITE_FAIL;
      }
    }
    return LBERR_OBJ_STATE_LIMIT;
  }

  int32 tcp_ch::recv_msg(char *msgbuf, int32 len)
  {
    int32 ret;
    int32 rlen = 0;
    do
    {
      ret = recv(sysfd, msgbuf + rlen, len - rlen, 0); // MSG_DONTWAIT);
      if (likely(ret == len - rlen))
      {
        return len;
      }
      else if (ret > 0)
      {
        rlen += ret;
        continue;
      }
      else if (ret == 0)
      {
        return LBERR_CH_LINK_BROKEN;
      }
      else
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          return rlen;
        }
        else if (errno == EINTR)
        {
          continue;
        }
        else
        {
          return LBERR_OBJ_READ_FAIL;
        }
      }
    } while (is_work());
    return LBERR_OBJ_STATE_LIMIT;
  }

  int32 tcp_ch::recv_msg_fc(char *msgbuf, int32 len)
  {
    int32 ret;
    int32 rlen = 0;
    do
    {
      ret = recv(sysfd, msgbuf + rlen, len - rlen, 0); // MSG_DONTWAIT);
      if (likely(ret == len - rlen))
      {
        return len;
      }
      else if (ret > 0)
      {
        rlen += ret;
        continue;
      }
      else if (ret == 0)
      {
        return LBERR_CH_LINK_BROKEN;
      }
      else
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
          CPU_PAUSE();
          continue;
        }
        else
        {
          return LBERR_OBJ_READ_FAIL;
        }
      }
    } while (is_work());
    return LBERR_OBJ_STATE_LIMIT;
  }

  int32 tcp_ch::delive_ch_event(mthread &tpth, event_op *tpe_op, void *tpe_data, int32 data_len, int32 needadd)
  {
    if (unlikely(data_len < 0 || data_len > (int32)sizeof(event_info::buf)))
    {
      return LBERR_OBJ_WRITE_FAIL;
    }
    if (needadd == 1)
    {
      if (unlikely(!add_ref()))
      {
        return LBERR_REF_ADD_FAIL;
      }
    }
    int64 tpos;
    event_info *pe;
    if (likely(tpth.get_event(pe, tpos)))
    {
      pe->op = tpe_op;
      if (data_len > 0)
      {
        std::memcpy(pe->buf, tpe_data, data_len);
      }
      tpth.cmt_event(tpos);
      return 0;
    }
    if (needadd == 1)
      sub_ref();
    return LBERR_OBJ_IS_FULL;
  }

  int32 tcp_ch::connect_ch(channel_attr &tattr, tcp_ch_op *tpfunc, int32 isasynconnect, csock_addr *premote,
                           csock_addr *plocal, mthread *tpconnectth)
  {
    if ((isasynconnect == 1 && NULL == tpconnectth) || NULL == premote || tattr.family != CHANNEL_FAMILY_IPV4)
      return LBERR_ARGV_WRONG;

    if (!thctl.to_init())
      return LBERR_OBJ_STATE_LIMIT;

    pfunc = tpfunc;
    sysfd = -1;
    errcode = 0;
    mevent.pch = this;
    mevent.pconnectth = tpconnectth;
    mevent.set_event(1, 1, 0);

    bool tinit_closed = false;
    int32 ret = sock_utils::create_tcp_connect_sock(isasynconnect, tattr, premote, plocal, sysfd);
    if (ret < 0)
    {
      destroy();
      thctl.end_init(tinit_closed, false);
      return ret;
    }
    if (ret == 1)
    {
      struct sockaddr_in taddrsin;
      socklen_t taddrlen = sizeof(struct sockaddr_in);
      csock_addr tsaddr;
      std::memset(&tsaddr, 0, sizeof(tsaddr));
      if (getsockname(sysfd, (struct sockaddr *)&taddrsin, &taddrlen) == 0)
      {
        inet_ntop(CHANNEL_FAMILY_IPV4, &(taddrsin.sin_addr), tsaddr.ip, CHANNEL_IP_LEN);
        tsaddr.port = ntohs(taddrsin.sin_port);
      }
      thctl.end_init(tinit_closed, true);
      if (tinit_closed)
      {
        destroy();
        return LBERR_OBJ_STATE_LIMIT;
      }
      thctl.set_work();
      if (NULL != tpfunc)
        tpfunc->deal_tcp_connect(this, tsaddr);
    }
    else
    {
      ret = tpconnectth->add_poll_event(static_cast<epoll_event_op &>(mevent));
      if (ret < 0)
      {
        // 这里 fd 是本函数创建的 socket，必须 close 并清理状态。
        destroy();
        thctl.end_init(tinit_closed, false);
      }
    }
    return ret;
  }

  void tcp_ch::tcp_connect_ev::deal_close() { assert(0); }
  void tcp_ch::tcp_connect_ev::deal_event() { pch->deal_asyn_connect(pconnectth, 0); }

  void tcp_ch::tcp_connect_ev::deal_error() { pch->deal_asyn_connect(pconnectth, 1); }

  void tcp_ch::end_connect_init(int32 errcode)
  {
    if (mevent.pconnectth != NULL && sysfd != -1)
    {
      mevent.pconnectth->delete_poll_event(static_cast<epoll_event_op &>(mevent));
      mevent.pconnectth = NULL;
    }
    bool tinit_closed = false;
    if (errcode == 0)
    {
      csock_addr tsaddr;
      std::memset(&tsaddr, 0, sizeof(tsaddr));
      struct sockaddr_in taddrsin;
      socklen_t taddrlen = sizeof(struct sockaddr_in);
      if (getsockname(sysfd, (struct sockaddr *)&taddrsin, &taddrlen) == 0)
      {
        inet_ntop(CHANNEL_FAMILY_IPV4, &(taddrsin.sin_addr), tsaddr.ip, CHANNEL_IP_LEN);
        tsaddr.port = ntohs(taddrsin.sin_port);
      }

      if (thctl.end_init(tinit_closed, true))
      {
        if (tinit_closed)
        {
          // 对象已被业务 close_ch 触发 CLOSING，end_init(false) 路径已
          // 在本分支后由状态机自动调用 deal_after_closed()。本函数直接返回。
          return;
        }
        thctl.set_work();
        if (NULL != pfunc)
        {
          pfunc->deal_tcp_connect(this, tsaddr);
        }
      }
    }
    else
    {
      set_err(errcode);

      if (thctl.end_init(tinit_closed, false))
      {
        // deal_closing();
        deal_after_closed();
      }
    }
  }
  void tcp_ch::deal_asyn_connect(mthread *pconnectth, int32 iserror)
  {
    int32 terr = 0;
    sock_utils::get_sock_error(sysfd, terr);

    csock_addr tsaddr;
    std::memset(&tsaddr, 0, sizeof(tsaddr));
    if (iserror == 1)
    {
      if (terr == 0)
        terr = LBERR_CH_CONNECT_FAIL;
      else if (terr > 0)
        terr = 0 - terr;
    }
    else if (terr != 0)
    {
      if (terr > 0)
        terr = 0 - terr;
    }

    end_connect_init(terr);
  }

  int32 tcp_ch::tcp_connect_ev::get_fd() { return pch->sys_fd(); }

  void tcp_ch::close_ch(int32 err)
  {
    set_err(err);
    bool closeflag = false;
    if (thctl.to_close(closeflag))
    {
      deal_closing();
    }
    if (closeflag)
    {
      deal_after_closed();
    }
  }

  void tcp_ch::deal_closing()
  {
    int32 terr = errcode;
    if (NULL != pfunc)
      pfunc->deal_tcp_to_close(this, terr);
  }

  void tcp_ch::destroy()
  {
    int32 tfd = atomic_exchange32(&sysfd, -1);
    if (tfd != -1)
      close(tfd);
  }

  void tcp_ch::deal_after_closed()
  {
    destroy();
    int32 terr = errcode;
    if (NULL != pfunc)
      pfunc->deal_tcp_closed(this, terr);
  }

  int32 tcp_ch::start_poll_recv(mthread *tprecvth, epoll_event_op *tprecvevent)
  {
    if (NULL == tprecvth || NULL == tprecvevent)
      return LBERR_ARGV_WRONG;
    if (!add_ref())
      return LBERR_REF_ADD_FAIL;
    int32 ret = 0;
    if ((ret = tprecvth->add_poll_event(*tprecvevent)) < 0)
    {
      sub_ref();
      return ret;
    }
    return 0;
  }

  void tcp_ch::remove_poll_recv(mthread *tprecvth, epoll_event_op *tprecvevent)
  {
    if (NULL == tprecvth || NULL == tprecvevent)
      return;
    // tprecvth->delete_poll_event(*tprecvevent);
    tprecvth->remove_poll_event(*tprecvevent);
  }

  int32 tcp_ch::accept_ch(channel_attr &tattr, tcp_ch_op *tpfunc, sock_fd fd)
  {
    // fd == 0 是合法（虽罕见）的文件描述符，原 `fd <= 0` 会误拒。改为 fd < 0。
    if (fd < 0 || tattr.family != CHANNEL_FAMILY_IPV4)
    {
      return LBERR_ARGV_WRONG;
    }

    if (!thctl.to_init())
      return LBERR_OBJ_STATE_LIMIT;

    pfunc = tpfunc;
    sysfd = -1;
    errcode = 0;
    mevent.pch = this;
    mevent.pconnectth = NULL;
    mevent.set_event(1, 1, 0);

    int32 ret;
    bool tinit_closed = false;
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;

    if ((ret = sock_utils::set_sock_buf(fd, tattr.sendsockbuflen, tattr.recvsockbuflen)) < 0)
    {
      destroy();
      thctl.end_init(tinit_closed, false);
      return ret;
    }

    if (tattr.tcpdelayack == 1)
    {
      int32 sock_opt = 1;
      if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&sock_opt, sizeof(sock_opt)) == -1)
      {
        destroy();
        thctl.end_init(tinit_closed, false);
        return LBERR_ATTR_SET_FAIL;
      }
    }
    if (tattr.sendrecvtime >= 1)
    {
      struct timeval recv_timeout{
          tattr.sendrecvtime, 0};
      if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) == -1)
      {
        // fd 属调用方，此处不 close，但应清理本对象状态机。
        thctl.end_init(tinit_closed, false);
        return LBERR_ATTR_SET_FAIL;
      }
      struct timeval send_timeout{
          tattr.sendrecvtime, 0};
      if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) == -1)
      {
        thctl.end_init(tinit_closed, false);
        return LBERR_ATTR_SET_FAIL;
      }
    }
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&so_linger, sizeof(so_linger)) == -1)
    {
      destroy();
      thctl.end_init(tinit_closed, false);
      return LBERR_ATTR_SET_FAIL;
    }
    if ((ret = sock_utils::set_non_block(fd)) < 0)
    {
      destroy();
      thctl.end_init(tinit_closed, false);
      return ret;
    }
    sysfd = fd;
    thctl.end_init(tinit_closed, true);
    if (tinit_closed)
    {
      return LBERR_OBJ_STATE_LIMIT;
    }
    thctl.set_work();
    return 0;
  }

  int32 tcp_listen_ch::start_listen(mthread *tplistenth, int32 oncetimes)
  {
    if (NULL == tplistenth)
      return LBERR_ARGV_WRONG;
    if (oncetimes <= 0)
      oncetimes = TCPCH_LISTEN_ONCELOOP_NUM;
    loopnum = oncetimes;
    if (!add_ref())
    {
      return LBERR_REF_ADD_FAIL;
    }

    int32 ret = tplistenth->add_poll_event(*this);
    if (ret < 0)
    {
      sub_ref();
      return ret;
    }
    return 0;
  }

  void tcp_listen_ch::remove_listen(mthread *tplistenth)
  {
    if (NULL == tplistenth)
      return;
    // tplistenth->delete_poll_event(*this);
    tplistenth->remove_poll_event(*this);
  }

  int32 tcp_listen_ch::accept_listen(int32 &o_fd, csock_addr &o_addr)
  {
    sock_fd acceptfd;
    struct sockaddr_in clientsin;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int32 ret = LBERR_CH_ACCEPT_FAIL;
    while (is_work())
    {
      acceptfd = accept(sysfd, (struct sockaddr *)&clientsin, (socklen_t *)&addrlen);
      if (acceptfd == -1)
      {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
          ret = 0;
        }
        else if (errno == ECONNABORTED)
        {
          ret = 0;
        }
        else if (errno == EMFILE || errno == ENFILE)
        {
          // P1-27: 水平触发 epoll 下，EMFILE/ENFILE（fd 耗尽/系统 fd 上限）
          // 与 EAGAIN 语义不同：连接仍在内核 accept 队列里。
          // 不处理会导致 epoll 立刻再次唤醒 + accept 又 EMFILE → 100% CPU。
          // 防御：sleep 1ms 让内核消化部分已建连接的 fd
          // （被其他线程 accept 或被 RST 清除），避免 CPU 风暴。
          // 业务可通过 pfunc->deal_listen_error 自行处理。
          if (NULL != pfunc)
          {
            pfunc->deal_listen_error(this, LBERR_CH_EPOLL_FAIL, errno);
          }
          comm_utils::sleep_us(1000);
          ret = 0;
        }
        else if (errno == EINTR)
        {
          continue;
        }
        else
        {
          set_err(ret);
        }
        return ret;
      }

      if ((ret = getpeername(acceptfd, (struct sockaddr *)&clientsin, &addrlen)) < 0)
      {
        close(acceptfd);
        return LBERR_ATTR_GET_FAIL;
      }
      if (NULL == inet_ntop(CHANNEL_FAMILY_IPV4, &(clientsin.sin_addr), o_addr.ip, CHANNEL_IP_LEN))
      {
        close(acceptfd);
        return LBERR_ATTR_GET_FAIL;
      }

      o_fd = acceptfd;
      o_addr.port = ntohs(clientsin.sin_port);
      return 1;
    }
    return LBERR_OBJ_CLOSE_FAIL;
  }

  void tcp_listen_ch::deal_event()
  {
    csock_addr tcaddr;
    sock_fd tfd;
    int32 ret = 0;
    int32 i = 0;
    while (i < loopnum)
    {
      ret = accept_listen(tfd, tcaddr);
      if (ret == 1)
      {
        if (NULL != pfunc)
        {
          if (pfunc->deal_listen_accept(this, tcaddr, tfd) < 0)
          {
            close(tfd);
          }
        }
        i++;
        continue;
      }
      else if (ret < 0)
      {
        if (NULL != pfunc)
        {
          pfunc->deal_listen_error(this, ret, errno);
        }
      }
      break;
    }
  }

  void tcp_listen_ch::deal_error()
  {
    set_err(LBERR_CH_EPOLL_FAIL);
    if (NULL != pfunc)
    {
      int32 terr = 0;
      sock_utils::get_sock_error(sysfd, terr);
      pfunc->deal_listen_error(this, LBERR_CH_EPOLL_FAIL, terr);
    }
  }

  void tcp_listen_ch::deal_close() { sub_ref(); }

  void tcp_listen_ch::close_ch(int32 err)
  {
    set_err(err);
    bool closeflag = false;
    if (thctl.to_close(closeflag))
    {
      deal_closing();
    }
    if (closeflag)
    {
      deal_after_closed();
    }
  }

  void tcp_listen_ch::destroy()
  {
    int32 tfd = atomic_exchange32(&sysfd, -1);
    if (tfd != -1)
      close(tfd);
  }

  void tcp_listen_ch::deal_closing()
  {
    int32 terr = errcode;
    if (NULL != pfunc)
      pfunc->deal_listen_to_close(this, terr);
  }

  void tcp_listen_ch::deal_after_closed()
  {
    destroy();
    int32 terr = errcode;
    if (NULL != pfunc)
      pfunc->deal_listen_closed(this, terr);
  }

  int32 tcp_listen_ch::listen_ch(channel_attr &attr, tcp_listen_op *tpfunc, csock_addr *plocal, void *puserdata)
  {
    if (NULL == tpfunc || NULL == plocal)
      return LBERR_ARGV_WRONG;

    if (!thctl.to_init())
      return LBERR_OBJ_STATE_LIMIT;

    sysfd = -1;
    loopnum = TCPCH_LISTEN_ONCELOOP_NUM;
    errcode = 0;
    pfunc = tpfunc;
    userdata = puserdata;
    set_event(0, 0, 1);

    bool tinit_closed = false;
    int32 ret = sock_utils::create_tcp_listen_sock(attr, plocal, sysfd);
    if (ret < 0)
    {
      destroy();
      thctl.end_init(tinit_closed, false);
      return ret;
    }

    thctl.end_init(tinit_closed, true);
    if (tinit_closed)
    {
      destroy();
      return LBERR_OBJ_STATE_LIMIT;
    }
    thctl.set_work();
    return 0;
  }

} // namespace lb_common

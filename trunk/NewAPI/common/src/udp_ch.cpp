#include "udp_ch.h"
#include "comm_errno.h"
#include <cstring>
/**
 * @file udp_ch.cpp
 * @brief UDP通道通信模块实现
 *
 * 实现UDP通道的管理功能，支持单播、组播和广播模式。
 * 包含连接状态管理、数据收发、事件处理等完整功能。
 */

namespace lb_common {

int32 udp_ch::send_msg(char *pmsg, int32 len) {
  int32 ret;
  while (is_work()) {
    ret = sendto(sysfd, pmsg, len, 0, (struct sockaddr *)&(remoteaddr), sizeof(sockaddr_in));
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        ret = 0;
      else if (errno == EINTR)
        continue;
      else
        ret = LBERR_OBJ_WRITE_FAIL;
    }
    return ret;
  }
  return LBERR_OBJ_STATE_LIMIT;
}

int32 udp_ch::send_msg_fc(char *pmsg, int32 len) {
  int32 ret;
  while (is_work()) {
    ret = sendto(sysfd, pmsg, len, 0, (struct sockaddr *)&(remoteaddr), sizeof(sockaddr_in));
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        continue;
      else
        ret = LBERR_OBJ_WRITE_FAIL;
    }
    return ret;
  }
  return LBERR_OBJ_STATE_LIMIT;
}

int32 udp_ch::recv_msg(char *msgbuf, int32 len) {
  int32 ret;
  do {
    ret = recvfrom(sysfd, msgbuf, len, 0, NULL, NULL);
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        ret = 0;
      } else if (errno == EINTR) {
        continue;
      } else
        ret = LBERR_OBJ_READ_FAIL;
    }
    return ret;
  } while (is_work());
  return LBERR_OBJ_STATE_LIMIT;
}

int32 udp_ch::recv_msg_fc(char *msgbuf, int32 len) {
  int32 ret;
  do {
    ret = recvfrom(sysfd, msgbuf, len, 0, NULL, NULL);
    if (unlikely(ret < 0)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        continue;
      } else
        ret = LBERR_OBJ_READ_FAIL;
    }
    return ret;
  } while (is_work());
  return LBERR_OBJ_STATE_LIMIT;
}

int32 udp_ch::delive_ch_event(mthread &tpth, event_op *tpe_op, void *tpe_data, int32 data_len, int32 needadd) {
  if (needadd == 1) {
    if (unlikely(!add_ref())) {
      return LBERR_REF_ADD_FAIL;
    }
  }
  int64 tpos;
  event_info *pe;
  if (likely(tpth.get_event(pe, tpos))) {
    pe->op = tpe_op;
    assert(data_len <= (int32)sizeof(pe->buf));
    if (data_len > 0) {
      std::memcpy(pe->buf, tpe_data, data_len);
    }
    tpth.cmt_event(tpos);
    return 0;
  }
  if (needadd == 1)
    sub_ref();
  return LBERR_OBJ_IS_FULL;
}

int32 udp_ch::init_ch(channel_attr &attr, udp_ch_op *tpasyfunc, int32 isrecv, int32 ismulti, csock_addr *premote,
                      csock_addr *plocal) {
  if (!thctl.to_init())
    return LBERR_OBJ_STATE_LIMIT;

  sysfd = -1;
  errcode = 0;
  std::memset((void *)(&remoteaddr), 0, sizeof(remoteaddr));
  pfunc = tpasyfunc;

  int32 ret = 0;
  bool tinit_closed = false;
  if (isrecv == 1 && ismulti == 1) {
    ret = sock_utils::create_mudp_recv_sock(attr, premote, plocal, remoteaddr, sysfd);
  } else if (isrecv == 0 && ismulti == 1) {
    ret = sock_utils::create_mudp_send_sock(attr, premote, plocal, remoteaddr, sysfd);
  } else if (isrecv == 1 && ismulti == 0) {
    ret = sock_utils::create_udp_recv_sock(attr, plocal, remoteaddr, sysfd);
  } else if (isrecv == 0 && ismulti == 0) {
    ret = sock_utils::create_udp_send_sock(attr, premote, remoteaddr, sysfd);
  } else {
    ret = LBERR_ARGV_WRONG;
  }
  if (ret < 0) {
    set_err(ret);
    destroy();
    thctl.end_init(tinit_closed, false);
    return ret;
  }
  thctl.end_init(tinit_closed, true);
  assert(!tinit_closed);
  thctl.set_work();
  return ret;
}

void udp_ch::close_ch(int32 err) {
  set_err(err);
  bool closeflag = false;
  if (thctl.to_close(closeflag)) {
    deal_closing();
  }
  if (closeflag) {
    deal_after_close();
  }
}

void udp_ch::deal_closing() {
  int32 terr = errcode;
  if (NULL != pfunc)
    pfunc->deal_udp_to_close(this, terr);
}

void udp_ch::destroy() {
  int32 tfd = atomic_exchange32(&sysfd, -1);
  if (tfd != -1)
    close(tfd);
}

void udp_ch::deal_after_close() {
  destroy();
  int32 terr = errcode;
  if (NULL != pfunc)
    pfunc->deal_udp_closed(this, terr);
}

int32 udp_ch::start_poll_recv(mthread *tprecvth, epoll_event_op *tprecvevent) {
  if (NULL == tprecvth || NULL == tprecvevent)
    return LBERR_ARGV_WRONG;
  if (!add_ref())
    return LBERR_REF_ADD_FAIL;

  int32 ret = 0;
  if ((ret = tprecvth->add_poll_event(*tprecvevent)) < 0) {
    sub_ref();
    return ret;
  }
  return 0;
}

void udp_ch::remove_poll_recv(mthread *tprecvth, epoll_event_op *tprecvevent) {
  if (NULL == tprecvth || NULL == tprecvevent)
    return;
  // tprecvth->delete_poll_event(*tprecvevent);
  tprecvth->remove_poll_event(*tprecvevent);
}

} // namespace lb_common

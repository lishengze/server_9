/**
 * @file tcpdir_stack.cpp
 * @brief Solarflare TCPDirect 封装模块实现
 *
 * @note 整个文件内容仅在 HAS_TCPDIRECT 宏定义时编译
 */

#include "tcpdir_stack.h"
#include "comm_errno.h"
#include "comm_sys.h"
#include "matomic.h"
#include "mutils.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <new>
#include <sys/socket.h>
#include <unistd.h>

namespace lb_common {

// ==================== tcpdir_stack 进程级 zf_init 管理 ====================

/** @brief zf_init 进程级初始化标志 */
static int32 g_zf_init_flag = 0;

int32 tcpdir_stack::zf_load_driver() {
  int32 t = atomic_load32(&g_zf_init_flag);
  do {
    if (t == 0) {
      if (atomic_cas32(&g_zf_init_flag, &t, 1))
        break;
    } else if (t == 2) {
      return 0;
    }
  } while (true);

  int32 ret = zf_init();
  if (ret < 0) {
    atomic_store32(&g_zf_init_flag, 0);
    return LBERR_OBJ_INIT_FAIL;
  }

  atomic_store32(&g_zf_init_flag, 2);
  return 0;
}

// ==================== tcpdir_stack 实现 ====================

int32 tcpdir_stack::set_attr(tcpdir_stack_attr &attr) {
  // 通用属性：大缓冲区应对突发
  if (attr.rx_ring_size <= 512)
    attr.rx_ring_size = 512;
  else if (attr.rx_ring_size <= 1024)
    attr.rx_ring_size = 1024;
  else if (attr.rx_ring_size <= 2048)
    attr.rx_ring_size = 2048;
  else
    attr.rx_ring_size = 4096;

  if (attr.tx_ring_size <= 512)
    attr.tx_ring_size = 512;
  else if (attr.tx_ring_size <= 1024)
    attr.tx_ring_size = 1024;
  else
    attr.tx_ring_size = 2048;

  if (attr.max_connect_num <= 4)
    attr.max_connect_num = 4;
  else if (attr.max_connect_num > 64)
    attr.max_connect_num = 64;

  int32 ret = 0;
  //zf_attr_set_int(attr, "n_bufs", 0);                      // 最大的数据包缓冲池
  ret = zf_attr_set_int(attr, "rx_ring_max", attr.rx_ring_size); // 接收环大小
  if (ret < 0)
    return ret;
  ret = zf_attr_set_int(attr, "tx_ring_max", attr.tx_ring_size); // 发送环大小
  if (ret < 0)
    return ret;
  ret = zf_attr_set_int(attr, "max_tcp_endpoints", attr.max_connect_num);
  if (ret < 0)
    return ret;
  ret = zf_attr_set_int(attr, "pio", 0); // 在最新的 X3/X4 系列网卡上建议明确设为0，仅使用更优的CTPIO
  if (ret < 0)
    return ret;
  ret = zf_attr_set_int(attr, "tcp_delayed_ack", 0); // 关闭延迟确认
  if (ret < 0)
    return ret;

  if (attr.mode == TCPDIR_MODE_LOW_LATENCY) {
    // 单个stack最大支持的连接数
    // 直通模式（最低延迟）
    ret = zf_attr_set_int(attr, "ctpio", 1);
    if (ret < 0)
      return ret;
    ret = zf_attr_set_str(attr, "ctpio_mode", "ct");
    if (ret < 0)
      return ret;
    // 高频自旋
    if (attr.independ_send == 0)
      ret = zf_attr_set_int(attr, "reactor_spin_count", 30000);
    else
      ret = zf_attr_set_int(attr, "reactor_spin_count", 1);
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "tcp_retries", 3); //局域网环境，实现“快速失败”
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "tcp_syn_retries", 3);
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "rx_ring_refill_interval", 1); //每次 reactor 调用一次填充
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "rx_ring_refill_batch_size", 8);
    if (ret < 0)
      return ret;
  } else {
    // 安全存储转发模式
    ret = zf_attr_set_int(attr, "ctpio", 1);
    if (ret < 0)
      return ret;
    ret = zf_attr_set_str(attr, "ctpio_mode", "sf-np");
    if (ret < 0)
      return ret;
    // 适中自旋
    if (attr.independ_send == 0)
      ret = zf_attr_set_int(attr, "reactor_spin_count", 3000);
    else
      ret = zf_attr_set_int(attr, "reactor_spin_count", 1);
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "tcp_retries", 4); // 局域网环境，实现“快速失败”
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "tcp_syn_retries", 4);
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "rx_ring_refill_interval", 1); // 每次 reactor 调用一次填充
    if (ret < 0)
      return ret;
    ret = zf_attr_set_int(attr, "rx_ring_refill_batch_size", 16); // 32
    if (ret < 0)
      return ret;
  }
  return 0;
}

int32 tcpdir_stack::init_stack(tcpdir_stack_attr &attr) {
  // 确保 zf_init 已调用
  int32 ret = zf_load_driver();
  if (ret < 0)
    return ret;

  if (!thctl.to_init())
    return LBERR_OBJ_STATE_LIMIT;

  // 分配属性
  ret = zf_attr_alloc(&pattr);
  if (unlikely(ret < 0)) {
    thctl.end_init(false);
    return LBERR_OBJ_OPEN_FAIL;
  }

  // 根据模式配置属性
  mode = attr.mode;
  ret = set_attr(attr);
  if (unlikely(ret < 0)) {
    zf_attr_free(pattr);
    pattr = NULL;
    thctl.end_init(false);
    return LBERR_ATTR_SET_FAIL;
  }

  // 分配 stack
  ret = zf_stack_alloc(pattr, &pstack);
  if (unlikely(ret < 0)) {
    zf_attr_free(pattr);
    pattr = NULL;
    thctl.end_init(false);
    return LBERR_OBJ_OPEN_FAIL;
  }

  thctl.end_init(true);
  return 0;
}

int32 tcpdir_stack::poll_stack() {
  if (unlikely(!thctl.is_work()))
    return LBERR_OBJ_STATE_LIMIT;

  int32 ret = zf_reactor_perform();
  if (ret < 0)
    return LBERR_OBJ_READ_FAIL;

  return 0;
}
int32 tcpdir_stack::poll_stack_lock() {
  if (thctl.recv_lock()) {
    int32 ret = zf_reactor_perform();
    thctl.recv_unlock();
    if (ret < 0)
      return LBERR_OBJ_READ_FAIL;

    return 0;
  }
  return LBERR_OBJ_STATE_LIMIT;
}

void tcpdir_stack::destroy() {
  if (pstack != NULL) {
    zf_stack_free(pstack);
    pstack = NULL;
  }
  if (pattr != NULL) {
    zf_attr_free(pattr);
    pattr = NULL;
  }
  inited = 0;
  mode = 0;
}

// ==================== tcpdir_ch 实现 ====================

int32 tcpdir_ch::connect_ch(tcpdir_stack *pstack, tcpdir_ch_attr &ch_attr, tcpdir_msg_cb *tpmsg_cb,
                            tcpdir_ch_op *tpfunc, csock_addr *premote, csock_addr *plocal, int32 asyn_connect) {
  if (unlikely(pstack == NULL || tpfunc == NULL || tpmsg_cb == NULL || premote == NULL || ch_attr.max_msg_size <= 0))
    return LBERR_ARGV_WRONG;

  if (unlikely(!pstack->is_work()))
    return LBERR_OBJ_NOT_HAVE;

  int32 ret = 0;
  if (unlikely(!thctl.to_init()))
    return LBERR_OBJ_STATE_LIMIT;

  pzft = NULL;
  max_msglen = ch_attr.max_msg_size;
  if (max_msglen < 2048)
    max_msglen = 2048;
  recv_loop_num = ch_attr.recv_loop_num > 0 ? ch_attr.recv_loop_num : TCPDIR_RECV_LOOP_NUM;
  recv_len = 0;
  pmsg_cb = tpmsg_cb;
  pstk = pstack;
  pwait = NULL;
  errcode = 0;
  pfunc = tpfunc;
  puser_data = ch_attr.puser_data;
  heart.init(ch_attr.heart_interval);
  recv_buf = new (std::nothrow_t) char[ch_attr.max_msg_size];
  if (NULL == recv_buf) {
    thctl.end_init(false);
    return LBERR_MEM_ALLOC_FAIL;
  }
  struct zf_attr *tattr = zf_attr_dup(pstack->get_attr());
  if (NULL == tattr) {
    delete[] recv_buf;
    recv_buf = NULL;
    thctl.end_init(false);
    return LBERR_ATTR_SET_FAIL;
  }

  struct zft_handle *phand = NULL;
  // 分配 TCP zocket
  ret = zft_alloc(pstack->get_stack(), tattr, &phand);
  if (unlikely(ret < 0)) {
    delete[] recv_buf;
    recv_buf = NULL;
    zf_attr_free(tattr);
    thctl.end_init(false);
    return LBERR_OBJ_OPEN_FAIL;
  }

  // 转换地址并连接
  struct sockaddr_in loc_net;
  struct sockaddr_in rem_net;
  socklen_t tnet_len = sizeof(rem_net);
  sock_utils::addr_to_net(*premote, rem_net);
  if (NULL != plocal && plocal->ip[0] != '\0' && plocal->port > 0) {
    sock_utils::addr_to_net(*plocal, loc_net);
    ret = zft_addr_bind(phand, (const struct sockaddr *)(&loc_net), tnet_len, 0);
    if (unlikely(ret < 0)) {
      delete[] recv_buf;
      recv_buf = NULL;
      //zf_attr_free(tattr);
      zft_handle_free(phand);
      thctl.end_init(false);
      return LBERR_ATTR_SET_FAIL;
    }
  }

  ret = zft_connect(phand, (struct sockaddr *)(&rem_net), tnet_len, &pzft);
  if (ret < 0) {
    delete[] recv_buf;
    recv_buf = NULL;
    //zf_attr_free(tattr);
    zft_handle_free(phand);
    thctl.end_init(false);
    return LBERR_CH_CONNECT_FAIL;
  }

  ret = 0;
  if (asyn_connect == 0) {
    int32 try_times = 3000;
    while (zft_state(pzft) == TCP_SYN_SENT && try_times > 0) {
      comm_utils::sleep_us(100);
      try_times--;
    }
    if (zft_state(pzft) != TCP_ESTABLISHED) {
      destroy();
      thctl.end_init(false);
      return LBERR_CH_CONNECT_FAIL;
    }
  }

  if (zft_state(pzft) == TCP_ESTABLISHED) {
    struct sockaddr_in tlocal_addr;
    std::memset(&tlocal_addr, 0, sizeof(tlocal_addr));
    struct sockaddr_in tremote_addr;
    std::memset(&tremote_addr, 0, sizeof(tremote_addr));
    socklen_t tlocal_len = sizeof(tlocal_addr);
    socklen_t tremote_len = sizeof(tremote_addr);
    zft_getname(tn_ch, (struct sockaddr *)(&tlocal_addr), &tlocal_len, (struct sockaddr *)(&tremote_addr),
                &tremote_len);

    csock_addr to_addr;
    std::memset(&to_addr, 0, sizeof(to_addr));
    inet_ntop(AF_INET, &(tlocal_addr.sin_addr), to_addr.ip, sizeof(to_addr.ip));
    to_addr.port = ntohs(tlocal_addr.sin_port);
    if (NULL != pfunc) {
      pfunc->deal_ch_connect(this, to_addr);
    }
    ret = 1;
    thctl.end_init(true);
  }

  return ret;
}

bool tcpdir_ch::check_asyn_connected() {
  if (zft_state(pzft) == TCP_ESTABLISHED) {
    struct sockaddr_in tlocal_addr;
    std::memset(&tlocal_addr, 0, sizeof(tlocal_addr));
    struct sockaddr_in tremote_addr;
    std::memset(&tremote_addr, 0, sizeof(tremote_addr));
    socklen_t tlocal_len = sizeof(tlocal_addr);
    socklen_t tremote_len = sizeof(tremote_addr);
    zft_getname(tn_ch, (struct sockaddr *)(&tlocal_addr), &tlocal_len, (struct sockaddr *)(&tremote_addr),
                &tremote_len);

    csock_addr to_addr;
    std::memset(&to_addr, 0, sizeof(to_addr));
    inet_ntop(AF_INET, &(tlocal_addr.sin_addr), to_addr.ip, sizeof(to_addr.ip));
    to_addr.port = ntohs(tlocal_addr.sin_port);
    if (NULL != pfunc) {
      pfunc->deal_ch_connect(this, to_addr);
    }
    thctl.end_init(true);
    return true;
  } else if (zft_state(pzft) == TCP_SYN_SENT) {
    return false;
  } else {
    if (NULL != pfunc) {
      pfunc->deal_ch_error(this, AIO_CHERR_TYPE_CONNECT, LBERR_CH_CONNECT_FAIL);
    }
    thctl.end_init(false);
    destroy();
    if (NULL != pfunc)
      pfunc->deal_ch_closed(this, LBERR_CH_CONNECT_FAIL);
  }
  return false;
}

int32 tcpdir_ch::accept_ch(tcpdir_stack *pstack, tcpdir_ch_attr &ch_attr, tcpdir_msg_cb *tpmsg_cb, tcpdir_ch_op *tpfunc,
                           struct zft *pzft_accept) {
  if (unlikely(pstack == NULL || tpmsg_cb == NULL || pzft_accept == NULL))
    return LBERR_ARGV_WRONG;

  if (unlikely(!pstack->is_work()))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(!thctl.to_init()))
    return LBERR_OBJ_STATE_LIMIT;

  pzft = NULL;
  max_msglen = ch_attr.max_msg_size;
  if (max_msglen < 2048)
    max_msglen = 2048;
  recv_loop_num = ch_attr.recv_loop_num > 0 ? ch_attr.recv_loop_num : TCPDIR_RECV_LOOP_NUM;
  recv_len = 0;
  pmsg_cb = tpmsg_cb;
  pstk = pstack;
  pwait = NULL;
  errcode = 0;
  pfunc = tpfunc;
  puser_data = ch_attr.puser_data;
  heart.init(ch_attr.heart_interval);
  recv_buf = new (std::nothrow_t) char[ch_attr.max_msg_size];
  if (NULL == recv_buf) {
    thctl.end_init(false);
    return LBERR_MEM_ALLOC_FAIL;
  }

  thctl.end_init(true);
  return 0;
}

int32 tcpdir_ch::send_msg(char *pmsg, int32 len) {
  int32 ret = 0;
  if (thctl.is_work()) {
    ret = zft_send_single(pzft, (void *)pmsg, len, 0);
    if (likely(ret == len)) {
      return ret;
    } else if (ret == -EAGAIN || ret == -ENOMEM || ret == 0) {
      return 0;
    } else if (ret > 0) {
      return LBERR_OBJ_WRITE_PART;
    }
  }
  return LBERR_OBJ_STATE_LIMIT;
}
int32 tcpdir_ch::send_msg_fc(char *pmsg, int32 len) {
  int32 ret = 0;
  while (thctl.is_work()) {
    ret = zft_send_single(pzft, (void *)pmsg, len, 0);
    if (likely(ret == len)) {
      return ret;
    } else if (ret == -EAGAIN || ret == -ENOMEM || ret == 0) {
      continue;
    } else if (ret > 0) {
      return LBERR_OBJ_WRITE_PART;
    } else {
      break;
    }
  }
  return LBERR_OBJ_STATE_LIMIT;
}
int32 tcpdir_ch::send_msg_lock(char *pmsg, int32 len) {
  int32 ret = 0;
  if (pstk->thctl.send_lock()) {
    ret = send_msg(pmsg, len);
    pstk->thctl.send_unlock();
    return ret;
  }
  return LBERR_OBJ_STATE_LIMIT;
}

int32 tcpdir_ch::recv_msg(char *pbuf, int32 len) {
  int32 ret = 0;
  if (thctl.is_work()) {
    struct iovec iov = {(void *)(pbuf), (uint32)(len)};
    ret = zft_recv(pzft, &iov, 1, 0);
    if (likely(ret > 0)) {
      return ret;
    } else if (ret == -EAGAIN) {
      return 0;
    } else {
      return LBERR_OBJ_READ_FAIL;
    }
  }
  return LBERR_OBJ_STATE_LIMIT;
}

int32 tcpdir_ch::recv_msg_fc(char *pbuf, int32 len) {
  int32 ret = 0;
  int32 rlen = 0;
  while (thctl.is_work()) {
    struct iovec iov = {(void *)(pbuf + rlen), (uint32)(len - rlen)};
    ret = zft_recv(pzft, &iov, 1, 0);
    if (likely(ret > 0)) {
      rlen += ret;
      if (likely(rlen == len))
        return len;
      else if (rlen < len) {
        continue;
      } else {
        return LBERR_OBJ_READ_PART;
      }
    } else if (ret == -EAGAIN) {
      continue;
    } else {
      return LBERR_OBJ_READ_FAIL;
    }
  }
  return LBERR_OBJ_STATE_LIMIT;
}

int32 tcpdir_ch::recv_msg_lock(char *pbuf, int32 len) {
  int32 ret = 0;
  if (pstk->thctl.recv_lock()) {
    ret = recv_msg(pbuf, len);
    pstk->thctl.recv_unlock();
    return ret;
  }
  return LBERR_OBJ_STATE_LIMIT;
}

struct tcpdir_recv_chmsg {
  struct zft_msg msg;
  struct iovec endiov;
};

int32 tcpdir_ch::loop_deal_recv() {
  tcpdir_recv_chmsg tr_msg;
  int32 tr_num = 0;
  int32 loop_count = recv_loop_num;

  while (thctl.is_work()) {
    tr_msg.msg.iovcnt = 1;
    zft_zc_recv(pzft, &(tr_msg.msg), 0);
    tr_num = tr_msg.msg.iovcnt;
    if (tr_num == 0)
      return 0;

    for (int32 i = 0; i < tr_num; i++) {
      int32 rlen = tr_msg.msg.iov[i].iov_len;
      int32 ret = 0;
      if (recv_len == 0) {
        ret = pmsg_cb->deal_msg(this, (char *)(tr_msg.msg.iov[i].iov_base), rlen);
        if (likely(ret == rlen)) {
          continue;
        } else if (ret >= 0) {
          rlen -= ret;
          if (rlen < max_msglen) {
            std::memcpy(recv_buf, ((char *)(tr_msg.msg.iov[i].iov_base)) + ret, rlen);
            recv_len = rlen;
          } else {
            if (NULL != pfunc)
              pfunc->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, LBERR_OBJ_NUM_LIMIT);
            return LBERR_OBJ_NUM_LIMIT;
          }
        } else {
          if (NULL != pfunc)
            pfunc->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, ret);
          tr_num = loop_count;
          break;
        }
      } else {
        ret = max_msglen - recv_len;
        if (ret > rlen) {
          std::memcpy(recv_buf + recv_len, ((char *)(tr_msg.msg.iov[i].iov_base)), rlen);
          rlen += recv_len;
          ret = pmsg_cb->deal_msg(this, recv_buf, rlen);
          if (likely(ret == rlen)) {
            recv_len = 0;
          } else if (ret > 0) {
            rlen -= ret;
            std::memmove(recv_buf, recv_buf + ret, rlen);
            recv_len = rlen;
          } else if (ret == 0) {
            recv_len = rlen;
          } else {
            if (NULL != pfunc)
              pfunc->deal_ch_error(this, AIO
            if (NULL != pfunc)
              pfunc->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, ret);
            tr_num = loop_count;
            break;
          }
        } else if (recv_len + rlen < 65536) {
          char tbuf[65536];
          std::memcpy(tbuf, recv_buf, recv_len);
          std::memcpy(tbuf + recv_len, ((char *)(tr_msg.msg.iov[i].iov_base)), rlen);
          rlen += recv_len;
          ret = pmsg_cb->deal_msg(this, tbuf, rlen);
          if (likely(ret == rlen)) {
            recv_len = 0;
          } else if (ret >= 0) {
            rlen -= ret;
            if (rlen < max_msglen) {
              std::memcpy(recv_buf, tbuf + ret, rlen);
              recv_len = rlen;
            } else {
              if (NULL != pfunc)
                pfunc->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, LBERR_OBJ_NUM_LIMIT);
              return LBERR_OBJ_NUM_LIMIT;
            }
          } else {
            if (NULL != pfunc)
              pfunc->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, LBERR_OBJ_NUM_LIMIT);
            return LBERR_OBJ_NUM_LIMIT;
          }
        } else {
          if (NULL != pfunc)
            pfunc->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, LBERR_OBJ_NUM_LIMIT);
          return LBERR_OBJ_NUM_LIMIT;
        }
      }
    }

    loop_count -= tr_num;
    tr_num = zft_zc_recv_done(pzft, &(tr_msg.msg));
    if (tr_num == 0) {
      return 0;
    } else if (tr_num > 0 && loop_count > 0) {
      continue;
    } else if (tr_num > 0) {
      //pstk->poll_stack();
      return 1;
    } else {
      return LBERR_OBJ_READ_FAIL;
    }
  }
  return LBERR_OBJ_STATE_LIMIT;
}

int32 tcpdir_ch::loop_deal_recv_lock() {
  int32 loop_count = recv_loop_num;
  int32 ret = 0;
  int32 rlen = 0;
  while (thctl.is_work()) {
    rlen = max_msglen - recv_len;
    rlen = rlen < 2048 ? rlen : 2048;
    struct iovec iov = {(void *)(recv_buf + recv_len), (uint32)(rlen)};

    if (!pstk->thctl.recv_lock())
      break;
    ret = zft_recv(pzft, &iov, 1, 0);
    pstk->thctl.recv_unlock();
    if (likely(ret > 0)) {
      rlen = ret + recv_len;
      ret = pmsg_cb->deal_msg(this, recv_buf, rlen);
      if (likely(ret == rlen)) {
        recv_len = 0;
      } else if (ret > 0) {
        rlen -= ret;
        std::memmove(recv_buf, recv_buf + ret, rlen);
        recv_len = rlen;
      } else if (ret == 0) {
        recv_len = rlen;
      } else {
        if (NULL != pfunc)
          pfunc->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, ret);
        return LBERR_OBJ_MOD_FAIL;
      }
      loop_count--;
      if (loop_count == 0) {
        //pstk->poll_stack();
        return 1;
      }
    } else if (ret == -EAGAIN) {
      return 0;
    } else {
      return LBERR_OBJ_READ_FAIL;
    }
  }
  return LBERR_OBJ_STATE_LIMIT;
}

void tcpdir_ch::destroy() {
  if (pzft != NULL) {
    zft_free(pzft);
    pzft = NULL;
  }
  if (pwait != NULL) {
    pwait = NULL;
  }
  if (recv_buf != NULL) {
    delete[] recv_buf;
    recv_buf = NULL;
  }
  recv_len = 0;
}

void tcpdir_ch::close_ch(int32 err) {
  if (err != 0)
    errcode = err;
  int32 terr = errcode;

  if (thctl.to_close()) {
    if (NULL != pfunc)
      pfunc->deal_ch_closing(this, terr);
  }
}

void tcpdir_ch::check_close() {
  if (thctl.end_close()) {
    int32 terr = errcode;
    destroy();
    if (NULL != pfunc)
      pfunc->deal_ch_closed(this, terr);
  }
}

// ==================== tcpdir_listener 实现 ====================

int32 tcpdir_listener::listen_ch(tcpdir_stack *pstack, int32 once_loop_num, tcpdir_listener_op *tpfunc,
                                 csock_addr *plocal, void *puserdata) {
  if (unlikely(pstack == NULL || plocal == NULL))
    return LBERR_ARGV_WRONG;

  if (unlikely(!pstack->is_work()))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(!thctl.to_init()))
    return LBERR_OBJ_STATE_LIMIT;

  pstk = pstack;
  pzftl = NULL;
  pwait = NULL;
  accept_loop_num = once_loop_num;
  if (accept_loop_num < 2)
    accept_loop_num = 2;
  errcode = 0;
  local_addr.port = plocal->port;
  std::memcpy(local_addr.ip, plocal->ip, sizeof(local_addr.ip));
  pfunc = tpfunc;
  puser_data = puserdata;

  struct sockaddr_in taddr;
  taddr.sin_family = AF_INET;
  taddr.sin_port = htons(local_addr.port);
  taddr.sin_addr.s_addr = inet_addr(local_addr.ip);
  socklen_t tlen = sizeof(taddr);

  struct zf_attr *tattr = zf_attr_dup(pstack->get_attr());
  if (NULL == tattr) {
    thctl.end_init(false);
    return LBERR_ATTR_SET_FAIL;
  }

  // 分配 listener zocket
  int32 ret = zftl_listen(pstack->get_stack(), (const struct sockaddr *)(&taddr), tlen, tattr, &pzftl);
  if (unlikely(ret < 0)) {
    zf_attr_free(tattr);
    thctl.end_init(false);
    return LBERR_OBJ_OPEN_FAIL;
  }

  thctl.end_init(true);
  return 0;
}

int32 tcpdir_listener::accept_listen(struct zft *&o_zft, csock_addr &o_addr) {
  while (thctl.is_work()) {
    struct zft *tn_ch = NULL;
    int32 ret = zftl_accept(pzftl, &tn_ch);
    if (ret == 0) {
      struct sockaddr_in tlocal_addr;
      std::memset(&tlocal_addr, 0, sizeof(tlocal_addr));
      struct sockaddr_in tremote_addr;
      std::memset(&tremote_addr, 0, sizeof(tremote_addr));
      socklen_t tlocal_len = sizeof(tlocal_addr);
      socklen_t tremote_len = sizeof(tremote_addr);
      zft_getname(tn_ch, (struct sockaddr *)(&tlocal_addr), &tlocal_len, (struct sockaddr *)(&tremote_addr),
                  &tremote_len);

      if (NULL == inet_ntop(AF_INET, &(tremote_addr.sin_addr), o_addr.ip, sizeof(o_addr.ip))) {
        zft_free(tn_ch);
        return LBERR_ATTR_GET_FAIL;
      }
      o_addr.port = ntohs(tremote_addr.sin_port);
      o_zft = tn_ch;
      return 1;
    } else if (ret == EAGAIN) {
      return 0;
    } else {
      return LBERR_CH_ACCEPT_FAIL;
    }
  }
  return LBERR_OBJ_STATE_LIMIT;
}

int32 tcpdir_listener::loop_deal_accept() {
  int32 i = 0;
  while (i < accept_loop_num) {
    struct zft *tn_zft = NULL;
    csock_addr tremote_addr;
    std::memset((void *)(&tremote_addr), 0, sizeof(tremote_addr));
    int32 ret = accept_listen(tn_zft, tremote_addr);
    if (ret == 1) {
      i++;
      if (NULL != pfunc) {
        ret = pfunc->deal_listen_accept(this, tn_zft, tremote_addr);
        if (ret < 0) {
          zft_free(tn_zft);
        }
      }
    } else if (ret == 0) {
      return 0;
    } else {
      if (NULL != pfunc) {
        pfunc->deal_listen_error(this, ret);
      }
      return ret;
    }
  }
  return i;
}

void tcpdir_listener::destroy() {
  if (pzftl != NULL) {
    zftl_free(pzftl);
    pzftl = NULL;
  }
  if (pwait != NULL) {
    pwait = NULL;
  }
}

void tcpdir_listener::close_ch(int32 err) {
  if (err != 0)
    errcode = err;
  int32 terr = errcode;

  if (thctl.to_close()) {
    if (NULL != pfunc)
      pfunc->deal_listen_to_close(this, terr);
  }
}
void tcpdir_listener::check_close() {
  if (thctl.end_close()) {
    int32 terr = errcode;
    destroy();
    if (NULL != pfunc)
      pfunc->deal_listen_closed(this, terr);
  }
}

// ==================== tcpdir_poll 实现 ====================

int32 tcpdir_poll::init(tcpdir_stack *pstack, int32 tmax_events) {
  if (unlikely(pstack == NULL || !pstack->is_work()))
    return LBERR_ARGV_WRONG;

  if (unlikely(inited == 1))
    return LBERR_OBJ_HAVE_EXIST;

  pstk = pstack;
  max_events = tmax_events > 0 ? tmax_events : TCPDIR_POLL_MAX_EVENTS;
  if (max_events > TCPDIR_POLL_MAX_EVENTS)
    max_events = TCPDIR_POLL_MAX_EVENTS;

  int32 ret = zf_muxer_alloc(pstack, &pmuxer);
  if (ret < 0) {
    return LBERR_OBJ_OPEN_FAIL;
  }

  std::memset(events, 0, sizeof(events));
  inited = 1;
  return 0;
}

int32 tcpdir_poll::add_ch(tcpdir_ch *pch) {
  if (unlikely(inited == 0 || pstk == NULL))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(pch == NULL || pch->get_zft() == NULL))
    return LBERR_ARGV_WRONG;

  if (pch->pwait == NULL) {
    pch->pwait = zft_to_waitable(pch->get_zft());
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = tcpdir_poll_ev_tag::tag_ch(pch);

  int32 ret = zft_muxer_add(pmuxer, pch->pwait, &ev);
  if (ret < 0)
    return LBERR_OBJ_ADD_FAIL;

  return 0;
}

int32 tcpdir_poll::mod_ch(tcpdir_ch *pch) {
  if (unlikely(inited == 0 || pstk == NULL))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(pch == NULL || pch->get_zft() == NULL || pch->pwait == NULL))
    return LBERR_ARGV_WRONG;

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = tcpdir_poll_ev_tag::tag_ch(pch);

  int32 ret = zft_muxer_mod(pch->pwait, &ev);
  if (ret < 0)
    return LBERR_OBJ_MOD_FAIL;

  return 0;
}

int32 tcpdir_poll::remove_ch(tcpdir_ch *pch) {
  if (unlikely(inited == 0 || pstk == NULL))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(pch == NULL || pch->get_zft() == NULL || pch->pwait == NULL))
    return LBERR_ARGV_WRONG;

  int32 ret = zft_muxer_del(pch->pwait);
  if (ret < 0)
    return LBERR_OBJ_DEL_FAIL;

  return 0;
}

int32 tcpdir_poll::add_listener(tcpdir_listener *plistener) {
  if (unlikely(inited == 0 || pstk == NULL))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(plistener == NULL || plistener->get_zftl() == NULL))
    return LBERR_ARGV_WRONG;

  if (plistener->pwait == NULL) {
    plistener->pwait = zfl_to_waitable(plistener->get_zftl());
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = tcpdir_poll_ev_tag::tag_listener(plistener);

  int32 ret = zft_muxer_add(pmuxer, plistener->pwait, &ev);
  if (ret < 0)
    return LBERR_OBJ_ADD_FAIL;

  return 0;
}

int32 tcpdir_poll::mod_listener(tcpdir_listener *plistener) {
  if (unlikely(inited == 0 || pstk == NULL))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(plistener == NULL || plistener->get_zft() == NULL || plistener->pwait == NULL))
    return LBERR_ARGV_WRONG;

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = tcpdir_poll_ev_tag::tag_listener(plistener);

  int32 ret = zft_muxer_mod(plistener->pwait, &ev);
  if (ret < 0)
    return LBERR_OBJ_MOD_FAIL;

  return 0;
}

int32 tcpdir_poll::remove_listener(tcpdir_listener *plistener) {
  if (unlikely(inited == 0 || pstk == NULL))
    return LBERR_OBJ_NOT_HAVE;

  if (unlikely(plistener == NULL || plistener->get_zftl() == NULL || plistener->pwait == NULL))
    return LBERR_ARGV_WRONG;

  int32 ret = zft_muxer_del(plistener->pwait);
  if (ret < 0)
    return LBERR_OBJ_DEL_FAIL;

  return 0;
}

int32 tcpdir_poll::wait(int64 timeout_ns) {
  if (unlikely(inited == 0))
    return LBERR_OBJ_NOT_HAVE;
  int32 n = zf_muxer_wait(pmuxer, events, max_events, timeout_ns);
  for (int32 i = 0; i < n; i++) {
    void *pdata = ev.data.ptr;

    if (tcpdir_poll_ev_tag::is_ch(pdata)) {
      tcpdir_ch *pch = tcpdir_poll_ev_tag::to_ch(pdata);
      int32 ret = pch->loop_deal_recv();
      if (ret > 0) {
        mod_ch(pch);
      }
    } else {
      // 监听器事件: 接受新连接
      tcpdir_listener *pl = tcpdir_poll_ev_tag::to_listener(pdata);
      int32 ret = pl->loop_deal_accept();
      if (ret > 0) {
        mod_listener(pl);
      }
    }
  }

  return n;
}

void tcpdir_poll::destroy() {
  if (NULL != pmuxer) {
    zf_muxer_free(pmuxer);
    pmuxer = NULL;
  }
  pstk = NULL;
  max_events = 0;
  inited = 0;
}

} // namespace lb_common

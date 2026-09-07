// aio_socket_link - 基于 aio_tcp 的非阻塞 TCP 链接实现 (组合模式)
//
// 关键设计:
//   - 持有一个 aio_tcp 实例 (ch_), 通过 ch_.connect_ch / start_ch 驱动
//   - 实现 aio_ch_op<tcp_buf_ch> 接口, 作为 ch 事件回调桥接
//   - 内嵌 msg_cb (ch_recv_cb<tcp_buf_ch>), 消息回调直接转 counter.deal_recv_msg
//   - 业务线程不直接操作 ch_, 所有操作经"引擎 send_queue_ → 引擎线程 → link"
//
// aio_tcp 内部状态机:
//   IDLE -> CONNECTING (异步 connect) -> WORKING (建链成功, deal_ch_connect 回调)
//         |-> CLOSING/CLOSED (关闭, deal_ch_closing/closed 回调)
//   业务侧只关心 WORKING (is_work) 和 CLOSED (is_free/close), 用 check_reconnect 决策

#include "aio_socket_link.h"
#include "api_errno.h"
#include "comm_aio.h"
#include "comm_sock.h"
#include "mlog.h"
#include "reconnect_ctl.h"

#include <cstring>

namespace lb_api {

// ---- 消息回调: aio_tcp 收到完整消息时调用 ----
template <class TCounter>
int32 aio_socket_link<TCounter>::msg_cb::deal_msg(aio_socket_ch_t *pch, lb_common::aio_msg &msg) {
  // 直接将消息内容转给 counter 处理
  // NOTE: aio_tcp 的 deal_recv 会在 msg 处理完后自动 cmt_buf (zero_copy=0 时)
  return owner_->counter_->deal_recv_msg(msg.pmsg, msg.msglen, owner_->link_type_);
}

// ---- 构造 / 析构 ----
template <class TCounter>
aio_socket_link<TCounter>::aio_socket_link()
    : link_type_(0), once_recv_len_(0), check_interval_(0), counter_(nullptr), active_idx_(0), addr_valid_num(0),
      log_(nullptr) {
  std::memset(addrs_, 0, sizeof(addrs_));
  // 关联 msg_cb 与本 link (aio_tcp 回调时可通过 owner_ 找到外层 link)
  msg_cb_.owner_ = this;
  // aio_tcp 默认心跳间隔, 启动时由 init 阶段覆盖
  ch_.set_heart_interval(0);
}

template <class TCounter> aio_socket_link<TCounter>::~aio_socket_link() { close_ch(); }

// ---- init: 配置参数, 不连接 ----
template <class TCounter>
int32 aio_socket_link<TCounter>::init(lb_common::lb_log *tlog, TCounter *tfst, int16 link_type, int32 once_recv_len,
                                      int32 check_interval, int32 max_same_addr_fails) {
  ch_.set_heart_interval(check_interval);
  counter_ = tfst;
  link_type_ = link_type;
  switch_reconn_num = static_cast<uint16>(max_same_addr_fails); ///< 重连失败次数，切换地址
  once_recv_len_ = once_recv_len;
  check_interval_ = check_interval;
  ch_.set_heart_interval(check_interval);
  active_idx_ = 0;
  addr_valid_num = 0;
  reconn_.init(0);
  std::memset(addrs_, 0, sizeof(addrs_));
  log_ = tlog;
  msg_cb_.owner_ = this;

  return 0;
}

// ---- set_remote: 设置主地址 ----
template <class TCounter> int32 aio_socket_link<TCounter>::set_remote(const lb_common::csock_addr &remote) {
  if (addr_valid_num < 2) {
    addrs_[addr_valid_num] = remote;
    addr_valid_num++;
    if (addr_valid_num == 1) {
      reconn_.init(0);
    } else {
      reconn_.init(switch_reconn_num);
    }
    return 0;
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
}

// ---- 构造 aio_attr / channel_attr, 用于 ch_.connect_ch ----
template <class TCounter>
void aio_socket_link<TCounter>::build_aio_attr_(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr,
                                                int32 recv_pool_num) {
  std::memset(&buf_attr, 0, sizeof(buf_attr));
  std::memset(&ch_attr, 0, sizeof(ch_attr));
  // aio_attr: 接收循环 + 心跳 + 用户数据
  buf_attr.onerecvtimes = recv_pool_num; // 一次 epoll 唤醒最多 recv 次数
  buf_attr.oncerecvlen = once_recv_len_; // 单次接收长度
  buf_attr.maxmsglen = 1 * 1024 * 1024;  // 最大消息长度
  buf_attr.buf_size = 2 * 1024 * 1024;   // 接收缓冲 4MB
  buf_attr.dispatch_zero_copy = 0;       // 关闭零拷贝 (简化处理)
  buf_attr.heartinterval = check_interval_;
  buf_attr.puserdata = reinterpret_cast<void *>(this); // 业务数据为 link 自身
  // channel_attr: 系统 socket 选项
  ch_attr.family = 1; // IPv4 (CHANNEL_FAMILY_IPV4)
  ch_attr.recvsockbuflen = 1 * 1024 * 1024;
  ch_attr.sendsockbuflen = 1 * 1024 * 1024;
  ch_attr.tcpdelayack = 1;  // 禁用 Nagle (低延迟)
  ch_attr.sendrecvtime = 5; // 收发超时 5 秒
  ch_attr.localloop = 0;
}

// ---- connect: 重连 (被 link_timer_op 通过 check_reconnect 触发) ----
template <class TCounter>
int32 aio_socket_link<TCounter>::connect(int32 recv_pool_num, int32 need_switch, lb_common::mthread *recv_th) {

  lb_common::lb_log_hand tlh(log_);

  if (addr_valid_num == 0) {
    error_log(tlh) << "socket link remote addr error,link_type=" << link_type_ << ",remote_ip=" << addrs_[0].ip
                   << ",remote_port=" << addrs_[0].port << end_log;
    return LBAPI_ERR_LINK_ADDR;
  }

  if (ch_.is_work()) {
    return 0;
  } else if (!ch_.is_free()) {
    warning_log(tlh) << "socket link repeated,link_type=" << link_type_ << ",active_link=" << active_idx_
                     << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port
                     << end_log;
    return LBAPI_ERR_LINK_CONNECT_REPEAT;
  }

  int32 have_switch = 0;
  if (need_switch != 0 && addr_valid_num == 2) {
    active_idx_ = 1 - active_idx_; // 切换到备地址
    have_switch = 1;
  }
  if (addrs_[active_idx_].ip[0] == '\0' || addrs_[active_idx_].port == 0) {
    if (addr_valid_num == 2) {
      active_idx_ = 1 - active_idx_;
    }
    if (need_switch != 0) {
      have_switch = 0;
    }
  }

  if (addrs_[active_idx_].ip[0] == '\0' || addrs_[active_idx_].port == 0) {
    error_log(tlh) << "socket link remote addr error,link_type=" << link_type_ << ",active_link=" << active_idx_
                   << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port << end_log;
    return LBAPI_ERR_LINK_ADDR;
  }

  lb_common::aio_attr buf_attr;
  lb_common::channel_attr ch_attr;
  build_aio_attr_(buf_attr, ch_attr, recv_pool_num);

  int32 ret = ch_.connect_ch(buf_attr, ch_attr, &msg_cb_, this, recv_th,
                             const_cast<lb_common::csock_addr *>(&active_addr_()), nullptr, 0);

  if (ret < 0 && need_switch == 0 && addr_valid_num == 2) {
    // 主地址未连通, 自动尝试备地址, 此为 link 内部重试, 不算 caller 请求的切换
    active_idx_ = 1 - active_idx_; // 切换到备地址
    if (addrs_[active_idx_].ip[0] != '\0' && addrs_[active_idx_].port > 0) {
      ret = ch_.connect_ch(buf_attr, ch_attr, &msg_cb_, this, recv_th,
                           const_cast<lb_common::csock_addr *>(&active_addr_()), nullptr, 0);
    }
  }

  if (ret == 1) {
    reconn_.set_connected();
    info_log(tlh) << "socket link connect ok,link_type=" << link_type_ << ",active_link=" << active_idx_
                  << ",have_switch=" << have_switch << ",remote_ip=" << addrs_[active_idx_].ip
                  << ",remote_port=" << addrs_[active_idx_].port << end_log;
    if (NULL != recv_th) {
      ret = ch_.start_ch();
      if (ret < 0) {
        ch_.close_ch(ret);
        error_log(tlh) << "start socket link recv error,link_type=" << link_type_ << ",active_link=" << active_idx_
                       << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port
                       << ",ret=" << ret << end_log;

        return LBAPI_ERR_LINK_START_RECV;
      }
    }
    ret = counter_->deal_link_connect(link_type_, have_switch);
    if (ret < 0) {
      ch_.close_ch(ret);
      error_log(tlh) << "counter deal link connect error,link_type=" << link_type_ << ",active_link=" << active_idx_
                     << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port
                     << ",ret=" << ret << end_log;

      return ret;
    }
    return 0;
  } else {
    error_log(tlh) << "socket link connect error,link_type=" << link_type_ << ",active_link=" << active_idx_
                   << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port
                   << ",ret=" << ret << end_log;
  }
  return LBAPI_ERR_LINK_CONNECT;
}

// ---- close_ch: 关闭链接 ----
template <class TCounter> void aio_socket_link<TCounter>::close_ch(int32 err) { ch_.close_ch(err); }

// ---- deal_ch_*: aio_ch_op 虚函数实现 (aio_tcp 在 ch 事件时回调) ----

template <class TCounter>
void aio_socket_link<TCounter>::deal_ch_error(aio_socket_ch_t *pch, int32 err_type, int32 err_code) {
  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "get socket link error to close,link_type=" << link_type_ << ",active_link=" << active_idx_
                 << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port
                 << ",err_type=" << err_type << ",err_code=" << err_code << end_log;

  ch_.close_ch(err_code);
}

template <class TCounter> void aio_socket_link<TCounter>::deal_ch_closing(aio_socket_ch_t *pch, int32 errcode) {
  lb_common::lb_log_hand tlh(log_);
  warning_log(tlh) << "socket link is closing,link_type=" << link_type_ << ",active_link=" << active_idx_
                   << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port << end_log;
}

template <class TCounter> void aio_socket_link<TCounter>::deal_recv_stop(aio_socket_ch_t *pch) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "socket link recv stop,link_type=" << link_type_ << ",active_link=" << active_idx_
                << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port << end_log;
}

template <class TCounter> void aio_socket_link<TCounter>::deal_ch_closed(aio_socket_ch_t *pch, int32 errcode) {
  // 物理关闭: 通知 counter, 由 link_timer_op 驱动重连 (D28)
  // 重置重连计数器
  counter_->deal_link_close(link_type_);
  reconn_.set_disconnected();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "socket link closed end,link_type=" << link_type_ << ",active_link=" << active_idx_
                << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port << end_log;
}

template <class TCounter>
void aio_socket_link<TCounter>::deal_ch_connect(aio_socket_ch_t *pch, lb_common::csock_addr &localaddr) {
  // 同步链接，不处理
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "socket link connected,link_type=" << link_type_ << ",active_link=" << active_idx_
                << ",remote_ip=" << addrs_[active_idx_].ip << ",remote_port=" << addrs_[active_idx_].port
                << ",local_ip=" << localaddr.ip << ",local_port=" << localaddr.port << end_log;
}

} // namespace lb_api

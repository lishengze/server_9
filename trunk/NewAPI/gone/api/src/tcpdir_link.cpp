// tcpdir_link - 基于 tcpdir_ch 的 Solarflare TCPDirect 链接实现 (组合模式)
//
// 关键设计:
//   - 持有一个 tcpdir_ch (ch_) + tcpdir_stack (stack_), 自包含 TCPDirect 资源
//   - 实现 tcpdir_ch_op 接口, 作为 ch 事件回调桥接
//   - 内嵌 msg_cb (tcpdir_msg_cb), 消息回调直接转 counter.deal_recv_msg
//   - 业务线程不直接操作 ch_, 所有操作经"引擎 send_queue_ → 引擎线程 → link"
//
// tcpdir_ch 内部状态机:
//   IDLE -> CONNECTING (异步) -> WORKING (deal_ch_connect 回调)
//         |-> CLOSING/CLOSED (deal_ch_closing/closed 回调)
//   业务侧只关心 WORKING (is_work) 和 CLOSED (is_free/close), 用 check_reconnect 决策
//
// 与 aio_socket_link 差异:
//   - 不使用 mthread (TCPDirect 自带 zf_mux_wait, 由 tcpdir_poll 或循环 loop_deal_recv 驱动)
//   - 自带 stack 需 init_stack (需 Solarflare 网卡名)
//   - 单一地址 (无备地址)

#include "tcpdir_link.h"
#include "api_errno.h"
#include "comm_sock.h"
#include "mlog.h"
#include "reconnect_ctl.h"
#include "tcpdir_stack.h"

#include <cstring>

namespace lb_api {

// ---- 消息回调: tcpdir_ch 收到完整消息时调用 ----
template <class TFastCounter>
int32 tcpdir_link<TFastCounter>::msg_cb::deal_msg(lb_common::tcpdir_ch *pch, char *pbuf, int32 len) {
  return owner_->counter_->deal_recv_msg(pbuf, len, owner_->link_type_);
}

// ---- 构造 / 析构 ----
template <class TFastCounter>
tcpdir_link<TFastCounter>::tcpdir_link()
    : counter_(nullptr), link_type_(0), check_interval_(0), once_recv_len_(0), log_(nullptr) {
  std::memset(&addrs_, 0, sizeof(addrs_));
  // 关联 msg_cb 与本 link
  msg_cb_.owner_ = this;
}

template <class TFastCounter> tcpdir_link<TFastCounter>::~tcpdir_link() { close_ch(); }

// ---- init: 配置参数, 暂不连接 (start_recv 时才 init stack + connect) ----
template <class TFastCounter>
int32 tcpdir_link<TFastCounter>::init(lb_common::lb_log *tlog, TFastCounter *tfst, const char *solarflare_iface,
                                      int16 link_type, int32 once_recv_len, int32 check_interval,
                                      int32 max_same_addr_fails) {
  if (nullptr == solarflare_iface || solarflare_iface[0] == '\0') {
    return LBAPI_ERR_NO_SOLARFLARE;
  }

  ch_.set_heart_interval(check_interval);
  counter_ = tfst;
  link_type_ = link_type;
  check_interval_ = static_cast<uint16>(check_interval);
  once_recv_len_ = once_recv_len;
  reconn_.init(0);
  std::memset(&addrs_, 0, sizeof(addrs_));
  log_ = tlog;

  lb_common::lb_log_hand tlh(tlog);

  // 配置 stack 属性
  lb_common::tcpdir_stack_attr sattr;
  std::memset(&sattr, 0, sizeof(sattr));
  std::strncpy(sattr.interface_name, solarflare_iface, sizeof(sattr.interface_name) - 1);
  sattr.mode = TCPDIR_MODE_LOW_LATENCY; // 低延迟模式 (全局宏)
  sattr.independ_send = 0;              ///< 独立的带锁发送模式，0 默认
  sattr.max_connect_num = 4;            ///< 单个stack 最大支持的链接数，<=64
  sattr.rx_ring_size = 2048;            ///< 接收环大小，0 使用默认值
  sattr.tx_ring_size = 1024;            ///< 发送环大小，0 使用默认值

  // 初始化 stack (在 start_recv 前也允许这里 init, 这样 close_ch 时能析构)
  int32 ret = stack_.init_stack(sattr);
  if (ret == 0) {
    info_log(tlh) << "init tcpdirect link stack ok,link_type=" << link_type_ << ",solarflare_iface=" << solarflare_iface
                  << end_log;
    return 0;
  } else {
    error_log(tlh) << "init tcpdirect link stack error,link_type=" << link_type_
                   << ",solarflare_iface=" << solarflare_iface << ",ret=" << ret << end_log;
    return LBAPI_ERR_LINK_INIT;
  }
}

// ---- set_remote: 设置单一地址 ----
template <class TFastCounter> int32 tcpdir_link<TFastCounter>::set_remote(const lb_common::csock_addr &remote) {
  addrs_ = remote;
  return 0;
}

// ---- 构造 ch_attr, 用于 ch_.connect_ch ----
template <class TFastCounter>
void tcpdir_link<TFastCounter>::build_ch_attr_(lb_common::tcpdir_ch_attr &ch_attr, int32 recv_pool_num) {
  std::memset(&ch_attr, 0, sizeof(ch_attr));
  ch_attr.heart_interval = check_interval_;
  ch_attr.recv_loop_num = recv_pool_num;
  ch_attr.max_msg_size = 2 * 1024 * 1024;
  ch_attr.puser_data = reinterpret_cast<void *>(this);
}

// ---- connect: 重连 (被 link_timer_op 通过 check_reconnect 触发) ----
template <class TFastCounter>
int32 tcpdir_link<TFastCounter>::connect(int32 recv_pool_num, int32 need_switch, lb_common::mthread *recv_th) {
  lb_common::lb_log_hand tlh(log_);

  if (addrs_.ip[0] == '\0' || addrs_.port == 0) {
    error_log(tlh) << "tcpdirect link remote addr error,link_type=" << link_type_ << ",remote_ip=" << addrs_.ip
                   << ",remote_port=" << addrs_.port << end_log;
    return LBAPI_ERR_LINK_ADDR;
  }

  if (ch_.is_work()) {
    return 0;
  } else if (!ch_.is_free()) {
    warning_log(tlh) << "tcpdirect link repeated,link_type=" << link_type_ << ",remote_ip=" << addrs_.ip
                     << ",remote_port=" << addrs_.port << end_log;
    return LBAPI_ERR_LINK_CONNECT_REPEAT;
  }

  lb_common::tcpdir_ch_attr ch_attr;
  build_ch_attr_(ch_attr);

  int32 ret = ch_.connect_ch(&stack_, ch_attr, &msg_cb_, this, &addrs_, nullptr, 0);
  if (ret == 1) {
    reconn_.set_connected();
    // tcpdir_link 不支持备地址, have_switch 永远为 0
    info_log(tlh) << "tcpdirect link connect ok,link_type=" << link_type_ << ",have_switch=0"
                  << ",remote_ip=" << addrs_.ip << ",remote_port=" << addrs_.port << end_log;
    ret = counter_->deal_link_connect(link_type_, 0);
    if (ret < 0) {
      ch_.close_ch(ret);
      error_log(tlh) << "counter deal tcpdirect link connect error,link_type=" << link_type_
                     << ",remote_ip=" << addrs_.ip << ",remote_port=" << addrs_.port << ",ret=" << ret << end_log;

      return ret;
    }
    return 0;
  } else {
    error_log(tlh) << "tcpdirect link connect error,link_type=" << link_type_ << ",remote_ip=" << addrs_.ip
                   << ",remote_port=" << addrs_.port << ",ret=" << ret << end_log;
  }
  return LBAPI_ERR_LINK_CONNECT;
}

// ---- close_ch: 关闭链接 ----
template <class TFastCounter> void tcpdir_link<TFastCounter>::close_ch(int32 err) { ch_.close_ch(err); }

// ---- deal_ch_*: tcpdir_ch_op 虚函数实现 (tcpdir_ch 在 ch 事件时回调) ----

template <class TFastCounter>
void tcpdir_link<TFastCounter>::deal_ch_error(lb_common::tcpdir_ch *pch, int32 err_type, int32 err_code) {
  lb_common::lb_log_hand tlh(log_);
  error_log(tlh) << "get tcpdirect link error to close,link_type=" << link_type_ << ",remote_ip=" << addrs_.ip
                 << ",remote_port=" << addrs_.port << ",err_type=" << err_type << ",err_code=" << err_code << end_log;

  ch_.close_ch(err_code);
}

template <class TFastCounter>
void tcpdir_link<TFastCounter>::deal_ch_closing(lb_common::tcpdir_ch *pch, int32 errcode) {
  lb_common::lb_log_hand tlh(log_);
  warning_log(tlh) << "tcpdirect link is closing,link_type=" << link_type_ << ",remote_ip=" << addrs_.ip
                   << ",remote_port=" << addrs_.port << end_log;
}

template <class TFastCounter> void tcpdir_link<TFastCounter>::deal_ch_closed(lb_common::tcpdir_ch *pch, int32 errcode) {
  // 物理关闭: 通知 counter, 由 link_timer_op 驱动重连 (D28)
  // 重置重连计数器
  counter_->deal_link_close(link_type_);
  reconn_.set_disconnected();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "tcpdirect link closed end,link_type=" << link_type_ << ",remote_ip=" << addrs_.ip
                << ",remote_port=" << addrs_.port << end_log;
}

template <class TFastCounter>
void tcpdir_link<TFastCounter>::deal_ch_connect(lb_common::tcpdir_ch *pch, lb_common::csock_addr &localaddr) {
  // 同步链接，不处理
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "tcpdirect link connected,link_type=" << link_type_ << ",remote_ip=" << addrs_.ip
                << ",remote_port=" << addrs_.port << ",local_ip=" << localaddr.ip << ",local_port=" << localaddr.port
                << end_log;
}

} // namespace lb_api

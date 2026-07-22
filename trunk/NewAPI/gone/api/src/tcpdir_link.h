// tcpdir_link - 基于 tcpdir_ch 的 Solarflare TCPDirect 链接 (组合模式, 薄包装)
//
// 设计原则:
//   1. 组合而非继承 (tcpdir_ch 不可继承)
//   2. 自身是 tcpdir_ch_op, 作为 ch 事件回调
//   3. 持有内嵌 msg_cb (tcpdir_msg_cb), 消息回调直接转 counter.deal_recv_msg
//   4. 拥有 ch_ (tcpdir_ch) 实例 + stack_ (tcpdir_stack), 完整自包含
//   5. 链接生命周期: init -> set_remote -> start_recv (connect+stack init) -> ... -> close_ch
//
// 数据流:
//   引擎 do_work -> ch_.loop_deal_recv() -> msg_cb.deal_msg(this, buf, len) ->
//     counter.deal_recv_msg(buf, len, link_type)
//
// 与 aio_socket_link 关键差异:
//   - 不使用 mthread epoll (Solarflare TCPDirect 自带 zf_mux_wait, 由 tcpdir_poll 驱动)
//   - 单一地址 (无备地址), set_remote_ex 为 no-op
//   - 自带 tcpdir_stack (Solarflare 网卡 stack), 需在 init 阶段 init_stack
//   - 引擎线程通过循环调 ch_.loop_deal_recv() 驱动接收

#pragma once

#include "comm_sock.h"
#include "comm_sys.h"
#include "mlog.h"
#include "mthread.h"
#include "reconnect_ctl.h"
#include "tcpdir_stack.h"

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// tcpdir_link - 基于 tcpdir_ch 的 TCPDirect 链接 (模板类, 薄包装)
/// @tparam TFastCounter  极速柜台类型
template <class TFastCounter> class tcpdir_link : public lb_common::tcpdir_ch_op {
public:
  /// 内部消息接收回调 (由 tcpdir_ch 调用 deal_msg 回调, 通过 owner_ 找到外层 link)
  class msg_cb : public lb_common::tcpdir_msg_cb {
  public:
    tcpdir_link<TFastCounter> *owner_ = nullptr;

    /// tcpdir_ch 回调: 收到完整消息时调用, 转发到 counter.deal_recv_msg
    /// @param pch  tcpdir_ch 指针
    /// @param pbuf 消息缓冲区 (指向内部 recv_buf, 下次 recv 会被覆盖)
    /// @param len  消息长度
    /// @return 已处理字节数
    int32 deal_msg(lb_common::tcpdir_ch *pch, char *pbuf, int32 len) override;
  };

  friend class msg_cb;

  tcpdir_link();
  ~tcpdir_link();

  /// 禁用拷贝
  tcpdir_link(const tcpdir_link &) = delete;
  tcpdir_link &operator=(const tcpdir_link &) = delete;

  /// 链接初始化 (地址通过 set_remote 单独设置)
  /// @param tlog               api 实例日志指针
  /// @param tfst               所属柜台指针
  /// @param solarflare_iface   solarflare 网卡名
  /// @param link_type          链接类型 (tcpdirect_engine 固定为 LINK_TYPE_SPEED_TRADE)
  /// @param once_recv_len      一次调用recv 的输入长度，不同柜台和链接数值不同，建议 max(委托推送，成交推送)*N(2,3,4)
  /// @param check_interval     心跳,重连间隔 (秒, 0=不启用)
  /// @param max_same_addr_fails 重连失败次数，切换地址
  int32 init(lb_common::lb_log *tlog, TFastCounter *tfst, const char *solarflare_iface, int16 link_type,
             int32 once_recv_len, int32 check_interval, int32 max_same_addr_fails = 0);

  /// 设置主地址 (tcpdir_link 单一地址)
  int32 set_remote(const lb_common::csock_addr &remote);

  /// 重连 (被 link_timer_op 通过 check_reconnect 触发)
  /// @param need_switch  0=不切地址, 1=切换到备地址 (tcpdir_link 不支持, 仅占位)
  /// @return 0 成功发起重连, 负数错误码
  int32 connect(int32 recv_pool_num, int32 need_switch = 0, lb_common::mthread *recv_th = NULL);

  /// 关闭链接
  void close_ch(int32 err = 0);

  /// 发送数据
  FORCE_INLINE int32 send_msg(char *buf, int32 len) { return ch_.send_msg_fc(buf, len); }

  /// 处理接收 (供引擎 do_work 循环调用)
  FORCE_INLINE int32 deal_recv() { return ch_.loop_deal_recv(); }

  /// 获取链接类型
  FORCE_INLINE int16 get_link_type() const { return link_type_; }

  /// 是否处于工作状态
  FORCE_INLINE bool is_work() const { return ch_.is_work(); }

  FORCE_INLINE bool is_close() const { return ch_.is_close(); }

  FORCE_INLINE bool is_free() const { return ch_.is_free(); }

  FORCE_INLINE int32 get_heart_interval() { return static_cast<int32>(check_interval_); }

  FORCE_INLINE void deal_heart_ans() { ch_.on_heart_msg(); }

  // ---- 定时器检查 (供 link_timer_op 调用) ----
  FORCE_INLINE bool check_heart_send() {
    if (ch_.is_work())
      return ch_.check_heart_send();
    return false;
  }
  FORCE_INLINE bool check_heart_timeout() {
    if (ch_.is_work())
      return ch_.check_heart_timeout();
    return false;
  }
  bool check_reconnect(int32 &o_need_switch) {
    o_need_switch = 0;
    if (!ch_.is_free() || addrs_.port <= 0 || addrs_.ip[0] == '\0') {
      return false;
    }
    return reconn_.check_reconnect(check_interval_);
  }

  // ---- tcpdir_ch_op 直接实现 ----

  void deal_ch_connect(lb_common::tcpdir_ch *pch, lb_common::csock_addr &localaddr) override;
  void deal_ch_closing(lb_common::tcpdir_ch *pch, int32 errcode) override;
  void deal_ch_closed(lb_common::tcpdir_ch *pch, int32 errcode) override;
  void deal_ch_error(lb_common::tcpdir_ch *pch, int32 errtype, int32 errcode) override;

private:
  /// 构造 ch_attr, 用于 ch_.connect_ch
  void build_ch_attr_(lb_common::tcpdir_ch_attr &ch_attr, int32 recv_pool_num);

  lb_common::tcpdir_ch ch_;         ///< 底层 TCPDirect 通道 (owned)
  lb_common::tcpdir_stack stack_;   ///< TCPDirect stack (owned, 自包含)
  msg_cb msg_cb_;                   ///< 消息接收回调 (owned, 由 ch_ 调用)
  int16 link_type_;                 ///< 链接类型 (LINK_TYPE_*)
  uint16 check_interval_;           ///< 心跳和重连间隔(秒)
  int32 once_recv_len_;             ///< 一次调用recv 的输入长度
  TFastCounter *counter_;           ///< 所属柜台指针
  lb_common::reconnect_ctl reconn_; ///< 重连控制器
  lb_common::csock_addr addrs_;     ///< 单一地址
  lb_common::lb_log *log_;          ///< 日志指针
};

} // namespace lb_api

// aio_socket_link - 基于 aio_tcp 的非阻塞 TCP 链接 (组合模式, 薄包装)
//
// 设计原则:
//   1. 组合而非继承 (aio_tcp 是 final, 不能继承)
//   2. 自身是 aio_ch_op<tcp_buf_ch>, 作为 ch 事件回调
//   3. 持有内嵌 msg_cb (ch_recv_cb<tcp_buf_ch>), 作为消息回调
//   4. 拥有 ch_ (aio_tcp) 实例, 通过 ch_.connect_ch/start_ch 完成 IO
//   5. 链接生命周期: init -> set_remote -> start_recv (connect+start) -> ... -> close_ch
//
// 数据流:
//   mthread epoll -> aio_tcp::deal_event -> loop_deal_recv ->
//     msg_cb::deal_msg(this, aio_msg) -> counter.deal_recv_msg(msg.pmsg, msg.msglen, link_type)
//
// 地址管理:
//   addrs_[0] = primary, addrs_[1] = secondary
//   fast 单 socket / shared 等只用 [0]; 98 / fpga_gw 用 [0]+[1]
//   故障切换通过 check_reconnect(bool &o_need_switch) 决策, 内部切换 active_idx_

#pragma once

#include "aio_tcp.h"
#include "comm_aio.h"
#include "comm_sock.h"
#include "comm_sys.h"
#include "mlog.h"
#include "mthread.h"
#include "reconnect_ctl.h"

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// 通道类型别名
using aio_socket_ch_t = lb_common::tcp_buf_ch;

/// aio_socket_link - 基于 aio_tcp 的非阻塞 TCP 链接 (模板类, 薄包装)
/// @tparam TCounter  柜台类型
template <class TCounter> class aio_socket_link : public lb_common::aio_ch_op<aio_socket_ch_t> {
public:
  /// 内部消息回调 (由 aio_tcp 在收到完整消息时回调, 通过 owner_ 找到外层 link)
  class msg_cb : public lb_common::ch_recv_cb<aio_socket_ch_t> {
  public:
    aio_socket_link<TCounter> *owner_ = nullptr;

    /// aio_tcp 回调: 收到完整消息时调用, 转发到 counter.deal_recv_msg
    /// @param pch  aio_tcp 指针 (cast 为 aio_tcp<...>* 后可访问内部, 但本回调不直接用)
    /// @param msg  消息内容 (pmsg, msglen, buf_addr)
    /// @return >0 已处理字节数; 0 不处理 (应避免); <0 错误
    int32 deal_msg(aio_socket_ch_t *pch, lb_common::aio_msg &msg) override;
  };

  aio_socket_link();
  ~aio_socket_link();

  friend class msg_cb;

  /// 禁用拷贝
  aio_socket_link(const aio_socket_link &) = delete;
  aio_socket_link &operator=(const aio_socket_link &) = delete;

  /// 链接初始化 (地址通过 set_remote 单独设置)
  /// @param tlog               api 实例日志指针
  /// @param tfst               所属柜台指针
  /// @param link_type          链接类型 (tcpdirect_engine 固定为 LINK_TYPE_SPEED_TRADE)
  /// @param once_recv_len      一次调用recv 的输入长度，不同柜台和链接数值不同，建议 max(委托推送，成交推送)*N(2,3,4)
  /// @param check_interval     心跳,重连间隔 (秒, 0=不启用)
  /// @param max_same_addr_fails 重连失败次数，切换地址
  int32 init(lb_common::lb_log *tlog, TCounter *tfst, int16 link_type, int32 once_recv_len, int32 check_interval,
             int32 max_same_addr_fails = 0);

  /// 设置主地址 (必须在 start_recv 前调用),最多调用两次，传入不同地址
  int32 set_remote(const lb_common::csock_addr &remote);

  /// 重连 (被 link_timer_op 通过 check_reconnect 触发)
  /// @param need_switch  0=不切地址, 1=切换地址
  /// @return 0 成功发起重连, 负数错误码
  int32 connect(int32 recv_pool_num, int32 need_switch = 0, lb_common::mthread *recv_th = NULL);

  /// 关闭链接
  void close_ch(int32 err = 0);

  /// 发送数据
  FORCE_INLINE int32 send_msg(char *buf, int32 len) { return ch_.send_msg_fc(buf, len); }

  /// 接收处理 (供业务线程在无 mthread 时手动驱动, 正常情况下由 mthread epoll 驱动)
  FORCE_INLINE int32 deal_recv() {
    ch_.loop_deal_recv();
    return 0;
  }

  /// 获取链接类型
  FORCE_INLINE int16 get_link_type() const { return link_type_; }

  /// 是否处于工作状态
  FORCE_INLINE bool is_work() const { return ch_.is_work(); }

  FORCE_INLINE bool is_close() const { return ch_.is_close(); }

  FORCE_INLINE bool is_free() const { return ch_.is_free(); }

  FORCE_INLINE int32 get_heart_interval() { return static_cast<int32>(check_interval_); }

  FORCE_INLINE void deal_heart_ans() { ch_.on_heart_msg(); }

  // ---- 定时器检查 (供 link_timer_op 调用) ----
  /// 检查是否需要发送心跳
  FORCE_INLINE bool check_heart_send() {
    if (ch_.is_work())
      return ch_.check_heart_send();
    return false;
  }
  /// 检查心跳是否超时
  FORCE_INLINE bool check_heart_timeout() {
    if (ch_.is_work())
      return ch_.check_heart_timeout();
    return false;
  }
  /// 检查是否需要重连
  /// @param[out] o_need_switch  1 表示需要切换到备地址
  /// @return true 需要重连, false 不需要
  bool check_reconnect(int32 &o_need_switch) {
    o_need_switch = 0;
    if (!ch_.is_free() || addrs_[0].port <= 0 || addrs_[0].ip[0] == '\0') {
      return false;
    }
    if (reconn_.check_reconnect(static_cast<uint16>(check_interval_))) {
      return true;
    }
    if (addr_valid_num == 2) {
      reconn_.reset_reconnect_count();
      if (reconn_.check_reconnect(static_cast<uint16>(check_interval_))) {
        o_need_switch = 1;
        return true;
      }
    }
    return false;
  }

  // ---- aio_ch_op<tcp_buf_ch> 虚函数实现 (aio_tcp 在 ch 事件时回调) ----

  void deal_ch_error(aio_socket_ch_t *pch, int32 err_type, int32 err_code) override;
  void deal_ch_closing(aio_socket_ch_t *pch, int32 errcode) override;
  void deal_recv_stop(aio_socket_ch_t *pch) override;
  void deal_ch_closed(aio_socket_ch_t *pch, int32 errcode) override;
  void deal_ch_connect(aio_socket_ch_t *pch, lb_common::csock_addr &localaddr) override;

private:
  /// 获取当前活动地址 (内部用)
  const lb_common::csock_addr &active_addr_() const { return addrs_[active_idx_]; }

  /// 构造 aio_attr + channel_attr, 用于 ch_.connect_ch
  void build_aio_attr_(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr, int32 recv_pool_num);

  aio_socket_ch_t ch_;              ///< 底层非阻塞 TCP 通道 (owned, 唯一)
  msg_cb msg_cb_;                   ///< 消息接收回调 (owned, 由 ch_ 调用)
  TCounter *counter_;               ///< 所属柜台指针 (init 阶段注入)
  int16 link_type_;                 ///< 链接类型 (LINK_TYPE_*)
  uint16 switch_reconn_num;         ///< 重连失败次数，切换地址
  int32 once_recv_len_;             ///< 一次调用recv 的输入长度
  lb_common::reconnect_ctl reconn_; ///< 重连控制器
  int32 check_interval_;            ///< 心跳和重连间隔(秒)
  int16 active_idx_;                ///< 0=primary, 1=secondary
  int16 addr_valid_num;             ///< 有效地址数 (1=仅主, 2=主备)
  lb_common::csock_addr addrs_[2];  ///< [0]=primary, [1]=secondary
  lb_common::lb_log *log_;          ///< 日志指针
};

} // namespace lb_api

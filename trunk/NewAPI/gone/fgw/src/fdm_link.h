// fdm_link - fdm 物理链接（每板卡一条，1:1 物理绑定）
//
// Step 5 已确认成员定义
// 已做修正 #13：状态查询用 aio_tcp.is_work()/is_close()，不重定义 state_
// 已做修正 #15：用对象不用 unique_ptr（生命周期同 fdm_board）
// 已做修正 #22：重连逻辑直接放 fdm_link（无独立 FdmReconnectManager）
// R-9：board_no_ 自包含
// S-5：tcp_buf_ch 作为类成员使用（库对象使用深度分析）
//
// 接口（Step 8 增量追加，参照 api_link 模式重构）：
//   S1: send_msg / close_link            — link_send_que 发送线程调用（纯 IO 转发）
//   S3: connect / on_connected            — 主动连接（同步 connect_ch + start_ch）
//        check_reconnect                  — 重连判定
//
// 重连模型（参考 gone/api aio_socket_link + 架构范式 §3）：
//   - deal_close_event → rc_.set_disconnected（必须在 deal_ch_closed）
//   - 重连由 fgw_instance periodic_check 调 fdm_engine → link->check_reconnect → link->connect
//   - 不在 deal_ch_closed 直接重连（避免与接收线程 deal_msg 竞争 ch 状态）
//
// user_data 约定：connect 时将 puserdata 设为 fdm_board*，
// deal_ch_* 回调通过 get_user_data() 获取 fdm_board*，通过 board->get_link() 获取 link。

#pragma once

#include <cstdint>

#include "aio_tcp.h"
#include "comm_aio.h"
#include "comm_sock.h"
#include "comm_sys.h"
#include "link_send_que.h"
#include "mthread.h"
#include "reconnect_ctl.h"

namespace lb_fgw {

class fdm_board; // 前置声明
class fdm_engine;

/// fdm 物理链接
/// 数量级：每板卡 1；标识：归属 fdm_board（无独立标识）
/// 关键行为：重连不创建新实例（fdm_board::link_ 是值对象，身份不变）
class fdm_link {
public:
  // 链接状态
  FORCE_INLINE bool is_work() { return aio_.is_work(); }
  FORCE_INLINE bool is_close() { return aio_.is_close(); }
  FORCE_INLINE bool is_free() { return aio_.is_free(); }

  FORCE_INLINE uint16_t get_board_no() const { return board_no_; }
  FORCE_INLINE const lb_common::csock_addr &get_remote() const { return remote_; }

  /// 发送消息：转发到底层 aio 通道
  FORCE_INLINE int32_t send_msg(char *pmsg, int32_t len) { return aio_.send_msg_fc(pmsg, len); }

  /// 投递发送事件到发送队列（多线程安全）
  FORCE_INLINE int32_t push_send(const char *msg, int32_t len) { return send_que_->push_send_msg(this, msg, len); }

  FORCE_INLINE void on_heart_msg() { aio_.on_heart_msg(); }

  FORCE_INLINE void on_msg_heart() { aio_.on_com_msg(); }

  /// 主动连接（同步，由 fdm_engine 首次调用或重连调用）
  /// @return 0 成功；<0 失败
  int32_t connect(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr, lb_common::mthread *recvth,
                  lb_common::mthread *sendth, link_send_que<fdm_link, fdm_engine> *sendque,
                  lb_common::ch_recv_cb<lb_common::tcp_buf_ch> *cb_msg,
                  lb_common::aio_ch_op<lb_common::tcp_buf_ch> *cb_ch, fdm_board *fdm);

  /// 连接已建立：start_ch + rc_.set_connected（connect_ch 返回成功后调）
  int32_t on_connected();

  /// 检查是否应重连（供 fdm_engine 周期检查调用，使用内部 heart_interval_）
  bool check_reconnect();

  /// 心跳超时检测（主线程/periodic_check 调用）
  bool check_heart_timeout();

  bool check_heart_send();

  /// 关闭链接（deal_ch_error 或心跳超时时调用）
  void close_link(int32_t errcode);

  /// 投递关闭事件到发送队列（deal_ch_closing 中调用）
  int32_t push_close_event();

  /// 关闭事件处理（deal_ch_closed 中调用：减负载 + set_disconnected + sub_ref）
  void deal_close_event();

  /// 引用计数（发送线程分配时 add_ref，消费 close_link 时 sub_ref）
  FORCE_INLINE bool add_ref() { return aio_.add_ref(); }
  FORCE_INLINE void sub_ref() { aio_.sub_ref(); }

  /// 投递事件到接收线程（delive_recv_event 封装，供 api_engine 解除 api_link 引用）
  FORCE_INLINE int32_t post_recv_event(lb_common::event_op *ev_op, void *ev_data, int32_t data_len,
                                       int32_t needadd = 1) {
    return aio_.delive_recv_event(ev_op, ev_data, data_len, needadd);
  }
  /// 结束接收事件（与 post_recv_event(needadd=1) 配对，deal 后调用）
  FORCE_INLINE void end_recv_event(int32_t needadd = 1) { aio_.end_recv_event(needadd); }

  fdm_link() : send_que_(nullptr), board_no_(0), heart_interval_(5), send_thread_(nullptr) {}
  ~fdm_link() { aio_.close_ch(); }

private:
  lb_common::tcp_buf_ch aio_;                     ///< 异步 IO（深度分析）
  link_send_que<fdm_link, fdm_engine> *send_que_; ///< 发送队列（隶属fdm engine，引用）
  uint16_t board_no_;                             ///< 板卡号（自包含）
  int32_t heart_interval_;                        ///< 心跳和重连间隔
  lb_common::mthread *send_thread_;               ///< send 线程（connect 时由 fdm_engine 分配，引用）
  lb_common::reconnect_ctl rc_;                   ///< 重连控制（应用行为，非 aio_tcp 状态）
  lb_common::csock_addr remote_;                  ///< 远端地址（connect 时保存，供日志/重连用）
};

} // namespace lb_fgw

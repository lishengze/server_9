// api_link - api 协议实体（业务方客户端链接，含 LINK_TYPE 枚举）
//
// Step 5 已确认成员定义（Step 3 铁律：仅识别对象 + Step 4 存储决策）
// 已做修正 #14：保留 link_type_（DIRECT/GATEWAY 区分）
// R-14：热字段在前
// R-8：自带 state+count（aio_tcp 自带）→ 不用 shared_ptr
//
// 接口（Step 8 增量追加）：
//   S1: send_msg / close_link     — link_send_que 发送线程调用（纯 IO 转发）
//   S2: accept / on_connected      — 链接建立（accept_ch + start_ch）
//        on_closed                — 链接关闭通知（业务清理留待登录场景）

#pragma once

#include <cstdint>

#include "aio_tcp.h"
#include "comm_aio.h"
#include "comm_sock.h"
#include "comm_sys.h"
#include "g1msghead.h"
#include "link_send_que.h"
#include "mthread.h"

namespace lb_fgw {

class api_engine;
// ============================================================================
// 链接类型（api 链接的业务身份标记，区别于接收/发送线程 kind）
// ============================================================================
static constexpr int LINK_TYPE_UNKNOWN = 0; ///< accept 时未知，由后续业务确定
static constexpr int LINK_TYPE_DIRECT = 1;  ///< 直连客户（持有 board_no/user_id）
static constexpr int LINK_TYPE_GATEWAY = 2; ///< 网关客户（不持有 board_no/user_id）

/// api 协议实体
/// 数量级：≤1000；标识：api_connect_id（fgw 内部分配，全实例内唯一）
/// 生命周期：accept 时创建 → close 时销毁（由 api_engine 延迟清理队列 delete）
class api_link {
public:
  // 链接状态
  FORCE_INLINE bool is_work() { return aio_.is_work(); }
  FORCE_INLINE bool is_close() { return aio_.is_close(); }
  FORCE_INLINE bool is_free() { return aio_.is_free(); }

  /// 链接标识（关联 key）
  FORCE_INLINE int32_t get_connect_id() const { return api_connect_id_; }

  /// 发送消息：转发到底层 aio 通道
  FORCE_INLINE int32_t send_msg(char *pmsg, int32_t len) {
    // 强制发送
    return aio_.send_msg_fc(pmsg, len);
  }

  /// 投递发送事件到发送队列（多线程安全）
  FORCE_INLINE int32_t push_send(const char *msg, int32_t len) {
    // 入队并触发
    return send_que_->push_send_msg(this, msg, len);
  }

  FORCE_INLINE void on_heart_msg() { aio_.on_heart_msg(); }

  FORCE_INLINE void on_msg_heart() { aio_.on_com_msg(); }

  /// accept 时初始化并接受连接（由 api_engine::deal_listen_accept 调用）
  /// @param recvth   接收线程（aio_ 挂载其 epoll）
  /// @param sendth   发送线程（消费 send_que_）
  /// @param sendque  所属发送队列
  /// @param cb_msg   消息回调（api_engine，继承 ch_recv_cb）
  /// @param cb_ch    通道回调（api_engine，继承 aio_ch_op）
  /// @param connect_id  fgw 内部分配的链接标识
  /// @return 0 成功；<0 失败
  int32_t accept(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr, lb_common::mthread *recvth,
                 lb_common::mthread *sendth, link_send_que<api_link, api_engine> *sendque,
                 lb_common::ch_recv_cb<lb_common::tcp_buf_ch> *cb_msg,
                 lb_common::aio_ch_op<lb_common::tcp_buf_ch> *cb_ch, int32_t connect_id, lb_common::sock_fd fd);

  /// 连接已建立：start_ch 把 aio_ 加入 recv 线程 epoll（accept_ch 成功后调）
  /// @return 0 成功；<0 失败
  int32_t on_connected();

  /// 心跳超时检测（主线程周期检测）
  bool check_heart_timeout();

  /// 关闭链接（心跳超时或deal_ch_error 调用）
  void close_link(int32_t errcode);

  /// 投递关闭事件到发送队列（首次设置链接关闭中时调用，deal_ch_closing）
  int32_t push_close_event();

  /// 关闭事件处理
  void deal_close_event();

  void set_user_board(int32_t log_type, uint16_t board, uint16_t user);

  void push_onboard_state(g1_msg_head *head);

  bool add_ref() { return aio_.add_ref(); }
  void sub_ref() { aio_.sub_ref(); }

  api_link()
      : send_que_(nullptr), api_connect_id_(0), link_type_(0), board_no_(0), user_id_(0), send_thread_(nullptr) {}
  ~api_link() {}

private:
  lb_common::tcp_buf_ch aio_;                     ///< 异步 IO（库对象作为成员，引用）
  link_send_que<api_link, api_engine> *send_que_; ///< 发送队列（隶属api engine，引用）
  int32_t api_connect_id_;                        ///< fgw 内部分配的链接标识（关联 key）
  int32_t link_type_;                             ///< 链接类型：accept 时未知，由后续业务消息确定
  uint16_t board_no_;                             ///< 直连客户板卡号（GATEWAY 模式无效）
  uint16_t user_id_;                              ///< 直连客户板内 user_id（GATEWAY 模式无效）
  lb_common::mthread *send_thread_;               ///< send 线程（accept 时由 api_engine 分配，引用）
};

} // namespace lb_fgw

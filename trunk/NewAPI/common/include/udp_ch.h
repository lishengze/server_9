#pragma once

/**
 * @file udp_ch.h
 * @brief UDP通道通信模块
 *
 * 提供UDP通道的管理功能，支持单播、组播和广播模式。
 * 包含连接状态管理、数据收发、事件处理等完整功能。
 *
 * 主要特性：
 * - UDP单播通道管理
 * - UDP组播通道管理
 * - UDP广播通道管理
 * - 事件驱动架构
 * - 线程安全的状态管理
 * - 高性能数据收发
 *
 * 使用场景：
 * - UDP客户端应用
 * - UDP服务器应用
 * - 组播通信应用
 * - 广播通信应用
 * - 实时音视频传输
 */

#include "comm_sock.h"
#include "comm_sys.h"
#include "matomic.h"
#include "mevent.h"
#include "mthread.h"
#include "state_machine.h"
#include "wait_poll.h"

#include <cstring>

namespace lb_common {

class udp_ch;

/**
 * @brief UDP通道操作回调接口
 *
 * 定义UDP通道生命周期中的各种回调函数，用户继承此类实现自定义逻辑。
 */
class udp_ch_op {
public:
  /**
   * @brief UDP通道开始关闭回调
   *
   * 首次关闭成功后回调，多线程使用时底层保证只会调用一次，但不一定哪个线程。
   * 回调后，用户可在此函数关闭对象引用，不再使用此对象。
   * 此后用户调用接收，发送，关闭都会失败。
   *
   * @param[in] pch UDP通道指针
   * @param[in] errcode 错误码
   */
  virtual void deal_udp_to_close(udp_ch *pch, int32 errcode) {};

  /**
   * @brief UDP通道完全关闭回调
   *
   * 多线程下，各线程都结束使用，用户在此可释放此对象资源或对象。
   *
   * @param[in] pch UDP通道指针
   * @param[in] errcode 错误码
   */
  virtual void deal_udp_closed(udp_ch *pch, int32 errcode) {};

  udp_ch_op(){};
  virtual ~udp_ch_op(){};
};

/**
 * @brief UDP通道类
 *
 * 管理UDP连接的完整生命周期，支持单播、组播和广播模式。
 * 提供数据收发、状态管理、事件处理等功能。
 */
class udp_ch {
protected:
  src_stat_ref thctl;            ///< 状态引用管理
  int32 sysfd;                   ///< 系统文件描述符
  int32 errcode;                 ///< 错误码
  struct sockaddr_in remoteaddr; ///< 远程地址
  udp_ch_op *pfunc;              ///< 回调函数指针

  /**
   * @brief 销毁资源
   */
  void destroy();

  /**
   * @brief 处理系统关闭
   */
  void deal_closing();

  /**
   * @brief 处理关闭后操作
   */
  void deal_after_close();

public:
  /**
   * @brief 发送数据
   * @param[in] pmsg 消息缓冲区
   * @param[in] len 消息长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  int32 send_msg(char *pmsg, int32 len);

  /**
   * @brief 强制发送数据
   * @param[in] pmsg 消息缓冲区
   * @param[in] len 消息长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  int32 send_msg_fc(char *pmsg, int32 len);

  /**
   * @brief 接收数据
   * @param[in] msgbuf 消息缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   */
  int32 recv_msg(char *msgbuf, int32 len);

  /**
   * @brief 强制接收数据
   * @param[in] msgbuf 消息缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   */
  int32 recv_msg_fc(char *msgbuf, int32 len);

  /**
   * @brief 增加引用计数
   * @return bool 成功返回true，失败返回false
   */
  FORCE_INLINE bool add_ref() { return thctl.add_ref(); }

  /**
   * @brief 减少引用计数
   */
  FORCE_INLINE void sub_ref() {
    if (thctl.sub_ref()) {
      deal_after_close();
    }
  }

  /**
   * @brief 检查是否工作状态
   * @return bool 工作状态返回true，否则返回false
   */
  FORCE_INLINE bool is_work() const { return thctl.is_work(); }

  /**
   * @brief 检查是否已关闭
   * @return bool 已关闭返回true，否则返回false
   */
  FORCE_INLINE bool is_close() const { return thctl.is_close(); }

  /**
   * @brief 检查是否空闲
   * @return bool 空闲返回true，否则返回false
   */
  FORCE_INLINE bool is_free() const { return thctl.is_free(); }

  /**
   * @brief 设置错误码
   * @param[in] err 错误码
   */
  FORCE_INLINE void set_err(int32 err) {
    if (err != 0)
      errcode = err;
  }

  FORCE_INLINE int32 sys_fd() const { return atomic_load32(&sysfd); }

  /**
   * @brief 初始化UDP通道
   * @param[in] attr 通道属性
   * @param[in] tpasyfunc 异步回调函数指针
   * @param[in] isrecv 是否接收模式
   * @param[in] ismulti 是否组播/广播模式
   * @param[in] premote 远程地址
   * @param[in] plocal 本地地址
   * @return int32 成功返回0，失败返回错误码
   */
  int32 init_ch(channel_attr &attr, udp_ch_op *tpasyfunc, int32 isrecv, int32 ismulti, csock_addr *premote,
                csock_addr *plocal);

  /**
   * @brief 关闭UDP通道
   * @param[in] err 错误码
   */
  void close_ch(int32 err = 0);

public:
  /**
   * @brief 投递通道事件到线程（如通道发送事件，控制事件）
   * @param[in] tpth 线程引用
   * @param[in] tpe_op 事件操作指针
   * @param[in] tpe_data 事件数据，POD 类型，不超过56字节
   * @param[in] data_len 事件数据大小，可为0
   * @param[in] needadd 是否需要增加引用
   * @return int32 成功返回0，失败返回错误码
   */
  int32 delive_ch_event(mthread &tpth, event_op *tpe_op, void *tpe_data, int32 data_len, int32 needadd = 1);

  /**
   * @brief 结束通道事件处理后，调用
   * @param[in] needadd 是否需要增加引用
   */
  FORCE_INLINE void end_ch_event(int32 needadd = 1) {
    if (needadd == 1)
      sub_ref();
  }

  /**
   * @brief 开始轮询接收
   * @param[in] tprecvth 接收线程指针
   * @param[in] tprecvevent 接收事件指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 start_poll_recv(mthread *tprecvth, epoll_event_op *tprecvevent);

  /**
   * @brief 移除轮询接收
   * @param[in] tprecvth 接收线程指针
   * @param[in] tprecvevent 接收事件指针
   */
  void remove_poll_recv(mthread *tprecvth, epoll_event_op *tprecvevent);

  /**
   * @brief 从轮询线程移除后，调用
   */
  FORCE_INLINE void end_poll_recv() { sub_ref(); }

  /**
   * @brief 构造函数
   */
  udp_ch() {
    sysfd = -1;
    errcode = 0;
    std::memset((void *)(&remoteaddr), 0, sizeof(remoteaddr));
    pfunc = NULL;
  }

  /**
   * @brief 析构函数
   */
  virtual ~udp_ch() { destroy(); }
};

} // namespace lb_common

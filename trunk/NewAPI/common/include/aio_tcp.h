
#pragma once

/**
 * @file aio_tcp.h
 * @brief AIO TCP模块
 *
 * 提供基于TCP的异步IO功能：
 * - TCP连接管理和数据传输
 * - 支持多种缓存类型（缓冲区、内存池、队列）
 * - 异步事件处理和回调机制
 * - 心跳检测和超时管理
 */

#include "aio_recv_buf.h"
#include "aio_recv_pool.h"
#include "aio_recv_que.h"
#include "comm_aio.h"
#include "comm_sock.h"
#include "comm_sys.h"
#include "heart_manage.h"
#include "mthread.h"
#include "tcp_ch.h"
// #include <type_traits>

namespace lb_common {

/**
 * @brief AIO TCP类
 *
 * 基于TCP通道的异步IO实现。
 * 支持多种缓存类型，提供高效的TCP连接管理和数据传输。
 *
 * @tparam B 缓存类型，由RC 决定，如果RC为aio_recv_buf，则B为char;
 * 如果RC为aio_recv_pool，则B为fix_pool; 如果RC为aio_recv_que，则B为
 * que_swr_buf
 * @tparam RC 接收缓存类型
 */
template <class B, class RC> class aio_tcp final : public epoll_event_op {
protected:
  /**
   * @brief AIO TCP通道操作回调类
   */
  class aio_tcp_ch_op final : public tcp_ch_op {
  public:
    aio_tcp *mch; /**< TCP AIO对象指针 */

    /**
     * @brief 处理TCP通道即将关闭
     *
     * @param[in] pch TCP通道指针
     * @param[in] errcode 错误码
     */
    virtual void deal_tcp_to_close(tcp_ch *pch, int32 errcode) {
      if (unlikely(NULL == mch))
        return;
      if (likely(NULL != mch->th_recv)) {
        mch->ch.remove_poll_recv(mch->th_recv, static_cast<epoll_event_op *>(mch));
      }
      if (likely(mch->f_ch != NULL)) {
        mch->f_ch->deal_ch_closing(mch, errcode);
      }
    }

    /**
     * @brief 处理TCP通道已关闭
     *
     * @param[in] pch TCP通道指针
     * @param[in] errcode 错误码
     */
    virtual void deal_tcp_closed(tcp_ch *pch, int32 errcode) {
      if (unlikely(NULL == mch))
        return;
      // mch->buf.close_buf();
      mch->buf.reset_buf();
      if (likely(mch->f_ch != NULL)) {
        mch->f_ch->deal_ch_closed(mch, errcode);
      }
    }

    /**
     * @brief 处理TCP连接建立
     *
     * @param[in] pch TCP通道指针
     * @param[in] localaddr 本地地址引用
     */
    virtual void deal_tcp_connect(tcp_ch *pch, csock_addr &localaddr) {
      if (unlikely(NULL == mch))
        return;
      if (likely(mch->f_ch != NULL)) {
        mch->f_ch->deal_ch_connect(mch, localaddr);
      }
    }

    /**
     * @brief 构造函数
     */
    aio_tcp_ch_op() : mch(NULL){};
    /**
     * @brief 虚析构函数
     */
    virtual ~aio_tcp_ch_op(){};
  };

  tcp_ch ch;                  /**< TCP通道对象 */
  RC buf;                     /**< 接收缓存对象 */
  int32 n_recv_loop;          /**< 接收循环次数 */
  int32 dispatch_zero_copy;   /**< 接收消息分发处理时，是否使用零拷贝 */
  ch_recv_cb<aio_tcp> *f_msg; /**< 消息处理回调 */
  heart_manage heart;         /**< 心跳管理对象 */
  mthread *th_recv;           /**< 接收线程指针 */
  aio_ch_op<aio_tcp> *f_ch;   /**< 通道操作回调 */
  void *user_data;            /**< 用户数据指针 */
  aio_tcp_ch_op op_base;      /**< 基础操作回调对象 */

  friend class aio_tcp_ch_op;

public:
  /**
   * @brief 发送消息
   *
   * @param[in] pmsg 消息缓冲区
   * @param[in] len 消息长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  FORCE_INLINE int32 send_msg(char *pmsg, int32 len) { return ch.send_msg(pmsg, len); }

  /**
   * @brief 强制发送数据
   * @param[in] pmsg 消息缓冲区
   * @param[in] len 消息长度
   * @return int32 成功返回发送字节数，失败返回错误码
   */
  FORCE_INLINE int32 send_msg_fc(char *pmsg, int32 len) { return ch.send_msg_fc(pmsg, len); }

  /**
   * @brief 接收数据
   * @param[in] msgbuf 消息缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   * @warning
   * 大多时候用户不应使用该接口直接接收数据，而是使用loo_deal_recv接口,若使用只能在start_ch
   * 前使用。
   */
  FORCE_INLINE int32 recv_msg(char *msgbuf, int32 len) { return ch.recv_msg(msgbuf, len); }

  /**
   * @brief 强制接收数据
   * @param[in] msgbuf 消息缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数，失败返回错误码
   * @warning
   * 大多时候用户不应使用该接口直接接收数据，而是使用loop_deal_recv接口,若使用只能在start_ch
   * 前使用。
   */
  FORCE_INLINE int32 recv_msg_fc(char *msgbuf, int32 len) { return ch.recv_msg_fc(msgbuf, len); }

  /**
   * @brief 接收并调用回调处理消息
   *
   * @return int32 >0 接收到数据并处理成功；=0 通道正常但无数据；<0
   * 数据处理错误或接收错误
   */
  int32 deal_recv() {
    aio_msg tmsg;
    int32 dlen = 0;
    int32 rlen = buf.template recv_msg<tcp_ch>(ch, tmsg);

    if (likely(rlen > 0)) {
      dlen = f_msg->deal_msg(this, tmsg);
      buf.cmt_deal(dlen);
      if (likely(dlen > 0)) {
        // heart.on_msg();
        if (dispatch_zero_copy == 0)
          buf.cmt_buf(tmsg.buf_addr, dlen);
      } else if (dlen < 0) {
        f_ch->deal_ch_error(this, AIO_CHERR_TYPE_DEAL, dlen);
        rlen = LBERR_OBJ_MOD_FAIL;
      }
    } else if (rlen < 0) {
      f_ch->deal_ch_error(this, AIO_CHERR_TYPE_RECV, rlen);
    }
    return rlen;
  }

  /**
   * @brief 判断接收的消息是否存在未处理的
   *
   * @return true 处理完成
   * @return false 处理未完成
   */
  FORCE_INLINE bool is_recv_complete() { return buf.is_complete(); }

  /**
   * @brief 接收消息处理循环
   */
  void loop_deal_recv() {
    int32 ret = 0;
    int32 loop_count = 0;

    while ((loop_count < n_recv_loop) && ch.is_work()) {
      ret = deal_recv();
      if (likely(ret > 0)) {
        if (buf.is_complete())
          loop_count++;
      } else if (ret == 0) {
        if (buf.is_complete())
          break;
      } else {
        // ch.close_ch(ret);
        break;
      }
    }
  }

  /**
   * @brief 接收消息分发后，被分发的线程处理完消息后，调用此函数让缓存可重新使用
   * @warning 在非分发消息再处理的场景中，禁止调用此函数，即
   * aio_attr.dispatch_zero_copy = 0 时
   */
  FORCE_INLINE void dispatch_deal_end(int64 buf_addr, int32 deal_len) { buf.cmt_buf(buf_addr, deal_len); }

  /**
   * @brief 投递接收事件
   * @param[in] ev_op 事件操作指针
   * @param[in] ev_data 事件数据
   * @param[in] data_len 数据长度
   * @param[in] needadd 是否需要添加，默认为1
   * @return int32 成功返回0，失败返回错误码
   */
  FORCE_INLINE int32 delive_recv_event(event_op *ev_op, void *ev_data, int32 data_len, int32 needadd = 1) {
    if (likely(NULL != th_recv)) {
      return ch.delive_ch_event(*th_recv, ev_op, ev_data, data_len, needadd);
    }
    return LBERR_OBJ_NOT_HAVE;
  }
  /**
   * @brief 结束接收事件
   * @param[in] needadd 是否需要添加，默认为1
   */
  FORCE_INLINE void end_recv_event(int32 needadd = 1) { ch.end_ch_event(needadd); }

  FORCE_INLINE bool add_ref() { return ch.add_ref(); }

  FORCE_INLINE void sub_ref() { ch.sub_ref(); }

  FORCE_INLINE bool is_work() const { return ch.is_work(); }

  FORCE_INLINE bool is_close() const { return ch.is_close(); }

  FORCE_INLINE bool is_free() const { return ch.is_free(); }

  FORCE_INLINE int32 get_sys_fd() const { return ch.sys_fd(); }

  FORCE_INLINE void *get_user_data() const { return user_data; }

  FORCE_INLINE int32 get_heart_interval() const { return heart.get_interval(); }

  /**
   * @brief 收到心跳消息
   */
  FORCE_INLINE void on_heart_msg() { heart.on_heart(); }

  /**
   * @brief 收到普通消息
   */
  FORCE_INLINE void on_com_msg() { heart.on_msg(); }

  /**
   * @brief 检查心跳发送
   */
  FORCE_INLINE bool check_heart_send() { return heart.check_send(); }

  /**
   * @brief 检查心跳超时
   * @return bool 超时返回true，否则返回false
   */
  FORCE_INLINE bool check_heart_timeout() const { return heart.check_timeout(); }

  FORCE_INLINE void set_heart_interval(int32 interval_sec) { heart.set_interval(interval_sec); }

  /**
   * @brief 连接管理接口
   * @param[in] buf_attr 缓冲区属性
   * @param[in] ch_attr 通道属性
   * @param[in] pcb_msg 消息处理回调
   * @param[in] pcb_ch 通道操作回调
   * @param[in] precvth 接收线程
   * @param[in] premote 远程地址
   * @param[in] plocal 本地地址，可选
   * @param[in] asyn_connect 是否异步连接，默认为0
   * @return int32 1 成功链接，0 异步链接中，<0 失败返回错误码
   */
  int32 connect_ch(aio_attr &buf_attr, channel_attr &ch_attr, ch_recv_cb<aio_tcp> *pcb_msg, aio_ch_op<aio_tcp> *pcb_ch,
                   mthread *precvth, csock_addr *premote, csock_addr *plocal = NULL, int32 asyn_connect = 0) {
    if (NULL == precvth || NULL == pcb_msg || NULL == pcb_ch || NULL == premote) {
      return LBERR_ARGV_WRONG;
    }

    init_set(buf_attr, pcb_msg, pcb_ch, precvth);

    int32 ret = buf.build_buf(buf_attr);
    if (ret < 0)
      return ret;

    ret = ch.connect_ch(ch_attr, &op_base, asyn_connect, premote, plocal, precvth);
    if (ret < 0) {
      buf.close_buf();
      return ret;
    }
    return 0;
  }

  /**
   * @brief 接受TCP连接
   * @param[in] buf_attr 缓冲区属性
   * @param[in] ch_attr 通道属性
   * @param[in] pcb_msg 消息处理回调
   * @param[in] pcb_ch 通道操作回调
   * @param[in] precvth 接收线程
   * @param[in] fd 已接受的套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  int32 accept_ch(aio_attr &buf_attr, channel_attr &ch_attr, ch_recv_cb<aio_tcp> *pcb_msg, aio_ch_op<aio_tcp> *pcb_ch,
                  mthread *precvth, sock_fd fd) {
    if (NULL == precvth || NULL == pcb_msg || NULL == pcb_ch || fd <= 0) {
      return LBERR_ARGV_WRONG;
    }

    init_set(buf_attr, pcb_msg, pcb_ch, precvth);

    int32 ret = buf.build_buf(buf_attr);
    if (ret < 0)
      return ret;

    ret = ch.accept_ch(ch_attr, &op_base, fd);
    if (ret < 0) {
      buf.close_buf();
      return ret;
    }
    return 0;
  }

  /**
   * @brief 连接TCP服务器（使用外部缓冲区）
   * @param[in] buf_attr 缓冲区属性
   * @param[in] t_buf 外部缓冲区指针
   * @param[in] ch_attr 通道属性
   * @param[in] pcb_msg 消息处理回调
   * @param[in] pcb_ch 通道操作回调
   * @param[in] precvth 接收线程
   * @param[in] premote 远程地址
   * @param[in] plocal 本地地址，可选
   * @param[in] asyn_connect 是否异步连接，默认为0
   * @return int32 成功链接返回1,异步链接中返回0，失败返回错误码
   */
  int32 connect_ch(aio_attr &buf_attr, B *t_buf, channel_attr &ch_attr, ch_recv_cb<aio_tcp> *pcb_msg,
                   aio_ch_op<aio_tcp> *pcb_ch, mthread *precvth, csock_addr *premote, csock_addr *plocal = NULL,
                   int32 asyn_connect = 0) {
    if (NULL == pcb_msg || NULL == pcb_ch || NULL == premote) {
      return LBERR_ARGV_WRONG;
    }

    init_set(buf_attr, pcb_msg, pcb_ch, precvth);

    buf.init_buf(buf_attr, t_buf);

    int32 ret = ch.connect_ch(ch_attr, &op_base, asyn_connect, premote, plocal, precvth);
    if (ret < 0) {
      buf.close_buf();
      return ret;
    }
    return ret;
  }

  /**
   * @brief 接受TCP连接（使用外部缓冲区）
   * @param[in] buf_attr 缓冲区属性
   * @param[in] t_buf 外部缓冲区指针
   * @param[in] ch_attr 通道属性
   * @param[in] pcb_msg 消息处理回调
   * @param[in] pcb_ch 通道操作回调
   * @param[in] precvth 接收线程
   * @param[in] fd 已接受的套接字文件描述符
   * @return int32 成功返回0，失败返回错误码
   */
  int32 accept_ch(aio_attr &buf_attr, B *t_buf, channel_attr &ch_attr, ch_recv_cb<aio_tcp> *pcb_msg,
                  aio_ch_op<aio_tcp> *pcb_ch, mthread *precvth, sock_fd fd) {
    if (NULL == pcb_msg || NULL == pcb_ch || fd <= 0) {
      return LBERR_ARGV_WRONG;
    }

    init_set(buf_attr, pcb_msg, pcb_ch, precvth);

    buf.init_buf(buf_attr, t_buf);

    int32 ret = ch.accept_ch(ch_attr, &op_base, fd);
    if (ret < 0) {
      buf.close_buf();
      return ret;
    }
    return 0;
  }

  /**
   * @brief 启动接收线程
   * @return int32 成功返回0，失败返回错误码
   */
  int32 start_ch() {
    assert(th_recv != NULL);
    return ch.start_poll_recv(th_recv, this);
  }

  /**
   * @brief 关闭连接
   * @param[in] err 错误码，默认为0
   */
  void close_ch(int32 err = 0) { ch.close_ch(err); }

protected:
  /**
   * @brief 处理事件
   */
  virtual void deal_event() { loop_deal_recv(); }

  /**
   * @brief 处理错误
   */
  virtual void deal_error() {
    int32 terr = 0;
    sock_utils::get_sock_error(ch.sys_fd(), terr);
    if (f_ch != NULL) {
      if (terr == 0)
        terr = LBERR_OBJ_WAIT_FAIL;
      else if (terr > 0)
        terr = 0 - terr;
      f_ch->deal_ch_error(this, AIO_CHERR_TYPE_EPOLL, terr);
    }
    // ch.close_ch();
  }

  /**
   * @brief 处理关闭
   */
  virtual void deal_close() {
    buf.reset_buf();
    ch.end_poll_recv();
    if (f_ch != NULL) {
      f_ch->deal_recv_stop(this);
    }
  }

  /**
   * @brief 获取文件描述符
   * @return int32 文件描述符
   */
  virtual int32 get_fd() { return ch.sys_fd(); }

  /**
   * @brief 初始化设置
   * @param[in] buf_attr 缓冲区属性
   * @param[in] pcb_msg 消息处理回调
   * @param[in] pcb_ch 通道操作回调
   * @param[in] precvth 接收线程
   */
  void init_set(aio_attr &buf_attr, ch_recv_cb<aio_tcp> *pcb_msg, aio_ch_op<aio_tcp> *pcb_ch, mthread *precvth) {
    set_event(0, 0, 1);
    n_recv_loop = buf_attr.onerecvtimes;
    if (n_recv_loop <= 0)
      n_recv_loop = AIO_RECV_ONCE_NUM;
    if (buf_attr.dispatch_zero_copy == 1)
      dispatch_zero_copy = 1;
    else
      dispatch_zero_copy = 0;
    f_msg = pcb_msg;
    if (buf_attr.heartinterval <= 0)
      buf_attr.heartinterval = 0;
    heart.init(buf_attr.heartinterval);
    th_recv = precvth;
    f_ch = pcb_ch;
    user_data = buf_attr.puserdata;
    op_base.mch = this;
  }

public:
  /**
   * @brief 构造函数
   */
  aio_tcp() : n_recv_loop(0), dispatch_zero_copy(0), f_msg(NULL), th_recv(NULL), f_ch(NULL), user_data(NULL) {
    op_base.mch = this;
  };
  /**
   * @brief 析构函数
   */
  virtual ~aio_tcp() { ch.close_ch(); }

  /**
   * @brief 禁用拷贝构造函数
   */
  aio_tcp(const aio_tcp &) = delete;
  /**
   * @brief 禁用赋值操作符
   */
  aio_tcp &operator=(const aio_tcp &) = delete;
};

/** @brief TCP缓冲区通道类型 */
typedef aio_tcp<char, aio_recv_buf> tcp_buf_ch;

/** @brief TCP MTU池通道类型 */
typedef aio_tcp<aio_mtu_pool, aio_recv_mtu_pool> tcp_mtu_pool_ch;

/** @brief TCP队列通道类型 */
typedef aio_tcp<que_swr_buf, aio_recv_que> tcp_que_ch;

} // namespace lb_common

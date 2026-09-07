#pragma once

/**
 * @file tcp_ch.h
 * @brief TCP通道通信模块
 *
 * 提供TCP连接和监听通道的管理功能，支持同步和异步连接模式。
 * 包含连接状态管理、数据收发、事件处理等完整功能。
 * 
 * 主要特性：
 * - TCP客户端连接管理
 * - TCP服务器监听管理
 * - 异步连接支持
 * - 事件驱动架构
 * - 线程安全的状态管理
 * - 高性能数据收发
 * 
 * 使用场景：
 * - TCP客户端应用
 * - TCP服务器应用
 * - 网络通信中间件
 * - 高并发网络服务
 */

#include "comm_sock.h"
#include "comm_sys.h"
#include "matomic.h"
#include "mevent.h"
#include "mthread.h"
#include "state_machine.h"
#include "wait_poll.h"

namespace lb_common {

/** @brief TCP监听单次循环处理数量 */
#define TCPCH_LISTEN_ONCELOOP_NUM 3

class tcp_ch;
class tcp_listen_ch;

/**
 * @brief TCP通道操作回调接口
 * 
 * 定义TCP通道生命周期中的各种回调函数，用户继承此类实现自定义逻辑。
 */
class tcp_ch_op {
public:
  /**
	 * @brief TCP通道开始关闭回调
	 * 
	 * 首次关闭成功后回调，多线程使用时底层保证只会调用一次，但不一定哪个线程。
	 * 回调后，用户可在此函数关闭对象引用，不再使用此对象。
	 * 此后用户调用接收，发送，关闭都会失败。
	 * 
	 * @param[in] pch TCP通道指针
	 * @param[in] errcode 错误码
	 */
  virtual void deal_tcp_to_close(tcp_ch *pch, int32 errcode) {};

  /**
	 * @brief TCP通道完全关闭回调
	 * 
	 * 多线程下，各线程都结束使用，用户在此可释放此对象资源或对象。
	 * 链接失败后，也会调用此函数。
	 * 
	 * @param[in] pch TCP通道指针
	 * @param[in] errcode 错误码
	 */
  virtual void deal_tcp_closed(tcp_ch *pch, int32 errcode) {};

  /**
	 * @brief TCP连接成功回调
	 * 
	 * TCP连接建立成功后调用。
	 * 
	 * @param[in] pch TCP通道指针
	 * @param[in] localaddr 本地地址
	 */
  virtual void deal_tcp_connect(tcp_ch *pch, csock_addr &localaddr) {};

  tcp_ch_op(){};
  virtual ~tcp_ch_op(){};
};

/**
 * @brief TCP通道类
 * 
 * 管理TCP连接的完整生命周期，支持同步和异步连接模式。
 * 提供数据收发、状态管理、事件处理等功能。
 */
class tcp_ch {
protected:
  /**
	 * @brief TCP连接事件处理类
	 * 
	 * 继承自epoll_event_op，处理TCP连接相关的事件。
	 */
  class tcp_connect_ev final : public epoll_event_op {
  protected:
    tcp_ch *pch;         ///< TCP通道指针
    mthread *pconnectth; ///< 连接线程指针

  public:
    friend class tcp_ch;

    /**
		 * @brief 处理轮询事件
		 */
    virtual void deal_event();

    /**
		 * @brief 处理轮询错误
		 */
    virtual void deal_error();

    /**
		 * @brief 处理轮询关闭
		 * 
		 * 从epoll移除后调用
		 */
    virtual void deal_close();

    /**
		 * @brief 获取文件描述符
		 * @return int32 文件描述符
		 */
    virtual int32 get_fd();

    tcp_connect_ev() : pch(NULL), pconnectth(NULL){};
    virtual ~tcp_connect_ev(){};
  };

  src_stat_ref thctl; ///< 状态引用管理
  int32 sysfd;        ///< 系统文件描述符
  int32 errcode;      ///< 错误码

  tcp_ch_op *pfunc;      ///< 回调函数指针
  tcp_connect_ev mevent; ///< 连接事件对象

  /**
	 * @brief 异步连接事件结束初始化处理
	 * @param[in] errcode 错误原因
	 */
  void end_connect_init(int32 errcode);
  /**
	 * @brief 处理异步连接
	 * @param[in] pconnectth 连接线程指针
	 * @param[in] iserror 是否错误
	 */
  void deal_asyn_connect(mthread *pconnectth, int32 iserror);

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
  void deal_after_closed();

public:
  friend class tcp_connect_ev;
  friend class tcp_listen_ch;

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
      deal_after_closed();
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
	 * @brief 连接TCP通道
	 * @param[in] tattr 通道属性
	 * @param[in] tpfunc 回调函数指针
	 * @param[in] isasynconnect 是否异步连接
	 * @param[in] premote 远程地址
	 * @param[in] plocal 本地地址
	 * @param[in] tpconnectth 连接线程指针
	 * @return 1 成功链接，0 异步链接中，<0 失败返回错误码
	 */
  int32 connect_ch(channel_attr &tattr, tcp_ch_op *tpfunc, int32 isasynconnect, csock_addr *premote,
                   csock_addr *plocal = NULL, mthread *tpconnectth = NULL);

  /**
	 * @brief 接受TCP连接
	 * @param[in] tattr 通道属性
	 * @param[in] tpfunc 回调函数指针
	 * @param[in] fd 套接字文件描述符
	 * @return int32 成功返回0，失败返回错误码
	 */
  int32 accept_ch(channel_attr &tattr, tcp_ch_op *tpfunc, sock_fd fd);

  /**
	 * @brief 关闭TCP通道
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
  tcp_ch() {
    sysfd = -1;
    errcode = 0;
    pfunc = NULL;
  }

  /**
	 * @brief 析构函数
	 */
  virtual ~tcp_ch() { destroy(); }
};

/**
 * @brief TCP监听操作回调接口
 * 
 * 定义TCP监听通道生命周期中的各种回调函数。
 */
class tcp_listen_op {
public:
  /**
	 * @brief TCP监听开始关闭回调
	 * 
	 * 首次关闭成功后回调，多线程使用时底层保证只会调用一次，但不一定哪个线程。
	 * 回调后，用户可在此函数关闭对象引用，不再使用此对象。
	 * 此后用户调用接收，发送，关闭都会失败。
	 * 
	 * @param[in] pch TCP监听通道指针
	 * @param[in] errcode 错误码
	 */
  virtual void deal_listen_to_close(tcp_listen_ch *pch, int32 errcode) {};

  /**
	 * @brief TCP监听完全关闭回调
	 * 
	 * 多线程下，各线程都结束使用，用户在此可释放此对象资源或对象。
	 * 
	 * @param[in] pch TCP监听通道指针
	 * @param[in] errcode 错误码
	 */
  virtual void deal_listen_closed(tcp_listen_ch *pch, int32 errcode) {};

  /**
	 * @brief 处理监听接受连接
	 * 
	 * 返回<0，底层认为出错，自动关闭通道。
	 * 
	 * @param[in] plistench 监听通道指针
	 * @param[in] clientaddr 客户端地址
	 * @param[in] fd 连接文件描述符
	 * @return int32 成功返回0，失败返回错误码
	 */
  virtual int32 deal_listen_accept(tcp_listen_ch *plistench, csock_addr &clientaddr, sock_fd fd) { return 0; }

  /**
	 * @brief 处理监听错误
	 * 
	 * @param[in] pch 监听通道指针
	 * @param[in] errcode 错误码
	 * @param[in] reasonerr 原因错误码
	 */
  virtual void deal_listen_error(tcp_listen_ch *pch, int32 errcode, int32 reasonerr) {};

  tcp_listen_op(){};
  virtual ~tcp_listen_op(){};
};

/**
 * @brief TCP监听通道类
 * 
 * 管理TCP监听服务，支持多连接并发处理。
 */
class tcp_listen_ch : private epoll_event_op {
protected:
  src_stat_ref thctl;   ///< 状态引用管理
  int32 sysfd;          ///< 系统文件描述符
  int32 loopnum;        ///< 单次循环处理数量
  int32 errcode;        ///< 错误码
  tcp_listen_op *pfunc; ///< 回调函数指针
  void *userdata;       ///< 用户数据

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
  void deal_after_closed();

public:
  /**
	 * @brief 监听TCP通道
	 * @param[in] attr 通道属性
	 * @param[in] tpfunc 异步回调函数指针
	 * @param[in] plocal 本地地址
	 * @param[in] puserdata 用户数据
	 * @return int32 成功返回0，失败返回错误码
	 */
  int32 listen_ch(channel_attr &attr, tcp_listen_op *tpfunc, csock_addr *plocal, void *puserdata);

  /**
	 * @brief 接受监听连接
	 * @param[out] o_fd 输出文件描述符
	 * @param[out] o_addr 输出地址
	 * @return int32 成功返回1，失败返回错误码
	 */
  int32 accept_listen(int32 &o_fd, csock_addr &o_addr);

  /**
	 * @brief 关闭监听通道
	 * @param[in] err 错误码
	 */
  void close_ch(int32 err = 0);

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
      deal_after_closed();
    }
  }

  /**
	 * @brief 检查是否工作状态
	 * @return bool 工作状态返回true，否则返回false
	 */
  FORCE_INLINE bool is_work() { return thctl.is_work(); }

  /**
	 * @brief 检查是否已关闭
	 * @return bool 已关闭返回true，否则返回false
	 */
  FORCE_INLINE bool is_close() { return thctl.is_close(); }

  /**
	 * @brief 检查是否空闲
	 * @return bool 空闲返回true，否则返回false
	 */
  FORCE_INLINE bool is_free() { return thctl.is_free(); }

  /**
	 * @brief 设置错误码
	 * @param[in] err 错误码
	 */
  FORCE_INLINE void set_err(int32 err) {
    if (err != 0)
      errcode = err;
  }

  FORCE_INLINE int32 sys_fd() { return atomic_load32(&sysfd); }

  FORCE_INLINE void *user_data() { return userdata; }

  /**
	 * @brief 开始监听
	 * @param[in] tplistenth 监听线程指针
	 * @param[in] oncetimes 单次处理数量
	 * @return int32 成功返回0，失败返回错误码
	 */
  int32 start_listen(mthread *tplistenth, int32 oncetimes = 0);

private:
  virtual int32 get_fd() { return atomic_load32(&sysfd); }

  /**
	 * @brief 处理轮询事件
	 */
  virtual void deal_event();

  /**
	 * @brief 处理轮询错误
	 */
  virtual void deal_error();

  /**
	 * @brief 处理轮询关闭
	 * 
	 * 从epoll移除后调用
	 */
  virtual void deal_close();

public:
  /**
	 * @brief 构造函数
	 */
  tcp_listen_ch() {
    sysfd = -1;
    loopnum = TCPCH_LISTEN_ONCELOOP_NUM;
    errcode = 0;
    pfunc = NULL;
    userdata = NULL;
  }

  /**
	 * @brief 析构函数
	 */
  virtual ~tcp_listen_ch() { destroy(); }
};

} // namespace lb_common

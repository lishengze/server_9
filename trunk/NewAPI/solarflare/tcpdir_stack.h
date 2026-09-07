#pragma once

/**
 * @file tcpdir_stack.h
 * @brief Solarflare TCPDirect 封装模块
 *
 * @details 封装 Solarflare TCPDirect 库的核心功能，提供低延迟和平衡两种模式的
 * TCP 通信能力。需要编译环境安装 Solarflare 网卡驱动和 TCPDirect 库，
 * 编译时由 CMake 检测并定义 HAS_TCPDIRECT 宏。
 *
 * 主要特性：
 * - tcpdir_stack: 封装 TCPDirect stack 操作和属性设置
 * - tcpdir_ch: 封装 TCP 连接及其收发操作，支持心跳和重连
 * - tcpdir_listener: 封装 listen 和 accept
 * - tcpdir_poll: 监听多个连接的数据到达事件
 *
 * 支持三种网络 IO 模型：
 * 1. 单链接模型: 一个 stack + 一个客户端主动链接 + 一个处理线程 + 线程外带锁发送，
 *    处理线程带锁不断轮询stack reactor,若有事件表示需要接收;轮询异步链接是否成功;轮询是否需发送心跳;轮询是否需重建链接
 * 2. 单链接模型: 一个 stack + 一个客户端主动链接 + 一个处理线程 + 发送队列
      处理线程不断轮询发送队列，发送数据或处理心跳发送和链接重建事件; 轮询stack reactor,若有事件表示需要接收;轮询异步链接是否成功
 * 3. 多链接poll模型: 一个 stack + 多个链接 + 一个处理线程 + 发送队列，
 *    独立线程轮询发送队列发送、zf_mux_wait 检查数据到达、接收回调、心跳、重连
 *    处理线程不断轮询发送队列，发送数据或处理心跳发送和链接重建事件; zf_mux_wait 轮询stack,若有事件表示需要接收;轮询异步链接是否成功
 *
 */

#include "comm_sock.h"
#include "comm_sys.h"
#include "heart_manage.h"
#include "state_machine.h"

#include <zf/muxer.h>
#include <zf/zf.h>
#include <zf/zf_reactor.h>
#include <zf/zf_tcp.h>

namespace lb_common {

/** @brief TCPDirect stack 低延时模式 */
#define TCPDIR_MODE_LOW_LATENCY 1
/** @brief TCPDirect stack 平衡模式 */
#define TCPDIR_MODE_BALANCED 2

/** @brief TCPDirect 单次接收循环默认次数 */
#define TCPDIR_RECV_LOOP_NUM 4
/** @brief TCPDirect 单次 poll 事件上限 */
#define TCPDIR_POLL_MAX_EVENTS 64
/** @brief TCPDirect 默认心跳间隔（秒） */
#define TCPDIR_HEART_INTERVAL_DEFAULT 5

class tcpdir_stack;
class tcpdir_ch;
class tcpdir_listener;
class tcpdir_poll;

/**
 * @brief tcpdir_poll 事件包装结构
 *
 * 用于 zf_mux_wait 返回的 epoll_event 中 data.ptr 字段。
 * 利用指针最低位做类型标记（对象对齐保证最低位为0）：
 * - 最低位为0: tcpdir_ch 指针
 * - 最低位为1: tcpdir_listener 指针（存储时做 ptr|1 标记）
 */
struct tcpdir_poll_ev_tag {
  /**
   * @brief 为 tcpdir_ch 指针打标签
   * @param[in] pch 通道指针
   * @return void* 标记后的指针（最低位0）
   */
  static FORCE_INLINE void *tag_ch(tcpdir_ch *pch) { return pch; }

  /**
   * @brief 为 tcpdir_listener 指针打标签
   * @param[in] pl 监听器指针
   * @return void* 标记后的指针（最低位1）
   */
  static FORCE_INLINE void *tag_listener(tcpdir_listener *pl) {
    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(pl) | 1);
  }

  /**
   * @brief 判断标签指针是否为普通通道
   * @param[in] p 标签指针
   * @return bool 是普通通道返回 true
   */
  static FORCE_INLINE bool is_ch(void *p) { return (reinterpret_cast<uintptr_t>(p) & 1) == 0; }

  /**
   * @brief 从标签指针还原 tcpdir_ch 指针
   * @param[in] p 标签指针
   * @return tcpdir_ch* 通道指针
   */
  static FORCE_INLINE tcpdir_ch *to_ch(void *p) { return reinterpret_cast<tcpdir_ch *>(p); }

  /**
   * @brief 从标签指针还原 tcpdir_listener 指针
   * @param[in] p 标签指针
   * @return tcpdir_listener* 监听器指针
   */
  static FORCE_INLINE tcpdir_listener *to_listener(void *p) {
    return reinterpret_cast<tcpdir_listener *>(reinterpret_cast<uintptr_t>(p) & (~uintptr_t(1)));
  }
};

/**
 * @brief TCPDirect 通道操作回调接口
 *
 * 定义 TCPDirect 通道生命周期中的各种回调函数，用户继承此类实现自定义逻辑。
 * 适用于两种 IO 模型中的链接状态通知。
 */
class tcpdir_ch_op {
public:
  /**
   * @brief 处理通道连接建立
   *
   * @param[in] pch 通道指针
   * @param[in] localaddr 本地地址引用
   * @note 对象建立链接成功后会调用，包括被动accept链接和主动connect链接
   */
  virtual void deal_ch_connect(tcpdir_ch *pch, csock_addr &localaddr) {};

  /**
   * @brief TCP 通道即将关闭回调
   *
   * 首次关闭成功后回调，此后收发操作将失败。
   *
   * @param[in] pch TCPDirect 通道指针
   * @param[in] errcode 错误码
   */
  virtual void deal_ch_closing(tcpdir_ch *pch, int32 errcode) {}

  /**
   * @brief TCP 通道完全关闭回调
   *
   * 通道资源已释放，可回收对象。若通道资源是可重用的，用户应维护可重用标识，并在此函数中标识资源可重新使用。
   *
   * @param[in] pch TCPDirect 通道指针
   * @param[in] errcode 错误码
   */
  virtual void deal_ch_closed(tcpdir_ch *pch, int32 errcode) {}

  /**
   * @brief TCP 通道错误回调
   *
   * @param[in] pch TCPDirect 通道指针
   * @param[in] errtype 错误类型
   * @param[in] errcode 错误码
   */
  virtual void deal_ch_error(tcpdir_ch *pch, int32 errtype, int32 errcode) {}

  tcpdir_ch_op() {}
  virtual ~tcpdir_ch_op() {}
};

/**
 * @brief TCPDirect 监听操作回调接口
 *
 * 定义 TCPDirect 监听器生命周期中的各种回调函数。
 */
class tcpdir_listener_op {
public:
  /**
   * @brief 新连接接受回调
   *
   * 监听器 accept 到新连接后调用，pch 已完成 accept 初始化。
   * 返回 <0 表示出错，底层关闭该连接。
   *
   * @param[in] plistener 监听器指针
   * @param[in] new_zft 新接受的通道指针
   * @param[in] clientaddr 客户端地址
   * @return int32 成功返回0，失败返回错误码
   */
  virtual int32 deal_listen_accept(tcpdir_listener *plistener, struct zft *new_zft, csock_addr &clientaddr) {
    return 0;
  }

  /**
   * @brief 监听器正在关闭中回调
   *
   * @param[in] plistener 监听器指针
   * @param[in] errcode 错误码
   */
  virtual void deal_listen_to_close(tcpdir_listener *plistener, int32 errcode) {}

  /**
   * @brief 监听器关闭回调
   *
   * @param[in] plistener 监听器指针
   * @param[in] errcode 错误码
   */
  virtual void deal_listen_close(tcpdir_listener *plistener, int32 errcode) {}

  /**
   * @brief 监听器错误回调
   *
   * @param[in] plistener 监听器指针
   * @param[in] errcode 错误码
   */
  virtual void deal_listen_error(tcpdir_listener *plistener, int32 errcode) {}

  tcpdir_listener_op() {}
  virtual ~tcpdir_listener_op() {}
};

/**
 * @brief TCPDirect 消息接收回调接口
 *
 * 用户继承此类实现消息解析逻辑，用于 loop_recv 中的消息分发。
 */
class tcpdir_msg_cb {
public:
  /**
   * @brief 处理接收到的消息
   *
   * @param[in] pch 通道指针
   * @param[in] pbuf 消息缓冲区
   * @param[in] len 消息长度
   * @return int32 成功返回已处理字节数（>0），消息不完整返回0，错误返回负数
   *
   * @note 一次回调中可能有多个用户消息+不完整消息，用户应尽量保证一次处理完所有完整消息
   */
  virtual int32 deal_msg(tcpdir_ch *pch, char *pbuf, int32 len) = 0;

  tcpdir_msg_cb() {}
  virtual ~tcpdir_msg_cb() {}
};

/**
 * @brief TCPDirect stack 属性结构
 *
 * 配置 TCPDirect stack 的各项参数，依据 mode 分为低延时和平衡两种模式。
 */
struct tcpdir_stack_attr {
  char interface_name[CHANNEL_ETH_NAME_LEN]; ///< 网卡接口名称
  int32 mode;                                ///< 模式: TCPDIR_MODE_LOW_LATENCY 或 TCPDIR_MODE_BALANCED
  int32 independ_send;                       ///< 独立的带锁发送模式，0 默认
  int32 max_connect_num;                     ///< 单个stack 最大支持的链接数，<=64
  int32 rx_ring_size;                        ///< 接收环大小，0 使用默认值
  int32 tx_ring_size;                        ///< 发送环大小，0 使用默认值

  /**
   * @brief 构造函数，设置默认值
   */
  tcpdir_stack_attr() {
    std::memset(interface_name, 0, sizeof(interface_name));
    mode = TCPDIR_MODE_LOW_LATENCY;
    independ_send = 0;
    max_connect_num = 16;
    rx_ring_size = 2048;
    tx_ring_size = 1024;
  }
};

/**
 * @brief TCPDirect 通道属性结构
 *
 * 配置通道的心跳、接收循环次数、重连等参数。
 */
struct tcpdir_ch_attr {
  int32 heart_interval; ///< 心跳间隔（秒），0=不启用心跳
  int32 recv_loop_num;  ///< 单次接收循环次数，默认 TCPDIR_RECV_LOOP_NUM
  int32 max_msg_size;   ///< 单个消息的最大长度
  void *puser_data;     ///< 用户数据指针

  /**
   * @brief 构造函数，设置默认值
   */
  tcpdir_ch_attr() {
    heart_interval = TCPDIR_HEART_INTERVAL_DEFAULT;
    recv_loop_num = TCPDIR_RECV_LOOP_NUM;
    max_msg_size = CHANNEL_MAC_MTU_LEN;
    puser_data = NULL;
  }
};

/**
 * @brief TCPDirect 通道连接状态定义
 */
#define TCPDIR_CH_STATE_IDLE       0 ///< 空闲
#define TCPDIR_CH_STATE_CONNECTING 1 ///< 连接中
#define TCPDIR_CH_STATE_WORKING    2 ///< 工作中
#define TCPDIR_CH_STATE_CLOSING    3 ///< 关闭中
#define TCPDIR_CH_STATE_CLOSED     4 ///< 已关闭

/**
 * @brief TCPDirect 通道事件类型，供外部发送队列使用
 */
#define TCPDIR_EV_SEND      0 ///< 发送数据事件
#define TCPDIR_EV_HEART     1 ///< 心跳发送事件
#define TCPDIR_EV_CLOSE     2 ///< 关闭连接事件
#define TCPDIR_EV_RECONNECT 3 ///< 重连事件

/**
 * @brief TCPDirect stack 类
 *
 * 封装 Solarflare TCPDirect stack 的创建、属性配置、poll 驱动等操作。
 * 支持低延时和平衡两种模式的属性设置。
 *
 * 使用流程：
 * 1. 调用 init_stack() 初始化 stack（内部自动调用 zf_init）
 * 2. 在工作线程中循环调用 poll_stack() 驱动 stack
 * 3. 使用完毕后调用 destroy() 释放资源
 */
class tcpdir_stack {
protected:
  spin_state_lock thctl;   ///< 状态引用管理
  struct zf_stack *pstack; ///< TCPDirect stack 指针
  struct zf_attr *pattr;   ///< TCPDirect 属性指针

  int32 zf_load_driver();
  int32 set_attr(tcpdir_stack_attr &attr);

  friend class tcpdir_ch;
  friend class tcpdir_listener;
  friend class tcpdir_poll;

public:
  /**
   * @brief 初始化 TCPDirect stack
   *
   * 根据 attr 中的模式配置属性，创建 stack。
   * 低延时模式: 小缓冲区、禁用 Nagle、忙轮询优化
   * 平衡模式: 较大缓冲区、标准配置
   *
   * @param[in] attr stack 属性配置
   * @return int32 成功返回0，失败返回错误码
   */
  int32 init_stack(tcpdir_stack_attr &attr);

  /**
   * @brief 驱动 stack 处理事件
   *
   * 必须在工作线程中循环调用，驱动 TCPDirect 的收发和状态处理。
   *
   * @return int32 成功返回0，失败返回错误码
   */
  int32 poll_stack();
  int32 poll_stack_lock();

  /**
   * @brief 销毁 stack
   *
   * 释放 stack 和 attr 资源。
   */
  void destroy();

  /**
   * @brief 获取内部 zf_stack 指针
   * @return struct zf_stack* stack 指针
   */
  FORCE_INLINE struct zf_stack *get_stack() const { return pstack; }

  /**
   * @brief 获取内部 zf_attr 指针
   * @return struct zf_attr* 属性指针
   */
  FORCE_INLINE struct zf_attr *get_attr() const { return pattr; }

  /**
   * @brief 检查是否已初始化
   * @return bool 已初始化返回 true
   */
  FORCE_INLINE bool is_work() const { return thctl.is_work(); }

  /**
   * @brief 检查是否关闭
   * @return bool 关闭返回 true
   */
  FORCE_INLINE bool is_close() const { return thctl.is_close(); }

  /**
   * @brief 构造函数
   */
  tcpdir_stack() : pstack(NULL), pattr(NULL) {}

  /**
   * @brief 析构函数
   */
  ~tcpdir_stack() { destroy(); }

  tcpdir_stack(const tcpdir_stack &) = delete;
  tcpdir_stack &operator=(const tcpdir_stack &) = delete;
};

/**
 * @brief TCPDirect 通道类
 *
 * 封装 TCPDirect TCP 连接的完整生命周期，支持：
 * - connect / accept 建立连接
 * - send / recv 数据收发
 * - 心跳检测和超时判定
 * - 重连状态管理
 * - 接收循环回调处理
 *
 * 支持两种 IO 模型：
 * - 模型1: 单链接，独立线程轮询接收，deal_ch_recv 回调通知
 * - 模型2: 多链接，由 tcpdir_poll 统一 zf_mux_wait，直接读取数据
 */
class tcpdir_ch {
protected:
  spin_state_lock thctl;     ///< 状态引用管理
  int32 max_msglen;          ///< 最大支持的消息长度
  struct zft *pzft;          ///< TCPDirect TCP zocket 指针
  tcpdir_stack *pstk;        ///< 所属 stack 指针
  int32 recv_loop_num;       ///< 单次接收循环次数
  int32 recv_len;            ///< 不完整包长度
  char *recv_buf;            ///< 不完整包缓存
  tcpdir_msg_cb *pmsg_cb;    ///< 消息接收回调
  struct zf_waitable *pwait; ///< poll 等待对象
  heart_manage heart;        ///< 心跳管理
  int32 errcode;             ///< 错误代码
  tcpdir_ch_op *pfunc;       ///< 通道操作回调
  void *puser_data;          ///< 用户数据

  /**
   * @brief 内部关闭处理
   */
  void destroy();

public:
  friend class tcpdir_poll;
  friend class tcpdir_listener;

  /**
   * @brief 发送数据
   *
   * 通过 TCPDirect 发送数据，非阻塞。
   *
   * @param[in] pmsg 消息缓冲区
   * @param[in] len 消息长度
   * @return int32 成功返回发送字节数（>0），无数据可发返回0，失败返回错误码（<0）
   */
  int32 send_msg(char *pmsg, int32 len);
  int32 send_msg_fc(char *pmsg, int32 len);
  int32 send_msg_lock(char *pmsg, int32 len);

  /**
   * @brief 接收数据
   *
   * 从 TCPDirect 接收数据，非阻塞。返回接收到的字节数。
   *
   * @param[out] pbuf 接收缓冲区
   * @param[in] len 缓冲区长度
   * @return int32 成功返回接收字节数（>0），无数据返回0，失败返回错误码（<0）
   */
  int32 recv_msg(char *pbuf, int32 len);
  int32 recv_msg_fc(char *pbuf, int32 len);
  int32 recv_msg_lock(char *pbuf, int32 len);

  /**
   * @brief 接收并回调处理消息循环
   *
   * 循环调用 recv_msg 接收数据，通过 pmsg_cb 回调用户处理消息。
   * 最多循环 recv_loop_num 次或直到无数据可读。
   * 适用于模型1中独立线程的接收处理，和模型2中 tcpdir_poll 事件驱动后的接收处理。
   *
   * @return int32 成功处理的次数，0=无数据，<0=错误
   */
  int32 loop_deal_recv();
  int32 loop_deal_recv_lock();

  /**
   * @brief 检查异步链接是否链接成功
   *
   * @return bool 需要重连返回 true
   */
  bool check_asyn_connected();

  /**
   * @brief 连接到远程服务器
   *
   * 创建 TCP zocket 并发起连接。
   * 对于同步连接，成功后状态为 WORKING。
   * 对于异步连接，需要后续调用 check_connect_done() 检查。
   *
   * @param[in] pstk 所属 stack 指针
   * @param[in] ch_attr 通道属性
   * @param[in] tpmsg_cb 消息接收回调
   * @param[in] tpfunc 通道操作回调
   * @param[in] premote 远程地址
   * @param[in] plocal 本地地址（可为 NULL）
   * @param[in] asyn_connect 是否采用异步链接
   * @return int32 成功返回1，异步连接中返回0，失败返回错误码（<0）
   */
  int32 connect_ch(tcpdir_stack *pstk, tcpdir_ch_attr &ch_attr, tcpdir_msg_cb *tpmsg_cb, tcpdir_ch_op *tpfunc,
                   csock_addr *premote, csock_addr *plocal = NULL, int32 asyn_connect = 0);

  /**
   * @brief 从 accept 初始化通道
   *
   * 由 tcpdir_listener 在 accept 后调用，完成通道初始化。
   *
   * @param[in] pstk 所属 stack 指针
   * @param[in] ch_attr 通道属性
   * @param[in] tpfunc 通道操作回调
   * @param[in] tpmsg_cb 消息接收回调
   * @param[in] pzft 已 accept 的 zocket 指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 accept_ch(tcpdir_stack *pstk, tcpdir_ch_attr &ch_attr, tcpdir_msg_cb *tpmsg_cb, tcpdir_ch_op *tpfunc,
                  struct zft *pzft);

  /**
   * @brief 关闭通道
   *
   * @param[in] err 错误码，默认0
   */
  void close_ch(int32 err = 0);

  /**
   * @brief 获取心跳间隔
   * @return int32 心跳间隔（秒）
   */
  FORCE_INLINE int32 get_heart_interval() const { return heart.get_interval(); }

  /**
   * @brief 触发心跳消息
   *
   * 收到心跳响应时调用，更新心跳时间。
   */
  FORCE_INLINE void on_heart_msg() { heart.on_heart(); }

  /**
   * @brief 触发普通消息
   *
   * 收到业务消息时调用，更新消息计数。
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
   * @brief 获取用户数据
   * @return void* 用户数据指针
   */
  FORCE_INLINE void *get_user_data() const { return puser_data; }

  /**
   * @brief 获取内部 zft 指针
   *
   * 用于 tcpdir_poll 注册事件。
   *
   * @return struct zft* zocket 指针
   */
  FORCE_INLINE struct zft *get_zft() const { return pzft; }

  FORCE_INLINE bool is_work() const { return thctl.is_work(); }

  FORCE_INLINE bool is_close() const { return thctl.is_close(); }

  FORCE_INLINE bool is_free() const { return thctl.is_free(); }

  /**
   * @brief 获取所属 stack 指针
   * @return tcpdir_stack* stack 指针
   */
  FORCE_INLINE tcpdir_stack *get_stack() const { return pstk; }

  /**
   * @brief 构造函数
   */

  tcpdir_ch()
      : max_msglen(0), pzft(NULL), pstk(NULL), recv_loop_num(TCPDIR_RECV_LOOP_NUM), recv_len(0), recv_buf(NULL),
        pmsg_cb(NULL), pwait(NULL), errcode(0), pfunc(NULL), puser_data(NULL) {}

  /**
   * @brief 析构函数
   */
  ~tcpdir_ch() { destroy(); };

  tcpdir_ch(const tcpdir_ch &) = delete;
  tcpdir_ch &operator=(const tcpdir_ch &) = delete;
};

/**
 * @brief TCPDirect 监听器类
 *
 * 封装 TCPDirect 的 listen 和 accept 操作。
 * 在指定地址上监听，接受新连接后创建 tcpdir_ch 通道对象。
 */
class tcpdir_listener {
protected:
  spin_state_lock thctl;     ///< 状态引用管理
  tcpdir_stack *pstk;        ///< 所属 stack 指针
  struct zftl *pzftl;        ///< TCPDirect TCP listener 指针
  struct zf_waitable *pwait; ///< 等待对象指针
  int32 accept_loop_num;     ///< 单次接受链接循环次数
  int32 errcode;             ///< 错误码
  csock_addr local_addr;     ///< 监听地址
  tcpdir_listener_op *pfunc; ///< 监听回调
  void *puser_data;          ///< 用户数据指针

  void destroy();

public:
  friend class tcpdir_poll;

  /**
   * @brief 启动监听
   *
   * 创建 listener zocket 并开始监听指定地址。
   *
   * @param[in] pstk 所属 stack 指针
   * @param[in] once_loop_num 一次接收链接数量
   * @param[in] tpfunc 监听操作回调
   * @param[in] plocal 监听地址
   * @param[in] puserdata 用户数据
   * @return int32 成功返回0，失败返回错误码
   */
  int32 listen_ch(tcpdir_stack *pstk, int32 once_loop_num, tcpdir_listener_op *tpfunc, csock_addr *plocal,
                  void *puserdata);

  /**
   * @brief 尝试接受一个新连接
   *
   * 非阻塞模式，无连接时返回0。
   *
   * @param[out] o_zft 返回新建的zft 对象
   * @param[out] o_addr 返回对端地址
   * @return int32 成功返回1，无连接返回0，失败返回错误码（<0）
   */
  int32 accept_listen(struct zft *&o_zft, csock_addr &o_addr);

  /**
   * @brief 循环尝试接受新连接
   *
   * 非阻塞模式，无连接时返回0。
   * 成功后，调用 deal_listen_accept 回调。
   *
   * @return int32 成功返回链接数量，无连接返回0，失败返回错误码（<0）
   */
  int32 loop_deal_accept();

  /**
   * @brief 关闭监听器
   *
   * @param[in] err 错误码，默认0
   */
  void close_ch(int32 err = 0);

  /**
   * @brief 检查关闭状态，真正关闭和回收资源
   */
  void check_close();

  /**
   * @brief 检查是否工作状态
   * @return bool 工作中返回 true
   */
  FORCE_INLINE bool is_work() const { return thctl.is_work(); }

  /**
   * @brief 检查是否已关闭
   * @return bool 已关闭返回 true
   */
  FORCE_INLINE bool is_close() const { return thctl.is_close(); }

  /**
   * @brief 获取内部 zftl 指针
   * @return struct zftl* listener 指针
   */
  FORCE_INLINE struct zftl *get_zftl() const { return pzftl; }

  /**
   * @brief 获取监听地址
   * @return const csock_addr& 监听地址引用
   */
  FORCE_INLINE const csock_addr &get_local_addr() const { return local_addr; }

  /**
   * @brief 获取用户数据
   * @return void* 用户数据指针
   */
  FORCE_INLINE void *get_user_data() const { return puser_data; }

  /**
   * @brief 获取所属 stack 指针
   * @return tcpdir_stack* stack 指针
   */
  FORCE_INLINE tcpdir_stack *get_stack() const { return pstk; }

  /**
   * @brief 构造函数
   */
  tcpdir_listener()
      : pstk(NULL), pzftl(NULL), pwait(NULL), accept_loop_num(4), errcode(0), pfunc(NULL), puser_data(NULL) {
    std::memset(&local_addr, 0, sizeof(local_addr));
  }

  /**
   * @brief 析构函数
   */
  ~tcpdir_listener() { destroy(); }

  tcpdir_listener(const tcpdir_listener &) = delete;
  tcpdir_listener &operator=(const tcpdir_listener &) = delete;
};

/**
 * @brief TCPDirect 轮询器类
 *
 * 监听多个 tcpdir_ch 和 tcpdir_listener 的数据到达事件。
 * 基于 zf_mux_wait 实现多连接的事件通知，适用于模型2（多链接场景）。
 *
 * 事件类型区分机制：
 * 使用 tagged pointer（指针最低位标记）区分 tcpdir_ch 和 tcpdir_listener：
 * - 最低位为0: tcpdir_ch 指针
 * - 最低位为1: tcpdir_listener 指针
 *
 * 使用流程：
 * 1. 调用 init() 初始化轮询器
 * 2. 调用 add_ch() 注册需要监听的通道
 * 3. 调用 add_listener() 注册需要监听的监听器
 * 4. 在工作线程中循环调用 poll_and_wait() 或 wait_and_process()
 * 5. 不再需要时调用 remove_ch() / remove_listener() 移除
 */
class tcpdir_poll {
protected:
  tcpdir_stack *pstk;                                ///< 所属 stack 指针
  struct zf_muxer_set *pmuxer;                       ///< 事件队列 poll 对象指针
  struct epoll_event events[TCPDIR_POLL_MAX_EVENTS]; ///< 事件数组
  int32 max_events;                                  ///< 最大事件数
  int32 inited;                                      ///< 是否已初始化

public:
  /**
   * @brief 初始化轮询器
   *
   * @param[in] pstk 所属 stack 指针
   * @param[in] tmax_events 最大事件数，默认 TCPDIR_POLL_MAX_EVENTS
   * @return int32 成功返回0，失败返回错误码
   */
  int32 init(tcpdir_stack *pstk, int32 tmax_events = TCPDIR_POLL_MAX_EVENTS);

  /**
   * @brief 添加通道到轮询监听
   *
   * 注册通道的可读事件，zf_mux_wait 返回时通知该通道有数据。
   *
   * @param[in] pch 通道指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 add_ch(tcpdir_ch *pch);

  /**
   * @brief 重置通道到轮询监听
   *
   * 重置通道的可读事件，以避免边缘触发一次处理未完成而事件丢失。
   *
   * @param[in] pch 通道指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 mod_ch(tcpdir_ch *pch);

  /**
   * @brief 移除通道的轮询监听
   *
   * @param[in] pch 通道指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 remove_ch(tcpdir_ch *pch);

  /**
   * @brief 添加监听器到轮询监听
   *
   * 注册监听器的可读事件，有新连接时通知。
   *
   * @param[in] plistener 监听器指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 add_listener(tcpdir_listener *plistener);

  /**
   * @brief 重置通道到轮询监听
   *
   * 重置通道的链接accept事件，以避免边缘触发一次处理未完成而事件丢失。
   *
   * @param[in] plistener 通道指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 mod_listener(tcpdir_listener *plistener);

  /**
   * @brief 移除监听器的轮询监听
   *
   * @param[in] plistener 监听器指针
   * @return int32 成功返回0，失败返回错误码
   */
  int32 remove_listener(tcpdir_listener *plistener);

  /**
   * @brief 等待事件并处理
   *
   * 调用 zf_mux_wait 等待事件，然后对每个事件：
   * - 若为通道事件，调用该通道的 loop_recv 接收数据
   * - 若为监听器事件，调用 accept_ch 接受新连接
   *
   * @param[in] timeout_ns 等待超时（纳秒），0=非阻塞，-1=无限等待
   * @return int32 成功返回处理的事件数，失败返回错误码（<0）
   */
  int32 wait(int64 timeout_ns);

  /**
   * @brief 销毁轮询器
   */
  void destroy();

  /**
   * @brief 检查是否已初始化
   * @return bool 已初始化返回 true
   */
  FORCE_INLINE bool is_inited() const { return inited == 1; }

  /**
   * @brief 构造函数
   */
  tcpdir_poll() : pstk(NULL), pmuxer(NULL), max_events(0), inited(0) {}

  /**
   * @brief 析构函数
   */
  ~tcpdir_poll() { destroy(); }

  tcpdir_poll(const tcpdir_poll &) = delete;
  tcpdir_poll &operator=(const tcpdir_poll &) = delete;
};

} // namespace lb_common

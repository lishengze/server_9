#pragma once

/**
 * @file comm_aio.h
 * @brief AIO公共定义模块
 *
 * 提供AIO相关的公共定义和接口：
 * - AIO配置参数和消息结构体
 * - 通道消息接收回调基类
 * - AIO通道操作回调基类
 * - 错误类型定义
 */

#include "comm_sock.h"
#include "comm_sys.h"

namespace lb_common {

/** @brief 一次poll唤醒循环接收次数 */
#define AIO_RECV_ONCE_NUM 4
/** @brief 一次接收最小长度 */
#define AIO_RECV_ONCE_MIN_LEN 248
/** @brief 一次接收最大长度 */
#define AIO_RECV_ONCE_MAX_LEN 4088

/**
 * @brief AIO配置参数结构体
 */
struct aio_attr {
  int32 onerecvtimes;       /**< 一次poll唤醒，循环接收几次 */
  int32 oncerecvlen;        /**< 一次接收的消息长度，小于等于maxmsglen */
  int32 maxmsglen;          /**< 支持的最大消息长度 */
  int32 buf_size;           /**< 缓存大小，不同缓存类型意义不同 */
  int32 dispatch_zero_copy; /**< 接收消息分发处理时，是否使用零拷贝 */
  int32 heartinterval;      /**< 心跳间隔，0-无心跳 */
  void *puserdata;          /**< 用户数据指针 */
};

/**
 * @brief AIO消息结构体
 */
struct aio_msg {
  char *pmsg;     /**< 指向消息 */
  int32 msglen;   /**< 消息长度 */
  int64 buf_addr; /**< 消息的缓存地址 */
};

/**
 * @brief 通道消息接收回调基类
 *
 * @tparam CH 通道类型
 */
template <class CH> class ch_recv_cb {
public:
  /**
   * @brief 处理接收到的消息
   *
   * 底层依赖该函数返回的长度，处理缓存。
   * @param[in] pch 通道指针
   * @param[in] msg 消息结构体引用
   * @return int32 返回值含义：
   *         - >0 : 成功解析或处理长度
   *         - =0 : 未处理，比如消息头不完整或消息不完整
   *         - <0 : 出错，底层会丢弃本次消息，并调用通道错误通知函数
   */
  virtual int32 deal_msg(CH *pch, aio_msg &msg) = 0;

  /**
   * @brief 默认构造函数
   */
  ch_recv_cb(){};
  /**
   * @brief 虚析构函数
   */
  virtual ~ch_recv_cb(){};
};

/** @brief 接收错误类型 */
#define AIO_CHERR_TYPE_RECV 1
/** @brief 处理错误类型 */
#define AIO_CHERR_TYPE_DEAL 2
/** @brief 发送错误类型 */
#define AIO_CHERR_TYPE_SEND 3
/** @brief EPOLL错误类型 */
#define AIO_CHERR_TYPE_EPOLL 4
/** @brief 监听错误类型 */
#define AIO_CHERR_TYPE_LISTEN 5
/** @brief 连接错误类型*/
#define AIO_CHERR_TYPE_CONNECT 6
/* 接受错误类型（未使用）
#define AIO_CHERR_TYPE_ACCEPT    7
*/

/**
 * @brief AIO通道操作回调基类
 *
 * @tparam CH 通道类型
 */
template <class CH> class aio_ch_op {
public:
  /**
   * @brief 处理通道错误
   *
   * @param[in] pch 通道指针
   * @param[in] err_type 错误类型：如处理出错，接收错误，发送错误，链接错误等
   * @param[in] err_code 错误码
   * @note 用户可在此主动关闭对象
   */
  virtual void deal_ch_error(CH *pch, int32 err_type, int32 err_code) {};

  /**
   * @brief 处理通道正在关闭
   *
   * @param[in] pch 通道指针
   * @param[in] errcode 错误码
   * @note
   * 首次关闭成功后回调，多线程使用时底层保证只会调用一次，但不一定哪个线程
   * @note 回调后，用户可在此函数关闭对象引用，使对象不再可见或不会产生新的引用
   */
  virtual void deal_ch_closing(CH *pch, int32 errcode) {};

  /**
   * @brief 处理接收停止
   *
   * @param[in] pch 通道指针
   * @note 接收线程移除监听对象后，调用通知
   */
  virtual void deal_recv_stop(CH *pch) {};

  /**
   * @brief 处理通道已关闭
   *
   * @param[in] pch 通道指针
   * @param[in] errcode 错误码
   * @note 多线程下，各线程都结束使用，用户在此可释放此对象资源或对象。若通道资源是可重用的，用户应维护可重用标识，并在此函数中标识资源可重新使用。
   */
  virtual void deal_ch_closed(CH *pch, int32 errcode) {};

  /**
   * @brief 处理通道连接建立
   *
   * @param[in] pch 通道指针
   * @param[in] localaddr 本地地址引用
   * @note 对象建立链接成功后会调用，包括被动accept链接和主动connect链接
   */
  virtual void deal_ch_connect(CH *pch, csock_addr &localaddr) {};

  /**
   * @brief 默认构造函数
   */
  aio_ch_op(){};
  /**
   * @brief 虚析构函数
   */
  virtual ~aio_ch_op(){};
};

} // namespace lb_common

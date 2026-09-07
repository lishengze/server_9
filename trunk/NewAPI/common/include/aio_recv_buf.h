
#pragma once

/**
 * @file aio_recv_buf.h
 * @brief AIO 通道私有的接收缓存类
 * @details 基于单线程接收的临时缓存实现，支持分片接收和处理结果提交。
 */

#include "comm_aio.h"
#include "comm_errno.h"
#include "comm_sys.h"
#include "mlock.h"

#include <cstring>
#include <new>

namespace lb_common {

/**
 * @brief AIO接收缓存类
 * @details
 * 单线程接收，作为临时缓存只支持回调后提交消息长度,每个通道有独占缓存,在接收线程移除通道后的回调，关闭该缓存。
 */
class aio_recv_buf {
protected:
  atomic_lock buflock; ///< 缓存锁
  int32 maxmsglen;     ///< 最大消息长度
  int32 recvlen;       ///< 已接收长度
  int16 onerecvlen;    ///< 单次接收的最大长度，若分片则等于分片包大小
  int16 bufflag;       ///< 缓存标志
  char *curbuf;        ///< 当前缓存指针

  /**
   * @brief 销毁缓存内存
   * @details 释放缓存内存并重置状态，加锁保护确保线程安全。
   */
  void destroy() {
    buflock.lock();
    if (NULL != curbuf && bufflag == 1) {
      delete[] curbuf;
    }
    curbuf = NULL;
    bufflag = 0;
    recvlen = 0;
    buflock.unlock();
  }

public:
  /**
   * @brief 接收消息
   *
   * @tparam CH 通道类型
   * @param[in] pch 通道引用
   * @param[out] o_msg 输出的消息结构体
   * @return int32 返回值含义：
   *         - >0 : o_msg有效
   *         - =0 : o_msg无效
   *         - <0 : 通道接收出错
   */
  template <class CH> int32 recv_msg(CH &pch, aio_msg &o_msg) {
    int32 rlen = recvlen < onerecvlen ? (onerecvlen - recvlen) : (maxmsglen - recvlen);
    if (unlikely(rlen <= 0)) {
      return LBERR_OBJ_NUM_LIMIT;
    }
    int32 ret = pch.recv_msg(curbuf + recvlen, rlen);
    if (likely(ret > 0)) {
      ret += recvlen;
      recvlen = ret;
      o_msg.pmsg = curbuf;
      o_msg.msglen = ret;
      o_msg.buf_addr = 0;
    }
    return ret;
  }

  /**
   * @brief 除非通道关闭，否则强制接收指定长度的数据。
   *
   * @tparam CH 通道类型
   * @param[in] pch 通道引用
   * @param[out] o_msg 输出的消息结构体
   * @return int32 接收结果,同上
   */
  template <class CH> int32 recv_msg_fc(CH &pch, aio_msg &o_msg) {
    int32 rlen = recvlen < onerecvlen ? (onerecvlen - recvlen) : (maxmsglen - recvlen);
    if (unlikely(rlen <= 0)) {
      return LBERR_OBJ_NUM_LIMIT;
    }
    int32 ret = pch.recv_msg_fc(curbuf + recvlen, rlen);
    if (likely(ret > 0)) {
      ret += recvlen;
      recvlen = ret;
      o_msg.pmsg = curbuf;
      o_msg.msglen = ret;
      o_msg.buf_addr = 0;
    }
    return ret;
  }

  /**
   * @brief 调用消息处理后，提交处理结果
   *
   * @param[in] deal_len 处理长度，为回调函数返回值
   */
  FORCE_INLINE void cmt_deal(int32 deal_len) {
    if (likely(deal_len > 0)) {
      int32 clen = deal_len < recvlen ? deal_len : recvlen;
      recvlen -= clen;
      if (unlikely(recvlen > 0)) {
        std::memmove(curbuf, curbuf + clen, recvlen);
      }
    } else if (deal_len < 0) {
      recvlen = 0;
    }
  }

  /**
   * @brief 判断接收的消息是否存在未处理的
   *
   * @return true 处理完成
   * @return false 处理未完成
   */
  FORCE_INLINE bool is_complete() const { return true; } //(recvlen == 0);}

  /**
   * @brief 提交缓存（空实现）
   *
   * @param[in] buf_addr 缓存地址
   * @param[in] cmt_len 提交长度
   */
  FORCE_INLINE void cmt_buf(int64 buf_addr, int32 cmt_len) {};

  /**
   * @brief 读取缓存未处理的消息
   *
   * @param[out] o_pmsg 输出的消息指针
   * @param[in] buf_addr 缓存地址
   */
  FORCE_INLINE void read_buf(char *&o_pmsg, int64 buf_addr) const {
    int32 rt = recvlen;
    if (rt > 0) {
      o_pmsg = curbuf;
    } else {
      o_pmsg = NULL;
    }
  }

  /**
   * @brief 构建缓存
   *
   * @param[in,out] attr AIO属性配置
   * @return int32 0=成功，其他=错误码
   */
  int32 build_buf(aio_attr &attr) {
    assert(attr.maxmsglen > 0);
    attr.buf_size = attr.maxmsglen;
    if (attr.oncerecvlen <= 0)
      attr.oncerecvlen = AIO_RECV_ONCE_MIN_LEN;
    if (attr.oncerecvlen > AIO_RECV_ONCE_MAX_LEN)
      attr.oncerecvlen = AIO_RECV_ONCE_MAX_LEN;
    if (attr.oncerecvlen > attr.maxmsglen)
      attr.oncerecvlen = attr.maxmsglen;

    buflock.lock();
    if (NULL != curbuf) {
      buflock.unlock();
      return 0;
    }

    maxmsglen = attr.maxmsglen;
    recvlen = 0;
    onerecvlen = attr.oncerecvlen;
    curbuf = new (std::nothrow) char[attr.buf_size];
    if (NULL == curbuf) {
      buflock.unlock();
      return LBERR_MEM_ALLOC_FAIL;
    }
    bufflag = 1;
    buflock.unlock();
    return 0;
  }

  /**
   * @brief 初始化缓存
   *
   * @param[in,out] attr AIO属性配置
   * @param[in] pbuf 外部提供的缓存指针
   */
  void init_buf(aio_attr &attr, char *pbuf) {
    assert(NULL != pbuf);
    assert(attr.maxmsglen > 0);
    attr.buf_size = attr.maxmsglen;
    if (attr.oncerecvlen <= 0)
      attr.oncerecvlen = AIO_RECV_ONCE_MIN_LEN;
    if (attr.oncerecvlen > AIO_RECV_ONCE_MAX_LEN)
      attr.oncerecvlen = AIO_RECV_ONCE_MAX_LEN;
    if (attr.oncerecvlen > attr.maxmsglen)
      attr.oncerecvlen = attr.maxmsglen;

    buflock.lock();
    if (NULL != curbuf) {
      buflock.unlock();
      return;
    }

    maxmsglen = attr.maxmsglen;
    recvlen = 0;
    onerecvlen = attr.oncerecvlen;
    bufflag = 0;
    curbuf = pbuf;
    buflock.unlock();
  }

  /**
   * @brief 是否已经初始化
   */
  bool is_init() const { return (NULL != curbuf); }

  /**
   * @brief 重置本通道的接收的未完成的消息
   * @note 在通道关闭时，需要调用此函数，否则会残留未处理的消息。
   */
  void reset_buf() { recvlen = 0; }

  /**
   * @brief 关闭缓存
   */
  void close_buf() { destroy(); }

  /**
   * @brief 构造函数
   */
  aio_recv_buf() : maxmsglen(0), recvlen(0), onerecvlen(0), bufflag(0), curbuf(NULL) {}
  /**
   * @brief 析构函数
   */
  ~aio_recv_buf() { destroy(); }
};

} // namespace lb_common

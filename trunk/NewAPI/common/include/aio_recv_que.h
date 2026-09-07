#pragma once

/**
 * @file aio_recv_que.h
 * @brief AIO接收队列操作类
 *
 * @details
 * 基于可共享的单写单读队列的AIO接收缓存实现，多通道单线程共享队列，私有该操作类。
 */

#include "comm_aio.h"
#include "comm_errno.h"
#include "comm_sys.h"
#include "que_swr_buf.h"

/* #include "que_mth_buf.h" */
#include "mlock.h"
#include "mutils.h"

#include <cstring>

namespace lb_common {

/**
 * @brief AIO接收队列操作类
 *
 * @details 基于单写单读队列的AIO接收缓存操作类。
 * 支持多通道共享队列，提供高效的队列式内存管理。
 * 共享队列的通道应在同一个线程中接收数据。
 *
 * @warning 不适合用在请求应答模式中，因为应答模式中，消息数据应与通道一起存储。
 */
class aio_recv_que {
protected:
  cmutex buflock;   /**< 缓存锁 */
  int32 recvlen;    /**< 已接收长度 */
  int32 maxmsglen;  /**< 最大消息长度 */
  int32 onerecvlen; /**< 单次接收的最大长度 */
  int32 bufflag;    /**< 缓存标志 */
  char *curbuf;     /**< 当前缓存指针 */
  int64 buf_pos;    /**< 缓存位置 */
  que_swr_buf *mq;  /**< 单写单读队列指针 */

  /**
   * @brief 销毁接收队列
   */
  void destroy() {
    buflock.lock();
    if (NULL != mq && bufflag == 1) {
      comm_utils::aligned_free(reinterpret_cast<void *>(mq));
    }
    mq = NULL;
    curbuf = NULL;
    bufflag = 0;
    recvlen = 0;
    buf_pos = 0;
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
    int32 rlen = onerecvlen;
    if (recvlen == 0) {
      char *tnbuf = NULL;
      while ((buf_pos = mq->write_get(tnbuf, maxmsglen)) <= 0) {
      };
      curbuf = tnbuf;
    } else {
      rlen = recvlen < onerecvlen ? (onerecvlen - recvlen) : (maxmsglen - recvlen);
    }
    if (unlikely(rlen <= 0)) {
      return LBERR_OBJ_NUM_LIMIT;
    }

    int32 ret = pch.recv_msg(curbuf + recvlen, rlen);
    if (likely(ret > 0)) {
      ret += recvlen;
      recvlen = ret;
      o_msg.pmsg = curbuf;
      o_msg.msglen = ret;
      o_msg.buf_addr = buf_pos;
    }
    return ret;
  }

  /**
   * @brief 除非通道关闭，否则强制接收指定长度的数据。
   *
   * @tparam CH 通道类型
   * @param[in] pch 通道引用
   * @param[out] o_msg 输出的消息结构体
   * @return int32 接收结果，同上
   */
  template <class CH> int32 recv_msg_fc(CH &pch, aio_msg &o_msg) {
    int32 rlen = onerecvlen;
    if (recvlen == 0) {
      char *tnbuf = NULL;
      while ((buf_pos = mq->write_get(tnbuf, maxmsglen)) <= 0) {
      };
      curbuf = tnbuf;
    } else {
      rlen = recvlen < onerecvlen ? (onerecvlen - recvlen) : (maxmsglen - recvlen);
    }

    int32 ret = pch.recv_msg_fc(curbuf + recvlen, rlen);
    if (likely(ret > 0)) {
      ret += recvlen;
      recvlen = ret;
      o_msg.pmsg = curbuf;
      o_msg.msglen = ret;
      o_msg.buf_addr = buf_pos;
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
      mq->write_cmt(buf_pos, deal_len);

      if (recvlen > 0) {
        char *tnbuf = NULL;
        while ((buf_pos = mq->write_get(tnbuf, maxmsglen)) <= 0) {
        };
        // CACHE_PREFETCH_L2(tnbuf,1);
        curbuf += clen;
        if (tnbuf != curbuf) {
          std::memmove(tnbuf, curbuf, recvlen);
        }
        curbuf = tnbuf;
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
  FORCE_INLINE bool is_complete() const { return (recvlen == 0); }

  /**
   * @brief 提交缓存
   *
   * @warning 不检查cmt_len，用户应保证一次提交完整的消息。
   *
   * @param[in] buf_addr 缓存地址
   * @param[in] cmt_len 提交长度
   */
  FORCE_INLINE void cmt_buf(int64 buf_addr, int32 cmt_len) {
    assert(buf_addr > 0 && cmt_len >= 0);
    mq->read_cmt(buf_addr + cmt_len);
  }

  /**
   * @brief 读取缓存数据
   *
   * @param[out] o_pmsg 输出的消息指针
   * @param[in] buf_addr 缓存地址
   */
  FORCE_INLINE void read_buf(char *&o_pmsg, int64 buf_addr) const {
    if (likely(buf_addr > 0))
      o_pmsg = mq->get_data(buf_addr);
    else
      o_pmsg = NULL;
  }

  /**
   * @brief 构建接收队列
   *
   * @param[in,out] attr AIO属性配置
   * @return int32 0=成功，其他=错误码
   */
  int32 build_buf(aio_attr &attr) {
    assert(attr.maxmsglen > 0 && attr.buf_size > 0);

    if (attr.buf_size < attr.maxmsglen * 16)
      attr.buf_size = attr.maxmsglen * 16;
    if (attr.buf_size < 65536)
      attr.buf_size = 65536;
    if (attr.oncerecvlen <= 0)
      attr.oncerecvlen = AIO_RECV_ONCE_MIN_LEN;
    if (attr.oncerecvlen > AIO_RECV_ONCE_MAX_LEN)
      attr.oncerecvlen = AIO_RECV_ONCE_MAX_LEN;
    if (attr.oncerecvlen > attr.maxmsglen)
      attr.oncerecvlen = attr.maxmsglen;

    buflock.lock();
    if (NULL != mq) {
      buflock.unlock();
      return 0;
    }

    recvlen = 0;
    maxmsglen = attr.maxmsglen;
    onerecvlen = attr.oncerecvlen;
    curbuf = NULL;
    buf_pos = 0;

    int64 ts = que_swr_buf::need_buf_size(attr.buf_size);
    int64 ts_que = sizeof(que_swr_buf);
    ts_que = ALIGN_UP(ts_que, CACHE_ALIGN_SIZE);
    ts += ts_que;

    void *tpm = comm_utils::aligned_malloc(ts, CACHE_ALIGN_SIZE);
    if (NULL == tpm) {
      buflock.unlock();
      return LBERR_MEM_ALLOC_FAIL;
    }

    mq = reinterpret_cast<que_swr_buf *>(tpm);
    char *tpc = static_cast<char *>(tpm);
    tpc += ts_que;

    assert(0 == mq->init(tpc, ts - ts_que, 0, 0, 8));
    bufflag = 1;

    buflock.unlock();
    return 0;
  }

  /**
   * @brief 使用共享的队列初始化。
   *
   * @param[in,out] attr AIO属性配置
   * @param[in] share_queue 共享的队列指针
   */
  void init_buf(aio_attr &attr, que_swr_buf *share_queue) {
    assert(NULL != share_queue);
    assert(attr.maxmsglen > 0 && attr.buf_size > 0);
    assert(attr.buf_size >= attr.maxmsglen * 16);

    if (attr.oncerecvlen <= 0)
      attr.oncerecvlen = AIO_RECV_ONCE_MIN_LEN;
    if (attr.oncerecvlen > AIO_RECV_ONCE_MAX_LEN)
      attr.oncerecvlen = AIO_RECV_ONCE_MAX_LEN;
    if (attr.oncerecvlen > attr.maxmsglen)
      attr.oncerecvlen = attr.maxmsglen;

    buflock.lock();
    if (NULL != mq) {
      buflock.unlock();
      return;
    }

    recvlen = 0;
    maxmsglen = attr.maxmsglen;
    onerecvlen = attr.oncerecvlen;
    bufflag = 0;
    curbuf = NULL;
    buf_pos = 0;
    mq = share_queue;

    buflock.unlock();
  }

  /**
   * @brief 是否已经初始化
   */
  bool is_init() const { return (NULL != mq); }

  /**
   * @brief 重置本通道的接收的未完成的消息
   * @note 在通道关闭时，需要调用此函数，否则会残留未处理的消息。
   */
  void reset_buf() { recvlen = 0; }

  /**
   * @brief 关闭接收队列
   */
  void close_buf() { destroy(); }

  /**
   * @brief 构造函数
   */
  aio_recv_que()
      : recvlen(0), maxmsglen(0), onerecvlen(0), bufflag(0), curbuf(NULL), buf_pos(0),
        mq(NULL){
            // static_assert((N&7)==0 && N<=CACHE_ALIGN_SIZE,"N must
            // in 8,16,32,64");
        };
  /**
   * @brief 析构函数
   */
  ~aio_recv_que() { destroy(); }
};

} // namespace lb_common

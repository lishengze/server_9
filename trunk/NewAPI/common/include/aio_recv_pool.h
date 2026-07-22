
#pragma once

/**
 * @file aio_recv_pool.h
 * @brief AIO 通道接收缓存池操作类
 * @details
 * 基于共享的固定内存池的AIO接收缓存实现，多通道多线程共享该缓存池，私有自己的该操作类。
 * 提供高效的内存管理。适用于高并发网络环境下的数据接收。
 * @note 该模块使用模板编程，支持自定义缓存块大小
 */

#include "comm_aio.h"
#include "comm_errno.h"
#include "comm_sys.h"
#include "fix_pool.h"
#include "mlock.h"
#include "mutils.h"

#include <cstring>

namespace lb_common {

/** @brief AIO接收缓存池默认长度 */
#define AIO_RECV_POOL_DEFAULT_LEN 2048

/**
 * @brief AIO缓存池项目结构体
 *
 * @tparam N 缓存块大小
 */
template <int N> struct aio_pool_item {
  char buf[N]; /**< 缓存数据 */
};

/**
 * @brief AIO接收缓存池类
 *
 * @details 基于固定内存池的AIO接收缓存实现。
 * 支持多通道在多线程中共享缓存池，提供高效的内存管理。
 *
 * @tparam N 缓存块大小，必须大于等于最大消息长度
 */
template <int N> class aio_recv_pool {
protected:
  using m_it = aio_pool_item<N>; /**< 缓存项目类型别名 */

  cmutex buflock;        /**< 缓存锁 */
  int32 maxmsglen;       /**< 最大消息长度 */
  int32 recvlen;         /**< 已接收长度 */
  int32 onerecvlen;      /**< 单次接收的最大长度，若分片则等于分片包大小 */
  char *curbuf;          /**< 当前缓存指针 */
  int32 buf_pos;         /**< 缓存位置 */
  int32 bufflag;         /**< 缓存标志 */
  fix_pool<m_it> *mpool; /**< 固定内存池指针 */

  /**
   * @brief 销毁缓存池
   */
  void destroy() {
    buflock.lock();
    if (NULL != mpool && buf_pos >= 0) {
      mpool->release(buf_pos);
      buf_pos = -1;
    }
    if (NULL != mpool && bufflag == 1) {
      comm_utils::aligned_free(reinterpret_cast<void *>(mpool));
    }
    mpool = NULL;
    curbuf = NULL;
    bufflag = 0;
    recvlen = 0;
    buf_pos = -1;
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
      char *tnbuf = NULL;
      while ((buf_pos = mpool->take(tnbuf)) < 0) {
      };
      CACHE_PREFETCH_L2(tnbuf, 1);
      if (unlikely(recvlen > 0)) {
        std::memcpy(tnbuf, curbuf + clen, recvlen);
      }
      curbuf = tnbuf;
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
   * @brief 提交缓存
   *
   * @warning 不检查cmt_len，用户应保证一次提交完整的缓存块。
   * @param[in] buf_addr 缓存地址
   * @param[in] cmt_len 提交长度
   */
  FORCE_INLINE void cmt_buf(int64 buf_addr, int32 cmt_len) {
    int32 tpos = (int32)(buf_addr & 0x7fffffff);
    mpool->release(tpos);
  }

  /**
   * @brief 读取缓存数据
   *
   * @param[out] o_pmsg 输出的消息指针
   * @param[in] buf_addr 缓存地址
   */
  FORCE_INLINE void read_buf(char *&o_pmsg, int64 buf_addr) const {
    int32 tpos = (int32)(buf_addr & 0x7fffffff);
    if (likely(tpos > 0 && tpos < mpool->size()))
      o_pmsg = mpool->pos2buf(tpos);
    else
      o_pmsg = NULL;
  }

  /**
   * @brief 构建缓存池
   *
   * @param[in,out] attr AIO属性配置
   * @return int32 0=成功，其他=错误码
   */
  int32 build_buf(aio_attr &attr) {
    assert(attr.maxmsglen > 0 && attr.buf_size > 0);
    assert(attr.maxmsglen <= N);

    if (attr.buf_size < 128)
      attr.buf_size = 128;
    if (attr.oncerecvlen <= 0)
      attr.oncerecvlen = AIO_RECV_ONCE_MIN_LEN;
    if (attr.oncerecvlen > AIO_RECV_ONCE_MAX_LEN)
      attr.oncerecvlen = AIO_RECV_ONCE_MAX_LEN;
    if (attr.oncerecvlen > attr.maxmsglen)
      attr.oncerecvlen = attr.maxmsglen;

    buflock.lock();
    if (NULL != mpool) {
      if (buf_pos < 0 || curbuf == NULL) {
        while ((buf_pos = mpool->take(curbuf)) < 0) {
        };
      }
      buflock.unlock();
      return 0;
    }

    maxmsglen = attr.maxmsglen;
    recvlen = 0;
    onerecvlen = attr.oncerecvlen;

    int64 ts_pool = sizeof(fix_pool<m_it>);
    ts_pool = ALIGN_UP(ts_pool, CACHE_ALIGN_SIZE);
    int64 ts = fix_pool<m_it>::need_buf_size(attr.buf_size);
    ts += ts_pool;
    ts = ALIGN_UP(ts, CACHE_ALIGN_SIZE);
    void *tpm = comm_utils::aligned_malloc(ts, CACHE_ALIGN_SIZE);
    if (NULL == tpm) {
      buflock.unlock();
      return LBERR_MEM_ALLOC_FAIL;
    }
    mpool = reinterpret_cast<fix_pool<m_it> *>(tpm);
    char *tpc = static_cast<char *>(tpm);
    tpc += ts_pool;
    mpool->init(static_cast<void *>(tpc), ts - ts_pool, 0);
    bufflag = 1;

    while ((buf_pos = mpool->take(curbuf)) < 0) {
    };

    buflock.unlock();
    return 0;
  }

  /**
   * @brief 使用共享的固定内存池初始化。
   *
   * @param[in,out] attr AIO属性配置
   * @param[in] share_pool 共享的固定内存池指针
   */
  void init_buf(aio_attr &attr, fix_pool<m_it> *share_pool) {
    assert(NULL != share_pool);
    assert(attr.maxmsglen > 0 && attr.buf_size > 0);
    assert(attr.maxmsglen <= N);

    if (attr.buf_size < 128)
      attr.buf_size = 128;
    if (attr.oncerecvlen <= 0)
      attr.oncerecvlen = AIO_RECV_ONCE_MIN_LEN;
    if (attr.oncerecvlen > AIO_RECV_ONCE_MAX_LEN)
      attr.oncerecvlen = AIO_RECV_ONCE_MAX_LEN;
    if (attr.oncerecvlen > attr.maxmsglen)
      attr.oncerecvlen = attr.maxmsglen;

    buflock.lock();
    if (NULL != mpool) {
      if (buf_pos < 0 || curbuf == NULL) {
        while ((buf_pos = mpool->take(curbuf)) < 0) {
        };
      }
      buflock.unlock();
      return;
    }

    maxmsglen = attr.maxmsglen;
    recvlen = 0;
    onerecvlen = attr.oncerecvlen;
    bufflag = 0;
    mpool = share_pool;

    while ((buf_pos = mpool->take(curbuf)) < 0) {
    };

    buflock.unlock();
  }

  /**
   * @brief 是否已经初始化
   */
  bool is_init() {
    bool ret = false;
    buflock.lock();
    if (NULL != mpool && buf_pos >= 0 && curbuf != NULL) {
      ret = true;
    }
    buflock.unlock();
    return ret;
  }

  /**
   * @brief 重置本通道的接收的未完成的消息
   * @note 在通道关闭时，需要调用此函数，否则会残留未处理的消息。
   */
  void reset_buf() { recvlen = 0; }

  /**
   * @brief 关闭缓存池
   */
  void close_buf() { destroy(); }

  /**
   * @brief 构造函数
   */
  aio_recv_pool() : maxmsglen(0), recvlen(0), onerecvlen(0), curbuf(NULL), buf_pos(-1), bufflag(0), mpool(NULL){};
  /**
   * @brief 析构函数
   */
  ~aio_recv_pool() { destroy(); }
};

/** @brief AIO MTU缓存池项目类型 */
typedef aio_pool_item<AIO_RECV_POOL_DEFAULT_LEN> aio_mtu_pool_it;

/** @brief AIO MTU固定内存池类型 */
typedef fix_pool<aio_mtu_pool_it> aio_mtu_pool;

/** @brief AIO MTU接收缓存池类型 */
typedef aio_recv_pool<AIO_RECV_POOL_DEFAULT_LEN> aio_recv_mtu_pool;

} // namespace lb_common

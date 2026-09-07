#pragma once

/**
 * @file que_mth_fixed.h
 * @brief 多读多写固定大小队列模块
 *
 * 提供一个高性能的单进程多读多写队列实现，专门用于存储POD类型数据。
 * 队列采用环形缓冲区设计，支持原子操作和CAS操作保证多线程安全。
 *
 * 主要特性：
 * - 多读多写模式，支持并发访问
 * - 固定大小数据项，支持POD类型
 * - 原子操作和CAS操作保证线程安全
 * - 高性能，低延迟
 * - 支持内存对齐优化
 * - 提供单线程和多线程两种操作接口
 *
 * 使用场景：
 * - 高并发生产者-消费者模式
 * - 多线程间数据传递
 * - 高性能数据缓冲
 * - 实时数据处理系统
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "matomic.h"
#include "mutils.h"
#include "que_comm.h"

namespace lb_common {

/**
 * @brief 多读多写固定大小队列类
 *
 * 这是一个高性能的单进程多读多写队列，专门用于存储POD类型数据。
 * 采用环形缓冲区设计，使用原子操作和CAS操作保证多线程安全。
 *
 * @tparam T 数据类型，必须是POD类型
 *
 * @note 队列大小必须是2的幂次方
 * @note 支持多线程并发读写
 * @note 提供单线程和多线程两种操作接口
 * @note 支持内存对齐以提高性能
 */
template <class T> class alignas(CACHE_ALIGN_SIZE) que_mth_fixed {
protected:
  /**
   * @brief 队列项类型定义
   */
  typedef que_fixed_it<T> mt_it;

  int64 wrcmt;                      ///< 写提交计数器（原子操作）
  int64 wrpos;                      ///< 写位置计数器（原子操作）
  int64 mask;                       ///< 掩码，用于快速取模运算
  mt_it *pbuf;                      ///< 缓冲区指针
  int64 create_mem;                 ///< 内存创建标志：0=共享内存，1=内部创建
  int64 mem_size;                   ///< 内存总大小
  int64 userpos[2];                 ///< 用户自定义位置数组
  int64 rdcmt;                      ///< 读提交计数器（原子操作）
  int64 rdpos;                      ///< 读位置计数器（原子操作）
  char userdata[QUE_USER_DATA_LEN]; ///< 用户数据缓冲区

  /**
   * @brief 初始化队列参数设置
   *
   * 设置队列的基本参数，包括读写位置、掩码等
   *
   * @param[in] tnum 队列总大小
   */
  void init_set(int64 tnum) {
    atomic_store64(&wrcmt, 1);
    atomic_store64(&wrpos, 1);
    pbuf = NULL;
    mask = tnum - 1;
    create_mem = 0;
    mem_size = tnum * sizeof(mt_it);
    userpos[0] = 1;
    userpos[1] = 1;
    atomic_store64(&rdcmt, 1);
    atomic_store64(&rdpos, 1);
    memset(userdata, 0, QUE_USER_DATA_LEN);
  }

public:
  /**
   * @brief 获取指定位置的数据指针
   *
   * 根据位置获取队列中对应数据的指针
   *
   * @param[in] pos 数据位置
   * @return T* 数据指针
   */
  FORCE_INLINE T *get_data(int64 pos) const { return &(pbuf[pos & mask].data); }

  /**
   * @brief 获取写入位置和数据指针（单线程模式）
   *
   * 保留一个空间以减少读判断，获取写入位置和对应的数据指针
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @return int64 >0表示获取的位置，0表示队列已满
   * @note 仅适用于单线程写入场景
   */
  FORCE_INLINE int64 write_get(T *&o_data) {
    int64 wr = wrcmt;
    if (unlikely(wr - atomic_load64(&rdcmt) > mask)) {
      return 0;
    }
    o_data = &(pbuf[wr & mask].data);
    return wr;
  }

  /**
   * @brief 提交写入操作（单线程模式）
   *
   * 更新写位置计数器，完成写入操作
   *
   * @param[in] get_pos 写入位置
   * @note 仅适用于单线程写入场景
   */
  FORCE_INLINE void write_cmt(int64 get_pos) { atomic_store64(&wrcmt, get_pos + 1); }

  /**
   * @brief 写入数据到队列（单线程模式）
   *
   * 直接将数据写入队列并更新写位置
   *
   * @param[in] userdata 要写入的用户数据
   * @return int64 >0表示写入的位置，0表示队列已满
   * @note 仅适用于单线程写入场景
   */
  FORCE_INLINE int64 write(const T &userdata) {
    int64 wr = wrcmt;
    if (unlikely(wr - atomic_load64(&rdcmt) > mask)) {
      return 0;
    }
    pbuf[wr & mask].data = userdata;
    atomic_store64(&wrcmt, wr + 1);
    return wr;
  }

  /**
   * @brief 获取写入位置和数据指针（多线程模式）
   *
   * 使用CAS操作原子地获取写入位置，支持多线程并发写入
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @return int64 >0表示获取的位置，0表示队列已满
   * @note 适用于多线程并发写入场景
   */
  inline int64 write_get_mth(T *&o_data) {
    int64 wr = atomic_load64(&wrpos);
    do {
      if (unlikely(wr - atomic_load64(&rdcmt) > mask)) {
        break;
      }
      if (likely(atomic_cas64_weak(&wrpos, &wr, wr + 1))) {
        o_data = &(pbuf[wr & mask].data);
        return wr;
      }
    } while (1);

    return 0;
  }

  /**
   * @brief 提交写入操作（多线程模式）
   *
   * 使用忙等待策略更新写提交计数器，支持多线程并发
   *
   * @param[in] get_pos 写入位置
   * @note 适用于多线程并发写入场景
   */
  inline void write_cmt_mth(int64 get_pos) {
    int32 i = QUE_LOOP_BUSY_COUNT;
    do {
      int64 wr = atomic_load64(&wrcmt);
      if (wr == get_pos) {
        atomic_store64(&wrcmt, get_pos + 1);
        break;
      } else if (wr > get_pos) {
        break;
      }

      i--;
      if (i == 0) {
        i = QUE_LOOP_BUSY_COUNT / 2;
        CPU_PAUSE();
      }
    } while (1);
  }

  /**
   * @brief 写入数据到队列（多线程模式）
   *
   * 使用CAS操作原子地写入数据，支持多线程并发写入
   *
   * @param[in] userdata 要写入的用户数据
   * @return int64 >0表示写入的位置，0表示队列已满
   * @note 适用于多线程并发写入场景
   */
  int64 write_mth(const T &userdata) {
    int64 wr = atomic_load64(&wrpos);
    do {
      if (unlikely(wr - atomic_load64(&rdcmt) > mask)) {
        return 0;
      }
      if (likely(atomic_cas64_weak(&wrpos, &wr, wr + 1))) {
        break;
      }
    } while (1);

    pbuf[wr & mask].data = userdata;

    write_cmt_mth(wr);
    return wr;
  }

  /**
   * @brief 获取读取位置和数据指针（单线程模式）
   *
   * 获取当前读取位置和对应的数据指针
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @return int64 >0表示读取位置，0表示队列为空
   * @note 仅适用于单线程读取场景
   */
  FORCE_INLINE int64 read_get(T *&o_data) const {
    int64 pos = rdcmt;
    if (unlikely(atomic_load64(&wrcmt) <= pos)) {
      return 0;
    }
    o_data = &(pbuf[pos & mask].data);
    return pos;
  }

  /**
   * @brief 提交读取操作（单个元素，单线程模式）
   *
   * 更新读位置计数器，增加1
   *
   * @note 仅适用于单线程读取场景
   */
  FORCE_INLINE void read_cmt() { atomic_fetch_add64(&rdcmt, 1); }

  /**
   * @brief 提交读取操作（多个元素，单线程模式）
   *
   * 更新读位置计数器，增加指定数量
   *
   * @param[in] num 要增加的元素数量
   * @note 仅适用于单线程读取场景
   */
  FORCE_INLINE void read_cmt(int32 num) { atomic_fetch_add64(&rdcmt, num); }

  /**
   * @brief 获取下一个位置（单个步长）
   *
   * 计算下一个位置（当前位置+1）
   *
   * @param[in] curpos 当前位置
   * @return int64 下一个位置
   */
  FORCE_INLINE int64 next_pos(int64 curpos) const { return curpos + 1; }

  /**
   * @brief 获取下一个位置（多个步长）
   *
   * 计算下一个位置（当前位置+指定步长）
   *
   * @param[in] curpos 当前位置
   * @param[in] read_num 步长
   * @return int64 下一个位置
   */
  FORCE_INLINE int64 next_pos(int64 curpos, int32 read_num) const { return curpos + read_num; }

  /**
   * @brief 获取指定读取位置的数据指针（单线程模式）
   *
   * 根据指定的读取位置获取数据指针，会自动调整位置避免越过读指针
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @param[in] read_pos 指定的读取位置
   * @return int64 >0表示实际读取位置，0表示队列为空
   * @note 仅适用于单线程读取场景
   */
  FORCE_INLINE int64 read_get(T *&o_data, int64 read_pos) const {
    if (unlikely(atomic_load64(&wrcmt) <= read_pos)) {
      return 0;
    }
    int64 rc = atomic_load64(&rdcmt);
    if (unlikely(rc > read_pos)) {
      read_pos = rc;
    }
    o_data = &(pbuf[read_pos & mask].data);
    return read_pos;
  }

  /**
   * @brief 提交读取操作（指定位置，单线程模式）
   *
   * 更新读位置到指定位置
   *
   * @param[in] next_pos 下一个位置
   * @note 仅适用于单线程读取场景
   */
  FORCE_INLINE void read_cmt(int64 next_pos) {
    if (likely(rdcmt < next_pos)) {
      atomic_store64(&rdcmt, next_pos);
    }
  }

  /**
   * @brief 提交读取操作（指定位置，多线程模式）
   *
   * 使用CAS操作原子地更新读位置，支持多线程并发读取
   *
   * @param[in] next_pos 下一个位置
   * @note 适用于多线程并发读取场景
   */
  FORCE_INLINE void read_cmt_mth(int64 next_pos) {
    int64 pos = atomic_load64(&rdcmt);
    do {
      if (unlikely(pos >= next_pos)) {
        break;
      }
      if (likely(atomic_cas64_weak(&rdcmt, &pos, next_pos))) {
        break;
      }
    } while (1);
  }

  /**
   * @brief 弹出数据并复制到输出缓冲区（单线程模式）
   *
   * 从队列头部弹出一个数据项并复制到指定的输出缓冲区
   *
   * @param[out] o_buf 输出缓冲区指针
   * @return int64 返回读取的位置，0表示队列为空
   * @note 仅适用于单线程读取场景
   */
  FORCE_INLINE int64 pop(T *o_buf) {
    int64 pos = rdcmt;
    if (unlikely(atomic_load64(&wrcmt) <= pos)) {
      return 0;
    }

    *o_buf = pbuf[pos & mask].data;
    atomic_store64(&rdcmt, pos + 1);
    return pos;
  }

  /**
   * @brief 弹出数据并复制到输出缓冲区（多线程模式）
   *
   * 使用CAS操作原子地弹出数据，支持多线程并发读取
   *
   * @param[out] o_buf 输出缓冲区指针
   * @return int64 返回读取的位置，0表示队列为空
   * @note 适用于多线程并发读取场景
   */
  int64 pop_mth(T *o_buf) {
    int64 rd = atomic_load64(&rdpos);
    do {
      if (unlikely(atomic_load64(&wrcmt) <= rd)) {
        return 0;
      }
      if (likely(atomic_cas64_weak(&rdpos, &rd, rd + 1))) {
        break;
      }
    } while (1);

    *o_buf = pbuf[rd & mask].data;

    int32 i = QUE_LOOP_BUSY_COUNT;
    do {
      int64 pos = atomic_load64(&rdcmt);
      if (pos == rd) {
        atomic_store64(&rdcmt, rd + 1);
        break;
      } else if (pos > rd) {
        break;
      }

      i--;
      if (i == 0) {
        i = QUE_LOOP_BUSY_COUNT / 2;
        CPU_PAUSE();
      }
    } while (1);

    return rd;
  }

  /**
   * @brief 获取队列空闲空间数量
   *
   * 计算当前可用的空闲槽位数量，考虑多线程写入的情况
   *
   * @return int64 可用的空闲槽位数量
   */
  FORCE_INLINE int64 get_free() const {
    int64 tcmt = atomic_load64(&wrcmt);
    int64 tpos = atomic_load64(&wrpos);
    return (mask + 1 - (tcmt > tpos ? tcmt : tpos) + atomic_load64(&rdcmt));
  }

  /**
   * @brief 获取队列已使用空间数量
   *
   * 计算当前已使用的槽位数量，考虑多线程写入的情况
   *
   * @return int64 已使用的槽位数量
   */
  FORCE_INLINE int64 get_used() const {
    int64 tcmt = atomic_load64(&wrcmt);
    int64 tpos = atomic_load64(&wrpos);
    return ((tcmt > tpos ? tcmt : tpos) - atomic_load64(&rdcmt));
  }

  /**
   * @brief 查找并复制指定位置的数据
   *
   * 从指定位置查找数据并复制到输出缓冲区
   *
   * @param[out] o_buf 输出缓冲区指针
   * @param[in] pos 要查找的位置
   * @return int32 1表示成功，0表示失败（位置无效或已过期）
   */
  int32 find_copy(T *o_buf, int64 pos) const {
    if (unlikely(pos >= atomic_load64(&wrcmt) || pos < atomic_load64(&rdcmt))) {
      return 0;
    }

    *o_buf = pbuf[pos & mask].data;

    if (likely(pos >= atomic_load64(&rdcmt))) {
      return 1;
    }
    return 0;
  }

  /**
   * @brief 计算队列所需的缓冲区大小
   *
   * 根据队列大小计算实际需要的缓冲区大小（对齐到2的幂次方）
   *
   * @param[in] que_size 期望的队列大小
   * @return int64 实际需要的缓冲区大小
   */
  static int64 need_buf_size(int64 que_size) {
    int32 tn = comm_utils::calc_power(que_size);
    int64 tnum = (1L << tn);
    return tnum * sizeof(mt_it);
  }

  /**
   * @brief 重置队列位置
   *
   * 重置读写位置，确保队列处于有效状态
   *
   * @param[in] is_continue 0表示完全重置，1表示继续模式
   * @note 读写都不使用时方可调用此函数
   */
  void reset_pos(int32 is_continue) {
    if (atomic_load64(&rdcmt) < 1)
      atomic_store64(&rdcmt, 1);
    if (atomic_load64(&wrcmt) < 1)
      atomic_store64(&wrcmt, 1);

    int64 tw = atomic_load64(&wrcmt);
    if (atomic_load64(&rdcmt) > tw) {
      atomic_store64(&rdcmt, tw);
    }

    if (is_continue == 0) {
      atomic_store64(&wrpos, tw);
      atomic_store64(&rdcmt, tw);
      atomic_store64(&rdpos, tw);
    } else {
      atomic_store64(&wrpos, tw);
      int64 tr = atomic_load64(&rdcmt);
      int64 trp = atomic_load64(&rdpos);
      if (trp < tr)
        atomic_store64(&rdpos, tr);
      else
        atomic_store64(&rdcmt, trp);
    }
  }

  /**
   * @brief 初始化队列（内部内存）
   *
   * 使用内部分配的内存初始化队列
   *
   * @param[in] que_size 队列大小
   * @return int32 0表示成功，负数表示错误码
   * @note 队列单线程创建调用，不可有其他线程使用
   */
  int32 init(int64 que_size) {
    if (que_size <= 0)
      return LBERR_ARGV_WRONG;

    int32 tn = comm_utils::calc_power(que_size);
    int64 tnum = (1L << tn);
    init_set(tnum);

    int64 tsize = tnum * sizeof(mt_it);
    void *tpbuf = comm_utils::aligned_malloc(tsize, CACHE_ALIGN_SIZE);
    if (NULL == tpbuf) {
      return LBERR_MEM_ALLOC_FAIL;
    }
    create_mem = 1;
    pbuf = new (tpbuf) mt_it[tnum];
    return 0;
  }
  /**
   * @brief 初始化队列（共享内存）
   *
   * 使用共享内存初始化队列
   *
   * @param[in] shm_addr 共享内存地址
   * @param[in] que_size 队列大小
   * @param[in] shm_isexist 共享内存是否已存在：0=新建，1=已存在
   * @param[in] is_continue 是否继续模式：0=重置，1=继续
   * @return int32 0表示成功，负数表示错误码
   */
  int32 init(char *shm_addr, int64 que_size, int32 shm_isexist, int32 is_continue) {
    if (que_size <= 0 || NULL == shm_addr)
      return LBERR_ARGV_WRONG;
    pbuf = reinterpret_cast<mt_it *>(shm_addr);

    int32 tn = comm_utils::calc_power(que_size);
    int64 tnum = (1L << tn);
    if (shm_isexist == 0 || (mask != 0 && mask + 1 != tnum)) {
      init_set(tnum);
      return 0;
    }

    reset_pos(is_continue);
    return 0;
  }

  /**
   * @brief 开始读取操作
   *
   * 设置读取起始位置
   *
   * @param[in] read_pos 输入输出参数，读取位置
   * @param[in] is_continue 0表示从写位置开始，1表示继续模式
   */
  void read_start(int64 *read_pos, int32 is_continue) const {
    if (is_continue == 0) {
      *read_pos = atomic_load64(&wrcmt);
    } else {
      int64 tc = atomic_load64(&rdcmt);
      if (*read_pos < tc)
        *read_pos = tc;
    }
  }

  /**
   * @brief 关闭队列并释放资源
   *
   * 释放队列占用的资源，重置队列状态
   */
  void close() {
    if (NULL != pbuf && create_mem == 1) {
      comm_utils::aligned_free(reinterpret_cast<void *>(pbuf));
      pbuf = NULL;
      create_mem = 0;
    }
    reset_pos(0);
  }

  FORCE_INLINE int64 get_write_pos() const { return atomic_load64(&wrcmt); }

  FORCE_INLINE int64 get_read_pos() const { return atomic_load64(&rdcmt); }

  FORCE_INLINE int64 get_size() const { return mask + 1; }

  FORCE_INLINE int64 *get_user_pos() { return userpos; }

  FORCE_INLINE char *get_user_data() { return userdata; }
};

} // namespace lb_common

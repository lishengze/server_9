#pragma once

/**
 * @file que_spw_fixed.h
 * @brief 单进程多写多进程读固定大小队列模块
 *
 * 提供一个支持单进程多线程写入、多进程读取的固定大小队列实现。
 * 队列采用环形缓冲区设计，使用共享内存实现跨进程通信，支持原子操作保证多线程安全。
 *
 * 主要特性：
 * - 单进程内多线程写入支持
 * - 多进程读取支持
 * - 固定大小数据项，支持POD类型
 * - 原子操作和CAS操作保证线程安全
 * - 共享内存实现跨进程通信
 * - 高性能，低延迟
 * - 支持内存对齐优化
 * - 提供单线程和多线程两种操作接口
 *
 * 使用场景：
 * - 生产者-消费者跨进程模式
 * - 多线程写入、多进程读取的数据传递
 * - 高性能跨进程数据缓冲
 * - 实时数据处理系统
 *
 * 注意事项：
 * - 单进程内可多线程写入
 * - 不支持多进程pop读
 * - 写入进程必须为创建者
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "matomic.h"
#include "mutils.h"
#include "que_comm.h"

namespace lb_common {

/**
 * @brief 单进程多写多进程读队列信息结构体
 *
 * 存储队列的控制信息和状态数据，用于跨进程共享
 */
struct alignas(CACHE_ALIGN_SIZE) que_spw_fixed_info {
  int64 wrcmt;                      ///< 写提交计数器（原子操作）
  int64 wrpos;                      ///< 写位置计数器（原子操作）
  int64 mask;                       ///< 掩码，用于快速取模运算
  int64 wrpid;                      ///< 写进程ID
  int64 create_mem;                 ///< 内存创建标志：0=外部内存，1=内部创建
  int64 mem_size;                   ///< 内存总大小
  int64 userpos[2];                 ///< 用户自定义位置数组
  int64 rdcmt;                      ///< 读提交计数器（原子操作）
  int64 rdpos;                      ///< 读位置计数器（原子操作）
  char userdata[QUE_USER_DATA_LEN]; ///< 用户数据缓冲区
};

/**
 * @brief 单进程多写多进程读固定大小队列类
 *
 * 这是一个支持单进程多线程写入、多进程读取的固定大小队列实现。
 * 采用环形缓冲区设计，使用共享内存实现跨进程通信，支持原子操作保证多线程安全。
 *
 * @tparam T 数据类型，必须是POD类型
 *
 * @note 队列大小必须是2的幂次方
 * @note 单进程内支持多线程并发写入
 * @note 支持多进程读取
 * @note 提供单线程和多线程两种操作接口
 * @note 支持内存对齐以提高性能
 * @note 写入进程必须为创建者
 */
template <class T> class que_spw_fixed {
protected:
  /**
   * @brief 队列项类型定义
   */
  typedef que_fixed_it<T> mt_it;

  que_spw_fixed_info *info; ///< 队列信息结构体指针
  int64 mask;               ///< 掩码，用于快速取模运算
  mt_it *pbuf;              ///< 缓冲区指针

  /**
   * @brief 释放缓冲区资源
   *
   * 释放共享内存映射的资源
   */
  void free_buf() {
    if (NULL == pbuf || NULL == info)
      return;
    if (info->create_mem == 1) {
      comm_utils::unmap_shm(reinterpret_cast<void *>(info), info->mem_size + sizeof(que_spw_fixed_info));
    }
    pbuf = NULL;
    info = NULL;
  }

  /**
   * @brief 初始化队列参数设置
   *
   * 设置队列的基本参数，包括读写位置、掩码等
   *
   * @param[in] tnum 队列总大小
   */
  void init_set(int64 tnum) {
    atomic_store64(&info->wrcmt, 1);
    atomic_store64(&info->wrpos, 1);
    info->mask = 0;
    info->wrpid = 0;
    info->create_mem = 0;
    info->mem_size = sizeof(mt_it) * tnum;
    info->userpos[0] = 1;
    info->userpos[1] = 1;
    atomic_store64(&info->rdcmt, 1);
    atomic_store64(&info->rdpos, 1);
    std::memset(info->userdata, 0, QUE_USER_DATA_LEN);
    info->mask = tnum - 1;
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
    int64 wr = info->wrcmt;
    if (unlikely(wr - atomic_load64(&info->rdcmt) > mask)) {
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
  FORCE_INLINE void write_cmt(int64 get_pos) { atomic_store64(&info->wrcmt, get_pos + 1); }

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
    int64 wr = info->wrcmt;
    if (unlikely(wr - atomic_load64(&info->rdcmt) > mask)) {
      return 0;
    }
    pbuf[wr & mask].data = userdata;
    atomic_store64(&info->wrcmt, wr + 1);
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
    int64 wr = atomic_load64(&info->wrpos);
    do {
      if (unlikely(wr - atomic_load64(&info->rdcmt) > mask)) {
        break;
      }
      if (likely(atomic_cas64_weak(&info->wrpos, &wr, wr + 1))) {
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
      int64 wr = atomic_load64(&info->wrcmt);
      if (wr == get_pos) {
        atomic_store64(&info->wrcmt, get_pos + 1);
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
    int64 wr = atomic_load64(&info->wrpos);
    do {
      if (unlikely(wr - atomic_load64(&info->rdcmt) > mask)) {
        return 0;
      }
      if (likely(atomic_cas64_weak(&info->wrpos, &wr, wr + 1))) {
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
    int64 pos = info->rdcmt;
    if (unlikely(atomic_load64(&info->wrcmt) <= pos)) {
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
  FORCE_INLINE void read_cmt() { atomic_fetch_add64(&info->rdcmt, 1); }

  /**
   * @brief 提交读取操作（多个元素，单线程模式）
   *
   * 更新读位置计数器，增加指定数量
   *
   * @param[in] num 要增加的元素数量
   * @note 仅适用于单线程读取场景
   */
  FORCE_INLINE void read_cmt(int32 num) { atomic_fetch_add64(&info->rdcmt, num); }

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
    if (unlikely(atomic_load64(&info->wrcmt) <= read_pos)) {
      return 0;
    }
    int64 rc = atomic_load64(&info->rdcmt);
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
    if (likely(info->rdcmt < next_pos)) {
      atomic_store64(&info->rdcmt, next_pos);
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
    int64 pos = atomic_load64(&info->rdcmt);
    do {
      if (unlikely(pos >= next_pos)) {
        break;
      }
      if (likely(atomic_cas64_weak(&info->rdcmt, &pos, next_pos))) {
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
    int64 pos = info->rdcmt;
    if (unlikely(atomic_load64(&info->wrcmt) <= pos)) {
      return 0;
    }

    *o_buf = pbuf[pos & mask].data;
    atomic_store64(&info->rdcmt, pos + 1);
    return pos;
  }

  /**
   * @brief 获取队列空闲空间数量
   *
   * 计算当前可用的空闲槽位数量，考虑多线程写入的情况
   *
   * @return int64 可用的空闲槽位数量
   */
  FORCE_INLINE int64 get_free() const {
    int64 tcmt = atomic_load64(&info->wrcmt);
    int64 tpos = atomic_load64(&info->wrpos);
    return (mask + 1 - (tcmt > tpos ? tcmt : tpos) + atomic_load64(&info->rdcmt));
  }

  /**
   * @brief 获取队列已使用空间数量
   *
   * 计算当前已使用的槽位数量，考虑多线程写入的情况
   *
   * @return int64 已使用的槽位数量
   */
  FORCE_INLINE int64 get_used() const {
    int64 tcmt = atomic_load64(&info->wrcmt);
    int64 tpos = atomic_load64(&info->wrpos);
    return ((tcmt > tpos ? tcmt : tpos) - atomic_load64(&info->rdcmt));
  }

  /**
   * @brief 获取队列信息结构体指针
   *
   * @return que_spw_fixed_info* 队列信息结构体指针
   */
  FORCE_INLINE que_spw_fixed_info *get_info() const { return info; }

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
    if (unlikely(pos >= atomic_load64(&info->wrcmt) || pos < atomic_load64(&info->rdcmt))) {
      return 0;
    }

    *o_buf = pbuf[pos & mask].data;

    if (likely(pos >= atomic_load64(&info->rdcmt))) {
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
   */
  void reset_pos(int32 is_continue) {
    if (atomic_load64(&info->rdcmt) < 1)
      atomic_store64(&info->rdcmt, 1);
    if (atomic_load64(&info->wrcmt) < 1)
      atomic_store64(&info->wrcmt, 1);

    int64 tw = atomic_load64(&info->wrcmt);
    if (atomic_load64(&info->rdcmt) > tw) {
      atomic_store64(&info->rdcmt, tw);
    }

    if (is_continue == 0) {
      atomic_store64(&info->wrpos, tw);
      atomic_store64(&info->rdcmt, tw);
      atomic_store64(&info->rdpos, tw);
    } else {
      atomic_store64(&info->wrpos, tw);
      int64 tr = atomic_load64(&info->rdcmt);
      int64 trp = atomic_load64(&info->rdpos);
      if (trp < tr)
        atomic_store64(&info->rdpos, tr);
      else
        atomic_store64(&info->rdcmt, trp);
    }
  }

  /**
   * @brief 初始化队列（共享内存模式）
   *
   * 使用共享内存初始化队列，支持跨进程访问
   *
   * @param[in] pname 共享内存名称
   * @param[in] que_size 队列大小
   * @param[in] is_continue 是否继续模式：0=重置，1=继续
   * @param[in] is_create 是否创建者：0=连接现有，1=创建新的
   * @return int32 0表示成功，负数表示错误码
   * @note 单独的写进程为创建者，其必须保证读者不工作，才可设置is_continue =0
   */
  int32 init(const char *pname, int64 que_size, int32 is_continue, int32 is_create) {
    if (que_size <= 0 || NULL == pname)
      return LBERR_ARGV_WRONG;
    int32 tn = comm_utils::calc_power(que_size);
    int64 tnum = (1L << tn);

    int64 ts = tnum * sizeof(mt_it);
    ts += sizeof(que_spw_fixed_info);
    char tname[64];
    comm_utils::str_copy_format(tname, pname, sizeof(tname));

    void *pshm = NULL;
    int32 shmisexist = 0;
    int32 ishuge = 0;
    if (ts >= HUGE_PAGE_SIZE) {
      ishuge = 1;
    }

    shmisexist = comm_utils::map_shm(pshm, tname, ts, ishuge, is_create);
    if (shmisexist < 0) {
      return shmisexist;
    }

    info = reinterpret_cast<que_spw_fixed_info *>(pshm);
    pbuf = reinterpret_cast<mt_it *>(((uint8 *)pshm) + sizeof(que_spw_fixed_info));

    if (is_create == 1) {
      if (shmisexist == 0 || is_continue == 0 || info->mask + 1 != tnum) {
        init_set(tnum);
      } else {
        reset_pos(is_continue);
      }
      info->wrpid = comm_utils::get_pid();
      info->create_mem = 1;
    } else {
      while (info->mask == 0) {
        comm_utils::sleep_us(300);
      }
      if (info->mask + 1 != tnum)
        return LBERR_OBJ_NUM_LIMIT;
    }
    mask = info->mask;
    return 0;
  }

  /**
   * @brief 初始化队列（外部内存模式）
   *
   * 使用外部提供的内存初始化队列
   *
   * @param[in] pinfo 队列信息结构体指针
   * @param[in] pmem 外部内存指针
   * @param[in] que_size 队列大小
   * @param[in] shm_isexist 共享内存是否已存在：0=新建，1=已存在
   * @param[in] is_continue 是否继续模式：0=重置，1=继续
   * @param[in] is_create 是否创建者：0=连接现有，1=创建新的
   * @return int32 0表示成功，负数表示错误码
   * @note 单独的写进程为创建者，其必须保证读者不工作，才可设置is_continue =0
   */
  int32 init(que_spw_fixed_info *pinfo, char *pmem, int64 que_size, int32 shm_isexist, int32 is_continue,
             int32 is_create) {
    if (que_size <= 0 || NULL == pinfo || NULL == pmem)
      return LBERR_ARGV_WRONG;

    info = pinfo;
    pbuf = reinterpret_cast<mt_it *>(pmem);

    int32 tn = comm_utils::calc_power(que_size);
    int64 tnum = (1L << tn);
    if (is_create == 1) {
      if (shm_isexist == 0 || is_continue == 0 || (info->mask + 1 != tnum)) {
        init_set(tnum);
      } else {
        reset_pos(is_continue);
      }
      info->wrpid = comm_utils::get_pid();
      info->create_mem = 0;
    } else {
      while (info->mask == 0) {
        comm_utils::sleep_us(300);
      }
      if (info->mask + 1 != tnum)
        return LBERR_OBJ_NUM_LIMIT;
    }
    mask = info->mask;
    return 0;
  }
  /**
   * @brief 开始读取操作
   *
   * 设置读取起始位置，确保读写位置的一致性
   *
   * @param[in] read_pos 输入输出参数，读取位置
   * @param[in] is_continue 0表示从写位置开始，1表示继续模式
   */
  void read_start(int64 *read_pos, int32 is_continue) const {
    int64 tw = atomic_load64(&info->wrcmt);
    int64 tr = atomic_load64(&info->rdcmt);
    if (tr > tw) {
      atomic_store64(&info->rdcmt, tw);
    }
    if (is_continue == 0) {
      *read_pos = tw;
    } else {
      if (*read_pos < tr)
        *read_pos = tr;
    }
  }

  /**
   * @brief 关闭队列并释放资源
   *
   * 释放队列占用的资源，重置队列状态
   *
   * @param[in] is_create 是否为创建者：1=释放资源，0=仅重置
   */
  void close(int32 is_create = 1) {
    if (is_create == 1) {
      free_buf();
      reset_pos(0);
    }
  }

  /**
   * @brief 获取当前写位置
   *
   * @return int64 当前写位置
   */
  FORCE_INLINE int64 get_write_pos() const { return atomic_load64(&info->wrcmt); }

  /**
   * @brief 获取当前读位置
   *
   * @return int64 当前读位置
   */
  FORCE_INLINE int64 get_read_pos() const { return atomic_load64(&info->rdcmt); }

  /**
   * @brief 获取队列总大小
   *
   * @return int64 队列总大小
   */
  FORCE_INLINE int64 get_size() const { return mask + 1; }

  /**
   * @brief 获取用户自定义位置数组
   *
   * @return int64* 用户位置数组指针
   */
  FORCE_INLINE int64 *get_user_pos() const { return info->userpos; }

  /**
   * @brief 获取用户数据缓冲区
   *
   * @return char* 用户数据缓冲区指针
   */
  FORCE_INLINE char *get_user_data() const { return info->userdata; }
};

} // namespace lb_common

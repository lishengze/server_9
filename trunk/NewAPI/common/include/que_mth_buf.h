#pragma once

/**
 * @file que_mth_buf.h
 * @brief 多读多写变长数据队列模块
 *
 * 提供一个高性能的单进程多读多写队列实现，专门用于存储POD类型数据。
 * 队列采用环形缓冲区设计，支持原子操作保证多线程安全，支持变长数据存储。
 *
 * 主要特性：
 * - 多读多写模式，支持并发访问
 * - 变长数据项支持
 * - 原子操作和CAS操作保证线程安全
 * - 高性能，低延迟
 * - 支持内存对齐优化
 * - 支持环形缓冲区边界处理
 * - 提供单线程和多线程两种操作接口
 *
 * 使用场景：
 * - 高并发生产者-消费者模式
 * - 多线程间数据传递
 * - 高性能数据缓冲
 * - 变长数据存储
 * - 实时数据处理系统
 *
 * 注意事项：
 * - 不存储数据长度，写入者保证每次写入的完整性
 * - 读者每次读一个完整的数据，提交与写入匹配的长度
 * - 由于读者未知长度，不支持多读的Pop读
 */

#include "comm_sys.h"
#include "matomic.h"
#include "que_comm.h"
#include <cstring>

namespace lb_common {

/**
 * @brief 多读多写变长数据队列类
 *
 * 这是一个高性能的单进程多读多写队列，专门用于存储POD类型数据。
 * 采用环形缓冲区设计，使用原子操作保证读写操作的线程安全，支持变长数据存储。
 *
 * @note 队列大小必须是2的幂次方
 * @note 支持多线程并发写入
 * @note 支持多线程并发读取
 * @note 支持内存对齐以提高性能
 * @note 支持变长数据存储
 * @note 不存储数据长度，写入者保证数据完整性
 * @note 读者需要知道数据长度并提交匹配的长度
 */
class alignas(CACHE_ALIGN_SIZE) que_mth_buf {
protected:
  int64 wrcmt;                      ///< 写提交计数器（原子操作）
  int64 roundend;                   ///< 环形缓冲区结束位置（原子操作）
  int64 wrpos;                      ///< 写位置计数器（原子操作）
  int64 mask;                       ///< 掩码，用于快速取模运算
  char *pbuf;                       ///< 缓冲区指针
  int32 align_n;                    ///< 对齐字节数
  int32 create_mem;                 ///< 内存创建标志：0=共享内存，1=内部创建
  int64 userpos[2];                 ///< 用户自定义位置数组
  int64 rdcmt;                      ///< 读提交计数器（原子操作）
  int64 filled;                     ///< 填充标志
  char userdata[QUE_USER_DATA_LEN]; ///< 用户数据缓冲区

  /**
   * @brief 初始化队列参数设置
   *
   * 设置队列的基本参数，包括读写位置、掩码等
   *
   * @param[in] tsize 队列总大小
   * @param[in] align_byte 对齐字节数
   */
  void init_set(int64 tsize, int32 align_byte);

public:
  /**
   * @brief 获取指定位置的数据指针
   *
   * 根据位置获取队列中对应数据的指针
   *
   * @param[in] pos 数据位置
   * @return char* 数据指针
   */
  FORCE_INLINE char *get_data(int64 pos) const {
    assert(pos > 0);
    return (pbuf + (pos & mask));
  }

  /**
   * @brief 获取写入位置和数据指针（单线程模式）
   *
   * 获取写入位置和对应的数据指针，支持变长数据
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @param[in] len 要写入的数据长度
   * @return int64 >0表示获取的位置，0表示队列已满
   * @note 仅适用于单线程写入场景
   */
  int64 write_get(char *&o_data, int32 len);

  /**
   * @brief 提交写入操作（单线程模式）
   *
   * 更新写位置计数器，完成写入操作
   *
   * @param[in] getpos 写入位置
   * @param[in] len 写入的数据长度
   * @note 仅适用于单线程写入场景
   */
  FORCE_INLINE void write_cmt(int64 getpos, int32 len) {
    assert(getpos > 0);
    if (unlikely((getpos & mask) == 0)) {
      int64 tw = atomic_load64(&wrcmt);
      atomic_store64(&roundend, tw);
    }
    atomic_store64(&wrcmt, getpos + ALIGN_UP(len, align_n));
  }

  /**
   * @brief 写入数据到队列（单线程模式）
   *
   * 直接将数据写入队列并更新写位置，支持变长数据
   *
   * @param[in] userdata 要写入的用户数据
   * @param[in] len 数据长度
   * @return int64 >0表示写入的位置，0表示队列已满
   * @note 仅适用于单线程写入场景
   */
  int64 write(const char *userdata, int32 len);

  /**
   * @brief 获取写入位置和数据指针（多线程模式）
   *
   * 使用CAS操作原子地获取写入位置，支持多线程并发写入
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @param[in] len 要写入的数据长度
   * @return int64 >0表示获取的位置，0表示队列已满
   * @note 适用于多线程并发写入场景
   */
  int64 write_get_mth(char *&o_data, int32 len);

  /**
   * @brief 提交写入操作（多线程模式）
   *
   * 使用忙等待策略更新写提交计数器，支持多线程并发
   *
   * @param[in] getpos 写入位置
   * @param[in] len 写入的数据长度
   * @note 适用于多线程并发写入场景
   */
  void write_cmt_mth(int64 getpos, int32 len);

  /**
   * @brief 写入数据到队列（多线程模式）
   *
   * 使用CAS操作原子地写入数据，支持多线程并发写入
   *
   * @param[in] userdata 要写入的用户数据
   * @param[in] len 数据长度
   * @return int64 >0表示写入的位置，0表示队列已满
   * @note 适用于多线程并发写入场景
   */
  int64 write_mth(const char *userdata, int32 len);

  /**
   * @brief 获取读取位置和数据指针（从上次读提交位置读）
   *
   * 获取当前读取位置和对应的数据指针
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @return int64 >0表示读取位置，0表示队列为空
   */
  int64 read_get(char *&o_data);

  /**
   * @brief 提交读取操作（指定长度）
   *
   * 更新读位置计数器，增加指定长度
   *
   * @param[in] len 要增加的长度
   */
  FORCE_INLINE void read_cmt(int32 len) { atomic_fetch_add64(&rdcmt, ALIGN_UP(len, align_n)); }

  /**
   * @brief 获取下一个位置
   *
   * 计算下一个位置（当前位置+对齐后的长度）
   *
   * @param[in] curpos 当前位置
   * @param[in] read_len 读取长度
   * @return int64 下一个位置
   */
  FORCE_INLINE int64 next_pos(int64 curpos, int32 read_len) const { return curpos + ALIGN_UP(read_len, align_n); }

  /**
   * @brief 获取指定读取位置的数据指针
   *
   * 根据指定的读取位置获取数据指针
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @param[in] read_pos 指定的读取位置
   * @return int64 >0表示实际读取位置，0表示队列为空
   */
  int64 read_get(char *&o_data, int64 read_pos);

  /**
   * @brief 提交读取操作（指定位置）
   *
   * 更新读位置到指定位置
   *
   * @param[in] next_pos 下一个位置
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
   * @brief 获取队列空闲空间数量
   *
   * 计算当前可用的空闲字节数量，考虑多线程写入的情况
   *
   * @return int64 可用的空闲字节数量
   */
  FORCE_INLINE int64 get_free() const {
    int64 tcmt = atomic_load64(&wrcmt);
    int64 tpos = atomic_load64(&wrpos);
    tcmt = tcmt > tpos ? tcmt : tpos;
    tpos = atomic_load64(&rdcmt);
    return (mask + 1 - (tcmt - tpos));
  }

  /**
   * @brief 获取队列已使用空间数量
   *
   * 计算当前已使用的字节数量，考虑多线程写入的情况
   *
   * @return int64 已使用的字节数量
   */
  FORCE_INLINE int64 get_used() const {
    int64 tcmt = atomic_load64(&wrcmt);
    int64 tpos = atomic_load64(&wrpos);
    return ((tcmt > tpos ? tcmt : tpos) - atomic_load64(&rdcmt));
  }

  /**
   * @brief 计算队列所需的缓冲区大小
   *
   * 根据队列大小计算实际需要的缓冲区大小（对齐到2的幂次方）
   *
   * @param[in] que_size 期望的队列大小
   * @return int64 实际需要的缓冲区大小
   */
  static int64 need_buf_size(int64 que_size);

  /**
   * @brief 初始化队列（内部内存）
   *
   * 使用内部分配的内存初始化队列
   *
   * @param[in] que_size 队列大小
   * @param[in] align_byte 对齐字节数，默认为8
   * @return int32 0表示成功，负数表示错误码
   */
  int32 init(int64 que_size, int32 align_byte = 8);

  /**
   * @brief 初始化队列（共享内存）
   *
   * 使用共享内存初始化队列
   *
   * @param[in] shm_addr 共享内存地址
   * @param[in] que_size 队列大小
   * @param[in] shm_isexist 共享内存是否已存在：0=新建，1=已存在
   * @param[in] is_continue 是否继续模式：0=重置，1=继续
   * @param[in] align_byte 对齐字节数，默认为8
   * @return int32 0表示成功，负数表示错误码
   */
  int32 init(char *shm_addr, int64 que_size, int32 shm_isexist, int32 is_continue, int32 align_byte = 8);

  /**
   * @brief 开始读取操作
   *
   * 设置读取起始位置
   *
   * @param[in] read_pos 输入输出参数，读取位置
   * @param[in] is_continue 0表示从写位置开始，1表示继续模式
   */
  void read_start(int64 *read_pos, int32 is_continue) {
    if (is_continue == 0) {
      *read_pos = atomic_load64(&wrcmt);
    } else {
      int64 tc = atomic_load64(&rdcmt);
      if (*read_pos < tc)
        *read_pos = tc;
    }
  }

  /**
   * @brief 重置队列位置
   *
   * 重置读写位置，确保队列处于有效状态
   *
   * @param[in] is_continue 0表示完全重置，1表示继续模式
   * @note 读写都不使用方可调用此函数
   */
  void reset_pos(int32 is_continue);

  /**
   * @brief 关闭队列并释放资源
   *
   * 释放队列占用的资源，重置队列状态
   */
  void close();

  FORCE_INLINE int64 get_write_pos() const { return atomic_load64(&wrcmt); }

  FORCE_INLINE int64 get_read_pos() const { return atomic_load64(&rdcmt); }

  FORCE_INLINE int64 get_size() const { return mask + 1; }

  FORCE_INLINE int64 *get_user_pos() { return &(userpos[0]); }

  FORCE_INLINE char *get_user_data() { return &(userdata[0]); }
};

} // namespace lb_common

#pragma once

/**
 * @file que_swr_buf.h
 * @brief 单写单读变长数据队列模块
 *
 * 提供一个高性能的单进程单写单读队列实现，专门用于存储POD类型数据。
 * 队列采用环形缓冲区设计，支持原子操作保证线程安全，支持变长数据存储。
 *
 * 主要特性：
 * - 单写单读模式，无锁设计
 * - 变长数据项支持
 * - 原子操作保证线程安全
 * - 高性能，低延迟
 * - 支持内存对齐优化
 * - 支持环形缓冲区边界处理
 *
 * 使用场景：
 * - 生产者-消费者模式
 * - 线程间数据传递
 * - 高性能数据缓冲
 * - 变长数据存储
 * - 实时数据处理
 */

#include "comm_sys.h"
#include "matomic.h"

// #include <cstring>

namespace lb_common {

/**
 * @brief 单写单读变长数据队列类
 *
 * 这是一个高性能的单进程单写单读队列，专门用于存储POD类型数据。
 * 采用环形缓冲区设计，使用原子操作保证读写操作的线程安全，支持变长数据存储。
 *
 * @note 队列大小必须是2的幂次方
 * @note 写操作只能由单一线程执行
 * @note 读操作只能由单一线程执行
 * @note 支持内存对齐以提高性能
 * @note 支持变长数据存储
 */
class alignas(CACHE_ALIGN_SIZE) que_swr_buf {
protected:
  int64 wrcmt;      ///< 写位置计数器（原子操作）
  int64 roundend;   ///< 环形缓冲区结束位置（原子操作）
  int64 mask;       ///< 掩码，用于快速取模运算
  char *pbuf;       ///< 缓冲区指针
  int64 rdcmt;      ///< 读位置计数器（原子操作）
  int32 align_n;    ///< 对齐字节数
  int32 create_mem; ///< 内存创建标志：0=共享内存，1=内部创建
  int64 userpos[2]; ///< 用户自定义位置数组

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
    assert(pos >= 0);
    return (pbuf + (pos & mask));
  }

  /**
   * @brief 获取写入位置和数据指针
   *
   * 获取写入位置和对应的数据指针，支持变长数据
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @param[in] len 要写入的数据长度
   * @return int64 >0表示获取的位置，0表示队列已满
   */
  int64 write_get(char *&o_data, int32 len);

  /**
   * @brief 提交写入操作
   *
   * 更新写位置计数器，完成写入操作
   *
   * @param[in] getpos 写入位置
   * @param[in] len 写入的数据长度
   */
  FORCE_INLINE void write_cmt(int64 getpos, int32 len) {
    if (unlikely((getpos & mask) == 0)) {
      int64 tw = atomic_load64(&wrcmt);
      atomic_store64(&roundend, tw);
    }
    atomic_store64(&wrcmt, getpos + ALIGN_UP(len, align_n));
  }

  /**
   * @brief 写入数据到队列
   *
   * 直接将数据写入队列并更新写位置，支持变长数据
   *
   * @param[in] userdata 要写入的用户数据
   * @param[in] len 数据长度
   * @return int64 >0表示写入的位置，0表示队列已满
   */
  int64 write(const char *userdata, int32 len);

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
   * @brief 获取队列已使用空间数量
   *
   * @return int64 已使用的字节数量
   */
  FORCE_INLINE int64 get_used() const { return (atomic_load64(&wrcmt) - atomic_load64(&rdcmt)); }

  /**
   * @brief 获取队列空闲空间数量
   *
   * @return int64 可用的空闲字节数量
   */
  FORCE_INLINE int64 get_free() const { return (mask + 1 - atomic_load64(&wrcmt) + atomic_load64(&rdcmt)); }

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
   * @note 读写都不使用时可调用此函数
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

  FORCE_INLINE int64 *get_user_pos() { return userpos; }
};

} // namespace lb_common

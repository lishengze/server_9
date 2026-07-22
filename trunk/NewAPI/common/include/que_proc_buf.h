#pragma once

/**
 * @file que_proc_buf.h
 * @brief 多进程多读多写变长数据队列模块
 *
 * 提供一个高性能的多进程多读多写队列实现，专门用于存储POD类型数据。
 * 队列采用环形缓冲区设计，支持原子操作保证多线程和多进程安全，支持变长数据存储。
 *
 * 主要特性：
 * - 多进程多读多写模式，支持并发访问
 * - 变长数据项支持，包含数据头部信息
 * - 原子操作和CAS操作保证线程安全
 * - 进程崩溃检测和恢复机制
 * - 高性能，低延迟
 * - 支持内存对齐优化
 * - 支持环形缓冲区边界处理
 * - 提供单线程和多线程两种操作接口
 *
 * 使用场景：
 * - 高并发跨进程生产者-消费者模式
 * - 多进程间数据传递
 * - 高性能跨进程数据缓冲
 * - 变长数据存储
 * - 实时数据处理系统
 * - 进程崩溃恢复场景
 *
 * 注意事项：
 * - 不存储数据长度，写入者保证每次写入的完整性
 * - 读者每次读一个完整的数据，提交与写入匹配的长度
 * - 由于读者未知长度，不支持多读的Pop读
 * - 支持进程崩溃检测和恢复
 * - 写进程为创建者，必须保证其他进程不工作才可重置
 */

#include "comm_sys.h"
#include "matomic.h"
#include "que_comm.h"
#include <cstring>

namespace lb_common {

/**
 * @brief 多进程多读多写队列信息结构体
 *
 * 存储队列的控制信息和状态数据，用于跨进程共享
 */
struct alignas(CACHE_ALIGN_SIZE) que_proc_buf_info {
  int64 wrcmt;                      ///< 写提交计数器（原子操作）
  int64 roundend;                   ///< 环形缓冲区结束位置（原子操作）
  int64 wrpos;                      ///< 写位置计数器（原子操作）
  int64 mask;                       ///< 掩码，用于快速取模运算
  int32 align_n;                    ///< 对齐字节数
  int32 create_mem;                 ///< 内存创建标志：0=外部内存，1=内部创建
  int64 discard_pos;                ///< 丢弃位置计数器（原子操作）
  int64 userpos[2];                 ///< 用户自定义位置数组
  int64 rdcmt;                      ///< 读提交计数器（原子操作）
  int64 filled;                     ///< 填充标志
  char userdata[QUE_USER_DATA_LEN]; ///< 用户数据缓冲区
};

/**
 * @brief 多进程多读多写变长数据队列类
 *
 * 这是一个高性能的多进程多读多写队列，专门用于存储POD类型数据。
 * 采用环形缓冲区设计，使用原子操作保证读写操作的线程安全，支持变长数据存储。
 * 支持进程崩溃检测和恢复机制。
 *
 * @note 队列大小必须是2的幂次方
 * @note 支持多进程并发写入
 * @note 支持多进程并发读取
 * @note 支持内存对齐以提高性能
 * @note 支持变长数据存储
 * @note 不存储数据长度，写入者保证数据完整性
 * @note 读者需要知道数据长度并提交匹配的长度
 * @note 支持进程崩溃检测和恢复
 * @note 写进程为创建者，必须保证其他进程不工作才可重置
 */

class que_proc_buf {
protected:
#define PROC_BUF_PID_INVALID    -11
#define PROC_BUF_PID_INIT       0
#define PROC_BUF_YIELD_NOTEXIST 10
#define PROC_BUF_YIELD_EXIST    15

  /**
   * @brief 进程缓冲区头部结构体
   *
   * 存储每个数据块的元信息，包括用户长度、内存长度和操作进程ID
   */
  struct proc_buf_head {
    int32 user_len; ///< 用户数据长度
    int32 mem_len;  ///< 内存对齐后的总长度
    int64 op_pid;   ///< 操作进程ID
  };

  que_proc_buf_info *que; ///< 队列信息结构体指针
  int32 align_n;          ///< 对齐字节数
  int64 mask;             ///< 掩码，用于快速取模运算
  char *pbuf;             ///< 缓冲区指针
  int64 op_pid;           ///< 当前操作进程ID

  /**
   * @brief 释放缓冲区资源
   *
   * 释放共享内存映射的资源
   */
  void free_buf();

  /**
   * @brief 检查提交写入状态
   *
   * 检查写进程是否core还是提交慢，用于进程崩溃检测
   *
   * @param[in] yield_cnt 让出计数
   * @param[in] last_dis 上次丢弃位置
   * @param[in] src_pos 源位置
   * @param[in] last_cmt 上次提交位置
   * @return int32 1表示成功，0表示失败
   */
  int32 check_cmt_write(int32 yield_cnt, int64 last_dis, int64 src_pos, int64 last_cmt);

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
   * 根据位置获取队列中对应数据的指针，跳过头部信息
   *
   * @param[in] pos 数据位置
   * @return char* 数据指针
   */
  FORCE_INLINE char *get_data(int64 pos) const {
    assert(pos >= 0);
    return (pbuf + (pos & (mask)) + sizeof(proc_buf_head));
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
    proc_buf_head *thead = (proc_buf_head *)(pbuf + (getpos & (mask)));
    thead->user_len = len;
    thead->mem_len = ALIGN_UP(len + sizeof(proc_buf_head), align_n);
    thead->op_pid = PROC_BUF_PID_INIT;

    if (unlikely((getpos & mask) == 0)) {
      int64 tw = atomic_load64(&que->wrcmt);
      atomic_store64(&que->roundend, tw);
    }
    atomic_store64(&que->wrcmt, getpos + thead->mem_len);
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
   * @note 仅用于单个写入进程的多线程写入
   */
  int64 write_get_mth(char *&o_data, int32 len);

  /**
   * @brief 提交写入操作（多线程模式）
   *
   * 使用忙等待策略更新写提交计数器，支持多线程并发
   *
   * @param[in] getpos 写入位置
   * @param[in] len 写入的数据长度
   * @note 仅用于单个写入进程的多线程写入
   */
  void write_cmt_mth(int64 getpos, int32 len);

  /**
   * @brief 写入数据到队列（多进程模式）
   *
   * 支持多进程写入，包含进程崩溃检测和恢复机制
   *
   * @param[in] userdata 要写入的用户数据
   * @param[in] len 数据长度
   * @return int64 >0表示写入的位置，0表示队列已满
   * @note 可以用于多进程写入
   */
  int64 write_mp(const char *userdata, int32 len);

  /**
   * @brief 获取读取位置和数据指针（从上次读提交位置读）
   *
   * 获取当前读取位置和对应的数据指针，跳过无效数据
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @return int64 >0表示读取位置，0表示队列为空
   */
  int64 read_get(char *&o_data);

  /**
   * @brief 提交读取操作（根据头部信息）
   *
   * 根据数据头部中的内存长度更新读位置计数器
   */
  FORCE_INLINE void read_cmt() {
    proc_buf_head *thead = (proc_buf_head *)(pbuf + ((que->rdcmt) & mask));
    atomic_fetch_add64(&que->rdcmt, thead->mem_len);
  }

  /**
   * @brief 提交读取操作（指定长度）
   *
   * 更新读位置计数器，增加指定长度（包含头部）
   *
   * @param[in] read_len 要增加的用户数据长度
   */
  FORCE_INLINE void read_cmt(int32 read_len) {
    atomic_fetch_add64(&que->rdcmt, ALIGN_UP(read_len + sizeof(proc_buf_head), align_n));
  }

  /**
   * @brief 获取下一个位置（根据头部信息）
   *
   * 根据当前位置数据头部中的内存长度计算下一个位置
   *
   * @param[in] curpos 当前位置
   * @return int64 下一个位置
   */
  FORCE_INLINE int64 next_pos(int64 curpos) const {
    proc_buf_head *thead = (proc_buf_head *)(pbuf + (curpos & mask));
    return curpos + thead->mem_len;
  }

  /**
   * @brief 获取下一个位置（指定长度）
   *
   * 根据指定长度计算下一个位置（包含头部）
   *
   * @param[in] curpos 当前位置
   * @param[in] read_len 读取长度
   * @return int64 下一个位置
   */
  FORCE_INLINE int64 next_pos(int64 curpos, int32 read_len) const {
    return curpos + ALIGN_UP(read_len + sizeof(proc_buf_head), align_n);
  }

  /**
   * @brief 获取指定读取位置的数据指针
   *
   * 根据指定的读取位置获取数据指针，跳过无效数据
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
  FORCE_INLINE void read_cmt_pos(int64 next_pos) {
    if (likely(que->rdcmt < next_pos)) {
      atomic_store64(&que->rdcmt, next_pos);
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
    int64 pos = atomic_load64(&que->rdcmt);
    do {
      if (unlikely(pos >= next_pos)) {
        break;
      }
      if (likely(atomic_cas64_weak(&que->rdcmt, &pos, next_pos))) {
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
    int64 tcmt = atomic_load64(&que->wrcmt);
    int64 tpos = atomic_load64(&que->wrpos);
    return (mask + 1 - (tcmt > tpos ? tcmt : tpos) + atomic_load64(&que->rdcmt));
  }

  /**
   * @brief 获取队列已使用空间数量
   *
   * 计算当前已使用的字节数量，考虑多线程写入的情况
   *
   * @return int64 已使用的字节数量
   */
  FORCE_INLINE int64 get_used() const {
    int64 tcmt = atomic_load64(&que->wrcmt);
    int64 tpos = atomic_load64(&que->wrpos);
    return ((tcmt > tpos ? tcmt : tpos) - atomic_load64(&que->rdcmt));
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
   * @brief 初始化队列（共享内存模式）
   *
   * 使用共享内存初始化队列，支持跨进程访问
   *
   * @param[in] pname 共享内存名称
   * @param[in] que_size 队列大小
   * @param[in] is_continue 0表示重置，1表示继续模式
   * @param[in] is_create 0表示连接，1表示创建
   * @param[in] align_byte 对齐字节数，默认为8
   * @return int32 0表示成功，负数表示错误码
   * @note 写进程为创建者，必须保证其他写和读进程不工作，才可设置is_continue=0
   */
  int32 init(const char *pname, int64 que_size, int32 is_continue, int32 is_create, int32 align_byte = 8);

  /**
   * @brief 初始化队列（外部内存模式）
   *
   * 使用外部提供的内存初始化队列
   *
   * @param[in] pinfo 队列信息结构体指针
   * @param[in] shm_addr 共享内存地址
   * @param[in] que_size 队列大小
   * @param[in] shm_isexist 共享内存是否已存在：0=新建，1=已存在
   * @param[in] is_continue 0表示重置，1表示继续模式
   * @param[in] is_create 0表示连接，1表示创建
   * @param[in] align_byte 对齐字节数，默认为8
   * @return int32 0表示成功，负数表示错误码
   */
  int32 init(que_proc_buf_info *pinfo, char *shm_addr, int64 que_size, int32 shm_isexist, int32 is_continue,
             int32 is_create, int32 align_byte = 8);

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
      *read_pos = atomic_load64(&que->wrcmt);
    } else {
      int64 tc = atomic_load64(&que->rdcmt);
      if (*read_pos < tc)
        *read_pos = tc;
    }
  }

  /**
   * @brief 关闭队列并释放资源
   *
   * 释放队列占用的资源，重置队列状态
   *
   * @param[in] is_create 1表示创建者，0表示连接者，默认为1
   */
  void close(int32 is_create = 1) {
    if (is_create == 1) {
      free_buf();
    }
  }

  FORCE_INLINE int64 get_write_pos() const { return atomic_load64(&que->wrcmt); }

  FORCE_INLINE int64 get_read_pos() const { return atomic_load64(&que->rdcmt); }

  FORCE_INLINE int64 get_size() const { return (mask) + 1; }

  FORCE_INLINE int64 get_discard_pos() const { return atomic_load64(&que->discard_pos); }

  FORCE_INLINE int64 *get_user_pos() const { return que->userpos; }

  FORCE_INLINE char *get_user_data() const { return que->userdata; }
};

} // namespace lb_common

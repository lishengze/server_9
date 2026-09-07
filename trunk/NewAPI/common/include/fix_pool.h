#pragma once

/**
 * @file fix_pool.h
 * @brief 固定数量内存池模块
 *
 * 这是一个固定数量不可动态扩展的对象内存池。
 *
 * 限制：
 *    1. 对象尽量为POD类型，不应含虚接口
 *    2. 一般用于全局性缓存池，而非临时
 *    3. 初始化尽量单线程
 *
 * 特征：
 *    1. 使用原子自旋锁支持多线程申请释放，take/release
 *    2. 同时提供单线程接口，get/back
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "mlock.h"

#include <new>

namespace lb_common {

/**
 * @brief 固定内存池迭代器结构体
 *
 * 用于固定内存池中的节点管理，包含数据存储和链表管理信息。
 * 每个节点包含一个数据对象和用于空闲链表管理的指针。
 *
 * @tparam MT 数据类型
 */
template <typename MT> struct fix_pool_it {
  /** @brief 空闲链表下一节点索引，-1表示末尾，-2表示已分配不在空闲表 */
  int32 next_free;
  /** @brief 在池中的位置索引，用于快速定位 */
  int32 m_pos;
  /** @brief 实际数据存储区域 */
  MT data;
};

/**
 * @brief 固定数量内存池类
 *
 * 固定大小不可动态扩展的对象内存池。
 * 使用空闲链表管理未使用对象，支持单线程和多线程操作。
 * 支持外部内存初始化，适用于共享内存场景。
 *
 * 限制：
 * - 对象尽量为POD类型，不应含虚接口
 * - 一般用于全局性缓存池，而非临时
 * - 初始化尽量单线程
 *
 * 特征：
 * - 使用原子自旋锁支持多线程申请释放
 * - 同时提供单线程接口
 *
 * @tparam T 对象类型
 */
template <typename T> class fix_pool {
private:
  /** @brief 迭代器类型别名 */
  using m_it = fix_pool_it<T>;

  /** @brief 原子自旋锁，用于多线程操作保护 */
  atomic_lock mlock;
  /** @brief 空闲链表头节点索引 */
  int32 free_first;
  /** @brief 指向内存池起始地址的指针 */
  m_it *pmem;
  /** @brief 当前空闲节点数量 */
  int32 n_free;
  /** @brief 内存池总容量 */
  int32 n_all;
  /** @brief 标记内存是否由池内部申请，1表示内部申请 */
  int32 new_mem;
  /** @brief 初始化状态标志，1表示已初始化 */
  int32 is_init;

  /**
   * @brief 根据数据指针获取对应的迭代器指针
   *
   * 通过数据指针反向计算出包含该数据的迭代器结构体地址。
   * 使用指针偏移计算，避免使用offsetof以支持编译期优化。
   *
   * @param[in] pd 指向数据的指针
   * @return 指向包含该数据的迭代器指针
   */
  FORCE_INLINE fix_pool_it<T> *data2it(T *pd) const {
    constexpr fix_pool_it<T> *null_ptr = NULL;
    const void *pdata = reinterpret_cast<const void *>(&(null_ptr->data));
    size_t toff = reinterpret_cast<size_t>(pdata);
    // 使用offsetof计算data的偏移量（编译期计算）
    // constexpr size_t toff = std::offsetof(fix_pool_it<T>,data);
    char *data_addr = reinterpret_cast<char *>(pd);
    return reinterpret_cast<fix_pool_it<T> *>(data_addr - toff);
  }

  /**
   * @brief 重置内存池状态
   *
   * 将内存池恢复到未初始化状态，清空所有状态变量。
   * 通常用于析构或重新初始化前的清理工作。
   */
  void reset() {
    is_init = 0;
    free_first = -1;
    pmem = NULL;
    n_free = 0;
    new_mem = 0;
  }
  /**
   * @brief 初始化内存池内存结构
   *
   * 设置空闲链表结构，将所有节点链接成空闲链表。
   * 初始化各个状态变量，使内存池进入可用状态。
   *
   * @param[in] pm 指向内存块的指针
   * @param[in] tnum 内存块中可容纳的对象数量
   * @param[in] is_new 标记内存是否为新申请的，1表示新申请
   */
  void init_mem(m_it *pm, int32 tnum, int32 is_new) {
    pm[tnum - 1].next_free = -1;
    for (int32 i = tnum - 2; i >= 0; i--) {
      m_it &tp = pm[i];
      tp.next_free = i + 1;
      tp.m_pos = i;
    }

    free_first = 0;
    pmem = pm;
    n_free = tnum;
    n_all = tnum;
    new_mem = is_new;
    is_init = 1;
  }

public:
  /**
   * @brief 根据位置索引获取数据指针
   *
   * 通过内存池中的位置索引直接获取对应对象的数据指针。
   *
   * @param[in] pos 位置索引，取值范围 [0, n_all)
   * @return T* 数据指针
   */
  FORCE_INLINE T *pos2buf(int32 pos) {
    assert(pos >= 0 && pos < n_all);
    return &(pmem[pos].data);
  }

  /**
   * @brief 单线程获取一个对象
   *
   * 从空闲链表中获取一个可用对象，返回对象指针和在池中的位置。
   * 此函数不使用锁，仅适用于单线程环境。
   *
   * @param[out] o_buf 输出参数，返回获取到的对象指针
   * @return 成功返回对象在池中的位置索引（>=0），失败返回LBERR_OBJ_IS_EMPTY
   */
  FORCE_INLINE int32 get(T *&o_buf) {
    int32 tr = free_first;
    if (likely(tr >= 0)) {
      m_it *tp = pmem + tr;
      free_first = tp->next_free;
      tp->next_free = -2;
      n_free--;
      o_buf = &(tp->data);
      return tr;
    }
    return LBERR_OBJ_IS_EMPTY;
  }
  /**
   * @brief 单线程释放对象（通过位置）
   *
   * 将指定位置的对象释放回空闲链表。
   * 此函数不使用锁，仅适用于单线程环境。
   *
   * @param[in] get_pos 要释放的对象在池中的位置索引
   */
  FORCE_INLINE void back(int32 get_pos) {
    if (unlikely(get_pos < 0 || get_pos >= n_all)) {
      return;
    }
    m_it *tp = pmem + get_pos;
    tp->next_free = free_first;
    free_first = get_pos;
    n_free++;
  }
  /**
   * @brief 单线程释放对象（通过指针）
   *
   * 将指定指针的对象释放回空闲链表。
   * 此函数不使用锁，仅适用于单线程环境。
   * 内部通过data2it函数计算出对象位置。
   *
   * @param[in] get_buf 要释放的对象指针
   */
  FORCE_INLINE void back(T *get_buf) {
    m_it *pi = data2it(get_buf);
    pi->next_free = free_first;
    free_first = pi->m_pos;
    n_free++;
  }

  /**
   * @brief 多线程获取一个对象
   *
   * 从空闲链表中获取一个可用对象，返回对象指针和在池中的位置。
   * 此函数使用原子自旋锁保护，适用于多线程环境。
   *
   * @param[out] o_buf 输出参数，返回获取到的对象指针
   * @return 成功返回对象在池中的位置索引（>=0），失败返回LBERR_OBJ_IS_EMPTY
   */
  FORCE_INLINE int32 take(T *&o_buf) {
    mlock.lock();
    int32 tr = free_first;
    if (likely(tr >= 0)) {
      m_it *tp = pmem + tr;
      free_first = tp->next_free;
      tp->next_free = -2;
      n_free--;
      o_buf = &(tp->data);
      mlock.unlock();
      return tr;
    }
    mlock.unlock();
    return LBERR_OBJ_IS_EMPTY;
  }
  /**
   * @brief 多线程释放对象（通过位置）
   *
   * 将指定位置的对象释放回空闲链表。
   * 此函数使用原子自旋锁保护，适用于多线程环境。
   * 包含重复释放检查，防止同一对象被多次释放。
   *
   * @param[in] get_pos 要释放的对象在池中的位置索引
   */
  FORCE_INLINE void release(int32 get_pos) {
    if (unlikely(get_pos < 0 || get_pos >= n_all)) {
      return;
    }
    m_it *tp = pmem + get_pos;
    mlock.lock();
    if (tp->next_free != -2) { // 检查是否已被释放
      tp->next_free = free_first;
      free_first = get_pos;
      n_free++;
    }
    mlock.unlock();
  }
  /**
   * @brief 多线程释放对象（通过指针）
   *
   * 将指定指针的对象释放回空闲链表。
   * 此函数使用原子自旋锁保护，适用于多线程环境。
   * 内部通过data2it函数计算出对象位置。
   *
   * @param[in] get_buf 要释放的对象指针
   */
  FORCE_INLINE void release(T *get_buf) {
    m_it *pi = data2it(get_buf);
    mlock.lock();
    pi->next_free = free_first;
    free_first = pi->m_pos;
    n_free++;
    mlock.unlock();
  }

  /**
   * @brief 获取当前空闲对象数量
   *
   * @return 当前可用的空闲对象数量
   */
  FORCE_INLINE int32 free_num() const { return n_free; }
  /**
   * @brief 获取当前已使用对象数量
   *
   * @return 当前已分配使用的对象数量
   */
  FORCE_INLINE int32 used_num() const { return n_all - n_free; }
  /**
   * @brief 获取内存池总容量
   *
   * @return 内存池可容纳的对象总数
   */
  FORCE_INLINE int32 size() const { return n_all; }

  /**
   * @brief 计算指定数量对象所需的内存池缓冲区大小
   *
   * 用于外部内存分配时计算所需的缓冲区大小。
   *
   * @param[in] buf_size 对象数量
   * @return int64 所需的缓冲区大小（字节）
   */
  static int64 need_buf_size(int32 buf_size) {
    int64 ts = sizeof(fix_pool_it<T>);
    return ts * buf_size;
  }

  /**
   * @brief 初始化内存池
   *
   * 分配指定大小的内存块并初始化空闲链表结构。
   * 如果已经初始化则直接返回成功。
   *
   * @param[in] tnum 要创建的对象数量
   * @return 成功返回0，失败返回LBERR_MEM_ALLOC_FAIL
   */
  int32 init(int32 tnum) {
    mlock.lock();
    if (is_init == 1) {
      mlock.unlock();
      return 0;
    }

    reset();
    n_all = 0;

    m_it *pm = new (std::nothrow) m_it[tnum];
    if (NULL == pm) {
      mlock.unlock();
      return LBERR_MEM_ALLOC_FAIL;
    }

    init_mem(pm, tnum, 1);
    mlock.unlock();
    return 0;
  }
  /**
   * @brief 关闭内存池
   *
   * 释放内存池占用的资源，重置所有状态变量。
   * 如果内存是由池内部申请的，则释放该内存。
   */
  void close() {
    mlock.lock();
    free_first = -1;
    if (NULL != pmem && new_mem == 1) {
      delete[] pmem;
      pmem = NULL;
    }
    reset();
    mlock.unlock();
  }
  /**
   * @brief 使用外部内存初始化内存池
   *
   * 使用预分配的外部内存块初始化内存池，适用于共享内存场景。
   * 支持恢复模式，可以从已存在的内存状态中恢复空闲链表。
   *
   * @param[in] pshm 指向外部内存块的指针
   * @param[in] mem_size 内存块大小（字节）
   * @param[in] is_recove 恢复模式标志，0表示全新初始化，1表示从已有状态恢复
   */
  void init(void *pshm, int64 mem_size, int32 is_recove) {
    int64 tn = mem_size / sizeof(m_it);
    int32 tnum = (int32)(tn & 0xfffffff);
    m_it *pm = reinterpret_cast<m_it *>(pshm);

    mlock.lock();
    if (is_init == 1) {
      mlock.unlock();
      return;
    }

    if (is_recove == 0) {
      init_mem(pm, tnum, 0);
    } else {
      int32 tfn = 0;
      int32 tflast = -1;
      for (int32 i = tnum - 1; i >= 0; i--) {
        m_it &tp = pm[i];
        tp.m_pos = i;
        if (tp.next_free == -2)
          continue;
        tp.next_free = tflast;
        tflast = i;
        tfn++;
      }

      free_first = tflast;
      pmem = pm;
      n_free = tfn;
      n_all = tnum;
      new_mem = 0;
      is_init = 1;
    }
    mlock.unlock();
  }

  /**
   * @brief 构造函数
   *
   * 初始化原子锁和内存池状态变量。
   */
  fix_pool() {
    mlock.init();
    reset();
    n_all = 0;
  }
  /**
   * @brief 析构函数
   *
   * 自动调用close()释放资源。
   */
  ~fix_pool() { close(); }

  /** @brief 禁用拷贝构造函数 */
  fix_pool(const fix_pool &) = delete;
  /** @brief 禁用拷贝赋值操作符 */
  fix_pool &operator=(const fix_pool &) = delete;
};

} // namespace lb_common

#pragma once

/**
 * @file ring_hash.h
 * @brief 哈希环模块
 *
 * 这是一个固定大小的特殊用途的哈希环，用于存储在途的流式数据，
 * 数据有整型的递增的流序号。
 *
 * 配合缓存池使用，缓存池为哈希表项的池
 * 以流序号取模为索引
 *
 * @note 单线程使用
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "mutils.h"
#include <new>

namespace lb_common {

/**
 * @brief 哈希环键值对结构体
 *
 * 用于存储在途流式数据的键值对，配合环形哈希表使用。
 * 支持通过seq_no进行快速查找和删除。
 *
 * @tparam D 数据类型
 */
template <class D> struct ring_hash_kv {
  /** @brief 哈希链表下一节点指针 */
  ring_hash_kv *hash_next;
  /** @brief 流序号，作为哈希键 */
  uint64 seq_no;
  /** @brief 实际存储的数据 */
  D data;
};

/**
 * @brief 环形哈希表类
 *
 * 固定大小的特殊用途哈希环，用于存储在途的流式数据。
 * 数据有整型的递增的流序号，以流序号取模为索引。
 *
 * 配合缓存池使用，缓存池为哈希表项的池。
 *
 * @tparam D 数据类型
 *
 * @note 单线程使用，非线程安全
 * @note 需要外部提供ring_hash_kv对象的内存管理
 */
template <typename D> class ring_hash {
private:
  using m_kv = ring_hash_kv<D>;

  /** @brief 哈希掩码，用于计算哈希桶索引 */
  uint64 hash_mask;
  /** @brief 哈希桶数组，存储链表头指针 */
  m_kv **basket;
  /** @brief 是否创建内存标志，1表示创建，0表示外部提供 */
  int32 creat_mem;
  /** @brief 已填充的桶数量 */
  int32 filled;

public:
  /**
   * @brief 根据流序号查找键值对
   *
   * 使用seq_no & hash_mask计算哈希桶索引，然后遍历链表查找。
   *
   * @param[in] seq_no 流序号
   * @return 找到的键值对指针，未找到返回NULL
   */
  ring_hash_kv<D> *find(uint64 seq_no) {
    uint64 tk = (seq_no & hash_mask);
    m_kv *pb = basket[tk];

    while (NULL != pb) {
      if (pb->seq_no == seq_no)
        return pb;
      pb = pb->hash_next;
    }

    return NULL;
  }

  /**
   * @brief 插入键值对到哈希表
   *
   * 使用头插法将键值对插入到对应的哈希桶链表中。
   *
   * @param[in] val 要插入的键值对指针
   * @param[in] seq_no 流序号作为键
   */
  void insert(ring_hash_kv<D> *val, uint64 seq_no) {
    uint64 tk = (seq_no & hash_mask);
    val->seq_no = seq_no;
    val->hash_next = basket[tk];
    basket[tk] = val;
  }

  /**
   * @brief 根据流序号删除键值对
   *
   * 查找并移除指定seq_no的键值对，返回被移除的节点指针。
   * 调用者负责管理返回节点的内存。
   *
   * @param[in] seq_no 要删除的流序号
   * @return 被删除的键值对指针，未找到返回NULL
   */
  ring_hash_kv<D> *erase(uint64 seq_no) {
    uint64 tk = (seq_no & hash_mask);
    ring_hash_kv<D> *pb = basket[tk];
    ring_hash_kv<D> *prev = NULL;
    while (NULL != pb) {
      if (pb->seq_no == seq_no) {
        if (NULL == prev) {
          basket[tk] = pb->hash_next;
        } else {
          prev->hash_next = pb->hash_next;
        }
        pb->hash_next = NULL;
        return pb;
      }
      prev = pb;
      pb = pb->hash_next;
    }

    return NULL;
  }

  /**
   * @brief 初始化环形哈希表
   *
   * 根据指定的桶数量初始化哈希表。桶数量会被向上取整到2的幂。
   * 可以选择使用外部提供的内存或内部申请内存。
   *
   * @param[in] it_num 期望的桶数量，会被向上取整到2的幂
   * @param[in] pmem 外部提供的内存指针，NULL表示内部申请
   * @return 0表示成功，负值表示失败
   */
  int32 init(int32 it_num, ring_hash_kv<D> **pmem = NULL) {
    int32 tb = comm_utils::calc_power(it_num);
    uint64 ts = (1L << tb);

    hash_mask = ts - 1;
    if (NULL == pmem) {
      basket = new (std::nothrow) ring_hash_kv<D> *[ts];
      if (NULL == basket)
        return LBERR_MEM_ALLOC_FAIL;

      for (uint64 i = 0; i < ts; i++) {
        basket[i] = NULL;
      }
      creat_mem = 1;
    } else {
      basket = pmem;
      creat_mem = 0;
    }
    return 0;
  }

  /**
   * @brief 清空哈希表
   *
   * 清空所有哈希桶的链表头指针，不释放节点内存。
   */
  void clear() {
    if (hash_mask > 0) {
      for (uint64 i = 0; i < hash_mask + 1; i++) {
        basket[i] = NULL;
      }
    }
  }
  /**
   * @brief 关闭哈希表
   *
   * 释放哈希表占用的资源，包括哈希桶数组。
   * 注意：不释放链表中的节点，需要调用者自行管理。
   */
  void close() {
    hash_mask = 0;
    if (creat_mem == 1 && NULL != basket) {
      delete[] basket;
      basket = NULL;
    }
  }

  /**
   * @brief 默认构造函数
   */
  ring_hash() : hash_mask(0), basket(NULL), creat_mem(0), filled(0){};

  /**
   * @brief 析构函数
   *
   * 自动调用close()释放资源。
   */
  ~ring_hash() { close(); }

  /**
   * @brief 拷贝构造函数
   *
   * @warning 仅共享basket指针，creat_mem设为0防止双重释放
   */
  ring_hash(const ring_hash &src) {
    hash_mask = src.hash_mask;
    basket = src.basket;
    creat_mem = 0;
  }
  /**
   * @brief 赋值操作符
   *
   * @warning 仅共享basket指针，creat_mem设为0防止双重释放
   */
  ring_hash &operator=(const ring_hash &src) {
    hash_mask = src.hash_mask;
    basket = src.basket;
    creat_mem = 0;
    return *this;
  }
};

} // namespace lb_common

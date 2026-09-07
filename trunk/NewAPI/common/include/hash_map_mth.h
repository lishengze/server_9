#pragma once

/**
 * @file hash_map_mth.h
 * @brief 多线程哈希映射模块
 *
 * 这是一个可动态扩展的支持多线程的哈希map。
 *
 * 限制：
 *    1. key, value应尽量为POD类型，不应含虚接口
 *    2. value应当尽量小，过大或非POD类型应存指针
 *    3. 一般用于全局性缓存池，而非临时
 *    4. 初始化尽量单线程
 *
 * 特征：
 *    1. 使用表级的原子读写自旋锁支持多线程查找插入删除
 *    2. 通过扩展时，新申请篮子数组，复制旧篮子后，再更新篮子指针来
 *       拆分查找和扩展，使扩展独立不影响查找，从而实现非对称锁。
 *    3. 使用全局扩展读写锁（读写互斥），来保证唯一扩展
 *    4. 扩展采用非一次性重新哈希，而在后续插入时按步长哈希。
 *       为此，篮子大小设定为2的幂，篮子项内分单双冲突链表，
 *       扩展按2倍扩展，这样扩展后，篮子i的单数冲突表在篮子i+n中（n为原大小）
 *       双数仍保持在i中
 *    5. 仅提供读遍历操作，遍历操作时，获取扩展读锁来禁止扩展
 *
 * @note 使用模板实现，N为哈希块大小
 */

#include "comm_errno.h"
#include "comm_sys.h"
// #include "matomic.h"
#include "mlock.h"
#include "mutils.h"

#include <new>
#include <utility>
#include <vector>

namespace lb_common {
/**
 * @brief 多线程哈希映射类
 *
 * 这是一个支持动态扩展的多线程安全哈希表，使用分离锁策略优化并发性能。
 * 支持高并发的读、写、插入和删除操作。
 *
 * @tparam N 哈希块大小，每个块包含的键值对数量
 * @tparam K 键类型
 * @tparam V 值类型
 * @tparam HASH 哈希函数类型
 */
// V 应当尽量小，大的应存指针
template <int32 N, typename K, typename V, typename HASH> class hash_map_mth {
private:
  /**
   * @brief 哈希表键值对节点结构
   *
   * 用于存储哈希表中的单个键值对，支持链表冲突解决
   *
   * @tparam T_K 键类型
   * @tparam T_V 值类型
   */
  template <typename T_K, typename T_V> struct hash_mth_kv {
    T_K key;           ///< 键
    hash_mth_kv *next; ///< 指向下一个冲突节点的指针
    T_V val;           ///< 值
    int32 pos;         ///< 在块中的位置索引
    int32 next_free;   ///< 下一个空闲位置的索引
  };
  using m_kv = hash_mth_kv<K, V>;

  /**
   * @brief 哈希表篮子结构
   *
   * 每个篮子包含两个链表：偶数链表和奇数链表
   * 用于支持动态扩展时的增量重新哈希
   */
  struct hash_mth_basket {
    int32 need_rehash; ///< 是否需要重新哈希的标志
    m_kv *list_even;   ///< 偶数哈希值的链表头
    m_kv *list_odd;    ///< 奇数哈希值的链表头
  };

  /**
   * @brief 哈希表键值对块管理类
   *
   * 管理固定大小的键值对存储块，支持快速分配和回收
   *
   * @tparam N_B 块大小，每个块包含的键值对数量
   */
  template <int32 N_B> class hash_kv_block {
  protected:
    int32 free_pos;          ///< 当前空闲位置索引
    int32 free_num;          ///< 空闲节点数量
    hash_kv_block *next;     ///< 指向下一个空闲块的指针
    hash_kv_block *all_next; ///< 指向所有已分配块的链表指针
    m_kv datas[N_B];         ///< 存储键值对的数组

    friend class hash_map_mth;

  public:
    /**
     * @brief 从块中获取一个空闲的键值对节点
     *
     * @param[out] o_buf 输出参数，返回获取到的节点指针
     * @return int32 0表示成功，LBERR_OBJ_IS_EMPTY表示块已满
     */
    FORCE_INLINE int32 get(m_kv *&o_buf) {
      if (likely(free_pos != -1)) {
        o_buf = &(datas[free_pos]);
        free_pos = o_buf->next_free;
        free_num--;
        return 0;
      }
      free_num = 0;
      return LBERR_OBJ_IS_EMPTY;
    }
    /**
     * @brief 将节点归还到块中
     *
     * @param[in] pbuf 要归还的节点指针
     */
    FORCE_INLINE void back(m_kv *pbuf) {
      pbuf->next = NULL;
      pbuf->next_free = free_pos;
      free_pos = pbuf->pos;
      free_num++;
    }

    /**
     * @brief 初始化块，设置空闲链表
     */
    void init() {
      free_pos = 0;
      free_num = N_B;
      next = NULL;
      all_next = NULL;

      datas[N_B - 1].next = NULL;
      datas[N_B - 1].pos = N_B - 1;
      datas[N_B - 1].next_free = -1;

      for (int32 i = N_B - 2; i >= 0; --i) {
        datas[i].next = NULL;
        datas[i].pos = i;
        datas[i].next_free = i + 1;
      }
    }
    /**
     * @brief 构造函数
     */
    hash_kv_block() {
      free_pos = 0;
      free_num = N_B;
      next = NULL;
      all_next = NULL;
    };
    /**
     * @brief 析构函数
     */
    ~hash_kv_block(){};
  };

  atomic_rwlock data_lock;     ///< 数据读写锁，保护哈希表数据结构
  int32 hash_bit;              ///< 哈希位数，2的幂数
  uint64 hash_mask;            ///< 哈希掩码，等于(2^hash_bit - 1)
  hash_mth_basket *basks;      ///< 篮子数组指针
  uint64 num;                  ///< 当前存储的键值对数量
  uint64 rehash_pos;           ///< 重新哈希的位置指针
  hash_kv_block<N> *kv_pool;   ///< 空闲块链表头指针
  rwlock exp_lock;             ///< 扩展和遍历迭代锁
  hash_kv_block<N> *alloc_kvs; ///< 所有已分配块的链表头指针

public:
  /**
   * @brief 查找指定键的值
   *
   * @param[out] o_val 输出参数，返回找到的值
   * @param[in] key 要查找的键
   * @param[in] need_lock 是否需加锁，若不需加锁，则一般用于先构造好map,然后才允许读的情况
   * @return int32
   * 0表示成功，LBERR_OBJ_IS_EMPTY表示哈希表为空，LBERR_OBJ_NOT_HAVE表示键不存在
   */
  int32 find(V &o_val, const K &key, int32 need_lock = 1) {
    if (unlikely(0 == hash_mask))
      return LBERR_OBJ_IS_EMPTY;

    uint64 tk = HASH()(key);
    int32 ret = 0;

    if (need_lock == 1) {
      data_lock.read_lock();
      ret = find_basket(o_val, key, basks[tk & hash_mask]);
      data_lock.read_unlock();
    } else {
      ret = find_basket(o_val, key, basks[tk & hash_mask]);
    }

    return ret;
  }

  /**
   * @brief 插入新的键值对（如果键已存在会失败）
   *
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32 0表示成功，LBERR_MEM_ALLOC_FAIL表示内存分配失败
   */
  int32 insert_new(V &val, const K &key) {
    // hash_kv_block<N> *tblk = NULL;
    uint64 tk = HASH()(key);
    m_kv *tnkv = NULL;

    while (true) {
      data_lock.write_lock();

      uint64 tcur_size = hash_mask > 0 ? (hash_mask + 1) : 0;
      if (unlikely((num << 1) >= tcur_size)) {
        data_lock.write_unlock(); // 让读继续可读

        // 若插入数量超过篮子大小的1/2，则扩展篮子大小*2
        exp_lock.write_lock(); // 扩展互斥
        hash_mth_basket *tpn_basks = NULL;
        int64 tnum_basks = expand(tpn_basks, tcur_size * 2);
        if (unlikely(tnum_basks <= 0)) {
          exp_lock.write_unlock();

          if (tnum_basks == 0) // 其他已经扩展
            continue;
          return LBERR_MEM_ALLOC_FAIL;
        }

        hash_mth_basket *tpold_basks;
        data_lock.write_lock();
        hash_bit += 1;
        tpold_basks = basks;
        basks = tpn_basks;
        hash_mask = tnum_basks - 1;
        rehash_pos = 0;
        data_lock.write_unlock();
        exp_lock.write_unlock();

        if (NULL != tpold_basks)
          delete[] tpold_basks;

        continue;
      }

      int32 ret = alloc_kv(tnkv);
      if (ret < 0) {
        data_lock.write_unlock();

        hash_kv_block<N> *tnblk = new (std::nothrow) hash_kv_block<N>;
        if (NULL == tnblk)
          return LBERR_MEM_ALLOC_FAIL;
        tnblk->init();

        data_lock.write_lock();
        tnblk->next = kv_pool;
        kv_pool = tnblk;
        tnblk->all_next = alloc_kvs;
        alloc_kvs = tnblk;
        alloc_kv(tnkv);
      }

      hash_mth_basket &tbask = basks[tk & hash_mask];
      tnkv->key = key;
      tnkv->next = NULL;
      tnkv->val = val;
      if (((tk >> hash_bit) & 1) == 0) {
        tnkv->next = tbask.list_even;
        tbask.list_even = tnkv;
      } else {
        tnkv->next = tbask.list_odd;
        tbask.list_odd = tnkv;
      }
      num++;

      ret = 7;
      // 每次新插入，至少重新rehash 7次，
      // 以保证下次需要rehash时，所有篮子全部rehash 过
      while (ret > 0 && rehash_pos <= hash_mask) {
        if (basks[rehash_pos].need_rehash == 1) {
          rehash_basket(basks[rehash_pos]);
        }
        ++rehash_pos;
        --ret;
      }
      data_lock.write_unlock();
      return 0;
    }
    while (true)
      ;
  }

  /**
   * @brief 插入键值对（如果键已存在会失败）
   *
   * 与insert_new的区别是会先检查键是否已存在
   *
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32
   * 0表示成功，LBERR_OBJ_HAVE_EXIST表示键已存在，LBERR_MEM_ALLOC_FAIL表示内存分配失败
   */
  int32 insert(V &val, const K &key) {
    // hash_kv_block<N> *tblk = NULL;
    uint64 tk = HASH()(key);
    m_kv *tnkv = NULL;

    while (true) {
      data_lock.write_lock();

      uint64 tcur_size = hash_mask > 0 ? (hash_mask + 1) : 0;
      if (unlikely((num << 1) >= tcur_size)) {
        data_lock.write_unlock(); // 让读继续可读

        // 若插入数量超过篮子大小的1/2，则扩展篮子大小*2
        exp_lock.write_lock(); // 扩展互斥
        hash_mth_basket *tpn_basks = NULL;
        int64 tnum_basks = expand(tpn_basks, tcur_size * 2);
        if (unlikely(tnum_basks <= 0)) {
          exp_lock.write_unlock();

          if (tnum_basks == 0) // 其他已经扩展
            continue;
          return LBERR_MEM_ALLOC_FAIL;
        }

        hash_mth_basket *tpold_basks;
        data_lock.write_lock();
        hash_bit += 1;
        tpold_basks = basks;
        basks = tpn_basks;
        hash_mask = tnum_basks - 1;
        rehash_pos = 0;
        data_lock.write_unlock();
        exp_lock.write_unlock();

        if (NULL != tpold_basks)
          delete[] tpold_basks;

        continue;
      }

      int32 ret = alloc_kv(tnkv);
      if (ret < 0) {
        data_lock.write_unlock();

        hash_kv_block<N> *tnblk = new (std::nothrow) hash_kv_block<N>;
        if (NULL == tnblk)
          return LBERR_MEM_ALLOC_FAIL;
        tnblk->init();

        data_lock.write_lock();
        tnblk->next = kv_pool;
        kv_pool = tnblk;
        tnblk->all_next = alloc_kvs;
        alloc_kvs = tnblk;
        alloc_kv(tnkv);
      }

      CACHE_PREFETCH_TMP((void *)tnkv, 1);

      hash_mth_basket &tbask = basks[tk & hash_mask];
      ret = insert_find_basket(key, tbask);
      if (ret == 0) {
        kv_pool->back(tnkv);
        data_lock.write_unlock();
        return LBERR_OBJ_HAVE_EXIST;
      }

      tnkv->key = key;
      tnkv->next = NULL;
      tnkv->val = val;

      if (((tk >> hash_bit) & 1) == 0) {
        tnkv->next = tbask.list_even;
        tbask.list_even = tnkv;
      } else {
        tnkv->next = tbask.list_odd;
        tbask.list_odd = tnkv;
      }
      num++;

      ret = 7;
      // 每次新插入，至少重新rehash 7次，
      // 以保证下次需要rehash时，所有篮子全部rehash 过
      while (ret > 0 && rehash_pos <= hash_mask) {
        if (basks[rehash_pos].need_rehash == 1) {
          rehash_basket(basks[rehash_pos]);
        }
        ++rehash_pos;
        --ret;
      }
      data_lock.write_unlock();
      return 0;
    }
    while (true)
      ;
  }

  /**
   * @brief 删除指定键的键值对
   *
   * @param[in] key 要删除的键
   */
  void erase(const K &key) {
    m_kv *tnkv = NULL;
    uint64 tk = HASH()(key);

    data_lock.write_lock();

    if (unlikely(0 == hash_mask)) {
      data_lock.write_unlock();
      return;
    }

    int32 ret = del_from_basket(tnkv, key, basks[tk & hash_mask]);
    if (ret == 0) {
      num--;
      constexpr size_t offset = offsetof(hash_kv_block<N>, datas);
      char *datas_addr = reinterpret_cast<char *>(tnkv - tnkv->pos);
      hash_kv_block<N> *tpb = reinterpret_cast<hash_kv_block<N> *>(datas_addr - offset);
      tpb->back(tnkv);
      if (tpb->free_num == 1) {
        tpb->next = kv_pool;
        kv_pool = tpb;
      }
    }

    data_lock.write_unlock();
    return;
  }

  /**
   * @brief 删除指定键的键值对并返回其值
   *
   * @param[out] o_val 输出参数，返回被删除的值
   * @param[in] key 要删除的键
   */
  void erase(V &o_val, const K &key) {
    m_kv *tnkv = NULL;
    uint64 tk = HASH()(key);

    data_lock.write_lock();
    if (unlikely(0 == hash_mask)) {
      data_lock.write_unlock();
      return;
    }

    int32 ret = del_from_basket(tnkv, key, basks[tk & hash_mask]);
    if (ret == 0) {
      o_val = tnkv->val;
      num--;

      constexpr size_t offset = offsetof(hash_kv_block<N>, datas);
      char *datas_addr = reinterpret_cast<char *>(tnkv - tnkv->pos);
      hash_kv_block<N> *tpb = reinterpret_cast<hash_kv_block<N> *>(datas_addr - offset);
      tpb->back(tnkv);
      if (tpb->free_num == 1) {
        tpb->next = kv_pool;
        kv_pool = tpb;
      }
    }
    data_lock.write_unlock();
    return;
  }
  /**
   * @brief 清空哈希表数据，但保留内存
   */
  void clear() {
    exp_lock.write_lock();
    data_lock.write_lock();

    if (NULL != basks) {
      for (uint64 i = 0; i < hash_mask + 1; i++) {
        basks[i].need_rehash = 0;
        basks[i].list_even = NULL;
        basks[i].list_odd = NULL;
      }
    }
    num = 0;
    rehash_pos = hash_mask + 1;

    kv_pool = NULL;
    while (NULL != alloc_kvs) {
      hash_kv_block<N> *tpb = alloc_kvs;
      alloc_kvs = alloc_kvs->all_next;
      delete tpb;
    }
    alloc_kvs = NULL;

    data_lock.write_unlock();
    exp_lock.write_unlock();
  }

  /**
   * @brief 获取哈希表中元素的数量
   *
   * @return uint64 元素数量
   */
  FORCE_INLINE uint64 size() const { return num; }
  /**
   * @brief 获取篮子数组的大小
   *
   * @return uint64 篮子数量
   */
  FORCE_INLINE uint64 basket_size() const { return (hash_mask > 0 ? (hash_mask + 1) : 0); }

  /**
   * @brief 初始化哈希表
   *
   * @param[in] init_num 初始容量，小于等于0时使用默认值
   * @return int32 0表示成功，其他值表示错误
   */
  int32 init(int64 init_num) {
    if (init_num <= 0) {
      init_num = N * 2;
    }

    int32 tn = comm_utils::calc_power(init_num);
    int64 ts = (1LL << tn);

    exp_lock.write_lock();
    data_lock.write_lock();

    if (hash_mask > 0) {
      data_lock.write_unlock();
      exp_lock.write_unlock();
      return 0;
    }

    hash_bit = 0;
    hash_mask = 0;
    basks = NULL;
    num = 0;
    rehash_pos = 0;
    kv_pool = NULL;
    alloc_kvs = NULL;

    hash_mth_basket *tpbask = NULL;
    int32 ret = expand(tpbask, ts);
    if (ret < 0) {
      data_lock.write_unlock();
      exp_lock.write_unlock();
      return ret;
    }

    hash_bit = tn;
    hash_mask = ts - 1;
    basks = tpbask;
    num = 0;
    rehash_pos = ts;

    hash_kv_block<N> *tnblk = new (std::nothrow) hash_kv_block<N>;
    if (NULL == tnblk) {
      data_lock.write_unlock();
      exp_lock.write_unlock();
      return LBERR_MEM_ALLOC_FAIL;
    }
    tnblk->init();

    tnblk->next = kv_pool;
    kv_pool = tnblk;
    tnblk->all_next = alloc_kvs;
    alloc_kvs = tnblk;

    data_lock.write_unlock();
    exp_lock.write_unlock();
    return 0;
  }

  /**
   * @brief 构造函数
   */
  hash_map_mth() : hash_bit(0), hash_mask(0), basks(NULL), num(0), rehash_pos(0), kv_pool(NULL), alloc_kvs(NULL) {
    data_lock.init();
  };
  /**
   * @brief 析构函数
   */
  ~hash_map_mth() {
    clear();
    if (NULL != basks) {
      delete[] basks;
      basks = NULL;
      hash_bit = 0;
      hash_mask = 0;
    }
  };

private:
  /**
   * @brief 在指定篮子中查找键值对
   *
   * @param[out] o_val 输出参数，返回找到的值
   * @param[in] key 要查找的键
   * @param[in] tb 篮子引用
   * @return int32 0表示成功，LBERR_OBJ_NOT_HAVE表示键不存在
   */
  int32 find_basket(V &o_val, const K &key, hash_mth_basket &tb) {
    m_kv *tp = tb.list_even;
    while (NULL != tp) {
      if (tp->key == key) {
        o_val = tp->val;
        return 0;
      }
      tp = tp->next;
    }
    tp = tb.list_odd;
    while (NULL != tp) {
      if (tp->key == key) {
        o_val = tp->val;
        return 0;
      }
      tp = tp->next;
    }
    return LBERR_OBJ_NOT_HAVE;
  }

  /**
   * @brief 在插入时查找篮子中的键，同时进行重新哈希
   *
   * @param[in] key 要查找的键
   * @param[in] tb 篮子引用
   * @return int32 0表示键已存在，LBERR_OBJ_NOT_HAVE表示键不存在
   */
  int32 insert_find_basket(const K &key, hash_mth_basket &tb) {
    if (tb.need_rehash == 0) {
      V tf_val;
      return find_basket(tf_val, key, tb);
    }

    // 发生篮子扩展，并且该篮子未重新hash
    // 遍历链表，查看是否存在并重新hash每项
    int32 ret = LBERR_OBJ_NOT_HAVE;
    m_kv *tprev = NULL;
    m_kv *tp = tb.list_even;
    while (NULL != tp) {
      if (tp->key == key) {
        ret = 0;
      }

      int64 tk = HASH()(tp->key);
      // tk>>hash_bit 等价于 tk/(hash_mask+1)
      if (((tk >> hash_bit) & 1) == 0) {
        // tk>>hash_bit为双数，保持在原篮子的双数
        tprev = tp;
        tp = tp->next;
      } else {
        m_kv *tnext = tp->next;
        if (NULL == tprev) {
          tb.list_even = tnext;
        } else {
          tprev->next = tnext;
        }
        tp->next = tb.list_odd;
        tb.list_odd = tp;
        tp = tnext;
      }
    }
    tb.need_rehash = 0;
    return ret;
  }

  /**
   * @brief 从空闲块链表中分配一个键值对节点
   *
   * @param[out] o_buf 输出参数，返回分配到的节点指针
   * @return int32 0表示成功，LBERR_MEM_ALLOC_FAIL表示分配失败
   */
  FORCE_INLINE int32 alloc_kv(m_kv *&o_buf) {
    while (NULL != kv_pool) {
      if (0 == kv_pool->get(o_buf))
        return 0;
      kv_pool = kv_pool->next;
    }
    return LBERR_MEM_ALLOC_FAIL;
  }
  /**
   * @brief 重新哈希指定篮子
   *
   * 将偶数链表中需要移动的节点移动到奇数链表
   *
   * @param[in] tb 篮子引用
   */
  void rehash_basket(hash_mth_basket &tb) {
    m_kv *tprev = NULL;
    m_kv *tp = tb.list_even;
    while (NULL != tp) {
      uint64 tk = HASH()(tp->key);
      if (((tk >> hash_bit) & 1) == 0) {
        tprev = tp;
        tp = tp->next;
      } else {
        m_kv *tnext = tp->next;
        if (NULL == tprev) {
          tb.list_even = tnext;
        } else {
          tprev->next = tnext;
        }
        tp->next = tb.list_odd;
        tb.list_odd = tp;
        tp = tnext;
      }
    }
    tb.need_rehash = 0;
  }

  /**
   * @brief 从篮子中删除指定键的节点
   *
   * @param[out] o_kv 输出参数，返回被删除的节点指针
   * @param[in] key 要删除的键
   * @param[in] tb 篮子引用
   * @return int32 0表示成功，LBERR_OBJ_NOT_HAVE表示键不存在
   */
  int32 del_from_basket(m_kv *&o_kv, const K &key, hash_mth_basket &tb) {
    int32 ret = LBERR_OBJ_NOT_HAVE;
    m_kv *tprev = NULL;
    m_kv *tp = tb.list_even;
    while (NULL != tp) {
      m_kv *tnext = tp->next;
      if (tp->key == key) {
        o_kv = tp;
        if (NULL == tprev) {
          tb.list_even = tnext;
        } else {
          tprev->next = tnext;
        }
        if (tb.need_rehash == 0)
          return 0;
        else {
          ret = 0;
          tp = tnext;
          continue;
        }
      } else {
        uint64 tk = HASH()(tp->key);
        if (((tk >> hash_bit) & 1) == 0) {
          tprev = tp;
          tp = tnext;
        } else {

          if (NULL == tprev) {
            tb.list_even = tnext;
          } else {
            tprev->next = tnext;
          }
          tp->next = tb.list_odd;
          tb.list_odd = tp;
          tp = tnext;
        }
      }
    }

    if (ret == 0 || tb.need_rehash == 1) {
      tb.need_rehash = 0;
      return ret;
    }

    tprev = NULL;
    tp = tb.list_odd;
    while (NULL != tp) {
      if (tp->key == key) {
        o_kv = tp;
        if (NULL == tprev) {
          tb.list_odd = tp->next;
        } else {
          tprev->next = tp->next;
        }
        return 0;
      }
      tprev = tp;
      tp = tp->next;
    }
    return LBERR_OBJ_NOT_HAVE;
  }

  /**
   * @brief 扩展哈希表篮子数组
   *
   * @param[out] o_baskets 输出参数，返回新的篮子数组指针
   * @param[in] ts 新的篮子数量
   * @return int64 新篮子数量，负数表示错误
   */
  int64 expand(hash_mth_basket *&o_baskets, uint64 ts) {
    if (unlikely(ts <= hash_mask + 1)) {
      return 0;
    }
    // 直接申请新的内存
    hash_mth_basket *tpm = new (std::nothrow) hash_mth_basket[ts];
    if (NULL == tpm)
      return LBERR_MEM_ALLOC_FAIL;

    uint64 i = 0;
    if (hash_mask == 0) {
      for (i = 0; i < ts; i++) {
        tpm[i].need_rehash = 0;
        tpm[i].list_even = NULL;
        tpm[i].list_odd = NULL;
      }
      o_baskets = tpm;
      return ts;
    }

    // 将原篮子按单双复制到新篮子内存，
    // 原双数复制到新篮子相同桶的双数
    // 原单数复制到新篮子的+原篮子数量的扩展桶的双数
    uint64 old_size = (hash_mask + 1);
    for (i = 0; i < old_size; i++) {
      tpm[i].need_rehash = 1;
      tpm[i].list_even = basks[i].list_even;
      tpm[i].list_odd = NULL;
    }
    for (i = old_size; i < ts; i++) {
      tpm[i].need_rehash = 1;
      tpm[i].list_even = basks[i - old_size].list_odd;
      tpm[i].list_odd = NULL;
    }

    o_baskets = tpm;
    return ts;
  }

public:
  friend class it_read_local;

  /**
   * @brief 局部变量读迭代器类
   *
   * 仅应用于需要遍历时，提供线程安全的只读迭代功能
   * 使用缓冲机制减少锁持有时间
   */
  class it_read_local {
  private:
    using KvPair = std::pair<K, V>; ///< 存储key-value对的类型

    hash_map_mth *map_;          ///< 指向哈希表的指针
    uint64 current_basket_;      ///< 当前正在处理的篮子索引
    uint64 basket_count_;        ///< 篮子总数（在遍历开始时固定）
    uint32 buffer_pos_;          ///< 在当前缓冲区中的位置
    int32 keep_exp_lock;         ///< 是否持有扩展锁的标志
    std::vector<KvPair> buffer_; ///< 当前篮子的数据缓冲区

  public:
    // 迭代器类型定义
    using value_type = std::pair<K, V>;
    using reference = value_type &;
    using pointer = value_type *;

    /**
     * @brief 构造函数
     *
     * @param[in] p_map 指向哈希表的指针
     */
    it_read_local(hash_map_mth *p_map)
        : map_(p_map), current_basket_(0), basket_count_(0), buffer_pos_(0), keep_exp_lock(0) {
      if (NULL == map_) {
        return;
      }

      buffer_.reserve(16);
    }

    it_read_local() = delete;
    it_read_local(const it_read_local &) = delete;
    it_read_local &operator=(const it_read_local &) = delete;
    it_read_local(it_read_local &) = delete;
    it_read_local &operator=(it_read_local &) = delete;

    /**
     * @brief 析构函数
     */
    ~it_read_local() {
      if (keep_exp_lock == 1 && NULL != map_) {
        map_->exp_lock.read_unlock();
      }
    }

    /**
     * @brief 开始遍历所有元素
     *
     * @return bool 成功返回true，哈希表为空返回false
     */
    bool begin() {
      if (NULL == map_) {
        return false;
      }
      if (keep_exp_lock == 1) {
        map_->exp_lock.read_unlock();
        keep_exp_lock = 0;
      }
      clear_buf();
      lock_set_bask();
      current_basket_ = 0;
      if (basket_count_ == 0)
        return false;

      move_to_next_basket();
      return !buffer_.empty();
    }

    /**
     * @brief 从指定键开始遍历
     *
     * @param[in] key 起始键
     * @return bool 成功返回true，哈希表为空返回false
     */
    bool begin(const K &key) {
      if (NULL == map_) {
        return false;
      }
      if (keep_exp_lock == 1) {
        map_->exp_lock.read_unlock();
        keep_exp_lock = 0;
      }
      clear_buf();
      lock_set_bask();
      current_basket_ = 0;
      if (basket_count_ == 0)
        return false;

      uint64 hash_val = HASH()(key);
      current_basket_ = (hash_val & (basket_count_ - 1));
      // 加载当前篮子
      load_basket(key, current_basket_);

      return !buffer_.empty();
    }
    /**
     * @brief 检查迭代器是否有效
     *
     * @return bool 有效返回true，否则返回false
     */
    FORCE_INLINE bool is_valid() const {
      return (NULL != map_ && current_basket_ < basket_count_ && buffer_pos_ < buffer_.size());
    }
    /**
     * @brief 移动到下一个元素
     *
     * @return bool 成功移动返回true，否则返回false
     */
    bool next() {
      if (NULL == map_ || keep_exp_lock == 0) {
        return false;
      }

      buffer_pos_++;
      if (buffer_pos_ < buffer_.size()) {
        return true;
      }
      clear_buf();
      current_basket_++;
      move_to_next_basket();
      return !buffer_.empty();
    }

    /**
     * @brief 解引用操作符
     *
     * @return reference 当前键值对的引用
     */
    reference operator*() const {
      if (unlikely(buffer_pos_ >= buffer_.size())) {
        assert(0);
        // throw std::out_of_range("read local iterator out of range");
      }
      return buffer_[buffer_pos_];
    }

    /**
     * @brief 箭头操作符
     *
     * @return pointer 当前键值对的指针
     */
    pointer operator->() const {
      if (unlikely(buffer_pos_ >= buffer_.size())) {
        assert(0);
        // throw std::out_of_range("read local iterator out of range");
      }
      return &(buffer_[buffer_pos_]);
    }

    /**
     * @brief 获取当前键
     *
     * @return const K& 键的引用
     */
    const K &first() const {
      if (unlikely(buffer_pos_ >= buffer_.size())) {
        assert(0);
        // throw std::out_of_range("read local iterator out of range");
      }
      return buffer_[buffer_pos_].first;
    }

    /**
     * @brief 获取当前值
     *
     * @return const V& 值的引用
     */
    const V &second() const {
      if (unlikely(buffer_pos_ >= buffer_.size())) {
        assert(0);
        // throw std::out_of_range("read local iterator out of range");
      }
      return buffer_[buffer_pos_].second;
    }
    /**
     * @brief 获取当前键（别名方法）
     *
     * @return const K& 键的引用
     */
    const K &key() const { return buffer_[buffer_pos_].first; }

    /**
     * @brief 获取当前值（别名方法）
     *
     * @return const V& 值的引用
     */
    const V &value() const { return buffer_[buffer_pos_].second; }

  private:
    /**
     * @brief 清空缓冲区
     */
    FORCE_INLINE void clear_buf() {
      buffer_pos_ = 0;
      buffer_.clear();
    }

    /**
     * @brief 加载指定篮子的所有数据到缓冲区
     *
     * @param[in] basket_idx 篮子索引
     */
    void load_basket(uint64 basket_idx) {
      // 加读锁，只保护当前篮子的读取
      map_->data_lock.read_lock();

      hash_mth_basket &basket = map_->basks[basket_idx];

      // 收集list_even中的所有元素
      m_kv *node = basket.list_even;
      while (NULL != node) {
        buffer_.emplace_back(node->key, node->val);
        node = node->next;
      }

      // 收集list_odd中的所有元素
      node = basket.list_odd;
      while (NULL != node) {
        buffer_.emplace_back(node->key, node->val);
        node = node->next;
      }

      map_->data_lock.read_unlock();
    }

    /**
     * @brief 加载指定篮子中从指定键开始的数据到缓冲区
     *
     * @param[in] key 起始键
     * @param[in] basket_idx 篮子索引
     */
    void load_basket(const K &key, uint64 basket_idx) {
      // 加读锁，只保护当前篮子的读取
      map_->data_lock.read_lock();

      hash_mth_basket &basket = map_->basks[basket_idx];
      bool is_find = false;
      // 收集list_even中的所有元素
      m_kv *node = basket.list_even;
      while (NULL != node) {
        if (!is_find) {
          if (node->key == key) {
            is_find = true;
          } else {
            node = node->next;
            continue;
          }
        }
        buffer_.emplace_back(node->key, node->val);
        node = node->next;
      }

      // 收集list_odd中的所有元素
      node = basket.list_odd;
      while (NULL != node) {
        if (!is_find) {
          if (node->key == key) {
            is_find = true;
          } else {
            node = node->next;
            continue;
          }
        }
        buffer_.emplace_back(node->key, node->val);
        node = node->next;
      }

      map_->data_lock.read_unlock();
    }

    /**
     * @brief 移动到下一个有数据的篮子
     */
    void move_to_next_basket() {
      while (current_basket_ < basket_count_) {
        load_basket(current_basket_);
        if (!buffer_.empty())
          return;
        current_basket_++;
      }
    }

    /**
     * @brief 锁定并设置篮子数量
     */
    void lock_set_bask() {
      map_->exp_lock.read_lock();
      map_->data_lock.read_lock();
      keep_exp_lock = 1;

      // 重新确认篮子数量（可能在获取锁的过程中发生了变化）
      basket_count_ = map_->basket_size();
      map_->data_lock.read_unlock();
    }
  };

  /**
   * @brief 便捷方法：遍历所有元素
   *
   * @tparam Func 函数类型，接受(key, value)参数
   * @param[in] func 要执行的函数
   */
  template <typename Func> void for_each(Func &&func) const {
    it_read_local it(this);
    it.begin();
    while (it.is_valid()) {
      func(it.key(), it.value());
      it.next();
    }
  }

  /**
   * @brief 从指定key开始遍历
   *
   * @tparam Func 函数类型，接受(key, value)参数
   * @param[in] start_key 起始键
   * @param[in] func 要执行的函数
   */
  template <typename Func> void for_each_from(const K &start_key, Func &&func) const {
    it_read_local it(this);
    it.begin(start_key);
    while (it.is_valid()) {
      func(it.key(), it.value());
      it.next();
    }
  }
};

} // namespace lb_common

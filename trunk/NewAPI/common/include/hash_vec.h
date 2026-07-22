#pragma once

/**
 * @file hash_vec.h
 * @brief 哈希向量和固定内存数组模块
 *
 * 本文件包含两个主要类：
 *
 * 1. hash_vec_pos - 固定大小的特殊用途hash map
 *    限制：
 *       - 大小固定，初始时设置数组元素个数，不可动态扩充
 *       - 只添加不删除
 *       - key, value必须为POD类型
 *    特征：
 *       - 内部使用数组存储kv
 *       - 查找添加，返回对应的占位索引，从1编号，>0
 *       - 可使用共享内存来存储KV
 *       - 写使用互斥自旋锁，以保证单一写入，从而可使得读不加锁
 *       - 单个线程插入，必须保证每个key只插入一次
 *       - 多线程插入操作检查重复
 *    用途：
 *       - 该map主要与数组内存联合使用，用于key的检索和数组中内存的占位
 *
 * 2. vec_fix_mem - 固定内存大小的数组
 *    限制：
 *       - 大小固定，初始时设置数组元素个数和内存，不可动态扩充
 *       - 存储对象必须为POD类型
 *       - 只添加不删除，并且添加必须指定数组索引添加
 *       - 通过数组索引获取的内存，不一定数据是有效的
 *    特征：
 *       - 读写可在不同线程
 *       - 可使用共享内存
 *       - 与上面哈希Map联合使用，先map占位，再用占位索引从数组获取内存
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "matomic.h"
#include "mlock.h"
#include "mutils.h"

#include <cstring>
#include <iostream>

namespace lb_common {

/**
 * @brief 固定大小的特殊用途哈希映射类
 *
 * 这是一个固定大小的哈希表，用于key的检索和数组中内存的占位。
 * 主要与数组内存联合使用，支持共享内存存储。
 *
 * @tparam K 键类型，必须为POD类型
 * @tparam V 值类型，必须为POD类型
 * @tparam HASH 哈希函数类型
 */
template <typename K, typename V, typename HASH> class hash_vec_pos {
private:
  /**
   * @brief 哈希表键值对节点结构
   *
   * 存储单个键值对及其在数组中的位置信息
   *
   * @tparam T_K 键类型
   * @tparam T_V 值类型
   */
  template <typename T_K, typename T_V> struct hash_pos_kv {
    T_K key;        ///< 键
    int32 next_pos; ///< 下一个冲突节点的位置索引
    int32 m_pos;    ///< 在数组中的位置索引（从1开始）
    T_V data;       ///< 存储的数据

    /**
     * @brief 初始化节点
     */
    void init() {
      next_pos = 0;
      m_pos = 0;
    }
  };
  using m_kv = hash_pos_kv<K, V>;

  /**
   * @brief 哈希表头部结构
   *
   * 存储哈希表的全局信息和状态
   */
  struct hash_vec_head {
    atomic_lock insert_lock; ///< 插入操作的互斥锁
    int32 i_num;             ///< 插入的元素数量
    int32 kv_num;            ///< 键值对总容量
    int32 kv_used;           ///< 已使用的键值对数量
    int32 is_inshm;          ///< 是否使用共享内存
    int64 mem_size;          ///< 内存大小
  };

  uint64 hash_mask;    ///< 哈希掩码，等于篮子数量-1
  int32 *basket;       ///< 篮子数组指针
  m_kv *kv_vec;        ///< 键值对数组指针
  hash_vec_head *head; ///< 头部结构指针

public:
  /**
   * @brief 查找指定键的位置索引
   *
   * 成功时返回占位索引（从1编号，>0）
   *
   * @param[in] key 要查找的键
   * @return int32
   * 占位索引，LBERR_OBJ_IS_EMPTY表示哈希表为空，LBERR_OBJ_NOT_HAVE表示键不存在
   */
  int32 find(const K &key) {
    if (unlikely(0 == hash_mask))
      return LBERR_OBJ_IS_EMPTY;

    uint64 tk = HASH()(key);
    int32 kv_pos = atomic_load32(basket + (tk & hash_mask));

    while (kv_pos > 0) {
      m_kv &pkv = kv_vec[kv_pos - 1];
      if (pkv.key == key) {
        return kv_pos;
      }
      kv_pos = pkv.next_pos;
    }

    return LBERR_OBJ_NOT_HAVE;
  }
  /**
   * @brief 查找指定键并返回数据指针
   *
   * @param[out] o_data 输出参数，返回找到的数据指针
   * @param[in] key 要查找的键
   * @return int32
   * 占位索引，LBERR_OBJ_IS_EMPTY表示哈希表为空，LBERR_OBJ_NOT_HAVE表示键不存在
   */
  int32 find(V *&o_data, const K &key) {
    if (unlikely(0 == hash_mask))
      return LBERR_OBJ_IS_EMPTY;

    uint64 tk = HASH()(key);
    int32 kv_pos = atomic_load32(basket + (tk & hash_mask));

    while (kv_pos > 0) {
      m_kv &pkv = kv_vec[kv_pos - 1];
      if (pkv.key == key) {
        o_data = &(pkv.data);
        return kv_pos;
      }
      kv_pos = pkv.next_pos;
    }
    return LBERR_OBJ_NOT_HAVE;
  }

  /**
   * @brief 根据位置索引获取数据指针
   *
   * @param[in] pos 位置索引（从1开始）
   * @return V* 数据指针
   */
  FORCE_INLINE V *pos2val(int32 pos) {
    assert(hash_mask > 0 && head->kv_used >= pos);
    return &(kv_vec[pos - 1].data);
  }

  /**
   * @brief 安全地根据位置索引获取数据指针
   *
   * @param[in] pos 位置索引（从1开始）
   * @return V* 数据指针，无效位置返回NULL
   */
  FORCE_INLINE V *get(int32 pos) {
    if (likely(hash_mask > 0 && head->kv_used >= pos)) {
      return &(kv_vec[pos - 1].data);
    }
    return NULL;
  }

  /**
   * @brief 单线程插入键值对
   *
   * 不检查重复性，一定插入新key。成功时返回占位索引（从1编号，>0）
   *
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32
   * 占位索引，LBERR_OBJ_IS_EMPTY表示哈希表为空，LBERR_OBJ_IS_FULL表示哈希表已满
   */
  int32 insert(V &val, const K &key) {
    if (unlikely(0 == hash_mask))
      return LBERR_OBJ_IS_EMPTY;

    int32 kv_pos = head->kv_used;
    if (unlikely(kv_pos >= head->kv_num)) {
      return LBERR_OBJ_IS_FULL;
    }
    head->kv_used = kv_pos + 1;

    m_kv &pkv = kv_vec[kv_pos];
    ++kv_pos;
    pkv.key = key;
    pkv.m_pos = kv_pos;
    pkv.next_pos = 0;
    pkv.data = val;
    // std::memcpy((void *)(pkv.data),(void *)(&val),sizeof(V));

    uint64 tk = HASH()(key);
    tk = (tk & hash_mask);
    int32 pb = basket[tk];
    // 此处core, 导致未入map，而内存丢失
    pkv.next_pos = pb;
    atomic_store32(basket + tk, kv_pos);
    ++head->i_num;
    return kv_pos;
  }
  /**
   * @brief 多线程插入键值对
   *
   * 检查重复，若重复返回已存在位置。成功时返回占位索引（从1编号，>0）
   *
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32
   * 占位索引，LBERR_OBJ_IS_EMPTY表示哈希表为空，LBERR_OBJ_IS_FULL表示哈希表已满
   */
  int32 insert_mth(V &val, const K &key) {
    if (unlikely(0 == hash_mask))
      return LBERR_OBJ_IS_EMPTY;

    head->insert_lock.lock();

    uint64 tk = HASH()(key);
    tk = (tk & hash_mask);
    int32 tpos = basket[tk];
    while (tpos > 0) {
      m_kv &pkv = kv_vec[tpos - 1];
      if (pkv.key == key) {
        head->insert_lock.unlock();
        return tpos;
      }
      tpos = pkv.next_pos;
    }
    tpos = basket[tk];

    int32 kv_pos = head->kv_used;
    if (unlikely(kv_pos >= head->kv_num)) {
      head->insert_lock.unlock();
      return LBERR_OBJ_IS_FULL;
    }
    head->kv_used = kv_pos + 1;

    m_kv &pkv = kv_vec[kv_pos];
    ++kv_pos;
    pkv.key = key;
    pkv.m_pos = kv_pos;
    pkv.next_pos = tpos;
    pkv.data = val;
    // std::memcpy((void *)(pkv.data),(void *)(&val),sizeof(V));
    atomic_store32(basket + tk, kv_pos);

    ++head->i_num;
    head->insert_lock.unlock();
    return kv_pos;
  }

  /**
   * @brief 获取当前插入的元素数量
   *
   * @return int32 元素数量
   */
  FORCE_INLINE int32 size() const { return (hash_mask > 0 ? head->i_num : 0); }
  /**
   * @brief 获取空闲位置数量
   *
   * @return int32 空闲位置数量
   */
  FORCE_INLINE int32 free_num() const { return (hash_mask > 0 ? (head->kv_num - head->kv_used) : 0); }
  /**
   * @brief 获取总容量
   *
   * @return int32 总容量
   */
  FORCE_INLINE int32 total_num() const { return (hash_mask > 0 ? head->kv_num : 0); }
  /**
   * @brief 获取篮子数组大小
   *
   * @return int64 篮子数量
   */
  FORCE_INLINE int64 basket_size() const { return (hash_mask > 0 ? (hash_mask + 1) : 0); }
  /**
   * @brief 初始化哈希表
   *
   * @param[in] shm_name 共享内存名称，NULL或空字符串表示不使用共享内存
   * @param[in] it_num 初始元素数量
   * @param[in] is_create 是否为创建模式，1表示创建，0表示打开
   * @return int32 0表示成功，负值表示失败
   */
  int32 init(const char *shm_name, int32 it_num, int32 is_create) {
    int32 ti = sizeof(m_kv) * (it_num);
    ti = ALIGN_UP(ti + sizeof(hash_vec_head), PAGE_SIZE);
    it_num = (ti - sizeof(hash_vec_head)) / sizeof(m_kv);

    int64 tb = it_num;
    tb = comm_utils::calc_power(tb);
    tb = (1L << tb);
    if (tb < it_num * 2)
      tb *= 2;

    int64 ts = ti;
    ts += sizeof(int32) * tb;

    char tname[256];
    if (NULL != shm_name && shm_name[0] != '\0') {
      comm_utils::str_copy_format(tname, shm_name, sizeof(tname));
    } else {
      std::memset(tname, 0, sizeof(tname));
    }
    void *pshm = NULL;
    int32 shmisexist = 0;
    int32 ishuge = 0;
    if (ts >= HUGE_PAGE_SIZE) {
      ishuge = 1;
    }

    if (tname[0] != '\0') {
      shmisexist = comm_utils::map_shm(pshm, tname, ts, ishuge, is_create);
      if (shmisexist < 0) {
        std::cerr << "open hash mmap error,name=" << tname << ",size=" << ts << ",hugepage=" << ishuge
                  << ",ret=" << shmisexist << std::endl;
        // assert(0);
        return LBERR_SHM_MAP_FAIL;
      }
    } else {
      pshm = comm_utils::aligned_malloc(ts, PAGE_SIZE);
      if (NULL == pshm)
        return LBERR_MEM_ALLOC_FAIL;
      shmisexist = 0;
      is_create = 1;
    }

    head = static_cast<hash_vec_head *>(pshm);
    basket = reinterpret_cast<int32 *>(((uint8 *)pshm) + sizeof(hash_vec_head));
    kv_vec = reinterpret_cast<m_kv *>(basket + tb);
    hash_mask = tb - 1;

    if (is_create == 1) {
      if (shmisexist == 0 || head->i_num == 0 || head->kv_num != it_num) {
        head->i_num = 0;
        head->kv_num = 0;
        head->kv_used = 0;
        if (NULL != shm_name) {
          head->is_inshm = 1;
        } else {
          head->is_inshm = 0;
        }
        head->mem_size = ts;
        head->insert_lock.init();

        int64 bask_num = hash_mask + 1;
        for (int64 i = 0; i < bask_num; i++) {
          basket[i] = 0;
        }
        for (int32 i = 0; i < it_num; i++) {
          kv_vec[i].init();
        }

        head->kv_num = it_num;
      } else {
        // 故障恢复
        assert(head->kv_used <= head->kv_num);

        head->insert_lock.init();
        int32 tpos = head->kv_used - 1;
        if (tpos >= 0 && kv_vec[tpos].m_pos != 0) {
          int64 tk = HASH()(kv_vec[tpos].key);
          tk = (tk & hash_mask);
          int32 pb = basket[tk];
          if (pb != tpos + 1) {
            kv_vec[tpos].next_pos = pb;
            basket[tk] = tpos + 1;
          }
        }

        // 不处理插入过程core，内存丢失的恢复
      }
    } else {
      while (head->kv_num == 0) {
        comm_utils::sleep_us(300);
      }
      if (head->kv_num != it_num || head->mem_size != ts)
        return LBERR_ARGV_WRONG;
    }

    return 0;
  }
  /**
   * @brief 关闭哈希表，释放资源
   *
   * 确保无其他线程使用时调用
   *
   * @param[in] is_create 是否为创建者，1表示创建者负责释放资源
   */
  void close(int32 is_create) {
    if (unlikely(NULL == head))
      return;

    if (is_create == 1) {
      void *tpshm = reinterpret_cast<void *>(head);
      if (head->is_inshm == 0) {
        comm_utils::aligned_free(tpshm);
      } else {
        int32 kv_num = head->kv_num;
        head->kv_used = 0;
        head->kv_num = 0;

        int64 bask_num = hash_mask + 1;
        for (int64 i = 0; i < bask_num; i++) {
          basket[i] = 0;
        }
        for (int32 i = 0; i < kv_num; i++) {
          kv_vec[i].init();
        }
        comm_utils::unmap_shm(tpshm, head->mem_size);
      }
    }

    hash_mask = 0;
    basket = NULL;
    kv_vec = NULL;
    head = NULL;
  }

  /**
   * @brief 构造函数
   */
  hash_vec_pos() : hash_mask(0), basket(NULL), kv_vec(NULL), head(NULL){};
  /**
   * @brief 析构函数
   */
  ~hash_vec_pos(){};
};

/**
 * @brief 固定内存大小的变长对象数组类
 *
 * 这是一个内存大小固定的存储对象数量固定的特殊用途的数组，存储的对象大小可变长。
 *
 * 限制：
 *    1. 大小固定，初始时设置数组元素个数和内存，不可动态扩充
 *    2. 存储对象必须为POD类型
 *    3. 只添加不删除，并且添加必须指定数组索引添加
 *    4. 通过数组索引获取的内存，不一定数据是有效的
 *
 * 特征：
 *    1. 读写可在不同线程
 *    2. 可使用共享内存
 *    3. 与hash_vec_pos联合使用，先map占位，再用占位索引从数组获取内存
 *
 * @tparam T 元素类型，必须为POD类型
 */
template <typename T> class vec_fix_mem {
protected:
  /**
   * @brief 变长内存数组头部结构体
   */
  struct vec_mem_head {
    /** @brief 当前已使用的元素数量 */
    int32 used;
    /** @brief 总元素数量 */
    int32 i_num;
    /** @brief 互斥锁，用于多线程保护 */
    atomic_lock mlock;
    /** @brief 是否为固定大小模式 */
    int16 is_fixed;
    /** @brief 是否使用共享内存 */
    int16 in_shm;
    /** @brief 当前内存偏移量 */
    int64 off;
    /** @brief 总内存大小 */
    int64 mem_size;
  };

  /** @brief 是否为固定大小模式 */
  int16 is_fixed;
  /** @brief 共享内存是否存在 */
  int16 shm_exist;
  /** @brief 总元素数量 */
  int32 i_num;
  /** @brief 头部结构体指针 */
  vec_mem_head *head;
  /** @brief 数据缓冲区指针 */
  T *buf;
  /** @brief 地址偏移数组指针（变长模式使用） */
  int64 *addrs;

public:
  /**
   * @brief 依据位置获取内存，用于读方
   *
   * 根据指定位置获取元素指针。读操作不需要加锁。
   *
   * @param[out] o_data 输出参数，返回元素指针
   * @param[in] pos 要获取的元素位置（从1开始）
   * @return 0表示成功，负值表示失败
   */
  int32 get(T *&o_data, int32 pos) {
    if (unlikely(i_num < pos)) { // || head->used < pos)){
      return LBERR_OBJ_IS_EMPTY;
    }
    if (is_fixed == 1) {
      o_data = buf + (pos - 1);
      return 0;
    } else {
      int64 ta = atomic_load64(&(addrs[pos - 1])); // addrs[pos - 1];
      if (ta >= 0) {
        char *tpm = reinterpret_cast<char *>(buf);
        o_data = reinterpret_cast<T *>(tpm + ta);
        return 0;
      }
    }
    return LBERR_OBJ_NOT_HAVE;
  }

  /**
   * @brief 单线程设置位置内存，用于写入方
   *
   * 在占位后，获取指定位置的内存用于写入。非线程安全。
   *
   * @param[out] o_data 输出参数，返回元素指针
   * @param[in] pos 要设置的位置（从1开始）
   * @param[in] data_size 数据大小（变长模式使用）
   * @return 0表示成功，负值表示失败
   */
  int32 set(T *&o_data, int32 pos, int32 data_size = 0) {
    if (unlikely(pos > i_num)) {
      return LBERR_ARGV_WRONG;
    }

    if (is_fixed == 1) {
      o_data = buf + (pos - 1);
      if (head->used < pos)
        head->used = pos;
      return 0;
    }

    int64 ta = addrs[pos - 1];
    if (ta >= 0) {
      char *tpm = reinterpret_cast<char *>(buf);
      o_data = reinterpret_cast<T *>(tpm + ta);
      return 0;
    }

    data_size = ALIGN_UP(data_size, 8);
    int64 rd = head->off;
    if (rd + data_size <= head->mem_size) {
      if (head->used < pos)
        head->used = pos;
      head->off += data_size;
      atomic_store64(&(addrs[pos - 1]), rd);
      char *tpm = reinterpret_cast<char *>(buf);
      o_data = reinterpret_cast<T *>(tpm + rd);
      return 0;
    }
    return LBERR_OBJ_IS_FULL;
  }

  /**
   * @brief 多线程设置位置内存，用于写入方
   *
   * 在占位后，获取指定位置的内存用于写入。线程安全，使用锁保护。
   *
   * @param[out] o_data 输出参数，返回元素指针
   * @param[in] pos 要设置的位置（从1开始）
   * @param[in] data_size 数据大小（变长模式使用）
   * @return 0表示成功，负值表示失败
   */
  int32 take(T *&o_data, int32 pos, int32 data_size = 0) {
    if (unlikely(pos > i_num)) {
      return LBERR_ARGV_WRONG;
    }

    if (is_fixed == 1) {
      o_data = buf + (pos - 1);
      head->mlock.lock();
      if (head->used < pos)
        head->used = pos;
      head->mlock.unlock();
      return 0;
    }

    head->mlock.lock();
    int64 ta = addrs[pos - 1];
    if (ta >= 0) {
      char *tpm = reinterpret_cast<char *>(buf);
      o_data = reinterpret_cast<T *>(tpm + ta);
      head->mlock.unlock();
      return 0;
    }

    data_size = ALIGN_UP(data_size, 8);
    int64 rd = head->off;
    if (rd + data_size <= head->mem_size) {
      head->off += data_size;
      atomic_store64(&(addrs[pos - 1]), rd);
      if (head->used < pos)
        head->used = pos;
      head->mlock.unlock();
      char *tpm = reinterpret_cast<char *>(buf);
      o_data = reinterpret_cast<T *>(tpm + rd);
      return 0;
    }
    head->mlock.unlock();
    return LBERR_OBJ_IS_FULL;
  }

  /**
   * @brief 获取当前已使用元素数量
   * @return 已使用元素数量
   */
  FORCE_INLINE int32 size() const { return (i_num > 0 ? head->used : 0); }

  /**
   * @brief 获取空闲元素数量
   * @return 空闲元素数量
   */
  FORCE_INLINE int32 free_num() const { return (i_num > 0 ? (i_num - head->used) : 0); }

  /**
   * @brief 获取总元素数量
   * @return 总元素数量
   */
  FORCE_INLINE int32 total_num() const { return i_num; }

  /**
   * @brief 检查共享内存是否存在
   * @return true表示共享内存已存在，false表示不存在
   */
  FORCE_INLINE bool is_shm_exist() const { return shm_exist == 1; }

  /**
   * @brief 初始化固定内存数组
   *
   * @param[in] shm_name 共享内存名称，NULL或空字符串表示不使用共享内存
   * @param[in] it_num 元素数量
   * @param[in] is_create 是否为创建模式，1表示创建，0表示打开
   * @param[in] mem_size
   * 内存大小，0表示固定大小模式（每项sizeof(T)），>0表示变长模式
   * @return 0表示成功，负值表示失败
   */
  int32 init(const char *shm_name, int32 it_num, int32 is_create, int64 mem_size = 0) {
    char tname[256];
    if (NULL != shm_name && shm_name[0] != '\0') {
      comm_utils::str_copy_format(tname, shm_name, sizeof(tname));
    } else {
      std::memset(tname, 0, sizeof(tname));
    }

    it_num = ALIGN_UP(it_num, 4);

    int32 fix_it = 0;
    int32 in_shm = 0;
    int64 bs = sizeof(vec_mem_head);

    if (mem_size == 0) {
      mem_size = sizeof(T) * it_num;
      bs += mem_size;
      fix_it = 1;
    } else {
      if (mem_size <= (int64)sizeof(T) * it_num * 2)
        return LBERR_ARGV_WRONG;

      bs += sizeof(int64) * it_num * 2;
      bs += mem_size;
      fix_it = 0;
    }

    void *pshm = NULL;
    int32 shmisexist = 0;
    int32 ishuge = 0;
    if (bs >= HUGE_PAGE_SIZE) {
      ishuge = 1;
    }

    if (NULL != shm_name && shm_name[0] != '\0') {
      shmisexist = comm_utils::map_shm(pshm, tname, bs, ishuge, is_create);
      if (shmisexist < 0) {
        std::cerr << "open vector mmap error,name=" << tname << ",size=" << bs << ",hugepage=" << ishuge
                  << ",ret=" << shmisexist << std::endl;
        // assert(0);
        return LBERR_SHM_MAP_FAIL;
      }
      in_shm = 1;
    } else {
      pshm = comm_utils::aligned_malloc(bs, PAGE_SIZE);
      if (NULL == pshm)
        return LBERR_MEM_ALLOC_FAIL;
      shmisexist = 0;
      is_create = 1;
      in_shm = 0;
    }
    char *tpm = static_cast<char *>(pshm);

    is_fixed = fix_it;
    i_num = it_num;
    head = reinterpret_cast<vec_mem_head *>(tpm);
    tpm += sizeof(vec_mem_head);
    if (fix_it == 0) {
      addrs = reinterpret_cast<int64 *>(tpm);
      tpm += sizeof(int64) * it_num;
    }
    buf = reinterpret_cast<T *>(tpm);

    if (is_create == 1) {
      if (shmisexist == 0 || head->i_num != it_num || head->mem_size != mem_size) {
        head->used = 0;
        head->i_num = 0;
        head->mlock.init();
        head->is_fixed = fix_it;
        head->in_shm = in_shm;
        head->off = 0;
        head->mem_size = mem_size;
        // std::memcpy(head->shm_name,tname,sizeof(head->shm_name));

        init_mem();
        head->i_num = it_num;

        shm_exist = 0;
      } else {
        // 故障重启
        assert(head->used <= head->i_num);
        recove_mem();

        shm_exist = 1;
      }
    } else {
      while (head->i_num == 0) {
        comm_utils::sleep_us(100);
      }
      if (head->i_num != it_num || head->mem_size != mem_size)
        return LBERR_ARGV_WRONG;
    }
    return 0;
  }

  /**
   * @brief 关闭数组，释放资源
   *
   * 确保无其他线程使用时调用。
   *
   * @param[in] is_create 是否为创建者，1表示创建者负责释放资源
   */
  void close(int32 is_create) {
    if (NULL == head)
      return;

    if (is_create == 1) {
      void *tpshm = reinterpret_cast<void *>(head);
      if (head->in_shm == 0) {
        head->mlock.init();
        comm_utils::aligned_free(tpshm);
      } else {
        head->mlock.init();
        head->used = 0;
        head->i_num = 0;
        init_mem();
        int64 bs = sizeof(vec_mem_head);
        if (is_fixed == 1) {
          bs += head->mem_size;
        } else {
          bs += sizeof(int64) * head->i_num * 2;
          bs += head->mem_size;
        }
        comm_utils::unmap_shm(tpshm, bs);
      }
    }

    i_num = 0;
    head = NULL;
    buf = NULL;
    addrs = NULL;
  }

  /**
   * @brief 默认构造函数
   */
  vec_fix_mem() : is_fixed(0), shm_exist(0), i_num(0), head(NULL), buf(NULL), addrs(NULL){};

  /**
   * @brief 析构函数
   */
  ~vec_fix_mem(){};

private:
  /**
   * @brief 初始化内存
   *
   * 清零数据内存，初始化地址数组（变长模式）
   */
  void init_mem() {
    std::memset(reinterpret_cast<void *>(buf), 0, head->mem_size);
    if (is_fixed == 0) {
      for (int32 i = 0; i < i_num; i++) {
        addrs[i] = -1;
      }
    }
  }
  /**
   * @brief 恢复内存状态
   *
   * 在故障重启后恢复内存状态，处理未完成的写入操作
   */
  void recove_mem() {
    if (is_fixed == 0) {
      int32 tpos = head->used;
      if (tpos >= 0) {
        int64 rd = addrs[tpos];
        if (rd >= 0) {
          int32 ts = head->off - rd;
          if (ts > 0) {
            std::memset(reinterpret_cast<char *>(buf) + rd, 0, ts);
          }
          head->off = rd;
          addrs[tpos] = -1;
        }
      }
    }
  }
};

} // namespace lb_common

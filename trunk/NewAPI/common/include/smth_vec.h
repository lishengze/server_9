#pragma once

/**
 * @file smth_vec.h
 * @brief 可扩展数组模块
 *
 * 这是一个支持多线程的可扩展大小的数组。
 *
 * 限制：
 *    1. 数组项必须为POD类型
 *    2. 数组项必须保存索引位置
 *    3. 最大数量限制
 *
 * 特征：
 *    1. 使用读写自旋锁包括读写，并保证同时只有一个写入
 *    2. 扩展新申请，复制原数据到新内存，并使用额外的扩展锁，保证只有一个扩展
 *    3. 索引位置 = 预定义前缀量+数组位置
 *
 */

#include "comm_sys.h"
// #include "matomic.h"
#include "mlock.h"
#include "mutils.h"

namespace lb_common {

/**
 * @brief 平滑扩展数组类
 *
 * 支持多线程的可扩展大小的数组，使用读写锁保证线程安全。
 * 支持动态扩容，扩容时使用SIMD优化内存复制。
 *
 * 限制：
 *    1. 数组项必须为POD类型
 *    2. 数组项必须保存索引位置
 *    3. 最大数量限制
 *
 * 特征：
 *    1. 使用读写自旋锁包括读写，并保证同时只有一个写入
 *    2. 扩展新申请，复制原数据到新内存，并使用额外的扩展锁，保证只有一个扩展
 *    3. 索引位置 = 预定义前缀量+数组位置
 *
 * @tparam T 元素类型，必须为POD类型且实现save_no方法
 *
 */
template <class T> class smth_vec {
private:
  /** @brief 读写锁，保护数组访问 */
  atomic_rwlock rwlock;
  /** @brief 索引前缀偏移量 */
  int32 pre_no;
  /** @brief 当前已使用元素数量 */
  int32 used;
  /** @brief 当前数组容量 */
  int32 num;
  /** @brief 最大容量限制，0表示无限制 */
  int32 max_num;
  /** @brief 数组指针 */
  T *vec;
  /** @brief 扩展锁，保证只有一个线程执行扩展 */
  cmutex exp_lock;

  /**
   * @brief 扩展数组容量
   *
   * 申请新的内存空间，复制原数据，返回新的数组指针。
   * 使用SIMD优化内存复制。
   *
   * @param[out] o_vals 输出参数，新的数组指针
   * @param[in] ts 目标容量
   * @return 实际分配的容量，负值表示失败
   */
  int32 expand(T *&o_vals, int32 ts) {
    if (ts < 8) {
      ts = 8;
    }
    if (unlikely(ts <= num)) {
      return 0;
    }
    if (unlikely(ts > max_num && max_num > 0)) {
      return LBERR_OBJ_NUM_LIMIT;
    }

    void *tpm = comm_utils::aligned_malloc(ts * sizeof(T), CACHE_ALIGN_SIZE);
    if (NULL == tpm)
      return LBERR_MEM_ALLOC_FAIL;

    int32 old_size = num * sizeof(T);
    void *tpold = reinterpret_cast<void *>(vec);
    comm_utils::simd_copy((uint8 *)(tpm), static_cast<const uint8 *>(tpold), old_size);

    o_vals = reinterpret_cast<T *>(tpm);
    return ts;
  }

public:
  /**
   * @brief 读取指定位置的元素
   *
   * 使用读锁保护，线程安全。
   *
   * @param[out] o_val 输出参数，读取的元素
   * @param[in] index 要读取的索引（包含pre_no前缀）
   * @return 0表示成功，LBERR_OBJ_NOT_HAVE表示索引无效
   */
  inline int32 read(T &o_val, int32 index) {
    int32 ret = LBERR_OBJ_NOT_HAVE;
    index -= pre_no;
    rwlock.read_lock();
    if (likely(index >= 0 && index < used)) {
      o_val = (vec[index]);
      ret = 0;
    }
    rwlock.read_unlock();
    return ret;
  }
  /**
   * @brief 写入元素到数组
   *
   * 如果数组已满，自动扩展容量（2倍扩展）。
   * 使用写锁保证线程安全。
   *
   * @param[in] val 要写入的元素
   * @return 元素所在的索引位置（包含pre_no前缀）
   */
  int32 write(T &val) {
    int32 tu;
    do {
      rwlock.write_lock();
      tu = used;
      if (likely(tu < num)) {
        val.save_no(tu + pre_no);
        vec[tu] = val;
        used++;
        rwlock.write_unlock();
        return tu + pre_no;
      } else {
        tu = num;
      }
      rwlock.write_unlock();

      T *tnew_vals = NULL;
      exp_lock.lock();
      int32 ret = expand(tnew_vals, tu * 2);
      if (unlikely(ret <= 0)) {
        exp_lock.unlock();

        if (ret == 0)
          continue;
        return ret;
      }

      rwlock.write_lock();
      T *tfree_vals = vec;
      vec = tnew_vals;
      used = num;
      num = ret;
      tu = used;
      val.save_no(tu + pre_no);
      vec[tu] = val;
      used++;
      rwlock.write_unlock();

      if (NULL != tfree_vals) {
        comm_utils::aligned_free(reinterpret_cast<void *>(tfree_vals));
      }
      exp_lock.unlock();
      return tu + pre_no;
    } while (true);
  }

  /**
   * @brief 按指定索引号占位恢复（用于 SHM 崩溃恢复路径）
   * @param tno 目标索引（含 pre_no 前缀）
   * @param val 要写入的元素
   * @return 0 表示成功（返回 tno），负值表示失败
   * @note 与 write() 不同：① 不调 val.save_no()（由调用方保证对象已就绪）；
   *       ② 允许在 used 之后的位置插入（不重新分配新号，保持原 stragy_id 等）
   */
  int32 recover_reserve(int32 tno, T &val) {
    int32 pos = tno - pre_no;
    if (unlikely(pos < 0)) {
      return LBERR_ARGV_WRONG;
    }

    exp_lock.lock();
    rwlock.write_lock();
    if (likely(pos < num)) {
      vec[pos] = val;
      if (unlikely(pos >= used)) {
        used = pos + 1;
      }
      rwlock.write_unlock();
      exp_lock.unlock();
      return tno;
    }
    rwlock.write_unlock();

    T *tnew_vals = NULL;
    int32 tts = pos + 1;
    int32 ret = expand(tnew_vals, tts);
    if (unlikely(ret < 0)) {
      exp_lock.unlock();
      return ret;
    }

    rwlock.write_lock();
    T *tfree_vals = vec;
    vec = tnew_vals;
    num = ret;
    vec[pos] = val;
    if (unlikely(pos >= used)) {
      used = pos + 1;
    }
    rwlock.write_unlock();

    if (NULL != tfree_vals) {
      comm_utils::aligned_free(reinterpret_cast<void *>(tfree_vals));
    }
    exp_lock.unlock();
    return tno;
  }

  inline int32 size() const { return used; }

  inline int32 get_preno() const { return pre_no; }

  /**
   * @brief 初始化平滑扩展数组
   *
   * @param[in] max_size 最大容量限制，0表示无限制
   * @param[in] init_num 初始容量
   * @param[in] tpre_no 索引前缀偏移量
   * @return 0表示成功，负值表示失败
   */
  int32 init(int32 max_size, int32 init_num, int32 tpre_no) {
    if (tpre_no > 0 && (tpre_no < max_size || max_size <= 0))
      return LBERR_ARGV_WRONG;

    exp_lock.lock();
    rwlock.init();
    rwlock.write_lock();
    pre_no = tpre_no;
    used = 0;
    num = 0;
    max_num = max_size;

    if (init_num <= 0) {
      rwlock.write_unlock();
      exp_lock.unlock();
      return 0;
    }

    T *tnew_vals = NULL;
    init_num = ALIGN_UP(init_num, 8);
    int32 ret = expand(tnew_vals, init_num);
    if (ret < 0) {
      rwlock.write_unlock();
      exp_lock.unlock();
      return ret;
    }

    used = 0;
    num = ret;
    vec = tnew_vals;
    rwlock.write_unlock();
    exp_lock.unlock();
    return 0;
  }

  /**
   * @brief 关闭数组，释放资源
   *
   * 释放数组占用的内存，重置所有状态。
   */
  void close() {
    exp_lock.lock();
    rwlock.write_lock();
    num = 0;
    used = 0;
    if (vec != NULL) {
      comm_utils::aligned_free(reinterpret_cast<void *>(vec));
      vec = NULL;
    }
    rwlock.write_unlock();
    exp_lock.unlock();
  }

  /**
   * @brief 默认构造函数
   */
  smth_vec() : pre_no(0), used(0), num(0), vec(NULL){};

  /**
   * @brief 析构函数
   *
   * 自动调用close()释放资源。
   */
  ~smth_vec() { close(); }

  /**
   * @brief 禁止拷贝构造
   */
  smth_vec(const smth_vec &src) = delete;

  /**
   * @brief 禁止赋值操作
   */
  smth_vec &operator=(const smth_vec &src) = delete;
};

} // namespace lb_common

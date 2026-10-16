#pragma once

/**
 * @file sort_comm.h
 * @brief 排序算法模块
 *
 * 提供通用排序功能，基于希尔排序算法实现。
 */

#include "comm_sys.h"

#include <vector>

namespace lb_common {

/**
 * @brief 32位整数比较器
 *
 * 用于排序时的元素比较
 */
struct sort_cmp_int32 {
  /**
	 * @brief 比较两个32位整数
	 * @param[in] cmped 待比较的数
	 * @param[in] cmpto 比较的目标数
	 * @return 三态比较结果：负数=cmped < cmpto；0=相等；正数=cmped > cmpto
	 *
	 * @note P2-04: 原版 cmped - cmpto 在 INT_MIN 等极值场景下有符号溢出。
	 */
  int32 operator()(int32 cmped, int32 cmpto) const {
    return (cmped > cmpto) - (cmped < cmpto);
  }
};

/**
 * @brief 64位整数比较器
 *
 * 用于排序时的元素比较
 */
struct sort_cmp_int64 {
  /**
	 * @brief 比较两个64位整数
	 * @param[in] cmped 待比较的数
	 * @param[in] cmpto 比较的目标数
	 * @return 三态比较结果
	 *
	 * @note P2-04: 原版 cmped - cmpto 在 INT64_MIN 等极值场景下有符号溢出。
	 */
  int64 operator()(int64 cmped, int64 cmpto) const {
    return (cmped > cmpto) - (cmped < cmpto);
  }
};

/**
 * @brief 键值对结构模板
 *
 * @tparam K 键类型
 * @tparam T 值类型
 */
template <class K, class T> struct sort_cmp_pair {
  K key;        ///< 键
  const T *obj; ///< 值指针
};

/**
 * @brief 希尔排序函数
 *
 * 使用希尔排序算法对向量进行排序。
 * 希尔排序是插入排序的改进版本，通过使用递减的间隔来排序元素。
 *
 * @tparam T 元素类型
 * @tparam CMP 比较器类型
 * @param[in] arr 待排序的向量（引用）
 *
 * @note 该函数会直接修改输入向量
 * @note 时间复杂度: O(n log n)，空间复杂度: O(1)
 *
 * @example
 * @code
 * std::vector<int32> nums = {5, 2, 8, 1, 9};
 * lb_sort_func<int32, sort_cmp_int32>(nums);
 * @endcode
 */
template <class T, class CMP> void lb_sort_func(std::vector<T> &arr) {
  // 使用 size_t 避免 n > INT32_MAX 时 int32 截断；t >= 0 比较依赖有符号，
  // 故 t/j 保留为带符号 long long，仅 n/d 用 size_t。
  size_t n = arr.size();
  if (n < 2)
    return;
  size_t d = n / 2; //确定gap,希尔增量
  size_t i, j;
  long long t;
  T tc;
  while (d > 0) {
    for (i = d; i < n; i++) {
      j = i;
      t = (long long)j - (long long)d;
      tc = arr[i];
      //从gap位置开始，对每项i，比较该项之前的按gap跳的每项t
      //  若项t大于项i，则项t后移一个gap到项j
      //  若项t小于等项i，则将项i插入此位置t。
      //遍历下一项i，
      //从而项i之前的项都小于等于项i
      while (t >= 0) {
        if (CMP()(arr[t], tc) > 0) {
          arr[j] = arr[t];
          j = (size_t)t;
          t = (long long)j - (long long)d;
        } else {
          break;
        }
      }
      arr[j] = tc;
    }
    d = d / 2; //gap 减半
  }
};

} // namespace lb_common

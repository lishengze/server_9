#pragma once

/**
 * @file que_comm.h
 * @brief 队列通信公共定义模块
 *
 * 提供队列相关的公共定义、常量和数据结构
 * 包括队列项结构体、对齐宏定义等
 */

namespace lb_common {

/**
 * @brief 用户数据长度定义
 *
 * 定义队列中每个用户数据的最大长度为48字节
 */
#define QUE_USER_DATA_LEN 48

/**
 * @brief 队列循环忙等待计数
 *
 * 定义队列操作时的忙等待循环次数上限
 */
#define QUE_LOOP_BUSY_COUNT 50000

#ifndef LBQUE_ALIGN_SIMD

/**
 * @brief 队列固定大小项结构体（8字节对齐）
 *
 * 用于存储队列中的单个数据项，提供8字节对齐以保证性能
 *
 * @tparam V 数据类型
 */
template <class V> struct que_fixed_it {
  V data; ///< 数据内容
} __attribute__((aligned(8)));

#else

/**
 * @brief 队列固定大小项结构体（SIMD对齐）
 *
 * 用于存储队列中的单个数据项，提供SIMD对齐以优化向量操作
 *
 * @tparam V 数据类型
 */
template <class V> struct que_fixed_it {
  V data; ///< 数据内容
} __attribute__((aligned(SIMD_ALIGN_SIZE)));
;

#endif

} // namespace lb_common

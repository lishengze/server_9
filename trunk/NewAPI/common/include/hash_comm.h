#pragma once

/**
 * @file hash_comm.h
 * @brief 哈希函数模块
 *
 * 提供高效的字符串哈希函数，用于哈希表等数据结构。
 *
 * @note 所有哈希函数都是线程安全的（无状态）
 */

#include "comm_sys.h"

#include <cstring>

namespace lb_common {

/**
 * @brief 可变长度字符串哈希
 *
 * 适用于长度较小的字符串（如32字节左右）
 * 使用黄金比例常数保证良好的哈希分布
 *
 * @param[in] key 字符串指针
 * @return 哈希值
 *
 * @note 时间复杂度: O(n)，n为字符串长度
 * @note 空间复杂度: O(1)
 * @note 适用于作为哈希表的哈希函数
 */
static inline uint64 hash_str(const char *key) {
  uint64 h = 0;
  while (*key != '\0') {
    h = (h << 5) - h + *key; // 31*h + *key;
    // h ^= (h>>32);
    key++;
  }

  // 64位系统的黄金比例常数
  const uint64_t golden = 0x61C8864680B583EBull; // 0x9e3779b97f4a7c15ull;
  __uint128_t prod = ((__uint128_t)h) * golden;
  return (uint64_t)(prod >> 64) ^ (uint64_t)prod;
}

/**
 * @brief 固定长度字符串哈希（8字节对齐）
 *
 * 适用于固定长度N为8的倍数的字符串，并且大小不超过64字节
 * 使用8字节对齐读取提高效率
 *
 * @tparam N 字符串长度（必须是8的倍数）
 * @param[in] key 字符串指针
 * @return 哈希值
 *
 * @note 使用 memcpy 安全读取，避免未对齐访问问题
 * @note 编译器会优化为单条汇编指令
 * @note 适用于定长键的场景，如MAC地址、UUID等
 */
template <int N> static inline uint64 hash_fm8(const char *key) {
  uint64 h = 0;
  int32 i = 0;
  while (i < N) {
    // 安全读取，防止输入参数不对齐4字节
    // 编译器会优化为单条汇编指令
    uint64 word;
    std::memcpy(&word, key + i, 8); // 安全读取
    h = (h << 7) - h + word;        // 127*h + word;
    h ^= (h >> 32);
    i += 8;
  }

  // 64位系统的黄金比例常数
  const uint64_t golden = 0x61C8864680B583EBull; // 0x9e3779b97f4a7c15ull;
  __uint128_t prod = ((__uint128_t)h) * golden;
  return (uint64_t)(prod >> 64) ^ (uint64_t)prod;
}

/**
 * @brief 固定长度字符串哈希（4字节对齐）
 *
 * 适用于固定长度N为8的倍数的字符串，并且大小不超过64字节
 * 使用4字节对齐读取
 *
 * @tparam N 字符串长度（必须是8的倍数）
 * @param[in] key 字符串指针
 * @return 哈希值
 *
 * @note 使用 memcpy 安全读取，避免未对齐访问问题
 * @note 编译器会优化为单条汇编指令
 * @note 适用于定长键且4字节对齐足够的场景
 */
template <int N> static inline uint64 hash_fm4(const char *key) {
  uint64 h = 0;
  int32 i = 0;
  while (i < N) {
    // 安全读取，防止输入参数不对齐4字节
    // 编译器会优化为单条汇编指令
    uint32 word;
    std::memcpy(&word, key + i, 4); // 安全读取
    h = (h << 7) - h + word;        // 127*h + word;
    h ^= (h >> 32);
    i += 4;
  }

  // 64位系统的黄金比例常数
  const uint64_t golden = 0x61C8864680B583EBull; // 0x9e3779b97f4a7c15ull;
  __uint128_t prod = ((__uint128_t)h) * golden;
  return (uint64_t)(prod >> 64) ^ (uint64_t)prod;
}

} // namespace lb_common

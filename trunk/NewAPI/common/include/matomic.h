#pragma once

/**
 * @file matomic.h
 * @brief 原子操作与内存屏障模块
 *
 * 本模块提供跨平台的原子操作和内存屏障支持，用于实现无锁数据结构和多线程同步。
 * 基于 GCC/Clang 的 __atomic 内置函数实现。
 */

#include "comm_sys.h"
// #include <atomic>

namespace lb_common {

/**
 * @brief 编译器内存屏障
 *
 * 阻止编译器对屏障前后的内存操作重排，但不影响CPU指令重排。
 * 特性：
 * - 屏障后的操作不会被编译器重排到屏障前
 * - 屏障前的数据操作可能越过屏障（CPU层面仍可重排）
 * - 无CPU指令开销，仅编译期约束
 *
 * @note 不具备CPU层面的内存可见性保证
 */
#define atomic_compiler_fence() __asm__ __volatile__("" : : : "memory")

// --------------------------
// CPU内存屏障（基于__atomic_thread_fence，阻止CPU+编译器重排）
// --------------------------

/**
 * @brief 读内存屏障（Acquire屏障）
 *
 * 特性：
 * - 屏障前的读操作不会越过屏障，保证读操作的可见性
 * - 屏障后的操作（读/写）可能被CPU重排到屏障前（但读操作本身不跨屏障）
 * - 适用场景：消费者读取共享数据前（如读取队列数据、标志位）
 *
 * @note 确保在屏障之后的操作能看到屏障之前的所有写操作
 */
#define atomic_read_fence() __atomic_thread_fence(__ATOMIC_ACQUIRE)

/**
 * @brief 写内存屏障（Release屏障）
 *
 * 特性：
 * - 屏障前的数据操作（读/写）不会越过屏障，保证写操作对其他线程可见
 * - 屏障后的操作可能被CPU重排到屏障前
 * - 适用场景：生产者写入共享数据后（如写入队列数据、设置标志位）
 *
 * @note 确保屏障之前的所有写操作对其他线程可见
 */
#define atomic_write_fence() __atomic_thread_fence(__ATOMIC_RELEASE)

/**
 * @brief 通用内存屏障（Acq_Rel屏障）
 *
 * 特性：
 * - 结合Acquire+Release语义，阻止屏障前后的读/写操作跨屏障重排
 * - 适用场景：无锁数据结构的核心操作（如CAS成功后、交换操作）
 *
 * @note 最常用的内存屏障，提供较好的性能和正确性平衡
 */
#define atomic_memory_fence() __atomic_thread_fence(__ATOMIC_ACQ_REL)

/**
 * @brief 强顺序内存屏障（Seq_Cst屏障）
 *
 * 特性：
 * - 最强的内存屏障，锁CPU缓存行/内存总线，触发缓存一致性同步
 * - 保证所有线程看到的内存操作顺序完全一致
 *
 * @note 性能开销最大，仅在需要严格顺序保证时使用
 */
#define atomic_seq_fence() __atomic_thread_fence(__ATOMIC_SEQ_CST)

// 原子Load（读取）；直接访问接近于load(__ATOMIC_RELAXED)

/**
 * @brief 原子读取32位整数
 *
 * @param[in] ptr 指向待读取数据的指针
 * @return 读取的值
 *
 * @note 使用 ACQUIRE 语义，保证读取后的操作不会被重排到读取之前
 */
static inline int32 atomic_load32(const int32 *ptr) { return __atomic_load_n(ptr, __ATOMIC_ACQUIRE); }

/**
 * @brief 原子读取64位整数
 *
 * @param[in] ptr 指向待读取数据的指针
 * @return 读取的值
 *
 * @note 使用 ACQUIRE 语义，保证读取后的操作不会被重排到读取之前
 */
static inline int64 atomic_load64(const int64 *ptr) { return __atomic_load_n(ptr, __ATOMIC_ACQUIRE); }

/**
 * @brief 原子读取指针
 *
 * @param[in] ptr 指向待读取指针的指针（应为 T* const*，与 atomic_store_ptr 等保持一致）
 * @return 读取的指针值
 *
 * @note 使用 ACQUIRE 语义，保证读取后的操作不会被重排到读取之前
 */
template <typename T> inline T *atomic_load_ptr(T *const *ptr) { return __atomic_load_n(ptr, __ATOMIC_ACQUIRE); }

/**
 * @brief 原子读取16位整数
 *
 * @param[in] ptr 指向待读取数据的指针
 * @return 读取的值
 *
 * @note 使用 ACQUIRE 语义，保证读取后的操作不会被重排到读取之前
 */
static inline int16 atomic_load16(const int16 *ptr) { return __atomic_load_n(ptr, __ATOMIC_ACQUIRE); }

// 原子Store（写入）：使用 RELEASE 内存序，保证 store 前的所有写对其他线程可见

/**
 * @brief 原子写入32位整数
 *
 * @param[out] ptr 指向待写入数据的指针
 * @param[in] val 要写入的值
 *
 * @note 使用 RELEASE 语义，保证写入前的操作不会被重排到写入之后
 */
static inline void atomic_store32(int32 *ptr, int32 val) { __atomic_store_n(ptr, val, __ATOMIC_RELEASE); }

/**
 * @brief 原子写入64位整数
 *
 * @param[out] ptr 指向待写入数据的指针
 * @param[in] val 要写入的值
 *
 * @note 使用 RELEASE 语义，保证写入前的操作不会被重排到写入之后
 */
static inline void atomic_store64(int64 *ptr, int64 val) { __atomic_store_n(ptr, val, __ATOMIC_RELEASE); }

/**
 * @brief 原子写入指针
 *
 * @param[out] ptr 指向待写入指针的指针
 * @param[in] val 要写入的指针值
 *
 * @note 使用 RELEASE 语义，保证写入前的操作不会被重排到写入之后
 */
template <typename T> inline void atomic_store_ptr(T **ptr, T *val) { __atomic_store_n(ptr, val, __ATOMIC_RELEASE); }

/**
 * @brief 原子写入16位整数
 *
 * @param[out] ptr 指向待写入数据的指针
 * @param[in] val 要写入的值
 *
 * @note 使用 RELEASE 语义，保证写入前的操作不会被重排到写入之后
 */
static inline void atomic_store16(int16 *ptr, int16 val) { __atomic_store_n(ptr, val, __ATOMIC_RELEASE); }

/**
 * @brief 原子交换32位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] val 要交换的值
 * @return 交换前的旧值
 *
 * @note 使用 ACQ_REL 内存序：写侧保证之前的写对其他线程可见，
 *       读侧保证读到建立此值的先前写入
 */
static inline int32 atomic_exchange32(int32 *ptr, int32 val) { return __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL); }

/**
 * @brief 原子交换64位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] val 要交换的值
 * @return 交换前的旧值
 *
 * @note 使用 ACQ_REL 内存序：写侧保证之前的写对其他线程可见，
 *       读侧保证读到建立此值的先前写入
 */
static inline int64 atomic_exchange64(int64 *ptr, int64 val) { return __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL); }

/**
 * @brief 原子交换指针
 *
 * @param[out] ptr 指向指针的指针
 * @param[in] val 要交换的指针值
 * @return 交换前的旧指针值
 *
 * @note 使用 ACQ_REL 内存序
 */
template <typename T> inline T *atomic_exchange_ptr(T **ptr, T *val) {
  return __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL);
}

/**
 * @brief 原子交换16位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] val 要交换的值
 * @return 交换前的旧值
 *
 * @note 使用 ACQ_REL 内存序
 */
static inline int16 atomic_exchange16(int16 *ptr, int16 val) { return __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL); }

// 原子CAS（比较并交换）
//
// 原因：本库的 CAS 在两类场景中使用：
//   1) "发布数据 + 推进位置" 模式（如 que_mth_buf 的 wrpos）— 需要 RELEASE 让之前的数据写可见
//   2) "锁获取"模式（如 mlock/state_machine）— 需要 ACQUIRE 看到前持有者的写
// ACQ_REL 同时覆盖两种语义，是唯一安全的统一选择。
// 失败路径用 ACQUIRE 读回最新值即可（无需 RELEASE，因为我们没写入）。
// GCC 文档：第一个 mem order 是 success，第二个是 failure。

/**
 * @brief 强CAS（比较并交换）32位整数
 *
 * 如果 *ptr 等于 *io_expect，则将 *ptr 设置为 dst_val
 *
 * @param[in,out] ptr 指向待比较交换的内存地址
 * @param[in,out] io_expect 期望值指针，如果成功则更新为实际值
 * @param[in] dst_val 要设置的新值
 * @return true=交换成功，false=交换失败（值不匹配或弱 CAS 虚假失败）
 *
 * @note 强 CAS 不会发生"虚假失败"（即使值匹配），但仍可能因值不匹配返回 false
 * @note **必须包在循环中使用**（do { ... } while (!cas(...))）
 * @note 成功用 ACQ_REL、失败用 ACQUIRE：覆盖"发布数据"和"取锁"两种模式
 */
static inline bool atomic_cas32(int32 *ptr, int32 *io_expect, int32 dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

/**
 * @brief 弱CAS（比较并交换）32位整数
 *
 * 如果 *ptr 等于 *io_expect，则将 *ptr 设置为 dst_val
 *
 * @param[in,out] ptr 指向待比较交换的内存地址
 * @param[in,out] io_expect 期望值指针
 * @param[in] dst_val 要设置的新值
 * @return true=交换成功，false=交换失败（含虚假失败，必须循环重试）
 *
 * @note 弱 CAS **可能虚假失败**（即使值相等也可能返回 false），必须用在循环中
 * @note 性能通常优于强 CAS，适合自旋循环热点
 */
static inline bool atomic_cas32_weak(int32 *ptr, int32 *io_expect, int32 dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

/**
 * @brief 强CAS（比较并交换）64位整数
 *
 * @note 详见 atomic_cas32，语义相同。
 */
static inline bool atomic_cas64(int64 *ptr, int64 *io_expect, int64 dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

/**
 * @brief 弱CAS（比较并交换）64位整数
 *
 * @note 详见 atomic_cas32_weak，语义相同。
 */
static inline bool atomic_cas64_weak(int64 *ptr, int64 *io_expect, int64 dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

/**
 * @brief 强CAS（比较并交换）指针
 *
 * @note 详见 atomic_cas32，语义相同。
 */
template <typename T> inline bool atomic_cas_ptr(T **ptr, T **io_expect, T *dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

/**
 * @brief 弱CAS（比较并交换）指针
 *
 * @note 详见 atomic_cas32_weak，语义相同。
 */
template <typename T> inline bool atomic_cas_ptr_weak(T **ptr, T **io_expect, T *dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

/**
 * @brief 强CAS（比较并交换）16位整数
 *
 * @note 详见 atomic_cas32，语义相同。
 */
static inline bool atomic_cas16(int16 *ptr, int16 *io_expect, int16 dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

/**
 * @brief 弱CAS（比较并交换）16位整数
 *
 * @note 详见 atomic_cas32_weak，语义相同。
 */
static inline bool atomic_cas16_weak(int16 *ptr, int16 *io_expect, int16 dst_val) {
  return __atomic_compare_exchange_n(ptr, io_expect, dst_val, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

// 原子Fetch_Add（自增），返回操作前的旧值

/**
 * @brief 原子自增32位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] add_val 要增加的值
 * @return 自增前的旧值
 *
 * @note 使用 ACQ_REL 内存序：写侧保证之前的写对其他线程可见，
 *       读侧保证读到建立此值的先前写入
 */
static inline int32 atomic_fetch_add32(int32 *ptr, int32 add_val) {
  return __atomic_fetch_add(ptr, add_val, __ATOMIC_ACQ_REL);
}

/**
 * @brief 原子自增64位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] add_val 要增加的值
 * @return 自增前的旧值
 *
 * @note 使用 ACQ_REL 内存序
 */
static inline int64 atomic_fetch_add64(int64 *ptr, int64 add_val) {
  return __atomic_fetch_add(ptr, add_val, __ATOMIC_ACQ_REL);
}

/**
 * @brief 原子自增16位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] add_val 要增加的值
 * @return 自增前的旧值
 *
 * @note 使用 ACQ_REL 内存序
 */
static inline int16 atomic_fetch_add16(int16 *ptr, int16 add_val) {
  return __atomic_fetch_add(ptr, add_val, __ATOMIC_ACQ_REL);
}

// 原子Fetch_Sub（自减），返回操作前的旧值

/**
 * @brief 原子自减32位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] add_val 要减少的值
 * @return 自减前的旧值
 *
 * @note 使用 ACQ_REL 内存序
 */
static inline int32 atomic_fetch_sub32(int32 *ptr, int32 add_val) {
  return __atomic_fetch_sub(ptr, add_val, __ATOMIC_ACQ_REL);
}

/**
 * @brief 原子自减64位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] add_val 要减少的值
 * @return 自减前的旧值
 *
 * @note 使用 ACQ_REL 内存序
 */
static inline int64 atomic_fetch_sub64(int64 *ptr, int64 add_val) {
  return __atomic_fetch_sub(ptr, add_val, __ATOMIC_ACQ_REL);
}

/**
 * @brief 原子自减16位整数
 *
 * @param[out] ptr 指向数据的指针
 * @param[in] add_val 要减少的值
 * @return 自减前的旧值
 *
 * @note 使用 ACQ_REL 内存序
 */
static inline int16 atomic_fetch_sub16(int16 *ptr, int16 add_val) {
  return __atomic_fetch_sub(ptr, add_val, __ATOMIC_ACQ_REL);
}

// --------------------------
// 原子标志操作（test_and_set / clear）
// --------------------------

/**
 * @brief 原子设置标志位（test-and-set）
 *
 * 原子地将 *ptr 写入非零值（旧值通常为 1），返回写入前的值。
 * 与 POSIX pthread_once 或一次性初始化场景配合使用。
 *
 * @param[out] ptr 指向标志位的指针（建议使用 unsigned char/uint8_t）
 * @return 设置前的值（0=之前未设置，非0=已设置）
 *
 * @note 使用 ACQ_REL 内存序：
 *       写入前返回 0 表示首次设置，后续写入应视为"已初始化"分支
 * @note 配套使用 atomic_clear() 重置标志
 */
static inline bool atomic_test_and_set(unsigned char *ptr) {
  return __atomic_test_and_set(ptr, __ATOMIC_ACQ_REL) != 0;
}

/**
 * @brief 原子清除标志位
 *
 * 原子地将 *ptr 写入 0。
 *
 * @param[out] ptr 指向标志位的指针
 *
 * @note 使用 RELEASE 内存序，保证之前的写对其他线程可见
 */
static inline void atomic_clear(unsigned char *ptr) { __atomic_clear(ptr, __ATOMIC_RELEASE); }

} // namespace lb_common

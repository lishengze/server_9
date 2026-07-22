#pragma once

/**
 * @file comm_sys.h
 * @brief 公共系统定义模块
 * @details 提供基础类型定义、系统宏和跨平台支持。
 * 包括：
 * - 整数类型别名（int8/16/32/64, uint8/16/32/64）
 * - 布尔类型定义（BOOL, TRUE, FALSE）
 * - 内存对齐宏（ALIGN_UP, CACHE_ALIGN, SIMD_ALIGN）
 * - CPU 特性支持（预取、pause指令）
 * - 编译器优化提示（likely, unlikely, FORCE_INLINE）
 */

// #define NDEBUG // release 版本编译禁用assert

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace lb_common {

typedef std::int8_t int8;
typedef std::uint8_t uint8;

typedef std::int16_t int16;
typedef std::uint16_t uint16;

typedef std::int32_t int32;
typedef std::uint32_t uint32;

typedef std::int64_t int64;
typedef std::uint64_t uint64;

#ifndef NULL
// #   ifdef __cplusplus
#define NULL nullptr
// #   else
// #       define NULL        ((void*) 0)
// #   endif
#endif

#ifndef BOOL
#define BOOL bool
#endif

#ifndef TRUE
#define TRUE true
#endif

#ifndef FALSE
#define FALSE false
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef HUGE_PAGE_SIZE
#define HUGE_PAGE_SIZE 2097152
#endif

/**
 * @def CACHE_ALIGN_SIZE
 * @brief 缓存行对齐大小（字节）
 * @details 根据 CPU 架构自动选择：
 * - x86_64 / amd64: 64 字节
 * - aarch64 / ARMv8-A / ARMv9-A: 64 字节
 * - riscv64: 64 字节
 * @note 可通过 sysconf(_SC_LEVEL1_DCACHE_LINESIZE) 在运行时获取
 */
#ifndef CACHE_ALIGN_SIZE
#if defined(__x86_64__) || defined(__amd64__) || defined(__x86_64)
#define CACHE_ALIGN_SIZE 64 //__builtin_ia32_cacheline_size()

#elif defined(__aarch64__) || defined(__ARM_ARCH_8A__) || defined(__ARM_ARCH_9A__) // ARM CPU
#define CACHE_ALIGN_SIZE 64

#elif defined(__riscv) && (__riscv_xlen == 64)
#define CACHE_ALIGN_SIZE 64

#else
#error "unsupported CPU architecture"
#endif
#endif

/**
 * @def CACHE_ALIGN
 * @brief 缓存行对齐属性修饰符
 * @details 指示变量按 CACHE_ALIGN_SIZE 字节对齐，
 * 避免伪共享（false sharing）问题。
 */
#ifndef CACHE_ALIGN
// #define CACHE_ALIGN __attribute__((aligned(CACHE_ALIGN_SIZE)))
#define CACHE_ALIGN alignas(CACHE_ALIGN_SIZE)
#endif

/**
 * @def CACHE_PREFETCH_TMP
 * @brief 预取数据到 L1 缓存（临时）
 * @param[in] addr 要预取的内存地址
 * @param[in] is_w 是否为写入操作（非零表示写入预取）
 * @details 临时少量数据，可能很快被交换出缓存，适用于一次性访问。
 */
#define CACHE_PREFETCH_TMP(addr, is_w) __builtin_prefetch(addr, is_w, 0)

/**
 * @def CACHE_PREFETCH_L1
 * @brief 预取数据到较低级缓存（L1/L2）
 * @param[in] addr 要预取的内存地址
 * @param[in] is_w 是否为写入操作
 * @details 较短时间后可被替换，适用于少量重复访问的内存。
 */
#define CACHE_PREFETCH_L1(addr, is_w) __builtin_prefetch(addr, is_w, 1)

/**
 * @def CACHE_PREFETCH_L2
 * @brief 预取数据到中级缓存（L2/L3）
 * @param[in] addr 要预取的内存地址
 * @param[in] is_w 是否为写入操作
 * @details 中等时间内不会被替换，适用于多次重复访问的内存。
 */
#define CACHE_PREFETCH_L2(addr, is_w) __builtin_prefetch(addr, is_w, 2)

/**
 * @def CACHE_PREFETCH_L3
 * @brief 预取数据到高级缓存（L3）
 * @param[in] addr 要预取的内存地址
 * @param[in] is_w 是否为写入操作
 * @details 尽可能长时间不被替换，适用于频繁访问的热点数据。
 */
#define CACHE_PREFETCH_L3(addr, is_w) __builtin_prefetch(addr, is_w, 3)

/**
 * @def SIMD_ALIGN_SIZE
 * @brief SIMD 指令集对齐大小（字节）
 * @details 根据 CPU 架构自动选择：
 * - x86_64 / amd64: 32 字节（AVX 支持）
 * - aarch64 / ARMv8-A / ARMv9-A: 16 字节（NEON 支持）
 * - riscv64: 16 字节
 */
#ifndef SIMD_ALIGN_SIZE
#if defined(__x86_64__) || defined(__amd64__) || defined(__x86_64)
#define SIMD_ALIGN_SIZE 32

#elif defined(__aarch64__) || defined(__ARM_ARCH_8A__) || defined(__ARM_ARCH_9A__) // ARM CPU
#define SIMD_ALIGN_SIZE 16

#elif defined(__riscv) && (__riscv_xlen == 64)
#define SIMD_ALIGN_SIZE 16

#else
#error "unsupported CPU architecture"
#endif
#endif

/**
 * @def SIMD_ALIGN
 * @brief SIMD 对齐属性修饰符
 * @details 指示变量按 SIMD_ALIGN_SIZE 字节对齐，
 * 以满足 SIMD 指令（如 AVX、NEON）对内存对齐的要求。
 */
#ifndef SIMD_ALIGN
// #define SIMD_ALIGN __attribute__((aligned(SIMD_ALIGN_SIZE)))
#define SIMD_ALIGN alignas(SIMD_ALIGN_SIZE)
#endif

/**
 * @def ALIGN_UP
 * @brief 将 size 向上对齐到 align 的整数倍
 * @param[in] size  原始大小
 * @param[in] align 对齐边界（必须为 2 的幂）
 * @return 对齐后的大小
 */
#ifndef ALIGN_UP
#define ALIGN_UP(size, align) (((size) + (align) - 1) & (~((align) - 1)))
#endif

/**
 * @def ALIGN_OFF
 * @brief 计算 size 在 align 对齐下的偏移量
 * @param[in] size  原始大小
 * @param[in] align 对齐边界（必须为 2 的幂）
 * @return size & (align - 1)，即未对齐部分的偏移字节数
 */
#ifndef ALIGN_OFF
#define ALIGN_OFF(size, align) ((size) & ((align) - 1))
#endif

/**
 * @def IS_ALIGN
 * @brief 判断 size 是否为 align 的整数倍
 * @param[in] size  原始大小
 * @param[in] align 对齐边界（必须为 2 的幂）
 * @retval true  已对齐
 * @retval false 未对齐
 */
#ifndef IS_ALIGN
#define IS_ALIGN(size, align) (((size) & ((align) - 1)) == 0)
#endif

/**
 * @def CPU_PAUSE
 * @brief CPU 暂停/hint 指令，用于自旋等待优化
 * @details 根据不同架构使用不同指令：
 * - x86_64: PAUSE (__builtin_ia32_pause)
 * - aarch64: YIELD
 * - riscv64: WFI
 * @warning 在自旋锁循环中使用可降低功耗并提高 SMT 性能
 */
#ifndef CPU_PAUSE
#if defined(__x86_64__) || defined(__amd64__) || defined(__x86_64)
#define CPU_PAUSE() __builtin_ia32_pause()

#elif defined(__aarch64__) || defined(__ARM_ARCH_8A__) || defined(__ARM_ARCH_9A__) // ARM CPU
#define CPU_PAUSE() __asm__ __volatile__("yield\n" : : : "memory")

#elif defined(__riscv) && (__riscv_xlen == 64)
#define CPU_PAUSE() __asm__ __volatile__("wfi\n" : : : "memory")

#else
#define CPU_PAUSE() ((void)0)

#endif
#endif

/**
 * @def likely
 * @brief 分支预测优化——指示条件大概率成立
 * @param[in] x 条件表达式
 * @return 返回 x 的值，编译器据此优化分支布局
 */
#ifndef likely
#define likely(x) __builtin_expect((x), 1)
#endif

/**
 * @def unlikely
 * @brief 分支预测优化——指示条件大概率不成立
 * @param[in] x 条件表达式
 * @return 返回 x 的值，编译器据此优化分支布局
 */
#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

/**
 * @def FORCE_INLINE
 * @brief 强制内联修饰符
 * @details 使用 inline + __attribute__((always_inline)) 确保函数始终内联展开。
 * @warning 过度使用可能导致代码膨胀，仅用于性能关键的短小函数。
 */
#ifndef FORCE_INLINE
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

/**
 * @def STATIC_FORCE_INLINE
 * @brief 静态强制内联修饰符
 * @details 结合 static、inline 和 __attribute__((always_inline))，
 * 确保函数仅在本编译单元内联展开，避免链接冲突。
 * @warning 仅用于性能关键的短小函数，且仅在该编译单元内部使用。
 */
#ifndef STATIC_FORCE_INLINE
#define STATIC_FORCE_INLINE static inline __attribute__((always_inline))
#endif

// #ifndef LIB_EXPORT
// #define LIB_EXPORT extern "C" __declspec(dllexport) // _WIN32
// #define LIB_EXPORT extern "C"
// #endif

} // namespace lb_common

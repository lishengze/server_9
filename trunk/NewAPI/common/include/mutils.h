#pragma once

/**
 * @file mutils.h
 * @brief 公共工具函数模块
 *
 * 提供常用的系统工具函数，包括：
 * - 字符串转换（整数到十六进制/十进制字符串）
 * - 字符串格式化与复制
 * - SIMD优化内存操作
 * - 时间获取与休眠
 * - 共享内存管理
 * - 文件操作
 * - 路径与进程操作
 */

#include "comm_sys.h"
#include <cstdio>
#include <type_traits>

namespace lb_common {

/**
 * @brief 公共工具函数类
 *
 * 提供静态方法进行各种系统工具操作
 */
class comm_utils {
public:
  /**
	 * @brief 将64位无符号整数转换为十六进制字符串
	 *
	 * @param[in] val 待转换的64位无符号整数
	 * @param[out] buffer 输出缓冲区
	 * @param[in] buf_size 缓冲区大小
	 * @return 转换后的字符串长度
	 */
  static int32 uint64_to_hex(uint64 val, char *buffer, uint32 buf_size);

  /**
	 * @brief 将64位无符号整数转换为十进制字符串
	 *
	 * @param[in] val 待转换的64位无符号整数
	 * @param[out] buffer 输出缓冲区
	 * @param[in] buf_size 缓冲区大小
	 * @return 转换后的字符串长度
	 */
  static int32 uint64_to_dec(uint64 val, char *buffer, uint32 buf_size);

  /**
	 * @brief 格式化字符串
	 *
	 * 将字符串中无效字符后的位置设为\0
	 *
	 * @param[in,out] dst 目标字符串
	 * @param[in] dst_size 缓冲区大小
	 * @return 字符串实际长度
	 */
  static int32 str_format(char *dst, int32 dst_size);

  /**
	 * @brief 安全字符串复制
	 *
	 * @param[out] dst 目标缓冲区
	 * @param[in] src 源字符串
	 * @param[in] dst_size 目标缓冲区大小
	 * @return 复制的字符数
	 */
  static int32 str_copy(char *dst, const char *src, int32 dst_size);

  /**
	 * @brief 格式化字符串复制
	 *
	 * 复制字符串并将有效字符后的位置设为\0
	 *
	 * @param[out] dst 目标缓冲区
	 * @param[in] src 源字符串
	 * @param[in] dst_size 目标缓冲区大小
	 * @return 复制的字符数
	 */
  static int32 str_copy_format(char *dst, const char *src, int32 dst_size);

  /**
	 * @brief SIMD优化的内存复制
	 *
	 * @note 源和目标地址必须对齐（X86 32字节，ARM 16字节）
	 * @note len必须大于等于对齐长度
	 *
	 * @param[out] dst 目标地址
	 * @param[in] src 源地址
	 * @param[in] len 复制长度
	 */
  static void simd_copy(uint8 *dst, const uint8 *src, int32 len);

  /**
	 * @brief SIMD优化的内存比较
	 *
	 * @param[in] dst 目标地址
	 * @param[in] src 源地址
	 * @param[in] len 比较长度
	 * @return true=相等, false=不等
	 */
  static bool simd_equal(const uint8 *dst, const uint8 *src, int32 len);

  /**
	 * @brief SIMD优化的内存清零
	 *
	 * @param[out] dst 目标地址
	 * @param[in] len 清零长度
	 */
  static void simd_zero(uint8 *dst, int32 len);

  /**
	 * @brief 获取CPU时间戳计数器（RDTSC）
	 *
	 * @return CPU周期数
	 */
  static uint64 get_rdtsc();

  /**
	 * @brief 获取单调时间
	 *
	 * @return 纳秒数
	 */
  static uint64 get_tick();

  /**
	 * @brief 秒级休眠
	 *
	 * @param[in] sec 休眠秒数
	 */
  static void sleep_s(int32 sec);

  /**
	 * @brief 毫秒级休眠
	 *
	 * @param[in] millisec 休眠毫秒数
	 */
  static void sleep_ms(int32 millisec);

  /**
	 * @brief 微秒级休眠
	 *
	 * @param[in] us 休眠微秒数
	 */
  static void sleep_us(int32 us);

  /**
	 * @brief 获取当前时间
	 *
	 * @return 时间值（HHMMSS格式，如123045表示12:30:45）
	 */
  static int32 get_time();
  /**
	 * @brief 计算两个 HHMMSS 格式时间的秒数差
	 *
	 * @param[in] t1 第一个时间值（HHMMSS格式）
	 * @param[in] t2 第二个时间值（HHMMSS格式）
	 * @return t1 - t2 的差值
	 */
  static int32 diff_time(int32 t1, int32 t2);

  /**
	 * @brief 获取当前时间（字符串格式，如12:30:45）
	 *
	 * @param[out] o_buf 输出缓冲区
	 * @param[in] buf_len 缓冲区大小（至少9字节）
	 */
  static void get_time(char *o_buf, int32 buf_len);

  /**
	 * @brief 获取微秒级时间
	 *
	 * @return 时间值（格式：HHMMSSmmmuuu）
	 */
  static int64 get_time_us();
  /**
	 * @brief 计算两个 HHMMSSmmmuuu 格式时间的微秒数差
	 *
	 * @param[in] t1 第一个时间值（HHMMSSmmmuuu格式）
	 * @param[in] t2 第二个时间值（HHMMSSmmmuuu格式）
	 * @return t1 - t2 的差值
	 */
  static int64 diff_time_us(int64 t1, int64 t2);

  /**
	 * @brief 获取微秒级时间
	 *
	 * @return 时间值（格式：HHMMSSmmm）
	 */
  static int32 get_time_ms();
  /**
	 * @brief 计算两个 HHMMSSmmm 格式时间的毫秒数差
	 *
	 * @param[in] t1 第一个时间值（HHMMSSmmm格式）
	 * @param[in] t2 第二个时间值（HHMMSSmmm格式）
	 * @return t1 - t2 的差值
	 */
  static int32 diff_time_ms(int32 t1, int32 t2);

  /**
	 * @brief 获取当前日期
	 *
	 * @return 日期值（格式：YYYYMMDD，如20260312）
	 */
  static int32 get_date();

  /**
	 * @brief 判断整数是否为2的幂
	 *
	 * 使用位运算快速判断，时间复杂度O(1)。仅适用于整数类型。
	 *
	 * @tparam T 整数类型
	 * @param[in] x 待判断的整数
	 * @return true=是2的幂，false=不是2的幂
	 *
	 * @note x必须为正整数
	 */
  template <typename T> static bool is_power_2(T x) {
    static_assert(std::is_integral<T>::value, "T must be integral");
    return x > 0 && (x & (x - 1)) == 0;
  }

  /**
	 * @brief 对齐内存分配
	 *
	 * @param[in] size 分配大小
	 * @param[in] alignment 对齐字节数（必须为2的幂）
	 * @return 分配的内存指针，失败返回NULL
	 */
  static void *aligned_malloc(size_t size, size_t alignment);

  /**
	 * @brief 释放对齐内存
	 *
	 * @param[in] p 内存指针
	 */
  static void aligned_free(void *p);

  /**
	 * @brief 映射共享内存
	 *
	 * @param[out] o_addr 输出参数，映射后的地址
	 * @param[in] shm_name 共享内存名称
	 * @param[in] shm_size 共享内存大小
	 * @param[in] hugepage 是否使用大页（1=是，0=否）
	 * @param[in] is_create 是否创建（1=创建，0=仅打开）
	 * @return 1=已存在并打开，0=新建打开，<0=失败
	 */
  static int32 map_shm(void *&o_addr, const char *shm_name, int64 shm_size, int32 hugepage, int32 is_create = 1);

  /**
	 * @brief 解除共享内存映射
	 *
	 * @param[in] shm_addr 映射地址
	 * @param[in] shm_size 共享内存大小
	 */
  static void unmap_shm(void *shm_addr, int64 shm_size);

  /**
	 * @brief 删除共享内存
	 *
	 * @param[in] shm_name 共享内存名称
	 */
  static void free_shm(const char *shm_name);

  /**
	 * @brief 写入文件
	 *
	 * @param[in] tfp 文件指针
	 * @param[in] data 数据缓冲区
	 * @param[in] len 数据长度
	 * @return 写入的字节数，负数=失败
	 */
  static int32 write_file(std::FILE *tfp, char *data, int32 len);

  /**
	 * @brief 读取文件
	 *
	 * @param[in] tfp 文件指针
	 * @param[out] buf 读取缓冲区
	 * @param[in] len 期望读取长度
	 * @return 实际读取的字节数，负数=失败
	 */
  static int32 read_file(std::FILE *tfp, char *buf, int len);

  /**
	 * @brief 判断文件是否存在
	 *
	 * @param[in] file_name 文件路径
	 * @return 1=存在，0=不存在
	 */
  static int32 exist_file(const char *file_name);

  /**
	 * @brief 创建目录
	 *
	 * @param[in] path_name 目录路径
	 * @return 0=成功，负数=失败
	 */
  static int32 make_path(const char *path_name);

  /**
	 * @brief 判断路径是否存在
	 *
	 * @param[in] path_name 路径
	 * @return 1=存在且为目录，0=不存在，负数=出错
	 */
  static int32 exist_path(const char *path_name);

  /**
	 * @brief 判断进程是否存在
	 *
	 * @param[in] proc_id 进程ID
	 * @return 1=存在，0=不存在，负数=出错
	 */
  static int32 exist_pid(int64 proc_id);

  /**
	 * @brief 获取当前进程ID
	 *
	 * @return 进程ID
	 */
  static int64 get_pid();

  /**
	 * @brief 计算2的幂次
	 *
	 * @param[in] n 数值
	 * @return 需要的位数
	 */
  static int32 calc_power(int64 n);
};

} // namespace lb_common

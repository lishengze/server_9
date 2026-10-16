/**
 * @file mutils.cpp
 * @brief 公共工具函数实现
 *
 * @see mutils.h
 */

#include "mutils.h"
#include "comm_errno.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// SIMD 指令集头文件（必须在文件顶部包含，不能放在函数内部）
#if defined(__x86_64__) || defined(__amd64__)
#if defined(__AVX2__)
#include <immintrin.h> // X86 AVX2 指令集: gcc -O2 -mavx2
#endif
#elif defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h> // AArch64 NEON 指令集: -O2 -march=armv8-a
#elif defined(__riscv) && (__riscv_xlen == 64) && defined(__riscv_vector)
#include <riscv_vector.h> // RISC-V RVV 指令集: -O2 -march=rv64gcv
#endif

#include <inttypes.h>

namespace lb_common
{

  int32 comm_utils::uint64_to_hex(uint64 val, char *buffer, uint32 buf_size)
  {
    assert(NULL != buffer && buf_size > 0);

    if (buf_size < 17)
    { // 16 hex 字符 + 至少 1 个 NUL
      if (buf_size > 0)
        buffer[0] = '\0';
      return 0;
    }

    static constexpr char hex[] = "0123456789abcdef";
    char *p = buffer;
    // 从最高有效位开始，跳过前导零
    bool started = false;
    for (int32 shift = 60; shift >= 0; shift -= 4)
    {
      uint8 nibble = (val >> shift) & 0xF;
      if (nibble != 0 || started || shift == 0)
      {
        started = true;
        *p++ = hex[nibble];
      }
    }
    int32 tlen = static_cast<int32>(p - buffer);
    for (uint32 j = tlen; j < buf_size; ++j)
    {
      *p++ = '\0';
    }
    return tlen;
  }

  int32 comm_utils::uint64_to_dec(uint64 val, char *buffer, uint32 buf_size)
  {
    assert(NULL != buffer && buf_size > 0);

    if (buf_size < 21)
    {
      if (buf_size > 0)
        buffer[0] = '\0';
      return 0;
    }

    char *end = buffer + 20; // 指向第21个字节（索引20）
    char *p = end;           // 从结束符前一位开始填数字

    do
    {
      uint64 tv = val / 10;
      *--p = '0' + (val - tv * 10); // 从低位到高位依次填入
      val = tv;
    } while (val);

    int32 tlen = static_cast<int32>(end - p);
    if (tlen < 20)
    {
      // 移动内存（重叠区域安全）
      std::memmove(buffer, p, tlen);
    }
    for (uint32 j = tlen; j < buf_size; ++j)
    {
      buffer[j] = '\0';
    }
    return tlen;
  }

  int32 comm_utils::str_format(char *dst, int32 dst_size)
  {
    assert(NULL != dst && dst_size > 0);
    int32 i = 0;
    for (; i < dst_size; ++i)
    {
      if (dst[i] == '\0')
        break;
    }
    for (int32 j = i; j < dst_size; ++j)
    {
      dst[j] = '\0';
    }
    return i;
  }
  int32 comm_utils::str_format_safe(char *dst, int32 dst_size)
  {
    assert(NULL != dst && dst_size > 0);
    int32 i = 0;
    dst_size--;
    for (; i < dst_size; ++i)
    {
      if (dst[i] == '\0')
        break;
    }
    dst_size++;
    for (int32 j = i; j < dst_size; ++j)
    {
      dst[j] = '\0';
    }
    return i;
  }
  int32 comm_utils::str_copy_format(char *dst, const char *src, int32 dst_size)
  {
    assert(NULL != dst && NULL != src && dst_size > 0);

    int32 i = 0;
    for (; i < dst_size && src[i] != '\0'; i++)
    {
      dst[i] = src[i];
    }
    for (int32 j = i; j < dst_size; ++j)
    {
      dst[j] = '\0';
    }
    return i;
  }
  int32 comm_utils::str_copy_safe(char *dst, int32 dst_size, const char *src, int32 src_size)
  {
    int32_t tl = (dst_size - 1) > src_size ? src_size : (dst_size - 1);
    int32_t i = 0;
    while (i < tl && src[i] != '\0')
    {
      dst[i] = src[i];
      i++;
    }
    tl = i;
    while (i < dst_size)
    {
      dst[i] = '\0';
      i++;
    }
    return tl;
  }

  void comm_utils::simd_copy(uint8 *dst, const uint8 *src, int32 len)
  {
    assert(NULL != dst && NULL != src);

#if defined(__x86_64__) || defined(__amd64__)

#if defined(__AVX2__)

    assert(len >= 32); // && IS_ALIGN(len,32));
    assert(IS_ALIGN((uintptr_t)dst, 32));
    assert(IS_ALIGN((uintptr_t)src, 32));

    size_t n_vec = (len >> 5);
    __m256i *dst_vec = (__m256i *)dst;
    const __m256i *src_vec = (const __m256i *)src;
    for (size_t i = 0; i < n_vec; ++i)
    {
      // AVX2 加载-存储（对齐版本，效率最高）
      *dst_vec++ = _mm256_load_si256(src_vec++);
    }

    int32 left = ALIGN_OFF(len, 32);
    if (left > 0)
    {
      n_vec = len - left; //(n_vec<<5);
      dst += n_vec;
      src += n_vec;
      while (left > 0)
      {
        *dst++ = *src++;
        left--;
      }
    }
#else
    memcpy(dst, src, len);
#endif

#elif defined(__aarch64__) && defined(__ARM_NEON__)
    // NEON 向量宽度：128 位（16 字节），对齐要求 16 字节（华为鲲鹏兼容）

    assert(len >= 16); // && IS_ALIGN(len,16));
    assert(IS_ALIGN((uintptr_t)dst, 16));
    assert(IS_ALIGN((uintptr_t)src, 16));

    size_t n_vec = (len >> 4);
    uint8x16_t *dst_vec = (uint8x16_t *)dst;
    const uint8x16_t *src_vec = (const uint8x16_t *)src;
    for (size_t i = 0; i < n_vec; ++i)
    {
      // NEON 加载-存储（对齐版本，鲲鹏平台性能最优）
      *dst_vec++ = vld1q_u8(src_vec++);
    }

    int32 left = ALIGN_OFF(len, 16);
    if (left > 0)
    {
      n_vec = len - left; //(n_vec<<4);
      dst += n_vec;
      src += n_vec;
      while (left > 0)
      {
        *dst++ = *src++;
        left--;
      }
    }

#elif defined(__riscv) && (__riscv_xlen == 64) && defined(__riscv_vector)

    assert(len >= 16); // && IS_ALIGN(len,16));
    assert(IS_ALIGN((uintptr_t)dst, 16));
    assert(IS_ALIGN((uintptr_t)src, 16));

    // 取硬件 VLMAX，避免硬编码 16 在 VLEN < 128 的平台上越界/漏拷字节
    size_t vs = vsetvlmax_e8m2();
    size_t processed = 0;
    while (processed + vs <= (size_t)len)
    {
      // RVV 加载-存储（对齐版本）
      vuint8m2_t vec = vle8_v_u8m2(src + processed, vs);
      vse8_v_u8m2(dst + processed, vec, vs);
      processed += vs;
    }

    // 处理剩余不足一个完整向量的部分
    for (size_t i = processed; i < (size_t)len; ++i)
    {
      dst[i] = src[i];
    }

#else

    memcpy(dst, src, len);
#endif
  }
  bool comm_utils::simd_equal(const uint8 *dst, const uint8 *src, int32 len)
  {
    assert(NULL != dst && NULL != src);

#if defined(__x86_64__) || defined(__amd64__)

#if defined(__AVX2__)

    assert(len >= 32); // && IS_ALIGN(len,32));
    assert(IS_ALIGN((uintptr_t)dst, 32));
    assert(IS_ALIGN((uintptr_t)src, 32));

    size_t n_vec = (len >> 5);
    const __m256i *v1 = (const __m256i *)(dst);
    const __m256i *v2 = (const __m256i *)(src);
    for (size_t i = 0; i < n_vec; ++i)
    {
      // 按字节对比：相等则对应位为 0xFF，否则为 0x00
      __m256i cmp = _mm256_cmpeq_epi8(v1[i], v2[i]);
      // 检查是否所有字节都相等（全 0xFF 则为 -1，否则存在不相等字节）
      int32 mask = _mm256_movemask_epi8(cmp);
      if (mask != -1) // 0xFFFFFFFF)
      {
        return false;
      }
    }

    int32 left = ALIGN_OFF(len, 32);
    if (left > 0)
    {
      n_vec = len - left; //(n_vec<<4);
      dst += n_vec;
      src += n_vec;
      for (int32 i = 0; i < left; ++i)
      {
        if (dst[i] != src[i])
        {
          return false;
        }
      }
    }
    return true;

#else
    return (0 == memcmp(dst, src, len));
#endif

#elif defined(__aarch64__) && defined(__ARM_NEON__)
    // NEON 向量宽度：128 位（16 字节），对齐要求 16 字节（华为鲲鹏兼容）

    assert(len >= 16); // && IS_ALIGN(len,16));
    assert(IS_ALIGN((uintptr_t)dst, 16));
    assert(IS_ALIGN((uintptr_t)src, 16));

    size_t n_vec = (len >> 4);
    const uint8x16_t *v1 = (const uint8x16_t *)(dst);
    const uint8x16_t *v2 = (const uint8x16_t *)(src);
    for (size_t i = 0; i < n_vec; ++i)
    {
      // 按字节对比，生成掩码（相等为 1，不等为 0）
      uint8x16_t cmp = vceqq_u8(v1[i], v2[i]);
      // P2-07: 旧版 mask = lane0 | lane1。当两 lane 互补不匹配时（如 lane0=0xFF..FF、
      // lane1=0x00..00），OR 仍可能得到全 1，误判整体相等。改用 AND。
      uint64 mask = vgetq_lane_u64(vreinterpretq_u64_u8(cmp), 0) &
                    vgetq_lane_u64(vreinterpretq_u64_u8(cmp), 1);
      if (mask != 0xFFFFFFFFFFFFFFFFULL)
      {
        return false;
      }
    }

    int32 left = ALIGN_OFF(len, 16);
    if (left > 0)
    {
      n_vec = len - left; //(n_vec<<4);
      dst += n_vec;
      src += n_vec;
      for (int32 i = 0; i < left; ++i)
      {
        if (dst[i] != src[i])
        {
          return false;
        }
      }
    }
    return true;

    // -------------------------- RISC-V 64（RVV）实现 --------------------------
#elif defined(__riscv) && (__riscv_xlen == 64) && defined(__riscv_vector)

    assert(len >= 16); // && IS_ALIGN(len,16));
    assert(IS_ALIGN((uintptr_t)dst, 16));
    assert(IS_ALIGN((uintptr_t)src, 16));

    // 取硬件 VLMAX，避免硬编码 16 在 VLEN < 128 的平台上漏检字节
    size_t vs = vsetvlmax_e8m1();
    size_t processed = 0;
    while (processed + vs <= (size_t)len)
    {
      // 加载向量（对齐加载）
      vint8m1_t v1 = vle8_v_i8m1(dst + processed, vs);
      vint8m1_t v2 = vle8_v_i8m1(src + processed, vs);
      // 按字节对比，生成掩码（相等为 1，不等为 0）
      vbool8_t cmp = vmseq_vx_i8m1_b8(v1, v2, vs);
      // 检查是否所有字节都相等（实际比较 vs 位，不再硬编码 16）
      if (vcpop_m_b8(cmp, vs) != vs)
      {
        return false;
      }
      processed += vs;
    }

    // 处理剩余不足一个完整向量的部分
    for (size_t i = processed; i < (size_t)len; ++i)
    {
      if (dst[i] != src[i])
      {
        return false;
      }
    }
    return true;

#else

    return (0 == memcmp(dst, src, len));
#endif
  }

  void comm_utils::simd_zero(uint8 *dst, int32 len)
  {
    assert(NULL != dst);

#if defined(__x86_64__) || defined(__amd64__)
    // X86_64 AVX2 指令集：32字节（256位）对齐，编译选项：-O2 -mavx2
#if defined(__AVX2__)

    assert(len >= 32);
    assert(IS_ALIGN((uintptr_t)dst, 32));

    // 计算32字节向量的个数（len >> 5 等价于 len / 32）
    size_t n_vec = (len >> 5);
    __m256i *dst_vec = (__m256i *)dst;
    // 生成256位全零向量（仅需初始化一次，复用提升性能）
    const __m256i zero_vec = _mm256_setzero_si256();

    // 批量清零：32字节/次
    for (size_t i = 0; i < n_vec; ++i)
    {
      *dst_vec++ = zero_vec; // AVX2 对齐存储全零向量
    }

    // 处理剩余不足32字节的部分
    int32 left = ALIGN_OFF(len, 32);
    if (left > 0)
    {
      int32 processed = len - left;
      dst += processed;
      while (left > 0)
      {
        *dst++ = 0;
        left--;
      }
    }
#else
    // 无AVX2时降级为标准memset
    memset(dst, 0, len);
#endif

#elif defined(__aarch64__) && defined(__ARM_NEON__)
    // AArch64 NEON 指令集：16字节（128位）对齐，编译选项：-O2 -march=armv8-a
    // -mneon

    assert(len >= 16);
    assert(IS_ALIGN((uintptr_t)dst, 16));

    // 计算16字节向量的个数（len >> 4 等价于 len / 16）
    size_t n_vec = (len >> 4);
    uint8x16_t *dst_vec = (uint8x16_t *)dst;
    // 生成128位全零向量
    const uint8x16_t zero_vec = vdupq_n_u8(0);

    // 批量清零：16字节/次
    for (size_t i = 0; i < n_vec; ++i)
    {
      vst1q_u8(dst_vec++, zero_vec); // NEON 对齐存储全零向量
    }

    // 处理剩余不足16字节的部分
    int32 left = ALIGN_OFF(len, 16);
    if (left > 0)
    {
      int32 processed = len - left;
      dst += processed;
      while (left > 0)
      {
        *dst++ = 0;
        left--;
      }
    }

#elif defined(__riscv) && (__riscv_xlen == 64) && defined(__riscv_vector)
    // RISC-V64 RVV 指令集：编译选项：-O2 -march=rv64gcv

    assert(len >= 16);
    assert(IS_ALIGN((uintptr_t)dst, 16));

    // 取硬件 VLMAX，避免硬编码 16 在 VLEN < 128 的平台上漏清零
    size_t vl = vsetvlmax_e8m2();
    // 生成全零向量
    const vuint8m2_t zero_vec = vmv_v_x_u8m2(0, vl);

    size_t processed = 0;
    while (processed + vl <= (size_t)len)
    {
      vse8_v_u8m2(dst + processed, zero_vec, vl); // RVV 对齐存储全零向量
      processed += vl;
    }

    // 处理剩余不足一个完整向量的部分
    for (size_t i = processed; i < (size_t)len; ++i)
    {
      dst[i] = 0;
    }

#else
    // 其他架构降级为标准memset
    memset(dst, 0, len);
#endif
  }

  uint64 comm_utils::get_rdtsc()
  {
// ns = rdtsc_val * 1e9 / cpu_freq
#if defined(__x86_64__) || defined(__amd64__)
    uint32_t lo, hi;
    // x86-64 rdtsc 指令：读取时间戳计数器（TSC），EDX:EAX
    // 使用 "=a"/"=d" 约束让编译器必须把结果写到 RAX/RDX，与指令语义一致
    __asm__ __volatile__("rdtsc\n"
                         : "=a"(lo), "=d"(hi)
                         :
                         : "memory" // 内存屏障，避免与后续代码重排
    );
    uint64 rt = ((uint64)hi << 32) | lo;
    return rt;

#elif defined(__aarch64__) || defined(__ARM_ARCH_8A__) || defined(__ARM_ARCH_9A__) // ARM CPU
    uint64_t cnt;
    // AArch64 读取 CNTPCT_EL0（固定频率物理计数器，用户态可访问）
    __asm__ __volatile__("mrs %0, cntpct_el0\n" // MRS 指令读取系统寄存器
                         : "=r"(cnt)
                         :
                         : "memory");
    return cnt;

#elif defined(__riscv) && (__riscv_xlen == 64) // RISC-V 64 架构（算能 SG2042 等信创 CPU）
    uint32_t lo, hi;
    // RISC-V 读取 rdcycleh/rdcycle（CPU 周期计数器，全兼容）
    __asm__ __volatile__("rdcycleh %0, x0\n" // 读取高位 32 位
                         "rdcycle  %1, x0\n" // 读取低位 32 位（先读高位避免溢出）
                         : "=r"(hi), "=r"(lo)
                         :
                         : "memory");
    uint64 rt = hi;
    rt = (rt << 32) | lo;
    return rt;

#else
#error "unsupported CPU architecture"
#endif
  }

  uint64 comm_utils::get_tick()
  {
    uint64 ret;
    struct timespec tms = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tms);
    // 用 uint64 强制无符号乘法，避免 tv_sec * 1e9 在 ~292 年后溢出 int64 UB
    ret = (uint64)tms.tv_sec * 1000000000ULL + (uint64)tms.tv_nsec;
    return ret;
  }

  void comm_utils::sleep_s(int32 sec)
  {
    // sleep 也可能被信号中断，这里简单忽略 EINTR 即可
    while (sleep((unsigned int)sec) != 0 && errno == EINTR)
    {
    }
  }

  void comm_utils::sleep_ms(int32 millisec)
  {
    struct timespec req;
    req.tv_sec = millisec / 1000;
    req.tv_nsec = (millisec % 1000) * 1000000;
    struct timespec rem;
    // nanosleep 被信号中断时返回 -1 且 errno=EINTR，剩余时间写入 rem，应接续重试
    while (nanosleep(&req, &rem) == -1 && errno == EINTR)
    {
      req = rem;
    }
  }
  void comm_utils::sleep_us(int32 us)
  {
    struct timespec req;
    req.tv_sec = us / 1000000;
    req.tv_nsec = (us % 1000000) * 1000;
    struct timespec rem;
    while (nanosleep(&req, &rem) == -1 && errno == EINTR)
    {
      req = rem;
    }
  }

  int32 comm_utils::get_time()
  {
    struct tm tmloc;
    time_t curtime = time(0);
    localtime_r(&curtime, &tmloc);

    return (tmloc.tm_hour * 10000 + tmloc.tm_min * 100 + tmloc.tm_sec);
  }
  int32 comm_utils::diff_time(int32 t1, int32 t2)
  {
    int32 tt1 = t1 / 10000;
    int32 tt2 = t2 / 10000;
    int32 tdiff = (tt1 - tt2) * 3600;
    t1 -= tt1 * 10000;
    t2 -= tt2 * 10000;

    tt1 = t1 / 100;
    tt2 = t2 / 100;
    tdiff += (tt1 - tt2) * 60;
    t1 -= tt1 * 100;
    t2 -= tt2 * 100;

    tdiff += (t1 - t2);
    return tdiff;
  }

  void comm_utils::get_time(char *o_buf, int32 buf_len)
  {
    assert(NULL != o_buf && buf_len >= 9);

    struct tm tmloc;
    time_t curtime = time(0);
    localtime_r(&curtime, &tmloc);

    snprintf(o_buf, buf_len, "%02d:%02d:%02d", tmloc.tm_hour, tmloc.tm_min, tmloc.tm_sec);
    o_buf[8] = '\0';
  }
  int64 comm_utils::get_time_us(int32 *o_errcode)
  {
    if (o_errcode != NULL)
      *o_errcode = 0;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
      if (o_errcode != NULL)
        *o_errcode = LBERR_TIME_TAKE_FAIL;
      return 0;
    }

    struct tm local_tm;
    if (localtime_r(&ts.tv_sec, &local_tm) == NULL)
    {
      if (o_errcode != NULL)
        *o_errcode = LBERR_TIME_TAKE_FAIL;
      return 0;
    }

    return ((int64)local_tm.tm_hour * 10000000000LL + (int64)local_tm.tm_min * 100000000LL +
            (int64)local_tm.tm_sec * 1000000LL + ts.tv_nsec / 1000);
  }
  int64 comm_utils::diff_time_us(int64 t1, int64 t2)
  {
    int64 tt1 = t1 / 10000000000LL;
    int64 tt2 = t2 / 10000000000LL;
    int64 tdiff = (tt1 - tt2) * 3600000000LL;
    t1 -= tt1 * 10000000000LL;
    t2 -= tt2 * 10000000000LL;

    tt1 = t1 / 100000000LL;
    tt2 = t2 / 100000000LL;
    tdiff += (tt1 - tt2) * 60000000LL;
    t1 -= tt1 * 100000000LL;
    t2 -= tt2 * 100000000LL;

    tt1 = t1 / 1000000LL;
    tt2 = t2 / 1000000LL;
    tdiff += (tt1 - tt2) * 1000000LL;
    t1 -= tt1 * 1000000LL;
    t2 -= tt2 * 1000000LL;

    tdiff += (t1 - t2);
    return tdiff;
  }

  int32 comm_utils::get_time_ms(int32 *o_errcode)
  {
    if (o_errcode != NULL)
      *o_errcode = 0;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
      if (o_errcode != NULL)
        *o_errcode = LBERR_TIME_TAKE_FAIL;
      return 0;
    }

    struct tm local_tm;
    if (localtime_r(&ts.tv_sec, &local_tm) == NULL)
    {
      if (o_errcode != NULL)
        *o_errcode = LBERR_TIME_TAKE_FAIL;
      return 0;
    }

    return ((int32)local_tm.tm_hour * 10000000 + (int32)local_tm.tm_min * 100000 + (int32)local_tm.tm_sec * 1000 +
            ts.tv_nsec / 1000000);
  }

  int32 comm_utils::diff_time_ms(int32 t1, int32 t2)
  {
    int32 tt1 = t1 / 10000000;
    int32 tt2 = t2 / 10000000;
    int32 tdiff = (tt1 - tt2) * 3600000;
    t1 -= tt1 * 10000000;
    t2 -= tt2 * 10000000;

    tt1 = t1 / 100000;
    tt2 = t2 / 100000;
    tdiff += (tt1 - tt2) * 60000;
    t1 -= tt1 * 100000;
    t2 -= tt2 * 100000;

    tt1 = t1 / 1000;
    tt2 = t2 / 1000;
    tdiff += (tt1 - tt2) * 1000;
    t1 -= tt1 * 1000;
    t2 -= tt2 * 1000;

    tdiff += (t1 - t2);
    return tdiff;
  }

  int32 comm_utils::get_date()
  {
    struct tm p;
    time_t curtime = time(0);
    localtime_r(&curtime, &p);

    return ((1900 + p.tm_year) * 10000 + (1 + p.tm_mon) * 100 + p.tm_mday);
  }

  void *comm_utils::aligned_malloc(size_t size, size_t alignment)
  {
    assert(alignment >= 8 && is_power_2(alignment));

    void *p = NULL;
    // posix_memalign 失败时 p 内容未定义（POSIX 规定），必须检查返回值
    if (::posix_memalign(&p, alignment, size) != 0)
    {
      return NULL;
    }
    return p;
  }
  void comm_utils::aligned_free(void *p) { ::free(p); }

  // 返回：1-共享内存存在并打开，0-新建打开；<0-失败
  int32 comm_utils::map_shm(void *&o_addr, const char *shm_name, int64 shm_size, int32 hugepage, int32 is_create)
  {
    assert(NULL != shm_name && shm_size > 0);

    int32 shmisexist = 0;
    int32 shmfd = shm_open(shm_name, O_RDWR, S_IRWXU | S_IRGRP | S_IWGRP);
    if (shmfd < 0)
    {
      if (errno != ENOENT)
        return LBERR_OBJ_OPEN_FAIL;
      if (is_create == 0)
        return LBERR_OBJ_NOT_HAVE;

      shmfd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRGRP | S_IWGRP);
      if (shmfd < 0)
      {
        // 两次 shm_open 之间另一进程并发创建同名 shm 时，O_EXCL 会返回 EEXIST。
        // 此时不能再断言 EEXIST 不存在（旧版会 abort），应作为"已存在"重新打开。
        if (errno == EEXIST)
        {
          shmfd = shm_open(shm_name, O_RDWR, S_IRWXU | S_IRGRP | S_IWGRP);
          if (shmfd < 0)
            return LBERR_OBJ_OPEN_FAIL;
          shmisexist = 1;
        }
        else
        {
          return LBERR_OBJ_OPEN_FAIL;
        }
      }
      else
      {
        shmisexist = 0;
      }
    }
    else
    {
      shmisexist = 1;
    }

    if (is_create == 1)
    {
      if (ftruncate(shmfd, shm_size) < 0)
      {
        close(shmfd);
        return LBERR_ATTR_SET_FAIL;
      }
    }

    void *pshm_addr = NULL;
    if (hugepage == 1)
    {
      pshm_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_HUGETLB | MAP_SHARED, shmfd, 0);
    }
    else
    {
      pshm_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    }
    if (pshm_addr == MAP_FAILED)
    {
      close(shmfd);
      return LBERR_SHM_MAP_FAIL;
    }

    if (is_create == 1)
    {
      // 锁住共享内存防止被 swap（提升实时性）
      // mlock 失败（如权限不足、内存不足）不应阻断映射，但应记录错误码便于诊断
      if (unlikely(mlock(pshm_addr, shm_size) != 0))
      {
        int32 mlock_err = errno;
        // 解除映射，向上层返回错误
        munmap(pshm_addr, shm_size);
        o_addr = NULL;
        errno = mlock_err;
        close(shmfd);
        return LBERR_ATTR_SET_FAIL;
      }
    }

    o_addr = pshm_addr;

    close(shmfd);
    return shmisexist;
  }
  void comm_utils::unmap_shm(void *shm_addr, int64 shm_size)
  {
    if (NULL != shm_addr && shm_size > 0)
      munmap(shm_addr, shm_size);
  }
  void comm_utils::free_shm(const char *shm_name)
  {
    if (NULL != shm_name && shm_name[0] != '\0')
      shm_unlink(shm_name);
  }

  int32 comm_utils::write_file(std::FILE *tfp, char *data, int32 len)
  {
    int nleft = len;
    int nwritten = 0;
    char *ptr = data;

    while (nleft > 0)
    {
      // 进入循环前清零 errno，避免被之前的 syscall 残留污染 EINTR 判断
      errno = 0;
      // fwrite 返回 size_t，永不为负；为 0 时通常表示 EOF / 致命错误（如 fd 失效、
      // 磁盘满），必须跳出而非 continue，否则会死循环。仅 EINTR 才允许重试。
      nwritten = static_cast<int>(std::fwrite(ptr, sizeof(char), nleft, tfp));
      if (nwritten == 0)
      {
        if (errno == EINTR)
          continue;
        return (nleft == len) ? LBERR_OBJ_WRITE_FAIL : LBERR_OBJ_WRITE_PART;
      }
      nleft -= nwritten;
      ptr += nwritten;
    }
    // std::fflush(tfp);
    return len;
  }

  int32 comm_utils::read_file(std::FILE *tfp, char *buf, int len)
  {
    int32 nleft = len;
    int32 readlen = 0;
    int32 ret = 0;
    errno = 0;
    while (nleft > 0)
    {
      if ((ret = static_cast<int32>(std::fread(buf + readlen, sizeof(char), nleft, tfp))) <= 0)
      {
        if (errno == EINTR || errno == EAGAIN)
        {
          errno = 0;
          continue;
        }
        else if (std::feof(tfp))
        {
          return readlen;
        }
        else if (std::ferror(tfp))
        {
          // 真错误（非 EOF、非 EINTR/EAGAIN）
          return (readlen == 0) ? LBERR_OBJ_READ_FAIL : readlen;
        }
        else
        {
          return readlen;
        }
      }
      nleft -= ret;
      readlen += ret;
    }
    return readlen;
  }

  int32 comm_utils::exist_file(const char *file_name)
  {
    if (access(file_name, F_OK) == 0)
      return 1;
    return 0;
  }
  int32 comm_utils::exist_path(const char *path_name)
  {
    struct stat attr;
    int32 ret = stat(path_name, &attr);
    if (ret == 0)
    {
      // 验证是否为目录
      return S_ISDIR(attr.st_mode) ? 1 : 0;
    }
    else if (errno == ENOENT)
    {
      return 0;
    }
    else
    {
      // 其他错误,如权限问题
      return LBERR_OBJ_OPEN_FAIL;
    }
  }
  int32 comm_utils::make_path(const char *path_name)
  {
    // mkdir 返回 EEXIST，错误地返回 LBERR_OBJ_OPEN_FAIL。忽略 EEXIST 视为成功。
    if (mkdir(path_name, 0775) == 0)
      return 0;
    if (errno == EEXIST)
    {
      // 存在但是不是目录？
      if (exist_path(path_name) == 1)
        return 0;
      return LBERR_OBJ_OPEN_FAIL;
    }
    return LBERR_OBJ_OPEN_FAIL;
  }

  int32 comm_utils::exist_pid(int64 proc_id)
  {
    char pid_file[256];
    std::memset(pid_file, 0, sizeof(pid_file));
    // 使用 PRId64 宏确保跨平台 int64_t 格式化正确（Linux 上 long 是 64 位，
    // 但 int64_t 严格说是 long long，%ld 在某些平台上不匹配）
    std::snprintf(pid_file, sizeof(pid_file) - 1, "/proc/%" PRId64, proc_id);

    return exist_path(pid_file);
  }
  int64 comm_utils::get_pid() { return (int64)(getpid()); }

  int32 comm_utils::calc_power(int64 n)
  {
    if (unlikely(n <= 1))
      return 0;

    int32 tn = 0;
#if defined(__GNUC__) || defined(__clang__)
    // 结果是 INT64_MIN），tn++ 后变 64 则 1LL << 64 也是 UB。改用无符号并对
    // tn >= 64 显式饱和。
    tn = 64 - __builtin_clzll((uint64)(n - 1));
#else
    int64 ts = n;
    while (ts != 1)
    {
      tn++;
      ts = (ts >> 1);
    }
#endif
    if (tn >= 63)
      return 63;
    if ((1ULL << tn) < (uint64)n)
      tn++;
    return tn;
  }

} // namespace lb_common

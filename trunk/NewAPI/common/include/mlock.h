#pragma once

/**
 * @file mlock.h
 * @brief 锁机制模块
 *
 * 提供多种锁实现，包括：
 * - 原子自旋锁（atomic_lock）
 * - 原子读写锁（atomic_rwlock）
 * - Linux系统自旋锁（cspinlock）
 * - 互斥锁（cmutex）
 * - 读写锁（rwlock）
 * - 进程间互斥锁（cmutex_proc）
 * - RAII锁守卫（clock_guard）
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "matomic.h"

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

namespace lb_common
{

/**
 * @brief 自旋锁最大自旋次数
 *
 * 达到此值后调用 sched_yield 让出 CPU。
 * 30000 次 CPU_PAUSE 在现代 CPU 上约消耗数百微秒，扩到 100000 让无锁竞争路径
 * 减少 syscall 调用（sched_yield 是 syscall，开销远大于 CPU_PAUSE hint）。
 */
#define SPIN_LOCK_BUSY_COUNT 100000

  /**
   * @brief 原子操作自旋锁,布局为单 int32，可安全 memcpy 放入共享内存（跨进程使用前需 init() 初始化）
   *
   * 使用CAS实现的轻量级自旋锁，适用于临界区较小、持有时间较短的场景。
   * 自旋一定次数后调用CPU_PAUSE指令，避免CPU总线占用。
   *
   * @note 适用于单进程多线程环境
   * @note 不支持递归锁
   *
   * @example
   * @code
   * atomic_lock lock;
   * lock.lock();
   * // 临界区代码
   * lock.unlock();
   * @endcode
   */
  class atomic_lock
  {
  public:
    /**
     * @brief 构造函数，默认初始化 mlock = 0
     *
     * 避免栈/全局实例未初始化时 lock() 出现随机值导致的"伪获取"或死锁
     */
    atomic_lock() : mlock(0) {}

    /**
     * @brief 获取锁
     *
     * 自旋等待直到获取锁，达到 SPIN_LOCK_BUSY_COUNT 后调用 sched_yield 让出 CPU。
     *
     * 当持锁线程与本线程被调度到同一 CPU 时,本线程会占满时间片
     * 在 atomic_cas32 上紧自旋,持锁线程永远拿不到 CPU → 饥饿。
     * 调用 sched_yield（POSIX 标准）主动让出 CPU, 给持锁线程机会跑上 CPU。
     * 注意 sched_yield 仅让出时间片,不一定能解决同 CPU pin 场景的饥饿,
     * 但比 CPU_PAUSE 紧自旋更友好。
     */
    FORCE_INLINE void lock()
    {
      int i = 0;
      do
      {
        int t = 0;
        if (atomic_cas32(&mlock, &t, 1))
          break;
        i++;
        if (i == SPIN_LOCK_BUSY_COUNT)
        {
          sched_yield();
          i = 0;
        }
      } while (true);
    }

    /**
     * @brief 释放锁
     */
    FORCE_INLINE void unlock() { atomic_store32(&mlock, 0); }

    /**
     * @brief 初始化锁
     *
     * 将锁状态设置为未锁定
     */
    FORCE_INLINE void init() { atomic_store32(&mlock, 0); }

    // 禁止拷贝（避免锁对象被复制导致 mlock 共享）
    atomic_lock(const atomic_lock &) = delete;
    atomic_lock &operator=(const atomic_lock &) = delete;

  private:
    // lock() 看到随机 mlock 值导致伪获取或永久死锁
    int32 mlock; ///< 锁状态：0=未锁定，1=已锁定
  };

  /**
   * @brief 原子操作读写锁,布局为单 int32，可安全 memcpy 放入共享内存（跨进程使用前需 init() 初始化）
   *
   * 使用原子操作实现的读写锁，支持多读者单写者。
   * 写锁优先：获取写锁时，会阻止新的读者进入。
   *
   * @note 适用于单进程多线程环境
   * @note 写锁获取会阻塞后续的读锁获取
   *
   *  holder 线程卡死（如 SIGSTOP）或长时间睡眠时，其他等待者将永久阻塞。
   *  对崩溃容错有要求的场景应使用 cmutex（系统互斥锁，线程死亡后进程退出）。
   *
   * @example
   * @code
   * atomic_rwlock rwlock;
   *
   * // 读操作
   * rwlock.read_lock();
   * // 读取共享数据
   * rwlock.read_unlock();
   *
   * // 写操作
   * rwlock.write_lock();
   * // 修改共享数据
   * rwlock.write_unlock();
   * @endcode
   */
  class atomic_rwlock
  {
  public:
#define RWLOCK_READ_MASK 0x0FFFFFFF
#define RWLOCK_WRITE_WAIT 0x10000000
#define RWLOCK_WRITE_ING 0x20000000
#define RWLOCK_WRITE_MASK 0x30000000

    /**
     * @brief 构造函数，默认初始化 mlock = 0
     */
    atomic_rwlock() : mlock(0) {}

    /**
     * @brief 获取读锁
     *
     * 多个线程可以同时获取读锁
     */
    FORCE_INLINE void read_lock()
    {
      // x86 上普通 load 与 atomic_load 等价，但跨架构/编译器不一致时不可靠）。
      int t = atomic_load32(&mlock);
      int i = 0;
      do
      {
        if ((t & RWLOCK_WRITE_MASK) == 0)
        {
          if (atomic_cas32(&mlock, &t, t + 1))
            break;
        }
        else
        {
          t = atomic_load32(&mlock);
        }
        i++;
        if (i == SPIN_LOCK_BUSY_COUNT)
        {
          sched_yield();
          i = 0;
        }
      } while (true);
    }

    /**
     * @brief 释放读锁
     */
    FORCE_INLINE void read_unlock() { atomic_fetch_sub32(&mlock, 1); }

    /**
     * @brief 获取写锁
     *
     * 写锁独占，阻塞其他读锁和写锁
     */
    FORCE_INLINE void write_lock()
    {
      int t = atomic_load32(&mlock);
      int i = 0;
      do
      {
        if (t == 0)
        {
          if (atomic_cas32(&mlock, &t, RWLOCK_WRITE_MASK))
            return;
        }
        else if ((t & RWLOCK_WRITE_MASK) == 0)
        {
          if (atomic_cas32(&mlock, &t, t | RWLOCK_WRITE_WAIT))
            break;
        }
        else
        {
          t = atomic_load32(&mlock);
        }
        i++;
        if (i == SPIN_LOCK_BUSY_COUNT)
        {
          sched_yield();
          i = 0;
        }
      } while (true);

      i = 0;
      do
      {
        t = atomic_load32(&mlock);
        if ((t & RWLOCK_READ_MASK) == 0)
        {
          if (atomic_cas32(&mlock, &t, t | RWLOCK_WRITE_ING))
            break;
        }
        i++;
        if (i == SPIN_LOCK_BUSY_COUNT)
        {
          sched_yield();
          i = 0;
        }
      } while (true);
    }

    /**
     * @brief 释放写锁
     */
    FORCE_INLINE void write_unlock() { atomic_store32(&mlock, 0); }

    /**
     * @brief 初始化锁
     */
    FORCE_INLINE void init() { atomic_store32(&mlock, 0); }

    // 禁止拷贝
    atomic_rwlock(const atomic_rwlock &) = delete;
    atomic_rwlock &operator=(const atomic_rwlock &) = delete;

  private:
    int32 mlock; ///< 锁状态：高位=写状态，低位=读计数
  };

  /**
   * @brief Linux系统自旋锁
   *
   * 基于pthread_spin_lock实现的系统自旋锁。
   *
   * @note 适用于单进程多线程环境
   * @note 必须在同一进程内使用
   * @note 临界区应尽可能短
   *
   * @example
   * @code
   * cspinlock lock;
   * lock.lock();
   * // 临界区代码
   * lock.unlock();
   * @endcode
   */
  class cspinlock
  {
  public:
    /**
     * @brief 获取锁
     *
     * pthread_spin_lock 唯一可能返回的错误是 EINVAL（mlock 未初始化或已 destroy）。
     * 这种错误不可恢复——重试永远不会成功,只会烧光 CPU。
     * 原 M6 修复曾改为自旋重试,现修正为：失败即直接终止进程,
     * 避免无锁进入临界区（旧 bug）也避免永久 hang（新 bug）。
     */
    FORCE_INLINE void lock()
    {
      int32 ret = pthread_spin_lock(&mlock);
      if (likely(ret == 0))
        return;
      std::fprintf(stderr,
                   "cspinlock::lock failed: %d (mlock uninitialized or destroyed) — aborting\n",
                   ret);
      std::abort();
    }

    /**
     * @brief 释放锁
     */
    FORCE_INLINE void unlock() { pthread_spin_unlock(&mlock); }

    /**
     * @brief 初始化锁
     *
     * 未初始化时 pthread_spin_destroy 返回 EINVAL，忽略即可。
     */
    FORCE_INLINE void init()
    {
      int32 ret = pthread_spin_init(&mlock, PTHREAD_PROCESS_PRIVATE);
      if (unlikely(ret != 0))
      {
        std::fprintf(stderr, "cspinlock::init failed: %d\n", ret);
        std::abort();
      }
    }

    /**
     * @brief 构造函数，自动初始化
     */
    cspinlock()
    {
      int32 ret = pthread_spin_init(&mlock, PTHREAD_PROCESS_PRIVATE);
      if (unlikely(ret != 0))
      {
        std::fprintf(stderr, "cspinlock ctor failed: %d\n", ret);
      }
    }

    /**
     * @brief 析构函数，销毁锁
     */
    ~cspinlock() { pthread_spin_destroy(&mlock); }

    // 禁止拷贝。const& 重载优先绑定,非 const 重载不可达,只保留一对删除声明即可。
    cspinlock(const cspinlock &) = delete;
    cspinlock &operator=(const cspinlock &) = delete;

  private:
    pthread_spinlock_t mlock; ///< 系统自旋锁对象
  };

  /**
   * @brief 单进程互斥锁
   *
   * 基于pthread_mutex实现的互斥锁。
   *
   * @note 适用于单进程多线程环境
   * @note 支持递归锁（需要配置）
   * @note 临界区可以较长
   *
   * @example
   * @code
   * cmutex lock;
   * lock.lock();
   * // 临界区代码
   * lock.unlock();
   * @endcode
   */
  class cmutex
  {
  public:
    /**
     * @brief 获取锁
     *
     * pthread_mutex_lock 的持久错误: EINVAL（mlock 未初始化或已 destroy）、
     * EAGAIN（仅非递归 errorcheck mutex 重试上限）、EDEADLK（递归加锁）。
     * 这些都是 invariant 违反——重试无法恢复,只会 hang。
     * 原 M6 修复曾改为自旋重试,现修正为：失败即直接终止进程,
     * 避免无锁进入临界区（旧 bug）也避免永久 hang（新 bug）。
     */
    FORCE_INLINE void lock()
    {
      int32 ret = pthread_mutex_lock(&mlock);
      if (likely(ret == 0))
        return;
      std::fprintf(stderr,
                   "cmutex::lock failed: %d (EINVAL/EDEADLK/mutex invalid) — aborting\n",
                   ret);
      std::abort();
    }

    /**
     * @brief 释放锁
     */
    FORCE_INLINE void unlock() { pthread_mutex_unlock(&mlock); }

    /**
     * @brief 初始化锁
     *
     * 安全约束：调用方必须保证**当前没有其他线程持有此锁**。
     * 若 pthread_mutex_destroy 返回 EBUSY（其他线程持锁），直接覆盖 mlock
     * 会破坏活 mutex 的内部状态,持锁线程后续 unlock 将写入已重新初始化的 mutex,
     * 触发未定义行为。这种情况应立即终止以暴露 bug。
     * 未初始化时 destroy 返回 EINVAL,按无害路径忽略。
     */
    FORCE_INLINE void init()
    {
      int32 ret = pthread_mutex_init(&mlock, NULL);
      if (unlikely(ret != 0))
      {
        std::fprintf(stderr, "cmutex::init failed: %d\n", ret);
        std::abort();
      }
    }

    /**
     * @brief 构造函数，自动初始化
     */
    cmutex()
    {
      int32 ret = pthread_mutex_init(&mlock, NULL);
      if (unlikely(ret != 0))
      {
        std::fprintf(stderr, "cmutex ctor failed: %d\n", ret);
      }
    }

    /**
     * @brief 析构函数，销毁锁
     */
    ~cmutex() { pthread_mutex_destroy(&mlock); }

    // 禁止拷贝。const& 重载优先绑定,非 const 重载不可达,只保留一对删除声明即可。
    cmutex(const cmutex &) = delete;
    cmutex &operator=(const cmutex &) = delete;

  private:
    pthread_mutex_t mlock; ///< 系统互斥锁对象
  };

  /**
   * @brief RAII锁守卫模板
   *
   * 模板参数T必须支持lock()和unlock()方法。
   * 在构造时获取锁，析构时自动释放锁，确保异常安全。
   *
   * @note 适用于需要异常安全的场景
   *
   * @example
   * @code
   * atomic_lock lock;
   * {
   *     clock_guard<atomic_lock> guard(lock);
   *     // 临界区代码
   * } // 自动释放锁
   * @endcode
   */
  template <class T>
  class clock_guard
  {
  public:
    /**
     * @brief 构造函数，获取锁
     * @param[in] tlock 锁的引用
     */
    clock_guard(T &tlock) : mlock(tlock) { mlock.lock(); };

    /**
     * @brief 析构函数，释放锁
     */
    ~clock_guard() { mlock.unlock(); };

    // M7: RAII 守卫持有锁引用且构造即加锁，拷贝/赋值会导致二次 unlock 或悬空引用。
    clock_guard(const clock_guard &) = delete;
    clock_guard &operator=(const clock_guard &) = delete;

  private:
    T &mlock; ///< 锁引用
  };

  /**
   * @brief 单进程读写锁
   *
   * 基于pthread_rwlock实现的读写锁。
   *
   * @note 适用于单进程多线程环境
   * @note 写锁优先：等待中的写锁会阻塞新的读锁
   *
   * @example
   * @code
   * rwlock lock;
   *
   * // 读操作
   * lock.read_lock();
   * // 读取共享数据
   * lock.read_unlock();
   *
   * // 写操作
   * lock.write_lock();
   * // 修改共享数据
   * lock.write_unlock();
   * @endcode
   */
  class rwlock
  {
  public:
    /**
     * @brief 获取写锁
     *
     * pthread_rwlock_wrlock 的持久错误: EINVAL（mlock 未初始化或已 destroy）、
     * EDEADLK（递归写锁）。两者都是 invariant 违反,无法恢复。
     * 原 M6 修复曾改为自旋重试,现修正为：失败即直接终止进程,
     * 避免无锁进入临界区（旧 bug）也避免永久 hang（新 bug）。
     */
    FORCE_INLINE void write_lock()
    {
      int32 ret = pthread_rwlock_wrlock(&mlock);
      if (likely(ret == 0))
        return;
      std::fprintf(stderr,
                   "rwlock::write_lock failed: %d (EINVAL/EDEADLK) — aborting\n",
                   ret);
      std::abort();
    }

    /**
     * @brief 释放写锁
     */
    FORCE_INLINE void write_unlock() { pthread_rwlock_unlock(&mlock); }

    /**
     * @brief 获取读锁
     *
     * pthread_rwlock_rdlock 的持久错误: EINVAL（mlock 未初始化或已 destroy）。
     * EAGAIN 仅在使用 PTHREAD_RWLOCK_PREFER_READER_NP 时才可能,本类用默认属性不会触发。
     * 该错误不可恢复——重试永远不会成功。
     * 原 M6 修复曾改为自旋重试,现修正为：失败即直接终止进程。
     */
    FORCE_INLINE void read_lock()
    {
      int32 ret = pthread_rwlock_rdlock(&mlock);
      if (likely(ret == 0))
        return;
      std::fprintf(stderr,
                   "rwlock::read_lock failed: %d (EINVAL) — aborting\n",
                   ret);
      std::abort();
    }

    /**
     * @brief 释放读锁
     */
    FORCE_INLINE void read_unlock() { pthread_rwlock_unlock(&mlock); }

    /**
     * @brief 初始化锁
     *
     * 安全约束：调用方必须保证**当前没有其他线程持有此锁**。
     * 若 pthread_rwlock_destroy 返回 EBUSY（其他线程持锁），直接覆盖 mlock
     * 会破坏活 rwlock 的内部状态,持锁线程后续 unlock 将写入已重新初始化的 rwlock,
     * 触发未定义行为（死锁或内存破坏）。这种情况应立即终止以暴露 bug,
     * 而不是静默继续。
     * 静态初始化的 pthread_rwlock_t 调用 destroy 通常返回 EINVAL,按无害路径忽略。
     */
    void init()
    {
      mlock = PTHREAD_RWLOCK_INITIALIZER;
    }

    /**
     * @brief 构造函数
     */
    rwlock() { mlock = PTHREAD_RWLOCK_INITIALIZER; }

    /**
     * @brief 析构函数
     *
     * pthread_rwlock_t 在某些 libc 下 destroy 会返回 EINVAL；已加锁但未释放的
     * 会返回 EBUSY。两种都是预期错误，不应静默忽略但也不应误报。
     */
    ~rwlock()
    {
      int32 ret = pthread_rwlock_destroy(&mlock);
      if (unlikely(ret != 0 && ret != EINVAL))
      {
        std::fprintf(stderr, "rwlock::~rwlock failed: %d\n", ret);
      }
    }

    // 禁止拷贝。const& 重载优先绑定,非 const 重载不可达,只保留一对删除声明即可。
    rwlock(const rwlock &) = delete;
    rwlock &operator=(const rwlock &) = delete;

  private:
    pthread_rwlock_t mlock; ///< 系统读写锁对象
  };

  /**
   * @brief 多进程间互斥锁
   *
   * 基于pthread_mutex实现的进程间互斥锁。
   * 锁对象必须存储在共享内存中，访问该共享内存的进程都可使用该锁。
   *
   * @note 适用于多进程环境
   * @note 锁对象必须位于共享内存中
   * @note 只初始化一次，初始化进程core后，锁不一定可用
   *
   * @example
   * @code
   * // 进程A
   * cmutex_proc* lock = (cmutex_proc*)shm_addr;
   * lock->init();
   * lock->lock();
   * // 临界区
   * lock->unlock();
   *
   * // 进程B
   * cmutex_proc* lock = (cmutex_proc*)shm_addr;
   * lock->lock();
   * // 临界区
   * lock->unlock();
   * @endcode
   */
  class cmutex_proc
  {
  public:
    /**
     * @brief 获取锁
     * @return 0=成功, 负数=错误码
     * @note 单参版本无 o_badlock 输出参数，EOWNERDEAD 恢复后无法区分"干净加锁"
     *       与"前持有者崩溃后的修复"，调用方可能照常读写半更新共享内存。为避免
     *       此隐患，EOWNERDEAD 恢复后本版本返回 LBERR_LOCK_NOT_CONSIST，强制调用
     *       方升级到 lock(int32 &o_badlock) 触发数据校验（设 o_badlock=1 即提示）。
     */
    FORCE_INLINE int32 lock()
    {
      int32 ret = pthread_mutex_lock(&mlock);
      if (likely(ret == 0))
      {
        return 0;
      }
      else if (ret == EOWNERDEAD)
      {
        if (pthread_mutex_consistent(&mlock) != 0)
        {
          // inconsistent mutex unlock 可能返回 ENOTRECOVERABLE，记录但不传播（已返回错误码）
          int32 ur = pthread_mutex_unlock(&mlock);
          if (ur != 0 && ur != EPERM)
          {
            std::fprintf(stderr, "cmutex_proc::lock unlock on inconsistent failed: %d\n", ur);
          }
          return LBERR_LOCK_NOT_CONSIST;
        }
        // 单参版本无 o_badlock 输出参数，强制要求调用方升级到双参版本
        // 触发数据校验（o_badlock==1 即为"前持有者崩溃，请校验共享数据"）。
        std::fprintf(stderr,
                     "cmutex_proc::lock: mutex recovered from EOWNERDEAD; "
                     "use lock(int32 &o_badlock) to verify shared data integrity\n");
        return LBERR_LOCK_NOT_CONSIST;
      }
      else if (ret == ENOTRECOVERABLE)
      {
        return LBERR_LOCK_NOT_RECOVER;
      }
      return LBERR_LOCK_TAKE_FAIL;
    }

    /**
     * @brief 获取锁（带锁状态输出）
     * @param[out] o_badlock 输出参数: 1=发生不一致并修复, 2=锁不一致且无法修复
     * @return 0=成功, 负数=错误码
     */
    FORCE_INLINE int32 lock(int32 &o_badlock)
    {
      int32 ret = pthread_mutex_lock(&mlock);
      if (likely(ret == 0))
      {
        o_badlock = 0;
        return 0;
      }
      else if (ret == EOWNERDEAD)
      {
        if (pthread_mutex_consistent(&mlock) != 0)
        {
          int32 ur = pthread_mutex_unlock(&mlock);
          if (ur != 0 && ur != EPERM)
          {
            std::fprintf(stderr, "cmutex_proc::lock unlock on inconsistent failed: %d\n", ur);
          }
          o_badlock = 2;
          return LBERR_LOCK_NOT_CONSIST;
        }
        o_badlock = 1;
        return 0;
      }
      else if (ret == ENOTRECOVERABLE)
      {
        o_badlock = 2;
        return LBERR_LOCK_NOT_RECOVER;
      }
      o_badlock = 0;
      return LBERR_LOCK_TAKE_FAIL;
    }

    /**
     * @brief 释放锁
     */
    FORCE_INLINE void unlock() { pthread_mutex_unlock(&mlock); }

    /**
     * @brief 初始化锁
     * @param[in] betweenthread 是否线程间共享（0=进程内，1=线程间）
     * @return 0=成功, 负数=错误码
     */
    int32 init(int32 betweenthread = 0);

    /**
     * @brief 销毁锁
     */
    void destroy();

  private:
    pthread_mutex_t mlock; ///< 进程间互斥锁对象
  };

} // namespace lb_common

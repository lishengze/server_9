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
 * - 进程间读写锁（rwlock_proc）
 * - 单写多读进程锁（swlock_proc）
 * - RAII锁守卫（clock_guard）
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "matomic.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>

namespace lb_common {

/**
 * @brief 自旋锁最大自旋次数
 */
#define SPIN_LOCK_BUSY_COUNT 30000

/**
 * @brief 原子操作自旋锁,确保为 POD 类型
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
class atomic_lock {
public:
  /**
   * @brief 获取锁
   *
   * 自旋等待直到获取锁，失败时调用CPU_PAUSE
   */
  FORCE_INLINE void lock() {
    int i = 0;
    do {
      int t = 0;
      if (atomic_cas32(&mlock, &t, 1))
        break;
      i++;
      if (i == SPIN_LOCK_BUSY_COUNT) {
        CPU_PAUSE();
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

private:
  int32 mlock; ///< 锁状态：0=未锁定，1=已锁定
};

/**
 * @brief 原子操作读写锁,确保为 POD 类型
 *
 * 使用原子操作实现的读写锁，支持多读者单写者。
 * 写锁优先：获取写锁时，会阻止新的读者进入。
 *
 * @note 适用于单进程多线程环境
 * @note 写锁获取会阻塞后续的读锁获取
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
class atomic_rwlock {
public:
#define RWLOCK_READ_MASK  0x0FFFFFFF
#define RWLOCK_WRITE_WAIT 0x10000000
#define RWLOCK_WRITE_ING  0x20000000
#define RWLOCK_WRITE_MASK 0x30000000

  /**
   * @brief 获取读锁
   *
   * 多个线程可以同时获取读锁
   */
  FORCE_INLINE void read_lock() {
    int t = mlock;
    int i = 0;
    do {
      if ((t & RWLOCK_WRITE_MASK) == 0) {
        if (atomic_cas32(&mlock, &t, t + 1))
          break;
      } else {
        t = mlock;
      }
      i++;
      if (i == SPIN_LOCK_BUSY_COUNT) {
        CPU_PAUSE();
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
  FORCE_INLINE void write_lock() {
    int t = mlock;
    int i = 0;
    do {
      if (t == 0) {
        if (atomic_cas32(&mlock, &t, RWLOCK_WRITE_MASK))
          return;
      } else if ((t & RWLOCK_WRITE_MASK) == 0) {
        if (atomic_cas32(&mlock, &t, t | RWLOCK_WRITE_WAIT))
          break;
      } else {
        t = mlock;
      }
      i++;
      if (i == SPIN_LOCK_BUSY_COUNT) {
        CPU_PAUSE();
        i = 0;
      }
    } while (true);

    i = 0;
    do {
      t = atomic_load32(&mlock);
      if ((t & RWLOCK_READ_MASK) == 0) {
        if (atomic_cas32(&mlock, &t, t | RWLOCK_WRITE_ING))
          break;
      }
      i++;
      if (i == SPIN_LOCK_BUSY_COUNT) {
        CPU_PAUSE();
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
class cspinlock {
public:
  /**
   * @brief 获取锁
   */
  FORCE_INLINE void lock() { assert(0 == pthread_spin_lock(&mlock)); }

  /**
   * @brief 释放锁
   */
  FORCE_INLINE void unlock() { pthread_spin_unlock(&mlock); }

  /**
   * @brief 初始化锁
   */
  FORCE_INLINE void init() { assert(0 == pthread_spin_init(&mlock, PTHREAD_PROCESS_PRIVATE)); }

  /**
   * @brief 构造函数，自动初始化
   */
  cspinlock() { assert(0 == pthread_spin_init(&mlock, PTHREAD_PROCESS_PRIVATE)); }

  /**
   * @brief 析构函数，销毁锁
   */
  ~cspinlock() { pthread_spin_destroy(&mlock); }

  cspinlock(const cspinlock &) = delete;
  cspinlock &operator=(const cspinlock &) = delete;
  cspinlock(cspinlock &) = delete;
  cspinlock &operator=(cspinlock &) = delete;

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
class cmutex {
public:
  /**
   * @brief 获取锁
   */
  FORCE_INLINE void lock() { assert(0 == pthread_mutex_lock(&mlock)); }

  /**
   * @brief 释放锁
   */
  FORCE_INLINE void unlock() { pthread_mutex_unlock(&mlock); }

  /**
   * @brief 初始化锁
   */
  FORCE_INLINE void init() { assert(0 == pthread_mutex_init(&mlock, NULL)); }

  /**
   * @brief 构造函数，自动初始化
   */
  cmutex() { assert(0 == pthread_mutex_init(&mlock, NULL)); }

  /**
   * @brief 析构函数，销毁锁
   */
  ~cmutex() { pthread_mutex_destroy(&mlock); }

  cmutex(const cmutex &) = delete;
  cmutex &operator=(const cmutex &) = delete;
  cmutex(cmutex &) = delete;
  cmutex &operator=(cmutex &) = delete;

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
template <class T> class clock_guard {
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
class rwlock {
public:
  /**
   * @brief 获取写锁
   */
  FORCE_INLINE void write_lock() { assert(0 == pthread_rwlock_wrlock(&mlock)); }

  /**
   * @brief 释放写锁
   */
  FORCE_INLINE void write_unlock() { pthread_rwlock_unlock(&mlock); }

  /**
   * @brief 获取读锁
   */
  FORCE_INLINE void read_lock() { assert(0 == pthread_rwlock_rdlock(&mlock)); }

  /**
   * @brief 释放读锁
   */
  FORCE_INLINE void read_unlock() { pthread_rwlock_unlock(&mlock); }

  /**
   * @brief 初始化锁
   */
  void init() { mlock = PTHREAD_RWLOCK_INITIALIZER; }

  /**
   * @brief 构造函数
   */
  rwlock() { mlock = PTHREAD_RWLOCK_INITIALIZER; }

  /**
   * @brief 析构函数
   */
  ~rwlock() { pthread_rwlock_destroy(&mlock); }

  rwlock(const rwlock &) = delete;
  rwlock &operator=(const rwlock &) = delete;
  rwlock(rwlock &) = delete;
  rwlock &operator=(rwlock &) = delete;

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
class cmutex_proc {
public:
  /**
   * @brief 获取锁
   * @return 0=成功, 负数=错误码
   */
  FORCE_INLINE int32 lock() {
    int32 ret = pthread_mutex_lock(&mlock);
    if (likely(ret == 0)) {
      return 0;
    } else if (ret == EOWNERDEAD) {
      if (pthread_mutex_consistent(&mlock) != 0) {
        pthread_mutex_unlock(&mlock);
        return LBERR_LOCK_NOT_CONSIST;
      }
      return 0;
    } else if (ret == ENOTRECOVERABLE) {
      return LBERR_LOCK_NOT_RECOVE;
    }
    return LBERR_LOCK_TAKE_FAIL;
  }

  /**
   * @brief 获取锁（带锁状态输出）
   * @param[out] o_badlock 输出参数: 1=发生不一致并修复, 2=锁不一致且无法修复
   * @return 0=成功, 负数=错误码
   */
  FORCE_INLINE int32 lock(int32 &o_badlock) {
    int32 ret = pthread_mutex_lock(&mlock);
    if (likely(ret == 0)) {
      o_badlock = 0;
      return 0;
    } else if (ret == EOWNERDEAD) {
      if (pthread_mutex_consistent(&mlock) != 0) {
        pthread_mutex_unlock(&mlock);
        o_badlock = 2;
        return LBERR_LOCK_NOT_CONSIST;
      }
      o_badlock = 1;
      return 0;
    } else if (ret == ENOTRECOVERABLE) {
      o_badlock = 2;
      return LBERR_LOCK_NOT_RECOVE;
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

/**
 * @brief 单写多读进程间锁
 *
 * 写优先的进程间读写锁。写发生占位后，新来的读者将等待。
 * 锁对象必须存储在共享内存中。
 *
 * @note 适用于多进程环境
 * @note 写锁优先：等待中的写锁会阻塞新的读锁
 * @note 使用信号量实现
 *
 * @example
 * @code
 * swlock_proc* lock = (swlock_proc*)shm_addr;
 * lock->init();
 *
 * lock->read_lock();
 * // 读操作
 * lock->read_unlock();
 *
 * lock->write_lock();
 * // 写操作
 * lock->write_unlock();
 * @endcode
 */
class swlock_proc {
public:
  /**
   * @brief 读等待锁状态
   */
#define SWLOCK_READ_WAIT_LOCK  0x1000000
#define SWLOCK_WRITE_WAIT_LOCK 0x2000000
#define SWLOCK_INIT_WAIT_ING   0x4000000
#define SWLOCK_INIT_STATE_OK   0x8000000

#define SWLOCK_STATE_TYPE_MASK  0xF000000
#define SWLOCK_READ_ONLINE_MASK 0xffffff

  /**
   * @brief 获取写锁
   * @return 0=成功, 负数=错误码
   */
  int32 write_lock();

  /**
   * @brief 等待写锁（带超时）
   * @param[in] ms 超时毫秒数
   * @return 0=成功, 1=超时, 负数=错误码
   */
  int32 write_wait(int32 ms);

  /**
   * @brief 释放写锁
   */
  FORCE_INLINE void write_unlock() {
    sem_post(&wlock);
    if (wlock_type == 1)
      sem_post(&rlock);
    clear_write();
  }

  /**
   * @brief 获取读锁
   *
   * 返回值说明:
   * - 1: 获得锁锁住
   * - 0: 只增引用计数锁住
   * - <0: 出错
   *
   * @return 状态码
   */
  int32 read_lock();

  /**
   * @brief 等待读锁（带超时）
   * @param[in] ms 超时毫秒数
   * @return 0=成功, 1=超时, 负数=错误码
   */
  int32 read_wait(int32 ms);

  /**
   * @brief 释放读锁
   *
   * 返回值说明:
   * - 1: 释放了锁
   * - 0: 只减引用计数
   *
   * @return 状态码
   */
  int32 read_unlock();

  /**
   * @brief 初始化锁
   * @param[in] shm_exist 共享内存是否已存在（0=新建，1=已存在）
   * @return 0=成功, 负数=错误码
   *
   * @note 同时调用初始化，只有一个会实际初始化，其他自旋等待其结束
   */
  int32 init(int32 shm_exist = 0);

  /**
   * @brief 销毁锁
   */
  void destroy();

  /**
   * @brief 重新初始化锁
   *
   * 在确定进程core后或者多次尝试超时锁超时后，可旁路辅助的检查恢复
   *
   * @return 0=成功, 负数=错误码
   */
  int32 reinit();

private:
  int32 statref;    ///< 在读数量+写状态
  int32 wlock_type; ///< 锁类型
  sem_t wlock;      ///< 写锁信号量
  sem_t rlock;      ///< 读锁信号量

  /**
   * @brief 设置写锁状态
   * @return 0=成功, 负数=错误码
   */
  int32 set_write();

  /**
   * @brief 清除写锁状态
   */
  void clear_write();

  /**
   * @brief 设置读锁状态
   * @return 0=成功, 负数=错误码
   */
  int32 set_read();

  /**
   * @brief 清除读锁状态
   */
  void clear_read();

  /**
   * @brief 获取信号量锁
   * @param[in] psem 信号量指针
   * @return 0=成功, 负数=错误码
   */
  FORCE_INLINE int32 lock_sem(sem_t *psem) {
    while (sem_wait(psem) < 0) {
      if (errno != EINTR) {
        return -errno;
      }
    }
    return 0;
  }

  /**
   * @brief 带超时的获取信号量锁
   * @param[in] psem 信号量指针
   * @param[in] ts 超时时间
   * @return 1=成功, 0=超时, 负数=错误码
   */
  FORCE_INLINE int32 lock_wait_sem(sem_t *psem, const struct timespec *ts) {
    do {
      if (sem_timedwait(psem, ts) == 0)
        return 1;
      if (errno == ETIMEDOUT)
        return 0;
      if (errno != EINTR) {
        break;
      }
    } while (TRUE);
    return -errno;
  }

  /**
   * @brief 设置初始化状态
   * @return 0=成功, 负数=错误码
   */
  int32 set_init();

  /**
   * @brief 清除初始化状态
   * @param[in] isok 是否成功
   */
  void clear_init(int32 isok);

  /**
   * @brief 设置销毁状态
   * @return 0=成功, 负数=错误码
   */
  int32 set_destroy();
};

} // namespace lb_common

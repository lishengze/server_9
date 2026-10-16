/**
 * @file mlock.cpp
 * @brief 锁机制实现
 *
 * @see mlock.h
 */

#include "mlock.h"

namespace lb_common
{

  int32 cmutex_proc::init(int32 betweenthread)
  {
    // cmutex_proc 同时 init() 会触发 pthread_mutex_init 在已初始化 mutex 上的
    // 未定义行为（Linux glibc 通常返回 EBUSY）。
    //
    // cmutex_proc **不内置串行化机制**，调用方必须确保 init() 只在一个进程中
    // 调用一次，或使用外部机制（如 flock、文件锁、共享内存标志位）做进程间串行化。
    //
    // 推荐用法：写进程 init()，其他进程只 lock/unlock。

    if (betweenthread == 1)
    {
      // 改为运行时检查 + 错误返回。
      int32 ret = pthread_mutex_init(&mlock, NULL);
      if (ret != 0)
      {
        std::fprintf(stderr, "cmutex_proc::init (thread) failed: %d\n", ret);
        return LBERR_ATTR_INIT_FAIL;
      }
      return 0;
    }

    // 初始化互斥锁属性
    pthread_mutexattr_t lock_attr;
    if (pthread_mutexattr_init(&lock_attr) != 0)
    {
      return LBERR_ATTR_INIT_FAIL;
    }
    // 设置为进程间共享
    if (pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED) != 0)
    {
      pthread_mutexattr_destroy(&lock_attr);
      return LBERR_ATTR_SET_FAIL;
    }

    // 设置健壮性，以跟踪加锁进程是否存在，若其coredump，锁状态变成不一致，而不会阻塞其他进程
    // 注意：glibc 不定义 _POSIX_THREAD_ROBUST_MUTEXES（POSIX 定义的是 _PRIO_INHERIT / _PROTECT），
#if defined(__linux__) && defined(_GNU_SOURCE)
    if (pthread_mutexattr_setrobust(&lock_attr, PTHREAD_MUTEX_ROBUST) != 0)
    {
      pthread_mutexattr_destroy(&lock_attr);
      return LBERR_ATTR_SET_FAIL;
    }
#endif

    // 用属性初始化互斥锁
    if (pthread_mutex_init(&mlock, &lock_attr) != 0)
    {
      pthread_mutexattr_destroy(&lock_attr);
      return LBERR_OBJ_INIT_FAIL;
    }
    // 销毁属性对象（不再需要）
    pthread_mutexattr_destroy(&lock_attr);
    return 0;
  }
  void cmutex_proc::destroy() { pthread_mutex_destroy(&mlock); }

} // namespace lb_common

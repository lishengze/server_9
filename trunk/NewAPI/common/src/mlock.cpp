/**
 * @file mlock.cpp
 * @brief 锁机制实现
 *
 * @see mlock.h
 */

#include "mlock.h"
#include "mutils.h"
#include <sched.h>

namespace lb_common {

int32 cmutex_proc::init(int32 betweenthread) {
  if (betweenthread == 1) {
    assert(0 == pthread_mutex_init(&mlock, NULL));
    return 0;
  }

  // 初始化互斥锁属性
  pthread_mutexattr_t lock_attr;
  if (pthread_mutexattr_init(&lock_attr) != 0) {
    return LBERR_ATTR_INIT_FAIL;
  }
  // 设置为进程间共享
  if (pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED) != 0) {
    return LBERR_ATTR_SET_FAIL;
  }

  // 设置健壮性,以跟踪加锁进程是否存在，若其coredump，锁状态变成不一致，而不会阻塞其他进程
#ifdef _POSIX_THREAD_ROBUST_MUTEXES
  if (pthread_mutexattr_setrobust(&lock_attr, PTHREAD_ROBUST) != 0) {
    return LBERR_ATTR_SET_FAIL;
  }
#endif

  // 用属性初始化互斥锁
  if (pthread_mutex_init(&mlock, &lock_attr) != 0) {
    return LBERR_OBJ_INIT_FAIL;
  }
  // 销毁属性对象（不再需要）
  pthread_mutexattr_destroy(&lock_attr);
  return 0;
}
void cmutex_proc::destroy() { pthread_mutex_destroy(&mlock); }

int32 swlock_proc::write_lock() {
  int32 t = set_write();
  if (unlikely(t < 0)) {
    return LBERR_OBJ_INIT_FAIL;
  }

  int32 ret;
  if ((t & SWLOCK_READ_ONLINE_MASK) == 0) {
    ret = lock_sem(&wlock);
    if (unlikely(ret < 0)) {
      clear_write();
      return LBERR_LOCK_TAKE_FAIL;
    }
    wlock_type = 0;
  } else {
    ret = lock_sem(&rlock);
    if (unlikely(ret < 0)) {
      clear_write();
      return LBERR_LOCK_TAKE_FAIL;
    }
    ret = lock_sem(&wlock);
    if (unlikely(ret < 0)) {
      sem_post(&rlock);
      clear_write();
      return LBERR_LOCK_TAKE_FAIL;
    }
    wlock_type = 1;
  }
  return 0;
}
int32 swlock_proc::set_write() {
  int32 t = atomic_load32(&statref);
  do {
    if (unlikely((t & SWLOCK_INIT_WAIT_ING) != 0)) {
      CPU_PAUSE();
      t = atomic_load32(&statref);
      continue;
    }
    if (likely((t & SWLOCK_INIT_STATE_OK) != 0)) {
      if (atomic_cas32_weak(&statref, &t, (t | SWLOCK_WRITE_WAIT_LOCK)))
        break;
    } else {
      return -1;
    }
  } while (true);
  return t;
}
void swlock_proc::clear_write() {
  int32 t = atomic_load32(&statref);
  while ((t & SWLOCK_WRITE_WAIT_LOCK) != 0) {
    if (atomic_cas32_weak(&statref, &t, (t ^ SWLOCK_WRITE_WAIT_LOCK)))
      break;
  }
}

int32 swlock_proc::read_lock() {
  int32 ret = set_read();
  if (unlikely(ret < 0)) {
    return LBERR_OBJ_INIT_FAIL;
  }

  do { // 可能惊群
    ret = lock_sem(&rlock);
    if (unlikely(ret < 0)) {
      return LBERR_LOCK_TAKE_FAIL;
    }

    ret = 0;
    int32 t = atomic_load32(&statref);
    while ((t & SWLOCK_WRITE_WAIT_LOCK) == 0) {
      int32 n = (t + 1) | SWLOCK_READ_WAIT_LOCK;
      if (atomic_cas32(&statref, &t, n)) {
        if ((t & SWLOCK_READ_ONLINE_MASK) == 0)
          ret = 1;
        else
          ret = 2;
        break;
      }
    }

    if (ret == 0) {
      sem_post(&rlock);
      sched_yield(); // 给写机会，也减少自身参与抢占
    } else {
      break;
    }
  } while (true);

  if (ret == 1) {
    ret = lock_sem(&wlock);
    if (unlikely(ret < 0)) {
      atomic_fetch_sub32(&statref, 1);
      sem_post(&rlock);
      clear_read(); // 有机会导致初始化问题
      return LBERR_LOCK_TAKE_FAIL;
    }
    return 1;
  }

  sem_post(&rlock);
  return 0;
}
int32 swlock_proc::set_read() {
  int32 t = atomic_load32(&statref);
  do {
    if (unlikely((t & SWLOCK_INIT_WAIT_ING) != 0)) {
      sched_yield();
      t = atomic_load32(&statref);
      continue;
    }
    if (likely((t & SWLOCK_INIT_STATE_OK) != 0)) {
      if ((t & SWLOCK_READ_WAIT_LOCK) != 0)
        break;
      if (atomic_cas32_weak(&statref, &t, (t | SWLOCK_READ_WAIT_LOCK)))
        break;
    } else {
      return -1;
    }
  } while (true);
  return t;
}
void swlock_proc::clear_read() {
  int32 t = atomic_load32(&statref);
  while ((t & SWLOCK_READ_WAIT_LOCK) != 0) {
    if ((t & SWLOCK_READ_ONLINE_MASK) == 0) {
      if (atomic_cas32(&statref, &t, (t ^ SWLOCK_READ_WAIT_LOCK)))
        break;
    } else {
      if (atomic_cas32(&statref, &t, ((t ^ SWLOCK_READ_WAIT_LOCK) | SWLOCK_READ_ONLINE_MASK)))
        break;
    }
  }
}
int32 swlock_proc::read_unlock() {
  if (1 == atomic_fetch_sub32(&statref, 1)) {
    sem_post(&wlock);
    clear_read();
    return 1;
  }
  return 0;
}

int32 swlock_proc::write_wait(int32 ms) {
  struct timespec ts;
  // 获取当前绝对时间并设置超时时间
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
    return LBERR_TIME_TAKE_FAIL;
  }
  if (ms < 1000) {
    ts.tv_nsec += (ms * 1000000);
  } else {
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += ((ms % 1000) * 1000000);
  }

  int32 t = set_write();
  if (unlikely(t < 0)) {
    return LBERR_OBJ_INIT_FAIL;
  }

  int32 ret;
  if ((t & SWLOCK_READ_ONLINE_MASK) == 0) {
    ret = lock_wait_sem(&wlock, &ts);
    if (unlikely(ret <= 0)) {
      clear_write();
      if (ret == 0) {
        return LBERR_TIME_WAIT_OUT;
      } else {
        return LBERR_LOCK_TAKE_FAIL;
      }
    }
    wlock_type = 0;
  } else {
    ret = lock_wait_sem(&rlock, &ts);
    if (unlikely(ret <= 0)) {
      clear_write();
      if (ret == 0) {
        return LBERR_TIME_WAIT_OUT;
      } else {
        return LBERR_LOCK_TAKE_FAIL;
      }
    }
    ret = lock_wait_sem(&wlock, &ts);
    if (unlikely(ret <= 0)) {
      sem_post(&rlock);
      clear_write();
      if (ret == 0) {
        return LBERR_TIME_WAIT_OUT;
      } else {
        return LBERR_LOCK_TAKE_FAIL;
      }
    }
    wlock_type = 1;
  }
  return 0;
}

int32 swlock_proc::read_wait(int32 ms) {
  struct timespec ts;
  // 获取当前绝对时间并设置超时时间
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
    return LBERR_TIME_TAKE_FAIL;
  }
  if (ms < 1000) {
    ts.tv_nsec += (ms * 1000000);
  } else {
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += ((ms % 1000) * 1000000);
  }

  int32 ret = set_read();
  if (unlikely(ret < 0)) {
    return LBERR_OBJ_INIT_FAIL;
  }

  do { // 可能惊群
    ret = lock_wait_sem(&rlock, &ts);
    if (unlikely(ret <= 0)) {
      clear_read(); // 有机会导致初始化问题
      if (ret == 0) {
        return LBERR_TIME_WAIT_OUT;
      } else {
        return LBERR_LOCK_TAKE_FAIL;
      }
    }

    ret = 0;
    int32 t = atomic_load32(&statref);
    while ((t & SWLOCK_WRITE_WAIT_LOCK) == 0) {
      int32 n = (t + 1) | SWLOCK_READ_WAIT_LOCK;
      if (atomic_cas32_weak(&statref, &t, n)) {
        if ((t & SWLOCK_READ_ONLINE_MASK) == 0)
          ret = 1;
        else
          ret = 2;
        break;
      }
    }

    if (ret == 0) {
      sem_post(&rlock);
      sched_yield(); // 给写机会，也减少自身参与抢占
    } else {
      break;
    }
  } while (true);

  if (ret == 1) {
    ret = lock_wait_sem(&wlock, &ts);
    if (unlikely(ret <= 0)) {
      atomic_fetch_sub32(&statref, 1);
      sem_post(&rlock);
      clear_read();
      if (ret == 0) {
        return LBERR_TIME_WAIT_OUT;
      } else {
        return LBERR_LOCK_TAKE_FAIL;
      }
    }
    return 1;
  }

  sem_post(&rlock);
  return 0;
}

int32 swlock_proc::set_init() {
  int32 t = atomic_load32(&statref);
  do {
    if ((t & SWLOCK_INIT_STATE_OK) == 0) {
      if (t == 0) {
        if (atomic_cas32(&statref, &t, SWLOCK_INIT_WAIT_ING))
          break;
      } else {
        sched_yield();
      }
    } else {
      return 0;
    }
  } while (true);
  return 1;
}
void swlock_proc::clear_init(int32 isok) {
  int32 t = atomic_load32(&statref);
  while ((t & SWLOCK_INIT_WAIT_ING) != 0) {
    int32 n = t ^ SWLOCK_INIT_WAIT_ING;
    if (isok == 1)
      n = n | SWLOCK_INIT_STATE_OK;
    if (atomic_cas32(&statref, &t, n))
      break;
  }
}
int32 swlock_proc::set_destroy() {
  int32 t = atomic_load32(&statref);
  int32 ver = 0;
  do {
    int32 n = (t & (SWLOCK_STATE_TYPE_MASK | SWLOCK_READ_ONLINE_MASK));
    if (n == SWLOCK_INIT_STATE_OK) {
      if (ver == 1)
        return 0;

      int32 wlock_val = 0;
      int32 rlock_val = 0;
      sem_getvalue(&wlock, &wlock_val);
      sem_getvalue(&rlock, &rlock_val);

      if (wlock_val == 1 && rlock_val == 1) {
        return 0;
      }
      if (atomic_cas32(&statref, &t, SWLOCK_INIT_WAIT_ING))
        break;
    } else if (n == 0) {
      return 0;
    } else if (n == SWLOCK_INIT_WAIT_ING) {
      ver = 1;
      sched_yield();
    } else {
      comm_utils::sleep_us(50);
    }
    t = atomic_load32(&statref);
  } while (true);
  return 1;
}

int32 swlock_proc::init(int32 shm_exist) {
  if (shm_exist == 0) {
    atomic_store32(&statref, 0);
    wlock_type = 0;
  }

  if (0 == set_init())
    return 0;

  atomic_store32(&statref, 0);
  wlock_type = 0;

  // 初始化信号量（只需要初始化一次）
  if (sem_init(&wlock, 1, 1) == -1) {
    clear_init(0);
    return LBERR_OBJ_INIT_FAIL;
  }
  if (sem_init(&rlock, 1, 1) == -1) {
    sem_destroy(&rlock);
    clear_init(0);
    return LBERR_OBJ_INIT_FAIL;
  }

  clear_init(1);
  return 0;
}
void swlock_proc::destroy() {
  if (0 == set_destroy())
    return;
  sem_destroy(&wlock);
  sem_destroy(&rlock);
  clear_init(0);
}
int32 swlock_proc::reinit() {
  int32 wlock_val = 0;
  int32 rlock_val = 0;
  sem_getvalue(&wlock, &wlock_val);
  sem_getvalue(&rlock, &rlock_val);

  if (wlock_val == 1 && rlock_val == 1) {
    return 0;
  }

  destroy();
  return init(1);
}

} // namespace lb_common

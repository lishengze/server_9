#include "time_thread.h"
#include "comm_errno.h"
#include "thread_comm.h"
// #include "mutils.h"
#include "matomic.h"
#include <fcntl.h>
#include <iostream>
#include <limits.h>
#include <pthread.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace lb_common
{

  void timer_thread::deal_event()
  {
    int64 ttime;
    read(timefd, &ttime, sizeof(int64));

    uint32 tinterval = 0;
    list.loop_timer(tinterval);
    if (tinterval == 0)
      return;

    struct itimerspec newtm;
    newtm.it_value.tv_nsec = (tinterval % 1000000) * 1000;
    newtm.it_value.tv_sec = tinterval / 1000000;
    newtm.it_interval.tv_nsec = 30000000;
    newtm.it_interval.tv_sec = 0;
    if (timerfd_settime(timefd, 0, &newtm, NULL) < 0)
    {
      // 改为调 deal_error 尝试重新初始化 timerfd 并重设超时。
      std::cerr << "timerfd_settime error in deal_event, retrying..." << std::endl;
      deal_error();
      return;
    }

    epollfd.mode_wake(*this);
  }

  void timer_thread::deal_error()
  {
    if (timefd != -1)
    {
      close(timefd);
      timefd = -1;
    }

    if (init_timer() < 0)
    {
      assert(false);
      return;
    }
    // 下次 timerfd 就绪时由 epoll 主循环再触发 deal_event。
    uint32 tinterval = list.get_interval();
    if (tinterval > 0)
    {
      struct itimerspec newtm;
      newtm.it_value.tv_nsec = (tinterval % 1000000) * 1000;
      newtm.it_value.tv_sec = tinterval / 1000000;
      newtm.it_interval.tv_nsec = 30000000;
      newtm.it_interval.tv_sec = 0;
      if (timerfd_settime(timefd, 0, &newtm, NULL) == 0)
      {
        epollfd.mode_wake(*this);
      }
    }
  }

  int32 timer_thread::init_timer()
  {
    if ((timefd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC)) < 0)
    {
      return LBERR_OBJ_OPEN_FAIL;
    }
    int rc = 0;

    if ((rc = fcntl(timefd, F_GETFL, 0)) < 0)
    {
      close(timefd);
      timefd = -1;
      return LBERR_ATTR_GET_FAIL;
    }
    rc |= O_NONBLOCK;
    if ((rc = fcntl(timefd, F_SETFL, rc)) < 0)
    {
      close(timefd);
      timefd = -1;
      return LBERR_ATTR_SET_FAIL;
    }

    set_event(0, 0);
    if ((rc = epollfd.add_wake(*this)) < 0)
    {
      close(timefd);
      timefd = -1;
      return rc;
    }

    return 0;
  }

  void timer_thread::destroy()
  {
    if (timefd > 0)
    {
      close(timefd);
      timefd = -1;
    }

    epollfd.destroy();
    list.clear();
    // run() 在 pthread_create 失败时调 destroy()，但 init() 已把 state 置 1，
    // destroy 不重置 → state 仍为 1。再次 run() 会通过 `state == 1` 校验，对一个
    // 已 teardown 的对象再次 pthread_create。把 state 重置为 0，使再 run() 直接
    // 返回 LBERR_OBJ_STATE_LIMIT，强制用户重新 init()。
    atomic_store32(&state, 0);
  }

  int32 timer_thread::init()
  {
    int32 ret = epollfd.init(2); // 1 timer fd + 1 change_event
    if (ret < 0)
    {
      return ret;
    }

    if ((ret = init_timer()) < 0)
    {
      destroy();
      return ret;
    }
    state = 1;
    return 0;
  }

  void *_f_timer_thread(void *arg)
  {
    timer_thread *info = reinterpret_cast<timer_thread *>(arg);

    mask_thread_signal();
    info->deal();
    return NULL;
  }

  int32 timer_thread::run()
  {
    // 避免重复 run() 创建第二个线程并覆盖 threadid（第一个线程成孤儿无法 join）
    if (atomic_load32(&state) != 1)
    {
      return LBERR_OBJ_STATE_LIMIT;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (::pthread_create(&threadid, &attr, &_f_timer_thread, reinterpret_cast<void *>(this)) == 0)
    {
      // pthread 创建成功，状态由 deal() 内部 CAS 从 1 改为 2
      pthread_attr_destroy(&attr);
      return 0;
    }
    pthread_attr_destroy(&attr);
    destroy();
    return LBERR_OBJ_OPEN_FAIL;
  }

  void timer_thread::deal()
  {
    int32 t = 1;
    // CAS 把 state 从 1（已初始化）改为 2（运行中）
    // 如果 state 不是 1（已被其他线程设为 2 或其他状态），CAS 失败，直接返回
    if (!atomic_cas32(&state, &t, 2))
      return;
    while (atomic_load32(&state) == 2 && g_signal_stopctl == -1)
    {
      epollfd.wait(100);
    }
    state = 4;
  }

  void timer_thread::to_stop()
  {
    int32 t;
    while (true)
    {
      t = atomic_load32(&state);
      if (t == 2)
      {
        if (!atomic_cas32(&state, &t, 3))
          continue;
      }
      break;
    }
  }

  void timer_thread::join()
  {
    to_stop();
    if (threadid > 0 && state > 1)
    {
      pthread_join(threadid, NULL);
      threadid = 0;
    }
  }

  int32 timer_thread::add_timer(time_event_info &eventinfo)
  {
    int32 ret = list.add_timer(eventinfo);
    if (ret < 0)
      return ret;

    uint32 tinterval = list.get_interval();
    struct itimerspec newtm;
    newtm.it_value.tv_nsec = (tinterval % 1000000) * 1000;
    newtm.it_value.tv_sec = tinterval / 1000000;
    newtm.it_interval.tv_nsec = 30000000;
    newtm.it_interval.tv_sec = 0;
    if (timerfd_settime(timefd, 0, &newtm, NULL) < 0)
    {
      // timerfd_settime 失败：回滚已添加的定时器，避免泄漏
      list.delete_timer(eventinfo.op, eventinfo.timer);
      std::cerr << "timerfd_settime error" << std::endl;
      return LBERR_ATTR_SET_FAIL;
    }

    epollfd.mode_wake(*this);
    return 0;
  }

} // namespace lb_common

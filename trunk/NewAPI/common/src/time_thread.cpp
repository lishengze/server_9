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

namespace lb_common {

void timer_thread::deal_event() {
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
  if (timerfd_settime(timefd, 0, &newtm, NULL) < 0) {
    std::cerr << "timerfd_settime error" << std::endl;
  }

  epollfd.mode_wake();
}

void timer_thread::deal_error() {
  if (timefd != -1) {
    close(timefd);
    timefd = -1;
  }

  if (init_timer() < 0) {
    assert(false);
    return;
  }
  deal_event();
}

int32 timer_thread::init_timer() {
  if ((timefd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC)) < 0) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  int rc = 0;

  if ((rc = fcntl(timefd, F_GETFL, 0)) < 0) {
    return LBERR_ATTR_GET_FAIL;
  }
  rc |= O_NONBLOCK;
  if ((rc = fcntl(timefd, F_SETFL, rc)) < 0) {
    return LBERR_ATTR_SET_FAIL;
  }

  set_event(0, 0);
  if ((rc = epollfd.add_wake(*this)) < 0) {
    return rc;
  }

  return 0;
}

void timer_thread::destroy() {
  if (timefd > 0) {
    close(timefd);
    timefd = -1;
  }

  epollfd.destroy();
  list.clear();
}

int32 timer_thread::init() {
  int32 ret = epollfd.init();
  if (ret < 0) {
    return ret;
  }

  if ((ret = init_timer()) < 0) {
    destroy();
    return ret;
  }
  state = 1;
  return 0;
}

void *_f_timer_thread(void *arg) {
  timer_thread *info = reinterpret_cast<timer_thread *>(arg);

  mask_thread_signal();
  info->deal();
  return NULL;
}

int32 timer_thread::run() {
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (::pthread_create(&threadid, &attr, &_f_timer_thread, reinterpret_cast<void *>(this)) == 0) {
    return 0;
  }

  return -1;
}

void timer_thread::deal() {
  int32 t = 1;
  if (atomic_cas32(&state, &t, 2))
    return;
  while (state == 2 && g_signal_stopctl == -1) {
    epollfd.wait(100);
  }
  state = 4;
}

void timer_thread::to_stop() {
  int32 t;
  while (true) {
    t = atomic_load32(&state);
    if (t == 2) {
      if (!atomic_cas32(&state, &t, 3))
        continue;
    }
    break;
  }
}

void timer_thread::join() {
  to_stop();
  if (threadid > 0 && state > 1)
    pthread_join(threadid, NULL);
}

int32 timer_thread::add_timer(time_event_info &eventinfo) {
  int32 ret = list.add_timer(eventinfo);
  if (ret < 0)
    return ret;

  uint32 tinterval = list.get_interval();
  struct itimerspec newtm;
  newtm.it_value.tv_nsec = (tinterval % 1000000) * 1000;
  newtm.it_value.tv_sec = tinterval / 1000000;
  newtm.it_interval.tv_nsec = 30000000;
  newtm.it_interval.tv_sec = 0;
  if (timerfd_settime(timefd, 0, &newtm, NULL) < 0) {
    std::cerr << "timerfd_settime error" << std::endl;
  }

  epollfd.mode_wake();
  return 0;
}

} // namespace lb_common

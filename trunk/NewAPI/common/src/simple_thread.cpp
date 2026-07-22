
#include "simple_thread.h"
#include "comm_errno.h"
#include "thread_comm.h"

#include <fcntl.h>
#include <sched.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/types.h>

namespace lb_common {

void simple_thread::trigger() {
  if (waitflag == 1) {
    int64 tc = 1;
    write(evtfd, &tc, sizeof(int64));
  }
}

void simple_thread::busy_run() {
  waitflag = 0;
  while (state == 2 && g_signal_stopctl == -1) {
    do_work();
  }
  state = 1;
}

void simple_thread::busy_wait_run() {
  int64 event_data = 0;
  struct epoll_event epevent;
  int32 waitret;
  int32 curround;

  waitflag = 1;
  while (state == 2 && g_signal_stopctl == -1) {
    waitret = epoll_wait(epollfd, &epevent, 1, pollwaitms);
    if (waitret > 0) {
      read(evtfd, &event_data, sizeof(int64));
    } else if (waitret < 0) {
      continue;
    }

    waitflag = 0;
    curround = 0;
    while (curround < loopround) {
      do_work();
      curround++;
    }
    waitflag = 1;

    if (need_work()) {
      event_data = 1;
      write(evtfd, &event_data, sizeof(int64));
    }
  }
  state = 1;
}

void *simple_thread::simple_thread_func(void *arg) {
  mask_thread_signal();

  simple_thread *tpth = reinterpret_cast<simple_thread *>(arg);
  // simple_thread *tpth = static_cast<simple_thread *>(arg);
  if (tpth->runmod == 1) {
    tpth->busy_run();
  } else {
    tpth->busy_wait_run();
  }

  return NULL;
}

int32 simple_thread::init_th(int32 busyround, int32 tcpuid, int32 pollms) {
  int32 needwakeup = 0;
  state = 0;
  epollfd = -1;
  pollwaitms = 0;
  evtfd = -1;
  if (busyround == 0) {
    waitflag = 0;
    needwakeup = 0;
  } else {
    waitflag = 0;
    needwakeup = 1;
  }

  loopround = busyround;
  if (needwakeup == 0) {
    runmod = 1;
  } else {
    if (loopround < 10)
      loopround = 10;
    runmod = 2;
    pollwaitms = pollms;
  }
  cpuid = tcpuid;
  thid = 0;

  if (runmod == 1) {
    state = 1;
    return 0;
  }

  int32 rc = 0;

  evtfd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);
  if (-1 == evtfd) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  if ((rc = fcntl(evtfd, F_GETFL, 0)) < 0) {
    close(evtfd);
    return LBERR_ATTR_GET_FAIL;
  }
  rc |= O_NONBLOCK;
  if ((rc = fcntl(evtfd, F_SETFL, rc)) < 0) {
    close(evtfd);
    return LBERR_ATTR_SET_FAIL;
  }

  epollfd = epoll_create(4);
  if (epollfd == -1) {
    close(evtfd);
    evtfd = -1;
    return LBERR_OBJ_INIT_FAIL;
  }
  struct epoll_event event_info;
  event_info.events = EPOLLIN;
  event_info.data.ptr = reinterpret_cast<void *>(this);
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evtfd, &event_info) < 0) {
    close(evtfd);
    evtfd = -1;
    close(epollfd);
    epollfd = -1;
    return LBERR_OBJ_ADD_FAIL;
  }

  state = 1;
  return 0;
}

int32 simple_thread::run() {
  int32 ret;
  if (state != 1)
    return LBERR_OBJ_INIT_FAIL;

  int32 tpolicy = 0;
  pthread_attr_t attr;
  struct sched_param sparam;
  pthread_attr_init(&attr);
  memset((void *)&sparam, 0, sizeof(sparam));

  if (-1 != cpuid) {
    cpu_set_t cpu_info;
    CPU_ZERO(&cpu_info);
    CPU_SET(cpuid, &cpu_info);
    pthread_attr_setschedpolicy(&attr, SCHED_RR); // SCHED_FIFO;
    pthread_attr_getschedpolicy(&attr, &tpolicy);
    int32 tmaxpri = sched_get_priority_max(tpolicy);
    // int32 tminpri = sched_get_priority_min(tpolicy);
    // sparam.__sched_priority = tmaxpri;
    sparam.sched_priority = tmaxpri;
    pthread_attr_setschedparam(&attr, &sparam);
    // pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if (0 != pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_info)) {
      return LBERR_ATTR_SET_FAIL;
    }
  }

  state = 2;
  if (::pthread_create(&thid, &attr, simple_thread::simple_thread_func, reinterpret_cast<void *>(this)) == 0) {
    ret = 0;
    pthread_attr_destroy(&attr);
  } else {
    state = 1;
    pthread_attr_destroy(&attr);
    ret = LBERR_OBJ_OPEN_FAIL;
  }

  return ret;
}

void simple_thread::join() {
  int32 ts = state;
  to_stop();

  if (ts != 2)
    return;

  if (thid > 0)
    pthread_join(thid, NULL);
}

} // namespace lb_common

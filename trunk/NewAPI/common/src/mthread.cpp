#include "mthread.h"
#include "matomic.h"
#include "thread_comm.h"

#include <new>
// #include <errno.h>

namespace lb_common {

void mthread::epoll_wait_func() {
  int32 t;
  while (g_signal_stopctl == -1) {
    t = atomic_load32(&state);
    if ((t & DynThread_State_ToStop) == 0) {
      waitpoll.wait(waitms);
    } else {
      break;
    }
  }
  to_stop();
}
void mthread::event_que_func() {
  int32 t;
  while (g_signal_stopctl == -1) {
    t = atomic_load32(&state);
    if ((t & DynThread_State_ToStop) == 0) {
      deal_queue();
    } else {
      break;
    }
  }
  to_stop();
}
void *mthread::_f_mthread_func(void *arg) {
  mthread *info = reinterpret_cast<mthread *>(arg);

  mask_thread_signal();
  if (info->waitms > 0 || info->eventq.get_size() < 32)
    info->epoll_wait_func();
  else
    info->event_que_func();
  return NULL;
}

bool mthread::deal_queue() {
  int32 i = 0;
  int32 tn = eventq_busy_num;
  int32 tctl = 0;
  event_info *pe;
  while (i < tn) {
    if (unlikely(eventq.read_get(pe) == 0)) {
      tctl = 1;
      i++;
      continue;
    }
    pe->op->deal(pe->buf);
    eventq.read_cmt(1);
    tctl = 0;
    i++;
  }
  return tctl == 1;
}
void mthread::deal_event() {
  bool tc = deal_queue();
  if (waitms > 0) {
    if (tc)
      que_wake.reset();
    else
      que_wake.wake();
  }
}

int32 mthread::init_th(int32 id, int32 max_poll_num, int32 eventq_size, int32 eventq_loop_num, int32 twaitms) {
  int32 ret;

  state = 0;
  waitms = twaitms;
  if (waitms < 0) {
    waitms = 100;
  }
  eventq_busy_num = eventq_loop_num;
  load = 0;
  mid = id;
  cpuid = -1;
  thid = -1;
  set_event(0, 0, 1);

  if (waitms > 0 || eventq_size <= 0) {
    ret = waitpoll.init(max_poll_num + 2);
    if (ret < 0)
      return ret;
  }

  if (eventq_size > 0) {
    eventq_size = eventq_size > 64 ? eventq_size : 64;
    ret = eventq.init(eventq_size);
    if (ret < 0) {
      destroy();
      return ret;
    }
    ret = que_wake.init(1);
    if (ret < 0) {
      destroy();
      return ret;
    }
    ret = add_poll_event(*this);
    if (ret < 0) {
      destroy();
      return ret;
    }
  }

  state = DynThread_State_Init;
  return 0;
}

void mthread::destroy() {
  state = DynThread_State_None;
  waitpoll.destroy();
  que_wake.destroy();
  eventq.close();
}

int32 mthread::run() {
  int32 t;
  while (true) {
    t = state;
    if ((t & DynThread_State_ToStop) || t == DynThread_State_None)
      return LBERR_OBJ_STATE_LIMIT;
    else {
      if (atomic_cas32(&state, &t, DynThread_State_Running))
        break;
      continue;
    }
  }

  int32 tpolicy = 0;
  pthread_attr_t attr;
  struct sched_param sparam;
  pthread_attr_init(&attr);
  memset((void *)&sparam, 0, sizeof(sparam));
  if (-1 != cpuid) {
    cpu_set_t cpu_info;
    CPU_ZERO(&cpu_info);
    CPU_SET(cpuid, &cpu_info);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_getschedpolicy(&attr, &tpolicy);
    int32 tmaxpri = sched_get_priority_max(tpolicy);
    sparam.sched_priority = tmaxpri;
    pthread_attr_setschedparam(&attr, &sparam);
    if (0 != pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_info)) {
      pthread_attr_destroy(&attr);
      atomic_store32(&state, DynThread_State_Init);
      return LBERR_ATTR_SET_FAIL;
    }
  }

  if (::pthread_create(&thid, &attr, mthread::_f_mthread_func, reinterpret_cast<void *>(this)) == 0) {
    pthread_attr_destroy(&attr);
    return 0;
  } else {
    atomic_store32(&state, DynThread_State_Init);
    pthread_attr_destroy(&attr);
    return LBERR_OBJ_OPEN_FAIL;
  }
}

void mthread::to_stop() {
  int32 t;
  while (true) {
    t = state;
    if ((t & DynThread_State_ToStop) || t == DynThread_State_None)
      return;
    else {
      if (atomic_cas32(&state, &t, t | DynThread_State_ToStop))
        break;
      continue;
    }
  }
}

void mthread::join() {
  to_stop();
  if (thid > 0) {
    pthread_join(thid, NULL);
  }
}

int32 mthread::get_fd() { return que_wake.get_fd(); }

int32 mthread_pool::assign_thread(mthread *&o_thread, int32 exid, int32 isround) {
  if (num == 1) {
    o_thread = ths;
    return 0;
  }

  mthread *pth;
  int32 t;
  int32 cmp = 0xFFFFFFF;
  int32 i;
  int32 ri = 0;
  bool found = false;
  int32 tass = lastassign >= num ? 0 : lastassign;

  for (i = tass; i < num; i++) {
    if (i == exid)
      continue;
    if (ths[i].is_running()) {
      pth = ths + i;
      if (isround != 0) {
        lastassign = i + 1;
        o_thread = pth;
        return i;
      }
      t = atomic_load32(&pth->load);
      if (t < cmp) {
        cmp = t;
        ri = i;
        found = true;
      }
    }
  }
  for (i = 0; i < tass; i++) {
    if (i == exid)
      continue;
    if (ths[i].is_running()) {
      pth = ths + i;
      if (isround != 0) {
        lastassign = i + 1;
        o_thread = pth;
        return i;
      }
      t = atomic_load32(&pth->load);
      if (t < cmp) {
        cmp = t;
        ri = i;
        found = true;
      }
    }
  }
  if (found) {
    o_thread = ths + ri;
    lastassign = ri + 1;
    return ri;
  }
  // Fallback: return first thread if no running thread found
  o_thread = ths;
  lastassign = 1;
  return 0;
}

int32 mthread_pool::assign_by_cpu(mthread *&o_thread, int32 cpuid) {
  assert(cpuid >= 0);

  mthread *pth;
  int32 i;
  for (i = 0; i < num; i++) {
    pth = ths + i;
    if (cpuid == pth->cpuid) {
      o_thread = pth;
      return i;
    }
  }
  return assign_thread(o_thread, -1, 1);
}

int32 mthread_pool::init(int32 max_thread_num, int32 max_poll_num, int32 eventq_size, int32 eventq_busy_num,
                         int32 twaitms) {
  int32 i;
  int32 ret;
  mthread *pth;

  num = max_thread_num;
  lastassign = 0;

  pth = new (std::nothrow) mthread[max_thread_num];
  if (NULL == pth)
    return LBERR_MEM_ALLOC_FAIL;

  for (i = 0; i < max_thread_num; i++) {
    ret = pth[i].init_th(i, max_poll_num, eventq_size, eventq_busy_num, twaitms);
    if (ret < 0) {
      delete[] pth;
      return ret;
    }
  }
  ths = pth;
  return 0;
}

void mthread_pool::init_set_cpu(int32 thid, int32 tcpuid) {
  assert(thid >= 0 && thid < num);
  ths[thid].init_set_cpu(tcpuid);
}

int32 mthread_pool::run() {
  mthread *pth;
  int32 i;
  int32 ret;
  if (NULL == ths)
    return -1;
  for (i = 0; i < num; i++) {
    pth = ths + i;
    if ((ret = pth->run()) < 0)
      return ret;
  }
  return 0;
}

void mthread_pool::to_stop() {
  mthread *pth;
  if (NULL == ths)
    return;
  for (int32 i = 0; i < num; i++) {
    pth = ths + i;
    pth->join();
  }
}

void mthread_pool::destroy() {
  mthread *pth;
  if (NULL == ths)
    return;
  for (int32 i = 0; i < num; i++) {
    pth = ths + i;
    pth->join();
  }
  delete[] ths;
  ths = NULL;
}

} // namespace lb_common


#include "time_event.h"
#include "comm_errno.h"
#include "mutils.h"

#include <new>

namespace lb_common {

void timer_order_list::add_list(timer_list_it &info) {
  uint64 tm = comm_utils::get_tick();
  tm = tm / 1000;
  info.begintime = tm;
  info.outtime = tm + info.event.timer;

  timer_list_it *tprev = NULL;

  mlock.lock();
  timer_list_it *tpi = first;
  while (NULL != tpi) {
    if (info.outtime >= tpi->outtime) {
      tprev = tpi;
      tpi = tpi->next;
    } else {
      break;
    }
  }
  if (NULL != tprev) {
    info.next = tprev->next;
    tprev->next = &info;
  } else {
    info.next = first;
    first = &info;
  }
  mlock.unlock();
}

int32 timer_order_list::add_timer(time_event_info &eventinfo) {
  int32 waitus = eventinfo.timer;
  if (waitus < TIMER_MIN_UNIT_US) {
    waitus = TIMER_MIN_UNIT_US;
    eventinfo.timer = waitus;
  }

  timer_list_it *pevent = new (std::nothrow) timer_list_it();
  if (NULL == pevent)
    return LBERR_MEM_ALLOC_FAIL;
  pevent->next = NULL;
  pevent->event.op = eventinfo.op;
  pevent->event.isperiod = eventinfo.isperiod;
  pevent->event.timer = eventinfo.timer;
  memcpy(pevent->event.buf, eventinfo.buf, sizeof(eventinfo.buf));

  add_list(*pevent);
  return 0;
}

void timer_order_list::delete_timer(time_event_op *op, int32 timer) {
  timer_list_it *tfind = NULL;
  timer_list_it *tprev = NULL;
  mlock.lock();
  timer_list_it *tpi = first;
  while (NULL != tpi) {
    if (tpi->event.timer == timer && tpi->event.op == op) {
      if (NULL != tprev) {
        tprev->next = tpi->next;
      } else {
        first = tpi->next;
      }
      tfind = tpi;
      break;
    }
    tprev = tpi;
    tpi = tpi->next;
  }
  mlock.unlock();

  if (NULL != tfind)
    delete tfind;
}
uint32 timer_order_list::get_interval() {
  uint64 tm = comm_utils::get_tick();
  tm = tm / 1000;
  uint32 tinterval = 0;

  mlock.lock();
  if (NULL == first) {
    mlock.unlock();
    return 0;
  }
  if (first->outtime > tm)
    tinterval = (uint32)((first->outtime - tm) & 0xffffffff);
  else
    tinterval = 10;
  mlock.unlock();
  return tinterval;
}
void timer_order_list::loop_timer(uint32 &o_interval) {
  uint64 tm = 0;
  timer_list_it *pevent = NULL;
  while (true) {
    tm = comm_utils::get_tick();
    tm = tm / 1000;

    mlock.lock();
    pevent = first;
    if (NULL == pevent) {
      mlock.unlock();
      o_interval = 0;
      break;
    }

    if (tm >= pevent->outtime) {
      first = pevent->next;
    } else {
      o_interval = (uint32)((pevent->outtime - tm) & 0xffffffff);
      mlock.unlock();
      break;
    }
    mlock.unlock();

    if (pevent->event.op != NULL) {
      pevent->event.op->deal_timer(pevent->event.isperiod, pevent->event.timer, pevent->event.buf);
      if (pevent->event.isperiod == 0) {
        delete pevent;
      } else {
        pevent->next = NULL;
        add_list(*pevent);
      }
    } else {
      delete pevent;
    }
  }
}

void timer_order_list::clear() {
  timer_list_it *tnext = NULL;
  mlock.lock();
  timer_list_it *tpi = first;
  while (NULL != tpi) {
    tnext = tpi->next;
    delete tpi;
    tpi = tnext;
  }
  mlock.unlock();
}

} // namespace lb_common

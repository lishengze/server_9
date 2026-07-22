#include "state_machine.h"
#include "matomic.h"

namespace lb_common {

bool src_stat_ref::to_init() {
  int64 t = atomic_load64(&statref);
  while (true) {
    if (t >= CSTATEREF_INITING || t != CSTATEREF_CLOSED)
      return false;
    if (likely(atomic_cas64_weak(&statref, &t, (t | CSTATEREF_INITING) + 1))) {
      return true;
    }
  }
}
bool src_stat_ref::end_init(bool &o_closed, bool init_ok) {
  int64 n;
  int64 t = atomic_load64(&statref);
  while (true) {
    if ((t & CSTATEREF_INITING) == 0 || (t & CSTATEREF_REFMASK) < 1)
      return false;
    assert((t & CSTATEREF_REFMASK) == 1);

    if (init_ok && (t & CSTATEREF_CLOSING) == 0) {
      n = CSTATEREF_INITED;
      o_closed = false;
    } else {
      n = CSTATEREF_CLOSED;
      o_closed = true;
    }
    if (likely(atomic_cas64(&statref, &t, n))) {
      return true;
    }
  }
}
bool src_stat_ref::set_work() {
  int64 t = atomic_load64(&statref);
  while (true) {
    if ((t & CSTATEREF_INITED) == 0 || (t & (CSTATEREF_CLOSING | CSTATEREF_WORK)) != 0)
      return false;
    if (likely(atomic_cas64(&statref, &t, t | CSTATEREF_WORK))) {
      return true;
    }
  }
}
bool src_stat_ref::stop_work() {
  int64 t = atomic_load64(&statref);
  while (true) {
    if ((t & (CSTATEREF_WORK | CSTATEREF_INITED)) == 0 || (t & CSTATEREF_CLOSING) != 0)
      return false;
    if ((t & CSTATEREF_WORK) != 0) {
      if (likely(atomic_cas64(&statref, &t, t - CSTATEREF_WORK))) {
        return true;
      }
    } else
      return false;
  }
}

bool src_stat_ref::add_ref() {
  int64 t = atomic_load64(&statref);
  while (true) {
    if (unlikely((t & (CSTATEREF_WORK | CSTATEREF_INITED)) == 0 || (t & CSTATEREF_CLOSING) != 0))
      return false;
    if (likely(atomic_cas64_weak(&statref, &t, t + 1))) {
      return true;
    }
  }
}
bool src_stat_ref::sub_ref() {
  int64 t = atomic_load64(&statref);
  while (true) {
    if ((t & CSTATEREF_REFMASK) > 1 || ((t & CSTATEREF_REFMASK) == 1 && (t & CSTATEREF_CLOSING) == 0)) {
      if (likely(atomic_cas64_weak(&statref, &t, t - 1))) {
        return false;
      }
    } else if ((t & CSTATEREF_REFMASK) == 1 && (t & CSTATEREF_CLOSING) != 0) {
      if (likely(atomic_cas64(&statref, &t, CSTATEREF_CLOSED))) {
        return true;
      }
    } else
      return false;
  }
}
bool src_stat_ref::to_close(bool &o_closed) {
  int64 t = atomic_load64(&statref);
  while (true) {
    if ((t & CSTATEREF_CLOSING) != 0 || t == CSTATEREF_CLOSED) {
      o_closed = false;
      return false;
    }
    if ((t & CSTATEREF_REFMASK) > 0) {
      if (likely(atomic_cas64(&statref, &t, t | CSTATEREF_CLOSING))) {
        o_closed = false;
        return true;
      }
    } else {
      if (likely(atomic_cas64(&statref, &t, CSTATEREF_CLOSED))) {
        o_closed = true;
        return true;
      }
    }
  }
}

} // namespace lb_common

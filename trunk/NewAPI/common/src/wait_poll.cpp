#include "wait_poll.h"
#include "comm_errno.h"
#include "matomic.h"

#include <errno.h>
#include <fcntl.h>
#include <new>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

namespace lb_common {

int32 wait_poll_one::init() {
  info = nullptr;
  atomic_store32(&isremove, 0);
  epollfd = epoll_create(4);
  if (epollfd == -1) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  return 0;
}

int32 wait_poll_one::add_wake(epoll_event_op &pinfo) {
  while (true) {
    int32 tmove = atomic_load32(&isremove);
    if (unlikely(tmove != 0))
      return LBERR_OBJ_STATE_LIMIT;
    if (atomic_cas32(&isremove, &tmove, 1))
      break;
  }

  struct epoll_event eventValue;
  if (pinfo.isout == 0)
    eventValue.events = EPOLLIN;
  else
    eventValue.events = EPOLLOUT;
  eventValue.events |= EPOLLERR;
  if (pinfo.isedge == 1)
    eventValue.events |= EPOLLET;

  info = &pinfo;
  eventValue.data.ptr = reinterpret_cast<void *>(info);

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pinfo.get_fd(), &eventValue) < 0) {
    info = nullptr;
    atomic_store32(&isremove, 0);
    return LBERR_OBJ_ADD_FAIL;
  }
  atomic_store32(&isremove, 2);
  return 0;
}

int32 wait_poll_one::wait(int32 waitms) {
  int32 ret = 0;
  if (waitms < 0)
    waitms = 0;

  while (true) {
    int32 tmove = atomic_load32(&isremove);
    if (unlikely(tmove == 0 || tmove == 1 || tmove == 4))
      return 0;
    if (unlikely(tmove == 3)) {
      if (atomic_cas32(&isremove, &tmove, 4)) {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, info->get_fd(), nullptr);
        info->deal_close();
        info = nullptr;
        atomic_store32(&isremove, 0);
        return 0;
      }
      continue;
    }

    ret = epoll_wait(epollfd, &epevent, 1, waitms);
    if (ret > 0) {
      if (likely((epevent.events & EPOLLERR) == 0)) {
        info->deal_event();
      } else {
        info->deal_error();
      }
    } else if (ret < 0 && (errno == EINTR || errno == EAGAIN)) {
      continue;
    }
    break;
  }
  return ret;
}

int32 wait_poll_one::mode_wake() {
  if (nullptr == info || atomic_load32(&isremove) != 2)
    return LBERR_OBJ_STATE_LIMIT;
  // if(info->isout == 0){
  struct epoll_event eventValue;
  eventValue.events = EPOLLIN | EPOLLET | EPOLLERR;
  eventValue.data.ptr = reinterpret_cast<void *>(info);
  if (epoll_ctl(epollfd, EPOLL_CTL_MOD, info->get_fd(), &eventValue) < 0)
    return LBERR_OBJ_MOD_FAIL;
  //}
  return 0;
}

int32 wait_poll_one::remove_wake() {
  while (true) {
    int32 tmove = atomic_load32(&isremove);
    if (unlikely(tmove != 2))
      return LBERR_OBJ_STATE_LIMIT;
    if (atomic_cas32(&isremove, &tmove, 3)) {
      break;
    }
  }
  return 0;
}

int32 wait_poll_one::delete_wake() {
  while (true) {
    int32 tmove = atomic_load32(&isremove);
    if (unlikely(tmove != 2 && tmove != 3))
      return LBERR_OBJ_STATE_LIMIT;
    if (atomic_cas32(&isremove, &tmove, 4)) {
      break;
    }
  }

  epoll_ctl(epollfd, EPOLL_CTL_DEL, info->get_fd(), nullptr);
  // info->deal_close();
  atomic_store32(&isremove, 0);
  return 0;
}

void wait_poll_one::destroy() {
  atomic_store32(&isremove, 0);
  if (epollfd > 0) {
    close(epollfd);
    epollfd = -1;
  }
  info = nullptr;
}

int32 wait_poll_multi::init(int32 wakenum) {
  int32 ret = 0;
  if ((ret = change_event.init(0)) < 0)
    return ret;

  del_evs = new (std::nothrow) epoll_event_op *[wakenum];
  if (nullptr == del_evs)
    return LBERR_MEM_ALLOC_FAIL;

  int32 i = 0;
  for (i = 0; i < wakenum; ++i) {
    del_evs[i] = nullptr;
  }

  wait_evs = new (std::nothrow) epoll_event[wakenum + 1];
  if (nullptr == wait_evs) {
    delete[] del_evs;
    del_evs = nullptr;
    return LBERR_MEM_ALLOC_FAIL;
  }

  epollfd = epoll_create(wakenum + 1);
  if (epollfd == -1) {
    delete[] del_evs;
    del_evs = nullptr;
    delete[] wait_evs;
    wait_evs = nullptr;
    return LBERR_OBJ_OPEN_FAIL;
  }

  struct epoll_event eventValue;
  eventValue.events = EPOLLIN;
  eventValue.data.ptr = reinterpret_cast<void *>(&change_event);

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, change_event.get_fd(), &eventValue) < 0) {
    delete[] del_evs;
    del_evs = nullptr;
    delete[] wait_evs;
    wait_evs = nullptr;
    return LBERR_OBJ_ADD_FAIL;
  }

  eventnum = wakenum + 1;
  dellock.init();
  delnum = 0;
  return 0;
}

int32 wait_poll_multi::add_wake(epoll_event_op &pinfo) {
  struct epoll_event eventValue;

  if (pinfo.isout == 0)
    eventValue.events = EPOLLIN;
  else
    eventValue.events = EPOLLOUT;
  eventValue.events |= EPOLLERR;
  if (pinfo.isedge == 1)
    eventValue.events |= EPOLLET;

  eventValue.data.ptr = reinterpret_cast<void *>(&pinfo);

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pinfo.get_fd(), &eventValue) < 0) {
    if (errno != EEXIST)
      return LBERR_OBJ_ADD_FAIL;
  }

  return 0;
}

void wait_poll_multi::del_wake(int32 ipos, int32 wpos) {
  int32 t = 0;
  int32 i = 0;

  while (true) {
    // dellock.lock();
    t = delnum;
    // dellock.unlock();

    while (i < t) {
      if (del_evs[i] != nullptr) {
        for (int32 k = ipos + 1; k < wpos; k++) {
          epoll_event_op *tp = reinterpret_cast<epoll_event_op *>(wait_evs[k].data.ptr);
          if (del_evs[i] == tp) {
            wait_evs[k].data.ptr = nullptr;
            break;
          }
        }
        epoll_ctl(epollfd, EPOLL_CTL_DEL, del_evs[i]->get_fd(), nullptr);
        del_evs[i]->deal_close();
        del_evs[i] = nullptr;
      }
      i++;
    }

    dellock.lock();
    if (t == delnum) {
      delnum = 0;
      dellock.unlock();
      change_event.reset();
      break;
    }
    dellock.unlock();
  }
}

int32 wait_poll_multi::wait(int32 waitms) {
  int32 ret, i;
  if (waitms < 0)
    waitms = 0;
  void *tpchange = reinterpret_cast<void *>(&change_event);

  while (true) {
    ret = epoll_wait(epollfd, wait_evs, eventnum, waitms);
    if (ret > 0) {
      for (i = 0; i < ret; i++) {
        if (nullptr == wait_evs[i].data.ptr) {
          continue;
        }
        if (wait_evs[i].data.ptr == tpchange) {
          del_wake(i, ret);
          continue;
        }
        epoll_event_op *pinfo = reinterpret_cast<epoll_event_op *>(wait_evs[i].data.ptr);
        if (likely((wait_evs[i].events & EPOLLERR) == 0)) {
          pinfo->deal_event();
        } else {
          pinfo->deal_error();
        }
      }
    } else if (ret < 0 && (errno == EINTR || errno == EAGAIN)) {
      continue;
    }
    break;
  }
  return ret;
}

int32 wait_poll_multi::mode_wake(epoll_event_op &pinfo) {
  if (pinfo.isedge == 1 && pinfo.isout == 0) {
    struct epoll_event eventValue;
    eventValue.events = EPOLLIN | EPOLLET | EPOLLERR;
    eventValue.data.ptr = reinterpret_cast<void *>(&pinfo);
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, pinfo.get_fd(), &eventValue) < 0)
      return LBERR_OBJ_MOD_FAIL;
  }
  return 0;
}

int32 wait_poll_multi::remove_wake(epoll_event_op &pinfo) {
  dellock.lock();
  int32 t = delnum;
  del_evs[t++] = &pinfo;
  delnum = t;
  dellock.unlock();

  change_event.wake(1);
  return 0;
}

int32 wait_poll_multi::delete_wake(epoll_event_op &pinfo) {
  if (0 == epoll_ctl(epollfd, EPOLL_CTL_DEL, pinfo.get_fd(), nullptr))
    return 0;
  else
    return LBERR_OBJ_DEL_FAIL;
}

void wait_poll_multi::destroy() {
  if (epollfd > 0) {
    close(epollfd);
    epollfd = -1;
  }
  if (del_evs != nullptr) {
    delete[] del_evs;
    del_evs = nullptr;
  }
  if (wait_evs != nullptr) {
    delete[] wait_evs;
    wait_evs = nullptr;
  }
  change_event.destroy();
  eventnum = 0;
}

} // namespace lb_common

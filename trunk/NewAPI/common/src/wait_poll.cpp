#include "wait_poll.h"
#include "comm_errno.h"
#include "matomic.h"

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <new>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

namespace lb_common
{

  int32 wait_poll_multi::init(int32 wakenum)
  {
    // 注意：init 不可重入。重复调用会导致 epollfd / del_evs / wait_evs 全部泄漏。
    // 重复初始化语义请先调 destroy()。
    int32 ret = 0;
    if ((ret = change_event.init(0)) < 0)
      return ret;

    del_evs = new (std::nothrow) epoll_event_op *[wakenum];
    if (nullptr == del_evs)
    {
      change_event.destroy();
      return LBERR_MEM_ALLOC_FAIL;
    }

    int32 i = 0;
    for (i = 0; i < wakenum; ++i)
    {
      del_evs[i] = nullptr;
    }

    wait_evs = new (std::nothrow) epoll_event[wakenum + 1];
    if (nullptr == wait_evs)
    {
      delete[] del_evs;
      del_evs = nullptr;
      change_event.destroy();
      return LBERR_MEM_ALLOC_FAIL;
    }

    // 避免 fd 泄漏到 fork 子进程。
    epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd == -1)
    {
      delete[] del_evs;
      del_evs = nullptr;
      delete[] wait_evs;
      wait_evs = nullptr;
      change_event.destroy();
      return LBERR_OBJ_OPEN_FAIL;
    }

    struct epoll_event eventValue;
    eventValue.events = EPOLLIN;
    eventValue.data.ptr = reinterpret_cast<void *>(&change_event);

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, change_event.get_fd(), &eventValue) < 0)
    {
      delete[] del_evs;
      del_evs = nullptr;
      delete[] wait_evs;
      wait_evs = nullptr;
      close(epollfd);
      epollfd = -1;
      change_event.destroy();
      return LBERR_OBJ_ADD_FAIL;
    }

    eventnum = wakenum + 1;
    delnum = 0;
    return 0;
  }

  int32 wait_poll_multi::add_wake(epoll_event_op &pinfo)
  {
    struct epoll_event eventValue;

    if (pinfo.isout == 0)
      eventValue.events = EPOLLIN;
    else
      eventValue.events = EPOLLOUT;
    eventValue.events |= EPOLLERR;
    if (pinfo.isedge == 1)
      eventValue.events |= EPOLLET;

    eventValue.data.ptr = reinterpret_cast<void *>(&pinfo);

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pinfo.get_fd(), &eventValue) < 0)
    {
      if (errno == EEXIST)
      {
        // fd 已在 epoll 中（典型场景：连接复用 — 同 fd 被新 pinfo 注册）。
        // 原版静默返回 0，导致 epoll 仍以旧 data.ptr 派发事件，
        // 用户持新 pinfo 时，旧 pinfo 的 deal_event 被触发（UAF 风险）。
        // 必须用 EPOLL_CTL_MOD 更新 data.ptr 为新 pinfo 的地址。
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, pinfo.get_fd(), &eventValue) < 0)
        {
          return LBERR_OBJ_ADD_FAIL;
        }
      }
      else
      {
        return LBERR_OBJ_ADD_FAIL;
      }
    }

    return 0;
  }

  void wait_poll_multi::del_wake(int32 ipos, int32 wpos)
  {
    int32 t = 0;
    int32 i = 0;

    while (true)
    {
      // 改为 lock/unlock 包裹，并改用 atomic_load32 读 delnum。
      dellock.lock();
      t = delnum;
      dellock.unlock();

      while (i < t)
      {
        if (del_evs[i] != nullptr)
        {
          for (int32 k = ipos + 1; k < wpos; k++)
          {
            epoll_event_op *tp = reinterpret_cast<epoll_event_op *>(wait_evs[k].data.ptr);
            if (del_evs[i] == tp)
            {
              wait_evs[k].data.ptr = nullptr;
              break;
            }
          }
          // del_wake 一次性执行：本函数末尾置 delnum=0，无"下一轮"重试机会。
          // epoll_ctl(DEL) 失败几乎只可能是瞬时 ENOMEM，故先在本次调用内对 ENOMEM
          // 做有界重试；成功即走正常 deal_close。
          // 失败时不可直接 deal_close：deal_close 不关 fd，fd 仍留 epoll 时级联释放 pinfo
          // 后，后续 epoll_wait 触发 deal_event 可能 UAF；亦不可在 del_wake 内裸 close(fd)——
          // tcp_ch::destroy() 会按陈旧 sysfd 再 close，若该 fd 号已被复用将误关无关连接
          // （fd 复用竞争）。故持续失败时只能跳过 deal_close（泄漏该 pinfo/fd）作兜底，
          // 比 UAF/竞争安全；该路径极罕见。
          int32 del_fd = del_evs[i]->get_fd();
          int32 del_ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, del_fd, nullptr);
          if (del_ret < 0 && errno == ENOMEM)
          {
            for (int32 r = 0; r < 8 && del_ret < 0 && errno == ENOMEM; ++r)
            {
              del_ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, del_fd, nullptr);
            }
          }
          if (del_ret == 0)
          {
            del_evs[i]->deal_close();
            del_evs[i] = nullptr;
          }
          else
          {
            int32 del_errno = errno;
            std::fprintf(stderr,
                         "wait_poll_multi::del_wake epoll_ctl DEL failed after retry: errno=%d fd=%d\n",
                         del_errno, del_fd);
            del_evs[i] = nullptr;
          }
        }
        i++;
      }

      dellock.lock();
      if (t == delnum)
      {
        delnum = 0;
        dellock.unlock();
        change_event.reset();
        break;
      }
      dellock.unlock();
    }
  }

  int32 wait_poll_multi::wait(int32 waitms)
  {
    int32 ret, i;
    if (waitms < 0)
      waitms = 0;
    void *tpchange = reinterpret_cast<void *>(&change_event);

    while (true)
    {
      ret = epoll_wait(epollfd, wait_evs, eventnum, waitms);
      if (ret > 0)
      {
        for (i = 0; i < ret; i++)
        {
          if (nullptr == wait_evs[i].data.ptr)
          {
            continue;
          }
          if (wait_evs[i].data.ptr == tpchange)
          {
            del_wake(i, ret);
            continue;
          }
          epoll_event_op *pinfo = reinterpret_cast<epoll_event_op *>(wait_evs[i].data.ptr);
          uint32 events = wait_evs[i].events;
          if (likely((events & (EPOLLERR | EPOLLHUP)) == 0))
          {
            pinfo->deal_event();
          }
          else
          {
            pinfo->deal_error();
          }
        }
      }
      else if (ret < 0 && (errno == EINTR || errno == EAGAIN))
      {
        continue;
      }
      break;
    }
    return ret;
  }

  int32 wait_poll_multi::mode_wake(epoll_event_op &pinfo)
  {
    // 而不是硬编码 EPOLLIN | EPOLLET | EPOLLERR。原实现只在 isedge==1 && isout==0 时
    // 才更新，且更新值与 pinfo 无关，导致 isout==1 或 isedge==0 的 mode_wake 完全 no-op，
    // 调用方以为模式已切换但实际未生效。
    struct epoll_event eventValue;
    eventValue.events = (pinfo.isout ? EPOLLOUT : EPOLLIN) | EPOLLERR;
    if (pinfo.isedge == 1)
      eventValue.events |= EPOLLET;
    eventValue.data.ptr = reinterpret_cast<void *>(&pinfo);
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, pinfo.get_fd(), &eventValue) < 0)
      return LBERR_OBJ_MOD_FAIL;
    return 0;
  }

  int32 wait_poll_multi::remove_wake(epoll_event_op &pinfo)
  {
    dellock.lock();
    int32 t = delnum;
    // wakenum 即越界写堆。增加 bounds check，超过容量直接返回错误（调用方应
    // 调大 wakenum 或尽早 del_wake）。
    if (unlikely(t >= eventnum - 1))
    {
      dellock.unlock();
      return LBERR_OBJ_NUM_LIMIT;
    }
    del_evs[t++] = &pinfo;
    delnum = t;
    dellock.unlock();

    change_event.wake(1);
    return 0;
  }

  int32 wait_poll_multi::delete_wake(epoll_event_op &pinfo)
  {
    if (0 == epoll_ctl(epollfd, EPOLL_CTL_DEL, pinfo.get_fd(), nullptr))
      return 0;
    else
      return LBERR_OBJ_DEL_FAIL;
  }

  void wait_poll_multi::destroy()
  {
    if (epollfd > 0)
    {
      close(epollfd);
      epollfd = -1;
    }
    if (del_evs != nullptr)
    {
      delete[] del_evs;
      del_evs = nullptr;
    }
    if (wait_evs != nullptr)
    {
      delete[] wait_evs;
      wait_evs = nullptr;
    }
    change_event.destroy();
    eventnum = 0;
  }

} // namespace lb_common

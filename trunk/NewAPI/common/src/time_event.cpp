
#include "time_event.h"
#include "comm_errno.h"
#include "mutils.h"

#include <new>
#include <vector>

namespace lb_common
{

  void timer_order_list::add_list(timer_list_it &info)
  {
    uint64 tm = comm_utils::get_tick();
    tm = tm / 1000;
    info.begintime = tm;
    info.outtime = tm + info.event.timer;

    timer_list_it *tprev = NULL;

    mlock.lock();
    timer_list_it *tpi = first;
    while (NULL != tpi)
    {
      if (info.outtime >= tpi->outtime)
      {
        tprev = tpi;
        tpi = tpi->next;
      }
      else
      {
        break;
      }
    }
    if (NULL != tprev)
    {
      info.next = tprev->next;
      tprev->next = &info;
    }
    else
    {
      info.next = first;
      first = &info;
    }
    mlock.unlock();
  }

  int32 timer_order_list::add_timer(time_event_info &eventinfo)
  {
    int32 waitus = eventinfo.timer;
    if (waitus < TIMER_MIN_UNIT_US)
    {
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

  void timer_order_list::delete_timer(time_event_op *op, int32 timer)
  {
    // 修复：原版 break 后只删第一个匹配项。
    // 用户在同一 op 上注册多个相同 timer（典型：5 个 10ms 周期 timer）时,
    // delete_timer(self, 10) 仅取消第一个,其余继续 fire 永不停止。
    // 改为遍历全表,删除所有匹配的节点。
    //
    // 与 loop_timer 的协调见 loop_timer 注释：loop_timer 把 fire + re-add 放在
    // 同一临界区，因此 delete_timer 不需要额外扫描 inflight。
    timer_list_it *tprev = NULL;
    timer_list_it *tpi = first;
    std::vector<timer_list_it *> to_delete;
    mlock.lock();
    while (NULL != tpi)
    {
      if (tpi->event.timer == timer && tpi->event.op == op)
      {
        timer_list_it *tnext = tpi->next;
        if (NULL != tprev)
        {
          tprev->next = tnext;
        }
        else
        {
          first = tnext;
        }
        to_delete.push_back(tpi);
        tpi = tnext;
      }
      else
      {
        tprev = tpi;
        tpi = tpi->next;
      }
    }
    mlock.unlock();

    for (size_t i = 0; i < to_delete.size(); ++i)
    {
      delete to_delete[i];
    }
  }
  uint32 timer_order_list::get_interval()
  {
    uint64 tm = comm_utils::get_tick();
    tm = tm / 1000;
    uint32 tinterval = 0;

    mlock.lock();
    if (NULL == first)
    {
      mlock.unlock();
      return 0;
    }
    if (first->outtime > tm)
      tinterval = (uint32)((first->outtime - tm) & 0xffffffff);
    else
    {
      // 链头已过期时返回最小退避时间（1ms）。
      // 原版返回 10us，配合 timer_thread 的 epoll_wait(100ms)
      // 会以 10us 间隔自旋、CPU 占用陡增。
      // 100us 既能保证延迟触发，又避免 CPU 风暴。
      tinterval = 100;
    }
    mlock.unlock();
    return tinterval;
  }
  void timer_order_list::loop_timer(uint32 &o_interval)
  {
    // 临界区包含 fire + re-add，避免周期 timer 在 deal_timer 与 add_list
    // 之间被 delete_timer 漏取消导致复活。
    //
    // 代价：deal_timer 用户回调期间，整个 timer_order_list 串行化。
    // 约束：
    //   1) deal_timer 应保持简短（典型为发心跳、置标志、投递事件），
    //      避免长时间阻塞 add_timer/delete_timer。
    //   2) deal_timer 禁止回调本 timer_order_list 的 add_timer/delete_timer
    //      （atomic_lock 非递归，重入即死锁）。
    uint64 tm = 0;
    timer_list_it *pevent = NULL;
    while (true)
    {
      tm = comm_utils::get_tick();
      tm = tm / 1000;

      mlock.lock();
      pevent = first;
      if (NULL == pevent)
      {
        mlock.unlock();
        o_interval = 0;
        break;
      }

      if (tm < pevent->outtime)
      {
        o_interval = (uint32)((pevent->outtime - tm) & 0xffffffff);
        mlock.unlock();
        break;
      }

      // 摘链（整个 fire + re-add 都在锁内）
      first = pevent->next;

      if (pevent->event.op != NULL)
      {
        pevent->event.op->deal_timer(pevent->event.isperiod, pevent->event.timer, pevent->event.buf);
      }

      if (pevent->event.isperiod == 0)
      {
        // 单次 timer：fire 后释放
        delete pevent;
      }
      else
      {
        // 周期 timer：就地重插入（不调 add_list，避免重入 mlock）
        pevent->begintime = tm;
        pevent->outtime = tm + pevent->event.timer;

        timer_list_it *tprev = NULL;
        timer_list_it *tcur = first;
        while (NULL != tcur)
        {
          if (pevent->outtime >= tcur->outtime)
          {
            tprev = tcur;
            tcur = tcur->next;
          }
          else
          {
            break;
          }
        }
        if (NULL != tprev)
        {
          pevent->next = tprev->next;
          tprev->next = pevent;
        }
        else
        {
          pevent->next = first;
          first = pevent;
        }
      }
      mlock.unlock();
    }
  }

  void timer_order_list::clear()
  {
    timer_list_it *tnext = NULL;
    mlock.lock();
    timer_list_it *tpi = first;
    while (NULL != tpi)
    {
      tnext = tpi->next;
      delete tpi;
      tpi = tnext;
    }
    // 调用 clear() 时第二次沿悬空指针遍历。
    first = nullptr;
    mlock.unlock();
  }

} // namespace lb_common

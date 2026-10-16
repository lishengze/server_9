
#include "que_mth_buf.h"
#include "comm_errno.h"
#include "mutils.h"
#include <cstring>
#include <sched.h>

namespace lb_common
{

  int64 que_mth_buf::write_get(char *&o_data, int32 len)
  {
    if (unlikely(len < 0 || len > mask))
    {
      return 0;
    }
    len = ALIGN_UP(len, align_n);

    int64 tc = atomic_load64(&wrcmt);
    if (unlikely(tc + len - atomic_load64(&rdcmt) > mask))
      return 0;

    int64 wr = (tc & mask);
    // 保证不会写到末尾反转，wr 永远不会为0
    if (likely(wr + len <= mask))
    {
      o_data = (pbuf + wr);
    }
    else
    {
      tc += (mask + 1 - wr);
      if (tc + len - atomic_load64(&rdcmt) > mask)
        return 0;
      o_data = (pbuf); // + (tc&mask));
    }
    return tc;
  }
  int64 que_mth_buf::write(const char *userdata, int32 len)
  {
    // P1-11: 长度校验仅用 assert（Debug 编译生效，Release 自动失效）。
    // write_get/write_mth 有运行时检查；write 没有，故此处补 assert 与之对齐。
    assert(len > 0 && len <= mask);
    int32 tlen = ALIGN_UP(len, align_n);

    int64 tc = atomic_load64(&wrcmt);
    if (unlikely(tc + tlen - atomic_load64(&rdcmt) > mask))
      return 0;

    int64 wr = (tc & mask);
    // 保证不会写到末尾反转，wr 永远不会为0
    if (likely(wr + tlen <= mask))
    {
      std::memcpy((pbuf + wr), userdata, len);
    }
    else
    {
      tc += (mask + 1 - wr);
      if (tc + tlen - atomic_load64(&rdcmt) > mask)
        return 0;
      std::memcpy(pbuf, userdata, len);
      atomic_store64(&roundend, atomic_load64(&wrcmt));
    }
    atomic_store64(&wrcmt, tc + tlen);
    return tc;
  }

  int64 que_mth_buf::write_get_mth(char *&o_data, int32 len)
  {
    if (unlikely(len < 0 || len > mask))
    {
      return 0;
    }
    len = ALIGN_UP(len, align_n);

    int64 ret = 0;
    int64 t;
    int64 wr = atomic_load64(&wrpos);

    do
    {
      if (unlikely(wr + len - atomic_load64(&rdcmt) > mask))
        return 0;

      t = (wr & mask);
      // 保证不会写到末尾反转，t 永远不会为0
      if (likely(t + len <= mask))
      {
        t = wr + len;
        ret = wr;
      }
      else
      {
        ret = wr + (mask + 1 - t);
        t = ret + len;
        if (t - atomic_load64(&rdcmt) > mask)
          return 0;
      }

      if (likely(atomic_cas64_weak(&wrpos, &wr, t)))
      {
        if ((ret & mask) == 0)
          atomic_store64(&roundend, wr);

        o_data = (pbuf + (ret & mask));
        return ret;
      }
    } while (1);
  }
  void que_mth_buf::write_cmt_mth(int64 getpos, int32 len)
  {
    int64 oldpos = getpos;
    if (unlikely((mask & oldpos) == 0))
    {
      oldpos = atomic_load64(&roundend);
    }

    int32 i = QUE_LOOP_BUSY_COUNT;
    len = ALIGN_UP(len, align_n);
    getpos += len;

    // 1) 当持有者异常死掉时本线程可能永久自旋；
    // 2) `oldpos <= wr` 判断在多生产者场景下存在 race，可能错误覆盖别人提交。
    // 改为 CAS 循环：只有当 wrcmt 仍是 oldpos 时才提交本线程的 getpos。
    // 若持有者死掉，调用方（业务层）应通过 check_cmt_write 回收。
    do
    {
      int64 wr = atomic_load64(&wrcmt);
      if (unlikely(wr >= getpos))
        return; // 已被其他线程提交到 ≥ getpos

      if (likely(oldpos <= wr))
      {
        if (likely(atomic_cas64_weak(&wrcmt, &wr, getpos)))
        {
          return;
        }
        // CAS 失败说明 wrcmt 已被别人更新，重试
        continue;
      }

      // wr 仍在 (oldpos, getpos) 之间，前任持有者未提交，让出 CPU 避免死等。
      i--;
      if (i == 0)
      {
        i = QUE_LOOP_BUSY_COUNT / 2;
        sched_yield();
      }
    } while (1);
  }
  int64 que_mth_buf::write_mth(const char *userdata, int32 len)
  {
    if (unlikely(len < 0 || len > mask || (len > 0 && userdata == NULL)))
    {
      return 0;
    }
    int32 tlen = ALIGN_UP(len, align_n);

    int64 ret = 0;
    int64 t;
    int64 wr = atomic_load64(&wrpos);

    do
    {
      if (unlikely(wr + tlen - atomic_load64(&rdcmt) > mask))
        return 0;

      t = (wr & mask);
      // 保证不会写到末尾反转，t 永远不会为0
      if (likely(t + tlen <= mask))
      {
        t = wr + tlen;
        ret = wr;
      }
      else
      {
        ret = wr + (mask + 1 - t);
        t = ret + tlen;
        if (t - atomic_load64(&rdcmt) > mask)
          return 0;
      }
      if (likely(atomic_cas64_weak(&wrpos, &wr, t)))
      {
        if ((ret & mask) == 0)
          atomic_store64(&roundend, wr);
        break;
      }
    } while (1);

    std::memcpy((pbuf + (ret & mask)), userdata, len);

    write_cmt_mth(ret, len);

    return ret;
  }

  int64 que_mth_buf::read_get(char *&o_data)
  {
    int64 wr = atomic_load64(&wrcmt);
    int64 rd = atomic_load64(&rdcmt);
    if (unlikely(rd >= wr))
      return 0;

    int64 t = (rd & mask);
    o_data = (pbuf + t);
    // if(likely((wr&mask) != 0)){
    t = (wr & mask) - t;
    if (likely(t > 0))
    {
      return rd;
    }
    //}

    int64 tre = atomic_load64(&roundend);
    if (tre >= wr || tre < rd)
    {
      return 0;
    }
    t = tre - rd;
    if (t > 0)
    {
      return rd;
    }

    t = (rd & mask);
    if (t != 0)
    {
      rd += (mask + 1 - t);
    }
    t = wr - rd;
    if (t > 0)
    {
      atomic_store64(&rdcmt, rd);
      o_data = (pbuf) + (rd & mask);
      return rd; //(int32)(t > len?len:t);
    }
    return 0;
  }
  int64 que_mth_buf::read_get(char *&o_data, int64 read_pos)
  {
    int64 wr = atomic_load64(&wrcmt);
    int64 rd = read_pos;
    if (unlikely(rd >= wr))
      return 0;
    int64 t = atomic_load64(&rdcmt);
    if (unlikely(rd < t))
    {
      rd = t;
    }

    t = (rd & mask);
    o_data = (pbuf + t);
    // if(likely((wr&mask) != 0)){
    t = (wr & mask) - t;
    if (likely(t > 0))
    {
      return rd;
    }
    //}

    int64 tre = atomic_load64(&roundend);
    if (tre >= wr || tre < rd)
    {
      return 0;
    }
    t = tre - rd;
    if (t > 0)
    {
      return rd;
    }

    t = (rd & mask);
    if (t != 0)
    {
      rd += (mask + 1 - t);
    }
    t = wr - rd;
    if (t > 0)
    {
      o_data = (pbuf) + (rd & mask);
      return rd; //(int32)(t > len?len:t);
    }
    return 0;
  }

  int64 que_mth_buf::need_buf_size(int64 que_size)
  {
    int32 t = comm_utils::calc_power(que_size);
    return (1L << t);
  }
  void que_mth_buf::init_set(int64 tsize, int32 align_byte)
  {
    atomic_store64(&wrcmt, CACHE_ALIGN_SIZE);
    atomic_store64(&roundend, 0);
    atomic_store64(&wrpos, CACHE_ALIGN_SIZE);
    pbuf = NULL;
    atomic_store64(&rdcmt, CACHE_ALIGN_SIZE);

    if (align_byte == SIMD_ALIGN_SIZE)
      align_n = SIMD_ALIGN_SIZE;
    else
      align_n = 8;

    create_mem = 0;
    userpos[0] = CACHE_ALIGN_SIZE;
    userpos[1] = CACHE_ALIGN_SIZE;
    std::memset(userdata, 0, QUE_USER_DATA_LEN);
    filled = 0;
    mask = tsize - 1;
  }
  int32 que_mth_buf::init(int64 que_size, int32 align_byte)
  {
    if (que_size <= 0)
      return LBERR_ARGV_WRONG;

    int64 tnum = need_buf_size(que_size);
    init_set(tnum, align_byte);

    pbuf = (char *)(comm_utils::aligned_malloc(tnum, CACHE_ALIGN_SIZE));
    if (NULL == pbuf)
    {
      return LBERR_MEM_ALLOC_FAIL;
    }
    create_mem = 1;
    return 0;
  }
  int32 que_mth_buf::init(char *shm_addr, int64 que_size, int32 shm_isexist, int32 is_continue, int32 align_byte)
  {
    if (que_size <= 0 || NULL == shm_addr)
      return LBERR_ARGV_WRONG;

    int64 tnum = need_buf_size(que_size);
    if (shm_isexist == 0)
    {
      init_set(tnum, align_byte);
      pbuf = shm_addr;
      return 0;
    }

    pbuf = shm_addr;
    reset_pos(is_continue);
    return 0;
  }
  void que_mth_buf::reset_pos(int32 is_continue)
  {
    if (atomic_load64(&rdcmt) < CACHE_ALIGN_SIZE)
      atomic_store64(&rdcmt, CACHE_ALIGN_SIZE);
    if (atomic_load64(&wrcmt) < CACHE_ALIGN_SIZE)
      atomic_store64(&wrcmt, CACHE_ALIGN_SIZE);

    int64 tw = atomic_load64(&wrcmt);
    if (atomic_load64(&roundend) > tw)
      atomic_store64(&roundend, tw);
    if (atomic_load64(&rdcmt) > tw)
    {
      atomic_store64(&rdcmt, tw);
    }
    if (is_continue == 0)
    {
      atomic_store64(&wrpos, tw);
      atomic_store64(&rdcmt, tw);
      atomic_store64(&roundend, 0);
    }
    else
    {
      atomic_store64(&wrpos, tw);
    }
  }

  void que_mth_buf::close()
  {
    if (NULL != pbuf && create_mem == 1)
    {
      comm_utils::aligned_free((void *)pbuf);
      pbuf = NULL;
      create_mem = 0;
    }
    reset_pos(0);
  }

} // namespace lb_common

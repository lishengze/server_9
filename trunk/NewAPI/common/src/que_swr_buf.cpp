
#include "que_swr_buf.h"
#include "comm_errno.h"
#include "mutils.h"
#include <cstring>

namespace lb_common
{

  int64 que_swr_buf::write_get(char *&o_data, int32 len)
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
  int64 que_swr_buf::write(const char *userdata, int32 len)
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

  int64 que_swr_buf::read_get(char *&o_data)
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
      // 跨线程可见性：写者通过 atomic_load64(&rdcmt) 读取本读者的进度。
      // 必须用 atomic_store64 发布，否则弱序架构（ARM/RISC-V）下写者可能看不到更新。
      atomic_store64(&rdcmt, rd);
      o_data = (pbuf) + (rd & mask);
      return rd; //(int32)(t > len?len:t);
    }
    return 0;
  }
  int64 que_swr_buf::read_get(char *&o_data, int64 read_pos)
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

  int64 que_swr_buf::need_buf_size(int64 que_size)
  {
    int32 t = comm_utils::calc_power(que_size);
    return (1L << t);
  }
  void que_swr_buf::init_set(int64 tsize, int32 align_byte)
  {
    atomic_store64(&wrcmt, CACHE_ALIGN_SIZE);
    atomic_store64(&roundend, 0);
    pbuf = NULL;
    mask = tsize - 1;
    atomic_store64(&rdcmt, CACHE_ALIGN_SIZE);

    if (align_byte == SIMD_ALIGN_SIZE)
      align_n = SIMD_ALIGN_SIZE;
    else
      align_n = 8;

    create_mem = 0;
    userpos[0] = CACHE_ALIGN_SIZE;
    userpos[1] = CACHE_ALIGN_SIZE;
  }
  int32 que_swr_buf::init(int64 que_size, int32 align_byte)
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
  int32 que_swr_buf::init(char *shm_addr, int64 que_size, int32 shm_isexist, int32 is_continue, int32 align_byte)
  {
    if (que_size <= 0 || NULL == shm_addr)
      return LBERR_ARGV_WRONG;

    // 把 pbuf 置 NULL，导致后续读写解引用空指针。
    // 修复：init_set 完成后再设置 pbuf，让 init_set 只初始化元数据。
    int64 tnum = need_buf_size(que_size);
    if (shm_isexist == 0 || (mask != 0 && mask + 1 != tnum))
    {
      init_set(tnum, align_byte);
    }
    pbuf = shm_addr;

    if (shm_isexist == 0 || (mask != 0 && mask + 1 != tnum))
    {
      return 0;
    }

    reset_pos(is_continue);
    return 0;
  }
  void que_swr_buf::reset_pos(int32 is_continue)
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
      atomic_store64(&rdcmt, tw);
      atomic_store64(&roundend, 0);
    }
  }

  void que_swr_buf::close()
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

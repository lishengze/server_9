
#include "que_proc_buf.h"
#include "comm_errno.h"
#include "mutils.h"
#include <assert.h>
#include <cstring>

// #include <iostream>
#include <sched.h>

namespace lb_common {

int64 que_proc_buf::write_get(char *&o_data, int32 len) {
  int32 tlen = len + sizeof(proc_buf_head);
  tlen = ALIGN_UP(tlen, align_n);

  int64 tc = que->wrcmt;
  if (unlikely(tc + tlen - atomic_load64(&que->rdcmt) > mask))
    return 0;

  int64 wr = (tc & mask);
  // 保证不会写到末尾反转，wr 永远不会为0
  if (likely(wr + tlen <= mask)) {
    /*proc_buf_head *thead = (proc_buf_head *)(pbuf +wr);
    thead->user_len = len;
    thead->mem_len = tlen;
    thead->op_pid = PROC_BUF_PID_INIT;*/
    o_data = (pbuf + wr + sizeof(proc_buf_head));
  } else {
    tc += (mask + 1 - wr);
    if (tc + tlen - atomic_load64(&que->rdcmt) > mask)
      return 0;

    /*proc_buf_head *thead = (proc_buf_head *)(pbuf +wr);
    thead->user_len = len;
    thead->mem_len = tlen;
    thead->op_pid = PROC_BUF_PID_INIT;*/
    o_data = (pbuf + sizeof(proc_buf_head));
  }
  return tc;
}
int64 que_proc_buf::write(const char *userdata, int32 len) {
  int32 tlen = len + sizeof(proc_buf_head);
  tlen = ALIGN_UP(tlen, align_n);

  int64 tc = que->wrcmt;
  if (unlikely(tc + tlen - atomic_load64(&que->rdcmt) > mask))
    return 0;

  proc_buf_head *thead;
  int64 wr = (tc & mask);
  // 保证不会写到末尾反转，wr 永远不会为0
  if (likely(wr + tlen <= mask)) {
    thead = (proc_buf_head *)(pbuf + wr);
    thead->user_len = len;
    thead->mem_len = tlen;
    thead->op_pid = PROC_BUF_PID_INIT;
    std::memcpy((pbuf + wr + sizeof(proc_buf_head)), userdata, len);
  } else {
    tc += (mask + 1 - wr);
    if (tc + tlen - atomic_load64(&que->rdcmt) > mask)
      return 0;

    thead = (proc_buf_head *)(pbuf);
    thead->user_len = len;
    thead->mem_len = tlen;
    thead->op_pid = PROC_BUF_PID_INIT;
    std::memcpy((pbuf + sizeof(proc_buf_head)), userdata, len);
    atomic_store64(&que->roundend, que->wrcmt);
  }

  atomic_store64(&que->wrcmt, tc + tlen);
  return tc;
}

int64 que_proc_buf::write_get_mth(char *&o_data, int32 len) {
  int32 tlen = len + sizeof(proc_buf_head);
  tlen = ALIGN_UP(tlen, align_n);
  // int32 again_ctl = 0;
  int64 ret = 0;
  int64 t;
  int64 wr = atomic_load64(&que->wrpos);

  do {
    if (unlikely(wr + tlen - atomic_load64(&que->rdcmt) > mask))
      return 0;

    t = (wr & mask);
    if (likely(t + tlen <= mask)) {
      t = wr + tlen;
      ret = wr;
    } else {
      ret = wr + (mask + 1 - t);
      t = ret + tlen;
      if (t - atomic_load64(&que->rdcmt) > mask)
        return 0;
    }

    proc_buf_head *thead = (proc_buf_head *)(pbuf + (ret & mask));
    if (likely(atomic_cas64_weak(&que->wrpos, &wr, t))) {
      /*
        1. 假定几次线程调度后，若进程在，设置wrpos后一定会设置到op_pid
                 也就是长度字段都已被设置。
        2. 若进程core，而pid未设置，即长度字段未设置，其他写者应重来
      */
      if ((ret & mask) == 0)
        atomic_store64(&que->roundend, wr);
      thead->user_len = len;
      thead->mem_len = tlen;
      thead->op_pid = op_pid;
      o_data = (pbuf + (ret & (mask)) + sizeof(proc_buf_head));
      return ret;
    }
  } while (1);
}

void que_proc_buf::write_cmt_mth(int64 getpos, int32 len) {
  int64 oldpos = getpos;
  if (unlikely((mask & oldpos) == 0)) {
    oldpos = atomic_load64(&que->roundend);
  }

  int32 i = QUE_LOOP_BUSY_COUNT;
  proc_buf_head *thead = (proc_buf_head *)(pbuf + (getpos & (mask)));
  int32 tlen = thead->mem_len;
  int64 tcmtpos = getpos + tlen;
  int64 wr = atomic_load64(&que->wrcmt);

  do {
    if (unlikely(wr >= tcmtpos))
      break;

    if (likely(oldpos <= wr)) {
      if (atomic_cas64_weak(&que->wrcmt, &wr, tcmtpos))
        break;
    } else {
      i--;
      if (i == 0) {
        i = QUE_LOOP_BUSY_COUNT / 2;
        CPU_PAUSE();
      }
    }
  } while (1);
}

int64 que_proc_buf::write_mp(const char *userdata, int32 len) {
  int32 i = QUE_LOOP_BUSY_COUNT;
  int32 cnt = 0;

  // const char *pdata = userdata;
  char *pmem = NULL;
  int32 tlen = ALIGN_UP(len + sizeof(proc_buf_head), align_n);

  int64 getpos = write_get_mth(pmem, len);
  if (unlikely(getpos == 0))
    return 0;
  std::memcpy(pmem, userdata, len);

  int64 dispos = atomic_load64(&que->discard_pos);
  do {
    int64 ter = atomic_load64(&que->roundend);
    int64 oldpos = (mask & getpos) != 0 ? getpos : ter;
    int64 tcmtpos = getpos + tlen;

    int64 wr = atomic_load64(&que->wrcmt);
    if (likely(wr < tcmtpos)) {
      if (likely(oldpos <= wr)) {
        if (atomic_cas64_weak(&que->wrcmt, &wr, tcmtpos)) {
          return getpos;
        } else {
          continue;
        }
      } else {
        i--;
        if (i > 0) {
          dispos = atomic_load64(&que->discard_pos);
          continue;
        }
        i = QUE_LOOP_BUSY_COUNT / 2;
        sched_yield();

        int64 tdis = atomic_load64(&que->discard_pos);
        if (tdis != dispos) {
          dispos = tdis;
          cnt = 0;
          continue;
        }

        cnt++;
        if (1 == check_cmt_write(cnt, dispos, oldpos, wr)) {
          cnt = 0;
          dispos = atomic_load64(&que->discard_pos);
        }
      }
    } else {
#ifdef NDEBUG
      // std::cerr<<"proc buf queue to write
      // again,wrcmt="<<wr<<",get_pos="<<getpos<<std::endl;
#endif
      i = QUE_LOOP_BUSY_COUNT;
      cnt = 0;

      getpos = write_get_mth(pmem, len);
      if (unlikely(getpos == 0))
        return 0;
      std::memcpy(pmem, userdata, len);

      dispos = atomic_load64(&que->discard_pos);
    }
  } while (1);
}

int32 que_proc_buf::check_cmt_write(int32 yield_cnt, int64 last_dis, int64 src_pos, int64 last_cmt) {
  /**
    检查写进程是否core还是提交慢。
    1.若进程core,其可能占据wrpos，而未设置roundend,头中的userlen,memlen和pid等
    2.core时，队列内存维持上次写入的数据，所以可能不可用于判断core后是否设置
    3.仍使用头中pid
  来判断进程是否core（虽然可能是上次的数据恰好是个存在的进程ID），
          以此判定是否为提交慢，对于进程存在=提交慢，等待较多的轮询
    4.断定写入异常后，将写入位置提交到下一个正常位置，并重新设置头，将pid置为异常，
          不可读，memlen设置为异常长度。
  **/
  proc_buf_head *thead = (proc_buf_head *)(pbuf + (last_cmt & (mask)));
  int64 cmt_pos = src_pos;
  int32 new_mem_len = 0;
  int32 tctl = 0;
  int64 tpid = 0;
  int64 tround = 0;

  if ((src_pos & mask) >= (last_cmt & mask)) {
    tpid = thead->op_pid;
    if (tpid > 0) {
      if (1 == comm_utils::exist_pid(tpid))
        tctl = 1;
    }
    new_mem_len = (int32)((src_pos - last_cmt) & 0xfffffff);
  } else {
    int64 tpos = last_cmt + mask + 1 - (last_cmt & mask);
    tround = atomic_load64(&que->roundend);
    if (last_cmt < tround) {
      tpid = thead->op_pid;
      if (tpid > 0) {
        if (1 == comm_utils::exist_pid(tpid))
          tctl = 1;
      }
      new_mem_len = (int32)((tround - last_cmt) & 0xfffffff);
      cmt_pos = tpos;
    } else if (last_cmt == tround) {
      thead = (proc_buf_head *)(pbuf);
      tpid = thead->op_pid;
      if (tpid > 0) {
        if (1 == comm_utils::exist_pid(tpid))
          tctl = 1;
      }
      tround = 0;
      new_mem_len = (int32)((src_pos - tpos) & 0xfffffff);
      cmt_pos = src_pos;
    } else {
      tpid = thead->op_pid;
      if (tpid > 0) {
        if (1 == comm_utils::exist_pid(tpid))
          tctl = 1;
      }
      tround = tpos - 1;
      new_mem_len = (int32)((tround - last_cmt) & 0xfffffff);
      cmt_pos = tpos;
    }
  }

  if (tpid != PROC_BUF_PID_INVALID) {
    // 还未设置op_pid,重新调度
    if (tctl == 0 && yield_cnt < PROC_BUF_YIELD_NOTEXIST)
      return 0;
    else if (tctl == 1 && yield_cnt < PROC_BUF_YIELD_EXIST)
      return 0;
  }

#ifdef NDEBUG
  /*std::cerr<<"proc buf queue to commit write,last_cmt="<<last_cmt\
                   <<",new_cmt="<<cmt_pos<<",discard_pos="<<que->discard_pos\
                   <<",yield_count="<<yield_cnt<<",dump_pid="<<tpid\
                   <<",roundend="<<tround<<",discard_len="<<new_mem_len<<std::endl;*/
#endif

  // int64 tdis = last_dis;
  if (last_dis >= last_cmt)
    return 1;
  // 只有唯一的写者可成功，进行后续操作。其他失败者应重新检查新的位置
  if (!atomic_cas64(&que->discard_pos, &last_dis, last_cmt))
    return 1;

  if (tround > atomic_load64(&que->roundend))
    atomic_store64(&que->roundend, tround);

  thead->user_len = 0;
  thead->mem_len = new_mem_len;
  atomic_write_fence();
  thead->op_pid = PROC_BUF_PID_INVALID;

  if (!atomic_cas64(&que->wrcmt, &last_cmt, cmt_pos)) {
    // std::cerr<<"proc buf queue error to check pid and commit
    // pos,last_cmt="<<last_cmt
    //	<<",new_cmt="<<cmt_pos<<std::endl;
    assert(0);
  }

  return 1;
}

int64 que_proc_buf::read_get(char *&o_data) {

  do {

    int64 wr = atomic_load64(&que->wrcmt);
    int64 rd = que->rdcmt;
    if (unlikely(rd >= wr))
      return 0;

    int64 t = (rd & mask);
    o_data = (pbuf + t + sizeof(proc_buf_head));
    // if(likely((wr&mask) != 0)){
    t = (wr & mask) - t;
    if (likely(t > 0)) {
      proc_buf_head *thead = (proc_buf_head *)(pbuf + (rd & (mask)));
      if (likely(thead->op_pid != PROC_BUF_PID_INVALID)) {
        return rd;
      } else {
        atomic_store64(&que->rdcmt, rd + thead->mem_len);
        continue;
      }
    }
    //}

    int64 tre = atomic_load64(&que->roundend);
    if (tre >= wr || tre < rd) {
      return 0;
    }
    t = tre - rd;
    if (t > 0) {
      proc_buf_head *thead = (proc_buf_head *)(pbuf + (rd & (mask)));
      if (likely(thead->op_pid != PROC_BUF_PID_INVALID)) {
        return rd;
      } else {
        atomic_store64(&que->rdcmt, rd + thead->mem_len);
        continue;
      }
    }

    t = (rd & mask);
    if (t != 0) {
      rd += (mask + 1 - t);
    }
    t = wr - rd;
    if (t > 0) {
      atomic_store64(&que->rdcmt, rd);
      o_data = (pbuf) + (rd & mask) + sizeof(proc_buf_head);
      proc_buf_head *thead = (proc_buf_head *)(pbuf + (rd & (mask)));
      if (likely(thead->op_pid != PROC_BUF_PID_INVALID)) {
        return rd;
      } else {
        atomic_store64(&que->rdcmt, rd + thead->mem_len);
        continue;
      }
    }
    return 0;

  } while (1);
}
int64 que_proc_buf::read_get(char *&o_data, int64 read_pos) {
  int64 rd = read_pos;

  do {

    int64 wr = atomic_load64(&que->wrcmt);
    if (unlikely(rd >= wr))
      return 0;
    int64 t = atomic_load64(&que->rdcmt);
    if (unlikely(rd < t)) {
      rd = t;
    }

    t = (rd & mask);
    o_data = (pbuf + t + sizeof(proc_buf_head));
    // if(likely((wr&mask) != 0)){
    t = (wr & mask) - t;
    if (likely(t > 0)) {
      proc_buf_head *thead = (proc_buf_head *)(pbuf + (rd & (mask)));
      if (likely(thead->op_pid != PROC_BUF_PID_INVALID)) {
        return rd;
      } else {
        rd += thead->mem_len;
        continue;
      }
    }
    //}

    int64 tre = atomic_load64(&que->roundend);
    if (tre >= wr || tre < rd) {
      return 0;
    }
    t = tre - rd;
    if (t > 0) {
      proc_buf_head *thead = (proc_buf_head *)(pbuf + (rd & (mask)));
      if (likely(thead->op_pid != PROC_BUF_PID_INVALID)) {
        return rd;
      } else {
        rd += thead->mem_len;
        continue;
      }
    }

    t = (rd & mask);
    if (t != 0) {
      rd += (mask + 1 - t);
    }
    t = wr - rd;
    if (t > 0) {
      o_data = (pbuf) + (rd & mask) + sizeof(proc_buf_head);
      proc_buf_head *thead = (proc_buf_head *)(pbuf + (rd & (mask)));
      if (likely(thead->op_pid != PROC_BUF_PID_INVALID)) {
        return rd;
      } else {
        rd += thead->mem_len;
        continue;
      }
    }
    return 0;

  } while (1);
}

int64 que_proc_buf::need_buf_size(int64 que_size) {
  int32 t = comm_utils::calc_power(que_size);
  return (1L << t);
}
void que_proc_buf::init_set(int64 tsize, int32 align_byte) {
  atomic_store64(&que->wrcmt, CACHE_ALIGN_SIZE);
  atomic_store64(&que->roundend, 0);
  atomic_store64(&que->wrpos, CACHE_ALIGN_SIZE);
  pbuf = NULL;
  que->mask = 0;
  atomic_store64(&que->rdcmt, CACHE_ALIGN_SIZE);

  if (align_byte == SIMD_ALIGN_SIZE)
    que->align_n = SIMD_ALIGN_SIZE;
  else
    que->align_n = 8;

  que->create_mem = 0;
  que->userpos[0] = CACHE_ALIGN_SIZE;
  que->userpos[1] = CACHE_ALIGN_SIZE;
  std::memset(que->userdata, 0, QUE_USER_DATA_LEN);
  que->filled = 0;
  atomic_store64(&que->mask, tsize - 1);
}
int32 que_proc_buf::init(const char *pname, int64 que_size, int32 is_continue, int32 is_create, int32 align_byte) {
  if (que_size <= 0 || NULL == pname)
    return LBERR_ARGV_WRONG;
  int64 tnum = need_buf_size(que_size);

  int64 ts = tnum;
  ts += sizeof(que_proc_buf_info);
  char tname[64];
  comm_utils::str_copy_format(tname, pname, sizeof(tname));

  void *pshm = NULL;
  int32 shmisexist = 0;
  int32 ishuge = 0;
  if (ts >= HUGE_PAGE_SIZE) {
    ishuge = 1;
  }

  shmisexist = comm_utils::map_shm(pshm, tname, ts, ishuge, is_create);
  if (shmisexist < 0) {
    return shmisexist;
  }

  que = reinterpret_cast<que_proc_buf_info *>(pshm);
  pbuf = ((char *)pshm) + sizeof(que_proc_buf_info);
  op_pid = comm_utils::get_pid();

  if (is_create == 1) {
    if (shmisexist == 0 || is_continue == 0 || que->mask + 1 != tnum) {
      init_set(tnum, align_byte);
    } else {
    }
    que->create_mem = 1;
  } else {
    while (que->mask == 0) {
      comm_utils::sleep_us(300);
    }
    if (que->mask + 1 != tnum)
      return LBERR_OBJ_NUM_LIMIT;
  }

  mask = que->mask;
  align_n = que->align_n;
  return 0;
}

int32 que_proc_buf::init(que_proc_buf_info *pinfo, char *shm_addr, int64 que_size, int32 shm_isexist, int32 is_continue,
                         int32 is_create, int32 align_byte) {
  if (que_size <= 0 || NULL == pinfo || NULL == shm_addr)
    return LBERR_ARGV_WRONG;
  pbuf = shm_addr;
  que = pinfo;
  op_pid = comm_utils::get_pid();

  int64 tnum = need_buf_size(que_size);
  if (is_create == 1) {
    if (shm_isexist == 0 || is_continue == 0 || (que->mask + 1 != tnum)) {
      init_set(tnum, align_byte);
    } else {
    }
  } else {
    while (que->mask == 0) {
      comm_utils::sleep_us(300);
    }
    if (que->mask + 1 != tnum)
      return LBERR_OBJ_NUM_LIMIT;
  }

  mask = que->mask;
  align_n = que->align_n;
  return 0;
}
void que_proc_buf::free_buf() {
  if (NULL == pbuf || NULL == que)
    return;
  if (que->create_mem == 1) {
    comm_utils::unmap_shm(reinterpret_cast<void *>(que), que->mask + 1 + sizeof(que_proc_buf_info));
  }
  pbuf = NULL;
  que = NULL;
}

} // namespace lb_common

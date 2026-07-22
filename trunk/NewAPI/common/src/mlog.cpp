#include "mlog.h"
#include "comm_errno.h"
#include "mutils.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace lb_common {

static const char *g_lblog_level[] = {"[debug]", "[inform]", "[warning]", "[error]", "[fatal]"};
static const int32 g_lblog_level_len[] = {7, 8, 9, 7, 7};

void lb_log::write_log(const char *pmsg, int32 msglen, int32 msglevel) {
  if (level > msglevel || msglen <= 0)
    return;

  if (NULL != pq) {
    char *pbuf;
    int32 tlen = sizeof(log_que_head) + msglen;
    int64 tqpos = pq->write_get_mth(pbuf, tlen);
    if (unlikely(tqpos == 0))
      return;

    log_que_head *ph = (log_que_head *)pbuf;
    ph->alen = tlen;
    ph->clevel = msglevel;
    memcpy(pbuf + sizeof(log_que_head), pmsg, msglen);
    pq->write_cmt_mth(tqpos, tlen);
    trigger();
  } else {
    write_file(pmsg);
  }
}
void lb_log::write_file(const char *pmsg) {
  clock_guard<cmutex> tlockguard(filelock);
  fprintf(fp, "%s\n", pmsg);
  // fflush(fp);

  linenum++;
  if (linenum >= LOG_FILE_LINE_MAXNUM) {
    fflush(fp);

    FILE *tfp = NULL;
    char tmpfile[LBLOG_FILE_PATH_LEN] = {0};
    snprintf(tmpfile, LBLOG_FILE_PATH_LEN - 1, "%s_%d_%d.log", pathname, date, fileno);
    tfp = fopen(tmpfile, "w+");
    if (NULL != tfp) {
      fclose(fp);
      fileno++;
      linenum = 0;
      fp = tfp;
    }
  }
}

int32 lb_log::int_str(char *o_buf, int64 num) {
  const char *preval = "0123456789";
  char tstr[64];
  uint64 tv;
  int32 tlen = 0;
  if (num >= 0) {
    tv = num;
  } else {
    tv = -num;
  }
  do {
    uint64 td = tv / 10;
    tstr[tlen++] = preval[tv - td * 10];
    tv = td;
  } while (tv);

  // tstr[tlen++]='\0';

  int32 j = 0;
  if (num < 0) {
    o_buf[0] = '-';
    j = 1;
  }
  for (int32 i = tlen - 1; i >= 0; i--) {
    o_buf[j++] = tstr[i];
  }
  return j;
}

int32 lb_log::format_level(char *o_buf, int32 wlevel) {
  const char *plstr;
  int32 tlen = 0;
  if (wlevel < 4) {
    tlen = g_lblog_level_len[wlevel];
    plstr = g_lblog_level[wlevel];
  } else {
    tlen = g_lblog_level_len[4];
    plstr = g_lblog_level[4];
  }
  for (int32 i = 0; i < tlen; i++) {
    o_buf[i] = plstr[i];
  }
  return tlen;
}

int32 lb_log::log_pre(char *o_buf, const char *file, int32 line, int32 wlevel) {
  uint64 ticktm = comm_utils::get_tick();
  ticktm = ticktm % 1000000000;

  int32 rtlen = format_level(o_buf, wlevel);
  int32 i = 0;

  struct tm tmloc;
  time_t curtime = time(0);
  localtime_r(&curtime, &tmloc);

  //[%02d:%02d:%02d.%d]
  o_buf[rtlen++] = '[';
  if (tmloc.tm_hour < 10) {
    o_buf[rtlen++] = '0';
    o_buf[rtlen++] = tmloc.tm_hour + '0';
  } else if (tmloc.tm_hour < 20) {
    o_buf[rtlen++] = '1';
    o_buf[rtlen++] = tmloc.tm_hour - 10 + '0';
  } else {
    o_buf[rtlen++] = '2';
    o_buf[rtlen++] = tmloc.tm_hour - 20 + '0';
  }
  o_buf[rtlen++] = ':';
  if (tmloc.tm_min < 10) {
    o_buf[rtlen++] = '0';
    o_buf[rtlen++] = tmloc.tm_min + '0';
  } else if (tmloc.tm_min < 20) {
    o_buf[rtlen++] = '1';
    o_buf[rtlen++] = tmloc.tm_min - 10 + '0';
  } else if (tmloc.tm_min < 30) {
    o_buf[rtlen++] = '2';
    o_buf[rtlen++] = tmloc.tm_min - 20 + '0';
  } else if (tmloc.tm_min < 40) {
    o_buf[rtlen++] = '3';
    o_buf[rtlen++] = tmloc.tm_min - 30 + '0';
  } else if (tmloc.tm_min < 50) {
    o_buf[rtlen++] = '4';
    o_buf[rtlen++] = tmloc.tm_min - 40 + '0';
  } else {
    o_buf[rtlen++] = '5';
    o_buf[rtlen++] = tmloc.tm_min - 50 + '0';
  }
  o_buf[rtlen++] = ':';
  if (tmloc.tm_sec < 10) {
    o_buf[rtlen++] = '0';
    o_buf[rtlen++] = tmloc.tm_sec + '0';
  } else if (tmloc.tm_sec < 20) {
    o_buf[rtlen++] = '1';
    o_buf[rtlen++] = tmloc.tm_sec - 10 + '0';
  } else if (tmloc.tm_sec < 30) {
    o_buf[rtlen++] = '2';
    o_buf[rtlen++] = tmloc.tm_sec - 20 + '0';
  } else if (tmloc.tm_sec < 40) {
    o_buf[rtlen++] = '3';
    o_buf[rtlen++] = tmloc.tm_sec - 30 + '0';
  } else if (tmloc.tm_sec < 50) {
    o_buf[rtlen++] = '4';
    o_buf[rtlen++] = tmloc.tm_sec - 40 + '0';
  } else {
    o_buf[rtlen++] = '5';
    o_buf[rtlen++] = tmloc.tm_sec - 50 + '0';
  }

  o_buf[rtlen++] = '.';
  i = int_str(o_buf + rtlen, ticktm);
  rtlen += i;
  o_buf[rtlen++] = ']';

  //[os_errno:%d]
  o_buf[rtlen++] = '[';
  o_buf[rtlen++] = 'o';
  o_buf[rtlen++] = 's';
  o_buf[rtlen++] = '_';
  o_buf[rtlen++] = 'e';
  o_buf[rtlen++] = 'r';
  o_buf[rtlen++] = 'r';
  o_buf[rtlen++] = 'n';
  o_buf[rtlen++] = 'o';
  o_buf[rtlen++] = ':';
  ticktm = errno;
  i = int_str(o_buf + rtlen, ticktm);
  rtlen += i;
  o_buf[rtlen++] = ']';

  //[file:%s line:%d]
  o_buf[rtlen++] = '[';
  o_buf[rtlen++] = 'f';
  o_buf[rtlen++] = 'i';
  o_buf[rtlen++] = 'l';
  o_buf[rtlen++] = 'e';
  o_buf[rtlen++] = ':';
  const char *pc = file;
  const char *pbeg = pc;
  while (*pc != '\0') {
    if (*pc == '/')
      pbeg = pc + 1;
    pc++;
  }
  pc = pbeg;
  while (*pc != '\0') {
    o_buf[rtlen++] = *pc++;
  }
  o_buf[rtlen++] = ' ';
  o_buf[rtlen++] = 'l';
  o_buf[rtlen++] = 'i';
  o_buf[rtlen++] = 'n';
  o_buf[rtlen++] = 'e';
  o_buf[rtlen++] = ':';
  ticktm = line;
  i = int_str(o_buf + rtlen, ticktm);
  rtlen += i;
  o_buf[rtlen++] = ']';
  o_buf[rtlen++] = ',';

  return rtlen;
}

lb_log_hand &lb_log_hand::log_begin(int32 loglevel) {
  if (loglevel < 4) {
    curlevel = loglevel;
    curlen = 0;
  } else {
    curlevel = 4;
    curlen = 0;
  }
  if (no_log()) {
    return *this;
  }

  curlen = mlog->format_level(buf, curlevel);
  return *this;
}

lb_log_hand &lb_log_hand::log_begin(const char *file, int32 line, int32 loglevel) {
  assert(NULL != file);

  if (loglevel < 4) {
    curlevel = loglevel;
    curlen = 0;
  } else {
    curlevel = 4;
    curlen = 0;
  }
  if (no_log()) {
    return *this;
  }

  curlen = mlog->log_pre(buf, file, line, curlevel);
  return *this;
}

void lb_log::do_work() {
  int64 tlen;
  char *pmsg;
  log_que_head *ph;
  while ((tlen = pq->read_get(pmsg)) > 0) {
    ph = (log_que_head *)pmsg;
    write_file(pmsg + sizeof(log_que_head));
    pq->read_cmt(ph->alen);
  }
}

int32 lb_log::change_date(int32 newdate) {
  if (newdate == date)
    return 0;

  FILE *tfp = NULL;
  char tmpfile[LBLOG_FILE_PATH_LEN] = {0};
  snprintf(tmpfile, LBLOG_FILE_PATH_LEN - 1, "%s_%d_%d.log", pathname, newdate, 0);

  clock_guard<cmutex> tlockguard(filelock);

  tfp = fopen(tmpfile, "w+");
  if (NULL != tfp) {
    if (NULL != fp) {
      fflush(fp);
      fclose(fp);
    }

    fileno = 0;
    linenum = 0;
    date = newdate;
    fp = tfp;
    return 0;
  }
  return LBERR_OBJ_OPEN_FAIL;
}

int32 lb_log::open_log(const char *path, const char *prename, int32 loglevel, int32 curdate, int32 quesize,
                       int32 cpuid) {
  char tmppath[LBLOG_FILE_PATH_LEN];
  memset(tmppath, 0, sizeof(tmppath));

  clock_guard<cmutex> tlockguard(filelock);

  if (cpuid <= 0)
    cpuid = -1;

  date = curdate;
  if (loglevel < 4)
    level = loglevel;
  else
    level = 4;

  int32 ret = 0;
  int32 len = 0;
  if (NULL != path) {
    len = comm_utils::str_copy_format(tmppath, path, sizeof(tmppath));
  }
  if (NULL == path || len <= 1) {
    snprintf(tmppath, sizeof(tmppath), "%s", LOG_DEFAULT_PATH);
  } else {
    if (tmppath[len - 1] != '/') {
      tmppath[len] = '/';
      len++;
    }
  }
  memset(pathname, 0, sizeof(pathname));
  snprintf(pathname, sizeof(pathname) - 1, "%s%s", tmppath, prename);

  if ((ret = comm_utils::make_path(tmppath)) < 0)
    return ret;

  int32 i = 0;
  while (1) {
    memset(tmppath, 0, sizeof(tmppath));
    snprintf(tmppath, LBLOG_FILE_PATH_LEN - 1, "%s_%d_%d.log", pathname, date, i);
    if (comm_utils::exist_file(tmppath) == 0)
      break;
    i++;
  }

  fileno = i;
  linenum = 0;
  memset(tmppath, 0, sizeof(tmppath));
  snprintf(tmppath, LBLOG_FILE_PATH_LEN - 1, "%s_%d_%d.log", pathname, date, fileno);
  FILE *tfp = fopen(tmppath, "w+");
  if (NULL == tfp) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  fp = tfp;

  if (quesize == 0) {
    pq = NULL;
    return 0;
  }

  if (quesize < (1 << 25))
    quesize = (1 << 25);

  void *tpm = comm_utils::aligned_malloc(sizeof(que_mth_buf), CACHE_ALIGN_SIZE);
  if (NULL == tpm) {
    destroy();
    return LBERR_MEM_ALLOC_FAIL;
  }
  pq = reinterpret_cast<que_mth_buf *>(tpm);
  if ((ret = pq->init(quesize, 8)) < 0) {
    destroy();
    return ret;
  }
  if ((ret = init_th(10000, cpuid, 500)) < 0) {
    destroy();
    return ret;
  }
  if ((ret = run()) < 0) {
    destroy();
    return ret;
  }
  return 0;
}
void lb_log::destroy() {
  if (NULL != pq) {
    join();
    comm_utils::aligned_free(reinterpret_cast<void *>(pq));
    pq = NULL;
  }

  if (NULL != fp) {
    fflush(fp);
    fclose(fp);
    fp = NULL;
  }
}
void lb_log::close_log() { destroy(); }

void lb_log_hand::init(lb_log *plog, int32 mylevel) {
  assert(NULL != plog);
  if (mylevel < 4)
    curlevel = mylevel;
  else
    curlevel = 4;
  wrlevel = plog->get_level();
  curlen = 0;
  mlog = plog;
}

} // namespace lb_common

/**
 * @file comm_errno.cpp
 * @brief 错误码处理实现
 *
 * @see comm_errno.h
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include <cerrno>
#include <string>

namespace lb_common {

static int cpy_err_str(char *buf, const char *src, int dstoff) {
  // dstoff 表示目标缓冲中已有数据长度，避免在 dstoff 较大时越界写入
  // buf[LBCOMM_ERRMSG_MAX_LEN]。第二次调用时 dstoff=tlen，tlen 可达 254，
  // 旧版逻辑会写穿 err_msg[256] 边界。
  int i = dstoff;
  int avail = LBCOMM_ERRMSG_MAX_LEN - 1 - dstoff; // 剩余可写空间
  if (unlikely(avail <= 0)) {
    buf[LBCOMM_ERRMSG_MAX_LEN - 1] = '\0';
    return dstoff;
  }
  char *dst = buf + dstoff;
  for (; i < dstoff + avail; i++) {
    char c = *src;
    *dst = c;
    if (c == '\0')
      break;
    dst++;
    src++;
  }
  buf[i] = '\0';
  return i;
}

#define LBERR_SEQNO_MAX 40 // 索引 0 = "成功"，索引 1~39 对应 LBERR_ARGV_WRONG(-1) ~ LBERR_CH_LINK_BROKEN(-39)

void perror_lb(lb_comm_err &o_errmsg, int err_code) {
  // 在函数入口即保存 errno，避免后续 strerror 等调用修改全局 errno 导致消息不一致
  int saved_errno = errno;
  static const char *g_lberr_msg[LBERR_SEQNO_MAX] = {
      "成功",                   // 索引0 ok
      "参数错误",               // 索引1 LBERR_ARGV_WRONG (-1)
      "不支持的操作",           // 索引2 LBERR_NOT_SUPPORT (-2)
      "申请内存失败",           // 索引3 LBERR_MEM_ALLOC_FAIL (-3)
      "属性初始化失败",         // 索引4 LBERR_ATTR_INIT_FAIL (-4)
      "属性获取失败",           // 索引5 LBERR_ATTR_GET_FAIL (-5)
      "属性设置失败",           // 索引6 LBERR_ATTR_SET_FAIL (-6)
      "目标对象打开失败",       // 索引7 LBERR_OBJ_OPEN_FAIL (-7)
      "目标对象初始化失败",     // 索引8 LBERR_OBJ_INIT_FAIL (-8)
      "目标对象启动失败",       // 索引9 LBERR_OBJ_START_FAIL (-9)
      "目标对象停止失败",       // 索引10 LBERR_OBJ_STOP_FAIL (-10)
      "目标对象关闭失败",       // 索引11 LBERR_OBJ_CLOSE_FAIL (-11)
      "目标对象已存在",         // 索引12 LBERR_OBJ_HAVE_EXIST (-12)
      "目标对象不存在",         // 索引13 LBERR_OBJ_NOT_HAVE (-13)
      "目标对象读取失败",       // 索引14 LBERR_OBJ_READ_FAIL (-14)
      "目标对象读取部分不完整", // 索引15 LBERR_OBJ_READ_PART (-15)
      "目标对象写入失败",       // 索引16 LBERR_OBJ_WRITE_FAIL (-16)
      "目标对象写入部分不完整", // 索引17 LBERR_OBJ_WRITE_PART (-17)
      "目标对象为空",           // 索引18 LBERR_OBJ_IS_EMPTY (-18)
      "目标对象已满",           // 索引19 LBERR_OBJ_IS_FULL (-19)
      "目标对象状态限制操作",   // 索引20 LBERR_OBJ_STATE_LIMIT (-20)
      "目标对象大小数量限制",   // 索引21 LBERR_OBJ_NUM_LIMIT (-21)
      "目标对象添加失败",       // 索引22 LBERR_OBJ_ADD_FAIL (-22)
      "目标对象修改失败",       // 索引23 LBERR_OBJ_MOD_FAIL (-23)
      "目标对象删除失败",       // 索引24 LBERR_OBJ_DEL_FAIL (-24)
      "目标对象查找失败",       // 索引25 LBERR_OBJ_FIND_FAIL (-25)
      "目标对象等待失败",       // 索引26 LBERR_OBJ_WAIT_FAIL (-26)
      "等待超时",               // 索引27 LBERR_TIME_WAIT_OUT (-27)
      "获取时间失败",           // 索引28 LBERR_TIME_TAKE_FAIL (-28)
      "映射共享内存失败",       // 索引29 LBERR_SHM_MAP_FAIL (-29)
      "锁不一致",               // 索引30   LBERR_LOCK_NOT_CONSIST (-30)
      "锁自恢复故障",           // 索引31 LBERR_LOCK_NOT_RECOVER (-31)
      "锁获取失败",             // 索引32 LBERR_LOCK_TAKE_FAIL (-32)
      "增加引用计数失败",       // LBERR_REF_ADD_FAIL    -33
      "通道链接失败",           // LBERR_CH_CONNECT_FAIL  -34
      "通道监听失败",           // LBERR_CH_LISTEN_FAIL   -35
      "链接接受失败",           // LBERR_CH_ACCEPT_FAIL   -36
      "通道监听故障",           // LBERR_CH_EPOLL_FAIL    -37
      "通道绑定失败",           // LBERR_CH_BIND_FAIL   -38
      "通道链接断开"            // LBERR_CH_LINK_BROKEN   -39
  };

  o_errmsg.err_code = err_code;

  // 用 int64 承接取反结果：err_code=INT_MIN 时 -err_code 对 int 是有符号溢出（UB），
  // 回绕为负数会通过 i<LBERR_SEQNO_MAX 检查并以负索引访问 g_lberr_msg 导致越界。
  int64 i = (err_code < 0) ? -(int64)err_code : (int64)0;
  const char *tmsg = NULL;
  if (i < LBERR_SEQNO_MAX) {
    tmsg = g_lberr_msg[i];
    int tlen = cpy_err_str(o_errmsg.err_msg, tmsg, 0);

    if (saved_errno != 0 && tlen < LBCOMM_ERRMSG_MAX_LEN - 16) {
      o_errmsg.err_msg[tlen] = ',';
      tlen++;
      // M11: std::strerror 返回指向静态缓冲区的指针，多线程并发调用会互相覆盖。
      // 改用线程安全的 strerror_r。版本判定：g++ 在 C++ 模式下恒预定义 _GNU_SOURCE，
      // features.h 随之定义 _POSIX_C_SOURCE/_XOPEN_SOURCE，因此不能用"未定义
      // _POSIX_C_SOURCE"来识别 GNU 版本；__USE_GNU（features.h 内部宏，包含
      // <string.h> 后可用）才是 GNU 版（返回 char*）的可靠标志，否则为 XSI 版（返回 int）。
      char tbuf[128] = {0};
      const char *pmsg = NULL;
#if defined(__GLIBC__) && defined(__USE_GNU)
      pmsg = strerror_r(saved_errno, tbuf, sizeof(tbuf)); // GNU 版本返回 char*
#else
      if (strerror_r(saved_errno, tbuf, sizeof(tbuf)) != 0) // XSI 版本返回 int
        tbuf[0] = '\0';
      pmsg = tbuf;
#endif
      std::string tsys_errmsg(pmsg != NULL ? pmsg : "");
      o_errmsg.err_msg_len = cpy_err_str(o_errmsg.err_msg, tsys_errmsg.c_str(), tlen);
    } else {
      o_errmsg.err_msg_len = tlen;
    }
  } else {
    tmsg = "未知错误";
    o_errmsg.err_msg_len = cpy_err_str(o_errmsg.err_msg, tmsg, 0);
  }

  return;
}

} // namespace lb_common

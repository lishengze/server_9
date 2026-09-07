/**
 * @file comm_errno.cpp
 * @brief 错误码处理实现
 *
 * @see comm_errno.h
 */

#include "comm_errno.h"
#include <cerrno>
#include <string>

namespace lb_common {

static int cpy_err_str(char *buf, const char *src, int dstoff) {
  int i = dstoff;
  int e = LBCOMM_ERRMSG_MAX_LEN - 2;
  char *dst = buf + dstoff;
  for (; i < e; i++) {
    char c = *src;
    *dst = c;
    if (c == '\0')
      break;
    dst++;
    src++;
  }
  dst[i] = '\0';
  dst[i + 1] = '\0';
  return i;
}

#define LBERR_SEQNO_MAX 40

void perror_lb(lb_comm_err &o_errmsg, int err_code) {
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
      "锁自恢复故障",           // 索引31 LBERR_LOCK_NOT_RECOVE (-31)
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

  int i = (err_code < 0) ? (0 - err_code) : 0;
  const char *tmsg = NULL;
  if (i < LBERR_SEQNO_MAX) {
    tmsg = g_lberr_msg[i];
    int tlen = cpy_err_str(o_errmsg.err_msg, tmsg, 0);

    if (errno != 0 && tlen < LBCOMM_ERRMSG_MAX_LEN - 16) {
      o_errmsg.err_msg[tlen] = ',';
      tlen++;
      // char tsys_errmsg[128] = {0};
      // strerror_r(errno, tsys_errmsg, sizeof(tsys_errmsg));
      std::string tsys_errmsg = std::strerror(errno);
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

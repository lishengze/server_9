#pragma once

/**
 * @file comm_errno.h
 * @brief 错误码定义与错误处理模块
 *
 * 本模块定义了统一的错误码体系和错误信息结构体。
 * 所有模块的错误码都使用 LBERR_ 前缀，返回负值表示错误。
 */

#include <cerrno>
#include <cstring>

namespace lb_common {

/**
 * @brief 错误信息最大长度
 */
#define LBCOMM_ERRMSG_MAX_LEN 256

/**
 * @brief 错误信息结构体
 *
 * @details 封装错误码和对应的错误消息，用于统一的错误处理。
 */
struct lb_comm_err {
  int err_code;                        ///< 错误码（负值表示错误）
  int err_msg_len;                     ///< 错误消息长度
  char err_msg[LBCOMM_ERRMSG_MAX_LEN]; ///< 错误消息缓冲区

  /**
   * @brief 清除错误信息
   *
   * 重置所有成员为初始状态
   */
  void clear() {
    err_code = 0;
    err_msg_len = 0;
    std::memset(err_msg, 0, sizeof(err_msg));
  }
};

/**
 * @brief 将错误码转换为错误消息
 *
 * @param[out] o_errmsg 输出的错误信息结构体
 * @param[in] err_code 错误码
 * @return 无
 */
extern void perror_lb(lb_comm_err &o_errmsg, int err_code);

/**
 * @name 错误码定义
 * @brief 统一错误码（负值），涵盖参数错误、资源操作、锁、通道等类别
 * @{
 */
#define LBERR_ARGV_WRONG       (-1)  ///< 参数错误
#define LBERR_NOT_SUPPORT      (-2)  ///< 不支持的操作
#define LBERR_MEM_ALLOC_FAIL   (-3)  ///< 申请内存失败
#define LBERR_ATTR_INIT_FAIL   (-4)  ///< 属性初始化失败
#define LBERR_ATTR_GET_FAIL    (-5)  ///< 属性获取失败
#define LBERR_ATTR_SET_FAIL    (-6)  ///< 属性设置失败
#define LBERR_OBJ_OPEN_FAIL    (-7)  ///< 目标对象打开失败
#define LBERR_OBJ_INIT_FAIL    (-8)  ///< 目标对象初始化失败
#define LBERR_OBJ_START_FAIL   (-9)  ///< 目标对象启动失败
#define LBERR_OBJ_STOP_FAIL    (-10) ///< 目标对象停止失败
#define LBERR_OBJ_CLOSE_FAIL   (-11) ///< 目标对象关闭失败
#define LBERR_OBJ_HAVE_EXIST   (-12) ///< 目标对象已存在
#define LBERR_OBJ_NOT_HAVE     (-13) ///< 目标对象不存在
#define LBERR_OBJ_READ_FAIL    (-14) ///< 目标对象读取失败
#define LBERR_OBJ_READ_PART    (-15) ///< 目标对象读取部分不完整
#define LBERR_OBJ_WRITE_FAIL   (-16) ///< 目标对象写入失败
#define LBERR_OBJ_WRITE_PART   (-17) ///< 目标对象写入部分不完整
#define LBERR_OBJ_IS_EMPTY     (-18) ///< 目标对象为空
#define LBERR_OBJ_IS_FULL      (-19) ///< 目标对象已满
#define LBERR_OBJ_STATE_LIMIT  (-20) ///< 目标对象状态限制操作
#define LBERR_OBJ_NUM_LIMIT    (-21) ///< 目标对象大小数量限制
#define LBERR_OBJ_ADD_FAIL     (-22) ///< 目标对象添加失败
#define LBERR_OBJ_MOD_FAIL     (-23) ///< 目标对象修改失败
#define LBERR_OBJ_DEL_FAIL     (-24) ///< 目标对象删除失败
#define LBERR_OBJ_FIND_FAIL    (-25) ///< 目标对象查找失败
#define LBERR_OBJ_WAIT_FAIL    (-26) ///< 目标对象等待失败
#define LBERR_TIME_WAIT_OUT    (-27) ///< 等待超时
#define LBERR_TIME_TAKE_FAIL   (-28) ///< 获取时间失败
#define LBERR_SHM_MAP_FAIL     (-29) ///< 映射共享内存失败
#define LBERR_LOCK_NOT_CONSIST (-30) ///< 锁不一致
#define LBERR_LOCK_NOT_RECOVE  (-31) ///< 锁自恢复故障
#define LBERR_LOCK_TAKE_FAIL   (-32) ///< 锁获取失败
#define LBERR_REF_ADD_FAIL     (-33) ///< 增加引用计数失败
#define LBERR_CH_CONNECT_FAIL  (-34) ///< 通道链接失败
#define LBERR_CH_LISTEN_FAIL   (-35) ///< 通道监听失败
#define LBERR_CH_ACCEPT_FAIL   (-36) ///< 链接接受失败
#define LBERR_CH_EPOLL_FAIL    (-37) ///< 通道监听故障
#define LBERR_CH_BIND_FAIL     (-38) ///< 通道绑定失败
#define LBERR_CH_LINK_BROKEN   (-39) ///< 通道链接断开
/** @} */

} // namespace lb_common

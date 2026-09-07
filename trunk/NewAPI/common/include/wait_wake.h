#pragma once

/**
 * @file wait_wake.h
 * @brief 事件唤醒机制模块
 *
 * 提供基于Linux eventfd的事件唤醒功能，用于线程间或进程间的事件通知。
 * 支持单次唤醒和多次唤醒模式，适用于epoll等事件驱动机制。
 *
 * 主要特性：
 * - 基于eventfd的高性能事件通知
 * - 支持信号量模式和计数器模式
 * - 非阻塞I/O操作
 * - 线程安全的事件通知
 * - 自动资源管理
 *
 * 使用场景：
 * - epoll事件驱动的唤醒机制
 * - 线程间事件通知
 * - 异步任务调度
 * - 事件循环的退出通知
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include <unistd.h>

namespace lb_common {

/**
 * @brief 事件唤醒类
 *
 * 基于Linux eventfd实现的事件唤醒机制，用于高效的事件通知。
 * 提供初始化、唤醒、重置和销毁等基本操作。
 */
class event_wake {
private:
  int32 ev_fd; ///< eventfd文件描述符

public:
  /**
   * @brief 初始化事件唤醒对象
   *
   * 创建eventfd文件描述符并设置为非阻塞模式。
   *
   * @param[in] need_multicast
   * 是否需要支持多次唤醒，1=信号量模式，0=计数器模式，默认为1
   * @return int32 0表示成功，负数表示错误码
   */
  int32 init(int32 need_multicast = 1);

  /**
   * @brief 获取eventfd文件描述符
   *
   * @return int32 返回eventfd文件描述符，-1表示未初始化
   */
  FORCE_INLINE int32 get_fd() const { return ev_fd; }

  /**
   * @brief 触发唤醒事件
   *
   * 向eventfd写入数据以触发唤醒事件。
   *
   * @param[in] ctl 写入的控制值，默认为1
   * @return int32 0表示成功，LBERR_OBJ_WRITE_FAIL表示写入失败
   */
  FORCE_INLINE int32 wake(int64 ctl = 1) {
    if (write(ev_fd, &ctl, sizeof(int64)) > 0) {
      return 0;
    }
    return LBERR_OBJ_WRITE_FAIL;
  }

  /**
   * @brief 重置唤醒状态
   *
   * 从eventfd读取数据以清除唤醒状态，使其可以再次被唤醒。
   * 该函数为非阻塞操作，如果没有数据可读会立即返回。
   */
  FORCE_INLINE void reset() {
    int64 data;
    read(ev_fd, &data, sizeof(int64));
  }

  /**
   * @brief 销毁事件唤醒对象
   *
   * 关闭eventfd文件描述符并重置状态。
   */
  void destroy();

  /**
   * @brief 构造函数
   */
  event_wake() { ev_fd = -1; }

  /**
   * @brief 析构函数
   *
   * 自动调用destroy()清理资源
   */
  ~event_wake() { destroy(); }
};

} // namespace lb_common

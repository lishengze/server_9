#pragma once

/**
 * @file time_thread.h
 * @brief 定时器线程管理模块
 *
 * 提供基于线程的定时器管理功能，支持单次和周期性定时器。
 * 使用epoll事件驱动机制，支持高并发环境下的定时器处理。
 *
 * 主要特性：
 * - 基于epoll的事件驱动定时器
 * - 支持单次和周期性定时器
 * - 线程安全的定时器操作
 * - 高效的定时器管理
 * - Linux专用优化实现
 *
 * 使用场景：
 * - 网络超时管理
 * - 周期性任务调度
 * - 事件驱动架构
 * - 异步任务处理
 */

#include "comm_sys.h"
#include "time_event.h"
#include "wait_poll.h"

namespace lb_common {

/**
 * @brief 定时器线程类
 *
 * 提供基于线程的定时器管理功能，支持单次和周期性定时器。
 * 使用epoll事件驱动机制，支持高并发环境下的定时器处理。
 *
 * 该类继承自epoll_event_op，实现了定时器事件的异步处理。
 * 通过Linux的timerfd机制实现高精度定时器，结合epoll实现高效的事件驱动。
 *
 * 线程状态说明：
 * - 0: 未初始化
 * - 1: 已初始化，但未启动
 * - 2: 运行中
 * - 3: 停止中
 * - 4: 已停止
 */

class timer_thread : public epoll_event_op {
private:
  int32 timefd;          ///< 定时器文件描述符
  int32 state;           ///< 线程状态：0=未初始化，1=已初始化，2=运行中，3=停止中，4=已停止
  pthread_t threadid;    ///< 线程ID
  timer_order_list list; ///< 定时器有序链表
  wait_poll_one epollfd; ///< epoll轮询器

  /**
   * @brief 关闭定时器线程资源
   *
   * 清理定时器文件描述符和相关资源
   */
  void destroy();

  /**
   * @brief 初始化定时器文件描述符
   *
   * 创建并配置定时器文件描述符，添加到epoll中
   *
   * @return int32 1表示成功，负数表示失败
   */
  int32 init_timer();

  /**
   * @brief 处理定时器事件
   *
   * 当定时器超时时调用，处理所有已超时的定时器
   */
  virtual void deal_event();

  /**
   * @brief 处理定时器错误
   *
   * 当定时器发生错误时调用，重新初始化定时器
   */
  virtual void deal_error();

  /**
   * @brief 处理关闭事件
   *
   * 从epoll中移除后调用，默认为空实现
   */
  virtual void deal_close() {};

  /**
   * @brief 获取事件句柄
   *
   * @return int32 定时器句柄
   */
  virtual int32 get_fd() { return timefd; }

public:
  /**
   * @brief 构造函数
   */
  timer_thread() : timefd(-1), state(0), threadid(0) {}
  /**
   * @brief 析构函数
   */
  ~timer_thread() { destroy(); }
  timer_thread(const timer_thread &) = delete;
  timer_thread &operator=(const timer_thread &) = delete;

  /**
   * @brief 初始化定时器线程
   *
   * 初始化epoll轮询器、定时器链表和定时器文件描述符
   *
   * @return int32 0表示成功，负数表示失败
   */
  int32 init();

  /**
   * @brief 启动定时器线程
   *
   * 创建并启动定时器处理线程
   *
   * @return int32 0表示成功，-1表示失败
   */
  int32 run();

  /**
   * @brief 线程主循环
   *
   * 定时器线程的主处理循环，等待并处理epoll事件
   */
  void deal();

  /**
   * @brief 停止定时器线程
   *
   * 设置停止标志，通知线程退出
   */
  void to_stop();

  /**
   * @brief 等待线程结束
   *
   * 阻塞等待定时器线程结束
   */
  void join();

  /**
   * @brief 添加定时器
   *
   * 添加新的定时器到管理列表中
   *
   * @param[in] eventinfo 定时器事件信息
   * @return int32 0表示成功，负数表示失败
   */
  int32 add_timer(time_event_info &eventinfo);
};

} // namespace lb_common

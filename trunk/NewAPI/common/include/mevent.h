#pragma once

/**
 * @file mevent.h
 * @brief 事件操作模块
 *
 * 提供事件处理的抽象接口和事件信息结构体定义，包括普通事件和定时器事件。
 * 用于事件驱动架构中的事件回调处理。
 */

#include "comm_sys.h"

namespace lb_common {

/**
 * @brief 事件操作基类
 *
 * 提供事件处理的抽象接口，所有具体的事件处理器都需要继承此类。
 * 主要用于事件驱动架构中的事件回调处理。
 */
class event_op {
public:
  /**
   * @brief 处理事件的纯虚函数
   *
   * 子类必须实现此方法来处理具体的事件逻辑。
   *
   * @param[in] pbuf 事件数据缓冲区指针
   */
  virtual void deal(char *pbuf) = 0;

  /**
   * @brief 默认构造函数
   */
  event_op(){};

  /**
   * @brief 虚析构函数
   */
  virtual ~event_op(){};
};

/**
 * @brief 事件信息结构体
 *
 * 包含事件处理器和相关数据缓冲区的信息。
 * 用于在事件系统中传递事件相关的完整信息。
 */
struct event_info {
  /** @brief 事件处理器指针 */
  event_op *op;
  /** @brief 事件数据缓冲区，大小为56字节,必须为POD 类型 */
  char buf[56];
};

/**
 * @brief 定时器事件操作基类
 *
 * 提供定时器事件处理的抽象接口，所有具体的定时器处理器都需要继承此类。
 * 主要用于定时任务和超时处理的回调。
 */
class time_event_op {
public:
  /**
   * @brief 处理定时器事件的纯虚函数
   *
   * 子类必须实现此方法来处理具体的定时器事件逻辑。
   *
   * @param[in] is_period 是否为周期性定时器（1=周期性，0=单次）
   * @param[in] timer 定时器ID或间隔时间
   * @param[in] pbuf 定时器事件数据缓冲区指针
   */
  virtual void deal_timer(int32 is_period, int32 timer, char *pbuf) = 0;

  /**
   * @brief 默认构造函数
   */
  time_event_op(){};

  /**
   * @brief 虚析构函数
   */
  virtual ~time_event_op(){};
};

/**
 * @brief 定时器事件信息结构体
 *
 * 包含定时器处理器、定时器类型、ID和相关数据缓冲区的信息。
 * 用于在定时器系统中传递定时器相关的完整信息。
 */
struct time_event_info {
  /** @brief 定时器事件处理器指针 */
  time_event_op *op;
  /** @brief 是否为周期性定时器（1=周期性，0=单次） */
  int32 isperiod;
  /** @brief 定时器ID或间隔时间（微秒） */
  int32 timer;
  /** @brief 定时器事件数据缓冲区，大小为48字节,必须为POD 类型 */
  char buf[48];
};

} // namespace lb_common

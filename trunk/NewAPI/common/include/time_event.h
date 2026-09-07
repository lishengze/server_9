#pragma once

/**
 * @file time_event.h
 * @brief 定时器事件管理模块
 *
 * 提供基于时间排序链表的定时器管理功能，支持单次和周期性定时器。
 * 使用动态内存池管理定时器节点，支持高并发环境下的安全操作。
 *
 * 主要特性：
 * - 基于时间排序的定时器链表
 * - 支持单次和周期性定时器
 * - 高效的定时器插入和删除
 *
 * 使用场景：
 * - 网络超时管理
 * - 周期性任务调度
 * - 事件驱动架构
 * - 异步任务处理
 */

#include "comm_sys.h"
#include "mevent.h"
#include "mlock.h"

namespace lb_common {

class timer_order_list {
private:
  /**
   * @brief 定时器链表节点结构体
   *
   * 包含定时器事件信息、链表指针和时间戳。
   * 用于在定时器有序链表中存储单个定时器的完整信息。
   */
  struct timer_list_it {
    time_event_info event; ///< 定时器事件信息
    timer_list_it *next;   ///< 下一个节点指针
    uint64 begintime;      ///< 定时器开始时间（微秒）
    uint64 outtime;        ///< 定时器超时时间（微秒）

    /**
     * @brief 构造函数
     */
    timer_list_it() : next(nullptr) {
      begintime = 0;
      outtime = 0;
    }

    /**
     * @brief 析构函数
     */
    ~timer_list_it() {}
  };

  /** @brief 用于并发控制 */
  atomic_lock mlock;
  /** @brief 链表头节点索引 */
  timer_list_it *first;
  /** @brief 链表尾节点索引 */
  timer_list_it *end;

  /** @brief 最小定时器间隔（微秒） */
  static const int32 TIMER_MIN_UNIT_US = 10;

private:
  /**
   * @brief 内部添加定时器到链表
   *
   * 将定时器节点按照超时时间插入到有序链表的正确位置。
   *
   * @param[in] info 定时器节点引用
   */
  void add_list(timer_list_it &info);

public:
  /**
   * @brief 添加定时器
   *
   * 创建新的定时器节点并添加到有序链表中。
   *
   * @param[in] eventinfo 定时器事件信息
   * @return int32 0表示成功，负数表示失败
   */
  int32 add_timer(time_event_info &eventinfo);

  /**
   * @brief 删除定时器
   *
   * 从定时器链表中删除指定的定时器。
   *
   * @param[in] op 定时器事件处理器指针
   * @param[in] timer 定时器间隔时间
   */
  void delete_timer(time_event_op *op, int32 timer);

  /**
   * @brief 获取下一个定时器间隔时间
   *
   * 计算并返回链表中第一个定时器的剩余时间。
   * 如果没有定时器则返回0。
   *
   * @return uint32 下一个定时器的间隔时间（微秒），0表示无定时器
   */
  uint32 get_interval();
  /**
   * @brief 处理超时的定时器
   *
   * 遍历链表，处理所有已超时的定时器事件。
   * 对于周期性定时器会重新添加到链表中。
   * @param[out] o_interval 输出参数，返回下一个定时器的剩余时间
   */
  void loop_timer(uint32 &o_interval);

  /**
   * @brief 清理所有定时器
   *
   * 清空定时器链表，释放所有定时器节点的内存。
   * 通常在对象析构时调用。
   */
  void clear();

  /**
   * @brief 构造函数
   */
  timer_order_list() : first(nullptr), end(nullptr) { mlock.init(); }

  /**
   * @brief 析构函数
   */
  ~timer_order_list() { clear(); };

  /**
   * @brief 禁用拷贝构造函数
   */
  timer_order_list(const timer_order_list &) = delete;
  /**
   * @brief 禁用赋值操作符
   */
  timer_order_list &operator=(const timer_order_list &) = delete;
};

} // namespace lb_common

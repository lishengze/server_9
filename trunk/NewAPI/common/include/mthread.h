#pragma once

/**
 * @file mthread.h
 * @brief 多线程管理模块
 *
 * 提供基于epoll事件驱动的多线程管理功能，支持高性能的事件处理和线程池管理。
 * 使用原子操作保证多线程环境下的状态一致性，支持CPU亲和性设置。
 *
 * 主要特性：
 * - 基于epoll的事件驱动架构
 * - 多线程池管理
 * - 原子操作保证线程安全
 * - CPU亲和性支持
 * - 高性能事件队列
 * - Linux专用优化实现
 *
 * 使用场景：
 * - 高并发网络服务器
 * - 事件驱动应用
 * - 多线程任务处理
 * - CPU密集型应用
 */

#include "matomic.h"
#include "mevent.h"
#include "que_mth_fixed.h"
#include "wait_poll.h"
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

namespace lb_common {

class mthread_pool;

/**
 * @brief 多线程类
 *
 * 提供基于epoll事件驱动的多线程管理功能，支持高性能的事件处理。
 * 继承自epoll_event_op，实现了事件处理的抽象接口。
 *
 * 线程状态定义：
 * - DynThread_State_None: 未初始化
 * - DynThread_State_Init: 已初始化，未启动
 * - DynThread_State_Running: 运行中
 * - DynThread_State_ToStop: 停止中
 */
class mthread : private epoll_event_op {
protected:
#define DynThread_State_None    0
#define DynThread_State_Init    1
#define DynThread_State_Running 2
#define DynThread_State_ToStop  4

  wait_poll_multi waitpoll;         ///< epoll轮询器
  que_mth_fixed<event_info> eventq; ///< 事件队列

  int32 waitms;          ///< 等待超时时间
  int32 eventq_busy_num; ///< 忙轮询时处理的事件数量
  event_wake que_wake;   ///< 队列唤醒器
  int32 load;            ///< 当前负载
  int32 state;           ///< 线程状态
  int32 mid;             ///< 线程ID
  int32 cpuid;           ///< CPU亲和性ID
  pthread_t thid;        ///< 线程句柄

  /**
   * @brief 销毁线程资源
   *
   * 清理线程相关资源，重置状态为未初始化。
   */
  void destroy();

public:
  friend class mthread_pool;

  /**
   * @brief 获取事件
   *
   * 从事件队列中获取一个事件。
   *
   * @param[out] o_event 输出参数，返回获取到的事件指针
   * @param[out] o_pos 输出参数，返回事件在队列中的位置
   * @return bool 成功返回true，失败返回false
   */
  FORCE_INLINE bool get_event(event_info *&o_event, int64 &o_pos) {
    o_pos = eventq.write_get_mth(o_event);
    return (o_pos > 0);
  }

  /**
   * @brief 提交事件
   *
   * 提交已处理的事件，并更新负载统计。
   *
   * @param[in] get_pos 事件在队列中的位置
   */
  FORCE_INLINE void cmt_event(int64 get_pos) {
    eventq.write_cmt_mth(get_pos);
    if (waitms > 0 && eventq.get_used() < 4)
      que_wake.wake();
  }

  /**
   * @brief 添加轮询事件
   *
   * 将事件添加到epoll轮询器中。
   *
   * @param[in] info 事件信息引用
   * @return int32 成功返回0，失败返回负数
   */
  int32 add_poll_event(epoll_event_op &info) {
    int32 ret = waitpoll.add_wake(info);
    if (unlikely(ret < 0))
      return ret;
    atomic_fetch_add32(&load, info.mload);
    return 0;
  }

  /**
   * @brief 修改轮询事件
   *
   * 修改已添加到epoll轮询器中的事件。
   *
   * @param[in] info 事件信息引用
   * @return int32 成功返回0，失败返回负数
   */
  int32 mode_poll_event(epoll_event_op &info) { return waitpoll.mode_wake(info); }

  /**
   * @brief 修改负载
   *
   * 增加或减少当前负载统计。
   *
   * @param[in] diffload 负载变化量
   */
  FORCE_INLINE void mod_load(int32 diffload) {
    if (diffload >= 0) {
      atomic_fetch_add32(&load, diffload);
    } else {
      while (true) {
        int32 t = atomic_load32(&load);
        if (t + diffload >= 0) {
          if (atomic_cas32(&load, &t, t + diffload))
            break;
        } else {
          break;
        }
      }
    }
  }

  /**
   * @brief 移除轮询事件
   *
   * 从epoll轮询器中移除事件。
   *
   * @param[in] info 事件信息引用
   * @return int32 成功返回0，失败返回负数
   */
  int32 remove_poll_event(epoll_event_op &info) {
    int32 ret = waitpoll.remove_wake(info);
    if (unlikely(ret < 0))
      return ret;
    mod_load(0 - info.mload);
    return 0;
  }

  /**
   * @brief 删除轮询事件
   *
   * 从epoll轮询器中删除事件。
   *
   * @param[in] info 事件信息引用
   * @return int32 成功返回0，失败返回负数
   */
  int32 delete_poll_event(epoll_event_op &info) {
    int32 ret = waitpoll.delete_wake(info);
    if (unlikely(ret < 0))
      return ret;
    atomic_fetch_sub32(&load, info.mload);
    return 0;
  }

  /**
   * @brief 获取线程ID
   *
   * @return int32 线程ID
   */
  FORCE_INLINE int32 get_id() const { return mid; }

  /**
   * @brief 检查是否正在运行
   *
   * @return bool 运行中返回true，否则返回false
   */
  inline bool is_running() const {
    int32 ts = atomic_load32(&state);
    return ((ts & DynThread_State_ToStop) == 0) && (ts != 0);
  }

  /**
   * @brief 初始化线程
   *
   * 初始化线程相关资源，包括epoll轮询器和事件队列。
   *
   * @param[in] id 线程ID
   * @param[in] max_poll_num 最大轮询事件数量
   * @param[in] eventq_size 事件队列大小
   * @param[in] eventq_loop_num 忙轮询时处理的事件数量
   * @param[in] twaitms 等待超时时间，默认100ms
   * @return int32 成功返回0，失败返回负数
   */
  int32 init_th(int32 id, int32 max_poll_num, int32 eventq_size, int32 eventq_loop_num, int32 twaitms = 100);

  /**
   * @brief 设置CPU亲和性
   *
   * 设置线程运行的CPU核心。
   *
   * @param[in] tcpuid CPU ID
   */
  void init_set_cpu(int32 tcpuid) { cpuid = tcpuid; }

  /**
   * @brief 启动线程
   *
   * 创建并启动线程，设置CPU亲和性。
   *
   * @return int32 成功返回1，失败返回负数
   */
  int32 run();

  /**
   * @brief 停止线程
   *
   * 设置停止标志，通知线程退出。
   */
  void to_stop();

  /**
   * @brief 等待线程结束
   *
   * 等待线程执行完成并回收资源。
   */
  void join();

  /**
   * @brief 构造函数
   *
   * 初始化成员变量为默认值。
   */
  mthread() {
    waitms = 100;
    eventq_busy_num = 6;
    load = 0;
    state = 0;
    mid = -1;
    cpuid = -1;
    thid = -1;
  }

  /**
   * @brief 析构函数
   *
   * 清理所有资源。
   */
  virtual ~mthread() { destroy(); }

  mthread(const mthread &) = delete;
  mthread &operator=(const mthread &) = delete;

private:
  /**
   * @brief 处理epoll事件
   *
   * 处理从epoll轮询器接收到的事件。
   */
  virtual void deal_event();

  /**
   * @brief 处理epoll错误
   *
   * 处理epoll轮询器的错误事件。
   */
  virtual void deal_error() {};

  /**
   * @brief 处理epoll关闭
   *
   * 从epoll移除后调用，用于清理资源。
   */
  virtual void deal_close() { load--; };

  /**
   * @brief 获取文件描述符
   *
   * @return int32 文件描述符
   */
  virtual int32 get_fd();

private:
  /**
   * @brief 处理队列事件
   *
   * 处理事件队列中的待处理事件。
   *
   * @return bool 队列非空且处理成功返回true，队列为空返回false
   */
  bool deal_queue();
  /**
   * @brief 线程主处理函数
   *
   * 线程的主循环，处理epoll事件和队列事件。
   */
  void epoll_wait_func();
  /**
   * @brief 事件队列处理函数
   *
   * 纯事件队列模式的主循环，仅处理队列事件。
   */
  void event_que_func();
  /**
   * @brief 线程入口函数
   *
   * pthread_create的静态线程入口函数，作为回调传递给pthread_create。
   *
   * @param[in] arg 线程参数，指向mthread实例
   * @return void* 线程退出状态，始终返回NULL
   */
  static void *_f_mthread_func(void *arg);
};

/**
 * @brief 多线程池类
 *
 * 管理多个mthread实例，提供线程分配、事件投递和生命周期管理。
 * 支持轮询分配和CPU亲和性设置。
 *
 * 主要功能：
 * - 线程池管理
 * - 智能线程分配
 * - CPU亲和性管理
 * - 事件投递
 * - 批量操作
 */
class mthread_pool {
protected:
  int32 num;        ///< 线程数量
  int32 lastassign; ///< 上次分配的线程索引
  mthread *ths;     ///< 线程数组

public:
  /**
   * @brief 获取线程
   *
   * 根据线程ID获取对应的线程实例。
   *
   * @param[in] thid 线程ID
   * @return mthread* 线程指针，失败返回NULL
   */
  FORCE_INLINE mthread *get_thread(int32 thid) const {
    assert(thid >= 0 && thid < num);
    return ths + thid;
  }

  /**
   * @brief 分配线程
   *
   * 根据负载情况分配一个可用的线程。
   * 一定成功。
   *
   * @param[out] o_thread 输出参数，返回分配到的线程指针
   * @param[in] exid 排除的线程ID
   * @param[in] isround 是否使用轮询分配
   * @return int32 返回分配的线程ID
   */
  int32 assign_thread(mthread *&o_thread, int32 exid, int32 isround = 0);

  /**
   * @brief 根据CPU分配线程
   *
   * 分配指定CPU亲和性的线程。
   * 可能失败，CPU不存在时返回-1。
   *
   * @param[out] o_thread 输出参数，返回分配到的线程指针
   * @param[in] cpuid CPU ID
   * @return int32 返回线程ID
   */
  int32 assign_by_cpu(mthread *&o_thread, int32 cpuid);

  /**
   * @brief 初始化线程池
   *
   * 创建并初始化指定数量的线程。
   *
   * @param[in] max_thread_num 最大线程数量
   * @param[in] max_poll_num 每个线程的最大轮询事件数量
   * @param[in] eventq_size 事件队列大小
   * @param[in] eventq_busy_num 忙轮询时处理的事件数量
   * @param[in] twaitms 等待超时时间，默认100ms
   * @return int32 成功返回0，失败返回负数
   */
  int32 init(int32 max_thread_num, int32 max_poll_num, int32 eventq_size, int32 eventq_busy_num, int32 twaitms = 100);

  /**
   * @brief 设置线程CPU亲和性
   *
   * 为指定线程设置CPU亲和性。
   *
   * @param[in] thid 线程ID
   * @param[in] tcpuid CPU ID
   */
  void init_set_cpu(int32 thid, int32 tcpuid);

  /**
   * @brief 启动所有线程
   *
   * 启动线程池中的所有线程。
   *
   * @return int32 成功返回0，失败返回负数
   */
  int32 run();

  /**
   * @brief 停止所有线程
   *
   * 设置所有线程的停止标志。
   */
  void to_stop();

  /**
   * @brief 等待所有线程结束
   *
   * 等待所有线程执行完成并回收资源。
   */
  void destroy();

  /**
   * @brief 获取线程数量
   *
   * @return int32 线程总数
   */
  FORCE_INLINE int32 get_num() const { return num; }

public:
  /**
   * @brief 构造函数
   *
   * 初始化成员变量为默认值。
   */
  mthread_pool() {
    num = 0;
    lastassign = 0;
    ths = NULL;
  }

  /**
   * @brief 析构函数
   *
   * 清理所有线程资源。
   */
  ~mthread_pool() { destroy(); };
};

} // namespace lb_common

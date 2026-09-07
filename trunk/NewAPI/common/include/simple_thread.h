#pragma once

/**
 * @file simple_thread.h
 * @brief 简单线程模型模块
 *
 * 提供一个简单的线程封装类，支持两种运行模式：
 * - 忙轮询模式：持续检查工作标志
 * - 等待触发模式：使用 epoll/eventfd 等待工作触发
 *
 * 使用时需要继承此类并实现 do_work() 和 need_work() 虚函数
 */

#include "comm_sys.h"

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

namespace lb_common {

/**
 * @brief 简单线程模型类
 *
 * 提供基础的线程管理功能，包括初始化、启动、停止、等待等。
 * 支持两种工作模式：
 * - 忙轮询模式（busy_round > 0）：线程持续检查是否有工作
 * - 等待触发模式（poll_wait_ms > 0）：线程阻塞等待工作触发
 *
 * @note 使用时需要继承此类并实现以下虚函数：
 * - do_work(): 执行实际工作
 * - need_work(): 检查是否有工作需要处理
 *
 * @example
 * @code
 * class MyWorker : public simple_thread {
 * protected:
 *     virtual void do_work() override {
 *         // 处理工作
 *     }
 *     virtual bool need_work() override {
 *         return !work_queue.empty();
 *     }
 * };
 *
 * MyWorker worker;
 * worker.init_th(0, -1, 100);  // 使用等待触发模式
 * worker.run();
 *
 * // 主线程可以触发worker处理工作
 * worker.trigger();
 *
 * worker.to_stop();
 * worker.join();
 * @endcode
 */
class simple_thread {
protected:
  /**
   * @brief 默认等待超时时间（毫秒）
   *
   * 定义线程等待触发时的默认超时时间
   */
#define SIMPLE_THREAD_WAIT_MS 500

  volatile int32 state;    ///< 线程状态：0=未初始化，1=已初始化，2=运行中，3=停止请求
  int32 epollfd;           ///< epoll文件描述符，用于等待触发模式
  volatile int32 waitflag; ///< 等待标志：0=工作中，1=等待中
  int32 evtfd;             ///< eventfd文件描述符，用于线程唤醒
  int32 pollwaitms;        ///< 轮询等待毫秒数（仅在等待触发模式下有效）
  int32 loopround;         ///< 循环轮次（每次触发后执行的工作轮数）
  int32 runmod;            ///< 运行模式：1=忙轮询模式，2=等待触发模式
  int32 cpuid;             ///< CPU亲和ID，-1表示不绑定CPU
  pthread_t thid;          ///< 线程ID

public:
  /**
   * @brief 执行实际工作的纯虚函数
   *
   * 子类必须实现此函数来处理实际工作逻辑
   *
   * @note 此函数会在工作线程中被反复调用
   */
  virtual void do_work() = 0;

  /**
   * @brief 检查是否有工作需要处理的虚函数
   *
   * @return true表示有工作需要处理，false表示无工作
   *
   * @note 子类可以重写此函数来实现自定义的工作检查逻辑
   * @note 在等待触发模式下，此函数会在每次触发后被调用
   * @note 默认实现返回false（无工作）
   */
  virtual bool need_work() { return FALSE; }

  /**
   * @brief 初始化线程属性和资源
   *
   * 根据参数配置线程的运行模式和属性，包括CPU亲和性、轮询设置等
   *
   * @param[in] busyround 忙轮询轮次，>0触发式+忙轮询次数模式，=0使用死轮询
   * @param[in] tcpuid CPU亲和ID，-1表示不绑定CPU，>=0绑定到指定CPU核心
   * @param[in] pollms 轮询等待毫秒数，仅在等待触发模式下有效
   * @return int32 0表示成功，负数表示错误码
   *
   * @note 必须在调用run()之前调用此函数进行初始化
   * @note 忙轮询模式：线程持续执行do_work()，适合CPU密集型任务
   * @note 等待触发模式：线程阻塞等待，通过trigger()唤醒，适合I/O密集型任务
   */
  int32 init_th(int32 busyround = 0, int32 tcpuid = -1, int32 pollms = 100);

  /**
   * @brief 启动线程执行
   *
   * 创建并启动工作线程，线程开始执行相应的运行模式
   *
   * @return int32 0表示成功，负数表示错误码
   *
   * @note 调用此函数后，线程开始执行run()或wait_run()
   * @note 线程启动后会自动屏蔽相关信号
   */
  int32 run();

  /**
   * @brief 触发线程工作
   *
   * 在等待触发模式下唤醒线程处理工作
   *
   * @note 仅在等待触发模式下有效
   * @note 忙轮询模式下调用此函数无效果
   * @note 线程会被唤醒并执行指定轮次的do_work()
   */
  void trigger();

  /**
   * @brief 请求线程停止
   *
   * 设置停止标志，线程会在完成当前工作后安全退出
   *
   * @note 这是异步操作，线程不会立即停止
   * @note 线程检测到停止标志后会清理资源并退出
   */
  void to_stop() { state = 3; }

  /**
   * @brief 等待线程完全结束
   *
   * 阻塞当前线程直到工作线程完全退出
   *
   * @note 会自动调用to_stop()确保线程停止
   * @note 如果线程已经停止，此函数会立即返回
   */
  void join();

  /**
   * @brief 默认构造函数
   *
   * 初始化所有成员变量为默认值
   *
   * @note 构造后线程处于未初始化状态，需要调用init_th()
   */

  simple_thread() : state(0), epollfd(-1), waitflag(1), evtfd(-1), pollwaitms(0), loopround(0), cpuid(-1), thid(0) {}
  simple_thread(const simple_thread &) = delete;
  simple_thread &operator=(const simple_thread &) = delete;

  /**
   * @brief 虚析构函数
   *
   * 清理线程资源，关闭文件描述符
   *
   * @note 会自动关闭epollfd和evtfd文件描述符
   * @note 如果线程仍在运行，建议先调用join()等待线程结束
   */
  virtual ~simple_thread() {
    if (epollfd > 0) {
      close(epollfd);
      epollfd = -1;
    }
    if (evtfd > 0) {
      close(evtfd);
      evtfd = -1;
    }
  }

protected:
  /**
   * @brief 忙轮询运行模式
   *
   * 线程持续执行do_work()直到收到停止信号
   *
   * @note 适用于CPU密集型任务，响应速度快但CPU占用率高
   */
  void busy_run();

  /**
   * @brief 等待触发运行模式
   *
   * 线程阻塞等待事件触发，触发后执行指定轮次的工作
   *
   * @note 适用于I/O密集型任务，CPU占用率低但响应有延迟
   * @note 通过epoll+eventfd机制实现高效的等待/唤醒
   */
  void busy_wait_run();

  /**
   * @brief 线程入口函数
   *
   * 静态函数，作为pthread_create的入口点
   *
   * @param[in] arg 指向simple_thread实例的指针
   * @return void* 线程返回值（始终为NULL）
   */
  static void *simple_thread_func(void *arg);
};

} // namespace lb_common

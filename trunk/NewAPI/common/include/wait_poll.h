#pragma once

/**
 * @file wait_poll.h
 * @brief epoll事件轮询模块
 *
 * 提供基于epoll的事件轮询机制，支持单事件和多事件处理。
 * 包含事件操作基类、单事件轮询器和多事件轮询器。
 *
 * 主要特性：
 * - 基于epoll的高性能事件轮询
 * - 支持单事件和多事件处理
 * - 支持边缘触发和水平触发模式
 * - 支持读写事件分离
 * - 原子操作保证线程安全
 * - 动态事件添加和删除
 *
 * 使用场景：
 * - 高性能网络服务器
 * - 事件驱动的应用程序
 * - 多路复用I/O处理
 * - 异步事件处理
 */

#include "mlock.h"
#include "wait_wake.h"

#include <sys/epoll.h>

namespace lb_common {

/**
 * @brief epoll事件操作基类
 *
 * 定义epoll事件处理的基本接口，支持事件处理、错误处理和关闭处理
 */
class epoll_event_op {
public:
  int16 isout;  ///< 输出事件标志：0=输入，1=输出
  int16 isedge; ///< 边缘触发标志：0=水平触发，1=边缘触发
  int32 mload;  ///< 事件负载计数

  /**
   * @brief 设置事件属性
   *
   * @param[in] outflag 输出事件标志，默认为0
   * @param[in] edgeflag 边缘触发标志，默认为0
   * @param[in] load_cnt 负载计数，默认为1
   */
  inline void set_event(int16 outflag, int16 edgeflag = 0, int32 load_cnt = 1) {
    isout = outflag;
    isedge = edgeflag;
    mload = load_cnt;
  }

  /**
   * @brief 处理事件
   *
   * 虚函数，子类可重写实现具体的事件处理逻辑
   */
  virtual void deal_event() {};

  /**
   * @brief 处理错误事件
   *
   * 虚函数，子类可重写实现具体的错误处理逻辑
   */
  virtual void deal_error() {};

  /**
   * @brief 处理关闭事件
   *
   * 从epoll中移除后调用，子类可重写实现清理逻辑
   */
  virtual void deal_close() {};

  /**
   * @brief 获取文件描述符
   *
   * @return int32 文件描述符，默认返回-1
   */
  virtual int32 get_fd() { return -1; }

  /**
   * @brief 构造函数
   */
  epoll_event_op() {
    isout = 0;
    isedge = 0;
    mload = 1;
  }

  /**
   * @brief 虚析构函数
   */
  virtual ~epoll_event_op(){};
};

/**
 * @brief 单事件轮询器
 *
 * 管理单个epoll事件的轮询处理，适用于简单的事件处理场景
 */
class wait_poll_one {
private:
  int32 epollfd;              ///< epoll文件描述符
  int32 isremove;             ///< 移除标志：0=空闲，1=添加中，2=工作，3=待删除，4=删除中
  epoll_event_op *info;       ///< 事件操作对象指针
  struct epoll_event epevent; ///< epoll事件结构

public:
  /**
   * @brief 初始化单事件轮询器，单线程调用
   *
   * @return int32 0表示成功，负数表示错误码
   */
  int32 init();

  /**
   * @brief 添加事件到epoll
   *
   * @param[in] pinfo 事件操作对象引用
   * @return int32 0表示成功，负数表示错误码
   */
  int32 add_wake(epoll_event_op &pinfo);

  /**
   * @brief 修改事件属性
   *
   * @return int32 0表示成功，负数表示失败
   */
  int32 mode_wake();

  /**
   * @brief 标记事件为待移除
   *
   * @return int32 0表示成功，负数表示失败
   */
  int32 remove_wake();

  /**
   * @brief 立即删除事件
   *
   * @return int32 0表示成功，负数表示失败
   */
  int32 delete_wake();

  /**
   * @brief 等待事件
   *
   * @param[in] waitms 等待超时时间（毫秒）
   * @return int32 事件数量，负数表示错误
   */
  int32 wait(int32 waitms);

  /**
   * @brief 销毁轮询器并释放资源
   */
  void destroy();

  /**
   * @brief 构造函数
   */
  wait_poll_one() {
    epollfd = -1;
    isremove = 0;
    info = nullptr;
  }

  /**
   * @brief 析构函数
   */
  ~wait_poll_one() { destroy(); }

  /**
   * @brief 禁用拷贝构造函数
   *
   * 防止对象被拷贝，确保资源唯一性
   */
  wait_poll_one(const wait_poll_one &) = delete;

  /**
   * @brief 禁用赋值操作符
   *
   * 防止对象被赋值，确保资源唯一性
   */
  wait_poll_one &operator=(const wait_poll_one &) = delete;
};

/**
 * @brief 多事件轮询器
 *
 * 管理多个epoll事件的轮询处理，支持动态添加和删除事件
 */
class wait_poll_multi {
private:
  int32 epollfd;                ///< epoll文件描述符
  int32 eventnum;               ///< 事件数量
  struct epoll_event *wait_evs; ///< 事件数组
  atomic_lock dellock;          ///< 删除锁标志
  int32 delnum;                 ///< 待删除事件数量
  event_wake change_event;      ///< 变更通知事件
  epoll_event_op **del_evs;     ///< 待删除事件指针数组

  /**
   * @brief 删除指定范围内的事件
   *
   * 从等待事件数组中删除指定范围内标记为待删除的事件。
   * 该函数在多线程环境下安全，使用原子锁保护删除操作。
   *
   * @param[in] ipos 起始位置（通常为变更通知事件的位置）
   * @param[in] wpos 结束位置（事件数组中的最后一个有效位置）
   */
  void del_wake(int32 ipos, int32 wpos);

public:
  /**
   * @brief 初始化多事件轮询器
   *
   * @param[in] wakenum 事件数量
   * @return int32 0表示成功，负数表示错误码
   */
  int32 init(int32 wakenum);

  /**
   * @brief 添加事件到epoll
   *
   * @param[in] pinfo 事件操作对象引用
   * @return int32 0表示成功，负数表示错误码
   */
  int32 add_wake(epoll_event_op &pinfo);

  /**
   * @brief 修改事件属性
   *
   * @param[in] pinfo 事件操作对象引用
   * @return int32 0表示成功，负数表示失败
   */
  int32 mode_wake(epoll_event_op &pinfo);

  /**
   * @brief 标记事件为待移除
   *
   * @param[in] pinfo 事件操作对象引用
   * @return int32 0表示成功，负数表示失败
   */
  int32 remove_wake(epoll_event_op &pinfo);

  /**
   * @brief 立即删除事件
   *
   * @param[in] pinfo 事件操作对象引用
   * @return int32 0表示成功，负数表示失败
   */
  int32 delete_wake(epoll_event_op &pinfo);

  /**
   * @brief 等待事件
   *
   * @param[in] waitms 等待超时时间（毫秒）
   * @return int32 事件数量，负数表示错误
   */
  int32 wait(int32 waitms);

  /**
   * @brief 销毁轮询器并释放资源
   */
  void destroy();

  /**
   * @brief 构造函数
   */
  wait_poll_multi() {
    delnum = 0;
    dellock.init();
    epollfd = -1;
    eventnum = 0;
    wait_evs = nullptr;
    del_evs = nullptr;
  }

  /**
   * @brief 析构函数
   */
  ~wait_poll_multi() { destroy(); }
  /**
   * @brief 禁用拷贝构造函数
   *
   * 防止对象被拷贝，确保资源唯一性
   */
  wait_poll_multi(const wait_poll_multi &) = delete;
  /**
   * @brief 禁用赋值操作符
   *
   * 防止对象被赋值，确保资源唯一性
   */
  wait_poll_multi &operator=(const wait_poll_multi &) = delete;
};

} // namespace lb_common

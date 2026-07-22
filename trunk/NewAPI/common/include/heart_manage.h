#pragma once

/**
 * @file heart_manage.h
 * @brief 心跳管理模块
 *
 * 提供连接心跳超时检测功能，支持：
 * - 心跳间隔配置
 * - 业务消息计数
 * - 超时检测与链路异常判定
 */

#include "comm_sys.h"
#include "matomic.h"
#include <ctime>

namespace lb_common {

/** @brief 心跳超时次数阈值（连续超时次数超过此值判定链路异常） */
#define CH_HEART_TIMEOUT_NUM 3

/**
 * @brief 心跳管理类
 *
 * 用于管理连接心跳超时检测。维护上次心跳时间和业务消息计数，
 * 通过 check_timeout() 判定链路是否异常。
 */
class heart_manage {
public:
  /**
   * @brief 构造函数
   * @param[in] interval_sec 心跳超时间隔（秒），默认5秒
   */
  explicit heart_manage() : msg_count(0), m_interval(5) {
    last_heart = std::time(nullptr);
    last_send = 0;
  }

  /**
   * @brief 初始化
   * @param[in] interval_sec 新的心跳间隔（秒）
   */
  void init(int32 interval_sec) {
    msg_count = 0;
    atomic_store32(&m_interval, interval_sec);
    last_heart = std::time(nullptr);
    last_send = 0;
  }

  /**
   * @brief 设置心跳间隔
   * @param[in] interval_sec 新的心跳间隔（秒）
   */
  void set_interval(int32 interval_sec) { atomic_store32(&m_interval, interval_sec); }

  /**
   * @brief 获取当前心跳间隔
   * @return 心跳间隔（秒）
   */
  int32 get_interval() const { return atomic_load32(&m_interval); }

  /**
   * @brief 收到心跳消息时调用
   * 功能：更新上次心跳时间 + 原子清空业务消息计数
   */
  FORCE_INLINE void on_heart() {
    if (m_interval > 0) {
      last_heart = std::time(nullptr);
      atomic_store32(&msg_count, 0);
    }
  }

  /**
   * @brief 收到业务消息（非心跳）时调用
   * 功能：原子增加业务消息计数
   */
  FORCE_INLINE void on_msg() {
    if (m_interval > 0) {
      // atomic_fetch_add32(&msg_count,1);
      msg_count++;
    }
  }

  /**
   * @brief 检查是否心跳超时/链路异常
   * @return true: 超时 或 无业务消息；false: 链路正常
   */
  bool check_timeout() const {
    int32 tin = atomic_load32(&m_interval);
    if (tin == 0)
      return false;
    std::time_t now = std::time(nullptr);
    int32 tdiff = std::difftime(now, last_heart);
    int32 tcnt = atomic_load32(&msg_count);
    return ((tdiff >= tin * CH_HEART_TIMEOUT_NUM) && (tcnt == 0));
  }

  /**
   * @brief 检查是否发送心跳
   * @return true: 发送；false: 不发送
   */
  bool check_send() {
    int32 tin = atomic_load32(&m_interval);
    if (tin == 0) {
      return false;
    }
    std::time_t now = std::time(nullptr);
    if (last_send != 0) {
      int32 tdiff = std::difftime(now, last_send);
      if (tdiff >= tin) {
        last_send = now;
        return true;
      }
    } else {
      last_send = now;
      return true;
    }
    return false;
  }

private:
  int32 msg_count;   ///< 上次心跳来收到消息个数
  int32 m_interval;  ///< 心跳间隔（秒）
  time_t last_heart; ///< 上次心跳时间（秒）
  time_t last_send;  ///< 上次发送心跳时间（秒）
};

} // namespace lb_common

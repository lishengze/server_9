#pragma once

/**
 * @file reconnect_ctl.h
 * @brief 断线重连控制模块
 *
 * 提供连接状态管理与断线重连控制功能，支持：
 * - 连接状态跟踪（已连接/断线）
 * - 最大重连次数限制（0=无限重连）
 * - 重连计数与重连间隔检查
 * - 断线时间记录
 *
 * 重连间隔由外部传入，类内部不保存。
 * 仅维护断线时间，should_reconnect() 返回 true 时自动重置断线时间
 * 并递增重连计数，作为下次重连间隔计算的起点。
 *
 * 使用场景：
 * - TCP连接断线重连
 * - 会话链路恢复
 * - 需要控制重连策略的网络通道
 */

#include "comm_sys.h"
#include "matomic.h"
#include <ctime>

namespace lb_common {

/**
 * @brief 断线重连控制类
 *
 * 管理连接状态和重连策略。记录重连次数与断线时间，
 * 由外部传入重连间隔判断是否应发起重连。
 *
 * 典型使用流程：
 * @code
 * reconnect_ctl rc;
 * rc.init(5);              // 最大重连5次
 * rc.set_connected();      // 连接成功
 * // ... 断线后
 * rc.set_disconnected();   // 标记断线
 * if (rc.should_reconnect(3)) {  // 重连间隔3秒
 *     // 发起重连
 * }
 * rc.set_connected();      // 重连成功，清除重连计数
 * @endcode
 */
class reconnect_ctl {
public:
/** @brief 连接状态：断线 */
#define RECONN_STATE_DISCONNECTED 0
/** @brief 连接状态：已连接 */
#define RECONN_STATE_CONNECTED 1
/** @brief 连接状态：等待链接 */
#define RECONN_STATE_NEED_CONNECT 2

  /**
   * @brief 构造函数
   *
   * 初始为断线状态，无重连限制
   */
  reconnect_ctl()
      : m_disconnect_time(0), m_reconnect_count(0), m_max_reconnect(0), m_happen_connected(0),
        m_state(RECONN_STATE_DISCONNECTED) {}

  /**
   * @brief 初始化重连控制参数
   *
   * @param[in] max_reconnect 最大重连次数，0表示无限重连
   */
  void init(uint16 max_reconnect) {
    m_disconnect_time = 0;
    if (max_reconnect > 0)
      m_max_reconnect = max_reconnect;
    else
      m_max_reconnect = 0;
    m_reconnect_count = 0;
    m_happen_connected = 0;
    m_state = RECONN_STATE_DISCONNECTED;
  }

  /**
   * @brief 设置为已连接状态
   *
   * 连接成功时调用，清除重连计数。
   * 仅在断线状态下生效。
   */
  void set_connected() {
    m_happen_connected = 1;
    if (atomic_load16(&m_state) == RECONN_STATE_CONNECTED) {
      return;
    }
    m_reconnect_count = 0;
    atomic_store16(&m_state, RECONN_STATE_CONNECTED);
  }

  /**
   * @brief 设置为断线状态
   *
   * 连接断开时调用，记录断线时间。
   * 仅在已连接状态下生效。
   */
  void set_disconnected() {
    m_disconnect_time = std::time(nullptr);
    atomic_store16(&m_state, RECONN_STATE_DISCONNECTED);
  }

  /**
   * @brief 检查是否应发起重连
   *
   * 同时满足以下条件时返回true：
   * 1. 当前处于断线状态
   * 2. 重连次数未超过最大限制（max_reconnect=0时无限制）
   * 3. 距断线时间已超过 reconnect_interval
   *
   * 返回 true 时：递增重连计数，重置断线时间为当前时刻
   * （作为下次重连间隔计算的起点）。
   *
   * @param[in] reconnect_interval 重连间隔时间（秒），必须>0
   * @return true=应发起重连，false=不应重连
   */
  bool check_reconnect(int32 reconnect_interval) {
    if (m_happen_connected == 0) {
      return false;
    }
    if (m_disconnect_time == 0) {
      ++m_reconnect_count;
      m_disconnect_time = std::time(nullptr);
      atomic_store16(&m_state, RECONN_STATE_NEED_CONNECT);
      return true;
    }

    if (atomic_load16(&m_state) == RECONN_STATE_CONNECTED) {
      return false;
    }
    if (m_max_reconnect > 0 && m_reconnect_count >= m_max_reconnect) {
      return false;
    }

    time_t now = std::time(nullptr);
    if (std::difftime(now, m_disconnect_time) >= reconnect_interval) {
      ++m_reconnect_count;
      m_disconnect_time = now;
      atomic_store16(&m_state, RECONN_STATE_NEED_CONNECT);
      return true;
    }
    return false;
  }

  /**
   * @brief 检查是否已连接
   * @return true=已连接
   */
  bool is_connected() const { return m_state == RECONN_STATE_CONNECTED; }

  /**
   * @brief 获取当前重连次数
   * @return 重连次数
   */
  uint16 get_reconnect_count() const { return m_reconnect_count; }

  /**
   * @brief 获取最大重连次数
   * @return 最大重连次数，0=无限重连
   */
  uint16 get_max_reconnect() const { return m_max_reconnect; }

  /**
   * @brief 获取断线时间
   * @return 断线时间戳（秒），0=未断线过
   */
  time_t get_disconnect_time() const { return m_disconnect_time; }

  /**
   * @brief 设置最大重连次数
   * @param[in] max_reconnect 最大重连次数，0=无限重连
   */
  void set_max_reconnect(uint16 max_reconnect) { m_max_reconnect = max_reconnect; }

  /**
   * @brief 重置重连计数
   *
   * 清除重连计数，保留其他状态。可用于外部重置重连策略。
   */
  void reset_reconnect_count() { m_reconnect_count = 0; }

private:
  time_t m_disconnect_time; ///< 断线时间（should_reconnect返回true时重置为当前时刻）
  uint16 m_reconnect_count; ///< 当前重连次数
  uint16 m_max_reconnect;   ///< 最大重连次数，0=无限
  int16 m_happen_connected; ///< 链接成功过
  int16 m_state;            ///< 连接状态 RECONN_STATE_DISCONNECTED/RECONN_STATE_CONNECTED
};

} // namespace lb_common

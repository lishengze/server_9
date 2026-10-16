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
 * 仅维护断线时间，check_reconnect() 返回 true 时递增重连计数，
 * 曾连接过路径同时重置断线时间（作为下次重连间隔计算的起点）。
 *
 * 使用场景：
 * - TCP连接断线重连
 * - 会话链路恢复
 * - 需要控制重连策略的网络通道
 */

#include "comm_sys.h"
#include "matomic.h"
#include <ctime>

namespace lb_common
{

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
   * if (rc.check_reconnect(3)) {  // 重连间隔3秒
   *     // 发起重连
   * }
   * rc.set_connected();      // 重连成功，清除重连计数
   * @endcode
   */
  class reconnect_ctl
  {
  public:
    /**
     * @brief 构造函数
     *
     * 初始为断线状态，无重连限制
     */
    reconnect_ctl()
        : m_disconnect_time(0), m_reconnect_count(0), m_max_reconnect(0), m_happen_connected(0) {}

    /**
     * @brief 初始化重连控制参数
     *
     * @param[in] max_reconnect 最大重连次数，0表示无限重连
     */
    void init(int16 max_reconnect)
    {
      atomic_store64(&m_disconnect_time, 0);
      if (max_reconnect > 0)
        atomic_store16(&m_max_reconnect, (int32)max_reconnect);
      else
        atomic_store16(&m_max_reconnect, 0);
      atomic_store32(&m_reconnect_count, 0);
      atomic_store16(&m_happen_connected, 0);
    }

    /**
     * @brief 设置为已连接状态
     *
     * 连接成功时调用，清除重连计数。
     * 仅在断线状态下生效。
     */
    void set_connected()
    {
      atomic_store16(&m_happen_connected, 1);
      atomic_store32(&m_reconnect_count, 0);
    }

    /**
     * @brief 设置为断线状态
     *
     * 连接断开时调用，记录断线时间。
     * 幂等：若已处于断线状态则不重置断线时间（避免反复推迟重连间隔）。
     * 允许 CONNECTED/NEED_CONNECT → DISCONNECTED 转移。
     */
    void set_disconnected()
    {
      atomic_store64(&m_disconnect_time, (int64)std::time(nullptr));
    }

    /**
     * @brief 检查是否应发起重连，单线程检查
     *
     * 判定顺序（与代码一致）：
     * 0. 上限门（对所有状态生效）：max_reconnect>0 且重连计数已达上限 → false
     *    （max_reconnect=0 时无限制）
     * 1. 从未成功连接过（set_connected 未被调过）：计数重试，返回 true
     *    （每次调用即允许一次重试，不受重连间隔限制，受上限门限制）
     * 2. 曾连接过：距上次尝试（断线时间）已超过 reconnect_interval → true
     *
     * 返回 true 时递增重连计数；曾连接过路径同时重置断线时间为当前时刻
     * （作为下次重连间隔计算的起点）。
     *
     * @param[in] reconnect_interval 重连间隔时间（秒），必须>0（仅对曾连接过路径生效）
     * @return true=应发起重连，false=不应重连
     */
    bool check_reconnect(int32 reconnect_interval)
    {
      int32 tmax = atomic_load16(&m_max_reconnect);
      if (tmax > 0 && atomic_load32(&m_reconnect_count) >= tmax)
      {
        return false;
      }

      // 首次连接门：从未成功连接过（set_connected 未被调过）则重连
      if (atomic_load16(&m_happen_connected) == 0)
      {
        atomic_fetch_add32(&m_reconnect_count, 1);
        return true;
      }

      int64 now = (int64)std::time(nullptr);
      int64 tdisc = atomic_load64(&m_disconnect_time);
      if (tdisc == 0 || std::difftime((time_t)now, (time_t)tdisc) >= reconnect_interval)
      {
        atomic_fetch_add32(&m_reconnect_count, 1);
        atomic_store64(&m_disconnect_time, now);
        return true;
      }
      return false;
    }

    /**
     * @brief 是否链接成功过
     *
     */
    bool have_connected() const { return m_happen_connected != 0; }

    /**
     * @brief 获取当前重连次数
     * @return 重连次数
     */
    int32 get_reconnect_count() const
    {
      return atomic_load32(&m_reconnect_count);
    }

    /**
     * @brief 获取最大重连次数
     * @return 最大重连次数，0=无限重连
     */
    uint16 get_max_reconnect() const { return (uint16)atomic_load16(&m_max_reconnect); }

    /**
     * @brief 获取断线时间
     * @return 断线时间戳（秒），0=未断线过
     */
    time_t get_disconnect_time() const { return (time_t)atomic_load64(&m_disconnect_time); }

    /**
     * @brief 设置最大重连次数
     * @param[in] max_reconnect 最大重连次数，0=无限重连
     */
    void set_max_reconnect(int16 max_reconnect) { atomic_store16(&m_max_reconnect, max_reconnect); }

    /**
     * @brief 重置重连计数
     *
     * 清除重连计数，保留其他状态。可用于外部重置重连策略。
     */
    void reset_reconnect_count() { atomic_store32(&m_reconnect_count, 0); }

  private:
    int64 m_disconnect_time;  ///< 断线时间（秒；跨线程原子访问。check_reconnect返回true时重置为当前时刻）
    int32 m_reconnect_count;  ///< 当前重连次数（跨线程原子访问）
    int16 m_max_reconnect;    ///< 最大重连次数，0=无限（跨线程原子访问）
    int16 m_happen_connected; ///< 链接成功过（跨线程原子访问）
  };

} // namespace lb_common

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

namespace lb_common
{

/** @brief 心跳超时次数阈值（连续超时次数超过此值判定链路异常） */
#define CH_HEART_TIMEOUT_NUM 3

  /**
   * @brief 心跳管理类
   *
   * 用于管理连接心跳超时检测。维护上次心跳时间和业务消息计数，
   * 通过 check_timeout() 判定链路是否异常。
   */
  class heart_manage
  {
  public:
    /**
     * @brief 构造函数
     * @param[in] interval_sec 心跳超时间隔（秒），默认5秒
     */
    explicit heart_manage() : m_interval(5)
    {
      atomic_store64(&last_heart, std::time(nullptr));
      atomic_store64(&last_send, 0);
    }

    /**
     * @brief 初始化
     * @param[in] interval_sec 新的心跳间隔（秒）
     */
    void init(int32 interval_sec)
    {
      atomic_store32(&m_interval, interval_sec);
      atomic_store64(&last_heart, std::time(nullptr));
      atomic_store64(&last_send, 0);
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
     * 功能：更新上次活动时间（心跳也算链路活动，刷新活性时钟）
     */
    FORCE_INLINE void on_heart()
    {
      if (atomic_load32(&m_interval) > 0)
      {
        atomic_store64(&last_heart, std::time(nullptr));
      }
    }

    /**
     * @brief 收到业务消息（非心跳）时调用
     * 功能：更新上次活动时间——业务消息与心跳一样刷新活性时钟，
     *       使“仅靠业务消息维持活性（不发心跳）的稀疏链路”不会在
     *       3×interval 后被误判超时；B5 半死链路（收一条后静默）由
     *       last_heart 老化统一检出，不再依赖 msg_count。
     */
    FORCE_INLINE void on_msg()
    {
      if (atomic_load32(&m_interval) > 0)
      {
        atomic_store64(&last_heart, std::time(nullptr));
      }
    }

    /**
     * @brief 检查是否心跳超时/链路异常
     * @return true: 超时；false: 链路正常
     * @note check_timeout 由外部周期性调用。last_heart 由 on_heart（心跳）与
     *       on_msg（业务消息）共同刷新——任何链路活动都会重置活性时钟。
     *       链路在收到一条消息后静默（无心跳、无业务消息）超过 3×interval，
     *       last_heart 老化 → tdiff ≥ 3×interval → 超时（半死链路检出）。
     *       仅靠稀疏业务消息维持的活链路：每次消息刷新 last_heart，tdiff 不会
     *       成熟 → 不误超时。
     */
    bool check_timeout() const
    {
      int32 tin = atomic_load32(&m_interval);
      if (tin == 0)
        return false;
      std::time_t now = std::time(nullptr);
      int32 tdiff = std::difftime(now, atomic_load64(&last_heart));
      return (tdiff >= tin * CH_HEART_TIMEOUT_NUM);
    }

    /**
     * @brief 检查是否发送心跳，单线程检查
     * @return true: 发送；false: 不发送
     */
    bool check_send()
    {
      int32 tin = atomic_load32(&m_interval);
      if (tin == 0)
      {
        return false;
      }
      std::time_t now = std::time(nullptr);
      int64 tlast = atomic_load64(&last_send);
      if (tlast != 0)
      {
        int32 tdiff = std::difftime(now, tlast);
        if (tdiff >= tin)
        {
          atomic_store64(&last_send, now);
          return true;
        }
      }
      else
      {
        atomic_store64(&last_send, now);
        return true;
      }
      return false;
    }

  private:
    int32 m_interval; ///< 心跳间隔（秒，原子访问）
    int64 last_heart; ///< 上次活动时间（秒，心跳或业务消息均刷新；按 int64 原子访问）
    int64 last_send;  ///< 上次发送心跳时间（秒，同上）
  };

} // namespace lb_common

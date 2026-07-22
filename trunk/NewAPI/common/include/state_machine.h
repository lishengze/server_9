#pragma once

/**
 * @file state_machine.h
 * @brief 状态机管理模块
 *
 * 提供线程安全的状态机管理功能，支持资源生命周期控制和异步事件处理。
 * 使用原子操作保证多线程环境下的状态一致性，支持引用计数和状态转换。
 *
 * 主要特性：
 * - 原子操作保证线程安全
 * - 状态机生命周期管理
 * - 异步事件投递控制
 * - 引用计数机制
 * - 高性能状态转换
 *
 * 使用场景：
 * - 网络连接管理
 * - 资源生命周期控制
 * - 异步事件处理
 * - 多线程同步控制
 */

#include "comm_sys.h"
#include "matomic.h"

namespace lb_common {

/**
 * @brief 资源状态引用管理类
 *
 * 提供线程安全的资源状态管理，支持初始化、工作、关闭和已关闭四种状态。
 * 使用64位整数存储状态信息和引用计数，通过原子操作保证多线程安全。
 *
 * 状态定义：
 * - CSTATEREF_INITING: 初始化中状态
 * - CSTATEREF_INITED: 初始化成功状态
 * - CSTATEREF_WORK: 工作状态
 * - CSTATEREF_CLOSING: 关闭中状态
 * - CSTATEREF_CLOSED: 已关闭状态
 *
 * 状态转换流程：
 * CLOSED -> INITING -> INITED -> WORK -> CLOSING -> CLOSED
 */

class src_stat_ref {
private:
#define CSTATEREF_CLOSED  0
#define CSTATEREF_CLOSING 0x10000000000000LL
#define CSTATEREF_INITING 0x20000000000000LL
#define CSTATEREF_INITED  0x40000000000000LL
#define CSTATEREF_WORK    0x80000000000000LL
#define CSTATEREF_REFMASK 0xFFFFFFFFFFFFLL

  ///< 状态和引用计数的组合值
  int64 statref;

public:
  /**
   * @brief 构造函数
   *
   * 初始化为已关闭状态
   */
  src_stat_ref() { statref = CSTATEREF_CLOSED; }

  /**
   * @brief 析构函数
   *
   * 重置为已关闭状态
   */
  ~src_stat_ref() { statref = CSTATEREF_CLOSED; }

public:
  /**
   * @brief 初始化资源
   *
   * 从已关闭状态转换为初始化中状态，并增加引用计数。
   * 只能在已关闭状态下调用。
   *
   * @return bool 成功返回true，失败返回false
   */
  bool to_init();

  /**
   * @brief 结束初始化,设置为初始化成功状态
   *
   * 根据初始化结果将状态转换为初始化成功状态或已关闭状态。
   * 只能在初始化状态下调用。
   *
   * @param[in] init_ok 初始化结果是否成功
   * @param[out] o_closed
   * 输出参数，返回是否已关闭，函数返回true时，应检查此输出参数
   * @return bool 成功清除initing状态，返回true，失败返回false
   */
  bool end_init(bool &o_closed, bool init_ok);

  /**
   * @brief 设置为工作状态
   *
   * 只能在初始化成功状态后调用。
   *
   * @return bool 成功返回true，失败返回false
   */
  bool set_work();

  /**
   * @brief 结束工作,设置为初始化成功状态
   *
   * 只能在初始化成功状态或工作状态调用。
   *
   * @return bool 成功返回true，失败返回false
   */
  bool stop_work();

  /**
   * @brief 增加引用计数
   *
   * 在初始化成功或工作状态下增加引用计数。
   * 只能在非关闭状态下调用。
   *
   * @return bool 成功返回true，失败返回false
   */
  bool add_ref();

  /**
   * @brief 减少引用计数
   *
   * 减少引用计数，当引用计数为0时自动转换为已关闭状态。
   *
   * @return bool 返回true表示资源已关闭，false表示仍需继续使用
   */
  bool sub_ref();

  /**
   * @brief 开始关闭资源
   *
   * 将状态转换为关闭中，阻止新的引用增加。
   *
   * @param[out] o_closed 输出参数，返回是否引用计数为0
   * @return bool 成功返回true，失败返回false
   */
  bool to_close(bool &o_closed);

  /**
   * @brief 检查是否已初始化
   *
   * @return bool 正在初始化返回true，否则返回false
   */
  FORCE_INLINE bool is_inited() const {
    return ((statref & (CSTATEREF_CLOSING | CSTATEREF_INITED)) == CSTATEREF_INITED);
  }

  /**
   * @brief 检查是否空闲（已关闭）
   *
   * @return bool 已关闭返回true，否则返回false
   */
  FORCE_INLINE bool is_free() const { return (statref == CSTATEREF_CLOSED); }

  /**
   * @brief 检查是否在工作状态
   *
   * @return bool 工作状态返回true，否则返回false
   */
  FORCE_INLINE bool is_work() const { return ((statref & (CSTATEREF_CLOSING | CSTATEREF_WORK)) == CSTATEREF_WORK); }

  /**
   * @brief 检查是否正在关闭
   *
   * @return bool 关闭中或已关闭返回true，否则返回false
   */
  FORCE_INLINE bool is_close() const { return (statref <= CSTATEREF_CLOSING); }
};

/**
 * @brief 异步事件控制类
 *
 * 用于保护多线程环境下的异步事件投递，减少重复投递。
 * 适用于资源自身为事件和投递引用本资源的其他事件场景。
 *
 * 主要功能：
 * - 异步事件投递控制
 * - 引用计数管理
 * - 重复投递防护
 * - 生命周期管理
 *
 * 使用场景：
 * - 网络读队列事件
 * - 定时器事件
 * - 多线程事件处理
 * - 资源回收保护
 */
class src_async_ctl {
protected:
#define CDELIVE_ASYNC_REFMASK 0xFFFFFFFFFFFFLL
#define CDELIVE_ASYNC_HAVE    0x1000000000000LL
#define CDELIVE_ASYNC_CLOSE   0x2000000000000LL

  ///< 状态和引用计数的组合值
  int64 statref;

public:
  /**
   * @brief 检查是否在异步处理中
   *
   * @return bool 异步处理中返回true，否则返回false
   */
  FORCE_INLINE bool in_async() const { return (statref & CDELIVE_ASYNC_HAVE) == CDELIVE_ASYNC_HAVE; }

  /**
   * @brief 增加引用计数
   *
   * 在非关闭状态下增加引用计数。
   *
   * @return bool 成功返回true，失败返回false
   */
  FORCE_INLINE bool add_ref() {
    int64 t = atomic_load64(&statref);
    while (true) {
      if (unlikely(t >= CDELIVE_ASYNC_CLOSE))
        return false;
      if (likely(atomic_cas64_weak(&statref, &t, t + 1))) {
        break;
      }
    }
    return true;
  }

  /**
   * @brief 减少引用计数
   *
   * 减少引用计数，当引用计数为1且处于关闭状态时返回true。
   *
   * @return bool 返回true表示需要执行关闭后操作
   */
  FORCE_INLINE bool sub_ref() {
    int64 t = atomic_load64(&statref);
    while (true) {
      if (unlikely((t & CDELIVE_ASYNC_REFMASK) == 0)) {
        break;
      }
      if (likely(atomic_cas64_weak(&statref, &t, t - 1))) {
        break;
      }
    }
    if (unlikely(t > CDELIVE_ASYNC_CLOSE && (t & CDELIVE_ASYNC_REFMASK) == 1)) {
      return true;
    }
    return false;
  }

  /**
   * @brief 检查是否已关闭
   *
   * @return bool 已关闭返回true，否则返回false
   */
  FORCE_INLINE bool is_close() const { return statref >= CDELIVE_ASYNC_CLOSE; }

  /**
   * @brief 投递异步事件
   *
   * 投递自身事件并增加标记，防止重复投递。
   * 最多允许一个异步事件投递。
   *
   * @return bool 成功返回true，失败返回false
   */
  FORCE_INLINE bool to_async() {
    int64 t = atomic_load64(&statref);
    while (true) {
      if (t >= CDELIVE_ASYNC_HAVE)
        return false;
      if (likely(atomic_cas64_weak(&statref, &t, (t | CDELIVE_ASYNC_HAVE) + 1))) {
        break;
      }
    }
    return true;
  }

  /**
   * @brief 结束异步事件处理
   *
   * 结束异步事件处理，减少引用计数并清除异步标记。
   *
   * @return bool 返回true表示需要执行关闭后操作
   */
  FORCE_INLINE bool end_async() {
    int64 t = atomic_load64(&statref);
    while (true) {
      if (unlikely((t & CDELIVE_ASYNC_HAVE) == 0))
        return false;
      if (likely(atomic_cas64(&statref, &t, (t ^ CDELIVE_ASYNC_HAVE) - 1))) {
        break;
      }
    }
    if (unlikely(t > CDELIVE_ASYNC_CLOSE && (t & CDELIVE_ASYNC_REFMASK) == 1)) {
      return true;
    }
    return false;
  }

  /**
   * @brief 清除异步标记
   *
   * 和sub_ref一起使用，可允许重复投递，最多两个。
   * 清除异步投递标记，允许重新投递。
   */
  FORCE_INLINE void clear_async() {
    int64 t = atomic_load64(&statref);
    while (true) {
      if ((t & CDELIVE_ASYNC_HAVE) == 0)
        break;
      if (likely(atomic_cas64(&statref, &t, t ^ CDELIVE_ASYNC_HAVE))) {
        break;
      }
    }
  }

  /**
   * @brief 开始关闭操作
   *
   * 设置关闭标志，阻止新的异步投递。
   *
   * @param[out] o_closed 输出参数，返回是否引用计数为0
   * @return bool 成功返回true，失败返回false
   */
  bool to_close(int32 &o_closed) {
    o_closed = 0;
    int64 t = atomic_load64(&statref);
    while (true) {
      if (unlikely(t >= CDELIVE_ASYNC_CLOSE)) {
        return false;
      }
      if (likely(atomic_cas64(&statref, &t, t | CDELIVE_ASYNC_CLOSE))) {
        break;
      }
    }
    if (t == 0) {
      o_closed = 1;
    }
    return true;
  }

  /**
   * @brief 结束关闭操作
   *
   * 重置状态为0，完成关闭操作。
   */
  FORCE_INLINE void end_close() { atomic_store64(&statref, 0); }

  /**
   * @brief 构造函数
   *
   * 初始化为空闲状态
   */
  src_async_ctl() { atomic_store64(&statref, 0); }

  /**
   * @brief 析构函数
   */
  ~src_async_ctl() { atomic_store64(&statref, 0); }
};

/**
 * @brief 自旋状态锁
 *
 * 使用单个int32原子变量，低16位为主状态，高16位为附加操作状态。
 * 主状态和附加状态通过位掩码组合在一个原子变量中，保证状态转换的原子性。
 *
 * 主状态定义（低16位）：
 * - SPIN_STATE_IDLE       (0)  空闲
 * - SPIN_STATE_INITING    (1)  初始化中
 * - SPIN_STATE_WORKING    (2)  工作
 * - SPIN_STATE_CLOSING    (3)  关闭中
 *
 * 附加操作状态（高16位）：
 * - SPIN_OP_SEND          (1<<16) 发送中
 * - SPIN_OP_RECV          (1<<17) 接收中
 *
 * 状态转换规则：
 * - 空闲 → 初始化中：仅空闲时可进入，CAS保证唯一
 * - 初始化中 → 工作 或 空闲：初始化完成（成功→工作，失败→空闲）
 * - 工作 → 关闭中：仅工作状态可进入关闭
 * - 关闭中 → 空闲：关闭完成
 * - 发送/接收：仅工作状态且非关闭中时可操作
 *
 * @note POD兼容设计，可放入共享内存
 * @note 临界区必须极短，禁止在持有锁时做IO或长时间操作
 *
 * @example
 * @code
 * spin_state_lock lock;
 * lock.init();
 *
 * // 初始化
 * if (lock.begin_init()) {
 *     // 执行初始化...
 *     lock.end_init(true);  // 成功进入工作状态
 *     // 或 lock.end_init(false); // 失败回到空闲状态
 * }
 *
 * // 发送
 * if (lock.send_lock()) {
 *     // 发送数据...
 *     lock.send_unlock();
 * }
 *
 * // 接收
 * if (lock.recv_lock()) {
 *     // 接收数据...
 *     lock.recv_unlock();
 * }
 *
 * // 关闭
 * if (lock.begin_close()) {
 *     // 关闭完成
 *     lock.end_close();
 * }
 * @endcode
 */
class spin_state_lock {
public:
  // ==================== 主状态定义（低16位） ====================

#define SPIN_STATE_IDLE    0 ///< 空闲状态
#define SPIN_STATE_INITING 1 ///< 初始化中状态
#define SPIN_STATE_WORKING 2 ///< 工作状态
#define SPIN_STATE_CLOSING 3 ///< 关闭中状态

#define SPIN_STATE_MASK 0x0000FFFF ///< 主状态掩码

  // ==================== 附加操作状态（高16位） ====================

#define SPIN_OP_SEND 0x00010000 ///< 发送操作标志
#define SPIN_OP_RECV 0x00020000 ///< 接收操作标志
#define SPIN_OP_MASK 0xFFFF0000 ///< 附加操作状态掩码

  /**
   * @brief 初始化锁
   *
   * 将状态设置为空闲
   */
  FORCE_INLINE void init() { atomic_store32(&state_, SPIN_STATE_IDLE); }

  /**
   * @brief 是否处于工作状态
   * @return true=工作状态
   */
  FORCE_INLINE bool is_work() const { return (state_ & SPIN_STATE_MASK) == SPIN_STATE_WORKING; }

  /**
   * @brief 是否处于关闭状态
   * @return true=关闭中状态
   */
  FORCE_INLINE bool is_close() const {
    int32 ts = (atomic_load32(&state_) & SPIN_STATE_MASK);
    return (ts & SPIN_STATE_CLOSING) != 0 || ts == SPIN_STATE_IDLE;
  }

  /**
   * @brief 是否处于空闲状态
   * @return true=空闲状态
   */
  FORCE_INLINE bool is_free() const { return (atomic_load32(&state_) & SPIN_STATE_MASK) == SPIN_STATE_IDLE; }

  // ==================== 初始化操作 ====================

  /**
   * @brief 开始初始化
   *
   * 仅在空闲状态下可进入初始化中状态，CAS保证只有一个线程成功。
   *
   * @return true=成功占住初始化状态, false=非空闲状态
   */
  FORCE_INLINE bool to_init() {
    int32 expect = SPIN_STATE_IDLE;
    if (atomic_cas32(&state_, &expect, SPIN_STATE_INITING)) {
      return true;
    }
    return false;
  }

  /**
   * @brief 结束初始化
   *
   * 依据初始化成功与否，转换到工作状态或回到空闲状态。
   * 必须在初始化中状态调用。
   *
   * @param[in] success true=初始化成功进入工作状态, false=初始化失败回到空闲状态
   */
  FORCE_INLINE void end_init(bool success) {
    int32 cur = atomic_load32(&state_);
    if ((cur & SPIN_STATE_MASK) != SPIN_STATE_INITING) {
      return;
    }

    if (success) {
      atomic_store32(&state_, SPIN_STATE_WORKING);
    } else {
      atomic_store32(&state_, SPIN_STATE_IDLE);
    }
  }

  // ==================== 发送操作 ====================

  /**
   * @brief 获取发送锁
   *
   * 发送优先策略：
   * 1. 必须在工作状态且非关闭中
   * 2. 若已有附加发送状态，自旋等待发送状态清空
   * 3. 占住附加发送状态
   * 4. 自旋等待接收状态清空且主状态仍为工作（非关闭中）
   * 5. 全部满足后返回成功
   *
   * @return true=成功获取发送锁, false=状态非工作
   *
   * @note 必须与 send_unlock() 配对使用
   * @note 临界区必须极短
   */
  bool send_lock() {
    int32 i = 0;
    do {
      int32 cur = atomic_load32(&state_);

      // 必须在工作状态且非关闭中
      if (unlikely((cur & SPIN_STATE_MASK) != SPIN_STATE_WORKING)) {
        return false;
      }

      // 若已有发送状态，自旋等待
      if (unlikely((cur & SPIN_OP_SEND) != 0)) {
        SPIN_PAUSE(i);
        continue;
      }

      // 尝试占住发送状态
      if (likely(atomic_cas32_weak(&state_, &cur, cur | SPIN_OP_SEND))) {
        if ((cur & SPIN_OP_RECV) == 0) {
          return true;
        }
      } else {
        SPIN_PAUSE(i);
        continue;
      }

      // 发送状态已占住，等待接收状态清空且主状态仍为工作
      i = 0;
      do {
        cur = atomic_load32(&state_);

        // 检查主状态是否仍为工作（非关闭中）
        if ((cur & SPIN_STATE_MASK) != SPIN_STATE_WORKING) {
          // 主状态已变，释放发送状态并返回错误
          atomic_fetch_sub32(&state_, SPIN_OP_SEND);
          return false;
        }

        // 检查接收状态
        if ((cur & SPIN_OP_RECV) == 0) {
          return 0;
        }

        SPIN_PAUSE(i);
      } while (true);
    } while (true);
  }

  /**
   * @brief 释放发送锁
   *
   * 清除附加发送状态
   */
  FORCE_INLINE void send_unlock() { atomic_fetch_sub32(&state_, SPIN_OP_SEND); }

  // ==================== 接收操作 ====================

  /**
   * @brief 获取接收锁
   *
   * 接收策略：
   * 1. 必须在工作状态且非关闭中
   * 2. 若有附加发送状态，自旋等待发送状态清空
   * 3. 再次检查工作状态
   * 4. 占住附加接收状态
   *
   * @return true=成功获取接收锁, false=状态不允许接收
   *
   * @note 必须与 recv_unlock() 配对使用
   * @note 临界区必须极短
   */
  bool recv_lock() {
    int32 i = 0;
    do {
      int32 cur = atomic_load32(&state_);

      // 若已有发送状态，自旋等待发送清空
      if (unlikely((cur & SPIN_OP_SEND) != 0)) {
        SPIN_PAUSE(i);
        continue;
      }

      if (unlikely((cur & SPIN_STATE_MASK) != SPIN_STATE_WORKING)) {
        return false;
      }

      // 尝试占住接收状态
      if (likely(atomic_cas32_weak(&state_, &cur, cur | SPIN_OP_RECV))) {
        return true;
      }
      SPIN_PAUSE(i);
    } while (true);
  }

  /**
   * @brief 释放接收锁
   *
   * 清除附加接收状态
   */
  FORCE_INLINE void recv_unlock() { atomic_fetch_sub32(&state_, SPIN_OP_RECV); }

  // ==================== 关闭操作 ====================

  /**
   * @brief 设置关闭标记,非工作不可关闭
   *
   * @return true=设置成功, false=已在关闭中或空闲中
   */
  FORCE_INLINE bool to_close() {
    int32 i = 0;
    do {
      int32 cur = atomic_load32(&state_);

      // 若已在关闭中状态，返回false
      if ((cur & SPIN_STATE_MASK) != SPIN_STATE_WORKING) {
        return false;
      }

      // CAS设置关闭中状态，保留附加操作状态
      if (!atomic_cas32(&state_, &cur, cur | SPIN_STATE_CLOSING)) {
        SPIN_PAUSE(i);
        continue;
      }
      return true;
      // 成功设置关闭中状态，等待发送和接收状态清空

    } while (true);
  }

  /**
   * @brief 结束关闭
   *
   * 将状态从关闭中恢复为空闲
   */
  FORCE_INLINE bool end_close() {
    int32 i = 0;
    do {
      int32 cur = atomic_load32(&state_);
      if ((cur & SPIN_STATE_CLOSING) == 0) {
        return false;
      }

      if ((cur & SPIN_OP_MASK) == 0) {
        atomic_store32(&state_, SPIN_STATE_IDLE);
        return true;
      }
      SPIN_PAUSE(i);
    } while (true);
  }

private:
  int32 state_; ///< 状态变量：低16位=主状态，高16位=附加操作状态

  /**
   * @brief 自旋忙碌阈值
   *
   * 达到此值后调用CPU_PAUSE释放总线
   */
  static constexpr int32 BUSY_COUNT = 30000;

  /**
   * @brief 自旋暂停辅助
   *
   * 达到自旋上限时调用CPU_PAUSE，避免总线占用
   *
   * @param[in,out] spin_count 自旋计数引用
   */
  static FORCE_INLINE void SPIN_PAUSE(int32 &spin_count) {
    spin_count++;
    if (spin_count == BUSY_COUNT) {
      CPU_PAUSE();
      spin_count = 0;
    }
  }
};

} // namespace lb_common

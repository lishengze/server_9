// 链接定时事件类
//
// 模板参数: TLink (链接类型) + TEngine (该 link 所属工作引擎)
// 由工作引擎持有, 知道 link + engine 两个指针.
// deal_event 中调 link 的检查函数, 根据结果向 engine 投递事件.
//
// 不再由 link 内部持有, link 也不再有 on_timer() 方法.

#pragma once

#include "comm_sys.h"
#include "mthread.h"
#include "que_mth_buf.h"
#include "wait_poll.h"

#include "api_event_msg.h"

#include <cstdint>
#include <sys/timerfd.h>
#include <unistd.h>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

using lb_common::int16;
using lb_common::int32;

/// 链接定时事件 (双模板)
/// @tparam TLink   链接类型
/// @tparam TEngine 该 link 所属工作引擎类型
template <class TLink, class TEngine> class link_timer_op : public lb_common::epoll_event_op {
public:
  /// 构造函数: 创建 timerfd
  link_timer_op();

  /// 析构函数: 关闭 timerfd
  ~link_timer_op();

  /// 获取 timerfd 文件描述符
  int32 get_fd() override { return timer_fd_; }
  /// 获取定时器间隔 (秒)
  int32 get_interval() const { return interval_; }

  /// 绑定 link + engine + link_type (由持有本对象的 engine 在 init 阶段调用)
  /// @param link      链接指针
  /// @param engine    link 所属工作引擎指针
  /// @param interval  定时循环的事件间隔
  int32 init_timer(TLink *link, TEngine *engine, int32 interval);

  /// 将timerfd 以自动定时循环方式加入线程的epoll,使用水平触发
  int32 add_timer_poll(lb_common::mthread *th);

  /// 停止定时器
  void close();

  int32 delive_link_event(int16 event_type, int32 event_data);

  /// 定时器触发回调: 读 timerfd -> 决策 -> 调 link 检查函数, 投递事件到 engine
  void deal_event() override;

  /// 错误回调 (timerfd 自身出错几乎不会发生, 保留)
  void deal_error() override {}

  /// 关闭回调
  void deal_close() override {}

private:
  int32 timer_fd_;  ///< timerfd 文件描述符,重复循环定时
  int32 interval_;  ///< 定时循环间隔(秒, 由 init_timer 注入)
  TLink *link_;     ///< 链接指针
  TEngine *engine_; ///< link 所属工作引擎指针
};

} // namespace lb_api

// shared_engine - 共享极速引擎 (模型 d, 骨架)
//
// 仅当 speed_link_type == socket_shared 时存在.
// 内部仅持有多 socket 引擎的发送队列指针, 无链接、无线程.
// 行为 (post_send_event / send_queue) 全部转发到多 socket 引擎.
//
// 接口与 tcpdirect_engine / single_socket_engine 完全一致:
//   init(cfg, tfst, log)   -- 签名统一
//   start() / stop() / close()  -- 均为 no-op, 资源由 multi 引擎管理
//   post_send_event / send_queue  -- 转发到 multi 引擎

#pragma once

#include "aio_socket_link.h"
#include "api_config_impl.h"
#include "api_event_msg.h"
#include "comm_sys.h"
#include "mlog.h"
#include "que_mth_buf.h"

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// 空引擎，用于在 api 模板实例极速引擎的占位，api 实例调用其函数，但控制运行时，实际不会调用 get_queue 和 get_out_op
/// @tparam TFastCounter  极速柜台类型
template <class TFastCounter> class idle_engine {
public:
  /// 初始化
  int32 init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log);

  int32 add_timer_poll(lb_common::mthread *th) { return 0; }

  /// 启动 (no-op, 资源由 multi 引擎管理)
  int32 start() { return 0; }
  /// 停止 (no-op, 资源由 multi 引擎管理)
  void stop() {};

  /// 获取发送队列 (供柜台类 / api_impl 注入)
  lb_common::que_mth_buf *get_queue() { return nullptr; }

  link_engine_outop *get_out_op() { return nullptr; }

  idle_engine();
  ~idle_engine();

  idle_engine(const idle_engine &) = delete;
  idle_engine &operator=(const idle_engine &) = delete;

private:
  TFastCounter *counter_;  ///< 极速柜台指针 (init 阶段注入)
  lb_common::lb_log *log_; ///< 日志指针
};

} // namespace lb_api

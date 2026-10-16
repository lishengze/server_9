// idle_engine - 空占位极速引擎 (fpga_gateway 模式用)
//
// fpga_gateway (socket_shared) 模式下, 极速柜台业务全部由 multi_socket_engine
// 槽 1 的 fast_gw_link 承载, 本类无链接、无线程、无队列,
// 仅为保持 api_impl<TFastCounter, TEngine> 模板签名统一而存在:
//   init(cfg, tfst, log)     -- 签名统一, 仅记录指针
//   start() / stop() / close() / add_timer_poll() -- 均为 no-op, 资源由 multi 引擎管理
//   get_queue() / get_out_op() -- 返回 nullptr (api_impl 对 fpga_gateway 直接使用 multi 引擎队列)

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

/// 空占位引擎: 保持 api_impl 模板签名统一; 运行期业务由 multi 引擎承载,
/// 不会实际使用 get_queue / get_out_op 返回的队列与回调
/// @tparam TFastCounter  极速柜台类型
template <class TFastCounter> class idle_engine {
public:
  /// 初始化
  int32 init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log);

  int32 add_timer_poll(lb_common::mthread *th) { return 0; }

  /// 启动 (no-op, 资源由 multi 引擎管理)
  int32 start() { return 0; }
  /// 回退接口 (no-op, 资源由 multi 引擎管理)
  void stop() {};
  /// 关闭接口 (no-op, 资源由 multi 引擎管理)
  void close() {}

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

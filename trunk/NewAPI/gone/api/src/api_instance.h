// api_instance - API 实例类 (骨架)
//
// api_impl<TFastCounter, TEngine> 继承 api_interface, 按配置选型后实例化.
// 唯一按配置选型的实体, 持有所有子模块.
// link 由对应 engine 内部持有 (tcpdir_link / aio_socket_link), api_impl 不再冗余保存 link 指针.

#pragma once

#include "aio_socket_link.h"
#include "api_callback.h"
#include "api_config.h"
#include "api_config_impl.h"
#include "api_interface.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "counter98.h"
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "gw_counter_direct.h"
#include "idle_engine.h"
#include "mlog.h"
#include "multi_socket_engine.h"
#include "single_socket_engine.h"
#include "tcpdir_link.h"
#include "tcpdirect_engine.h"

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// API 实例模板类
/// @tparam TFastCounter  极速柜台类 (gw_counter_direct / fpga_counter_direct / fpga_counter_gateway)
/// @tparam TEngine       极速引擎类型 (tcpdirect_engine / single_socket_engine / shared_engine)
template <class TFastCounter, class TEngine> class api_impl : public api_interface {
public:
  /// 初始化 (在 create_instance 中调用)
  /// @param cfg  配置实现 (含 fast_counter_type / market_type / 地址等)
  /// @param cb   用户回调接口
  int32 init(const api_config_impl &cfg, api_callback *cb);

  // ---- 实现 api_interface 虚函数 (委托到 fast_/c98_, 业务语义见 api_interface.h) ----

  int32 login(const LoginReq &req) override;
  int32 order_insert(const OrderReq &req) override;
  int32 etf_order_insert(const OrderReq &req) override;
  int32 bse_order_insert(const OrderReq &req) override;
  int32 order_cancel(const CancelReq &req) override;
  int32 order_query(const OrderQueryReq &req) override;
  int32 order_batch_query(const OrderBatchQueryReq &req) override;
  int32 trade_query(const TradeQueryReq &req) override;
  int32 trade_batch_query(const TradeBatchQueryReq &req) override;
  int32 fund_query(const FundQueryReq &req) override;
  int32 position_query(const PositionQueryReq &req) override;
  int32 start() override;
  void stop() override;

  api_impl();
  ~api_impl() override { stop(); };

  api_impl(const api_impl &) = delete;
  api_impl &operator=(const api_impl &) = delete;

private:
  // ---- 拷贝自 cfg 的配置项 ----
  speed_link_type speed_link_type_;                ///< 极速链接类型
  counter_type speed_counter_type_;                ///< 极速柜台类型
  TFastCounter fast_;                              ///< 极速柜台
  TEngine fast_engine_;                            ///< 极速引擎
  multi_socket_engine<TFastCounter> multi_engine_; ///< 2 槽：g98 + second_link_
  counter98 c98_;                                  ///< 98 柜台
  callback_manager cb_mgr_;                        ///< 回调管理器
  int32_t have_start;                              ///< 是否启动
  lb_common::lb_log log_;                          ///< 日志
};

} // namespace lb_api

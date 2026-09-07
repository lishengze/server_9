// shared_engine - 共享极速引擎 (模型 d, 实际是 multi_engine 的代理)
//
// 本类无链接、无线程、转发所有动作到 multi_engine 的 send_queue_。

#include "idle_engine.h"
#include "fpga_counter_gateway.h"

namespace lb_api {

// 构造函数: 默认无柜台 / 无日志
template <class TFastCounter> idle_engine<TFastCounter>::idle_engine() : counter_(nullptr), log_(nullptr) {}

template <class TFastCounter> idle_engine<TFastCounter>::~idle_engine() = default;

// 初始化: 仅记录柜台 / 日志, 队列由 init_set_que 注入
template <class TFastCounter>
int32 idle_engine<TFastCounter>::init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log) {
  counter_ = tfst;
  log_ = log;
  return 0;
}

template class idle_engine<fpga_counter_gateway>;

} // namespace lb_api

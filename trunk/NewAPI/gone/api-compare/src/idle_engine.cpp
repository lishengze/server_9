// idle_engine - 空占位极速引擎实现
//
// 无链接、无线程、无队列; fpga_gateway 模式的业务由 multi 引擎承载, 本类不转发任何动作。

#include "idle_engine.h"
#include "fpga_counter_gateway.h"

namespace lb_api {

// 构造函数: 默认无柜台 / 无日志
template <class TFastCounter> idle_engine<TFastCounter>::idle_engine() : counter_(nullptr), log_(nullptr) {}

template <class TFastCounter> idle_engine<TFastCounter>::~idle_engine() = default;

// 初始化: 仅记录柜台 / 日志指针
template <class TFastCounter>
int32 idle_engine<TFastCounter>::init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log) {
  counter_ = tfst;
  log_ = log;
  return 0;
}

template class idle_engine<fpga_counter_gateway>;

} // namespace lb_api

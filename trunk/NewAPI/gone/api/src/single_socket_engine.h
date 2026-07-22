// single_socket_engine - 单 socket 极速引擎 (模型 b, 骨架)
//
// 一个 aio_socket_link + 一个 link_timer_op + 一个发送队列 + 一条专属线程.
// 与 tcpdirect_engine 结构对称, 但使用 aio_socket_link (基于 aio_tcp / mthread epoll)
// 仅在 socket_single 模式下使用 (C1/C3 配置)

#pragma once

#include "aio_socket_link.h"
#include "api_config_impl.h"
#include "comm_sys.h"
#include "link_timer_op.h"
#include "mlog.h"
//#include "multi_socket_engine.h"
#include "que_mth_buf.h"
#include "simple_thread.h"

#include <cstdint>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// 单 socket 极速引擎 (模型 b, 模板类)
/// @tparam TFastCounter  极速柜台类型
template <class TFastCounter> class single_socket_engine : public lb_common::simple_thread {
public:
  class eng_link_op : public link_engine_outop {
  public:
    single_socket_engine<TFastCounter> *owner_ = nullptr;
    void deal_heart_msg_ans(int16 link_type) override;
    void trigger_send() override {};
    void deal_close_link(int16 link_type, int32 err_code) override;
  };

  /// 初始化
  int32 init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log);

  int32 add_timer_poll(lb_common::mthread *th);

  /// 启动引擎线程
  int32 start();
  /// 停止引擎线程
  void stop();

  /// 获取发送队列 (供柜台类 / api_impl 注入)
  lb_common::que_mth_buf *get_queue() { return &send_queue_; }

  link_engine_outop *get_out_op() { return &link_outop_; }

  single_socket_engine() : send_poll_num_(0), recv_poll_num_(0), counter_(nullptr), log_(nullptr) {}
  ~single_socket_engine() override { stop(); }

  single_socket_engine(const single_socket_engine &) = delete;
  single_socket_engine &operator=(const single_socket_engine &) = delete;

  friend class eng_heart_ans_op;

protected:
  /// simple_thread 虚函数实现: 极速引擎主循环 (从 send_queue_ 取事件分发到 link/柜台)
  void do_work() override;
  /// simple_thread 虚函数实现: 极速引擎是否需要工作 (恒为 true, 极速引擎不休眠)
  bool need_work() override { return true; }

  void deal_cust_login(const acc_login_event_info &pmlog);

  void deal_fpga_core_connect(const fpga_core_connect_info &pmlog);

private:
  int32 send_poll_num_;                ///< 发送轮询批量数 (从 cfg 拷贝)
  int32 recv_poll_num_;                ///< 接收轮询批量数 (从 cfg 拷贝)
  lb_common::que_mth_buf send_queue_;  ///< 发送队列 (柜台 / api_impl 共享)
  aio_socket_link<TFastCounter> link_; ///< 极速交易链接
  TFastCounter *counter_;              ///< 极速柜台指针 (init 阶段注入)
  lb_common::lb_log *log_;             ///< 日志指针
  link_timer_op<aio_socket_link<TFastCounter>, single_socket_engine> timer_op_; ///< 链接定时器 (由 link_timer_op 决策)
  eng_link_op link_outop_;
};

} // namespace lb_api

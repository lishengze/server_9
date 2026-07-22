// multi_socket_engine - 多 socket 引擎 (模型 c, 骨架)
//
// 简化为 2 槽设计（融合 multi 槽 1 不同柜台语义）：
//   槽 0: g98_link_         - 98 柜台链接（恒存在）
//   槽 1: fast_gw_link_      - 极速柜台网关链接：
//                              · TFastCounter=gw_direct   → ❌ 闲置（个微在 fast_engine）
//                              · TFastCounter=fpga_direct  → fpga GW 链接（登录/证券信息）
//                              · TFastCounter=fpga_gateway → fpga GW 链接（登录/证券信息/业务）
//
// 模板参数 TFastCounter 硬编码消息路由 (无适配器, 无虚基类).
//
// 自身同时承担两种角色:
//   1) 多个链接的工作引擎 (98 + fpga_gw/个微/闲置 都在本线程)
//   2) 极速引擎 (tcpdirect / single_socket) 的 timerfd 注册目标 (即控制平面 epoll)

#pragma once

#include "aio_socket_link.h"
#include "api_config_impl.h"
#include "comm_sys.h"
#include "counter98.h"
#include "link_timer_op.h"
#include "mlog.h"
#include "mthread.h"
#include "que_mth_buf.h"
#include "wait_poll.h"
#include "wait_wake.h"

#include <cstdint>
#include <sys/eventfd.h>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// 多 socket 引擎 (模型 c, 模板类)
/// @tparam TFastCounter  柜台类型 (硬编码路由: fpga_* / counter98 / gw_direct_counter)
template <class TFastCounter> class multi_socket_engine : public lb_common::epoll_event_op {
public:
  class eng_link_op : public link_engine_outop {
  public:
    multi_socket_engine<TFastCounter> *owner_ = nullptr;
    void deal_heart_msg_ans(int16 link_type) override;
    void trigger_send() override;
    void deal_close_link(int16 link_type, int32 err_code) override;
  };

  /// 初始化
  int32 init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log, counter98 *c98);

  int32 connect_98agw();

  /// 启动多 socket 引擎线程 (同时启动 epoll_th_)
  int32 start();
  /// 停止多 socket 引擎线程
  void stop();

  /// 获取发送队列 (供柜台类 / api_impl 注入)
  lb_common::que_mth_buf *get_queue() { return &send_queue_; }

  link_engine_outop *get_out_op() { return &link_outop_; }

  lb_common::mthread *get_thread() { return &epoll_th_; }

  multi_socket_engine()
      : send_poll_num_(0), recv_poll_num_(0), queue_dealing_(0), counter_(nullptr), counter98_(nullptr), log_(nullptr) {
  }
  ~multi_socket_engine() override { stop(); }

  /// 禁用拷贝
  multi_socket_engine(const multi_socket_engine &) = delete;
  multi_socket_engine &operator=(const multi_socket_engine &) = delete;

  friend class eng_heart_ans_op;

protected:
  /// 获取 timerfd 文件描述符
  int32 get_fd() override { return queue_wake_.get_fd(); }

  void deal_event() override;

  /// 错误回调 (timerfd 自身出错几乎不会发生, 保留)
  void deal_error() override {}

  /// 关闭回调
  void deal_close() override {}

  void deal_cust_login(const acc_login_event_info &pmlog);
  void deal_cust98_login(const acc_login_event_info &pmlog);

  void deal_agw98_login();

  // ---- 公共成员（先放队列，再放链接）----

  int32 send_poll_num_;               ///< 发送轮询批量数 (从 cfg 拷贝)
  int32 recv_poll_num_;               ///< 接收轮询批量数 (从 cfg 拷贝)
  lb_common::que_mth_buf send_queue_; ///< 发送队列（柜台类 / 极速引擎 共享）
  lb_common::event_wake queue_wake_;  ///< 发送队列触发事件
  int32 queue_dealing_;               ///< 发送队列处理中

  /// 98 链接（恒存在）
  aio_socket_link<counter98> g98_link_;
  link_timer_op<aio_socket_link<counter98>, multi_socket_engine> g98_link_timer_;

  /// 极速柜台网关链接（fast_gw_link）
  /// 语义：fpga_direct 模式 = fpga GW；fpga_gateway 模式 = fpga GW（业务也走此）；个微模式 = 闲置
  aio_socket_link<TFastCounter> fast_gw_link_;
  link_timer_op<aio_socket_link<TFastCounter>, multi_socket_engine> fast_gw_link_timer_;

  TFastCounter *counter_;       ///< 极速柜台指针 (init 阶段注入)
  counter98 *counter98_;        ///< 98 柜台指针 (init_98_counter 注入)
  lb_common::mthread epoll_th_; ///< epoll 线程
  lb_common::lb_log *log_;      ///< 日志
  eng_link_op link_outop_;
};

} // namespace lb_api

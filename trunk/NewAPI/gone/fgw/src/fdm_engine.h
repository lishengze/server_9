// fdm_engine - fdm 端连接管理和操作引擎
//
// Step 5 已确认成员定义
// Step 5 P-4：执行整合 — IO 消息接收 + fdm 链接 → fdm 引擎
// 多重继承（库的虚接口实现约定）：
//   - ch_recv_cb<tcp_buf_ch>：消息接收回调
//   - aio_ch_op<tcp_buf_ch>  ：通道操作回调（连接成功/关闭等）
// R-8：跨模块引用用裸指针（datas_ 是 data_repo*）
//
// 线程：mthread_pool（每线程一个 epoll）+ 每线程一个 link_send_que（waker 挂 epoll）
//   - 发送线程 = 接收线程（onload 加速最优）
//
// 接口（Step 8 增量追加）：
//   S3: init / stop / start_connect / reconnect + 全部 override
//   S4: deal_msg 解析 g1 头 + msg_id 分发骨架
//   S7: set_gw_login_info / send_gw_login（log_type=2）+ deal_ch_connect 触发

#pragma once

#include "fdm_link.h"

#include "api_engine.h"

#include "aio_tcp.h"
#include "comm_aio.h"
#include "data_repo.h"

#include "g1msghead.h"
#include "link_send_que.h"
#include "mlog.h"
#include "mthread.h"

#include <cstdint>
#include <vector>

namespace lb_fgw {

/// fdm 端连接管理和操作引擎
class fdm_engine : public lb_common::ch_recv_cb<lb_common::tcp_buf_ch>,
                   public lb_common::aio_ch_op<lb_common::tcp_buf_ch> {
public:
  /// 初始化引擎（内部构造 send_ques[thread_num]；que_size 可配）
  int32_t init(data_repo *datas, int32_t heart_interval, int32_t thread_num, int32_t que_size, api_engine *apis,
               lb_common::lb_log *log, std::vector<int32_t> &cpus);

  int32_t start();

  /// 停止线程池（fdm_link 由 fdm_board 持有，其析构自动 close）
  void stop();

  ~fdm_engine() override {
    stop();
    delete[] send_ques_;
  }

  /// 对单个 fdm_link 发起连接（首次和重连统一入口）
  int32_t start_connect(fdm_link *link, fdm_board *fdm);

  /// 周期检查：心跳超时、心跳发送、断线重连（fgw_instance periodic_check 调用）
  void check_board_link();

  // ---- ch_recv_cb 纯虚 override ----
  lb_common::int32 deal_msg(lb_common::tcp_buf_ch *pch, lb_common::aio_msg &msg) override;

  void deal_send_error(fdm_link *link, char *pmsg, int32_t msg_len, int32_t err_ret);

  // ---- aio_ch_op override ----
  void deal_ch_connect(lb_common::tcp_buf_ch *pch, lb_common::csock_addr &localaddr) override;
  void deal_ch_closing(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) override;
  void deal_ch_closed(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) override;
  void deal_ch_error(lb_common::tcp_buf_ch *pch, lb_common::int32 err_type, lb_common::int32 err_code) override;
  void deal_recv_stop(lb_common::tcp_buf_ch *pch) override;

private:
  // 跨模块引用（R-8 裸指针，访问 data_repo）
  data_repo *datas_;
  // 发送队列数组（引擎拥有，内部构造/析构）
  link_send_que<fdm_link, fdm_engine> *send_ques_ = nullptr;
  int32_t thread_num_ = 2;
  int32_t check_interval_ = 5;       ///< 重连/心跳检查间隔（秒）
  lb_common::mthread_pool th_pool_;  ///< 线程池
  lb_common::lb_log *log_ = nullptr; ///< 日志
  api_engine *api_eng_ = nullptr;    ///< api_engine 引用（S9 登录应答回找 api_link）

  /// 业务下行（ORDER_RTN/TRADE_RTN/CANCEL_RSP）：(board,session) → api_link 透传
  void route_downstream(fdm_board *pfdm, g1_msg_head *head);

  void deal_login_ans(fdm_board *pfdm, g1_msg_head *head);

  void deal_user_offline(fdm_board *pfdm, g1_msg_head *head);

  /// 构造 aio_attr + channel_attr
  void build_aio_attr_(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr);
};

} // namespace lb_fgw

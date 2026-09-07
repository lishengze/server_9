// api_engine - api 端连接管理和操作引擎
//
// Step 5 已确认成员定义
// Step 5 P-4：执行整合 — IO 消息接收 + api 链接 + listen → api 引擎
// 多重继承（库的虚接口实现约定）：
//   - ch_recv_cb<tcp_buf_ch>：消息接收回调
//   - aio_ch_op<tcp_buf_ch>  ：通道操作回调（连接成功/关闭等）
//   - tcp_listen_op          ：listen 操作回调（接受新连接）
// R-8：跨模块引用用裸指针（datas_ 是 data_repo*）
//
// 线程：mthread_pool（每线程一个 epoll）+ 每线程一个 link_send_que（waker 挂 epoll）
//   - 发送线程 = 接收线程（onload 加速最优，链接收/发/关全在接收线程单线程）
//
// 接口（Step 8 增量追加）：
//   S2: init / start_listen / stop + 全部 override
//   S4: deal_msg 解析 g1 头 + msg_id 分发骨架
//   S8: 业务上行路由 + 登录上行中转
//   S10: 证券信息推送（secs_ 由 fgw_instance init 时注入，R-8 只读引用）
//
// api_link 生命周期：accept 时 new → deal_ch_closed 直接 delete
//   （deal_after_closed 后引用归零、无其他线程持有，回调栈上 delete 安全，
//     无需延迟清理队列——gone/api 常驻 link 的经验不适用于 fgw 一次性 link）

#pragma once

#include "aio_tcp.h"
#include "api_link.h"

#include "comm_aio.h"
#include "data_repo.h"
#include "g1msghead.h"
#include "link_send_que.h"
#include "mlock.h"
#include "mlog.h"
#include "mthread.h"
#include "tcp_ch.h"

#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace lb_fgw {

/// api 端连接管理和操作引擎
class api_engine : public lb_common::ch_recv_cb<lb_common::tcp_buf_ch>,
                   public lb_common::aio_ch_op<lb_common::tcp_buf_ch>,
                   public lb_common::tcp_listen_op {
public:
  /// 初始化引擎（内部构造 send_ques[thread_num]；que_size/heart_interval 可配）
  int32_t init(data_repo *datas, int32_t heart_interval, int32_t thread_num, int32_t que_size,
               lb_common::csock_addr &listen_addr, lb_common::lb_log *log, std::vector<int32_t> &cpus);

  /// 启动
  int32_t start();
  /// 停止：关闭监听 + 所有 api 链接 + 停止线程池
  void stop();

  ~api_engine() override {
    stop();
    delete[] send_ques_;
  }

  /// 心跳超时检查（fgw_instance periodic_check 调用，内部遍历 api_links_）
  void check_heartbeat();

  /// 通过 connect_id 查找 api_link（S9 登录应答回找，fdm_engine 调用）
  int32_t find_api_link(api_link *&o_link, int32_t connect_id);

  // ---- ch_recv_cb 纯虚 override ----
  lb_common::int32 deal_msg(lb_common::tcp_buf_ch *pch, lb_common::aio_msg &msg) override;

  void deal_send_error(api_link *link, char *pmsg, int32_t msg_len, int32_t err_ret);
  void send_gw_rej(api_link *link, g1_msg_head *req_head, int32_t err_code, const char *err_msg);
  void send_login_error(api_link *link, g1_msg_head *req_head, int32_t err_code, const char *err_msg);

  // ---- aio_ch_op override ----
  void deal_ch_connect(lb_common::tcp_buf_ch *pch, lb_common::csock_addr &localaddr) override;
  void deal_ch_closing(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) override;
  void deal_ch_closed(lb_common::tcp_buf_ch *pch, lb_common::int32 errcode) override;
  void deal_ch_error(lb_common::tcp_buf_ch *pch, lb_common::int32 err_type, lb_common::int32 err_code) override;
  void deal_recv_stop(lb_common::tcp_buf_ch *pch) override;

  // ---- tcp_listen_op override ----
  lb_common::int32 deal_listen_accept(lb_common::tcp_listen_ch *plistench, lb_common::csock_addr &clientaddr,
                                      lb_common::sock_fd fd) override;
  void deal_listen_error(lb_common::tcp_listen_ch *pch, lb_common::int32 errcode, lb_common::int32 reasonerr) override;
  void deal_listen_to_close(lb_common::tcp_listen_ch *pch, lb_common::int32 errcode) override;
  void deal_listen_closed(lb_common::tcp_listen_ch *pch, lb_common::int32 errcode) override;

private:
  // 跨模块只读引用（R-8 裸指针，访问 data_repo）
  data_repo *datas_ = nullptr;

  // 发送队列数组（引擎拥有，内部构造/析构）
  link_send_que<api_link, api_engine> *send_ques_ = nullptr;

  // 保护 api_links_ 的并发访问
  lb_common::cmutex map_lock_;
  // 生命周期管理：api_link 指针（accept 时 new，deal_ch_closed 直接 delete）
  std::unordered_map<uint32_t, api_link *> api_links_;

  lb_common::mthread_pool th_pool_;     ///< 线程池
  lb_common::tcp_listen_ch api_listen_; ///< 监听 socket
  lb_common::csock_addr listen_addr_;   ///< 监听地址
  lb_common::lb_log *log_ = nullptr;    ///< 日志
  std::atomic<int32_t> connect_id_;     ///< 自增生成链接号
  int32_t heart_interval_ = 5;          ///< 心跳间隔（秒，0=禁用）

  /// 构造 aio_attr + channel_attr（参考 gone/api build_aio_attr_）
  void build_aio_attr_(lb_common::aio_attr &buf_attr, lb_common::channel_attr &ch_attr);

  // S8 业务上行路由
  void route_upstream(api_link *link, g1_msg_head *head);
  void deal_login_req(api_link *link, g1_msg_head *head);
  void deal_sec_req(api_link *link, g1_msg_head *req_head);
  void send_heart_ans(api_link *link);
};

} // namespace lb_fgw

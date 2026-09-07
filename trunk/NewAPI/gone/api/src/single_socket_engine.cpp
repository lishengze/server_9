// single_socket_engine - 单 socket 极速引擎实现
//
// 与 tcpdirect_engine 结构对称, 但使用 aio_socket_link (基于 aio_tcp / 内核 socket)
// 仅在 socket_single 模式下使用 (C1/C3 配置)
//
// 主循环: 消费 send_queue_ 事件, 分发到 link_ / counter_
// 链接层: aio_socket_link 异步 IO, 由 link_timer_op 驱动心跳 / 重连
// 异步路径: 收到消息 → link.deal_recv → msg_cb → counter.deal_recv_msg

#include "single_socket_engine.h"
#include "aio_socket_link.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "comm_sock.h"
#include "comm_sys.h"
#include "g1trademsg.h"
#include "link_timer_op.h"
#include "mlog.h"
#include "que_mth_buf.h"

// 显式实例化需要的完整类型
#include "fpga_counter_direct.h"
#include "gw_counter_direct.h"
#include <cstring>

namespace lb_api {

template <class TFastCounter>
int32 single_socket_engine<TFastCounter>::init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log) {
  send_poll_num_ = cfg.get_send_poll_num();
  recv_poll_num_ = cfg.get_recv_poll_num();
  counter_ = tfst;
  log_ = log;
  link_outop_.owner_ = this;

  lb_common::lb_log_hand tlh(log);

  int32 heart_interval = cfg.get_heartbeat_interval();
  int32 max_fails = cfg.get_max_reconnect_count();
  int32 check_interval = heart_interval / 2;
  check_interval = check_interval == 0 ? 1 : check_interval;
  int32 tcpuid = cfg.get_speed_engine_cpu();

  int64 tq_size = cfg.get_send_queue_size_mb();
  if (tq_size < 2)
    tq_size = 2;
  tq_size = tq_size * 1024 * 1024;

  int32 ret = send_queue_.init(tq_size, 8);
  if (ret < 0) {
    error_log(tlh) << "init single socket engine queue error,que_size=" << tq_size << ",ret=" << ret << end_log;
    return LBAPI_ERR_QUEUE_INIT;
  }

  int32 once_recv_len = sizeof(trade_rtn) * 4;
  ret = link_.init(log, tfst, LINK_TYPE_SPEED_TRADE, once_recv_len, check_interval, max_fails);
  if (ret < 0) {
    return ret;
  }

  // 设置主地址 (aio_socket_link 支持最多两个地址: 主 + 备)
  net_addr tcfg_addr = cfg.get_speed_counter_addr();
  lb_common::csock_addr taddr;
  if (tcfg_addr.ip[0] != '\0' && tcfg_addr.port != 0) {
    taddr.port = tcfg_addr.port;
    std::memcpy(taddr.ip, tcfg_addr.ip, sizeof(taddr.ip));
    link_.set_remote(taddr);
  }

  // 设置备地址 (若已配置, 故障切换由 aio_socket_link 内部 check_reconnect 决策)
  net_addr tcfg_addr_bak = cfg.get_speed_counter_addr_bak();
  if (tcfg_addr_bak.ip[0] != '\0' && tcfg_addr_bak.port != 0) {
    lb_common::csock_addr tbak;
    tbak.port = tcfg_addr_bak.port;
    std::memcpy(tbak.ip, tcfg_addr_bak.ip, sizeof(tbak.ip));
    link_.set_remote(tbak);
  }

  ret = timer_op_.init_timer(&link_, this, check_interval);
  if (ret < 0) {
    error_log(tlh) << "init single socket engine timer error,interval=" << check_interval << ",ret=" << ret << end_log;
    return LBAPI_ERR_TIMER_INIT;
  }

  ret = init_th(0, tcpuid, 0);
  if (ret < 0) {
    error_log(tlh) << "init single socket engine thread error,cpuid=" << tcpuid << ",ret=" << ret << end_log;
    return LBAPI_ERR_THREAD_INIT;
  }
  info_log(tlh) << "init single socket engine ok,cpuid=" << tcpuid << ",timer_interval=" << heart_interval
                << ",que_size=" << tq_size << ",once_recv_len=" << once_recv_len << end_log;
  return 0;
}

template <class TFastCounter> int32 single_socket_engine<TFastCounter>::start() {

  lb_common::lb_log_hand tlh(log_);

  int32 ret = run();
  if (ret < 0) {
    error_log(tlh) << "start single socket engine thread error,ret=" << ret << end_log;
    return LBAPI_ERR_THRAD_START;
  }
  info_log(tlh) << "start single socket engine ok" << end_log;
  return 0;
}

template <class TFastCounter> void single_socket_engine<TFastCounter>::stop() {
  timer_op_.close();
  join();
  send_queue_.close();
  link_.close_ch();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "stop single socket engine ok" << end_log;
}

template <class TFastCounter> void single_socket_engine<TFastCounter>::do_work() {

  for (int32 i = 0; i < send_poll_num_; ++i) {
    char *raw = nullptr;
    int64 pos = send_queue_.read_get(raw);
    if (pos <= 0) {
      break;
    }
    const link_send_event *evt = reinterpret_cast<const link_send_event *>(raw);
    int32 evt_len = sizeof(link_send_event) + evt->data_len;
    int32 ret = 0;

    switch (evt->type) {
    case LINK_EVENT_TYPE_SEND_MSG: {
      ret = link_.send_msg(const_cast<char *>(evt->data), evt->data_len);
      if (unlikely(ret < 0)) {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "single socket link send msg error,ret=" << ret << end_log;
        counter_->deal_send_error(const_cast<char *>(evt->data), evt->data_len, evt->link_type, ret);
      }
      break;
    }
    case LINK_EVENT_TYPE_SEND_HEART: {
      char heart_buf[256];
      int32 heart_len = counter_->build_heart_msg(heart_buf, sizeof(heart_buf));
      if (heart_len > 0) {
        link_.send_msg(heart_buf, heart_len);
      }
      break;
    }
    case LINK_EVENT_TYPE_LINK_CLOSE: {
      const link_close_event_info *tcl = reinterpret_cast<const link_close_event_info *>(evt->data);
      link_.close_ch(tcl->err_code);
      break;
    }
    case LINK_EVENT_TYPE_LINK_CONNECT: {
      const link_connect_event_info *pcon = reinterpret_cast<const link_connect_event_info *>(evt->data);
      if (counter_->can_link_connect(evt->link_type))
        link_.connect(recv_poll_num_, pcon->need_switch, NULL);
      break;
    }
    case LINK_EVENT_TYPE_ACCOUNT_LOGIN: {
      ///< 账户登陆请求(当前只能是个微)
      const acc_login_event_info *pmlog = reinterpret_cast<const acc_login_event_info *>(evt->data);
      deal_cust_login(*pmlog);
      break;
    }
    case LINK_EVENT_TYPE_FPGA_CORE_CONNECT: {
      // fpga core 链接由 fpga_counter 同步处理
      const fpga_core_connect_info *pmlog = reinterpret_cast<const fpga_core_connect_info *>(evt->data);
      deal_fpga_core_connect(*pmlog);
      break;
    }
    //case LINK_EVENT_TYPE_AGWUSER_LOGIN: {
    ///< agw用户登陆请求
    //  break;
    //}
    default:
      break;
    }
    send_queue_.read_cmt(evt_len);
  }

  link_.deal_recv();
}

template <class TFastCounter>
void single_socket_engine<TFastCounter>::deal_cust_login(const acc_login_event_info &pmlog) {
  lb_common::lb_log_hand tlh(log_);
  char log_buf[8192];

  // 只可能是个微柜台的登陆
  if (counter_->get_counter_type() != static_cast<int32_t>(counter_type::gw_direct)) {
    error_log(tlh) << "fast socket link only support gw counter user login,branch_id=" << pmlog.branch_id
                   << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                   << ",client_req_no=" << pmlog.cust_req_no << end_log;
    return;
  }

  int32 ret = 0;
  if (link_.is_free()) {
    // 个微柜台直连核心，不支持切换
    ret = link_.connect(recv_poll_num_, 0, NULL);
    if (ret < 0) {
      error_log(tlh) << "fast socket link connect to login error,branch_id=" << pmlog.branch_id
                     << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                     << ",client_req_no=" << pmlog.cust_req_no << ",ret=" << ret << end_log;
      counter_->ans_cust_login(pmlog, LBAPI_ERR_LINK_CONNECT, "connect fast counter error");
      return;
    }
  }

  int32 tlen = counter_->deal_cust_login(pmlog, log_buf, sizeof(log_buf));
  if (tlen < 0) {
    error_log(tlh) << "build fast counter login msg error,ret=" << tlen << end_log;
    counter_->ans_cust_login(pmlog, LBAPI_ERR_BUILD_MSG, "build fast counter login msg error");
    return;
  } else if (tlen == 0) {
    info_log(tlh) << "fast counter cust have login,branch_id=" << pmlog.branch_id
                  << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                  << ",client_req_no=" << pmlog.cust_req_no << end_log;
    return;
  }

  ret = link_.send_msg(log_buf, tlen);
  if (ret > 0) {
    info_log(tlh) << "fast socket link send cust login ok,branch_id=" << pmlog.branch_id
                  << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                  << ",client_req_no=" << pmlog.cust_req_no << ",msglen=" << tlen << end_log;
  } else {
    error_log(tlh) << "fast socket link send cust login error,branch_id=" << pmlog.branch_id
                   << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                   << ",client_req_no=" << pmlog.cust_req_no << ",ret=" << ret << end_log;
    counter_->ans_cust_login(pmlog, LBAPI_ERR_SEND_MSG, "send fast counter login msg error");
  }
}

template <class TFastCounter>
void single_socket_engine<TFastCounter>::deal_fpga_core_connect(const fpga_core_connect_info &pmlog) {
  lb_common::lb_log_hand tlh(log_);

  lb_common::csock_addr taddr;
  taddr.port = pmlog.trade_port;
  std::memcpy(taddr.ip, pmlog.trade_ip, sizeof(taddr.ip));
  link_.set_remote(taddr);

  int32 ret = link_.connect(recv_poll_num_, 0, NULL);
  if (ret == 0) {
    info_log(tlh) << "cust login to connect fast counter ok,trade_ip=" << pmlog.trade_ip
                  << ",trade_port=" << pmlog.trade_port << end_log;
  } else {
    error_log(tlh) << "cust login to connect fast counter error,trade_ip=" << pmlog.trade_ip
                   << ",trade_port=" << pmlog.trade_port << ",ret=" << ret << end_log;
  }
}

template <class TFastCounter> int32 single_socket_engine<TFastCounter>::add_timer_poll(lb_common::mthread *th) {
  lb_common::lb_log_hand tlh(log_);
  int32 ret = timer_op_.add_timer_poll(th);
  if (ret < 0) {
    error_log(tlh) << "fast socket link add timer error,ret=" << ret << end_log;
    return LBAPI_ERR_ADD_TIMER;
  }
  info_log(tlh) << "fast socket link add timer ok" << end_log;
  return 0;
}

template <class TFastCounter>
void single_socket_engine<TFastCounter>::eng_link_op::deal_heart_msg_ans(int16 link_type) {
  owner_->link_.deal_heart_ans();
}
template <class TFastCounter>
void single_socket_engine<TFastCounter>::eng_link_op::deal_close_link(int16 link_type, int32 err_code) {
  owner_->link_.close_ch(err_code);
}

// 显式实例化
template class aio_socket_link<gw_counter_direct>;
template class aio_socket_link<fpga_counter_direct>;
//template class aio_socket_link<fpga_counter_gateway>;

template class single_socket_engine<gw_counter_direct>;
template class single_socket_engine<fpga_counter_direct>;
//template class single_socket_engine<fpga_counter_gateway>;

template class link_timer_op<aio_socket_link<gw_counter_direct>, single_socket_engine<gw_counter_direct>>;
template class link_timer_op<aio_socket_link<fpga_counter_direct>, single_socket_engine<fpga_counter_direct>>;
//template class link_timer_op<aio_socket_link<fpga_counter_gateway>, single_socket_engine<fpga_counter_gateway>>;

} // namespace lb_api

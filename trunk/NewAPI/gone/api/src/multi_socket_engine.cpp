// multi_socket_engine - 多 socket 引擎实现 (2 槽)
//
// 槽 0: g98_link_         - 98 柜台链接 (恒存在)
// 槽 1: fast_gw_link_     - 极速柜台网关链接
//                            · TFastCounter=gw_direct  → 闲置 (D13)
//                            · TFastCounter=fpga_direct → fpga GW
//                            · TFastCounter=fpga_gateway → fpga GW (业务也走)
//
// 本类自身是 simple_thread, 同时也是 fast_engine 的 timerfd 注册目标 (即控制平面 epoll)。

#include "multi_socket_engine.h"
#include "aio_socket_link.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "counter98.h"
#include "link_timer_op.h"
#include "mlog.h"
#include "que_mth_buf.h"

// 显式实例化需要完整类型
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "gw_counter_direct.h"

namespace lb_api {

template <class TFastCounter>
int32 multi_socket_engine<TFastCounter>::init(const api_config_impl &cfg, TFastCounter *tfst, lb_common::lb_log *log,
                                              counter98 *c98) {

  send_poll_num_ = cfg.get_send_poll_num();
  recv_poll_num_ = cfg.get_recv_poll_num();
  queue_dealing_ = 0;
  counter_ = tfst;
  counter98_ = c98;
  log_ = log;
  set_event(0, 0, 1);
  link_outop_.owner_ = this;

  lb_common::lb_log_hand tlh(log);

  int32 heart_interval = cfg.get_heartbeat_interval();
  int32 max_fails = cfg.get_max_reconnect_count();
  int32 check_interval = heart_interval / 2;
  check_interval = check_interval == 0 ? 1 : check_interval;
  int32 tcpuid = cfg.get_mgmt_engine_cpu();
  int32 twaitms = cfg.get_multi_io_wait_ms();

  int64 tq_size = cfg.get_send_queue_size_mb();
  if (tq_size < 2)
    tq_size = 2;
  tq_size = tq_size * 1024 * 1024;

  int32 ret = send_queue_.init(tq_size, 8);
  fprintf(stderr, "[DBG] multi_engine.init: send_queue ret=%d\n", ret);
  if (ret < 0) {
    error_log(tlh) << "init multi socket engine queue error,que_size=" << tq_size << ",ret=" << ret << end_log;
    return LBAPI_ERR_QUEUE_INIT;
  }
  ret = queue_wake_.init(0);
  fprintf(stderr, "[DBG] multi_engine.init: queue_wake ret=%d\n", ret);
  if (ret < 0) {
    error_log(tlh) << "init multi socket engine queue wake error,ret=" << ret << end_log;
    return LBAPI_ERR_ATTR_INIT;
  }

  int32 once_recv_len = 2048;
  ret = g98_link_.init(log, counter98_, LINK_TYPE_98, once_recv_len, check_interval, max_fails);
  fprintf(stderr, "[DBG] multi_engine.init: g98_link.init ret=%d\n", ret);
  if (ret < 0) {
    return ret;
  }

  net_addr tcfg_addr = cfg.get_counter98_addr();
  lb_common::csock_addr taddr;
  if (tcfg_addr.port > 0 && tcfg_addr.ip[0] != '\0') {
    taddr.port = tcfg_addr.port;
    std::memcpy(taddr.ip, tcfg_addr.ip, sizeof(taddr.ip));
    g98_link_.set_remote(taddr);
    fprintf(stderr, "[DBG] multi_engine.init: set_remote 98 addr=%s:%d\n", taddr.ip, (int)taddr.port);
  }
  tcfg_addr = cfg.get_counter98_addr_bak();
  if (tcfg_addr.port > 0 && tcfg_addr.ip[0] != '\0') {
    taddr.port = tcfg_addr.port;
    std::memcpy(taddr.ip, tcfg_addr.ip, sizeof(taddr.ip));
    g98_link_.set_remote(taddr);
  }

  ret = g98_link_timer_.init_timer(&g98_link_, this, check_interval);
  fprintf(stderr, "[DBG] multi_engine.init: g98_timer.init ret=%d\n", ret);
  if (ret < 0) {
    error_log(tlh) << "init multi socket engine 98 timer error,interval=" << check_interval << ",ret=" << ret
                   << end_log;
    return ret;
  }

  /// 槽 1: 极速柜台网关链接（fast_gw_link）
  /// 语义：fpga_direct 模式 = fpga GW；fpga_gateway 模式 = fpga GW（业务也走此）；个微模式 = 闲置
  counter_type t_fast_counter = cfg.get_fast_counter_type();
  if (t_fast_counter == counter_type::fpga_direct || t_fast_counter == counter_type::fpga_gateway) {
    once_recv_len = 2048;
    ret = fast_gw_link_.init(log, counter_, LINK_TYPE_SPEED_GW, once_recv_len, check_interval, max_fails);
    if (ret < 0) {
      return ret;
    }

    tcfg_addr = cfg.get_speed_counter_addr();
    if (tcfg_addr.port > 0 && tcfg_addr.ip[0] != '\0') {
      taddr.port = tcfg_addr.port;
      std::memcpy(taddr.ip, tcfg_addr.ip, sizeof(taddr.ip));
      fast_gw_link_.set_remote(taddr);
    }
    tcfg_addr = cfg.get_speed_counter_addr_bak();
    if (tcfg_addr.port > 0 && tcfg_addr.ip[0] != '\0') {
      taddr.port = tcfg_addr.port;
      std::memcpy(taddr.ip, tcfg_addr.ip, sizeof(taddr.ip));
      fast_gw_link_.set_remote(taddr);
    }
    ret = fast_gw_link_timer_.init_timer(&fast_gw_link_, this, check_interval);
    if (ret < 0) {
      error_log(tlh) << "init multi socket engine fast gateway timer error,interval=" << check_interval
                     << ",ret=" << ret << end_log;
      return ret;
    }
  }

  /// 槽 2: 极速柜台 Core 链接（fast_core_link，仅 fpga_direct 模式）
  /// 语义：fpga_direct 模式 = fpga Core（委托/撤单/心跳）；fpga_gateway 模式 = 闲置
  if (t_fast_counter == counter_type::fpga_direct) {
    once_recv_len = sizeof(trade_rtn) * 4;
    ret = fast_core_link_.init(log, counter_, LINK_TYPE_SPEED_TRADE, once_recv_len, check_interval, max_fails);
    if (ret < 0) {
      return ret;
    }
    ret = fast_core_link_timer_.init_timer(&fast_core_link_, this, check_interval);
    if (ret < 0) {
      error_log(tlh) << "init multi socket engine fast core timer error,interval=" << check_interval
                     << ",ret=" << ret << end_log;
      return ret;
    }
  }

  ret = epoll_th_.init_th(0, 16, 0, 0, twaitms);
  fprintf(stderr, "[DBG] multi_engine.init: epoll_th.init ret=%d\n", ret);
  if (ret < 0) {
    error_log(tlh) << "init multi socket engine thread error,cpuid=" << tcpuid << ",waitms=" << twaitms
                   << ",ret=" << ret << end_log;
    return LBAPI_ERR_THREAD_INIT;
  }
  info_log(tlh) << "init multi socket engine ok,cpuid=" << tcpuid << ",waitms=" << twaitms
                << ",timer_interval=" << heart_interval << ",que_size=" << tq_size << ",once_recv_len=" << once_recv_len
                << end_log;
  fprintf(stderr, "[DBG] multi_engine.init: about to return 0\n");

  return 0;
}

template <class TFastCounter> int32 multi_socket_engine<TFastCounter>::connect_98agw() {
  lb_common::lb_log_hand tlh(log_);

  if (g98_link_.is_work()) {
    info_log(tlh) << "98 agw link have connected" << end_log;
    return 0;
  } else if (g98_link_.is_free()) {
    int32 ret = g98_link_.connect(recv_poll_num_, 0, &epoll_th_);
    if (ret == 0) {
      info_log(tlh) << "connect 98 agw ok" << end_log;
      return 0;
    } else {
      error_log(tlh) << "connect 98 agw error,ret=" << ret << end_log;
      return ret;
    }
  } else {
    error_log(tlh) << "98 agw link state error" << end_log;
    return LBAPI_ERR_STATE_LIMITED;
  }
}

template <class TFastCounter> int32 multi_socket_engine<TFastCounter>::start() {
  lb_common::lb_log_hand tlh(log_);

  int32 ret = epoll_th_.add_poll_event(*this);
  if (ret < 0) {
    error_log(tlh) << "add queue wake to thread epoll error,ret=" << ret << end_log;
    return LBAPI_ERR_ADD_WAKEFD;
  }

  ret = g98_link_timer_.add_timer_poll(&epoll_th_);
  if (ret < 0) {
    error_log(tlh) << "add 98 timer to thread epoll error,ret=" << ret << end_log;
    return LBAPI_ERR_ADD_TIMER;
  }

  int32 t_fast_counter = counter_->get_counter_type();
  if (t_fast_counter == static_cast<int32_t>(counter_type::fpga_direct) ||
      t_fast_counter == static_cast<int32_t>(counter_type::fpga_gateway)) {
    ret = fast_gw_link_timer_.add_timer_poll(&epoll_th_);
    if (ret < 0) {
      error_log(tlh) << "add fast gateway timer to thread epoll error,ret=" << ret << end_log;
      return LBAPI_ERR_ADD_TIMER;
    }
  }

  if (t_fast_counter == static_cast<int32_t>(counter_type::fpga_direct)) {
    ret = fast_core_link_timer_.add_timer_poll(&epoll_th_);
    if (ret < 0) {
      error_log(tlh) << "add fast core timer to thread epoll error,ret=" << ret << end_log;
      return LBAPI_ERR_ADD_TIMER;
    }
  }

  ret = epoll_th_.run();
  if (ret < 0) {
    error_log(tlh) << "start multi socket engine thread error,ret=" << ret << end_log;
    return LBAPI_ERR_THRAD_START;
  }
  queue_wake_.wake();
  info_log(tlh) << "start multi socket engine ok" << end_log;
  return 0;
}

template <class TFastCounter> void multi_socket_engine<TFastCounter>::stop() {
  // 幂等保护：api_impl::stop() 会调用本方法，析构函数 ~multi_socket_engine 也会调用，
  // 若重复执行会 double-close 资源并在 log_ 已 close 后再次写日志导致崩溃。
  if (stopped_) {
    return;
  }
  stopped_ = 1;

  g98_link_.close_ch();
  fast_gw_link_.close_ch();
  fast_core_link_.close_ch();
  epoll_th_.join();
  g98_link_timer_.close();
  fast_gw_link_timer_.close();
  fast_core_link_timer_.close();
  send_queue_.close();

  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "stop multi socket engine ok" << end_log;
}

// 消费 send_queue_
template <class TFastCounter> void multi_socket_engine<TFastCounter>::deal_event() {

  queue_dealing_ = 1;
  queue_wake_.reset();

  for (int32 i = 0; i < send_poll_num_; ++i) {
    char *raw = nullptr;
    int64 pos = send_queue_.read_get(raw);
    if (pos <= 0) {
      queue_dealing_ = 0;
      return;
    }
    const link_send_event *evt = reinterpret_cast<const link_send_event *>(raw);
    int32 evt_len = sizeof(link_send_event) + evt->data_len;
    int32 ret = 0;

    switch (evt->type) {
    case LINK_EVENT_TYPE_SEND_MSG: {
      if (evt->link_type == LINK_TYPE_SPEED_GW) {
        ret = fast_gw_link_.send_msg(const_cast<char *>(evt->data), evt->data_len);
        if (unlikely(ret < 0)) {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fast gateway link send msg error,ret=" << ret << end_log;
          counter_->deal_send_error(const_cast<char *>(evt->data), evt->data_len, evt->link_type, ret);
        }
        if (evt->leave_time_ptr) {
          __atomic_store_n(evt->leave_time_ptr, perf_now_ns(), __ATOMIC_RELEASE);
        }
      } else if (evt->link_type == LINK_TYPE_SPEED_TRADE) {
        ret = fast_core_link_.send_msg(const_cast<char *>(evt->data), evt->data_len);
        if (unlikely(ret < 0)) {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fast core link send msg error,ret=" << ret << end_log;
          counter_->deal_send_error(const_cast<char *>(evt->data), evt->data_len, evt->link_type, ret);
        }
        if (evt->leave_time_ptr) {
          __atomic_store_n(evt->leave_time_ptr, perf_now_ns(), __ATOMIC_RELEASE);
        }
      } else {
        ret = g98_link_.send_msg(const_cast<char *>(evt->data), evt->data_len);
        if (unlikely(ret < 0)) {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "98 link send msg error,ret=" << ret << end_log;
          counter98_->deal_send_error(const_cast<char *>(evt->data), evt->data_len, evt->link_type, ret);
        }
        if (evt->leave_time_ptr) {
          __atomic_store_n(evt->leave_time_ptr, perf_now_ns(), __ATOMIC_RELEASE);
        }
      }
      break;
    }
    case LINK_EVENT_TYPE_SEND_HEART: {
      char heart_buf[256];
      int32 heart_len = 0;
      if (evt->link_type == LINK_TYPE_SPEED_GW) {
        heart_len = counter_->build_heart_msg(heart_buf, sizeof(heart_buf));
        if (heart_len > 0) {
          fast_gw_link_.send_msg(heart_buf, heart_len);
        }
      } else if (evt->link_type == LINK_TYPE_SPEED_TRADE) {
        heart_len = counter_->build_heart_msg(heart_buf, sizeof(heart_buf));
        if (heart_len > 0) {
          fast_core_link_.send_msg(heart_buf, heart_len);
        }
      } else {
        heart_len = counter98_->build_heart_msg(heart_buf, sizeof(heart_buf));
        if (heart_len > 0) {
          g98_link_.send_msg(heart_buf, heart_len);
        }
      }
      break;
    }
    case LINK_EVENT_TYPE_LINK_CLOSE: {
      const link_close_event_info *tcl = reinterpret_cast<const link_close_event_info *>(evt->data);
      if (evt->link_type == LINK_TYPE_SPEED_GW) {
        fast_gw_link_.close_ch(tcl->err_code);
      } else if (evt->link_type == LINK_TYPE_SPEED_TRADE) {
        fast_core_link_.close_ch(tcl->err_code);
      } else {
        g98_link_.close_ch(tcl->err_code);
      }
      break;
    }
    case LINK_EVENT_TYPE_LINK_CONNECT: {
      const link_connect_event_info *pcon = reinterpret_cast<const link_connect_event_info *>(evt->data);
      if (evt->link_type == LINK_TYPE_SPEED_GW) {
        if (counter_->can_link_connect(evt->link_type))
          fast_gw_link_.connect(recv_poll_num_, pcon->need_switch, &epoll_th_);
      } else if (evt->link_type == LINK_TYPE_SPEED_TRADE) {
        if (counter_->can_link_connect(evt->link_type))
          fast_core_link_.connect(recv_poll_num_, pcon->need_switch, &epoll_th_);
      } else {
        if (counter98_->can_link_connect(evt->link_type))
          g98_link_.connect(recv_poll_num_, pcon->need_switch, &epoll_th_);
      }
      break;
    }
    case LINK_EVENT_TYPE_ACCOUNT_LOGIN: {
      const acc_login_event_info *pmlog = reinterpret_cast<const acc_login_event_info *>(evt->data);
      if (evt->link_type != LINK_TYPE_98) {
        deal_cust_login(*pmlog);
      } else {
        deal_cust98_login(*pmlog);
      }
      break;
    }
    case LINK_EVENT_TYPE_FPGA_CORE_CONNECT: {
      const fpga_core_connect_info *pcon =
          reinterpret_cast<const fpga_core_connect_info *>(evt->data);
      deal_fpga_core_connect(*pcon);
      break;
    }
    case LINK_EVENT_TYPE_AGWUSER_LOGIN: {
      // agw 用户登录: c98.build_agw_login_msg
      deal_agw98_login();
      break;
    }
    default:
      break;
    }
    send_queue_.read_cmt(evt_len);
  }

  queue_wake_.wake();
}

template <class TFastCounter>
void multi_socket_engine<TFastCounter>::deal_cust_login(const acc_login_event_info &pmlog) {
  lb_common::lb_log_hand tlh(log_);
  char log_buf[8192];

  // 极速柜台只可能是fpga柜台的登陆
  if (counter_->get_counter_type() != static_cast<int32_t>(counter_type::fpga_direct) &&
      counter_->get_counter_type() != static_cast<int32_t>(counter_type::fpga_gateway)) {
    error_log(tlh) << "fast gateway link only support fpga counter user login,branch_id=" << pmlog.branch_id
                   << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                   << ",client_req_no=" << pmlog.cust_req_no << end_log;
    return;
  }

  int32 ret = 0;
  if (fast_gw_link_.is_free()) {
    ret = fast_gw_link_.connect(recv_poll_num_, 0, &epoll_th_);
    if (ret < 0) {
      error_log(tlh) << "fast gateway link connect to login error,branch_id=" << pmlog.branch_id
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

  ret = fast_gw_link_.send_msg(log_buf, tlen);
  if (ret > 0) {
    info_log(tlh) << "fast gateway link send cust login ok,branch_id=" << pmlog.branch_id
                  << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                  << ",client_req_no=" << pmlog.cust_req_no << ",msglen=" << tlen << end_log;

  } else {
    error_log(tlh) << "fast gateway link send cust login error,branch_id=" << pmlog.branch_id
                   << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                   << ",client_req_no=" << pmlog.cust_req_no << ",ret=" << ret << end_log;
    counter_->ans_cust_login(pmlog, LBAPI_ERR_SEND_MSG, "send fast login msg error");
  }
}

// fpga core 链接建立 (仅 fpga_direct 模式): 从 login_ans 提取 trade_ip/trade_port 后连接
template <class TFastCounter>
void multi_socket_engine<TFastCounter>::deal_fpga_core_connect(const fpga_core_connect_info &pmlog) {
  lb_common::lb_log_hand tlh(log_);

  if (counter_->get_counter_type() != static_cast<int32_t>(counter_type::fpga_direct)) {
    error_log(tlh) << "fpga core connect only support fpga_direct counter" << end_log;
    return;
  }

  if (fast_core_link_.is_work()) {
    info_log(tlh) << "fpga core link have connected" << end_log;
    return;
  }

  lb_common::csock_addr taddr;
  std::memset(&taddr, 0, sizeof(taddr));
  taddr.port = pmlog.trade_port;
  std::memcpy(taddr.ip, pmlog.trade_ip, sizeof(taddr.ip));
  fast_core_link_.set_remote(taddr);

  int32 ret = fast_core_link_.connect(recv_poll_num_, 0, &epoll_th_);
  if (ret == 0) {
    info_log(tlh) << "cust login to connect fpga core ok,trade_ip=" << pmlog.trade_ip
                  << ",trade_port=" << pmlog.trade_port << end_log;
  } else {
    error_log(tlh) << "cust login to connect fpga core error,trade_ip=" << pmlog.trade_ip
                   << ",trade_port=" << pmlog.trade_port << ",ret=" << ret << end_log;
  }
}
template <class TFastCounter>
void multi_socket_engine<TFastCounter>::deal_cust98_login(const acc_login_event_info &pmlog) {
  lb_common::lb_log_hand tlh(log_);
  char log_buf[8192];

  int32 tlen = counter98_->deal_cust_login(pmlog, log_buf, sizeof(log_buf));
  if (tlen < 0) {
    error_log(tlh) << "build 98 counter cust login msg error,ret=" << tlen << end_log;
    counter98_->ans_cust_login(pmlog, LBAPI_ERR_BUILD_MSG, "build 98 cust login msg error");
    return;
  } else if (tlen == 0) {
    info_log(tlh) << "98 counter cust have login,branch_id=" << pmlog.branch_id
                  << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                  << ",client_req_no=" << pmlog.cust_req_no << end_log;
    return;
  }
  int32 ret = g98_link_.send_msg(log_buf, tlen);
  if (ret > 0) {
    info_log(tlh) << "98 link send cust login ok,branch_id=" << pmlog.branch_id
                  << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                  << ",client_req_no=" << pmlog.cust_req_no << ",msglen=" << tlen << end_log;
  } else {
    error_log(tlh) << "98 link send cust login error,branch_id=" << pmlog.branch_id
                   << ",fund_account=" << pmlog.fund_account_id << ",session=" << pmlog.session
                   << ",client_req_no=" << pmlog.cust_req_no << ",ret=" << ret << end_log;
    counter98_->ans_cust_login(pmlog, LBAPI_ERR_SEND_MSG, "send 98 cust login msg failed ");
  }
}
template <class TFastCounter> void multi_socket_engine<TFastCounter>::deal_agw98_login() {
  lb_common::lb_log_hand tlh(log_);

  char log_buf[8192];
  int32 tlen = counter98_->build_agw_login_msg(log_buf, sizeof(log_buf));
  if (tlen < 0) {
    error_log(tlh) << "build agw user login msg error,ret=" << tlen << end_log;
    counter98_->ans_agwuser_login(LBAPI_ERR_BUILD_MSG, NULL);
    return;
  }
  int32 ret = g98_link_.send_msg(log_buf, tlen);
  if (ret > 0) {
    info_log(tlh) << "98 link send agwuser login ok,msglen=" << tlen << end_log;
  } else {
    error_log(tlh) << "98 link send agwuser login error,msglen=" << tlen << ",ret=" << ret << end_log;
    counter98_->ans_agwuser_login(LBAPI_ERR_SEND_MSG, "send agwuser login msg error");
  }
}

template <class TFastCounter> void multi_socket_engine<TFastCounter>::eng_link_op::deal_heart_msg_ans(int16 link_type) {
  if (link_type == LINK_TYPE_SPEED_GW) {
    owner_->fast_gw_link_.deal_heart_ans();
  } else if (link_type == LINK_TYPE_SPEED_TRADE) {
    owner_->fast_core_link_.deal_heart_ans();
  } else {
    owner_->g98_link_.deal_heart_ans();
  }
}

template <class TFastCounter> void multi_socket_engine<TFastCounter>::eng_link_op::trigger_send() {
  if (owner_->queue_dealing_ == 0)
    owner_->queue_wake_.wake();
}
template <class TFastCounter>
void multi_socket_engine<TFastCounter>::eng_link_op::deal_close_link(int16 link_type, int32 err_code) {
  if (link_type == LINK_TYPE_SPEED_GW) {
    owner_->fast_gw_link_.close_ch(err_code);
  } else if (link_type == LINK_TYPE_SPEED_TRADE) {
    owner_->fast_core_link_.close_ch(err_code);
  } else {
    owner_->g98_link_.close_ch(err_code);
  }
}

// 显式实例化
template class aio_socket_link<fpga_counter_gateway>;
template class aio_socket_link<counter98>;

template class multi_socket_engine<gw_counter_direct>;
template class multi_socket_engine<fpga_counter_direct>;
template class multi_socket_engine<fpga_counter_gateway>;

template class link_timer_op<aio_socket_link<counter98>, multi_socket_engine<gw_counter_direct>>;
template class link_timer_op<aio_socket_link<counter98>, multi_socket_engine<fpga_counter_direct>>;
template class link_timer_op<aio_socket_link<counter98>, multi_socket_engine<fpga_counter_gateway>>;

template class link_timer_op<aio_socket_link<fpga_counter_direct>, multi_socket_engine<fpga_counter_direct>>;
template class link_timer_op<aio_socket_link<fpga_counter_gateway>, multi_socket_engine<fpga_counter_gateway>>;
//template class link_timer_op<aio_socket_link<gw_counter_direct>, multi_socket_engine<gw_counter_direct>>;

} // namespace lb_api

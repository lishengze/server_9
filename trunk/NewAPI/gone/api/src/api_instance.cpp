// api_instance - API 实例模板类实现 (编排所有子模块 + 工厂)
//
// api_impl<TFastCounter, TEngine> 继承 api_interface, 按配置选型后实例化.
// 唯一按配置选型的实体, 持有所有子模块.
// 业务流: 用户 → api_impl → fast_/c98_ → 队列 → 引擎 → 链接 → 柜台回调

#include "api_instance.h"
#include "aio_socket_link.h"
#include "api_callback.h"
#include "api_config.h"
#include "api_errno.h"
#include "callback_manager.h"
#include "counter98.h"
#include "fpga_counter_base.h"
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "gw_counter_direct.h"
#include "idle_engine.h"
#include "link_timer_op.h"
#include "matomic.h"
#include "mlog.h"
#include "multi_socket_engine.h"
#include "mutils.h"
#include "single_socket_engine.h"
#include "tcpdir_link.h"
#include "tcpdirect_engine.h"

#include <cstring>
#include <stdatomic.h>

namespace lb_api {

// ---- 工厂函数: create_instance ----
//
// 由于 api_impl 是模板类, 工厂函数用 if-else 链按 5 种配置分别 new
// 然后调用各模板实例的 init. 注意: 这里不能用 dynamic_cast, 因为模板
// 实例化是编译期的. 我们直接按 5 种配置分别调用, 失败时 delete.

int32 api_interface::create_instance(api_interface *&o_api, api_config &config, api_callback *cb) {
  if (cb == nullptr) {
    return LBAPI_ERR_INVALID_PARAM;
  }
  // 1. 校验配置
  int32 ret = config.validate();
  if (ret != LBAPI_OK) {
    return LBAPI_ERR_CFG_INVALID;
  }
  // 2. 取出 api_config_impl
  api_config_impl *cfg = dynamic_cast<api_config_impl *>(&config);
  if (cfg == nullptr) {
    return LBAPI_ERR_TRANS_TYPE;
  }

  // 3. 按 fast_counter_type + speed_link_type 选型 (5 种有效组合)
  counter_type ct = cfg->get_fast_counter_type();
  speed_link_type slt = cfg->get_speed_link_type();
  o_api = nullptr;

  if (ct == counter_type::gw_direct) {
    if (slt == speed_link_type::socket_single) {
      auto *impl = new api_impl<gw_counter_direct, single_socket_engine<gw_counter_direct>>();
      ret = impl->init(*cfg, cb);
      if (ret == LBAPI_OK) {
        o_api = static_cast<api_interface *>(impl);
      } else {
        delete impl;
      }
    } else if (slt == speed_link_type::tcpdirect) {
      auto *impl = new api_impl<gw_counter_direct, tcpdirect_engine<gw_counter_direct>>();
      ret = impl->init(*cfg, cb);
      if (ret == LBAPI_OK) {
        o_api = static_cast<api_interface *>(impl);
      } else {
        delete impl;
      }
    } else {
      return LBAPI_ERR_UNSUPPORT_LINK;
    }
  } else if (ct == counter_type::fpga_direct) {
    if (slt == speed_link_type::socket_single) {
      auto *impl = new api_impl<fpga_counter_direct, single_socket_engine<fpga_counter_direct>>();
      ret = impl->init(*cfg, cb);
      if (ret == LBAPI_OK) {
        o_api = static_cast<api_interface *>(impl);
      } else {
        delete impl;
      }
    } else if (slt == speed_link_type::tcpdirect) {
      auto *impl = new api_impl<fpga_counter_direct, tcpdirect_engine<fpga_counter_direct>>();
      ret = impl->init(*cfg, cb);
      if (ret == LBAPI_OK) {
        o_api = static_cast<api_interface *>(impl);
      } else {
        delete impl;
      }
    } else {
      return LBAPI_ERR_UNSUPPORT_LINK;
    }
  } else if (ct == counter_type::fpga_gateway) {
    if (slt == speed_link_type::socket_shared) {
      auto *impl = new api_impl<fpga_counter_gateway, idle_engine<fpga_counter_gateway>>();
      ret = impl->init(*cfg, cb);
      if (ret == LBAPI_OK) {
        o_api = static_cast<api_interface *>(impl);
      } else {
        delete impl;
      }
    } else {
      return LBAPI_ERR_UNSUPPORT_LINK;
    }
  } else {
    return LBAPI_ERR_UNSUPPORT_COUNTER;
  }
  return ret;
}

void api_interface::release_instance(api_interface *api) {
  if (api != nullptr) {
    delete api;
  }
}

// ---- 模板实现: api_impl<TF, TE> 继承 api_interface ----

template <class TF, class TE> int32 api_impl<TF, TE>::init(const api_config_impl &cfg, api_callback *cb) {
  speed_link_type_ = cfg.get_speed_link_type();
  speed_counter_type_ = cfg.get_fast_counter_type();

  // 1. 初始化日志 (D17)
  int32 tlog_que_size = cfg.get_log_queue_size_mb();
  if (tlog_que_size < 16)
    tlog_que_size = 16;
  int32 ret = log_.open_log(cfg.get_log_output_dir(), cfg.get_api_instance_name(), cfg.get_log_level(), 0,
                            tlog_que_size * 1024 * 1024);
  if (ret < 0) {
    return LBAPI_ERR_LOG_INIT;
  }

  // 2. 初始化回调管理器
  int32 single_writer = (speed_counter_type_ == counter_type::fpga_gateway) ? 1 : 0;
  ret = cb_mgr_.init(&log_, cb, cfg.get_callback_mode(), cfg.get_callback_thread_cpu(),
                     cfg.get_callback_queue_size_mb(), cfg.get_callback_wait_ms(), single_writer);
  if (ret < 0) {
    return ret;
  }

  // 3. 初始化柜台
  ret = fast_.init(cfg, &cb_mgr_, &log_);
  if (ret < 0) {
    return ret;
  }
  ret = c98_.init(cfg, &cb_mgr_, &log_);
  if (ret < 0) {
    return ret;
  }

  // 4. 初始化引擎
  ret = fast_engine_.init(cfg, &fast_, &log_);
  if (ret < 0) {
    return ret;
  }
  ret = multi_engine_.init(cfg, &fast_, &log_, &c98_);
  if (ret < 0) {
    return ret;
  }

  // 4. 初始化柜台与引擎间的引用
  if (speed_counter_type_ == counter_type::fpga_gateway) {
    c98_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
    fast_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
  } else {
    c98_.init_trade(fast_engine_.get_queue(), fast_engine_.get_out_op());
    fast_.init_trade(fast_engine_.get_queue(), fast_engine_.get_out_op());
  }
  fast_.init_gateway(multi_engine_.get_queue(), multi_engine_.get_out_op());
  c98_.init_gateway(multi_engine_.get_queue(), multi_engine_.get_out_op());

  lb_common::lb_log_hand tlh(&log_);
  info_log(tlh) << "init api instance ok" << end_log;
  return LBAPI_OK;
}

// start: 启动所有链接和线程
template <class TF, class TE> int32 api_impl<TF, TE>::start() {
  while (true) {
    int32 t = lb_common::atomic_load32(&have_start);
    if (t == 0) {
      if (lb_common::atomic_cas32(&have_start, &t, 1)) {
        break;
      }
    } else if (t == 1) {
      lb_common::comm_utils::sleep_us(300);
    } else {
      return LBAPI_OK;
    }
  }

  // 1. cb 管理器 (仅 queued 模式)
  int32 ret = cb_mgr_.start();
  if (ret < 0) {
    return ret;
  }

  // 2. 添加极速链接定时到multi engine 线程中监听
  ret = fast_engine_.add_timer_poll(multi_engine_.get_thread());
  if (ret < 0) {
    return ret;
  }

  // 3. 98 链接同步连接 (仅建链, 不收发)
  ret = multi_engine_.connect_98agw();
  if (ret < 0) {
    return ret;
  }

  // 4. multi 引擎 (异步启动 epoll 线程, 接管 98 + 极速GW 链接的 IO)
  ret = multi_engine_.start();
  if (ret < 0) {
    return ret;
  }

  // 5. fast 引擎 (异步启动业务 epoll 线程, 接管极速业务链接的 IO)
  ret = fast_engine_.start();
  if (ret < 0) {
    return ret;
  }

  // 6. 同步 agw 登陆 (必须最后, 此时 multi 线程已运行, 可接收 AGW_LOGIN_ANS 应答)
  ret = c98_.deal_agw_login();
  if (ret < 0) {
    return ret;
  }

  lb_common::atomic_store32(&have_start, 2);

  lb_common::lb_log_hand tlh(&log_);
  info_log(tlh) << "start api instance ok" << end_log;
  return LBAPI_OK;
}

// stop
template <class TF, class TE> void api_impl<TF, TE>::stop() {
  if (have_start == 0) {
    return;
  }

  fast_engine_.stop();
  multi_engine_.stop();
  cb_mgr_.stop();

  lb_common::lb_log_hand tlh(&log_);
  info_log(tlh) << "stop api instance end" << end_log;
  lb_common::comm_utils::sleep_ms(1000);

  log_.close_log();
  have_start = 0;
}

// ---- 业务方法实现 ----

// login: 发起完整登录流程 (异步, 通过 on_login 通知)
template <class TF, class TE> int32 api_impl<TF, TE>::login(const LoginReq &req) {
  // 先登陆 98
  return c98_.deal_login_req(req);
}

// order_insert: 买卖委托 (D22: a+b 组合降级)
template <class TF, class TE> int32 api_impl<TF, TE>::order_insert(const OrderReq &req) {
  int32 ret = fast_.deal_order_req(req);
  if (ret == LBAPI_ERR_COUNTER_OFFLINE || ret == LBAPI_ERR_UNSUPPORTED_OP) {
    // 降级到 98
    return c98_.deal_order_req(req);
  }
  return ret;
}

// etf_order_insert: ETF 申购赎回
template <class TF, class TE> int32 api_impl<TF, TE>::etf_order_insert(const OrderReq &req) {
  int32 ret = fast_.deal_etf_order_req(req);
  if (ret == LBAPI_ERR_COUNTER_OFFLINE || ret == LBAPI_ERR_UNSUPPORTED_OP) {
    return c98_.deal_etf_order_req(req);
  }
  return ret;
}

// bse_order_insert: 北交所委托 (BSE 永远走 98)
template <class TF, class TE> int32 api_impl<TF, TE>::bse_order_insert(const OrderReq &req) {
  return c98_.deal_bse_order_req(req);
}

// order_cancel: 委托撤单
template <class TF, class TE> int32 api_impl<TF, TE>::order_cancel(const CancelReq &req) {
  int32 ret = fast_.deal_cancel_req(req);
  if (ret == LBAPI_ERR_COUNTER_OFFLINE || ret == LBAPI_ERR_UNSUPPORTED_OP) {
    return c98_.deal_cancel_req(req);
  }
  return ret;
}

// 6 个 query_* (D23: 未登录返回 NOT_LOGGED_IN)
template <class TF, class TE> int32 api_impl<TF, TE>::order_query(const OrderQueryReq &req) {
  return c98_.deal_order_query(req);
}
template <class TF, class TE> int32 api_impl<TF, TE>::order_batch_query(const OrderBatchQueryReq &req) {
  return c98_.deal_order_batch_query(req);
}
template <class TF, class TE> int32 api_impl<TF, TE>::trade_query(const TradeQueryReq &req) {
  return c98_.deal_trade_query(req);
}
template <class TF, class TE> int32 api_impl<TF, TE>::trade_batch_query(const TradeBatchQueryReq &req) {
  return c98_.deal_trade_batch_query(req);
}
template <class TF, class TE> int32 api_impl<TF, TE>::fund_query(const FundQueryReq &req) {
  return c98_.deal_fund_query(req);
}
template <class TF, class TE> int32 api_impl<TF, TE>::position_query(const PositionQueryReq &req) {
  return c98_.deal_position_query(req);
}

// ---- 构造 / 析构 (基类 api_interface 已有默认实现) ----

template <class TF, class TE> api_impl<TF, TE>::api_impl() = default;

// 显式实例化 5 种配置
template class api_impl<gw_counter_direct, single_socket_engine<gw_counter_direct>>;
template class api_impl<gw_counter_direct, tcpdirect_engine<gw_counter_direct>>;
template class api_impl<fpga_counter_direct, single_socket_engine<fpga_counter_direct>>;
template class api_impl<fpga_counter_direct, tcpdirect_engine<fpga_counter_direct>>;
template class api_impl<fpga_counter_gateway, idle_engine<fpga_counter_gateway>>;

} // namespace lb_api

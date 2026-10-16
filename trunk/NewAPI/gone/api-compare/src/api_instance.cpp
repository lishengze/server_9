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
#ifdef HAS_TCPDIRECT
#include "tcpdir_link.h"
#include "tcpdirect_engine.h"
#endif

#include <cstring>
#include <stdatomic.h>

namespace lb_api
{

  // ---- 工厂函数: create_instance ----
  //
  // 由于 api_impl 是模板类, 工厂函数用 if-else 链按 5 种配置分别 new
  // 然后调用各模板实例的 init. 注意: 这里不能用 dynamic_cast, 因为模板
  // 实例化是编译期的. 我们直接按 5 种配置分别调用, 失败时 delete.

  int32 api_interface::create_instance(api_interface *&o_api, api_config &config, api_callback *cb)
  {
    if (cb == nullptr)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    // 1. 校验配置
    int32 ret = config.validate();
    if (ret != LBAPI_OK)
    {
      return LBAPI_ERR_CFG_INVALID;
    }
    // 2. 取出 api_config_impl
    api_config_impl *cfg = dynamic_cast<api_config_impl *>(&config);
    if (cfg == nullptr)
    {
      return LBAPI_ERR_TRANS_TYPE;
    }

    // 3. 按 fast_counter_type + speed_link_type 选型 (5 种有效组合)
    counter_type ct = cfg->get_fast_counter_type();
    speed_link_type slt = cfg->get_speed_link_type();
    o_api = nullptr;

    if (ct == counter_type::gw_direct)
    {
      if (slt == speed_link_type::socket_single)
      {
        auto *impl = new api_impl<gw_counter_direct, single_socket_engine<gw_counter_direct>>();
        ret = impl->init(*cfg, cb);
        if (ret == LBAPI_OK)
        {
          o_api = static_cast<api_interface *>(impl);
        }
        else
        {
          delete impl;
        }
      }
#ifdef HAS_TCPDIRECT
      else if (slt == speed_link_type::tcpdirect)
      {
        auto *impl = new api_impl<gw_counter_direct, tcpdirect_engine<gw_counter_direct>>();
        ret = impl->init(*cfg, cb);
        if (ret == LBAPI_OK)
        {
          o_api = static_cast<api_interface *>(impl);
        }
        else
        {
          delete impl;
        }
      }
#endif // HAS_TCPDIRECT（无 solarflare 变体：tcpdirect 选型落入下方 UNSUPPORT_LINK）
      else
      {
        return LBAPI_ERR_UNSUPPORT_LINK;
      }
    }
    else if (ct == counter_type::fpga_direct)
    {
      if (slt == speed_link_type::socket_single)
      {
        auto *impl = new api_impl<fpga_counter_direct, single_socket_engine<fpga_counter_direct>>();
        ret = impl->init(*cfg, cb);
        if (ret == LBAPI_OK)
        {
          o_api = static_cast<api_interface *>(impl);
        }
        else
        {
          delete impl;
        }
      }
#ifdef HAS_TCPDIRECT
      else if (slt == speed_link_type::tcpdirect)
      {
        auto *impl = new api_impl<fpga_counter_direct, tcpdirect_engine<fpga_counter_direct>>();
        ret = impl->init(*cfg, cb);
        if (ret == LBAPI_OK)
        {
          o_api = static_cast<api_interface *>(impl);
        }
        else
        {
          delete impl;
        }
      }
#endif // HAS_TCPDIRECT（无 solarflare 变体：tcpdirect 选型落入下方 UNSUPPORT_LINK）
      else
      {
        return LBAPI_ERR_UNSUPPORT_LINK;
      }
    }
    else if (ct == counter_type::fpga_gateway)
    {
      if (slt == speed_link_type::socket_shared)
      {
        auto *impl = new api_impl<fpga_counter_gateway, idle_engine<fpga_counter_gateway>>();
        ret = impl->init(*cfg, cb);
        if (ret == LBAPI_OK)
        {
          o_api = static_cast<api_interface *>(impl);
        }
        else
        {
          delete impl;
        }
      }
      else
      {
        return LBAPI_ERR_UNSUPPORT_LINK;
      }
    }
    else
    {
      return LBAPI_ERR_UNSUPPORT_COUNTER;
    }
    return ret;
  }

  void api_interface::release_instance(api_interface *api)
  {
    if (api != nullptr)
    {
      delete api;
    }
  }

  // ---- 模板实现: api_impl<TF, TE> 继承 api_interface ----

  template <class TF, class TE>
  int32 api_impl<TF, TE>::init(const api_config_impl &cfg, api_callback *cb)
  {
    speed_link_type_ = cfg.get_speed_link_type();
    speed_counter_type_ = cfg.get_fast_counter_type();
    market_type_ = cfg.get_market_type();

    // 1. 初始化日志
    int32 tlog_que_size = cfg.get_log_queue_size_mb();
    if (tlog_que_size < 16)
      tlog_que_size = 16;
    if (tlog_que_size > 1024)
      tlog_que_size = 1024;
    int32 ret = log_.open_log(cfg.get_log_output_dir(), cfg.get_api_instance_name(), cfg.get_log_level(), 0,
                              tlog_que_size * 1024 * 1024);
    if (ret < 0)
    {
      return LBAPI_ERR_LOG_INIT;
    }

    // 2. 初始化回调管理器
    ret = cb_mgr_.init(&log_, cb, cfg.get_callback_mode(), cfg.get_callback_thread_cpu(),
                       cfg.get_callback_queue_size_mb(), cfg.get_callback_wait_ms());
    if (ret < 0)
    {
      return ret;
    }

    // 3. 初始化柜台
    ret = fast_.init(cfg, &cb_mgr_, &log_);
    if (ret < 0)
    {
      return ret;
    }
    ret = c98_.init(cfg, &cb_mgr_, &log_);
    if (ret < 0)
    {
      return ret;
    }

    // 4. 初始化引擎
    ret = fast_engine_.init(cfg, &fast_, &log_);
    if (ret < 0)
    {
      return ret;
    }
    ret = multi_engine_.init(cfg, &fast_, &log_, &c98_);
    if (ret < 0)
    {
      return ret;
    }

    // 4. 初始化柜台与引擎间的引用
    if (speed_counter_type_ == counter_type::fpga_gateway)
    {
      c98_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
      fast_.init_trade(multi_engine_.get_queue(), multi_engine_.get_out_op());
    }
    else
    {
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
  // 并发与重入: have_start 控制 (0-未启动,1-启动中,2-已启动);
  // 已启动幂等返回 OK; 某成员失败时调用各成员 stop() 回退已启动部分, 复位 have_start 后可重试
  template <class TF, class TE>
  int32 api_impl<TF, TE>::start()
  {
    lb_common::lb_log_hand tlh(&log_);

    while (true)
    {
      int32 t = lb_common::atomic_load32(&have_start);
      if (t == 0)
      {
        if (lb_common::atomic_cas32(&have_start, &t, 1))
        {
          break; ///< 抢占启动权成功
        }
      }
      else if (t == 1)
      {
        // 另一线程正在启动: 等装配完成, 视最终状态返回 (避免重入)
        lb_common::comm_utils::sleep_us(300);
      }
      else if (t == 3)
      {
        error_log(tlh) << "api instance have stopped" << end_log;
        return LBAPI_ERR_STATE_LIMITED;
      }
      else
      {
        return LBAPI_OK; ///< t==2 已启动, 幂等返回成功
      }
    }

    int32 ret = LBAPI_OK;
    do
    {
      // 1. cb 管理器 (仅 queued 模式)
      ret = cb_mgr_.start();
      if (ret < 0)
      {
        break;
      }

      // 2. 添加极速链接定时到multi engine 线程中监听
      ret = fast_engine_.add_timer_poll(multi_engine_.get_thread());
      if (ret < 0)
      {
        break;
      }

      // 3. 98 链接同步连接 (仅建链, 不收发)
      ret = multi_engine_.connect_98agw();
      if (ret < 0)
      {
        break;
      }

      // 4. multi 引擎 (异步启动 epoll 线程, 接管 98 + 极速GW 链接的 IO)
      ret = multi_engine_.start();
      if (ret < 0)
      {
        break;
      }

      // 5. fast 引擎 (异步启动业务 epoll 线程, 接管极速业务链接的 IO)
      ret = fast_engine_.start();
      if (ret < 0)
      {
        break;
      }
    } while (false);

    if (ret < 0)
    {
      // 失败回退: 逆序回退已启动模块 (各成员 stop 为回退接口: 未启动为 no-op,
      // 已启动仅停线程/关已建链接, 不销毁 init 资源); 复位 have_start, 允许重试
      fast_engine_.stop();
      multi_engine_.stop();
      cb_mgr_.stop();
      lb_common::atomic_store32(&have_start, 0);

      error_log(tlh) << "start api instance failed,ret=" << ret << end_log;
      return ret;
    }

    lb_common::atomic_store32(&have_start, 2);

    info_log(tlh) << "start api instance ok" << end_log;
    return LBAPI_OK;
  }

  // stop: 用户主动停止 (关闭所有链接和线程)
  // 并发与重入: have_start 控制, 仅已启动 (2) 时进入拆卸, 未启动/启动中为 no-op
  template <class TF, class TE>
  void api_impl<TF, TE>::stop()
  {
    while (true)
    {
      int32 t = lb_common::atomic_load32(&have_start);
      if (t == 3 || t == 0)
        return;
      else if (t == 1)
      {
        lb_common::comm_utils::sleep_us(300);
      }
      else
      {
        if (lb_common::atomic_cas32(&have_start, &t, 3))
        {
          break; ///< 抢占启动权成功
        }
      }
    }

    // 先停 multi (其线程可能正向 fast 发送队列投递事件), 再停 fast, 最后停回调线程;
    // 避免 multi 线程在途 write_cmt_mth 写入已关闭的 fast 队列
    multi_engine_.close();
    fast_engine_.close();
    cb_mgr_.close();

    lb_common::lb_log_hand tlh(&log_);
    info_log(tlh) << "stop api instance end" << end_log;
    lb_common::comm_utils::sleep_ms(1000);

    log_.close_log();
  }

  // ---- 业务方法实现 ----

  // login: 发起完整登录流程 (异步, 通过 on_login 通知)
  // 编排 (REQ 4.4): 1.agw 同步登录(内部幂等) → 2.98 账户登录(内部查缓存: 已成功过则跳过 98
  //                直达极速登录投递) → 3.极速登录投递统一入口判定:
  //                  fpga 用户已下线 → 不登录极速, 98 登录成功即回调成功 (降级 98 模式);
  //                  未下线 → 正常投递 (极速侧对已登录账户幂等回成功, 不重发)
  // 入口校验 (B6, ): branch_id/fund_account_id 空 (首字节 NUL) 直接拒绝 ——
  // 空身份登录包发出后柜台应答数组不含该账户, 98 步 pending 永挂且后续 login 静默合并 (§7-B6)
  template <class TF, class TE>
  int32 api_impl<TF, TE>::login(const LoginReq &req)
  {
    if (req.branch_id[0] == '\0' || req.fund_account_id[0] == '\0')
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    // 1. agw 同步登录 (deal_agw_login 内部幂等: 已登录成功直接返回 OK, 在途等待首者结果)
    int32 ret = c98_.deal_agw_login();
    if (ret != LBAPI_OK)
    {
      return ret;
    }

    // 2. 98 账户登录
    return c98_.deal_login_req(req);
  }

  // build_logout_rtn: 构造登出应答 (编排层职责: LogoutAns 构造属于 api_impl, 不下沉柜台)
  static void build_logout_rtn(const LogoutReq &req, int32 err_ret, const char *err_msg, LogoutAns &ans)
  {
    ans.client_req_no = req.client_req_no;
    lb_common::comm_utils::str_copy_format(ans.cust_id.data(), req.cust_id.data(), sizeof(ans.cust_id));
    lb_common::comm_utils::str_copy_format(ans.fund_account_id.data(), req.fund_account_id.data(),
                                           sizeof(ans.fund_account_id));
    lb_common::comm_utils::str_copy_format(ans.account_id.data(), req.account_id.data(), sizeof(ans.account_id));
    lb_common::comm_utils::str_copy_format(ans.branch_id.data(), req.branch_id.data(), sizeof(ans.branch_id));
    ans.err_code = err_ret;
    if (err_msg != nullptr)
    {
      lb_common::comm_utils::str_copy_format(ans.err_msg.data(), err_msg, sizeof(ans.err_msg));
    }
    else
    {
      std::memset(ans.err_msg.data(), 0, sizeof(ans.err_msg));
    }
  }

  // logout: 用户主动登出 (§3.2.4) —— 纯本地状态操作 (不发协议消息), 同步完成
  // 门控: 柜台层先行(状态机 to_logout: 用户在途 → STATE_LIMITED; 账户不匹配 → NO_CUST;
  //       offline 降级账户 → 成功) —— 拒绝时 98 状态尚未变更;
  //       98 层随后(cust_login_map 无成员 → NOT_LOG_CUST; 柜台登出成功 ⇒ 必经 98 登录
  //       成功, map 必有成员, 正常不可达, 仅防御)
  // 成功路径: 柜台状态→LOGOUT+按模式关链(direct 关 core/个微不关链·多账户共用/gateway 不关 GW 链)
  //           + 98 val→LOGOUT(成员不移除) + on_logout 成功回调
  template <class TF, class TE>
  int32 api_impl<TF, TE>::logout(const LogoutReq &req)
  {
    lb_common::lb_log_hand tlh(&log_);

    int32_t ret = fast_.deal_logout_req(req);
    if (ret < 0)
    {
      return ret;
    }

    ret = c98_.deal_logout_req(req);
    if (ret < 0)
    {
      return ret;
    }

    // 本地同步操作无失败路径 → 回调成功
    LogoutAns ans;
    build_logout_rtn(req, LBAPI_OK, nullptr, ans);
    cb_mgr_.on_logout(ans);

    info_log(tlh) << "cust logout ok,branch_id=" << req.branch_id.data() << ",fund_account=" << req.fund_account_id.data()
                  << ",client_req_no=" << req.client_req_no << end_log;
    return LBAPI_OK;
  }

  // ---- 委托请求校验 (RUL-21: 「接口 × 柜台」矩阵, 不过统一 LBAPI_ERR_INVALID_PARAM) ----
  // 通用项: 市场 (RUL-20: 非北交所==实例市场, 北交所==109)、order_qty>0。
  // 接口差异项 (side 合法集随 business_type 分叉 / ETF 价格与类型语义):
  //   order_insert   = 现货集中竞价 (business_type=1): side 买/卖/正回购/逆回购 (含回购, RUL-21),
  //                    仅限价, 价格>0
  //   etf_order_insert = ETF实时申赎 (business_type=12, RUL-22): side 申购/赎回; 价格允许 0; 不校验类型
  //   bse_order_insert = 北交所 (98 专用): side 买/卖, 仅限价, 价格>0
  // 柜台差异项: FPGA 仅买卖/沪深/限价 (fpga_counter_base::check_order_dict 二道, 回购单到 FPGA
  //   实例被拒不降级); FPGA 不支持 ETF (UNSUPPORTED_OP → 上层降级 98); BSE 永远走 98。
  // 字典其余 side (CALL 认购/PLEDGE_IN/OUT 质押) 为回报回显+未来扩展, 本期无委托组包路径。
  static int32 check_order_spot_dict(const OrderReq &req, int16_t inst_mkt)
  {
    if (req.order_type != ORDER_TYPE_LIMIT || req.order_qty <= 0 || req.order_price <= 0)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    if (req.side != ORDER_SIDE_BUY && req.side != ORDER_SIDE_SELL && req.side != ORDER_SIDE_REPO &&
        req.side != ORDER_SIDE_ANTI_REPO)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    if (req.market_type != inst_mkt)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    return LBAPI_OK;
  }

  static int32 check_order_etf_dict(const OrderReq &req, int16_t inst_mkt)
  {
    if (req.order_qty <= 0 || req.order_price < 0) ///< ETF 申赎允许价格 0; order_type 不校验
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    if (req.side != ORDER_SIDE_PURCHASE && req.side != ORDER_SIDE_REDEEM)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    if (req.market_type != inst_mkt)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    return LBAPI_OK;
  }

  static int32 check_order_bse_dict(const OrderReq &req)
  {
    if (req.order_type != ORDER_TYPE_LIMIT || req.order_qty <= 0 || req.order_price <= 0)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    if (req.side != ORDER_SIDE_BUY && req.side != ORDER_SIDE_SELL)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    if (req.market_type != MARKET_TYPE_BSE)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    return LBAPI_OK;
  }

  // order_insert: 买卖委托 (极速返回不支持/柜台离线时自动降级 98)
  template <class TF, class TE>
  int32 api_impl<TF, TE>::order_insert(const OrderReq &req)
  {
    int32 tdict = check_order_spot_dict(req, market_type_);
    if (tdict != LBAPI_OK)
    {
      return tdict;
    }
    int32 ret = fast_.deal_order_req(req);
    if (ret == LBAPI_ERR_UNSUPPORTED_OP || ret == LBAPI_ERR_COUNTER_OFFLINE)
    {
      // 降级到 98
      return c98_.deal_order_req(req);
    }
    return ret;
  }

  // etf_order_insert: ETF 申购赎回
  template <class TF, class TE>
  int32 api_impl<TF, TE>::etf_order_insert(const OrderReq &req)
  {
    int32 tdict = check_order_etf_dict(req, market_type_);
    if (tdict != LBAPI_OK)
    {
      return tdict;
    }
    int32 ret = fast_.deal_etf_order_req(req);
    if (ret == LBAPI_ERR_UNSUPPORTED_OP || ret == LBAPI_ERR_COUNTER_OFFLINE)
    {
      return c98_.deal_etf_order_req(req);
    }
    return ret;
  }

  // bse_order_insert: 北交所委托 (BSE 永远走 98)
  template <class TF, class TE>
  int32 api_impl<TF, TE>::bse_order_insert(const OrderReq &req)
  {
    int32 tdict = check_order_bse_dict(req);
    if (tdict != LBAPI_OK)
    {
      return tdict;
    }
    return c98_.deal_bse_order_req(req);
  }

  // order_cancel: 委托撤单
  // 入口校验 (B5, ): 双定位键全 0 无法定位任何委托, 拒绝;
  // 单键非 0 保留 (98/gw 用户提供定位双键 orig_cli_ord_no+orig_client_seq_id, RUL-4,
  // 单键场景交柜台裁决); fund/branch 空由柜台侧会话查找兜底 (NOT_LOG_CUST/NO_CUST)
  template <class TF, class TE>
  int32 api_impl<TF, TE>::order_cancel(const CancelReq &req)
  {
    if (req.order_sys_no == 0 && req.client_seq_id == 0)
    {
      return LBAPI_ERR_INVALID_PARAM;
    }
    int32 ret = fast_.deal_cancel_req(req);
    if (ret == LBAPI_ERR_UNSUPPORTED_OP || ret == LBAPI_ERR_COUNTER_OFFLINE)
    {
      return c98_.deal_cancel_req(req);
    }
    return ret;
  }

  // 12 个 query_* (全部走 98 柜台; 前置: 98 链接已建立 + agw 用户登录成功 + 该账户 98 登录成功)
  template <class TF, class TE>
  int32 api_impl<TF, TE>::order_query(const OrderQueryReq &req)
  {
    return c98_.deal_order_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::order_batch_query(const OrderBatchQueryReq &req)
  {
    return c98_.deal_order_batch_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::trade_query(const TradeQueryReq &req)
  {
    return c98_.deal_trade_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::trade_batch_query(const TradeBatchQueryReq &req)
  {
    return c98_.deal_trade_batch_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::fund_query(const FundQueryReq &req)
  {
    return c98_.deal_fund_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::position_query(const PositionQueryReq &req)
  {
    return c98_.deal_position_query(req);
  }

  // 6 个扩展查询 (98 柜台扩展查询消息集, 均走 98, 极速柜台无查询接口)
  template <class TF, class TE>
  int32 api_impl<TF, TE>::max_order_qty_query(const MaxOrderQtyQueryReq &req)
  {
    return c98_.deal_max_order_qty_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::total_fund_asset_query(const TotalFundAssetQueryReq &req)
  {
    return c98_.deal_total_fund_asset_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::fund_maxdraw_query(const FundMaxdrawQueryReq &req)
  {
    return c98_.deal_fund_maxdraw_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::cust_margin_avl_query(const CustMarginAvlQueryReq &req)
  {
    return c98_.deal_cust_margin_avl_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::asset_debts_query(const AssetDebtsQueryReq &req)
  {
    return c98_.deal_asset_debts_query(req);
  }
  template <class TF, class TE>
  int32 api_impl<TF, TE>::credit_max_order_qty_query(const CreditMaxOrderQtyQueryReq &req)
  {
    return c98_.deal_credit_max_order_qty_query(req);
  }

  // ---- 构造 / 析构 (基类 api_interface 已有默认实现) ----

  template <class TF, class TE>
  api_impl<TF, TE>::api_impl() = default;

  // 显式实例化 5 种配置（tcpdirect 2 种仅 WITH_TCPDIRECT 变体，socket-only 变体为 3 种）
  template class api_impl<gw_counter_direct, single_socket_engine<gw_counter_direct>>;
#ifdef HAS_TCPDIRECT
  template class api_impl<gw_counter_direct, tcpdirect_engine<gw_counter_direct>>;
#endif
  template class api_impl<fpga_counter_direct, single_socket_engine<fpga_counter_direct>>;
#ifdef HAS_TCPDIRECT
  template class api_impl<fpga_counter_direct, tcpdirect_engine<fpga_counter_direct>>;
#endif
  template class api_impl<fpga_counter_gateway, idle_engine<fpga_counter_gateway>>;

} // namespace lb_api

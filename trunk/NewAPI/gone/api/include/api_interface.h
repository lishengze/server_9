#pragma once

#include "api_callback.h"
#include "api_config.h"
#include "api_errno.h"
#include "order_trade_type.h"
#include <cstdint>

namespace lb_api {

/// API接口类 - 供用户调用的虚基类
///
/// 创建实例:
///   api_config* cfg = api_config::create_config();
///   cfg->set_attr(config_name::fast_counter_type, ...);
///   ... 设置各项配置 ...
///   api_interface* api = nullptr;
///   int32_t ret = api_interface::create_instance(*cfg, &my_callback, &api);
///   api_config::destroy_config(cfg);  // 实例创建成功后可释放配置
///   if (ret != LBAPI_OK) { /* 处理错误 */ }
///
/// 使用:
///   api->start();      // 启动链接和线程
///   api->login(req);   // 发起登录(自动完成98agw→98账户→极速柜台登录流程)
///   api->order_insert(order_req);
///   ...
///   api->stop();        // 停止
///   api->release();     // 释放实例
class api_interface {
public:
  virtual ~api_interface() {}

  /// 登录
  /// 自动完成完整登录流程:
  ///   1. 同步连接98柜台, 发起agw登录(使用配置的98agw_user/98agw_user_password)
  ///   2. agw登录成功后, 发起98柜台账户登录(使用req中的账户信息)
  ///   3. 98账户登录成功后(回调中), 发起极速柜台登录
  ///   4. 极速柜台登录成功后, 通过on_login回调通知用户
  ///   任一步骤失败, 通过on_login回调通知用户失败原因
  /// @param req  登录请求(账户信息)
  /// @return api_errno, LBAPI_OK=登录流程已发起(结果通过on_login回调通知)
  virtual int32_t login(const LoginReq &req) = 0;

  /// 买卖委托(发送到极速柜台)
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t order_insert(const OrderReq &req) = 0;

  /// ETF申购赎回委托(发送到极速柜台)
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t etf_order_insert(const OrderReq &req) = 0;

  /// 北交所委托(发送到极速柜台)
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t bse_order_insert(const OrderReq &req) = 0;

  /// 委托撤单(发送到极速柜台)
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t order_cancel(const CancelReq &req) = 0;

  /// 客户委托查询(发送到98柜台)
  /// @param req  委托查询请求
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t order_query(const OrderQueryReq &req) = 0;

  /// 客户委托批量查询(发送到98柜台)
  /// @param req  委托批量查询请求
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t order_batch_query(const OrderBatchQueryReq &req) = 0;

  /// 客户成交查询(发送到98柜台)
  /// @param req  成交查询请求
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t trade_query(const TradeQueryReq &req) = 0;

  /// 客户成交批量查询(发送到98柜台)
  /// @param req  成交批量查询请求
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t trade_batch_query(const TradeBatchQueryReq &req) = 0;

  /// 客户资金查询(发送到98柜台)
  /// @param req  资金查询请求
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t fund_query(const FundQueryReq &req) = 0;

  /// 客户持仓查询(发送到98柜台)
  /// @param req  持仓查询请求
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t position_query(const PositionQueryReq &req) = 0;

  /// 启动所有链接和线程
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t start() = 0;

  /// 停止所有链接和线程
  virtual void stop() = 0;

  /// 创建API实例(静态工厂函数)
  /// 内部自动完成: 配置校验→模块初始化→返回可用实例
  /// 创建成功后, config对象不再使用, 用户应调用api_config::destroy_config()释放
  /// @param o_api  输出实例指针, 需由调用方通过release()释放
  /// @param config  配置属性
  /// @param cb      用户回调接口
  /// @return api_errno, LBAPI_OK=成功
  static int32_t create_instance(api_interface *&o_api, api_config &config, api_callback *cb);

  /// 释放API 实例
  /// @param api  实例指针
  static void release_instance(api_interface *api);
};

} // namespace lb_api

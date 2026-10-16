#pragma once

#include "api_callback.h"
#include "api_config.h"
#include "api_errno.h"
#include "order_trade_type.h"
#include <cstdint>

namespace lb_api
{

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
  ///
  /// 生命周期约束(调用方必须遵守):
  ///   - stop()/release() 之后不得再调用本类任何业务接口(login/order_*/各查询等):
  ///     stop 会停止全部线程并释放内部收发队列, 之后的调用将解引用已释放内存导致崩溃;
  ///     生命周期由调用方自行保证, 接口内部不做启动状态检查
  ///   - stop() 后实例不可重启, 重新使用请 release() 后重新 create_instance()
  class api_interface
  {
  public:
    virtual ~api_interface() {}

    /// 登录
    /// 自动完成完整登录流程:
    ///   1. 同步连接98柜台, 发起agw登录(使用配置的98agw_user/98agw_user_password)
    ///   2. agw登录成功后, 发起98柜台账户登录(使用req中的账户信息)
    ///   3. 98账户登录成功后(回调中), 发起极速柜台登录
    ///   4. 极速柜台登录成功后, 通过on_login回调通知用户
    ///   任一步骤失败, 通过on_login回调通知用户失败原因
    /// 使用约束(调用方必须遵守):
    ///   login()发起后、收到该次on_login结果前, 不要对该账户调用logout() ——
    ///   在途登录期间登出, 迟到的登录成功应答可能把柜台登录状态恢复为已登录,
    ///   且on_logout与on_login回调时序可能与预期相反(时序由调用方自行控制)
    /// @param req  登录请求(账户信息)
    /// @return api_errno, LBAPI_OK=登录流程已发起(结果通过on_login回调通知)
    virtual int32_t login(const LoginReq &req) = 0;

    /// 登出
    /// 清除该客户的 98 柜台账户登录状态与极速柜台(fpga/个微)登录状态; 不影响 agwuser 登录
    /// fpga 直连模式关闭与 FPGA 的核心链接(不影响网关链接); 个微模式不关链接(多账户共用链路)
    /// 登出为本地同步操作(不向柜台发协议消息);
    /// 未成功登录过则登出失败(NOT_LOG_CUST); 已成功登录但柜台登录在途时拒绝(STATE_LIMITED)
    /// 使用约束(调用方必须遵守):
    ///   登出成功后不得再对该账户下发委托/撤单 —— 登出为纯本地状态操作,
    ///   若登出时存在未应答的在途登录, 迟到的登录成功应答会恢复柜台登录状态
    /// @param req  登出请求(账户信息)
    /// @return api_errno, LBAPI_OK=登出成功(结果同步经on_logout回调通知)
    virtual int32_t logout(const LogoutReq &req) = 0;

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

    /// 最大可委托数查询(发送到98柜台)
    /// @param req  最大可委托数查询请求
    /// @return api_errno, LBAPI_OK=成功
    virtual int32_t max_order_qty_query(const MaxOrderQtyQueryReq &req) = 0;

    /// 客户总资产查询(发送到98柜台)
    /// @param req  总资产查询请求
    /// @return api_errno, LBAPI_OK=成功
    virtual int32_t total_fund_asset_query(const TotalFundAssetQueryReq &req) = 0;

    /// 可取资金查询(发送到98柜台)
    /// @param req  可取资金查询请求
    /// @return api_errno, LBAPI_OK=成功
    virtual int32_t fund_maxdraw_query(const FundMaxdrawQueryReq &req) = 0;

    /// 融资融券信用数据(可用保证金)查询(发送到98柜台)
    /// @param req  可用保证金查询请求
    /// @return api_errno, LBAPI_OK=成功
    virtual int32_t cust_margin_avl_query(const CustMarginAvlQueryReq &req) = 0;

    /// 资产负债查询(发送到98柜台)
    /// @param req  资产负债查询请求
    /// @return api_errno, LBAPI_OK=成功
    virtual int32_t asset_debts_query(const AssetDebtsQueryReq &req) = 0;

    /// 两融最大可委托数查询(发送到98柜台)
    /// @param req  两融最大可委托数查询请求
    /// @return api_errno, LBAPI_OK=成功
    virtual int32_t credit_max_order_qty_query(const CreditMaxOrderQtyQueryReq &req) = 0;

    /// 启动所有链接和线程
    /// @return api_errno, LBAPI_OK=成功
    virtual int32_t start() = 0;

    /// 停止所有链接和线程
    /// 停止后不得再调用任何业务接口(login/order_*/各查询等):
    /// 内部线程与收发队列已被释放, 违规调用会解引用已释放内存导致崩溃;
    /// 实例不可重启, 重新使用请 release() 后重新 create_instance()
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

#pragma once

#include "api_config.h"
#include "order_trade_type.h"
#include <cstdint>

namespace lb_api
{

  /// 流信息(用于委托回报/成交推送/撤单应答回调, 标识推送来源; RUL-32)
  struct StreamInfo
  {
    int32_t counter_type; ///< 柜台类型 (counter_type 枚举值: 0=fixed_98 / 1=gw_direct 个微 /
                          ///< 2=fpga_direct / 3=fpga_gateway; API 层拒绝回报填拒绝方柜台类型)
    uint16_t partno;      ///< 分区/板卡: 98 柜台=回报分区号; fpga 柜台=板卡号; 个微柜台无意义填 0
    int64_t stream_seq;   ///< 流序号: 98 柜台=分区内回报记录号(index,与 partno 组成回报去重位置,
                          ///< 未填记录号时原值透传); 个微/fpga 恒 0 (RUL-33)
  };

  /// 批量查询应答控制(用于委托/成交/持仓查询应答回调)
  struct QueryAnsCtl
  {
    int32_t count;         ///< 本次回调中的数据条数
    int32_t is_last;       ///< 本次查询是否结束(true=所有数据已返回)
    int64_t client_req_no; ///< 客户请求号(对应查询请求中的 client_req_no)
  };

  /// 错误描述最大长度(含结尾\0)
  static constexpr int32_t MAX_ERR_DESC_LEN = 256;

  /// 事件类型(用于 on_event 回调标识事件来源)
  /// (agw 用户登录结果通过 login 同步接口返回码告知, 不走 on_event 回调)
  /// 注: 原 fast_user_offline=2 (极速柜台用户上下线) 已删除, 该通知改由 on_trade_counter
  /// 回调独立承载; 其后枚举值显式编号顺延重排保持连续 (仅进程内使用, 不上协议)
  enum class api_event_type : int32_t
  {
    none_type = 0,       ///< 无效类型
    not_support_msg = 1, ///< 收到不兼容消息 (消息体短于本版本已知最小长度, 协议不可信;
                         ///< 用户收到此回调应销毁 API 实例 —— stop + release 后重新创建)
    get_sec_info = 2,    ///< 获取证券信息事件 (err_code=0 完成, 非0 失败)
    report_sync_finish = 3 ///< 98 回报同步完成事件 (agw 登录后回报同步流程 SyncFinish 到达,
                         ///< err_code=0; 同步发送失败不回调 —— 该失败按 agw 登录失败处理,
                         ///< 经 login 返回码/关链重连感知)
  };

  /// API回调接口类
  ///
  /// 用户可选择性地实现关心的回调方法。
  /// 所有方法均有默认空实现（用户不实现 = 不回调），
  /// 避免用户被迫实现全部 11 个方法。
  class api_callback
  {
  public:
    virtual ~api_callback() {}

    /// 登录结果回调,用户实现必须支持函数可重入
    /// login()调用后，整个登录流程(98agw→98账户→极速柜台)的最终结果
    /// @param ans  登录应答(含错误码和错误信息)
    virtual void on_login(const LoginAns &ans) {}

    /// 登出结果回调,用户实现必须支持函数可重入
    /// @param ans  登录应答(含错误码和错误信息)
    virtual void on_logout(const LogoutAns &ans) {}

    /// 委托回报回调
    /// @param si   流信息(柜台类型+流序号)
    /// @param rtn  委托回报，包括委托响应 + 委托确认 + 委托废单  + 撤单成交
    virtual void on_order_rtn(const StreamInfo &si, const OrderRtn &rtn) {}

    /// 成交推送回调 —— 仅委托成交 (exec_type='F'); 成交回报通道的其他事件类型
    /// (委托确认/撤单成交/委托废单等)经 on_order_rtn 回调 (RUL-23)
    /// @param si   流信息(柜台类型+流序号)
    /// @param rtn  成交推送
    virtual void on_trade_rtn(const StreamInfo &si, const TradeRtn &rtn) {}

    /// 撤单响应回调，用于撤单柜台废单
    /// @param si   流信息(柜台类型+流序号)
    /// @param rsp  撤单响应信息
    virtual void on_cancel_rsp(const StreamInfo &si, const CancelRsp &rsp) {}

    /// 委托查询应答回调
    virtual void on_order_query_ans(const OrderRtn *ans_arr, const QueryAnsCtl &ctl) {}

    /// 成交查询应答回调
    virtual void on_trade_query_ans(const TradeInfo *ans_arr, const QueryAnsCtl &ctl) {}

    /// 资金查询应答回调
    virtual void on_fund_query_ans(const CustFundInfo &info) {}

    /// 持仓查询应答回调
    virtual void on_position_query_ans(const CustPositionInfo *ans_arr, const QueryAnsCtl &ctl) {}

    /// 最大可委托数查询应答回调 (98 柜台扩展查询)
    virtual void on_max_order_qty_query_ans(const CustMaxOrderQtyInfo &info) {}

    /// 客户总资产查询应答回调 (98 柜台扩展查询; 按币种多条一次回调, 无分页)
    virtual void on_total_fund_asset_query_ans(const CustTotalFundAssetInfo *ans_arr, const QueryAnsCtl &ctl) {}

    /// 可取资金查询应答回调 (98 柜台扩展查询; 按币种多条一次回调, 无分页)
    virtual void on_fund_maxdraw_query_ans(const CustFundMaxdrawInfo *ans_arr, const QueryAnsCtl &ctl) {}

    /// 融资融券信用数据(可用保证金)查询应答回调 (98 柜台扩展查询)
    virtual void on_cust_margin_avl_query_ans(const CustMarginAvlInfo &info) {}

    /// 资产负债查询应答回调 (98 柜台扩展查询)
    virtual void on_asset_debts_query_ans(const AssetDebtsInfo &info) {}

    /// 两融最大可委托数查询应答回调 (98 柜台扩展查询)
    virtual void on_credit_max_order_qty_query_ans(const CreditMaxOrderQtyInfo &info) {}

    /// 链接状态变化通知回调,用户实现必须支持函数可重入
    /// @param counter_type  柜台类型: 0=98柜台, 1=极速柜台
    /// @param link_type  链接类型: 0=网关链接, 1=极速交易链接
    /// @param status        状态: 0=已断线, 1=连接成功
    virtual void on_link_status(int32_t counter_type, int32_t link_type, int32_t status) {}

    /// 柜台用户状态通知回调,用户实现必须支持函数可重入
    /// (fpga 柜台用户/板卡状态推送, 板卡级通知为 api 实例对应客户属性)
    /// @param counter_type      柜台类型: 取值见 counter_type 枚举 (fpga_direct=2 / fpga_gateway=3)
    /// @param branch_id         分支机构代码 (C 串)
    /// @param fund_account_id   资金账号 (C 串)
    /// @param status            状态: 1-正常, 2-故障, 3-故障切换柜台 (G1_FPGA_USER_STATE_* 字典)
    virtual void on_trade_counter(int32_t counter_type, const char *branch_id, const char *fund_account_id, int32_t status) {}

    /// 通用事件通知回调,用户实现必须支持函数可重入
    /// @param event_type  事件类型(标识事件来源, 取值见 api_event_type 枚举)
    /// @param err_code    错误码(0-成功/通知类事件, 非0-错误)
    /// @param err_desc    描述信息(具体事件内容描述)，长度 < MAX_ERR_DESC_LEN
    /// @note event_type==not_support_msg 时为协议不兼容致命错误, 用户应销毁 API 实例
    virtual void on_event(api_event_type event_type, int32_t err_code, const char *err_desc) {}
  };

} // namespace lb_api

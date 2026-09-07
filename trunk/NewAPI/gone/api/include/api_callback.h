#pragma once

#include "api_config.h"
#include "order_trade_type.h"
#include <cstdint>

namespace lb_api {

static constexpr int32_t STREAM_API_COUNTER = 100;
/// 流信息(用于委托回报/成交推送回调, 标识推送来源和流序号)
struct StreamInfo {
  int32_t counter_type; ///< 柜台类型,STREAM_API_COUNTER-api,此时流序号=0
  int64_t stream_seq;   ///< 流序号
};

/// 批量查询应答控制(用于委托/成交/持仓查询应答回调)
struct QueryAnsCtl {
  int32_t count;         ///< 本次回调中的数据条数
  int32_t is_last;       ///< 本次查询是否结束(true=所有数据已返回)
  int64_t client_req_no; ///< 客户请求号(对应查询请求中的 client_req_no)
};

/// 错误描述最大长度(含结尾\0)
static constexpr int32_t MAX_ERR_DESC_LEN = 256;

/// 错误事件类型(用于on_error回调标识错误来源)
enum class err_event_type : int32_t {
  none_type = 0,         ///< 无效类型
  agw_user_login = 1,    ///< agw 用户登陆处理错误
  fast_user_offline = 2, ///< 极速柜台用户下线
  get_sec_info = 3       ///< 获取证券信息错误
};

/// API回调接口类
///
/// 用户可选择性地实现关心的回调方法。
/// 所有方法均有默认空实现（用户不实现 = 不回调），
/// 避免用户被迫实现全部 9 个方法。
class api_callback {
public:
  virtual ~api_callback() {}

  /// 登录结果回调
  /// login()调用后，整个登录流程(98agw→98账户→极速柜台)的最终结果
  /// @param ans  登录应答(含错误码和错误信息)
  virtual void on_login(const LoginAns &ans) {}

  /// 委托回报回调
  /// @param si   流信息(柜台类型+流序号)
  /// @param rtn  委托回报
  virtual void on_order_rtn(const StreamInfo &si, const OrderRtn &rtn) {}

  /// 成交推送回调
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

  /// 链接状态变化通知回调
  /// @param counter_type  柜台类型: 0=98柜台, 1=极速柜台
  /// @param link_type  链接类型: 0=网关链接, 1=极速交易链接
  /// @param status        状态: 0=已断线, 1=连接成功
  virtual void on_link_status(int32_t counter_type, int32_t link_type, int32_t status) {}

  /// 通用错误回调
  /// @param event_type  错误事件类型(标识错误来源, 取值见 err_event_type 枚举)
  /// @param err_code    错误码(具体含义见 err_desc)
  /// @param err_desc    错误描述信息(具体错误内容描述, 非错误码对应的通用信息)，长度 < MAX_ERR_DESC_LEN
  virtual void on_error(err_event_type event_type, int32_t err_code, const char *err_desc) {}
};

} // namespace lb_api

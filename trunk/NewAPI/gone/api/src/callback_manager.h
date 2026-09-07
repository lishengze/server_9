#pragma once

#include "api_callback.h"
#include "api_config.h"
#include "comm_sys.h"
#include "mlog.h"
#include "que_mth_buf.h"
#include "simple_thread.h"
#include <cstdint>

namespace lb_api {

/// 回调事件类型枚举
enum class cb_event_type : int32_t {
  login = 0,
  order_rtn = 1,
  trade_rtn = 2,
  cancel_rsp = 3,
  order_query = 4,
  trade_query = 5,
  fund_query = 6,
  position_query = 7,
  link_status = 8,
  error = 9
};

/// 回调事件头(每个回调事件开头)
struct cb_event_head {
  cb_event_type type; ///< 事件类型 (对应具体 on_* 分发逻辑)
  int32_t data_len;   ///< 事件数据长度(不含头)
};

/// 回调管理类
/// 继承simple_thread, 使用que_mth_buf队列实现队列模式回调
/// 队列模式下, 事件入队后trigger()唤醒线程, do_work()中读取分发
/// user_callback_初始化后一定有效, 无需判空
///
/// 单写/多写模式:
///   socket_shared模式下, 所有回调入队来自同一线程(mgmt_engine), 使用单写接口
///   socket_single/tcpdirect模式下, 多线程入队, 使用多写接口(mth)
class callback_manager : public lb_common::simple_thread {
public:
  callback_manager();
  ~callback_manager() override;

  /// 初始化
  /// @param log          日志对象指针(入队失败时打印错误日志, 可为nullptr)
  /// @param cb            用户回调接口指针(初始化后一定有效)
  /// @param mode          回调模式
  /// @param cpu_affinity  回调线程CPU亲和(-1=不绑定)
  /// @param queue_size_mb 队列大小(MB)
  /// @param wait_ms       回调线程epoll等待毫秒(0=死轮询)
  /// @param single_writer 单写模式(socket_shared时为true, 使用单写接口)
  int32_t init(lb_common::lb_log *log, api_callback *cb, callback_mode mode, int32_t cpu_affinity,
               int32_t queue_size_mb, int32_t wait_ms, int32_t single_writer);

  /// 启动回调线程(仅队列模式)
  int32_t start();

  /// 停止回调线程
  void stop();

  /// ---- 回调接口 (柜台/引擎调 → callback_manager, 内部按 mode 同步直调或入队) ----

  /// 登录结果回调 (从 cb_mgr_ 入口)
  /// @param ans  登录应答(含错误码和错误信息)
  void on_login(const LoginAns &ans);

  /// 委托回报回调
  /// @param si   流信息(柜台类型+流序号)
  /// @param rtn  委托回报
  void on_order_rtn(const StreamInfo &si, const OrderRtn &rtn);

  /// 成交推送回调
  /// @param si   流信息(柜台类型+流序号)
  /// @param rtn  成交推送
  void on_trade_rtn(const StreamInfo &si, const TradeRtn &rtn);

  /// 撤单响应回调，用于撤单柜台废单
  /// @param si   流信息(柜台类型+流序号)
  /// @param rsp  撤单响应信息
  void on_cancel_rsp(const StreamInfo &si, const CancelRsp &rsp);

  /// 委托查询应答回调
  /// @param ans_arr  委托回报数组(由 c98 柜台分配, callback_manager 不负责释放)
  /// @param ctl      本次应答控制(条数+是否结束+客户请求号)
  void on_order_query_ans(const OrderRtn *ans_arr, const QueryAnsCtl &ctl);

  /// 成交查询应答回调
  /// @param ans_arr  成交数组(由 c98 柜台分配, callback_manager 不负责释放)
  /// @param ctl      本次应答控制(条数+是否结束+客户请求号)
  void on_trade_query_ans(const TradeInfo *ans_arr, const QueryAnsCtl &ctl);

  /// 资金查询应答回调
  /// @param info  客户资金信息
  void on_fund_query_ans(const CustFundInfo &info);

  /// 持仓查询应答回调
  /// @param ans_arr  持仓数组(由 c98 柜台分配, callback_manager 不负责释放)
  /// @param ctl      本次应答控制(条数+是否结束+客户请求号)
  void on_position_query_ans(const CustPositionInfo *ans_arr, const QueryAnsCtl &ctl);

  /// 链接状态变化通知回调
  /// @param counter_type  柜台类型: 0=98柜台, 1=极速柜台
  /// @param link_type  链接类型: 0=网关链接, 1=极速交易链接
  /// @param status        状态: 0=已断线, 1=连接成功
  void on_link_status(int32_t counter_type, int32_t link_type, int32_t status);

  /// 通用错误回调
  /// @param event_type  错误事件类型(取值见 err_event_type 枚举)
  /// @param err_code    错误码
  /// @param err_desc    错误描述信息(长度 < MAX_ERR_DESC_LEN)
  void on_error(err_event_type event_type, int32_t err_code, const char *err_desc);

protected:
  /// simple_thread虚函数实现
  void do_work() override;
  bool need_work() override;

private:
  FORCE_INLINE int64_t write_get(char *&data, int32_t len) {
    if (0 == single_writer_)
      return cb_queue_.write_get_mth(data, len);
    else
      return cb_queue_.write_get(data, len);
  }

  FORCE_INLINE void write_cmt(int64_t pos, int32_t len) {
    if (0 == single_writer_)
      cb_queue_.write_cmt_mth(pos, len);
    else
      cb_queue_.write_cmt(pos, len);
  }

  callback_mode mode_;
  int32_t single_writer_; ///< 单写模式(使用write_get/write_cmt, 否则使用_mth版本)
  api_callback *user_callback_;
  lb_common::lb_log *log_;          ///< 日志对象(入队失败时打印错误日志)
  lb_common::que_mth_buf cb_queue_; ///< 回调事件队列(队列模式)
};

} // namespace lb_api

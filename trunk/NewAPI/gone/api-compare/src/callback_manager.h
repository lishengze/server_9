#pragma once

#include "api_callback.h"
#include "api_config.h"
#include "comm_sys.h"
#include "mlog.h"
#include "que_mth_buf.h"
#include "simple_thread.h"
#include <cstdint>

namespace lb_api
{

  /// 回调事件类型枚举
  enum class cb_event_type : int32_t
  {
    login = 0,
    order_rtn = 1,
    trade_rtn = 2,
    cancel_rsp = 3,
    order_query = 4,
    trade_query = 5,
    fund_query = 6,
    position_query = 7,
    link_status = 8,
    error = 9,
    logout = 10,
    trade_counter = 11,
    max_order_qty_query = 12,
    total_fund_asset_query = 13,
    fund_maxdraw_query = 14,
    cust_margin_avl_query = 15,
    asset_debts_query = 16,
    credit_max_order_qty_query = 17
  };

  /// 回调事件头(每个回调事件开头)
  struct cb_event_head
  {
    cb_event_type type; ///< 事件类型 (对应具体 on_* 分发逻辑)
    int32_t data_len;   ///< 事件数据长度(不含头)
  };

  /// 回调管理类
  /// 继承simple_thread, 使用que_mth_buf队列实现队列模式回调
  /// 队列模式下, 事件入队后trigger()唤醒线程, do_work()中读取分发
  /// user_callback_初始化后一定有效, 无需判空
  ///
  /// 仅为多写模式
  /// 队列满时事件丢弃(仅记错误日志), 不阻塞生产者 —— 回调不保证不丢,
  /// 需按回调流量配置队列大小(callback_queue_size_mb)
  class callback_manager : public lb_common::simple_thread
  {
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
    int32_t init(lb_common::lb_log *log, api_callback *cb, callback_mode mode, int32_t cpu_affinity,
                 int32_t queue_size_mb, int32_t wait_ms);

    /// 启动回调线程(仅队列模式)
    int32_t start();

    /// 回退接口: api_impl::start 失败时回退用
    /// 仅停止已运行的回调线程(未启动为 no-op), 不关闭 cb_queue_ (保持可重新 start);
    /// 线程 join 后复位 simple_thread state, 允许再次 run()
    void stop();

    /// 关闭接口: 用户主动停止 (api_impl::stop 调用)
    /// 停止回调线程并关闭回调队列, 之后不可再 start
    void close();

    /// ---- 回调接口 (柜台/引擎调 → callback_manager, 内部按 mode 同步直调或入队) ----

    /// 登录结果回调 (从 cb_mgr_ 入口)
    /// @param ans  登录应答(含错误码和错误信息)
    void on_login(const LoginAns &ans);

    /// 登出结果回调 (用户主动登出, 纯本地操作, 同步完成)
    /// @param ans  登出应答(含错误码和错误信息)
    void on_logout(const LogoutAns &ans);

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

    /// 最大可委托数查询应答回调 (98 扩展查询)
    void on_max_order_qty_query_ans(const CustMaxOrderQtyInfo &info);

    /// 客户总资产查询应答回调 (98 扩展查询; 按币种多条一次回调)
    void on_total_fund_asset_query_ans(const CustTotalFundAssetInfo *ans_arr, const QueryAnsCtl &ctl);

    /// 可取资金查询应答回调 (98 扩展查询; 按币种多条一次回调)
    void on_fund_maxdraw_query_ans(const CustFundMaxdrawInfo *ans_arr, const QueryAnsCtl &ctl);

    /// 融资融券信用数据(可用保证金)查询应答回调 (98 扩展查询)
    void on_cust_margin_avl_query_ans(const CustMarginAvlInfo &info);

    /// 资产负债查询应答回调 (98 扩展查询)
    void on_asset_debts_query_ans(const AssetDebtsInfo &info);

    /// 两融最大可委托数查询应答回调 (98 扩展查询)
    void on_credit_max_order_qty_query_ans(const CreditMaxOrderQtyInfo &info);

    /// 链接状态变化通知回调
    /// @param counter_type  柜台类型: 0=98柜台, 1=极速柜台
    /// @param link_type  链接类型: 0=网关链接, 1=极速交易链接
    /// @param status        状态: 0=已断线, 1=连接成功
    void on_link_status(int32_t counter_type, int32_t link_type, int32_t status);

    /// 柜台用户状态通知回调 (fpga 柜台用户/板卡状态推送)
    /// @param counter_type      柜台类型: 取值见 counter_type 枚举 (fpga_direct=2 / fpga_gateway=3)
    /// @param branch_id         分支机构代码 (C 串)
    /// @param fund_account_id   资金账号 (C 串)
    /// @param status            状态: 1-正常, 2-故障, 3-故障切换柜台 (G1_FPGA_USER_STATE_* 字典)
    void on_trade_counter(int32_t counter_type, const char *branch_id, const char *fund_account_id, int32_t status);

    /// 通用事件通知回调
    /// @param event_type  事件类型(取值见 api_event_type 枚举)
    /// @param err_code    错误码(0-成功/通知类事件, 非0-错误)
    /// @param err_desc    描述信息(长度 < MAX_ERR_DESC_LEN)
    void on_event(api_event_type event_type, int32_t err_code, const char *err_desc);

  protected:
    /// simple_thread虚函数实现
    void do_work() override;
    bool need_work() override;

  private:
    FORCE_INLINE int64_t write_get(char *&data, int32_t len)
    {
      return cb_queue_.write_get_mth(data, len);
    }

    FORCE_INLINE void write_cmt(int64_t pos, int32_t len)
    {
      cb_queue_.write_cmt_mth(pos, len);
    }

    callback_mode mode_;
    api_callback *user_callback_;
    lb_common::lb_log *log_;          ///< 日志对象(入队失败时打印错误日志)
    lb_common::que_mth_buf cb_queue_; ///< 回调事件队列(队列模式)
  };

} // namespace lb_api

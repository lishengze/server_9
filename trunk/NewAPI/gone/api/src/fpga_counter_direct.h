// fpga_counter_direct - FPGA 直连模式柜台（骨架）
//
// 继承 fpga_counter_base, 直连模式: 单实例、单客户.
// 唯一额外存储: fpga_cust_info client_info_ (单一客户)
// GW 登录成功后才可建立 fpga trade 链接 (trade_ip/port 由 GW 登录应答回填到 client_info_).
//
// 重要：fpga 柜台的多态通过模板特化实现（无 virtual 钩子）

#pragma once

#include "api_config_impl.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "fpga_counter_base.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "mlog.h"
#include "order_trade_type.h"
#include "que_mth_buf.h"

#include <cstdint>

namespace lb_api {

using lb_common::int16;
using lb_common::int32;
using lb_common::int64;
using lb_common::int8;
using lb_common::uint16;
using lb_common::uint32;
using lb_common::uint64;
using lb_common::uint8;

/// FPGA 直连模式柜台 (单客户)
class fpga_counter_direct : public fpga_counter_base {
public:
  FORCE_INLINE int32 get_counter_type() const { return static_cast<int32_t>(counter_type::fpga_direct); }

  /// api instance 调用，init 中
  int32 init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);
  /// api instance 调用，init 中
  void init_trade(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {
    trade_send_queue_ = que;
    trade_eng_op_ = link_outop;
  }
  void init_gateway(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {
    gw_send_queue_ = que;
    gw_eng_op_ = link_outop;
  }

  /// api instance 调用，处理买卖委托
  /// 若 fpga 柜台客户状态不正常，返回柜台离线错误，让上层重新路由到98柜台
  int32 deal_order_req(const OrderReq &req);

  /// api instance 调用，处理ETF申购赎回委托
  /// fpga 柜台不支持，返回柜台不支持的错误，让上层重新路由到98柜台
  int32 deal_etf_order_req(const OrderReq &req);

  /// api instance 调用，处理委托撤单
  int32_t deal_cancel_req(const CancelReq &req);

  /// engine 调用，接收消息处理（由 link 回调, link_type 从哪个类型链接接收）
  /// 可能有多个完整消息+不完整消息，需要依据消息头一个个解析，确认一个个什么业务并处理
  /// 返回成功解析处理的长度
  /// @param link_type 链接类型 (LINK_TYPE_SPEED_GW=来自 GW 链接, LINK_TYPE_SPEED_TRADE=来自 core 链接)
  int32 deal_recv_msg(const char *buf, uint16 len, int16 link_type);

  /// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
  void deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret);

  /// engine 调用，处理账户登陆事件
  /// 成功返回消息长度，含消息头，0-不需重复登陆,<0 出错
  int32 deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len);

  void ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg);

  /// engine 调用，处理链接重连是否可建立链接
  /// @param link_type  链接类型 (LINK_TYPE_SPEED_TRADE/GW)
  bool can_link_connect(int16 link_type) {
    if (LINK_TYPE_SPEED_TRADE == link_type)
      return client_info_.login_state == 2;
    return true;
  }

  /// Link 在 engine 中调用，链接成功时
  /// @param link_type    链接类型
  /// @param have_switch  1=链接建立时发生地址切换 (need_switch=1 + 实际切换)
  /// 返回错误时，底层关闭链接
  int32 deal_link_connect(int16 link_type, int32 have_switch);

  /// Link 在 engine 中调用，链接关闭时
  void deal_link_close(int16 link_type);

  fpga_counter_direct();
  ~fpga_counter_direct();

  /// 禁用拷贝
  fpga_counter_direct(const fpga_counter_direct &) = delete;
  fpga_counter_direct &operator=(const fpga_counter_direct &) = delete;

protected:
  // ---- 派生类内部方法（非"重写基类"！）----
  // 注：这些方法与基类同名但基类中不存在。基类注释已修正。
  /// 处理 FPGA 用户状态消息
  void deal_fpag_state(const fpga_user_state &msg);

  /// 处理 FPGA 账户登录应答 (成功则保存 trade_ip/port, 后续触发 fpga core 链接)
  void deal_log_ans(login_ans &msg);
  int32 delive_fpga_connect();

  /// 派生类内部：证券信息获取完成后检查登录状态
  void check_ans_log(int32 err_code);

private:
  fpga_cust_info client_info_{};                       ///< 单一客户
  lb_common::que_mth_buf *trade_send_queue_ = nullptr; ///< 极速柜台发送队列
  lb_common::que_mth_buf *gw_send_queue_ = nullptr;    ///< gw 链接发送队列
  link_engine_outop *gw_eng_op_ = nullptr;             ///< 98链接引擎导出的链接相关操作
  link_engine_outop *trade_eng_op_ = nullptr;          ///< 极速引擎导出的链接相关操作
};

} // namespace lb_api
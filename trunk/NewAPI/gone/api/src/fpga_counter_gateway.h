// fpga_counter_gateway - FPGA 网关模式柜台
//
// 继承 fpga_counter_base, 网关模式: 多客户、所有业务走同一条 fpga_gw 链接.
// 与 fpga_counter_direct 的主要差异:
//   - 多客户存储 (client_map_ + clients_, 按 user_id 索引)
//   - 无 fpga core 链接, 委托/撤单/回报全部走 fpga_gw 链接
//   - 登录类型 log_type=2 (网关代多客户登录)
//   - login_ans 成功后直接 cb_mgr_->on_login (无 core 同步 connect 步骤)
//   - 证券信息独立获取, 不作为登录回调前提 (未就绪时委托按无证券信息保护性拒绝)
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
#include "hash_comm.h"
#include "hash_map_mth.h"
#include "mlog.h"
#include "que_mth_buf.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace lb_api
{

  using lb_common::int16;
  using lb_common::int32;
  using lb_common::int64;
  using lb_common::int8;
  using lb_common::uint16;
  using lb_common::uint32;
  using lb_common::uint64;
  using lb_common::uint8;

  /// 现实对象：无（多客户索引键：板卡号＋用户号，纯机制）
  struct fpga_fundacc_index
  {
    uint16 board_no;
    uint16 user_id;

    fpga_fundacc_index() {};
    /// 拷贝构造 (按 64-bit 块复制, 避免逐字节)
    fpga_fundacc_index(const fpga_fundacc_index &src)
    {
      board_no = src.board_no;
      user_id = src.user_id;
    }
    fpga_fundacc_index(uint16 tno, uint16 tid)
    {
      board_no = tno;
      user_id = tid;
    }
    /// 拷贝赋值
    fpga_fundacc_index &operator=(const fpga_fundacc_index &src)
    {
      board_no = src.board_no;
      user_id = src.user_id;
      return *this;
    }
    /// 相等比较 (按 64-bit 块)
    bool operator==(const fpga_fundacc_index &src) const
    {
      if (board_no == src.board_no && user_id == src.user_id)
        return true;
      return false;
    }
    /// 析构
    ~fpga_fundacc_index() {};
  };
  /// 现实对象：无（索引键哈希函子，纯机制）
  struct fpga_fundacc_index_hash
  {
    uint32_t operator()(const fpga_fundacc_index &key) const noexcept { return ((key.board_no << 5) | key.user_id); }
  };

  /// FPGA 多客户 hash map (fund_account_id + branch_id -> fpga_cust_info 指针)
  using fpga_client_map_acc = lb_common::hash_map_mth<60, fpga_fundacc_key, fpga_cust_info *, fpga_fundacc_hash>;
  /// FPGA 多客户 hash map (boardno + user_id -> fpga_cust_info 指针)
  using fpga_client_map_index =
      lb_common::hash_map_mth<60, fpga_fundacc_index, fpga_cust_info *, fpga_fundacc_index_hash>;
  /// FPGA 网关模式柜台 (多客户)
  /// 现实对象：fpga 硬件极速柜台（网关模式多客户，经 GW 链接的报文组包/解析）
  class fpga_counter_gateway : public fpga_counter_base
  {
  public:
    /// 现实操作：取本实例 fpga 柜台的柜台类型（恒 fpga）
    FORCE_INLINE int32 get_counter_type() const { return static_cast<int32_t>(counter_type::fpga); }

    /// api instance 调用，init 中
    /// 现实操作：对 fpga 柜台对象的初始化（配置/回调/日志注入）
    int32 init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log);
    /// api instance 调用，init 中 (网关模式无独立 trade 链接, 此处为 no-op)
    /// 现实操作：无（网关模式无独立 trade 链接，接口仅为引擎模板对齐）
    void init_trade(lb_common::que_mth_buf *que, link_engine_outop *link_outop) {}
    /// api instance 调用，init 中 (注入 gw 链接队列与回调)
    /// 现实操作：注入 fpga GW 链接发送队列与引擎操作出口
    void init_gateway(lb_common::que_mth_buf *que, link_engine_outop *link_outop)
    {
      gw_send_queue_ = que;
      gw_eng_op_ = link_outop;
    }

    /// api instance 调用，处理买卖委托
    /// 若 fpga 柜台客户状态不正常，返回柜台离线错误，让上层重新路由到98柜台
    /// fpga_gateway 模式下业务也走 gw_send_queue_
    /// 现实操作：对客户委托单的 fpga 柜台报单（组包入队，业务也走 GW 链接）
    int32 deal_order_req(const OrderReq &req);

    /// api instance 调用，处理委托撤单
    /// 现实操作：对客户撤单请求的 fpga 柜台撤单报单（组包入队，业务也走 GW 链接）
    int32_t deal_cancel_req(const CancelReq &req);

    /// engine 调用，接收消息处理（由 link 回调, link_type 从哪个类型链接接收）
    /// 可能有多个完整消息+不完整消息，需要依据消息头一个个解析，确认一个个什么业务并处理
    /// 返回成功解析处理的长度
    /// @param link_type 链接类型 (网关模式下业务也走 LINK_TYPE_SPEED_GW)
    /// 现实操作：对 fpga 柜台下行报文流的切帧、解析与分发（GW 链接）
    int32 deal_recv_msg(const char *buf, int32 len, int16 link_type);

    /// engine 调用，处理发送消息失败，如对于委托，构建委托rtn 回调通知客户
    /// 现实操作：对发送失败报单的 API 层拒绝回报构造与回调
    void deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret);

    /// engine 调用，处理账户登陆事件
    /// 成功返回消息长度，含消息头，0-不需重复登陆,<0 出错
    /// 现实操作：对客户登录事件的 fpga 网关代登录报单（组包）
    int32 deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len);

    /// engine 调用，账户登陆同步失败时回调通知客户
    /// 现实操作：对登录阶段失败事件的失败回调
    void ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg);

    /// api_impl 登出编排调用: 柜台侧登出 —— 状态→LOGIN_STATE_LOGOUT、清在途;
    /// 共享 GW 链接不关闭; 未登录/未入库幂等无动作
    /// 现实操作：对客户会话的柜台侧登出（置 LOGOUT、清在途；共享 GW 链接不关闭）
    int32_t deal_logout_req(const LogoutReq &req);

    /// engine 调用，处理链接重连是否可建立链接
    /// @param link_type  链接类型 (LINK_TYPE_SPEED_GW)
    /// 现实操作：对 fpga GW 链接可建性判定
    bool can_link_connect(int16 link_type)
    {
      if (link_type == LINK_TYPE_SPEED_GW)
      {
        return true;
      }
      return false;
    }

    /// Link 在 engine 中调用，链接成功时
    /// @param link_type    链接类型
    /// @param have_switch  1=链接建立时发生地址切换 (need_switch=1 + 实际切换)
    /// 返回错误时，底层关闭链接
    /// 现实操作：对 fpga GW 链接建立成功的处置（证券信息请求投递、已登录客户批量重登、链接状态回调）
    int32 deal_link_connect(int16 link_type, int32 have_switch);

    /// Link 在 engine 中调用，链接关闭时
    /// 现实操作：对 fpga GW 链接断开的处置（客户在途清理、证券信息状态复位、链接状态回调）
    void deal_link_close(int16 link_type);

    /// 现实操作：无
    fpga_counter_gateway();
    /// 现实操作：无
    ~fpga_counter_gateway();

    /// 禁用拷贝
    fpga_counter_gateway(const fpga_counter_gateway &) = delete;
    fpga_counter_gateway &operator=(const fpga_counter_gateway &) = delete;

  protected:
    // ---- 派生类内部方法（非"重写基类"！）----
    /// 处理 FPGA 用户状态消息
    void deal_fpag_state(const fpga_user_state &msg);

    /// 处理 FPGA 账户登录应答 (gateway 模式无 core 链接, 登录成功即回调; 证券信息独立获取不作为前提)
    void deal_log_ans(login_ans &msg);

    /// 获取单个客户信息（网关模式：查 client_map_）
    /// @param branch_len/fund_len 源缓冲长度 (98/api 侧 branch 为 10 字节, fpga 协议侧为 12 字节;
    ///        定长构造按 16/12 字节读取, 经带长度构造函数补零构造避免越界读)
    int32 get_client_info(fpga_cust_info *&o_info, const char *branch_id, int32 branch_len,
                          const char *fund_account_id, int32 fund_len);

  private:
    lb_common::que_mth_buf *gw_send_queue_ = nullptr; ///< gw 链接发送队列
    link_engine_outop *gw_eng_op_ = nullptr;          ///< 98链接引擎导出的链接相关操作 (gw 链接)
    // ---- 多客户存储 ----
    fpga_client_map_acc client_map_;            ///< 资金账号+分支机构 ->  user_id+board_no, 发送查询（参数needlock=1, 与引擎线程insert并发）
    fpga_client_map_index clients_;             ///< boardno+userid -> fpga_cust_info 指针，接收查询用（参数needlock=1）
    std::vector<fpga_cust_info *> clients_vec_; ///< 仅用于回收
  };

} // namespace lb_api

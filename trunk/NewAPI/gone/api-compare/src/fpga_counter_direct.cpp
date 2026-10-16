// fpga_counter_direct - FPGA 直连模式柜台实现
//
// 继承 fpga_counter_base, 直连模式: 单实例、单客户.
// 唯一额外存储: fpga_cust_info client_info_ (单一客户).
// GW 登录成功后才可建立 fpga core 链接 (trade_ip/port 由 GW 登录应答回填到 client_info_).
//
// 委托/撤单流程:
//   1. 取队列内存 (que_mth_buf::write_get_mth)
//   2. 在该内存中直接构造 link_send_event + g1_msg_head + order_req/cancel_req
//   3. 提交写入 (write_cmt_mth)
//   队列满立即返回错误, 不阻塞.
#include "fpga_counter_direct.h"

// 注意: api_errno.h 必须在 fpga_counter_direct.h 之前包含 (依赖 api_event_type).
#include "api_callback.h"
#include "api_config.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "counter98.h"
#include "fpga_counter_base.h"
#include "fpga_counter_direct.h"
#include "g1dictdefine.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "matomic.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

#include <cstring>

namespace lb_api
{

  // ---- 构造 / 析构 ----
  fpga_counter_direct::fpga_counter_direct() { std::memset(&client_info_, 0, sizeof(client_info_)); }

  fpga_counter_direct::~fpga_counter_direct() = default;

  // init: 初始化基类 + 业务字段
  int32 fpga_counter_direct::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log)
  {

    int32 ret = init_base(cfg, cb, log);
    if (ret < 0)
    {
      return ret;
    }

    std::memset(&client_info_, 0, sizeof(client_info_));

    lb_common::lb_log_hand tlh(log);
    info_log(tlh) << "init fpga_counter_direct ok, market_type=" << static_cast<int32_t>(cfg.get_market_type())
                  << end_log;

    return 0;
  }

  // ---- deal_order_req: 买卖委托 (客户下线返回 COUNTER_OFFLINE, 供上层降级 98) ----
  // 直连模式业务请求只依赖 fpga 核心链接 (REQ: 网关登录状态不阻塞 —— GW 断线重登期间
  // core 正常则交易不受影响, GW 登录状态仅控制登录流程与 core 建链)
  int32 fpga_counter_direct::deal_order_req(const OrderReq &req)
  {
    // FPGA 支持域二道校验 (RUL-23 精简后仅 side 买卖; 市场/限价由 api_impl 一道校验保证,
    // 防线依赖说明见 fpga_counter_base.h check_order_dict)
    int32 tdict = check_order_dict(req);
    if (unlikely(tdict != 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_order_req: argv wrong,branch_id=" << req.branch_id.data()
                     << ",fund_account=" << req.fund_account_id.data() << ",client_seq_id=" << req.client_seq_id << end_log;
      return tdict;
    }

    // 账户一致性校验(单客户绑定): 已绑定账户后, 异账户请求一律拒绝,
    // 防止错号委托静默归属已登录账户 (与 deal_cust_login / deal_logout_req 同判)
    bool same_acc =
        std::strncmp(client_info_.fund_account_id, req.fund_account_id.data(), sizeof(req.fund_account_id)) == 0 &&
        std::strncmp(client_info_.branch_id, req.branch_id.data(), sizeof(req.branch_id)) == 0;
    if (unlikely(!same_acc && client_info_.fund_account_id[0] != '\0'))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_order_req: account not match,login_branch_id=" << client_info_.branch_id
                     << ",login_fund_account=" << client_info_.fund_account_id << ",req_branch_id=" << req.branch_id.data()
                     << ",req_fund_account=" << req.fund_account_id.data() << ",client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NO_CUST;
    }
    if (unlikely(lb_common::atomic_load32(&client_info_.fpga_state) != G1_FPGA_USER_STATE_NORMAL))
    {
      lb_common::lb_log_hand tlh(log_);
      debug_log(tlh) << "fpga_direct deal_order_req: cust fault, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_COUNTER_OFFLINE;
    }
    if (unlikely(!client_info_.log_state.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_order_req: not login, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }
    if (unlikely(lb_common::atomic_load32(&trade_link_connect) == 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_order_req: not connected, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_LINK_DISCONNECTED;
    }

    // 2. 查询 sec_index
    uint16_t sec_index = 0;
    int32_t ret = get_sec_index(req.security_id.data(), sec_index);
    if (unlikely(ret != 0))
    {
      return ret;
    }

    // 4. 申请队列内存, 直接构造 link_send_event + g1_msg_head + order_req
    int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(order_req));
    char *data = nullptr;
    int64 pos = trade_send_queue_->write_get_mth(data, total_len);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                     << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    // 5. 在队列内存中直接构造 (不复制中间 buf, 省一次 memcpy)
    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_SPEED_TRADE;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(order_req));

    g1_msg_head *head = reinterpret_cast<g1_msg_head *>(evt->data);
    build_order_msg(req, client_info_, sec_index, head);

    // 6. 提交
    trade_send_queue_->write_cmt_mth(pos, total_len);

    return LBAPI_OK;
  }

  // deal_etf_order_req: ETF 申购赎回 (fpga 柜台不支持, 返回柜台不支持错误)
  int32 fpga_counter_direct::deal_etf_order_req(const OrderReq &req)
  {
    return LBAPI_ERR_UNSUPPORTED_OP; // fpga 不支持 ETF, 上层降级到 98
  }

  // deal_cancel_req: 委托撤单 (直连模式请求前提同 deal_order_req: 只依赖 fpga 核心链接)
  int32_t fpga_counter_direct::deal_cancel_req(const CancelReq &req)
  {
    // FPGA 仅支持系统委托号撤单: order_sys_no==0 本地拒绝 (RUL-36)
    int32 tdict = check_cancel_dict(req);
    if (unlikely(tdict != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_cancel_req: order_sys_no required (fpga not support private-no cancel), "
                        "client_req_no="
                     << req.client_req_no << ", client_seq_id=" << req.client_seq_id << end_log;
      return tdict;
    }

    // 账户一致性校验(单客户绑定): 同 deal_order_req, 已绑定账户后异账户撤单一律拒绝
    bool same_acc =
        std::strncmp(client_info_.fund_account_id, req.fund_account_id.data(), sizeof(req.fund_account_id)) == 0 &&
        std::strncmp(client_info_.branch_id, req.branch_id.data(), sizeof(req.branch_id)) == 0;
    if (unlikely(!same_acc && client_info_.fund_account_id[0] != '\0'))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_cancel_req: account not match,login_branch_id=" << client_info_.branch_id
                     << ",login_fund_account=" << client_info_.fund_account_id << ",req_branch_id=" << req.branch_id.data()
                     << ",req_fund_account=" << req.fund_account_id.data() << ",client_req_no=" << req.client_req_no << end_log;
      return LBAPI_ERR_NO_CUST;
    }
    if (unlikely(lb_common::atomic_load32(&client_info_.fpga_state) != G1_FPGA_USER_STATE_NORMAL))
    {
      lb_common::lb_log_hand tlh(log_);
      debug_log(tlh) << "fpga_direct deal_cancel_req: cust fault, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_COUNTER_OFFLINE;
    }
    if (unlikely(!client_info_.log_state.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_cancel_req: not login, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }
    if (unlikely(lb_common::atomic_load32(&trade_link_connect) == 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_cancel_req: not connected, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_LINK_DISCONNECTED;
    }

    int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(cancel_req));
    char *data = nullptr;
    int64 pos = trade_send_queue_->write_get_mth(data, total_len);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_direct deal_cancel_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                     << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_SPEED_TRADE;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(cancel_req));

    g1_msg_head *head = reinterpret_cast<g1_msg_head *>(evt->data);
    build_cancel_msg(req, client_info_, head);

    trade_send_queue_->write_cmt_mth(pos, total_len);

    return LBAPI_OK;
  }

  // deal_recv_msg: 收到 fpga 消息 (来自 GW 链接 / core 链接)
  // 可能有多个完整消息+不完整消息, 依据消息头一个个解析
  int32 fpga_counter_direct::deal_recv_msg(const char *buf, int32 len, int16 link_type)
  {
    int32 deal_len = 0;
    int32 mlen = 0;
    const g1_msg_head *head;

    while (static_cast<int32>(len) - deal_len >= static_cast<int32>(sizeof(g1_msg_head)))
    {
      head = reinterpret_cast<const g1_msg_head *>(buf + deal_len);
      // msg_len 为 uint32: 先按无符号校验上界, 防止 (int32)msg_len 为负时解析指针向缓冲区前方游走
      if (unlikely(head->msg_len > (uint32_t)(G1_MSG_MAX_LEN - sizeof(g1_msg_head))))
      {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga counter recv msg len error, msg_len=" << head->msg_len << ", link_type=" << link_type
                       << end_log;
        // 返回错误，让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
      mlen = static_cast<int32>(head->msg_len) + static_cast<int32>(sizeof(g1_msg_head));
      if (unlikely(mlen > static_cast<int32>(len) - deal_len))
      {
        return deal_len;
      }

      switch (head->msg_id)
      {
      case G1_MSG_ORDER_RTN:
      {
        deal_order_rtn(head, client_info_, static_cast<int32>(counter_type::fpga_direct));
        break;
      }
      case G1_MSG_TRADE_RTN:
      {
        deal_trade_rtn(head, client_info_, static_cast<int32>(counter_type::fpga_direct));
        break;
      }
      case G1_MSG_CANCEL_RSP:
      {
        deal_cancel_rsp(head, client_info_, static_cast<int32>(counter_type::fpga_direct));
        break;
      }
      case G1_MSG_HEART_ANS:
      {
        // 心跳应答: 通知 link_engine
        if (link_type == LINK_TYPE_SPEED_TRADE)
        {
          if (trade_eng_op_ != nullptr)
            trade_eng_op_->deal_heart_msg_ans(LINK_TYPE_SPEED_TRADE);
        }
        else
        {
          if (gw_eng_op_ != nullptr)
            gw_eng_op_->deal_heart_msg_ans(LINK_TYPE_SPEED_GW);
        }
        break;
      }
      case G1_MSG_SEC_INFO_ANS:
      {
        if (deal_sec_info_ans(head) < 0)
        {
          gw_eng_op_->deal_close_link(LINK_TYPE_SPEED_GW, LBAPI_ERR_NO_SEC);
        }
        break;
      }
      case G1_MSG_LOGIN_ANS:
      {
        if (head->msg_len >= sizeof(login_ans))
        {
          login_ans *ans = const_cast<login_ans *>(reinterpret_cast<const login_ans *>(head + 1));
          deal_log_ans(*ans);
        }
        else
        {
          // 短于本版本已知最小长度: 协议不可兼容, 通知用户销毁 API; 跳过该消息继续解析
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fpga counter recv login_ans msg len too short, msg_len=" << head->msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "login_ans msg len too short");
        }
        break;
      }
      case G1_MSG_OFFLINE_PUSH:
      {
        if (head->msg_len >= sizeof(fpga_user_state))
        {
          const fpga_user_state *st = reinterpret_cast<const fpga_user_state *>(head + 1);
          deal_fpag_state(*st);
        }
        else
        {
          // 短于本版本已知最小长度: 协议不可兼容, 通知用户销毁 API; 跳过该消息继续解析
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fpga counter recv offline_push msg len too short, msg_len=" << head->msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "offline_push msg len too short");
        }
        break;
      }
      case G1_MSG_GW_REJ:
      {
        // 网关路由拒绝: g1_msg_head + g1_gw_rej_head + (order_req | cancel_req)
        // 仅 fpga 柜台单客户, 用唯一的 client_info_
        deal_gw_rej(head, client_info_, static_cast<int32>(counter_type::fpga_direct));
        break;
      }
      default:
      {
        // 未知消息, 跳过，不返回错误，以避免链接关闭，保证旧api对新增消息的兼容性
        lb_common::lb_log_hand tlh(log_);
        info_log(tlh) << "fpga counter recv unknown msg, msg_id=" << head->msg_id << end_log;
        break;
      }
      }

      deal_len += mlen;
    }

    return deal_len;
  }

  // deal_send_error: 发送消息失败时处理 (如委托撤单, 构建 rtn 回调通知客户)
  void fpga_counter_direct::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret)
  {
    const g1_msg_head *head = reinterpret_cast<const g1_msg_head *>(msg_buf);
    switch (head->msg_id)
    {
    case G1_MSG_ORDER_REQ:
    {
      // 委托发送失败: 构造 OrderRtn 通知客户
      OrderRtn rtn;
      StreamInfo stream;
      build_api_order_rej(head, client_info_, get_counter_type(), err_ret, rtn, stream);

      cb_mgr_->on_order_rtn(stream, rtn);
      break;
    }
    case G1_MSG_CANCEL_REQ:
    {
      // 撤单发送失败: 构造 CancelRsp 通知客户
      CancelRsp rtn;
      StreamInfo stream;
      build_api_cancel_rej(head, client_info_, get_counter_type(), err_ret, rtn, stream);

      cb_mgr_->on_cancel_rsp(stream, rtn);
      break;
    }
    default:
      break;
    }
  }

  // deal_cust_login: 处理账户登录事件, 构造 fpga 登录消息发送 (log_state_ctl 状态机统一判定)
  // 成功返回消息长度 (含消息头), 失败返回负数，0-不需重复登陆(幂等/合并/静默终止/下线降级)
  int32 fpga_counter_direct::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len)
  {
    lb_common::lb_log_hand tlh(log_);

    // strncmp 在首个\0处停止: 比较有效字符, 免疫两侧\0后的尾部差异(规范形不变式之外的双保险)
    bool same_acc =
        std::strncmp(client_info_.fund_account_id, req.fund_account_id, sizeof(req.fund_account_id)) == 0 &&
        std::strncmp(client_info_.branch_id, req.branch_id, sizeof(req.branch_id)) == 0;

    if (!same_acc && (!client_info_.log_state.is_free() || client_info_.fund_account_id[0] != '\0'))
    {
      // fpga 直连不支持换账户登录
      // client_info_ 绑定过账户(曾成功登录, 含已登出)后, 异账户登录一律拒绝 ——
      // fgw 侧"同连接不可换号"不变式维持; 同账户流程不受影响, 从未登录过则首登不受限
      error_log(tlh) << "fpga user login but account not match, not support switch,old_branch_id="
                     << client_info_.branch_id << ",old_fund_account=" << client_info_.fund_account_id
                     << ",new_branch_id=" << req.branch_id << ",new_fund_account=" << req.fund_account_id
                     << ",client_req_no=" << req.cust_req_no << end_log;
      LoginAns ans;
      build_login_rtn(req, LBAPI_ERR_NOT_LOG_CUST, "fpga direct login,account not match", ans);
      cb_mgr_->on_login(ans);
      return LBAPI_ERR_NOT_LOG_CUST;
    }

    // FPGA 下线 (fpga_state 非正常: 故障/故障切换柜台) → 不登录 FPGA, 98 登录成功即成功。
    if (same_acc && lb_common::atomic_load32(&client_info_.fpga_state) != G1_FPGA_USER_STATE_NORMAL)
    {
      int32 ret = client_info_.log_state.clear_pending();
      if (req.user_origin == 1 || ret == 1)
      {
        info_log(tlh) << "fpga user offline, login ok by 98 degraded,branch_id=" << req.branch_id
                      << ",fund_account=" << req.fund_account_id << ",client_req_no=" << req.cust_req_no << end_log;
        LoginAns ans;
        build_login_rtn(req, 0, NULL, ans);
        cb_mgr_->on_login(ans);
      }
      return 0;
    }

    // 状态机统一判定:
    //   用户: 已登录→幂等回调成功 / 用户在途→合并 / 否则占位 USER 发送;
    //   内部(重连投递): 非登录成功态→静默终止 / 用户在途→合并 / 否则置 RELOGIN 发送(状态保持)
    int32 thavelog = 0;
    int32 tret = client_info_.log_state.to_login(thavelog, req.user_origin);
    if (tret == 0)
    {
      if (thavelog == 1)
      {
        // 幂等: 登录成功状态下用户重复 login → 直接回调成功 (含 offline 降级场景, 不发后台)
        info_log(tlh) << "fpga user have login ok,branch_id=" << req.branch_id
                      << ",fund_account=" << req.fund_account_id << ",session=" << req.session
                      << ",client_req_no=" << req.cust_req_no << ",user_origin=" << req.user_origin << end_log;
        LoginAns ans;
        build_login_rtn(req, 0, NULL, ans);
        cb_mgr_->on_login(ans);
      }
      return 0; ///< 合并(在途等待首应答)/内部事件静默终止
    }

    std::memcpy(agw_session_, req.session, sizeof(agw_session_));

    int32 msg_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(login_req));
    if (buf_len < msg_len)
    {
      error_log(tlh) << "fpga user login and msg len error,branch_id=" << req.branch_id
                     << ",fund_account=" << req.fund_account_id << ",session=" << req.session
                     << ",client_req_no=" << req.cust_req_no << end_log;
      ans_cust_login(req, LBAPI_ERR_BUILD_MSG, "build fast counter login msg error");
      return LBAPI_ERR_MSG_LEN;
    }
    g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
    // log_type=1 表示用户登录 (fpga_direct 是单客户, 使用用户登录)
    build_login_msg(req, 1, head);
    return msg_len;
  }

  // ans_cust_login: 引擎同步阶段失败 (connect/组包/发送失败) —— 状态机 clear_pending 作废在途:
  // 原用户在途 → 回调失败 (状态位本为 NONE); 原内部重登在途 → 静默回退(保登录状态, 下次重连重试);
  // 关链保留 (引擎 connect/发送失败意味着链路不可用, 关链走重连恢复)
  void fpga_counter_direct::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg)
  {
    LoginAns ans;
    build_login_rtn(req, err_ret, err_msg, ans);

    // 由于重连重登时，后台登录失败，回调客户，所以异步处理失败，也应回调客户
    int32 tpending = client_info_.log_state.clear_pending();
    if (tpending != 0)
    {
      cb_mgr_->on_login(ans);
    }

    if (gw_eng_op_ != nullptr)
    {
      gw_eng_op_->deal_close_link(LINK_TYPE_SPEED_GW, err_ret);
    }
  }

  // deal_log_ans: 处理 FPGA 账户登录应答
  // fpga_direct 模式: 提取 trade_ip/port/user_id, 投递 FPGA_CORE_CONNECT 事件到 fast_engine
  void fpga_counter_direct::deal_log_ans(login_ans &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv fpga cust login answer msg,user_seq_no=" << msg.cust_req_no << ",branch_id=" << msg.branch_id
                  << ",fund_account=" << msg.fund_account_id << ",session" << msg.session << ",user_id=" << msg.user_id
                  << ",board_no=" << msg.board_no << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port
                  << ",login_time=" << msg.login_time << ",version=" << msg.version << ",sec_state=" << sec_state_
                  << end_log;

    LoginAns ans;
    build_login_rtn(msg, ans);

    if (msg.err_code == 0)
    {
      // 保存客户信息,要求后台登录成功返回的信息幂等：
      //     对于同一个 agwuser + 账户,返回的 session id 不变
      //     对于同一个 账户,返回的 boardno , userid ,ip ,port不变
      save_client_info(msg, client_info_);

      int32 tcb = client_info_.log_state.ans_login_ok();

      if (client_info_.log_state.is_logined())
      {
        // 发起 fpga 核心链接: 失败仅日志, timer 重拨兜底 (core 建链结果不参与登录回调)
        // 链接建立自带状态控制和fpga 下线控制。
        int32 ret = delive_fpga_connect();
        if (ret == 0)
        {
          info_log(tlh) << "user login answer to delive fpga connect ok, branch_id=" << msg.branch_id
                        << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                        << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port << end_log;
        }
        else
        {
          error_log(tlh) << "user login answer to delive fpga connect error, branch_id=" << msg.branch_id
                         << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                         << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port << ",ret=" << ret << end_log;
        }
      }

      // 状态机置成功 + 回调判定: 用户在途 → 回调一次; 内部重登/断链作废后的迟到成功 → 静默
      if (tcb == 1)
      {
        cb_mgr_->on_login(ans);
      }
    }
    else
    {
      // 失败应答: 状态机统一处理 (USER/RELOGIN → 清状态 NONE + 回调失败[RELOGIN 曾成功客户
      // 重登被拒视为异常, 不再自动重登]; LOGOUT/无在途迟到 → 忽略)
      // 登录失败一律不关链 (失败后重连不再重登, 关链无自愈意义)
      if (client_info_.log_state.ans_login_err() == 1)
      {
        error_log(tlh) << "fpga cust login failed, branch_id=" << msg.branch_id
                       << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                       << ",err_code=" << ans.err_code << ",err_msg=" << ans.err_msg.data()
                       << ",version=" << msg.version << ",sec_state=" << sec_state_ << end_log;
        cb_mgr_->on_login(ans);
      }
      else
      {
        // 无在途迟到失败 (旧请求应答迟到, 在途已被首应答/断链清除): 忽略
        warning_log(tlh) << "ignore late fpga cust login fail ans without pending,branch_id=" << msg.branch_id
                         << ",fund_account=" << msg.fund_account_id << ",err_code=" << ans.err_code << end_log;
      }
    }
  }

  int32 fpga_counter_direct::delive_fpga_connect()
  {
    int32 evt_len = static_cast<int32>(sizeof(link_send_event) + sizeof(fpga_core_connect_info));
    char *data = nullptr;
    int64 pos = trade_send_queue_->write_get_mth(data, evt_len);
    if (pos <= 0)
    {
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_SPEED_TRADE;
    evt->type = LINK_EVENT_TYPE_FPGA_CORE_CONNECT;
    evt->data_len = sizeof(fpga_core_connect_info);
    fpga_core_connect_info *info = reinterpret_cast<fpga_core_connect_info *>(evt->data);
    info->trade_port = client_info_.trade_port;
    std::memcpy(info->trade_ip, client_info_.trade_ip, sizeof(info->trade_ip));

    trade_send_queue_->write_cmt_mth(pos, evt_len);
    return 0;
  }

  // deal_fpag_state: 处理 FPGA 客户状态消息 (板卡级/单客户状态推送)
  // 状态更新后统一经 on_trade_counter 回调用户 (含恢复 state=1);
  // 故障/故障切换柜台(2/3)均为柜台不可用态: 关闭 core 链接 (原仅 state==2 关链, 扩展到 3)
  void fpga_counter_direct::deal_fpag_state(const fpga_user_state &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv fpga user state msg,board_no=" << msg.board_no << ",user_id=" << msg.user_id
                  << ", state=" << msg.state << ", branch_id=" << msg.branch_id
                  << ", fund_account=" << msg.fund_account_id << end_log;

    // 查找对应客户, 更新 fpga_state
    if (msg.fund_account_id[0] != '\0')
    {
      // 比较资金账号: strncmp 按 C 串比较有效字符, 免疫 wire 字段\0后的尾部差异
      if (std::strncmp(msg.fund_account_id, client_info_.fund_account_id,
                       std::min<size_t>(sizeof(msg.fund_account_id), sizeof(client_info_.fund_account_id))) == 0)
      {
        lb_common::atomic_store32(&client_info_.fpga_state, msg.state);
      }
      else
      {
        return;
      }
    }
    else
    {
      // 没有 fund_account_id (板卡级通知), 直连单客户默认更新本客户
      lb_common::atomic_store32(&client_info_.fpga_state, msg.state);
    }

    // 状态通知回调: 消息用户字段为空(板卡级)时取本客户 client_info_ 的账号属性
    if (cb_mgr_ != nullptr)
    {
      cb_mgr_->on_trade_counter(static_cast<int32_t>(counter_type::fpga_direct),
                                msg.branch_id[0] != '\0' ? msg.branch_id : client_info_.branch_id,
                                msg.fund_account_id[0] != '\0' ? msg.fund_account_id : client_info_.fund_account_id,
                                msg.state);
    }

    // 故障(2)/故障切换柜台(3): 均为柜台不可用态
    // 下线为独立故障维度, **不清除登录状态**(§3.2.0);
    // 用户 login() 命中 offline 幂等分支回成功(98 降级), 委托/撤单按 COUNTER_OFFLINE 降级 98;
    // 恢复推送(state=1)后: 登录状态仍 OK → core 链接 timer 自动重拨 + GW 重连自动重登, 闭环恢复
    if (msg.state == G1_FPGA_USER_STATE_FAULT || msg.state == G1_FPGA_USER_STATE_SWITCH)
    {
      // 关闭 core 链接 (fgw 侧会话不可用, 留链无益; can_link_connect(TRADE) 按正常态门控重拨)
      trade_eng_op_->deal_close_link(LINK_TYPE_SPEED_TRADE, LBAPI_ERR_COUNTER_OFFLINE);
    }
  }

  // ---- deal_link_connect: 链接建立成功通知 ----
  int32 fpga_counter_direct::deal_link_connect(int16 link_type, int32 have_switch)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "fpga link connect, link_type=" << link_type << ",have_switch=" << have_switch << end_log;

    int32_t tl_type = 0;
    if (link_type == LINK_TYPE_SPEED_TRADE)
    {
      tl_type = 1;
      lb_common::atomic_store32(&trade_link_connect, 1);
    }
    else
    {
      //  网关链接: 首连或断线重连后 (重新) 发起证券信息请求
      {
        int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(sec_info_req));
        char *data = nullptr;
        int64 pos = gw_send_queue_->write_get_mth(data, total_len);
        if (unlikely(pos <= 0))
        {
          error_log(tlh) << "fpga_direct delive sec req event queue full, ret=" << pos << end_log;
          cb_mgr_->on_event(api_event_type::get_sec_info, LBAPI_ERR_SEND_QUEUE_FULL, "delive sec req to queue error");
          // 返回错误，底层关闭链接
          return LBAPI_ERR_SEND_QUEUE_FULL;
        }

        sec_state_ = 1;
        link_send_event *evt = reinterpret_cast<link_send_event *>(data);
        evt->link_type = LINK_TYPE_SPEED_GW;
        evt->type = LINK_EVENT_TYPE_SEND_MSG;
        evt->data_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(sec_info_req));

        g1_msg_head *head = reinterpret_cast<g1_msg_head *>(evt->data);
        build_sec_info_req_msg(0, head);

        gw_send_queue_->write_cmt_mth(pos, total_len);
        gw_eng_op_->trigger_send();
      }

      // 重登资格 = 登录成功状态 && 柜台正常态 (§3.2.5; 故障/故障切换柜台均不可用)
      if (client_info_.log_state.is_logined() &&
          lb_common::atomic_load32(&client_info_.fpga_state) == G1_FPGA_USER_STATE_NORMAL)
      {
        acc_login_event_info info;
        build_login_event(client_info_, info);
        // 在途/状态控制统一在 deal_cust_login 的状态机 to_login 中处理, 此处只投递事件
        int32 ret = delive_login_event(info, gw_send_queue_, gw_eng_op_);
        if (ret == 0)
        {
          info_log(tlh) << "fpga_direct gateway link reconnected, re-deliver cust login, branch_id=" << client_info_.branch_id
                        << ",fund_account=" << client_info_.fund_account_id << end_log;
        }
        else
        {
          error_log(tlh) << "fpga_direct re-login delive error,  branch_id=" << client_info_.branch_id
                         << ",fund_account=" << client_info_.fund_account_id << ",ret = " << ret << end_log;
          // 返回错误，底层关闭链接
          return ret;
        }
      }
    }
    if (cb_mgr_ != nullptr)
    {
      cb_mgr_->on_link_status(1, tl_type, 1);
    }
    return 0;
  }

  // ---- deal_link_close: 链接关闭通知 ----
  void fpga_counter_direct::deal_link_close(int16 link_type)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "fpga link close, link_type=" << link_type << end_log;

    int32_t tl_type = 0;
    if (link_type == LINK_TYPE_SPEED_TRADE)
    {
      tl_type = 1;
      lb_common::atomic_store32(&trade_link_connect, 0);
    }
    if (link_type == LINK_TYPE_SPEED_GW)
    {
      if (sec_state_ == 1)
      {
        cb_mgr_->on_event(api_event_type::get_sec_info, LBAPI_ERR_LINK_DISCONNECTED, "sec not complete by link close");
        sec_state_ = 0;
      }

      client_info_.log_state.clear_pending();
    }

    if (cb_mgr_ != nullptr)
    {
      cb_mgr_->on_link_status(1, tl_type, 0);
    }
  }

  // deal_logout_req: 柜台侧登出 (状态机 to_logout 统一判定)
  // 已绑定账户不匹配 → NO_CUST; 用户在途登录 → STATE_LIMITED;
  // 返回 1 → 置 LOGOUT 成功(关闭 core 链); 0 → 本来就未登录/未绑定 → 幂等成功
  // ("登录成功过"由 98 层门控); 不影响 GW 链接
  int32_t fpga_counter_direct::deal_logout_req(const LogoutReq &req)
  {
    bool same_acc =
        std::strncmp(client_info_.fund_account_id, req.fund_account_id.data(), sizeof(req.fund_account_id)) == 0 &&
        std::strncmp(client_info_.branch_id, req.branch_id.data(), sizeof(req.branch_id)) == 0;

    // 账户匹配拒绝仅对已绑定账户生效: client_info_ 从未绑定 (offline 降级 / FPGA 登录
    // 从未成功) 时不做匹配拒绝, 走 to_logout 幂等成功 (状态本为 NONE),
    // 与 gateway "对象未入库 → 登出成功" 对称
    if (!same_acc && client_info_.fund_account_id[0] != '\0')
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga user login buf cust not match,old_branch_id=" << client_info_.branch_id
                     << ",old_fund_account=" << client_info_.fund_account_id << ",new_branch_id=" << req.branch_id.data()
                     << ",new_fund_account=" << req.fund_account_id.data() << ",client_req_no=" << req.client_req_no << end_log;
      return LBAPI_ERR_NO_CUST;
    }

    int32_t tret = client_info_.log_state.to_logout();
    if (tret < 0)
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "cust logout but now logining,branch_id=" << client_info_.branch_id
                     << ",fund_account=" << client_info_.fund_account_id << end_log;
      return LBAPI_ERR_STATE_LIMITED;
    }
    if (tret == 1)
    {
      // 关闭 fpga core 链接 (会话与账户绑定; core 重连门 is_logined 拒绝重拨,
      // 重新 login 成功后由 login_ans 路径重建)
      if (trade_eng_op_ != nullptr)
      {
        trade_eng_op_->deal_close_link(LINK_TYPE_SPEED_TRADE, LBAPI_ERR_NOT_LOG_CUST);
      }

      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "fpga_direct cust logout ok,branch_id=" << client_info_.branch_id
                    << ",fund_account=" << client_info_.fund_account_id << end_log;
    }
    return 0;
  }

} // namespace lb_api
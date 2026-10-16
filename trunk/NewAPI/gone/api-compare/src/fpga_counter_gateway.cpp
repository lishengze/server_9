// fpga_counter_gateway - FPGA 网关模式柜台实现
//
// 继承 fpga_counter_base, 网关模式: 多客户、所有业务走同一条 fpga_gw 链接.
// 与 direct 的主要差异:
//   - 多客户存储 (client_map_ + clients_, 按 user_id 索引)
//   - 无 fpga core 链接, 委托/撤单/回报全部走 fpga_gw 链接
//   - login_ans 成功后直接 on_login (无 core 同步 connect 步骤)
//
// 委托/撤单流程:
//   1. 按 fund_account_id + branch_id 查 user_id (-> fpga_cust_info)
//   2. 取 gw_send_queue_ 内存
//   3. 在该内存中直接构造 link_send_event + g1_msg_head + order_req/cancel_req
//   4. 提交写入 (write_cmt_mth)
//   队列满立即返回错误, 不阻塞.
#include "fpga_counter_gateway.h"

// 注意: api_errno.h 必须在 fpga_counter_gateway.h 之前包含 (依赖 api_event_type).
#include "api_callback.h"
#include "api_config.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "fpga_counter_base.h"
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
  fpga_counter_gateway::fpga_counter_gateway() = default;

  fpga_counter_gateway::~fpga_counter_gateway()
  {
    // clients_ 持有 new 出来的 fpga_cust_info*, 析构时需逐个释放, 避免泄漏
    for (uint32 i = 0; i < clients_vec_.size(); i++)
    {
      delete clients_vec_[i];
    }
    clients_.clear();
    client_map_.clear();
  }

  // init: 初始化基类 + 多客户存储
  int32 fpga_counter_gateway::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log)
  {
    lb_common::lb_log_hand tlh(log);

    int32 ret = init_base(cfg, cb, log);
    if (ret < 0)
    {
      return ret;
    }

    // 预分配 hash map 容量 (cfg 未提供最大客户数, 使用默认值)
    // 多板卡网关场景: 板卡不连续、每板 user_id 从 1 开始, 用 hash_map_mth 而非 vector
    ret = client_map_.init(256);
    if (ret < 0)
    {
      error_log(tlh) << "init fpga_counter_gateway client map error, ret=" << ret << end_log;
      return LBAPI_ERR_ALLOC_MEM;
    }
    ret = clients_.init(256);
    if (ret < 0)
    {
      error_log(tlh) << "init fpga_counter_gateway client index map error, ret=" << ret << end_log;
      return LBAPI_ERR_ALLOC_MEM;
    }
    info_log(tlh) << "init fpga_counter_gateway ok, market_type=" << static_cast<int32_t>(cfg.get_market_type())
                  << end_log;

    return 0;
  }

  int32 fpga_counter_gateway::get_client_info(fpga_cust_info *&o_info, const char *branch_id, int32 branch_len,
                                              const char *fund_account_id, int32 fund_len)
  {
    // 网关模式: (fund_account_id, branch_id) -> (board_no, user_id) -> fpga_cust_info*
    // 用户线程(发送)与引擎线程(登录 insert_new)并发访问, 必须加读锁 (needlock=1);
    // needlock=0 仅适用于"先构造完之后只读"的 map (hash_map_mth 契约, 扩容会 delete[] 旧篮数组)
    // branch 源可能为 10 字节(98/api 侧), 短于定长构造读取的 12 字节, 经带长度构造补零
    fpga_fundacc_key key(fund_account_id, fund_len, branch_id, branch_len);
    fpga_cust_info *cust = nullptr;
    if (client_map_.find(cust, key, 1) == 0)
    {
      o_info = cust;
      return 0;
    }
    o_info = nullptr;
    return LBAPI_ERR_NO_CUST;
  }

  // ---- deal_order_req: 买卖委托 (客户下线返回 COUNTER_OFFLINE, 供上层降级 98) ----
  int32 fpga_counter_gateway::deal_order_req(const OrderReq &req)
  {
    // FPGA 支持域二道校验 (RUL-23 精简后仅 side 买卖; 市场/限价由 api_impl 一道校验保证,
    // 防线依赖说明见 fpga_counter_base.h check_order_dict)
    int32 tdict = check_order_dict(req);
    if (unlikely(tdict != 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_order_req: argv wrong,branch_id=" << req.branch_id.data()
                     << ",fund_account=" << req.fund_account_id.data() << ",client_seq_id=" << req.client_seq_id << end_log;
      return tdict;
    }

    // 1. 查客户信息 , 不支持非G1 客户。
    fpga_cust_info *cust = nullptr;
    if (unlikely(get_client_info(cust, req.branch_id.data(), (int32_t)sizeof(req.branch_id), req.fund_account_id.data(),
                                 (int32_t)sizeof(req.fund_account_id)) != 0 ||
                 cust == nullptr))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_order_req: cust not found, fund_account=" << req.fund_account_id.data()
                     << ", branch_id=" << req.branch_id.data() << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NO_CUST;
    }

    CACHE_PREFETCH_TMP(reinterpret_cast<void *>(cust), 0);

    uint16_t sec_index = 0;
    int32_t ret = get_sec_index(req.security_id.data(), sec_index);
    if (unlikely(ret != 0))
    {
      return ret;
    }

    if (unlikely(lb_common::atomic_load32(&cust->fpga_state) != G1_FPGA_USER_STATE_NORMAL))
    {
      lb_common::lb_log_hand tlh(log_);
      debug_log(tlh) << "fpga_gateway deal_order_req: cust fault, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_COUNTER_OFFLINE;
    }

    if (unlikely(!cust->log_state.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_order_req: not login, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }

    if (unlikely(lb_common::atomic_load32(&trade_link_connect) == 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_order_req: not connected, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_LINK_DISCONNECTED;
    }

    // 申请队列内存, 直接构造 link_send_event + g1_msg_head + order_req
    int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(order_req));
    char *data = nullptr;
    int64 pos = gw_send_queue_->write_get_mth(data, total_len);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                     << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    // 在队列内存中直接构造 (不复制中间 buf, 省一次 memcpy)
    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_SPEED_GW;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(order_req));

    g1_msg_head *head = reinterpret_cast<g1_msg_head *>(evt->data);
    build_order_msg(req, *cust, sec_index, head);

    // 提交
    gw_send_queue_->write_cmt_mth(pos, total_len);
    gw_eng_op_->trigger_send();
    return LBAPI_OK;
  }

  // deal_etf_order_req: ETF 申购赎回 (fpga 柜台不支持, 返回柜台不支持错误)
  int32 fpga_counter_gateway::deal_etf_order_req(const OrderReq &req)
  {
    return LBAPI_ERR_UNSUPPORTED_OP; // fpga 不支持 ETF, 上层降级到 98
  }

  // deal_cancel_req: 委托撤单
  int32_t fpga_counter_gateway::deal_cancel_req(const CancelReq &req)
  {
    // FPGA 仅支持系统委托号撤单: order_sys_no==0 本地拒绝 (RUL-36)
    int32 tdict = check_cancel_dict(req);
    if (unlikely(tdict != LBAPI_OK))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_cancel_req: order_sys_no required (fpga not support private-no cancel), "
                        "client_req_no="
                     << req.client_req_no << ", client_seq_id=" << req.client_seq_id << end_log;
      return tdict;
    }

    fpga_cust_info *cust = nullptr;
    if (unlikely(get_client_info(cust, req.branch_id.data(), (int32_t)sizeof(req.branch_id), req.fund_account_id.data(),
                                 (int32_t)sizeof(req.fund_account_id)) != 0 ||
                 cust == nullptr))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_cancel_req: cust not found, fund_account=" << req.fund_account_id.data()
                     << ", branch_id=" << req.branch_id.data() << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NO_CUST;
    }

    if (unlikely(lb_common::atomic_load32(&cust->fpga_state) != G1_FPGA_USER_STATE_NORMAL))
    {
      lb_common::lb_log_hand tlh(log_);
      debug_log(tlh) << "fpga_gateway deal_cancel_req: cust fault, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_COUNTER_OFFLINE;
    }

    if (unlikely(!cust->log_state.is_logined()))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_cancel_req: not login, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_NOT_LOG_CUST;
    }

    if (unlikely(lb_common::atomic_load32(&trade_link_connect) == 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_cancel_req: not connected, fund_account=" << req.fund_account_id.data()
                     << ", client_seq_id=" << req.client_seq_id << end_log;
      return LBAPI_ERR_LINK_DISCONNECTED;
    }

    int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(cancel_req));
    char *data = nullptr;
    int64 pos = gw_send_queue_->write_get_mth(data, total_len);
    if (unlikely(pos <= 0))
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_cancel_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                     << end_log;
      return LBAPI_ERR_SEND_QUEUE_FULL;
    }

    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_SPEED_GW;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(cancel_req));

    g1_msg_head *head = reinterpret_cast<g1_msg_head *>(evt->data);
    build_cancel_msg(req, *cust, head);

    gw_send_queue_->write_cmt_mth(pos, total_len);
    gw_eng_op_->trigger_send();
    return LBAPI_OK;
  }

  // deal_recv_msg: 收到 fpga 消息 (网关模式只来自 GW 链接)
  // 可能有多个完整消息+不完整消息, 依据消息头一个个解析
  int32 fpga_counter_gateway::deal_recv_msg(const char *buf, int32 len, int16 link_type)
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
        // 网关模式: 按 (board_no, user_id) 复合键查客户 (多板卡 user_id 各自从 1 开始)
        // 键取自消息头 (头长已由帧循环校验), 不预读消息体, 避免短帧越界读
        fpga_fundacc_index idx(head->board_no, head->user_id);
        fpga_cust_info *cust = nullptr;
        if (clients_.find(cust, idx, 0) == 0 && cust != nullptr)
        {
          deal_order_rtn(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fpga_gateway order_rtn cust not found, board_no=" << head->board_no
                         << ", user_id=" << head->user_id << end_log;
        }
        break;
      }
      case G1_MSG_TRADE_RTN:
      {
        fpga_fundacc_index idx(head->board_no, head->user_id);
        fpga_cust_info *cust = nullptr;
        if (clients_.find(cust, idx, 0) == 0 && cust != nullptr)
        {
          deal_trade_rtn(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fpga_gateway trade_rtn cust not found, board_no=" << head->board_no
                         << ", user_id=" << head->user_id << end_log;
        }
        break;
      }
      case G1_MSG_CANCEL_RSP:
      {
        fpga_fundacc_index idx(head->board_no, head->user_id);
        fpga_cust_info *cust = nullptr;
        if (clients_.find(cust, idx, 0) == 0 && cust != nullptr)
        {
          deal_cancel_rsp(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fpga_gateway cancel_rsp cust not found, board_no=" << head->board_no
                         << ", user_id=" << head->user_id << end_log;
        }
        break;
      }
      case G1_MSG_HEART_ANS:
      {
        // 心跳应答: 通知 link_engine (网关模式只有 GW 一条链接)
        if (gw_eng_op_ != nullptr)
        {
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
        // 先校验 rej 头完整 (未命中客户的日志分支也要读 rej 字段, 短帧越界读)
        if (head->msg_len < sizeof(g1_gw_rej_head))
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fpga_gateway gw_rej head too short, msg_len=" << head->msg_len << end_log;
          cb_mgr_->on_event(api_event_type::not_support_msg, LBAPI_ERR_MSG_LEN, "gw_rej head too short");
          break;
        }
        // 网关模式多客户, 从被拒消息体中取 board_no/user_id 定位客户
        const g1_gw_rej_head *rej = reinterpret_cast<const g1_gw_rej_head *>(head + 1);
        fpga_fundacc_index idx(head->board_no, head->user_id);
        fpga_cust_info *cust = nullptr;
        if (clients_.find(cust, idx, 0) == 0 && cust != nullptr)
        {
          deal_gw_rej(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
        }
        else
        {
          lb_common::lb_log_hand tlh(log_);
          error_log(tlh) << "fpga_gateway gw_rej cust not found, board_no=" << head->board_no
                         << ", user_id=" << head->user_id << ", rej_msg_id=" << rej->rej_msg_id
                         << ", err_code=" << rej->err_code << end_log;
        }
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
  void fpga_counter_gateway::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret)
  {
    const g1_msg_head *head = reinterpret_cast<const g1_msg_head *>(msg_buf);
    switch (head->msg_id)
    {
    case G1_MSG_ORDER_REQ:
    {
      // 委托发送失败: 构造 OrderRtn 通知客户 (按 (board_no, user_id) 复合键反查客户)
      const order_req *body = reinterpret_cast<const order_req *>(head + 1);
      fpga_fundacc_index idx(body->board_no, body->user_id);
      fpga_cust_info *cust = nullptr;
      if (clients_.find(cust, idx, 1) != 0 || cust == nullptr)
      {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga_gateway deal_send_error: order cust not found, board_no=" << body->board_no
                       << ", user_id=" << body->user_id << end_log;
        return;
      }
      OrderRtn rtn;
      StreamInfo stream;
      build_api_order_rej(head, *cust, get_counter_type(), err_ret, rtn, stream);

      cb_mgr_->on_order_rtn(stream, rtn);
      break;
    }
    case G1_MSG_CANCEL_REQ:
    {
      // 撤单发送失败: 构造 CancelRsp 通知客户
      const cancel_req *body = reinterpret_cast<const cancel_req *>(head + 1);
      fpga_fundacc_index idx(body->board_no, body->user_id);
      fpga_cust_info *cust = nullptr;
      if (clients_.find(cust, idx, 1) != 0 || cust == nullptr)
      {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga_gateway deal_send_error: cancel cust not found, board_no=" << body->board_no
                       << ", user_id=" << body->user_id << end_log;
        return;
      }
      CancelRsp rtn;
      StreamInfo stream;
      build_api_cancel_rej(head, *cust, get_counter_type(), err_ret, rtn, stream);

      cb_mgr_->on_cancel_rsp(stream, rtn);
      break;
    }
    default:
      break;
    }
  }

  // deal_cust_login: 处理账户登录事件, 构造 fpga 网关登录消息投递到 gw 队列 (状态机三场景)
  // 成功返回消息长度 (含消息头), 失败返回负数, 0-幂等/合并/静默终止/下线降级(不发包)
  int32 fpga_counter_gateway::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len)
  {
    lb_common::lb_log_hand tlh(log_);
    fpga_cust_info *cust = nullptr;

    int32 find_ret =
        get_client_info(cust, req.branch_id, (int32_t)sizeof(req.branch_id), req.fund_account_id,
                        (int32_t)sizeof(req.fund_account_id));
    if (find_ret == 0 && cust != nullptr)
    {
      // FPGA 下线 (fpga_state 非正常: 故障/故障切换柜台) → 不登录 FPGA, 98 登录成功即成功。
      if (lb_common::atomic_load32(&cust->fpga_state) != G1_FPGA_USER_STATE_NORMAL)
      {
        int32 ret = cust->log_state.clear_pending();
        if (ret == 1 || req.user_origin == 1)
        {
          info_log(tlh) << "fpga user offline, login ok by 98 degraded,branch_id=" << req.branch_id
                        << ",fund_account=" << req.fund_account_id << ",client_req_no=" << req.cust_req_no << end_log;
          LoginAns ans;
          build_login_rtn(req, 0, NULL, ans);
          cb_mgr_->on_login(ans);
        }
        return 0;
      }

      // 状态机统一判定
      int32 thavelog = 0;
      int32 tret = cust->log_state.to_login(thavelog, req.user_origin);
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
    }
    // 注: 新客户(对象未入库)直接组包发送 —— 对象在首个成功应答时入库 (deal_log_ans);
    // 缓存 agw 会话号, GW 链接重连后重登时复用 (与首登同源)
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
    // log_type=2 表示网关登录 (fpga_gateway 模式下网关代客户登录)
    build_login_msg(req, 2, head);
    return msg_len;
  }

  // ans_cust_login: 引擎同步阶段失败 (connect/组包/发送失败) —— 状态机 clear_pending 作废在途:
  // 原用户在途 → 回调失败 (状态位本为 NONE); 原内部重登在途 → 静默回退(保登录状态, 下次重连重试);
  // gateway 共享 GW 链接不关闭
  void fpga_counter_gateway::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg)
  {
    LoginAns ans;
    build_login_rtn(req, err_ret, err_msg, ans);
    fpga_cust_info *fcust = nullptr;
    get_client_info(fcust, req.branch_id, (int32_t)sizeof(req.branch_id), req.fund_account_id,
                    (int32_t)sizeof(req.fund_account_id));

    if (fcust != nullptr)
    {
      // 由于重连重登时，后台登录失败，回调客户，所以异步处理失败，也应回调客户
      int32 tpending = fcust->log_state.clear_pending();
      if (tpending != 0)
        cb_mgr_->on_login(ans);
    }
    else
    {
      lb_common::lb_log_hand tlh(log_);
      warning_log(tlh) << "fpga_gateway inner relogin sync fail, keep login state,branch_id=" << req.branch_id
                       << ",fund_account=" << req.fund_account_id << ",err_ret=" << err_ret << end_log;

      cb_mgr_->on_login(ans);
    }
  }

  // deal_log_ans: 处理 FPGA 账户登录应答
  // fpga_gateway 模式: 把 login_ans 写入 clients_/client_map_, 登录成功即回调 (证券信息独立获取)
  void fpga_counter_gateway::deal_log_ans(login_ans &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv fpga cust login answer msg,user_seq_no=" << msg.cust_req_no << ",branch_id=" << msg.branch_id
                  << ",fund_account=" << msg.fund_account_id << ",session" << msg.session << ",user_id=" << msg.user_id
                  << ",board_no=" << msg.board_no << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port
                  << ",login_time=" << msg.login_time << ",version=" << msg.version << ",sec_state=" << sec_state_
                  << end_log;

    LoginAns ans;
    build_login_rtn(msg, ans);
    fpga_cust_info *cust = nullptr;
    int32 ret = get_client_info(cust, msg.branch_id, (int32_t)sizeof(msg.branch_id), msg.fund_account_id,
                                (int32_t)sizeof(msg.fund_account_id));

    if (msg.err_code == 0)
    {
      if (cust != nullptr)
      {
        // fgw 契约: board_no,user_id 当日不可变 (同一账户重登应答不变)。
        // 变化视为异常: 经 ans_login_err 消费在途位 (避免在途滞留导致该账户后续 login
        // 全部命中"用户在途合并"静默无回调), 回调失败, 不更新映射
        if (cust->board_no != msg.board_no || cust->user_id != msg.user_id)
        {
          fatal_log(tlh) << "fpga_gateway cust board or user changed error,branch_id=" << msg.branch_id
                         << ",fund_account=" << msg.fund_account_id << ",old_board=" << cust->board_no
                         << ",old_user=" << cust->user_id << ",new_board=" << msg.board_no
                         << ",new_user=" << msg.user_id << end_log;

          ans.err_code = LBAPI_ERR_UNSUPPORTED_TYPE;
          std::memcpy(ans.err_msg.data(), "cust board or user changed error", std::strlen("cust board or user changed error"));
          if (cust->log_state.ans_login_err() == 1)
          {
            cb_mgr_->on_login(ans);
          }
          return;
        }
        // 保存客户信息 (仅业务字段+fpga_state; 登录状态由下方 ans_login_ok 统一置位)
        save_client_info(msg, *cust);
      }
      else
      {
        // 新客户入库 (对象在首个成功应答时创建, deal_cust_login 不预建)。
        // 两个 map ，不保证事务，但应在处理时考虑幂等重登时结果一致
        // 考虑用户 clients_ 用于后台委托成交等推送时使用，client_map_ 在用户登录委托等使用，
        // 先建立clients_ 再建 client_map_
        fpga_fundacc_index new_idx(msg.board_no, msg.user_id);
        ret = clients_.find(cust, new_idx, 0);
        if (ret < 0)
        {
          cust = new (std::nothrow) fpga_cust_info();
          if (nullptr == cust)
          {
            error_log(tlh) << "user login answer ok,but alloc client info error, branch_id=" << msg.branch_id
                           << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                           << ",user_seq_no=" << msg.cust_req_no << end_log;
            LoginAns ans;
            build_login_rtn(msg, ans);
            ans.err_code = LBAPI_ERR_ALLOC_MEM;
            std::memcpy(ans.err_msg.data(), "alloc client info failed", std::strlen("alloc client info failed"));
            cb_mgr_->on_login(ans);
            return;
          }
          save_client_info(msg, *cust);
          // 裸写 pending (): 对象尚在入库中途、未进 client_map_, 对外不可见
          // 无并发读者; 置位先于 client_map_ 插入, 插入失败路径经 ans_login_err 消费清位
          cust->log_state.log_state |= LOGIN_PENDING_USER;
          ret = clients_.insert_new(cust, new_idx);
          if (ret < 0)
          {
            error_log(tlh) << "user login answer ok,but insert client info to map, branch_id=" << msg.branch_id
                           << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                           << ",user_seq_no=" << msg.cust_req_no << ",ret=" << ret << end_log;
            LoginAns ans;
            build_login_rtn(msg, ans);
            ans.err_code = LBAPI_ERR_INSERT_MAP;
            std::memcpy(ans.err_msg.data(), "insert client info to map error",
                        std::strlen("insert client info to map error"));
            cb_mgr_->on_login(ans);
            delete cust;
            return;
          }
          clients_vec_.push_back(cust);
        }
        else
        {
          save_client_info(msg, *cust);
          cust->log_state.log_state |= LOGIN_PENDING_USER; ///< 同上: 入库中途裸写 pending (重试/并发首登场景)
        }

        // 键构造统一用带长度构造 (补零定长), 与 get_client_info 查询侧保持一致
        fpga_fundacc_key fund_key(msg.fund_account_id, (int32_t)sizeof(msg.fund_account_id), msg.branch_id,
                                  (int32_t)sizeof(msg.branch_id));
        ret = client_map_.insert_new(cust, fund_key);
        if (ret < 0)
        {
          error_log(tlh) << "user login answer ok,but insert client info to map, branch_id=" << msg.branch_id
                         << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                         << ",user_seq_no=" << msg.cust_req_no << ",ret=" << ret << end_log;

          ans.err_code = LBAPI_ERR_INSERT_MAP;
          std::memcpy(ans.err_msg.data(), "insert client info to map error",
                      std::strlen("insert client info to map error"));
          cust->log_state.ans_login_err();
          cb_mgr_->on_login(ans);
          return;
        }
      }

      // 状态机置成功 + 回调判定: 用户在途 → 回调一次; 内部重登/迟到 → 静默
      if (cust->log_state.ans_login_ok() == 1)
      {
        cb_mgr_->on_login(ans);
      }
    }
    else
    {
      // ---- 失败: 状态机统一处理 (USER/RELOGIN → 清状态 NONE + 回调失败[RELOGIN 曾成功客户
      // 重登被拒视为异常, 不再自动重登]; LOGOUT/无在途迟到 → 忽略);
      // 登录失败一律不关链 (共享 GW 链接, 失败后重连不再重登); 未入库客户(首登被拒)仅回调 ----
      if (cust != nullptr)
      {
        if (cust->log_state.ans_login_err() == 1)
        {
          error_log(tlh) << "fpga cust login failed, branch_id=" << msg.branch_id
                         << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                         << ",err_code=" << ans.err_code << ",err_msg=" << ans.err_msg.data()
                         << ",version=" << msg.version << ",sec_state=" << sec_state_ << end_log;
          cb_mgr_->on_login(ans);
        }
        else
        {
          // 无在途迟到失败 (在途已被首应答/断链清除): 忽略
          warning_log(tlh) << "ignore late fpga cust login fail ans without pending,branch_id=" << msg.branch_id
                           << ",fund_account=" << msg.fund_account_id << ",err_code=" << ans.err_code << end_log;
        }
        return;
      }

      // 首登被拒(未入库): 用户请求失败回调
      error_log(tlh) << "fpga cust login failed, branch_id=" << msg.branch_id << ",fund_account=" << msg.fund_account_id
                     << ",session" << msg.session << ",err_code=" << ans.err_code << ",err_msg=" << ans.err_msg.data()
                     << ",version=" << msg.version << ",sec_state=" << sec_state_ << end_log;
      cb_mgr_->on_login(ans);
    }
  }

  // deal_fpag_state: 处理 FPGA 客户状态消息 (板卡级/单客户状态推送)
  // 状态更新后统一经 on_trade_counter 回调用户 (含恢复 state=1):
  // 板卡级通知仅对已登录客户回调 (未登录客户无会话, 只置状态); 单客户通知置状态后回调该客户
  void fpga_counter_gateway::deal_fpag_state(const fpga_user_state &msg)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "recv fpga user state msg,board_no=" << msg.board_no << ",user_id=" << msg.user_id
                  << ", state=" << msg.state << ", branch_id=" << msg.branch_id
                  << ", fund_account=" << msg.fund_account_id << end_log;

    // 查找对应客户, 更新 fpga_state (优先按 fund_account_id + branch_id 查)
    fpga_cust_info *cust = nullptr;
    if (msg.fund_account_id[0] != '\0')
    {
      // 键构造统一用带长度构造 (补零定长), 与 client_map_ 插入侧保持一致
      fpga_fundacc_key key(msg.fund_account_id, (int32_t)sizeof(msg.fund_account_id), msg.branch_id,
                           (int32_t)sizeof(msg.branch_id));
      client_map_.find(cust, key, 0);
    }
    else
    {
      // 板卡级通知: 遍历该板卡全部客户置状态;
      // 状态通知仅对已登录客户回调（未登录客户无会话, 只置状态）
      // 下线为独立故障维度, **不清除登录状态**(§3.2.0);
      // 用户 login() 命中 offline 幂等分支回成功(98 降级), 委托/撤单按 COUNTER_OFFLINE 降级 98;
      // 重登资格门(非正常态)天然跳过下线客户; 恢复推送(state=1)后自动恢复重登资格
      for (auto *tc : clients_vec_)
      {
        if (tc == nullptr || tc->board_no != msg.board_no)
          continue;
        lb_common::atomic_store32(&tc->fpga_state, msg.state);
        if (tc->log_state.is_logined() && cb_mgr_ != nullptr)
        {
          cb_mgr_->on_trade_counter(static_cast<int32_t>(counter_type::fpga_gateway), tc->branch_id,
                                    tc->fund_account_id, msg.state);
        }
      }
      info_log(tlh) << "fpga board state msg,board_no=" << msg.board_no << ", state=" << msg.state << end_log;
      return;
    }
    if (nullptr == cust)
    {
      // 携带 fund_account_id 但账号索引未命中（如异客户/迟到消息）, 退化按 (board_no, user_id) 复合键索引
      fpga_fundacc_index idx(msg.board_no, msg.user_id);
      if (clients_.find(cust, idx, 0) < 0 || cust == nullptr)
      {
        error_log(tlh) << "not find cust info,board_no=" << msg.board_no << ",user_id=" << msg.user_id
                       << ", state=" << msg.state << ", branch_id=" << msg.branch_id
                       << ", fund_account=" << msg.fund_account_id << end_log;
        return;
      }
    }
    lb_common::atomic_store32(&cust->fpga_state, msg.state);

    // 单客户通知: 置状态后回调该客户
    if (cb_mgr_ != nullptr)
    {
      cb_mgr_->on_trade_counter(static_cast<int32_t>(counter_type::fpga_gateway), cust->branch_id,
                                cust->fund_account_id, msg.state);
    }
  }

  // ---- deal_link_connect: 链接建立成功通知 ----
  // 网关模式只有 GW 一条链接, 视其为 "trade 通道":
  //   - 首连或断线重连后 (重新) 发起证券信息请求
  //   - 同时把 trade_link_connect 标记为 1 (业务侧用于守门)
  //   - GW 链接重连后, 遍历已登录客户 (login_state==2 且非下线) 自动重登;
  //     地址切换 (have_switch==1) 时任一客户投递失败即关链重来
  int32 fpga_counter_gateway::deal_link_connect(int16 link_type, int32 have_switch)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "fpga link connect, link_type=" << link_type << ",have_switch=" << have_switch << end_log;

    if (link_type == LINK_TYPE_SPEED_GW)
    {
      lb_common::atomic_store32(&trade_link_connect, 1);
      {
        int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(sec_info_req));
        char *data = nullptr;
        int64 pos = gw_send_queue_->write_get_mth(data, total_len);
        if (unlikely(pos <= 0))
        {
          error_log(tlh) << "fpga_gateway delive sec req event queue full, ret=" << pos << end_log;
          cb_mgr_->on_event(api_event_type::get_sec_info, LBAPI_ERR_SEND_QUEUE_FULL, "delive sec req to queue error");
          // 返回错误，底层关闭链接
          return LBAPI_ERR_SEND_QUEUE_FULL;
        }

        link_send_event *evt = reinterpret_cast<link_send_event *>(data);
        evt->link_type = LINK_TYPE_SPEED_GW;
        evt->type = LINK_EVENT_TYPE_SEND_MSG;
        evt->data_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(sec_info_req));

        g1_msg_head *head = reinterpret_cast<g1_msg_head *>(evt->data);
        build_sec_info_req_msg(0, head);
        sec_state_ = 1;
        gw_send_queue_->write_cmt_mth(pos, total_len);
        gw_eng_op_->trigger_send();
      }

      {
        int32 re_count = 0;   // 投递成功的客户数
        int32 need_count = 0; // 应重登客户数 (曾登录成功未下线: login_state==2 且 fpga_state 为正常态)
        for (auto it = clients_vec_.begin(); it != clients_vec_.end(); ++it)
        {
          fpga_cust_info *cust = *it;
          if (cust == nullptr)
          {
            continue;
          }
          // 重登资格 = 登录成功状态 && 柜台正常态 (故障/故障切换柜台均不可用);
          // 在途/状态控制统一在 deal_cust_login 的状态机 to_login (CAS) 中处理 (S3)
          if (lb_common::atomic_load32(&cust->fpga_state) != G1_FPGA_USER_STATE_NORMAL ||
              !cust->log_state.is_logined())
          {
            continue;
          }
          need_count++;

          acc_login_event_info ttinfo;
          build_login_event(*cust, ttinfo);
          // 在途/状态控制统一在 deal_cust_login 的状态机 to_login 中处理, 此处只投递事件
          int32 ret = delive_login_event(ttinfo, gw_send_queue_, gw_eng_op_);
          if (ret == 0)
          {
            re_count++;
            info_log(tlh) << "fpga_gateway re-login delive ok, branch_id=" << cust->branch_id
                          << ",fund_account=" << cust->fund_account_id << end_log;
          }
          else
          {
            error_log(tlh) << "fpga_gateway re-login delive error, branch_id=" << cust->branch_id
                           << ",fund_account=" << cust->fund_account_id << ",ret=" << ret << end_log;

            if (have_switch == 1)
            {
              // 若发生网关切换，必须每个客户都登录
              return LBAPI_ERR_LOGIN_FAIL;
            }
          }
        }

        if (re_count > 0)
        {
          info_log(tlh) << "fpga_gateway GW link reconnected, re-login " << re_count << "/" << need_count << " cust"
                        << end_log;
        }
        else if (need_count > 0)
        {
          // 仅有"应重登客户却全部投递失败(队满)"才关链重来(暂态, 下次重连重试);
          // 无应登客户(首连/全部下线/在途)不关链: 链保持, 由用户 login() 经引擎路径恢复登录
          error_log(tlh) << "fpga_gateway re-login no delive ok error, link_type=" << link_type
                         << ",need_count=" << need_count << end_log;
          return LBAPI_ERR_SEND_QUEUE_FULL; // 返回错误，底层关闭链接重来
        }
      }

      if (cb_mgr_ != nullptr)
      {
        // 网关模式无独立 trade 链接, 用 tl=0 (GW) 上报
        cb_mgr_->on_link_status(1, 0, 1);
      }
      return 0;
    }
    return LBAPI_ERR_UNSUPPORTED_TYPE;
  }

  // ---- deal_link_close: 链接关闭通知 ----
  void fpga_counter_gateway::deal_link_close(int16 link_type)
  {
    lb_common::lb_log_hand tlh(log_);
    info_log(tlh) << "fpga link close, link_type=" << link_type << end_log;

    if (link_type == LINK_TYPE_SPEED_GW)
    {
      lb_common::atomic_store32(&trade_link_connect, 0);

      if (sec_state_ == 1)
      {
        cb_mgr_->on_event(api_event_type::get_sec_info, LBAPI_ERR_LINK_DISCONNECTED, "sec not complete by link close");
        sec_state_ = 0;
      }

      // GW 断链不清除客户登录状态 (解耦不变量, §3.2.0);
      // 断链清除全部在途 (无登录回调, 用户经链接断开事件感知), 重连后重登恢复
      for (auto *tc : clients_vec_)
      {
        if (tc == nullptr)
        {
          continue;
        }
        tc->log_state.clear_pending();
      }

      if (cb_mgr_ != nullptr)
      {
        cb_mgr_->on_link_status(1, 0, 0);
      }
    }
  }

  // deal_logout_req: 柜台侧登出 (状态机 to_logout 统一判定)
  // 对象不存在("登录成功过"由 98 层门控) → 返回成功;
  // to_logout 返回 1 → 置 LOGOUT 成功; 0 → 本来就未登录 → 成功; <0 → 用户在途 → 登出失败
  int32_t fpga_counter_gateway::deal_logout_req(const LogoutReq &req)
  {
    fpga_cust_info *cust = nullptr;
    if (get_client_info(cust, req.branch_id.data(), (int32_t)sizeof(req.branch_id), req.fund_account_id.data(),
                        (int32_t)sizeof(req.fund_account_id)) != 0 ||
        nullptr == cust)
    {
      // 未入库(offline 降级: 98 登录成功即成功, 柜台未登录) → 登出成功 (客户信息本就不存在)
      return 0;
    }

    int32_t tret = cust->log_state.to_logout();
    if (tret < 0)
    {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "cust logout but now logining,branch_id=" << req.branch_id.data()
                     << ",fund_account=" << req.fund_account_id.data()
                     << ",client_req_no=" << req.client_req_no << end_log;
      return LBAPI_ERR_STATE_LIMITED;
    }
    if (tret == 1)
    {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "fpga_gateway cust logout ok,branch_id=" << cust->branch_id
                    << ",fund_account=" << cust->fund_account_id << end_log;
    }
    return 0;
  }

} // namespace lb_api

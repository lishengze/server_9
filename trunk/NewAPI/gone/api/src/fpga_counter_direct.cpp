// fpga_counter_direct - FPGA 直连模式柜台实现
//
// 继承 fpga_counter_base, 直连模式: 单实例、单客户.
// 唯一额外存储: fpga_cust_info client_info_ (单一客户).
// GW 登录成功后才可建立 fpga core 链接 (trade_ip/port 由 GW 登录应答回填到 client_info_).
//
// 委托/撤单流程 (D33):
//   1. 取队列内存 (que_mth_buf::write_get_mth)
//   2. 在该内存中直接构造 link_send_event + g1_msg_head + order_req/cancel_req
//   3. 提交写入 (write_cmt_mth)
//   队列满立即返回错误, 不阻塞.
#include "fpga_counter_direct.h"

// 注意: api_errno.h 必须在 fpga_counter_direct.h 之前包含 (依赖 err_event_type).
#include "api_callback.h"
#include "api_config.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "counter98.h"
#include "fpga_counter_base.h"
#include "fpga_counter_direct.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

#include <cstring>

namespace lb_api {

// ---- 构造 / 析构 ----
fpga_counter_direct::fpga_counter_direct() { std::memset(&client_info_, 0, sizeof(client_info_)); }

fpga_counter_direct::~fpga_counter_direct() = default;

// init: 初始化基类 + 业务字段
int32 fpga_counter_direct::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log) {

  int32 ret = init_base(cfg, cb, log);
  if (ret < 0) {
    return ret;
  }

  std::memset(&client_info_, 0, sizeof(client_info_));

  lb_common::lb_log_hand tlh(log);
  info_log(tlh) << "init fpga_counter_direct ok, market_type=" << static_cast<int32_t>(cfg.get_market_type())
                << end_log;

  return 0;
}

// ---- deal_order_req: 买卖委托 (D22: 客户状态异常返回 COUNTER_OFFLINE) ----
int32 fpga_counter_direct::deal_order_req(const OrderReq &req) {

  if (unlikely(trade_link_connect == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_direct deal_order_req: not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (unlikely(client_info_.login_state != 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_direct deal_order_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }
  if (unlikely(client_info_.fpga_state == 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_direct deal_order_req: cust fault, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_COUNTER_OFFLINE;
  }

  // 2. 查询 sec_index
  uint16_t sec_index = 0;
  int32_t ret = get_sec_index(req.security_id.data(), sec_index);
  if (unlikely(ret != 0)) {
    return ret;
  }

  // 4. 申请队列内存, 直接构造 link_send_event + g1_msg_head + order_req
  int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(order_req));
  char *data = nullptr;
  int64 pos = trade_send_queue_->write_get_mth(data, total_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_direct deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  // 5. 在队列内存中直接构造 (D33: 不复制中间 buf, 省一次 memcpy)
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
int32 fpga_counter_direct::deal_etf_order_req(const OrderReq &req) {
  return LBAPI_ERR_UNSUPPORTED_OP; // fpga 不支持 ETF, 上层降级到 98
}

// deal_cancel_req: 委托撤单
int32_t fpga_counter_direct::deal_cancel_req(const CancelReq &req) {

  if (unlikely(trade_link_connect == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_direct deal_cancel_req: not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }
  if (unlikely(client_info_.login_state != 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_direct deal_cancel_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }
  if (unlikely(client_info_.fpga_state == 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_direct deal_cancel_req: cust fault, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_COUNTER_OFFLINE;
  }

  int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(cancel_req));
  char *data = nullptr;
  int64 pos = trade_send_queue_->write_get_mth(data, total_len);
  if (unlikely(pos <= 0)) {
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
int32 fpga_counter_direct::deal_recv_msg(const char *buf, uint16 len, int16 link_type) {
  int32 deal_len = 0;
  int32 mlen = 0;
  const g1_msg_head *head;

  while (static_cast<int32>(len) - deal_len >= static_cast<int32>(sizeof(g1_msg_head))) {
    head = reinterpret_cast<const g1_msg_head *>(buf + deal_len);
    mlen = static_cast<int32>(head->msg_len) + static_cast<int32>(sizeof(g1_msg_head));
    if (unlikely(mlen > static_cast<int32>(len) - deal_len)) {
      if (mlen < G1_MSG_MAX_LEN) {
        return deal_len;
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga counter recv msg len error, mlen=" << mlen << ", link_type=" << link_type << end_log;
        // 返回错误，让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
    }

    switch (head->msg_id) {
    case G1_MSG_ORDER_RTN: {
      deal_order_rtn(head, client_info_, static_cast<int32>(counter_type::fpga_direct));
      break;
    }
    case G1_MSG_TRADE_RTN: {
      deal_trade_rtn(head, client_info_, static_cast<int32>(counter_type::fpga_direct));
      break;
    }
    case G1_MSG_CANCEL_RSP: {
      deal_cancel_rsp(head, client_info_, static_cast<int32>(counter_type::fpga_gateway));
      break;
    }
    case G1_MSG_HEART_ANS: {
      // 心跳应答: 通知 link_engine
      if (link_type == LINK_TYPE_SPEED_TRADE) {
        if (trade_eng_op_ != nullptr)
          trade_eng_op_->deal_heart_msg_ans(LINK_TYPE_SPEED_TRADE);
      } else {
        if (gw_eng_op_ != nullptr)
          gw_eng_op_->deal_heart_msg_ans(LINK_TYPE_SPEED_GW);
      }
      break;
    }
    case G1_MSG_SEC_INFO_ANS: {
      int32 ret = deal_sec_info_ans(head);
      if (ret == 1) {
        check_ans_log(0);
      } else if (ret < 0) {
        check_ans_log(ret);
        // 返回错误，让底层关闭链接
        return ret;
      }
      break;
    }
    case G1_MSG_LOGIN_ANS: {
      if (head->msg_len >= sizeof(login_ans)) {
        login_ans *ans = const_cast<login_ans *>(reinterpret_cast<const login_ans *>(head + 1));
        deal_log_ans(*ans);
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga counter recv login_ans msg len too short, msg_len=" << head->msg_len << end_log;
        // 返回错误，让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
      break;
    }
    case G1_MSG_OFFLINE_PUSH: {
      if (head->msg_len >= sizeof(fpga_user_state)) {
        const fpga_user_state *st = reinterpret_cast<const fpga_user_state *>(head + 1);
        deal_fpag_state(*st);
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga counter recv offline_push msg len too short, msg_len=" << head->msg_len << end_log;
        // 返回错误，让底层关闭链接
        return LBAPI_ERR_MSG_LEN;
      }
      break;
    }
    case G1_MSG_GW_REJ: {
      // 网关路由拒绝: g1_msg_head + g1_gw_rej_head + (order_req | cancel_req)
      // 仅 fpga 柜台单客户, 用唯一的 client_info_
      deal_gw_rej(head, client_info_, static_cast<int32>(counter_type::fpga_direct));
      break;
    }
    default: {
      // 未知消息, 跳过，不返回错误，以避免链接关闭，保证旧api对新增消息的兼容性
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "fpga counter recv unknown msg, msg_id=" << head->msg_id << end_log;
      break;
      //return LBAPI_ERR_MSG_TYPE;
    }
    }

    deal_len += mlen;
  }

  return deal_len;
}

// deal_send_error: 发送消息失败时处理 (如委托撤单, 构建 rtn 回调通知客户)
void fpga_counter_direct::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret) {
  const g1_msg_head *head = reinterpret_cast<const g1_msg_head *>(msg_buf);
  switch (head->msg_id) {
  case G1_MSG_ORDER_REQ: {
    // 委托发送失败: 构造 CancelRsp 通知客户
    OrderRtn rtn;
    StreamInfo stream;
    build_api_order_rej(head, client_info_, err_ret, rtn, stream);

    cb_mgr_->on_order_rtn(stream, rtn);
    break;
  }
  case G1_MSG_CANCEL_REQ: {
    // 撤单发送失败: 构造 CancelRsp 通知客户
    CancelRsp rtn;
    StreamInfo stream;
    build_api_cancel_rej(head, client_info_, err_ret, rtn, stream);

    cb_mgr_->on_cancel_rsp(stream, rtn);
    break;
  }
  default:
    break;
  }
}

// deal_cust_login: 处理账户登录事件, 构造 fpga 登录消息投递到 gw 队列
// 成功返回消息长度 (含消息头), 失败返回负数，0-不需重复登陆
int32 fpga_counter_direct::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len) {
  if (client_info_.login_state == 2) {
    LoginAns ans;
    build_login_rtn(req, 0, NULL, ans);

    cb_mgr_->on_login(ans);
    return 0;
  }

  int32 msg_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(login_req));
  if (buf_len < msg_len) {
    return LBAPI_ERR_MSG_LEN;
  }
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
  // log_type=1 表示用户登录 (fpga_direct 是单客户, 使用用户登录)
  build_login_msg(req, 1, head);
  client_info_.login_state = 1;
  return msg_len;
}

// ans_cust_login: 登录结果回调
void fpga_counter_direct::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg) {

  /*if (err_ret == 0) {
    if (sec_state_ != 2) {
      lb_common::lb_log_hand tlh(log_);
      info_log(tlh) << "user login answer ok,but sec info not finished,branch_id=" << req.branch_id
                               << ",fund_account=" << req.fund_account_id << ",client_seq_id=" << client_seq_id
                               << ",session=" << req.session << ",sec_state=" << sec_state_ << end_log;
      return;
    }
  } else {client_info_.login_state = 0;}*/

  client_info_.login_state = 0;

  LoginAns ans;
  build_login_rtn(req, err_ret, err_msg, ans);

  cb_mgr_->on_login(ans);
}

// deal_log_ans: 处理 FPGA 账户登录应答
// fpga_direct 模式: 提取 trade_ip/port/user_id, 投递 FPGA_CORE_CONNECT 事件到 fast_engine
void fpga_counter_direct::deal_log_ans(login_ans &msg) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "recv fpga cust login answer msg,user_seq_no=" << msg.cust_req_no << ",branch_id=" << msg.branch_id
                << ",fund_account=" << msg.fund_account_id << ",session" << msg.session << ",user_id=" << msg.user_id
                << ",board_no=" << msg.board_no << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port
                << ",login_time=" << msg.login_time << ",version=" << msg.version << ",sec_state=" << sec_state_
                << end_log;

  LoginAns ans;
  build_login_rtn(msg, ans);

  if (msg.err_code == 0 && sec_state_ != 0) {
    // 保存客户信息 (含 user_id / board_no, 后续业务消息头将填充)
    save_client_info(msg, client_info_);

    //发起fpga 核心同步链接
    int32 ret = delive_fpga_connect();
    if (ret == 0) {
      info_log(tlh) << "user login answer to delive fpga connect ok, branch_id=" << msg.branch_id
                    << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                    << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port << end_log;
    } else {
      warning_log(tlh) << "user login answer to delive fpga connect error, branch_id=" << msg.branch_id
                       << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                       << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port << ",ret=" << ret << end_log;
    }

    //回调通知客户
    if (sec_state_ == 2) {
      cb_mgr_->on_login(ans);
    } else {
      info_log(tlh) << "user login answer ok,but sec info not finished, branch_id=" << msg.branch_id
                    << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                    << ",sec_state=" << sec_state_ << end_log;
    }
  } else {
    client_info_.login_state = 0;

    if (sec_state_ == 0 && msg.err_code == 0) {
      ans.err_code = LBAPI_ERR_NO_SEC;
      std::memcpy(ans.err_msg.data(), "not have sec info", std::strlen("not have sec info"));
    }
    error_log(tlh) << "fpga cust login failed, branch_id=" << msg.branch_id << ",fund_account=" << msg.fund_account_id
                   << ",session" << msg.session << ",err_code=" << ans.err_code << ",err_msg=" << ans.err_msg.data()
                   << ",version=" << msg.version << ",sec_state=" << sec_state_ << end_log;
    cb_mgr_->on_login(ans);
  }
}

int32 fpga_counter_direct::delive_fpga_connect() {
  int32 evt_len = static_cast<int32>(sizeof(link_send_event) + sizeof(fpga_core_connect_info));
  char *data = nullptr;
  int64 pos = trade_send_queue_->write_get_mth(data, evt_len);
  if (pos <= 0) {
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

// 证券信息获取完成后检查登录状态
void fpga_counter_direct::check_ans_log(int32 err_code) {
  lb_common::lb_log_hand tlh(log_);

  if (err_code == 0) {
    if (client_info_.login_state == 2) {
      LoginAns ans;
      build_login_rtn(client_info_, 0, NULL, ans);
      cb_mgr_->on_login(ans);

      info_log(tlh) << "sec info complete and answer user login,branch_id=" << ans.branch_id.data()
                    << ",fund_account=" << ans.fund_account_id.data() << end_log;
    }
  } else {
    sec_state_ = 0;

    error_log(tlh) << "sec info error to close link,err_code=" << err_code << end_log;

    gw_eng_op_->deal_close_link(LINK_TYPE_SPEED_GW, LBAPI_ERR_NO_SEC);
  }
}

// deal_fpag_state: 处理 FPGA 客户状态消息
void fpga_counter_direct::deal_fpag_state(const fpga_user_state &msg) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "recv fpga user state msg,board_no=" << msg.board_no << ",user_id=" << msg.user_id
                << ", state=" << msg.state << ", branch_id=" << msg.branch_id
                << ", fund_account=" << msg.fund_account_id << end_log;

  // 查找对应客户, 更新 fpga_state
  if (msg.fund_account_id[0] != '\0') {
    // 比较资金账号
    if (std::memcmp(msg.fund_account_id, client_info_.fund_account_id,
                    std::min<size_t>(sizeof(msg.fund_account_id), sizeof(client_info_.fund_account_id))) == 0) {
      client_info_.fpga_state = msg.state;
    } else {
      return;
    }
  } else {
    // 没有 fund_account_id, 默认更新单客户
    client_info_.fpga_state = msg.state;
  }

  if (cb_mgr_ != nullptr) {
    if (msg.state == 2) {
      cb_mgr_->on_error(err_event_type::fast_user_offline, LBAPI_ERR_COUNTER_OFFLINE, "recv fpga user offline");
    } else {
      cb_mgr_->on_error(err_event_type::fast_user_offline, 0, "recv fpga user online");
    }
  }
}

// ---- deal_link_connect: 链接建立成功通知 ----
int32 fpga_counter_direct::deal_link_connect(int16 link_type, int32 have_switch) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fpga link connect, link_type=" << link_type << ",have_switch=" << have_switch << end_log;

  int32_t tl = 0;
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    tl = 1;
    trade_link_connect = 1;
  } else {
    //  若是网关链接，检查是否需要发起证券信息请求
    if (sec_state_ != 2) {
      int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(sec_info_req));
      char *data = nullptr;
      int64 pos = gw_send_queue_->write_get_mth(data, total_len);
      if (unlikely(pos <= 0)) {
        error_log(tlh) << "fpga_direct delive sec req event queue full, ret=" << pos << end_log;
        cb_mgr_->on_error(err_event_type::get_sec_info, LBAPI_ERR_SEND_QUEUE_FULL, "delive sec req to queue error");
        //返回错误，底层关闭链接
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

    // v2.1: GW 链接重连后自动重登已登录客户
    if (client_info_.login_state == 2) {
      info_log(tlh) << "fpga_direct GW link reconnected, re-deliver cust login, branch_id=" << client_info_.branch_id
                    << ",fund_account=" << client_info_.fund_account_id << end_log;
      int32 ret = delive_cust_login(client_info_, gw_send_queue_, gw_eng_op_);
      if (ret != 0) {
        error_log(tlh) << "fpga_direct re-login delive error, ret=" << ret << end_log;
      }
    }
  }
  if (cb_mgr_ != nullptr) {
    cb_mgr_->on_link_status(1, tl, 1);
  }
  return 0;
}

// ---- deal_link_close: 链接关闭通知 ----
void fpga_counter_direct::deal_link_close(int16 link_type) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fpga link close, link_type=" << link_type << end_log;

  int32_t tl = 0;
  if (link_type == LINK_TYPE_SPEED_TRADE) {
    tl = 1;
    trade_link_connect = 0;
  }
  if (cb_mgr_ != nullptr) {
    cb_mgr_->on_link_status(1, tl, 0);
  }
  if (link_type == LINK_TYPE_SPEED_GW) {
    if (sec_state_ == 1) {
      sec_state_ = 0;
      cb_mgr_->on_error(err_event_type::get_sec_info, LBAPI_ERR_LINK_DISCONNECTED, "sec not complete by link close");
    }
    // sec_state_ = 0;
    if (client_info_.login_state == 1) {
      client_info_.login_state = 0;
      LoginAns ans;
      build_login_rtn(client_info_, LBAPI_ERR_LINK_DISCONNECTED, "login not complete by link close", ans);
      cb_mgr_->on_login(ans);

      info_log(tlh) << "login not complete by link close,branch_id=" << ans.branch_id.data()
                    << ",fund_account=" << ans.fund_account_id.data() << end_log;

      //cb_mgr_->on_error(err_event_type::login_disconnet, LBAPI_ERR_LINK_DISCONNECTED,
      //                  "login not complete by link close");
    }
  }
}

} // namespace lb_api
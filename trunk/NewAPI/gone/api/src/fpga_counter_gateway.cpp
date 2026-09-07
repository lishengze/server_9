// fpga_counter_gateway - FPGA 网关模式柜台实现
//
// 继承 fpga_counter_base, 网关模式: 多客户、所有业务走同一条 fpga_gw 链接.
// 与 direct 的主要差异 (D26):
//   - 多客户存储 (client_map_ + clients_, 按 user_id 索引)
//   - 无 fpga core 链接, 委托/撤单/回报全部走 fpga_gw 链接
//   - login_ans 成功后直接 on_login (无 core 同步 connect 步骤)
//
// 委托/撤单流程 (D33):
//   1. 按 fund_account_id + branch_id 查 user_id (-> fpga_cust_info)
//   2. 取 gw_send_queue_ 内存
//   3. 在该内存中直接构造 link_send_event + g1_msg_head + order_req/cancel_req
//   4. 提交写入 (write_cmt_mth)
//   队列满立即返回错误, 不阻塞.
#include "fpga_counter_gateway.h"

// 注意: api_errno.h 必须在 fpga_counter_gateway.h 之前包含 (依赖 err_event_type).
#include "api_callback.h"
#include "api_config.h"
#include "api_errno.h"
#include "api_event_msg.h"
#include "callback_manager.h"
#include "comm_sys.h"
#include "fpga_counter_base.h"
#include "g1msghead.h"
#include "g1trademsg.h"
#include "mlog.h"
#include "mutils.h"
#include "que_mth_buf.h"

#include <cstring>

namespace lb_api {

// ---- 构造 / 析构 ----
fpga_counter_gateway::fpga_counter_gateway() = default;

fpga_counter_gateway::~fpga_counter_gateway() {
  // clients_ 持有 new 出来的 fpga_cust_info*, 析构时需逐个释放, 避免泄漏
  for (uint32 i = 0; i < clients_vec_.size(); i++) {
    delete clients_vec_[i];
  }
  clients_.clear();
  client_map_.clear();
}

// init: 初始化基类 + 多客户存储
int32 fpga_counter_gateway::init(const api_config_impl &cfg, callback_manager *cb, lb_common::lb_log *log) {
  lb_common::lb_log_hand tlh(log);

  int32 ret = init_base(cfg, cb, log);
  if (ret < 0) {
    return ret;
  }

  // 预分配 hash map 容量 (cfg 未提供最大客户数, 使用默认值)
  // 多板卡网关场景: 板卡不连续、每板 user_id 从 1 开始, 用 hash_map_mth 而非 vector
  ret = client_map_.init(1024);
  if (ret < 0) {
    error_log(tlh) << "init fpga_counter_gateway client map error, ret=" << ret << end_log;
    return LBAPI_ERR_ALLOC_MEM;
  }
  ret = clients_.init(1024);
  if (ret < 0) {
    error_log(tlh) << "init fpga_counter_gateway client index map error, ret=" << ret << end_log;
    return LBAPI_ERR_ALLOC_MEM;
  }
  login_cache_.clear();

  info_log(tlh) << "init fpga_counter_gateway ok, market_type=" << static_cast<int32_t>(cfg.get_market_type())
                << end_log;

  return 0;
}

int32 fpga_counter_gateway::get_client_info(fpga_cust_info *&o_info, const char *branch_id,
                                            const char *fund_account_id) {
  // 网关模式: (fund_account_id, branch_id) -> (board_no, user_id) -> fpga_cust_info*
  // 热路径: 发送查询, 走 client_map_ 锁无关读 (needlock=0), 再查 clients_ 拿指针
  fpga_fundacc_key key(fund_account_id, branch_id);
  fpga_cust_info *cust = nullptr;
  if (client_map_.find(cust, key, 0) == 0) {
    o_info = cust;
    return 0;
  }
  o_info = nullptr;
  return LBAPI_ERR_NO_CUST;
}

// ---- deal_order_req: 买卖委托 (D22: 客户状态异常返回 COUNTER_OFFLINE) ----
int32 fpga_counter_gateway::deal_order_req(const OrderReq &req) {

  if (unlikely(trade_link_connect == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_order_req: not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  // 1. 查客户信息
  fpga_cust_info *cust = nullptr;
  if (unlikely(get_client_info(cust, req.branch_id.data(), req.fund_account_id.data()) != 0 || cust == nullptr)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_order_req: cust not found, fund_account=" << req.fund_account_id.data()
                   << ", branch_id=" << req.branch_id.data() << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NO_CUST;
  }

  CACHE_PREFETCH_TMP(reinterpret_cast<void *>(cust), 0);

  uint16_t sec_index = 0;
  int32_t ret = get_sec_index(req.security_id.data(), sec_index);
  if (unlikely(ret != 0)) {
    return ret;
  }

  if (unlikely(cust->login_state != 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_order_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }
  if (unlikely(cust->fpga_state == 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_order_req: cust fault, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_COUNTER_OFFLINE;
  }

  // 申请队列内存, 直接构造 link_send_event + g1_msg_head + order_req
  int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(order_req));
  char *data = nullptr;
  int64 pos = gw_send_queue_->write_get_mth(data, total_len);
  if (unlikely(pos <= 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_order_req: queue full, pos=" << pos << ", client_seq_id=" << req.client_seq_id
                   << end_log;
    return LBAPI_ERR_SEND_QUEUE_FULL;
  }

  // 在队列内存中直接构造 (D33: 不复制中间 buf, 省一次 memcpy)
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
int32 fpga_counter_gateway::deal_etf_order_req(const OrderReq &req) {
  return LBAPI_ERR_UNSUPPORTED_OP; // fpga 不支持 ETF, 上层降级到 98
}

// deal_cancel_req: 委托撤单
int32_t fpga_counter_gateway::deal_cancel_req(const CancelReq &req) {

  if (unlikely(trade_link_connect == 0)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_cancel_req: not connected, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_LINK_DISCONNECTED;
  }

  fpga_cust_info *cust = nullptr;
  if (unlikely(get_client_info(cust, req.branch_id.data(), req.fund_account_id.data()) != 0 || cust == nullptr)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_cancel_req: cust not found, fund_account=" << req.fund_account_id.data()
                   << ", branch_id=" << req.branch_id.data() << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NO_CUST;
  }
  if (unlikely(cust->login_state != 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_cancel_req: not login, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_NOT_LOG_CUST;
  }
  if (unlikely(cust->fpga_state == 2)) {
    lb_common::lb_log_hand tlh(log_);
    error_log(tlh) << "fpga_gateway deal_cancel_req: cust fault, fund_account=" << req.fund_account_id.data()
                   << ", client_seq_id=" << req.client_seq_id << end_log;
    return LBAPI_ERR_COUNTER_OFFLINE;
  }

  int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(cancel_req));
  char *data = nullptr;
  int64 pos = gw_send_queue_->write_get_mth(data, total_len);
  if (unlikely(pos <= 0)) {
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
int32 fpga_counter_gateway::deal_recv_msg(const char *buf, uint16 len, int16 link_type) {
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
      // 网关模式: 按 (board_no, user_id) 复合键查客户 (多板卡 user_id 各自从 1 开始)
      const order_rtn *body = reinterpret_cast<const order_rtn *>(head + 1);
      fpga_fundacc_index idx(body->board_no, body->user_id);
      fpga_cust_info *cust = nullptr;
      if (clients_.find(cust, idx, 1) == 0 && cust != nullptr) {
        deal_order_rtn(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga_gateway order_rtn cust not found, board_no=" << body->board_no
                       << ", user_id=" << body->user_id << end_log;
      }
      break;
    }
    case G1_MSG_TRADE_RTN: {
      const trade_rtn *body = reinterpret_cast<const trade_rtn *>(head + 1);
      fpga_fundacc_index idx(body->board_no, body->user_id);
      fpga_cust_info *cust = nullptr;
      if (clients_.find(cust, idx, 1) == 0 && cust != nullptr) {
        deal_trade_rtn(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga_gateway trade_rtn cust not found, board_no=" << body->board_no
                       << ", user_id=" << body->user_id << end_log;
      }
      break;
    }
    case G1_MSG_CANCEL_RSP: {
      const cancel_rsp *body = reinterpret_cast<const cancel_rsp *>(head + 1);
      fpga_fundacc_index idx(body->board_no, body->user_id);
      fpga_cust_info *cust = nullptr;
      if (clients_.find(cust, idx, 1) == 0 && cust != nullptr) {
        deal_cancel_rsp(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga_gateway cancel_rsp cust not found, board_no=" << body->board_no
                       << ", user_id=" << body->user_id << end_log;
      }
      break;
    }
    case G1_MSG_HEART_ANS: {
      // 心跳应答: 通知 link_engine (网关模式只有 GW 一条链接)
      if (gw_eng_op_ != nullptr) {
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
      // 网关模式多客户, 从被拒消息体中取 board_no/user_id 定位客户
      const g1_gw_rej_head *rej = reinterpret_cast<const g1_gw_rej_head *>(head + 1);
      fpga_fundacc_index idx(head->board_no, head->user_id);
      fpga_cust_info *cust = nullptr;
      if (clients_.find(cust, idx, 1) == 0 && cust != nullptr) {
        deal_gw_rej(head, *cust, static_cast<int32>(counter_type::fpga_gateway));
      } else {
        lb_common::lb_log_hand tlh(log_);
        error_log(tlh) << "fpga_gateway gw_rej cust not found, board_no=" << head->board_no
                       << ", user_id=" << head->user_id << ", rej_msg_id=" << rej->rej_msg_id
                       << ", err_code=" << rej->err_code << end_log;
      }
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
void fpga_counter_gateway::deal_send_error(char *msg_buf, int32 msg_len, int16 link_type, int32 err_ret) {
  const g1_msg_head *head = reinterpret_cast<const g1_msg_head *>(msg_buf);
  switch (head->msg_id) {
  case G1_MSG_ORDER_REQ: {
    // // 委托发送失败: 构造 CancelRsp 通知客户 (按 (board_no, user_id) 复合键反查客户)
    const order_req *body = reinterpret_cast<const order_req *>(head + 1);
    fpga_fundacc_index idx(body->board_no, body->user_id);
    fpga_cust_info *cust = nullptr;
    if (clients_.find(cust, idx, 1) != 0 || cust == nullptr) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_send_error: order cust not found, board_no=" << body->board_no
                     << ", user_id=" << body->user_id << end_log;
      return;
    }
    OrderRtn rtn;
    StreamInfo stream;
    build_api_order_rej(head, *cust, err_ret, rtn, stream);

    cb_mgr_->on_order_rtn(stream, rtn);
    break;
  }
  case G1_MSG_CANCEL_REQ: {
    // 撤单发送失败: 构造 CancelRsp 通知客户
    const cancel_req *body = reinterpret_cast<const cancel_req *>(head + 1);
    fpga_fundacc_index idx(body->board_no, body->user_id);
    fpga_cust_info *cust = nullptr;
    if (clients_.find(cust, idx, 1) != 0 || cust == nullptr) {
      lb_common::lb_log_hand tlh(log_);
      error_log(tlh) << "fpga_gateway deal_send_error: cancel cust not found, board_no=" << body->board_no
                     << ", user_id=" << body->user_id << end_log;
      return;
    }
    CancelRsp rtn;
    StreamInfo stream;
    build_api_cancel_rej(head, *cust, err_ret, rtn, stream);

    cb_mgr_->on_cancel_rsp(stream, rtn);
    break;
  }
  default:
    break;
  }
}

// deal_cust_login: 处理账户登录事件, 构造 fpga 网关登录消息投递到 gw 队列
// 成功返回消息长度 (含消息头), 失败返回负数
int32 fpga_counter_gateway::deal_cust_login(const acc_login_event_info &req, char *o_buf, int32 buf_len) {
  fpga_cust_info *cust = nullptr;
  int32 find_ret = get_client_info(cust, req.branch_id, req.fund_account_id);
  if (find_ret == 0 && cust != nullptr) {
    if (cust->login_state == 2) {
      LoginAns ans;
      build_login_rtn(req, 0, NULL, ans);

      cb_mgr_->on_login(ans);
      return 0;
    }
  }

  int32 msg_len = static_cast<int32>(sizeof(g1_msg_head) + sizeof(login_req));
  if (buf_len < msg_len) {
    return LBAPI_ERR_MSG_LEN;
  }
  g1_msg_head *head = reinterpret_cast<g1_msg_head *>(o_buf);
  // log_type=2 表示网关登录 (fpga_gateway 模式下网关代客户登录)
  build_login_msg(req, 2, head);
  return msg_len;
}

// ans_cust_login: 登录结果回调 (失败路径: 在 engine 同步阶段失败时调用)
void fpga_counter_gateway::ans_cust_login(const acc_login_event_info &req, int32 err_ret, const char *err_msg) {
  LoginAns ans;
  build_login_rtn(req, err_ret, err_msg, ans);
  cb_mgr_->on_login(ans);
}

// deal_log_ans: 处理 FPGA 账户登录应答
// fpga_gateway 模式: 把 login_ans 写入 clients_/client_map_;
//   - 若证券信息已就绪 (sec_state_==2) 则立即回调 on_login;
//   - 否则缓存到 login_cache_, 待 check_ans_log 统一回调.
// (gateway 无 core 链接, 不需要 delive_fpga_connect)
void fpga_counter_gateway::deal_log_ans(login_ans &msg) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "recv fpga cust login answer msg,user_seq_no=" << msg.cust_req_no << ",branch_id=" << msg.branch_id
                << ",fund_account=" << msg.fund_account_id << ",session" << msg.session << ",user_id=" << msg.user_id
                << ",board_no=" << msg.board_no << ",trade_ip=" << msg.trade_ip << ",trade_port=" << msg.trade_port
                << ",login_time=" << msg.login_time << ",version=" << msg.version << ",sec_state=" << sec_state_
                << end_log;

  if (msg.err_code == 0 && sec_state_ != 0) {
    // ---- 成功: 创建/获取客户信息指针, 写入 clients_/client_map_ ----
    // 多板卡场景: (board_no, user_id) 复合键, 板卡不连续且每板 user_id 独立从 1 开始,
    // 用 hash_map 存指针而非 vector 按 user_id 索引.

    // 1) 查 (fund, branch) 是否已映射到某个 (board, user) (再登录/换板卡)
    fpga_cust_info *cust = nullptr;
    int32 ret = get_client_info(cust, msg.branch_id, msg.fund_account_id);

    if (ret == 0) {
      save_client_info(msg, *cust);
    } else {
      fpga_fundacc_index new_idx(msg.board_no, msg.user_id);
      ret = clients_.find(cust, new_idx, 0);
      if (ret < 0) {
        cust = new (std::nothrow) fpga_cust_info();
        if (nullptr == cust) {
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
        ret = clients_.insert_new(cust, new_idx);
        if (ret < 0) {
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
      } else {
        save_client_info(msg, *cust);
      }
      fpga_fundacc_key fund_key(msg.fund_account_id, msg.branch_id);
      ret = client_map_.insert_new(cust, fund_key);
      if (ret < 0) {
        error_log(tlh) << "user login answer ok,but insert client info to map, branch_id=" << msg.branch_id
                       << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                       << ",user_seq_no=" << msg.cust_req_no << ",ret=" << ret << end_log;
        LoginAns ans;
        build_login_rtn(msg, ans);
        ans.err_code = LBAPI_ERR_INSERT_MAP;
        std::memcpy(ans.err_msg.data(), "insert client info to map error",
                    std::strlen("insert client info to map error"));
        cb_mgr_->on_login(ans);
        return;
      }
    }

    // 网关模式下证券信息就绪才能立即回调, 否则缓存
    if (sec_state_ == 2) {
      LoginAns ans;
      build_login_rtn(msg, ans);
      cb_mgr_->on_login(ans);
    } else {
      login_cache_.push_back(msg);
      info_log(tlh) << "user login answer ok,but sec info not finished, branch_id=" << msg.branch_id
                    << ",fund_account=" << msg.fund_account_id << ",session" << msg.session
                    << ",sec_state=" << sec_state_ << end_log;
    }
  } else {
    // ---- 失败 (或证券信息状态非法): 仅回调通知客户, 不入库 ----
    LoginAns ans;
    build_login_rtn(msg, ans);

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

// 证券信息获取完成或失败后, 对缓存的 login_ans 统一处理 (对应 direct 的 check_ans_log)
void fpga_counter_gateway::check_ans_log(int32 err_code) {
  lb_common::lb_log_hand tlh(log_);

  if (err_code == 0) {
    // 证券信息就绪: 把缓存的 login_ans 全部回调 on_login
    for (auto &cached : login_cache_) {
      LoginAns ans;
      build_login_rtn(cached, ans);
      cb_mgr_->on_login(ans);

      info_log(tlh) << "sec info complete and answer user login,branch_id=" << ans.branch_id.data()
                    << ",fund_account=" << ans.fund_account_id.data() << end_log;
    }
    login_cache_.clear();
  } else {
    sec_state_ = 0;

    error_log(tlh) << "sec info error to close link,err_code=" << err_code << end_log;

    if (gw_eng_op_ != nullptr) {
      gw_eng_op_->deal_close_link(LINK_TYPE_SPEED_GW, LBAPI_ERR_NO_SEC);
    }
  }
}

// deal_fpag_state: 处理 FPGA 客户状态消息
void fpga_counter_gateway::deal_fpag_state(const fpga_user_state &msg) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "recv fpga user state msg,board_no=" << msg.board_no << ",user_id=" << msg.user_id
                << ", state=" << msg.state << ", branch_id=" << msg.branch_id
                << ", fund_account=" << msg.fund_account_id << end_log;

  // 查找对应客户, 更新 fpga_state (优先按 fund_account_id + branch_id 查)
  fpga_cust_info *cust = nullptr;
  if (msg.fund_account_id[0] != '\0') {
    fpga_fundacc_key key(msg.fund_account_id, msg.branch_id);
    client_map_.find(cust, key, 0);
  }
  if (nullptr == cust) {
    // 协议未携带 fund_account_id, 退化按 (board_no, user_id) 复合键索引
    fpga_fundacc_index idx(msg.board_no, msg.user_id);
    if (clients_.find(cust, idx, 0) < 0 || cust == nullptr) {
      return;
    }
  }
  cust->fpga_state = msg.state;
  if (cb_mgr_ != nullptr) {
    if (msg.state == 2) {
      cb_mgr_->on_error(err_event_type::fast_user_offline, LBAPI_ERR_COUNTER_OFFLINE, "recv fpga user offline");
    } else {
      cb_mgr_->on_error(err_event_type::fast_user_offline, 0, "recv fpga user online");
    }
  }
}

// ---- deal_link_connect: 链接建立成功通知 ----
// 网关模式只有 GW 一条链接, 视其为 "trade 通道":
//   - 首次建立时若 sec_state_==0, 主动发起证券信息请求
//   - 同时把 trade_link_connect 标记为 1 (业务侧用于守门)
//   - v2.1: GW 链接重连后, 遍历已登录客户 (login_state==2) 自动重登
int32 fpga_counter_gateway::deal_link_connect(int16 link_type, int32 have_switch) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fpga link connect, link_type=" << link_type << ",have_switch=" << have_switch << end_log;
  // todo : 链接地址切换, 是否重新登陆

  if (link_type == LINK_TYPE_SPEED_GW) {
    trade_link_connect = 1;
    //  网关模式下 GW 链接即业务链接, 检查是否需要发起证券信息请求
    if (sec_state_ == 0) {
      int32 total_len = static_cast<int32>(sizeof(link_send_event) + sizeof(g1_msg_head) + sizeof(sec_info_req));
      char *data = nullptr;
      int64 pos = gw_send_queue_->write_get_mth(data, total_len);
      if (unlikely(pos <= 0)) {
        error_log(tlh) << "fpga_gateway delive sec req event queue full, ret=" << pos << end_log;
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

    // v2.1: GW 链接重连后遍历所有已登录客户, 自动重登
    if (have_switch == 1 || true /* 任何重连都触发重登, 不仅 address switch */) {
      int32 re_count = 0;
      for (auto it = clients_vec_.begin(); it != clients_vec_.end(); ++it) {
        fpga_cust_info *cust = *it;
        if (cust == nullptr || cust->login_state != 2) {
          continue;
        }
        int32 ret = delive_cust_login(*cust, gw_send_queue_, gw_eng_op_);
        if (ret == 0) {
          re_count++;
        } else {
          error_log(tlh) << "fpga_gateway re-login delive error, branch_id=" << cust->branch_id
                         << ",fund_account=" << cust->fund_account_id << ",ret=" << ret << end_log;
        }
      }
      if (re_count > 0) {
        info_log(tlh) << "fpga_gateway GW link reconnected, re-login " << re_count << " cust" << end_log;
      }
    }
  }
  if (cb_mgr_ != nullptr) {
    // 网关模式无独立 trade 链接, 用 tl=0 (GW) 上报
    cb_mgr_->on_link_status(1, 0, 1);
  }
  return 0;
}

// ---- deal_link_close: 链接关闭通知 ----
void fpga_counter_gateway::deal_link_close(int16 link_type) {
  lb_common::lb_log_hand tlh(log_);
  info_log(tlh) << "fpga link close, link_type=" << link_type << end_log;

  if (link_type == LINK_TYPE_SPEED_GW) {
    trade_link_connect = 0;
  }
  if (cb_mgr_ != nullptr) {
    cb_mgr_->on_link_status(1, 0, 0);
  }
  if (link_type == LINK_TYPE_SPEED_GW) {
    if (sec_state_ == 1) {
      cb_mgr_->on_error(err_event_type::get_sec_info, LBAPI_ERR_LINK_DISCONNECTED, "sec not complete by link close");
    }
    // 通知所有处于"登陆中"的客户登陆中断
    for (auto &cached : login_cache_) {
      LoginAns ans;
      build_login_rtn(cached, ans);
      ans.err_code = LBAPI_ERR_LINK_DISCONNECTED;
      std::memcpy(ans.err_msg.data(), "login not complete by link close",
                  std::strlen("login not complete by link close"));
      cb_mgr_->on_login(ans);

      info_log(tlh) << "login not complete by link close,branch_id=" << ans.branch_id.data()
                    << ",fund_account=" << ans.fund_account_id.data() << end_log;
    }
    login_cache_.clear();
  }
}

} // namespace lb_api

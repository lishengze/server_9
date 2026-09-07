// data_engine - 数据加载引擎实现

#include "data_engine.h"

#include <cstdint>
#include <ctime>

#include "data_def.h"
#include "fdm_board.h"
#include "mlog.h"
#include "mutils.h"

namespace lb_fgw {

// ---- init：db 连接 + data_repo + 后台线程 ----
int32_t data_engine::init(const char *dsn, const char *user, const char *passwd, int32_t load_interval,
                          int32_t reconnect_interval, int32_t th_cpu, lb_common::lb_log *log) {
  log_ = log;
  load_interval_ = load_interval;
  if (load_interval_ < 10)
    load_interval_ = 10;
  reconnect_interval = reconnect_interval < 180 ? 180 : reconnect_interval;

  lb_common::lb_log_hand tlh(log_);
  int32_t ret = datas_.init();
  if (ret < 0) {
    error_log(tlh) << "init data repo error, ret=" << ret << end_log;
    return ret;
  }
  info_log(tlh) << "init data repo ok" << end_log;

  ret = db_op_.init_db(dsn, user, passwd, reconnect_interval);
  if (ret < 0) {
    error_log(tlh) << "init db op error, ret=" << ret << end_log;
    return ret;
  }
  info_log(tlh) << "init db op ok" << end_log;

  ret = init_th(0, th_cpu, 1000);
  if (ret < 0) {
    error_log(tlh) << "init load db thread error, ret=" << ret << end_log;
    return ret;
  }
  info_log(tlh) << "init load engine ok, load_interval=" << load_interval_ << ", db_cpu_id_=" << th_cpu << end_log;
  return 0;
}
int32_t data_engine::start() {
  lb_common::lb_log_hand tlh(log_);
  int32_t ret = run();
  if (ret < 0) {
    error_log(tlh) << "start load engine error, ret=" << ret << end_log;
    return ret;
  }
  info_log(tlh) << "start load engine ok" << end_log;
  return 0;
}

/// 停止后台线程
void data_engine::stop() {
  lb_common::lb_log_hand tlh(log_);
  join();
  info_log(tlh) << "stop load engine ok" << end_log;
}

// ---- do_work：周期加载板卡状态 + 客户 + 证券 ----
void data_engine::do_work() {
  lb_common::lb_log_hand tlh(log_);

  std::vector<board_load_info> boards;
  if (db_op_.load_fdms(boards) < 0) {
    lb_common::comm_utils::sleep_s(load_interval_ / 2);
    return; // db 错误，下个周期重试
  }

  for (auto &info : boards) {
    // 仅加载上场完成的板卡
    if (info.data_status_ != '1') {
      continue;
    }

    // 交易日变化：跨天清理 + 重新加载
    if (tradeday_ == 0) {
      tradeday_ = info.tradeday_;
    } else if (tradeday_ < info.tradeday_) {
      fatal_log(tlh) << "change trade_day error to exited, board_no=" << info.board_no_
                     << ", new_day=" << info.tradeday_ << end_log;
      lb_common::comm_utils::sleep_s(5);
      exit(0);
    }
    db_op_.set_trade_day(tradeday_);

    fdm_board *board = nullptr;
    int32_t ret = datas_.find_fdm(board, info.board_no_);
    if (ret == 0) {
      // 板卡已存在：检查上场时间是否更新（重上厂）
      if (!board->check_load_reset(info.data_load_time_)) {
        continue; // upload_time 未变，已加载，跳过
      }
      // upload_time 更新 → 板卡状态已重置为 BOARD_LOAD_NOT，删除板卡，重新加载
      fatal_log(tlh) << "board reload error to exited, board_no=" << info.board_no_ << ", new_day=" << info.tradeday_
                     << end_log;
      lb_common::comm_utils::sleep_s(5);
      exit(0);
    }

    //if (nullptr == board) {
    // 新板卡：创建 + 注册
    board = new fdm_board();
    board->init(info.board_no_, info.market_type_);
    //}

    // 尝试加载证券（DB 判断本板卡是否写入者，非写入者返回空 vec）
    std::vector<sec_info> secs;
    ret = db_op_.load_secs(secs, info.market_type_, info.board_no_);
    if (ret < 0) {
      error_log(tlh) << "load_secs failed, board_no=" << info.board_no_ << ", err=" << db_op_.get_dberr() << end_log;

      delete board;
      lb_common::comm_utils::sleep_s(load_interval_ / 2);
      return;
    } else if (!secs.empty()) {
      datas_.add_sec(secs);
      info_log(tlh) << "securities loaded, board_no=" << info.board_no_ << ", sec_num=" << datas_.get_sec_size()
                    << end_log;
    }

    // 加载客户
    std::vector<customer> custs;
    ret = db_op_.load_custs(custs, info.board_no_);
    if (ret < 0) {
      error_log(tlh) << "load_custs failed, board_no=" << info.board_no_ << ", err=" << db_op_.get_dberr() << end_log;

      delete board;
      lb_common::comm_utils::sleep_s(load_interval_ / 2);
      return;
    } else {
      board->fill_customers(custs);
      for (const auto &c : custs) {
        if (c.fund_account_id_[0] == '\0')
          continue;
        fgw_fundacc_key fkey(c.fund_account_id_, c.branch_id_);
        board_user_id buid;
        buid.board_no_ = c.board_no_;
        buid.user_id_ = c.user_id_;
        ret = datas_.add_cust(fkey, buid);
        if (ret < 0) {
          error_log(tlh) << "add_cust failed, board_no=" << c.board_no_ << ", user_id=" << c.user_id_
                         << ", branch_id=" << c.branch_id_ << ", fund_account_id=" << c.fund_account_id_
                         << ", ret=" << ret << end_log;

          delete board;
          lb_common::comm_utils::sleep_s(load_interval_ / 2);
          return;
        }
      }
    }

    // 加载完成
    board->load_done(info.gw_ip_, info.gw_port_, info.data_load_time_);

    ret = datas_.add_fdm(info.board_no_, board);
    if (ret == 0) {
      info_log(tlh) << "board loaded, board_no=" << info.board_no_ << ", market=" << info.market_type_
                    << ", cust_num=" << custs.size() << ", sec_num=" << secs.size() << ", gw_ip=" << info.gw_ip_ << ":"
                    << info.gw_port_ << end_log;
    } else {
      error_log(tlh) << "insert board map error, board_no=" << info.board_no_ << ", market=" << info.market_type_
                     << ", cust_num=" << custs.size() << ", sec_num=" << secs.size() << ", gw_ip=" << info.gw_ip_ << ":"
                     << info.gw_port_ << ", ret=" << ret << end_log;

      delete board;
      lb_common::comm_utils::sleep_s(load_interval_ / 2);
      return;
    }

    // 立即建链
    fdm_link *link = board->get_link();
    ret = fdm_eng_->start_connect(link, board);
    if (ret < 0) {
      error_log(tlh) << "start_connect failed, board_no=" << info.board_no_ << ", ret=" << ret << end_log;
    }
  }

  lb_common::comm_utils::sleep_s(load_interval_);
}

} // namespace lb_fgw

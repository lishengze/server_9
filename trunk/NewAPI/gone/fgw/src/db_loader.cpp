// db_loader - 数据库操作封装实现
//
// DB Schema（v2 用户确认，2026-06-29）：
//   t_FpgaCounterStatus: TradingDay, BoardNo, MarketType, GWIp, GWPort, DataStatus, DataLoadTime
//   t_SecBaseInfo:       TradingDay, MarketType, SecurityID, BoardNo, SecurityIndex, PriceUnit, ...
//   t_UserBaseInfo:      TradingDay, BoardNo, UserID, BranchID, FundAccountID, AccountID, CustID, ...
//
// 非线程安全（odbc skill §3）：由 data_engine 后台线程独占调用

#include "db_loader.h"
#include "mutils.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

namespace lb_fgw {

// ---- 工具：当前时间（秒）----
static int64_t now_seconds() { return static_cast<int64_t>(std::time(nullptr)); }

// ---- 工具：char(8) YYYYMMDD → int32_t（兼容 DB 字段类型差异）----
static int32_t parse_tradeday(const std::string &s) {
  if (s.size() < 8)
    return 0;
  return std::atoi(s.c_str());
}

// ---- init_db：创建环境 + 连接 + 关自动提交 + 预编译 ----
int32_t db_loader::init_db(const char *srcname, const char *username, const char *passwd, int32_t reconnect_interval) {
  try {
    db_source_ = srcname;
    db_user_ = username;
    db_passwd_ = passwd;
    tradeday_ = 0;
    reconnect_interval_ = reconnect_interval;
    if (reconnect_interval_ < 180)
      reconnect_interval_ = 180;
    last_check_time_ = 0;
    env_ = odbc::Environment::create();
    link_ = env_->createConnection();
    link_->connect(srcname, username, passwd);
    link_->setAutoCommit(false);
    init_statements_();
  } catch (const odbc::Exception &e) {
    db_err_msg_ = std::string("db_loader init failed: ") + e.what();
    return -1;
  }
  return 0;
}

// ---- init_statements_：预编译 SQL（init + 重连重建）----
void db_loader::init_statements_() {
  // t_FpgaCounterStatus: 列名 DataStatus/DataLoadTime/GWIp/GWPort
  op_load_fdms_ = link_->prepareStatement("SELECT TradingDay,BoardNo,MarketType,DataStatus,DataLoadTime,GWPort,GWIp "
                                          "FROM t_FpgaCounterStatus WHERE DataStatus=? AND TradingDay=?");
  // t_SecBaseInfo: 主键 (TradingDay, MarketType, SecurityID)；where 用 (TradingDay, MarketType, BoardNo) 需索引 [Q]
  op_load_secs_ = link_->prepareStatement("SELECT SecurityIndex,SecurityID,MarketType,BuyQtyUnit,SellQtyUnit,PriceUnit "
                                          "FROM t_SecBaseInfo WHERE BoardNo=? AND TradingDay=? AND MarketType=?");
  // t_UserBaseInfo: 股东账户列名 AccountID
  op_load_custs_ = link_->prepareStatement("SELECT BoardNo,UserID,MarketType,BranchID,FundAccountID,CustID,AccountID "
                                           "FROM t_UserBaseInfo WHERE BoardNo=? AND TradingDay=?");
}

// ---- check_connection_：探活 + 重连重建（周期或失败 immediate）----
void db_loader::check_connection_(bool immediate) {
  try {
    int64_t now = now_seconds();
    bool need_check = immediate || (now - last_check_time_ >= reconnect_interval_);
    if (!need_check)
      return;
    last_check_time_ = now;

    bool need_rebuild = false;
    if (link_->connected()) {
      if (!link_->isValid())
        need_rebuild = true;
    } else {
      need_rebuild = true;
    }
    if (need_rebuild) {
      // 重连：新 Connection → connect → setAutoCommit → 重新 prepareStatement
      link_ = env_->createConnection();
      link_->connect(db_source_.c_str(), db_user_.c_str(), db_passwd_.c_str());
      link_->setAutoCommit(false);
      init_statements_(); // 旧 ps 绑定旧 Connection，重连后必重建
    }
  } catch (const odbc::Exception &e) {
    db_err_msg_ = std::string("check_connection failed: ") + e.what();
  }
}

// ---- load_fdms：加载所有上场完成（DataStatus='1'）的 board ----
int32_t db_loader::load_fdms(std::vector<board_load_info> &o_lines) {
  try {
    check_connection_();
    char status = '1'; // 上场完成
    op_load_fdms_->setCString(1, &status, sizeof(status));
    std::string td_str = std::to_string(tradeday_);
    op_load_fdms_->setCString(2, td_str.c_str(), (int32_t)td_str.size());
    odbc::ResultSetRef rs = op_load_fdms_->executeQuery();
    while (rs->next()) {
      board_load_info info;
      std::memset(&info, 0, sizeof(info));
      info.tradeday_ = parse_tradeday(*rs->getString(1));
      info.board_no_ = *rs->getInt(2);
      info.market_type_ = *rs->getInt(3);
      std::string st = *rs->getString(4);
      info.data_status_ = st.empty() ? '0' : st.at(0);
      info.data_load_time_ = *rs->getInt(5);
      info.gw_port_ = *rs->getInt(6);
      std::string ip = *rs->getString(7);
      std::strncpy(info.gw_ip_, ip.c_str(), sizeof(info.gw_ip_) - 1);
      o_lines.push_back(info);
    }
  } catch (const odbc::Exception &e) {
    db_err_msg_ = std::string("load_fdms failed: ") + e.what();
    check_connection_(true);
    return -1;
  }
  return 0;
}

// ---- load_secs：加载某 board 某市场的全量证券 ----
int32_t db_loader::load_secs(std::vector<sec_info> &o_lines, int16_t market, uint16_t boardno) {
  try {
    check_connection_();
    std::string td_str = std::to_string(tradeday_);
    op_load_secs_->setInt(1, boardno);
    op_load_secs_->setCString(2, td_str.c_str(), (int32_t)td_str.size());
    op_load_secs_->setInt(3, market);
    odbc::ResultSetRef rs = op_load_secs_->executeQuery();
    while (rs->next()) {
      sec_info info;
      std::memset(&info, 0, sizeof(info));
      info.sec_index_ = *rs->getInt(1);
      std::string sid = *rs->getString(2);
      std::strncpy(info.security_id_, sid.c_str(), sizeof(info.security_id_) - 1);
      info.market_type_ = *rs->getInt(3);
      info.buy_qty_unit_ = *rs->getInt(4);
      info.sell_qty_unit_ = *rs->getInt(5);
      info.price_unit_ = *rs->getInt(6);
      o_lines.push_back(info);
    }
  } catch (const odbc::Exception &e) {
    db_err_msg_ = std::string("load_secs failed: ") + e.what();
    check_connection_(true);
    return -1;
  }
  return 0;
}

// ---- load_custs：加载某 board 的全量用户 ----
int32_t db_loader::load_custs(std::vector<customer> &o_lines, uint16_t boardno) {
  try {
    check_connection_();
    std::string td_str = std::to_string(tradeday_);
    op_load_custs_->setInt(1, boardno);
    op_load_custs_->setCString(2, td_str.c_str(), (int32_t)td_str.size());
    odbc::ResultSetRef rs = op_load_custs_->executeQuery();
    while (rs->next()) {
      customer c;
      std::memset(&c, 0, sizeof(c));
      c.board_no_ = *rs->getInt(1);
      c.user_id_ = *rs->getInt(2);
      c.market_type_ = *rs->getInt(3);
      std::string br = *rs->getString(4);
      std::strncpy(c.branch_id_, br.c_str(), sizeof(c.branch_id_) - 1);
      std::string fa = *rs->getString(5);
      std::strncpy(c.fund_account_id_, fa.c_str(), sizeof(c.fund_account_id_) - 1);
      std::string ci = *rs->getString(6);
      std::strncpy(c.cust_id_, ci.c_str(), sizeof(c.cust_id_) - 1);
      std::string ha = *rs->getString(7);
      std::strncpy(c.holder_acc_, ha.c_str(), sizeof(c.holder_acc_) - 1);
      o_lines.push_back(c);
    }
  } catch (const odbc::Exception &e) {
    db_err_msg_ = std::string("load_custs failed: ") + e.what();
    check_connection_(true);
    return -1;
  }
  return 0;
}

// ---- close_db：主动断开 + Ref 释放（顺序：ps → Connection → Environment）----
void db_loader::close_db() {
  try {
    if (!link_.isNull() && link_->connected())
      link_->disconnect();
  } catch (const odbc::Exception &e) {
    db_err_msg_ = std::string("close_db failed: ") + e.what();
  }
  op_load_fdms_.reset();
  op_load_secs_.reset();
  op_load_custs_.reset();
  link_.reset();
  env_.reset();
}

// ---- commit_trans ----
void db_loader::commit_trans() {
  try {
    if (!link_.isNull())
      link_->commit();
  } catch (const odbc::Exception &e) {
    db_err_msg_ = std::string("commit failed: ") + e.what();
  }
}

} // namespace lb_fgw

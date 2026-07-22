#pragma once

#include "comm_sys.h"
#include "data_def.h"
#include <cstdint>
#include <odbc/Connection.h>
#include <odbc/Environment.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>

#include <string>
#include <vector>

// 数据库操作封装（非线程安全，由 data_engine 后台线程独占）
// 参考 third_odbc example/sysdbfunc.cpp：长连接 + 预编译复用 + 重连重建

namespace lb_fgw {

class db_loader {
public:
  // 此函数会周期性调用 , 加载所有状态为上场完成的 fdm board 信息
  // 数据库表 ： t_FpgaCounterStatus
  int32_t load_fdms(std::vector<board_load_info> &o_lines);
  // 某个 fdm 上场完成后，加载其全量的证券信息，若证券信息表不是该 fdm 写入，返回空的 vector
  // 数据库表 ： t_SecBaseInfo
  int32_t load_secs(std::vector<sec_info> &o_lines, int16_t market, uint16_t boardno);
  // 某个 fdm 上场完成后，加载其全量的用户信息
  // 数据库表 ： t_UserBaseInfo
  int32_t load_custs(std::vector<customer> &o_lines, uint16_t boardno);

  const char *get_dberr() { return db_err_msg_.c_str(); }

  //参数待定
  int32_t init_db(const char *srcname, const char *username, const char *passwd, int32_t reconnect_interval);
  void set_trade_day(int32_t trade_day) { tradeday_ = trade_day; }
  void close_db();
  void commit_trans();

  db_loader() : tradeday_(0), last_check_time_(0){};
  ~db_loader(){};

private:
  odbc::EnvironmentRef env_;
  odbc::ConnectionRef link_;
  odbc::PreparedStatementRef op_load_fdms_;
  odbc::PreparedStatementRef op_load_secs_;
  odbc::PreparedStatementRef op_load_custs_;
  std::string db_err_msg_;
  std::string db_source_;
  std::string db_user_;
  std::string db_passwd_;

  int32_t tradeday_; ///< 当前交易日 YYYYMMDD（init_db 时算）
  int32_t reconnect_interval_;
  int64_t last_check_time_; ///< 上次连接探活时间（秒，5min 周期）

  /// 预编译所有 SQL（init + 重连重建；旧 ps 绑定旧 Connection，重连后必重建）
  void init_statements_();
  /// 连接探活 + 重连重建（5min 周期或 immediate）
  void check_connection_(bool immediate = false);
};

} // namespace lb_fgw

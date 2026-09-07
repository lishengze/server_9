// data_engine - 数据加载引擎（含 sec_info 结构）
//
// Step 5 决策：data_engine 是 sec_info 的拥有者（不在 data_repo）
//   - sec_info：data_engine::secs_ 持有，故定义在 data_engine.h
//
// Step 5 P-4：执行整合 — 数据加载线程 + 数据访问 → 数据引擎
// R-13：data_repo 是 data_engine 拥有的数据子模块（其他模块指针只读访问）
//
// 线程：simple_thread 后台线程（等待触发，1s 唤醒，do_work 内 30s 计时）
//   - 启动 + 周期 30s 加载板卡状态（load_fdms）
//   - 板卡"上场完成"时懒加载客户（load_custs，db）
//   - 证券同步（secs_）由 S10 fdm SEC_INFO_ANS 填充（需求 §11 非 db）
//
// 接口（Step 8 S6 增量）：
//   init(dsn,user,pwd,log) + start + do_work（周期加载板卡 + 客户）
//   fdm 连接 + 网关登录由 fgw_instance 编排（S7/S11）

#pragma once

#include <cstdint>

#include "fdm_engine.h"

#include "data_repo.h"
#include "db_loader.h"

#include "mlog.h"
#include "simple_thread.h"

namespace lb_fgw {

/// 数据加载引擎（继承 simple_thread，do_work 周期循环驱动）
class data_engine : public lb_common::simple_thread {
public:
  /// 初始化：db 连接 + data_repo + 后台线程
  int32_t init(const char *dsn, const char *user, const char *passwd, int32_t load_interval, int32_t reconnect_interval,
               int32_t th_cpu, lb_common::lb_log *log);

  /// 注入 fdm_engine 引用（load_done 后立即建链；在 fdm_engine::init 之后调用）
  void set_fdm_engine(fdm_engine *fdm_eng) { fdm_eng_ = fdm_eng; }

  /// 启动后台线程
  int32_t start();

  /// 停止后台线程
  void stop();

  /// 数据访问（其他模块只读引用）
  data_repo *get_datas() { return &datas_; }

protected:
  /// simple_thread 后台线程主循环（30s 周期加载板卡状态 + 客户）
  void do_work() override;

  /// simple_thread 触发条件（默认 false，靠 epoll 1s 超时唤醒）
  bool need_work() override { return false; }

private:
  // 拥有的数据子模块（R-13）
  data_repo datas_;
  db_loader db_op_;                  ///< 数据库操作封装
  fdm_engine *fdm_eng_ = nullptr;    ///< fdm 引擎引用（load_done 后立即建链）
  int32_t tradeday_ = 0;             ///< 交易日
  int32_t load_interval_ = 10;       ///< 加载周期（秒，可配）
  lb_common::lb_log *log_ = nullptr; ///< 日志
};

} // namespace lb_fgw

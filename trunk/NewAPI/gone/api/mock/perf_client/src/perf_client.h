// perf_client.h - FTE 委托通路性能测试客户端
//
// 职责：
//   1. 从 JSON 配置加载连接配置 + 测试参数（测试时长 / TPS / warmup / CPU 等）
//   2. 初始化 API（创建 api_config → create_instance → start）并登录 FTE
//   3. 匀速发单（每笔间隔 1/TPS 秒），调用 api_->order_insert()
//   4. 通过 OrderReq 的 api_arrive_time_ns / api_leave_time_ns 计算每笔 api 内耗时
//   5. 收集全部耗时样本，供 metric_stats 分析
//
// 性能测量语义：
//   api_arrive_time_ns 在 api_impl::order_insert 入口记录（请求到达 api）
//   api_leave_time_ns  在 api_impl::order_insert 出口记录（请求离开 api）
//   耗时 = api_leave_time_ns - api_arrive_time_ns（即 api 内处理委托的时间）

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "api_callback.h"
#include "api_interface.h"

namespace perf {

/// 性能测试回调：记录登录结果与链接状态
class PerfCallback : public lb_api::api_callback {
public:
  void on_login(const lb_api::LoginAns& ans) override;
  void on_link_status(int32_t counter_type, int32_t link_type, int32_t status) override;
  void on_error(lb_api::err_event_type event_type, int32_t err_code, const char* err_desc) override;

  bool login_done = false;   ///< 是否已收到登录应答
  bool login_ok = false;     ///< 登录是否成功
  int32_t last_err = 0;      ///< 最近错误码
  int32_t link_status = 0;   ///< 极速链接状态
};

/// 性能测试配置（JSON 加载）
struct PerfConfig {
  // ---- 连接配置 ----
  std::string api_instance_name;
  int32_t market_type = 1;
  int32_t fast_counter_type = 1;
  int32_t speed_link_type = 1;
  std::string speed_counter_ip;
  int32_t speed_counter_port = 0;
  std::string counter98_ip;
  int32_t counter98_port = 0;
  std::string agw_user;
  std::string agw_user_password;
  int32_t heartbeat_interval = 5;
  int32_t agw_user_login_timeout = 10;
  int32_t log_level = 0;
  std::string log_output_dir;

  // ---- 测试参数 ----
  int32_t test_duration_sec = 10;  ///< 测试时长（秒），默认 10
  int32_t tps = 1000;              ///< 每秒发单量（匀速，间隔 = 1/TPS 秒）
  int32_t warmup_sec = 3;          ///< 启动后等待秒数（确保绑核成功）
  int32_t cpu_id = -1;             ///< 绑定的 CPU 编号（-1 不绑定）
  std::string report_file;         ///< 报告输出文件

  // ---- 委托模板 ----
  std::string fund_account_id;
  std::string branch_id;
  std::string password;          ///< 登录密码
  char side = '1';
  char order_type = '2';
  std::string security_id;
  int64_t order_price = 0;
  int64_t order_qty = 0;
  int64_t stop_price = 0;
  int32_t order_market_type = 1;

  /// 从 JSON 文件加载配置
  bool load(const std::string& path);
};

/// 性能测试客户端
class PerfClient {
public:
  /// 初始化：加载配置 + 创建 API 实例 + start
  bool init(const std::string& config_path);

  /// 登录 FTE（含 98agw → 98账户 → 极速柜台全链路），等待回调确认
  bool login(int timeout_ms = 30000);

  /// 发送一笔委托，返回 api_->order_insert 的返回值
  int32_t send_order(lb_api::OrderReq& req);

  /// 关闭并释放资源
  void shutdown();

  const PerfConfig& config() const { return cfg_; }

private:
  PerfConfig cfg_;
  lb_api::api_interface* api_ = nullptr;
  PerfCallback* cb_ = nullptr;
};

/// 匀速发单并收集每笔耗时（纳秒）到 latencies
void run_benchmark(PerfClient& client, const PerfConfig& cfg, std::vector<uint64_t>& latencies);

} // namespace perf
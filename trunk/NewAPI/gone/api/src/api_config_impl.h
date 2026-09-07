#pragma once

#include "api_config.h"
#include <cstdint>
#include <cstring>
#include <string>

namespace lb_api {

/// 属性期望类型(内部类型检查用)
enum class attr_type : int32_t {
  int16_val = 0,
  int32_val = 1,
  int64_val = 2,
  bool_val = 3,
  string_val = 4,
  addr_val = 5,
  int8_val = 6
};

/// api_config的具体实现类
///
/// 内部持有所有配置属性值，每个属性有默认值
/// 实现虚接口set_attr/get_attr，内部做属性名和类型校验
/// 提供便捷getter方法供内部模块直接使用(非虚，无类型间接)
class api_config_impl : public api_config {
public:
  api_config_impl();
  ~api_config_impl() override;

  // ---- 虚接口实现 ----
  int32_t set_attr(const char *attr_name, int8_t attr_val) override;
  int32_t set_attr(const char *attr_name, int16_t attr_val) override;
  int32_t set_attr(const char *attr_name, int32_t attr_val) override;
  int32_t set_attr(const char *attr_name, int64_t attr_val) override;
  int32_t set_attr(const char *attr_name, const char *attr_val) override;
  int32_t set_attr(const char *attr_name, const std::string &attr_val) override;
  int32_t set_attr(const char *attr_name, bool attr_val) override;
  int32_t set_attr(const char *attr_name, const net_addr &attr_val) override;

  int32_t get_attr(const char *attr_name, int8_t &o_val) const override;
  int32_t get_attr(const char *attr_name, int16_t &o_val) const override;
  int32_t get_attr(const char *attr_name, int32_t &o_val) const override;
  int32_t get_attr(const char *attr_name, int64_t &o_val) const override;
  int32_t get_attr(const char *attr_name, char *o_val, int32_t buf_len) const override;
  int32_t get_attr(const char *attr_name, std::string &o_val) const override;
  int32_t get_attr(const char *attr_name, bool &o_val) const override;
  int32_t get_attr(const char *attr_name, net_addr &o_val) const override;

  int32_t validate() const override;
  int32_t copy_from(const api_config &src) override;

  // ---- 便捷获取方法(非虚，内部使用) ----

  /// 获取极速柜台类型
  counter_type get_fast_counter_type() const { return fast_counter_type_; }
  /// 获取市场类型
  market_type_t get_market_type() const { return market_type_; }
  /// 获取极速链接类型
  speed_link_type get_speed_link_type() const { return speed_link_type_; }
  /// 获取 Solarflare 网卡接口名 (tcpdirect 模式时必填)
  const char *get_solarflare_iface() const { return solarflare_iface_; }
  /// 获取极速柜台主地址
  net_addr get_speed_counter_addr() const { return speed_counter_addr_; }
  /// 获取极速柜台备地址 (故障切换用)
  net_addr get_speed_counter_addr_bak() const { return speed_counter_addr_bak_; }
  /// 获取 98 柜台地址
  net_addr get_counter98_addr() const { return counter98_addr_; }
  /// 获取 98 柜台备地址
  net_addr get_counter98_addr_bak() const { return counter98_addr_bak_; }
  /// 获取回调模式 (direct/queued)
  callback_mode get_callback_mode() const { return callback_mode_; }
  // run_mode 概念已合并到 fast_counter_type，无独立 getter
  /// 获取发送轮询批量数
  int32_t get_send_poll_num() const { return send_poll_num_; }
  /// 获取接收轮询批量数
  int32_t get_recv_poll_num() const { return recv_poll_num_; }
  /// 获取心跳间隔(秒)
  int32_t get_heartbeat_interval() const { return heartbeat_interval_; }
  /// 获取最大重连次数 (0=无限)
  int32_t get_max_reconnect_count() const { return max_reconnect_count_; }
  /// 获取极速引擎 CPU 亲和 (-1=不绑定)
  int32_t get_speed_engine_cpu() const { return speed_engine_cpu_; }
  /// 获取管理引擎 CPU 亲和 (-1=不绑定)
  int32_t get_mgmt_engine_cpu() const { return mgmt_engine_cpu_; }
  /// 获取回调线程 CPU 亲和 (-1=不绑定)
  int32_t get_callback_thread_cpu() const { return callback_thread_cpu_; }
  /// 获取发送队列大小 (MB)
  int32_t get_send_queue_size_mb() const { return send_queue_size_mb_; }
  /// 获取回调队列大小 (MB)
  int32_t get_callback_queue_size_mb() const { return callback_queue_size_mb_; }
  /// 获取回调线程 epoll 等待毫秒 (0=死轮询)
  int32_t get_callback_wait_ms() const { return callback_wait_ms_; }
  /// 获取多 socket 引擎 epoll_wait 等待毫秒 (0=不等待)
  int32_t get_multi_io_wait_ms() const { return multi_io_wait_ms_; }
  /// 获取 98 网关用户名
  const char *get_agw98_user() const { return agw98_user_; }
  /// 获取 98 网关用户密码
  const char *get_agw98_user_password() const { return agw98_user_password_; }
  /// 获取 AGW 用户登录超时 (秒)
  int32_t get_agw_user_login_timeout() const { return agw_user_login_timeout_; }
  // 日志相关
  /// 获取日志队列大小 (MB, 0=同步写文件)
  int32_t get_log_queue_size_mb() const { return log_queue_size_mb_; }
  /// 获取日志级别 (1=通知)
  int32_t get_log_level() const { return log_level_; }
  /// 获取 API 实例名称
  const char *get_api_instance_name() const { return api_instance_name_; }
  /// 获取日志输出目录
  const char *get_log_output_dir() const { return log_output_dir_; }

private:
  /// 检查属性名是否已知且期望类型匹配
  int32_t check_attr_type(const char *attr_name, attr_type expected) const;

  // ---- 成员变量(均有默认值) ----

  counter_type fast_counter_type_;  ///< 极速柜台类型(默认: fixed_98, 必须设置)
  market_type_t market_type_;       ///< 市场类型(默认: 0, 必须设置, 1=上海, 2=深交所/北交所)
  speed_link_type speed_link_type_; ///< 极速链接类型(默认: socket_single)
  char solarflare_iface_[64];       ///< Solarflare网卡接口名(默认: 空, tcpdirect模式时必须设置)
  net_addr speed_counter_addr_;     ///< 极速柜台地址-主(默认: 空, 必须设置)
  net_addr speed_counter_addr_bak_; ///< 极速柜台地址-备(默认: 空)
  net_addr counter98_addr_;         ///< 98柜台地址(默认: 空, 必须设置)
  net_addr counter98_addr_bak_;     ///< 98柜台地址-备(默认: 空)
  callback_mode callback_mode_;     ///< 回调模式(默认: direct)
  int32_t send_poll_num_;           ///< 发送轮询批量数(默认: 6)
  int32_t recv_poll_num_;           ///< 接收轮询批量数(默认: 2)
  int32_t heartbeat_interval_;      ///< 心跳间隔秒数(默认: 5)
  int32_t max_reconnect_count_;     ///< 最大重连次数(默认: 0=无限)
  int32_t speed_engine_cpu_;        ///< 极速引擎CPU亲和(默认: -1)
  int32_t mgmt_engine_cpu_;         ///< 管理引擎CPU亲和(默认: -1)
  int32_t callback_thread_cpu_;     ///< 回调线程CPU亲和(默认: -1)
  int32_t send_queue_size_mb_;      ///< 发送队列大小/MB(默认: 2)
  int32_t callback_queue_size_mb_;  ///< 回调队列大小/MB(默认: 8)
  int32_t callback_wait_ms_;        ///< 回调线程epoll等待毫秒(默认: 10, 0=死轮询)
  int32_t multi_io_wait_ms_;        ///< 多socket引擎epoll_wait等待毫秒(默认: 100, 0=不等待)
  char agw98_user_[32];             ///< 98网关用户名(默认: 空, 必须设置)
  char agw98_user_password_[256];   ///< 98网关用户密码(默认: 空, 必须设置)
  int32_t agw_user_login_timeout_;  ///< AGW 登录超时秒数（默认: 10；建链超时由基础库提供）
  int32_t log_queue_size_mb_;       ///< 日志队列大小/MB(默认: 8, 0=同步写文件)
  int32_t log_level_;               ///< 日志级别(默认: 1=通知)
  char api_instance_name_[64];      ///< API实例名称(默认: 空, 必须设置)
  char log_output_dir_[256];        ///< 日志输出目录(默认: ./api_log)
};

} // namespace lb_api

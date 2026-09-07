// fgw_config - fgw 配置结构
//
// Step 5 已确认结构体；Step 8 S11 定义成员（fgw_instance::load_config 填充）
// [Q] 配置文件格式待定（暂用 ini/key=value，load_config 对接 common ini_file）

#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

namespace lb_fgw {

/// fgw 配置结构
struct fgw_config {
  // 日志
  char log_path_[256];   ///< 日志路径
  int32_t log_level_;    ///< 日志级别
  int32_t log_que_size_; ///< 单位M , 0 -同步写文件

  // db 引擎
  char db_source_[128];          ///< 数据库源
  char db_user_[128];            ///< 数据库账户
  char db_passwd_[512];          ///< 数据库账户密码
  int32_t db_reconnect_interval; ///< 数据库链接异常重连间隔
  int32_t db_load_interval_;     ///< data_engine 加载周期（秒，默认 10）
  int32_t db_cpu_id_;            ///< db 线程绑定的cpu

  // api 引擎
  char api_listen_ip_[32];               ///< api 监听ip
  int32_t api_listen_port_;              ///< api 监听端口
  int32_t api_heart_interval_;           ///< 心跳间隔，默认 5s
  int32_t api_que_size_;                 ///< 单位M
  int32_t api_thread_num_;               ///< api 引擎线程数（= 发送队列数）
  std::vector<int32_t> api_thread_cpus_; ///< 绑定的cpu, 若设置，应与数量一致

  // fdm 引擎
  int32_t fdm_heart_interval_;           ///< 心跳间隔，默认 5s
  int32_t fdm_que_size_;                 ///< 单位M
  int32_t fdm_thread_num_;               ///< fdm 引擎线程数（= 发送队列数）
  std::vector<int32_t> fdm_thread_cpus_; ///< 绑定的cpu, 若设置，应与数量一致

  fgw_config() {
    std::memset(log_path_, 0, sizeof(log_path_));
    log_level_ = 0;
    log_que_size_ = 0;

    std::memset(db_source_, 0, sizeof(db_source_));
    std::memset(db_user_, 0, sizeof(db_user_));
    std::memset(db_passwd_, 0, sizeof(db_passwd_));
    db_reconnect_interval = 300;
    db_load_interval_ = 10;
    db_cpu_id_ = -1;

    std::memset(api_listen_ip_, 0, sizeof(api_listen_ip_));
    api_listen_port_ = 0;
    api_heart_interval_ = 5;
    api_que_size_ = 16;
    api_thread_num_ = 4;

    fdm_heart_interval_ = 5;
    fdm_que_size_ = 32;
    fdm_thread_num_ = 4;
  }
};

} // namespace lb_fgw

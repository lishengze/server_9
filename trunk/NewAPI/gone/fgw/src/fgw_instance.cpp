// fgw_instance - fgw 服务实例（编排入口）实现
//
// 流程：
//   init  : load_config（ini_reader 读 fgw.cfg） + log_.open_log + 3 引擎 init + 依赖注入
//   start : data_eng → fdm_eng → api_eng
//   stop  : 停 api/fdm/data + 关日志
//   periodic_check : fdm 板卡连接/重连/心跳 + api 心跳超时

#include "fgw_instance.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "comm_sock.h"
#include "fgw_config.h"
#include "ini_file.h"
#include "mlog.h"
#include "mutils.h"

namespace lb_fgw {

// ---- load_config：用 ini_reader 读 fgw.cfg ----
// 节：[log] [db] [api] [fdm]
// 策略：open 失败 return ret；关键字段（listen_port/db_source/user/passwd/thread_num）缺失 → return -1；
//      非关键字段缺失保留构造默认值。
int fgw_instance::load_config(fgw_config &o_cfg, const char *cfg_file) {
  if (cfg_file == nullptr) {
    std::fprintf(stderr, "fgw load config: cfg_file is null\n");
    return -1;
  }

  lb_common::ini_reader tini;
  int32_t ret = tini.open(cfg_file);
  if (ret < 0) {
    std::fprintf(stderr, "fgw open ini failed, file=%s, ret=%d\n", cfg_file, ret);
    return ret;
  }

  // ---- [log] ----
  tini.read("log", "path", o_cfg.log_path_, sizeof(o_cfg.log_path_));
  tini.read("log", "level", o_cfg.log_level_);
  tini.read("log", "que_size", o_cfg.log_que_size_);

  // ---- [db] ----
  ret = tini.read("db", "source", o_cfg.db_source_, sizeof(o_cfg.db_source_));
  if (ret != 0) {
    std::fprintf(stderr, "fgw read [db] source failed, ret=%d\n", ret);
    tini.close();
    return -1;
  }
  ret = tini.read("db", "user", o_cfg.db_user_, sizeof(o_cfg.db_user_));
  if (ret != 0) {
    std::fprintf(stderr, "fgw read [db] user failed, ret=%d\n", ret);
    tini.close();
    return -1;
  }
  tini.read("db", "passwd", o_cfg.db_passwd_, sizeof(o_cfg.db_passwd_)); // 允许空
  tini.read("db", "reconnect_interval", o_cfg.db_reconnect_interval);
  tini.read("db", "load_interval", o_cfg.db_load_interval_);
  tini.read("db", "cpu_id", o_cfg.db_cpu_id_);

  // ---- [api] ----
  ret = tini.read("api", "listen_ip", o_cfg.api_listen_ip_, sizeof(o_cfg.api_listen_ip_));
  if (ret != 0) {
    std::fprintf(stderr, "fgw read [api] listen_ip failed, ret=%d\n", ret);
    tini.close();
    return -1;
  }
  ret = tini.read("api", "listen_port", o_cfg.api_listen_port_);
  if (ret != 0) {
    std::fprintf(stderr, "fgw read [api] listen_port failed, ret=%d\n", ret);
    tini.close();
    return -1;
  }
  tini.read("api", "heart_interval", o_cfg.api_heart_interval_);
  tini.read("api", "que_size", o_cfg.api_que_size_);
  ret = tini.read("api", "thread_num", o_cfg.api_thread_num_);
  if (ret != 0) {
    std::fprintf(stderr, "fgw read [api] thread_num failed, ret=%d\n", ret);
    tini.close();
    return -1;
  }
  tini.read_list("api", "thread_cpus", o_cfg.api_thread_cpus_);

  // ---- [fdm] ----
  tini.read("fdm", "heart_interval", o_cfg.fdm_heart_interval_);
  tini.read("fdm", "que_size", o_cfg.fdm_que_size_);
  ret = tini.read("fdm", "thread_num", o_cfg.fdm_thread_num_);
  if (ret != 0) {
    std::fprintf(stderr, "fgw read [fdm] thread_num failed, ret=%d\n", ret);
    tini.close();
    return -1;
  }
  tini.read_list("fdm", "thread_cpus", o_cfg.fdm_thread_cpus_);

  tini.close();
  return 0;
}

// ---- init：读配置 + 初始化 3 引擎 + 注入依赖 ----
int fgw_instance::init(const char *cfg_file) {
  fgw_config cfg_; // 走构造函数默认（非关键字段缺省回退）

  int32_t ret = load_config(cfg_, cfg_file);
  if (ret < 0) {
    std::fprintf(stderr, "fgw load config file error, file=%s, ret=%d\n", cfg_file, ret);
    return ret;
  }

  int32_t tdate = lb_common::comm_utils::get_date();
  ret = log_.open_log(cfg_.log_path_, "fgw", cfg_.log_level_, tdate, cfg_.log_que_size_);
  if (ret < 0) {
    std::fprintf(stderr, "fgw open log failed, path=%s, ret=%d\n", cfg_.log_path_, ret);
    return ret;
  }

  lb_common::lb_log_hand tlh(&log_);
  info_log(tlh) << "fgw config loaded: listen=" << cfg_.api_listen_ip_ << ":" << cfg_.api_listen_port_
                << ", db=" << cfg_.db_source_ << ", load_interval=" << cfg_.db_load_interval_
                << ", api_thread_num=" << cfg_.api_thread_num_ << ", fdm_thread_num=" << cfg_.fdm_thread_num_
                << end_log;

  // data_engine：dsn + user + passwd + load_interval + reconnect_interval + th_cpu + log
  ret = data_eng_.init(cfg_.db_source_, cfg_.db_user_, cfg_.db_passwd_, cfg_.db_load_interval_,
                       cfg_.db_reconnect_interval, cfg_.db_cpu_id_, &log_);
  if (ret < 0) {
    error_log(tlh) << "data engine init failed, ret=" << ret << end_log;
    return ret;
  }

  // api_engine：datas + heart + thread + que + listen_addr + log + cpus
  lb_common::csock_addr api_addr;
  std::strncpy(api_addr.ip, cfg_.api_listen_ip_, sizeof(api_addr.ip) - 1);
  api_addr.ip[sizeof(api_addr.ip) - 1] = '\0';
  api_addr.port = cfg_.api_listen_port_;
  ret = api_eng_.init(data_eng_.get_datas(), cfg_.api_heart_interval_, cfg_.api_thread_num_, cfg_.api_que_size_,
                      api_addr, &log_, cfg_.api_thread_cpus_);
  if (ret < 0) {
    error_log(tlh) << "api engine init failed, ret=" << ret << end_log;
    return ret;
  }

  // fdm_engine：datas + heart + thread + que + api_eng + log + cpus
  ret = fdm_eng_.init(data_eng_.get_datas(), cfg_.fdm_heart_interval_, cfg_.fdm_thread_num_, cfg_.fdm_que_size_,
                      &api_eng_, &log_, cfg_.fdm_thread_cpus_);
  if (ret < 0) {
    error_log(tlh) << "fdm engine init failed, ret=" << ret << end_log;
    return ret;
  }

  // data_engine 注入 fdm_engine 引用（load_done 后立即建链）
  data_eng_.set_fdm_engine(&fdm_eng_);

  info_log(tlh) << "fgw init ok" << end_log;
  return 0;
}

// ---- start：启动后台加载 + fdm + api 监听 ----
int fgw_instance::start() {
  int32_t ret = data_eng_.start();
  if (ret < 0)
    return ret;

  ret = fdm_eng_.start();
  if (ret < 0)
    return ret;

  ret = api_eng_.start();
  if (ret < 0)
    return ret;

  return 0;
}

// ---- stop：停止接收 + 关闭链接 + 停线程池 ----
void fgw_instance::stop() {
  api_eng_.stop();
  fdm_eng_.stop();
  data_eng_.stop();
  log_.close_log();
}

// ---- periodic_check：fdm 连接/重连 + api 心跳超时 ----
void fgw_instance::periodic_check() {
  fdm_eng_.check_board_link();
  api_eng_.check_heartbeat();
}

} // namespace lb_fgw

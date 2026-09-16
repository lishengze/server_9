// perf_client.cpp - FTE 委托通路性能测试客户端实现

#include "perf_client.h"

#include "api_config.h"

#include "../include/json_utils.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace perf {

using mock::JsonParser;
using mock::JsonValue;

// ==================== PerfCallback ====================

void PerfCallback::on_login(const lb_api::LoginAns& ans) {
  login_done = true;
  login_ok = (ans.err_code == 0);
  last_err = ans.err_code;
  std::cout << "[PerfCallback] on_login err_code=" << ans.err_code << std::endl;
}

void PerfCallback::on_link_status(int32_t counter_type, int32_t link_type, int32_t status) {
  link_status = status;
  std::cout << "[PerfCallback] on_link_status counter=" << counter_type
            << " link=" << link_type << " status=" << status << std::endl;
}

void PerfCallback::on_error(lb_api::err_event_type event_type, int32_t err_code, const char* err_desc) {
  std::cout << "[PerfCallback] on_error type=" << static_cast<int>(event_type)
            << " code=" << err_code
            << " desc=" << (err_desc ? err_desc : "") << std::endl;
}

// ==================== PerfConfig ====================

bool PerfConfig::load(const std::string& path) {
  try {
    JsonValue root = JsonParser::parse_file(path);

    // 连接配置
    api_instance_name = root["api_instance_name"].as_string();
    market_type = static_cast<int32_t>(root["market_type"].as_int());
    fast_counter_type = static_cast<int32_t>(root["fast_counter_type"].as_int());
    speed_link_type = static_cast<int32_t>(root["speed_link_type"].as_int());

    JsonValue sca = root["speed_counter_addr"];
    speed_counter_ip = sca["ip"].as_string();
    speed_counter_port = static_cast<int32_t>(sca["port"].as_int());

    JsonValue c98a = root["counter98_addr"];
    counter98_ip = c98a["ip"].as_string();
    counter98_port = static_cast<int32_t>(c98a["port"].as_int());

    agw_user = root["98agw_user"].as_string();
    agw_user_password = root["98agw_user_password"].as_string();
    heartbeat_interval = static_cast<int32_t>(root["heartbeat_interval"].as_int());
    agw_user_login_timeout = static_cast<int32_t>(root["agw_user_login_timeout"].as_int());
    log_level = static_cast<int32_t>(root["log_level"].as_int());
    log_output_dir = root["log_output_dir"].as_string();

    // 测试参数
    test_duration_sec = static_cast<int32_t>(root["test_duration_sec"].as_int());
    tps = static_cast<int32_t>(root["tps"].as_int());
    warmup_sec = static_cast<int32_t>(root["warmup_sec"].as_int());
    cpu_id = static_cast<int32_t>(root["cpu_id"].as_int());
    report_file = root["report_file"].as_string();

    // 委托模板
    JsonValue od = root["order"];
    fund_account_id = od["fund_account_id"].as_string();
    branch_id = od["branch_id"].as_string();
    password = od["password"].as_string();
    std::string side_s = od["side"].as_string();
    side = side_s.empty() ? '1' : side_s[0];
    std::string ot_s = od["order_type"].as_string();
    order_type = ot_s.empty() ? '2' : ot_s[0];
    security_id = od["security_id"].as_string();
    order_price = od["order_price"].as_int();
    order_qty = od["order_qty"].as_int();
    stop_price = od["stop_price"].as_int();
    order_market_type = static_cast<int32_t>(od["market_type"].as_int());

    return true;
  } catch (const std::exception& e) {
    std::cerr << "[PerfConfig] 加载失败: " << e.what() << std::endl;
    return false;
  }
}

// ==================== PerfClient ====================

bool PerfClient::init(const std::string& config_path) {
  if (!cfg_.load(config_path)) {
    return false;
  }

  // 创建 API 配置
  lb_api::api_config* cfg = lb_api::api_config::create_config();
  if (!cfg) {
    std::cerr << "[PerfClient] create_config 失败" << std::endl;
    return false;
  }

  int32_t ra = 0;
  ra = cfg->set_attr("api_instance_name", cfg_.api_instance_name.c_str());
  ra = cfg->set_attr("market_type", static_cast<int8_t>(cfg_.market_type));
  ra = cfg->set_attr("fast_counter_type", static_cast<int32_t>(cfg_.fast_counter_type));
  ra = cfg->set_attr("speed_link_type", static_cast<int32_t>(cfg_.speed_link_type));

  std::string speed_addr = cfg_.speed_counter_ip + ":" + std::to_string(cfg_.speed_counter_port);
  ra = cfg->set_attr("speed_counter_addr", speed_addr.c_str());
  std::string c98_addr = cfg_.counter98_ip + ":" + std::to_string(cfg_.counter98_port);
  ra = cfg->set_attr("counter98_addr", c98_addr.c_str());
  ra = cfg->set_attr("98agw_user", cfg_.agw_user.c_str());
  ra = cfg->set_attr("98agw_user_password", cfg_.agw_user_password.c_str());
  ra = cfg->set_attr("heartbeat_interval", static_cast<int32_t>(cfg_.heartbeat_interval));
  ra = cfg->set_attr("agw_user_login_timeout", static_cast<int32_t>(cfg_.agw_user_login_timeout));
  ra = cfg->set_attr("log_level", static_cast<int32_t>(cfg_.log_level));
  ra = cfg->set_attr("log_output_dir", cfg_.log_output_dir.c_str());
  (void)ra;

  // 创建回调
  cb_ = new PerfCallback();

  // 创建 API 实例
  int32_t ret = lb_api::api_interface::create_instance(api_, *cfg, cb_);
  lb_api::api_config::destroy_config(cfg);

  if (ret != 0 || !api_) {
    std::cerr << "[PerfClient] create_instance 失败: " << ret << std::endl;
    delete cb_;
    cb_ = nullptr;
    return false;
  }
  std::cout << "[PerfClient] API 实例创建成功" << std::endl;

  // 启动
  ret = api_->start();
  if (ret != 0) {
    std::cerr << "[PerfClient] api->start() 失败: " << ret << std::endl;
    delete cb_;
    cb_ = nullptr;
    lb_api::api_interface::release_instance(api_);
    api_ = nullptr;
    return false;
  }
  std::cout << "[PerfClient] API 启动成功" << std::endl;
  return true;
}

bool PerfClient::login(int timeout_ms) {
  if (!api_ || !cb_) {
    std::cerr << "[PerfClient] 未初始化" << std::endl;
    return false;
  }

  lb_api::LoginReq req;
  std::memset(&req, 0, sizeof(req));
  std::memcpy(req.fund_account_id.data(), cfg_.fund_account_id.c_str(),
              std::min(cfg_.fund_account_id.size(), req.fund_account_id.size()));
  std::memcpy(req.branch_id.data(), cfg_.branch_id.c_str(),
              std::min(cfg_.branch_id.size(), req.branch_id.size()));
  std::memcpy(req.account_id.data(), cfg_.fund_account_id.c_str(),
              std::min(cfg_.fund_account_id.size(), req.account_id.size()));
  std::memcpy(req.cust_id.data(), cfg_.fund_account_id.c_str(),
              std::min(cfg_.fund_account_id.size(), req.cust_id.size()));
  std::memcpy(req.password.data(), cfg_.password.c_str(),
              std::min(cfg_.password.size(), req.password.size()));
  std::memcpy(req.order_way_ext.data(), "01", 2);   // order_way_ext 为 2 字节，避免溢出
  std::memcpy(req.user_info.data(), "perf_user", 9);

  int32_t ret = api_->login(req);
  std::cout << "[PerfClient] login() 返回: " << ret << std::endl;
  if (ret != 0) {
    return false;
  }

  // 等待登录回调
  auto start = std::chrono::steady_clock::now();
  while (!cb_->login_done) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() > timeout_ms) {
      std::cerr << "[PerfClient] 登录回调超时" << std::endl;
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return cb_->login_ok;
}

int32_t PerfClient::send_order(lb_api::OrderReq& req) {
  return api_->order_insert(req);
}

void PerfClient::shutdown() {
  if (api_) {
    api_->stop();
    std::cout << "[PerfClient] API 已停止" << std::endl;
    lb_api::api_interface::release_instance(api_);
    api_ = nullptr;
  }
  delete cb_;
  cb_ = nullptr;
}

// ==================== 匀速发单 ====================

// 构造一笔委托请求（使用配置模板 + 递增 client_seq_id）
static void build_order(const PerfConfig& cfg, int64_t seq, lb_api::OrderReq& req) {
  std::memset(&req, 0, sizeof(req));
  std::memcpy(req.fund_account_id.data(), cfg.fund_account_id.c_str(),
              std::min(cfg.fund_account_id.size(), req.fund_account_id.size()));
  std::memcpy(req.branch_id.data(), cfg.branch_id.c_str(),
              std::min(cfg.branch_id.size(), req.branch_id.size()));
  req.side = cfg.side;
  req.order_type = cfg.order_type;
  std::memcpy(req.security_id.data(), cfg.security_id.c_str(),
              std::min(cfg.security_id.size(), req.security_id.size()));
  req.order_price = cfg.order_price;
  req.order_qty = cfg.order_qty;
  req.stop_price = cfg.stop_price;
  req.client_seq_id = seq;
  req.market_type = static_cast<uint16_t>(cfg.order_market_type);
}

void run_benchmark(PerfClient& client, const PerfConfig& cfg, std::vector<uint64_t>& latencies) {
  if (cfg.tps <= 0) {
    std::cerr << "[PerfClient] TPS 必须 > 0" << std::endl;
    return;
  }

  // 匀速发单间隔（微秒）
  const double interval_us = 1000000.0 / static_cast<double>(cfg.tps);
  const auto interval = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::duration<double, std::micro>(interval_us));

  const auto end_time = std::chrono::steady_clock::now() +
                        std::chrono::seconds(cfg.test_duration_sec);

  int64_t seq = 1;
  size_t sent = 0;
  size_t ok = 0;

  auto next_send = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() < end_time) {
    lb_api::OrderReq req;
    build_order(cfg, seq++, req);

    int32_t ret = client.send_order(req);
    ++sent;
    if (ret == 0) {
      ++ok;
    }

    // 计算 api 内耗时（纳秒）
    uint64_t lat = req.api_leave_time_ns - req.api_arrive_time_ns;
    latencies.push_back(lat);

    // 等待到下一笔的发送时刻（匀速）
    next_send += interval;
    auto now = std::chrono::steady_clock::now();
    if (now < next_send) {
      std::this_thread::sleep_for(next_send - now);
    } else {
      // 落后于节奏（吞吐不足），重新对齐
      next_send = now + interval;
    }
  }

  std::cout << "[PerfClient] 发单完成: 发送=" << sent << " 成功=" << ok
            << " 失败=" << (sent - ok) << std::endl;
}

} // namespace perf
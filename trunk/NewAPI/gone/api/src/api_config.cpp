#include "api_config.h"
#include "api_config_impl.h"
#include "api_errno.h"
#include <cstdio>
#include <cstring>

namespace lb_api {

/// 已知属性信息表(用于类型检查)
struct attr_info_entry {
  const char *name;
  attr_type type;
};

static const attr_info_entry known_attrs[] = {
    {config_name::fast_counter_type, attr_type::int32_val},
    {config_name::market_type, attr_type::int8_val},
    {config_name::speed_link_type_name, attr_type::int32_val},
    {config_name::solarflare_iface, attr_type::string_val},
    {config_name::speed_counter_addr, attr_type::addr_val},
    {config_name::speed_counter_addr_bak, attr_type::addr_val},
    {config_name::counter98_addr, attr_type::addr_val},
    {config_name::counter98_addr_bak, attr_type::addr_val},
    {config_name::callback_mode_name, attr_type::int32_val},
    {config_name::send_poll_num, attr_type::int32_val},
    {config_name::recv_poll_num, attr_type::int32_val},
    {config_name::heartbeat_interval, attr_type::int32_val},
    {config_name::max_reconnect_count, attr_type::int32_val},
    {config_name::speed_engine_cpu, attr_type::int32_val},
    {config_name::mgmt_engine_cpu, attr_type::int32_val},
    {config_name::callback_thread_cpu, attr_type::int32_val},
    {config_name::send_queue_size_mb, attr_type::int32_val},
    {config_name::callback_queue_size_mb, attr_type::int32_val},
    {config_name::callback_wait_ms, attr_type::int32_val},
    {config_name::multi_io_wait_ms, attr_type::int32_val},
    {config_name::agw98_user, attr_type::string_val},
    {config_name::agw98_user_password, attr_type::string_val},
    {config_name::agw_user_login_timeout, attr_type::int32_val},
    {config_name::log_queue_size_mb, attr_type::int32_val},
    {config_name::log_level, attr_type::int32_val},
    {config_name::api_instance_name, attr_type::string_val},
    {config_name::log_output_dir, attr_type::string_val},
};

static constexpr int32_t known_attrs_count = sizeof(known_attrs) / sizeof(known_attrs[0]);

// ---- 构造/析构 ----

api_config_impl::api_config_impl()
    : fast_counter_type_(counter_type::fixed_98), market_type_(static_cast<market_type_t>(0)),
      speed_link_type_(speed_link_type::socket_single), callback_mode_(callback_mode::direct), send_poll_num_(6),
      recv_poll_num_(2), heartbeat_interval_(5), max_reconnect_count_(0), speed_engine_cpu_(-1), mgmt_engine_cpu_(-1),
      callback_thread_cpu_(-1), send_queue_size_mb_(2), callback_queue_size_mb_(8), callback_wait_ms_(100),
      multi_io_wait_ms_(1), agw_user_login_timeout_(10), log_queue_size_mb_(8), log_level_(1) {
  std::memset(solarflare_iface_, 0, sizeof(solarflare_iface_));
  std::memset(agw98_user_, 0, sizeof(agw98_user_));
  std::memset(agw98_user_password_, 0, sizeof(agw98_user_password_));
  std::memset(api_instance_name_, 0, sizeof(api_instance_name_));
  std::memset(log_output_dir_, 0, sizeof(log_output_dir_));
  std::memcpy(log_output_dir_, "./api_log", 9);
  std::memset(&speed_counter_addr_, 0, sizeof(speed_counter_addr_));
  std::memset(&speed_counter_addr_bak_, 0, sizeof(speed_counter_addr_bak_));
  std::memset(&counter98_addr_, 0, sizeof(counter98_addr_));
  std::memset(&counter98_addr_bak_, 0, sizeof(counter98_addr_bak_));
}

api_config_impl::~api_config_impl() {}

// ---- 类型检查 ----

int32_t api_config_impl::check_attr_type(const char *attr_name, attr_type expected) const {
  if (!attr_name)
    return LBAPI_ERR_INVALID_PARAM;

  for (int32_t i = 0; i < known_attrs_count; ++i) {
    if (std::strcmp(attr_name, known_attrs[i].name) == 0) {
      return (known_attrs[i].type == expected) ? LBAPI_OK : LBAPI_ERR_TRANS_TYPE;
    }
  }
  return LBAPI_ERR_CFG_INVALID;
}

// ---- set_attr 实现 ----

int32_t api_config_impl::set_attr(const char *attr_name, int8_t attr_val) {
  int32_t ret = check_attr_type(attr_name, attr_type::int8_val);
  if (ret != LBAPI_OK)
    return ret;

  if (std::strcmp(attr_name, config_name::market_type) == 0) {
    market_type_ = static_cast<market_type_t>(attr_val);
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

int32_t api_config_impl::set_attr(const char *attr_name, int16_t attr_val) {
  (void)attr_val;
  if (!attr_name)
    return LBAPI_ERR_INVALID_PARAM;
  return LBAPI_ERR_UNSUPPORTED_OP;
}

int32_t api_config_impl::set_attr(const char *attr_name, int32_t attr_val) {
  int32_t ret = check_attr_type(attr_name, attr_type::int32_val);
  if (ret != LBAPI_OK)
    return ret;

  if (std::strcmp(attr_name, config_name::fast_counter_type) == 0) {
    fast_counter_type_ = static_cast<counter_type>(attr_val);
  } else if (std::strcmp(attr_name, config_name::speed_link_type_name) == 0) {
    speed_link_type_ = static_cast<speed_link_type>(attr_val);
  } else if (std::strcmp(attr_name, config_name::callback_mode_name) == 0) {
    callback_mode_ = static_cast<callback_mode>(attr_val);
  } else if (std::strcmp(attr_name, config_name::agw_user_login_timeout) == 0) {
    if (attr_val <= 0)
      attr_val = 15;
    agw_user_login_timeout_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::send_poll_num) == 0) {
    send_poll_num_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::recv_poll_num) == 0) {
    recv_poll_num_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::heartbeat_interval) == 0) {
    heartbeat_interval_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::max_reconnect_count) == 0) {
    max_reconnect_count_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::speed_engine_cpu) == 0) {
    speed_engine_cpu_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::mgmt_engine_cpu) == 0) {
    mgmt_engine_cpu_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::callback_thread_cpu) == 0) {
    callback_thread_cpu_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::send_queue_size_mb) == 0) {
    send_queue_size_mb_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::callback_queue_size_mb) == 0) {
    callback_queue_size_mb_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::callback_wait_ms) == 0) {
    callback_wait_ms_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::multi_io_wait_ms) == 0) {
    multi_io_wait_ms_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::log_queue_size_mb) == 0) {
    log_queue_size_mb_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::log_level) == 0) {
    log_level_ = attr_val;
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

int32_t api_config_impl::set_attr(const char *attr_name, int64_t attr_val) {
  (void)attr_val;
  if (!attr_name)
    return LBAPI_ERR_INVALID_PARAM;
  return LBAPI_ERR_UNSUPPORTED_OP;
}

int32_t api_config_impl::set_attr(const char *attr_name, const char *attr_val) {
  int32_t ret = check_attr_type(attr_name, attr_type::string_val);
  if (ret != LBAPI_OK)
    return ret;
  if (!attr_val)
    return LBAPI_ERR_INVALID_PARAM;

  if (std::strcmp(attr_name, config_name::solarflare_iface) == 0) {
    std::memset(solarflare_iface_, 0, sizeof(solarflare_iface_));
    std::strncpy(solarflare_iface_, attr_val, sizeof(solarflare_iface_) - 1);
  } else if (std::strcmp(attr_name, config_name::agw98_user) == 0) {
    std::memset(agw98_user_, 0, sizeof(agw98_user_));
    std::strncpy(agw98_user_, attr_val, sizeof(agw98_user_) - 1);
  } else if (std::strcmp(attr_name, config_name::agw98_user_password) == 0) {
    std::memset(agw98_user_password_, 0, sizeof(agw98_user_password_));
    std::strncpy(agw98_user_password_, attr_val, sizeof(agw98_user_password_) - 1);
  } else if (std::strcmp(attr_name, config_name::api_instance_name) == 0) {
    std::memset(api_instance_name_, 0, sizeof(api_instance_name_));
    std::strncpy(api_instance_name_, attr_val, sizeof(api_instance_name_) - 1);
  } else if (std::strcmp(attr_name, config_name::log_output_dir) == 0) {
    std::memset(log_output_dir_, 0, sizeof(log_output_dir_));
    std::strncpy(log_output_dir_, attr_val, sizeof(log_output_dir_) - 1);
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

int32_t api_config_impl::set_attr(const char *attr_name, const std::string &attr_val) {
  return set_attr(attr_name, attr_val.c_str());
}

int32_t api_config_impl::set_attr(const char *attr_name, bool attr_val) {
  (void)attr_val;
  if (!attr_name)
    return LBAPI_ERR_INVALID_PARAM;
  return LBAPI_ERR_UNSUPPORTED_OP;
}

int32_t api_config_impl::set_attr(const char *attr_name, const net_addr &attr_val) {
  int32_t ret = check_attr_type(attr_name, attr_type::addr_val);
  if (ret != LBAPI_OK)
    return ret;

  if (std::strcmp(attr_name, config_name::speed_counter_addr) == 0) {
    speed_counter_addr_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::speed_counter_addr_bak) == 0) {
    speed_counter_addr_bak_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::counter98_addr) == 0) {
    counter98_addr_ = attr_val;
  } else if (std::strcmp(attr_name, config_name::counter98_addr_bak) == 0) {
    counter98_addr_bak_ = attr_val;
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

// ---- get_attr 实现 ----
int32_t api_config_impl::get_attr(const char *attr_name, int8_t &o_val) const {
  int32_t ret = check_attr_type(attr_name, attr_type::int8_val);
  if (ret != LBAPI_OK)
    return ret;

  if (std::strcmp(attr_name, config_name::market_type) == 0) {
    o_val = static_cast<int8_t>(market_type_);
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

int32_t api_config_impl::get_attr(const char *attr_name, int16_t &o_val) const {
  (void)o_val;
  if (!attr_name)
    return LBAPI_ERR_INVALID_PARAM;
  return LBAPI_ERR_UNSUPPORTED_OP;
}

int32_t api_config_impl::get_attr(const char *attr_name, int32_t &o_val) const {
  int32_t ret = check_attr_type(attr_name, attr_type::int32_val);
  if (ret != LBAPI_OK)
    return ret;

  if (std::strcmp(attr_name, config_name::fast_counter_type) == 0) {
    o_val = static_cast<int32_t>(fast_counter_type_);
  } else if (std::strcmp(attr_name, config_name::speed_link_type_name) == 0) {
    o_val = static_cast<int32_t>(speed_link_type_);
  } else if (std::strcmp(attr_name, config_name::callback_mode_name) == 0) {
    o_val = static_cast<int32_t>(callback_mode_);
  } else if (std::strcmp(attr_name, config_name::agw_user_login_timeout) == 0) {
    o_val = agw_user_login_timeout_;
  } else if (std::strcmp(attr_name, config_name::send_poll_num) == 0) {
    o_val = send_poll_num_;
  } else if (std::strcmp(attr_name, config_name::recv_poll_num) == 0) {
    o_val = recv_poll_num_;
  } else if (std::strcmp(attr_name, config_name::heartbeat_interval) == 0) {
    o_val = heartbeat_interval_;
  } else if (std::strcmp(attr_name, config_name::max_reconnect_count) == 0) {
    o_val = max_reconnect_count_;
  } else if (std::strcmp(attr_name, config_name::speed_engine_cpu) == 0) {
    o_val = speed_engine_cpu_;
  } else if (std::strcmp(attr_name, config_name::mgmt_engine_cpu) == 0) {
    o_val = mgmt_engine_cpu_;
  } else if (std::strcmp(attr_name, config_name::callback_thread_cpu) == 0) {
    o_val = callback_thread_cpu_;
  } else if (std::strcmp(attr_name, config_name::send_queue_size_mb) == 0) {
    o_val = send_queue_size_mb_;
  } else if (std::strcmp(attr_name, config_name::callback_queue_size_mb) == 0) {
    o_val = callback_queue_size_mb_;
  } else if (std::strcmp(attr_name, config_name::callback_wait_ms) == 0) {
    o_val = callback_wait_ms_;
  } else if (std::strcmp(attr_name, config_name::multi_io_wait_ms) == 0) {
    o_val = multi_io_wait_ms_;
  } else if (std::strcmp(attr_name, config_name::log_queue_size_mb) == 0) {
    o_val = log_queue_size_mb_;
  } else if (std::strcmp(attr_name, config_name::log_level) == 0) {
    o_val = log_level_;
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

int32_t api_config_impl::get_attr(const char *attr_name, int64_t &o_val) const {
  (void)o_val;
  if (!attr_name)
    return LBAPI_ERR_INVALID_PARAM;
  return LBAPI_ERR_UNSUPPORTED_OP;
}

int32_t api_config_impl::get_attr(const char *attr_name, char *o_val, int32_t buf_len) const {
  int32_t ret = check_attr_type(attr_name, attr_type::string_val);
  if (ret != LBAPI_OK)
    return ret;
  if (!o_val || buf_len <= 0)
    return LBAPI_ERR_INVALID_PARAM;

  if (std::strcmp(attr_name, config_name::solarflare_iface) == 0) {
    std::memset(o_val, 0, buf_len);
    std::strncpy(o_val, solarflare_iface_, buf_len - 1);
  } else if (std::strcmp(attr_name, config_name::agw98_user) == 0) {
    std::memset(o_val, 0, buf_len);
    std::strncpy(o_val, agw98_user_, buf_len - 1);
  } else if (std::strcmp(attr_name, config_name::agw98_user_password) == 0) {
    std::memset(o_val, 0, buf_len);
    std::strncpy(o_val, agw98_user_password_, buf_len - 1);
  } else if (std::strcmp(attr_name, config_name::api_instance_name) == 0) {
    std::memset(o_val, 0, buf_len);
    std::strncpy(o_val, api_instance_name_, buf_len - 1);
  } else if (std::strcmp(attr_name, config_name::log_output_dir) == 0) {
    std::memset(o_val, 0, buf_len);
    std::strncpy(o_val, log_output_dir_, buf_len - 1);
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

int32_t api_config_impl::get_attr(const char *attr_name, std::string &o_val) const {
  int32_t ret = check_attr_type(attr_name, attr_type::string_val);
  if (ret != LBAPI_OK)
    return ret;

  if (std::strcmp(attr_name, config_name::solarflare_iface) == 0) {
    o_val = solarflare_iface_;
  } else if (std::strcmp(attr_name, config_name::agw98_user) == 0) {
    o_val = agw98_user_;
  } else if (std::strcmp(attr_name, config_name::agw98_user_password) == 0) {
    o_val = agw98_user_password_;
  } else if (std::strcmp(attr_name, config_name::api_instance_name) == 0) {
    o_val = api_instance_name_;
  } else if (std::strcmp(attr_name, config_name::log_output_dir) == 0) {
    o_val = log_output_dir_;
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

int32_t api_config_impl::get_attr(const char *attr_name, bool &o_val) const {
  (void)o_val;
  if (!attr_name)
    return LBAPI_ERR_INVALID_PARAM;
  return LBAPI_ERR_UNSUPPORTED_OP;
}

int32_t api_config_impl::get_attr(const char *attr_name, net_addr &o_val) const {
  int32_t ret = check_attr_type(attr_name, attr_type::addr_val);
  if (ret != LBAPI_OK)
    return ret;

  if (std::strcmp(attr_name, config_name::speed_counter_addr) == 0) {
    o_val = speed_counter_addr_;
  } else if (std::strcmp(attr_name, config_name::speed_counter_addr_bak) == 0) {
    o_val = speed_counter_addr_bak_;
  } else if (std::strcmp(attr_name, config_name::counter98_addr) == 0) {
    o_val = counter98_addr_;
  } else if (std::strcmp(attr_name, config_name::counter98_addr_bak) == 0) {
    o_val = counter98_addr_bak_;
  } else {
    return LBAPI_ERR_CFG_INVALID;
  }
  return LBAPI_OK;
}

// ---- 配置校验 ----

int32_t api_config_impl::validate() const {
  // 必须有极速柜台(fast_counter_type不可为fixed_98)
  if (fast_counter_type_ == counter_type::fixed_98) {
    return LBAPI_ERR_UNSUPPORT_COUNTER;
  }

  // ---- 配置组合不兼容校验 ----
  // 个微（gw_direct）禁 socket_shared
  if (fast_counter_type_ == counter_type::gw_direct && speed_link_type_ == speed_link_type::socket_shared) {
    return LBAPI_ERR_CFG_INVALID;
  }
  // fpga_gateway 必 socket_shared
  if (fast_counter_type_ == counter_type::fpga_gateway && speed_link_type_ != speed_link_type::socket_shared) {
    return LBAPI_ERR_CFG_INVALID;
  }
  // 新需求：fpga_direct 禁 socket_shared
  if (fast_counter_type_ == counter_type::fpga_direct && speed_link_type_ == speed_link_type::socket_shared) {
    return LBAPI_ERR_CFG_INVALID;
  }
  // ---- 配置组合校验结束 ----

  // agw_user_login_timeout 必须 > 0
  if (agw_user_login_timeout_ <= 0) {
    return LBAPI_ERR_LOGIN_TIMEOUT;
  }

  // 市场类型必须设置(1=上海, 2=深交所/北交所)
  if (market_type_ != market_type_t::SH_MARKET && market_type_ != market_type_t::SZ_MARKET) {
    return LBAPI_ERR_CFG_INVALID;
  }

  // 98柜台地址必须配置
  if (counter98_addr_.ip[0] == '\0' || counter98_addr_.port <= 0) {
    return LBAPI_ERR_LINK_ADDR;
  }

  // 极速柜台地址(主)必须配置
  if (speed_counter_addr_.ip[0] == '\0' || speed_counter_addr_.port <= 0) {
    return LBAPI_ERR_LINK_ADDR;
  }

  // tcpdirect模式需要solarflare网卡接口名
  if (speed_link_type_ == speed_link_type::tcpdirect) {
    if (solarflare_iface_[0] == '\0') {
      return LBAPI_ERR_NO_SOLARFLARE;
    }
  }

  // 队列大小必须>0
  if (send_queue_size_mb_ <= 0 || callback_queue_size_mb_ <= 0) {
    return LBAPI_ERR_INVALID_PARAM;
  }

  // 98网关用户名必须配置
  if (agw98_user_[0] == '\0') {
    return LBAPI_ERR_INVALID_PARAM;
  }

  // 98网关用户密码必须配置
  if (agw98_user_password_[0] == '\0') {
    return LBAPI_ERR_INVALID_PARAM;
  }

  // API实例名称必须配置
  if (api_instance_name_[0] == '\0') {
    return LBAPI_ERR_INVALID_PARAM;
  }

  return LBAPI_OK;
}

// ---- 属性复制 ----

int32_t api_config_impl::copy_from(const api_config &src) {
  // int32_t属性
  int32_t int_val = 0;
  const char *int32_attrs[] = {
      config_name::fast_counter_type,
      config_name::speed_link_type_name,
      config_name::callback_mode_name,
      // run_mode 概念已合并到 fast_counter_type
      config_name::agw_user_login_timeout,
      config_name::send_poll_num,
      config_name::recv_poll_num,
      config_name::heartbeat_interval,
      config_name::max_reconnect_count,
      config_name::speed_engine_cpu,
      config_name::mgmt_engine_cpu,
      config_name::callback_thread_cpu,
      config_name::send_queue_size_mb,
      config_name::callback_queue_size_mb,
      config_name::callback_wait_ms,
      config_name::multi_io_wait_ms,
      config_name::log_queue_size_mb,
      config_name::log_level,
  };
  for (const char *name : int32_attrs) {
    if (src.get_attr(name, int_val) == LBAPI_OK) {
      set_attr(name, int_val);
    }
  }

  // int8_t属性
  int8_t int8_val = 0;
  const char *int8_attrs[] = {
      config_name::market_type,
  };
  for (const char *name : int8_attrs) {
    if (src.get_attr(name, int8_val) == LBAPI_OK) {
      set_attr(name, int8_val);
    }
  }

  // string属性
  char str_val[4096];
  const char *str_attrs[] = {
      config_name::solarflare_iface,  config_name::agw98_user,     config_name::agw98_user_password,
      config_name::api_instance_name, config_name::log_output_dir,
  };
  for (const char *name : str_attrs) {
    if (src.get_attr(name, str_val, sizeof(str_val)) == LBAPI_OK) {
      set_attr(name, str_val);
    }
  }

  // addr属性
  net_addr addr_val;
  const char *addr_attrs[] = {
      config_name::speed_counter_addr,
      config_name::speed_counter_addr_bak,
      config_name::counter98_addr,
      config_name::counter98_addr_bak,
  };
  for (const char *name : addr_attrs) {
    if (src.get_attr(name, addr_val) == LBAPI_OK) {
      set_attr(name, addr_val);
    }
  }

  return LBAPI_OK;
}

// ---- 工厂函数 ----

api_config *api_config::create_config() { return new api_config_impl(); }

void api_config::destroy_config(api_config *cfg) { delete cfg; }

} // namespace lb_api

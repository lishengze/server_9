#pragma once

#include <cstdint>
#include <string>

namespace lb_api {

/// 市场类型标识
enum class market_type_t : int8_t {
  SH_MARKET = 1, ///< 上交所
  SZ_MARKET = 2, ///< 深交所（支持北交所）
  BSE_MARKET = 3 ///< 北交所（仅用于 bse_order_insert 中的市场标识，api 实例不直接配置此值）
};

/// 柜台类型标识
/// fast_counter_type默认为fixed_98(占位)，必须设置为gw_direct / fpga_direct / fpga_gateway 之一
/// 柜台模式信息（直连/网关）已合并到本枚举，无需单独的 run_mode 配置
enum class counter_type : int32_t {
  fixed_98 = 0,    ///< 98柜台标识(默认占位值，不可作为fast_counter_type最终配置值)
  gw_direct = 1,   ///< 个微软件极速柜台（直连模式，兼容未来 gw_gateway）
  fpga_direct = 2, ///< FPGA 极速柜台（直连模式，单客户）
  fpga_gateway = 3 ///< FPGA 极速柜台（网关模式，多客户）
};

/// 极速链接类型
/// 决定极速交易链接的IO模型和引擎组合
///   socket_shared  → 仅 fpga_gateway 适用
///   socket_single  → gw_direct / fpga_direct 适用
///   tcpdirect      → gw_direct / fpga_direct 适用
enum class speed_link_type : int32_t {
  socket_shared = 0, ///< 共享socket（仅 fpga_gateway）
  socket_single = 1, ///< 独占socket
  tcpdirect = 2      ///< Solarflare TCPDirect加速
};

/// 回调模式
enum class callback_mode : int32_t {
  direct = 0, ///< 多线程直接回调
  queued = 1  ///< 回调队列+回调线程
};

/// 网络地址结构
struct net_addr {
  char ip[56];
  int32_t port;
};

/// 配置属性名常量
/// 新增配置项只需在此增加常量，api_config接口不变
/// 格式: 属性名 = 常量值 // 类型  默认值  是否必须设置
namespace config_name {
constexpr const char *fast_counter_type =
    "fast_counter_type"; // int32   fixed_98   必须设置(须设为gw_direct/fpga_direct/fpga_gateway)
constexpr const char *market_type = "market_type"; // int8   0          必须设置(1=上海, 2=深交所/北交所)
constexpr const char *speed_link_type_name = "speed_link_type"; // int32   socket_single  非必须
constexpr const char *solarflare_iface = "solarflare_iface"; // string  空            非必须(tcpdirect模式时必须设置)
constexpr const char *speed_counter_addr = "speed_counter_addr";         // net_addr  空          必须设置
constexpr const char *speed_counter_addr_bak = "speed_counter_addr_bak"; // net_addr  空          非必须
constexpr const char *counter98_addr = "counter98_addr";                 // net_addr  空          必须设置
constexpr const char *counter98_addr_bak = "counter98_addr_bak";         // net_addr  空          非必须
constexpr const char *callback_mode_name = "callback_mode";              // int32   direct        非必须
constexpr const char *send_poll_num = "send_poll_num";                   // int32   6             非必须
constexpr const char *recv_poll_num = "recv_poll_num";                   // int32   2             非必须
constexpr const char *heartbeat_interval = "heartbeat_interval";         // int32   5             非必须
constexpr const char *max_reconnect_count = "max_reconnect_count";       // int32   0(无限)       非必须
constexpr const char *speed_engine_cpu = "speed_engine_cpu";             // int32   -1(不绑定)    非必须
constexpr const char *mgmt_engine_cpu = "mgmt_engine_cpu";               // int32   -1(不绑定)    非必须
constexpr const char *callback_thread_cpu = "callback_thread_cpu";       // int32   -1(不绑定)    非必须
constexpr const char *send_queue_size_mb = "send_queue_size_mb";         // int32   2             非必须
constexpr const char *callback_queue_size_mb = "callback_queue_size_mb"; // int32   8           非必须
constexpr const char *callback_wait_ms = "callback_wait_ms";             // int32   10         非必须(0=死轮询)
constexpr const char *multi_io_wait_ms = "multi_io_wait_ms"; // int32   100           非必须(0=epoll_wait不等待)
constexpr const char *agw98_user = "98agw_user";             // string  空            必须设置
constexpr const char *agw98_user_password = "98agw_user_password"; // string  空            必须设置
constexpr const char *agw_user_login_timeout =
    "agw_user_login_timeout"; // int32  10(秒)      非必须(AGW 登录超时，建链超时由基础库提供)
constexpr const char *log_queue_size_mb = "log_queue_size_mb"; // int32   8(0=同步写文件)  非必须
constexpr const char *log_level = "log_level";                 // int32   1(通知)          非必须
constexpr const char *api_instance_name = "api_instance_name"; // string  空              必须设置
constexpr const char *log_output_dir = "log_output_dir";       // string  ./api_log       非必须
} // namespace config_name

// 注：run_mode 概念已合并到 counter_type，无独立的 run_mode_name 配置项

/// API配置属性虚接口类
///
/// 通过 set_attr/get_attr 系列重载函数访问配置属性
/// 新增配置项只需增加属性名常量和实现类中的成员，接口不变
///
/// 使用示例:
///   api_config* cfg = api_config::create_config();
///   cfg->set_attr(config_name::fast_counter_type, static_cast<int32_t>(counter_type::fpga_direct));
///   cfg->set_attr(config_name::counter98_addr, net_addr{"192.168.1.1", 9000});
///   cfg->set_attr(config_name::solarflare_iface, "eth0");
///
///   int32_t ct_val;
///   cfg->get_attr(config_name::fast_counter_type, ct_val);
///   net_addr addr;
///   cfg->get_attr(config_name::counter98_addr, addr);
///
/// 注意:
///   create_instance成功后，各配置参数已被分发到内部模块，不可再修改;
///   config对象不再使用，用户应调用destroy_config()释放该对象。
class api_config {
public:
  virtual ~api_config() {}

  /// ---- 设置属性值(按类型重载) ----
  /// @param attr_name  属性名(使用config_name命名空间中的常量)
  /// @param attr_val   属性值
  /// @return api_errno, LBAPI_OK=成功
  /// 注意: int16/int64/bool 类型为预留接口, 当前调用返回 LBAPI_ERR_UNSUPPORTED

  virtual int32_t set_attr(const char *attr_name, int8_t attr_val) = 0;
  virtual int32_t set_attr(const char *attr_name, int16_t attr_val) = 0;
  virtual int32_t set_attr(const char *attr_name, int32_t attr_val) = 0;
  virtual int32_t set_attr(const char *attr_name, int64_t attr_val) = 0;
  virtual int32_t set_attr(const char *attr_name, const char *attr_val) = 0;
  virtual int32_t set_attr(const char *attr_name, const std::string &attr_val) = 0;
  virtual int32_t set_attr(const char *attr_name, bool attr_val) = 0;
  virtual int32_t set_attr(const char *attr_name, const net_addr &attr_val) = 0;

  /// ---- 获取属性值(按类型重载) ----
  /// @param attr_name  属性名
  /// @param o_val      输出参数，接收属性值
  /// @return api_errno, LBAPI_OK=成功
  /// 对于string类型(char*版本): o_val为输出缓冲区, buf_len为缓冲区长度
  /// 注意: int16/int64/bool 类型为预留接口, 当前调用返回 LBAPI_ERR_UNSUPPORTED

  virtual int32_t get_attr(const char *attr_name, int8_t &attr_val) const = 0;
  virtual int32_t get_attr(const char *attr_name, int16_t &o_val) const = 0;
  virtual int32_t get_attr(const char *attr_name, int32_t &o_val) const = 0;
  virtual int32_t get_attr(const char *attr_name, int64_t &o_val) const = 0;
  virtual int32_t get_attr(const char *attr_name, char *o_val, int32_t buf_len) const = 0;
  virtual int32_t get_attr(const char *attr_name, std::string &o_val) const = 0;
  virtual int32_t get_attr(const char *attr_name, bool &o_val) const = 0;
  virtual int32_t get_attr(const char *attr_name, net_addr &o_val) const = 0;

  /// 配置校验
  /// @return api_errno, LBAPI_OK=校验通过
  virtual int32_t validate() const = 0;

  /// 从源配置复制所有属性
  /// @param src  源配置
  /// @return api_errno, LBAPI_OK=成功
  virtual int32_t copy_from(const api_config &src) = 0;

  /// 创建默认配置实例(工厂函数)
  /// @return api_config指针
  static api_config *create_config();

  /// 释放配置实例
  /// create_instance成功后，配置参数已分发到各内部模块，config对象不再使用，应调用此函数释放
  /// @param cfg  由create_config创建的配置指针
  static void destroy_config(api_config *cfg);
};

} // namespace lb_api

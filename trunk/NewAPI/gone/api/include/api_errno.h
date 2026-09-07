#pragma once

#include <cstdint>

namespace lb_api {

/// API错误码定义
/// 0=成功, 负值=错误

enum api_errno : int32_t {
  // ---- 成功 ----
  LBAPI_OK = 0,
  LBAPI_ERR_INVALID_PARAM = -1,        ///< 无效参数
  LBAPI_ERR_UNSUPPORTED_TYPE = -2,     ///< 不支持的类型
  LBAPI_ERR_UNSUPPORTED_OP = -3,       ///< 不支持的操作
  LBAPI_ERR_STATE_LIMITED = -4,        ///< 状态错误(init/start调用时序不对)
  LBAPI_ERR_CFG_INVALID = -5,          ///< 配置错误
  LBAPI_ERR_TRANS_TYPE = -6,           ///< 类型转换错误
  LBAPI_ERR_UNSUPPORT_LINK = -7,       ///< 不支持的链接
  LBAPI_ERR_UNSUPPORT_COUNTER = -8,    ///< 不支持的柜台
  LBAPI_ERR_LOG_INIT = -9,             ///< 日志初始化失败
  LBAPI_ERR_QUEUE_INIT = -10,          ///< 队列初始化失败
  LBAPI_ERR_THREAD_INIT = -11,         ///< 线程初始化失败
  LBAPI_ERR_TIMER_INIT = -12,          ///< 定时器初始化失败
  LBAPI_ERR_ATTR_INIT = -13,           ///< 资源属性初始化失败
  LBAPI_ERR_LINK_INIT = -14,           ///< 链接对象初始化失败
  LBAPI_ERR_COUNTER_INIT = -15,        ///< 柜台对象初始化失败
  LBAPI_ERR_THRAD_START = -16,         ///< 启动线程失败
  LBAPI_ERR_LINK_ADDR = -17,           ///< 链接地址错误
  LBAPI_ERR_LINK_CONNECT = -18,        ///< 建立连接错误
  LBAPI_ERR_NO_SOLARFLARE = -19,       ///< 无solarflare网卡
  LBAPI_ERR_LINK_CONNECT_REPEAT = -20, ///< 重复链接
  LBAPI_ERR_LINK_START_RECV = -21,     ///< 启动链接接收失败
  LBAPI_ERR_LINK_DISCONNECTED = -22,   ///< 物理断线
  LBAPI_ERR_LINK_HEART_TIMEOUT = -23,  ///< 心跳超时
  LBAPI_ERR_COUNTER_OFFLINE = -24,     ///< 柜台客户状态不正常
  LBAPI_ERR_SEND_QUEUE_FULL = -25,     ///< 发送队列已满
  LBAPI_ERR_MSG_LEN = -26,             ///< 消息长度错误
  LBAPI_ERR_MSG_TYPE = -27,            ///< 消息类型错误
  LBAPI_ERR_BUILD_MSG = -28,           ///< 构建消息失败
  LBAPI_ERR_SEND_MSG = -29,            ///< 发送消息失败
  LBAPI_ERR_LOGIN_FAIL = -30,          ///< 登陆失败
  LBAPI_ERR_LOGIN_TIMEOUT = -31,       ///< 登陆超时
  LBAPI_ERR_ADD_TIMER = -32,           ///< 添加定时事件失败
  LBAPI_ERR_ADD_WAKEFD = -33,          ///< 添加触发事件失败
  LBAPI_ERR_NOT_LOG_AGW = -34,         ///< 未登陆agw用户
  LBAPI_ERR_NOT_LOG_CUST = -35,        ///< 柜台投资者未登陆
  LBAPI_ERR_NO_SEC = -36,              ///< 证券信息不存在
  LBAPI_ERR_ALLOC_MEM = -37,           ///< 获取内存失败
  LBAPI_ERR_NO_CUST = -38,             ///< 客户信息不存在
  LBAPI_ERR_INSERT_MAP = -39           ///< 插入map 失败
};

/// 获取错误码对应的错误信息
/// @param err  错误码
/// @return 错误描述字符串(静态存储, 无需释放)
const char *api_strerror(int32_t err);

} // namespace lb_api

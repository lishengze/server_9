#include "api_errno.h"

namespace lb_api {

const char *api_strerror(int32_t err) {
  switch (err) {
  case LBAPI_OK:
    return "success";
  // 通用错误
  case LBAPI_ERR_INVALID_PARAM:
    return "invalid parameter";
  case LBAPI_ERR_UNSUPPORTED_TYPE:
    return "unsupported type";
  case LBAPI_ERR_UNSUPPORTED_OP:
    return "unsupported operation";
  case LBAPI_ERR_STATE_LIMITED:
    return "invalid state (init/start call order wrong)";
  case LBAPI_ERR_CFG_INVALID:
    return "config invalid";
  case LBAPI_ERR_TRANS_TYPE:
    return "type conversion error";
  case LBAPI_ERR_UNSUPPORT_LINK:
    return "unsupported link";
  case LBAPI_ERR_UNSUPPORT_COUNTER:
    return "unsupported counter";
  // 资源初始化
  case LBAPI_ERR_LOG_INIT:
    return "log module init failed";
  case LBAPI_ERR_QUEUE_INIT:
    return "queue init failed";
  case LBAPI_ERR_THREAD_INIT:
    return "thread init failed";
  case LBAPI_ERR_TIMER_INIT:
    return "timer init failed";
  case LBAPI_ERR_ATTR_INIT:
    return "resource attribute init failed";
  case LBAPI_ERR_LINK_INIT:
    return "link object init failed";
  case LBAPI_ERR_COUNTER_INIT:
    return "counter object init failed";
  case LBAPI_ERR_THRAD_START:
    return "start thread failed";
  // 链接相关
  case LBAPI_ERR_LINK_ADDR:
    return "link address error";
  case LBAPI_ERR_LINK_CONNECT:
    return "link connect failed";
  case LBAPI_ERR_NO_SOLARFLARE:
    return "no solarflare interface available";
  case LBAPI_ERR_LINK_CONNECT_REPEAT:
    return "link connect repeated";
  case LBAPI_ERR_LINK_START_RECV:
    return "start link recv failed";
  case LBAPI_ERR_LINK_DISCONNECTED:
    return "link physically disconnected";
  case LBAPI_ERR_LINK_HEART_TIMEOUT:
    return "link heart beat timeout";
  // 柜台状态
  case LBAPI_ERR_COUNTER_OFFLINE:
    return "counter customer state abnormal";
  // 消息相关
  case LBAPI_ERR_SEND_QUEUE_FULL:
    return "send queue full";
  case LBAPI_ERR_MSG_LEN:
    return "message length error";
  case LBAPI_ERR_MSG_TYPE:
    return "message type error";
  case LBAPI_ERR_BUILD_MSG:
    return "build message failed";
  case LBAPI_ERR_SEND_MSG:
    return "send message failed";
  // 登录相关
  case LBAPI_ERR_LOGIN_FAIL:
    return "login failed";
  case LBAPI_ERR_LOGIN_TIMEOUT:
    return "login timeout";
  // 事件注册
  case LBAPI_ERR_ADD_TIMER:
    return "add timer event failed";
  case LBAPI_ERR_ADD_WAKEFD:
    return "add wake fd event failed";
  // 业务校验
  case LBAPI_ERR_NOT_LOG_AGW:
    return "agw user not logged in";
  case LBAPI_ERR_NOT_LOG_CUST:
    return "counter investor not logged in";
  case LBAPI_ERR_NO_SEC:
    return "security info not found";
  case LBAPI_ERR_ALLOC_MEM:
    return "allocate memory failed";
  case LBAPI_ERR_NO_CUST:
    return "customer info not found";
  case LBAPI_ERR_INSERT_MAP:
    return "insert into map failed";
  default:
    return "unknown error";
  }
}

} // namespace lb_api

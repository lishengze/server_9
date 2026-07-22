#pragma once

/**
 * @file lib_loader.h
 * @brief 动态库加载模块
 *
 * 提供动态库加载和符号解析功能，用于插件系统。
 *
 * @note 编译时必须链接 -ldl 库
 *
 * @example
 * @code
 * lib_loader loader;
 * lb_comm_err err;
 *
 * // 加载动态库
 * loader.open(err, "./plugin.so");
 *
 * // 获取函数指针
 * int (*plugin_init)();
 * loader.get_func(err, plugin_init, "plugin_init");
 *
 * // 调用函数
 * plugin_init();
 *
 * loader.close();
 * @endcode
 */

#include "comm_errno.h"
#include "comm_sys.h"

#include <cstdio>
#include <cstring>
#include <dlfcn.h>

namespace lb_common {

/**
 * @brief 动态库加载类
 *
 * 提供动态库加载、卸载和符号解析功能。
 * 适用于插件系统、模块化架构等场景。
 */
class lib_loader {
private:
  void *instance; ///< 动态库句柄

public:
  /**
   * @brief 打开动态库
   *
   * @param[out] o_err 错误信息输出参数
   * @param[in] path_and_name 动态库完整路径和名称
   * @return 0=成功, 负数=错误码
   */
  int32 open(lb_comm_err &o_err, const char *path_and_name);

  /**
   * @brief 关闭动态库
   *
   * @note 关闭后所有获取的函数指针都将失效
   */
  void close();

  /**
   * @brief 从动态库获取函数指针
   *
   * @param[out] o_err 错误信息输出参数
   * @param[out] o_func 输出的函数指针
   * @param[in] func_name 函数名称
   * @return 0=成功, 负数=错误码
   *
   * @note 模板参数FuncPtr必须匹配函数签名
   *
   * @example
   * @code
   * int (*init_func)(void);
   * loader.get_func(err, init_func, "plugin_init");
   * @endcode
   */
  template <typename FuncPtr> int32 get_func(lb_comm_err &o_err, FuncPtr &o_func, const char *func_name) {
    o_err.clear();
    dlerror();

    if (NULL == instance) {
      o_err.err_code = LBERR_OBJ_INIT_FAIL;
      o_err.err_msg_len = strlen("not open lib instance");
      std::snprintf(o_err.err_msg, sizeof(o_err.err_msg) - 1, "not open lib instance");
      return LBERR_OBJ_INIT_FAIL;
    }

    void *ret = dlsym(instance, func_name);
    if (NULL != ret) {
      o_func = reinterpret_cast<FuncPtr>(ret);
      return 0;
    }

    o_err.err_code = LBERR_OBJ_NOT_HAVE;
    char *tpe_msg = dlerror();
    std::snprintf(o_err.err_msg, sizeof(o_err.err_msg) - 1, "%s", tpe_msg);
    o_err.err_msg_len = strlen(o_err.err_msg);
    return LBERR_OBJ_NOT_HAVE;
  }

  /**
   * @brief 构造函数
   */
  lib_loader() : instance(NULL){};

  /**
   * @brief 析构函数，自动关闭动态库
   */
  ~lib_loader() { close(); }

  /**
   * @brief 禁止拷贝构造
   */
  lib_loader(lib_loader const &) = delete;

  /**
   * @brief 禁止拷贝赋值
   */
  lib_loader &operator=(lib_loader const &) = delete;
};

} // namespace lb_common

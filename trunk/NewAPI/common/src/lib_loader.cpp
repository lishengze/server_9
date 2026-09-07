/**
 * @file lib_loader.cpp
 * @brief 动态库加载实现
 *
 * @see lib_loader.h
 */

#include "lib_loader.h"

namespace lb_common {

int32 lib_loader::open(lb_comm_err &o_err, const char *path_and_name) {
  o_err.clear();

  /*
	RTLD_LOCAL:
		当前加载的动态库的导出符号，仅对自身可见，对进程中其他动态库
		（包括后续加载的动态库）不可见，也无法被其他动态库的dlsym()查找或符号解析使用。
		简单说：动态库的符号 “私有化”，不和其他动态库共享，互不干扰。
		隔离性好，无符号冲突风险：多个动态库中即使有同名的导出符号
		（比如两个第三方库都有create_instance()函数），也不会互相覆盖，各自使用自身的符号
	RTLD_NOW：反之RTLD_LAZY
		在 **dlopen()调用执行时 **，就一次性解析（绑定）该动态库中所有的未定义符号
		（即动态库中调用的、自身未实现的函数 / 全局变量），并完成地址映射。
		加载慢，启动耗时较长：因为要一次性解析所有符号，尤其是大型动态库，
		会增加dlopen()的执行时间，拖慢程序启动速度。
	*/
  dlerror();
  instance = dlopen(path_and_name, RTLD_NOW | RTLD_LOCAL);
  if (NULL == instance) {
    o_err.err_code = LBERR_OBJ_OPEN_FAIL;
    char *tpe_msg = dlerror();
    std::snprintf(o_err.err_msg, sizeof(o_err.err_msg) - 1, "%s", tpe_msg);
    o_err.err_msg_len = strlen(o_err.err_msg);
    return LBERR_OBJ_OPEN_FAIL;
  }
  return 0;
}

void lib_loader::close() {
  if (NULL != instance) {
    dlclose(instance);
    instance = NULL;
  }
}

} // namespace lb_common

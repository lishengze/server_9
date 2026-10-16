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

  // B1: 校验 path_and_name。dlopen(NULL, ...) 会返回全局符号句柄而非 NULL，
  // 之后 dlsym 查到的符号是主程序里的同名导出，会污染 dl_loader 的语义且
  // 后续 dlclose 会释放/影响主进程状态。空字符串同样应拒绝。
  if (NULL == path_and_name || path_and_name[0] == '\0') {
    o_err.err_code = LBERR_ARGV_WRONG;
    int32 expected_len = std::snprintf(o_err.err_msg, sizeof(o_err.err_msg),
                                       "%s", "lib_loader::open: path_and_name is NULL or empty");
    o_err.err_msg_len = (expected_len < 0)
                            ? 0
                            : ((expected_len >= (int32)sizeof(o_err.err_msg))
                                   ? (int32)sizeof(o_err.err_msg) - 1
                                   : expected_len);
    return LBERR_ARGV_WRONG;
  }

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
  // B2: 先 dlopen 新库，只有成功后才释放旧句柄。失败时保留旧 instance，
  // 避免"新库加载失败导致旧句柄丢失"的资源/状态丢失。
  dlerror();
  void *new_instance = dlopen(path_and_name, RTLD_NOW | RTLD_LOCAL);
  if (NULL == new_instance) {
    o_err.err_code = LBERR_OBJ_OPEN_FAIL;
    char *tpe_msg = dlerror();
    int32 expected_len = 0;
    if (NULL != tpe_msg) {
      expected_len = std::snprintf(o_err.err_msg, sizeof(o_err.err_msg), "%s", tpe_msg);
    } else {
      expected_len = std::snprintf(o_err.err_msg, sizeof(o_err.err_msg), "%s", "dlopen failed");
    }
    // snprintf 返回期望写入的字节数（不含 \0），实际写入会被截断到 buf_size-1
    // err_msg_len 取实际写入的长度（不超过 buf_size-1）
    if (expected_len < 0) {
      o_err.err_msg_len = 0;
    } else if (expected_len >= (int32)sizeof(o_err.err_msg)) {
      o_err.err_msg_len = sizeof(o_err.err_msg) - 1;
    } else {
      o_err.err_msg_len = expected_len;
    }
    return LBERR_OBJ_OPEN_FAIL;
  }

  // 新句柄成功后才释放旧句柄。仅当 instance 是有效 dlopen 句柄时才 dlclose。
  if (NULL != instance) {
    dlclose(instance);
  }
  instance = new_instance;
  return 0;
}

void lib_loader::close() {
  if (NULL != instance) {
    dlclose(instance);
    instance = NULL;
  }
}

} // namespace lb_common

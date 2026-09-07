#pragma once

/**
 * @file cstr_utils.h
 * @brief C字符串工具模块
 *
 * 提供常用的C字符串处理工具函数，包括字符串分割、修剪、大小写转换等。
 */

#include "comm_sys.h"

#include <vector>

namespace lb_common {

/**
 * @brief C字符串工具类
 *
 * 提供静态方法进行C字符串处理
 */
class cstr_utils {
public:
  /**
   * @brief 分割字符串
   *
   * 按指定字符分割字符串，每项中不可有回车'\r'、换行符'\n'、'\0'
   *
   * @param[out] o_splits 输出的分割结果字符串指针数组
   * @param[in] line_buf 待分割的字符串缓冲区
   * @param[in] split_c 分隔符
   * @return 处理的字符串长度
   *
   * @note 一次解析line_buf到\n'、'\r'、'\0'结束，会将'\n'、'\r'替换为'\0'
   * @note 使用场景: CSV解析、INI文件解析、命令行参数解析
   */
  static int32 split(std::vector<char *> &o_splits, char *line_buf, char split_c);

  /**
   * @brief 去除字符串首尾空格，置为‘\0'，并返回首字符指针。
   *
   * @param[in,out] str 待处理的字符串
   * @return 处理后的字符串指针（可能与输入不同）
   *
   * @note 直接在原字符串上修改，修改了字符串内容
   * @note 使用场景: 配置文件解析、用户输入清理
   */
  static char *trim(char *str);

  /**
   * @brief 转换为小写
   *
   * 将字符串中所有字符转换为小写
   *
   * @param[in,out] str 待转换的字符串
   *
   * @note 直接在原字符串上修改
   * @note 使用场景: 不区分大小写的字符串比较
   */
  static void lower(char *str);

  /**
   * @brief UTF-8转换为指定编码实现
   *
   * 使用iconv进行字符编码转换
   *
   * @param[in] gb_name 目标编码名称
   * @param[in] dst 目标缓冲区
   * @param[in] src 源字符串
   * @param[in] dst_size 目标缓冲区大小
   * @param[in] src_size 源字符串大小
   * @return 0=成功, 负数=错误码
   *
   * @note 调用者需确保iconv_t资源正确释放
   */
  // static int32 utf8_to_gb(const char *gb_name,char *dst,char *src,
  //	int32 dst_size,int32 src_size);

  /**
   * @brief 指定编码转换为UTF-8实现
   *
   * 使用iconv进行字符编码转换
   *
   * @param[in] gb_name 源编码名称
   * @param[in] dst 目标缓冲区
   * @param[in] src 源字符串
   * @param[in] dst_size 目标缓冲区大小
   * @param[in] src_size 源字符串大小
   * @return 0=成功, 负数=错误码
   */
  // static int32 gb_to_utf8(const char *gb_name,char *dst,char *src,
  //	int32 dst_size,int32 src_size);
};

} // namespace lb_common

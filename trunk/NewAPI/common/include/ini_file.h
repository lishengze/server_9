#pragma once

/**
 * @file ini_file.h
 * @brief INI配置文件解析模块
 *
 * 提供INI格式配置文件的读取功能，支持多种数据类型和列表读取。
 *
 * @note INI文件格式:
 *   [section1]
 *   key1 = value1
 *   key2 = value2, value3, value4  // 列表值用逗号分隔
 *
 *   [section2]
 *   key3 = 123
 */

#include "comm_sys.h"

#include <cfloat>
#include <cstdio>
#include <map>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace lb_common {

/**
 * @brief INI节名最大长度
 */
static constexpr int MAX_LEN_INI_SECT_NAME = 128;

/**
 * @brief INI键名最大长度
 */
static constexpr int MAX_LEN_INI_KEY = 128;

/**
 * @brief INI值最大长度
 */
static constexpr int MAX_LEN_INI_VAL = 512;

/**
 * @brief INI键值对结构体
 *
 * 使用固定长度数组存储，避免动态内存分配，适合对外接口。
 */
struct ini_key_pair {
  char key[MAX_LEN_INI_KEY];   ///< 键（固定长度，避免动态分配）
  char value[MAX_LEN_INI_VAL]; ///< 值（固定长度）
};

/**
 * @brief INI文件读取类
 *
 * 支持INI格式配置文件的解析，提供多种数据类型读取接口。
 * 支持列表值读取（逗号分隔）和按节读取所有键值对。
 * 非线程安全
 *
 * @note 限制:
 * - 注释行以 ; 或 # 开头
 * - 键值对格式: key = value
 * - 列表值: key = val1, val2, val3
 * - 字符串值中的空格会被保留
 *
 * @example
 * @code
 * ini_reader reader;
 *
 * // 打开INI文件
 * reader.open("config.ini");
 *
 * // 读取整数值
 * int32 port;
 * reader.read("server", "port", port);
 *
 * // 读取浮点数值
 * double timeout;
 * reader.read("server", "timeout", timeout);
 *
 * // 读取字符串
 * char host[64];
 * reader.read("server", "host", host, sizeof(host));
 *
 * // 读取列表
 * std::vector<int32> paths;
 * reader.read_list("paths", "search", paths);
 *
 * // 读取整个节
 * std::vector<ini_key_pair> kvs;
 * reader.read_section("database", kvs);
 *
 * reader.close();
 * @endcode
 */
class ini_reader {
private:
  /**
   * @brief INI节内部结构
   */
  struct Section {
    char name[MAX_LEN_INI_SECT_NAME];             ///< 节名
    std::map<std::string, std::string> keyValues; ///< 键值对映射
    Section *next;                                ///< 下一节指针

    /**
     * @brief 构造函数
     * @param[in] sec_name 节名
     */
    Section(const char *sec_name) : keyValues(), next(NULL) {
      memset(name, 0, sizeof(name));
      strncpy(name, sec_name, sizeof(name) - 1);
    }
  };

  Section *m_sections; ///< 节链表头指针
  int32 section_num;   ///< 节数量
  std::FILE *m_file;   ///< 文件句柄

  /**
   * @brief 验证参数有效性
   * @param[in] section 节名
   * @param[in] key 键名
   * @return true=有效, false=无效
   */
  static bool is_valid(const char *section, const char *key) {
    if (section == NULL || key == NULL) {
      return false;
    }
    if (strlen(section) >= MAX_LEN_INI_SECT_NAME || strlen(key) >= MAX_LEN_INI_KEY) {
      return false;
    }
    return true;
  }

  /**
   * @brief 查找节
   * @param[in] sec_name 节名
   * @return 节指针，未找到返回NULL
   */
  Section *find_section(const char *sec_name) const {
    Section *current = m_sections;
    while (current != NULL) {
      if (strcmp(current->name, sec_name) == 0) {
        return current;
      }
      current = current->next;
    }
    return NULL;
  }

  /**
   * @brief 创建节
   * @param[in] sec_name 节名
   * @return 新创建的节指针
   */
  Section *create_section(const char *sec_name) {
    Section *sec = new (std::nothrow) Section(sec_name);
    if (NULL == sec) {
      return NULL;
    }

    sec->next = m_sections;
    m_sections = sec;
    section_num++;

    return sec;
  }

public:
  /**
   * @brief 读取字符串类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的字符串
   * @param[in] val_size 缓冲区大小
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, char *o_val, int32 val_size);

  /**
   * @brief 读取16位整数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的16位整数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, int16 &o_val);

  /**
   * @brief 读取32位整数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的32位整数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, int32 &o_val);

  /**
   * @brief 读取64位整数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的64位整数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, int64 &o_val);

  /**
   * @brief 读取无符号16位整数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的无符号16位整数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, uint16 &o_val);

  /**
   * @brief 读取无符号32位整数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的无符号32位整数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, uint32 &o_val);

  /**
   * @brief 读取无符号64位整数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的无符号64位整数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, uint64 &o_val);

  /**
   * @brief 读取浮点数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的浮点数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, float &o_val);

  /**
   * @brief 读取双精度浮点数类型值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_val 输出的双精度浮点数值
   * @return 0=成功, <0=出错
   */
  int32 read(const char *section, const char *key, double &o_val);

  /**
   * @brief 读取int32列表值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_vals 输出的int32列表
   * @return 0=成功, <0=出错
   *
   * @note 值用逗号分隔，如: 1, 2, 3, 4
   */
  int32 read_list(const char *section, const char *key, std::vector<int32> &o_vals);

  /**
   * @brief 读取int64列表值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_vals 输出的int64列表
   * @return 0=成功, <0=出错
   */
  int32 read_list(const char *section, const char *key, std::vector<int64> &o_vals);

  /**
   * @brief 读取double列表值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_vals 输出的double列表
   * @return 0=成功, <0=出错
   */
  int32 read_list(const char *section, const char *key, std::vector<double> &o_vals);

  /**
   * @brief 读取字符串列表值
   *
   * @param[in] section 节名
   * @param[in] key 键名
   * @param[out] o_vals 输出的字符串列表
   * @return 0=成功, <0=出错
   */
  int32 read_list(const char *section, const char *key, std::vector<std::string> &o_vals);

  /**
   * @brief 读取整个节的所有键值对
   *
   * @param[in] section 节名
   * @param[out] o_kvs 输出的键值对列表
   * @return 0=成功, <0=出错
   */
  int32 read_section(const char *section, std::vector<ini_key_pair> &o_kvs);

  /**
   * @brief 获取节数量
   * @return 节数量
   */
  int32 get_section_num() const { return section_num; }

  /**
   * @brief 打开INI文件
   *
   * @param[in] filename INI文件路径
   * @return 0=成功, <0=出错
   */
  int32 open(const char *filename);

  /**
   * @brief 关闭INI文件
   */
  void close();

  /**
   * @brief 构造函数
   */
  ini_reader() : m_sections(NULL), section_num(0), m_file(NULL) {}

  /**
   * @brief 析构函数，自动关闭文件
   */
  ~ini_reader() { close(); }

  /**
   * @brief 禁用拷贝构造
   */
  ini_reader(const ini_reader &) = delete;

  /**
   * @brief 禁用拷贝赋值
   */
  ini_reader &operator=(const ini_reader &) = delete;
};

} // namespace lb_common

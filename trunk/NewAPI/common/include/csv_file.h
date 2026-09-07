#pragma once

/**
 * @file csv_file.h
 * @brief CSV文件读写模块
 *
 * 提供CSV文件的读取和写入功能，支持多种数据类型。
 *
 * @note csv_reader 不是线程安全的，多线程使用时需要外部加锁
 */

#include "comm_sys.h"
#include "mlock.h"

#include <cfloat>
#include <cstdio>
#include <map>
#include <string>
#include <unistd.h>

namespace lb_common {

/**
 * @brief CSV文件读取类
 *
 * 支持按行解析CSV文件，第一行为列名，后续行为数据。
 * 自动按块读取文件（64KB），提高IO效率。
 *
 * @note 限制：
 * - CSV文件第一行必须是列名
 * - 列名与对应结构体成员名称需要一致
 * - 不支持多线程并发读取
 * - 每个字段不能包含回车、换行符、空字符
 *
 * @example
 * @code
 * csv_reader reader;
 *
 * // 打开CSV文件
 * reader.open("data.csv");
 *
 * // 逐行解析
 * while(reader.parse_line() > 0) {
 *     char name[64];
 *     int32 age;
 *     double salary;
 *
 *     reader.read("name", name, sizeof(name));
 *     reader.read("age", age);
 *     reader.read("salary", salary);
 *
 *     // 处理数据...
 * }
 *
 * reader.close();
 * @endcode
 */
class csv_reader {
private:
  /**
	 * @brief CSV文件读取缓冲区大小
	 */
#define CSV_FILE_READ_LEN 65536 //一次读取CSV文件的长度

  /**
     * @brief 列名-值映射表
     *
     * key: csv 文件第一个有效的行，该行表示每列的名称
     * val: 解析每行数据的按逗号分割后的每项指针，指向read_buf中的位置
     */
  std::map<std::string, char *> kv_map;

  int32 read_len;  ///< 每次实际读取的长度
  int32 parse_len; ///< 本次读取的已解析长度
  int64 read_off;  ///< 已读到的文件位置，可用于检查是否已读到文件末尾

  char read_buf[CSV_FILE_READ_LEN]; ///< CSV文件读取缓冲区（64KB）

  std::FILE *m_file; ///< 文件句柄

  /**
	 * @brief 查找列值
	 * @param[in] item_name 列名
	 * @return 列值指针
	 */
  char *find_col_value(const char *item_name);

public:
  /**
	 * @brief 解析CSV文件的一行
	 *
	 * 检查已读取的长度是否都已解析，继续解析已读取的剩余部分。
	 * 若存在一个完整行，则解析该行；
	 * 否则，若该不完整的行不是缓存中的最后部分，则出错；
	 * 若是，则继续按64k-剩余长度，读取文件，再次查看解析完整行。
	 * 将解析的完整行的位置，更新设置kv_map的值部分。
	 *
	 * @return 1=成功解析1行, 0=到文件末尾完成文件解析, <0=出错
	 */
  int32 parse_line();

  /**
	 * @brief 读取字符类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的字符值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, char &o_val);

  /**
	 * @brief 读取无符号字符类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的无符号字符值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, unsigned char &o_val);

  /**
	 * @brief 读取字符串类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的字符串缓冲区
	 * @param[in] val_size 缓冲区大小
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, char *o_val, int32 val_size);

  /**
	 * @brief 读取16位整数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的16位整数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, int16 &o_val);

  /**
	 * @brief 读取32位整数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的32位整数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, int32 &o_val);

  /**
	 * @brief 读取64位整数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的64位整数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, int64 &o_val);

  /**
	 * @brief 读取无符号16位整数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的无符号16位整数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, uint16 &o_val);

  /**
	 * @brief 读取无符号32位整数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的无符号32位整数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, uint32 &o_val);

  /**
	 * @brief 读取无符号64位整数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的无符号64位整数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, uint64 &o_val);

  /**
	 * @brief 读取浮点数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的浮点数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, float &o_val);

  /**
	 * @brief 读取双精度浮点数类型列
	 *
	 * @param[in] iterm_name 列名
	 * @param[out] o_val 输出的双精度浮点数值
	 * @return 0=成功, <0=出错
	 */
  int32 read(const char *iterm_name, double &o_val);

  /**
	 * @brief 打开CSV文件
	 *
	 * 打开文件并做一次读取，解析出第一个有效的行作为列名，构建好kv_map
	 *
	 * @param[in] filename CSV文件路径
	 * @return 0=成功, <0=出错
	 *
	 * @note 若文件为空，视为正常
	 */
  int32 open(const char *filename);

  /**
	 * @brief 关闭CSV文件
	 */
  void close();

  /**
	 * @brief 构造函数
	 */
  csv_reader() : read_len(0), parse_len(0), read_off(0), m_file(NULL) { memset(read_buf, '\0', sizeof(read_buf)); }

  /**
	 * @brief 析构函数，自动关闭文件
	 */
  ~csv_reader() { close(); }

  /**
	 * @brief 禁用拷贝构造
	 */
  csv_reader(const csv_reader &) = delete;

  /**
	 * @brief 禁用拷贝赋值
	 */
  csv_reader &operator=(const csv_reader &) = delete;
};

/**
 * @brief CSV文件写入类
 *
 * 支持按行写入CSV文件，内部使用互斥锁保证线程安全。
 *
 * @note 写入时每行需要自己格式化为逗号分隔的字符串
 *
 * @example
 * @code
 * csv_writer writer;
 *
 * // 打开文件（追加模式）
 * writer.open("output.csv", 1);
 *
 * // 写入数据行
 * char line[256];
 * sprintf(line, "Alice,30,5000.50");
 * writer.write_line(line, strlen(line));
 *
 * writer.sync_line(); // 刷新到磁盘
 *
 * writer.close();
 * @endcode
 */
class csv_writer {
private:
  cmutex m_lock;     ///< 互斥锁，保护文件写入
  std::FILE *m_file; ///< 文件句柄

public:
  /**
	 * @brief 写入一行数据
	 *
	 * @param[in] line_data 行数据缓冲区
	 * @param[in] data_len 数据长度
	 * @return 0=成功, <0=出错
	 *
	 * @note 每次写入一个完整行，行数据应由调用者格式化
	 */
  int32 write_line(char *line_data, int32 data_len);

  /**
	 * @brief 刷新文件缓冲区到磁盘
	 */
  void sync_line();

  /**
	 * @brief 打开CSV文件用于写入
	 *
	 * @param[in] filename 文件名
	 * @param[in] is_add 模式: 0=覆盖, 1=追加
	 * @return 0=成功, <0=出错
	 */
  int32 open(const char *filename, int32 is_add);

  /**
	 * @brief 关闭CSV文件
	 */
  void close();

  /**
	 * @brief 构造函数
	 */
  csv_writer() : m_file(NULL) {}

  /**
	 * @brief 析构函数，自动关闭文件
	 */
  ~csv_writer() { close(); }

  /**
	 * @brief 禁用拷贝构造
	 */
  csv_writer(const csv_writer &) = delete;

  /**
	 * @brief 禁用拷贝赋值
	 */
  csv_writer &operator=(const csv_writer &) = delete;
};

} // namespace lb_common

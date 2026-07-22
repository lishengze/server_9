/**
 * @file csv_file.cpp
 * @brief CSV文件读写实现
 *
 * @see csv_file.h
 */

#include "csv_file.h"
#include "comm_errno.h"
#include "cstr_utils.h"
#include "mutils.h"

#include <cstdlib>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

namespace lb_common {

// 关闭文件，清理资源
void csv_reader::close() {
  if (m_file != NULL) {
    std::fclose(m_file);
    m_file = NULL;
  }
  kv_map.clear();
  read_len = 0;
  parse_len = 0;
  read_off = 0;
  std::memset(read_buf, 0, sizeof(read_buf));
}

// 打开CSV文件，解析表头构建kv_map
int32 csv_reader::open(const char *filename) {
  // 参数校验
  if (filename == NULL || *filename == '\0') {
    return LBERR_ARGV_WRONG;
  }

  // 先关闭已打开的文件
  close();

  // 打开文件（二进制模式，避免Linux下换行符转换）
  m_file = std::fopen(filename, "rb");
  if (m_file == NULL) {
    return LBERR_OBJ_OPEN_FAIL; // 文件打开失败
  }
  std::memset(read_buf, 0, CSV_FILE_READ_LEN);
  // 第一次读取数据到缓存
  read_len = comm_utils::read_file(m_file, read_buf, CSV_FILE_READ_LEN);
  if (read_len < 0) {
    close();
    return read_len;
  }
  read_off = read_len;
  parse_len = 0;

  // 解析表头（第一个有效行）
  char *line_start = read_buf;
  char *line_end = NULL;
  // 找第一个完整行（换行符\n为行结束）
  while (parse_len < read_len) {
    line_end = static_cast<char *>(std::memchr(line_start, '\n', read_len - parse_len));
    if (line_end == NULL) {
      return LBERR_OBJ_NOT_HAVE;
    }
    parse_len = (line_end - read_buf + 1);

    // 截断行尾，处理换行符
    *line_end = '\0';
    line_start = cstr_utils::trim(line_start); // 去除首尾空白
    if (line_start[0] != '\0') {               // 非空行作为表头
      break;
    }

    // 跳过空行，继续找
    line_start = read_buf + parse_len;
  }

  // 无有效表头（文件为空），视为正常
  if (line_end == NULL) {
    return 0;
  }

  // 分割表头，构建kv_map
  std::vector<char *> cols;
  // 调用split：返回值是遍历的字符长度，段数取cols.size()
  int32 traverse_len = cstr_utils::split(cols, line_start, ',');
  if (traverse_len <= 0) { // 检查split是否失败
    close();
    return LBERR_OBJ_NOT_HAVE;
  }
  // 遍历分割后的列
  for (size_t i = 0; i < cols.size(); i++) {
    char *tpi = cstr_utils::trim(cols[i]); // 列名去空白
    kv_map[std::string(tpi)] = NULL;
  }

  return 0;
}

// 解析缓存中的完整行，更新kv_map的val
int32 csv_reader::parse_line() {
  if (m_file == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 文件未打开
  }
  if (read_len == 0 && parse_len == 0) {
    // 已读到结束后，又再次调用
    return 0; // LBERR_OBJ_IS_EMPTY;  // 无任何数据可解析
  }

  char *line_start = NULL;
  bool is_eof = false;
  // 先检查未解析区域是否有完整行，有则解析；无则补充读取
  while (true) {
    line_start = NULL;
    int32 unparse_len = read_len - parse_len;   // 未解析字节数
    char *unparse_start = read_buf + parse_len; // 未解析区域起始地址

    if (unparse_len > 0) {
      // 检查未解析区域是否有换行符（有则直接判定为完整行）
      char *line_end = static_cast<char *>(std::memchr(unparse_start, '\n', unparse_len));
      if (line_end != NULL) {
        *line_end = '\0';
        parse_len = line_end - read_buf + 1;

        line_start = cstr_utils::trim(unparse_start); // 去除首尾空白
        if (line_start[0] != '\0') {                  // 非空行作为表头
          break;
        } else {
          continue;
        }
      }
    } else {
      unparse_len = 0;
    }

    if (is_eof) {
      // 无换行符 + 已到文件末尾
      return 0;
    }

    if (CSV_FILE_READ_LEN - unparse_len <= 0) {
      // 缓存已满且无换行符解析失败（超长行）
      return LBERR_OBJ_IS_FULL;
    }

    if (unparse_len > 0) {
      std::memmove(read_buf, unparse_start, unparse_len);
      read_len = unparse_len;
    } else {
      read_len = 0;
    }
    parse_len = 0;
    std::memset(read_buf + read_len, 0, CSV_FILE_READ_LEN - read_len);
    // 读取新数据到缓存末尾（read_buf + read_len 开始）
    int32 new_read = comm_utils::read_file(m_file, read_buf + read_len, CSV_FILE_READ_LEN - read_len);
    if (std::feof(m_file)) {
      is_eof = true;
    }
    if (new_read < 0) {
      return LBERR_OBJ_READ_FAIL;
    }

    // 读取成功,更新缓存状态，继续循环检查
    read_len += new_read;
    read_off += new_read;
  }

  // 解析找到的完整行
  assert(NULL != line_start);
  std::vector<char *> cols;
  cstr_utils::split(cols, line_start, ',');
  if (cols.size() < kv_map.size()) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  // 更新kv_map：列名对应该行的列数据指针
  auto kv_iter = kv_map.begin();
  for (size_t i = 0; i < cols.size() && kv_iter != kv_map.end(); ++i, ++kv_iter) {
    char *tpi = cstr_utils::trim(cols[i]);
    kv_iter->second = tpi;
  }

  return 1; // 解析成功
}

// 辅助函数：查找列名对应的字符串
char *csv_reader::find_col_value(const char *item_name) {
  // 列名转小写匹配（和表头统一规则）
  std::string key(item_name);
  auto iter = kv_map.find(key);
  return (iter != kv_map.end()) ? iter->second : NULL;
}

// 读取char类型
int32 csv_reader::read(const char *iterm_name, char &o_val) {
  if (NULL == iterm_name || iterm_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(iterm_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }
  o_val = (val_str[0] != '\0') ? val_str[0] : '\0';
  return 0;
}
int32 csv_reader::read(const char *iterm_name, unsigned char &o_val) {
  if (NULL == iterm_name || iterm_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(iterm_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }
  o_val = (val_str[0] != '\0') ? val_str[0] : '\0';
  return 0;
}

// 读取char*（字符串）类型
int32 csv_reader::read(const char *item_name, char *o_val, int32 val_size) {
  if (NULL == item_name || item_name[0] == '\0' || o_val == NULL || val_size <= 0) {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE;
  }

  // 安全拷贝，避免越界
  std::strncpy(o_val, val_str, val_size - 1);
  o_val[val_size - 1] = '\0';
  return 0;
}

// 读取int16类型
int32 csv_reader::read(const char *item_name, int16 &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  long val = std::strtol(val_str, &end_ptr, 10);
  if (end_ptr == val_str || val < INT16_MIN || val > INT16_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }
  o_val = static_cast<int16>(val);
  return 0;
}
int32 csv_reader::read(const char *item_name, uint16 &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  long val = std::strtol(val_str, &end_ptr, 10);
  if (end_ptr == val_str || val < 0 || val > UINT16_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }
  o_val = static_cast<uint16>(val);
  return 0;
}

// 读取int32类型
int32 csv_reader::read(const char *item_name, int32 &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  long val = std::strtol(val_str, &end_ptr, 10);
  if (end_ptr == val_str || val < INT32_MIN || val > INT32_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }
  o_val = static_cast<int32>(val);
  return 0;
}
int32 csv_reader::read(const char *item_name, uint32 &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  long val = std::strtol(val_str, &end_ptr, 10);
  if (end_ptr == val_str || val < 0 || val > UINT32_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }
  o_val = static_cast<uint32>(val);
  return 0;
}

// 读取int64类型
int32 csv_reader::read(const char *item_name, int64 &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  long long val = std::strtoll(val_str, &end_ptr, 10);
  if (end_ptr == val_str) {
    return LBERR_OBJ_NUM_LIMIT;
  }
  o_val = static_cast<int64>(val);
  return 0;
}
int32 csv_reader::read(const char *item_name, uint64 &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  long long val = std::strtoll(val_str, &end_ptr, 10);
  if (end_ptr == val_str) {
    return LBERR_OBJ_NUM_LIMIT;
  }
  o_val = static_cast<uint64>(val);
  return 0;
}

// 读取float类型
int32 csv_reader::read(const char *item_name, float &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  float val = std::strtof(val_str, &end_ptr);
  if (end_ptr == val_str) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = val;
  return 0;
}

// 读取double类型
int32 csv_reader::read(const char *item_name, double &o_val) {
  if (NULL == item_name || item_name[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  char *val_str = find_col_value(item_name);
  if (val_str == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 列名不存在
  }

  char *end_ptr = NULL;
  double val = std::strtod(val_str, &end_ptr);
  if (end_ptr == val_str) {
    return LBERR_OBJ_NUM_LIMIT;
  }
  o_val = val;
  return 0;
}

// 打开CSV文件（支持新建/追加，默认覆盖原有内容）
int32 csv_writer::open(const char *filename, int32 is_add) {
  if (filename == NULL || *filename == '\0') {
    return LBERR_ARGV_WRONG;
  }

  close();

  clock_guard<cmutex> tlg(m_lock);

  // 打开文件
  if (is_add == 0) {
    m_file = std::fopen(filename, "wb");
  } else {
    m_file = std::fopen(filename, "ab");
  }
  if (m_file == NULL) {
    return LBERR_OBJ_OPEN_FAIL;
  }

  // 可选优化：_IONBF 直接系统调用写入内核；
  //           _IOLBF 每行写入内核
  // 设置库缓冲大小为PIPE_BUF，匹配原子写入阈值
  std::setvbuf(m_file, NULL, _IOFBF, PIPE_BUF);

  return 0;
}

// 关闭文件，刷新缓冲区（关键：防止缓冲数据丢失）
void csv_writer::close() {
  clock_guard<cmutex> tlg(m_lock);

  if (m_file != NULL) {
    // 先刷新库缓冲到内核
    std::fflush(m_file);
    // 关闭文件句柄
    std::fclose(m_file);
    m_file = NULL;
  }
}

// 写入一行完整数据，平衡Linux下的完整性和性能
int32 csv_writer::write_line(char *line_data, int32 data_len) {
  if (line_data == NULL || data_len <= 0) {
    return LBERR_ARGV_WRONG; // 参数非法
  }

  clock_guard<cmutex> tlg(m_lock);

  if (m_file == NULL) {
    return LBERR_OBJ_NOT_HAVE; // 文件未打开
  }

  // Linux内核特性：PIPE_BUF内的写入是原子的（默认4096字节）
  const int PIPE_BUF_SIZE = PIPE_BUF; // 从unistd.h获取

  // 原子写入整行（fwrite是库缓冲，最终会调用内核write）
  int32 write_bytes = comm_utils::write_file(m_file, line_data, data_len);
  if (write_bytes < data_len) {
    return LBERR_OBJ_WRITE_FAIL;
  }

  // 强制换行（如果line_data末尾无\n，补充换行符）
  if (data_len == 0 || line_data[data_len - 1] != '\n') {
    std::fputc('\n', m_file);
  }

  // 如需强一致性，打开此注释
  if (data_len >= PIPE_BUF_SIZE)
    std::fflush(m_file);

  return 0;
}
void csv_writer::sync_line() {
  if (m_file != NULL) {
    fsync(fileno(m_file));
  }
}

} // namespace lb_common

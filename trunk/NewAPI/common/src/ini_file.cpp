/**
 * @file ini_file.cpp
 * @brief INI配置文件解析实现
 *
 * @see ini_file.h
 */

#include "ini_file.h"
#include "comm_errno.h"
#include "cstr_utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

//#pragma warning(disable:4267)

namespace lb_common {

int32 ini_reader::open(const char *filename) {
  close();

  // 参数校验
  if (filename == NULL || filename[0] == '\0') {
    return LBERR_ARGV_WRONG;
  }

  // 打开文件
  m_file = std::fopen(filename, "r");
  if (m_file == NULL) {
    return LBERR_OBJ_OPEN_FAIL;
  }

  char line_buf[8192] = {0};
  Section *current_section = NULL;
  char *pline_avlid = NULL;

  // 逐行解析
  while (std::fgets(line_buf, sizeof(line_buf), m_file) != NULL) {
    pline_avlid = cstr_utils::trim(line_buf);

    //过滤空行（trim后无内容）
    if (pline_avlid[0] == '\0') {
      std::memset(line_buf, 0, sizeof(line_buf));
      continue;
    }

    // 过滤纯注释行（行首为;/#，含前置空白）
    // 跳过所有以;/#开头的行（无论前面是否有空白）
    if (pline_avlid[0] == ';' || pline_avlid[0] == '#') {
      std::memset(line_buf, 0, sizeof(line_buf));
      continue;
    }

    // 截断键值对后的内联注释（如key=val;comment）
    // 查找第一个;/#（非字符串内的，INI无字符串引号，简化处理）
    char *comment_ptr = NULL;
    for (int32 i = 0; pline_avlid[i] != '\0'; i++) {
      if (pline_avlid[i] == ';' || pline_avlid[i] == '#') {
        comment_ptr = &(pline_avlid[i]);
        break;
      }
    }
    if (comment_ptr != NULL) {
      *comment_ptr = '\0';                         // 截断注释部分
      pline_avlid = cstr_utils::trim(pline_avlid); // 截断后重新trim（避免注释前的空白）
      if (pline_avlid[0] == '\0') {                // 截断后无内容，视为空行
        std::memset(line_buf, 0, sizeof(line_buf));
        continue;
      }
    }

    //处理节[section]
    if (pline_avlid[0] == '[' && std::strrchr(pline_avlid, ']') != NULL) {
      // 提取节名（去掉[]）
      char section_name[128] = {0};
      size_t sec_name_len = std::strrchr(pline_avlid, ']') - pline_avlid - 1;
      if (sec_name_len >= 128) {
        sec_name_len = 127; // 截断，留1字节给'\0'
        std::fprintf(stderr, "警告：Section名称超过128字节，已截断！\n");
      }
      std::strncpy(section_name, pline_avlid + 1, sec_name_len);
      char *psec_name = cstr_utils::trim(section_name);

      // 创建节失败返回内存错误
      current_section = find_section(psec_name);
      if (current_section != NULL) {
        close(); // 回滚已分配的资源
        return LBERR_OBJ_HAVE_EXIST;
      }
      current_section = create_section(psec_name);
      if (current_section == NULL) {
        close(); // 回滚已分配的资源
        return LBERR_MEM_ALLOC_FAIL;
      }
      std::memset(line_buf, 0, sizeof(line_buf));
      continue;
    }

    // 处理键值对 key=value
    char *equal_sign = std::strchr(pline_avlid, '=');
    if (equal_sign != NULL && current_section != NULL) {
      // 分割键和值
      char key_buf[MAX_LEN_INI_KEY] = {0};
      char val_buf[MAX_LEN_INI_VAL] = {0};
      std::strncpy(key_buf, pline_avlid, equal_sign - pline_avlid);
      char *pkey = cstr_utils::trim(key_buf);
      std::strncpy(val_buf, equal_sign + 1, sizeof(val_buf) - 1);
      char *pval = cstr_utils::trim(val_buf);

      // 跳过空键
      if (pkey[0] != '\0') {
        current_section->keyValues[std::string(pkey)] = std::string(pval);
      }
    }

    std::memset(line_buf, 0, sizeof(line_buf));
  }

  return 0;
}

void ini_reader::close() {
  if (m_file != NULL) {
    std::fclose(m_file);
    m_file = NULL;
  }

  // 释放Section链表
  Section *temp = NULL;
  while (m_sections != NULL) {
    temp = m_sections;
    m_sections = m_sections->next;
    delete temp;
  }

  section_num = 0;
}

int32 ini_reader::read(const char *section, const char *key, char *o_val, int32 val_size) {
  if (!is_valid(section, key) || o_val == NULL || val_size <= 0) {
    return LBERR_ARGV_WRONG;
  }

  Section *sec = find_section(section);
  if (sec == NULL) {
    return LBERR_OBJ_NOT_HAVE;
  }

  auto it = sec->keyValues.find(std::string(key));
  if (it == sec->keyValues.end()) {
    return LBERR_OBJ_NOT_HAVE;
  }

  std::strncpy(o_val, it->second.c_str(), val_size - 1);
  o_val[val_size - 1] = '\0';
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, int16 &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  int64 val = std::strtol(buf, &end_ptr, 10);
  if (end_ptr == buf || val < INT16_MIN || val > INT16_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = static_cast<int16>(val);
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, int32 &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  int64 val = std::strtol(buf, &end_ptr, 10);
  if (end_ptr == buf || val < INT32_MIN || val > INT32_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = static_cast<int32>(val);
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, int64 &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  long long val = std::strtoll(buf, &end_ptr, 10);
  if (end_ptr == buf) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = val;
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, uint16 &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  int64 val = std::strtol(buf, &end_ptr, 10);
  if (end_ptr == buf || val < 0 || val > UINT16_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = static_cast<uint16>(val);
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, uint32 &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  int64 val = std::strtol(buf, &end_ptr, 10);
  if (end_ptr == buf || val < 0 || val > UINT32_MAX) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = static_cast<uint32>(val);
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, uint64 &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  long long val = std::strtoll(buf, &end_ptr, 10);
  if (end_ptr == buf) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = val;
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, float &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  float val = std::strtof(buf, &end_ptr);
  if (end_ptr == buf) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = val;
  return 0;
}

int32 ini_reader::read(const char *section, const char *key, double &o_val) {
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }
  char buf[64] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  char *end_ptr = NULL;
  double val = std::strtod(buf, &end_ptr);
  if (end_ptr == buf) {
    return LBERR_OBJ_NUM_LIMIT;
  }

  o_val = val;
  return 0;
}

int32 ini_reader::read_list(const char *section, const char *key, std::vector<int32> &o_vals) {
  o_vals.clear();
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }

  char buf[MAX_LEN_INI_VAL] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  std::vector<char *> splits;
  cstr_utils::split(splits, buf, ',');
  int32 tn = splits.size();
  for (int32 i = 0; i < tn; i++) {
    char *end_ptr = NULL;
    int64 val = std::strtol(splits[i], &end_ptr, 10);
    if (end_ptr == splits[i] || val < INT32_MIN || val > INT32_MAX) {
      return LBERR_OBJ_NUM_LIMIT;
    }

    o_vals.push_back(static_cast<int32>(val));
  }

  return 0;
}

int32 ini_reader::read_list(const char *section, const char *key, std::vector<int64> &o_vals) {
  o_vals.clear();
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }

  char buf[MAX_LEN_INI_VAL] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  std::vector<char *> splits;
  cstr_utils::split(splits, buf, ',');
  int32 tn = splits.size();
  for (int32 i = 0; i < tn; i++) {
    char *end_ptr = NULL;
    long long val = std::strtoll(splits[i], &end_ptr, 10);
    if (end_ptr == splits[i] || val < INT64_MIN || val > INT64_MAX) {
      return LBERR_OBJ_NUM_LIMIT;
    }

    o_vals.push_back(static_cast<int64>(val));
  }

  return 0;
}

int32 ini_reader::read_list(const char *section, const char *key, std::vector<double> &o_vals) {
  o_vals.clear();
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }

  char buf[MAX_LEN_INI_VAL] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  std::vector<char *> splits;
  cstr_utils::split(splits, buf, ',');
  int32 tn = splits.size();
  for (int32 i = 0; i < tn; i++) {
    char *end_ptr = NULL;
    double val = std::strtod(splits[i], &end_ptr);
    if (end_ptr == splits[i]) {
      return LBERR_OBJ_NUM_LIMIT;
    }

    o_vals.push_back(val);
  }

  return 0;
}

int32 ini_reader::read_list(const char *section, const char *key, std::vector<std::string> &o_vals) {
  o_vals.clear();
  if (!is_valid(section, key)) {
    return LBERR_ARGV_WRONG;
  }

  char buf[MAX_LEN_INI_VAL] = {0};
  int32 ret = read(section, key, buf, sizeof(buf));
  if (ret != 0) {
    return ret;
  }

  std::vector<char *> splits;
  cstr_utils::split(splits, buf, ',');
  int32 tn = splits.size();
  for (int32 i = 0; i < tn; i++) {
    o_vals.push_back(std::string(splits[i]));
  }

  return 0;
}

int32 ini_reader::read_section(const char *section, std::vector<ini_key_pair> &o_kvs) {
  // 参数校验
  if (section == NULL || strlen(section) >= MAX_LEN_INI_SECT_NAME) {
    return LBERR_ARGV_WRONG;
  }

  // 清空输出vector
  o_kvs.clear();

  // 查找目标Section
  Section *sec = find_section(section);
  if (sec == NULL) {
    return LBERR_OBJ_NOT_HAVE;
  }

  // 遍历map，复制所有键值对到vector
  for (const auto &kv : sec->keyValues) {
    ini_key_pair item;
    std::memset(&item, 0, sizeof(ini_key_pair));
    // 拷贝key（防止溢出）
    std::strncpy(item.key, kv.first.c_str(), sizeof(item.key) - 1);
    // 拷贝value（防止溢出）
    std::strncpy(item.value, kv.second.c_str(), sizeof(item.value) - 1);
    o_kvs.push_back(item);
  }

  return 0;
}

} // namespace lb_common

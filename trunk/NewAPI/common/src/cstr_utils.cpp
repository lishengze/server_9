/**
 * @file cstr_utils.cpp
 * @brief C字符串工具函数实现
 *
 * @see cstr_utils.h
 */

#include "cstr_utils.h"
#include <cctype>
#include <cstdio>
#include <cstring>

//#include  <iconv.h>

namespace lb_common {

int32 cstr_utils::split(std::vector<char *> &o_splits, char *line_buf, char split_c) {
  o_splits.clear();
  if (NULL == line_buf) {
    return 0;
  }
  int32 rn = 0;
  int32 have_flag = 0;
  char *flag_end = NULL;
  char *ts = NULL;
  //char *te = NULL;
  char *tp = line_buf;
  while ((*tp) != '\0' && (*tp) != '\n' && (*tp) != '\r') {
    if (NULL == ts) {
      //去除开头的空格
      // S8: std::isspace 入参要求 unsigned char 可表示值（0..255），
      // 直接传 char（高位为1时为负）是 UB。trim/lower 已 cast，这里补齐。
      if (std::isspace(static_cast<unsigned char>(*tp))) {
        (*tp) = '\0';
        tp++;
        rn++;
        continue;
      }
      ts = tp;
      have_flag = 0;
      flag_end = NULL;
      if (*ts == '"') {
        *ts = '\0';
        ts++;
        tp++;
        rn++;
        have_flag = 1;
      }
    } else {
      // 在字段内
      if (have_flag == 1) {
        // 引号内：分隔符不分割字段，只有右引号才关闭引号状态
        if (*tp == '"') {
          // 关闭引号：标记引号位置，后续字段结束时截断
          flag_end = tp;
          have_flag = 0;
          tp++;
          rn++;
          continue;
        }
        // 引号内其他字符：继续累积
        tp++;
        rn++;
      } else {
        // 非引号字段内
        if ((*tp) != split_c) {
          tp++;
          rn++;
        } else {
          // 字段结束
          (*tp) = '\0';
          if (flag_end != NULL) {
            *flag_end = '\0';
          }
          o_splits.push_back(ts);
          ts = NULL;
          flag_end = NULL;
          tp++;
          rn++;
        }
      }
    }
  }

  if ((*tp) == '\n' || (*tp) == '\r') {
    // 行终止符规则：
    //   - 标准 CRLF（\r\n，Windows 行尾）：消费 2 字节，作为 1 个终止符。
    //   - 单个 \n（Unix 行尾）或 单个 \r（经典 Mac）：消费 1 字节，作为 1 个终止符。
    //   - 其他连续序列（\n\n、\r\r、\n\r）：仅消费首个字节，剩余字符留给下一次 split() 调用。
    //   这种规则避免了原代码中"任何连续的 \n/\r 都合并为 1 个终止符"导致的 \n\n 被
    //   误吞 2 字节、或 \r\n\r\n 只吞 1 个 \r\n 的不一致行为。
    char c1 = *tp;
    (*tp) = '\0';
    tp++;
    rn++;
    if (c1 == '\r' && *tp == '\n') {
      (*tp) = '\0';
      tp++;
      rn++;
    }
  }
  if ((*tp) == '\0') {
    rn++;
  }

  if (ts != NULL) {
    // 行尾结束：若最后一个字段是被引号包裹的，需截断尾部引号
    if (flag_end != NULL) {
      *flag_end = '\0';
    }
    o_splits.push_back(ts);
  }
  return rn;
}

char *cstr_utils::trim(char *str) {
  if (NULL == str || str[0] == '\0')
    return str;

  int32 len = std::strlen(str);
  while (len > 0 &&
         (std::isspace(static_cast<unsigned char>(str[len - 1])) || str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[--len] = '\0';
  }

  int32 start = 0;
  while (len > 0 && std::isspace(static_cast<unsigned char>(str[start]))) {
    str[start] = '\0';
    ++start;
    --len;
  }

  return &(str[start]);
  //if(start > 0){
  //	memmove(str, str + start, len - start + 1);
  //}
}

void cstr_utils::lower(char *str) {
  if (NULL == str || str[0] == '\0')
    return;

  // P2-03: std::tolower 接受 int 且要求输入为 unsigned char 可表示值（0..255）。
  // 直接传入 char（可能为负）触发 UB。先 cast 到 unsigned char 再扩展。
  for (int32 i = 0; str[i] != '\0'; ++i) {
    str[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
  }
}

/*
int32 cstr_utils::utf8_to_gb(const char *gb_name,char *dst,char *src,
	int32 dst_size,int32 src_size)
{
	iconv_t tcd;
	if((tcd = iconv_open(gb_name,"utf-8")) == reinterpret_cast<iconv_t>(-1))
		return LBERR_OBJ_OPEN_FAIL;

	std::memset(dst,0,dst_size);
	char **tsource = &src;
	char **tdest = &dst;
	size_t tsn = src_size;
	size_t tdn = dst_size;
	if (iconv(tcd,tsource,&tsn,tdest,&tdn) == static_cast<size_t>(-1)) {
		iconv_close(tcd);
		return LBERR_OBJ_WRITE_FAIL;
	}
	iconv_close(tcd);
	return 0;
}

int32 cstr_utils::gb_to_utf8(const char *gb_name,char *dst,char *src,
	int32 dst_size,int32 src_size)
{
	iconv_t tcd;
	if((tcd = iconv_open("utf-8",gb_name)) == reinterpret_cast<iconv_t>(-1))
		return LBERR_OBJ_OPEN_FAIL;

	std::memset(dst,0,dst_size);
	char **tsource = &src;
	char **tdest = &dst;
	size_t tsn = src_size;
	size_t tdn = dst_size;
	if (iconv(tcd,tsource,&tsn,tdest,&tdn) == static_cast<size_t>(-1)) {
		iconv_close(tcd);
		return LBERR_OBJ_WRITE_FAIL;
	}
	iconv_close(tcd);
	return 0;
}
*/

} // namespace lb_common

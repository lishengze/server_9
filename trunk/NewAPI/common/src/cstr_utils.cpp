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
      if (std::isspace(*tp)) {
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
    } else if ((*tp) != split_c) {
      if (have_flag == 1 && (*tp) == '"') {
        flag_end = tp;
      }
      tp++;
      rn++;
    } else {
      (*tp) = '\0';
      if (have_flag == 1 && flag_end != NULL) {
        *flag_end = '\0';
      }
      o_splits.push_back(ts);
      ts = NULL;
      have_flag = 0;
      flag_end = NULL;
      tp++;
      rn++;
    }
  }

  if ((*tp) == '\n' || (*tp) == '\r') {
    (*tp) = '\0';
    tp++;
    rn++;
    if ((*tp) == '\n' || (*tp) == '\r') {
      (*tp) = '\0';
      tp++;
      rn++;
    }
  }
  if ((*tp) == '\0') {
    rn++;
  }

  if (ts != NULL) {
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

  for (int32 i = 0; str[i] != '\0'; ++i) {
    str[i] = std::tolower(str[i]);
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

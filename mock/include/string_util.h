//
// Created by zhouxingbao on 2020/12/2.
//

#ifndef TEST_MMAP_STRING_UTIL_H
#define TEST_MMAP_STRING_UTIL_H

#include <string>
#include <vector>
#include <string.h>
#include <sstream>
#include <stdint.h>
#include <cstring>
#include <algorithm>
#include <functional>

const static std::string G_SEP(1, '\1');

template <size_t Size>
std::string AsString(const std::array<char, Size>& from)
{
    return std::string(&from[0], Size);
}

static void SplitStdString(const std::string& s, const std::string& delim, std::vector<std::string>& sv) {
    sv.clear();
    std::istringstream iss(s);
    std::string temp;

    while (std::getline(iss, temp, delim[0])) {
        sv.push_back((temp));
    }
    return;
}


template <class T>
std::string join(T& val, std::string delim)
{
    std::string str;
    typename T::iterator it;
    const typename T::iterator itlast = val.end()-1;
    for (it = val.begin(); it != val.end(); it++)
    {
        str += *it;
        if (it != itlast)
        {
            str += delim;
        }
    }
    return str;
}
// trim from start
inline void LTrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end
inline void RTrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends
inline std::string &Trim(std::string &s) {
    LTrim(s);
    RTrim(s);
}

template <typename T>
void ToValue(const std::string& str, T& val)
{
    std::stringstream ss;
    ss << str;
    ss >> val;
}
template <size_t Size>
inline void CopyToArray(const std::string& from,
                        std::array<char, Size>& to)
{
    size_t min = std::min(from.size(), Size);
    std::memcpy(&to[0], from.c_str(), min);
    if (min < Size)
    {
        std::memset(&to[min], ' ', Size - min);
    }
}

template <size_t Size>
inline void CopyToArray(const char* buf,
						size_t buf_len,
						std::array<char, Size>& to)
{
	size_t min = std::min(buf_len, Size);
	if (min <= 0)
	{
		std::memset(&to[0], ' ', Size);
	}
	else
	{
		std::memcpy(&to[0], buf, min);
		if (min < Size)
		{
			std::memset(&to[min], ' ', Size - min);
		}
		
		if (buf[min - 1] == 0)
		{
			to[min - 1] = ' ';
		}
	}
}
#endif //TEST_MMAP_STRING_UTIL_H

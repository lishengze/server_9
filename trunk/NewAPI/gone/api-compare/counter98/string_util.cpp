/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
/**
* @file       string_util.h
* @author     liujie
* @date       20170210
*/

#include <stdint.h>
#include <string>
#include <cstring>
#include "string_util.h"
//#include <boost/array.hpp>
// #include <boost/endian/conversion.hpp>  // 对应的 string_util.h 中已引入
#include "mp_export.h"
#include <exception>
#include <math.h>
namespace mp
{

#ifndef GW_API_NO_BOOST



	MP_EXPORT uint64_t NetworkToHost(uint64_t host64)
	{
		return boost::endian::big_to_native(host64);
	}
	
	MP_EXPORT uint32_t NetworkToHost(uint32_t host32)
	{
		return boost::endian::big_to_native(host32);
	}
	
	MP_EXPORT uint16_t NetworkToHost(uint16_t host16)
	{
		return boost::endian::big_to_native(host16);
	}
	
	MP_EXPORT uint64_t NetworkToHost(int64_t host64)
	{
		return boost::endian::big_to_native(host64);
	}
	
	MP_EXPORT uint32_t NetworkToHost(int32_t host32)
	{
		return boost::endian::big_to_native(host32);
	}
	
	MP_EXPORT uint16_t NetworkToHost(int16_t host16)
	{
		return boost::endian::big_to_native(host16);
	}
	
	MP_EXPORT uint64_t HostToNetwork(uint64_t host64)
	{
		return boost::endian::native_to_big(host64);
	}
	
	MP_EXPORT uint32_t HostToNetwork(uint32_t host32)
	{
		return boost::endian::native_to_big(host32);
	}
	
	MP_EXPORT uint16_t HostToNetwork(uint16_t host16)
	{
		return boost::endian::native_to_big(host16);
	}
	
	MP_EXPORT uint64_t HostToNetwork(int64_t host64)
	{
		return boost::endian::native_to_big(host64);
	}
	
	MP_EXPORT uint32_t HostToNetwork(int32_t host32)
	{
		return boost::endian::native_to_big(host32);
	}
	
	
	MP_EXPORT uint16_t HostToNetwork(int16_t host16)
	{
		return boost::endian::native_to_big(host16);
	}

#endif

class NumericConvertException : public std::exception
{
public:
    NumericConvertException() :
            message_("Decimal Convert Error:")
    {}
    NumericConvertException(const NumericConvertException& rhs)
    {
        message_ = rhs.message_;
    }
    NumericConvertException(const std::string& str) :
            message_(str)
    {}
    ~NumericConvertException() throw () {
    }
    virtual const char* what() const throw () {
        return message_.c_str();
    }
private:
    std::string message_;
};

std::string FormatNumericConvertMsg(const std::string& type, const std::string& field,  const std::string& value)
{
    return type + ":" + field + ":[" + value + "]";
}

static const int64_t MAX_I64 =  922337203685477580LL;
MP_EXPORT int64_t StringToInteger(const std::string& str, const size_t & n, const std::string& field)
{
    if (str.empty()) {
        throw NumericConvertException(FormatNumericConvertMsg("1", field, str));
    }

    if(n < 0) {
        throw NumericConvertException(FormatNumericConvertMsg("3", field, str));
    }
    const char *p = str.c_str();
    const char *q = p+str.size()-1;
    //left trim
    while (*p) {
        if (*p != ' ') {
            break;
        }
        p++;
    }
    //right trim
    while (q != p) {
        if (*q != ' ') {
            break;
        }
        q--;
    }
    q++;
    // first can't be point
    if ((*p) == '.') {
        throw NumericConvertException(FormatNumericConvertMsg("3", field, str));
    }
    // neg check;
    bool is_neg = false;
    if ((*p) == '-') {
        is_neg = true;
        p++;
    }
    // 提取整数部分;
    int64_t num = 0;
    while (p!=q) {
        if ((*p) >= '0' && (*p) <= '9') {
            num *= 10;
            num += ((*p) - '0');
            if(num >= MAX_I64) {
                throw NumericConvertException(FormatNumericConvertMsg("2", field, str));
            }
        }
        else if(*p == '.') {
            p++; //跳过小数点, 后续进行小数部分提取;
            break;
        }
        else {
            throw NumericConvertException(FormatNumericConvertMsg("3", field, str));
        }
        p++;
    }

    // 提取小数部分;
    size_t i = 0;
    while (i < n) {
        if (p != q) {
            if ((*p) >= '0' && (*p) <= '9') {
                num *= 10;
                num += (*p - '0');
            }
            else {
                throw NumericConvertException(FormatNumericConvertMsg("3", field, str));
            }
            p++;
        }
        else {
            num *= 10;
        }
        i++;
    }
    // check invalid char;
    while (p != q) {
        if ((*p) < '0' || (*p) > '9') {
            throw NumericConvertException(FormatNumericConvertMsg("3", field, str));
        }
        p++;
    }
    if (is_neg) {
        if (num == 0) {
            throw NumericConvertException(FormatNumericConvertMsg("3", field, str));
        }
        else {
            return -num;
        }
    }
    else {
        return num;
    }
}

//int64_t StringToInteger(const std::string& str, const size_t & n, const int64_t& default_value)
//{
//    return StringToInteger(str, n, default_value, true);
//}
//int64_t StringToInteger(const std::string& str, const size_t & n)
//{
//    return StringToInteger(str, n, 0, false);
//}

MP_EXPORT std::string IntegerToString(const int64_t& num, const size_t& n)
{
    if(n < 0) {
        throw NumericConvertException("invalid precision: " + std::to_string(n));
    }
    std::string str = std::to_string(num);
    size_t offset = 0;
    if (num >= 0) {
        if (n != 0) {
            if (str.size() <= n) {
                offset = n - str.size();
                std::string sub_str = "0.";
                sub_str.insert(2, offset, '0');
                str.insert(0, sub_str);
            }
            else{
                offset = str.size() - n;
                str.insert(offset, 1, '.');
            }
        }
    }
    else {
        if (n != 0) {
            if ((str.size()-1) <= n) {
                offset = n - str.size()+1;
                std::string sub_str = "0.";
                sub_str.insert(2, offset, '0');
                str.insert(1, sub_str);
            }
            else {
                offset = str.size() - n;
                str.insert(offset, 1, '.');
            }
        }
    }

    return str;
}

}

/*
template <size_t Size>
inline void CopyToArray(const std::string& from, 
						boost::array<char, Size>& to)
{
	size_t min = std::min(from.size(), Size);
	std::memcpy(&to[0], from.c_str(), min);
	if (min < Size)
	{
		std::memset(&to[min], ' ', Size - min);
	}
}

template <size_t Size>
inline std::string AsString(const boost::array<char, Size>& from)
{
	return std::string(&from[0], Size);
}

template <size_t Size>
inline void CopyToArray(const char* buf,
						size_t buf_len,
						boost::array<char, Size>& to)
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

template <size_t Size, int CArraySize>
inline void CopyToArray(const char (&c_array)[CArraySize],
						boost::array<char, Size>& target)
{
	CopyToArray(&c_array[0], CArraySize, target);
}

template <size_t Size>
inline std::string AsStringTrim(const boost::array<char, Size>& from)
{
	ssize_t i = Size;
	while (true)
	{
		if (from[--i] != ' ')
			break;
		
		if (i == 0)
			return std::string();
	}
	
	return std::string(&from[0], i + 1);
}
*/
/*
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
inline std::string AsString(const std::array<char, Size>& from)
{
	return std::string(&from[0], Size);
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

template <size_t Size, int CArraySize>
inline void CopyToArray(const char (&c_array)[CArraySize],
						std::array<char, Size>& target)
{
	CopyToArray(&c_array[0], CArraySize, target);
}

template <size_t Size>
inline std::string AsStringTrim(const std::array<char, Size>& from)
{
	ssize_t i = Size;
	while (true)
	{
		if (from[--i] != ' ')
			break;
		
		if (i == 0)
			return std::string();
	}
	
	return std::string(&from[0], i + 1);
}
*/
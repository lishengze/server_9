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


#ifndef MP_STRING_UTIL_H_
#define MP_STRING_UTIL_H_

#include <stdint.h>
#include <string>
#include <cstring>
//#include "array.h"    //该头文件可以删除了
//#include <boost/array.hpp>
#include "mp_export.h"
#include <array>

#define GW_API_NO_BOOST

#ifdef GW_API_NO_BOOST
    #include <algorithm>
    #include <cctype>
#else
    #include <boost/endian/conversion.hpp>
    #include <boost/algorithm/string.hpp>
#endif


namespace mp
{
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

inline std::string ChnUtf8SubStr(const std::string& chn_str)
{
    unsigned int end = 0;
    unsigned int size_of_last = 0;
    while(end <= chn_str.size())
    {
        size_of_last = ((unsigned int)chn_str[end] > 0x80) ? 3 : 1;
        end += size_of_last;
    }
    end -= size_of_last;
    return chn_str.substr(0, end);
}

template<typename NUMBER>
inline std::string AsStringTrim(const NUMBER& from) {
    return AsString(from);
}

template<typename NUMBER>
inline std::string AsString(const typename std::enable_if<std::is_arithmetic<NUMBER>::value>::type& from) {
    return std::to_string(from);
}

inline std::string AsString(const std::string& from) {
    return from;
}

#ifdef GW_API_NO_BOOST
    inline std::string AsStringTrim(const std::string& from) {
        auto last_non_space_it = std::find_if(from.rbegin(), from.rend(),
            [](char c) { return !std::isspace(static_cast<unsigned char>(c)); }
        );
    
        if (last_non_space_it == from.rend()) {
            return std::string();
        }
    
        auto last_keep_index = from.rend() - last_non_space_it - 1; // points to the actual last non-space char
        return from.substr(0, last_keep_index + 1); // length = last_keep_index + 1 
    }

    inline int8_t NetworkToHost(int8_t in)
    {
        return in;
    }		
	
    inline uint8_t NetworkToHost(uint8_t in)
    {
        return in;
    }	


    inline int16_t NetworkToHost(int16_t in)
    {
        return be16toh(in);
        // return static_cast<int16_t>(be16toh(static_cast<uint16_t>(in)));
    }
    
    inline uint16_t NetworkToHost(uint16_t in) 
    {
        return be16toh(in); //converts big-endian 16-bit to native(host) order
    }
    
    inline int32_t NetworkToHost(int32_t in)
    {
        return be32toh(in);
        // return static_cast<int32_t>(be32toh(static_cast<int32_t>(in)));
    }
    
    inline uint32_t NetworkToHost(uint32_t in) 
    {
        return be32toh(in);
    }
    
    inline int64_t NetworkToHost(int64_t in)
    {
        return be64toh(in);
        // return static_cast<int64_t>(be64toh(static_cast<uint64_t>(in)));
    }
    
    inline uint64_t NetworkToHost(uint64_t in) 
    {
        return be64toh(in);
    }

    // --- Native to Big Endian (Network Order) ---
    inline int8_t HostToNetwork(int8_t in)
    {
        return in;
    }		
	
    inline uint8_t HostToNetwork(uint8_t in)
    {
        return in;
    }		
	
    inline int16_t HostToNetwork(int16_t in)
    {
        return htobe16(in);
        // return static_cast<int16_t>(htobe16(static_cast<uint16_t>(in)));
    }
    
    inline uint16_t HostToNetwork(uint16_t in) 
    {
        return htobe16(in);
    }
    
    inline int32_t HostToNetwork(int32_t in)
    {
        return htobe32(in);
        // return static_cast<int32_t>(htobe32(static_cast<uint32_t>(in)));
    }
    
    inline uint32_t HostToNetwork(uint32_t in) 
    {
        return htobe32(in);
    }
    
    inline int64_t HostToNetwork(int64_t in)
    {
        return htobe64(in);
        // return static_cast<int64_t>(htobe64(static_cast<uint64_t>(in)));
    }
    
    inline uint64_t HostToNetwork(uint64_t in) 
    {
        return htobe64(in);
    }
    
/*     template<typename T>
    T NetworkToHost(T in)
    {
        return big_to_native(in);
    }
    
    template<typename T>
    T HostToNetwork(T in)
    {
        return native_to_big(in);
    } */

#else
    inline std::string AsStringTrim(const std::string& from) {
        return boost::trim_right_copy(from);
    }
    
    MP_EXPORT uint64_t HostToNetwork(uint64_t net64);

    MP_EXPORT uint32_t HostToNetwork(uint32_t net32);

    MP_EXPORT uint16_t HostToNetwork(uint16_t net16);

    inline uint8_t HostToNetwork(uint8_t net8)
    {
        return net8;
    }

    MP_EXPORT uint64_t HostToNetwork(int64_t net64);

    MP_EXPORT uint32_t HostToNetwork(int32_t net32);

    MP_EXPORT uint16_t HostToNetwork(int16_t net16) ;

    inline uint8_t HostToNetwork(int8_t net8)
    {
        return net8;
    }



    MP_EXPORT uint64_t NetworkToHost(uint64_t net64);

    MP_EXPORT uint32_t NetworkToHost(uint32_t net32);

    MP_EXPORT uint16_t NetworkToHost(uint16_t net16);

    inline uint8_t NetworkToHost(uint8_t net8)
    {
        return net8;
    }

    MP_EXPORT uint64_t NetworkToHost(int64_t net64);

    MP_EXPORT uint32_t NetworkToHost(int32_t net32);

    MP_EXPORT uint16_t NetworkToHost(int16_t net16) ;

    inline uint8_t NetworkToHost(int8_t net8)
    {
        return net8;
    }

#endif


MP_EXPORT int64_t StringToInteger(const std::string& str, const size_t& n, const std::string& field = "");
//int64_t StringToInteger(const std::string& str, const size_t& n, const int64_t& default_value);
MP_EXPORT std::string IntegerToString(const int64_t& num, const size_t& n = 0);
}

#endif // MP_STRING_UTIL_H_

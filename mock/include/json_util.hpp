#pragma once

#include "json.hpp"

#include "code_util.h"

#include <string>
#include <iostream>
#include <cstring>
#include <array>
#include <functional>
#include <algorithm>
#include <string>

using std::string;

// sometimes, the int/long/double type filed is string ,
#define GET_JSON_FLOAT_FIELD(js, field) (js[field].is_null() ? 0 : (js[field].is_string() ? atof(js[field].get<std::string>().c_str()) : js[field].get<double>()))
#define GET_JSON_NUM_FIELD(js, field) (js[field].is_null() ? 0 : (js[field].is_string() ? atol(js[field].get<std::string>().c_str()) : js[field].get<long>()))
#define GET_JSON_INT_FIELD(js, field) (js[field].is_null() ? 0 : (js[field].is_string() ? atol(js[field].get<std::string>().c_str()) : js[field].get<int>()))
#define GET_JSON_BOOL_FIELD(js, field) (js[field].is_null() ? 0 : (js[field].is_string() ? atol(js[field].get<std::string>().c_str()) : js[field].get<bool>()))
#define GET_JSON_STR_FIELD(js, field) js[field].is_null() ? "" : js[field].get<std::string>().c_str()

#define GET_JSON_FLOAT(js) (js.is_null() ? 0 : (js.is_string() ? atof(js.get<std::string>().c_str()) : js.get<double>()))
#define GET_JSON_NUM(js) (js.is_null() ? 0 : (js.is_string() ? atol(js.get<std::string>().c_str()) : js.get<long>()))
#define GET_JSON_INT(js) (js.is_null() ? 0 : (js.is_string() ? atol(js.get<std::string>().c_str()) : js.get<int>()))
#define GET_JSON_BOOL(js) (js.is_null() ? 0 : (js.is_string() ? atol(js.get<std::string>().c_str()) : js.get<bool>()))
#define GET_JSON_STR(js) js.is_null() ? "" : js.get<std::string>().c_str()

using njson = nlohmann::json;

template <size_t Size>
inline std::string StdArrayToStr( std::array<char, Size>& to)
{
	std::string s(to.data(), Size);
	// str = s;
	return s;
}


template <size_t Size>
inline void CopyToStdArray(const char* buf,
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

template <size_t Size>
inline void CopyToStdArray(std::string& str,
						std::array<char, Size>& to)
{
	CopyToStdArray<Size>(str.c_str(), str.size(), to);
}

class Error {
public:
    Error():m_iErrorCode{0}, m_sErrorMsg{""} {

    }

    Error(int iErrorCode, string sErrMsg):m_iErrorCode(iErrorCode), m_sErrorMsg{sErrMsg} {

    }

    Error Set(int iErrorCode, string sErrorMsg) {
        m_iErrorCode = iErrorCode;
        m_sErrorMsg = sErrorMsg;

        return *this;
    }

    string Str() {
        return string("err_code: ") + std::to_string(m_iErrorCode) + ", err_msg: " + m_sErrorMsg;
    }

    int     m_iErrorCode;
    string  m_sErrorMsg;

    bool IsFailed() {
        return m_iErrorCode > 0;
    }
};

inline Error GetJsonFromFileFull(njson& dstJsonData, string sFullFileName) {
    Error error;
    try
    {
        std::fstream file(sFullFileName.c_str());
        std::ifstream in_config(sFullFileName);
        std::string sOriContents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());
        string sUtf8Contents = GetUtf8String(sOriContents);
        dstJsonData = njson::parse(sUtf8Contents);
    }
    catch(const std::exception& e)
    {
        // sErrMsg = sFullFileName + " parse exception: " + e.what();
        error.Set(1,  sFullFileName + " parse exception: " + e.what());
        // std::cerr << sErrMsg << '\n';
    }
    return error;    
}

inline bool GetJsonFromFile(njson& dstJsonData, string sFullFileName) {
    try
    {
        std::fstream file(sFullFileName.c_str());
        std::ifstream in_config(sFullFileName);
        std::string sOriContents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());
        string sUtf8Contents = GetUtf8String(sOriContents);
        dstJsonData = njson::parse(sUtf8Contents);
        return true;
    }
    catch(const std::exception& e)
    {

        std::cerr << sFullFileName << " parse exception: " << e.what() << '\n';
    }
    return false;    
}

/// -------------- char;
inline bool GetValueFromJson(njson& jsonData, char& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (!jsonData.is_null() && jsonData.is_string()) {
            string sJsData = GET_JSON_STR(jsonData);
            if (sJsData.length() > 0) {
                dst = sJsData[0];
            } else {
                dst = 0;
            }
        }        
        else if (!jsonData.is_null() && jsonData.is_number_unsigned()) {
            dst = jsonData.get<unsigned int>();
            // if (dst < 48) {
            //     dst += 48;
            // }
        }             
        else if (!jsonData.is_null() && jsonData.is_number_integer()) {
            dst = jsonData.get<int>();
            // if (dst < 48) {
            //     dst += 48;
            // }            
        }
        else if (bAllowDefaultValue) {
            dst = 0;
        }   
        else {
            sErrMsg = metaInfo + " Need " + "[char]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg= metaInfo + " Need [char]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonCharField(njson& jsonData, string filedName, char& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {

    try
    {
        if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
            string sJsData = GET_JSON_STR_FIELD(jsonData, filedName);
            if (sJsData.length() > 0) {
                dst = sJsData[0];
            } else {
                dst = 0;
            }
        }        
        else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_unsigned()) {
            dst = jsonData[filedName].get<unsigned int>();
            // if (dst < 48) {
            //     dst += 48;
            // }
        }             
        else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
            dst = jsonData[filedName].get<int>();
            // if (dst < 48) {
            //     dst += 48;
            // }            
        }
        else if (bAllowDefaultValue) {
            dst = '-';
        }   
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[char]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg= metaInfo + " Need " + filedName + "[char]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, char& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {

    try
    {
        if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
            string sJsData = GET_JSON_STR_FIELD(jsonData, filedName);
            if (sJsData.length() > 0) {
                dst = sJsData[0];
            }
        }        
        else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_unsigned()) {
            dst = jsonData[filedName].get<unsigned int>();
            // if (dst < 48) {
            //     dst += 48;
            // }
        }             
        else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
            dst = jsonData[filedName].get<int>();
            // if (dst < 48) {
            //     dst += 48;
            // }            
        }
        else if (bAllowDefaultValue) {
            dst = '-';
        }   
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[char]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg= metaInfo + " Need " + filedName + "[char]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, char& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {

    try
    {
        if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
            string sJsData = GET_JSON_STR_FIELD(jsonData, filedName);
            if (sJsData.length() > 0) {
                dst = sJsData[0];
            }
        }        
        else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_unsigned()) {
            dst = jsonData[filedName].get<unsigned int>();
            // if (dst < 48) {
            //     dst += 48;
            // }
        }             
        else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
            dst = jsonData[filedName].get<int>();
            // if (dst < 48) {
            //     dst += 48;
            // }            
        }
        else if (bAllowDefaultValue) {
            dst = '-';
        }   
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[char]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg= metaInfo + " Need " + filedName + "[char]--[ERROR] " + e.what();
    }
    
    return false;
}

/// --------------- unsigned char
inline bool GetValueFromJson(njson& jsonData, unsigned char& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_string()) {
        string sJsData = GET_JSON_STR(jsonData);
        if (sJsData.length() > 0) {
            dst = sJsData[0];
        }
    }
    else if (!jsonData.is_null() && jsonData.is_number_unsigned()) {
        dst = jsonData.get<unsigned int>();
        // if (dst < 48) {
        //     dst += 48;
        // }
    }             
    else if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<int>();
        // if (dst < 48) {
        //     dst += 48;
        // }            
    }    
    else if (bAllowDefaultValue) {
        dst = '-';
    } 
    else {
        sErrMsg = metaInfo + " Need [unsigned char]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonUnsignedCharField(njson& jsonData, string filedName, unsigned char& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
        string sJsData = GET_JSON_STR_FIELD(jsonData, filedName);
        if (sJsData.length() > 0) {
            dst = sJsData[0];
        }
    }
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_unsigned()) {
        dst = jsonData[filedName].get<unsigned int>();
        // if (dst < 48) {
        //     dst += 48;
        // }
    }             
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<int>();
        // if (dst < 48) {
        //     dst += 48;
        // }            
    }    
    else if (bAllowDefaultValue) {
        dst = '-';
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned char]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonUnsignedCharField(njson& jsonData, string filedName, bool& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
        string sJsData = GET_JSON_STR_FIELD(jsonData, filedName);
        if (sJsData.length() > 0) {
            dst = sJsData[0];
        }
    }
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_unsigned()) {
        dst = jsonData[filedName].get<unsigned int>();
        // if (dst < 48) {
        //     dst += 48;
        // }
    }             
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<int>();
        // if (dst < 48) {
        //     dst += 48;
        // }            
    }    
    else if (bAllowDefaultValue) {
        dst = '-';
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned char]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned char& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
        string sJsData = GET_JSON_STR_FIELD(jsonData, filedName);
        if (sJsData.length() > 0) {
            dst = sJsData[0];
        }
    }
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_unsigned()) {
        dst = jsonData[filedName].get<unsigned int>();
        // if (dst < 48) {
        //     dst += 48;
        // }
    }             
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<int>();
        // if (dst < 48) {
        //     dst += 48;
        // }            
    }    
    else if (bAllowDefaultValue) {
        dst = '-';
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned char]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned char& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
        string sJsData = GET_JSON_STR_FIELD(jsonData, filedName);
        if (sJsData.length() > 0) {
            dst = sJsData[0];
        }
    }
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_unsigned()) {
        dst = jsonData[filedName].get<unsigned int>();
        // if (dst < 48) {
        //     dst += 48;
        // }
    }             
    else if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<int>();
        // if (dst < 48) {
        //     dst += 48;
        // }            
    }    
    else if (bAllowDefaultValue) {
        dst = '-';
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned char]--[ERROR]";
        return false;
    }
    return true;
}

///---------------- bool;
inline bool GetValueFromJson(njson& jsonData, bool& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (jsonData.is_number_integer()) {
       dst = (bool)(jsonData.get<int>());
    }      
    else if (bAllowDefaultValue) {
        dst = true;
    } 
    else {
        sErrMsg = metaInfo + " Need [bool]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonBoolField(njson& jsonData, string filedName, bool& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (jsonData[filedName].is_number_integer()) {
       dst = (bool)(jsonData[filedName].get<int>());
    }      
    else if (bAllowDefaultValue) {
        dst = true;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[bool]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, bool& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
       dst = (bool)(jsonData[filedName].get<int>());
    } 
    else if (bAllowDefaultValue) {
        dst = true;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[bool]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, bool& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
       dst = (bool)(jsonData[filedName].get<int>());
    } 
    else if (bAllowDefaultValue) {
        dst = true;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[bool]--[ERROR]";
        return false;
    }
    return true;
}


/// ---------------- string
inline bool GetValueFromJson(njson& jsonData, string& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_string()) {
        dst = GET_JSON_STR(jsonData);
        dst = GetGBKString(dst);
    } 
    else if (bAllowDefaultValue) {
        dst = "";
        dst.clear();
    } 
    else {
        sErrMsg = metaInfo + " Need [string]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonStringField(njson& jsonData, string filedName, string& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
        dst = GET_JSON_STR_FIELD(jsonData, filedName);
        dst = GetGBKString(dst);
    } 
    else if (bAllowDefaultValue) {
        dst = "";
        dst.clear();
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[string]--[ERROR]";
        return false;
    }
    return true;
}
template <size_t Size>
inline bool GetJsonArrayStringField(njson& jsonData, string filedName, std::array<char, Size>& dst, 
                                string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    std::string strDst;
    GetJsonStringField(jsonData, filedName, strDst, sErrMsg, metaInfo, bAllowDefaultValue);
    CopyToStdArray<Size>(strDst, dst);
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, string& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
        dst = GET_JSON_STR_FIELD(jsonData, filedName);
        dst = GetGBKString(dst);
    } 
    else if (bAllowDefaultValue) {
        dst = "-";
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[string]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, string& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_string()) {
        dst = GET_JSON_STR_FIELD(jsonData, filedName);
        dst = GetGBKString(dst);
    } 
    else if (bAllowDefaultValue) {
        dst = "-";
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[string]--[ERROR]";
        return false;
    }
    return true;
}

/// -----------  char*
inline bool GetValueFromJson(njson& jsonData, char* cDst, int size, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (jsonData.is_string() && size >= 1) {
        string tmp = GET_JSON_STR(jsonData);
        tmp = GetGBKString(tmp);
        snprintf(cDst, size, "%s", tmp.c_str());
        } 
        else if (bAllowDefaultValue) {
            if (size >= 1) {
                cDst[0] = '-';
            } else {
                sErrMsg = metaInfo +  " size: "+ std::to_string(size) + ", illegal--[ERROR]";
                return false;
            }        
        } 
        else {
            sErrMsg = metaInfo + " Need [string]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg =  metaInfo + " Need [string]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonCharStringField(njson& jsonData, string filedName, char* cDst, int size, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (jsonData[filedName].is_string() && size >= 1) {
        string tmp = GET_JSON_STR_FIELD(jsonData, filedName);
        tmp = GetGBKString(tmp);
        snprintf(cDst, size, "%s", tmp.c_str());
        } 
        else if (bAllowDefaultValue) {
            if (size >= 1) {
                cDst[0] = '-';
            } else {
                sErrMsg = metaInfo + filedName + " size: "+ std::to_string(size) + ", illegal--[ERROR]";
                return false;
            }        
        } 
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[string]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg =  metaInfo + " Need " + filedName + "[string]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, char* cDst, int size, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (jsonData[filedName].is_string() && size >= 1) {
        string tmp = GET_JSON_STR_FIELD(jsonData, filedName);
        // cout << filedName<<  " ori_tmp: "  << tmp << endl;
        tmp = GetGBKString(tmp);
        // cout << filedName<<  " gbk_tmp: "  << tmp << endl;
        snprintf(cDst, size, "%s", tmp.c_str());
        } 
        else if (bAllowDefaultValue) {
            if (size >= 1) {
                cDst[0] = '-';
            } else {
                sErrMsg = metaInfo + filedName + " size: "+ std::to_string(size) + ", illegal--[ERROR]";
                return false;
            }        
        } 
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[string]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg =  metaInfo + " Need " + filedName + "[string]--[ERROR] " + e.what();
    }
    
    return false;
}

/// ------------------ int
inline bool GetValueFromJson(njson& jsonData, int& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<int>();
    } 
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need [int]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonIntField(njson& jsonData, string filedName, int& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<int>();
    } 
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[int]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonShortField(njson& jsonData, short& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<int>();
    } 
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need [int]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, int& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<int>();
    } 
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[int]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, int& dst,  int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<int>();
    } 
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[int]--[ERROR]";
        return false;
    }
    return true;
}


/// -------------- unsigned int
inline bool GetValueFromJson(njson& jsonData, unsigned int& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<unsigned int>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need  [unsigned int]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonUnsignedIntField(njson& jsonData, string filedName, unsigned int& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned int>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned int]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned int& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned int>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned int]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned int& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned int>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned int]--[ERROR]";
        return false;
    }
    return true;
}

///--------------- short
inline bool GetValueFromJson(njson& jsonData, short& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<short>();
    } else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need [short]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonShortField(njson& jsonData, string filedName, short& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<short>();
    } else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[short]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, short& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<short>();
    } else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[short]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, short& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<short>();
    } else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[short]--[ERROR]";
        return false;
    }
    return true;
}

///------------ unsigned short
inline bool GetValueFromJson(njson& jsonData, unsigned short& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<unsigned short>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need [unsigned short]--[ERROR]";
        return false;
    }
    return true;
}


inline bool GetJsonUnsignedShortField(njson& jsonData, string filedName, unsigned short& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned short>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned short]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned short& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned short>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned short]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned short& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned short>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned short]--[ERROR]";
        return false;
    }
    return true;
}

/// ------------------- long
inline bool GetValueFromJson(njson& jsonData,  long long& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need [long long]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonLongLongField(njson& jsonData, string filedName, long long& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[long long]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, long long& dst, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[long long]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, long long& dst, int size/*无效只为接口统一*/, string& sErrMsg, string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[long long]--[ERROR]";
        return false;
    }
    return true;
}


/// ------------------- unsigned long
inline bool GetValueFromJson(njson& jsonData,  unsigned long long& dst, string& sErrMsg, const string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData.is_null() && jsonData.is_number_integer()) {
        dst = jsonData.get<unsigned long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need [unsigned long long]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonUnsignedLongLongField(njson& jsonData, string filedName, unsigned long long& dst, string& sErrMsg, const string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned long long]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned long long& dst, string& sErrMsg, const string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned long long]--[ERROR]";
        return false;
    }
    return true;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, unsigned long long& dst, int size/*无效只为接口统一*/,string& sErrMsg, const string metaInfo = "", bool bAllowDefaultValue=true) {
    if (!jsonData[filedName].is_null() && jsonData[filedName].is_number_integer()) {
        dst = jsonData[filedName].get<unsigned long long>();
    }
    else if (bAllowDefaultValue) {
        dst = 0;
    } 
    else {
        sErrMsg = metaInfo + " Need " + filedName + "[unsigned long long]--[ERROR]";
        return false;
    }
    return true;
}

/// ----------------- double
inline bool GetValueFromJson(njson& jsonData, double& dst, string& sErrMsg, const string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (jsonData.is_number_integer() ){
            dst = jsonData.get<long long>();
        } else if (jsonData.is_number()) {
            dst = jsonData.get<double>();
        }
        else if (bAllowDefaultValue) {
            dst = 0;
        } 
        else {
            sErrMsg = metaInfo + " Need  [double]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg = metaInfo + " Need  [double]--[ERROR] " + e.what();
    }
    
    return false;
}


inline bool GetJsonDoubleField(njson& jsonData, string filedName, double& dst, string& sErrMsg, const string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (jsonData[filedName].is_number_integer() ){
            dst = jsonData[filedName].get<long long>();
        } else if (jsonData[filedName].is_number()) {
            dst = jsonData[filedName].get<double>();
        }
        else if (bAllowDefaultValue) {
            dst = 0;
        } 
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[double]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg = metaInfo + " Need " + filedName + "[double]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, double& dst, string& sErrMsg, const string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (!jsonData[filedName].is_null() ){
            dst = jsonData[filedName].get<double>();
        }
        else if (bAllowDefaultValue) {
            dst = 0;
        } 
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[double]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg = metaInfo + " Need " + filedName + "[double]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonDataField(njson& jsonData, string filedName, double& dst, int size/*无效只为接口统一*/,string& sErrMsg, const string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (!jsonData[filedName].is_null() ){
            dst = jsonData[filedName].get<double>();
        }
        else if (bAllowDefaultValue) {
            dst = 0;
        } 
        else {
            sErrMsg = metaInfo + " Need " + filedName + "[double]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg = metaInfo + " Need " + filedName + "[double]--[ERROR] " + e.what();
    }
    
    return false;
}

/// ----------------- float
inline bool GetValueFromJson(njson& jsonData, float& dst, string& sErrMsg, const string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (jsonData.is_number_integer() ){
            dst = jsonData.get<long long>();
        } else if (jsonData.is_number()) {
            dst = jsonData.get<float>();
        }
        else if (bAllowDefaultValue) {
            dst = 0;
        } 
        else {
            sErrMsg = metaInfo + " Need  [float]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg = metaInfo + " Need  [float]--[ERROR] " + e.what();
    }
    
    return false;
}

inline bool GetJsonSimpleStringField(njson& jsonData, string& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (!jsonData.is_null() && jsonData.is_string()) {
            dst = jsonData.get<std::string>();
        } 
        else if (bAllowDefaultValue) {
            dst = "-";
        } 
        else {
            sErrMsg = metaInfo + " Need [string]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg= metaInfo + " Need [SimpleString]--[ERROR] " + e.what();
        std::cerr << e.what() << '\n';
    }
    
    return true;
}


inline bool GetJsonDataField(njson& jsonData, string& dst, string& sErrMsg, string metaInfo="", bool bAllowDefaultValue=true) {
    try
    {
        if (!jsonData.is_null() && jsonData.is_string()) {
            dst = jsonData.get<std::string>();
        } 
        else if (bAllowDefaultValue) {
            dst = "-";
        } 
        else {
            sErrMsg = metaInfo + " Need [string]--[ERROR]";
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        sErrMsg= metaInfo + " Need [SimpleString]--[ERROR] " + e.what();
        std::cerr << e.what() << '\n';
    }
    
    return true;
}

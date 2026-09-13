#pragma once

#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <sstream>
#include <chrono>
#include <map>
#include <set>
#include <vector>
#include <cstring>

#include "json.hpp"

using std::string;
using std::cout;
using std::endl;
using std::fstream;
using std::map;
using std::set;
using std::vector;

inline bool isGBK(const char* str, int length)
{
    unsigned int nBytes = 0;//GBK??1-2?????,???? ,????
    unsigned char chr = *str;
    bool bAllAscii = true; //??????ASCII,
    for (unsigned int i = 0; i < length && str[i] != '\0'; ++i){
        chr = *(str + i);
        if ((chr & 0x80) != 0 && nBytes == 0){// ????ASCII??,????,??????GBK
        bAllAscii = false;
        }
        if (nBytes == 0) {
            if (chr >= 0x80) {
                if (chr >= 0x81 && chr <= 0xFE){
                    nBytes = +2;
                }
                else{
                    return false;
                }
                nBytes--;
            }
        }
        else{
            if (chr < 0x40 || chr>0xFE){
            return false;
            }
            nBytes--;
        }//else end
    }
    if (nBytes != 0) {   //????
    return false;
    }
    if (bAllAscii){ //??????ASCII, ??GBK
    return true;
    }
    return true;
}

inline bool isGBK(string& sSrc) {
    return isGBK(sSrc.c_str(), sSrc.length());
}


#if defined(_WIN32) || defined(_WIN64)

inline string GbkToUtf8(const char *src_str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    string strTemp = str;
    if (wstr) delete[] wstr;
    if (str) delete[] str;
    return strTemp;
} 

inline string Utf8ToGbk(const char *src_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
    wchar_t* wszGBK = new wchar_t[len + 1];
    memset(wszGBK, 0, len * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    char* szGBK = new char[len + 1];
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
    string strTemp(szGBK);
    if (wszGBK) delete[] wszGBK;
    if (szGBK) delete[] szGBK;
    return strTemp;
}
#else
#include <iconv.h>

inline int GbkToUtf8( const char *src_str, size_t src_len, char *dst_str, size_t dst_len)
{
    iconv_t cd = iconv_open("utf8", "gbk");
    // char** pin = &src_str;
    // char** pout = &dst_str;

    if (cd == 0) {
        return 1;
    }
        
    memset(dst_str, 0, dst_len);
    if (iconv(cd, (char**)&src_str, &src_len, (char**)&dst_str, &dst_len) == -1) {
        return 2;
    }

    iconv_close(cd);

    return 0;
}


inline int Utf8ToGbk(const char *src_str, size_t src_len, char *dst_str, size_t dst_len)
{
    iconv_t cd = iconv_open("gbk", "utf8");
    if (cd == 0)
        return 1;
    memset(dst_str, 0, dst_len);
    if (iconv(cd, (char**)&src_str, &src_len, (char**)&dst_str, &dst_len) == -1)
        return 2;
    iconv_close(cd);

    return 0;
}
#endif


inline bool IsJsonAcceptUtf8(const string& sOriStr) {
    try
    {
        nlohmann::json tmpJson;
        tmpJson["test"] = sOriStr;
        tmpJson.dump(2);

        return true;
    }
    catch(const std::exception& e)
    {
        // cout <<"[FAILED] Json Dump " << sOriStr << "\n[Exception]: "<< e.what() << "\n" << endl;
        return false;
    }
    
    return false;
}

inline string GetUtf8String(const char* sSrc, int iSrcLen) {
    string result(sSrc);

    if (isGBK(sSrc, iSrcLen)) {
        int iDstLen = iSrcLen * 4;
        char* sDst = new char[iDstLen];
        memset(sDst, 0, iDstLen);

        int flag = GbkToUtf8(sSrc, iSrcLen, sDst, iDstLen);
        if (0 == flag) {
            result =  sDst;
        } 
        delete []sDst;

    } else {
        // cout << "Char[] sSrc: " << sSrc << " is UTF8" << endl;
    }

    if (!IsJsonAcceptUtf8(result)) {
        result = "";
    }

    return result;
}

inline double GetUtf8String(double& sSrc, int iSrcLen) {
    return sSrc;
}

inline string GetUtf8String(string sSrc) {
    return  GetUtf8String(sSrc.c_str(), sSrc.length());;
}

inline bool isUtf8(const char* str, int length)
{
    unsigned int nBytes = 0;//UFT8??1-6?????,ASCII?????
    unsigned char chr = *str;
    bool bAllAscii = true;
    for (unsigned int i = 0; i < length && str[i] != '\0'; ++i){
        chr = *(str + i);
        //????ASCII??,????,??????UTF8,ASCII?7???,??????0,0xxxxxxx
        if (nBytes == 0 && (chr & 0x80) != 0){
            bAllAscii = false;
        }
        if (nBytes == 0) {
            //????ASCII?,???????,?????
            if (chr >= 0x80) {
                if (chr >= 0xFC && chr <= 0xFD){
                    nBytes = 6;
                }
                else if (chr >= 0xF8){
                    nBytes = 5;
                }
                else if (chr >= 0xF0){
                    nBytes = 4;
                }
                else if (chr >= 0xE0){
                    nBytes = 3;
                }
                else if (chr >= 0xC0){
                    nBytes = 2;
                }
                else{
                    return false;
                }
                    nBytes--;
                }
            }
            else{
                //?????????,?? 10xxxxxx
                if ((chr & 0xC0) != 0x80){
                    return false;
                }
                //??????
                nBytes--;
            }
    }
    //??UTF8????
    if (nBytes != 0) {
        return false;
    }
    if (bAllAscii){ //??????ASCII, ??UTF8
        return true;
    }
    return true;
}

inline bool isUtf8(string& sSrc) {
    return isUtf8(sSrc.c_str(), sSrc.length());
}

inline string GetGBKString(const char* sSrc, int iSrcLen) {
    string result(sSrc);

    if (iSrcLen == 0) return result;

    if (isUtf8(sSrc, iSrcLen)) {

        int iDstLen = iSrcLen*4;
        char* sDst = new char[iDstLen];
        memset(sDst, 0, iDstLen);

        int flag = Utf8ToGbk(sSrc, iSrcLen, sDst, iDstLen);
        if (0 == flag) {
            result = sDst;
        } else {
            // cout << "[FAILED] sSrcCpy: " << sSrcCpy << ", Len: " << len  << ", flag: " << flag << endl;
            result = "";     
        }
        delete []sDst;

    } else {
        // cout << "Char[] sSrc: " << sSrc << " is UTF8" << endl;
    }

    return result;
}

inline string GetGBKString(string sSrc) {
    if (sSrc.length() == 0) return sSrc;
    return  GetGBKString(sSrc.c_str(), sSrc.length());
}


// 功能：把单个 char 转换成等价的 string
inline std::string charToString(char c) {
    // 返回只包含这个字符的字符串
    return std::string(1, c);
}
/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
/**
 * @brief mp dll导出宏定义
 * @author liujie
 */
#ifdef _MSC_VER
# pragma once
#endif
#ifndef MP_EXPORT_H
#define MP_EXPORT_H
#	if defined (_WIN32)
#		if defined (MP_EXPORT_DLL)
#		  define MP_EXPORT __declspec(dllexport)
#		else
#		  define MP_EXPORT __declspec(dllimport)
#		endif
#	else
#   	define MP_EXPORT __attribute__((visibility("default")))
#	endif
#endif /* MP_EXPORT_H */


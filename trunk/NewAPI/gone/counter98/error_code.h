/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
#ifndef MP_ERROR_CODE_H_
#define MP_ERROR_CODE_H_

#include <string>

namespace mp
{

//typedef int32_t ErrorCode_def;

/**
 * @brief error code
 */
enum ErrorCode
{
    kSuccess = 0,
    kFailure,
    kQueueEmpty,
    kMemPoolIndexError,
    kMemPoolDoubleDelete,
    kWouldblock,
    kInvalidParameters,
    kQueueFull,
    kInvalidInvoke,
    kCreateLockFileDirFaild,
    kOutOfRange,
    kNoMemory,
    kDuplicatedOption,
    kInvalidOptionName,
	kNotReadAll,
    kMessageIsShort
};

/**
 * @brief      Gets the error description.
 *
 * @param[in]  ec    error code value
 *
 * @return     The error description.
 */
const std::string& GetErrorDesc(ErrorCode ec);
} // adk

#define MP_CHECK_RET_SUCCESS(result)\
    do\
    {\
        int32_t local_result = (result);\
        if (MP_UNLIKELY(kSuccess != local_result))\
            return local_result;\
    }while(false)

#define MP_CHECK_RET_SUCCESS_WITH_EXCEPTION_PRO(result, exception_pro)\
    do\
    {\
        int32_t local_result = (result);\
        if (MP_UNLIKELY(kSuccess != local_result))\
		{\
			exception_pro;\
            return local_result;\
		}\
    }while(false)

#endif // MP_ERROR_CODE_H_

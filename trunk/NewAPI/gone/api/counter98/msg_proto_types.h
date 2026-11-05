/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
#ifndef MSG_PROTO_TYPES_H_
#define MSG_PROTO_TYPES_H_

#include <array>
#include <stdint.h>

// mp为message proto简称，消息协议
namespace mp
{
typedef uint32_t MsgTypeNo_def; ///< 消息编码
typedef uint32_t Length_def; ///< 长度,表示字节为单位的数据长度
typedef int64_t EpochTime_def; ///< 相对时间,从1970-01-01 00：00：00.000开始所经过的毫秒数
typedef std::array<char, 50> BusinessRejectText_def; ///< 拒绝原因说明
typedef uint32_t NumInGroup_def; ///< 重复组数量
typedef int32_t ErrorCode_def;
typedef uint32_t MsgVer_def ; ///< 消息版本号
}

#endif // MSG_PROTO_TYPES_H_

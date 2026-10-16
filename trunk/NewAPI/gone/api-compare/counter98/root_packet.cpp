/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
/**
* @file       root_packet.h
* @author     liujie
* @date       20170210
*/
#include <stdint.h>
#include <string>
#include "error_code.h"
#include "message_decoder.h"
#include "message_encoder.h"
#include "msg_proto_types.h"
#include <sstream>
#include "root_packet.h"

namespace mp
{
	RootPacket::~RootPacket(){}
	ErrorCode RootPacket::PushFields(MessageEncoder& encoder) const
	{
		return Encode(encoder);
	}

	ErrorCode RootPacket::PushFields(SerializeBuffer& buffer, bool host_to_network) const
	{
		MessageEncoder encoder(buffer, false, host_to_network);
		return PushFields(encoder);
	}

	ErrorCode RootPacket::PopFields(MessageDecoder& decoder, bool allow_msg_short)
	{
        ErrorCode code = Decode(decoder);
        if (code == mp::kSuccess)
        {
            return mp::kSuccess;
        }
        if (allow_msg_short && code == mp::kMessageIsShort)
        {
            return mp::kSuccess;
        }
		return code;
	}

	ErrorCode RootPacket::PopFields(const SerializeBuffer& data, bool host_to_network, bool allow_msg_short)
	{
		if (data.Empty())
		{
			return mp::kSuccess;
		}
		return PopFields(&data[0], data.Size(), host_to_network, allow_msg_short);
	}

	ErrorCode RootPacket::PopFields(const char* data, size_t size, bool host_to_network, bool allow_msg_short)
	{
		MessageDecoder decoder(data, size, host_to_network);
		ErrorCode result = Decode(decoder);
		if (result == mp::kSuccess)
		{
			return result;
		}
		/* 有剩余字段可以不处理，兼容委托增加字段
		if (decoder.left_bytes() > 0)
		{
			return mp::kNotReadAll;
		}
		*/
        if (allow_msg_short && result == mp::kMessageIsShort)
        {
            return mp::kSuccess;
        }

		return result;
	}

	void RootPacket::DumpAsString(std::ostream& os) const
	{
	}

    void RootPacket::FillDefaultValue()
    {
    }

	std::string RootPacket::ToString()
	{
		std::stringstream ss;
		this->DumpAsString(ss);
		return ss.str();
	}
}


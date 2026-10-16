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


#ifndef MP_ROOT_PACKET_H_
#define MP_ROOT_PACKET_H_

#include <stdint.h>
#include <string>


#include "error_code.h"
#include "message_decoder.h"
#include "message_encoder.h"
#include "msg_proto_types.h"
#include "mp_export.h"

#include <sstream>
namespace mp
{
template <typename ListType>
inline size_t GetSequenceSize(const ListType& list_data)
{
	size_t size = 0;
	typename ListType::const_iterator element = list_data.begin();
	for (; element != list_data.end(); ++element)
	{
		size += element->GetSize();
	}
	return size;
}

 class MessageDecoder;
 class MessageEncoder;
 class SerializeBuffer;

class MP_EXPORT RootPacket
{
public:
	virtual ~RootPacket(); 

	virtual size_t GetSize() const = 0;

	ErrorCode PushFields(MessageEncoder& encoder) const;

	ErrorCode PushFields(SerializeBuffer& buffer, bool host_to_network = false) const;

	ErrorCode PopFields(MessageDecoder& decoder, bool allow_msg_short = true);

	ErrorCode PopFields(const SerializeBuffer& data, bool host_to_network = false, bool allow_msg_short = true);

	ErrorCode PopFields(const char* data, size_t size, bool host_to_network = false, bool allow_msg_short = true);

	virtual void DumpAsString(std::ostream& os) const;

  	virtual void FillDefaultValue();

	uint32_t GetMessagelen(){return message_len;}

	void SetMessagelen(uint32_t len){message_len = len;}

	std::string ToString();

	virtual mp::MsgTypeNo_def GetTypeID() const { return 0; }

	virtual mp::MsgVer_def GetMsgVer() const { return 0 ;}
    virtual int64_t GetAgwSeqID() const {return 0 ;} 

protected:
	virtual ErrorCode Decode(MessageDecoder& decoder) = 0;
	virtual ErrorCode Encode(MessageEncoder& encoder) const = 0;
private:
	uint32_t message_len;
};
}

#endif // MP_ROOT_PACKET_H_

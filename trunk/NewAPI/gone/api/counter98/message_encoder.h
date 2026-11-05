/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
/**
* @file       message_encoder.h
* @author     liujie
* @date       20170210
*/

#ifndef MP_STREAM_WRITER_H_
#define MP_STREAM_WRITER_H_

#include <string>
#include <vector>
#include <algorithm>
#include <type_traits>

#ifdef __linux
#ifdef USE_AMI_ZERO_COPY
#include <ami/message.h>
#endif
#endif

#include "serialize_buffer.h"
#include "string_util.h"
#include <array>
namespace mp
{

class MessageEncoder
{
public:
#ifndef USE_AMI_ZERO_COPY
    explicit MessageEncoder(SerializeBuffer& buffer,
                            bool clean_before_use = true,
                            bool host_to_network = false)
        : buffer_(buffer),
          host_to_network_(host_to_network)
    {
        if (clean_before_use)
        {
            buffer_.Clear();
        }
    }

    inline void DoFieldPushImpl(const char* begin, size_t size)
    {
        buffer_.Append(begin, size);
    }

    inline void DoFieldPushImpl(const unsigned char* begin, size_t size)
    {
        buffer_.Append(reinterpret_cast<const char*>(begin), size);
    }

#else
#ifdef __linux 
    
    explicit MessageEncoder(SerializeBuffer& buffer,
                            bool clean_before_use = true,
                            bool host_to_network = false)
        : ami_buffer_(NULL),
          buffer_(buffer),
          host_to_network_(host_to_network)
    {
        if (clean_before_use)
        {
            buffer_.Clear();
        }
    }

    //追加字节流
    inline void DoFieldPushImpl(const char* begin, size_t size)
    {
        if (ami_buffer_)
        {
            ami_buffer_->append(reinterpret_cast<const void*>(begin), size);
            return;
        }

        buffer_.Append(begin, size);
    }

    inline void DoFieldPushImpl(const unsigned char* begin, size_t size)
    {
        if (ami_buffer_)
        {
            ami_buffer_->append(reinterpret_cast<const void*>(begin), size);
            return;
        }

        buffer_.Append(reinterpret_cast<const char*>(begin), size);
    }

    explicit MessageEncoder(ami::Message* ami_buffer,
                            SerializeBuffer& buffer,
                            bool clean_before_use = true,
                            bool host_to_network = false)
        : ami_buffer_(ami_buffer),
          buffer_(buffer),
          host_to_network_(host_to_network)
    {
    }
    ami::Message* ami_buffer_;
#endif
#endif

    inline void PushFieldChar(char data)
    {
        DoIntegerFieldPushImpl(data);
    }

    inline void PushFieldInt8(int8_t data)
    {
        DoIntegerFieldPushImpl(data);
    }

    inline void PushFieldUInt8(uint8_t data)
    {
        DoIntegerFieldPushImpl(data);
    }

    inline void PushFieldInt16(int16_t data)
    {
        DoIntegerFieldPush(data);
    }

    inline void PushFieldUInt16(uint16_t data)
    {
        DoIntegerFieldPush(data);
    }

    inline void PushFieldInt32(int32_t data)
    {
        DoIntegerFieldPush(data);
    }

    inline void PushFieldUInt32(uint32_t data)
    {
        DoIntegerFieldPush(data);
    }

    inline void PushFieldInt64(int64_t data)
    {
        DoIntegerFieldPush(data);
    }

    inline void PushFieldUInt64(uint64_t data)
    {
        DoIntegerFieldPush(data);
    }

    inline void PushField(char data)
    {
        PushFieldChar(data);
    }

    inline void PushField(int8_t data)
    {
        PushFieldInt8(data);
    }

    inline void PushField(uint8_t data)
    {
        PushFieldUInt8(data);
    }

    inline void PushField(int16_t data)
    {
        PushFieldInt16(data);
    }

    inline void PushField(uint16_t data)
    {
        PushFieldUInt16(data);
    }

    inline void PushField(int32_t data)
    {
        PushFieldInt32(data);
    }

    inline void PushField(uint32_t data)
    {
        PushFieldUInt32(data);
    }

    inline void PushField(int64_t data)
    {
        PushFieldInt64(data);
    }

    inline void PushField(uint64_t data)
    {
        PushFieldUInt64(data);
    }

    template <class T>
    inline void PushField(const T* data, size_t size)
    {
        DoFieldPush(data, size);
    }

    template <class T, size_t size>
    inline void PushField(const T (&data)[size])
    {
        PushField(data, size);
    }
    template<class T,size_t size>
    inline void PushField(const std::array<T,size>&data)
    {
        if(size > 0)
        {
            PushField(&data[0],size);
        }
    }
    
    template <class T, class A>
    inline void PushField(const std::vector<T, A>& data)
    {
        if (!data.empty())
        {
            PushField(&data[0], data.size());
        }
    }

    inline void PushField(const std::string& str)
    {
        DoFieldPush(str.c_str(), str.size());
    }

private:
    template <typename IntegerType>
    inline void DoFieldPush(const IntegerType* begin, size_t size)
    {
        if (std::is_integral<IntegerType>::value && (sizeof(IntegerType) > 1))
        {
            DoIntegerFieldPush(begin, size);
        }
        else
        {
            DoFieldPushImpl(begin, size);
        }
    }

    template <typename IterType>
    inline void DoFieldPush(IterType begin, size_t size)
    {
        if (std::is_integral<typename IterType::value_type>::value
            && (sizeof(typename IterType::value_type) > 1))
        {
            DoIntegerFieldPush(begin, size);
        }
        else
        {
            DoFieldPushImpl(begin, size);
        }
    }

    template<typename IterType>
    inline void DoIntegerFieldPush(IterType begin, size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            DoIntegerFieldPush(*begin++);
        }
    }

    template<typename IntegerType>
    inline void DoIntegerFieldPush(IntegerType value)
    {
        if (host_to_network_)
        {
            value = HostToNetwork(value);
        }
        DoIntegerFieldPushImpl(value);
    }

    template<typename IntegerType>
    inline void DoIntegerFieldPushImpl(IntegerType value)
    {
        buffer_.Append(value);
    }

    SerializeBuffer& buffer_;
    bool host_to_network_;
};

}

#endif // MP_STREAM_WRITER_H_

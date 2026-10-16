/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
/**
* @file       message_decoder.h
* @author     liujie
* @date       20170210
*/

#ifndef MP_MESSAGE_DECODER_H_
#define MP_MESSAGE_DECODER_H_

#include <algorithm>
#include <string>
#include <vector>
#include "error_code.h"
#include "serialize_buffer.h"
#include "string_util.h"
#include <array>
namespace mp
{
class MessageDecoder
{
public:
    typedef ErrorCode error_code;

    explicit MessageDecoder(bool ntoh = false)
        :buffer_start_(NULL),
         read_ptr_(NULL),
         left_bytes_(0),
         ntoh_(ntoh)
    {}

    MessageDecoder(const SerializeBuffer& buffer, bool ntoh = false)
        : buffer_start_(NULL),
          read_ptr_(NULL),
          left_bytes_(0),
          ntoh_(ntoh)
    {
        if (!buffer.Empty())
        {
            Reset(&buffer[0], buffer.Size());
        }
    }

    MessageDecoder(const char* data, size_t size, bool ntoh = false)
        : buffer_start_(NULL),
          read_ptr_(NULL),
          left_bytes_(0),
          ntoh_(ntoh)
    {
        Reset(data, size);
    }

    MessageDecoder(const char* data, size_t offset, size_t size, bool ntoh = false)
        : buffer_start_(NULL),
          read_ptr_(NULL),
          left_bytes_(0),
          ntoh_(ntoh)
    {
        Reset(data, offset, size);
    }

    inline error_code Reset()
    {
        return Reset(NULL, 0, 0);
    }

    inline error_code Reset(const char* data, size_t size)
    {
        return Reset(data, 0, size);
    }

    inline error_code Reset(const char* data, size_t offset, size_t size)
    {
        if (size && !data)
        {
            return mp::kFailure;
        }

        if (offset > size)
        {
            return mp::kFailure;
        }

        buffer_start_ = data;
        read_ptr_ = data + offset;
        left_bytes_ = size - offset;

        return mp::kSuccess;
    }

    inline size_t left_bytes()
    {
        return left_bytes_;
    }

    inline size_t Offset()
    {
        return read_ptr_ - buffer_start_;
    }

    inline error_code PopChar(char& val)
    {
		if (HaveBytes(sizeof(val)))
		{
			val = *read_ptr_;
			MovePointer(sizeof(val));
			return mp::kSuccess;
		}
        else if (LeftZeroBytes())
        {
            return mp::kMessageIsShort;
        }
		else
		{
			return mp::kFailure;
		}
    }

    inline error_code PopInt8(int8_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopUInt8(uint8_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopInt16(int16_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopUInt16(uint16_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopInt32(int32_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopUInt32(uint32_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopInt64(int64_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopUInt64(uint64_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(char& val)
    {
        return PopChar(val);
    }

    inline error_code PopField(int8_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(uint8_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(int16_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(uint16_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(int32_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(uint32_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(int64_t& val)
    {
        return PopInteger(val);
    }

    inline error_code PopField(uint64_t& val)
    {
        return PopInteger(val);
    }

    template <class T>
    inline error_code PopField(T* data, size_t size)
    {
        if (HaveBytes(sizeof(T) * size))
        {
            memcpy(data, read_ptr_, sizeof(T) * size);
            MovePointer(sizeof(T) * size);
            return mp::kSuccess;
        }
        else if (LeftZeroBytes())
        {
            return mp::kMessageIsShort;   // arrary<char>
        }
        else
        {
            return mp::kFailure;
        }
    }

    template <class T, class A>
    inline error_code PopField(std::vector<T, A>& data, size_t size)
    {
        data.resize(size);
        return PopField(&data[0], size);
    }

    inline error_code PopField(SerializeBuffer& data, size_t size)
    {
        data.Resize(size);
        return PopField(&data[0], size);
    }

    template <class T, class A>
    inline error_code PopField(std::basic_string<char, T, A>& data, size_t size)
    {
        if (HaveBytes(size))
        {
            data.assign(read_ptr_, size);
            MovePointer(size);
            return mp::kSuccess;
        }
        else if (LeftZeroBytes())
        {
            return mp::kMessageIsShort;
        }
        else
        {
            return mp::kFailure;
        }
    }

    template <class T, class A>
    inline error_code PopField(std::vector<T, A>& data)
    {
        return PopField(&data[0], data.size());
    }

    inline error_code PopField(SerializeBuffer& data)
    {
        return PopField(&data[0], data.Size());
    }

    template <class T, size_t size>
    inline error_code PopField(T (&data)[size])
    {
        return PopField(data, size);
    }
    template<class T,size_t size>
    inline error_code PopField(std::array<T,size>&data)
    {
        return PopField(&data[0],size);
    }
    
    template <class T>
    inline error_code PopAllLeft(T& data)
    {
        return PopField(data, left_bytes_);
    }
private:
    inline bool HaveBytes(size_t size) const
    {
        return left_bytes_ >= size;
    }

    inline bool LeftZeroBytes() const
    {
        return left_bytes_ == 0;
    }

    inline void MovePointer(size_t size)
    {
        assert(size <= left_bytes_);
        assert(read_ptr_);

        read_ptr_ += size;
        left_bytes_ -= size;
    }

    template <class T>
    inline error_code PopInteger(T& val)
    {
        if (HaveBytes(sizeof(T)))
        {
            val = ntoh_ ? NetworkToHost(*reinterpret_cast<const T*>(read_ptr_))
                        : *reinterpret_cast<const T*>(read_ptr_);
            MovePointer(sizeof(val));
            return mp::kSuccess;
        }
        else if (LeftZeroBytes())
        {
            return mp::kMessageIsShort;
        }
        else
        {
            return mp::kFailure;
        }
    }

    const char* buffer_start_;
    const char* read_ptr_;
    size_t left_bytes_;
    bool ntoh_;
};

}

#endif // MP_MESSAGE_DECODER_H_

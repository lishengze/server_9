/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
/**
* @file       serialize_buffer.h
* @author     liujie
* @date       20170210
*/


#ifndef MP_SERIALIZE_BUFFER_H_
#define MP_SERIALIZE_BUFFER_H_

#include <assert.h>
#include <string.h>
#include <algorithm>
#include <type_traits>
#include <stdint.h>

namespace mp
{

static const size_t kSerializeBufferReserveSize = 1024;

class SerializeBuffer
{
public:
    typedef char        value_type;
    typedef char*       iterator;
    typedef const char* const_iterator;
    typedef char&       reference;
    typedef const char& const_reference;

    SerializeBuffer()
        : begin_(0), end_(0) 
    {
        buffer_size_ = kSerializeBufferReserveSize;
        buffer_ = new char[buffer_size_];
    }

    explicit SerializeBuffer(size_t size)
        : begin_(0), end_(size)
    {
        buffer_size_ = ((size + kSerializeBufferReserveSize) / kSerializeBufferReserveSize) * kSerializeBufferReserveSize;
        buffer_ = new char[buffer_size_];
    }

    SerializeBuffer(const char* begin, const char* end)
    {
        assert(end >= begin);

        const size_t data_size = end - begin;
        begin_ = 0;
        end_ = data_size;
        buffer_size_ = ((data_size + kSerializeBufferReserveSize) / kSerializeBufferReserveSize) * kSerializeBufferReserveSize;
        buffer_ = new char[buffer_size_];
        memcpy(buffer_, begin, data_size);
    }

    SerializeBuffer(const char* begin, size_t size)
    {
        auto end=begin + size;
		assert(end >= begin);

        const size_t data_size = end - begin;
        begin_ = 0;
        end_ = data_size;
        buffer_size_ = ((data_size + kSerializeBufferReserveSize) / kSerializeBufferReserveSize) * kSerializeBufferReserveSize;
        buffer_ = new char[buffer_size_];
        memcpy(buffer_, begin, data_size);
    }

    SerializeBuffer(SerializeBuffer& byte_buffer)
    {
        begin_ = 0;
        end_ = byte_buffer.end_ - byte_buffer.begin_;
        buffer_size_ = byte_buffer.buffer_size_;
        buffer_ = new char[buffer_size_];
        memcpy(buffer_, byte_buffer.buffer_ + byte_buffer.begin_, end_);
    }

    SerializeBuffer(SerializeBuffer&& byte_buffer)
    {
        begin_ = byte_buffer.begin_;
        end_ = byte_buffer.end_;
        buffer_size_ = byte_buffer.buffer_size_;
        buffer_ = byte_buffer.buffer_;
        byte_buffer.buffer_ = nullptr;
    }

    SerializeBuffer& operator = (SerializeBuffer& byte_buffer)
    {
        if (&byte_buffer != this)
        {
            begin_ = 0;
            end_ = byte_buffer.end_ - byte_buffer.begin_;
            buffer_size_ = byte_buffer.buffer_size_;
            buffer_ = new char[buffer_size_];
            memcpy(buffer_, byte_buffer.buffer_ + byte_buffer.begin_, end_);
        }
        return *this;
    }

    SerializeBuffer& operator = (SerializeBuffer&& byte_buffer)
    {
        if (&byte_buffer != this)
        {
            begin_ = byte_buffer.begin_;
            end_ = byte_buffer.end_;
            buffer_size_ = byte_buffer.buffer_size_;
            buffer_ = byte_buffer.buffer_;
            byte_buffer.buffer_ = nullptr;
        }
        return *this;
    }

    ~SerializeBuffer()
    {
        if (nullptr != buffer_)
        {
            delete[] buffer_;
        }
    }

    inline bool Empty() const
    {
        return end_ == begin_;
    }

    inline size_t RemainSize() const
    {
        return buffer_size_ - end_;
    }

    inline size_t Size() const
    {
        return end_ - begin_;
    }

    inline size_t Capacity() const
    {
        return buffer_size_ - begin_;
    }

    iterator Begin()
    {
        return buffer_ + begin_;
    }

    const_iterator Begin() const
    {
        return buffer_ + begin_;
    }

    iterator End()
    {
        return buffer_ + end_;
    }

    const_iterator End() const
    {
        return buffer_ + end_;
    }

    inline void Clear()
    {
        begin_ = 0;
        end_ = 0;
    }

    inline void Resize(size_t size)
    {
        if (size > 0)
        {
            if ((begin_ > 0) && (begin_ == end_))
            {
                Clear();
            }

            Reserve(size);
            end_ = begin_ + size;
        } 
        else
        {
            Clear();
        }
    }

    inline void Reserve(size_t size)
    {
        if (begin_ + size > buffer_size_)
        {
            DoReserve(size);
        }
    }

    inline void Append(const char* data, size_t size)
    {
        if (size > 0)
        {
            if (end_ + size > buffer_size_)
            {
                DoReserve(Size() + size);
            }

            memcpy(buffer_ + end_, data, size);
            end_ += size;
        }
    }

    template<typename IntegerType>
    inline void Append(IntegerType data)
    {
        //BOOST_STATIC_ASSERT_MSG(boost::is_integral<IntegerType>::value, "append invalid type");
        static_assert(std::is_integral<IntegerType>::value,"append invalid type");
        if (end_ + sizeof(IntegerType) > buffer_size_)
        {
            DoReserve(Size() + sizeof(IntegerType));
        }

        char* const assign_buffer = buffer_ + end_;
        *reinterpret_cast<IntegerType*>(assign_buffer) = data;
        end_ += sizeof(IntegerType);
    }

    template<size_t kSize>
    inline void Append(const char* data)
    {
        //BOOST_STATIC_ASSERT_MSG(kSize > 0, "append invalid size");
        static_assert(kSize>0,"append invalid size");
        if (end_ + kSize > buffer_size_)
        {
            DoReserve(Size() + kSize);
        }

        char* const assign_buffer = buffer_ + end_;
        switch (kSize)
        {
        case 1:
            *assign_buffer = *data;
            break;
        case 2:
            *((uint16_t*)assign_buffer) = *((uint16_t*)data);
            break;
        case 3:
            *((uint16_t*)assign_buffer) = *((uint16_t*)data);
            *(assign_buffer + 2) = *(data + 2);
            break;
        case 4:
            *((uint32_t*)assign_buffer) = *((uint32_t*)data);
            break;
        case 5:
            *((uint32_t*)assign_buffer) = *((uint32_t*)data);
            *(assign_buffer + 4) = *(data + 4);
            break;
        case 6:
            *((uint32_t*)assign_buffer) = *((uint32_t*)data);
            *((uint16_t*)(assign_buffer + 4)) = *((uint16_t*)(data + 4));
            break;
        case 7:
            *((uint32_t*)assign_buffer) = *((uint32_t*)data);
            *((uint16_t*)(assign_buffer + 4)) = *((uint16_t*)(data + 4));
            *(assign_buffer + 6) = *(data + 6);
            break;
        case 8:
            *((uint64_t*)assign_buffer) = *((uint64_t*)data);
            break;
        default:
            memcpy(assign_buffer, data, kSize);
        }

        end_ += kSize;
    }

    inline void Append(const SerializeBuffer& data)
    {
        if (!data.Empty())
        {
            Append(&data[0], data.Size());
        }
    }

    template<size_t size>
    inline void Append(const char (&data)[size])
    {
        Append(data, size);
    }

    inline char* GetAppendBuffer(size_t size)
    {
        size_t new_size = end_ + size;
        if (new_size > buffer_size_)
        {
            DoReserve(new_size);
        }

        char* to_return = buffer_ + end_;
        end_ += size;
        return to_return;
    }

    inline void Assign(const char* data, size_t size)
    {
        Clear();
        Append(data, size);
    }

    inline void Shift(size_t size)
    {
        if (begin_ + size >= end_)
        {
            Clear();
        }
        else
        {
            begin_ += size;
        }
    }

    inline void HardShift(size_t size)
    {
        const size_t buffer_size = Size();
        if (size >= buffer_size)
        {
            Clear();
        } 
        else
        {
            end_ = buffer_size - size;
            memmove(buffer_, buffer_ + begin_ + size, end_);
            begin_ = 0;
        }
    }

    inline const_reference operator[] (size_t n) const
    {
        return buffer_[begin_ + n];
    }

    inline reference operator[] (size_t n)
    {
        return buffer_[begin_ + n];
    }

    void Swap(SerializeBuffer& rhs)
    {
        std::swap(begin_, rhs.begin_);
        std::swap(end_, rhs.end_);
        std::swap(buffer_size_, rhs.buffer_size_);
        std::swap(buffer_, rhs.buffer_);
    }
private:

    inline void DoReserve(size_t new_size)
    {
        buffer_size_ = ((new_size + kSerializeBufferReserveSize) / kSerializeBufferReserveSize) * kSerializeBufferReserveSize;
        char* const new_buffer = new char[buffer_size_];
        memcpy(new_buffer, buffer_ + begin_, end_ - begin_);
        delete[] buffer_;
        buffer_ = new_buffer;
        end_ -= begin_;
        begin_ = 0;
    }

    size_t    begin_;
    size_t    end_;
    size_t    buffer_size_;
    char*     buffer_;
};

inline void Swap(SerializeBuffer& lhs, SerializeBuffer& rhs)
{
    lhs.Swap(rhs);
}
}

#endif // MP_SERIALIZE_BUFFER_H_

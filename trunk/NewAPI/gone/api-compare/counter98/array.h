/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved. 
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/

#ifndef MP_ARRAY_H_
#define MP_ARRAY_H_

#include <stdint.h>
#include <assert.h>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <functional>

namespace mp
{
    template<typename T,std::size_t N>
    class array
    {
    public:
        typedef T           value_type ;
        typedef T*          iterator ;
        typedef const T*    const_iterator ;
        typedef T&          reference ;
        typedef const T&    const_reference ;
        typedef std::size_t size_type ;
        typedef std::ptrdiff_t   difference_type;

        iterator        begin()         {return elems ; }
        const_iterator  begin()  const  { return elems ;}
        iterator        cbegin() const  { return elems ;}

        iterator        end()           { return elems+N; }
        const_iterator  end()    const  { return elems+N; }
        const_iterator  cend()   const  { return elems+N; }

        iterator        rbegin()        { return end(); }
        const_iterator  crbegin() const { return end(); }
        iterator        rend()          { return begin(); }
        const_iterator  crend() const   { return begin(); }

        /**
        *   @brief array默认构造函数   
        **/
        array()
        {
            std::memset(elems,' ',N);
        }

        /**
        *   @brief array构造函数
        *   @param[in] std::string 
        *   @note      超过N将会截断,不足自动补空格   
        **/
        array(const std::string& str)
        {
           assign(str);
        }

        /**
        *   @brief array构造函数
        *   @param[in] char* 字符串地址
        *   @param[in] length 字符串长度
        *   @note  超过N将会截断，不足自动补空格
        **/
        array(const char* str,size_type length)
        {
            std::string ss(str,length);
            assign(ss);
        }

        /**
        *   @brief ToString函数
        *   @ret  返回std::string   
        **/
        std::string ToString() const
        {
            return std::string(elems, N);
        }

        /**
        *   @brief ToStringTrimRight函数
        *   @ret  返回去掉右边空格的std::string 
        **/
        std::string ToStringTrimRight() const
        {
            size_type i = N;
            while (true)
            {
                if (elems[--i] != ' ')
                    break;
                
                if (i == 0)
                    return std::string();
            }
            
            return std::string(elems, i + 1);
        }

        /**
        *   @brief []操作符
        *   @ret  返回当前指定位置的字符   
        **/
        reference  operator[](size_type i)
        {
            assert(i<N);
            return elems[i] ;
        }
        
        /**
        *   @brief []操作符
        *   @ret  返回当前指定位置的字符   
        **/
        const_reference operator[](size_type i) const
        {
            assert(i<N);
            return elems[i];
        }

         /**
        *   @brief size函数
        *   @note 返回array总长度  
        **/
        static size_type size() 
        { 
            return N ;
        }

        reference at(size_type i)
        {
            rangecheck(i); 
            return elems[i];
        }

        const_reference at(size_type i) const 
        {
            rangecheck(i);
            return elems[i] ;
        }

        /**
        *   @brief front函数
        *   @note   返回第一个元素   
        **/
        reference front()
        {
            return elems[0];
        }

        /**
        *   @brief front函数
        *   @note   返回第一个元素   
        **/
        const_reference front() const
        {
            return elems[0] ;
        }

        /**
        *   @brief back函数
        *   @note   返回最后一个元素   
        **/
        reference back()
        {
            return elems[N-1] ;
        }

        /**
        *   @brief back函数
        *   @note   返回最后一个元素   
        **/
        const_reference back() const
        {
            return elems[N-1] ;
        }

        /**
        *   @brief data函数
        *   @note   返回首地址   
        **/
        const T*  data() const 
        {
            return elems;
        }

        /**
        *   @brief data函数
        *   @note   返回首地址   
        **/
        T* data()
        {
            return elems;
        }

         /**
        *   @brief 赋值操作符   
        **/
        array<T,N>& operator=(const array<T,N>& rhs)
        {
            std::copy(rhs.begin(),rhs.end(),begin());
            return *this;
        }

        /**
        *   @brief swap函数
        *   @note 交换array的值   
        **/
        void swap(array<T,N>& y)
        {
            for(size_type i = 0 ;i<N; ++i)
            {
                std::swap(elems[i],y.elems[i]);
            }
        }

        void assign(std::string& str)
        {
            size_type min = std::min(str.size(), N);
            std::memcpy(elems, str.c_str(), min);
            if (min < N)
            {
                std::memset(&elems[min], ' ', N - min);
            }
        }
        
        void assign(const std::string& str)
        {
            size_type min = std::min(str.size(), N);
            std::memcpy(elems, str.c_str(), min);
            if (min < N)
            {
                std::memset(&elems[min], ' ', N - min);
            }
        }

        /**
        *   @brief assign函数   
        **/
        void assign(const T& value) 
        { 
            fill(value);
        }

         /**
        *   @brief fill函数   
        **/
        void fill(const T& value)
        {
            std::fill_n(begin(),size(),value);
        }

    public:
        T elems[N] ;

    private:
        static void rangecheck(size_type i)
        {
            if (i>= N)
            {
                std::out_of_range e("out of range");
                throw e ;
            }
        }
    };

    template<typename T, std::size_t N>
    bool operator==(const array<T,N>& x,const array<T,N>& y)
    {
        return std::equal(x.begin(),x.end(),y.begin());
    }

    template<typename T,std::size_t N>
    bool operator< (const array<T,N>& x,const array<T,N>& y)
    {
        return std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());
    }

    template<typename T,std::size_t N>
    bool operator!=(const array<T,N>&x,const array<T,N>&y)
    {
        return !(x==y);
    }

    template<typename T,std::size_t N>
    bool operator > (const array<T,N>&x,const array<T,N>& y)
    {
        return y < x ;
    }

    template<typename T,std::size_t N>
    bool operator <=(const array<T,N>&x, const array<T,N>& y)
    {
        return !(y<x);
    }

    template<typename T,std::size_t N>
    bool operator >=(const array<T,N>&x,const array<T,N>&y)
    {
        return !(x<y);
    }

    template<typename T,std::size_t N>
    inline void swap(array<T,N>& x,array<T,N>& y)
    {
        x.swap(y);
    }

    /*
    template<typename T, std::size_t N>
    std::size_t hash_value(const mp::array<T,N>& arr)
    {
        return boost::hash_range(arr.begin(), arr.end());
    }
    */


}// end namespace mp

namespace std
{
    
    template<typename T,size_t N>
    struct hash<mp::array<T,N> >
    {
        typedef mp::array<T,N> argument_type ;
        typedef std::size_t result_type ;
        result_type operator()(argument_type const & s) const
        {
            return (std::hash<std::string>{}(s.ToString()));
        }
    };
    
}// namespace std

#endif  

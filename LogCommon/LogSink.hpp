// Copyright � 2012-2013 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <limits>
#include <type_traits>
#include <memory>
#include <boost/config.hpp>
#include "OptimisticBuffer.hpp"

namespace Instalog
{
    class log_sink
    {
    public:
        virtual void append(char const* data, std::size_t dataLength) = 0;
        virtual ~log_sink()
        {}
    };

    class string_sink final : public log_sink
    {
        std::string target;
    public:
        virtual void append(char const* data, std::size_t dataLength)
        {
            this->target.append(data, dataLength);
        }
        std::string const& get() const
        {
            return this->target;
        }
    };

    class format_intrusive_result
    {
        char const* ptr;
        std::size_t length;
    public:
        format_intrusive_result() = default;
        format_intrusive_result(char const* data_, std::size_t length_)
            : ptr(data_)
            , length(length_)
        {}
        char const* data() const
        {
            return this->ptr;
        }
        std::size_t size() const
        {
            return this->length;
        }
    };

    template <std::size_t allocLength>
    class format_stack_result
    {
        char array[allocLength];
        std::size_t length;
    public:
        static const std::size_t declared_size = allocLength;
        decltype(array)& data()
        {
            return this->array;
        }
        char const* data() const
        {
            return this->array;
        }
        void set_size(std::size_t size_)
        {
            this->length = size_;
        }
        std::size_t size() const
        {
            return length;
        }
    };

    format_intrusive_result format_value(std::string const& value)
    {
        return format_intrusive_result(value.c_str(), value.size());
    }

    format_intrusive_result format_value(char const* ptr)
    {
        return format_intrusive_result(ptr, std::strlen(ptr));
    }

    template <typename IntegralType, typename>
    struct stack_result_for_digits_impl
    {};

    template <typename IntegralType>
    struct stack_result_for_digits_impl<IntegralType, std::true_type>
    {
        typedef format_stack_result<std::numeric_limits<IntegralType>::digits10 + 3> type;
    };

    template<typename IntegralType>
    struct stack_result_for_digits : public stack_result_for_digits_impl<IntegralType, typename std::is_integral<IntegralType>::type>
    {};

    template <typename IntegralType>
    typename typename stack_result_for_digits<IntegralType>::type format_value(IntegralType value)
    {
        typedef typename stack_result_for_digits<IntegralType>::type result_type;
        result_type result;
        long long tempValue = value;
        result.set_size(sprintf_s(result.data(), "%lld", tempValue));
        return result;
    }

#ifdef BOOST_WINDOWS
    format_intrusive_result get_newline()
    {
        return format_intrusive_result("\r\n", 2);
    }
#else
    format_stack_result<1> get_newline()
    {
        return format_intrusive_result("\n", 1);
    }
#endif

    std::size_t sum_sizes()
    {
        return 0;
    }

    template <typename... Integral>
    std::size_t sum_sizes(std::size_t size, Integral ...sizes)
    {
        return size + sum_sizes(sizes...);
    }

    template <typename Sink, typename... Slices>
    Sink& write_impl(Sink& target, Slices &&...slices)
    {
        // Special thanks to Nawaz for his help here.
        // See: http://stackoverflow.com/a/20440197/82320
        std::size_t const length = sum_sizes(slices.size()...);
        OptimisticBuffer<256> buff(length);
        char* ptr = buff.GetAs<char>();
        char* endPtr = ptr;
        char* expand[] = {(endPtr = std::copy_n(slices.data(), slices.size(), endPtr))...};
        (void)expand; // silence unreferenced parameter warning.
        target.append(ptr, endPtr - ptr);
        return target;
    }

    template <typename Sink, typename... Values>
    Sink& write(Sink& target, Values &&...values)
    {
        return write_impl(target, format_value(std::forward<Values>(values))...);
    }

    template <typename Sink, typename... Values>
    Sink& writeln(Sink& target, Values &&...values)
    {
        return write_impl(target, format_value(std::forward<Values>(values))..., get_newline());
    }
}

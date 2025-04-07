// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Memory/StlWrapper.h"
#include <ThirdParty/fmt/format.h>

namespace fmt_flax
{
    typedef std_flax::allocator<Char> allocator;
    typedef std_flax::allocator<char> allocator_ansi;

    typedef fmt::basic_memory_buffer<Char, fmt::inline_buffer_size, allocator> memory_buffer;
    typedef fmt::basic_memory_buffer<char, fmt::inline_buffer_size, allocator_ansi> memory_buffer_ansi;

    template<typename T, typename... Args>
    FORCE_INLINE static void format(fmt::basic_memory_buffer<T, fmt::inline_buffer_size, std_flax::allocator<T>>& buffer, const T* format, const Args& ... args)
    {
        typedef fmt::buffer_context<T> context;
        fmt::detail::vformat_to(buffer, fmt::basic_string_view<T>(format), fmt::make_format_args<context>(args...), {});
    }
}

#define DEFINE_DEFAULT_FORMATTING(type, formatText, ...) \
    namespace fmt \
    { \
        template<> \
        struct formatter<type, Char> \
        { \
            template<typename ParseContext> \
            auto parse(ParseContext& ctx) \
            { \
                return ctx.begin(); \
            } \
            template<typename FormatContext> \
            auto format(const type& v, FormatContext& ctx) -> decltype(ctx.out()) \
            { \
                return fmt::format_to(ctx.out(), basic_string_view<Char>(TEXT(formatText)), ##__VA_ARGS__); \
            } \
        }; \
    }

#define DEFINE_DEFAULT_FORMATTING_VIA_TO_STRING(type) \
    namespace fmt \
    { \
        template<> \
        struct formatter<type, Char> \
        { \
            template<typename ParseContext> \
            auto parse(ParseContext& ctx) \
            { \
                return ctx.begin(); \
            } \
            template<typename FormatContext> \
            auto format(const type& v, FormatContext& ctx) -> decltype(ctx.out()) \
            { \
                const String str = v.ToString(); \
                return fmt::detail::copy_str<Char>(str.Get(), str.Get() + str.Length(), ctx.out()); \
            } \
        }; \
    }

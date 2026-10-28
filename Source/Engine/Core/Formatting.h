// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Memory/StlWrapper.h"

#include <ThirdParty/fmt/xchar.h>
#include <ThirdParty/fmt/format.h>

FMT_BEGIN_NAMESPACE
FMT_BEGIN_EXPORT
namespace detail {
    extern template FMT_API void vformat_to(buffer<Char>&, basic_string_view<Char>, basic_format_args<buffered_context<Char>>, locale_ref);
    extern template FMT_API const char* parse_width(const char* begin, const char* end, format_specs& specs, arg_ref<char>& width_ref, parse_context<char>& ctx);
    extern template FMT_API const char* parse_precision(const char* begin, const char* end, format_specs& specs, arg_ref<char>& precision_ref, parse_context<char>& ctx);
    extern template FMT_API parse_dynamic_spec_result<char> parse_dynamic_spec(const char* begin, const char* end, int& value, arg_ref<char>& ref, parse_context<char>& ctx);
}
FMT_END_EXPORT
FMT_END_NAMESPACE

namespace fmt_flax
{
    typedef std_flax::allocator<Char> allocator;
    typedef std_flax::allocator<char> allocator_ansi;

    typedef fmt::basic_memory_buffer<Char, fmt::inline_buffer_size, allocator> memory_buffer;
    typedef fmt::basic_memory_buffer<char, fmt::inline_buffer_size, allocator_ansi> memory_buffer_ansi;

    template<typename T, typename... Args>
    void format(fmt::basic_memory_buffer<T, fmt::inline_buffer_size, std_flax::allocator<T>>& buffer, const T* format, const Args& ... args)
    {
        typedef fmt::buffered_context<T> context;
        fmt::detail::vformat_to(buffer, fmt::basic_string_view<T>(format), fmt::make_format_args<context>(args...), {});
    }

    extern template void format<Char, int32>(fmt::basic_memory_buffer<Char, fmt::inline_buffer_size, std_flax::allocator<Char>>&, const Char*, const int32&);
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
            auto format(const type& v, FormatContext& ctx) const -> decltype(ctx.out()) \
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
            auto format(const type& v, FormatContext& ctx) const -> decltype(ctx.out()) \
            { \
                const String str = v.ToString(); \
                return fmt::detail::copy<Char>(str.Get(), str.Get() + str.Length(), ctx.out()); \
            } \
        }; \
    }

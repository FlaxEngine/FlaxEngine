// Copyright (c) Wojciech Figat. All rights reserved.

#include "Formatting.h"

FMT_BEGIN_NAMESPACE
namespace detail {
    template FMT_API void vformat_to(buffer<Char>&, basic_string_view<Char>, basic_format_args<buffered_context<Char>>, locale_ref);
    
    template FMT_API const char* parse_width(const char* begin, const char* end, format_specs& specs, arg_ref<char>& width_ref, parse_context<char>& ctx);
    template FMT_API const char* parse_precision(const char* begin, const char* end, format_specs& specs, arg_ref<char>& precision_ref, parse_context<char>& ctx);
    template FMT_API parse_dynamic_spec_result<char> parse_dynamic_spec(const char* begin, const char* end, int& value, arg_ref<char>& ref, parse_context<char>& ctx);
}
FMT_END_NAMESPACE

namespace fmt_flax
{
    template void format<Char, int32>(fmt::basic_memory_buffer<Char, fmt::inline_buffer_size, std_flax::allocator<Char>>&, const Char*, const int32&);
}

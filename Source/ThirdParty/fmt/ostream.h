// Formatting library for C++ - std::ostream support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_OSTREAM_H_
#define FMT_OSTREAM_H_

#include <ostream>

#include "format.h"

FMT_BEGIN_NAMESPACE

template <typename OutputIt, typename Char> class basic_printf_context;

namespace detail {

// Checks if T has a user-defined operator<<.
template <typename T, typename Char, typename Enable = void>
class is_streamable {
 private:
  template <typename U>
  static auto test(int)
      -> bool_constant<sizeof(std::declval<std::basic_ostream<Char>&>()
                              << std::declval<U>()) != 0>;

  template <typename> static auto test(...) -> std::false_type;

  using result = decltype(test<T>(0));

 public:
  is_streamable() = default;

  static const bool value = result::value;
};

// Formatting of built-in types and arrays is intentionally disabled because
// it's handled by standard (non-ostream) formatters.
template <typename T, typename Char>
struct is_streamable<
    T, Char,
    enable_if_t<
        std::is_arithmetic<T>::value || std::is_array<T>::value ||
        std::is_pointer<T>::value || std::is_same<T, char8_type>::value ||
        std::is_convertible<T, fmt::basic_string_view<Char>>::value ||
        std::is_same<T, std_string_view<Char>>::value ||
        (std::is_convertible<T, int>::value && !std::is_enum<T>::value)>>
    : std::false_type {};


// Write the content of buf to os.
// It is a separate function rather than a part of vprint to simplify testing.
template <typename Char>
void write_buffer(std::basic_ostream<Char>& os, buffer<Char>& buf) {
  const Char* buf_data = buf.data();
  using unsigned_streamsize = std::make_unsigned<std::streamsize>::type;
  unsigned_streamsize size = buf.size();
  unsigned_streamsize max_size = to_unsigned(max_value<std::streamsize>());
  do {
    unsigned_streamsize n = size <= max_size ? size : max_size;
    os.write(buf_data, static_cast<std::streamsize>(n));
    buf_data += n;
    size -= n;
  } while (size != 0);
}

template <typename Char, typename T>
void format_value(buffer<Char>& buf, const T& value,
                  locale_ref loc = locale_ref()) {
  auto&& format_buf = formatbuf<std::basic_streambuf<Char>>(buf);
  auto&& output = std::basic_ostream<Char>(&format_buf);
#if !defined(FMT_STATIC_THOUSANDS_SEPARATOR)
  if (loc) output.imbue(loc.get<std::locale>());
#endif
  output << value;
  output.exceptions(std::ios_base::failbit | std::ios_base::badbit);
}

template <typename T> struct streamed_view { const T& value; };

}  // namespace detail

// Formats an object of type T that has an overloaded ostream operator<<.
template <typename Char>
struct basic_ostream_formatter : formatter<basic_string_view<Char>, Char> {
  void set_debug_format() = delete;

  template <typename T, typename OutputIt>
  auto format(const T& value, basic_format_context<OutputIt, Char>& ctx) const
      -> OutputIt {
    auto buffer = basic_memory_buffer<Char>();
    format_value(buffer, value, ctx.locale());
    return formatter<basic_string_view<Char>, Char>::format(
        {buffer.data(), buffer.size()}, ctx);
  }
};

using ostream_formatter = basic_ostream_formatter<char>;

template <typename T, typename Char>
struct formatter<detail::streamed_view<T>, Char>
    : basic_ostream_formatter<Char> {
  template <typename OutputIt>
  auto format(detail::streamed_view<T> view,
              basic_format_context<OutputIt, Char>& ctx) const -> OutputIt {
    return basic_ostream_formatter<Char>::format(view.value, ctx);
  }
};

/**
  \rst
  Returns a view that formats `value` via an ostream ``operator<<``.

  **Example**::

    fmt::print("Current thread id: {}\n",
               fmt::streamed(std::this_thread::get_id()));
  \endrst
 */
template <typename T>
auto streamed(const T& value) -> detail::streamed_view<T> {
  return {value};
}

FMT_END_NAMESPACE

#endif  // FMT_OSTREAM_H_

// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "String.h"
#include "StringView.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Flax implementation for strings building class that supports UTF-16 (Unicode)
/// </summary>
class FLAXENGINE_API StringBuilder
{
private:
    /// <summary>
    /// Array with characters of the string (it's not null-terminated)
    /// </summary>
    Array<Char> _data;

public:
    /// <summary>
    /// Init
    /// </summary>
    StringBuilder()
    {
    }

    /// <summary>
    /// Create string builder with initial capacity.
    /// </summary>
    /// <param name="capacity">Initial capacity for chars count</param>
    StringBuilder(int32 capacity)
        : _data(capacity)
    {
    }

public:
    /// <summary>
    /// Gets the buffer capacity.
    /// </summary>
    FORCE_INLINE int32 Capacity() const
    {
        return _data.Capacity();
    }

    /// <summary>
    /// Sets the buffer capacity.
    /// </summary>
    /// <param name="capacity">Capacity to set</param>
    FORCE_INLINE void SetCapacity(const int32 capacity)
    {
        _data.SetCapacity(capacity);
    }

    /// <summary>
    /// Gets string length
    /// </summary>
    /// <returns>String length</returns>
    FORCE_INLINE int32 Length() const
    {
        return _data.Count();
    }

    /// <summary>
    /// Clears data
    /// </summary>
    FORCE_INLINE void Clear()
    {
        _data.Clear();
    }

    /// <summary>
    /// Gets the string.
    /// </summary>
    /// <param name="result">String</param>
    void ToString(String& result) const
    {
        result = String(_data.Get(), _data.Count());
    }

public:
    StringBuilder& Append(const Char c)
    {
        _data.Add(c);
        return *this;
    }

    StringBuilder& Append(const char c)
    {
        _data.Add(c);
        return *this;
    }

    StringBuilder& Append(const Char* str)
    {
        const int32 length = StringUtils::Length(str);
        _data.Add(str, length);
        return *this;
    }

    StringBuilder& Append(const Char* str, int32 length)
    {
        _data.Add(str, length);
        return *this;
    }

    StringBuilder& Append(const char* str)
    {
        const int32 length = str && *str ? StringUtils::Length(str) : 0;
        const int32 prevCnt = _data.Count();
        _data.AddDefault(length);
        int32 tmp;
        StringUtils::ConvertANSI2UTF16(str, _data.Get() + prevCnt, length, tmp);
        return *this;
    }

    StringBuilder& Append(const String& str)
    {
        _data.Add(*str, str.Length());
        return *this;
    }

    StringBuilder& Append(const StringView& str)
    {
        _data.Add(*str, str.Length());
        return *this;
    }

    StringBuilder& Append(int32 val)
    {
        auto str = StringUtils::ToString(val);
        _data.Add(*str, str.Length());
        return *this;
    }

    StringBuilder& Append(uint32 val)
    {
        auto str = StringUtils::ToString(val);
        _data.Add(*str, str.Length());
        return *this;
    }

    template<typename... Args>
    StringBuilder& AppendFormat(const Char* format, const Args&... args)
    {
        fmt_flax::allocator allocator;
        fmt_flax::memory_buffer buffer(allocator);
        fmt_flax::format(buffer, format, args...);
        return Append(buffer.data(), (int32)buffer.size());
    }

    StringBuilder& AppendLine()
    {
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    StringBuilder& AppendLine(int32 val)
    {
        Append(val);
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    StringBuilder& AppendLine(uint32 val)
    {
        Append(val);
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    StringBuilder& AppendLine(const Char* str)
    {
        const int32 length = StringUtils::Length(str);
        _data.Add(str, length);
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    StringBuilder& AppendLine(const String& str)
    {
        _data.Add(*str, str.Length());
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

public:
    // Gets pointer to the string.
    FORCE_INLINE const Char* operator*() const
    {
        return _data.Count() > 0 ? _data.Get() : TEXT("");
    }

    // Gets pointer to the string.
    FORCE_INLINE Char* operator*()
    {
        return _data.Count() > 0 ? _data.Get() : (Char*)TEXT("");
    }

    // Gets string buffer as array of Chars.
    FORCE_INLINE Array<Char>& GetCharArray()
    {
        return _data;
    }

    // Gets string buffer as const array of Chars.
    FORCE_INLINE const Array<Char>& GetCharArray() const
    {
        return _data;
    }

public:
    String ToString() const
    {
        return String(_data.Get(), _data.Count());
    }

    StringView ToStringView() const;
};

inline uint32 GetHash(const StringBuilder& key)
{
    return StringUtils::GetHashCode(key.GetCharArray().Get(), key.Length());
}

namespace fmt
{
    template<>
    struct formatter<StringBuilder, Char>
    {
        template<typename ParseContext>
        auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const String& v, FormatContext& ctx) -> decltype(ctx.out())
        {
            return fmt::detail::copy_str<Char>(v.Get(), v.Get() + v.Length(), ctx.out());
        }
    };
}

// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "String.h"
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
    /// Create string builder with initial capacity
    /// </summary>
    /// <param name="capacity">Initial capacity for chars count</param>
    StringBuilder(int32 capacity)
        : _data(capacity)
    {
    }

public:

    /// <summary>
    /// Gets capacity
    /// </summary>
    /// <returns>Capacity of the string builder</returns>
    FORCE_INLINE int32 Capacity() const
    {
        return _data.Capacity();
    }

    /// <summary>
    /// Sets capacity.
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
    /// Gets string
    /// </summary>
    /// <param name="result">String</param>
    void ToString(String& result) const
    {
        result = String(_data.Get(), _data.Count());
    }

public:

    // Append single character to the string
    // @param c Character to append
    // @return Current String Builder instance
    StringBuilder& Append(const Char c)
    {
        _data.Add(c);
        return *this;
    }

    // Append single character to the string
    // @param c Character to append
    // @return Current String Builder instance
    StringBuilder& Append(const char c)
    {
        _data.Add(c);
        return *this;
    }

    // Append characters sequence to the string
    // @param str String to append
    // @return Current String Builder instance
    StringBuilder& Append(const Char* str)
    {
        const int32 length = StringUtils::Length(str);
        _data.Add(str, length);
        return *this;
    }

    // Append characters sequence to the string
    // @param str String to append
    // @param length String length
    // @return Current String Builder instance
    StringBuilder& Append(const Char* str, int32 length)
    {
        _data.Add(str, length);
        return *this;
    }

    // Append characters sequence to the string
    // @param str String to append
    // @return Current String Builder instance
    StringBuilder& Append(const char* str)
    {
        const int32 length = str && *str ? StringUtils::Length(str) : 0;
        const int32 prevCnt = _data.Count();
        _data.AddDefault(length);
        StringUtils::ConvertANSI2UTF16(str, _data.Get() + prevCnt, length);
        return *this;
    }

    // Append String to the string
    // @param str String to append
    // @return Current String Builder instance
    StringBuilder& Append(const String& str)
    {
        _data.Add(*str, str.Length());
        return *this;
    }

    // Append int to the string
    // @param val Value to append
    // @return Current String Builder instance
    StringBuilder& Append(int32 val)
    {
        auto str = StringUtils::ToString(val);
        _data.Add(*str, str.Length());
        return *this;
    }

    // Append int to the string
    // @param val Value to append
    // @return Current String Builder instance
    StringBuilder& Append(uint32 val)
    {
        auto str = StringUtils::ToString(val);
        _data.Add(*str, str.Length());
        return *this;
    }

    // Append formatted message to the string
    // @param format Format string
    // @param args Array with custom arguments for the message
    template<typename... Args>
    StringBuilder& AppendFormat(const Char* format, const Args& ... args)
    {
        fmt_flax::allocator allocator;
        fmt_flax::memory_buffer buffer(allocator);
        fmt_flax::format(buffer, format, args...);
        return Append(buffer.data(), (int32)buffer.size());
    }

public:

    StringBuilder& AppendLine()
    {
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    // Append int to the string
    // @param val Value to append
    // @return Current String Builder instance
    StringBuilder& AppendLine(int32 val)
    {
        Append(val);
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    // Append int to the string
    // @param val Value to append
    // @return Current String Builder instance
    StringBuilder& AppendLine(uint32 val)
    {
        Append(val);
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    // Append String to the string
    // @param str String to append
    // @return Current String Builder instance
    StringBuilder& AppendLine(const Char* str)
    {
        const int32 length = StringUtils::Length(str);
        _data.Add(str, length);
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

    // Append String to the string
    // @param str String to append
    // @return Current String Builder instance
    StringBuilder& AppendLine(const String& str)
    {
        _data.Add(*str, str.Length());
        Append(TEXT(PLATFORM_LINE_TERMINATOR));
        return *this;
    }

public:

    // Retrieves substring created from characters starting from startIndex
    // @param startIndex Index of the first character to subtract
    // @param count Amount of characters to retrieve
    // @return Substring created from StringBuilder data
    String Substring(int32 startIndex, int32 count) const
    {
        ASSERT(startIndex >= 0 && startIndex + count <= _data.Count() && count > 0);
        return String(_data.Get() + startIndex, count);
    }

public:

    // Get pointer to the string
    // @returns Pointer to Array of Chars if Num, otherwise the empty string
    FORCE_INLINE const Char* operator*() const
    {
        return _data.Count() > 0 ? _data.Get() : TEXT("");
    }

    // Get pointer to the string
    // @returns Pointer to Array of Chars if Num, otherwise the empty string
    FORCE_INLINE Char* operator*()
    {
        return _data.Count() > 0 ? _data.Get() : (Char*)TEXT("");
    }

    // Get string as array of Chars
    FORCE_INLINE Array<Char>& GetCharArray()
    {
        return _data;
    }

    // Get string as const array of Chars
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
            return fmt::internal::copy(v.Get(), v.Get() + v.Length(), ctx.out());
        }
    };
}

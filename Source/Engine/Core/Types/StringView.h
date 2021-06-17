// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Core/Formatting.h"

/// <summary>
/// Represents static text view as a sequence of characters. Characters sequence might not be null-terminated.
/// </summary>
template<typename T>
class StringViewBase
{
protected:

    const T* _data;
    int32 _length;

public:

    /// <summary>
    /// Gets the specific const character from this string.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The character at given index.</returns>
    FORCE_INLINE const T& operator[](int32 index) const
    {
        ASSERT(index >= 0 && index <= _length);
        return _data[index];
    }

    /// <summary>
    /// Lexicographically tests how this string compares to the other given string.
    /// </summary>
    /// <param name="str">The another string test against.</param>
    /// <param name="searchCase">The case sensitivity mode.</param>
    /// <returns>0 if equal, -1 if less than, 1 if greater than.</returns>
    int32 Compare(const StringViewBase& str, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        const int32 lengthDiff = Length() - str.Length();
        if (lengthDiff != 0)
            return lengthDiff;
        if (searchCase == StringSearchCase::CaseSensitive)
            return StringUtils::Compare(this->GetText(), str.GetText(), Length());
        return StringUtils::CompareIgnoreCase(this->GetText(), str.GetText(), Length());
    }

public:

    /// <summary>
    /// Returns true if string is empty.
    /// </summary>
    FORCE_INLINE bool IsEmpty() const
    {
        return _length == 0;
    }

    /// <summary>
    /// Returns true if string isn't empty.
    /// </summary>
    FORCE_INLINE bool HasChars() const
    {
        return _length != 0;
    }

    /// <summary>
    /// Gets the length of the string.
    /// </summary>
    FORCE_INLINE constexpr int32 Length() const
    {
        return _length;
    }

    /// <summary>
    /// Gets the pointer to the string.
    /// </summary>
    FORCE_INLINE constexpr const T* operator*() const
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the string.
    /// </summary>
    FORCE_INLINE constexpr const T* Get() const
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the string or to the static empty text if string is null. Returned pointer is always valid (read-only).
    /// </summary>
    FORCE_INLINE const T* GetText() const
    {
        return _data ? _data : (const T*)TEXT("");
    }

public:

    /// <summary>
    /// Searches the string for the occurrence of a character.
    /// </summary>
    /// <param name="c">The character to search for.</param>
    /// <returns>The index of the character position in the string or -1 if not found.</returns>
    int32 Find(T c) const
    {
        const T* RESTRICT start = Get();
        for (const T * RESTRICT data = start, *RESTRICT dataEnd = data + _length; data != dataEnd; ++data)
        {
            if (*data == c)
            {
                return static_cast<int32>(data - start);
            }
        }
        return -1;
    }

    /// <summary>
    /// Searches the string for the last occurrence of a character.
    /// </summary>
    /// <param name="c">The character to search for.</param>
    /// <returns>The index of the character position in the string or -1 if not found.</returns>
    int32 FindLast(T c) const
    {
        const T* RESTRICT end = Get() + _length;
        for (const T * RESTRICT data = end, *RESTRICT dataStart = data - _length; data != dataStart;)
        {
            --data;
            if (*data == c)
            {
                return static_cast<int32>(data - dataStart);
            }
        }
        return -1;
    }

    bool StartsWith(T c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
    {
        const int32 length = Length();
        if (searchCase == StringSearchCase::IgnoreCase)
            return length > 0 && StringUtils::ToLower(_data[0]) == StringUtils::ToLower(c);
        return length > 0 && _data[0] == c;
    }

    bool EndsWith(T c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
    {
        const int32 length = Length();
        if (searchCase == StringSearchCase::IgnoreCase)
            return length > 0 && StringUtils::ToLower(_data[length - 1]) == StringUtils::ToLower(c);
        return length > 0 && _data[length - 1] == c;
    }

    bool StartsWith(const StringViewBase& prefix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
    {
        if (prefix.IsEmpty() || Length() < prefix.Length())
            return false;
        if (searchCase == StringSearchCase::IgnoreCase)
            return StringUtils::CompareIgnoreCase(this->GetText(), *prefix, prefix.Length()) == 0;
        return StringUtils::Compare(this->GetText(), *prefix, prefix.Length()) == 0;
    }

    bool EndsWith(const StringViewBase& suffix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
    {
        if (suffix.IsEmpty() || Length() < suffix.Length())
            return false;
        if (searchCase == StringSearchCase::IgnoreCase)
            return StringUtils::CompareIgnoreCase(&(*this)[Length() - suffix.Length()], *suffix) == 0;
        return StringUtils::Compare(&(*this)[Length() - suffix.Length()], *suffix) == 0;
    }
};

/// <summary>
/// Represents static text view as a sequence of UTF-16 characters. Characters sequence might not be null-terminated.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API StringView : public StringViewBase<Char>
{
public:

    /// <summary>
    /// Instance of the empty string.
    /// </summary>
    static StringView Empty;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="StringView"/> class.
    /// </summary>
    constexpr StringView()
    {
        _data = nullptr;
        _length = 0;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringView"/> class.
    /// </summary>
    /// <param name="str">The reference to the string.</param>
    StringView(const String& str);

    /// <summary>
    /// Initializes a new instance of the <see cref="StringView"/> class.
    /// </summary>
    /// <param name="str">The reference to the static string.</param>
    constexpr StringView(const StringView& str)
    {
        _data = str._data;
        _length = str._length;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringView"/> class.
    /// </summary>
    /// <param name="str">The characters sequence.</param>
    StringView(const Char* str)
    {
        _data = str;
        _length = StringUtils::Length(str);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringView"/> class.
    /// </summary>
    /// <param name="str">The characters sequence.</param>
    /// <param name="length">The characters sequence length (excluding null-terminator character).</param>
    constexpr StringView(const Char* str, int32 length)
    {
        _data = str;
        _length = length;
    }

public:

    /// <summary>
    /// Assigns the static string.
    /// </summary>
    /// <param name="str">The other string.</param>
    /// <returns>The reference to this object.</returns>
    FORCE_INLINE StringView& operator=(const Char* str)
    {
        _data = str;
        _length = StringUtils::Length(str);
        return *this;
    }

    /// <summary>
    /// Assigns the static string.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns>The reference to this object.</returns>
    FORCE_INLINE constexpr StringView& operator=(const StringView& other)
    {
        if (this != &other)
        {
            _data = other._data;
            _length = other._length;
        }
        return *this;
    }

    /// <summary>
    /// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator==(const StringView& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) == 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator!=(const StringView& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) != 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator==(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other ? other : TEXT("")) == 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator!=(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other ? other : TEXT("")) != 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
    bool operator==(const String& other) const;

    /// <summary>
    /// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
    bool operator!=(const String& other) const;

public:

    /// <summary>
    /// Gets the left most given number of characters.
    /// </summary>
    /// <param name="count">The characters count.</param>
    /// <returns>The substring.</returns>
    StringView Left(int32 count) const;

    /// <summary>
    /// Gets the string of characters from the right (end of the string).
    /// </summary>
    /// <param name="count">The characters count.</param>
    /// <returns>The substring.</returns>
    StringView Right(int32 count) const;

    /// <summary>
    /// Retrieves substring created from characters starting from startIndex to the String end.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <returns>The substring created from String data.</returns>
    StringView Substring(int32 startIndex) const;

    /// <summary>
    /// Retrieves substring created from characters starting from start index.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <param name="count">The amount of characters to retrieve.</param>
    /// <returns>The substring created from String data.</returns>
    StringView Substring(int32 startIndex, int32 count) const;

public:

    String ToString() const;
    StringAnsi ToStringAnsi() const;
};

inline uint32 GetHash(const StringView& key)
{
    return StringUtils::GetHashCode(key.Get(), key.Length());
}

bool FLAXENGINE_API operator==(const String& a, const StringView& b);
bool FLAXENGINE_API operator!=(const String& a, const StringView& b);

namespace fmt
{
    template<>
    struct formatter<StringView, Char>
    {
        template<typename ParseContext>
        auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const StringView& v, FormatContext& ctx) -> decltype(ctx.out())
        {
            return fmt::internal::copy(v.Get(), v.Get() + v.Length(), ctx.out());
        }
    };
}

/// <summary>
/// Represents static text view as a sequence of ANSI characters. Characters sequence might not be null-terminated.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API StringAnsiView : public StringViewBase<char>
{
public:

    /// <summary>
    /// Instance of the empty string.
    /// </summary>
    static StringAnsiView Empty;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="StringView"/> class.
    /// </summary>
    constexpr StringAnsiView()
    {
        _data = nullptr;
        _length = 0;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsiView"/> class.
    /// </summary>
    /// <param name="str">The reference to the string.</param>
    StringAnsiView(const StringAnsi& str);

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsiView"/> class.
    /// </summary>
    /// <param name="str">The reference to the static string.</param>
    constexpr StringAnsiView(const StringAnsiView& str)
    {
        _data = str._data;
        _length = str._length;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsiView"/> class.
    /// </summary>
    /// <param name="str">The characters sequence.</param>
    StringAnsiView(const char* str)
    {
        _data = str;
        _length = StringUtils::Length(str);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsiView"/> class.
    /// </summary>
    /// <param name="str">The characters sequence.</param>
    /// <param name="length">The characters sequence length (excluding null-terminator character).</param>
    constexpr StringAnsiView(const char* str, int32 length)
    {
        _data = str;
        _length = length;
    }

public:

    /// <summary>
    /// Assigns the static string.
    /// </summary>
    /// <param name="str">The other string.</param>
    /// <returns>The reference to this object.</returns>
    FORCE_INLINE StringAnsiView& operator=(const char* str)
    {
        _data = str;
        _length = StringUtils::Length(str);
        return *this;
    }

    /// <summary>
    /// Assigns the static string.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns>The reference to this object.</returns>
    FORCE_INLINE constexpr StringAnsiView& operator=(const StringAnsiView& other)
    {
        if (this != &other)
        {
            _data = other._data;
            _length = other._length;
        }
        return *this;
    }

    /// <summary>
    /// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator==(const StringAnsiView& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) == 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator!=(const StringAnsiView& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) != 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator==(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other ? other : "") == 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
    FORCE_INLINE bool operator!=(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other ? other : "") != 0;
    }

    /// <summary>
    /// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
    bool operator==(const StringAnsi& other) const;

    /// <summary>
    /// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
    /// </summary>
    /// <param name="other">The other text.</param>
    /// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
    bool operator!=(const StringAnsi& other) const;

public:

    /// <summary>
    /// Retrieves substring created from characters starting from startIndex to the String end.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <returns>The substring created from String data.</returns>
    StringAnsi Substring(int32 startIndex) const;

    /// <summary>
    /// Retrieves substring created from characters starting from start index.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <param name="count">The amount of characters to retrieve.</param>
    /// <returns>The substring created from String data.</returns>
    StringAnsi Substring(int32 startIndex, int32 count) const;

public:

    String ToString() const;
    StringAnsi ToStringAnsi() const;
};

inline uint32 GetHash(const StringAnsiView& key)
{
    return StringUtils::GetHashCode(key.Get(), key.Length());
}

bool FLAXENGINE_API operator==(const StringAnsi& a, const StringAnsiView& b);
bool FLAXENGINE_API operator!=(const StringAnsi& a, const StringAnsiView& b);

namespace fmt
{
    template<>
    struct formatter<StringAnsiView, char>
    {
        template<typename ParseContext>
        auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const StringAnsiView& v, FormatContext& ctx) -> decltype(ctx.out())
        {
            return fmt::internal::copy(v.Get(), v.Get() + v.Length(), ctx.out());
        }
    };
}

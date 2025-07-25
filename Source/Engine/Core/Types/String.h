// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Core/Formatting.h"

/// <summary>
/// Represents text as a sequence of characters. Container uses a single dynamic memory allocation to store the characters data. Characters sequence is always null-terminated.
/// </summary>
template<typename T>
class StringBase
{
protected:
    T* _data = nullptr;
    int32 _length = 0;

public:
    typedef T CharType;

    /// <summary>
    /// Finalizes an instance of the <see cref="StringBase"/> class.
    /// </summary>
    ~StringBase()
    {
        Platform::Free(_data);
    }

public:
    /// <summary>
    /// Clears this instance. Frees the memory and sets the string to empty.
    /// </summary>
    void Clear()
    {
        Platform::Free(_data);
        _data = nullptr;
        _length = 0;
    }

public:
    /// <summary>
    /// Gets the character at the specific index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The character</returns>
    FORCE_INLINE T& operator[](int32 index)
    {
        ASSERT(index >= 0 && index < _length);
        return _data[index];
    }

    /// <summary>
    /// Gets the character at the specific index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The character</returns>
    FORCE_INLINE const T& operator[](int32 index) const
    {
        ASSERT(index >= 0 && index < _length);
        return _data[index];
    }

public:
    /// <summary>
    /// Lexicographically tests how this string compares to the other given string.
    /// In case sensitive mode 'A' is less than 'a'.
    /// </summary>
    /// <param name="str">The another string test against.</param>
    /// <param name="searchCase">The case sensitivity mode.</param>
    /// <returns>0 if equal, negative number if less than, positive number if greater than.</returns>
    int32 Compare(const StringBase& str, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        if (searchCase == StringSearchCase::CaseSensitive)
            return StringUtils::Compare(this->GetText(), str.GetText());
        return StringUtils::CompareIgnoreCase(this->GetText(), str.GetText());
    }

public:
    /// <summary>
    /// Returns true if string is empty.
    /// </summary>
    /// <returns>True if string is empty, otherwise false.</returns>
    FORCE_INLINE bool IsEmpty() const
    {
        return _length == 0;
    }

    /// <summary>
    /// Returns true if string isn't empty.
    /// </summary>
    /// <returns>True if string has characters, otherwise false.</returns>
    FORCE_INLINE bool HasChars() const
    {
        return _length != 0;
    }

    /// <summary>
    /// Gets the length of the string.
    /// </summary>
    /// <returns>The text length.</returns>
    FORCE_INLINE int32 Length() const
    {
        return _length;
    }

    /// <summary>
    /// Gets the pointer to the string (or null if text is empty).
    /// </summary>
    /// <returns>The string handle.</returns>
    FORCE_INLINE const T* operator*() const
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the string (or null if text is empty).
    /// </summary>
    /// <returns>The string handle.</returns>
    FORCE_INLINE T* operator*()
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the string(or null if text is empty).
    /// </summary>
    /// <returns>The string handle.</returns>
    FORCE_INLINE T* Get()
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the string (or null if text is empty).
    /// </summary>
    /// <returns>The string handle.</returns>
    FORCE_INLINE const T* Get() const
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the string or to the static empty text if string is null. Returned pointer is always valid (read-only).
    /// </summary>
    /// <returns>The string handle.</returns>
    FORCE_INLINE const T* GetText() const
    {
        return _data ? _data : (const T*)TEXT("");
    }

public:
    /// <summary>
    /// Checks whether this string contains the specified substring.
    /// </summary>
    /// <param name="subStr">The string sequence to search for.</param>
    /// <param name="searchCase">The search case sensitivity mode.</param>
    /// <returns>True if the given substring is contained by ths string, otherwise false.</returns>
    FORCE_INLINE bool Contains(const T* subStr, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        return Find(subStr, searchCase) != -1;
    }

    /// <summary>
    /// Checks whether this string contains the specified substring.
    /// </summary>
    /// <param name="subStr">The string sequence to search for.</param>
    /// <param name="searchCase">The search case sensitivity mode.</param>
    /// <returns>True if the given substring is contained by ths string, otherwise false.</returns>
    FORCE_INLINE bool Contains(const StringBase& subStr, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        return Find(*subStr, searchCase) != -1;
    }

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

    /// <summary>
    /// Searches the string starting from beginning for a substring, and returns index into this string of the first found instance.
    /// </summary>
    /// <param name="subStr">The string sequence to search for.</param>
    /// <param name="searchCase">The search case sensitivity mode.</param>
    /// <param name="startPosition">The start character position to search from.</param>
    /// <returns>The index of the found substring or -1 if not found.</returns>
    int32 Find(const T* subStr, StringSearchCase searchCase = StringSearchCase::CaseSensitive, int32 startPosition = -1) const
    {
        if (subStr == nullptr || !_data)
            return -1;
        const T* start = _data;
        if (startPosition != -1)
            start += startPosition < Length() ? startPosition : Length();
        const T* tmp = searchCase == StringSearchCase::IgnoreCase ? StringUtils::FindIgnoreCase(start, subStr) : StringUtils::Find(start, subStr);
        return tmp ? static_cast<int32>(tmp - **this) : -1;
    }

    /// <summary>
    /// Searches the string starting from end for a substring, and returns index into this string of the first found instance.
    /// </summary>
    /// <param name="subStr">The string sequence to search for.</param>
    /// <param name="searchCase">The search case sensitivity mode.</param>
    /// <param name="startPosition">The start character position to search from.</param>
    /// <returns>The index of the found substring or -1 if not found.</returns>
    int32 FindLast(const T* subStr, StringSearchCase searchCase = StringSearchCase::CaseSensitive, int32 startPosition = -1) const
    {
        const int32 subStrLen = StringUtils::Length(subStr);
        if (subStrLen == 0 || !_data)
            return -1;
        if (startPosition == -1)
            startPosition = Length();
        const T* start = _data;
        if (searchCase == StringSearchCase::IgnoreCase)
        {
            for (int32 i = startPosition - subStrLen; i >= 0; i--)
            {
                if (StringUtils::CompareIgnoreCase(start + i, subStr, subStrLen) == 0)
                    return i;
            }
        }
        else
        {
            for (int32 i = startPosition - subStrLen; i >= 0; i--)
            {
                if (StringUtils::Compare(start + i, subStr, subStrLen) == 0)
                    return i;
            }
        }
        return -1;
    }

    /// <summary>
    /// Searches the string starting from beginning for a substring, and returns index into this string of the first found instance.
    /// </summary>
    /// <param name="subStr">The string sequence to search for.</param>
    /// <param name="searchCase">The search case sensitivity mode.</param>
    /// <param name="startPosition">The start character position to search from.</param>
    /// <returns>The index of the found substring or -1 if not found.</returns>
    FORCE_INLINE int32 Find(const StringBase& subStr, StringSearchCase searchCase = StringSearchCase::CaseSensitive, int32 startPosition = -1) const
    {
        return Find(subStr.Get(), searchCase, startPosition);
    }

    /// <summary>
    /// Searches the string starting from end for a substring, and returns index into this string of the first found instance.
    /// </summary>
    /// <param name="subStr">The string sequence to search for.</param>
    /// <param name="searchCase">The search case sensitivity mode.</param>
    /// <param name="startPosition">The start character position to search from.</param>
    /// <returns>The index of the found substring or -1 if not found.</returns>
    FORCE_INLINE int32 FindLast(const StringBase& subStr, StringSearchCase searchCase = StringSearchCase::CaseSensitive, int32 startPosition = -1) const
    {
        return FindLast(subStr.Get(), searchCase, startPosition);
    }

    /// <summary>
    /// Searches the string for the first character that matches the character specified.
    /// </summary>
    /// <param name="c">The character to search for.</param>
    /// <param name="startPos">The start position of the search. The search only includes characters at or after position, ignoring any possible occurrences before it.</param>
    /// <returns>The position of the first character that matches. If no matches are found, the function returns -1.</returns>
    int32 FindFirstOf(T c, int32 startPos = 0) const
    {
        for (int32 i = startPos; i < Length(); i++)
        {
            if (_data[i] == c)
                return i;
        }
        return -1;
    }

    /// <summary>
    /// Searches the string for the first character that matches any of the characters specified in its arguments.
    /// </summary>
    /// <param name="str">The pointer to the array of characters that are searched for.</param>
    /// <param name="startPos">The start position of the search. The search only includes characters at or after position, ignoring any possible occurrences before it.</param>
    /// <returns>The position of the first character that matches. If no matches are found, the function returns -1.</returns>
    int32 FindFirstOf(const T* str, int32 startPos = 0) const
    {
        if (!str)
            return -1;
        for (int32 i = startPos; i < _length; i++)
        {
            const T c = _data[i];
            const T* s = str;
            while (*s)
            {
                if (c == *s)
                    return i;
                s++;
            }
        }
        return -1;
    }

    /// <summary>
    /// Reserves space for the characters. Discards the existing contents. Caller is responsible to initialize contents (excluding null-termination character).
    /// </summary>
    /// <param name="length">The amount of characters to reserve space for (excluding null-terminated character).</param>
    void ReserveSpace(int32 length)
    {
        ASSERT(length >= 0);
        if (length == _length)
            return;
        Platform::Free(_data);
        if (length != 0)
        {
            _data = (T*)Platform::Allocate((length + 1) * sizeof(T), 16);
            _data[length] = 0;
        }
        else
        {
            _data = nullptr;
        }
        _length = length;
    }

public:
    bool StartsWith(T c, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        const int32 length = Length();
        if (searchCase == StringSearchCase::CaseSensitive)
            return length > 0 && _data[0] == c;
        return length > 0 && StringUtils::ToLower(_data[0]) == StringUtils::ToLower(c);
    }

    bool EndsWith(T c, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        const int32 length = Length();
        if (searchCase == StringSearchCase::CaseSensitive)
            return length > 0 && _data[length - 1] == c;
        return length > 0 && StringUtils::ToLower(_data[length - 1]) == StringUtils::ToLower(c);
    }

    bool StartsWith(const StringBase& prefix, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        if (prefix.IsEmpty())
            return true;
        if (Length() < prefix.Length())
            return false;
        if (searchCase == StringSearchCase::IgnoreCase)
            return StringUtils::CompareIgnoreCase(this->GetText(), *prefix, prefix.Length()) == 0;
        return StringUtils::Compare(this->GetText(), *prefix, prefix.Length()) == 0;
    }

    bool EndsWith(const StringBase& suffix, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
    {
        if (suffix.IsEmpty())
            return true;
        if (Length() < suffix.Length())
            return false;
        if (searchCase == StringSearchCase::IgnoreCase)
            return StringUtils::CompareIgnoreCase(&(*this)[Length() - suffix.Length()], *suffix) == 0;
        return StringUtils::Compare(&(*this)[Length() - suffix.Length()], *suffix) == 0;
    }

    int32 Replace(T searchChar, T replacementChar, StringSearchCase searchCase = StringSearchCase::CaseSensitive)
    {
        int32 replacedChars = 0;
        const int32 length = Length();
        if (searchCase == StringSearchCase::IgnoreCase)
        {
            const T toCompare = StringUtils::ToLower(searchChar);
            for (int32 i = 0; i < length; i++)
            {
                if (StringUtils::ToLower(_data[i]) == toCompare)
                {
                    _data[i] = replacementChar;
                    replacedChars++;
                }
            }
        }
        else
        {
            for (int32 i = 0; i < length; i++)
            {
                if (_data[i] == searchChar)
                {
                    _data[i] = replacementChar;
                    replacedChars++;
                }
            }
        }
        return replacedChars;
    }

    /// <summary>
    /// Replaces all occurences of searchText within current string with replacementText.
    /// </summary>
    /// <param name="searchText">String to search for. If empty or null no replacements are done.</param>
    /// <param name="replacementText">String to replace with. Null is treated as empty string.</param>
    /// <returns>Number of replacements made. (In case-sensitive mode if search text and replacement text are equal no replacements are done, and zero is returned.)</returns>
    int32 Replace(const T* searchText, const T* replacementText, StringSearchCase searchCase = StringSearchCase::CaseSensitive)
    {
        const int32 searchTextLength = StringUtils::Length(searchText);
        const int32 replacementTextLength = StringUtils::Length(replacementText);
        return Replace(searchText, searchTextLength, replacementText, replacementTextLength, searchCase);
    }

    /// <summary>
    /// Replaces all occurences of searchText within current string with replacementText.
    /// </summary>
    /// <param name="searchText">String to search for.</param>
    /// <param name="searchTextLength">Length of searchText. Must be greater than zero.</param>
    /// <param name="replacementText">String to replace with. Null is treated as empty string.</param>
    /// <param name="replacementTextLength">Length of replacementText.</param>
    /// <returns>Number of replacements made (in other words number of occurences of searchText).</returns>
    int32 Replace(const T* searchText, int32 searchTextLength, const T* replacementText, int32 replacementTextLength, StringSearchCase searchCase = StringSearchCase::CaseSensitive)
    {
        if (!HasChars() || searchTextLength == 0)
            return 0;

        int32 replacedCount = 0;

        if (searchTextLength == replacementTextLength)
        {
            T* pos = (T*)(searchCase == StringSearchCase::IgnoreCase ? StringUtils::FindIgnoreCase(_data, searchText) : StringUtils::Find(_data, searchText));
            while (pos != nullptr)
            {
                replacedCount++;

                for (int32 i = 0; i < replacementTextLength; i++)
                    pos[i] = replacementText[i];

                if (pos + searchTextLength - **this < Length())
                    pos = (T*)(searchCase == StringSearchCase::IgnoreCase ? StringUtils::FindIgnoreCase(pos + searchTextLength, searchText) : StringUtils::Find(pos + searchTextLength, searchText));
                else
                    break;
            }
        }
        else if (Contains(searchText, searchCase))
        {
            T* readPosition = _data;
            T* searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ? StringUtils::FindIgnoreCase(readPosition, searchText) : StringUtils::Find(readPosition, searchText));
            while (searchPosition != nullptr)
            {
                replacedCount++;
                readPosition = searchPosition + searchTextLength;
                searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ? StringUtils::FindIgnoreCase(readPosition, searchText) : StringUtils::Find(readPosition, searchText));
            }

            const auto oldLength = _length;
            const auto oldData = _data;
            _length += replacedCount * (replacementTextLength - searchTextLength);
            _data = (T*)Platform::Allocate((_length + 1) * sizeof(T), 16);

            T* writePosition = _data;
            readPosition = oldData;
            searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ? StringUtils::FindIgnoreCase(readPosition, searchText) : StringUtils::Find(readPosition, searchText));
            while (searchPosition != nullptr)
            {
                const int32 writeOffset = (int32)(searchPosition - readPosition);
                Platform::MemoryCopy(writePosition, readPosition, writeOffset * sizeof(T));
                writePosition += writeOffset;

                if (replacementTextLength > 0)
                    Platform::MemoryCopy(writePosition, replacementText, replacementTextLength * sizeof(T));
                writePosition += replacementTextLength;

                readPosition = searchPosition + searchTextLength;
                searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ? StringUtils::FindIgnoreCase(readPosition, searchText) : StringUtils::Find(readPosition, searchText));
            }

            const int32 writeOffset = (int32)(oldData - readPosition) + oldLength;
            Platform::MemoryCopy(writePosition, readPosition, writeOffset * sizeof(T));

            _data[_length] = 0;
            Platform::Free(oldData);
        }

        return replacedCount;
    }

    /// <summary>
    /// Reverses the string.
    /// </summary>
    void Reverse()
    {
        T c;
        int32 tmp, count = _length, end = count / 2;
        for (int32 i = 0; i < end; i++)
        {
            tmp = count - i - 2;
            c = _data[i];
            _data[i] = _data[tmp];
            _data[tmp] = c;
        }
    }

    /// <summary>
    /// Resizes string contents.
    /// </summary>
    /// <param name="length">New length of the string.</param>
    void Resize(int32 length)
    {
        ASSERT(length >= 0);
        if (_length != length)
        {
            const auto oldData = _data;
            const auto minLength = _length < length ? _length : length;
            _length = length;
            _data = (T*)Platform::Allocate((length + 1) * sizeof(T), 16);
            Platform::MemoryCopy(_data, oldData, minLength * sizeof(T));
            _data[length] = 0;
            Platform::Free(oldData);
        }
    }
};

/// <summary>
/// Represents text as a sequence of UTF-16 characters. Container uses a single dynamic memory allocation to store the characters data. Characters sequence is always null-terminated.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API String : public StringBase<Char>
{
public:
    /// <summary>
    /// Instance of the empty string.
    /// </summary>
    static const String Empty;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    String() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The reference to the string.</param>
    String(const String& str)
        : String()
    {
        Set(str.Get(), str.Length());
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The double reference to the string.</param>
    String(String&& str) noexcept
    {
        _data = str._data;
        _length = str._length;
        str._data = nullptr;
        str._length = 0;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The reference to the string.</param>
    explicit String(const StringAnsi& str);

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">ANSI string</param>
    explicit String(const char* str)
        : String(str, StringUtils::Length(str))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The ANSI string.</param>
    /// <param name="length">The ANSI string length.</param>
    explicit String(const char* str, int32 length)
        : String()
    {
        Set(str, length);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The UTF-16 string.</param>
    String(const Char* str)
        : String()
    {
        Set(str, StringUtils::Length(str));
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The UTF-16 string.</param>
    /// <param name="length">The UTF-16 string length.</param>
    String(const Char* str, int32 length)
        : String()
    {
        Set(str, length);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The other string.</param>
    String(const StringView& str);

    /// <summary>
    /// Initializes a new instance of the <see cref="String"/> class.
    /// </summary>
    /// <param name="str">The other string.</param>
    explicit String(const StringAnsiView& str);

public:
    /// <summary>
    /// Sets an array of characters to the string.
    /// </summary>
    /// <param name="chars">The pointer to the start of an array of characters to set (UTF-16). This array need not be null-terminated, and null characters are not treated specially.</param>
    /// <param name="length">The number of characters to assign.</param>
    void Set(const Char* chars, int32 length);

    /// <summary>
    /// Sets an array of characters to the string.
    /// </summary>
    /// <param name="chars">The pointer to the start of an array of characters to set (ANSI). This array need not be null-terminated, and null characters are not treated specially.</param>
    /// <param name="length">The number of characters to assign.</param>
    void Set(const char* chars, int32 length);

    /// <summary>
    /// Sets an array of characters to the string.
    /// </summary>
    /// <param name="chars">The pointer to the start of an array of characters to set (UTF-8). This array need not be null-terminated, and null characters are not treated specially.</param>
    /// <param name="length">The number of characters to assign.</param>
    void SetUTF8(const char* chars, int32 length);

    /// <summary>
    /// Appends an array of characters to the string.
    /// </summary>
    /// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
    /// <param name="count">The number of characters to append.</param>
    void Append(const Char* chars, int32 count);

    /// <summary>
    /// Appends an array of characters to the string.
    /// </summary>
    /// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
    /// <param name="count">The number of characters to append.</param>
    void Append(const char* chars, int32 count);

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="text">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE String& Append(const String& text)
    {
        Append(text.Get(), text.Length());
        return *this;
    }

    /// <summary>
    /// Appends the specified character to this string.
    /// </summary>
    /// <param name="c">The character to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE String& Append(const Char c)
    {
        ASSERT_LOW_LAYER(c != 0);
        Append(&c, 1);
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE String& operator+=(const Char* str)
    {
        Append(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE String& operator+=(const char* str)
    {
        Append(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Appends the specified character to this string.
    /// </summary>
    /// <param name="c">The character to append.</param>
    /// <returns>The reference to this string.</returns>
    String& operator+=(const Char c)
    {
        ASSERT_LOW_LAYER(c != 0);
        Append(&c, 1);
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE String& operator+=(const String& str)
    {
        Append(str.Get(), str.Length());
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    String& operator+=(const StringView& str);

    /// <summary>
    /// Concatenates a string with a character.
    /// </summary>
    /// <param name="a">The left string.</param>
    /// <param name="b">The right character.</param>
    /// <returns>The concatenated string.</returns>
    friend String operator+(const String& a, const Char b)
    {
        String result;
        result._length = a.Length() + 1;
        result._data = (Char*)Platform::Allocate((result._length + 1) * sizeof(Char), 16);
        Platform::MemoryCopy(result._data, a.Get(), a.Length() * sizeof(Char));
        result._data[a.Length()] = b;
        result._data[result._length] = 0;
        return result;
    }

public:
    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="s">The other string.</param>
    /// <returns>The reference to this.</returns>
    FORCE_INLINE String& operator=(String&& s) noexcept
    {
        if (this != &s)
        {
            Platform::Free(_data);
            _data = s._data;
            _length = s._length;
            s._data = nullptr;
            s._length = 0;
        }
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="s">The other string.</param>
    /// <returns>The reference to this.</returns>
    FORCE_INLINE String& operator=(const String& s)
    {
        if (this != &s)
            Set(s.Get(), s.Length());
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="s">The other string.</param>
    /// <returns>The reference to this.</returns>
    String& operator=(const StringView& s);

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="str">The other string.</param>
    /// <returns>The reference to this.</returns>
    String& operator=(const Char* str)
    {
        if (_data != str)
            Set(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="str">The other string.</param>
    /// <returns>The reference to this.</returns>
    String& operator=(const char* str)
    {
        Set(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="c">The other character.</param>
    /// <returns>The reference to this.</returns>
    String& operator=(const Char c)
    {
        Set(&c, 1);
        return *this;
    }

public:
    // @formatter:off
    FORCE_INLINE friend String operator+(const String& a, const String& b)
    {
        return ConcatStrings<const String&, const String&>(a, b);
    }
    FORCE_INLINE friend String operator+(String&& a, const String& b)
    {
        return ConcatStrings(MoveTemp(a), b);
    }
    FORCE_INLINE friend String operator+(const String& a, String&& b)
    {
        return ConcatStrings<const String&, String&&>(a, MoveTemp(b));
    }
    FORCE_INLINE friend String operator+(String&& a, String&& b)
    {
        return ConcatStrings<String&&, String&&>(MoveTemp(a), MoveTemp(b));
    }
    FORCE_INLINE friend String operator+(const Char* a, const String& b)
    {
        return ConcatCharsToString<const String&>(a, b);
    }
    FORCE_INLINE friend String operator+(const Char* a, String&& b)
    {
        return ConcatCharsToString<String&&>(a, MoveTemp(b));
    }
    FORCE_INLINE friend String operator+(const String& a, const Char* b)
    {
        return ConcatStringToChars<const String&>(a, b);
    }
    FORCE_INLINE friend String operator+(String&& a, const Char* b)
    {
        return ConcatStringToChars<String&&>(MoveTemp(a), b);
    }
    FORCE_INLINE friend String operator+(const String& a, char b)
    {
        return a + String(&b, 1);
    }
    FORCE_INLINE friend String operator+(String&& a, char b)
    {
        return a + String(&b, 1);
    }
    FORCE_INLINE bool operator<=(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) <= 0;
    }
    FORCE_INLINE bool operator<(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) < 0;
    }
    FORCE_INLINE bool operator<(const String& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) < 0;
    }
    FORCE_INLINE bool operator>=(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) >= 0;
    }
    FORCE_INLINE bool operator>(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) > 0;
    }
    FORCE_INLINE bool operator>(const String& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) > 0;
    }
    FORCE_INLINE bool operator==(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) == 0;
    }
    FORCE_INLINE bool operator==(const String& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) == 0;
    }
    FORCE_INLINE bool operator!=(const Char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) != 0;
    }
    FORCE_INLINE bool operator!=(const String& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) != 0;
    }
    FORCE_INLINE bool operator<=(const String& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) <= 0;
    }
    FORCE_INLINE bool operator>=(const String& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) >= 0;
    }
    // @formatter:on

public:
    /// <summary>
    /// Checks if string contains only ANSI characters.
    /// </summary>
    /// <returns>True if contains only ANSI characters, otherwise false.</returns>
    bool IsANSI() const;

public:
    using StringBase::StartsWith;
    bool StartsWith(const StringView& prefix, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const;

    using StringBase::EndsWith;
    bool EndsWith(const StringView& suffix, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const;

    /// <summary>
    /// Converts all uppercase characters to lowercase.
    /// </summary>
    /// <returns>The lowercase string.</returns>
    String ToLower() const;

    /// <summary>
    /// Converts all lowercase characters to uppercase.
    /// </summary>
    /// <returns>The uppercase string.</returns>
    String ToUpper() const;

    /// <summary>
    /// Gets the left most given number of characters.
    /// </summary>
    /// <param name="count">The characters count.</param>
    /// <returns>The substring.</returns>
    FORCE_INLINE String Left(int32 count) const
    {
        const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
        return String(**this, countClamped);
    }

    /// <summary>
    /// Gets the string of characters from the right (end of the string).
    /// </summary>
    /// <param name="count">The characters count.</param>
    /// <returns>The substring.</returns>
    FORCE_INLINE String Right(int32 count) const
    {
        const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
        return String(**this + Length() - countClamped);
    }

    /// <summary>
    /// Retrieves substring created from characters starting from startIndex to the String end.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <returns>The substring created from String data.</returns>
    String Substring(int32 startIndex) const
    {
        ASSERT(startIndex >= 0 && startIndex < Length());
        return String(_data + startIndex, _length - startIndex);
    }

    /// <summary>
    /// Retrieves substring created from characters starting from start index.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <param name="count">The amount of characters to retrieve.</param>
    /// <returns>The substring created from String data.</returns>
    String Substring(int32 startIndex, int32 count) const
    {
        ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
        return String(_data + startIndex, count);
    }

    /// <summary>
    /// Inserts string into current string instance at given location.
    /// </summary>
    /// <param name="startIndex">The index of the first character to insert.</param>
    /// <param name="other">The string to insert.</param>
    void Insert(int32 startIndex, const String& other);

    /// <summary>
    /// Removes characters from the string at given location until the end.
    /// </summary>
    /// <param name="startIndex">The index of the first character to remove.</param>
    void Remove(int32 startIndex)
    {
        Remove(startIndex, _length - startIndex);
    }

    /// <summary>
    /// Removes characters from the string at given location and length.
    /// </summary>
    /// <param name="startIndex">The index of the first character to remove.</param>
    /// <param name="length">The amount of characters to remove.</param>
    void Remove(int32 startIndex, int32 length);

    /// <summary>
    /// Splits a string into substrings that are based on the character.
    /// </summary>
    /// <param name="c">A character that delimits the substrings in this string.</param>
    /// <param name="results">An array whose elements contain the substrings from this instance that are delimited by separator.</param>
    void Split(Char c, Array<String, HeapAllocation>& results) const;

    /// <summary>
    /// Gets the first line of the text (searches for the line terminator char).
    /// </summary>
    /// <returns>The single line of text.</returns>
    String GetFirstLine() const;

    /// <summary>
    /// Trims the string to the first null terminator character in the characters buffer.
    /// </summary>
    void TrimToNullTerminator();

    /// <summary>
    /// Removes trailing whitespace characters from end and begin of the string.
    /// </summary>
    String TrimTrailing() const;

public:
    /// <summary>
    /// Formats the message and gets it as a string.
    /// </summary>
    /// <param name="format">The format string.</param>
    /// <param name="args">The custom arguments.</param>
    /// <returns>The formatted text.</returns>
    template<typename... Args>
    static String Format(const Char* format, const Args& ... args)
    {
        fmt_flax::allocator allocator;
        fmt_flax::memory_buffer buffer(allocator);
        fmt_flax::format(buffer, format, args...);
        return String(buffer.data(), (int32)buffer.size());
    }

public:
    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    String& operator/=(const Char* str);

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    String& operator/=(const char* str);

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="c">The character to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    String& operator/=(Char c);

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    FORCE_INLINE String& operator/=(const String& str)
    {
        return operator/=(*str);
    }

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    String& operator/=(const StringView& str);

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    FORCE_INLINE String operator/(const Char* str) const
    {
        return String(*this) /= str;
    }

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    FORCE_INLINE String operator/(const char* str) const
    {
        return String(*this) /= str;
    }

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="c">The character to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    FORCE_INLINE String operator/(const Char c) const
    {
        return String(*this) /= c;
    }

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    FORCE_INLINE String operator/(const String& str) const
    {
        return String(*this) /= str;
    }

    /// <summary>
    /// Concatenates this path with given path ensuring the '/' character is used between them.
    /// </summary>
    /// <param name="str">The string to be concatenated onto the end of this.</param>
    /// <returns>The combined path.</returns>
    FORCE_INLINE String operator/(const StringView& str) const
    {
        return String(*this) /= str;
    }

public:
    String ToString() const
    {
        return *this;
    }

    StringAnsi ToStringAnsi() const;

private:
    template<typename T1, typename T2>
    static String ConcatStrings(T1 left, T2 right)
    {
        if (left.IsEmpty())
            return MoveTemp(right);
        if (right.IsEmpty())
            return MoveTemp(left);

        const Char* leftStr = left.Get();
        const int32 leftLen = left.Length();
        const Char* rightStr = right.Get();
        const int32 rightLen = right.Length();

        String result;
        result.ReserveSpace(leftLen + rightLen);
        Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(Char));
        Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(Char));

        return result;
    }

    template<typename T>
    static String ConcatCharsToString(const Char* left, T right)
    {
        if (!left || !*left)
            return MoveTemp(right);

        const Char* leftStr = left;
        const int32 leftLen = StringUtils::Length(left);
        const Char* rightStr = right.Get();
        const int32 rightLen = right.Length();

        String result;
        result.ReserveSpace(leftLen + rightLen);
        Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(Char));
        Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(Char));

        return result;
    }

    template<typename T>
    static String ConcatStringToChars(T left, const Char* right)
    {
        if (!right || !*right)
            return MoveTemp(left);

        const Char* leftStr = left.Get();
        const int32 leftLen = left.Length();
        const Char* rightStr = right;
        const int32 rightLen = StringUtils::Length(right);

        String result;
        result.ReserveSpace(leftLen + rightLen);
        Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(Char));
        Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(Char));

        return result;
    }
};

inline uint32 GetHash(const String& key)
{
    return StringUtils::GetHashCode(key.Get());
}

namespace fmt
{
    template<>
    struct formatter<String, Char>
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

/// <summary>
/// Represents text as a sequence of ANSI characters. Container uses a single dynamic memory allocation to store the characters data. Characters sequence is always null-terminated.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API StringAnsi : public StringBase<char>
{
public:
    /// <summary>
    /// Instance of the empty string.
    /// </summary>
    static StringAnsi Empty;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    StringAnsi() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The reference to the string.</param>
    StringAnsi(const StringAnsi& str)
    {
        Set(str.Get(), str.Length());
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The double reference to the string.</param>
    StringAnsi(StringAnsi&& str) noexcept
    {
        _data = str._data;
        _length = str._length;
        str._data = nullptr;
        str._length = 0;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The reference to the string.</param>
    explicit StringAnsi(const String& str)
    {
        Set(str.Get(), str.Length());
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">ANSI string</param>
    explicit StringAnsi(const Char* str)
        : StringAnsi(str, StringUtils::Length(str))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The ANSI string.</param>
    /// <param name="length">The ANSI string length.</param>
    explicit StringAnsi(const char* str, int32 length)
    {
        Set(str, length);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The UTF-16 string.</param>
    StringAnsi(const char* str)
        : StringAnsi(str, StringUtils::Length(str))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The UTF-16 string.</param>
    /// <param name="length">The UTF-16 string length.</param>
    StringAnsi(const Char* str, int32 length)
    {
        Set(str, length);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The other string.</param>
    explicit StringAnsi(const StringView& str);

    /// <summary>
    /// Initializes a new instance of the <see cref="StringAnsi"/> class.
    /// </summary>
    /// <param name="str">The other string.</param>
    explicit StringAnsi(const StringAnsiView& str);

public:
    /// <summary>
    /// Sets an array of characters to the string.
    /// </summary>
    /// <param name="chars">The pointer to the start of an array of characters to set (ANSI). This array need not be null-terminated, and null characters are not treated specially.</param>
    /// <param name="length">The number of characters to assign.</param>
    void Set(const char* chars, int32 length);

    /// <summary>
    /// Sets an array of characters to the string.
    /// </summary>
    /// <param name="chars">The pointer to the start of an array of characters to set (UTF-16). This array need not be null-terminated, and null characters are not treated specially.</param>
    /// <param name="length">The number of characters to assign.</param>
    void Set(const Char* chars, int32 length);

    /// <summary>
    /// Appends an array of characters to the string.
    /// </summary>
    /// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
    /// <param name="count">The number of characters to append.</param>
    void Append(const char* chars, int32 count);

    /// <summary>
    /// Appends an array of characters to the string.
    /// </summary>
    /// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
    /// <param name="count">The number of characters to append.</param>
    void Append(const Char* chars, int32 count);

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="text">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE StringAnsi& Append(const StringAnsi& text)
    {
        Append(text.Get(), text.Length());
        return *this;
    }

    /// <summary>
    /// Appends the specified character to this string.
    /// </summary>
    /// <param name="c">The character to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE StringAnsi& Append(const char c)
    {
        ASSERT_LOW_LAYER(c != 0);
        Append(&c, 1);
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE StringAnsi& operator+=(const char* str)
    {
        Append(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE StringAnsi& operator+=(const Char* str)
    {
        Append(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Appends the specified character to this string.
    /// </summary>
    /// <param name="c">The character to append.</param>
    /// <returns>The reference to this string.</returns>
    StringAnsi& operator+=(const char c)
    {
        ASSERT_LOW_LAYER(c != 0);
        Append(&c, 1);
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    FORCE_INLINE StringAnsi& operator+=(const StringAnsi& str)
    {
        Append(str.Get(), str.Length());
        return *this;
    }

    /// <summary>
    /// Appends the specified text to this string.
    /// </summary>
    /// <param name="str">The text to append.</param>
    /// <returns>The reference to this string.</returns>
    StringAnsi& operator+=(const StringAnsiView& str);

    /// <summary>
    /// Concatenates a string with a character.
    /// </summary>
    /// <param name="a">The left string.</param>
    /// <param name="b">The right character.</param>
    /// <returns>The concatenated string.</returns>
    friend StringAnsi operator+(const StringAnsi& a, const char b)
    {
        StringAnsi result;
        result._length = a.Length() + 1;
        result._data = (char*)Platform::Allocate((result._length + 1) * sizeof(char), 16);
        Platform::MemoryCopy(result._data, a.Get(), a.Length() * sizeof(char));
        result._data[a.Length()] = b;
        result._data[result._length] = 0;
        return result;
    }

public:
    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="s">The other string.</param>
    /// <returns>The reference to this.</returns>
    FORCE_INLINE StringAnsi& operator=(StringAnsi&& s) noexcept
    {
        if (this != &s)
        {
            Platform::Free(_data);
            _data = s._data;
            _length = s._length;
            s._data = nullptr;
            s._length = 0;
        }
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="s">The other string.</param>
    /// <returns>The reference to this.</returns>
    FORCE_INLINE StringAnsi& operator=(const StringAnsi& s)
    {
        if (this != &s)
            Set(s.Get(), s.Length());
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="s">The other string.</param>
    /// <returns>The reference to this.</returns>
    StringAnsi& operator=(const StringAnsiView& s);

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="str">The other string.</param>
    /// <returns>The reference to this.</returns>
    StringAnsi& operator=(const char* str)
    {
        if (_data != str)
            Set(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="str">The other string.</param>
    /// <returns>The reference to this.</returns>
    StringAnsi& operator=(const Char* str)
    {
        Set(str, StringUtils::Length(str));
        return *this;
    }

    /// <summary>
    /// Sets the text value.
    /// </summary>
    /// <param name="c">The other character.</param>
    /// <returns>The reference to this.</returns>
    StringAnsi& operator=(const char c)
    {
        Set(&c, 1);
        return *this;
    }

public:
    // @formatter:off
    FORCE_INLINE friend StringAnsi operator+(const StringAnsi& a, const StringAnsi& b)
    {
        return ConcatStrings<const StringAnsi&, const StringAnsi&>(a, b);
    }
    FORCE_INLINE friend StringAnsi operator+(StringAnsi&& a, const StringAnsi& b)
    {
        return ConcatStrings(MoveTemp(a), b);
    }
    FORCE_INLINE friend StringAnsi operator+(const StringAnsi& a, StringAnsi&& b)
    {
        return ConcatStrings<const StringAnsi&, StringAnsi&&>(a, MoveTemp(b));
    }
    FORCE_INLINE friend StringAnsi operator+(StringAnsi&& a, StringAnsi&& b)
    {
        return ConcatStrings<StringAnsi&&, StringAnsi&&>(MoveTemp(a), MoveTemp(b));
    }
    FORCE_INLINE friend StringAnsi operator+(const char* a, const StringAnsi& b)
    {
        return ConcatCharsToString<const StringAnsi&>(a, b);
    }
    FORCE_INLINE friend StringAnsi operator+(const char* a, StringAnsi&& b)
    {
        return ConcatCharsToString<StringAnsi&&>(a, MoveTemp(b));
    }
    FORCE_INLINE friend StringAnsi operator+(const StringAnsi& a, const char* b)
    {
        return ConcatStringToChars<const StringAnsi&>(a, b);
    }
    FORCE_INLINE friend StringAnsi operator+(StringAnsi&& a, const char* b)
    {
        return ConcatStringToChars<StringAnsi&&>(MoveTemp(a), b);
    }
    FORCE_INLINE bool operator<=(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) <= 0;
    }
    FORCE_INLINE bool operator<(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) < 0;
    }
    FORCE_INLINE bool operator<(const StringAnsi& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) < 0;
    }
    FORCE_INLINE bool operator>=(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) >= 0;
    }
    FORCE_INLINE bool operator>(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) > 0;
    }
    FORCE_INLINE bool operator>(const StringAnsi& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) > 0;
    }
    FORCE_INLINE bool operator==(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) == 0;
    }
    FORCE_INLINE bool operator==(const StringAnsi& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) == 0;
    }
    FORCE_INLINE bool operator!=(const char* other) const
    {
        return StringUtils::Compare(this->GetText(), other) != 0;
    }
    FORCE_INLINE bool operator!=(const StringAnsi& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) != 0;
    }
    FORCE_INLINE bool operator<=(const StringAnsi& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) <= 0;
    }
    FORCE_INLINE bool operator>=(const StringAnsi& other) const
    {
        return StringUtils::Compare(this->GetText(), other.GetText()) >= 0;
    }
    // @formatter:on

public:
    using StringBase::StartsWith;
    bool StartsWith(const StringAnsiView& prefix, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const;

    using StringBase::EndsWith;
    bool EndsWith(const StringAnsiView& suffix, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const;

    /// <summary>
    /// Converts all uppercase characters to lowercase.
    /// </summary>
    /// <returns>The lowercase string.</returns>
    StringAnsi ToLower() const;

    /// <summary>
    /// Converts all lowercase characters to uppercase.
    /// </summary>
    /// <returns>The uppercase string.</returns>
    StringAnsi ToUpper() const;

    /// <summary>
    /// Gets the left most given number of characters.
    /// </summary>
    /// <param name="count">The characters count.</param>
    /// <returns>The substring.</returns>
    FORCE_INLINE StringAnsi Left(int32 count) const
    {
        const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
        return StringAnsi(**this, countClamped);
    }

    /// <summary>
    /// Gets the string of characters from the right (end of the string).
    /// </summary>
    /// <param name="count">The characters count.</param>
    /// <returns>The substring.</returns>
    FORCE_INLINE StringAnsi Right(int32 count) const
    {
        const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
        return StringAnsi(**this + Length() - countClamped);
    }

    /// <summary>
    /// Retrieves substring created from characters starting from startIndex to the String end.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <returns>The substring created from String data.</returns>
    StringAnsi Substring(int32 startIndex) const
    {
        ASSERT(startIndex >= 0 && startIndex < Length());
        return StringAnsi(_data + startIndex, _length - startIndex);
    }

    /// <summary>
    /// Retrieves substring created from characters starting from start index.
    /// </summary>
    /// <param name="startIndex">The index of the first character to subtract.</param>
    /// <param name="count">The amount of characters to retrieve.</param>
    /// <returns>The substring created from String data.</returns>
    StringAnsi Substring(int32 startIndex, int32 count) const
    {
        ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
        return StringAnsi(_data + startIndex, count);
    }

    /// <summary>
    /// Inserts string into current string instance at given location.
    /// </summary>
    /// <param name="startIndex">The index of the first character to insert.</param>
    /// <param name="other">The string to insert.</param>
    void Insert(int32 startIndex, const StringAnsi& other);

    /// <summary>
    /// Removes characters from the string at given location and length.
    /// </summary>
    /// <param name="startIndex">The index of the first character to remove.</param>
    /// <param name="length">The amount of characters to remove.</param>
    void Remove(int32 startIndex, int32 length);

    /// <summary>
    /// Splits a string into substrings that are based on the character.
    /// </summary>
    /// <param name="c">A character that delimits the substrings in this string.</param>
    /// <param name="results">An array whose elements contain the substrings from this instance that are delimited by separator.</param>
    void Split(char c, Array<StringAnsi, HeapAllocation>& results) const;

public:
    /// <summary>
    /// Formats the message and gets it as a string.
    /// </summary>
    /// <param name="format">The format string.</param>
    /// <param name="args">The custom arguments.</param>
    /// <returns>The formatted text.</returns>
    template<typename... Args>
    static StringAnsi Format(const char* format, const Args& ... args)
    {
        std_flax::allocator<char> allocator;
        fmt::basic_memory_buffer<char, fmt::inline_buffer_size, std_flax::allocator<char>> buffer(allocator);
        fmt_flax::format(buffer, format, args...);
        return StringAnsi(buffer.data(), (int32)buffer.size());
    }

public:
    String ToString() const
    {
        return String(Get(), Length());
    }

    StringAnsi ToStringAnsi() const
    {
        return *this;
    }

private:
    template<typename T1, typename T2>
    static StringAnsi ConcatStrings(T1 left, T2 right)
    {
        if (left.IsEmpty())
            return MoveTemp(right);
        if (right.IsEmpty())
            return MoveTemp(left);

        const char* leftStr = left.Get();
        const int32 leftLen = left.Length();
        const char* rightStr = right.Get();
        const int32 rightLen = right.Length();

        StringAnsi result;
        result.ReserveSpace(leftLen + rightLen);
        Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(char));
        Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(char));

        return result;
    }

    template<typename T>
    static StringAnsi ConcatCharsToString(const char* left, T right)
    {
        if (!left || !*left)
            return MoveTemp(right);

        const char* leftStr = left;
        const int32 leftLen = StringUtils::Length(left);
        const char* rightStr = right.Get();
        const int32 rightLen = right.Length();

        StringAnsi result;
        result.ReserveSpace(leftLen + rightLen);
        Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(char));
        Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(char));

        return result;
    }

    template<typename T>
    static StringAnsi ConcatStringToChars(T left, const char* right)
    {
        if (!right || !*right)
            return MoveTemp(left);

        const char* leftStr = left.Get();
        const int32 leftLen = left.Length();
        const char* rightStr = right;
        const int32 rightLen = StringUtils::Length(right);

        StringAnsi result;
        result.ReserveSpace(leftLen + rightLen);
        Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(char));
        Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(char));

        return result;
    }
};

inline uint32 GetHash(const StringAnsi& key)
{
    return StringUtils::GetHashCode(key.Get());
}

namespace fmt
{
    template<>
    struct formatter<StringAnsi, char>
    {
        template<typename ParseContext>
        auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const StringAnsi& v, FormatContext& ctx) -> decltype(ctx.out())
        {
            return fmt::detail::copy_str<char>(v.Get(), v.Get() + v.Length(), ctx.out());
        }
    };
}

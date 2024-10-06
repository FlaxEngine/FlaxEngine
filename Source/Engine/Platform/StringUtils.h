// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Describes case sensitivity options for string comparisons.
/// </summary>
enum class StringSearchCase
{
    /// <summary>
    /// Case sensitive. Upper/lower casing must match for strings to be considered equal.
    /// </summary>
    CaseSensitive,

    /// <summary>
    /// Ignore case. Upper/lower casing does not matter when making a comparison.
    /// </summary>
    IgnoreCase,
};

/// <summary>
/// The string operations utilities.
/// </summary>
class FLAXENGINE_API StringUtils
{
public:
    /// <summary>
    /// Calculates the hash code for input string.
    /// </summary>
    /// <param name="str">The pointer to the input characters sequence.</param>
    /// <returns>The unique hash value.</returns>
    template<typename CharType>
    static uint32 GetHashCode(const CharType* str)
    {
        uint32 hash = 5381;
        CharType c;
        if (str)
        {
            while ((c = *str++) != 0)
                hash = ((hash << 5) + hash) + (uint32)c;
        }
        return hash;
    }

    /// <summary>
    /// Calculates the hash code for input string.
    /// </summary>
    /// <param name="str">The pointer to the input characters sequence.</param>
    /// <param name="length">The input sequence length (amount of characters).</param>
    /// <returns>The unique hash value.</returns>
    template<typename CharType>
    static uint32 GetHashCode(const CharType* str, int32 length)
    {
        uint32 hash = 5381;
        CharType c;
        if (str)
        {
            while ((c = *str++) != 0 && length-- > 0)
                hash = ((hash << 5) + hash) + (uint32)c;
        }
        return hash;
    }

public:
    static bool IsUpper(char c);
    static bool IsLower(char c);
    static bool IsAlpha(char c);
    static bool IsPunct(char c);
    static bool IsAlnum(char c);
    static bool IsDigit(char c);
    static bool IsHexDigit(char c);
    static bool IsWhitespace(char c);
    static char ToUpper(char c);
    static char ToLower(char c);

public:
    static bool IsUpper(Char c);
    static bool IsLower(Char c);
    static bool IsAlpha(Char c);
    static bool IsPunct(Char c);
    static bool IsAlnum(Char c);
    static bool IsDigit(Char c);
    static bool IsHexDigit(Char c);
    static bool IsWhitespace(Char c);
    static Char ToUpper(Char c);
    static Char ToLower(Char c);

public:
    // Compare two strings with case sensitive. Strings must not be null.
    static int32 Compare(const Char* str1, const Char* str2);

    // Compare two strings without case sensitive. Strings must not be null.
    static int32 Compare(const Char* str1, const Char* str2, int32 maxCount);

    // Compare two strings without case sensitive. Strings must not be null.
    static int32 CompareIgnoreCase(const Char* str1, const Char* str2);

    // Compare two strings without case sensitive. Strings must not be null.
    static int32 CompareIgnoreCase(const Char* str1, const Char* str2, int32 maxCount);

    // Compare two strings with case sensitive. Strings must not be null.
    static int32 Compare(const char* str1, const char* str2);

    // Compare two strings without case sensitive. Strings must not be null.
    static int32 Compare(const char* str1, const char* str2, int32 maxCount);

    // Compare two strings without case sensitive. Strings must not be null.
    static int32 CompareIgnoreCase(const char* str1, const char* str2);

    // Compare two strings without case sensitive. Strings must not be null.
    static int32 CompareIgnoreCase(const char* str1, const char* str2, int32 maxCount);

public:
    // Gets the string length. Returns 0 if str is null.
    static int32 Length(const Char* str);

    // Gets the string length. Returns 0 if str is null.
    static int32 Length(const char* str);

    // Copies the string.
    static Char* Copy(Char* dst, const Char* src);

    // Copies the string (count is maximum amount of characters to copy).
    static Char* Copy(Char* dst, const Char* src, int32 count);

    // Finds specific sub-string in the input string. Returns the first found position in the input string or nulll if failed.
    static const Char* Find(const Char* str, const Char* toFind);

    // Finds specific sub-string in the input string, case sensitive. Returns the first found position in the input string or nulll if failed.
    static const char* Find(const char* str, const char* toFind);

    // Finds specific sub-string in the input string, case insensitive. Returns the first found position in the input string or nulll if failed.
    static const Char* FindIgnoreCase(const Char* str, const Char* toFind);

    // Finds specific sub-string in the input string, case insensitive. Returns the first found position in the input string or nulll if failed.
    static const char* FindIgnoreCase(const char* str, const char* toFind);

public:
    // Converts characters from ANSI to UTF-16
    static void ConvertANSI2UTF16(const char* from, Char* to, int32 fromLength, int32& toLength);

    // Converts characters from UTF-16 to ANSI
    static void ConvertUTF162ANSI(const Char* from, char* to, int32 len);

    // Convert characters from UTF-8 to UTF-16
    static void ConvertUTF82UTF16(const char* from, Char* to, int32 fromLength, int32& toLength);

    // Convert characters from UTF-8 to UTF-16 (allocates the output buffer with Allocator::Allocate of size (toLength + 1) * sizeof(Char), call Allocator::Free after usage). Returns null on empty or invalid string.
    static Char* ConvertUTF82UTF16(const char* from, int32 fromLength, int32& toLength);

    // Convert characters from UTF-16 to UTF-8
    static void ConvertUTF162UTF8(const Char* from, char* to, int32 fromLength, int32& toLength);

    // Convert characters from UTF-16 to UTF-8 (allocates the output buffer with Allocator::Allocate of size toLength + 1, call Allocator::Free after usage). Returns null on empty or invalid string.
    static char* ConvertUTF162UTF8(const Char* from, int32 fromLength, int32& toLength);

public:
    /// <summary>
    /// Gets the directory name of the specified path string.
    /// </summary>
    /// <param name="path">The path string from which to obtain the directory name.</param>
    /// <returns>Directory name.</returns>
    static StringView GetDirectoryName(const StringView& path);

    /// <summary>
    /// Gets the file name and extension of the specified path string
    /// </summary>
    /// <param name="path">The path string from which to obtain the file name and extension.</param>
    /// <returns>File name with extension.</returns>
    static StringView GetFileName(const StringView& path);

    /// <summary>
    /// Gets the file name without extension of the specified path string.
    /// </summary>
    /// <param name="path">The path string from which to obtain the file name.</param>
    /// <returns>File name without extension.</returns>
    static StringView GetFileNameWithoutExtension(const StringView& path);

    /// <summary>
    /// Gets the path without extension.
    /// </summary>
    /// <param name="path">The path string.</param>
    /// <returns>The path string without extension.</returns>
    static StringView GetPathWithoutExtension(const StringView& path);

    /// <summary>
    /// Normalizes path string and removes any relative parts such as `/../` and `/./`.
    /// </summary>
    /// <param name="path">The input and output string with path to process.</param>
    static void PathRemoveRelativeParts(String& path);

public:
    // Converts hexadecimal character into the value.
    static int32 HexDigit(Char c);

    // Parses text to unsigned integer value. Returns true if failed to convert the value.
    template<typename T>
    static bool ParseHex(const T* str, int32 length, uint32* result)
    {
        uint32 sum = 0;
        const T* p = str;
        const T* end = str + length;
        if (*p == '0' && *(p + 1) == 'x')
            p += 2;
        while (*p && p < end)
        {
            int32 c = *p - '0';
            if (c < 0 || c > 9)
            {
                c = ToLower(*p) - 'a' + 10;
                if (c < 10 || c > 15)
                    return true;
            }
            sum = 16 * sum + c;
            p++;
        }
        *result = sum;
        return false;
    }

    // Parses text to unsigned integer value. Returns true if failed to convert the value.
    template<typename T>
    static bool ParseHex(const T* str, uint32* result)
    {
        return StringUtils::ParseHex(str, StringUtils::Length(str), result);
    }

    // Parses text to the unsigned integer value. Returns true if failed to convert the value.
    template<typename T>
    static bool Parse(const T* str, int32 length, uint64* result)
    {
        int64 sum = 0;
        const T* p = str;
        while (length--)
        {
            int32 c = *p++ - 48;
            if (c < 0 || c > 9)
                return true;
            sum = 10 * sum + c;
        }
        *result = sum;
        return false;
    }
    template<typename T>
    static bool Parse(const T* str, uint32 length, uint32* result)
    {
        uint64 tmp;
        const bool b = StringUtils::Parse(str, length, &tmp);
        *result = (uint32)tmp;
        return b;
    }
    template<typename T>
    static bool Parse(const T* str, uint32 length, uint16* result)
    {
        uint64 tmp;
        const bool b = StringUtils::Parse(str, length, &tmp);
        *result = (uint16)tmp;
        return b;
    }
    template<typename T>
    static bool Parse(const T* str, uint32 length, uint8* result)
    {
        uint64 tmp;
        const bool b = StringUtils::Parse(str, length, &tmp);
        *result = (uint8)tmp;
        return b;
    }
    
    // Parses text to the integer value. Returns true if failed to convert the value.
    template<typename T>
    static bool Parse(const T* str, int32 length, int64* result)
    {
        int64 sum = 0;
        const T* p = str;
        bool negate = false;
        while (length--)
        {
            int32 c = *p++ - 48;
            if (c == -3)
            {
                negate = true;
                continue;
            }
            if (c < 0 || c > 9)
                return true;
            sum = 10 * sum + c;
        }
        if (negate)
            sum = -sum;
        *result = sum;
        return false;
    }
    template<typename T>
    static bool Parse(const T* str, uint32 length, int32* result)
    {
        int64 tmp;
        const bool b = StringUtils::Parse(str, length, &tmp);
        *result = (int32)tmp;
        return b;
    }
    template<typename T>
    static bool Parse(const T* str, uint32 length, int16* result)
    {
        int64 tmp;
        const bool b = StringUtils::Parse(str, length, &tmp);
        *result = (int16)tmp;
        return b;
    }
    template<typename T>
    static bool Parse(const T* str, uint32 length, int8* result)
    {
        int64 tmp;
        const bool b = StringUtils::Parse(str, length, &tmp);
        *result = (int8)tmp;
        return b;
    }

    // Parses text to scalar integer value. Returns true if failed to convert the value.
    template<typename T, typename U>
    static bool Parse(const T* str, U* result)
    {
        return StringUtils::Parse(str, StringUtils::Length(str), result);
    }

    // Parses text to the scalar value. Returns true if failed to convert the value.
    static bool Parse(const Char* str, float* result);

    // Parses text to the scalar value. Returns true if failed to convert the value.
    static bool Parse(const char* str, float* result);

public:
    static String ToString(int32 value);
    static String ToString(int64 value);
    static String ToString(uint32 value);
    static String ToString(uint64 value);
    static String ToString(float value);
    static String ToString(double value);

public:
    // Converts the string to double null-terminated string.
    static String GetZZString(const Char* str);
};

inline uint32 GetHash(const char* key)
{
    return StringUtils::GetHashCode(key);
}

inline uint32 GetHash(const Char* key)
{
    return StringUtils::GetHashCode(key);
}

inline uint32 GetHash(const char* key, int32 length)
{
    return StringUtils::GetHashCode(key, length);
}

inline uint32 GetHash(const Char* key, int32 length)
{
    return StringUtils::GetHashCode(key, length);
}

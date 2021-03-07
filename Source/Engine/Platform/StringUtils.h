// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
/// The string operations utilities collection.
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

    // Returns true if character is uppercase
    static bool IsUpper(char c);

    // Returns true if character is lowercase
    static bool IsLower(char c);

    static bool IsAlpha(char c);

    static bool IsPunct(char c);

    static bool IsAlnum(char c);

    static bool IsDigit(char c);

    static bool IsHexDigit(char c);

    // Returns true if character is a whitespace
    static bool IsWhitespace(char c);

    // Convert wide character to upper case
    static char ToUpper(char c);

    // Convert wide character to lower case
    static char ToLower(char c);

public:

    // Returns true if character is uppercase
    static bool IsUpper(Char c);

    // Returns true if character is lowercase
    static bool IsLower(Char c);

    static bool IsAlpha(Char c);

    static bool IsPunct(Char c);

    static bool IsAlnum(Char c);

    static bool IsDigit(Char c);

    static bool IsHexDigit(Char c);

    // Returns true if character is a whitespace
    static bool IsWhitespace(Char c);

    // Convert wide character to upper case
    static Char ToUpper(Char c);

    // Convert wide character to lower case
    static Char ToLower(Char c);

public:

    // Compare two strings with case sensitive
    static int32 Compare(const Char* str1, const Char* str2);

    // Compare two strings without case sensitive
    static int32 Compare(const Char* str1, const Char* str2, int32 maxCount);

    // Compare two strings without case sensitive
    static int32 CompareIgnoreCase(const Char* str1, const Char* str2);

    // Compare two strings without case sensitive
    static int32 CompareIgnoreCase(const Char* str1, const Char* str2, int32 maxCount);

    // Compare two strings with case sensitive
    static int32 Compare(const char* str1, const char* str2);

    // Compare two strings without case sensitive
    static int32 Compare(const char* str1, const char* str2, int32 maxCount);

    // Compare two strings without case sensitive
    static int32 CompareIgnoreCase(const char* str1, const char* str2);

    // Compare two strings without case sensitive
    static int32 CompareIgnoreCase(const char* str1, const char* str2, int32 maxCount);

public:

    // Get string length
    static int32 Length(const Char* str);

    // Get string length
    static int32 Length(const char* str);

    // Copy string
    static Char* Copy(Char* dst, const Char* src);

    // Copy string (count is maximum amount of characters to copy)
    static Char* Copy(Char* dst, const Char* src, int32 count);

    // Finds string in string, case sensitive
    // @param str The string to look through
    // @param toFind The string to find inside str
    // @return Position in str if Find was found, otherwise null
    static const Char* Find(const Char* str, const Char* toFind);

    // Finds string in string, case sensitive
    // @param str The string to look through
    // @param toFind The string to find inside str
    // @return Position in str if Find was found, otherwise null
    static const char* Find(const char* str, const char* toFind);

    // Finds string in string, case insensitive
    // @param str The string to look through
    // @param toFind The string to find inside str
    // @return Position in str if toFind was found, otherwise null
    static const Char* FindIgnoreCase(const Char* str, const Char* toFind);

    // Finds string in string, case insensitive
    // @param str The string to look through
    // @param toFind The string to find inside str
    // @return Position in str if toFind was found, otherwise null
    static const char* FindIgnoreCase(const char* str, const char* toFind);

public:

    // Converts characters from ANSI to UTF-16
    static void ConvertANSI2UTF16(const char* from, Char* to, int32 len);

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

    // Returns the directory name of the specified path string
    // @param path The path string from which to obtain the directory name
    // @returns Directory name
    static String GetDirectoryName(const String& path);

    // Returns the file name and extension of the specified path string
    // @param path The path string from which to obtain the file name and extension
    // @returns File name with extension
    static String GetFileName(const String& path);

    // Returns the file name without extension of the specified path string
    // @param path The path string from which to obtain the file name
    // @returns File name without extension
    static String GetFileNameWithoutExtension(const String& path);

    static String GetPathWithoutExtension(const String& path);

    static void PathRemoveRelativeParts(String& path);

public:

    /// <summary>
    /// Convert integer value to string
    /// </summary>
    /// <param name="value">Value to convert</param>
    /// <param name="base">Base (8,10,16)</param>
    /// <param name="buffer">Input buffer</param>
    /// <param name="length">Result string length</param>
    template<typename CharType>
    static void itoa(int32 value, int32 base, CharType* buffer, int32& length)
    {
        // Allocate buffer
        bool isNegative = false;
        CharType* pos = buffer;
        CharType* pos1 = buffer;
        length = 0;

        // Validate input base
        if (base < 8 || base > 16)
        {
            *pos = '\0';
            return;
        }

        // Special case for zero
        if (value == 0)
        {
            length++;
            *pos++ = '0';
            *pos = '\0';
            return;
        }

        // Check if value is negative
        if (value < 0)
        {
            isNegative = true;
            value = -value;
        }

        // Convert. If base is power of two (2,4,8,16..)
        // we could use binary and operation and shift offset instead of division
        while (value)
        {
            length++;
            int32 reminder = value % base;
            *pos++ = reminder + (reminder > 9 ? 'a' - 10 : '0');
            value /= base;
        }

        // Apply negative sign
        if (isNegative)
            *pos++ = '-';

        // Add null terminator char
        *pos-- = 0;

        // Reverse the buffer
        while (pos1 < pos)
        {
            CharType c = *pos;
            *pos-- = *pos1;
            *pos1++ = c;
        }
    }

    static int32 HexDigit(Char c)
    {
        int32 result = 0;

        if (c >= '0' && c <= '9')
        {
            result = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            result = c + 10 - 'a';
        }
        else if (c >= 'A' && c <= 'F')
        {
            result = c + 10 - 'A';
        }
        else
        {
            result = 0;
        }

        return result;
    }

    // Parse text to unsigned integer value
    // @param str String to parse
    // @return Result value
    // @returns True if cannot convert data, otherwise false
    template<typename CharType>
    static bool ParseHex(const CharType* str, uint32* result)
    {
        uint32 sum = 0;
        const CharType* p = str;

        if (*p == '0' && *(p + 1) == 'x')
            p += 2;

        while (*p)
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

    // Parse text to unsigned integer value
    // @param str String to parse
    // @param length Text length
    // @return Result value
    // @returns True if cannot convert data, otherwise false
    template<typename CharType>
    static bool ParseHex(const CharType* str, int32 length, uint32* result)
    {
        uint32 sum = 0;
        const CharType* p = str;
        const CharType* end = str + length;

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

    // Parse text to scalar value
    // @param str String to parse
    // @return Result value
    // @returns True if cannot convert data, otherwise false
    template<typename T, typename U>
    static bool Parse(const T* str, U* result)
    {
        U sum = 0;
        const T* p = str;
        while (*p)
        {
            int32 c = *p++ - 48;
            if (c < 0 || c > 9)
                return true;
            sum = 10 * sum + c;
        }
        *result = sum;
        return false;
    }

    // Parse text to scalar value
    // @param str String to parse
    // @param length Text length to read
    // @return Result value
    // @returns True if cannot convert data, otherwise false
    template<typename T, typename U>
    static bool Parse(const T* str, uint32 length, U* result)
    {
        U sum = 0;
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

    // Parse Unicode text to float value
    // @param str String to parse
    // @return Result value
    // @returns True if cannot convert data, otherwise false
    static bool Parse(const Char* str, float* result);

public:

    static String ToString(int32 value);
    static String ToString(int64 value);
    static String ToString(uint32 value);
    static String ToString(uint64 value);
    static String ToString(float value);
    static String ToString(double value);
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

// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Mathd.h"
#include "Engine/Core/Collections/Array.h"
#if PLATFORM_TEXT_IS_CHAR16
#include <string>
#endif

constexpr char DirectorySeparatorChar = '\\';
constexpr char AltDirectorySeparatorChar = '/';
constexpr char VolumeSeparatorChar = ':';

const Char* StringUtils::FindIgnoreCase(const Char* str, const Char* toFind)
{
    if (toFind == nullptr || str == nullptr)
    {
        return nullptr;
    }

    const Char findInitial = ToUpper(*toFind);
    const int32 length = Length(toFind++) - 1;
    Char c = *str++;
    while (c)
    {
        c = ToUpper(c);
        if (c == findInitial && !CompareIgnoreCase(str, toFind, length))
        {
            return str - 1;
        }

        c = *str++;
    }

    return nullptr;
}

const char* StringUtils::FindIgnoreCase(const char* str, const char* toFind)
{
    if (toFind == nullptr || str == nullptr)
    {
        return nullptr;
    }

    const char findInitial = (char)ToUpper(*toFind);
    const int32 length = Length(toFind++) - 1;
    char c = *str++;
    while (c)
    {
        c = (char)ToUpper(c);
        if (c == findInitial && !CompareIgnoreCase(str, toFind, length))
        {
            return str - 1;
        }

        c = *str++;
    }

    return nullptr;
}

void PrintUTF8Error(const char* from, uint32 fromLength)
{
    LOG(Error, "Not a UTF-8 string. Length: {0}", fromLength);
    for (uint32 i = 0; i < fromLength; i++)
    {
        LOG(Error, "str[{0}] = {0}", i, (uint32)from[i]);
    }
}

void ConvertUTF82UTF16Helper(Array<uint32>& unicode, const char* from, int32 fromLength, int32& toLength)
{
    // Reference: https://stackoverflow.com/questions/7153935/how-to-convert-utf-8-stdstring-to-utf-16-stdwstring
    unicode.EnsureCapacity(fromLength);
    int32 i = 0, todo;
    uint32 uni;
    toLength = 0;
    while (i < fromLength)
    {
        byte ch = from[i++];

        if (ch <= 0x7F)
        {
            uni = ch;
            todo = 0;
        }
        else if (ch <= 0xBF)
        {
            PrintUTF8Error(from, fromLength);
            return;
        }
        else if (ch <= 0xDF)
        {
            uni = ch & 0x1F;
            todo = 1;
        }
        else if (ch <= 0xEF)
        {
            uni = ch & 0x0F;
            todo = 2;
        }
        else if (ch <= 0xF7)
        {
            uni = ch & 0x07;
            todo = 3;
        }
        else
        {
            PrintUTF8Error(from, fromLength);
            return;
        }

        for (int32 j = 0; j < todo; j++)
        {
            if (i == fromLength)
            {
                PrintUTF8Error(from, fromLength);
                return;
            }
            ch = from[i++];
            if (ch < 0x80 || ch > 0xBF)
            {
                PrintUTF8Error(from, fromLength);
                return;
            }

            uni <<= 6;
            uni += ch & 0x3F;
        }

        if ((uni >= 0xD800 && uni <= 0xDFFF) || uni > 0x10FFFF)
        {
            PrintUTF8Error(from, fromLength);
            return;
        }

        unicode.Add(uni);

        toLength++;
        if (uni > 0xFFFF)
        {
            toLength++;
        }
    }
}

void StringUtils::ConvertUTF82UTF16(const char* from, Char* to, int32 fromLength, int32& toLength)
{
    Array<uint32> unicode;
    ConvertUTF82UTF16Helper(unicode, from, fromLength, toLength);
    for (int32 i = 0, j = 0; j < unicode.Count(); i++, j++)
    {
        uint32 uni = unicode[j];
        if (uni <= 0xFFFF)
        {
            to[i] = (Char)uni;
        }
        else
        {
            uni -= 0x10000;
            to[i++] += (Char)((uni >> 10) + 0xD800);
            to[i] += (Char)((uni & 0x3FF) + 0xDC00);
        }
    }
}

Char* StringUtils::ConvertUTF82UTF16(const char* from, int32 fromLength, int32& toLength)
{
    Array<uint32> unicode;
    ConvertUTF82UTF16Helper(unicode, from, fromLength, toLength);
    if (toLength == 0)
        return nullptr;
    Char* to = (Char*)Allocator::Allocate((toLength + 1) * sizeof(Char));
    for (int32 i = 0, j = 0; j < unicode.Count(); i++, j++)
    {
        uint32 uni = unicode[j];
        if (uni <= 0xFFFF)
        {
            to[i] = (Char)uni;
        }
        else
        {
            uni -= 0x10000;
            to[i++] += (Char)((uni >> 10) + 0xD800);
            to[i] += (Char)((uni & 0x3FF) + 0xDC00);
        }
    }
    to[toLength] = 0;
    return to;
}

void PrintUTF16Error(const Char* from, uint32 fromLength)
{
    LOG(Error, "Not a UTF-16 string. Length: {0}", fromLength);
    for (uint32 i = 0; i < fromLength; i++)
    {
        LOG(Error, "str[{0}] = {0}", i, (uint32)from[i]);
    }
}

void ConvertUTF162UTF8Helper(Array<uint32>& unicode, const Char* from, int32 fromLength, int32& toLength)
{
    // Reference: https://stackoverflow.com/questions/21456926/how-do-i-convert-a-string-in-utf-16-to-utf-8-in-c
    unicode.EnsureCapacity(fromLength);
    toLength = 0;
    int32 i = 0;
    while (i < fromLength)
    {
        uint32 uni = from[i++];
        if (uni < 0xD800U || uni > 0xDFFFU)
        {
        }
        else if (uni >= 0xDC00U)
        {
            PrintUTF16Error(from, fromLength);
            return;
        }
        else if (i + 1 == fromLength)
        {
            PrintUTF16Error(from, fromLength);
            return;
        }
        else if (i < fromLength)
        {
            uni = (uni & 0x3FFU) << 10;
            if ((from[i] < 0xDC00U) || (from[i] > 0xDFFFU))
            {
                PrintUTF16Error(from, fromLength);
                return;
            }
            uni |= from[i++] & 0x3FFU;
            uni += 0x10000U;
        }

        unicode.Add(uni);

        toLength += uni <= 0x7FU ? 1 : uni <= 0x7FFU ? 2 : uni <= 0xFFFFU ? 3 : uni <= 0x1FFFFFU ? 4 : uni <= 0x3FFFFFFU ? 5 : uni <= 0x7FFFFFFFU ? 6 : 7;
    }
}

void StringUtils::ConvertUTF162UTF8(const Char* from, char* to, int32 fromLength, int32& toLength)
{
    Array<uint32> unicode;
    ConvertUTF162UTF8Helper(unicode, from, fromLength, toLength);
    for (int32 i = 0, j = 0; j < unicode.Count(); j++)
    {
        const uint32 uni = unicode[j];
        const uint32 count = uni <= 0x7FU ? 1 : uni <= 0x7FFU ? 2 : uni <= 0xFFFFU ? 3 : uni <= 0x1FFFFFU ? 4 : uni <= 0x3FFFFFFU ? 5 : uni <= 0x7FFFFFFFU ? 6 : 7;
        to[i++] = (char)(count <= 1 ? (byte)uni : ((byte(0xFFU) << (8 - count)) | byte(uni >> (6 * (count - 1)))));
        for (uint32 k = 1; k < count; k++)
            to[i++] = char(byte(0x80U | (byte(0x3FU) & byte(uni >> (6 * (count - 1 - k))))));
    }
}

char* StringUtils::ConvertUTF162UTF8(const Char* from, int32 fromLength, int32& toLength)
{
    Array<uint32> unicode;
    ConvertUTF162UTF8Helper(unicode, from, fromLength, toLength);
    if (toLength == 0)
        return nullptr;
    char* to = (char*)Allocator::Allocate(toLength + 1);
    for (int32 i = 0, j = 0; j < unicode.Count(); j++)
    {
        const uint32 uni = unicode[j];
        const uint32 count = uni <= 0x7FU ? 1 : uni <= 0x7FFU ? 2 : uni <= 0xFFFFU ? 3 : uni <= 0x1FFFFFU ? 4 : uni <= 0x3FFFFFFU ? 5 : uni <= 0x7FFFFFFFU ? 6 : 7;
        to[i++] = (char)(count <= 1 ? (byte)uni : ((byte(0xFFU) << (8 - count)) | byte(uni >> (6 * (count - 1)))));
        for (uint32 k = 1; k < count; k++)
            to[i++] = char(byte(0x80U | (byte(0x3FU) & byte(uni >> (6 * (count - 1 - k))))));
    }
    to[toLength] = 0;
    return to;
}

void RemoveLongPathPrefix(const String& path, String& result)
{
    if (!path.StartsWith(TEXT("\\\\?\\"), StringSearchCase::CaseSensitive))
    {
        result = path;
    }
    if (!path.StartsWith(TEXT("\\\\?\\UNC\\"), StringSearchCase::IgnoreCase))
    {
        result = path.Substring(4);
    }
    result = path;
    result.Remove(2, 6);
}

StringView StringUtils::GetDirectoryName(const StringView& path)
{
    const int32 lastFrontSlash = path.FindLast('\\');
    const int32 lastBackSlash = path.FindLast('/');
    const int32 splitIndex = Math::Max(lastBackSlash, lastFrontSlash);
    return splitIndex != INVALID_INDEX ? path.Left(splitIndex) : StringView::Empty;
}

StringView StringUtils::GetFileName(const StringView& path)
{
    Char chr;
    const int32 length = path.Length();
    int32 num = length;
    do
    {
        num--;
        if (num < 0)
            return path;
        chr = path[num];
    } while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != VolumeSeparatorChar);
    return path.Substring(num + 1, length - num - 1);
}

StringView StringUtils::GetFileNameWithoutExtension(const StringView& path)
{
    StringView filename = GetFileName(path);
    const int32 num = filename.FindLast('.');
    if (num != -1)
        return filename.Substring(0, num);
    return filename;
}

StringView StringUtils::GetPathWithoutExtension(const StringView& path)
{
    const int32 num = path.FindLast('.');
    if (num != -1)
        return path.Substring(0, num);
    return path;
}

void StringUtils::PathRemoveRelativeParts(String& path)
{
    FileSystem::NormalizePath(path);

    Array<String> components;
    path.Split(TEXT('/'), components);

    Array<String, InlinedAllocation<16>> stack;
    for (String& bit : components)
    {
        if (bit == TEXT(".."))
        {
            if (stack.HasItems())
            {
                String popped = stack.Pop();
                const int32 windowsDriveStart = popped.Find('\\');
                if (popped == TEXT(".."))
                {
                    stack.Push(popped);
                    stack.Push(bit);
                }
                else if (windowsDriveStart != -1)
                {
                    stack.Add(MoveTemp(popped.Left(windowsDriveStart + 1)));
                }
            }
            else
            {
                stack.Push(bit);
            }
        }
        else if (bit == TEXT("."))
        {
            // Skip /./
        }
        else
        {
            stack.Add(MoveTemp(bit));
        }
    }

    const bool isRooted = path.StartsWith(TEXT('/')) || (path.Length() >= 2 && path[0] == '.' && path[1] == '/');
    path.Clear();
    for (const String& e : stack)
        path /= e;
    if (isRooted && path.HasChars() && path[0] != '/')
        path.Insert(0, TEXT("/"));
}

int32 StringUtils::HexDigit(Char c)
{
    int32 result = 0;
    if (c >= '0' && c <= '9')
        result = c - '0';
    else if (c >= 'a' && c <= 'f')
        result = c + 10 - 'a';
    else if (c >= 'A' && c <= 'F')
        result = c + 10 - 'A';
    return result;
}

template<typename C, typename T>
bool ParseFloat(const C* str, T* ret)
{
    // [Reference: https://stackoverflow.com/a/44741229]
    T result = 0;
    T sign = *str == '-' ? str++, (T)-1.0 : (T)1.0;
    while (*str >= '0' && *str <= '9')
    {
        result *= 10;
        result += *str - '0';
        str++;
    }
    if (*str == ',' || *str == '.')
    {
        str++;
        T multiplier = 0.1;
        while (*str >= '0' && *str <= '9')
        {
            result += (*str - '0') * multiplier;
            multiplier /= 10;
            str++;
        }
    }
    result *= sign;
    if (*str == 'e' || *str == 'E')
    {
        str++;
        T powerer = *str == '-' ? str++, (T)0.1 : (T)10;
        T power = 0;
        while (*str >= '0' && *str <= '9')
        {
            power *= 10;
            power += *str - '0';
            str++;
        }
        result *= Math::Pow(powerer, power);
    }
    if (*str)
        return true;
    *ret = result;
    return false;
}

bool StringUtils::Parse(const Char* str, float* result)
{
#if 0
    // Convert '.' into ','
    char buffer[64];
    char* ptr = buffer;
    char* end = buffer + ARRAY_COUNT(buffer);
    while (str && *str && ptr != end)
    {
        Char c = *str++;
        if (c == '.')
            c = ',';
        *ptr++ = (char)c;
    }
    *ptr = 0;

    float v = (float)atof(buffer);
    *result = v;
    if (v == 0)
    {
        const int32 len = Length(str);
        return !(str[0] == '0' && ((len == 1) || (len == 3 && (str[1] == ',' || str[1] == '.') && str[2] == '0')));
    }
    return false;
#else
    return ParseFloat(str, result);
#endif
}

bool StringUtils::Parse(const char* str, float* result)
{
#if 0
    *result = (float)atof(str);
    return false;
#else
    return ParseFloat(str, result);
#endif
}

String StringUtils::ToString(int32 value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::ToString(int64 value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::ToString(uint32 value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::ToString(uint64 value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::ToString(float value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::ToString(double value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::GetZZString(const Char* str)
{
    const Char* end = str;
    while (*end != '\0')
    {
        end++;
        if (*end == '\0')
            end++;
    }
    return String(str, (int32)(end - str));
}

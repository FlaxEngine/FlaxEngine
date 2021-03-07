// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#if PLATFORM_TEXT_IS_CHAR16
#include <string>
#endif

const char DirectorySeparatorChar = '\\';
const char AltDirectorySeparatorChar = '/';
const char VolumeSeparatorChar = ':';

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

String StringUtils::GetDirectoryName(const String& path)
{
    const int32 lastFrontSlash = path.FindLast('\\');
    const int32 lastBackSlash = path.FindLast('/');
    const int32 splitIndex = Math::Max(lastBackSlash, lastFrontSlash);
    return splitIndex != INVALID_INDEX ? path.Left(splitIndex) : String::Empty;
}

String StringUtils::GetFileName(const String& path)
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

String StringUtils::GetFileNameWithoutExtension(const String& path)
{
    String filename = GetFileName(path);
    const int32 num = filename.FindLast('.');
    if (num != -1)
    {
        return filename.Substring(0, num);
    }
    return filename;
}

String StringUtils::GetPathWithoutExtension(const String& path)
{
    const int32 num = path.FindLast('.');
    if (num != -1)
    {
        return path.Substring(0, num);
    }
    return path;
}

void StringUtils::PathRemoveRelativeParts(String& path)
{
    FileSystem::NormalizePath(path);

    Array<String> components;
    path.Split(TEXT('/'), components);

    Array<String> stack;
    for (auto& bit : components)
    {
        if (bit == TEXT(".."))
        {
            if (stack.HasItems())
            {
                auto popped = stack.Pop();
                if (popped == TEXT(".."))
                {
                    stack.Push(popped);
                    stack.Push(bit);
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
            stack.Push(bit);
        }
    }

    bool isRooted = path.StartsWith(TEXT('/'));
    path.Clear();
    for (auto& e : stack)
        path /= e;
    if (isRooted && path[0] != '/')
        path.Insert(0, TEXT("/"));
}

const char digit_pairs[201] = {
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899"
};

#define STRING_UTILS_ITOSTR_BUFFER_SIZE 15

bool StringUtils::Parse(const Char* str, float* result)
{
#if PLATFORM_TEXT_IS_CHAR16
    std::u16string u16str = str;
    std::wstring wstr(u16str.begin(), u16str.end());
    float v = wcstof(wstr.c_str(), nullptr);
#else
    float v = wcstof(str, nullptr);
#endif
    *result = v;
    if (v == 0)
    {
        const int32 len = Length(str);
        return !(str[0] == '0' && ((len == 1) || (len == 3 && (str[1] == ',' || str[1] == '.') && str[2] == '0')));
    }
    return false;
}

String StringUtils::ToString(int32 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int32 div = value / 100;

    if (value >= 0)
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

        if (value < 10)
            it++;
    }
    else
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[-2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[-2 * value], 2);

        if (value <= -10)
            it--;

        *it = '-';
    }

    return String(it, (int32)(&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - it));
}

String StringUtils::ToString(int64 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int64 div = value / 100;

    if (value >= 0)
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

        if (value < 10)
            it++;
    }
    else
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[-2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[-2 * value], 2);

        if (value <= -10)
            it--;

        *it = '-';
    }

    return String(it, (int32)(&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - it));
}

String StringUtils::ToString(uint32 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int32 div = value / 100;
    while (div)
    {
        Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
        value = div;
        it -= 2;
        div = value / 100;
    }

    Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

    if (value < 10)
        it++;

    return String((char*)it, (int32)((char*)&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - (char*)it));
}

String StringUtils::ToString(uint64 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int64 div = value / 100;
    while (div)
    {
        Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
        value = div;
        it -= 2;
        div = value / 100;
    }

    Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

    if (value < 10)
        it++;

    return String((char*)it, (int32)((char*)&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - (char*)it));
}

String StringUtils::ToString(float value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::ToString(double value)
{
    return String::Format(TEXT("{}"), value);
}

#undef STRING_UTILS_ITOSTR_BUFFER_SIZE

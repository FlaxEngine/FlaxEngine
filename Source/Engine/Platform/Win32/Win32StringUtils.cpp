// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WIN32

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/StringUtils.h"
#include "IncludeWindowsHeaders.h"
#include <stdlib.h>

bool StringUtils::IsUpper(char c)
{
    return isupper(c) != 0;
}

bool StringUtils::IsLower(char c)
{
    return islower(c) != 0;
}

bool StringUtils::IsAlpha(char c)
{
    return iswalpha(c) != 0;
}

bool StringUtils::IsPunct(char c)
{
    return ispunct(c) != 0;
}

bool StringUtils::IsAlnum(char c)
{
    return isalnum(c) != 0;
}

bool StringUtils::IsDigit(char c)
{
    return isdigit(c) != 0;
}

bool StringUtils::IsHexDigit(char c)
{
    return isxdigit(c) != 0;
}

bool StringUtils::IsWhitespace(char c)
{
    return isspace(c) != 0;
}

char StringUtils::ToUpper(char c)
{
    return toupper(c);
}

char StringUtils::ToLower(char c)
{
    return tolower(c);
}

bool StringUtils::IsUpper(Char c)
{
    return iswupper(c) != 0;
}

bool StringUtils::IsLower(Char c)
{
    return iswlower(c) != 0;
}

bool StringUtils::IsAlpha(Char c)
{
    return iswalpha(c) != 0;
}

bool StringUtils::IsPunct(Char c)
{
    return iswpunct(c) != 0;
}

bool StringUtils::IsAlnum(Char c)
{
    return iswalnum(c) != 0;
}

bool StringUtils::IsDigit(Char c)
{
    return iswdigit(c) != 0;
}

bool StringUtils::IsHexDigit(Char c)
{
    return iswxdigit(c) != 0;
}

bool StringUtils::IsWhitespace(Char c)
{
    return iswspace(c) != 0;
}

Char StringUtils::ToUpper(Char c)
{
    return towupper(c);
}

Char StringUtils::ToLower(Char c)
{
    return towlower(c);
}

int32 StringUtils::Compare(const Char* str1, const Char* str2)
{
    return wcscmp(str1, str2);
}

int32 StringUtils::Compare(const Char* str1, const Char* str2, int32 maxCount)
{
    return wcsncmp(str1, str2, maxCount);
}

int32 StringUtils::CompareIgnoreCase(const Char* str1, const Char* str2)
{
    return _wcsicmp(str1, str2);
}

int32 StringUtils::CompareIgnoreCase(const Char* str1, const Char* str2, int32 maxCount)
{
    return _wcsnicmp(str1, str2, maxCount);
}

int32 StringUtils::Compare(const char* str1, const char* str2)
{
    return strcmp(str1, str2);
}

int32 StringUtils::Compare(const char* str1, const char* str2, int32 maxCount)
{
    return strncmp(str1, str2, maxCount);
}

int32 StringUtils::CompareIgnoreCase(const char* str1, const char* str2)
{
    return _strcmpi(str1, str2);
}

int32 StringUtils::CompareIgnoreCase(const char* str1, const char* str2, int32 maxCount)
{
    return _strnicmp(str1, str2, maxCount);
}

int32 StringUtils::Length(const Char* str)
{
    return str ? static_cast<int32>(wcslen(str)) : 0;
}

int32 StringUtils::Length(const char* str)
{
    return str ? static_cast<int32>(strlen(str)) : 0;
}

Char* StringUtils::Copy(Char* dst, const Char* src)
{
    return static_cast<Char*>(wcscpy(dst, src));
}

Char* StringUtils::Copy(Char* dst, const Char* src, int32 count)
{
    wcsncpy(dst, src, count - 1);
    dst[count - 1] = 0;
    return dst;
}

const Char* StringUtils::Find(const Char* str, const Char* toFind)
{
    return wcsstr(str, toFind);
}

const char* StringUtils::Find(const char* str, const char* toFind)
{
    return strstr(str, toFind);
}

void StringUtils::ConvertANSI2UTF16(const char* from, Char* to, int32 fromLength, int32& toLength)
{
    if (fromLength)
        toLength = (int32)mbstowcs(to, from, fromLength);
    else
        toLength = 0;
}

void StringUtils::ConvertUTF162ANSI(const Char* from, char* to, int32 len)
{
    if (len)
        wcstombs(to, from, len);
}

#endif

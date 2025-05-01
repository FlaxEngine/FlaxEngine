// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UNIX

#include "Engine/Platform/StringUtils.h"
#include <wctype.h>
#include <cctype>
#include <wchar.h>
#include <cstring>
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
    Char c1, c2;
    int32 i;
    do
    {
        c1 = *str1++;
        c2 = *str2++;
        i = (int32)c1 - (int32)c2;
    } while (i == 0 && c1 && c2);
    return i;
}

int32 StringUtils::Compare(const Char* str1, const Char* str2, int32 maxCount)
{
    Char c1, c2;
    int32 i;
    if (maxCount == 0)
        return 0;
    do
    {
        c1 = *str1++;
        c2 = *str2++;
        i = (int32)c1 - (int32)c2;
        maxCount--;
    } while (i == 0 && c1 && c2 && maxCount);
    return i;
}

int32 StringUtils::CompareIgnoreCase(const Char* str1, const Char* str2)
{
    Char c1, c2;
    int32 i;
    do
    {
        c1 = ToLower(*str1++);
        c2 = ToLower(*str2++);
        i = (int32)c1 - (int32)c2;
    } while (i == 0 && c1 && c2);
    return i;
}

int32 StringUtils::CompareIgnoreCase(const Char* str1, const Char* str2, int32 maxCount)
{
    Char c1, c2;
    int32 i;
    if (maxCount == 0)
        return 0;
    do
    {
        c1 = ToLower(*str1++);
        c2 = ToLower(*str2++);
        i = (int32)c1 - (int32)c2;
        maxCount--;
    } while (i == 0 && c1 && c2 && maxCount);
    return i;
}

int32 StringUtils::Length(const Char* str)
{
    if (!str)
        return 0;
    const Char* ptr = str;
    for (; *ptr; ++ptr)
    {
    }
    return ptr - str;
}

int32 StringUtils::Length(const char* str)
{
    if (!str)
        return 0;
    return static_cast<int32>(strlen(str));
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
    return strcasecmp(str1, str2);
}

int32 StringUtils::CompareIgnoreCase(const char* str1, const char* str2, int32 maxCount)
{
    return strncasecmp(str1, str2, maxCount);
}

Char* StringUtils::Copy(Char* dst, const Char* src)
{
    Char* q = dst;
    const Char* p = src;
    Char ch;
    do
    {
        *q++ = ch = *p++;
    } while (ch);
    return dst;
}

Char* StringUtils::Copy(Char* dst, const Char* src, int32 count)
{
    Char* q = dst;
    const Char* p = src;
    char ch;
    while (count)
    {
        count--;
        *q++ = ch = *p++;
        if (!ch)
            break;
    }
    *q = 0;
    return dst;
}

const Char* StringUtils::Find(const Char* str, const Char* toFind)
{
    while (*str)
    {
        const Char* start = str;
        const Char* sub = toFind;

        // If first character of sub string match, check for whole string
        while (*str && *sub && *str == *sub)
        {
            str++;
            sub++;
        }

        // If complete substring match, return starting address
        if (!*sub)
            return (Char*)start;

        // Increment main string 
        str = start + 1;
    }

    // No matches
    return nullptr;
}

const char* StringUtils::Find(const char* str, const char* toFind)
{
    return strstr(str, toFind);
}

/**
 * Returns 1-4 based on the number of leading bits
 *
 * 1111 -> 4
 * 1110 -> 3
 * 110x -> 2
 * 10xx -> 1
 * 0xxx -> 1
 */
static inline int32 Utf8CodepointLength(char ch)
{
    return ((0xe5000000 >> ((ch >> 3) & 0x1e)) & 3) + 1;
}

static inline void Utf8ShiftAndMask(uint32* codePoint, const char byte)
{
    *codePoint <<= 6;
    *codePoint |= 0x3F & byte;
}

static inline uint32 Utf8ToUtf32Codepoint(const char* src, int32 length)
{
    uint32 unicode;
    switch (length)
    {
    case 1:
        return src[0];
    case 2:
        unicode = src[0] & 0x1f;
        Utf8ShiftAndMask(&unicode, src[1]);
        return unicode;
    case 3:
        unicode = src[0] & 0x0f;
        Utf8ShiftAndMask(&unicode, src[1]);
        Utf8ShiftAndMask(&unicode, src[2]);
        return unicode;
    case 4:
        unicode = src[0] & 0x07;
        Utf8ShiftAndMask(&unicode, src[1]);
        Utf8ShiftAndMask(&unicode, src[2]);
        Utf8ShiftAndMask(&unicode, src[3]);
        return unicode;
    default:
        return 0xffff;
    }
}

void StringUtils::ConvertANSI2UTF16(const char* from, Char* to, int32 fromLength, int32& toLength)
{
    const char* const u8end = from + fromLength;
    const char* u8cur = from;
    char16_t* u16cur = to;
    while (u8cur < u8end)
    {
        int32 len = Utf8CodepointLength(*u8cur);
        uint32 codepoint = Utf8ToUtf32Codepoint(u8cur, len);

        // Convert the UTF32 codepoint to one or more UTF16 codepoints
        if (codepoint <= 0xFFFF)
        {
            // Single UTF16 character
            *u16cur++ = (char16_t)codepoint;
        }
        else
        {
            // Multiple UTF16 characters with surrogates
            codepoint = codepoint - 0x10000;
            *u16cur++ = (char16_t)((codepoint >> 10) + 0xD800);
            *u16cur++ = (char16_t)((codepoint & 0x3FF) + 0xDC00);
        }
        u8cur += len;
    }
    toLength = (int32)(u16cur - to);
}

static const char32_t kByteMask = 0x000000BF;
static const char32_t kByteMark = 0x00000080;

// Surrogates aren't valid for UTF-32 characters, so define some constants that will let us screen them out
static const char32_t kUnicodeSurrogateHighStart = 0x0000D800;
static const char32_t kUnicodeSurrogateLowEnd = 0x0000DFFF;
static const char32_t kUnicodeSurrogateStart = kUnicodeSurrogateHighStart;
static const char32_t kUnicodeSurrogateEnd = kUnicodeSurrogateLowEnd;
static const char32_t kUnicodeMaxCodepoint = 0x0010FFFF;

// Mask used to set appropriate bits in first byte of UTF-8 sequence, indexed by number of bytes in the sequence
// 0xxxxxxx
// -> (00-7f) 7bit. Bit mask for the first byte is 0x00000000
// 110yyyyx 10xxxxxx
// -> (c0-df)(80-bf) 11bit. Bit mask is 0x000000C0
// 1110yyyy 10yxxxxx 10xxxxxx
// -> (e0-ef)(80-bf)(80-bf) 16bit. Bit mask is 0x000000E0
// 11110yyy 10yyxxxx 10xxxxxx 10xxxxxx
// -> (f0-f7)(80-bf)(80-bf)(80-bf) 21bit. Bit mask is 0x000000F0
static const char32_t kFirstByteMark[] =
{
    0x00000000,
    0x00000000,
    0x000000C0,
    0x000000E0,
    0x000000F0
};

// Return number of UTF-8 bytes required for the character, otherwise if the character is invalid, return size of 0
static inline int32 utf32_codepoint_utf8_length(char32_t srcChar)
{
    // Figure out how many bytes the result will require
    if (srcChar < 0x00000080)
    {
        return 1;
    }
    else if (srcChar < 0x00000800)
    {
        return 2;
    }
    else if (srcChar < 0x00010000)
    {
        if ((srcChar < kUnicodeSurrogateStart) || (srcChar > kUnicodeSurrogateEnd))
        {
            return 3;
        }
        else
        {
            // Surrogates are invalid UTF-32 characters
            return 0;
        }
    }
        // Max code point for Unicode is 0x0010FFFF
    else if (srcChar <= kUnicodeMaxCodepoint)
    {
        return 4;
    }
    else
    {
        // Invalid UTF-32 character
        return 0;
    }
}

// Write out the source character to dstP
static inline void utf32_codepoint_to_utf8(byte* dstP, char32_t srcChar, int32 bytes)
{
    dstP += bytes;
    switch (bytes)
    {
        // Note: everything falls through
    case 4:
        *--dstP = (byte)((srcChar | kByteMark) & kByteMask);
        srcChar >>= 6;
    case 3:
        *--dstP = (byte)((srcChar | kByteMark) & kByteMask);
        srcChar >>= 6;
    case 2:
        *--dstP = (byte)((srcChar | kByteMark) & kByteMask);
        srcChar >>= 6;
    case 1:
        *--dstP = (byte)(srcChar | kFirstByteMark[bytes]);
    }
}

void StringUtils::ConvertUTF162ANSI(const Char* from, char* to, int32 len)
{
    if (from == nullptr || len == 0 || to == nullptr)
        return;

    const char16_t* curUtf16 = from;
    const char16_t* const endUtf16 = from + len;
    char* cur = to;

    while (curUtf16 < endUtf16)
    {
        char32_t utf32;

        // Surrogate pairs
        if ((*curUtf16 & 0xFC00) == 0xD800)
        {
            utf32 = (*curUtf16++ - 0xD800) << 10;
            utf32 |= *curUtf16++ - 0xDC00;
            utf32 += 0x10000;
        }
        else
        {
            utf32 = (char32_t)*curUtf16++;
        }
        const int32 cLen = utf32_codepoint_utf8_length(utf32);
        utf32_codepoint_to_utf8((byte*)cur, utf32, cLen);
        cur += cLen;
    }

    *cur = '\0';
}

#endif

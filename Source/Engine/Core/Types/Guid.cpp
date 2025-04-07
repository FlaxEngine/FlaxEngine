// Copyright (c) Wojciech Figat. All rights reserved.

#include "Guid.h"
#include "String.h"
#include "StringView.h"

Guid Guid::Empty;

String Guid::ToString() const
{
    return ToString(FormatType::N);
}

String Guid::ToString(FormatType format) const
{
    switch (format)
    {
    case FormatType::N:
        return String::Format(TEXT("{:0>8x}{:0>8x}{:0>8x}{:0>8x}"), A, B, C, D);
    case FormatType::B:
        return String::Format(TEXT("{{{:0>8x}-{:0>4x}-{:0>4x}-{:0>4x}-{:0>4x}{:0>8x}}}"), A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
    case FormatType::P:
        return String::Format(TEXT("({:0>8x}-{:0>4x}-{:0>4x}-{:0>4x}-{:0>4x}{:0>8x})"), A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
    default:
        return String::Format(TEXT("{:0>8x}-{:0>4x}-{:0>4x}-{:0>4x}-{:0>4x}{:0>8x}"), A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
    }
}

template<typename CharType>
FORCE_INLINE void GuidToString(const CharType* cachedGuidDigits, CharType* buffer, const Guid& value, Guid::FormatType format)
{
    switch (format)
    {
    case Guid::FormatType::N:
    {
        for (int32 i = 0; i < 32; i++)
            buffer[i] = '0';
        buffer[32] = 0;
        uint32 n = value.A;
        CharType* p = buffer + 7;
        do
        {
            *p-- = cachedGuidDigits[(int32)(n & 0xf)];
        } while ((n >>= 4) != 0);
        n = value.B;
        p = buffer + 15;
        do
        {
            *p-- = cachedGuidDigits[(int32)(n & 0xf)];
        } while ((n >>= 4) != 0);
        n = value.C;
        p = buffer + 23;
        do
        {
            *p-- = cachedGuidDigits[(int32)(n & 0xf)];
        } while ((n >>= 4) != 0);
        n = value.D;
        p = buffer + 31;
        do
        {
            *p-- = cachedGuidDigits[(int32)(n & 0xf)];
        } while ((n >>= 4) != 0);
        break;
    }
    case Guid::FormatType::B:
    // TODO: impl GuidToString for FormatType::B:
    case Guid::FormatType::P:
        // TODO: impl GuidToString for FormatType::P:
    default:
        // TODO: impl GuidToString for FormatType::D:
        Platform::MissingCode(__LINE__, __FILE__, "Missing GuidToString impl.");
    }
}

void Guid::ToString(char* buffer, FormatType format) const
{
    const static char* CachedGuidDigits = "0123456789abcdef";
    return GuidToString<char>(CachedGuidDigits, buffer, *this, format);
}

void Guid::ToString(Char* buffer, FormatType format) const
{
    const static Char* CachedGuidDigits = TEXT("0123456789abcdef");
    return GuidToString<Char>(CachedGuidDigits, buffer, *this, format);
}

template<typename StringType, typename StringViewType>
FORCE_INLINE bool GuidParse(const StringViewType& text, Guid& value)
{
    switch (text.Length())
    {
    // FormatType::N
    case 32:
    {
        return
                StringUtils::ParseHex(*text + 0, 8, &value.A) ||
                StringUtils::ParseHex(*text + 8, 8, &value.B) ||
                StringUtils::ParseHex(*text + 16, 8, &value.C) ||
                StringUtils::ParseHex(*text + 24, 8, &value.D);
    }
    // FormatType::D
    case 36:
    {
        StringType b = StringType(text.Substring(9, 4)) + text.Substring(14, 4);
        StringType c = StringType(text.Substring(19, 4)) + text.Substring(24, 4);
        return
                StringUtils::ParseHex(*text + 0, 8, &value.A) ||
                StringUtils::ParseHex(*b, &value.B) ||
                StringUtils::ParseHex(*c, &value.C) ||
                StringUtils::ParseHex(*text + 28, 8, &value.D);
    }
    // FormatType::B
    // FormatType::P
    case 38:
    {
        StringType b = StringType(text.Substring(10, 4)) + text.Substring(15, 4);
        StringType c = StringType(text.Substring(20, 4)) + text.Substring(25, 4);
        return
                text[0] != text[text.Length() - 1] ||
                StringUtils::ParseHex(*text + 1, 8, &value.A) ||
                StringUtils::ParseHex(*b, &value.B) ||
                StringUtils::ParseHex(*c, &value.C) ||
                StringUtils::ParseHex(*text + 29, 8, &value.D);
    }
    default:
        return true;
    }
}

bool Guid::Parse(const StringView& text, Guid& value)
{
    return GuidParse<String, StringView>(text, value);
}

bool Guid::Parse(const StringAnsiView& text, Guid& value)
{
    return GuidParse<StringAnsi, StringAnsiView>(text, value);
}

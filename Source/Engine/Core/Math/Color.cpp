// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Color.h"
#include "../Types/String.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color32.h"

static_assert(sizeof(Color) == 16, "Invalid Color type size.");

Color::Color(const Vector3& rgb, float a)
    : R(rgb.X)
    , G(rgb.Y)
    , B(rgb.Z)
    , A(a)
{
}

Color::Color(const Vector4& rgba)
    : R(rgba.X)
    , G(rgba.Y)
    , B(rgba.Z)
    , A(rgba.W)
{
}

Color::Color(const Color32& color)
    : R(color.R / 255.0f)
    , G(color.G / 255.0f)
    , B(color.B / 255.0f)
    , A(color.A / 255.0f)
{
}

Color Color::FromHex(const String& hexString, bool& isValid)
{
    int32 r, g, b, a = 255;
    isValid = true;

    int32 startIndex = !hexString.IsEmpty() && hexString[0] == Char('#') ? 1 : 0;
    if (hexString.Length() == 3 + startIndex)
    {
        r = StringUtils::HexDigit(hexString[startIndex++]);
        g = StringUtils::HexDigit(hexString[startIndex++]);
        b = StringUtils::HexDigit(hexString[startIndex]);

        r = (r << 4) + r;
        g = (g << 4) + g;
        b = (b << 4) + b;
    }
    else if (hexString.Length() == 6 + startIndex)
    {
        r = (StringUtils::HexDigit(hexString[startIndex + 0]) << 4) + StringUtils::HexDigit(hexString[startIndex + 1]);
        g = (StringUtils::HexDigit(hexString[startIndex + 2]) << 4) + StringUtils::HexDigit(hexString[startIndex + 3]);
        b = (StringUtils::HexDigit(hexString[startIndex + 4]) << 4) + StringUtils::HexDigit(hexString[startIndex + 5]);
    }
    else if (hexString.Length() == 8 + startIndex)
    {
        r = (StringUtils::HexDigit(hexString[startIndex + 0]) << 4) + StringUtils::HexDigit(hexString[startIndex + 1]);
        g = (StringUtils::HexDigit(hexString[startIndex + 2]) << 4) + StringUtils::HexDigit(hexString[startIndex + 3]);
        b = (StringUtils::HexDigit(hexString[startIndex + 4]) << 4) + StringUtils::HexDigit(hexString[startIndex + 5]);
        a = (StringUtils::HexDigit(hexString[startIndex + 6]) << 4) + StringUtils::HexDigit(hexString[startIndex + 7]);
    }
    else
    {
        r = g = b = 0;
        isValid = false;
    }

    return FromBytes(r, g, b, a);
}

Color Color::FromHSV(float hue, float saturation, float value, float alpha)
{
    const float hDiv60 = hue / 60.0f;
    const float hDiv60Floor = Math::Floor(hDiv60);
    const float hDiv60Fraction = hDiv60 - hDiv60Floor;

    const float rgbValues[4] =
    {
        value,
        value * (1.0f - saturation),
        value * (1.0f - hDiv60Fraction * saturation),
        value * (1.0f - (1.0f - hDiv60Fraction) * saturation),
    };
    const int32 rgbSwizzle[6][3]
    {
        { 0, 3, 1 },
        { 2, 0, 1 },
        { 1, 0, 3 },
        { 1, 2, 0 },
        { 3, 1, 0 },
        { 0, 1, 2 },
    };
    const int32 swizzleIndex = (int32)hDiv60Floor % 6;

    return Color(rgbValues[rgbSwizzle[swizzleIndex][0]],
                 rgbValues[rgbSwizzle[swizzleIndex][1]],
                 rgbValues[rgbSwizzle[swizzleIndex][2]],
                 alpha);
}

Color Color::FromHSV(const Vector3& hsv, float alpha)
{
    return FromHSV(hsv.X, hsv.Y, hsv.Z, alpha);
}

Color Color::Random()
{
    return FromRGB(rand(), 1);
}

String Color::ToString() const
{
    return String::Format(TEXT("{}"), *this);;
}

String Color::ToHexString() const
{
    static const Char* digits = TEXT("0123456789ABCDEF");

    const byte r = static_cast<byte>(R * MAX_uint8);
    const byte g = static_cast<byte>(G * MAX_uint8);
    const byte b = static_cast<byte>(B * MAX_uint8);

    Char result[6];

    result[0] = digits[r >> 4 & 0x0f];
    result[1] = digits[r & 0x0f];

    result[2] = digits[g >> 4 & 0x0f];
    result[3] = digits[g & 0x0f];

    result[4] = digits[b >> 4 & 0x0f];
    result[5] = digits[b & 0x0f];

    return String(result, 6);
}

bool Color::IsTransparent() const
{
    return Math::IsZero(R + G + B + A);
}

bool Color::HasOpacity() const
{
    return !Math::IsOne(A);
}

bool Color::NearEqual(const Color& a, const Color& b)
{
    return Math::NearEqual(a.R, b.R) && Math::NearEqual(a.G, b.G) && Math::NearEqual(a.B, b.B) && Math::NearEqual(a.A, b.A);
}

bool Color::NearEqual(const Color& a, const Color& b, float epsilon)
{
    return Math::NearEqual(a.R, b.R, epsilon) && Math::NearEqual(a.G, b.G, epsilon) && Math::NearEqual(a.B, b.B, epsilon) && Math::NearEqual(a.A, b.A, epsilon);
}

Vector3 Color::ToVector3() const
{
    return Vector3(R, G, B);
}

Vector4 Color::ToVector4() const
{
    return Vector4(R, G, B, A);
}

Vector3 Color::ToHSV() const
{
    const float rgbMin = Math::Min(R, G, B);
    const float rgbMax = Math::Max(R, G, B);
    const float rgbRange = rgbMax - rgbMin;
    const float hue = rgbMax == rgbMin ? 0.0f : rgbMax == R ? Math::Mod((G - B) / rgbRange * 60.0f + 360.0f, 360.0f) : rgbMax == G ? (B - R) / rgbRange * 60.0f + 120.0f : rgbMax == B ? (R - G) / rgbRange * 60.0f + 240.0f : 0.0f;
    const float saturation = rgbMax == 0.0f ? 0.0f : rgbRange / rgbMax;
    const float value = rgbMax;
    return Vector3(hue, saturation, value);
}

void Color::Lerp(const Color& start, const Color& end, float amount, Color& result)
{
    result.R = Math::Lerp(start.R, end.R, amount);
    result.G = Math::Lerp(start.G, end.G, amount);
    result.B = Math::Lerp(start.B, end.B, amount);
    result.A = Math::Lerp(start.A, end.A, amount);
}

Color Color::Lerp(const Color& start, const Color& end, float amount)
{
    Color result;
    Lerp(start, end, amount, result);
    return result;
}

Color Color::LinearToSrgb(const Color& linear)
{
#define LINEAR_TO_SRGB(value) value < 0.00313067f ? value * 12.92f : Math::Pow(value, (1.0f / 2.4f)) * 1.055f - 0.055f
    return Color(LINEAR_TO_SRGB(linear.R), LINEAR_TO_SRGB(linear.G), LINEAR_TO_SRGB(linear.B), LINEAR_TO_SRGB(linear.A));
#undef LINEAR_TO_SRGB
}

Color Color::SrgbToLinear(const Color& srgb)
{
#define SRGB_TO_LINEAR(value) value < 0.04045f ? value / 12.92f : Math::Pow((value + 0.055f) / 1.055f, 2.4f)
    return Color(SRGB_TO_LINEAR(srgb.R), SRGB_TO_LINEAR(srgb.G), SRGB_TO_LINEAR(srgb.B), SRGB_TO_LINEAR(srgb.A));
#undef LINEAR_TO_SRGB
}

uint32 Color::GetHashCode() const
{
    const int32 range = 100000;
    int32 hashCode = (int32)R * range;
    hashCode = hashCode * 397 ^ (int32)(G * range);
    hashCode = hashCode * 397 ^ (int32)(B * range);
    hashCode = hashCode * 397 ^ (int32)(A * range);
    return hashCode;
}

Color Color::Transparent = Color(0, 0, 0, 0);
Color Color::AliceBlue = FromRGB(0xF0F8FF);
Color Color::AntiqueWhite = FromRGB(0xFAEBD7);
Color Color::Aqua = FromRGB(0x00FFFF);
Color Color::Aquamarine = FromRGB(0x7FFFD4);
Color Color::Azure = FromRGB(0xF0FFFF);
Color Color::Beige = FromRGB(0xF5F5DC);
Color Color::Bisque = FromRGB(0xFFE4C4);
Color Color::Black = FromRGB(0x000000);
Color Color::BlanchedAlmond = FromRGB(0xFFEBCD);
Color Color::Blue = FromRGB(0x0000FF);
Color Color::BlueViolet = FromRGB(0x8A2BE2);
Color Color::Brown = FromRGB(0xA52A2A);
Color Color::BurlyWood = FromRGB(0xDEB887);
Color Color::CadetBlue = FromRGB(0x5F9EA0);
Color Color::Chartreuse = FromRGB(0x7FFF00);
Color Color::Chocolate = FromRGB(0xD2691E);
Color Color::Coral = FromRGB(0xFF7F50);
Color Color::CornflowerBlue = FromRGB(0x6495ED);
Color Color::Cornsilk = FromRGB(0xFFF8DC);
Color Color::Crimson = FromRGB(0xDC143C);
Color Color::Cyan = FromRGB(0x00FFFF);
Color Color::DarkBlue = FromRGB(0x00008B);
Color Color::DarkCyan = FromRGB(0x008B8B);
Color Color::DarkGoldenrod = FromRGB(0xB8860B);
Color Color::DarkGray = FromRGB(0xA9A9A9);
Color Color::DarkGreen = FromRGB(0x006400);
Color Color::DarkKhaki = FromRGB(0xBDB76B);
Color Color::DarkMagenta = FromRGB(0x8B008B);
Color Color::DarkOliveGreen = FromRGB(0x556B2F);
Color Color::DarkOrange = FromRGB(0xFF8C00);
Color Color::DarkOrchid = FromRGB(0x9932CC);
Color Color::DarkRed = FromRGB(0x8B0000);
Color Color::DarkSalmon = FromRGB(0xE9967A);
Color Color::DarkSeaGreen = FromRGB(0x8FBC8F);
Color Color::DarkSlateBlue = FromRGB(0x483D8B);
Color Color::DarkSlateGray = FromRGB(0x2F4F4F);
Color Color::DarkTurquoise = FromRGB(0x00CED1);
Color Color::DarkViolet = FromRGB(0x9400D3);
Color Color::DeepPink = FromRGB(0xFF1493);
Color Color::DeepSkyBlue = FromRGB(0x00BFFF);
Color Color::DimGray = FromRGB(0x696969);
Color Color::DodgerBlue = FromRGB(0x1E90FF);
Color Color::Firebrick = FromRGB(0xB22222);
Color Color::FloralWhite = FromRGB(0xFFFAF0);
Color Color::ForestGreen = FromRGB(0x228B22);
Color Color::Fuchsia = FromRGB(0xFF00FF);
Color Color::Gainsboro = FromRGB(0xDCDCDC);
Color Color::GhostWhite = FromRGB(0xF8F8FF);
Color Color::Gold = FromRGB(0xFFD700);
Color Color::Goldenrod = FromRGB(0xDAA520);
Color Color::Gray = FromRGB(0x808080);
Color Color::Green = FromRGB(0x008000);
Color Color::GreenYellow = FromRGB(0xADFF2F);
Color Color::Honeydew = FromRGB(0xF0FFF0);
Color Color::HotPink = FromRGB(0xFF69B4);
Color Color::IndianRed = FromRGB(0xCD5C5C);
Color Color::Indigo = FromRGB(0x4B0082);
Color Color::Ivory = FromRGB(0xFFFFF0);
Color Color::Khaki = FromRGB(0xF0E68C);
Color Color::Lavender = FromRGB(0xE6E6FA);
Color Color::LavenderBlush = FromRGB(0xFFF0F5);
Color Color::LawnGreen = FromRGB(0x7CFC00);
Color Color::LemonChiffon = FromRGB(0xFFFACD);
Color Color::LightBlue = FromRGB(0xADD8E6);
Color Color::LightCoral = FromRGB(0xF08080);
Color Color::LightCyan = FromRGB(0xE0FFFF);
Color Color::LightGoldenrodYellow = FromRGB(0xFAFAD2);
Color Color::LightGreen = FromRGB(0x90EE90);
Color Color::LightGray = FromRGB(0xD3D3D3);
Color Color::LightPink = FromRGB(0xFFB6C1);
Color Color::LightSalmon = FromRGB(0xFFA07A);
Color Color::LightSeaGreen = FromRGB(0x20B2AA);
Color Color::LightSkyBlue = FromRGB(0x87CEFA);
Color Color::LightSlateGray = FromRGB(0x778899);
Color Color::LightSteelBlue = FromRGB(0xB0C4DE);
Color Color::LightYellow = FromRGB(0xFFFFE0);
Color Color::Lime = FromRGB(0x00FF00);
Color Color::LimeGreen = FromRGB(0x32CD32);
Color Color::Linen = FromRGB(0xFAF0E6);
Color Color::Magenta = FromRGB(0xFF00FF);
Color Color::Maroon = FromRGB(0x800000);
Color Color::MediumAquamarine = FromRGB(0x66CDAA);
Color Color::MediumBlue = FromRGB(0x0000CD);
Color Color::MediumOrchid = FromRGB(0xBA55D3);
Color Color::MediumPurple = FromRGB(0x9370DB);
Color Color::MediumSeaGreen = FromRGB(0x3CB371);
Color Color::MediumSlateBlue = FromRGB(0x7B68EE);
Color Color::MediumSpringGreen = FromRGB(0x00FA9A);
Color Color::MediumTurquoise = FromRGB(0x48D1CC);
Color Color::MediumVioletRed = FromRGB(0xC71585);
Color Color::MidnightBlue = FromRGB(0x191970);
Color Color::MintCream = FromRGB(0xF5FFFA);
Color Color::MistyRose = FromRGB(0xFFE4E1);
Color Color::Moccasin = FromRGB(0xFFE4B5);
Color Color::NavajoWhite = FromRGB(0xFFDEAD);
Color Color::Navy = FromRGB(0x000080);
Color Color::OldLace = FromRGB(0xFDF5E6);
Color Color::Olive = FromRGB(0x808000);
Color Color::OliveDrab = FromRGB(0x6B8E23);
Color Color::Orange = FromRGB(0xFFA500);
Color Color::OrangeRed = FromRGB(0xFF4500);
Color Color::Orchid = FromRGB(0xDA70D6);
Color Color::PaleGoldenrod = FromRGB(0xEEE8AA);
Color Color::PaleGreen = FromRGB(0x98FB98);
Color Color::PaleTurquoise = FromRGB(0xAFEEEE);
Color Color::PaleVioletRed = FromRGB(0xDB7093);
Color Color::PapayaWhip = FromRGB(0xFFEFD5);
Color Color::PeachPuff = FromRGB(0xFFDAB9);
Color Color::Peru = FromRGB(0xCD853F);
Color Color::Pink = FromRGB(0xFFC0CB);
Color Color::Plum = FromRGB(0xDDA0DD);
Color Color::PowderBlue = FromRGB(0xB0E0E6);
Color Color::Purple = FromRGB(0x800080);
Color Color::Red = FromRGB(0xFF0000);
Color Color::RosyBrown = FromRGB(0xBC8F8F);
Color Color::RoyalBlue = FromRGB(0x4169E1);
Color Color::SaddleBrown = FromRGB(0x8B4513);
Color Color::Salmon = FromRGB(0xFA8072);
Color Color::SandyBrown = FromRGB(0xF4A460);
Color Color::SeaGreen = FromRGB(0x2E8B57);
Color Color::SeaShell = FromRGB(0xFFF5EE);
Color Color::Sienna = FromRGB(0xA0522D);
Color Color::Silver = FromRGB(0xC0C0C0);
Color Color::SkyBlue = FromRGB(0x87CEEB);
Color Color::SlateBlue = FromRGB(0x6A5ACD);
Color Color::SlateGray = FromRGB(0x708090);
Color Color::Snow = FromRGB(0xFFFAFA);
Color Color::SpringGreen = FromRGB(0x00FF7F);
Color Color::SteelBlue = FromRGB(0x4682B4);
Color Color::Tan = FromRGB(0xD2B48C);
Color Color::Teal = FromRGB(0x008080);
Color Color::Thistle = FromRGB(0xD8BFD8);
Color Color::Tomato = FromRGB(0xFF6347);
Color Color::Turquoise = FromRGB(0x40E0D0);
Color Color::Violet = FromRGB(0xEE82EE);
Color Color::Wheat = FromRGB(0xF5DEB3);
Color Color::White = FromRGB(0xFFFFFF);
Color Color::WhiteSmoke = FromRGB(0xF5F5F5);
Color Color::Yellow = FromRGB(0xFFFF00);
Color Color::YellowGreen = FromRGB(0x9ACD32);

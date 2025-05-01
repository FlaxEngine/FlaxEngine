// Copyright (c) Wojciech Figat. All rights reserved.

#include "Color32.h"
#include "../Types/String.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"

Color32 Color32::Transparent(0, 0, 0, 0);
Color32 Color32::Black(0, 0, 0, 255);
Color32 Color32::White(255, 255, 255, 255);

Color32::Color32(const Color& c)
{
    R = static_cast<byte>(c.R * 255);
    G = static_cast<byte>(c.G * 255);
    B = static_cast<byte>(c.B * 255);
    A = static_cast<byte>(c.A * 255);
}

Color32::Color32(const Vector4& c)
{
    R = static_cast<byte>(c.X * 255);
    G = static_cast<byte>(c.Y * 255);
    B = static_cast<byte>(c.Z * 255);
    A = static_cast<byte>(c.W * 255);
}

String Color32::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

String Color32::ToHexString() const
{
    static const Char* digits = TEXT("0123456789ABCDEF");

    Char result[6];

    result[0] = digits[R >> 4 & 0x0f];
    result[1] = digits[R & 0x0f];

    result[2] = digits[G >> 4 & 0x0f];
    result[3] = digits[G & 0x0f];

    result[4] = digits[B >> 4 & 0x0f];
    result[5] = digits[B & 0x0f];

    return String(result, 6);
}

uint32 Color32::GetHashCode() const
{
    uint32 hashCode = (uint32)R;
    hashCode = hashCode * 397 ^ (uint32)G;
    hashCode = hashCode * 397 ^ (uint32)B;
    hashCode = hashCode * 397 ^ (uint32)A;
    return hashCode;
}

Color32 Color32::Random()
{
    return FromRGB(rand());
}

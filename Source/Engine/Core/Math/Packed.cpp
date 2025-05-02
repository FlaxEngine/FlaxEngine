// Copyright (c) Wojciech Figat. All rights reserved.

#include "Packed.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"

FloatR10G10B10A2::FloatR10G10B10A2(uint32 packed)
    : Value(packed)
{
}

FloatR10G10B10A2::FloatR10G10B10A2(float x, float y, float z, float w)
{
    x = Math::Saturate(x);
    y = Math::Saturate(y);
    z = Math::Saturate(z);
    w = Math::Saturate(w);

    x = Math::Round(x * 1023.0f);
    y = Math::Round(y * 1023.0f);
    z = Math::Round(z * 1023.0f);
    w = Math::Round(w * 3.0f);

    Value = ((uint32)w << 30) | (((uint32)z & 0x3FF) << 20) | (((uint32)y & 0x3FF) << 10) | (((uint32)x & 0x3FF));
}

FloatR10G10B10A2::FloatR10G10B10A2(const Float3& v, float alpha)
    : FloatR10G10B10A2(v.X, v.Y, v.Z, alpha)
{
}

FloatR10G10B10A2::FloatR10G10B10A2(const Float4& v)
    : FloatR10G10B10A2(v.X, v.Y, v.Z, v.W)
{
}

FloatR10G10B10A2::FloatR10G10B10A2(const float* values)
    : FloatR10G10B10A2(values[0], values[1], values[2], values[3])
{
}

FloatR10G10B10A2::operator Float3() const
{
    return ToFloat3();
}

FloatR10G10B10A2::operator Float4() const
{
    return ToFloat4();
}

Float3 FloatR10G10B10A2::ToFloat3() const
{
    Float3 vectorOut;
    uint32 tmp;
    tmp = Value & 0x3FF;
    vectorOut.X = (float)tmp / 1023.f;
    tmp = (Value >> 10) & 0x3FF;
    vectorOut.Y = (float)tmp / 1023.f;
    tmp = (Value >> 20) & 0x3FF;
    vectorOut.Z = (float)tmp / 1023.f;
    return vectorOut;
}

Float4 FloatR10G10B10A2::ToFloat4() const
{
    Float4 vectorOut;
    uint32 tmp;
    tmp = Value & 0x3FF;
    vectorOut.X = (float)tmp / 1023.f;
    tmp = (Value >> 10) & 0x3FF;
    vectorOut.Y = (float)tmp / 1023.f;
    tmp = (Value >> 20) & 0x3FF;
    vectorOut.Z = (float)tmp / 1023.f;
    vectorOut.W = (float)(Value >> 30) / 3.f;
    return vectorOut;
}

FloatR11G11B10::FloatR11G11B10(float x, float y, float z)
{
    // Reference: https://github.com/microsoft/DirectXMath

    uint32 iValue[4];
    iValue[0] = *(uint32*)&x;
    iValue[1] = *(uint32*)&y;
    iValue[2] = *(uint32*)&z;
    iValue[3] = 0;

    // TODO: optimize this?

    uint32 result[3];

    // X & Y Channels (5-bit exponent, 6-bit mantissa)
    for (uint32 j = 0; j < 2; j++)
    {
        uint32 sign = iValue[j] & 0x80000000;
        uint32 i = iValue[j] & 0x7FFFFFFF;

        if ((i & 0x7F800000) == 0x7F800000)
        {
            // INF or NAN
            result[j] = 0x7c0;
            if ((i & 0x7FFFFF) != 0)
            {
                result[j] = 0x7c0 | (((i >> 17) | (i >> 11) | (i >> 6) | (i)) & 0x3f);
            }
            else if (sign)
            {
                // -INF is clamped to 0 since 3PK is positive only
                result[j] = 0;
            }
        }
        else if (sign)
        {
            // 3PK is positive only, so clamp to zero
            result[j] = 0;
        }
        else if (i > 0x477E0000U)
        {
            // The number is too large to be represented as a float11, set to max
            result[j] = 0x7BF;
        }
        else
        {
            if (i < 0x38800000U)
            {
                // The number is too small to be represented as a normalized float11
                // Convert it to a denormalized value.
                uint32 shift = 113U - (i >> 23U);
                i = (0x800000U | (i & 0x7FFFFFU)) >> shift;
            }
            else
            {
                // Rebias the exponent to represent the value as a normalized float11
                i += 0xC8000000U;
            }
            result[j] = ((i + 0xFFFFU + ((i >> 17U) & 1U)) >> 17U) & 0x7ffU;
        }
    }

    // Z Channel (5-bit exponent, 5-bit mantissa)
    uint32 sign = iValue[2] & 0x80000000;
    uint32 i = iValue[2] & 0x7FFFFFFF;

    if ((i & 0x7F800000) == 0x7F800000)
    {
        // INF or NAN
        result[2] = 0x3e0;
        if (i & 0x7FFFFF)
        {
            result[2] = 0x3e0 | (((i >> 18) | (i >> 13) | (i >> 3) | (i)) & 0x1f);
        }
        else if (sign)
        {
            // -INF is clamped to 0 since 3PK is positive only
            result[2] = 0;
        }
    }
    else if (sign)
    {
        // 3PK is positive only, so clamp to zero
        result[2] = 0;
    }
    else if (i > 0x477C0000U)
    {
        // The number is too large to be represented as a float10, set to max
        result[2] = 0x3df;
    }
    else
    {
        if (i < 0x38800000U)
        {
            // The number is too small to be represented as a normalized float10
            // Convert it to a denormalized value.
            uint32 shift = 113U - (i >> 23U);
            i = (0x800000U | (i & 0x7FFFFFU)) >> shift;
        }
        else
        {
            // Rebias the exponent to represent the value as a normalized float10
            i += 0xC8000000U;
        }
        result[2] = ((i + 0x1FFFFU + ((i >> 18U) & 1U)) >> 18U) & 0x3ffU;
    }

    // Pack result into memory
    Value = (result[0] & 0x7ff) | ((result[1] & 0x7ff) << 11) | ((result[2] & 0x3ff) << 22);
}

FloatR11G11B10::FloatR11G11B10(const Float3& v)
    : FloatR11G11B10(v.X, v.Y, v.Z)
{
}

FloatR11G11B10::FloatR11G11B10(const Float4& v)
    : FloatR11G11B10(v.X, v.Y, v.Z)
{
}

FloatR11G11B10::FloatR11G11B10(const Color& v)
    : FloatR11G11B10(v.R, v.G, v.B)
{
}

FloatR11G11B10::FloatR11G11B10(const float* values)
    : FloatR11G11B10(values[0], values[1], values[2])
{
}

FloatR11G11B10::operator Float3() const
{
    return ToFloat3();
}

Float3 FloatR11G11B10::ToFloat3() const
{
    // Reference: https://github.com/microsoft/DirectXMath

    uint32 result[4];
    uint32 mantissa;
    uint32 exponent;

    // TODO: optimize this?

    // X Channel (6-bit mantissa)
    mantissa = xm;

    if (xe == 0x1f) // INF or NAN
    {
        result[0] = 0x7f800000 | (xm << 17);
    }
    else
    {
        if (xe != 0) // The value is normalized
        {
            exponent = xe;
        }
        else if (mantissa != 0) // The value is denormalized
        {
            // Normalize the value in the resulting float
            exponent = 1;

            do
            {
                exponent--;
                mantissa <<= 1;
            } while ((mantissa & 0x40) == 0);

            mantissa &= 0x3F;
        }
        else // The value is zero
        {
            exponent = (uint32)-112;
        }

        result[0] = ((exponent + 112) << 23) | (mantissa << 17);
    }

    // Y Channel (6-bit mantissa)
    mantissa = ym;

    if (ye == 0x1f) // INF or NAN
    {
        result[1] = 0x7f800000 | (ym << 17);
    }
    else
    {
        if (ye != 0) // The value is normalized
        {
            exponent = ye;
        }
        else if (mantissa != 0) // The value is denormalized
        {
            // Normalize the value in the resulting float
            exponent = 1;

            do
            {
                exponent--;
                mantissa <<= 1;
            } while ((mantissa & 0x40) == 0);

            mantissa &= 0x3F;
        }
        else // The value is zero
        {
            exponent = (uint32)-112;
        }

        result[1] = ((exponent + 112) << 23) | (mantissa << 17);
    }

    // Z Channel (5-bit mantissa)
    mantissa = zm;

    if (ze == 0x1f) // INF or NAN
    {
        result[2] = 0x7f800000 | (zm << 17);
    }
    else
    {
        if (ze != 0) // The value is normalized
        {
            exponent = ze;
        }
        else if (mantissa != 0) // The value is denormalized
        {
            // Normalize the value in the resulting float
            exponent = 1;

            do
            {
                exponent--;
                mantissa <<= 1;
            } while ((mantissa & 0x20) == 0);

            mantissa &= 0x1F;
        }
        else // The value is zero
        {
            exponent = (uint32)-112;
        }

        result[2] = ((exponent + 112) << 23) | (mantissa << 18);
    }

    return Float3((float*)result);
}

Float2 RG16UNorm::ToFloat2() const
{
    return Float2(
        (float)X / MAX_uint16,
        (float)Y / MAX_uint16
    );
}

Float4 RGBA16UNorm::ToFloat4() const
{
    return Float4(
        (float)X / MAX_uint16,
        (float)Y / MAX_uint16,
        (float)Z / MAX_uint16,
        (float)W / MAX_uint16
    );
}

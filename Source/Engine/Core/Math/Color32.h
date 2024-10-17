// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Color;

/// <summary>
/// Representation of RGBA colors in 32 bit format (8 bits per component in RGBA order).
/// </summary>
API_STRUCT(InBuild) struct FLAXENGINE_API Color32
{
public:
    union
    {
        struct
        {
            /// <summary>
            /// Red component of the color.
            /// </summary>
            byte R;

            /// <summary>
            /// Green component of the color.
            /// </summary>
            byte G;

            /// <summary>
            /// Blue component of the color.
            /// </summary>
            byte B;

            /// <summary>
            /// Alpha component of the color.
            /// </summary>
            byte A;
        };

        /// <summary>
        /// Color packed into 32 bits
        /// </summary>
        uint32 Raw;
    };

public:
    static Color32 Transparent;
    static Color32 Black;
    static Color32 White;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Color32() = default;

    /// <summary>
    /// Constructs a new Color32 with given r, g, b, a components.
    /// </summary>
    /// <param name="r">The red component value.</param>
    /// <param name="g">The green component value.</param>
    /// <param name="b">The value component value.</param>
    /// <param name="a">The alpha component value.</param>
    Color32(byte r, byte g, byte b, byte a)
    {
        R = r;
        G = g;
        B = b;
        A = a;
    }

    explicit Color32(const Color& c);
    explicit Color32(const Vector4& c);

public:
    String ToString() const;
    String ToHexString() const;

public:
    bool operator==(const Color32& other) const
    {
        return Raw == other.Raw;
    }

    bool operator!=(const Color32& other) const
    {
        return Raw != other.Raw;
    }

    Color32 operator+(const Color32& b) const
    {
        return Color32(R + b.R, G + b.G, B + b.B, A + b.A);
    }

    Color32 operator-(const Color32& b) const
    {
        return Color32(R - b.R, G - b.G, B - b.B, A - b.A);
    }

    Color32 operator*(const Color32& b) const
    {
        return Color32(R * b.R, G * b.G, B * b.B, A * b.A);
    }

    Color32& operator+=(const Color32& b)
    {
        R += b.R;
        G += b.G;
        B += b.B;
        A += b.A;
        return *this;
    }

    Color32& operator-=(const Color32& b)
    {
        R -= b.R;
        G -= b.G;
        B -= b.B;
        A -= b.A;
        return *this;
    }

    Color32& operator*=(const Color32& b)
    {
        R *= b.R;
        G *= b.G;
        B *= b.B;
        A *= b.A;
        return *this;
    }

    Color32 operator*(float b) const
    {
        return Color32((byte)((float)R * b), (byte)((float)G * b), (byte)((float)B * b), (byte)((float)A * b));
    }

    // Returns true if color is fully transparent (all components are equal zero).
    bool IsTransparent() const
    {
        return Raw == 0;
    }

    // Returns true if color has opacity channel in use (different from 255).
    bool HasOpacity() const
    {
        return A != 255;
    }

public:
    uint32 GetAsABGR() const
    {
        return (R << 24) + (G << 16) + (B << 8) + A;
    }

    uint32 GetAsBGRA() const
    {
        return (A << 24) + (R << 16) + (G << 8) + B;
    }

    uint32 GetAsARGB() const
    {
        return (B << 24) + (G << 16) + (R << 8) + A;
    }

    uint32 GetAsRGB() const
    {
        return (R << 16) + (G << 8) + B;
    }

    uint32 GetAsRGBA() const
    {
        return Raw;
    }

public:
    uint32 GetHashCode() const;

    static uint32 GetHashCode(const Color32& v)
    {
        return v.GetHashCode();
    }

public:
    /// <summary>
    /// Initializes from packed RGB value of the color and separate alpha channel value.
    /// </summary>
    /// <param name="rgb">The packed RGB value.</param>
    /// <param name="a">The alpha channel.</param>
    /// <returns>The color.</returns>
    static Color32 FromRGB(uint32 rgb, byte a = 255)
    {
        return Color32(static_cast<byte>(rgb >> 16 & 0xff), static_cast<byte>(rgb >> 8 & 0xff), static_cast<byte>(rgb & 0xff), a);
    }

    static Color32 FromRGBA(float r, float g, float b, float a)
    {
        return Color32(static_cast<byte>(r * 255), static_cast<byte>(g * 255), static_cast<byte>(b * 255), static_cast<byte>(a * 255));
    }

    // Gets random color with opaque alpha.
    static Color32 Random();

    /// <summary>
    /// Linearly interpolates between colors a and b by normalized weight t.
    /// </summary>
    /// <param name="a">The start value.</param>
    /// <param name="b">The end value.</param>
    /// <param name="t">The linear blend weight.</param>
    static Color32 Lerp(const Color32& a, const Color32& b, const float t)
    {
        return Color32((byte)(a.R + (b.R - a.R) * t), (byte)(a.G + (b.G - a.G) * t), (byte)(a.B + (b.B - a.B) * t), (byte)(a.A + (b.A - a.A) * t));
    }
};

inline Color32 operator*(float a, const Color32& b)
{
    return b * a;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Color32& a, const Color32& b)
    {
        return a.Raw == b.Raw;
    }
}

template<>
struct TIsPODType<Color32>
{
    enum { Value = true };
};

inline uint32 GetHash(const Color32& key)
{
    return key.GetHashCode();
}

DEFINE_DEFAULT_FORMATTING(Color32, "R:{0} G:{1} B:{2} A:{3}", v.R, v.G, v.B, v.A);

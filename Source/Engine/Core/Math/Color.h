// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Formatting.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Templates.h"

struct Color32;

/// <summary>
/// Representation of the RGBA color.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Color
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Color);
public:
    union
    {
        struct
        {
            /// <summary>
            /// The red channel value.
            /// </summary>
            API_FIELD() float R;

            /// <summary>
            /// The green channel value.
            /// </summary>
            API_FIELD() float G;

            /// <summary>
            /// The blue channel value.
            /// </summary>
            API_FIELD() float B;

            /// <summary>
            /// The alpha channel value.
            /// </summary>
            API_FIELD() float A;
        };

        /// <summary>
        /// The packed values into array floats.
        /// </summary>
        float Raw[4];
    };

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Color() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Color"/> struct.
    /// </summary>
    /// <param name="rgba">The RGBA channels value.</param>
    explicit Color(float rgba)
        : R(rgba)
        , G(rgba)
        , B(rgba)
        , A(rgba)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Color"/> struct.
    /// </summary>
    /// <param name="r">The red channel value.</param>
    /// <param name="g">The green channel value.</param>
    /// <param name="b">The blue channel value.</param>
    /// <param name="a">The alpha channel value.</param>
    FORCE_INLINE Color(float r, float g, float b, float a = 1)
        : R(r)
        , G(g)
        , B(b)
        , A(a)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Color"/> struct.
    /// </summary>
    /// <param name="rgb">The red, green and blue channels value.</param>
    /// <param name="a">The alpha channel value.</param>
    Color(const Color& rgb, float a)
        : R(rgb.R)
        , G(rgb.G)
        , B(rgb.B)
        , A(a)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Color"/> struct.
    /// </summary>
    /// <param name="rgb">The red, green and blue channels value.</param>
    /// <param name="a">The alpha channel value.</param>
    Color(const Float3& rgb, float a);

    /// <summary>
    /// Initializes a new instance of the <see cref="Color"/> struct.
    /// </summary>
    /// <param name="rgba">The red, green, blue and alpha channels value.</param>
    explicit Color(const Float4& rgba);

    /// <summary>
    /// Initializes a new instance of the <see cref="Color"/> struct.
    /// </summary>
    /// <param name="color">The other color (32-bit RGBA).</param>
    explicit Color(const Color32& color);

public:
    /// <summary>
    /// Initializes from values in range [0;255].
    /// </summary>
    /// <param name="r">The red channel.</param>
    /// <param name="g">The green channel.</param>
    /// <param name="b">The blue channel.</param>
    /// <param name="a">The alpha channel.</param>
    /// <returns>The color.</returns>
    static Color FromBytes(byte r, byte g, byte b, byte a = 255)
    {
        return Color((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
    }

    /// <summary>
    /// Initializes from packed RGB value (bottom bits contain Blue) of the color and separate alpha channel value.
    /// </summary>
    /// <param name="rgb">The packed RGB value (bottom bits contain Blue).</param>
    /// <param name="a">The alpha channel.</param>
    /// <returns>The color.</returns>
    static Color FromRGB(uint32 rgb, float a = 1.0f)
    {
        return Color(static_cast<float>(rgb >> 16 & 0xff) / 255.0f, static_cast<float>(rgb >> 8 & 0xff) / 255.0f, static_cast<float>(rgb & 0xff) / 255.0f, a);
    }

    /// <summary>
    /// Initializes from packed ARGB value (bottom bits contain Blue).
    /// </summary>
    /// <param name="argb">The packed ARGB value (bottom bits contain Blue).</param>
    /// <returns>The color.</returns>
    static Color FromARGB(uint32 argb)
    {
        return Color((float)((argb >> 16) & 0xff) / 255.0f,(float)((argb >> 8) & 0xff) / 255.0f, (float)((argb >> 0) & 0xff) / 255.0f, (float)((argb >> 24) & 0xff) / 255.0f);
    }

    /// <summary>
    /// Initializes from packed RGBA value (bottom bits contain Alpha).
    /// </summary>
    /// <param name="rgba">The packed RGBA value (bottom bits contain Alpha).</param>
    /// <returns>The color.</returns>
    static Color FromRGBA(uint32 rgba)
    {
        return Color((float)((rgba >> 24) & 0xff) / 255.0f,(float)((rgba >> 16) & 0xff) / 255.0f, (float)((rgba >> 8) & 0xff) / 255.0f, (float)((rgba >> 0) & 0xff) / 255.0f);
    }

    static Color FromHex(const String& hex)
    {
        bool isValid;
        return FromHex(hex, isValid);
    }

    static Color FromHex(const String& hex, bool& isValid);

    /// <summary>
    /// Creates RGB color from Hue[0-360], Saturation[0-1] and Value[0-1].
    /// </summary>
    /// <param name="hue">The hue angle in degrees [0-360].</param>
    /// <param name="saturation">The saturation normalized [0-1].</param>
    /// <param name="value">The value normalized [0-1].</param>
    /// <param name="alpha">The alpha value. Default is 1.</param>
    /// <returns>The RGB color.</returns>
    static Color FromHSV(float hue, float saturation, float value, float alpha = 1.0f);

    /// <summary>
    /// Creates RGB color from Hue[0-360], Saturation[0-1] and Value[0-1] packed to XYZ vector.
    /// </summary>
    /// <param name="hsv">The HSV color.</param>
    /// <param name="alpha">The alpha value. Default is 1.</param>
    /// <returns>The RGB color.</returns>
    static Color FromHSV(const Float3& hsv, float alpha = 1.0f);

    /// <summary>
    /// Gets random color with opaque alpha.
    /// </summary>
    /// <returns>The color.</returns>
    static Color Random();

public:
    String ToString() const;
    String ToHexString() const;

public:
    bool operator==(const Color& other) const
    {
        return R == other.R && G == other.G && B == other.B && A == other.A;
    }

    bool operator!=(const Color& other) const
    {
        return R != other.R || G != other.G || B != other.B || A != other.A;
    }

    Color operator+(const Color& b) const
    {
        return Color(R + b.R, G + b.G, B + b.B, A + b.A);
    }

    Color operator-(const Color& b) const
    {
        return Color(R - b.R, G - b.G, B - b.B, A - b.A);
    }

    FORCE_INLINE Color operator*(const Color& b) const
    {
        return Color(R * b.R, G * b.G, B * b.B, A * b.A);
    }

    Color& operator+=(const Color& b)
    {
        R += b.R;
        G += b.G;
        B += b.B;
        A += b.A;
        return *this;
    }

    Color& operator-=(const Color& b)
    {
        R -= b.R;
        G -= b.G;
        B -= b.B;
        A -= b.A;
        return *this;
    }

    Color& operator*=(const Color& b)
    {
        R *= b.R;
        G *= b.G;
        B *= b.B;
        A *= b.A;
        return *this;
    }

    Color& operator*=(const float b)
    {
        R = R * b;
        G = G * b;
        B = B * b;
        A = A * b;
        return *this;
    }

    Color operator+(float b) const
    {
        return Color(R + b, G + b, B + b, A + b);
    }

    Color operator-(float b) const
    {
        return Color(R - b, G - b, B - b, A - b);
    }

    Color operator*(float b) const
    {
        return Color(R * b, G * b, B * b, A * b);
    }

    Color operator/(float b) const
    {
        return Color(R / b, G / b, B / b, A / b);
    }

    // Returns true if color is fully transparent (all components are equal zero).
    bool IsTransparent() const;

    // Returns true if color has opacity channel in use (different from 1).
    bool HasOpacity() const;

public:
    static bool NearEqual(const Color& a, const Color& b);
    static bool NearEqual(const Color& a, const Color& b, float epsilon);

public:
    // Get color as vector structure.
    Float3 ToFloat3() const;

    // Get color as vector structure.
    Float4 ToFloat4() const;

    /// <summary>
    /// Gets Hue[0-360], Saturation[0-1] and Value[0-1] from RGB color.
    /// </summary>
    /// <returns>HSV color</returns>
    Float3 ToHSV() const;

    /// <summary>
    /// Performs a linear interpolation between two colors.
    /// </summary>
    /// <param name="start">The start color.</param>
    /// <param name="end">The end color.</param>
    /// <param name="amount">The value between 0 and 1 indicating the weight of interpolation.</param>
    /// <param name="result">When the method completes, contains the linear interpolation of the two colors.</param>
    static void Lerp(const Color& start, const Color& end, float amount, Color& result);

    /// <summary>
    /// Performs a linear interpolation between two colors.
    /// </summary>
    /// <param name="start">The start color.</param>
    /// <param name="end">The end color.</param>
    /// <param name="amount">The value between 0 and 1 indicating the weight of interpolation.</param>
    /// <returns>The linear interpolation of the two colors.</returns>
    static Color Lerp(const Color& start, const Color& end, float amount);

    // Converts a [0.0, 1.0] linear value into a [0.0, 1.0] sRGB value.
    static Color LinearToSrgb(const Color& linear);

    // Converts a [0.0, 1.0] sRGB value into a [0.0, 1.0] linear value.
    static Color SrgbToLinear(const Color& srgb);

    /// <summary>
    /// Returns the color with RGB channels multiplied by the given scale factor. The alpha channel remains the same.
    /// </summary>
    /// <param name="multiplier">The multiplier.</param>
    /// <returns>The modified color.</returns>
    Color RGBMultiplied(float multiplier) const
    {
        return Color(R * multiplier, G * multiplier, B * multiplier, A);
    }

    /// <summary>
    /// Returns the color with RGB channels multiplied by the given color. The alpha channel remains the same.
    /// </summary>
    /// <param name="multiplier">The multiplier.</param>
    /// <returns>The modified color.</returns>
    Color RGBMultiplied(Color multiplier) const
    {
        return Color(R * multiplier.R, G * multiplier.G, B * multiplier.B, A);
    }

    /// <summary>
    /// Returns the color with alpha channel multiplied by the given scale factor. The RGB channels remain the same.
    /// </summary>
    /// <param name="multiplier">The multiplier.</param>
    /// <returns>The modified color.</returns>
    Color AlphaMultiplied(float multiplier) const
    {
        return Color(R, G, B, A * multiplier);
    }

public:
    uint32 GetHashCode() const;

    static uint32 GetHashCode(const Color& v)
    {
        return v.GetHashCode();
    }

public:
    static Color Transparent;
    static Color AliceBlue;
    static Color AntiqueWhite;
    static Color Aqua;
    static Color Aquamarine;
    static Color Azure;
    static Color Beige;
    static Color Bisque;
    static Color Black;
    static Color BlanchedAlmond;
    static Color Blue;
    static Color BlueViolet;
    static Color Brown;
    static Color BurlyWood;
    static Color CadetBlue;
    static Color Chartreuse;
    static Color Chocolate;
    static Color Coral;
    static Color CornflowerBlue;
    static Color Cornsilk;
    static Color Crimson;
    static Color Cyan;
    static Color DarkBlue;
    static Color DarkCyan;
    static Color DarkGoldenrod;
    static Color DarkGray;
    static Color DarkGreen;
    static Color DarkKhaki;
    static Color DarkMagenta;
    static Color DarkOliveGreen;
    static Color DarkOrange;
    static Color DarkOrchid;
    static Color DarkRed;
    static Color DarkSalmon;
    static Color DarkSeaGreen;
    static Color DarkSlateBlue;
    static Color DarkSlateGray;
    static Color DarkTurquoise;
    static Color DarkViolet;
    static Color DeepPink;
    static Color DeepSkyBlue;
    static Color DimGray;
    static Color DodgerBlue;
    static Color Firebrick;
    static Color FloralWhite;
    static Color ForestGreen;
    static Color Fuchsia;
    static Color Gainsboro;
    static Color GhostWhite;
    static Color Gold;
    static Color Goldenrod;
    static Color Gray;
    static Color Green;
    static Color GreenYellow;
    static Color Honeydew;
    static Color HotPink;
    static Color IndianRed;
    static Color Indigo;
    static Color Ivory;
    static Color Khaki;
    static Color Lavender;
    static Color LavenderBlush;
    static Color LawnGreen;
    static Color LemonChiffon;
    static Color LightBlue;
    static Color LightCoral;
    static Color LightCyan;
    static Color LightGoldenrodYellow;
    static Color LightGreen;
    static Color LightGray;
    static Color LightPink;
    static Color LightSalmon;
    static Color LightSeaGreen;
    static Color LightSkyBlue;
    static Color LightSlateGray;
    static Color LightSteelBlue;
    static Color LightYellow;
    static Color Lime;
    static Color LimeGreen;
    static Color Linen;
    static Color Magenta;
    static Color Maroon;
    static Color MediumAquamarine;
    static Color MediumBlue;
    static Color MediumOrchid;
    static Color MediumPurple;
    static Color MediumSeaGreen;
    static Color MediumSlateBlue;
    static Color MediumSpringGreen;
    static Color MediumTurquoise;
    static Color MediumVioletRed;
    static Color MidnightBlue;
    static Color MintCream;
    static Color MistyRose;
    static Color Moccasin;
    static Color NavajoWhite;
    static Color Navy;
    static Color OldLace;
    static Color Olive;
    static Color OliveDrab;
    static Color Orange;
    static Color OrangeRed;
    static Color Orchid;
    static Color PaleGoldenrod;
    static Color PaleGreen;
    static Color PaleTurquoise;
    static Color PaleVioletRed;
    static Color PapayaWhip;
    static Color PeachPuff;
    static Color Peru;
    static Color Pink;
    static Color Plum;
    static Color PowderBlue;
    static Color Purple;
    static Color Red;
    static Color RosyBrown;
    static Color RoyalBlue;
    static Color SaddleBrown;
    static Color Salmon;
    static Color SandyBrown;
    static Color SeaGreen;
    static Color SeaShell;
    static Color Sienna;
    static Color Silver;
    static Color SkyBlue;
    static Color SlateBlue;
    static Color SlateGray;
    static Color Snow;
    static Color SpringGreen;
    static Color SteelBlue;
    static Color Tan;
    static Color Teal;
    static Color Thistle;
    static Color Tomato;
    static Color Turquoise;
    static Color Violet;
    static Color Wheat;
    static Color White;
    static Color WhiteSmoke;
    static Color Yellow;
    static Color YellowGreen;
};

inline Color operator+(float a, const Color& b)
{
    return b + a;
}

inline Color operator*(float a, const Color& b)
{
    return b * a;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Color& a, const Color& b)
    {
        return Color::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Color>
{
    enum { Value = true };
};

inline uint32 GetHash(const Color& key)
{
    return key.GetHashCode();
}

DEFINE_DEFAULT_FORMATTING(Color, "R:{0} G:{1} B:{2} A:{3}", v.R, v.G, v.B, v.A);

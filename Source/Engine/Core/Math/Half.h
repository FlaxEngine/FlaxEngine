// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Vector2.h"
#include "Vector3.h"

/// <summary>
/// Half-precision 16 bit floating point number consisting of a sign bit, a 5 bit biased exponent, and a 10 bit mantissa
/// </summary>
API_TYPEDEF() typedef uint16 Half;

#define USE_SSE_HALF_CONVERSION 0

/// <summary>
/// Utility for packing/unpacking floating point value from single precision (32 bit) to half precision (16 bit).
/// </summary>
class FLAXENGINE_API Float16Compressor
{
    // Reference:
    // http://www.cs.cmu.edu/~jinlianw/third_party/float16_compressor.hpp

    union Bits
    {
        float f;
        int32 si;
        uint32 ui;
    };

    static const int shift = 13;
    static const int shiftSign = 16;
    static const int32 infN = 0x7F800000; // flt32 infinity
    static const int32 maxN = 0x477FE000; // max flt16 normal as a flt32
    static const int32 minN = 0x38800000; // min flt16 normal as a flt32
    static const int32 signN = 0x80000000; // flt32 sign bit
    static const int32 infC = infN >> shift;
    static const int32 nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
    static const int32 maxC = maxN >> shift;
    static const int32 minC = minN >> shift;
    static const int32 signC = signN >> shiftSign; // flt16 sign bit
    static const int32 mulN = 0x52000000; // (1 << 23) / minN
    static const int32 mulC = 0x33800000; // minN / (1 << (23 - shift))
    static const int32 subC = 0x003FF; // max flt32 subnormal down shifted
    static const int32 norC = 0x00400; // min flt32 normal down shifted
    static const int32 maxD = infC - maxC - 1;
    static const int32 minD = minC - subC - 1;

public:
#if USE_SSE_HALF_CONVERSION
    FORCE_INLINE static Half Compress(float value)
    {
		__m128 value1 = _mm_set_ss(value);
		__m128i value2 = _mm_cvtps_ph(value1, 0);
		return static_cast<Half>(_mm_cvtsi128_si32(value2));
    }
    FORCE_INLINE static float Decompress(Half value)
    {
		__m128i value1 = _mm_cvtsi32_si128(static_cast<int>(value));
		__m128 value2 = _mm_cvtph_ps(value1);
		return _mm_cvtss_f32(value2);
    }
#else
    static Half Compress(float value);
    static float Decompress(Half value);
#endif
};

/// <summary>
/// Defines a two component vector, using half precision floating point coordinates.
/// </summary>
struct FLAXENGINE_API Half2
{
public:
    /// <summary>
    /// Zero vector
    /// </summary>
    static Half2 Zero;

public:
    /// <summary>
    /// Gets or sets the X component of the vector.
    /// </summary>
    Half X;

    /// <summary>
    /// Gets or sets the Y component of the vector.
    /// </summary>
    Half Y;

public:
    /// <summary>
    /// Default constructor
    /// </summary>
    Half2() = default;

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="x">X component</param>
    /// <param name="y">Y component</param>
    FORCE_INLINE Half2(Half x, Half y)
        : X(x)
        , Y(y)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="x">X component</param>
    /// <param name="y">Y component</param>
    FORCE_INLINE Half2(float x, float y)
    {
        X = Float16Compressor::Compress(x);
        Y = Float16Compressor::Compress(y);
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="v">X and Y components</param>
    FORCE_INLINE Half2(const Float2& v)
    {
        X = Float16Compressor::Compress(v.X);
        Y = Float16Compressor::Compress(v.Y);
    }

public:
    Float2 ToFloat2() const;
};

/// <summary>
/// Defines a three component vector, using half precision floating point coordinates.
/// </summary>
struct FLAXENGINE_API Half3
{
public:
    /// <summary>
    /// Zero vector
    /// </summary>
    static Half3 Zero;

public:
    /// <summary>
    /// Gets or sets the X component of the vector.
    /// </summary>
    Half X;

    /// <summary>
    /// Gets or sets the Y component of the vector.
    /// </summary>
    Half Y;

    /// <summary>
    /// Gets or sets the Z component of the vector.
    /// </summary>
    Half Z;

public:
    Half3() = default;

    FORCE_INLINE Half3(Half x, Half y, Half z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }

    FORCE_INLINE Half3(float x, float y, float z)
    {
        X = Float16Compressor::Compress(x);
        Y = Float16Compressor::Compress(y);
        Z = Float16Compressor::Compress(z);
    }

    FORCE_INLINE Half3(const Float3& v)
    {
        X = Float16Compressor::Compress(v.X);
        Y = Float16Compressor::Compress(v.Y);
        Z = Float16Compressor::Compress(v.Z);
    }

public:
    Float3 ToFloat3() const;
};

/// <summary>
/// Defines a four component vector, using half precision floating point coordinates.
/// </summary>
struct FLAXENGINE_API Half4
{
public:
    /// <summary>
    /// Zero vector
    /// </summary>
    static Half4 Zero;

public:
    /// <summary>
    /// Gets or sets the X component of the vector.
    /// </summary>
    Half X;

    /// <summary>
    /// Gets or sets the Y component of the vector.
    /// </summary>
    Half Y;

    /// <summary>
    /// Gets or sets the Z component of the vector.
    /// </summary>
    Half Z;

    /// <summary>
    /// Gets or sets the W component of the vector.
    /// </summary>
    Half W;

public:
    Half4() = default;

    Half4(Half x, Half y, Half z, Half w)
        : X(x)
        , Y(y)
        , Z(z)
        , W(w)
    {
    }

    Half4(const float x, const float y, const float z)
    {
        X = Float16Compressor::Compress(x);
        Y = Float16Compressor::Compress(y);
        Z = Float16Compressor::Compress(z);
        W = 0;
    }

    Half4(const float x, const float y, const float z, const float w)
    {
        X = Float16Compressor::Compress(x);
        Y = Float16Compressor::Compress(y);
        Z = Float16Compressor::Compress(z);
        W = Float16Compressor::Compress(w);
    }

    explicit Half4(const Float4& v);
    explicit Half4(const Color& c);
    explicit Half4(const Rectangle& rect);

public:
    Float2 ToFloat2() const;
    Float3 ToFloat3() const;
    Float4 ToFloat4() const;
};

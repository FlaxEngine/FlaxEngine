// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"

/// <summary>
/// Half-precision 16 bit floating point number consisting of a sign bit, a 5 bit biased exponent, and a 10 bit mantissa
/// </summary>
typedef uint16 Half;

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

    static Half Compress(const float value)
    {
#if USE_SSE_HALF_CONVERSION
		__m128 value1 = _mm_set_ss(value);
		__m128i value2 = _mm_cvtps_ph(value1, 0);
		return static_cast<Half>(_mm_cvtsi128_si32(value2));
#else
        Bits v, s;
        v.f = value;
        uint32 sign = v.si & signN;
        v.si ^= sign;
        sign >>= shiftSign; // logical shift
        s.si = mulN;
        s.si = static_cast<int32>(s.f * v.f); // correct subnormals
        v.si ^= (s.si ^ v.si) & -(minN > v.si);
        v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
        v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
        v.ui >>= shift; // logical shift
        v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
        v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
        return v.ui | sign;
#endif
    }

    static float Decompress(const Half value)
    {
#if USE_SSE_HALF_CONVERSION
		__m128i value1 = _mm_cvtsi32_si128(static_cast<int>(value));
		__m128 value2 = _mm_cvtph_ps(value1);
		return _mm_cvtss_f32(value2);
#else
        Bits v;
        v.ui = value;
        int32 sign = v.si & signC;
        v.si ^= sign;
        sign <<= shiftSign;
        v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
        v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
        Bits s;
        s.si = mulC;
        s.f *= v.si;
        const int32 mask = -(norC > v.si);
        v.si <<= shift;
        v.si ^= (s.si ^ v.si) & mask;
        v.si |= sign;
        return v.f;
#endif
    }
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
    Half2()
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="x">X component</param>
    /// <param name="y">Y component</param>
    Half2(float x, float y)
    {
        X = Float16Compressor::Compress(x);
        Y = Float16Compressor::Compress(y);
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="v">X and Y components</param>
    Half2(const Vector2& v);

public:

    /// <summary>
    /// Convert to Vector2
    /// </summary>
    /// <returns>Vector2</returns>
    Vector2 ToVector2() const;
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

    Half3()
    {
    }

    Half3(const float x, const float y, const float z)
    {
        X = Float16Compressor::Compress(x);
        Y = Float16Compressor::Compress(y);
        Z = Float16Compressor::Compress(z);
    }

    Half3(const Vector3& v);

public:

    Vector3 ToVector3() const;
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

    Half4()
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

    explicit Half4(const Vector4& v);
    explicit Half4(const Color& c);
    explicit Half4(const Rectangle& rect);

public:

    Vector2 ToVector2() const;
    Vector3 ToVector3() const;
    Vector4 ToVector4() const;
};

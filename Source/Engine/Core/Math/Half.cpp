// Copyright (c) Wojciech Figat. All rights reserved.

#include "Half.h"
#include "Vector4.h"
#include "Rectangle.h"
#include "Color.h"

static_assert(sizeof(Half) == 2, "Invalid Half type size.");
static_assert(sizeof(Half2) == 4, "Invalid Half2 type size.");
static_assert(sizeof(Half3) == 6, "Invalid Half3 type size.");
static_assert(sizeof(Half4) == 8, "Invalid Half4 type size.");

Half2 Half2::Zero(0.0f, 0.0f);
Half3 Half3::Zero(0.0f, 0.0f, 0.0f);
Half4 Half4::Zero(0.0f, 0.0f, 0.0f, 0.0f);

#if !PLATFORM_SIMD_F16C

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

Half Float16Compressor::Compress(float value)
{
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
}

float Float16Compressor::Decompress(Half value)
{
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
}

#endif

Float2 Half2::ToFloat2() const
{
    return Float2(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y)
    );
}

Float3 Half3::ToFloat3() const
{
    return Float3(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y),
        Float16Compressor::Decompress(Z)
    );
}

Half4::Half4(const Float4& v)
{
    X = Float16Compressor::Compress(v.X);
    Y = Float16Compressor::Compress(v.Y);
    Z = Float16Compressor::Compress(v.Z);
    W = Float16Compressor::Compress(v.W);
}

Half4::Half4(const Color& c)
{
    X = Float16Compressor::Compress(c.R);
    Y = Float16Compressor::Compress(c.G);
    Z = Float16Compressor::Compress(c.B);
    W = Float16Compressor::Compress(c.A);
}

Half4::Half4(const Rectangle& rect)
{
    X = Float16Compressor::Compress(rect.Location.X);
    Y = Float16Compressor::Compress(rect.Location.Y);
    Z = Float16Compressor::Compress(rect.Size.X);
    W = Float16Compressor::Compress(rect.Size.Y);
}

Float2 Half4::ToFloat2() const
{
    return Float2(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y)
    );
}

Float3 Half4::ToFloat3() const
{
    return Float3(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y),
        Float16Compressor::Decompress(Z)
    );
}

Float4 Half4::ToFloat4() const
{
    return Float4(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y),
        Float16Compressor::Decompress(Z),
        Float16Compressor::Decompress(W)
    );
}

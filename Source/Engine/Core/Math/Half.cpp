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

#if !USE_SSE_HALF_CONVERSION

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

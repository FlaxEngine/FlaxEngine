// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Half.h"
#include "Rectangle.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"

static_assert(sizeof(Half) == 2, "Invalid Half type size.");
static_assert(sizeof(Half2) == 4, "Invalid Half2 type size.");
static_assert(sizeof(Half3) == 6, "Invalid Half3 type size.");
static_assert(sizeof(Half4) == 8, "Invalid Half4 type size.");

Half2 Half2::Zero(0.0f, 0.0f);
Half3 Half3::Zero(0.0f, 0.0f, 0.0f);
Half4 Half4::Zero(0.0f, 0.0f, 0.0f, 0.0f);

Half2::Half2(const Float2& v)
{
    X = Float16Compressor::Compress(v.X);
    Y = Float16Compressor::Compress(v.Y);
}

Float2 Half2::ToFloat2() const
{
    return Float2(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y)
    );
}

Half3::Half3(const Float3& v)
{
    X = Float16Compressor::Compress(v.X);
    Y = Float16Compressor::Compress(v.Y);
    Z = Float16Compressor::Compress(v.Z);
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

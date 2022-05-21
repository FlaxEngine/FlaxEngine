// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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

Half2::Half2(const Vector2& v)
{
    X = Float16Compressor::Compress(v.X);
    Y = Float16Compressor::Compress(v.Y);
}

Vector2 Half2::ToVector2() const
{
    return Vector2(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y)
    );
}

Half3::Half3(const Vector3& v)
{
    X = Float16Compressor::Compress(v.X);
    Y = Float16Compressor::Compress(v.Y);
    Z = Float16Compressor::Compress(v.Z);
}

Vector3 Half3::ToVector3() const
{
    return Vector3(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y),
        Float16Compressor::Decompress(Z)
    );
}

Half4::Half4(const Vector4& v)
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

Vector2 Half4::ToVector2() const
{
    return Vector2(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y)
    );
}

Vector3 Half4::ToVector3() const
{
    return Vector3(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y),
        Float16Compressor::Decompress(Z)
    );
}

Vector4 Half4::ToVector4() const
{
    return Vector4(
        Float16Compressor::Decompress(X),
        Float16Compressor::Decompress(Y),
        Float16Compressor::Decompress(Z),
        Float16Compressor::Decompress(W)
    );
}

// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

Half2 Half2::Zero(0, 0);
Half3 Half3::Zero(0, 0, 0);
Half4 Half4::Zero(0, 0, 0, 0);

Half2::Half2(const Vector2& v)
{
    X = ConvertFloatToHalf(v.X);
    Y = ConvertFloatToHalf(v.Y);
}

Vector2 Half2::ToVector2() const
{
    return Vector2(
        ConvertHalfToFloat(X),
        ConvertHalfToFloat(Y)
    );
}

Half3::Half3(const Vector3& v)
{
    X = ConvertFloatToHalf(v.X);
    Y = ConvertFloatToHalf(v.Y);
    Z = ConvertFloatToHalf(v.Z);
}

Vector3 Half3::ToVector3() const
{
    return Vector3(
        ConvertHalfToFloat(X),
        ConvertHalfToFloat(Y),
        ConvertHalfToFloat(Z)
    );
}

Half4::Half4(const Vector4& v)
{
    X = ConvertFloatToHalf(v.X);
    Y = ConvertFloatToHalf(v.Y);
    Z = ConvertFloatToHalf(v.Z);
    W = ConvertFloatToHalf(v.W);
}

Half4::Half4(const Color& c)
{
    X = ConvertFloatToHalf(c.R);
    Y = ConvertFloatToHalf(c.G);
    Z = ConvertFloatToHalf(c.B);
    W = ConvertFloatToHalf(c.A);
}

Half4::Half4(const Rectangle& rect)
{
    X = ConvertFloatToHalf(rect.Location.X);
    Y = ConvertFloatToHalf(rect.Location.Y);
    Z = ConvertFloatToHalf(rect.Size.X);
    W = ConvertFloatToHalf(rect.Size.Y);
}

Vector2 Half4::ToVector2() const
{
    return Vector2(
        ConvertHalfToFloat(X),
        ConvertHalfToFloat(Y)
    );
}

Vector3 Half4::ToVector3() const
{
    return Vector3(
        ConvertHalfToFloat(X),
        ConvertHalfToFloat(Y),
        ConvertHalfToFloat(Z)
    );
}

Vector4 Half4::ToVector4() const
{
    return Vector4(
        ConvertHalfToFloat(X),
        ConvertHalfToFloat(Y),
        ConvertHalfToFloat(Z),
        ConvertHalfToFloat(W)
    );
}

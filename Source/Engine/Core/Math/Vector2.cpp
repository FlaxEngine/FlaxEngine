// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"
#include "../Types/String.h"
#include "VectorInt.h"

static_assert(sizeof(Vector2) == 8, "Invalid Vector2 type size.");

const Vector2 Vector2::Zero(0);
const Vector2 Vector2::One(1);
const Vector2 Vector2::UnitX(1, 0);
const Vector2 Vector2::UnitY(0, 1);
const Vector2 Vector2::Minimum(MIN_float);
const Vector2 Vector2::Maximum(MAX_float);

Vector2::Vector2(const Int2& v)
    : X((float)v.X)
    , Y((float)v.Y)
{
}

Vector2::Vector2(const Vector3& v)
    : X(v.X)
    , Y(v.Y)
{
}

Vector2::Vector2(const Vector4& v)
    : X(v.X)
    , Y(v.Y)
{
}

Vector2::Vector2(const Color& color)
    : X(color.R)
    , Y(color.G)
{
}

String Vector2::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

Vector2 Vector2::Normalize(const Vector2& v)
{
    Vector2 result = v;
    const float length = v.Length();
    if (!Math::IsZero(length))
    {
        const float inv = 1.0f / length;
        result.X *= inv;
        result.Y *= inv;
    }
    return result;
}

Int2 Vector2::CeilToInt(const Vector2& v)
{
    return Int2(Math::CeilToInt(v.X), Math::CeilToInt(v.Y));
}

Int2 Vector2::FloorToInt(const Vector2& v)
{
    return Int2(Math::FloorToInt(v.X), Math::FloorToInt(v.Y));
}

float Vector2::TriangleArea(const Vector2& v0, const Vector2& v1, const Vector2& v2)
{
    return Math::Abs((v0.X * (v1.Y - v2.Y) + v1.X * (v2.Y - v0.Y) + v2.X * (v0.Y - v1.Y)) / 2);
}

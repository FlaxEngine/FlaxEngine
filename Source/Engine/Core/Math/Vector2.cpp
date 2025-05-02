// Copyright (c) Wojciech Figat. All rights reserved.

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"
#include "../Types/String.h"

// Float

static_assert(sizeof(Float2) == 8, "Invalid Float2 type size.");

template<>
const Float2 Float2::Zero(0.0f);
template<>
const Float2 Float2::One(1.0f);
template<>
const Float2 Float2::Half(0.5f);
template<>
const Float2 Float2::UnitX(1.0f, 0.0f);
template<>
const Float2 Float2::UnitY(0.0f, 1.0f);
template<>
const Float2 Float2::Minimum(MIN_float);
template<>
const Float2 Float2::Maximum(MAX_float);

template<>
Float2::Vector2Base(const Int3& xy)
    : X((float)xy.X)
    , Y((float)xy.Y)
{
}

template<>
Float2::Vector2Base(const Int4& xy)
    : X((float)xy.X)
    , Y((float)xy.Y)
{
}

template<>
Float2::Vector2Base(const Float3& xy)
    : X(xy.X)
    , Y(xy.Y)
{
}

template<>
Float2::Vector2Base(const Float4& xy)
    : X(xy.X)
    , Y(xy.Y)
{
}

template<>
Float2::Vector2Base(const Double3& xy)
    : X((float)xy.X)
    , Y((float)xy.Y)
{
}

template<>
Float2::Vector2Base(const Double4& xy)
    : X((float)xy.X)
    , Y((float)xy.Y)
{
}

template<>
Float2::Vector2Base(const Color& color)
    : X(color.R)
    , Y(color.G)
{
}

template<>
String Float2::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
float Float2::TriangleArea(const Float2& v0, const Float2& v1, const Float2& v2)
{
    return Math::Abs((v0.X * (v1.Y - v2.Y) + v1.X * (v2.Y - v0.Y) + v2.X * (v0.Y - v1.Y)) / 2);
}

template<>
float Float2::Angle(const Float2& from, const Float2& to)
{
    const float dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0f, 1.0f);
    if (Math::Abs(dot) > 1.0f - ZeroTolerance)
        return dot > 0.0f ? 0.0f : PI;
    return Math::Acos(dot);
}

// Double

static_assert(sizeof(Double2) == 16, "Invalid Double2 type size.");

template<>
const Double2 Double2::Zero(0.0);
template<>
const Double2 Double2::One(1.0);
template<>
const Double2 Double2::UnitX(1.0, 0.0);
template<>
const Double2 Double2::UnitY(0.0, 1.0);
template<>
const Double2 Double2::Minimum(MIN_double);
template<>
const Double2 Double2::Maximum(MAX_double);

template<>
Double2::Vector2Base(const Int3& xy)
    : X((double)xy.X)
    , Y((double)xy.Y)
{
}

template<>
Double2::Vector2Base(const Int4& xy)
    : X((double)xy.X)
    , Y((double)xy.Y)
{
}

template<>
Double2::Vector2Base(const Float3& xy)
    : X((double)xy.X)
    , Y((double)xy.Y)
{
}

template<>
Double2::Vector2Base(const Float4& xy)
    : X((int32)xy.X)
    , Y((double)xy.Y)
{
}

template<>
Double2::Vector2Base(const Double3& xy)
    : X(xy.X)
    , Y(xy.Y)
{
}

template<>
Double2::Vector2Base(const Double4& xy)
    : X(xy.X)
    , Y(xy.Y)
{
}

template<>
Double2::Vector2Base(const Color& color)
    : X((double)color.R)
    , Y((double)color.G)
{
}

template<>
String Double2::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
double Double2::TriangleArea(const Double2& v0, const Double2& v1, const Double2& v2)
{
    return Math::Abs((v0.X * (v1.Y - v2.Y) + v1.X * (v2.Y - v0.Y) + v2.X * (v0.Y - v1.Y)) / 2);
}

template<>
double Double2::Angle(const Double2& from, const Double2& to)
{
    const double dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0, 1.0);
    if (Math::Abs(dot) > 1.0 - ZeroTolerance)
        return dot > 0.0 ? 0.0 : PI;
    return Math::Acos(dot);
}

// Int

static_assert(sizeof(Int2) == 8, "Invalid Int2 type size.");

template<>
const Int2 Int2::Zero(0);
template<>
const Int2 Int2::One(1);
template<>
const Int2 Int2::UnitX(1, 0);
template<>
const Int2 Int2::UnitY(0, 1);
template<>
const Int2 Int2::Minimum(MIN_int32);
template<>
const Int2 Int2::Maximum(MAX_int32);

template<>
Int2::Vector2Base(const Int3& xy)
    : X(xy.X)
    , Y(xy.Y)
{
}

template<>
Int2::Vector2Base(const Int4& xy)
    : X(xy.X)
    , Y(xy.Y)
{
}

template<>
Int2::Vector2Base(const Float3& xy)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
{
}

template<>
Int2::Vector2Base(const Float4& xy)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
{
}

template<>
Int2::Vector2Base(const Double3& xy)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
{
}

template<>
Int2::Vector2Base(const Double4& xy)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
{
}

template<>
Int2::Vector2Base(const Color& color)
    : X((int32)color.R)
    , Y((int32)color.G)
{
}

template<>
String Int2::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
int32 Int2::TriangleArea(const Int2& v0, const Int2& v1, const Int2& v2)
{
    return Math::Abs((v0.X * (v1.Y - v2.Y) + v1.X * (v2.Y - v0.Y) + v2.X * (v0.Y - v1.Y)) / 2);
}

template<>
int32 Int2::Angle(const Int2& from, const Int2& to)
{
    return 0;
}

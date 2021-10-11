// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Double2.h"
#include "Double3.h"
#include "Double4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"
#include "Int2.h"
#include "Int3.h"
#include "Int4.h"
#include "../Types/String.h"

static_assert(sizeof(Double2) == 16, "Invalid Double2 type size.");

const Double2 Double2::Zero(0.0);
const Double2 Double2::One(1.0);
const Double2 Double2::UnitX(1.0, 0.0);
const Double2 Double2::UnitY(0.0, 1.0);
const Double2 Double2::Minimum(MIN_double);
const Double2 Double2::Maximum(MAX_double);

Double2::Double2(const Int2& xy)
    : X(static_cast<double>(xy.X))
    , Y(static_cast<double>(xy.Y))
{
}

Double2::Double2(const Int3& xyz)
    : X(static_cast<double>(xyz.X))
    , Y(static_cast<double>(xyz.Y))
{
}

Double2::Double2(const Int4& xyzw)
    : X(static_cast<double>(xyzw.X))
    , Y(static_cast<double>(xyzw.Y))
{
}

Double2::Double2(const Vector2& xy)
    : X(static_cast<double>(xy.X))
    , Y(static_cast<double>(xy.Y))
{
}

Double2::Double2(const Vector3& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
{
}

Double2::Double2(const Vector4& xyzw)
    : X(xyzw.X)
    , Y(xyzw.Y)
{
}

Double2::Double2(const Double3& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
{
}

Double2::Double2(const Double4& xyzw)
    : X(xyzw.X)
    , Y(xyzw.Y)
{
}

Double2::Double2(const Color& color)
    : X(color.R)
    , Y(color.G)
{
}

String Double2::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

Double2 Double2::Normalize(const Double2& v)
{
    Double2 result = v;
    const double length = v.Length();
    if (!Math::IsZero(length))
    {
        const double inv = 1. / length;
        result.X *= inv;
        result.Y *= inv;
    }
    return result;
}

Int2 Double2::CeilToInt(const Double2& v)
{
    return Int2((int32)Math::CeilToInt(v.X), (int32)Math::CeilToInt(v.Y));
}

Int2 Double2::FloorToInt(const Double2& v)
{
    return Int2((int32)Math::FloorToInt(v.X), (int32)Math::FloorToInt(v.Y));
}

double Double2::TriangleArea(const Double2& v0, const Double2& v1, const Double2& v2)
{
    return Math::Abs((v0.X * (v1.Y - v2.Y) + v1.X * (v2.Y - v0.Y) + v2.X * (v0.Y - v1.Y)) / 2.);
}

double Double2::Angle(const Double2& from, const Double2& to)
{
    const double dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0, 1.0);
    if (Math::Abs(dot) > (1.0 - ZeroTolerance))
        return dot > 0.0 ? 0.0 : PI;
    return Math::Acos(dot);
}

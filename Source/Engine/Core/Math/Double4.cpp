// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Double2.h"
#include "Double3.h"
#include "Double4.h"
#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Int2.h"
#include "Int3.h"
#include "Int4.h"
#include "Color.h"
#include "Matrix.h"
#include "Rectangle.h"
#include "../Types/String.h"

static_assert(sizeof(Double4) == 32, "Invalid Double4 type size.");

const Double4 Double4::Zero(0.0);
const Double4 Double4::One(1.0);
const Double4 Double4::UnitX(1, 0, 0, 0);
const Double4 Double4::UnitY(0, 1, 0, 0);
const Double4 Double4::UnitZ(0, 0, 1, 0);
const Double4 Double4::UnitW(0, 0, 0, 1);
const Double4 Double4::Minimum(MIN_double);
const Double4 Double4::Maximum(MAX_double);

Double4::Double4(double xyzw[4])
{
    Platform::MemoryCopy(Raw, xyzw, sizeof(double) * 4);
}

Double4::Double4(const Vector2& xy, double z, double w)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
    , W(w)
{
}

Double4::Double4(const Vector2& xy, const Vector2& zw)
    : X(xy.X)
    , Y(xy.Y)
    , Z(zw.X)
    , W(zw.Y)
{
}

Double4::Double4(const Vector3& xyz, double w)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
    , W(w)
{
}

Double4::Double4(const Vector4& xyzw)
    : X(xyzw.X)
    , Y(xyzw.Y)
    , Z(xyzw.Z)
    , W(xyzw.W)
{
}

Double4::Double4(const Int2& xy, double z, double w)
    : X(static_cast<double>(xy.X))
    , Y(static_cast<double>(xy.Y))
    , Z(z)
    , W(w)
{
}

Double4::Double4(const Int3& xyz, double w)
    : X(static_cast<double>(xyz.X))
    , Y(static_cast<double>(xyz.Y))
    , Z(static_cast<double>(xyz.Z))
    , W(w)
{
}

Double4::Double4(const Int4& xyzw)
    : X(static_cast<double>(xyzw.X))
    , Y(static_cast<double>(xyzw.Y))
    , Z(static_cast<double>(xyzw.X))
    , W(static_cast<double>(xyzw.Y))
{
}

Double4::Double4(const Double2& xy, double z, double w)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
    , W(w)
{
}

Double4::Double4(const Double3& xyz, double w)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
    , W(w)
{
}

Double4::Double4(const Color& color)
    : X(color.R)
    , Y(color.G)
    , Z(color.B)
    , W(color.A)
{
}

Double4::Double4(const Rectangle& rect)
    : X(rect.Location.X)
    , Y(rect.Location.Y)
    , Z(rect.Size.X)
    , W(rect.Size.Y)
{
}

String Double4::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

Double4 Double4::Floor(const Double4& v)
{
    return Double4(
        Math::Floor(v.X),
        Math::Floor(v.Y),
        Math::Floor(v.Z),
        Math::Floor(v.W)
    );
}

Double4 Double4::Frac(const Double4& v)
{
    return Double4(
        v.X - (int64)v.X,
        v.Y - (int64)v.Y,
        v.Z - (int64)v.Z,
        v.W - (int64)v.W
    );
}

Double4 Double4::Round(const Double4& v)
{
    return Double4(
        Math::Round(v.X),
        Math::Round(v.Y),
        Math::Round(v.Z),
        Math::Round(v.W)
    );
}

Double4 Double4::Ceil(const Double4& v)
{
    return Double4(
        Math::Ceil(v.X),
        Math::Ceil(v.Y),
        Math::Ceil(v.Z),
        Math::Ceil(v.W)
    );
}

Double4 Double4::Clamp(const Double4& value, const Double4& min, const Double4& max)
{
    double x = value.X;
    x = x > max.X ? max.X : x;
    x = x < min.X ? min.X : x;

    double y = value.Y;
    y = y > max.Y ? max.Y : y;
    y = y < min.Y ? min.Y : y;

    double z = value.Z;
    z = z > max.Z ? max.Z : z;
    z = z < min.Z ? min.Z : z;

    double w = value.W;
    w = w > max.W ? max.W : w;
    w = w < min.W ? min.W : w;

    return Double4(x, y, z, w);
}

void Double4::Clamp(const Double4& value, const Double4& min, const Double4& max, Double4& result)
{
    double x = value.X;
    x = x > max.X ? max.X : x;
    x = x < min.X ? min.X : x;

    double y = value.Y;
    y = y > max.Y ? max.Y : y;
    y = y < min.Y ? min.Y : y;

    double z = value.Z;
    z = z > max.Z ? max.Z : z;
    z = z < min.Z ? min.Z : z;

    double w = value.W;
    w = w > max.W ? max.W : w;
    w = w < min.W ? min.W : w;

    result = Double4(x, y, z, w);
}

Double4 Double4::Transform(const Double4& v, const Matrix& m)
{
    return Double4(
        m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0] * v.Raw[3],
        m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1] * v.Raw[3],
        m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2] * v.Raw[3],
        m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3] * v.Raw[3]
    );
}

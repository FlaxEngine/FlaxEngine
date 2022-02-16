// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Double4.h"
#include "Double3.h"
#include "Double2.h"
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

static_assert(sizeof(Vector4) == 16, "Invalid Vector4 type size.");

const Vector4 Vector4::Zero(0.0f);
const Vector4 Vector4::One(1.0f);
const Vector4 Vector4::UnitX(1, 0, 0, 0);
const Vector4 Vector4::UnitY(0, 1, 0, 0);
const Vector4 Vector4::UnitZ(0, 0, 1, 0);
const Vector4 Vector4::UnitW(0, 0, 0, 1);
const Vector4 Vector4::Minimum(MIN_float);
const Vector4 Vector4::Maximum(MAX_float);

Vector4::Vector4(float xyzw[4])
{
    Platform::MemoryCopy(Raw, xyzw, sizeof(float) * 4);
}

Vector4::Vector4(const Vector2& xy, float z, float w)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
    , W(w)
{
}

Vector4::Vector4(const Vector2& xy, const Vector2& zw)
    : X(xy.X)
    , Y(xy.Y)
    , Z(zw.X)
    , W(zw.Y)
{
}

Vector4::Vector4(const Vector3& xyz, float w)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
    , W(w)
{
}

Vector4::Vector4(const Int2& xy, float z, float w)
    : X(static_cast<float>(xy.X))
    , Y(static_cast<float>(xy.Y))
    , Z(z)
    , W(w)
{
}

Vector4::Vector4(const Int3& xyz, float w)
    : X(static_cast<float>(xyz.X))
    , Y(static_cast<float>(xyz.Y))
    , Z(static_cast<float>(xyz.Z))
    , W(w)
{
}

Vector4::Vector4(const Int4& xyzw)
    : X(static_cast<float>(xyzw.X))
    , Y(static_cast<float>(xyzw.Y))
    , Z(static_cast<float>(xyzw.Z))
    , W(static_cast<float>(xyzw.W))
{
}

Vector4::Vector4(const Double2& xy, float z, float w)
    : X(static_cast<float>(xy.X))
    , Y(static_cast<float>(xy.Y))
    , Z(z)
    , W(w)
{
}

Vector4::Vector4(const Double3& xyz, float w)
    : X(static_cast<float>(xyz.X))
    , Y(static_cast<float>(xyz.Y))
    , Z(static_cast<float>(xyz.Z))
    , W(w)
{
}

Vector4::Vector4(const Double4& xyzw)
    : X(static_cast<float>(xyzw.X))
    , Y(static_cast<float>(xyzw.Y))
    , Z(static_cast<float>(xyzw.Z))
    , W(static_cast<float>(xyzw.W))
{
}

Vector4::Vector4(const Color& color)
    : X(color.R)
    , Y(color.G)
    , Z(color.B)
    , W(color.A)
{
}

Vector4::Vector4(const Rectangle& rect)
    : X(rect.Location.X)
    , Y(rect.Location.Y)
    , Z(rect.Size.X)
    , W(rect.Size.Y)
{
}

String Vector4::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

Vector4 Vector4::Floor(const Vector4& v)
{
    return Vector4(
        Math::Floor(v.X),
        Math::Floor(v.Y),
        Math::Floor(v.Z),
        Math::Floor(v.W)
    );
}

Vector4 Vector4::Frac(const Vector4& v)
{
    return Vector4(
        v.X - (int32)v.X,
        v.Y - (int32)v.Y,
        v.Z - (int32)v.Z,
        v.W - (int32)v.W
    );
}

Vector4 Vector4::Round(const Vector4& v)
{
    return Vector4(
        Math::Round(v.X),
        Math::Round(v.Y),
        Math::Round(v.Z),
        Math::Round(v.W)
    );
}

Vector4 Vector4::Ceil(const Vector4& v)
{
    return Vector4(
        Math::Ceil(v.X),
        Math::Ceil(v.Y),
        Math::Ceil(v.Z),
        Math::Ceil(v.W)
    );
}

Vector4 Vector4::Clamp(const Vector4& value, const Vector4& min, const Vector4& max)
{
    float x = value.X;
    x = x > max.X ? max.X : x;
    x = x < min.X ? min.X : x;

    float y = value.Y;
    y = y > max.Y ? max.Y : y;
    y = y < min.Y ? min.Y : y;

    float z = value.Z;
    z = z > max.Z ? max.Z : z;
    z = z < min.Z ? min.Z : z;

    float w = value.W;
    w = w > max.W ? max.W : w;
    w = w < min.W ? min.W : w;

    return Vector4(x, y, z, w);
}

void Vector4::Clamp(const Vector4& value, const Vector4& min, const Vector4& max, Vector4& result)
{
    float x = value.X;
    x = x > max.X ? max.X : x;
    x = x < min.X ? min.X : x;

    float y = value.Y;
    y = y > max.Y ? max.Y : y;
    y = y < min.Y ? min.Y : y;

    float z = value.Z;
    z = z > max.Z ? max.Z : z;
    z = z < min.Z ? min.Z : z;

    float w = value.W;
    w = w > max.W ? max.W : w;
    w = w < min.W ? min.W : w;

    result = Vector4(x, y, z, w);
}

Vector4 Vector4::Transform(const Vector4& v, const Matrix& m)
{
    return Vector4(
        m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0] * v.Raw[3],
        m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1] * v.Raw[3],
        m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2] * v.Raw[3],
        m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3] * v.Raw[3]
    );
}

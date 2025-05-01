// Copyright (c) Wojciech Figat. All rights reserved.

#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Color.h"
#include "Matrix.h"
#include "Rectangle.h"
#include "../Types/String.h"

// Float

static_assert(sizeof(Float4) == 16, "Invalid Float4 type size.");

template<>
const Float4 Float4::Zero(0.0f);
template<>
const Float4 Float4::One(1.0f);
template<>
const Float4 Float4::Half(0.5f);
template<>
const Float4 Float4::UnitX(1.0f, 0.0f, 0.0f, 0.0f);
template<>
const Float4 Float4::UnitY(0.0f, 1.0f, 0.0f, 0.0f);
template<>
const Float4 Float4::UnitZ(0.0f, 0.0f, 1.0f, 0.0f);
template<>
const Float4 Float4::UnitW(0.0f, 0.0f, 0.0f, 1.0f);
template<>
const Float4 Float4::Minimum(MIN_float);
template<>
const Float4 Float4::Maximum(MAX_float);

template<>
Float4::Vector4Base(const Float2& xy, float z, float w)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Float4::Vector4Base(const Float2& xy, const Float2& zw)
    : X(xy.X)
    , Y(xy.Y)
    , Z(zw.X)
    , W(zw.Y)
{
}

template<>
Float4::Vector4Base(const Float3& xyz, float w)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
    , W(w)
{
}

template<>
Float4::Vector4Base(const Int2& xy, float z, float w)
    : X((float)xy.X)
    , Y((float)xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Float4::Vector4Base(const Int3& xyz, float w)
    : X((float)xyz.X)
    , Y((float)xyz.Y)
    , Z((float)xyz.Z)
    , W(w)
{
}

template<>
Float4::Vector4Base(const Double2& xy, float z, float w)
    : X((float)xy.X)
    , Y((float)xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Float4::Vector4Base(const Double2& xy, const Double2& zw)
    : X((float)xy.X)
    , Y((float)xy.Y)
    , Z((float)zw.X)
    , W((float)zw.Y)
{
}

template<>
Float4::Vector4Base(const Double3& xyz, float w)
    : X((float)xyz.X)
    , Y((float)xyz.Y)
    , Z((float)xyz.Z)
    , W(w)
{
}

template<>
Float4::Vector4Base(const Color& color)
    : X(color.R)
    , Y(color.G)
    , Z(color.B)
    , W(color.A)
{
}

template<>
Float4::Vector4Base(const Rectangle& rect)
    : X(rect.Location.X)
    , Y(rect.Location.Y)
    , Z(rect.Size.X)
    , W(rect.Size.Y)
{
}

template<>
String Float4::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
Float4 Float4::Transform(const Float4& v, const Matrix& m)
{
    return Float4(
        m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0] * v.Raw[3],
        m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1] * v.Raw[3],
        m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2] * v.Raw[3],
        m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3] * v.Raw[3]
    );
}

// Double

static_assert(sizeof(Double4) == 32, "Invalid Double4 type size.");

template<>
const Double4 Double4::Zero(0.0);
template<>
const Double4 Double4::One(1.0);
template<>
const Double4 Double4::UnitX(1.0, 0.0, 0.0, 0.0);
template<>
const Double4 Double4::UnitY(0.0, 1.0, 0.0, 0.0);
template<>
const Double4 Double4::UnitZ(0.0, 0.0, 1.0, 0.0);
template<>
const Double4 Double4::UnitW(0.0, 0.0, 0.0, 1.0);
template<>
const Double4 Double4::Minimum(MIN_double);
template<>
const Double4 Double4::Maximum(MAX_double);

template<>
Double4::Vector4Base(const Float2& xy, double z, double w)
    : X((double)xy.X)
    , Y((double)xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Double4::Vector4Base(const Float2& xy, const Float2& zw)
    : X((double)xy.X)
    , Y((double)xy.Y)
    , Z((double)zw.X)
    , W((double)zw.Y)
{
}

template<>
Double4::Vector4Base(const Float3& xyz, double w)
    : X((double)xyz.X)
    , Y((double)xyz.Y)
    , Z((double)xyz.Z)
    , W(w)
{
}

template<>
Double4::Vector4Base(const Int2& xy, double z, double w)
    : X((double)xy.X)
    , Y((double)xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Double4::Vector4Base(const Int3& xyz, double w)
    : X((double)xyz.X)
    , Y((double)xyz.Y)
    , Z((double)xyz.Z)
    , W(w)
{
}

template<>
Double4::Vector4Base(const Double2& xy, double z, double w)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Double4::Vector4Base(const Double2& xy, const Double2& zw)
    : X(xy.X)
    , Y(xy.Y)
    , Z(zw.X)
    , W(zw.Y)
{
}

template<>
Double4::Vector4Base(const Double3& xyz, double w)
    : X((double)xyz.X)
    , Y((double)xyz.Y)
    , Z((double)xyz.Z)
    , W(w)
{
}

template<>
Double4::Vector4Base(const Color& color)
    : X((double)color.R)
    , Y((double)color.G)
    , Z((double)color.B)
    , W((double)color.A)
{
}

template<>
Double4::Vector4Base(const Rectangle& rect)
    : X((double)rect.Location.X)
    , Y((double)rect.Location.Y)
    , Z((double)rect.Size.X)
    , W((double)rect.Size.Y)
{
}

template<>
String Double4::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
Double4 Double4::Transform(const Double4& v, const Matrix& m)
{
    return Double4(
        m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0] * v.Raw[3],
        m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1] * v.Raw[3],
        m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2] * v.Raw[3],
        m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3] * v.Raw[3]
    );
}

// Int

static_assert(sizeof(Int4) == 16, "Invalid Int4 type size.");

template<>
const Int4 Int4::Zero(0);
template<>
const Int4 Int4::One(1);
template<>
const Int4 Int4::UnitX(1, 0, 0, 0);
template<>
const Int4 Int4::UnitY(0, 1, 0, 0);
template<>
const Int4 Int4::UnitZ(0, 0, 1, 0);
template<>
const Int4 Int4::UnitW(0, 0, 0, 1);
template<>
const Int4 Int4::Minimum(MIN_int32);
template<>
const Int4 Int4::Maximum(MAX_int32);

template<>
Int4::Vector4Base(const Float2& xy, int32 z, int32 w)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Int4::Vector4Base(const Float2& xy, const Float2& zw)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
    , Z((int32)zw.X)
    , W((int32)zw.Y)
{
}

template<>
Int4::Vector4Base(const Float3& xyz, int32 w)
    : X((int32)xyz.X)
    , Y((int32)xyz.Y)
    , Z((int32)xyz.Z)
    , W(w)
{
}

template<>
Int4::Vector4Base(const Int2& xy, int32 z, int32 w)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Int4::Vector4Base(const Int3& xyz, int32 w)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
    , W(w)
{
}

template<>
Int4::Vector4Base(const Double2& xy, int32 z, int32 w)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
    , Z(z)
    , W(w)
{
}

template<>
Int4::Vector4Base(const Double2& xy, const Double2& zw)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
    , Z((int32)zw.X)
    , W((int32)zw.Y)
{
}

template<>
Int4::Vector4Base(const Double3& xyz, int32 w)
    : X((int32)xyz.X)
    , Y((int32)xyz.Y)
    , Z((int32)xyz.Z)
    , W(w)
{
}

template<>
Int4::Vector4Base(const Color& color)
    : X((int32)color.R)
    , Y((int32)color.G)
    , Z((int32)color.B)
    , W((int32)color.A)
{
}

template<>
Int4::Vector4Base(const Rectangle& rect)
    : X((int32)rect.Location.X)
    , Y((int32)rect.Location.Y)
    , Z((int32)rect.Size.X)
    , W((int32)rect.Size.Y)
{
}

template<>
String Int4::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
Int4 Int4::Transform(const Int4& v, const Matrix& m)
{
    return v;
}

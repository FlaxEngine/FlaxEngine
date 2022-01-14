// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Double2.h"
#include "Double3.h"
#include "Double4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "Color.h"
#include "Int2.h"
#include "Int3.h"
#include "Int4.h"
#include "../Types/String.h"

static_assert(sizeof(Double3) == 24, "Invalid Double3 type size.");

const Double3 Double3::Zero(0.0);
const Double3 Double3::One(1.0);
const Double3 Double3::Half(0.5);
const Double3 Double3::UnitX(1.0, 0.0, 0.0);
const Double3 Double3::UnitY(0.0, 1.0, 0.0);
const Double3 Double3::UnitZ(0.0, 0.0, 1.0);
const Double3 Double3::Up(0.0, 1.0, 0.0);
const Double3 Double3::Down(0.0, -1.0, 0.0);
const Double3 Double3::Left(-1.0, 0.0, 0.0);
const Double3 Double3::Right(1.0, 0.0, 0.0);
const Double3 Double3::Forward(0.0, 0.0, 1.0);
const Double3 Double3::Backward(0.0, 0.0, -1.0);
const Double3 Double3::Minimum(MIN_double);
const Double3 Double3::Maximum(MAX_double);

Double3::Double3(const Vector2& xy, double z)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
{
}

Double3::Double3(const Vector2& xy)
    : X(xy.X)
    , Y(xy.Y)
    , Z(0)
{
}

Double3::Double3(const Vector3& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
{
}

Double3::Double3(const Vector4& xyzw)
    : X(xyzw.X)
    , Y(xyzw.Y)
    , Z(xyzw.Z)
{
}

Double3::Double3(const Int2& xy, double z)
    : X(static_cast<double>(xy.X))
    , Y(static_cast<double>(xy.Y))
    , Z(z)
{
}

Double3::Double3(const Int3& xyz)
    : X(static_cast<double>(xyz.X))
    , Y(static_cast<double>(xyz.Y))
    , Z(static_cast<double>(xyz.Z))
{
}

Double3::Double3(const Int4& xyzw)
    : X(static_cast<double>(xyzw.X))
    , Y(static_cast<double>(xyzw.Y))
    , Z(static_cast<double>(xyzw.Z))
{
}

Double3::Double3(const Double2& xy)
    : X(xy.X)
    , Y(xy.Y)
    , Z(0)
{
}

Double3::Double3(const Double2& xy, double z)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
{
}

Double3::Double3(const Double4& xyzw)
    : X(xyzw.X)
    , Y(xyzw.Y)
    , Z(xyzw.Z)
{
}

Double3::Double3(const Color& color)
    : X(color.R)
    , Y(color.G)
    , Z(color.B)
{
}

String Double3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void Double3::UnwindEuler()
{
    X = Math::UnwindDegrees(X);
    Y = Math::UnwindDegrees(Y);
    Z = Math::UnwindDegrees(Z);
}

Double3 Double3::Floor(const Double3& v)
{
    return Double3(
        Math::Floor(v.X),
        Math::Floor(v.Y),
        Math::Floor(v.Z)
    );
}

Double3 Double3::Frac(const Double3& v)
{
    return Double3(
        v.X - (int64)v.X,
        v.Y - (int64)v.Y,
        v.Z - (int64)v.Z
    );
}

Double3 Double3::Clamp(const Double3& value, const Double3& min, const Double3& max)
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

    return Double3(x, y, z);
}

void Double3::Clamp(const Double3& value, const Double3& min, const Double3& max, Double3& result)
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

    result = Double3(x, y, z);
}

double Double3::Distance(const Double3& value1, const Double3& value2)
{
    const double x = value1.X - value2.X;
    const double y = value1.Y - value2.Y;
    const double z = value1.Z - value2.Z;
    return Math::Sqrt(x * x + y * y + z * z);
}

double Double3::DistanceSquared(const Double3& value1, const Double3& value2)
{
    const double x = value1.X - value2.X;
    const double y = value1.Y - value2.Y;
    const double z = value1.Z - value2.Z;
    return x * x + y * y + z * z;
}

Double3 Double3::Normalize(const Double3& input)
{
    Double3 output = input;
    const double length = input.Length();
    if (!Math::IsZero(length))
    {
        const double inv = 1.0f / length;
        output.X *= inv;
        output.Y *= inv;
        output.Z *= inv;
    }
    return output;
}

void Double3::Normalize(const Double3& input, Double3& result)
{
    result = input;
    const double length = input.Length();
    if (!Math::IsZero(length))
    {
        const double inv = 1.0f / length;
        result.X *= inv;
        result.Y *= inv;
        result.Z *= inv;
    }
}

void Double3::Hermite(const Double3& value1, const Double3& tangent1, const Double3& value2, const Double3& tangent2, double amount, Double3& result)
{
    const double squared = amount * amount;
    const double cubed = amount * squared;
    const double part1 = 2.0 * cubed - 3.0 * squared + 1.0;
    const double part2 = -2.0 * cubed + 3.0 * squared;
    const double part3 = cubed - 2.0 * squared + amount;
    const double part4 = cubed - squared;

    result.X = value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4;
    result.Y = value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4;
    result.Z = value1.Z * part1 + value2.Z * part2 + tangent1.Z * part3 + tangent2.Z * part4;
}

void Double3::Reflect(const Double3& vector, const Double3& normal, Double3& result)
{
    const double dot = vector.X * normal.X + vector.Y * normal.Y + vector.Z * normal.Z;
    result.X = vector.X - 2.0 * dot * normal.X;
    result.Y = vector.Y - 2.0 * dot * normal.Y;
    result.Z = vector.Z - 2.0 * dot * normal.Z;
}

void Double3::Transform(const Double3& vector, const Quaternion& rotation, Double3& result)
{
    const double x = rotation.X + rotation.X;
    const double y = rotation.Y + rotation.Y;
    const double z = rotation.Z + rotation.Z;
    const double wx = rotation.W * x;
    const double wy = rotation.W * y;
    const double wz = rotation.W * z;
    const double xx = rotation.X * x;
    const double xy = rotation.X * y;
    const double xz = rotation.X * z;
    const double yy = rotation.Y * y;
    const double yz = rotation.Y * z;
    const double zz = rotation.Z * z;

    result = Double3(
        vector.X * (1.0 - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
        vector.X * (xy + wz) + vector.Y * (1.0 - xx - zz) + vector.Z * (yz - wx),
        vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0 - xx - yy));
}

Double3 Double3::Transform(const Double3& vector, const Quaternion& rotation)
{
    const double x = rotation.X + rotation.X;
    const double y = rotation.Y + rotation.Y;
    const double z = rotation.Z + rotation.Z;
    const double wx = rotation.W * x;
    const double wy = rotation.W * y;
    const double wz = rotation.W * z;
    const double xx = rotation.X * x;
    const double xy = rotation.X * y;
    const double xz = rotation.X * z;
    const double yy = rotation.Y * y;
    const double yz = rotation.Y * z;
    const double zz = rotation.Z * z;

    return Double3(
        vector.X * (1.0 - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
        vector.X * (xy + wz) + vector.Y * (1.0 - xx - zz) + vector.Z * (yz - wx),
        vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0 - xx - yy));
}

void Double3::Transform(const Double3& vector, const Matrix& transform, Double4& result)
{
    result = Double4(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43,
        vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + transform.M44);
}

void Double3::Transform(const Double3& vector, const Matrix& transform, Double3& result)
{
    result = Double3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

void Double3::Transform(const Double3* vectors, const Matrix& transform, Double3* results, int32 vectorsCount)
{
    for (int32 i = 0; i < vectorsCount; i++)
    {
        Transform(vectors[i], transform, results[i]);
    }
}

Double3 Double3::Transform(const Double3& vector, const Matrix& transform)
{
    return Double3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

void Double3::TransformCoordinate(const Double3& coordinate, const Matrix& transform, Double3& result)
{
    Double4 vector;
    vector.X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + coordinate.Z * transform.M31 + transform.M41;
    vector.Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + coordinate.Z * transform.M32 + transform.M42;
    vector.Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + coordinate.Z * transform.M33 + transform.M43;
    vector.W = 1.0 / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + coordinate.Z * transform.M34 + transform.M44);
    result = Double3(vector.X * vector.W, vector.Y * vector.W, vector.Z * vector.W);
}

void Double3::TransformNormal(const Double3& normal, const Matrix& transform, Double3& result)
{
    result = Double3(
        normal.X * transform.M11 + normal.Y * transform.M21 + normal.Z * transform.M31,
        normal.X * transform.M12 + normal.Y * transform.M22 + normal.Z * transform.M32,
        normal.X * transform.M13 + normal.Y * transform.M23 + normal.Z * transform.M33);
}

Double3 Double3::Project(const Double3& vector, const Double3& onNormal)
{
    const double sqrMag = Dot(onNormal, onNormal);
    if (sqrMag < ZeroTolerance)
        return Zero;
    return onNormal * Dot(vector, onNormal) / sqrMag;
}

void Double3::Project(const Double3& vector, double x, double y, double width, double height, double minZ, double maxZ, const Matrix& worldViewProjection, Double3& result)
{
    Double3 v;
    TransformCoordinate(vector, worldViewProjection, v);

    result = Double3((1.0 + v.X) * 0.5 * width + x, (1.0 - v.Y) * 0.5 * height + y, v.Z * (maxZ - minZ) + minZ);
}

void Double3::Unproject(const Double3& vector, double x, double y, double width, double height, double minZ, double maxZ, const Matrix& worldViewProjection, Double3& result)
{
    Matrix matrix;
    Matrix::Invert(worldViewProjection, matrix);

    const Double3 v = Double3((vector.X - x) / width * 2.0 - 1.0, -((vector.Y - y) / height * 2.0 - 1.0), (vector.Z - minZ) / (maxZ - minZ));

    TransformCoordinate(v, matrix, result);
}

void Double3::CreateOrthonormalBasis(Double3& xAxis, Double3& yAxis, Double3& zAxis)
{
    xAxis -= (xAxis | zAxis) / (zAxis | zAxis) * zAxis;
    yAxis -= (yAxis | zAxis) / (zAxis | zAxis) * zAxis;

    if (xAxis.LengthSquared() < ZeroTolerance)
        xAxis = yAxis ^ zAxis;
    if (yAxis.LengthSquared() < ZeroTolerance)
        yAxis = xAxis ^ zAxis;

    xAxis.Normalize();
    yAxis.Normalize();
    zAxis.Normalize();
}

void Double3::FindBestAxisVectors(Double3& firstAxis, Double3& secondAxis) const
{
    const double absX = Math::Abs(X);
    const double absY = Math::Abs(Y);
    const double absZ = Math::Abs(Z);

    if (absZ > absX && absZ > absY)
        firstAxis = Double3(1, 0, 0);
    else
        firstAxis = Double3(0, 0, 1);

    firstAxis = (firstAxis - *this * (firstAxis | *this)).GetNormalized();
    secondAxis = firstAxis ^ *this;
}

double Double3::TriangleArea(const Double3& v0, const Double3& v1, const Double3& v2)
{
    return (v2 - v0 ^ v1 - v0).Length() * 0.5;
}

double Double3::Angle(const Double3& from, const Double3& to)
{
    const double dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0, 1.0);
    if (Math::Abs(dot) > (1.0 - ZeroTolerance))
        return dot > 0.0 ? 0.0 : PI;
    return Math::Acos(dot);
}

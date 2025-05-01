// Copyright (c) Wojciech Figat. All rights reserved.

#include "Vector3.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Color.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "Matrix3x3.h"
#include "Transform.h"
#include "../Types/String.h"

// Float

static_assert(sizeof(Float3) == 12, "Invalid Float3 type size.");

template<>
const Float3 Float3::Zero(0.0f);
template<>
const Float3 Float3::One(1.0f);
template<>
const Float3 Float3::Half(0.5f);
template<>
const Float3 Float3::UnitX(1.0f, 0.0f, 0.0f);
template<>
const Float3 Float3::UnitY(0.0f, 1.0f, 0.0f);
template<>
const Float3 Float3::UnitZ(0.0f, 0.0f, 1.0f);
template<>
const Float3 Float3::Up(0.0f, 1.0f, 0.0f);
template<>
const Float3 Float3::Down(0.0f, -1.0f, 0.0f);
template<>
const Float3 Float3::Left(-1.0f, 0.0f, 0.0f);
template<>
const Float3 Float3::Right(1.0f, 0.0f, 0.0f);
template<>
const Float3 Float3::Forward(0.0f, 0.0f, 1.0f);
template<>
const Float3 Float3::Backward(0.0f, 0.0f, -1.0f);
template<>
const Float3 Float3::Minimum(MIN_float);
template<>
const Float3 Float3::Maximum(MAX_float);

template<>
Float3::Vector3Base(const Float2& xy, float z)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
{
}

template<>
Float3::Vector3Base(const Double2& xy, float z)
    : X((float)xy.X)
    , Y((float)xy.Y)
    , Z(0)
{
}

template<>
Float3::Vector3Base(const Int2& xy, float z)
    : X((float)xy.X)
    , Y((float)xy.Y)
    , Z(z)
{
}

template<>
Float3::Vector3Base(const Int3& xyz)
    : X((float)xyz.X)
    , Y((float)xyz.Y)
    , Z((float)xyz.Z)
{
}

template<>
Float3::Vector3Base(const Int4& xyz)
    : X((float)xyz.X)
    , Y((float)xyz.Y)
    , Z((float)xyz.Z)
{
}

template<>
Float3::Vector3Base(const Float4& xyz)
    : X((float)xyz.X)
    , Y((float)xyz.Y)
    , Z((float)xyz.Z)
{
}

template<>
Float3::Vector3Base(const Double4& xyz)
    : X((float)xyz.X)
    , Y((float)xyz.Y)
    , Z((float)xyz.Z)
{
}

template<>
Float3::Vector3Base(const Color& color)
    : X(color.R)
    , Y(color.G)
    , Z(color.B)
{
}

template<>
String Float3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
void Float3::Hermite(const Float3& value1, const Float3& tangent1, const Float3& value2, const Float3& tangent2, float amount, Float3& result)
{
    const float squared = amount * amount;
    const float cubed = amount * squared;
    const float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
    const float part2 = -2.0f * cubed + 3.0f * squared;
    const float part3 = cubed - 2.0f * squared + amount;
    const float part4 = cubed - squared;
    result.X = value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4;
    result.Y = value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4;
    result.Z = value1.Z * part1 + value2.Z * part2 + tangent1.Z * part3 + tangent2.Z * part4;
}

template<>
void Float3::Reflect(const Float3& vector, const Float3& normal, Float3& result)
{
    const Real dot = vector.X * normal.X + vector.Y * normal.Y + vector.Z * normal.Z;
    result.X = vector.X - 2.0f * dot * normal.X;
    result.Y = vector.Y - 2.0f * dot * normal.Y;
    result.Z = vector.Z - 2.0f * dot * normal.Z;
}

template<>
void Float3::Transform(const Float3& vector, const Quaternion& rotation, Float3& result)
{
    const float x = rotation.X + rotation.X;
    const float y = rotation.Y + rotation.Y;
    const float z = rotation.Z + rotation.Z;
    const float wx = rotation.W * x;
    const float wy = rotation.W * y;
    const float wz = rotation.W * z;
    const float xx = rotation.X * x;
    const float xy = rotation.X * y;
    const float xz = rotation.X * z;
    const float yy = rotation.Y * y;
    const float yz = rotation.Y * z;
    const float zz = rotation.Z * z;
    result = Float3(
        vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
        vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
        vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy));
}

template<>
Float3 Float3::Transform(const Float3& vector, const Quaternion& rotation)
{
    const float x = rotation.X + rotation.X;
    const float y = rotation.Y + rotation.Y;
    const float z = rotation.Z + rotation.Z;
    const float wx = rotation.W * x;
    const float wy = rotation.W * y;
    const float wz = rotation.W * z;
    const float xx = rotation.X * x;
    const float xy = rotation.X * y;
    const float xz = rotation.X * z;
    const float yy = rotation.Y * y;
    const float yz = rotation.Y * z;
    const float zz = rotation.Z * z;
    return Float3(
        vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
        vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
        vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy));
}

template<>
void Float3::Transform(const Float3& vector, const Matrix& transform, Float4& result)
{
    result = Float4(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43,
        vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + transform.M44);
}

template<>
void Float3::Transform(const Float3& vector, const Matrix& transform, Float3& result)
{
    result = Float3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

template<>
void Float3::Transform(const Float3& vector, const Matrix3x3& transform, Float3& result)
{
    result = Float3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33);
}

template<>
void Float3::Transform(const Float3& vector, const ::Transform& transform, Float3& result)
{
#if USE_LARGE_WORLDS
    Vector3 tmp;
    transform.LocalToWorld(vector, tmp);
    result = tmp;
#else
    transform.LocalToWorld(vector, result);
#endif
}

template<>
Float3 Float3::Transform(const Float3& vector, const Matrix& transform)
{
    return Float3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

template<>
Float3 Float3::Transform(const Float3& vector, const ::Transform& transform)
{
    Vector3 result;
    transform.LocalToWorld(vector, result);
    return result;
}

template<>
void Float3::TransformCoordinate(const Float3& coordinate, const Matrix& transform, Float3& result)
{
    Float4 v;
    v.X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + coordinate.Z * transform.M31 + transform.M41;
    v.Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + coordinate.Z * transform.M32 + transform.M42;
    v.Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + coordinate.Z * transform.M33 + transform.M43;
    v.W = 1.0f / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + coordinate.Z * transform.M34 + transform.M44);
    result = Float3(v.X * v.W, v.Y * v.W, v.Z * v.W);
}

template<>
void Float3::TransformNormal(const Float3& normal, const Matrix& transform, Float3& result)
{
    result = Float3(
        normal.X * transform.M11 + normal.Y * transform.M21 + normal.Z * transform.M31,
        normal.X * transform.M12 + normal.Y * transform.M22 + normal.Z * transform.M32,
        normal.X * transform.M13 + normal.Y * transform.M23 + normal.Z * transform.M33);
}

template<>
Float3 Float3::Project(const Float3& vector, const Float3& onNormal)
{
    const float sqrMag = Dot(onNormal, onNormal);
    if (sqrMag < ZeroTolerance)
        return Zero;
    return onNormal * Dot(vector, onNormal) / sqrMag;
}

template<>
void Float3::Project(const Float3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Float3& result)
{
    Float3 v;
    TransformCoordinate(vector, worldViewProjection, v);
    result = Float3((1.0f + v.X) * 0.5f * width + x, (1.0f - v.Y) * 0.5f * height + y, v.Z * (maxZ - minZ) + minZ);
}

template<>
void Float3::Unproject(const Float3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Float3& result)
{
    Matrix matrix;
    Matrix::Invert(worldViewProjection, matrix);
    const Float3 v((vector.X - x) / width * 2.0f - 1.0f, -((vector.Y - y) / height * 2.0f - 1.0f), (vector.Z - minZ) / (maxZ - minZ));
    TransformCoordinate(v, matrix, result);
}

template<>
void Float3::CreateOrthonormalBasis(Float3& xAxis, Float3& yAxis, Float3& zAxis)
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

template<>
void Float3::FindBestAxisVectors(Float3& firstAxis, Float3& secondAxis) const
{
    const float absX = Math::Abs(X);
    const float absY = Math::Abs(Y);
    const float absZ = Math::Abs(Z);
    if (absZ > absX && absZ > absY)
        firstAxis = Float3(1, 0, 0);
    else
        firstAxis = Float3(0, 0, 1);
    firstAxis = (firstAxis - *this * (firstAxis | *this)).GetNormalized();
    secondAxis = firstAxis ^ *this;
}

template<>
float Float3::TriangleArea(const Float3& v0, const Float3& v1, const Float3& v2)
{
    return ((v2 - v0) ^ (v1 - v0)).Length() * 0.5f;
}

template<>
float Float3::Angle(const Float3& from, const Float3& to)
{
    const float dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0f, 1.0f);
    if (Math::Abs(dot) > 1.0f - ZeroTolerance)
        return dot > 0.0f ? 0.0f : 180.0f;
    return Math::Acos(dot) * RadiansToDegrees;
}

template<>
float Float3::SignedAngle(const Float3& from, const Float3& to, const Float3& axis)
{
    const float angle = Angle(from, to);
    const Float3 cross = Cross(from, to);
    const float sign = Math::Sign(axis.X * cross.X + axis.Y * cross.Y + axis.Z * cross.Z);
    return angle * sign;
}

template<>
Float3 Float3::SnapToGrid(const Float3& pos, const Float3& gridSize)
{
    return Float3(Math::Ceil((pos.X - (gridSize.X * 0.5f)) / gridSize.X) * gridSize.X,
                  Math::Ceil((pos.Y - (gridSize.Y * 0.5f)) / gridSize.Y) * gridSize.Y,
                  Math::Ceil((pos.Z - (gridSize.Z * 0.5f)) / gridSize.Z) * gridSize.Z);
}

template<>
Float3 Float3::SnapToGrid(const Float3& point, const Float3& gridSize, const Quaternion& gridOrientation, const Float3& gridOrigin, const Float3& offset)
{
    return (gridOrientation * (gridOrientation.Conjugated() * SnapToGrid(point - gridOrigin, gridSize) + offset)) + gridOrigin;
}

// Double

static_assert(sizeof(Double3) == 24, "Invalid Double3 type size.");

template<>
const Double3 Double3::Zero(0.0);
template<>
const Double3 Double3::One(1.0);
template<>
const Double3 Double3::Half(0.5);
template<>
const Double3 Double3::UnitX(1.0, 0.0, 0.0);
template<>
const Double3 Double3::UnitY(0.0, 1.0, 0.0);
template<>
const Double3 Double3::UnitZ(0.0, 0.0, 1.0);
template<>
const Double3 Double3::Up(0.0, 1.0, 0.0);
template<>
const Double3 Double3::Down(0.0, -1.0, 0.0);
template<>
const Double3 Double3::Left(-1.0, 0.0, 0.0);
template<>
const Double3 Double3::Right(1.0, 0.0, 0.0);
template<>
const Double3 Double3::Forward(0.0, 0.0, 1.0);
template<>
const Double3 Double3::Backward(0.0, 0.0, -1.0);
template<>
const Double3 Double3::Minimum(MIN_double);
template<>
const Double3 Double3::Maximum(MAX_double);

template<>
Double3::Vector3Base(const Float2& xy, double z)
    : X((double)xy.X)
    , Y((double)xy.Y)
    , Z(z)
{
}

template<>
Double3::Vector3Base(const Double2& xy, double z)
    : X(xy.X)
    , Y(xy.Y)
    , Z(0)
{
}

template<>
Double3::Vector3Base(const Int2& xy, double z)
    : X((double)xy.X)
    , Y((double)xy.Y)
    , Z(z)
{
}

template<>
Double3::Vector3Base(const Int3& xyz)
    : X((double)xyz.X)
    , Y((double)xyz.Y)
    , Z((double)xyz.Z)
{
}

template<>
Double3::Vector3Base(const Int4& xyz)
    : X((double)xyz.X)
    , Y((double)xyz.Y)
    , Z((double)xyz.Z)
{
}

template<>
Double3::Vector3Base(const Float4& xyz)
    : X((double)xyz.X)
    , Y((double)xyz.Y)
    , Z((double)xyz.Z)
{
}

template<>
Double3::Vector3Base(const Double4& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
{
}

template<>
Double3::Vector3Base(const Color& color)
    : X((double)color.R)
    , Y((double)color.G)
    , Z((double)color.B)
{
}

template<>
String Double3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
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

template<>
void Double3::Reflect(const Double3& vector, const Double3& normal, Double3& result)
{
    const double dot = vector.X * normal.X + vector.Y * normal.Y + vector.Z * normal.Z;
    result.X = vector.X - 2.0 * dot * normal.X;
    result.Y = vector.Y - 2.0 * dot * normal.Y;
    result.Z = vector.Z - 2.0 * dot * normal.Z;
}

template<>
void Double3::Transform(const Double3& vector, const Quaternion& rotation, Double3& result)
{
    const float x = rotation.X + rotation.X;
    const float y = rotation.Y + rotation.Y;
    const float z = rotation.Z + rotation.Z;
    const float wx = rotation.W * x;
    const float wy = rotation.W * y;
    const float wz = rotation.W * z;
    const float xx = rotation.X * x;
    const float xy = rotation.X * y;
    const float xz = rotation.X * z;
    const float yy = rotation.Y * y;
    const float yz = rotation.Y * z;
    const float zz = rotation.Z * z;
    result = Double3(
        vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
        vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
        vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy));
}

template<>
Double3 Double3::Transform(const Double3& vector, const Quaternion& rotation)
{
    const float x = rotation.X + rotation.X;
    const float y = rotation.Y + rotation.Y;
    const float z = rotation.Z + rotation.Z;
    const float wx = rotation.W * x;
    const float wy = rotation.W * y;
    const float wz = rotation.W * z;
    const float xx = rotation.X * x;
    const float xy = rotation.X * y;
    const float xz = rotation.X * z;
    const float yy = rotation.Y * y;
    const float yz = rotation.Y * z;
    const float zz = rotation.Z * z;
    return Double3(
        vector.X * double(1.0f - yy - zz) + vector.Y * double(xy - wz) + vector.Z * double(xz + wy),
        vector.X * double(xy + wz) + vector.Y * double(1.0f - xx - zz) + vector.Z * double(yz - wx),
        vector.X * double(xz - wy) + vector.Y * double(yz + wx) + vector.Z * double(1.0f - xx - yy));
}

template<>
void Double3::Transform(const Double3& vector, const Matrix& transform, Double4& result)
{
    result = Double4(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43,
        vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + transform.M44);
}

template<>
void Double3::Transform(const Double3& vector, const Matrix& transform, Double3& result)
{
    result = Double3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

template<>
void Double3::Transform(const Double3& vector, const Matrix3x3& transform, Double3& result)
{
    result = Double3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33);
}

template<>
void Double3::Transform(const Double3& vector, const ::Transform& transform, Double3& result)
{
#if USE_LARGE_WORLDS
    transform.LocalToWorld(vector, result);
#else
    Vector3 tmp;
    transform.LocalToWorld(vector, tmp);
    result = tmp;
#endif
}

template<>
Double3 Double3::Transform(const Double3& vector, const Matrix& transform)
{
    return Double3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

template<>
Double3 Double3::Transform(const Double3& vector, const ::Transform& transform)
{
    Vector3 result;
    transform.LocalToWorld(vector, result);
    return result;
}

template<>
void Double3::TransformCoordinate(const Double3& coordinate, const Matrix& transform, Double3& result)
{
    Double4 v;
    v.X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + coordinate.Z * transform.M31 + transform.M41;
    v.Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + coordinate.Z * transform.M32 + transform.M42;
    v.Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + coordinate.Z * transform.M33 + transform.M43;
    v.W = 1.0 / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + coordinate.Z * transform.M34 + transform.M44);
    result = Double3(v.X * v.W, v.Y * v.W, v.Z * v.W);
}

template<>
void Double3::TransformNormal(const Double3& normal, const Matrix& transform, Double3& result)
{
    result = Double3(
        normal.X * transform.M11 + normal.Y * transform.M21 + normal.Z * transform.M31,
        normal.X * transform.M12 + normal.Y * transform.M22 + normal.Z * transform.M32,
        normal.X * transform.M13 + normal.Y * transform.M23 + normal.Z * transform.M33);
}

template<>
Double3 Double3::Project(const Double3& vector, const Double3& onNormal)
{
    const Real sqrMag = Dot(onNormal, onNormal);
    if (sqrMag < ZeroTolerance)
        return Zero;
    return onNormal * Dot(vector, onNormal) / sqrMag;
}

template<>
void Double3::Project(const Double3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Double3& result)
{
    Double3 v;
    TransformCoordinate(vector, worldViewProjection, v);
    result = Double3((1.0f + v.X) * 0.5f * width + x, (1.0f - v.Y) * 0.5f * height + y, v.Z * (maxZ - minZ) + minZ);
}

template<>
void Double3::Unproject(const Double3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Double3& result)
{
    Matrix matrix;
    Matrix::Invert(worldViewProjection, matrix);
    const Double3 v((vector.X - x) / width * 2.0f - 1.0f, -((vector.Y - y) / height * 2.0f - 1.0f), (vector.Z - minZ) / (maxZ - minZ));
    TransformCoordinate(v, matrix, result);
}

template<>
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

template<>
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

template<>
double Double3::TriangleArea(const Double3& v0, const Double3& v1, const Double3& v2)
{
    return ((v2 - v0) ^ (v1 - v0)).Length() * 0.5;
}

template<>
double Double3::Angle(const Double3& from, const Double3& to)
{
    const double dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0, 1.0);
    if (Math::Abs(dot) > 1.0 - ZeroTolerance)
        return dot > 0.0f ? 0.0f : 180.0f;
    return Math::Acos(dot) * RadiansToDegrees;
}

template<>
double Double3::SignedAngle(const Double3& from, const Double3& to, const Double3& axis)
{
    const double angle = Angle(from, to);
    const Double3 cross = Cross(from, to);
    const double sign = Math::Sign(axis.X * cross.X + axis.Y * cross.Y + axis.Z * cross.Z);
    return angle * sign;
}

template<>
Double3 Double3::SnapToGrid(const Double3& pos, const Double3& gridSize)
{
    return Double3(Math::Ceil((pos.X - (gridSize.X * 0.5)) / gridSize.X) * gridSize.X,
                   Math::Ceil((pos.Y - (gridSize.Y * 0.5)) / gridSize.Y) * gridSize.Y,
                   Math::Ceil((pos.Z - (gridSize.Z * 0.5)) / gridSize.Z) * gridSize.Z);
}

template<>
Double3 Double3::SnapToGrid(const Double3& point, const Double3& gridSize, const Quaternion& gridOrientation, const Double3& gridOrigin, const Double3& offset)
{
    return (gridOrientation * (gridOrientation.Conjugated() * SnapToGrid(point - gridOrigin, gridSize) + offset)) + gridOrigin;
}

// Int

static_assert(sizeof(Int3) == 12, "Invalid Int3 type size.");

template<>
const Int3 Int3::Zero(0);
template<>
const Int3 Int3::One(1);
template<>
const Int3 Int3::Half(1);
template<>
const Int3 Int3::UnitX(1, 0, 0);
template<>
const Int3 Int3::UnitY(0, 1, 0);
template<>
const Int3 Int3::UnitZ(0, 0, 1);
template<>
const Int3 Int3::Up(0, 1, 0);
template<>
const Int3 Int3::Down(0, -1, 0);
template<>
const Int3 Int3::Left(-1, 0, 0);
template<>
const Int3 Int3::Right(1, 0, 0);
template<>
const Int3 Int3::Forward(0, 0, 1);
template<>
const Int3 Int3::Backward(0, 0, -1);
template<>
const Int3 Int3::Minimum(MIN_int32);
template<>
const Int3 Int3::Maximum(MAX_int32);

template<>
Int3::Vector3Base(const Float2& xy, int32 z)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
    , Z(z)
{
}

template<>
Int3::Vector3Base(const Double2& xy, int32 z)
    : X((int32)xy.X)
    , Y((int32)xy.Y)
    , Z(0)
{
}

template<>
Int3::Vector3Base(const Int2& xy, int32 z)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
{
}

template<>
Int3::Vector3Base(const Int3& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
{
}

template<>
Int3::Vector3Base(const Int4& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
{
}

template<>
Int3::Vector3Base(const Float4& xyz)
    : X((int32)xyz.X)
    , Y((int32)xyz.Y)
    , Z((int32)xyz.Z)
{
}

template<>
Int3::Vector3Base(const Double4& xyz)
    : X((int32)xyz.X)
    , Y((int32)xyz.Y)
    , Z((int32)xyz.Z)
{
}

template<>
Int3::Vector3Base(const Color& color)
    : X((int32)color.R)
    , Y((int32)color.G)
    , Z((int32)color.B)
{
}

template<>
String Int3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

template<>
void Int3::Hermite(const Int3& value1, const Int3& tangent1, const Int3& value2, const Int3& tangent2, int32 amount, Int3& result)
{
    result = value1;
}

template<>
void Int3::Reflect(const Int3& vector, const Int3& normal, Int3& result)
{
    result = vector;
}

template<>
void Int3::Transform(const Int3& vector, const Quaternion& rotation, Int3& result)
{
    result = vector;
}

template<>
Int3 Int3::Transform(const Int3& vector, const Quaternion& rotation)
{
    return vector;
}

template<>
void Int3::Transform(const Int3& vector, const Matrix& transform, Int4& result)
{
    result = Int4(vector);
}

template<>
void Int3::Transform(const Int3& vector, const Matrix& transform, Int3& result)
{
    result = vector;
}

template<>
void Int3::Transform(const Int3& vector, const Matrix3x3& transform, Int3& result)
{
    result = vector;
}

template<>
void Int3::Transform(const Int3& vector, const ::Transform& transform, Int3& result)
{
    result = vector;
}

template<>
Int3 Int3::Transform(const Int3& vector, const Matrix& transform)
{
    return vector;
}

template<>
Int3 Int3::Transform(const Int3& vector, const ::Transform& transform)
{
    return vector;
}

template<>
void Int3::TransformCoordinate(const Int3& coordinate, const Matrix& transform, Int3& result)
{
    result = coordinate;
}

template<>
void Int3::TransformNormal(const Int3& normal, const Matrix& transform, Int3& result)
{
    result = normal;
}

template<>
Int3 Int3::Project(const Int3& vector, const Int3& onNormal)
{
    return Zero;
}

template<>
void Int3::Project(const Int3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Int3& result)
{
    result = vector;
}

template<>
void Int3::Unproject(const Int3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Int3& result)
{
    result = vector;
}

template<>
void Int3::CreateOrthonormalBasis(Int3& xAxis, Int3& yAxis, Int3& zAxis)
{
}

template<>
void Int3::FindBestAxisVectors(Int3& firstAxis, Int3& secondAxis) const
{
}

template<>
int32 Int3::TriangleArea(const Int3& v0, const Int3& v1, const Int3& v2)
{
    return 0;
}

template<>
int32 Int3::Angle(const Int3& from, const Int3& to)
{
    return 0;
}

template<>
int32 Int3::SignedAngle(const Int3& from, const Int3& to, const Int3& axis)
{
    return 0;
}

template<>
Int3 Int3::SnapToGrid(const Int3& pos, const Int3& gridSize)
{
    return Int3(((pos.X - (gridSize.X / 2)) / gridSize.X) * gridSize.X,
                ((pos.Y - (gridSize.Y / 2)) / gridSize.Y) * gridSize.Y,
                ((pos.Z - (gridSize.Z / 2)) / gridSize.Z) * gridSize.Z);
}

template<>
Int3 Int3::SnapToGrid(const Int3& point, const Int3& gridSize, const Quaternion& gridOrientation, const Int3& gridOrigin, const Int3& offset)
{
    return (gridOrientation * (gridOrientation.Conjugated() * SnapToGrid(point - gridOrigin, gridSize) + offset)) + gridOrigin;
}

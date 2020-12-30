// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "VectorInt.h"
#include "../Types/String.h"

static_assert(sizeof(Vector3) == 12, "Invalid Vector3 type size.");

const Vector3 Vector3::Zero(0.0f);
const Vector3 Vector3::One(1.0f);
const Vector3 Vector3::Half(0.5f);
const Vector3 Vector3::UnitX(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::UnitY(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::UnitZ(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::Up(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::Down(0.0f, -1.0f, 0.0f);
const Vector3 Vector3::Left(-1.0f, 0.0f, 0.0f);
const Vector3 Vector3::Right(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::Forward(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::Backward(0.0f, 0.0f, -1.0f);
const Vector3 Vector3::Minimum(MIN_float);
const Vector3 Vector3::Maximum(MAX_float);

Vector3::Vector3(const Vector2& xy, float z)
    : X(xy.X)
    , Y(xy.Y)
    , Z(z)
{
}

Vector3::Vector3(const Vector2& xy)
    : X(xy.X)
    , Y(xy.Y)
    , Z(0)
{
}

Vector3::Vector3(const Int3& xyz)
    : X((float)xyz.X)
    , Y((float)xyz.Y)
    , Z((float)xyz.Z)
{
}

Vector3::Vector3(const Vector4& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
    , Z(xyz.Z)
{
}

Vector3::Vector3(const Color& color)
    : X(color.R)
    , Y(color.G)
    , Z(color.B)
{
}

String Vector3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void Vector3::UnwindEuler()
{
    X = Math::UnwindDegrees(X);
    Y = Math::UnwindDegrees(Y);
    Z = Math::UnwindDegrees(Z);
}

Vector3 Vector3::Floor(const Vector3& v)
{
    return Vector3(
        Math::Floor(v.X),
        Math::Floor(v.Y),
        Math::Floor(v.Z)
    );
}

Vector3 Vector3::Frac(const Vector3& v)
{
    return Vector3(
        v.X - (int32)v.X,
        v.Y - (int32)v.Y,
        v.Z - (int32)v.Z
    );
}

Vector3 Vector3::Clamp(const Vector3& value, const Vector3& min, const Vector3& max)
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

    return Vector3(x, y, z);
}

void Vector3::Clamp(const Vector3& value, const Vector3& min, const Vector3& max, Vector3& result)
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

    result = Vector3(x, y, z);
}

float Vector3::Distance(const Vector3& value1, const Vector3& value2)
{
    const float x = value1.X - value2.X;
    const float y = value1.Y - value2.Y;
    const float z = value1.Z - value2.Z;
    return Math::Sqrt(x * x + y * y + z * z);
}

float Vector3::DistanceSquared(const Vector3& value1, const Vector3& value2)
{
    const float x = value1.X - value2.X;
    const float y = value1.Y - value2.Y;
    const float z = value1.Z - value2.Z;
    return x * x + y * y + z * z;
}

Vector3 Vector3::Normalize(const Vector3& input)
{
    Vector3 output = input;
    const float length = input.Length();
    if (!Math::IsZero(length))
    {
        const float inv = 1.0f / length;
        output.X *= inv;
        output.Y *= inv;
        output.Z *= inv;
    }
    return output;
}

void Vector3::Normalize(const Vector3& input, Vector3& result)
{
    result = input;
    const float length = input.Length();
    if (!Math::IsZero(length))
    {
        const float inv = 1.0f / length;
        result.X *= inv;
        result.Y *= inv;
        result.Z *= inv;
    }
}

void Vector3::Hermite(const Vector3& value1, const Vector3& tangent1, const Vector3& value2, const Vector3& tangent2, float amount, Vector3& result)
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

void Vector3::Reflect(const Vector3& vector, const Vector3& normal, Vector3& result)
{
    const float dot = vector.X * normal.X + vector.Y * normal.Y + vector.Z * normal.Z;
    result.X = vector.X - 2.0f * dot * normal.X;
    result.Y = vector.Y - 2.0f * dot * normal.Y;
    result.Z = vector.Z - 2.0f * dot * normal.Z;
}

void Vector3::Transform(const Vector3& vector, const Quaternion& rotation, Vector3& result)
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

    result = Vector3(
        vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
        vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
        vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy));
}

Vector3 Vector3::Transform(const Vector3& vector, const Quaternion& rotation)
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

    return Vector3(
        vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
        vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
        vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy));
}

void Vector3::Transform(const Vector3& vector, const Matrix& transform, Vector4& result)
{
    result = Vector4(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43,
        vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + transform.M44);
}

void Vector3::Transform(const Vector3& vector, const Matrix& transform, Vector3& result)
{
    result = Vector3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

void Vector3::Transform(const Vector3* vectors, const Matrix& transform, Vector3* results, int32 vectorsCount)
{
    for (int32 i = 0; i < vectorsCount; i++)
    {
        Transform(vectors[i], transform, results[i]);
    }
}

Vector3 Vector3::Transform(const Vector3& vector, const Matrix& transform)
{
    return Vector3(
        vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
        vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
        vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
}

void Vector3::TransformCoordinate(const Vector3& coordinate, const Matrix& transform, Vector3& result)
{
    Vector4 vector;
    vector.X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + coordinate.Z * transform.M31 + transform.M41;
    vector.Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + coordinate.Z * transform.M32 + transform.M42;
    vector.Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + coordinate.Z * transform.M33 + transform.M43;
    vector.W = 1.0f / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + coordinate.Z * transform.M34 + transform.M44);
    result = Vector3(vector.X * vector.W, vector.Y * vector.W, vector.Z * vector.W);
}

void Vector3::TransformNormal(const Vector3& normal, const Matrix& transform, Vector3& result)
{
    result = Vector3(
        normal.X * transform.M11 + normal.Y * transform.M21 + normal.Z * transform.M31,
        normal.X * transform.M12 + normal.Y * transform.M22 + normal.Z * transform.M32,
        normal.X * transform.M13 + normal.Y * transform.M23 + normal.Z * transform.M33);
}

Vector3 Vector3::Project(const Vector3& vector, const Vector3& onNormal)
{
    const float sqrMag = Dot(onNormal, onNormal);
    if (sqrMag < ZeroTolerance)
        return Zero;
    return onNormal * Dot(vector, onNormal) / sqrMag;
}

void Vector3::Project(const Vector3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3& result)
{
    Vector3 v;
    TransformCoordinate(vector, worldViewProjection, v);

    result = Vector3((1.0f + v.X) * 0.5f * width + x, (1.0f - v.Y) * 0.5f * height + y, v.Z * (maxZ - minZ) + minZ);
}

void Vector3::Unproject(const Vector3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3& result)
{
    Matrix matrix;
    Matrix::Invert(worldViewProjection, matrix);

    const Vector3 v = Vector3((vector.X - x) / width * 2.0f - 1.0f, -((vector.Y - y) / height * 2.0f - 1.0f), (vector.Z - minZ) / (maxZ - minZ));

    TransformCoordinate(v, matrix, result);
}

void Vector3::CreateOrthonormalBasis(Vector3& xAxis, Vector3& yAxis, Vector3& zAxis)
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

void Vector3::FindBestAxisVectors(Vector3& firstAxis, Vector3& secondAxis) const
{
    const float absX = Math::Abs(X);
    const float absY = Math::Abs(Y);
    const float absZ = Math::Abs(Z);

    if (absZ > absX && absZ > absY)
        firstAxis = Vector3(1, 0, 0);
    else
        firstAxis = Vector3(0, 0, 1);

    firstAxis = (firstAxis - *this * (firstAxis | *this)).GetNormalized();
    secondAxis = firstAxis ^ *this;
}

float Vector3::TriangleArea(const Vector3& v0, const Vector3& v1, const Vector3& v2)
{
    return (v2 - v0 ^ v1 - v0).Length() * 0.5f;
}

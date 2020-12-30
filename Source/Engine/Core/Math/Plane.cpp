// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Plane.h"
#include "Matrix.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "../Types/String.h"

const float Plane::DistanceEpsilon = 0.0001f;
const float Plane::NormalEpsilon = 1.0f / 65535.0f;

Plane::Plane(const Vector3& point1, const Vector3& point2, const Vector3& point3)
{
    Vector3 cross;

    const Vector3 t1 = point2 - point1;
    const Vector3 t2 = point3 - point1;

    Vector3::Cross(t1, t2, cross);
    const float invPyth = cross.InvLength();

    Normal = cross * invPyth;
    D = -(Normal.X * point1.X + Normal.Y * point1.Y + Normal.Z * point1.Z);
}

String Plane::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void Plane::Constlection(Matrix& result) const
{
    const float x = Normal.X;
    const float y = Normal.Y;
    const float z = Normal.Z;
    const float x2 = -2.0f * x;
    const float y2 = -2.0f * y;
    const float z2 = -2.0f * z;

    result.M11 = x2 * x + 1.0f;
    result.M12 = y2 * x;
    result.M13 = z2 * x;
    result.M14 = 0.0f;
    result.M21 = x2 * y;
    result.M22 = y2 * y + 1.0f;
    result.M23 = z2 * y;
    result.M24 = 0.0f;
    result.M31 = x2 * z;
    result.M32 = y2 * z;
    result.M33 = z2 * z + 1.0f;
    result.M34 = 0.0f;
    result.M41 = x2 * D;
    result.M42 = y2 * D;
    result.M43 = z2 * D;
    result.M44 = 1.0f;
}

void Plane::Shadow(const Vector4& light, Matrix& result) const
{
    const float dot = Normal.X * light.X + Normal.Y * light.Y + Normal.Z * light.Z + D * light.W;
    const float x = -Normal.X;
    const float y = -Normal.Y;
    const float z = -Normal.Z;
    const float d = -D;

    result.M11 = x * light.X + dot;
    result.M21 = y * light.X;
    result.M31 = z * light.X;
    result.M41 = d * light.X;
    result.M12 = x * light.Y;
    result.M22 = y * light.Y + dot;
    result.M32 = z * light.Y;
    result.M42 = d * light.Y;
    result.M13 = x * light.Z;
    result.M23 = y * light.Z;
    result.M33 = z * light.Z + dot;
    result.M43 = d * light.Z;
    result.M14 = x * light.W;
    result.M24 = y * light.W;
    result.M34 = z * light.W;
    result.M44 = d * light.W + dot;
}

void Plane::Multiply(const Plane& value, float scale, Plane& result)
{
    result.Normal.X = value.Normal.X * scale;
    result.Normal.Y = value.Normal.Y * scale;
    result.Normal.Z = value.Normal.Z * scale;
    result.D = value.D * scale;
}

Plane Plane::Multiply(const Plane& value, float scale)
{
    return Plane(value.Normal.X * scale, value.Normal.Y * scale, value.Normal.Z * scale, value.D * scale);
}

void Plane::Dot(const Plane& left, const Vector4& right, float& result)
{
    result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
}

float Plane::Dot(const Plane& left, const Vector4& right)
{
    return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
}

void Plane::DotCoordinate(const Plane& left, const Vector3& right, float& result)
{
    result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
}

float Plane::DotCoordinate(const Plane& left, const Vector3& right)
{
    return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
}

void Plane::DotNormal(const Plane& left, const Vector3& right, float& result)
{
    result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
}

float Plane::DotNormal(const Plane& left, const Vector3& right)
{
    return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
}

void Plane::Normalize(const Plane& plane, Plane& result)
{
    const float magnitude = 1.0f / Math::Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
    result.Normal.X = plane.Normal.X * magnitude;
    result.Normal.Y = plane.Normal.Y * magnitude;
    result.Normal.Z = plane.Normal.Z * magnitude;
    result.D = plane.D * magnitude;
}

Plane Plane::Normalize(const Plane& plane)
{
    const float magnitude = 1.0f / Math::Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
    return Plane(plane.Normal.X * magnitude, plane.Normal.Y * magnitude, plane.Normal.Z * magnitude, plane.D * magnitude);
}

void Plane::Transform(const Plane& plane, const Quaternion& rotation, Plane& result)
{
    const float x2 = rotation.X + rotation.X;
    const float y2 = rotation.Y + rotation.Y;
    const float z2 = rotation.Z + rotation.Z;
    const float wx = rotation.W * x2;
    const float wy = rotation.W * y2;
    const float wz = rotation.W * z2;
    const float xx = rotation.X * x2;
    const float xy = rotation.X * y2;
    const float xz = rotation.X * z2;
    const float yy = rotation.Y * y2;
    const float yz = rotation.Y * z2;
    const float zz = rotation.Z * z2;

    const float x = plane.Normal.X;
    const float y = plane.Normal.Y;
    const float z = plane.Normal.Z;

    result.Normal.X = x * (1.0f - yy - zz) + y * (xy - wz) + z * (xz + wy);
    result.Normal.Y = x * (xy + wz) + y * (1.0f - xx - zz) + z * (yz - wx);
    result.Normal.Z = x * (xz - wy) + y * (yz + wx) + z * (1.0f - xx - yy);
    result.D = plane.D;
}

Plane Plane::Transform(const Plane& plane, const Quaternion& rotation)
{
    Plane result;
    const float x2 = rotation.X + rotation.X;
    const float y2 = rotation.Y + rotation.Y;
    const float z2 = rotation.Z + rotation.Z;
    const float wx = rotation.W * x2;
    const float wy = rotation.W * y2;
    const float wz = rotation.W * z2;
    const float xx = rotation.X * x2;
    const float xy = rotation.X * y2;
    const float xz = rotation.X * z2;
    const float yy = rotation.Y * y2;
    const float yz = rotation.Y * z2;
    const float zz = rotation.Z * z2;

    const float x = plane.Normal.X;
    const float y = plane.Normal.Y;
    const float z = plane.Normal.Z;

    result.Normal.X = x * (1.0f - yy - zz) + y * (xy - wz) + z * (xz + wy);
    result.Normal.Y = x * (xy + wz) + y * (1.0f - xx - zz) + z * (yz - wx);
    result.Normal.Z = x * (xz - wy) + y * (yz + wx) + z * (1.0f - xx - yy);
    result.D = plane.D;

    return result;
}

void Plane::Transform(Plane planes[], int32 planesCount, const Quaternion& rotation)
{
    ASSERT(planes && planesCount > 0);

    const float x2 = rotation.X + rotation.X;
    const float y2 = rotation.Y + rotation.Y;
    const float z2 = rotation.Z + rotation.Z;
    const float wx = rotation.W * x2;
    const float wy = rotation.W * y2;
    const float wz = rotation.W * z2;
    const float xx = rotation.X * x2;
    const float xy = rotation.X * y2;
    const float xz = rotation.X * z2;
    const float yy = rotation.Y * y2;
    const float yz = rotation.Y * z2;
    const float zz = rotation.Z * z2;

    for (int32 i = 0; i < planesCount; i++)
    {
        const float x = planes[i].Normal.X;
        const float y = planes[i].Normal.Y;
        const float z = planes[i].Normal.Z;

        planes[i].Normal.X = x * (1.0f - yy - zz) + y * (xy - wz) + z * (xz + wy);
        planes[i].Normal.Y = x * (xy + wz) + y * (1.0f - xx - zz) + z * (yz - wx);
        planes[i].Normal.Z = x * (xz - wy) + y * (yz + wx) + z * (1.0f - xx - yy);
    }
}

void Plane::Transform(const Plane& plane, const Matrix& transformation, Plane& result)
{
    const float x = plane.Normal.X;
    const float y = plane.Normal.Y;
    const float z = plane.Normal.Z;
    const float d = plane.D;

    Matrix inverse;
    Matrix::Invert(transformation, inverse);

    result.Normal.X = x * inverse.M11 + y * inverse.M12 + z * inverse.M13 + d * inverse.M14;
    result.Normal.Y = x * inverse.M21 + y * inverse.M22 + z * inverse.M23 + d * inverse.M24;
    result.Normal.Z = x * inverse.M31 + y * inverse.M32 + z * inverse.M33 + d * inverse.M34;
    result.D = x * inverse.M41 + y * inverse.M42 + z * inverse.M43 + d * inverse.M44;
}

Plane Plane::Transform(Plane& plane, Matrix& transformation)
{
    Plane result;
    const float x = plane.Normal.X;
    const float y = plane.Normal.Y;
    const float z = plane.Normal.Z;
    const float d = plane.D;

    transformation.Invert();
    result.Normal.X = x * transformation.M11 + y * transformation.M12 + z * transformation.M13 + d * transformation.M14;
    result.Normal.Y = x * transformation.M21 + y * transformation.M22 + z * transformation.M23 + d * transformation.M24;
    result.Normal.Z = x * transformation.M31 + y * transformation.M32 + z * transformation.M33 + d * transformation.M34;
    result.D = x * transformation.M41 + y * transformation.M42 + z * transformation.M43 + d * transformation.M44;

    return result;
}

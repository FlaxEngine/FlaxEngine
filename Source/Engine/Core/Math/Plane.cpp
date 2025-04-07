// Copyright (c) Wojciech Figat. All rights reserved.

#include "Plane.h"
#include "Matrix.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "../Types/String.h"

Plane::Plane(const Vector3& point1, const Vector3& point2, const Vector3& point3)
{
    Vector3 cross;

    const Vector3 t1 = point2 - point1;
    const Vector3 t2 = point3 - point1;

    Vector3::Cross(t1, t2, cross);
    const Real invPyth = cross.InvLength();

    Normal = cross * invPyth;
    D = -(Normal.X * point1.X + Normal.Y * point1.Y + Normal.Z * point1.Z);
}

String Plane::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void Plane::Normalize()
{
    const Real length = Normal.Length();
    if (!Math::IsZero(length))
    {
        const Real rcp = 1.0f / length;
        Normal *= rcp;
        D *= rcp;
    }
}

Vector3 Plane::Intersection(const Plane& inPlane1, const Plane& inPlane2, const Plane& inPlane3)
{
    // intersection point with 3 planes
    //  {
    //      x = -( c2*b1*d3-c2*b3*d1+b3*c1*d2+c3*b2*d1-b1*c3*d2-c1*b2*d3)/
    //           (-c2*b3*a1+c3*b2*a1-b1*c3*a2-c1*b2*a3+b3*c1*a2+c2*b1*a3), 
    //      y =  ( c3*a2*d1-c3*a1*d2-c2*a3*d1+d2*c1*a3-a2*c1*d3+c2*d3*a1)/
    //           (-c2*b3*a1+c3*b2*a1-b1*c3*a2-c1*b2*a3+b3*c1*a2+c2*b1*a3), 
    //      z = -(-a2*b1*d3+a2*b3*d1-a3*b2*d1+d3*b2*a1-d2*b3*a1+d2*b1*a3)/
    //           (-c2*b3*a1+c3*b2*a1-b1*c3*a2-c1*b2*a3+b3*c1*a2+c2*b1*a3)
    //  }

    // TODO: convet into cros products, dot products etc. ???

    const Real bc1 = inPlane1.Normal.Y * inPlane3.Normal.Z - inPlane3.Normal.Y * inPlane1.Normal.Z;
    const Real bc2 = inPlane2.Normal.Y * inPlane1.Normal.Z - inPlane1.Normal.Y * inPlane2.Normal.Z;
    const Real bc3 = inPlane3.Normal.Y * inPlane2.Normal.Z - inPlane2.Normal.Y * inPlane3.Normal.Z;

    const Real ad1 = inPlane1.Normal.X * inPlane3.D - inPlane3.Normal.X * inPlane1.D;
    const Real ad2 = inPlane2.Normal.X * inPlane1.D - inPlane1.Normal.X * inPlane2.D;
    const Real ad3 = inPlane3.Normal.X * inPlane2.D - inPlane2.Normal.X * inPlane3.D;

    const Real x = -(inPlane1.D * bc3 + inPlane2.D * bc1 + inPlane3.D * bc2);
    const Real y = -(inPlane1.Normal.Z * ad3 + inPlane2.Normal.Z * ad1 + inPlane3.Normal.Z * ad2);
    const Real z = +(inPlane1.Normal.Y * ad3 + inPlane2.Normal.Y * ad1 + inPlane3.Normal.Y * ad2);
    const Real w = -(inPlane1.Normal.X * bc3 + inPlane2.Normal.X * bc1 + inPlane3.Normal.X * bc2);

    // better to have detectable invalid values than to have reaaaaaaally big values
    if (w > -NormalEpsilon && w < NormalEpsilon)
    {
        return Vector3(NAN);
    }
    return Vector3(x / w, y / w, z / w);
}

void Plane::Multiply(const Plane& value, Real scale, Plane& result)
{
    result.Normal.X = value.Normal.X * scale;
    result.Normal.Y = value.Normal.Y * scale;
    result.Normal.Z = value.Normal.Z * scale;
    result.D = value.D * scale;
}

Plane Plane::Multiply(const Plane& value, Real scale)
{
    return Plane(value.Normal * scale, value.D * scale);
}

void Plane::Dot(const Plane& left, const Vector4& right, Real& result)
{
    result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
}

Real Plane::Dot(const Plane& left, const Vector4& right)
{
    return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
}

void Plane::DotCoordinate(const Plane& left, const Vector3& right, Real& result)
{
    result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
}

Real Plane::DotCoordinate(const Plane& left, const Vector3& right)
{
    return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
}

void Plane::DotNormal(const Plane& left, const Vector3& right, Real& result)
{
    result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
}

Real Plane::DotNormal(const Plane& left, const Vector3& right)
{
    return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
}

void Plane::Normalize(const Plane& plane, Plane& result)
{
    const Real magnitude = 1.0f / Math::Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
    result.Normal.X = plane.Normal.X * magnitude;
    result.Normal.Y = plane.Normal.Y * magnitude;
    result.Normal.Z = plane.Normal.Z * magnitude;
    result.D = plane.D * magnitude;
}

Plane Plane::Normalize(const Plane& plane)
{
    const Real magnitude = 1.0f / Math::Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
    return Plane(plane.Normal * magnitude, plane.D * magnitude);
}

void Plane::Transform(const Plane& plane, const Quaternion& rotation, Plane& result)
{
    const Real x2 = rotation.X + rotation.X;
    const Real y2 = rotation.Y + rotation.Y;
    const Real z2 = rotation.Z + rotation.Z;
    const Real wx = rotation.W * x2;
    const Real wy = rotation.W * y2;
    const Real wz = rotation.W * z2;
    const Real xx = rotation.X * x2;
    const Real xy = rotation.X * y2;
    const Real xz = rotation.X * z2;
    const Real yy = rotation.Y * y2;
    const Real yz = rotation.Y * z2;
    const Real zz = rotation.Z * z2;

    const Real x = plane.Normal.X;
    const Real y = plane.Normal.Y;
    const Real z = plane.Normal.Z;

    result.Normal.X = x * (1.0f - yy - zz) + y * (xy - wz) + z * (xz + wy);
    result.Normal.Y = x * (xy + wz) + y * (1.0f - xx - zz) + z * (yz - wx);
    result.Normal.Z = x * (xz - wy) + y * (yz + wx) + z * (1.0f - xx - yy);
    result.D = plane.D;
}

Plane Plane::Transform(const Plane& plane, const Quaternion& rotation)
{
    Plane result;
    Transform(plane, rotation, result);
    return result;
}

void Plane::Transform(const Plane& plane, const Matrix& transformation, Plane& result)
{
    const Real x = plane.Normal.X;
    const Real y = plane.Normal.Y;
    const Real z = plane.Normal.Z;
    const Real d = plane.D;

    Matrix inverse;
    Matrix::Invert(transformation, inverse);

    result.Normal.X = x * inverse.M11 + y * inverse.M12 + z * inverse.M13 + d * inverse.M14;
    result.Normal.Y = x * inverse.M21 + y * inverse.M22 + z * inverse.M23 + d * inverse.M24;
    result.Normal.Z = x * inverse.M31 + y * inverse.M32 + z * inverse.M33 + d * inverse.M34;
    result.D = x * inverse.M41 + y * inverse.M42 + z * inverse.M43 + d * inverse.M44;
}

Plane Plane::Transform(const Plane& plane, const Matrix& transformation)
{
    Plane result;
    Transform(plane, transformation, result);
    return result;
}

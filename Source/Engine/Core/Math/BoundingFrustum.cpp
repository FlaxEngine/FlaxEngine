// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "BoundingFrustum.h"
#include "BoundingBox.h"
#include "BoundingSphere.h"
#include "../Types/String.h"

String BoundingFrustum::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void BoundingFrustum::SetMatrix(const Matrix& matrix)
{
    // Set matrix
    _matrix = matrix;

    // Source:
    // http://www.chadvernon.com/blog/resources/directx9/frustum-culling/

    // Left plane
    _pLeft.Normal.X = matrix.M14 + matrix.M11;
    _pLeft.Normal.Y = matrix.M24 + matrix.M21;
    _pLeft.Normal.Z = matrix.M34 + matrix.M31;
    _pLeft.D = matrix.M44 + matrix.M41;
    _pLeft.Normalize();

    // Right plane
    _pRight.Normal.X = matrix.M14 - matrix.M11;
    _pRight.Normal.Y = matrix.M24 - matrix.M21;
    _pRight.Normal.Z = matrix.M34 - matrix.M31;
    _pRight.D = matrix.M44 - matrix.M41;
    _pRight.Normalize();

    // Top plane
    _pTop.Normal.X = matrix.M14 - matrix.M12;
    _pTop.Normal.Y = matrix.M24 - matrix.M22;
    _pTop.Normal.Z = matrix.M34 - matrix.M32;
    _pTop.D = matrix.M44 - matrix.M42;
    _pTop.Normalize();

    // Bottom plane
    _pBottom.Normal.X = matrix.M14 + matrix.M12;
    _pBottom.Normal.Y = matrix.M24 + matrix.M22;
    _pBottom.Normal.Z = matrix.M34 + matrix.M32;
    _pBottom.D = matrix.M44 + matrix.M42;
    _pBottom.Normalize();

    // Near plane
    _pNear.Normal.X = matrix.M13;
    _pNear.Normal.Y = matrix.M23;
    _pNear.Normal.Z = matrix.M33;
    _pNear.D = matrix.M43;
    _pNear.Normalize();

    // Far plane
    _pFar.Normal.X = matrix.M14 - matrix.M13;
    _pFar.Normal.Y = matrix.M24 - matrix.M23;
    _pFar.Normal.Z = matrix.M34 - matrix.M33;
    _pFar.D = matrix.M44 - matrix.M43;
    _pFar.Normalize();
}

Plane BoundingFrustum::GetPlane(int32 index) const
{
    switch (index)
    {
    case 0:
        return _pLeft;
    case 1:
        return _pRight;
    case 2:
        return _pTop;
    case 3:
        return _pBottom;
    case 4:
        return _pNear;
    case 5:
        return _pFar;
    default:
        return Plane();
    }
}

static Vector3 Get3PlanesInterPoint(const Plane& p1, const Plane& p2, const Plane& p3)
{
    const Vector3 n2Xn3 = Vector3::Cross(p2.Normal, p3.Normal);
    const Vector3 n3Xn1 = Vector3::Cross(p3.Normal, p1.Normal);
    const Vector3 n1Xn2 = Vector3::Cross(p1.Normal, p2.Normal);
    const float div1 = Vector3::Dot(p1.Normal, n2Xn3);
    const float div2 = Vector3::Dot(p2.Normal, n3Xn1);
    const float div3 = Vector3::Dot(p3.Normal, n1Xn2);
    if (Math::IsZero(div1 * div2 * div3))
        return Vector3::Zero;
    return n2Xn3 * (-p1.D / div1) - n3Xn1 * (p2.D / div2) - n1Xn2 * (p3.D / div3);
}

void BoundingFrustum::GetCorners(Vector3 corners[8]) const
{
    corners[0] = Get3PlanesInterPoint(_pNear, _pBottom, _pRight);
    corners[1] = Get3PlanesInterPoint(_pNear, _pTop, _pRight);
    corners[2] = Get3PlanesInterPoint(_pNear, _pTop, _pLeft);
    corners[3] = Get3PlanesInterPoint(_pNear, _pBottom, _pLeft);
    corners[4] = Get3PlanesInterPoint(_pFar, _pBottom, _pRight);
    corners[5] = Get3PlanesInterPoint(_pFar, _pTop, _pRight);
    corners[6] = Get3PlanesInterPoint(_pFar, _pTop, _pLeft);
    corners[7] = Get3PlanesInterPoint(_pFar, _pBottom, _pLeft);
}

void BoundingFrustum::GetBox(BoundingBox& result) const
{
    Vector3 corners[8];
    GetCorners(corners);
    BoundingBox::FromPoints(corners, 8, result);
}

void BoundingFrustum::GetSphere(BoundingSphere& result) const
{
    Vector3 corners[8];
    GetCorners(corners);
    BoundingSphere::FromPoints(corners, 8, result);
}

float BoundingFrustum::GetWidthAtDepth(float depth) const
{
    const float hAngle = PI / 2.0f - Math::Acos(Vector3::Dot(_pNear.Normal, _pLeft.Normal));
    return Math::Tan(hAngle) * depth * 2.0f;
}

float BoundingFrustum::GetHeightAtDepth(float depth) const
{
    const float vAngle = PI / 2.0f - Math::Acos(Vector3::Dot(_pNear.Normal, _pTop.Normal));
    return Math::Tan(vAngle) * depth * 2.0f;
}

ContainmentType BoundingFrustum::Contains(const Vector3& point) const
{
    PlaneIntersectionType result = PlaneIntersectionType::Front;
    PlaneIntersectionType planeResult = PlaneIntersectionType::Front;
    for (int i = 0; i < 6; i++)
    {
        switch (i)
        {
        case 0:
            planeResult = _pNear.Intersects(point);
            break;
        case 1:
            planeResult = _pFar.Intersects(point);
            break;
        case 2:
            planeResult = _pLeft.Intersects(point);
            break;
        case 3:
            planeResult = _pRight.Intersects(point);
            break;
        case 4:
            planeResult = _pTop.Intersects(point);
            break;
        case 5:
            planeResult = _pBottom.Intersects(point);
            break;
        }
        switch (planeResult)
        {
        case PlaneIntersectionType::Back:
            return ContainmentType::Disjoint;
        case PlaneIntersectionType::Intersecting:
            result = PlaneIntersectionType::Intersecting;
            break;
        }
    }
    switch (result)
    {
    case PlaneIntersectionType::Intersecting:
        return ContainmentType::Intersects;
    default:
        return ContainmentType::Contains;
    }
}

ContainmentType BoundingFrustum::Contains(const BoundingSphere& sphere) const
{
    auto result = PlaneIntersectionType::Front;
    auto planeResult = PlaneIntersectionType::Front;
    for (int i = 0; i < 6; i++)
    {
        switch (i)
        {
        case 0:
            planeResult = _pNear.Intersects(sphere);
            break;
        case 1:
            planeResult = _pFar.Intersects(sphere);
            break;
        case 2:
            planeResult = _pLeft.Intersects(sphere);
            break;
        case 3:
            planeResult = _pRight.Intersects(sphere);
            break;
        case 4:
            planeResult = _pTop.Intersects(sphere);
            break;
        case 5:
            planeResult = _pBottom.Intersects(sphere);
            break;
        }
        switch (planeResult)
        {
        case PlaneIntersectionType::Back:
            return ContainmentType::Disjoint;
        case PlaneIntersectionType::Intersecting:
            result = PlaneIntersectionType::Intersecting;
            break;
        }
    }
    switch (result)
    {
    case PlaneIntersectionType::Intersecting:
        return ContainmentType::Intersects;
    default:
        return ContainmentType::Contains;
    }
}

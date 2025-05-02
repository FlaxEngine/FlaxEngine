// Copyright (c) Wojciech Figat. All rights reserved.

#include "BoundingSphere.h"
#include "BoundingBox.h"
#include "Matrix.h"
#include "Ray.h"
#include "../Types/String.h"

const BoundingSphere BoundingSphere::Empty(Vector3(0, 0, 0), 0);

String BoundingSphere::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

bool BoundingSphere::Intersects(const Ray& ray) const
{
    Real distance;
    return CollisionsHelper::RayIntersectsSphere(ray, *this, distance);
}

bool BoundingSphere::Intersects(const Ray& ray, Real& distance) const
{
    return CollisionsHelper::RayIntersectsSphere(ray, *this, distance);
}

bool BoundingSphere::Intersects(const Ray& ray, Real& distance, Vector3& normal) const
{
    return CollisionsHelper::RayIntersectsSphere(ray, *this, distance, normal);
}

bool BoundingSphere::Intersects(const Ray& ray, Vector3& point) const
{
    return CollisionsHelper::RayIntersectsSphere(ray, *this, point);
}

PlaneIntersectionType BoundingSphere::Intersects(const Plane& plane) const
{
    return CollisionsHelper::PlaneIntersectsSphere(plane, *this);
}

bool BoundingSphere::Intersects(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3) const
{
    return CollisionsHelper::SphereIntersectsTriangle(*this, vertex1, vertex2, vertex3);
}

bool BoundingSphere::Intersects(const BoundingBox& box) const
{
    return CollisionsHelper::BoxIntersectsSphere(box, *this);
}

bool BoundingSphere::Intersects(const BoundingSphere& sphere) const
{
    const Real radiisum = Radius + sphere.Radius;
    const Real x = Center.X - sphere.Center.X;
    const Real y = Center.Y - sphere.Center.Y;
    const Real z = Center.Z - sphere.Center.Z;
    return x * x + y * y + z * z <= radiisum * radiisum;
}

ContainmentType BoundingSphere::Contains(const Vector3& point) const
{
    return CollisionsHelper::SphereContainsPoint(*this, point);
}

ContainmentType BoundingSphere::Contains(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3) const
{
    return CollisionsHelper::SphereContainsTriangle(*this, vertex1, vertex2, vertex3);
}

ContainmentType BoundingSphere::Contains(const BoundingBox& box) const
{
    return CollisionsHelper::SphereContainsBox(*this, box);
}

ContainmentType BoundingSphere::Contains(const BoundingSphere& sphere) const
{
    return CollisionsHelper::SphereContainsSphere(*this, sphere);
}

BoundingBox BoundingSphere::GetBoundingBox() const
{
    BoundingBox result;
    BoundingBox::FromSphere(*this, result);
    return result;
}

void BoundingSphere::GetBoundingBox(BoundingBox& result) const
{
    BoundingBox::FromSphere(*this, result);
}

void BoundingSphere::FromPoints(const Float3* points, int32 pointsCount, BoundingSphere& result)
{
    ASSERT(points && pointsCount > 0);

    // Find the center of all points
    Float3 center = Float3::Zero;
    for (int32 i = 0; i < pointsCount; i++)
        Float3::Add(points[i], center, center);
    center /= (float)pointsCount;

    // Find the radius of the sphere
    float radius = 0.0f;
    for (int32 i = 0; i < pointsCount; i++)
    {
        const float distance = Float3::DistanceSquared(center, points[i]);
        if (distance > radius)
            radius = distance;
    }

    // Construct the sphere
    result.Center = center;
    result.Radius = Math::Sqrt(radius);
}

void BoundingSphere::FromPoints(const Double3* points, int32 pointsCount, BoundingSphere& result)
{
    ASSERT(points && pointsCount > 0);

    // Find the center of all points
    Double3 center = Double3::Zero;
    for (int32 i = 0; i < pointsCount; i++)
        Double3::Add(points[i], center, center);
    center /= (double)pointsCount;

    // Find the radius of the sphere
    double radius = 0.0;
    for (int32 i = 0; i < pointsCount; i++)
    {
        const double distance = Double3::DistanceSquared(center, points[i]);
        if (distance > radius)
            radius = distance;
    }

    // Construct the sphere
    result.Center = center;
    result.Radius = (Real)Math::Sqrt(radius);
}

void BoundingSphere::FromBox(const BoundingBox& box, BoundingSphere& result)
{
    if (box.Minimum.IsNanOrInfinity() || box.Maximum.IsNanOrInfinity())
    {
        result = Empty;
        return;
    }
    const Real x = box.Maximum.X - box.Minimum.X;
    const Real y = box.Maximum.Y - box.Minimum.Y;
    const Real z = box.Maximum.Z - box.Minimum.Z;
    result.Center.X = box.Minimum.X + x * 0.5f;
    result.Center.Y = box.Minimum.Y + y * 0.5f;
    result.Center.Z = box.Minimum.Z + z * 0.5f;
    result.Radius = Math::Sqrt(x * x + y * y + z * z) * 0.5f;
}

void BoundingSphere::Merge(const BoundingSphere& value1, const BoundingSphere& value2, BoundingSphere& result)
{
    // Pre-exit if one of the bounding sphere by assuming that a merge with an empty sphere is equivalent at taking the non-empty sphere
    if (value1 == Empty)
    {
        result = value2;
        return;
    }
    if (value2 == Empty)
    {
        result = value1;
        return;
    }

    const Vector3 difference = value2.Center - value1.Center;
    const Real length = difference.Length();
    const Real radius = value1.Radius;
    const Real radius2 = value2.Radius;

    if (radius + radius2 >= length)
    {
        if (radius - radius2 >= length)
        {
            result = value1;
            return;
        }

        if (radius2 - radius >= length)
        {
            result = value2;
            return;
        }
    }

    const Vector3 vector = difference * (1.0f / length);
    const Real min = Math::Min(-radius, length - radius2);
    const Real max = (Math::Max(radius, length + radius2) - min) * 0.5f;

    result.Center = value1.Center + vector * (max + min);
    result.Radius = max;
}

void BoundingSphere::Merge(const BoundingSphere& value1, const Vector3& value2, BoundingSphere& result)
{
    const Vector3 difference = value2 - value1.Center;
    const Real length = difference.Length();
    const Real radius = value1.Radius;
    if (radius >= length)
    {
        result = value1;
        return;
    }

    const Vector3 vector = difference * (1.0f / length);
    const Real min = Math::Min(-radius, length);
    const Real max = (Math::Max(radius, length) - min) * 0.5f;

    result.Center = value1.Center + vector * (max + min);
    result.Radius = max;
}

void BoundingSphere::Transform(const BoundingSphere& sphere, const Matrix& matrix, BoundingSphere& result)
{
    Vector3::Transform(sphere.Center, matrix, result.Center);
    result.Radius = sphere.Radius * matrix.GetScaleVector().GetAbsolute().MaxValue();
}

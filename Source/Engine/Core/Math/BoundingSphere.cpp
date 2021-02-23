// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    float distance;
    return CollisionsHelper::RayIntersectsSphere(ray, *this, distance);
}

bool BoundingSphere::Intersects(const Ray& ray, float& distance) const
{
    return CollisionsHelper::RayIntersectsSphere(ray, *this, distance);
}

bool BoundingSphere::Intersects(const Ray& ray, float& distance, Vector3& normal) const
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
    return CollisionsHelper::SphereIntersectsSphere(*this, sphere);
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

void BoundingSphere::FromPoints(const Vector3* points, int32 pointsCount, BoundingSphere& result)
{
    ASSERT(points && pointsCount > 0);

    // Find the center of all points
    Vector3 center = Vector3::Zero;
    for (int32 i = 0; i < pointsCount; i++)
    {
        Vector3::Add(points[i], center, center);
    }

    // This is the center of our sphere
    center /= static_cast<float>(pointsCount);

    // Find the radius of the sphere
    float radius = 0.0f;
    for (int32 i = 0; i < pointsCount; i++)
    {
        // We are doing a relative distance comparison to find the maximum distance from the center of our sphere
        const float distance = Vector3::DistanceSquared(center, points[i]);

        if (distance > radius)
            radius = distance;
    }

    // Find the real distance from the DistanceSquared
    radius = Math::Sqrt(radius);

    // Construct the sphere
    result.Center = center;
    result.Radius = radius;
}

void BoundingSphere::FromBox(const BoundingBox& box, BoundingSphere& result)
{
    ASSERT(!box.Minimum.IsNanOrInfinity() && !box.Maximum.IsNanOrInfinity());

    Vector3::Lerp(box.Minimum, box.Maximum, 0.5f, result.Center);

    const float x = box.Minimum.X - box.Maximum.X;
    const float y = box.Minimum.Y - box.Maximum.Y;
    const float z = box.Minimum.Z - box.Maximum.Z;

    const float distance = Math::Sqrt(x * x + y * y + z * z);
    result.Radius = distance * 0.5f;
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

    const float length = difference.Length();
    const float radius = value1.Radius;
    const float radius2 = value2.Radius;

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
    const float min = Math::Min(-radius, length - radius2);
    const float max = (Math::Max(radius, length + radius2) - min) * 0.5f;

    result.Center = value1.Center + vector * (max + min);
    result.Radius = max;
}

void BoundingSphere::Merge(const BoundingSphere& value1, const Vector3& value2, BoundingSphere& result)
{
    const Vector3 difference = value2 - value1.Center;
    const float length = difference.Length();
    const float radius = value1.Radius;

    if (radius >= length)
    {
        result = value1;
        return;
    }

    const Vector3 vector = difference * (1.0f / length);
    const float min = Math::Min(-radius, length);
    const float max = (Math::Max(radius, length) - min) * 0.5f;

    result.Center = value1.Center + vector * (max + min);
    result.Radius = max;
}

void BoundingSphere::Transform(const BoundingSphere& sphere, const Matrix& matrix, BoundingSphere& result)
{
    Vector3::Transform(sphere.Center, matrix, result.Center);
    result.Radius = sphere.Radius * matrix.GetScaleVector().GetAbsolute().MaxValue();
}

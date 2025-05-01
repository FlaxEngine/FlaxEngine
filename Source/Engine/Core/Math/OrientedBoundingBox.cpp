// Copyright (c) Wojciech Figat. All rights reserved.

#include "OrientedBoundingBox.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"
#include "Matrix.h"
#include "Ray.h"
#include "../Types/String.h"

OrientedBoundingBox::OrientedBoundingBox(const BoundingBox& bb)
{
    const Vector3 center = bb.Minimum + (bb.Maximum - bb.Minimum) * 0.5f;
    Extents = bb.Maximum - center;
    Transformation = ::Transform(center);
}

OrientedBoundingBox::OrientedBoundingBox(const Vector3& extents, const Matrix& transformation)
    : Extents(extents)
{
    transformation.Decompose(Transformation);
}

OrientedBoundingBox::OrientedBoundingBox(const Vector3& extents, const Matrix3x3& rotationScale, const Vector3& translation)
    : Extents(extents)
    , Transformation(translation, rotationScale)
{
}

OrientedBoundingBox::OrientedBoundingBox(const Vector3& minimum, const Vector3& maximum)
{
    const Vector3 center = minimum + (maximum - minimum) * 0.5f;
    Extents = maximum - center;
    Transformation = ::Transform(center);
}

OrientedBoundingBox::OrientedBoundingBox(Vector3 points[], int32 pointCount)
{
    ASSERT(points && pointCount > 0);
    Vector3 minimum = points[0];
    Vector3 maximum = points[0];
    for (int32 i = 1; i < pointCount; i++)
    {
        Vector3::Min(minimum, points[i], minimum);
        Vector3::Max(maximum, points[i], maximum);
    }
    const Vector3 center = minimum + (maximum - minimum) * 0.5f;
    Extents = maximum - center;
    Transformation = ::Transform(center);
}

String OrientedBoundingBox::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void OrientedBoundingBox::GetCorners(Float3 corners[8]) const
{
    const Float3 xv = Transformation.LocalToWorldVector(Vector3((float)Extents.X, 0, 0));
    const Float3 yv = Transformation.LocalToWorldVector(Vector3(0, (float)Extents.Y, 0));
    const Float3 zv = Transformation.LocalToWorldVector(Vector3(0, 0, (float)Extents.Z));

    const Float3 center = Transformation.Translation;

    corners[0] = center + xv + yv + zv;
    corners[1] = center + xv + yv - zv;
    corners[2] = center - xv + yv - zv;
    corners[3] = center - xv + yv + zv;
    corners[4] = center + xv - yv + zv;
    corners[5] = center + xv - yv - zv;
    corners[6] = center - xv - yv - zv;
    corners[7] = center - xv - yv + zv;
}

void OrientedBoundingBox::GetCorners(Double3 corners[8]) const
{
    const Double3 xv = Transformation.LocalToWorldVector(Vector3((float)Extents.X, 0, 0));
    const Double3 yv = Transformation.LocalToWorldVector(Vector3(0, (float)Extents.Y, 0));
    const Double3 zv = Transformation.LocalToWorldVector(Vector3(0, 0, (float)Extents.Z));

    const Double3 center = Transformation.Translation;

    corners[0] = center + xv + yv + zv;
    corners[1] = center + xv + yv - zv;
    corners[2] = center - xv + yv - zv;
    corners[3] = center - xv + yv + zv;
    corners[4] = center + xv - yv + zv;
    corners[5] = center + xv - yv - zv;
    corners[6] = center - xv - yv - zv;
    corners[7] = center - xv - yv + zv;
}

Vector3 OrientedBoundingBox::GetSize() const
{
    const Vector3 xv = Transformation.LocalToWorldVector(Vector3(Extents.X * 2, 0, 0));
    const Vector3 yv = Transformation.LocalToWorldVector(Vector3(0, Extents.Y * 2, 0));
    const Vector3 zv = Transformation.LocalToWorldVector(Vector3(0, 0, Extents.Z * 2));
    return Vector3(xv.Length(), yv.Length(), zv.Length());
}

Vector3 OrientedBoundingBox::GetSizeSquared() const
{
    const Vector3 xv = Transformation.LocalToWorldVector(Vector3(Extents.X * 2, 0, 0));
    const Vector3 yv = Transformation.LocalToWorldVector(Vector3(0, Extents.Y * 2, 0));
    const Vector3 zv = Transformation.LocalToWorldVector(Vector3(0, 0, Extents.Z * 2));
    return Vector3(xv.LengthSquared(), yv.LengthSquared(), zv.LengthSquared());
}

BoundingBox OrientedBoundingBox::GetBoundingBox() const
{
    BoundingBox result;
    Vector3 corners[8];
    GetCorners(corners);
    BoundingBox::FromPoints(corners, 8, result);
    return result;
}

void OrientedBoundingBox::GetBoundingBox(BoundingBox& result) const
{
    Vector3 corners[8];
    GetCorners(corners);

    BoundingBox::FromPoints(corners, 8, result);
}

void OrientedBoundingBox::Transform(const Matrix& matrix)
{
    ::Transform transform;
    matrix.Decompose(transform);
    Transformation = transform.LocalToWorld(Transformation);
}

void OrientedBoundingBox::Transform(const ::Transform& transform)
{
    Transformation = transform.LocalToWorld(Transformation);
}

ContainmentType OrientedBoundingBox::Contains(const Vector3& point, Real* distance) const
{
    // Transform the point into the obb coordinates
    Vector3 locPoint;
    Transformation.WorldToLocal(point, locPoint);
    locPoint.X = Math::Abs(locPoint.X);
    locPoint.Y = Math::Abs(locPoint.Y);
    locPoint.Z = Math::Abs(locPoint.Z);

    if (distance)
    {
        // Get minimum distance to edge in local space
        Vector3 tmp;
        Vector3::Subtract(Extents, locPoint, tmp);
        const Real minDstToEdgeLocal = tmp.GetAbsolute().MinValue();

        // Transform distance to world space
        Vector3 dstVec = Vector3::UnitX * minDstToEdgeLocal;
        Transformation.LocalToWorldVector(dstVec, dstVec);
        *distance = dstVec.Length();
    }

    // Simple axes-aligned BB check
    if (locPoint.X < Extents.X && locPoint.Y < Extents.Y && locPoint.Z < Extents.Z)
        return ContainmentType::Contains;
    if (Vector3::NearEqual(locPoint, Extents))
        return ContainmentType::Intersects;
    return ContainmentType::Disjoint;
}

ContainmentType OrientedBoundingBox::Contains(const BoundingSphere& sphere, bool ignoreScale) const
{
    // Transform sphere center into the obb coordinates
    Vector3 locCenter;
    Transformation.WorldToLocal(sphere.Center, locCenter);

    Real locRadius;
    if (ignoreScale)
    {
        locRadius = sphere.Radius;
    }
    else
    {
        // Transform sphere radius into the obb coordinates
        Vector3 vRadius = Vector3::UnitX * sphere.Radius;
        Transformation.LocalToWorldVector(vRadius, vRadius);
        locRadius = vRadius.Length();
    }

    // Perform regular BoundingBox to BoundingSphere containment check
    const Vector3 minusExtents = -Extents;
    Vector3 vector;
    Vector3::Clamp(locCenter, minusExtents, Extents, vector);
    const Real distance = Vector3::DistanceSquared(locCenter, vector);

    if (distance > locRadius * locRadius)
        return ContainmentType::Disjoint;
    if (minusExtents.X + locRadius <= locCenter.X && locCenter.X <= Extents.X - locRadius && (Extents.X - minusExtents.X > locRadius && minusExtents.Y + locRadius <= locCenter.Y) && (locCenter.Y <= Extents.Y - locRadius && Extents.Y - minusExtents.Y > locRadius && (minusExtents.Z + locRadius <= locCenter.Z && locCenter.Z <= Extents.Z - locRadius && Extents.Z - minusExtents.Z > locRadius)))
        return ContainmentType::Contains;
    return ContainmentType::Intersects;
}

bool OrientedBoundingBox::Intersects(const Ray& ray, Vector3& point) const
{
    // Put ray in box space
    Ray bRay;
    Transformation.WorldToLocalVector(ray.Direction, bRay.Direction);
    Transformation.WorldToLocal(ray.Position, bRay.Position);

    // Perform a regular ray to BoundingBox check
    const BoundingBox bb(-Extents, Extents);
    const bool intersects = CollisionsHelper::RayIntersectsBox(bRay, bb, point);

    // Put the result intersection back to world
    if (intersects)
        Transformation.LocalToWorld(point, point);

    return intersects;
}

bool OrientedBoundingBox::Intersects(const Ray& ray, Real& distance) const
{
    Vector3 point;
    const bool result = Intersects(ray, point);
    distance = Vector3::Distance(ray.Position, point);
    return result;
}

bool OrientedBoundingBox::Intersects(const Ray& ray, Real& distance, Vector3& normal) const
{
    // Put ray in box space
    Ray bRay;
    Transformation.WorldToLocalVector(ray.Direction, bRay.Direction);
    Transformation.WorldToLocal(ray.Position, bRay.Position);

    // Perform a regular ray to BoundingBox check
    const BoundingBox bb(-Extents, Extents);
    if (CollisionsHelper::RayIntersectsBox(bRay, bb, distance, normal))
    {
        // Put the result intersection back to world
        Transformation.LocalToWorldVector(normal, normal);
        normal.Normalize();
        return true;
    }

    return false;
}

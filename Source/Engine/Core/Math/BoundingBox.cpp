// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "BoundingBox.h"
#include "BoundingSphere.h"
#include "Matrix.h"
#include "../Types/String.h"

const BoundingBox BoundingBox::Empty(Vector3(MAX_float), Vector3(MIN_float));
const BoundingBox BoundingBox::Zero(Vector3(0.0f));

String BoundingBox::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void BoundingBox::FromPoints(const Vector3* points, int32 pointsCount, BoundingBox& result)
{
    ASSERT(points && pointsCount > 0);
    Vector3 min = points[0];
    Vector3 max = points[0];
    for (int32 i = 1; i < pointsCount; i++)
    {
        Vector3::Min(min, points[i], min);
        Vector3::Max(max, points[i], max);
    }
    result = BoundingBox(min, max);
}

void BoundingBox::FromSphere(const BoundingSphere& sphere, BoundingBox& result)
{
    result = BoundingBox(
        Vector3(sphere.Center.X - sphere.Radius, sphere.Center.Y - sphere.Radius, sphere.Center.Z - sphere.Radius),
        Vector3(sphere.Center.X + sphere.Radius, sphere.Center.Y + sphere.Radius, sphere.Center.Z + sphere.Radius)
    );
}

void BoundingBox::Transform(const BoundingBox& box, const Matrix& matrix, BoundingBox& result)
{
    // Reference: http://dev.theomader.com/transform-bounding-boxes/

    const auto right = matrix.GetRight();
    const auto xa = right * box.Minimum.X;
    const auto xb = right * box.Maximum.X;

    const auto up = matrix.GetUp();
    const auto ya = up * box.Minimum.Y;
    const auto yb = up * box.Maximum.Y;

    const auto backward = matrix.GetBackward();
    const auto za = backward * box.Minimum.Z;
    const auto zb = backward * box.Maximum.Z;

    const auto translation = matrix.GetTranslation();
    result = BoundingBox(
        Vector3::Min(xa, xb) + Vector3::Min(ya, yb) + Vector3::Min(za, zb) + translation,
        Vector3::Max(xa, xb) + Vector3::Max(ya, yb) + Vector3::Max(za, zb) + translation);

    /*
    // Get box corners
    Vector3 corners[8];
    box.GetCorners(corners);

    // Transform them
    for (int32 i = 0; i < 8; i++)
    {
        corners[i] = Vector3::Transform(corners[i], matrix);
    }

    // Construct box from the points
    result = FromPoints(corners, 8);
    */
}

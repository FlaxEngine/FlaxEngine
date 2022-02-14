// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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

void BoundingBox::GetCorners(Vector3 corners[8]) const
{
    corners[0] = Vector3(Minimum.X, Maximum.Y, Maximum.Z);
    corners[1] = Vector3(Maximum.X, Maximum.Y, Maximum.Z);
    corners[2] = Vector3(Maximum.X, Minimum.Y, Maximum.Z);
    corners[3] = Vector3(Minimum.X, Minimum.Y, Maximum.Z);
    corners[4] = Vector3(Minimum.X, Maximum.Y, Minimum.Z);
    corners[5] = Vector3(Maximum.X, Maximum.Y, Minimum.Z);
    corners[6] = Vector3(Maximum.X, Minimum.Y, Minimum.Z);
    corners[7] = Vector3(Minimum.X, Minimum.Y, Minimum.Z);
}

BoundingBox BoundingBox::MakeOffsetted(const Vector3& offset) const
{
    BoundingBox result;
    result.Minimum = Minimum + offset;
    result.Maximum = Maximum + offset;
    return result;
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

BoundingBox BoundingBox::FromPoints(const Vector3* points, int32 pointsCount)
{
    BoundingBox result;
    FromPoints(points, pointsCount, result);
    return result;
}

void BoundingBox::FromSphere(const BoundingSphere& sphere, BoundingBox& result)
{
    result = BoundingBox(
        Vector3(sphere.Center.X - sphere.Radius, sphere.Center.Y - sphere.Radius, sphere.Center.Z - sphere.Radius),
        Vector3(sphere.Center.X + sphere.Radius, sphere.Center.Y + sphere.Radius, sphere.Center.Z + sphere.Radius)
    );
}

BoundingBox BoundingBox::FromSphere(const BoundingSphere& sphere)
{
    BoundingBox result;
    FromSphere(sphere, result);
    return result;
}

BoundingBox BoundingBox::Transform(const BoundingBox& box, const Matrix& matrix)
{
    BoundingBox result;
    Transform(box, matrix, result);
    return result;
}

BoundingBox BoundingBox::MakeOffsetted(const BoundingBox& box, const Vector3& offset)
{
    BoundingBox result;
    result.Minimum = box.Minimum + offset;
    result.Maximum = box.Maximum + offset;
    return result;
}

BoundingBox BoundingBox::MakeScaled(const BoundingBox& box, float scale)
{
    Vector3 size;
    Vector3::Subtract(box.Maximum, box.Minimum, size);
    Vector3 sizeHalf = size * 0.5f;
    const Vector3 center = box.Minimum + sizeHalf;
    sizeHalf = sizeHalf * scale;
    return BoundingBox(center - sizeHalf, center + sizeHalf);
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

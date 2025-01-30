// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "BoundingBox.h"
#include "BoundingSphere.h"
#include "Matrix.h"
#include "Transform.h"
#include "../Types/String.h"

const BoundingBox BoundingBox::Empty(Vector3(MAX_float), Vector3(MIN_float));
const BoundingBox BoundingBox::Zero(Vector3(0.0f));

String BoundingBox::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void BoundingBox::GetCorners(Float3 corners[8]) const
{
    corners[0] = Float3((float)Minimum.X, (float)Maximum.Y, (float)Maximum.Z);
    corners[1] = Float3((float)Maximum.X, (float)Maximum.Y, (float)Maximum.Z);
    corners[2] = Float3((float)Maximum.X, (float)Minimum.Y, (float)Maximum.Z);
    corners[3] = Float3((float)Minimum.X, (float)Minimum.Y, (float)Maximum.Z);
    corners[4] = Float3((float)Minimum.X, (float)Maximum.Y, (float)Minimum.Z);
    corners[5] = Float3((float)Maximum.X, (float)Maximum.Y, (float)Minimum.Z);
    corners[6] = Float3((float)Maximum.X, (float)Minimum.Y, (float)Minimum.Z);
    corners[7] = Float3((float)Minimum.X, (float)Minimum.Y, (float)Minimum.Z);
}

void BoundingBox::GetCorners(Double3 corners[8]) const
{
    corners[0] = Double3(Minimum.X, Maximum.Y, Maximum.Z);
    corners[1] = Double3(Maximum.X, Maximum.Y, Maximum.Z);
    corners[2] = Double3(Maximum.X, Minimum.Y, Maximum.Z);
    corners[3] = Double3(Minimum.X, Minimum.Y, Maximum.Z);
    corners[4] = Double3(Minimum.X, Maximum.Y, Minimum.Z);
    corners[5] = Double3(Maximum.X, Maximum.Y, Minimum.Z);
    corners[6] = Double3(Maximum.X, Minimum.Y, Minimum.Z);
    corners[7] = Double3(Minimum.X, Minimum.Y, Minimum.Z);
}

BoundingBox BoundingBox::MakeOffsetted(const Vector3& offset) const
{
    BoundingBox result;
    result.Minimum = Minimum + offset;
    result.Maximum = Maximum + offset;
    return result;
}

void BoundingBox::FromPoints(const Float3* points, int32 pointsCount, BoundingBox& result)
{
    ASSERT(points && pointsCount > 0);
    Float3 min = points[0];
    Float3 max = points[0];
    for (int32 i = 1; i < pointsCount; i++)
    {
        Float3::Min(min, points[i], min);
        Float3::Max(max, points[i], max);
    }
    result = BoundingBox(min, max);
}

void BoundingBox::FromPoints(const Double3* points, int32 pointsCount, BoundingBox& result)
{
    ASSERT(points && pointsCount > 0);
    Double3 min = points[0];
    Double3 max = points[0];
    for (int32 i = 1; i < pointsCount; i++)
    {
        Double3::Min(min, points[i], min);
        Double3::Max(max, points[i], max);
    }
    result = BoundingBox((Vector3)min, (Vector3)max);
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

BoundingBox BoundingBox::MakeScaled(const BoundingBox& box, Real scale)
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

    const auto forward = matrix.GetForward();
    const auto za = forward * box.Minimum.Z;
    const auto zb = forward * box.Maximum.Z;

    const auto translation = matrix.GetTranslation();
    const auto min = Vector3::Min(xa, xb) + Vector3::Min(ya, yb) + Vector3::Min(za, zb) + translation;
    const auto max = Vector3::Max(xa, xb) + Vector3::Max(ya, yb) + Vector3::Max(za, zb) + translation;
    result = BoundingBox(min, max);
}

void BoundingBox::Transform(const BoundingBox& box, const ::Transform& transform, BoundingBox& result)
{
    // Reference: http://dev.theomader.com/transform-bounding-boxes/

    const auto right = Float3::Transform(Float3::Right, transform.Orientation);
    const auto xa = right * box.Minimum.X;
    const auto xb = right * box.Maximum.X;

    const auto up = Float3::Transform(Float3::Up, transform.Orientation);
    const auto ya = up * box.Minimum.Y;
    const auto yb = up * box.Maximum.Y;

    const auto forward = Float3::Transform(Float3::Forward, transform.Orientation);
    const auto za = forward * box.Minimum.Z;
    const auto zb = forward * box.Maximum.Z;

    const auto min = Vector3::Min(xa, xb) + Vector3::Min(ya, yb) + Vector3::Min(za, zb) + transform.Translation;
    const auto max = Vector3::Max(xa, xb) + Vector3::Max(ya, yb) + Vector3::Max(za, zb) + transform.Translation;
    result = BoundingBox(min, max);
}

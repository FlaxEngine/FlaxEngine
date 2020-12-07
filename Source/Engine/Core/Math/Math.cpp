// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Math.h"
#include "Vector3.h"

Vector3 Math::RotateAboutAxis(const Vector3& normalizedRotationAxis, float angle, const Vector3& positionOnAxis, const Vector3& position)
{
    // Project position onto the rotation axis and find the closest point on the axis to Position
    const Vector3 closestPointOnAxis = positionOnAxis + normalizedRotationAxis * Vector3::Dot(normalizedRotationAxis, position - positionOnAxis);

    // Construct orthogonal axes in the plane of the rotation
    const Vector3 axisU = position - closestPointOnAxis;
    const Vector3 axisV = Vector3::Cross(normalizedRotationAxis, axisU);
    float cosAngle, sinAngle;
    Math::SinCos(angle, sinAngle, cosAngle);

    // Rotate using the orthogonal axes
    const Vector3 rotation = axisU * cosAngle + axisV * sinAngle;

    // Reconstruct the rotated world space position
    const Vector3 rotatedPosition = closestPointOnAxis + rotation;

    // Convert from position to a position offset
    return rotatedPosition - position;
}

Vector3 Math::ExtractLargestComponent(const Vector3& v)
{
    const Vector3 a = v.GetAbsolute();

    if (a.X > a.Y)
    {
        if (a.X > a.Z)
        {
            return Vector3(v.X > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
    }
    else
    {
        if (a.Y > a.Z)
        {
            return Vector3(0.0f, v.Y > 0.0f ? 1.0f : -1.0f, 0.0f);
        }
    }

    return Vector3(0.0f, 0.0f, v.Z > 0 ? 1.0f : -1.0f);
}

// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Math.h"
#include "Vector3.h"

void Math::SinCos(float angle, float& sine, float& cosine)
{
    sine = Math::Sin(angle);
    cosine = Math::Cos(angle);
}

uint32 Math::FloorLog2(uint32 value)
{
    // References:
    // http://codinggorilla.domemtech.com/?p=81
    // http://en.wikipedia.org/wiki/Binary_logarithm

    uint32 pos = 0;
    if (value >= 1 << 16)
    {
        value >>= 16;
        pos += 16;
    }
    if (value >= 1 << 8)
    {
        value >>= 8;
        pos += 8;
    }
    if (value >= 1 << 4)
    {
        value >>= 4;
        pos += 4;
    }
    if (value >= 1 << 2)
    {
        value >>= 2;
        pos += 2;
    }
    if (value >= 1 << 1)
    {
        pos += 1;
    }
    return value == 0 ? 0 : pos;
}

Vector3 Math::RotateAboutAxis(const Vector3& normalizedRotationAxis, float angle, const Vector3& positionOnAxis, const Vector3& position)
{
    const Vector3 closestPointOnAxis = positionOnAxis + normalizedRotationAxis * Vector3::Dot(normalizedRotationAxis, position - positionOnAxis);
    const Vector3 axisU = position - closestPointOnAxis;
    const Vector3 axisV = Vector3::Cross(normalizedRotationAxis, axisU);
    float cosAngle, sinAngle;
    Math::SinCos(angle, sinAngle, cosAngle);
    const Vector3 rotation = axisU * cosAngle + axisV * sinAngle;
    const Vector3 rotatedPosition = closestPointOnAxis + rotation;
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

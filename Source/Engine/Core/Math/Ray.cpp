// Copyright (c) Wojciech Figat. All rights reserved.

#include "Ray.h"
#include "Matrix.h"
#include "Viewport.h"
#include "../Types/String.h"

Ray Ray::Identity(Vector3(0, 0, 0), Vector3(0, 0, 1.0f));

String Ray::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

Vector3 Ray::GetPoint(Real distance) const
{
    return Position + Direction * distance;
}

Ray Ray::GetPickRay(float x, float y, const Viewport& viewport, const Matrix& vp)
{
    Vector3 nearPoint(x, y, 0.0f);
    Vector3 farPoint(x, y, 1.0f);

    nearPoint = Vector3::Unproject(nearPoint, viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth, vp);
    farPoint = Vector3::Unproject(farPoint, viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth, vp);

    Vector3 direction = farPoint - nearPoint;
    direction.Normalize();

    return Ray(nearPoint, direction);
}

// Copyright (c) Wojciech Figat. All rights reserved.

#include "Viewport.h"
#include "Rectangle.h"
#include "Vector3.h"
#include "Matrix.h"
#include "../Types/String.h"

Viewport::Viewport(const Rectangle& bounds)
    : X(bounds.Location.X)
    , Y(bounds.Location.Y)
    , Width(bounds.Size.X)
    , Height(bounds.Size.Y)
    , MinDepth(0.0f)
    , MaxDepth(1.0f)
{
}

String Viewport::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

Rectangle Viewport::GetBounds() const
{
    return Rectangle(X, Y, Width, Height);
}

void Viewport::SetBounds(const Rectangle& bounds)
{
    X = bounds.Location.X;
    Y = bounds.Location.Y;
    Width = bounds.Size.X;
    Height = bounds.Size.Y;
}

void Viewport::Project(const Vector3& source, const Matrix& vp, Vector3& result) const
{
    Vector3::Transform(source, vp, result);
    const Real a = source.X * vp.M14 + source.Y * vp.M24 + source.Z * vp.M34 + vp.M44;

    if (!Math::IsOne(a))
    {
        result /= a;
    }

    result.X = (result.X + 1.0f) * 0.5f * Width + X;
    result.Y = (-result.Y + 1.0f) * 0.5f * Height + Y;
    result.Z = result.Z * (MaxDepth - MinDepth) + MinDepth;
}

void Viewport::Unproject(const Vector3& source, const Matrix& ivp, Vector3& result) const
{
    result.X = (source.X - X) / Width * 2.0f - 1.0f;
    result.Y = -((source.Y - Y) / Height * 2.0f - 1.0f);
    result.Z = (source.Z - MinDepth) / (MaxDepth - MinDepth);

    const Real a = result.X * ivp.M14 + result.Y * ivp.M24 + result.Z * ivp.M34 + ivp.M44;
    Vector3::Transform(result, ivp, result);

    if (!Math::IsOne(a))
    {
        result /= a;
    }
}

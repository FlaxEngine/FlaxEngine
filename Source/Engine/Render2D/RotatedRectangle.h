// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Rectangle.h"

/// <summary>
/// Represents a rectangle that has been transformed by an arbitrary render transform.
/// </summary>
struct RotatedRectangle
{
public:

    /// <summary>
    /// The transformed top left corner.
    /// </summary>
    Vector2 TopLeft;

    /// <summary>
    /// The transformed X extent (right-left).
    /// </summary>
    Vector2 ExtentX;

    /// <summary>
    /// The transformed Y extent (bottom-top).
    /// </summary>
    Vector2 ExtentY;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="RotatedRectangle"/> struct.
    /// </summary>
    RotatedRectangle()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="RotatedRectangle"/> struct.
    /// </summary>
    /// <param name="rect">The aligned rectangle.</param>
    explicit RotatedRectangle(const Rectangle& rect)
        : TopLeft(rect.GetUpperLeft())
        , ExtentX(rect.GetWidth(), 0.0f)
        , ExtentY(0.0f, rect.GetHeight())
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="RotatedRectangle"/> struct.
    /// </summary>
    /// <param name="topLeft">The top left corner.</param>
    /// <param name="extentX">The extent on X axis.</param>
    /// <param name="extentY">The extent on Y axis.</param>
    RotatedRectangle(const Vector2& topLeft, const Vector2& extentX, const Vector2& extentY)
        : TopLeft(topLeft)
        , ExtentX(extentX)
        , ExtentY(extentY)
    {
    }

public:

    /// <summary>
    /// Convert rotated rectangle to the axis-aligned rectangle that builds rotated rectangle bounding box.
    /// </summary>
    /// <returns>The bounds rectangle.</returns>
    Rectangle ToBoundingRect() const
    {
        Vector2 points[4] =
        {
            TopLeft,
            TopLeft + ExtentX,
            TopLeft + ExtentY,
            TopLeft + ExtentX + ExtentY
        };
        return Rectangle::FromPoints(points, 4);
    }

    /// <summary>
    /// Determines whether the specified location is contained within this rotated rectangle.
    /// </summary>
    /// <param name="location">The location to test.</param>
    /// <returns><c>true</c> if the specified location is contained by this rotated rectangle; otherwise, <c>false</c>.</returns>
    bool ContainsPoint(const Vector2& location) const
    {
        const Vector2 offset = location - TopLeft;
        const float det = Vector2::Cross(ExtentX, ExtentY);
        const float s = Vector2::Cross(offset, ExtentX) / -det;
        if (Math::IsInRange(s, 0.0f, 1.0f))
        {
            const float t = Vector2::Cross(offset, ExtentY) / det;
            return Math::IsInRange(t, 0.0f, 1.0f);
        }
        return false;
    }

public:

    bool operator ==(const RotatedRectangle& other) const
    {
        return
                TopLeft == other.TopLeft &&
                ExtentX == other.ExtentX &&
                ExtentY == other.ExtentY;
    }
};

template<>
struct TIsPODType<RotatedRectangle>
{
    enum { Value = true };
};

// Copyright (c) Wojciech Figat. All rights reserved.

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
    Float2 TopLeft;

    /// <summary>
    /// The transformed X extent (right-left).
    /// </summary>
    Float2 ExtentX;

    /// <summary>
    /// The transformed Y extent (bottom-top).
    /// </summary>
    Float2 ExtentY;

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
    RotatedRectangle(const Float2& topLeft, const Float2& extentX, const Float2& extentY)
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
        Float2 points[4] =
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
    bool ContainsPoint(const Float2& location) const
    {
        const Float2 offset = location - TopLeft;
        const float det = Float2::Cross(ExtentX, ExtentY);
        const float s = Float2::Cross(offset, ExtentX) / -det;
        if (Math::IsInRange(s, 0.0f, 1.0f))
        {
            const float t = Float2::Cross(offset, ExtentY) / det;
            return Math::IsInRange(t, 0.0f, 1.0f);
        }
        return false;
    }

    /// <summary>
    /// Calculates a rectangle that contains the shared part of both rectangles.
    /// </summary>
    /// <param name="a">The first rectangle.</param>
    /// <param name="b">The second rectangle.</param>
    /// <returns>Rectangle that contains shared part of a and b rectangles.</returns>
    static RotatedRectangle Shared(const RotatedRectangle& a, const Rectangle& b)
    {
        // Clip the rotated rectangle bounds within the given AABB
        RotatedRectangle result = a;
        result.TopLeft = Float2::Max(a.TopLeft, b.GetTopLeft());
        // TODO: do a little better math below (in case of actually rotated rectangle)
        result.ExtentX.X = Math::Min(result.TopLeft.X + result.ExtentX.X, b.GetRight()) - result.TopLeft.X;
        result.ExtentY.Y = Math::Min(result.TopLeft.Y + result.ExtentY.Y, b.GetBottom()) - result.TopLeft.Y;
        return result;
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

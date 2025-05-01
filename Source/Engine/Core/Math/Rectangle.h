// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Vector2.h"

/// <summary>
/// Describes rectangle in 2D space defines by location of its upper-left corner and the size.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Rectangle
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Rectangle);

    /// <summary>
    /// The empty rectangle.
    /// </summary>
    static Rectangle Empty;

public:
    /// <summary>
    /// Rectangle location (coordinates of the upper-left corner)
    /// </summary>
    API_FIELD() Float2 Location;

    /// <summary>
    /// Rectangle size
    /// </summary>
    API_FIELD() Float2 Size;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Rectangle() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Rectangle"/> struct.
    /// </summary>
    /// <param name="x">The X coordinate of the upper left corner.</param>
    /// <param name="y">The Y coordinate of the upper left corner.</param>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    Rectangle(float x, float y, float width, float height)
        : Location(x, y)
        , Size(width, height)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Rectangle"/> struct.
    /// </summary>
    /// <param name="location">The location of the upper left corner.</param>
    /// <param name="size">The size.</param>
    Rectangle(const Float2& location, const Float2& size)
        : Location(location)
        , Size(size)
    {
    }

public:
    String ToString() const;

public:
    // Gets width of the rectangle.
    float GetWidth() const
    {
        return Size.X;
    }

    // Gets height of the rectangle.
    float GetHeight() const
    {
        return Size.Y;
    }

    // Gets Y coordinate of the top edge of the rectangle.
    float GetY() const
    {
        return Location.Y;
    }

    // Gets Y coordinate of the top edge of the rectangle.
    float GetTop() const
    {
        return Location.Y;
    }

    // Gets Y coordinate of the bottom edge of the rectangle.
    float GetBottom() const
    {
        return Location.Y + Size.Y;
    }

    // Gets X coordinate of the left edge of the rectangle.
    float GetX() const
    {
        return Location.X;
    }

    // Gets X coordinate of the left edge of the rectangle.
    float GetLeft() const
    {
        return Location.X;
    }

    // Gets X coordinate of the right edge of the rectangle.
    float GetRight() const
    {
        return Location.X + Size.X;
    }

    // Gets position of the upper left corner of the rectangle.
    Float2 GetUpperLeft() const
    {
        return Location;
    }

    // Gets position of the upper right corner of the rectangle.
    Float2 GetUpperRight() const
    {
        return Location + Float2(Size.X, 0);
    }

    // Gets position of the bottom right corner of the rectangle.
    Float2 GetLowerRight() const
    {
        return Location + Size;
    }

    // Gets position of the bottom left corner of the rectangle.
    Float2 GetLowerLeft() const
    {
        return Location + Float2(0, Size.Y);
    }

    // Gets position of the upper left corner of the rectangle.
    Float2 GetTopLeft() const
    {
        return Location;
    }

    // Gets position of the upper right corner of the rectangle.
    Float2 GetTopRight() const
    {
        return Location + Float2(Size.X, 0);
    }

    // Gets position of the bottom right corner of the rectangle.
    Float2 GetBottomRight() const
    {
        return Location + Size;
    }

    // Gets position of the bottom left corner of the rectangle.
    Float2 GetBottomLeft() const
    {
        return Location + Float2(0, Size.Y);
    }

    // Gets center position of the rectangle.
    Float2 GetCenter() const
    {
        return Location + Size * 0.5f;
    }

public:
    Rectangle operator+(const Float2& v) const
    {
        return Rectangle(Location + v, Size);
    }

    Rectangle operator-(const Float2& v) const
    {
        return Rectangle(Location - v, Size);
    }

    Rectangle& operator+=(const Float2& b)
    {
        Offset(b);
        return *this;
    }

    Rectangle& operator-=(const Float2& b)
    {
        Offset(-b);
        return *this;
    }

    Rectangle operator*(float b) const
    {
        return MakeScaled(b);
    }

    Rectangle operator/(float b) const
    {
        return MakeScaled(1.0f / b);
    }

    Rectangle& operator*=(float b)
    {
        Scale(b);
        return *this;
    }

    Rectangle& operator/=(float b)
    {
        Scale(1.0f / b);
        return *this;
    }

    // Comparison operators
    bool operator==(const Rectangle& b) const
    {
        return Location == b.Location && Size == b.Size;
    }

    bool operator!=(const Rectangle& b) const
    {
        return Location != b.Location || Size != b.Size;
    }

public:
    static bool NearEqual(const Rectangle& a, const Rectangle& b)
    {
        return Float2::NearEqual(a.Location, b.Location) && Float2::NearEqual(a.Size, b.Size);
    }

    static bool NearEqual(const Rectangle& a, const Rectangle& b, float epsilon)
    {
        return Float2::NearEqual(a.Location, b.Location, epsilon) && Float2::NearEqual(a.Size, b.Size, epsilon);
    }

public:
    // Checks if rectangle contains given point.
    bool Contains(const Float2& location) const;

    // Determines whether this rectangle entirely contains a specified rectangle.
    bool Contains(const Rectangle& value) const;

    // Determines whether a specified rectangle intersects with this rectangle.
    bool Intersects(const Rectangle& value) const;

public:
    // Offsets rectangle position.
    void Offset(float x, float y);

    // Offsets rectangle position.
    void Offset(const Float2& offset);

    // Make offseted rectangle.
    Rectangle MakeOffsetted(const Float2& offset) const;

    // Expands rectangle area in all directions by given amount.
    void Expand(float toExpand);

    // Makes expanded rectangle area in all directions by given amount.
    Rectangle MakeExpanded(float toExpand) const;

    // Scale rectangle area in all directions by given amount.
    void Scale(float scale);

    // Makes scaled rectangle area in all directions by given amount.
    Rectangle MakeScaled(float scale) const;

public:
    // Calculates a rectangle that contains the union of rectangle and the arbitrary point.
    static Rectangle Union(const Rectangle& a, const Float2& b);

    // Calculates a rectangle that contains the union of a and b rectangles.
    static Rectangle Union(const Rectangle& a, const Rectangle& b);

    // Calculates a rectangle that contains the shared part of a and b rectangles.
    static Rectangle Shared(const Rectangle& a, const Rectangle& b);

    // Creates rectangle from two points.
    static Rectangle FromPoints(const Float2& p1, const Float2& p2);

    // Creates rectangle from list of points.
    static Rectangle FromPoints(const Float2* points, int32 pointsCount);
};

template<>
struct TIsPODType<Rectangle>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Rectangle, "X:{0} Y:{1} Width:{2} Height:{3}", v.Location.X, v.Location.Y, v.Size.X, v.Size.Y);

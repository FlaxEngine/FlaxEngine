// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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

    // Init
    // @param x Rectangle location X coordinate
    // @param y Rectangle location Y coordinate
    // @param width Rectangle width
    // @param height Rectangle height
    Rectangle(float x, float y, float width, float height)
        : Location(x, y)
        , Size(width, height)
    {
    }

    // Init
    // @param location Rectangle location point
    // @param width Rectangle size
    Rectangle(const Float2& location, const Float2& size)
        : Location(location)
        , Size(size)
    {
    }

public:
    String ToString() const;

public:
    // Returns width of the rectangle
    inline float GetWidth() const;

    // Returns height of the rectangle
    inline float GetHeight() const;

    // Gets Y coordinate of the top edge of the rectangle
    inline float GetY() const;

    // Gets Y coordinate of the top edge of the rectangle
    inline float GetTop() const;

    // Gets Y coordinate of the bottom edge of the rectangle
    inline float GetBottom() const;

    // Gets X coordinate of the left edge of the rectangle
    inline float GetX() const;

    // Gets X coordinate of the left edge of the rectangle
    inline float GetLeft() const;

    // Gets X coordinate of the right edge of the rectangle
    inline float GetRight() const;

    // Gets position of the upper left corner of the rectangle
    inline Float2 GetUpperLeft() const;

    // Gets position of the upper right corner of the rectangle
    inline Float2 GetUpperRight() const;

    // Gets position of the bottom right corner of the rectangle
    inline Float2 GetBottomRight() const;

    // Gets position of the bottom left corner of the rectangle
    inline Float2 GetBottomLeft() const;

    /// <summary>
    /// Gets center position of the rectangle
    /// </summary>
    /// <returns>Center point</returns>
    inline Float2 GetCenter() const;  

    /// <summary>
    /// Sets the center.
    /// </summary>
    /// <param name="value">The value.</param>
    inline void SetCenter(const Float2& value);

    /// <summary>
    /// Sets the top.
    /// </summary>
    /// <param name="value">The value.</param>
    inline void SetTop(float value);

    /// <summary>
    /// Sets the left.
    /// </summary>
    /// <param name="value">The value.</param>
    inline void SetLeft(float value);

    /// <summary>
    /// Sets the bottom.
    /// </summary>
    /// <param name="value">The value.</param>
    inline void SetBottom(float value);

    /// <summary>
    /// Sets the right.
    /// </summary>
    /// <param name="value">The value.</param>
    inline void SetRight(float value);

#pragma region Funcions
    // Checks if rectangle contains given point
    // @param location Point location to check
    // @returns True if point is inside rectangle's area
    bool Contains(const Float2& location) const;

    // Determines whether this rectangle entirely contains a specified rectangle
    // @param value The rectangle to evaluate
    // @returns True if this rectangle entirely contains the specified rectangle, or false if not
    bool Contains(const Rectangle& value) const;

    // Determines whether a specified rectangle intersects with this rectangle
    // @ value The rectangle to evaluate
    // @returns True if the specified rectangle intersects with this one, otherwise false
    bool Intersects(const Rectangle& value) const;

    // Offset rectangle position
    // @param x X coordinate offset
    // @param y Y coordinate offset
    void Offset(float x, float y);

    // Offset rectangle position
    // @param offset X and Y coordinate offset
    void Offset(const Float2& offset);

    // Make offseted rectangle
    // @param offset X and Y coordinate offset
    // @returns Offseted rectangle
    Rectangle MakeOffsetted(const Float2& offset) const;

    // Expand rectangle area in all directions by given amount
    // @param toExpand Amount of units to expand a rectangle
    void Expand(float toExpand);

    // Make expanded rectangle area in all directions by given amount
    // @param toExpand Amount of units to expand a rectangle
    // @returns Expanded rectangle
    Rectangle MakeExpanded(float toExpand) const;

    // Scale rectangle area in all directions by given amount
    // @param scale Scale value to expand a rectangle
    void Scale(float scale);

    // Make scaled rectangle area in all directions by given amount
    // @param scale Scale value to expand a rectangle
    // @returns Scaled rectangle
    Rectangle MakeScaled(float scale) const;
#pragma endregion
#pragma region Static Funcions
    // Calculates a rectangle that contains the union of rectangle and the arbitrary point
    // @param a The rectangle
    // @param b The point
    // @returns Rectangle that contains both rectangle and the point
    static Rectangle Union(const Rectangle& a, const Float2& b);

    // Calculates a rectangle that contains the union of a and b rectangles
    // @param a First rectangle
    // @param b Second rectangle
    // @returns Rectangle that contains both a and b rectangles
    static Rectangle Union(const Rectangle& a, const Rectangle& b);

    // Calculates a rectangle that contains the shared part of a and b rectangles
    // @param a First rectangle
    // @param b Second rectangle
    // @returns Rectangle that contains shared part of a and b rectangles
    static Rectangle Shared(const Rectangle& a, const Rectangle& b);

    // Create rectangle from two points
    // @param p1 First point
    // @param p2 Second point
    // @returns Rectangle that contains both p1 and p2
    static Rectangle FromPoints(const Float2& p1, const Float2& p2);

    static Rectangle FromPoints(Float2* points, int32 pointsCount);
    
    /// <summary>
    /// The rectangle has coordinates between zero and one for the x and y axes.
    /// This function will compute the real coordinates and return as a Float2.
    /// </summary>
    /// <param name="rectangle">Rectangle to get a point inside.</param>
    /// <param name="normalizedRectangleCoordinates">Normalized coordinates to get a point for.</param>
    /// <returns>a point inside a rectangle, given normalized coordinates</returns>
    static Float2 NormalizedToPoint(const Rectangle& rectangle, const Float2& normalizedRectangleCoordinates);
        
    /// <summary>
    /// The normalized coordinates cooresponding the the point.
    /// </summary>
    /// <param name="rectangle">Rectangle to get normalized coordinates inside.</param>
    /// <param name="normalizedRectangleCoordinates">A point inside the rectangle to get normalized coordinates for.</param>
    /// <returns>The returned Float2 is in the range 0 to 1 with values more 1 or less than zero are clamped</returns>
    static Float2 PointToNormalized(const Rectangle& rectangle, const Float2& point);

    static bool NearEqual(const Rectangle& a, const Rectangle& b)
    {
        return Float2::NearEqual(a.Location, b.Location) && Float2::NearEqual(a.Size, b.Size);
    }

    static bool NearEqual(const Rectangle& a, const Rectangle& b, float epsilon)
    {
        return Float2::NearEqual(a.Location, b.Location, epsilon) && Float2::NearEqual(a.Size, b.Size, epsilon);
    }

#pragma endregion
#pragma region ops

    // Offset rectangle Location point
    // @param v Offset to add
    // @returns Result rectangle
    Rectangle operator+(const Float2& v) const
    {
        return Rectangle(Location + v, Size);
    }

    // Offset rectangle Location point
    // @param v Offset to subtract
    // @returns Result rectangle
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

#pragma endregion
};

template<>
struct TIsPODType<Rectangle>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Rectangle, "X:{0} Y:{1} Width:{2} Height:{3}", v.Location.X, v.Location.Y, v.Size.X, v.Size.Y);

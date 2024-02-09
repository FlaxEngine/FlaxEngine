// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Rectangle.h"
#include "../Types/String.h"

Rectangle Rectangle::Empty(0, 0, 0, 0);

String Rectangle::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

// Returns width of the rectangle

inline float Rectangle::GetWidth() const
{
    return Size.X;
}

// Returns height of the rectangle

inline float Rectangle::GetHeight() const
{
	return Size.Y;
}

// Gets Y coordinate of the top edge of the rectangle

inline float Rectangle::GetY() const
{
    return Location.Y;
}

// Gets Y coordinate of the top edge of the rectangle

inline float Rectangle::GetTop() const
{
    return Location.Y;
}

// Gets Y coordinate of the bottom edge of the rectangle

inline float Rectangle::GetBottom() const
{
    return Location.Y + Size.Y;
}

// Gets X coordinate of the left edge of the rectangle

inline float Rectangle::GetX() const
{
    return Location.X;
}

// Gets X coordinate of the left edge of the rectangle

inline float Rectangle::GetLeft() const
{
    return Location.X;
}

// Gets X coordinate of the right edge of the rectangle

inline float Rectangle::GetRight() const
{
    return Location.X + Size.X;
}

// Gets position of the upper left corner of the rectangle

inline Float2 Rectangle::GetUpperLeft() const
{
    return Location;
}

// Gets position of the upper right corner of the rectangle

inline Float2 Rectangle::GetUpperRight() const
{
    return Location + Float2(Size.X, 0);
}

// Gets position of the bottom right corner of the rectangle

inline Float2 Rectangle::GetBottomRight() const
{
    return Location + Size;
}

// Gets position of the bottom left corner of the rectangle

inline Float2 Rectangle::GetBottomLeft() const
{
    return Location + Float2(0, Size.Y);
}

inline Float2 Rectangle::GetCenter() const
{
    return Location + Size * 0.5f;
}

inline void Rectangle::SetCenter(const Float2& value)
{
    Location = value - Size * 0.5f;
}

inline void Rectangle::SetTop(float value)
{
    float bottom = GetBottom();
    Location.Y = value;
    Size.Y = bottom - Location.Y;
}

inline void Rectangle::SetLeft(float value)
{
    float right = GetRight();
    Location.X = value;
    Size.X = right - Location.X;
}

inline void Rectangle::SetRight(float value)
{
    Size.X = value - Location.X;
}

inline void Rectangle::SetBottom(float value)
{
	Size.Y = value - Location.Y;
}

bool Rectangle::Contains(const Float2& location) const
{
    return location.X >= Location.X && location.Y >= Location.Y && (location.X <= Location.X + Size.X && location.Y <= Location.Y + Size.Y);
}

bool Rectangle::Contains(const Rectangle& value) const
{
    return Location.X <= value.Location.X && value.GetRight() <= GetRight() && Location.Y <= value.Location.Y && value.GetBottom() <= GetBottom();
}

bool Rectangle::Intersects(const Rectangle& value) const
{
    return value.Location.X <= GetRight() && Location.X <= value.GetRight() && value.Location.Y <= GetBottom() && Location.Y <= value.GetBottom();
}

void Rectangle::Offset(float x, float y)
{
    Location.X += x;
    Location.Y += y;
}

void Rectangle::Offset(const Float2& offset)
{
    Location += offset;
}

Rectangle Rectangle::MakeOffsetted(const Float2& offset) const
{
    return Rectangle(Location + offset, Size);
}

void Rectangle::Expand(float toExpand)
{
    Location -= toExpand * 0.5f;
    Size += toExpand;
}

Rectangle Rectangle::MakeExpanded(float toExpand) const
{
    return Rectangle(Location - toExpand * 0.5f, Size + toExpand);
}

void Rectangle::Scale(float scale)
{
    const Float2 toExpand = Size * (scale - 1.0f) * 0.5f;
    Location -= toExpand * 0.5f;
    Size += toExpand;
}

Rectangle Rectangle::MakeScaled(float scale) const
{
    const Float2 toExpand = Size * (scale - 1.0f) * 0.5f;
    return Rectangle(Location - toExpand * 0.5f, Size + toExpand);
}

Rectangle Rectangle::Union(const Rectangle& a, const Float2& b)
{
    const float left = Math::Min(a.GetLeft(), b.X);
    const float right = Math::Max(a.GetRight(), b.X);
    const float top = Math::Min(a.GetTop(), b.Y);
    const float bottom = Math::Max(a.GetBottom(), b.Y);
    return Rectangle(left, top, Math::Max(right - left, 0.0f), Math::Max(bottom - top, 0.0f));
}

Rectangle Rectangle::Union(const Rectangle& a, const Rectangle& b)
{
    const float left = Math::Min(a.GetLeft(), b.GetLeft());
    const float right = Math::Max(a.GetRight(), b.GetRight());
    const float top = Math::Min(a.GetTop(), b.GetTop());
    const float bottom = Math::Max(a.GetBottom(), b.GetBottom());
    return Rectangle(left, top, Math::Max(right - left, 0.0f), Math::Max(bottom - top, 0.0f));
}

Rectangle Rectangle::Shared(const Rectangle& a, const Rectangle& b)
{
    const float left = Math::Max(a.GetLeft(), b.GetLeft());
    const float right = Math::Min(a.GetRight(), b.GetRight());
    const float top = Math::Max(a.GetTop(), b.GetTop());
    const float bottom = Math::Min(a.GetBottom(), b.GetBottom());
    return Rectangle(left, top, Math::Max(right - left, 0.0f), Math::Max(bottom - top, 0.0f));
}

Rectangle Rectangle::FromPoints(const Float2& p1, const Float2& p2)
{
    const Float2 upperLeft = Float2::Min(p1, p2);
    const Float2 rightBottom = Float2::Max(p1, p2);
    return Rectangle(upperLeft, Math::Max(rightBottom - upperLeft, Float2::Zero));
}

Rectangle Rectangle::FromPoints(Float2* points, int32 pointsCount)
{
    ASSERT(pointsCount > 0);
    Float2 upperLeft = points[0];
    Float2 rightBottom = points[0];
    for (int32 i = 1; i < pointsCount; i++)
    {
        upperLeft = Float2::Min(upperLeft, points[i]);
        rightBottom = Float2::Max(rightBottom, points[i]);
    }
    return Rectangle(upperLeft, Math::Max(rightBottom - upperLeft, Float2::Zero));
}

Float2 Rectangle::NormalizedToPoint(const Rectangle& rectangle, const Float2& normalizedRectangleCoordinates)
{
    return Float2(
        Math::Lerp(rectangle.GetLeft(), rectangle.GetRight(), normalizedRectangleCoordinates.X),
        Math::Lerp(rectangle.GetTop(), rectangle.GetBottom(), normalizedRectangleCoordinates.Y)
    );
}

Float2 Rectangle::PointToNormalized(const Rectangle& rectangle, const Float2& point)
{
    auto val = Math::Clamp(point, Float2::Zero, rectangle.Size);
    val.X = Math::IsZero(val.X) ? 0 : val.X /= rectangle.Size.X;
    val.Y = Math::IsZero(val.Y) ? 0 : val.Y /= rectangle.Size.Y;
    return val;
}

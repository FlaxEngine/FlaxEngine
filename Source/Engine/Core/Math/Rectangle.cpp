// Copyright (c) Wojciech Figat. All rights reserved.

#include "Rectangle.h"
#include "../Types/String.h"

Rectangle Rectangle::Empty(0, 0, 0, 0);

String Rectangle::ToString() const
{
    return String::Format(TEXT("{}"), *this);
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

Rectangle Rectangle::FromPoints(const Float2* points, int32 pointsCount)
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

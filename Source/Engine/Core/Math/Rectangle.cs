// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    partial struct Rectangle : IEquatable<Rectangle>
    {
        /// <summary>
        /// A <see cref="Rectangle"/> which represents an empty space.
        /// </summary>
        public static readonly Rectangle Empty = new Rectangle(Float2.Zero, Float2.Zero);

        /// <summary>
        /// Gets or sets X coordinate of the left edge of the rectangle
        /// </summary>
        public float X
        {
            get => Location.X;
            set => Location.X = value;
        }

        /// <summary>
        /// Gets or sets Y coordinate of the left edge of the rectangle
        /// </summary>
        public float Y
        {
            get => Location.Y;
            set => Location.Y = value;
        }

        /// <summary>
        /// Gets or sets width of the rectangle
        /// </summary>
        public float Width
        {
            get => Size.X;
            set => Size.X = value;
        }

        /// <summary>
        /// Gets or sets height of the rectangle
        /// </summary>
        public float Height
        {
            get => Size.Y;
            set => Size.Y = value;
        }

        /// <summary>
        /// Gets Y coordinate of the top edge of the rectangle
        /// </summary>
        public float Top => Location.Y;

        /// <summary>
        /// Gets Y coordinate of the bottom edge of the rectangle
        /// </summary>
        public float Bottom => Location.Y + Size.Y;

        /// <summary>
        /// Gets X coordinate of the left edge of the rectangle
        /// </summary>
        public float Left => Location.X;

        /// <summary>
        /// Gets X coordinate of the right edge of the rectangle
        /// </summary>
        public float Right => Location.X + Size.X;

        /// <summary>
        /// Gets position of the upper left corner of the rectangle
        /// </summary>
        public Float2 UpperLeft => Location;

        /// <summary>
        /// Gets position of the upper right corner of the rectangle
        /// </summary>
        public Float2 UpperRight => new Float2(Location.X + Size.X, Location.Y);

        /// <summary>
        /// Gets position of the bottom right corner of the rectangle
        /// </summary>
        public Float2 LowerRight => Location + Size;

        /// <summary>
        /// Gets position of the bottom left corner of the rectangle
        /// </summary>
        public Float2 LowerLeft => new Float2(Location.X, Location.Y + Size.Y);

        /// <summary>
        /// Gets position of the upper left corner of the rectangle
        /// </summary>
        public Float2 TopLeft => Location;

        /// <summary>
        /// Gets position of the upper right corner of the rectangle
        /// </summary>
        public Float2 TopRight => new Float2(Location.X + Size.X, Location.Y);

        /// <summary>
        /// Gets position of the bottom right corner of the rectangle
        /// </summary>
        public Float2 BottomRight => Location + Size;

        /// <summary>
        /// Gets position of the bottom left corner of the rectangle
        /// </summary>
        public Float2 BottomLeft => new Float2(Location.X, Location.Y + Size.Y);

        /// <summary>
        /// Gets center position of the rectangle
        /// </summary>
        public Float2 Center => Location + Size * 0.5f;

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="x">X coordinate</param>
        /// <param name="y">Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        public Rectangle(float x, float y, float width, float height)
        {
            Location = new Float2(x, y);
            Size = new Float2(width, height);
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="location">Location of the upper left corner</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        public Rectangle(Float2 location, float width, float height)
        {
            Location = location;
            Size = new Float2(width, height);
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="x">X coordinate</param>
        /// <param name="y">Y coordinate</param>
        /// <param name="size">Size</param>
        public Rectangle(float x, float y, Float2 size)
        {
            Location = new Float2(x, y);
            Size = size;
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="location">Location of the upper left corner</param>
        /// <param name="size">Size</param>
        public Rectangle(Float2 location, Float2 size)
        {
            Location = location;
            Size = size;
        }

        /// <summary>
        /// Checks if rectangle contains given point
        /// </summary>
        /// <param name="location">Point location to check</param>
        /// <returns>True if point is inside rectangle's area</returns>
        public bool Contains(Float2 location)
        {
            return (location.X >= Location.X && location.Y >= Location.Y) && (location.X <= Location.X + Size.X && location.Y <= Location.Y + Size.Y);
        }

        /// <summary>
        /// Checks if rectangle contains given point
        /// </summary>
        /// <param name="location">Point location to check</param>
        /// <returns>True if point is inside rectangle's area</returns>
        public bool Contains(ref Float2 location)
        {
            return (location.X >= Location.X && location.Y >= Location.Y) && (location.X <= Location.X + Size.X && location.Y <= Location.Y + Size.Y);
        }

        /// <summary>
        /// Determines whether this rectangle entirely contains a specified rectangle
        /// </summary>
        /// <param name="value">The rectangle to evaluate</param>
        /// <returns>True if this rectangle entirely contains the specified rectangle, or false if not</returns>
        public bool Contains(Rectangle value)
        {
            return (Location.X <= value.Location.X) && (value.Right <= Right) && (Location.Y <= value.Location.Y) && (value.Bottom <= Bottom);
        }

        /// <summary>
        /// Determines whether this rectangle entirely contains a specified rectangle
        /// </summary>
        /// <param name="value">The rectangle to evaluate</param>
        /// <returns>True if this rectangle entirely contains the specified rectangle, or false if not</returns>
        public bool Contains(ref Rectangle value)
        {
            return (Location.X <= value.Location.X) && (value.Right <= Right) && (Location.Y <= value.Location.Y) && (value.Bottom <= Bottom);
        }

        /// <summary>
        /// Determines whether a specified rectangle intersects with this rectangle
        /// </summary>
        /// <param name="value">The rectangle to evaluate</param>
        /// <returns>True if the specified rectangle intersects with this one, otherwise false</returns>
        public bool Intersects(Rectangle value)
        {
            return (value.Location.X <= Right) && (Location.X <= value.Right) && (value.Location.Y <= Bottom) && (Location.Y <= value.Bottom);
        }

        /// <summary>
        /// Determines whether a specified rectangle intersects with this rectangle
        /// </summary>
        /// <param name="value">The rectangle to evaluate</param>
        /// <returns>True if the specified rectangle intersects with this one, otherwise false</returns>
        public bool Intersects(ref Rectangle value)
        {
            return (value.Location.X <= Right) && (Location.X <= value.Right) && (value.Location.Y <= Bottom) && (Location.Y <= value.Bottom);
        }

        /// <summary>
        /// Offset rectangle position
        /// </summary>
        /// <param name="x">X coordinate offset</param>
        /// <param name="y">Y coordinate offset</param>
        public void Offset(float x, float y)
        {
            X += x;
            Y += y;
        }

        /// <summary>
        /// Offset rectangle position
        /// </summary>
        /// <param name="offset">X and Y coordinate offset</param>
        public void Offset(Float2 offset)
        {
            Location += offset;
        }

        /// <summary>
        /// Make offseted rectangle
        /// </summary>
        /// <param name="x">X coordinate offset</param>
        /// <param name="y">Y coordinate offset</param>
        /// <returns>Offseted rectangle</returns>
        public Rectangle MakeOffsetted(float x, float y)
        {
            return new Rectangle(Location + new Float2(x, y), Size);
        }

        /// <summary>
        /// Make offseted rectangle
        /// </summary>
        /// <param name="offset">X and Y coordinate offset</param>
        /// <returns>Offseted rectangle</returns>
        public Rectangle MakeOffsetted(Float2 offset)
        {
            return new Rectangle(Location + offset, Size);
        }

        /// <summary>
        /// Expand rectangle area in all directions by given amount
        /// </summary>
        /// <param name="toExpand">Amount of units to expand a rectangle</param>
        public void Expand(float toExpand)
        {
            Location -= toExpand * 0.5f;
            Size += toExpand;
        }

        /// <summary>
        /// Make expanded rectangle area in all directions by given amount
        /// </summary>
        /// <param name="toExpand">Amount of units to expand a rectangle</param>
        /// <returns>Expanded rectangle</returns>
        public Rectangle MakeExpanded(float toExpand)
        {
            return new Rectangle(Location - toExpand * 0.5f, Size + toExpand);
        }

        /// <summary>
        /// Scale rectangle area in all directions by given amount
        /// </summary>
        /// <param name="scale">Scale value to expand a rectangle</param>
        public void Scale(float scale)
        {
            Float2 toExpand = Size * (scale - 1.0f) * 0.5f;
            Location -= toExpand * 0.5f;
            Size += toExpand;
        }

        /// <summary>
        /// Make scaled rectangle area in all directions by given amount
        /// </summary>
        /// <param name="scale">Scale value to expand a rectangle</param>
        /// <returns>Scaled rectangle</returns>
        public Rectangle MakeScaled(float scale)
        {
            Float2 toExpand = Size * (scale - 1.0f) * 0.5f;
            return new Rectangle(Location - toExpand * 0.5f, Size + toExpand);
        }

        /// <summary>
        /// Computed nearest distance between 2 rectangles.
        /// </summary>
        /// <param name="a">First rectangle</param>
        /// <param name="b">Second rectangle</param>
        /// <returns>Resulting distance, 0 if overlapping</returns>
        public static float Distance(Rectangle a, Rectangle b)
        {
            return Float2.Max(Float2.Zero, Float2.Abs(a.Center - b.Center) - ((a.Size + b.Size) * 0.5f)).Length;
        }

        /// <summary>
        /// Computed distance between rectangle and the point.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        /// <param name="p">The point.</param>
        /// <returns>The resulting distance, 0 if point is inside the rectangle.</returns>
        public static float Distance(Rectangle rect, Float2 p)
        {
            var max = rect.Location + rect.Size;
            var dx = Mathf.Max(Mathf.Max(rect.Location.X - p.X, p.X - max.X), 0);
            var dy = Mathf.Max(Math.Max(rect.Location.Y - p.Y, p.Y - max.Y), 0);
            return Mathf.Sqrt(dx * dx + dy * dy);
        }

        /// <summary>
        /// Calculates a rectangle that includes the margins (inside).
        /// </summary>
        /// <param name="value">The rectangle.</param>
        /// <param name="margin">The margin to apply to the rectangle.</param>
        /// <returns>Rectangle inside the given rectangle after applying margins inside it.</returns>
        public static Rectangle Margin(Rectangle value, GUI.Margin margin)
        {
            value.Location += margin.Location;
            value.Size -= margin.Size;
            return value;
        }

        /// <summary>
        /// Calculates a rectangle that contains the union of a and b rectangles
        /// </summary>
        /// <param name="a">The first rectangle.</param>
        /// <param name="b">The second rectangle.</param>
        /// <returns>Rectangle that contains both a and b rectangles</returns>
        public static Rectangle Union(Rectangle a, Rectangle b)
        {
            float left = Mathf.Min(a.Left, b.Left);
            float right = Mathf.Max(a.Right, b.Right);
            float top = Mathf.Min(a.Top, b.Top);
            float bottom = Mathf.Max(a.Bottom, b.Bottom);
            return new Rectangle(left, top, Mathf.Max(right - left, 0.0f), Mathf.Max(bottom - top, 0.0f));
        }

        /// <summary>
        /// Calculates a rectangle that contains the union of a and b rectangles
        /// </summary>
        /// <param name="a">First rectangle</param>
        /// <param name="b">Second rectangle</param>
        /// <param name="result">When the method completes, contains the rectangle that both a and b rectangles.</param>
        public static void Union(ref Rectangle a, ref Rectangle b, out Rectangle result)
        {
            float left = Mathf.Min(a.Left, b.Left);
            float right = Mathf.Max(a.Right, b.Right);
            float top = Mathf.Min(a.Top, b.Top);
            float bottom = Mathf.Max(a.Bottom, b.Bottom);
            result = new Rectangle(left, top, Mathf.Max(right - left, 0.0f), Mathf.Max(bottom - top, 0.0f));
        }

        /// <summary>
        /// Calculates a rectangle that contains the shared part of a and b rectangles.
        /// </summary>
        /// <param name="a">The first rectangle.</param>
        /// <param name="b">The second rectangle.</param>
        /// <returns>Rectangle that contains shared part of a and b rectangles.</returns>
        public static Rectangle Shared(Rectangle a, Rectangle b)
        {
            float left = Mathf.Max(a.Left, b.Left);
            float right = Mathf.Min(a.Right, b.Right);
            float top = Mathf.Max(a.Top, b.Top);
            float bottom = Mathf.Min(a.Bottom, b.Bottom);
            return new Rectangle(left, top, Mathf.Max(right - left, 0.0f), Mathf.Max(bottom - top, 0.0f));
        }

        /// <summary>
        /// Calculates a rectangle that contains the shared part of a and b rectangles.
        /// </summary>
        /// <param name="a">The first rectangle.</param>
        /// <param name="b">The second rectangle.</param>
        /// <param name="result">When the method completes, contains the rectangle that shared part of a and b rectangles.</param>
        public static void Shared(ref Rectangle a, ref Rectangle b, out Rectangle result)
        {
            float left = Mathf.Max(a.Left, b.Left);
            float right = Mathf.Min(a.Right, b.Right);
            float top = Mathf.Max(a.Top, b.Top);
            float bottom = Mathf.Min(a.Bottom, b.Bottom);
            result = new Rectangle(left, top, Mathf.Max(right - left, 0.0f), Mathf.Max(bottom - top, 0.0f));
        }

        /// <summary>
        /// Creates rectangle from two points.
        /// </summary>
        /// <param name="p1">First point</param>
        /// <param name="p2">Second point</param>
        /// <returns>Rectangle that contains both p1 and p2</returns>
        public static Rectangle FromPoints(Float2 p1, Float2 p2)
        {
            Float2.Min(ref p1, ref p2, out var upperLeft);
            Float2.Max(ref p1, ref p2, out var rightBottom);
            return new Rectangle(upperLeft, Float2.Max(rightBottom - upperLeft, Float2.Zero));
        }

        /// <summary>
        /// Creates rectangle from two points.
        /// </summary>
        /// <param name="p1">First point</param>
        /// <param name="p2">Second point</param>
        /// <returns>Rectangle that contains both p1 and p2</returns>
        /// <param name="result">When the method completes, contains the rectangle that contains both p1 and p2 points.</param>
        public static void FromPoints(ref Float2 p1, ref Float2 p2, out Rectangle result)
        {
            Float2.Min(ref p1, ref p2, out var upperLeft);
            Float2.Max(ref p1, ref p2, out var rightBottom);
            result = new Rectangle(upperLeft, Float2.Max(rightBottom - upperLeft, Float2.Zero));
        }

        #region Operators

        /// <summary>
        /// Implements the operator +.
        /// </summary>
        /// <param name="rectangle">The rectangle.</param>
        /// <param name="offset">The offset.</param>
        /// <returns>The result of the operator.</returns>
        public static Rectangle operator +(Rectangle rectangle, Float2 offset)
        {
            return new Rectangle(rectangle.Location + offset, rectangle.Size);
        }

        /// <summary>
        /// Implements the operator -.
        /// </summary>
        /// <param name="rectangle">The rectangle.</param>
        /// <param name="offset">The offset.</param>
        /// <returns>The result of the operator.</returns>
        public static Rectangle operator -(Rectangle rectangle, Float2 offset)
        {
            return new Rectangle(rectangle.Location - offset, rectangle.Size);
        }

        /// <summary>
        /// Implements the operator *.
        /// </summary>
        /// <param name="rectangle">The rectangle.</param>
        /// <param name="scale">The scale.</param>
        /// <returns>The result of the operator.</returns>
        public static Rectangle operator *(Rectangle rectangle, float scale)
        {
            return rectangle.MakeScaled(scale);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Rectangle left, Rectangle right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Rectangle left, Rectangle right)
        {
            return !left.Equals(ref right);
        }

        #endregion

        /// <summary>
        /// Determines whether the specified <see cref="Rectangle" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Rectangle" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Rectangle" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Rectangle other)
        {
            return Location == other.Location && Size == other.Size;
        }

        /// <inheritdoc />
        public bool Equals(Rectangle other)
        {
            return Equals(ref other);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is Rectangle other && Equals(ref other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return (Location.GetHashCode() * 397) ^ Size.GetHashCode();
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "X:{0} Y:{1} Width:{2} Height:{3}", X, Y, Width, Height);
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Describes the space around a control.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct Margin : IEquatable<Margin>, IFormattable
    {
        private static readonly string _formatString = "Left:{0:F2} Right:{1:F2} Top:{2:F2} Bottom:{3:F2}";

        /// <summary>
        /// The size of the <see cref="Margin" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Margin));

        /// <summary>
        /// A <see cref="Margin" /> with all of its components set to zero.
        /// </summary>
        public static readonly Margin Zero;

        /// <summary>
        /// Holds the margin to the left.
        /// </summary>
        [EditorOrder(0)]
        public float Left;

        /// <summary>
        /// Holds the margin to the right.
        /// </summary>
        [EditorOrder(1)]
        public float Right;

        /// <summary>
        /// Holds the margin to the top.
        /// </summary>
        [EditorOrder(2)]
        public float Top;

        /// <summary>
        /// Holds the margin to the bottom.
        /// </summary>
        [EditorOrder(3)]
        public float Bottom;

        /// <summary>
        /// Gets the margin's location (Left, Top).
        /// </summary>
        public Float2 Location => new Float2(Left, Top);

        /// <summary>
        /// Gets the margin's total size. Cumulative margin size (Left + Right, Top + Bottom).
        /// </summary>
        public Float2 Size => new Float2(Left + Right, Top + Bottom);

        /// <summary>
        /// Gets the width (left + right).
        /// </summary>
        public float Width => Left + Right;

        /// <summary>
        /// Gets the height (top + bottom).
        /// </summary>
        public float Height => Top + Bottom;

        /// <summary>
        /// Initializes a new instance of the <see cref="Margin"/> struct.
        /// </summary>
        /// <param name="value">The value.</param>
        public Margin(float value)
        {
            Left = Right = Top = Bottom = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Margin"/> struct.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <param name="top">The top.</param>
        /// <param name="bottom">The bottom.</param>
        public Margin(float left, float right, float top, float bottom)
        {
            Left = left;
            Right = right;
            Top = top;
            Bottom = bottom;
        }

        /// <summary>
        /// Gets a value indicting whether this margin is zero.
        /// </summary>
        public bool IsZero => Mathf.IsZero(Left) && Mathf.IsZero(Right) && Mathf.IsZero(Top) && Mathf.IsZero(Bottom);

        /// <summary>
        /// Shrinks the rectangle by this margin.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        public void ShrinkRectangle(ref Rectangle rect)
        {
            rect.Location.X += Left;
            rect.Location.Y += Top;
            rect.Size.X -= Left + Right;
            rect.Size.Y -= Top + Bottom;
        }

        /// <summary>
        /// Expands the rectangle by this margin.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        public void ExpandRectangle(ref Rectangle rect)
        {
            rect.Location.X -= Left;
            rect.Location.Y -= Top;
            rect.Size.X += Left + Right;
            rect.Size.Y += Top + Bottom;
        }

        /// <summary>
        /// Adds two margins.
        /// </summary>
        /// <param name="left">The first margins to add.</param>
        /// <param name="right">The second margins to add.</param>
        /// <returns>The sum of the two margins.</returns>
        public static Margin operator +(Margin left, Margin right)
        {
            return new Margin(left.Left + right.Left, left.Right + right.Right, left.Top + right.Top, left.Bottom + right.Bottom);
        }

        /// <summary>
        /// Subtracts two margins.
        /// </summary>
        /// <param name="left">The first margins to subtract from.</param>
        /// <param name="right">The second margins to subtract.</param>
        /// <returns>The result of subtraction of the two margins.</returns>
        public static Margin operator -(Margin left, Margin right)
        {
            return new Margin(left.Left - right.Left, left.Right - right.Right, left.Top - right.Top, left.Bottom - right.Bottom);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Margin left, Margin right)
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
        public static bool operator !=(Margin left, Margin right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Left:{0} Right:{1} Top:{2} Bottom:{3}", Left, Right, Top, Bottom);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();
            return string.Format(CultureInfo.CurrentCulture, _formatString,
                                 Left.ToString(format, CultureInfo.CurrentCulture),
                                 Right.ToString(format, CultureInfo.CurrentCulture),
                                 Top.ToString(format, CultureInfo.CurrentCulture),
                                 Bottom.ToString(format, CultureInfo.CurrentCulture)
                                );
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString,
                                 Left,
                                 Right,
                                 Top,
                                 Bottom
                                );
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);
            return string.Format(formatProvider, _formatString,
                                 Left.ToString(format, formatProvider),
                                 Right.ToString(format, formatProvider),
                                 Top.ToString(format, formatProvider),
                                 Bottom.ToString(format, formatProvider)
                                );
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = Left.GetHashCode();
                hashCode = (hashCode * 397) ^ Right.GetHashCode();
                hashCode = (hashCode * 397) ^ Top.GetHashCode();
                hashCode = (hashCode * 397) ^ Bottom.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Margin" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Margin" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Margin" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public bool Equals(ref Margin other)
        {
            return Mathf.NearEqual(other.Left, Left) &&
                   Mathf.NearEqual(other.Right, Right) &&
                   Mathf.NearEqual(other.Top, Top) &&
                   Mathf.NearEqual(other.Bottom, Bottom);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Margin"/> are equal.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool Equals(ref Margin a, ref Margin b)
        {
            return a.Equals(ref b);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Margin" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Margin" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Margin" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Margin other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Margin margin && Equals(ref margin);
        }
    }
}

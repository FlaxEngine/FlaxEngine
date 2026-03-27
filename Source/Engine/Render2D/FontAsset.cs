// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial struct FontOptions
    {
        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="other">The other object to compare.</param>
        /// <returns><c>true</c> if this object has the same value as <paramref name="other" />; otherwise, <c>false</c> </returns>
        public bool Equals(FontOptions other)
        {
            return Hinting == other.Hinting && Flags == other.Flags && RasterMode == other.RasterMode;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is FontOptions other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return HashCode.Combine((int)Hinting, (int)Flags, (int)RasterMode);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator ==(FontOptions left, FontOptions right)
        {
            return left.Hinting == right.Hinting && left.Flags == right.Flags && left.RasterMode == right.RasterMode;
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise,<c>false</c>.</returns>
        public static bool operator !=(FontOptions left, FontOptions right)
        {
            return left.Hinting != right.Hinting || left.Flags != right.Flags || left.RasterMode != right.RasterMode;
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial struct TextRange
    {
        /// <summary>
        /// Gets the range length.
        /// </summary>
        public int Length => EndIndex - StartIndex;

        /// <summary>
        /// Initializes a new instance of the <see cref="TextRange"/> struct.
        /// </summary>
        /// <param name="startIndex">The start index.</param>
        /// <param name="endIndex">The end index.</param>
        public TextRange(int startIndex, int endIndex)
        {
            StartIndex = startIndex;
            EndIndex = endIndex;
        }

        /// <summary>
        /// Gets a value indicating whether range is empty.
        /// </summary>
        public bool IsEmpty => (EndIndex - StartIndex) <= 0;

        /// <summary>
        /// Determines whether this range contains the character index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns><c>true</c> if range contains the specified character index; otherwise, <c>false</c>.</returns>
        public bool Contains(int index) => index >= StartIndex && index < EndIndex;

        /// <summary>
        /// Determines whether this range intersects with the other range.
        /// </summary>
        /// <param name="other">The other text range.</param>
        /// <returns><c>true</c> if range intersects with the specified range index;, <c>false</c>.</returns>
        public bool Intersect(ref TextRange other) => Math.Min(EndIndex, other.EndIndex) > Math.Max(StartIndex, other.StartIndex);

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format("Range: [{0}, {1}), Length: {2}", StartIndex, EndIndex, Length);
        }
    }
}

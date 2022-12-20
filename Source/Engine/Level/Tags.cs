// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    partial struct Tag : IEquatable<Tag>, IEquatable<string>, IComparable, IComparable<Tag>, IComparable<string>
    {
        /// <summary>
        /// The default <see cref="Tag"/>.
        /// </summary>
        public static Tag Default => new Tag(-1);

        /// <summary>
        /// Initializes a new instance of the <see cref="Tag" /> struct.
        /// </summary>
        /// <param name="index">The tag index.</param>
        public Tag(int index)
        {
            Index = index;
        }

        [System.Runtime.Serialization.OnDeserializing]
        internal void OnDeserializing(System.Runtime.Serialization.StreamingContext context)
        {
            // Initialize structure with default values to replicate C++ deserialization behavior
            this.Index = -1;
        }

        /// <summary>
        /// Compares two tags.
        /// </summary>
        /// <param name="left">The lft tag.</param>
        /// <param name="right">The right tag.</param>
        /// <returns>True if both values are equal, otherwise false.</returns>
        public static bool operator ==(Tag left, Tag right)
        {
            return left.Index == right.Index;
        }

        /// <summary>
        /// Compares two tags.
        /// </summary>
        /// <param name="left">The lft tag.</param>
        /// <param name="right">The right tag.</param>
        /// <returns>True if both values are not equal, otherwise false.</returns>
        public static bool operator !=(Tag left, Tag right)
        {
            return left.Index == right.Index;
        }

        /// <summary>
        /// Checks if tag is valid.
        /// </summary>
        /// <param name="tag">The tag to check.</param>
        /// <returns>True if tag is valid, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator bool(Tag tag)
        {
            return tag.Index != -1;
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is string asString)
                return CompareTo(asString);
            if (obj is Tag asTag)
                return CompareTo(asTag);
            return 0;
        }

        /// <inheritdoc />
        public bool Equals(Tag other)
        {
            return Index == other.Index;
        }

        /// <inheritdoc />
        public bool Equals(string other)
        {
            return string.Equals(ToString(), other, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public int CompareTo(Tag other)
        {
            return string.Compare(ToString(), ToString(), StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public int CompareTo(string other)
        {
            return string.Compare(ToString(), other, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is Tag other && Index == other.Index;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Index;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Tags.Internal_GetTagName(Index);
        }
    }
}

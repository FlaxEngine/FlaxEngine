// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    [TypeConverter(typeof(TypeConverters.TagConverter))]
    partial struct Tag : IEquatable<Tag>, IEquatable<string>, IComparable, IComparable<Tag>, IComparable<string>
    {
        /// <summary>
        /// The default <see cref="Tag"/>.
        /// </summary>
        public static Tag Default => new Tag(0);

        /// <summary>
        /// Initializes a new instance of the <see cref="Tag" /> struct.
        /// </summary>
        /// <param name="index">The tag index.</param>
        public Tag(uint index)
        {
            Index = index;
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
            return left.Index != right.Index;
        }

        /// <summary>
        /// Compares two tags.
        /// </summary>
        /// <param name="left">The lft tag.</param>
        /// <param name="right">The right tag name.</param>
        /// <returns>True if both values are equal, otherwise false.</returns>
        public static bool operator ==(Tag left, string right)
        {
            return string.Equals(left.ToString(), right, StringComparison.Ordinal);
        }

        /// <summary>
        /// Compares two tags.
        /// </summary>
        /// <param name="left">The lft tag.</param>
        /// <param name="right">The right tag name.</param>
        /// <returns>True if both values are not equal, otherwise false.</returns>
        public static bool operator !=(Tag left, string right)
        {
            return !string.Equals(left.ToString(), right, StringComparison.Ordinal);
        }

        /// <summary>
        /// Checks if tag is valid.
        /// </summary>
        /// <param name="tag">The tag to check.</param>
        /// <returns>True if tag is valid, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator bool(Tag tag)
        {
            return tag.Index != 0;
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
            return (int)Index;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Tags.Internal_GetTagName(Index);
        }
    }

    partial class Tags
    {
        /// <summary>
        /// Checks if the list of tags contains a given tag (including parent tags check). For example, HasTag({"A.B"}, "A") returns true, for exact check use HasTagExact.
        /// </summary>
        /// <param name="list">The tags list to use.</param>
        /// <param name="tag">The tag to check.</param>
        /// <returns>True if given tag is contained by the list of tags. Returns false for empty list.</returns>
        public static bool HasTag(this Tag[] list, Tag tag)
        {
            if (tag.Index == 0)
                return false;
            string tagName = tag.ToString();
            foreach (Tag e in list)
            {
                string eName = e.ToString();
                if (e == tag || (eName.Length > tagName.Length + 1 && eName.StartsWith(tagName) && eName[tagName.Length] == '.'))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Checks if the list of tags contains a given tag (exact match). For example, HasTag({"A.B"}, "A") returns false, for parents check use HasTag.
        /// </summary>
        /// <param name="list">The tags list to use.</param>
        /// <param name="tag">The tag to check.</param>
        /// <returns>True if given tag is contained by the list of tags. Returns false for empty list.</returns>
        public static bool HasTagExact(this Tag[] list, Tag tag)
        {
            if (tag.Index == 0)
                return false;
            if (list == null)
                return false;
            foreach (Tag e in list)
            {
                if (e == tag)
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Checks if the list of tags contains any of the given tags (including parent tags check). For example, HasAny({"A.B", "C"}, {"A"}) returns true, for exact check use HasAnyExact.
        /// </summary>
        /// <param name="list">The tags list to use.</param>
        /// <param name="tags">The tags to check.</param>
        /// <returns>True if any of of the given tags is contained by the list of tags.</returns>
        public static bool HasAny(this Tag[] list, Tag[] tags)
        {
            if (list == null)
                return false;
            foreach (Tag tag in tags)
            {
                if (HasTag(list, tag))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Checks if the list of tags contains any of the given tags (exact match). For example, HasAnyExact({"A.B", "C"}, {"A"}) returns false, for parents check use HasAny.
        /// </summary>
        /// <param name="list">The tags list to use.</param>
        /// <param name="tags">The tags to check.</param>
        /// <returns>True if any of the given tags is contained by the list of tags.</returns>
        public static bool HasAnyExact(this Tag[] list, Tag[] tags)
        {
            if (list == null)
                return false;
            foreach (Tag tag in tags)
            {
                if (HasTagExact(list, tag))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Checks if the list of tags contains all the given tags (including parent tags check). For example, HasAll({"A.B", "C"}, {"A", "C"}) returns true, for exact check use HasAllExact.
        /// </summary>
        /// <param name="list">The tags list to use.</param>
        /// <param name="tags">The tags to check.</param>
        /// <returns>True if all the given tags are contained by the list of tags. Returns true for empty list.</returns>
        public static bool HasAll(this Tag[] list, Tag[] tags)
        {
            if (tags == null || tags.Length == 0)
                return true;
            if (list == null || list.Length == 0)
                return false;
            foreach (Tag tag in tags)
            {
                if (!HasTag(list, tag))
                    return false;
            }
            return true;
        }

        /// <summary>
        /// Checks if the list of tags contains all the given tags (exact match). For example, HasAllExact({"A.B", "C"}, {"A", "C"}) returns false, for parents check use HasAll.
        /// </summary>
        /// <param name="list">The tags list to use.</param>
        /// <param name="tags">The tags to check.</param>
        /// <returns>True if all the given tags are contained by the list of tags. Returns true for empty list.</returns>
        public static bool HasAllExact(this Tag[] list, Tag[] tags)
        {
            if (tags == null || tags.Length == 0)
                return true;
            if (list == null || list.Length == 0)
                return false;
            foreach (Tag tag in tags)
            {
                if (!HasTagExact(list, tag))
                    return false;
            }
            return true;
        }
    }
}

namespace FlaxEngine.TypeConverters
{
    internal class TagConverter : TypeConverter
    {
        /// <inheritdoc />
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
                return true;
            return base.CanConvertFrom(context, sourceType);
        }

        /// <inheritdoc />
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value is string str)
            {
                if (str.Length == 0)
                    return Tag.Default;
                return Tags.Get(str);
            }
            return base.ConvertFrom(context, culture, value);
        }

        /// <inheritdoc />
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                var v = (Tag)value;
                return v.ToString();
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}

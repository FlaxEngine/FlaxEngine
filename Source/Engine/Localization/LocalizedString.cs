// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;

namespace FlaxEngine
{
    partial class Localization
    {
        /// <summary>
        /// Creates new culture.
        /// </summary>
        /// <param name="name">The name (eg. en, pl-PL).</param>
        /// <returns>The culture.</returns>
        public static CultureInfo NewCulture(string name)
        {
            return new CultureInfo(name);
        }
    }

    partial class LocalizedString : IEquatable<LocalizedString>, IEquatable<string>, IComparable, IComparable<LocalizedString>, IComparable<string>
    {
        /// <summary>
        /// Empty string without localization.
        /// </summary>
        public static readonly LocalizedString Empty = new LocalizedString(null);

        /// <summary>
        /// Initializes a new instance of the <see cref="LocalizedString"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        public LocalizedString(string value)
        {
            Value = value;
        }

        /// <summary>
        /// Gets the localized plural string for the current language by using string id lookup.
        /// </summary>
        /// <param name="n">The value count for plural message selection.</param>
        /// <returns>The localized text.</returns>
        public string ToStringPlural(int n)
        {
            return string.IsNullOrEmpty(Value) ? Localization.GetPluralString(Id, n) : string.Format(Value, n);
        }

        /// <summary>
        /// Implicit converter of <see cref="LocalizedString"/> into <see cref="string"/>.
        /// </summary>
        /// <param name="str">The localized string.</param>
        /// <returns>The string.</returns>
        public static implicit operator string(LocalizedString str)
        {
            if ((object)str == null)
                return null;
            return string.IsNullOrEmpty(str.Value) ? Localization.GetString(str.Id) : str.Value;
        }

        /// <summary>
        /// Implicit converter of <see cref="string"/> into <see cref="LocalizedString"/>.
        /// </summary>
        /// <param name="str">The string.</param>
        /// <returns>The localized string.</returns>
        public static implicit operator LocalizedString(string str)
        {
            if (str == null)
                return null;
            return new LocalizedString(str);
        }

        /// <summary>
        /// Compares two localized strings.
        /// </summary>
        /// <param name="left">The lft string.</param>
        /// <param name="right">The right string.</param>
        /// <returns>True if both values are equal, otherwise false.</returns>
        public static bool operator ==(LocalizedString left, LocalizedString right)
        {
            return left?.Equals(right) ?? (object)right == null;
        }

        /// <summary>
        /// Compares two localized strings.
        /// </summary>
        /// <param name="left">The lft string.</param>
        /// <param name="right">The right string.</param>
        /// <returns>True if both values are not equal, otherwise false.</returns>
        public static bool operator !=(LocalizedString left, LocalizedString right)
        {
            if ((object)left == null)
                return (object)right != null;
            return !left.Equals(right);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is string asString)
                return CompareTo(asString);
            if (obj is LocalizedString asLocalizedString)
                return CompareTo(asLocalizedString);
            return 0;
        }

        /// <inheritdoc />
        public bool Equals(LocalizedString other)
        {
            return (object)other != null && Id == other.Id && Value == other.Value;
        }

        /// <inheritdoc />
        public bool Equals(string other)
        {
            return Value == other || Localization.GetString(Id) == other;
        }

        /// <inheritdoc />
        public int CompareTo(LocalizedString other)
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
            return (object)this == (object)obj || obj is LocalizedString other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return ((Id != null ? Id.GetHashCode() : 0) * 397) ^ (Value != null ? Value.GetHashCode() : 0);
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.IsNullOrEmpty(Value) ? Localization.GetString(Id) : Value;
        }
    }
}

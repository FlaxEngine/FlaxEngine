// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class LocalizedString : IEquatable<LocalizedString>, IEquatable<string>, IComparable, IComparable<LocalizedString>, IComparable<string>
    {
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
            return string.IsNullOrEmpty(Value) ? Localization.GetPluralString(Id, n) : Value;
        }

        /// <summary>
        /// Implicit converter of <see cref="LocalizedString"/> into <see cref="string"/>.
        /// </summary>
        /// <param name="str">The localized string.</param>
        /// <returns>The string.</returns>
        public static implicit operator string(LocalizedString str)
        {
            return str.ToString();
        }

        /// <summary>
        /// Implicit converter of <see cref="string"/> into <see cref="LocalizedString"/>.
        /// </summary>
        /// <param name="str">The string.</param>
        /// <returns>The localized string.</returns>
        public static implicit operator LocalizedString(string str)
        {
            return new LocalizedString(str);
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
            return Id == other.Id && Value == other.Value;
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
            return ReferenceEquals(this, obj) || obj is LocalizedString other && Equals(other);
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

// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Virtual input axis binding. Helps with listening for a selected axis input.
    /// </summary>
    public partial class InputAxis : IComparable, IComparable<InputAxis>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InputAxis"/> class.
        /// </summary>
        /// <param name="name">The axis name.</param>
        public InputAxis(string name)
        {
            Name = name;
        }

        /// <inheritdoc />
        public int CompareTo(InputAxis other)
        {
            return string.Compare(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return obj is InputAxis other ? CompareTo(other) : -1;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Name?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is InputAxis other && string.Equals(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Name;
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Virtual input action binding. Helps with listening for a selected input event.
    /// </summary>
    public partial class InputEvent : IComparable, IComparable<InputEvent>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InputEvent"/> class.
        /// </summary>
        /// <param name="name">The action name.</param>
        public InputEvent(string name)
        {
            Name = name;
        }

        /// <inheritdoc />
        public int CompareTo(InputEvent other)
        {
            return string.Compare(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return obj is InputEvent other ? CompareTo(other) : -1;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Name?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is InputEvent other && string.Equals(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Name;
        }
        
    }
}

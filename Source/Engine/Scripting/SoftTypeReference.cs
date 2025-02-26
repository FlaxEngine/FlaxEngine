// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// The soft reference to the scripting type contained in the scripting assembly.
    /// </summary>
    public struct SoftTypeReference : IComparable, IComparable<SoftTypeReference>
    {
        private string _typeName;

        /// <summary>
        /// Gets or sets the type full name (eg. FlaxEngine.Actor).
        /// </summary>
        public string TypeName
        {
            get => _typeName;
            set => _typeName = value;
        }

        /// <summary>
        /// Gets or sets the type (resolves soft reference).
        /// </summary>
        public Type Type
        {
            get => _typeName != null ? Type.GetType(_typeName) : null;
            set => _typeName = value?.FullName;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SoftTypeReference"/>.
        /// </summary>
        /// <param name="typeName">The type name.</param>
        public SoftTypeReference(string typeName)
        {
            _typeName = typeName;
        }

        /// <summary>
        /// Implicit cast operator from type name to string.
        /// </summary>
        /// <param name="s">The soft type reference.</param>
        /// <returns>The type name.</returns>
        public static implicit operator string(SoftTypeReference s)
        {
            return s._typeName;
        }

        /// <summary>
        /// Gets the soft type reference from full name.
        /// </summary>
        /// <param name="s">The type name.</param>
        /// <returns>The soft type reference.</returns>
        public static implicit operator SoftTypeReference(string s)
        {
            return new SoftTypeReference { _typeName = s };
        }

        /// <summary>
        /// Gets the soft type reference from runtime type.
        /// </summary>
        /// <param name="s">The type.</param>
        /// <returns>The soft type reference.</returns>
        public static implicit operator SoftTypeReference(Type s)
        {
            return new SoftTypeReference { _typeName = s?.FullName };
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return _typeName;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return _typeName?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is SoftTypeReference other)
                return CompareTo(other);
            return 0;
        }

        /// <inheritdoc />
        public int CompareTo(SoftTypeReference other)
        {
            return string.Compare(_typeName, other._typeName, StringComparison.Ordinal);
        }
    }
}

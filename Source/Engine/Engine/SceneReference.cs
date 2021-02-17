// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Represents the reference to the scene asset. Stores the unique ID of the scene to reference. Can be used to load the selected scene.
    /// </summary>
    public struct SceneReference : IComparable, IComparable<Guid>, IComparable<SceneReference>
    {
        /// <summary>
        /// The identifier of the scene asset (and the scene object).
        /// </summary>
        public Guid ID;

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneReference"/> class.
        /// </summary>
        /// <param name="id">The identifier of the scene asset.</param>
        public SceneReference(Guid id)
        {
            ID = id;
        }

        /// <summary>
        /// Compares two values and returns true if are equal or false if unequal.
        /// </summary>
        /// <param name="left">The left value.</param>
        /// <param name="right">The right value.</param>
        /// <returns>True if values are equal, otherwise false.</returns>
        public static bool operator ==(SceneReference left, SceneReference right)
        {
            return left.ID == right.ID;
        }

        /// <summary>
        /// Compares two values and returns false if are equal or true if unequal.
        /// </summary>
        /// <param name="left">The left value.</param>
        /// <param name="right">The right value.</param>
        /// <returns>True if values are not equal, otherwise false.</returns>
        public static bool operator !=(SceneReference left, SceneReference right)
        {
            return left.ID != right.ID;
        }

        /// <summary>
        /// Compares two values and returns true if are equal or false if unequal.
        /// </summary>
        /// <param name="left">The left value.</param>
        /// <param name="right">The right value.</param>
        /// <returns>True if values are equal, otherwise false.</returns>
        public static bool operator ==(SceneReference left, Guid right)
        {
            return left.ID == right;
        }

        /// <summary>
        /// Compares two values and returns false if are equal or true if unequal.
        /// </summary>
        /// <param name="left">The left value.</param>
        /// <param name="right">The right value.</param>
        /// <returns>True if values are not equal, otherwise false.</returns>
        public static bool operator !=(SceneReference left, Guid right)
        {
            return left.ID != right;
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is Guid id)
                return CompareTo(id);
            if (obj is SceneReference other)
                return CompareTo(other);
            return 0;
        }

        /// <inheritdoc />
        public int CompareTo(Guid other)
        {
            return ID.CompareTo(other);
        }

        /// <inheritdoc />
        public int CompareTo(SceneReference other)
        {
            return ID.CompareTo(other.ID);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            if (obj is Guid id)
                return ID == id;
            if (obj is SceneReference other)
                return ID == other.ID;
            return false;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return ID.GetHashCode();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return ID.ToString();
        }
    }
}

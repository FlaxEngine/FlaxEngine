// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Inserts an empty space between controls in the editor.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class SpaceAttribute : Attribute
    {
        /// <summary>
        /// The spacing in pixel (vertically).
        /// </summary>
        public float Height;

        private SpaceAttribute()
        {
            Height = 10.0f;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SpaceAttribute"/> class.
        /// </summary>
        /// <param name="height">The spacing.</param>
        public SpaceAttribute(float height)
        {
            Height = height;
        }
    }
}

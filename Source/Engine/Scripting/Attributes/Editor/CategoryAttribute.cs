// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Describes the category name for a type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface)]
    public sealed class CategoryAttribute : Attribute
    {
        /// <summary>
        /// The category name.
        /// </summary>
        public string Name;

        /// <summary>
        /// Initializes a new instance of the <see cref="CategoryAttribute"/> class.
        /// </summary>
        /// <param name="name">The category name.</param>
        public CategoryAttribute(string name)
        {
            Name = name;
        }
    }
}

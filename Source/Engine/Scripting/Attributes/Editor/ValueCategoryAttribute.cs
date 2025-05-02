// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Specifies the value category of a numeric value as either as-is (a scalar), a distance (formatted as cm/m/km) or an angle (formatted with a degree sign).
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class ValueCategoryAttribute : Attribute
    {
        /// <summary>
        /// The value category used for formatting.
        /// </summary>
        public Utils.ValueCategory Category;

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueCategoryAttribute"/> class.
        /// </summary>
        private ValueCategoryAttribute()
        {
            Category = Utils.ValueCategory.None;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueCategoryAttribute"/> class.
        /// </summary>
        /// <param name="category">The value category.</param>
        public ValueCategoryAttribute(Utils.ValueCategory category)
        {
            Category = category;
        }
    }
}

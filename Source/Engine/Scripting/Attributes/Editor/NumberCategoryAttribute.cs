// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Used to specify the value category of a numeric value as either as-is (a scalar), a distance (formatted as cm/m/km)
    /// or an angle (formatted with a degree sign).
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class NumberCategoryAttribute : Attribute
    {
        /// <summary>
        /// The value category used for formatting.
        /// </summary>
        public Utils.ValueCategory Category;

        private NumberCategoryAttribute()
        {
            Category = Utils.ValueCategory.None;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NumberCategoryAttribute"/> class.
        /// </summary>
        /// <param name="category">The value category.</param>
        public NumberCategoryAttribute(Utils.ValueCategory category)
        {
            Category = category;
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Used to make a float or int variable in a script be restricted to a specific range.
    /// When used, the float or int will be shown as a slider in the editor instead of default number field.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class RangeAttribute : Attribute
    {
        /// <summary>
        /// The minimum range value.
        /// </summary>
        public float Min;

        /// <summary>
        /// The maximum range value.
        /// </summary>
        public float Max;

        private RangeAttribute()
        {
            Min = 0.0f;
            Max = 1.0f;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RangeAttribute"/> class.
        /// </summary>
        /// <param name="min">The minimum range value.</param>
        /// <param name="max">The maximum range value.</param>
        public RangeAttribute(float min, float max)
        {
            Min = min;
            Max = max;
        }
    }
}

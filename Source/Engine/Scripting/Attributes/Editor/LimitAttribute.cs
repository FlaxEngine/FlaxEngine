// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Used to make a float or int variable in a script be restricted to a specific range.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class LimitAttribute : Attribute
    {
        /// <summary>
        /// The minimum range value.
        /// </summary>
        public float Min;

        /// <summary>
        /// The maximum range value.
        /// </summary>
        public float Max;

        /// <summary>
        /// The slider speed used to edit value.
        /// </summary>
        public float SliderSpeed;

        private LimitAttribute()
        {
            Min = 0.0f;
            Max = 100.0f;
            SliderSpeed = 1.0f;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="LimitAttribute"/> class.
        /// </summary>
        /// <param name="min">The minimum limit value.</param>
        /// <param name="max">The maximum limit value.</param>
        /// <param name="sliderSpeed">The slider speed.</param>
        public LimitAttribute(float min, float max = float.MaxValue, float sliderSpeed = 1.0f)
        {
            Min = min;
            Max = max;
            SliderSpeed = sliderSpeed;
        }
    }
}

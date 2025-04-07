// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Indicates that a member of a class cannot be animated by the scene animations system. This class cannot be inherited.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Method)]
    public sealed class NoAnimateAttribute : Attribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="NoAnimateAttribute"/> class.
        /// </summary>
        public NoAnimateAttribute()
        {
        }
    }
}

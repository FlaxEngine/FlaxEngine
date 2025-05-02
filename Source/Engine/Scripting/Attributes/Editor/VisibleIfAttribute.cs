// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Shows property/field in the editor only if the specified member has a given value. Can be used to hide properties based on other properties (also private properties). The given member has to be bool type. Multiple VisibleIf attributes can be added for additional conditions to be met.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = true)]
    public sealed class VisibleIfAttribute : Attribute
    {
        /// <summary>
        /// The member name.
        /// </summary>
        public string MemberName;

        /// <summary>
        /// True if invert member value when computing the visibility value.
        /// </summary>
        public bool Invert;

        private VisibleIfAttribute()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisibleIfAttribute"/> class.
        /// </summary>
        /// <param name="memberName">The name of the field or property of the object. Must be a bool type.</param>
        /// <param name="invert">True if invert member value when computing the visibility value.</param>
        public VisibleIfAttribute(string memberName, bool invert = false)
        {
            MemberName = memberName;
            Invert = invert;
        }
    }
}

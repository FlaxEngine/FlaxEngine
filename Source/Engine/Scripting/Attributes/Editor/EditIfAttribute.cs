// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Toggles property/field editing condition in the editor only if the specified member has a given value. Can be used to disable properties based on other properties (also private properties). The given member has to be bool type. Multiple EditIf attributes can be added for additional conditions to be met.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = true)]
    public sealed class EditIfAttribute : PropertyIfAttribute
    {
        /// <summary>
        /// Inlines the condition member into the checkbox right next to the property label that enables or disables it. Requires condition member to be writable (eg. property setter).
        /// </summary>
        public bool Inline;

        private EditIfAttribute()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="EditIfAttribute"/> class.
        /// </summary>
        /// <param name="memberName">The name of the field or property of the object. Must be a bool type.</param>
        /// <param name="invert">True if invert member value when computing the conditional value.</param>
        /// <param name="inline">True if inline condition member into the checkbox right next to the property label.</param>
        public EditIfAttribute(string memberName, bool invert = false, bool inline = false)
        {
            MemberName = memberName;
            Invert = invert;
            Inline = inline;
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Overrides the default editor provided for the target object/class/field/property. Allows to extend visuals and editing experience of the object.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Struct | AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class CustomEditorAttribute : Attribute
    {
        /// <summary>
        /// Custom editor class type.
        /// Note: if attribute is used on CustomEditor class it specifies object type to edit.
        /// </summary>
        public readonly Type Type;

        /// <summary>
        /// Overrides default editor provided for the target object.
        /// </summary>
        /// <param name="type">The custom editor class type.</param>
        public CustomEditorAttribute(Type type)
        {
            Type = type;
        }
    }
}

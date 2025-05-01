// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Overrides default editor provided for the target object/class/field/property. Allows to extend visuals and editing experience of the objects.
    /// </summary>
    /// <seealso cref="CustomEditorAttribute"/>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Struct | AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class CustomEditorAliasAttribute : Attribute
    {
        /// <summary>
        /// Custom editor class typename.
        /// </summary>
        public readonly string TypeName;

        /// <summary>
        /// Overrides default editor provided for the target object.
        /// </summary>
        /// <param name="typeName">The custom editor class typename.</param>
        public CustomEditorAliasAttribute(string typeName)
        {
            TypeName = typeName;
        }
    }
}

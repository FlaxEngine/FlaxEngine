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
        /// The type containing the singleton instance.
        /// </summary>
        public readonly Type SingletonType;

        /// <summary>
        /// The name of the static property/field that contains the singleton instance.
        /// </summary>
        public readonly string SingletonMemberName;

        /// <summary>
        /// Indicates whether the editor should be treated as a singleton instance.
        /// </summary>
        public bool IsSingleton { get; set; }

        /// <summary>
        /// Overrides default editor provided for the target object.
        /// </summary>
        /// <param name="type">The custom editor class type.</param>
        public CustomEditorAttribute(Type type)
        {
            Type = type;
            IsSingleton = false;
        }

        /// <summary>
        /// Overrides default editor provided for the target object.
        /// </summary>
        /// <param name="type">The custom editor class type.</param>
        /// <param name="isSingleton">True if the editor should be treated as a singleton instance.</param>
        public CustomEditorAttribute(Type type, bool isSingleton)
        {
            Type = type;
            IsSingleton = isSingleton;
        }

        /// <summary>
        /// Overrides default editor provided for the target object using a singleton instance.
        /// </summary>
        /// <param name="singletonType">The type containing the singleton instance.</param>
        /// <param name="memberName">The name of the static property/field containing the singleton.</param>
        public CustomEditorAttribute(Type singletonType, string memberName)
        {
            SingletonType = singletonType;
            SingletonMemberName = memberName;
            IsSingleton = true;
        }
    }
}
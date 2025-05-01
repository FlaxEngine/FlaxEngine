// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Specifies a options for an type reference picker in the editor. Allows to customize view or provide custom value assign policy (eg. restrict types to inherit from a given type).
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Parameter)]
    public class TypeReferenceAttribute : Attribute
    {
        /// <summary>
        /// The full name of the type to link (includes types inheriting from it). Use null or empty to skip it.
        /// </summary>
        public string TypeName;

        /// <summary>
        /// The name of the function (static or member) to invoke to check if the given type is valid to assign. Function must return boolean value and have one argument of type SystemType. Use null or empty to skip it.
        /// </summary>
        public string CheckMethod;

        /// <summary>
        /// Initializes a new instance of the <see cref="TypeReferenceAttribute"/> class.
        /// </summary>
        /// <param name="typeName">The full name of the type to link (includes types inheriting from it). Use null or empty to skip it.</param>
        /// <param name="checkMethod"> The name of the function (static or member) to invoke to check if the given type is valid to assign. Function must return boolean value and have one argument of type SystemType. Use null or empty to skip it.</param>
        public TypeReferenceAttribute(Type typeName, string checkMethod = null)
        {
            TypeName = typeName.FullName;
            CheckMethod = checkMethod;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TypeReferenceAttribute"/> class.
        /// </summary>
        /// <param name="typeName">The full name of the type to link (includes types inheriting from it). Use null or empty to skip it.</param>
        /// <param name="checkMethod"> The name of the function (static or member) to invoke to check if the given type is valid to assign. Function must return boolean value and have one argument of type SystemType. Use null or empty to skip it.</param>
        public TypeReferenceAttribute(string typeName = null, string checkMethod = null)
        {
            TypeName = typeName;
            CheckMethod = checkMethod;
        }
    }
}

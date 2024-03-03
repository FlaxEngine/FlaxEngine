// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Text;

namespace FlaxEngine.Utilities
{
    /// <summary>
    /// Editor utilities and helper functions for System.Type.
    /// </summary>
    public static partial class TypeUtils
    {
        /// <summary>
        /// Gets the typename full name.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The full typename of the type.</returns>
        public static string GetTypeName(this Type type)
        {
            if (type.IsGenericType && type.IsConstructedGenericType)
            {
                // For generic types (eg. Dictionary) FullName returns generic parameter types with fully qualified name so simplify it manually
                var sb = new StringBuilder();
                sb.Append(type.Namespace);
                sb.Append('.');
                sb.Append(type.Name);
                sb.Append('[');
                var genericArgs = type.GetGenericArguments();
                for (var i = 0; i < genericArgs.Length; i++)
                {
                    if (i != 0)
                        sb.Append(',');
                    sb.Append(genericArgs[i].GetTypeName());
                }
                sb.Append(']');
                return sb.ToString();
            }
            return type.FullName;
        }
    }
}

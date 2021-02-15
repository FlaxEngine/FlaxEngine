// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors
{
    internal static class CustomEditorsUtil
    {
        private static readonly StringBuilder CachedSb = new StringBuilder(256);

        internal static readonly Dictionary<Type, string> InBuildTypeNames = new Dictionary<Type, string>()
        {
            { typeof(bool), "bool" },
            { typeof(byte), "byte" },
            { typeof(sbyte), "sbyte" },
            { typeof(char), "char" },
            { typeof(short), "short" },
            { typeof(ushort), "ushort" },
            { typeof(int), "int" },
            { typeof(uint), "uint" },
            { typeof(long), "ulong" },
            { typeof(float), "float" },
            { typeof(double), "double" },
            { typeof(decimal), "decimal" },
            { typeof(string), "string" },
        };

        /// <summary>
        /// Gets the type name for UI. Handles in-build types like System.Single and returns float.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The result.</returns>
        public static string GetTypeNameUI(Type type)
        {
            if (!InBuildTypeNames.TryGetValue(type, out var result))
            {
                result = type.Name;
            }
            return result;
        }

        /// <summary>
        /// Gets the property name for UI. Removes unnecessary characters and filters text. Makes it more user-friendly.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>The result.</returns>
        public static string GetPropertyNameUI(string name)
        {
            int length = name.Length;
            StringBuilder sb = CachedSb;
            sb.Clear();
            sb.EnsureCapacity(length + 8);
            int startIndex = 0;

            // Skip some prefixes
            if (name.StartsWith("g_") || name.StartsWith("m_"))
                startIndex = 2;

            // Filter text
            for (int i = startIndex; i < length; i++)
            {
                var c = name[i];

                // Space before word starting with uppercase letter
                if (char.IsUpper(c) && i > 0)
                {
                    if (i + 1 < length && !char.IsUpper(name[i + 1]))
                        sb.Append(' ');
                }
                // Space instead of underscore
                else if (c == '_')
                {
                    if (sb.Length > 0)
                        sb.Append(' ');
                    continue;
                }
                // Space before digits sequence
                else if (i > 1 && char.IsDigit(c) && !char.IsDigit(name[i - 1]))
                    sb.Append(' ');

                sb.Append(c);
            }

            return sb.ToString();
        }

        internal static CustomEditor CreateEditor(ValueContainer values, CustomEditor overrideEditor, bool canUseRefPicker = true)
        {
            // Check if use provided editor
            if (overrideEditor != null)
                return overrideEditor;

            // Special case if property is a pure object type and all values are the same type
            if (values.Type.Type == typeof(object) && values.Count > 0 && values[0] != null && !values.HasDifferentTypes)
                return CreateEditor(TypeUtils.GetObjectType(values[0]), canUseRefPicker);

            // Use editor for the property type
            return CreateEditor(values.Type, canUseRefPicker);
        }

        internal static CustomEditor CreateEditor(ScriptType targetType, bool canUseRefPicker = true)
        {
            if (targetType.IsArray)
            {
                return new ArrayEditor();
            }
            var targetTypeType = TypeUtils.GetType(targetType);
            if (canUseRefPicker)
            {
                if (typeof(Asset).IsAssignableFrom(targetTypeType))
                {
                    return new AssetRefEditor();
                }
                if (typeof(FlaxEngine.Object).IsAssignableFrom(targetTypeType))
                {
                    return new FlaxObjectRefEditor();
                }
            }

            // Use custom editor
            {
                var checkType = targetTypeType;
                do
                {
                    var type = Internal_GetCustomEditor(checkType);
                    if (type != null)
                    {
                        return (CustomEditor)Activator.CreateInstance(type);
                    }
                    checkType = checkType.BaseType;

                    // Skip if cannot use ref editors
                    if (!canUseRefPicker && checkType == typeof(FlaxEngine.Object))
                        break;
                } while (checkType != null);
            }

            // Use attribute editor
            var attributes = targetType.GetAttributes(false);
            var customEditorAttribute = (CustomEditorAttribute)attributes.FirstOrDefault(x => x is CustomEditorAttribute);
            if (customEditorAttribute != null)
                return (CustomEditor)Activator.CreateInstance(customEditorAttribute.Type);

            // Select default editor (based on type)
            if (targetType.IsEnum)
            {
                return new EnumEditor();
            }
            if (targetType.IsGenericType)
            {
                if (DictionaryEditor.CanEditType(targetTypeType))
                {
                    return new DictionaryEditor();
                }

                // Use custom editor
                var genericTypeDefinition = targetType.GetGenericTypeDefinition();
                var type = Internal_GetCustomEditor(genericTypeDefinition);
                if (type != null)
                {
                    return (CustomEditor)Activator.CreateInstance(type);
                }
            }

            // The most generic editor
            return new GenericEditor();
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Type Internal_GetCustomEditor(Type targetType);
    }
}

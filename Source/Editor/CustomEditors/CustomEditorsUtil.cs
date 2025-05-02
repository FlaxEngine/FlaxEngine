// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using FlaxEditor.CustomEditors.Dedicated;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Interop;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors
{
    internal static partial class CustomEditorsUtil
    {
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

        internal static CustomEditor CreateEditor(ValueContainer values, CustomEditor overrideEditor, bool canUseRefPicker = true)
        {
            // Check if use provided editor
            if (overrideEditor != null)
                return overrideEditor;
            ScriptType targetType = values.Type;

            // Special case if property is a pure object type and all values are the same type
            if (targetType.Type == typeof(object) && values.Count > 0 && values[0] != null && !values.HasDifferentTypes)
                return CreateEditor(TypeUtils.GetObjectType(values[0]), canUseRefPicker);

            // Special case if property is interface but the value is implemented as Scripting Object that should use reference picker
            if (targetType.IsInterface && canUseRefPicker && values.Count > 0 && values[0] is FlaxEngine.Object)
                return new DummyEditor();

            // Use editor for the property type
            return CreateEditor(targetType, canUseRefPicker);
        }

        internal static CustomEditor CreateEditor(ScriptType targetType, bool canUseRefPicker = true)
        {
            if (targetType == ScriptType.Null)
                return new GenericEditor();
            if (targetType.IsArray)
            {
                if (targetType.Type == null)
                    return new ArrayEditor();
                if (targetType.Type.GetArrayRank() != 1)
                    return new GenericEditor(); // Not-supported multidimensional array

                // Allow using custom editor for array of custom type
                var customEditorType = Internal_GetCustomEditor(targetType.Type);
                if (customEditorType != null)
                    return (CustomEditor)Activator.CreateInstance(customEditorType);

                return new ArrayEditor();
            }
            var targetTypeType = TypeUtils.GetType(targetType);
            if (canUseRefPicker)
            {
                if (typeof(Asset).IsAssignableFrom(targetTypeType))
                    return new AssetRefEditor();
                if (typeof(FlaxEngine.Object).IsAssignableFrom(targetTypeType))
                    return new FlaxObjectRefEditor();
            }

            // Use custom editor
            {
                var checkType = targetTypeType;
                do
                {
                    var customEditorType = Internal_GetCustomEditor(checkType);
                    if (customEditorType != null)
                        return (CustomEditor)Activator.CreateInstance(customEditorType);
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
                return new EnumEditor();
            if (targetType.IsGenericType)
            {
                if (targetTypeType.GetGenericTypeDefinition() == typeof(Dictionary<,>))
                    return new DictionaryEditor();

                // Use custom editor
                var genericTypeDefinition = targetType.GetGenericTypeDefinition();
                var customEditorType = Internal_GetCustomEditor(genericTypeDefinition);
                if (customEditorType != null)
                    return (CustomEditor)Activator.CreateInstance(customEditorType);
            }
            if (typeof(FlaxEngine.Object).IsAssignableFrom(targetTypeType))
                return new ScriptingObjectEditor();

            // The most generic editor
            return new GenericEditor();
        }

        [LibraryImport("FlaxEngine", EntryPoint = "CustomEditorsUtilInternal_GetCustomEditor", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalUsing(typeof(SystemTypeMarshaller))]
        internal static partial Type Internal_GetCustomEditor([MarshalUsing(typeof(SystemTypeMarshaller))] Type targetType);
    }
}

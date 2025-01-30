// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using FlaxEditor;
using FlaxEditor.Scripting;

namespace FlaxEngine.Utilities
{
    partial class TypeUtils
    {
        /// <summary>
        /// Custom list of scripting types containers. Can be used by custom scripting languages to provide types info for the editor.
        /// </summary>
        public static List<IScriptTypesContainer> CustomTypes = new List<IScriptTypesContainer>();

        /// <summary>
        /// Gets the script type of the object.
        /// </summary>
        /// <param name="o">The object.</param>
        /// <returns>The scripting type of that object.</returns>
        public static ScriptType GetObjectType(object o)
        {
            if (o is FlaxEngine.Object flaxObject && FlaxEngine.Object.GetUnmanagedPtr(flaxObject) != IntPtr.Zero)
            {
                var type = flaxObject.GetType();
                var typeName = flaxObject.TypeName;
                return type.FullName != typeName ? GetType(typeName) : new ScriptType(type);
            }
            return o != null ? new ScriptType(o.GetType()) : ScriptType.Null;
        }

        /// <summary>
        /// Gets the typename name for UI.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The display of the type.</returns>
        public static string GetTypeDisplayName(this Type type)
        {
            // Special display for in-built basic types
            if (type == null)
                return "Null";
            if (type == typeof(bool))
                return "Bool";
            if (type == typeof(float))
                return "Float";
            if (type == typeof(int))
                return "Int";
            if (type == typeof(uint))
                return "Uint";

            // For generic types (eg. Dictionary) Name returns generic parameter types with fully qualified name so simplify it manually
            if (type.IsGenericType)
            {
                var sb = new StringBuilder();
                var name = type.Name;
                var idx = name.IndexOf('`');
                sb.Append(idx != -1 ? name.Substring(0, idx) : name);
                sb.Append('<');
                var genericArgs = type.GetGenericArguments();
                for (var i = 0; i < genericArgs.Length; i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    sb.Append(genericArgs[i].GetTypeDisplayName());
                }
                sb.Append('>');
                return sb.ToString();
            }

            // Default name
            return type.Name;
        }

        /// <summary>
        /// Gets the default value for the given type (can be value type or reference type).
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The created instance.</returns>
        public static object GetDefaultValue(ScriptType type)
        {
            if (type.Type == typeof(string))
                return string.Empty;
            if (type.Type == typeof(Color))
                return Color.White;
            if (type.Type == typeof(Quaternion))
                return Quaternion.Identity;
            if (type.Type == typeof(Transform))
                return Transform.Identity;
            if (type.Type == typeof(Matrix))
                return Matrix.Identity;
            if (type.Type == typeof(BoundingBox))
                return BoundingBox.Zero;
            if (type.Type == typeof(Rectangle))
                return Rectangle.Empty;
            if (type.Type == typeof(ChannelMask))
                return ChannelMask.Red;
            if (type.Type == typeof(MaterialSceneTextures))
                return MaterialSceneTextures.BaseColor;
            if (type.IsValueType)
            {
                var value = type.CreateInstance();
                FlaxEditor.Utilities.Utils.InitDefaultValues(value);
                return value;
            }
            if (ScriptType.Object.IsAssignableFrom(type))
                return null;
            if (type.CanCreateInstance)
            {
                var value = type.CreateInstance();
                FlaxEditor.Utilities.Utils.InitDefaultValues(value);
                return value;
            }
            return null;
        }

        /// <summary>
        /// Gets all the derived types from the given base type (excluding that type) within the given assembly.
        /// </summary>
        /// <param name="assembly">The target assembly to check its types.</param>
        /// <param name="baseType">The base type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        public static void GetDerivedTypes(Assembly assembly, ScriptType baseType, List<ScriptType> result)
        {
            GetDerivedTypes(assembly, baseType, result, type => true);
        }

        /// <summary>
        /// Gets all the derived types from the given base type (excluding that type) within all the loaded assemblies.
        /// </summary>
        /// <param name="baseType">The base type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        public static void GetDerivedTypes(ScriptType baseType, List<ScriptType> result)
        {
            GetDerivedTypes(baseType, result, type => true, assembly => true);
        }

        /// <summary>
        /// Gets all the derived types from the given base type (excluding that type) within the given assembly.
        /// </summary>
        /// <param name="assembly">The target assembly to check its types.</param>
        /// <param name="baseType">The base type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        public static void GetDerivedTypes(Assembly assembly, ScriptType baseType, List<ScriptType> result, Func<ScriptType, bool> checkFunc)
        {
            var types = assembly.GetTypes();
            for (int i = 0; i < types.Length; i++)
            {
                var t = new ScriptType(types[i]);
                if (baseType.IsAssignableFrom(t) && t != baseType && checkFunc(t))
                    result.Add(t);
            }
        }

        /// <summary>
        /// Gets all the derived types from the given base type (excluding that type) within all the loaded assemblies.
        /// </summary>
        /// <param name="baseType">The base type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        public static void GetDerivedTypes(ScriptType baseType, List<ScriptType> result, Func<ScriptType, bool> checkFunc)
        {
            var assemblies = Utils.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                GetDerivedTypes(assemblies[i], baseType, result, checkFunc);
            }
        }

        /// <summary>
        /// Gets all the derived types from the given base type (excluding that type) within all the loaded assemblies.
        /// </summary>
        /// <param name="baseType">The base type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        /// <param name="checkAssembly">Additional callback used to check if the given assembly is valid. Returns true if search for types in the given assembly, otherwise false.</param>
        public static void GetDerivedTypes(ScriptType baseType, List<ScriptType> result, Func<ScriptType, bool> checkFunc, Func<Assembly, bool> checkAssembly)
        {
            // C#/C++ types
            var assemblies = Utils.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                if (checkAssembly(assemblies[i]))
                    GetDerivedTypes(assemblies[i], baseType, result, checkFunc);
            }

            // Custom types
            foreach (var customTypesInfo in CustomTypes)
                customTypesInfo.GetDerivedTypes(baseType, result, checkFunc);
        }

        /// <summary>
        /// Gets all the types within the given assembly.
        /// </summary>
        /// <param name="assembly">The target assembly to check its types.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        public static void GetTypes(Assembly assembly, List<ScriptType> result, Func<ScriptType, bool> checkFunc)
        {
            var types = assembly.GetTypes();
            for (int i = 0; i < types.Length; i++)
            {
                var t = new ScriptType(types[i]);
                if (checkFunc(t))
                    result.Add(t);
            }
        }

        /// <summary>
        /// Gets all the types from all the loaded assemblies.
        /// </summary>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        /// <param name="checkAssembly">Additional callback used to check if the given assembly is valid. Returns true if search for types in the given assembly, otherwise false.</param>
        public static void GetTypes(List<ScriptType> result, Func<ScriptType, bool> checkFunc, Func<Assembly, bool> checkAssembly)
        {
            // C#/C++ types
            var assemblies = Utils.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                if (checkAssembly(assemblies[i]))
                    GetTypes(assemblies[i], result, checkFunc);
            }

            // Custom types
            foreach (var customTypesInfo in CustomTypes)
                customTypesInfo.GetTypes(result, checkFunc);
        }

        /// <summary>
        /// Gets all the types that have the given attribute defined within the given assembly.
        /// </summary>
        /// <param name="assembly">The target assembly to check its types.</param>
        /// <param name="attributeType">The attribute type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        public static void GetTypesWithAttributeDefined(Assembly assembly, Type attributeType, List<ScriptType> result, Func<ScriptType, bool> checkFunc)
        {
            var types = assembly.GetTypes();
            for (int i = 0; i < types.Length; i++)
            {
                var t = new ScriptType(types[i]);
                if (Attribute.IsDefined(t.Type, attributeType, false) && checkFunc(t))
                    result.Add(t);
            }
        }

        /// <summary>
        /// Gets all the types that have the given attribute defined within all the loaded assemblies.
        /// </summary>
        /// <param name="attributeType">The attribute type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        /// <param name="checkAssembly">Additional callback used to check if the given assembly is valid. Returns true if search for types in the given assembly, otherwise false.</param>
        public static void GetTypesWithAttributeDefined(Type attributeType, List<ScriptType> result, Func<ScriptType, bool> checkFunc, Func<Assembly, bool> checkAssembly)
        {
            var assemblies = Utils.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                if (checkAssembly(assemblies[i]))
                    GetTypesWithAttributeDefined(assemblies[i], attributeType, result, checkFunc);
            }
        }

        /// <summary>
        /// Tries to get the object type from the given full typename. Searches in-build Flax Engine/Editor assemblies and game assemblies.
        /// </summary>
        /// <param name="typeName">The full name of the type.</param>
        /// <returns>The type or null if failed.</returns>
        public static Type GetManagedType(string typeName)
        {
            if (string.IsNullOrEmpty(typeName))
                return null;
            var assemblies = Utils.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                var assembly = assemblies[i];
                if (assembly != null)
                {
                    var type = assembly.GetType(typeName);
                    if (type != null)
                        return type;
                }
            }
            return null;
        }

        /// <summary>
        /// Tries to get the object type from the given full typename. Searches in-build Flax Engine/Editor assemblies and game assemblies.
        /// </summary>
        /// <param name="typeName">The full name of the type.</param>
        /// <returns>The type or null if failed.</returns>
        public static ScriptType GetType(string typeName)
        {
            if (string.IsNullOrEmpty(typeName))
                return ScriptType.Null;

            // C#/C++ types
            {
                var type = Type.GetType(typeName);
                if (type != null)
                    return new ScriptType(type);
            }
            var assemblies = Utils.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                var assembly = assemblies[i];
                if (assembly != null)
                {
                    var type = assembly.GetType(typeName);
                    if (type != null)
                        return new ScriptType(type);
                }
            }

            // Custom types
            foreach (var customTypesInfo in CustomTypes)
            {
                var type = customTypesInfo.GetType(typeName);
                if (type)
                    return type;
            }
            if (typeName.EndsWith("[]"))
            {
                // Array of custom type
                if (typeName[0] == '.')
                    typeName = typeName.Substring(1);
                typeName = typeName.Substring(0, typeName.Length - 2);
                foreach (var customTypesInfo in CustomTypes)
                {
                    var type = customTypesInfo.GetType(typeName);
                    if (type)
                        return type.MakeArrayType();
                }
            }

            if (!FlaxEditor.Content.Settings.GameSettings.OptionalPlatformSettings.Contains(typeName))
                Editor.LogWarning($"Failed to find type '{typeName}'.");
            return ScriptType.Null;
        }

        /// <summary>
        /// Tries to create object instance of the given full typename. Searches in-build Flax Engine/Editor assemblies and game assemblies.
        /// </summary>
        /// <param name="typeName">The full name of the type.</param>
        /// <returns>The created object or null if failed.</returns>
        public static object CreateInstance(string typeName)
        {
            object obj = null;
            ScriptType type = GetType(typeName);
            if (type && type.CanCreateInstance)
            {
                try
                {
                    return obj = type.CreateInstance();
                }
                catch (Exception ex)
                {
                    Debug.LogException(ex);
                }
            }
            return obj;
        }

        /// <summary>
        /// Creates a one-dimensional <see cref="T:System.Array" /> of the specified type and length.
        /// </summary>
        /// <param name="elementType">The type of the array to create.</param>
        /// <param name="size">The length of the array to create.</param>
        /// <returns>The created object or null if failed.</returns>
        public static Array CreateArrayInstance(ScriptType elementType, int size)
        {
            return Array.CreateInstance(GetType(elementType), size);
        }

        /// <summary>
        /// Checks if the input type represents a structure (value type but not enum nor primitive type).
        /// </summary>
        /// <param name="type">The input type of the object to check.</param>
        /// <returns>Returns true if the input type represents a structure (value type but not enum nor primitive type).</returns>
        public static bool IsStructure(this Type type)
        {
            return type.IsValueType && !type.IsEnum && !type.IsPrimitive;
        }

        /// <summary>
        /// Checks if the input type represents a delegate.
        /// </summary>
        /// <param name="type">The input type of the object to check.</param>
        /// <returns>Returns true if the input type represents a delegate.</returns>
        public static bool IsDelegate(this Type type)
        {
            return typeof(MulticastDelegate).IsAssignableFrom(type.BaseType);
        }

        /// <summary>
        /// Gets the managed type fo the script type (the first found from this or base types).
        /// </summary>
        /// <param name="type">The input type of the object to check.</param>
        /// <returns>The managed type.</returns>
        public static Type GetType(ScriptType type)
        {
            if (type == ScriptType.Null)
                return null;
            while (type.Type == null)
                type = type.BaseType;
            return type.Type;
        }
    }
}

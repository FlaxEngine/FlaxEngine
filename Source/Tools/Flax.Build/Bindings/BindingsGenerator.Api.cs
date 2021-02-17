// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using BuildData = Flax.Build.Builder.BuildData;

namespace Flax.Build.Bindings
{
    partial class BindingsGenerator
    {
        private static class ApiTokens
        {
            public const string Enum = "API_ENUM";
            public const string Class = "API_CLASS";
            public const string Struct = "API_STRUCT";
            public const string Interface = "API_INTERFACE";
            public const string Function = "API_FUNCTION";
            public const string Property = "API_PROPERTY";
            public const string Field = "API_FIELD";
            public const string Event = "API_EVENT";
            public const string Param = "API_PARAM";
            public const string InjectCppCode = "API_INJECT_CPP_CODE";
            public const string AutoSerialization = "API_AUTO_SERIALIZATION";

            public static readonly string[] SearchTags =
            {
                Enum,
                Class,
                Struct,
                Interface,
            };
        }

        public static readonly Dictionary<TypeInfo, ApiTypeInfo> InBuildTypes = new Dictionary<TypeInfo, ApiTypeInfo>
        {
            {
                new TypeInfo
                {
                    Type = "void",
                },
                new LangType("void")
            },
            {
                new TypeInfo
                {
                    Type = "bool",
                },
                new LangType("bool")
            },
            {
                new TypeInfo
                {
                    Type = "byte",
                },
                new LangType("byte")
            },
            {
                new TypeInfo
                {
                    Type = "int8",
                },
                new LangType("int8")
            },
            {
                new TypeInfo
                {
                    Type = "int16",
                },
                new LangType("int16")
            },
            {
                new TypeInfo
                {
                    Type = "int32",
                },
                new LangType("int32")
            },
            {
                new TypeInfo
                {
                    Type = "int64",
                },
                new LangType("int64")
            },
            {
                new TypeInfo
                {
                    Type = "uint8",
                },
                new LangType("uint8")
            },
            {
                new TypeInfo
                {
                    Type = "uint16",
                },
                new LangType("uint16")
            },
            {
                new TypeInfo
                {
                    Type = "uint32",
                },
                new LangType("uint32")
            },
            {
                new TypeInfo
                {
                    Type = "uint64",
                },
                new LangType("uint64")
            },
            {
                new TypeInfo
                {
                    Type = "float",
                },
                new LangType("float")
            },
            {
                new TypeInfo
                {
                    Type = "double",
                },
                new LangType("double")
            },
            {
                new TypeInfo
                {
                    Type = "Char",
                },
                new LangType("char")
            },
            {
                new TypeInfo
                {
                    Type = "char",
                },
                new LangType("sbyte")
            },
            {
                new TypeInfo
                {
                    Type = "void*",
                },
                new LangType("IntPtr")
            },
        };

        private static readonly string[] InBuildRefTypes =
        {
            // Math library types
            "Vector2",
            "Vector3",
            "Vector4",
            "Int2",
            "Int3",
            "Int4",
            "Color",
            "Color32",
            "ColorHSV",
            "BoundingBox",
            "BoundingSphere",
            "BoundingFrustum",
            "Transform",
            "Rectangle",
            "Matrix",
            "Matrix3x3",
            "Matrix2x2",
            "Plane",
            "OrientedBoundingBox",
            "Quaternion",
            "Ray",
            "Viewport",
            "Guid",
            "Half",
            "Half2",
            "Half3",
            "Half4",
            "FloatR11G11B10",
            "FloatR10G10B10A2",

            // Engine API types
            "RenderView",
            "CreateWindowSettings",
            "TextLayoutOptions",
            "MaterialInfo",
            "D6JointDrive",
        };

        /// <summary>
        /// Checks if use reference when passing values of this type to via scripting API methods.
        /// </summary>
        /// <param name="buildData">The build data.</param>
        /// <param name="typeInfo">The value type information.</param>
        /// <param name="caller">The calling type. It's parent module types and references are used to find the given API type.</param>
        /// <returns>True if use reference when passing the values of this type, otherwise false.</returns>
        public static bool UsePassByReference(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller)
        {
            // Skip for pointers
            if (typeInfo.IsPtr)
                return false;

            // Skip for strings
            if ((typeInfo.Type == "String" || typeInfo.Type == "StringView" || typeInfo.Type == "StringAnsi" || typeInfo.Type == "StringAnsiView") && typeInfo.GenericArgs == null)
                return false;

            // Skip for collections
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span" || typeInfo.Type == "Dictionary" || typeInfo.Type == "HashSet") && typeInfo.GenericArgs != null)
                return false;

            // Skip for BytesContainer
            if (typeInfo.Type == "BytesContainer" && typeInfo.GenericArgs == null)
                return false;

            // Skip for Variant
            if (typeInfo.Type == "Variant" && typeInfo.GenericArgs == null)
                return false;

            // Skip for VariantType
            if (typeInfo.Type == "VariantType" && typeInfo.GenericArgs == null)
                return false;

            // Find API type info
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                // Skip for scripting objects
                if (apiType.IsScriptingObject)
                    return false;

                // Skip for classes
                if (apiType.IsClass)
                    return false;

                // Force for structures
                if (apiType.IsStruct)
                    return true;
            }

            // True for references
            if (typeInfo.IsRef)
                return true;

            // Force for in-build types
            if (InBuildRefTypes.Contains(typeInfo.Type))
                return true;

            return false;
        }

        /// <summary>
        /// Finds the API type information.
        /// </summary>
        /// <param name="buildData">The build data.</param>
        /// <param name="typeInfo">The type information.</param>
        /// <param name="caller">The calling type. It's parent module types and references are used to find the given API type.</param>
        /// <returns>The found API type inf oor null.</returns>
        public static ApiTypeInfo FindApiTypeInfo(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller)
        {
            if (typeInfo == null)
                return null;
            if (buildData.TypeCache.TryGetValue(typeInfo, out var result))
                return result;

            // Find across in-build types
            if (InBuildTypes.TryGetValue(typeInfo, out result))
                return result;

            // Find across all loaded modules for this build
            foreach (var e in buildData.ModulesInfo)
            {
                result = FindApiTypeInfoInner(typeInfo, e.Value);
                if (result != null)
                {
                    buildData.TypeCache.Add(typeInfo, result);
                    return result;
                }
            }

            // Check if using nested typename
            if (typeInfo.Type.Contains("::"))
            {
                var nesting = typeInfo.Type.Split(new[] { "::" }, StringSplitOptions.None);
                result = FindApiTypeInfo(buildData, new TypeInfo { Type = nesting[0], }, caller);
                for (int i = 1; i < nesting.Length; i++)
                {
                    if (result == null)
                        return null;
                    result = FindApiTypeInfoInner(new TypeInfo { Type = nesting[i], }, result);
                }
                return result;
            }

            //buildData.TypeCache.Add(typeInfo, null);
            //Log.Warning("Unknown API type: " + typeInfo);
            return null;
        }

        private static ApiTypeInfo FindApiTypeInfoInner(TypeInfo typeInfo, ApiTypeInfo parent)
        {
            foreach (var child in parent.Children)
            {
                if (child.Name == typeInfo.Type)
                    return child;

                var result = FindApiTypeInfoInner(typeInfo, child);
                if (result != null)
                    return result;
            }

            return null;
        }
    }
}

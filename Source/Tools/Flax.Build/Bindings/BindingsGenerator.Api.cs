// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
            public const string Typedef = "API_TYPEDEF";
            public const string InjectCode = "API_INJECT_CODE";
            public const string AutoSerialization = "API_AUTO_SERIALIZATION";

            public static readonly string[] SearchTags =
            {
                Enum,
                Class,
                Struct,
                Interface,
                Typedef,
            };
        }

        public static readonly Dictionary<TypeInfo, ApiTypeInfo> InBuildTypes = new Dictionary<TypeInfo, ApiTypeInfo>
        {
            {
                new TypeInfo("void"),
                new LangType("void")
            },
            {
                new TypeInfo("bool"),
                new LangType("bool")
            },
            {
                new TypeInfo("byte"),
                new LangType("byte")
            },
            {
                new TypeInfo("int8"),
                new LangType("int8")
            },
            {
                new TypeInfo("int16"),
                new LangType("int16")
            },
            {
                new TypeInfo("int32"),
                new LangType("int32")
            },
            {
                new TypeInfo("int64"),
                new LangType("int64")
            },
            {
                new TypeInfo("uint8"),
                new LangType("uint8")
            },
            {
                new TypeInfo("uint16"),
                new LangType("uint16")
            },
            {
                new TypeInfo("uint32"),
                new LangType("uint32")
            },
            {
                new TypeInfo("uint64"),
                new LangType("uint64")
            },
            {
                new TypeInfo("float"),
                new LangType("float")
            },
            {
                new TypeInfo("double"),
                new LangType("double")
            },
            {
                new TypeInfo("Char"),
                new LangType("char", "Char")
            },
            {
                new TypeInfo("char"),
                new LangType("sbyte", "char")
            },
            {
                new TypeInfo("void*"),
                new LangType("IntPtr", "void*")
            },
        };

        private static readonly string[] InBuildRefTypes =
        {
            // Math library types
            "Vector2",
            "Vector3",
            "Vector4",
            "Float2",
            "Float3",
            "Float4",
            "Double2",
            "Double3",
            "Double4",
            "Int2",
            "Int3",
            "Int4",
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
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span" || typeInfo.Type == "DataContainer" || typeInfo.Type == "Dictionary" || typeInfo.Type == "HashSet") && typeInfo.GenericArgs != null)
                return false;

            // Skip for special types
            if (typeInfo.GenericArgs == null)
            {
                if (typeInfo.Type == "BytesContainer")
                    return false;
                if (typeInfo.Type == "Variant")
                    return false;
                if (typeInfo.Type == "VariantType")
                    return false;
                if (typeInfo.Type == "ScriptingTypeHandle")
                    return false;
            }
            else
            {
                if (typeInfo.Type == "Function")
                    return false;
            }

            // Find API type info
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                if (apiType.MarshalAs != null)
                    return UsePassByReference(buildData, apiType.MarshalAs, caller);

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

#if USE_NETCORE
        /// <summary>
        /// Check if structure contains unblittable types that would require custom marshaller for the structure.
        /// </summary>
        public static bool UseCustomMarshalling(BuildData buildData, StructureInfo structureInfo, ApiTypeInfo caller)
        {
            if (structureInfo.Fields.Any(x => !x.IsStatic &&
                                         (x.Type.IsObjectRef || x.Type.Type == "Dictionary" || x.Type.Type == "Version")
                                         && x.Type.Type != "uint8" && x.Type.Type != "byte"))
            {
                return true;
            }

            foreach (var field in structureInfo.Fields)
            {
                if (field.Type.Type == structureInfo.FullNameNative)
                    continue;
                if (field.IsStatic)
                    continue;

                if (field.Type.Type == "String")
                    return true;

                var fieldApiType = FindApiTypeInfo(buildData, field.Type, caller);
                if (fieldApiType is StructureInfo fieldStructureInfo && UseCustomMarshalling(buildData, fieldStructureInfo, caller))
                    return true;
                else if (fieldApiType is ClassInfo)
                    return true;
            }

            return false;
        }
#endif

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
            var result = FindApiTypeInfoInner(buildData, typeInfo, caller);
            if (result != null)
                return result;
            if (buildData.TypeCache.TryGetValue(typeInfo, out result))
                return result;

            // Find across in-build types
            if (InBuildTypes.TryGetValue(typeInfo, out result))
                return result;
            if (typeInfo.IsRef)
            {
                typeInfo.IsRef = false;
                if (InBuildTypes.TryGetValue(typeInfo, out result))
                {
                    typeInfo.IsRef = true;
                    return result;
                }
                typeInfo.IsRef = true;
            }

            // Find across all loaded modules for this build
            foreach (var e in buildData.ModulesInfo)
            {
                result = FindApiTypeInfoInner(buildData, typeInfo, e.Value);
                if (result != null)
                {
                    buildData.TypeCache[typeInfo] = result;
                    return result;
                }
            }

            // Check if using nested typename
            if (typeInfo.Type.Contains("::"))
            {
                var nesting = typeInfo.Type.Split(new[] { "::" }, StringSplitOptions.None);
                result = FindApiTypeInfo(buildData, new TypeInfo(nesting[0]), caller);
                for (int i = 1; i < nesting.Length; i++)
                {
                    if (result == null)
                        return null;
                    result = FindApiTypeInfoInner(buildData, new TypeInfo(nesting[i]), result);
                }
                return result;
            }

            //buildData.TypeCache.Add(typeInfo, null);
            //Log.Warning("Unknown API type: " + typeInfo);
            return null;
        }

        private static ApiTypeInfo FindApiTypeInfoInner(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo parent)
        {
            ApiTypeInfo result = null;
            foreach (var child in parent.Children)
            {
                if (child.Name == typeInfo.Type)
                {
                    result = child;

                    // Special case when searching for template types (use instantiated template if input type has generic arguments)
                    if (child is ClassStructInfo classStructInfo && classStructInfo.IsTemplate && typeInfo.GenericArgs != null)
                    {
                        // Support instantiated template type with instantiated arguments (eg. 'typedef Vector3Base<Real> Vector3' where 'typedef float Real')
                        var inflatedType = typeInfo.Inflate(buildData, child);

                        // Find any typedef which introduced this type (eg. 'typedef Vector3Base<float> Float3')
                        result = FindTypedef(buildData, inflatedType, parent.ParentModule);
                        if (result == TypedefInfo.Current)
                            result = child; // Use template type for the current typedef
                        else if (result == null)
                            continue; // Invalid template match
                    }

                    result.EnsureInited(buildData);
                    if (result is TypedefInfo typedef)
                        result = typedef.Typedef;
                    break;
                }

                result = FindApiTypeInfoInner(buildData, typeInfo, child);
                if (result != null)
                    break;
            }
            return result;
        }

        private static ApiTypeInfo FindTypedef(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo parent)
        {
            ApiTypeInfo result = null;
            foreach (var child in parent.Children)
            {
                if (child is TypedefInfo typedef && typedef.Type.Equals(typeInfo))
                {
                    result = child;
                    break;
                }

                result = FindTypedef(buildData, typeInfo, child);
                if (result != null)
                    break;
            }
            return result;
        }
    }
}

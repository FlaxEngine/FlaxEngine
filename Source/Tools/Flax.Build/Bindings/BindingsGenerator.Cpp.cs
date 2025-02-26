// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using BuildData = Flax.Build.Builder.BuildData;

namespace Flax.Build.Bindings
{
    partial class BindingsGenerator
    {
        private static readonly bool[] CppParamsThatNeedLocalVariable = new bool[64];
        private static readonly bool[] CppParamsThatNeedConversion = new bool[64];
        private static readonly string[] CppParamsThatNeedConversionWrappers = new string[64];
        private static readonly string[] CppParamsThatNeedConversionTypes = new string[64];
        private static readonly string[] CppParamsWrappersCache = new string[64];
        public static readonly List<KeyValuePair<string, string>> CppInternalCalls = new List<KeyValuePair<string, string>>();
        public static readonly List<ApiTypeInfo> CppUsedNonPodTypes = new List<ApiTypeInfo>();
        private static readonly List<ApiTypeInfo> CppUsedNonPodTypesList = new List<ApiTypeInfo>();
        public static readonly HashSet<FileInfo> CppReferencesFiles = new HashSet<FileInfo>();
        private static readonly List<FieldInfo> CppAutoSerializeFields = new List<FieldInfo>();
        private static readonly List<PropertyInfo> CppAutoSerializeProperties = new List<PropertyInfo>();
        public static readonly HashSet<string> CppIncludeFiles = new HashSet<string>();
        private static readonly List<string> CppIncludeFilesList = new List<string>();
        private static readonly Dictionary<string, TypeInfo> CppVariantToTypes = new Dictionary<string, TypeInfo>();
        private static readonly Dictionary<string, TypeInfo> CppVariantFromTypes = new Dictionary<string, TypeInfo>();
        private static bool CppNonPodTypesConvertingGeneration = false;
        private static StringBuilder CppContentsEnd;

        public class ScriptingLangInfo
        {
            public bool Enabled;
            public string VirtualWrapperMethodsPostfix;
        }

        public static readonly List<ScriptingLangInfo> ScriptingLangInfos = new List<ScriptingLangInfo>
        {
            new ScriptingLangInfo
            {
                Enabled = true,
                VirtualWrapperMethodsPostfix = "_ManagedWrapper", // C#
            },
        };

        public static event Action<BuildData, IGrouping<string, Module>, StringBuilder> GenerateCppBinaryModuleHeader;
        public static event Action<BuildData, IGrouping<string, Module>, StringBuilder> GenerateCppBinaryModuleSource;
        public static event Action<BuildData, ModuleInfo, StringBuilder> GenerateCppModuleSource;
        public static event Action<BuildData, ApiTypeInfo, StringBuilder> GenerateCppTypeInternalsStatics;
        public static event Action<BuildData, ApiTypeInfo, StringBuilder> GenerateCppTypeInternals;
        public static event Action<BuildData, ApiTypeInfo, StringBuilder> GenerateCppTypeInitRuntime;
        public static event Action<BuildData, VirtualClassInfo, FunctionInfo, int, int, StringBuilder> GenerateCppScriptWrapperFunction;

        private static readonly List<string> CppInBuildVariantStructures = new List<string>
        {
            "Vector2",
            "Vector3",
            "Vector4",
            "Float2",
            "Float3",
            "Float4",
            "Double2",
            "Double2",
            "Double3",
            "Int2",
            "Int3",
            "Int4",
            "Color",
            "Quaternion",
            "Guid",
            "BoundingSphere",
            "BoundingBox",
            "Transform",
            "Matrix",
            "Ray",
            "Rectangle",
        };

        private static bool GenerateCppIsTemplateInstantiationType(ApiTypeInfo typeInfo)
        {
            return typeInfo.Instigator != null && typeInfo.Instigator.TypeInfo is ClassStructInfo classStructInfo && classStructInfo.IsTemplate;
        }

        private static string GenerateCppWrapperNativeToVariantMethodName(TypeInfo typeInfo)
        {
            var sb = GetStringBuilder();
            sb.Append(typeInfo.Type.Replace("::", "_"));
            if (typeInfo.IsPtr)
                sb.Append("Ptr");
            if (typeInfo.GenericArgs != null)
            {
                foreach (var arg in typeInfo.GenericArgs)
                {
                    sb.Append(arg.Type.Replace("::", "_"));
                    if (arg.IsPtr)
                        sb.Append("Ptr");
                }
            }
            var result = sb.ToString();
            PutStringBuilder(sb);
            return result;
        }

        private static string GenerateCppWrapperNativeToManagedParam(BuildData buildData, StringBuilder contents, TypeInfo paramType, string paramName, ApiTypeInfo caller, bool isRef, out bool useLocalVar)
        {
            useLocalVar = false;
            var nativeToManaged = GenerateCppWrapperNativeToManaged(buildData, paramType, caller, out var managedTypeAsNative, null);
            string result;
            if (!string.IsNullOrEmpty(nativeToManaged))
            {
                result = string.Format(nativeToManaged, paramName);
                if (managedTypeAsNative[managedTypeAsNative.Length - 1] == '*' && !isRef)
                {
                    // Pass pointer value
                }
                else
                {
                    // Pass as pointer to local variable converted for managed runtime
                    if (paramType.IsPtr)
                        result = string.Format(nativeToManaged, '*' + paramName);
                    contents.Append($"        auto __param_{paramName} = {result};").AppendLine();
                    result = $"&__param_{paramName}";
                    useLocalVar = true;
                }
            }
            else
            {
                result = paramName;
                if (paramType.IsRef && !paramType.IsConst && !isRef)
                {
                    // Pass reference as a pointer
                    result = '&' + result;
                }
                else if (paramType.IsPtr || managedTypeAsNative[managedTypeAsNative.Length - 1] == '*')
                {
                    // Pass pointer value
                }
                else
                {
                    // Pass as pointer to value
                    result = '&' + result;
                }
            }
            return $"(void*){result}";
        }


        private static void GenerateCppAddFileReference(BuildData buildData, ApiTypeInfo caller, TypeInfo typeInfo, ApiTypeInfo apiType)
        {
            CppReferencesFiles.Add(apiType?.File);
            if (typeInfo.GenericArgs != null)
            {
                for (int i = 0; i < typeInfo.GenericArgs.Count; i++)
                {
                    var g = typeInfo.GenericArgs[i];
                    GenerateCppAddFileReference(buildData, caller, g, FindApiTypeInfo(buildData, g, caller));
                }
            }
        }

        public static string GenerateCppWrapperNativeToVariant(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, string value)
        {
            if (typeInfo.Type == "Variant")
                return value;
            if (typeInfo.Type == "String")
                return $"Variant(StringView({value}))";
            if (typeInfo.Type == "StringAnsi")
                return $"Variant(StringAnsiView({value}))";
            if (typeInfo.IsObjectRef)
                return $"Variant({value}.Get())";
            if (typeInfo.Type == "SoftTypeReference")
                return $"Variant::Typename(StringAnsiView({value}))";
            if (typeInfo.IsArray)
            {
                var wrapperName = GenerateCppWrapperNativeToVariantMethodName(typeInfo);
                CppVariantFromTypes[wrapperName] = typeInfo;
                return $"VariantFrom{GenerateCppWrapperNativeToVariantMethodName(typeInfo)}Array((const {typeInfo}*){value}, {typeInfo.ArraySize})";
            }
            if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
            {
                var wrapperName = GenerateCppWrapperNativeToVariantMethodName(typeInfo.GenericArgs[0]);
                CppVariantFromTypes[wrapperName] = typeInfo;
                return $"VariantFrom{wrapperName}Array((const {typeInfo.GenericArgs[0]}*){value}.Get(), {value}.Count())";
            }
            if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
            {
                var wrapperName = GenerateCppWrapperNativeToVariantMethodName(typeInfo.GenericArgs[0]) + GenerateCppWrapperNativeToVariantMethodName(typeInfo.GenericArgs[1]);
                CppVariantFromTypes[wrapperName] = typeInfo;
                return $"VariantFrom{wrapperName}Dictionary({value})";
            }
            if (typeInfo.Type == "Span" && typeInfo.GenericArgs != null)
            {
                return "Variant()"; // TODO: Span to Variant converting (use utility method the same way as for arrays)
            }

            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                CppReferencesFiles.Add(apiType.File);
                if (apiType.IsStruct && !apiType.IsPod && !CppUsedNonPodTypes.Contains(apiType))
                    CppUsedNonPodTypes.Add(apiType);

                if (apiType.IsEnum)
                    return $"Variant::Enum(VariantType(VariantType::Enum, StringAnsiView(\"{apiType.FullNameManaged}\", {apiType.FullNameManaged.Length})), {value})";
                if (apiType.IsStruct && !CppInBuildVariantStructures.Contains(apiType.Name))
                    if (typeInfo.IsPtr)
                        return $"Variant::Structure(VariantType(VariantType::Structure, StringAnsiView(\"{apiType.FullNameManaged}\", {apiType.FullNameManaged.Length})), *{value})";
                    else
                        return $"Variant::Structure(VariantType(VariantType::Structure, StringAnsiView(\"{apiType.FullNameManaged}\", {apiType.FullNameManaged.Length})), {value})";
            }

            if (typeInfo.IsPtr && typeInfo.IsConst)
                return $"Variant(({typeInfo.Type}*){value})";
            return $"Variant({value})";
        }

        public static string GenerateCppWrapperVariantToNative(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, string value)
        {
            if (typeInfo.Type == "Variant")
                return value;
            if (typeInfo.Type == "String")
                return $"(StringView){value}";
            if (typeInfo.Type == "StringAnsi")
                return $"(StringAnsiView){value}";
            if (typeInfo.IsPtr && typeInfo.IsConst && typeInfo.Type == "Char")
                return $"((StringView){value}).GetText()"; // (StringView)Variant, if not empty, is guaranteed to point to a null-terminated buffer.
            if (typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "SoftObjectReference")
                return $"ScriptingObject::Cast<{typeInfo.GenericArgs[0].Type}>((ScriptingObject*){value})";
            if (typeInfo.IsObjectRef)
                return $"ScriptingObject::Cast<{typeInfo.GenericArgs[0].Type}>((Asset*){value})";
            if (typeInfo.Type == "SoftTypeReference")
                return $"(StringAnsiView){value}";
            if (typeInfo.IsArray)
                throw new Exception($"Not supported type to convert from the Variant to fixed-size array '{typeInfo}[{typeInfo.ArraySize}]'.");
            if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
            {
                var wrapperName = GenerateCppWrapperNativeToVariantMethodName(typeInfo);
                CppVariantToTypes[wrapperName] = typeInfo;
                return $"MoveTemp(VariantTo{wrapperName}({value}))";
            }
            if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
            {
                var wrapperName = GenerateCppWrapperNativeToVariantMethodName(typeInfo);
                CppVariantToTypes[wrapperName] = typeInfo;
                return $"MoveTemp(VariantTo{wrapperName}({value}))";
            }
            if (typeInfo.Type == "Span" && typeInfo.GenericArgs != null)
            {
                return $"{typeInfo}()"; // Cannot be implemented since Variant stores array of Variants thus cannot get linear span of data
            }

            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                CppReferencesFiles.Add(apiType.File);
                if (apiType.IsStruct && !apiType.IsPod && !CppUsedNonPodTypes.Contains(apiType))
                    CppUsedNonPodTypes.Add(apiType);

                if (apiType.IsEnum)
                    return $"({apiType.FullNameNative})(uint64){value}";
                if (apiType.IsScriptingObject)
                    return $"ScriptingObject::Cast<{typeInfo.Type}>((ScriptingObject*){value})";
                if (apiType.IsStruct && !CppInBuildVariantStructures.Contains(apiType.Name))
                {
                    var name = apiType.FullNameNative;
                    if (typeInfo.GenericArgs != null)
                    {
                        name += '<';
                        for (var i = 0; i < typeInfo.GenericArgs.Count; i++)
                        {
                            if (i != 0)
                                name += ", ";
                            name += typeInfo.GenericArgs[i];
                        }
                        name += '>';
                    }
                    if (typeInfo.IsPtr)
                        return $"({name}*){value}.AsBlob.Data";
                    else
                        return $"*({name}*){value}.AsBlob.Data";
                }
            }

            if (typeInfo.IsPtr)
                return $"({typeInfo})(void*){value}";
            return $"({typeInfo}){value}";
        }

        public static void GenerateCppVirtualWrapperCallBaseMethod(BuildData buildData, StringBuilder contents, VirtualClassInfo classInfo, FunctionInfo functionInfo, string scriptVTableBase, string scriptVTableOffset)
        {
            contents.AppendLine("            // Prevent stack overflow by calling native base method");
            if (buildData.Toolchain?.Compiler == TargetCompiler.Clang)
            {
                // Clang compiler
                // TODO: secure VTableFunctionInjector with mutex (even at cost of performance)
                contents.AppendLine($"            {functionInfo.UniqueName}_Signature funcPtr = &{classInfo.NativeName}::{functionInfo.Name};");
                contents.AppendLine($"            VTableFunctionInjector vtableInjector(object, *(void**)&funcPtr, {scriptVTableBase}[{scriptVTableOffset} + 2]); // TODO: this is not thread-safe");
                if (classInfo is InterfaceInfo)
                {
                    contents.Append($"            return (({classInfo.NativeName}*)(void*)object)->{functionInfo.Name}(");
                }
                else
                {
                    contents.Append("            return (object->*funcPtr)(");
                }
            }
            else
            {
                // MSVC or other compiler
                contents.Append($"            return (this->**({functionInfo.UniqueName}_Internal_Signature*)&{scriptVTableBase}[{scriptVTableOffset} + 2])(");
            }
            bool separator = false;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;
                contents.Append(parameterInfo.Name);
            }
            contents.AppendLine(");");
        }

        private static string GenerateCppGetMClass(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, FunctionInfo functionInfo)
        {
            // Optimal path for in-build types
            var managedType = GenerateCSharpNativeToManaged(buildData, typeInfo, caller, true);
            switch (managedType)
            {
            // In-built types (cached by the engine on startup)
            case "bool": return "MCore::TypeCache::Boolean";
            case "sbyte": return "MCore::TypeCache::SByte";
            case "byte": return "MCore::TypeCache::Byte";
            case "short": return "MCore::TypeCache::Int16";
            case "ushort": return "MCore::TypeCache::UInt16";
            case "int": return "MCore::TypeCache::Int32";
            case "uint": return "MCore::TypeCache::UInt32";
            case "long": return "MCore::TypeCache::Int64";
            case "ulong": return "MCore::TypeCache::UInt64";
            case "float": return "MCore::TypeCache::Single";
            case "double": return "MCore::TypeCache::Double";
            case "string": return "MCore::TypeCache::String";
            case "object": return "MCore::TypeCache::Object";
            case "void": return "MCore::TypeCache::Void";
            case "char": return "MCore::TypeCache::Char";
            case "IntPtr": return "MCore::TypeCache::IntPtr";
            case "UIntPtr": return "MCore::TypeCache::UIntPtr";

            // Vector2/3/4 have custom type in C# (due to lack of typename using in older C#)
            case "Vector2": return "Scripting::FindClass(\"FlaxEngine.Vector2\")";
            case "Vector3": return "Scripting::FindClass(\"FlaxEngine.Vector3\")";
            case "Vector4": return "Scripting::FindClass(\"FlaxEngine.Vector4\")";
            }

            // Find API type
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                CppReferencesFiles.Add(apiType.File);
                if (apiType.IsStruct && !apiType.IsPod && !CppUsedNonPodTypes.Contains(apiType))
                    CppUsedNonPodTypes.Add(apiType);
                if (!apiType.SkipGeneration && !apiType.IsEnum)
                {
                    // Use declared type initializer
                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                    return $"{apiType.FullNameNative}::TypeInitializer.GetClass()";
                }
            }

            // Pass it from C# in glue parameter if used inside the wrapper function
            if (functionInfo != null)
            {
                var customParam = new FunctionInfo.ParameterInfo
                {
                    Name = "resultArrayItemType" + functionInfo.Glue.CustomParameters.Count,
                    DefaultValue = "typeof(" + managedType + ')',
                    Type = new TypeInfo
                    {
                        Type = "MTypeObject",
                        IsPtr = true,
                    },
                };
                functionInfo.Glue.CustomParameters.Add(customParam);
                return "MUtils::GetClass(" + customParam.Name + ")";
            }

            // Find namespace for this type to build a fullname
            if (apiType != null)
            {
                var e = apiType.Parent;
                while (!(e is FileInfo))
                {
                    e = e.Parent;
                }
                if (e is FileInfo fileInfo && !managedType.StartsWith(fileInfo.Namespace))
                {
                    managedType = fileInfo.Namespace + '.' + managedType.Replace(".", "+");
                }
            }

            // Use runtime lookup from fullname of the C# class
            return "Scripting::FindClass(\"" + managedType + "\")";
        }

        private static string GenerateCppGetNativeType(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, FunctionInfo functionInfo)
        {
            CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");

            // Optimal path for in-build types
            var managedType = GenerateCSharpNativeToManaged(buildData, typeInfo, caller, true);
            switch (managedType)
            {
            case "bool":
            case "sbyte":
            case "byte":
            case "short":
            case "ushort":
            case "int":
            case "uint":
            case "long":
            case "ulong":
            case "float":
            case "double":
            case "string":
            case "object":
            case "void":
            case "char":
            case "IntPtr":
            case "UIntPtr": return $"{GenerateCppGetMClass(buildData, typeInfo, caller, null)}->GetType()";
            }

            // Find API type
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                CppReferencesFiles.Add(apiType.File);
                if (apiType.IsStruct && !apiType.IsPod && !CppUsedNonPodTypes.Contains(apiType))
                    CppUsedNonPodTypes.Add(apiType);
                if (!apiType.SkipGeneration && !apiType.IsEnum)
                {
                    // Use declared type initializer
                    return $"{apiType.FullNameNative}::TypeInitializer.GetClass()->GetType()";
                }
            }

            // Pass it from C# in glue parameter if used inside the wrapper function
            if (functionInfo != null)
            {
                var customParam = new FunctionInfo.ParameterInfo
                {
                    Name = "resultArrayItemType" + functionInfo.Glue.CustomParameters.Count,
                    DefaultValue = "typeof(" + managedType + ')',
                    Type = new TypeInfo
                    {
                        Type = "MTypeObject",
                        IsPtr = true,
                    },
                };
                functionInfo.Glue.CustomParameters.Add(customParam);
                return "INTERNAL_TYPE_OBJECT_GET(" + customParam.Name + ')';
            }

            // Convert MClass* into MType*
            return $"{GenerateCppGetMClass(buildData, typeInfo, caller, null)}->GetType()";
        }

        private static string GenerateCppManagedWrapperName(ApiTypeInfo type)
        {
            var result = type.NativeName + "Managed";
            while (type.Parent is ClassStructInfo)
            {
                result = type.Parent.NativeName + '_' + result;
                type = type.Parent;
            }
            return result;
        }

        private static string GenerateCppWrapperNativeToManaged(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, out string type, FunctionInfo functionInfo)
        {
            // Use dynamic array as wrapper container for fixed-size native arrays
            if (typeInfo.IsArray)
            {
                var arrayType = new TypeInfo { Type = "Array", GenericArgs = new List<TypeInfo> { new TypeInfo(typeInfo) { IsArray = false } } };
                var result = GenerateCppWrapperNativeToManaged(buildData, arrayType, caller, out type, functionInfo);
                return result.Replace("{0}", $"Span<{typeInfo.Type}>({{0}}, {typeInfo.ArraySize})");
            }

            // Special case for bit-fields
            if (typeInfo.IsBitField)
            {
                if (typeInfo.BitSize == 1)
                {
                    type = "bool";
                    return "{0} != 0";
                }
                throw new NotImplementedException("TODO: support bit-fields with more than 1 bit.");
            }

            switch (typeInfo.Type)
            {
            case "String":
            case "StringView":
            case "StringAnsi":
            case "StringAnsiView":
                type = "MString*";
                return "MUtils::ToString({0})";
            case "Variant":
                type = "MObject*";
                return "MUtils::BoxVariant({0})";
            case "VariantType":
                type = "MTypeObject*";
                return "MUtils::BoxVariantType({0})";
            case "ScriptingTypeHandle":
                type = "MTypeObject*";
                return "MUtils::BoxScriptingTypeHandle({0})";
            case "ScriptingObject":
            case "ManagedScriptingObject":
            case "PersistentScriptingObject":
                type = "MObject*";
                return "ScriptingObject::ToManaged((ScriptingObject*){0})";
            case "MClass":
                type = "MTypeObject*";
                return "MUtils::GetType({0})";
            case "CultureInfo":
                type = "void*";
                return "MUtils::ToManaged({0})";
            case "Version":
                type = "MObject*";
                return "MUtils::ToManaged({0})";
            default:
                // Object reference property
                if (typeInfo.IsObjectRef)
                {
                    type = "MObject*";
                    return "{0}.GetManagedInstance()";
                }

                // Array or DataContainer
                if ((typeInfo.Type == "Array" || typeInfo.Type == "Span" || typeInfo.Type == "DataContainer") && typeInfo.GenericArgs != null)
                {
                    var arrayTypeInfo = typeInfo.GenericArgs[0];
#if USE_NETCORE
                    // Boolean arrays does not support custom marshalling for some unknown reason
                    if (arrayTypeInfo.Type == "bool")
                    {
                        type = "bool*";
                        return "MUtils::ToBoolArray({0})";
                    }
                    var arrayApiType = FindApiTypeInfo(buildData, arrayTypeInfo, caller);
#endif
                    type = "MArray*";
                    if (arrayApiType != null && arrayApiType.MarshalAs != null)
                    {
                        // Convert array that uses different type for marshalling
                        if (arrayApiType != null && arrayApiType.MarshalAs != null)
                            arrayTypeInfo = arrayApiType.MarshalAs; // Convert array that uses different type for marshalling
                        var genericArgs = arrayApiType.MarshalAs.GetFullNameNative(buildData, caller);
                        if (typeInfo.GenericArgs.Count != 1)
                            genericArgs += ", " + typeInfo.GenericArgs[1];
                        return "MUtils::ToArray(Array<" + genericArgs + ">({0}), " + GenerateCppGetMClass(buildData, arrayTypeInfo, caller, functionInfo) + ")";
                    }
                    return "MUtils::ToArray({0}, " + GenerateCppGetMClass(buildData, arrayTypeInfo, caller, functionInfo) + ")";
                }

                // Span
                if (typeInfo.Type == "Span" && typeInfo.GenericArgs != null)
                {
                    type = "MonoArray*";
                    return "MUtils::Span({0}, " + GenerateCppGetMClass(buildData, typeInfo.GenericArgs[0], caller, functionInfo) + ")";
                }

                // BytesContainer
                if (typeInfo.Type == "BytesContainer" && typeInfo.GenericArgs == null)
                {
                    type = "MArray*";
                    return "MUtils::ToArray({0})";
                }

                // Dictionary
                if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
                {
                    CppIncludeFiles.Add("Engine/Scripting/Internal/ManagedDictionary.h");
                    type = "MObject*";
                    var keyClass = GenerateCppGetNativeType(buildData, typeInfo.GenericArgs[0], caller, functionInfo);
                    var valueClass = GenerateCppGetNativeType(buildData, typeInfo.GenericArgs[1], caller, functionInfo);
                    return "ManagedDictionary::ToManaged({0}, " + keyClass + ", " + valueClass + ")";
                }

                // HashSet
                if (typeInfo.Type == "HashSet" && typeInfo.GenericArgs != null)
                {
                    // TODO: automatic converting managed-native for HashSet
                    throw new NotImplementedException("TODO: converting native HashSet to managed");
                }

                // BitArray
                if (typeInfo.Type == "BitArray" && typeInfo.GenericArgs != null)
                {
                    CppIncludeFiles.Add("Engine/Scripting/Internal/ManagedBitArray.h");
#if USE_NETCORE
                    // Boolean arrays does not support custom marshalling for some unknown reason
                    type = "bool*";
                    return "MUtils::ToBoolArray({0})";
#else
                    type = "MObject*";
                    return "ManagedBitArray::ToManaged({0})";
#endif
                }

                // Function
                if (typeInfo.Type == "Function" && typeInfo.GenericArgs != null)
                {
                    // TODO: automatic converting managed-native for Function
                    throw new NotImplementedException("TODO: converting native Function to managed");
                }

                var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
                if (apiType != null)
                {
                    CppReferencesFiles.Add(apiType.File);

                    if (apiType.MarshalAs != null)
                        return GenerateCppWrapperNativeToManaged(buildData, apiType.MarshalAs, caller, out type, functionInfo);

                    // Scripting Object
                    if (apiType.IsScriptingObject)
                    {
                        type = "MObject*";
                        return "ScriptingObject::ToManaged((ScriptingObject*){0})";
                    }

                    // interface
                    if (apiType.IsInterface)
                    {
                        type = "MObject*";
                        return "ScriptingObject::ToManaged(ScriptingObject::FromInterface({0}, " + apiType.NativeName + "::TypeInitializer))";
                    }

                    // Non-POD structure passed as value (eg. it contains string or array inside)
                    if (apiType.IsStruct && !apiType.IsPod)
                    {
                        // Use wrapper structure that represents the memory layout of the managed data
                        if (!CppUsedNonPodTypes.Contains(apiType))
                            CppUsedNonPodTypes.Add(apiType);
                        type = GenerateCppManagedWrapperName(apiType);
                        if (functionInfo != null)
                            type += '*';
                        return "ToManaged({0})";
                    }

                    // Managed class
                    if (apiType.IsClass)
                    {
                        // Use wrapper that box the data into managed object
                        if (!CppUsedNonPodTypes.Contains(apiType))
                            CppUsedNonPodTypes.Add(apiType);
                        type = "MObject*";
                        return "MConverter<" + apiType.Name + ">::Box({0})";
                    }

                    // Nested type (namespace prefix is required)
                    if (!(apiType.Parent is FileInfo))
                    {
                        type = apiType.FullNameNative;
                        return string.Empty;
                    }
                }

                type = typeInfo.ToString();
                return string.Empty;
            }
        }

        private static string GenerateCppWrapperManagedToNative(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, out string type, out ApiTypeInfo apiType, FunctionInfo functionInfo, out bool needLocalVariable)
        {
            needLocalVariable = false;

            // Register any API types usage
            apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            GenerateCppAddFileReference(buildData, caller, typeInfo, apiType);

            // Use dynamic array as wrapper container for fixed-size native arrays
            if (typeInfo.IsArray)
            {
                var arrayType = new TypeInfo { Type = "Array", GenericArgs = new List<TypeInfo> { new TypeInfo(typeInfo) { IsArray = false } } };
                var result = GenerateCppWrapperManagedToNative(buildData, arrayType, caller, out type, out _, functionInfo, out needLocalVariable);
                return result + ".Get()";
            }

            // Special case for bit-fields
            if (typeInfo.IsBitField)
            {
                if (typeInfo.BitSize == 1)
                {
                    type = "bool";
                    return "{0} ? 1 : 0";
                }
                throw new NotImplementedException("TODO: support bit-fields with more than 1 bit.");
            }

            switch (typeInfo.Type)
            {
            case "String":
                type = "MString*";
                return "String(MUtils::ToString({0}))";
            case "StringView":
                type = "MString*";
                return "MUtils::ToString({0})";
            case "StringAnsi":
            case "StringAnsiView":
                type = "MString*";
                return "MUtils::ToStringAnsi({0})";
            case "Variant":
                type = "MObject*";
                return "MUtils::UnboxVariant({0})";
            case "VariantType":
                type = "MTypeObject*";
                return "MUtils::UnboxVariantType({0})";
            case "ScriptingTypeHandle":
                type = "MTypeObject*";
                return "MUtils::UnboxScriptingTypeHandle({0})";
            case "CultureInfo":
                type = "void*";
                return "MUtils::ToNative({0})";
            case "Version":
                type = "MObject*";
                return "MUtils::ToNative({0})";
            default:
                // Object reference property
                if (typeInfo.IsObjectRef)
                {
                    // For non-pod types converting only, other API converts managed to unmanaged object in C# wrapper code)
                    if (CppNonPodTypesConvertingGeneration)
                    {
                        type = "MObject*";
                        return "(" + typeInfo.GenericArgs[0].Type + "*)ScriptingObject::ToNative({0})";
                    }

                    type = typeInfo.GenericArgs[0].Type + '*';
                    return string.Empty;
                }

                // MClass
                if (typeInfo.Type == "MClass" && typeInfo.GenericArgs == null)
                {
                    type = "MTypeObject*";
                    return "MUtils::GetClass({0})";
                }

                // Array
                if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
                {
                    var arrayTypeInfo = typeInfo.GenericArgs[0];
                    var arrayApiType = FindApiTypeInfo(buildData, arrayTypeInfo, caller);
                    if (arrayApiType != null && arrayApiType.MarshalAs != null)
                        arrayTypeInfo = arrayApiType.MarshalAs;
                    var genericArgs = arrayTypeInfo.GetFullNameNative(buildData, caller);
                    if (typeInfo.GenericArgs.Count != 1)
                        genericArgs += ", " + typeInfo.GenericArgs[1];

                    type = "MArray*";
                    var result = "MUtils::ToArray<" + genericArgs + ">({0})";

                    if (arrayApiType != null && arrayApiType.MarshalAs != null)
                    {
                        // Convert array that uses different type for marshalling
                        genericArgs = typeInfo.GenericArgs[0].GetFullNameNative(buildData, caller);
                        if (typeInfo.GenericArgs.Count != 1)
                            genericArgs += ", " + typeInfo.GenericArgs[1];
                        result = $"Array<{genericArgs}>({result})";
                    }
                    return result;
                }

                // Span or DataContainer
                if ((typeInfo.Type == "Span" || typeInfo.Type == "DataContainer") && typeInfo.GenericArgs != null)
                {
                    type = "MArray*";

                    // Scripting Objects pointers has to be converted from managed object pointer into native object pointer to use Array converted for this
                    var t = FindApiTypeInfo(buildData, typeInfo.GenericArgs[0], caller);
                    if (typeInfo.GenericArgs[0].IsPtr && t != null && t.IsScriptingObject)
                    {
                        return "MUtils::ToSpan<" + typeInfo.GenericArgs[0] + ">(" + "MUtils::ToArray<" + typeInfo.GenericArgs[0] + ">({0}))";
                    }

                    return "MUtils::ToSpan<" + typeInfo.GenericArgs[0] + ">({0})";
                }

                // Dictionary
                if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
                {
                    CppIncludeFiles.Add("Engine/Scripting/Internal/ManagedDictionary.h");
                    type = "MObject*";
                    return string.Format("ManagedDictionary::ToNative<{0}, {1}>({{0}})", typeInfo.GenericArgs[0], typeInfo.GenericArgs[1]);
                }

                // HashSet
                if (typeInfo.Type == "HashSet" && typeInfo.GenericArgs != null)
                {
                    // TODO: automatic converting managed-native for HashSet
                    throw new NotImplementedException("TODO: converting managed HashSet to native");
                }

                // BitArray
                if (typeInfo.Type == "BitArray" && typeInfo.GenericArgs != null)
                {
                    // TODO: automatic converting managed-native for BitArray
                    throw new NotImplementedException("TODO: converting managed BitArray to native");
                }

                // BytesContainer
                if (typeInfo.Type == "BytesContainer" && typeInfo.GenericArgs == null)
                {
                    needLocalVariable = true;
                    type = "MArray*";
                    return "MUtils::LinkArray({0})";
                }

                // Function
                if (typeInfo.Type == "Function" && typeInfo.GenericArgs != null)
                {
                    var args = string.Empty;
                    if (typeInfo.GenericArgs.Count > 1)
                    {
                        args += typeInfo.GenericArgs[1].GetFullNameNative(buildData, caller);
                        for (int i = 2; i < typeInfo.GenericArgs.Count; i++)
                            args += ", " + typeInfo.GenericArgs[i].GetFullNameNative(buildData, caller);
                    }
                    var T = $"Function<{typeInfo.GenericArgs[0].GetFullNameNative(buildData, caller)}({args})>";
                    type = T + "::Signature";
                    return T + "({0})";
                }

                if (apiType != null)
                {
                    if (apiType.MarshalAs != null)
                        return GenerateCppWrapperManagedToNative(buildData, apiType.MarshalAs, caller, out type, out apiType, functionInfo, out needLocalVariable);

                    // Scripting Object (for non-pod types converting only, other API converts managed to unmanaged object in C# wrapper code)
                    if (CppNonPodTypesConvertingGeneration && apiType.IsScriptingObject && typeInfo.IsPtr)
                    {
                        type = typeInfo.ToString();
                        return "(" + type + ")ScriptingObject::ToNative({0})";
                    }

                    // Non-POD structure passed as value (eg. it contains string or array inside)
                    if (apiType.IsStruct && !apiType.IsPod)
                    {
                        // Use wrapper structure that represents the memory layout of the managed data
                        needLocalVariable = true;
                        if (!CppUsedNonPodTypes.Contains(apiType))
                            CppUsedNonPodTypes.Add(apiType);
                        type = GenerateCppManagedWrapperName(apiType);
                        if (functionInfo != null)
                            return "ToNative(*{0})";
                        return "ToNative({0})";
                    }

                    // Managed class
                    if (apiType.IsClass && !apiType.IsScriptingObject)
                    {
                        // Use wrapper that unboxes the data into managed object
                        if (!CppUsedNonPodTypes.Contains(apiType))
                            CppUsedNonPodTypes.Add(apiType);
                        type = "MObject*";
                        return "MConverter<" + apiType.Name + ">::Unbox({0})";
                    }

                    // Scripting Object
                    if (functionInfo == null && apiType.IsScriptingObject)
                    {
                        // Inside bindings function the managed runtime passes raw unamanged pointer
                        type = "MObject*";
                        return "(" + typeInfo.Type + "*)ScriptingObject::ToNative({0})";
                    }

                    // Nested type (namespace prefix is required)
                    if (!(apiType.Parent is FileInfo))
                    {
                        type = apiType.FullNameNative;
                        return string.Empty;
                    }
                }

                if (UsePassByReference(buildData, typeInfo, caller))
                {
                    type = typeInfo.Type;
                    if (typeInfo.GenericArgs != null)
                    {
                        type += '<';
                        for (var i = 0; i < typeInfo.GenericArgs.Count; i++)
                        {
                            if (i != 0)
                                type += ", ";
                            type += typeInfo.GenericArgs[i];
                        }
                        type += '>';
                    }
                    return string.Empty;
                }

                type = typeInfo.ToString();
                return string.Empty;
            }
        }

        private static string GenerateCppWrapperNativeToBox(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, out ApiTypeInfo apiType, string value)
        {
            // Optimize passing scripting objects
            apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null && apiType.IsScriptingObject)
                return $"ScriptingObject::ToManaged((ScriptingObject*){value})";

            // Array or Span or DataContainer
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span" || typeInfo.Type == "DataContainer") && typeInfo.GenericArgs != null && typeInfo.GenericArgs.Count >= 1)
                return $"MUtils::ToArray({value}, {GenerateCppGetMClass(buildData, typeInfo.GenericArgs[0], caller, null)})";

            // BytesContainer
            if (typeInfo.Type == "BytesContainer" && typeInfo.GenericArgs == null)
                return "MUtils::ToArray({0})";

            // Construct native typename for MUtils template argument
            var nativeType = new StringBuilder(64);
            nativeType.Append(typeInfo.Type);
            if (typeInfo.GenericArgs != null)
            {
                nativeType.Append('<');
                for (var j = 0; j < typeInfo.GenericArgs.Count; j++)
                {
                    if (j != 0)
                        nativeType.Append(", ");
                    nativeType.Append(typeInfo.GenericArgs[j]);
                }

                nativeType.Append('>');
            }
            if (typeInfo.IsPtr)
                nativeType.Append('*');

            // Use MUtils to box the value
            return $"MUtils::Box<{nativeType}>({value}, {GenerateCppGetMClass(buildData, typeInfo, caller, null)})";
        }

        private static bool GenerateCppWrapperFunctionImplicitBinding(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller)
        {
            if (typeInfo.IsVoid)
                return true;
            if (typeInfo.IsPtr || typeInfo.IsRef || typeInfo.IsArray || typeInfo.IsBitField || (typeInfo.GenericArgs != null && typeInfo.GenericArgs.Count != 0))
                return false;
            if (CSharpNativeToManagedBasicTypes.ContainsKey(typeInfo.Type) || CSharpNativeToManagedBasicTypes.ContainsValue(typeInfo.Type))
                return true;
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                if (apiType.IsEnum)
                    return true;
            }
            return false;
        }

        private static bool GenerateCppWrapperFunctionImplicitBinding(BuildData buildData, FunctionInfo functionInfo, ApiTypeInfo caller)
        {
            if (!functionInfo.IsStatic || functionInfo.Access != AccessLevel.Public || (functionInfo.Glue.CustomParameters != null && functionInfo.Glue.CustomParameters.Count != 0))
                return false;
            if (!GenerateCppWrapperFunctionImplicitBinding(buildData, functionInfo.ReturnType, caller))
                return false;
            for (int i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (parameterInfo.IsOut || parameterInfo.IsRef || !GenerateCppWrapperFunctionImplicitBinding(buildData, parameterInfo.Type, caller))
                    return false;
            }
            return true;
        }

        private static void GenerateCppWrapperFunction(BuildData buildData, StringBuilder contents, ApiTypeInfo caller, string callerName, FunctionInfo functionInfo, string callFormat = "{0}({1})")
        {
#if !USE_NETCORE
            // Optimize static function wrappers that match C# internal call ABI exactly
            // Use it for Engine-internally only because in games this makes it problematic to use the same function name but with different signature that is not visible to scripting
            if (CurrentModule.Module is EngineModule && callFormat == "{0}({1})" && GenerateCppWrapperFunctionImplicitBinding(buildData, functionInfo, caller))
            {
                // Ensure the function name is unique within a class/structure
                if (caller is ClassStructInfo classStructInfo && classStructInfo.Functions.All(f => f.Name != functionInfo.Name || f == functionInfo))
                {
                    // Use native method binding directly (no generated wrapper)
                    CppInternalCalls.Add(new KeyValuePair<string, string>(functionInfo.UniqueName, classStructInfo.Name + "::" + functionInfo.Name));
                    return;
                }
            }
#endif

            // Setup function binding glue to ensure that wrapper method signature matches for C++ and C#
            functionInfo.Glue = new FunctionInfo.GlueInfo
            {
                UseReferenceForResult = UsePassByReference(buildData, functionInfo.ReturnType, caller),
                CustomParameters = new List<FunctionInfo.ParameterInfo>(),
            };
            var returnType = functionInfo.ReturnType;
            var returnApiType = FindApiTypeInfo(buildData, returnType, caller);
            if (returnApiType != null && returnApiType.MarshalAs != null)
                returnType = returnApiType.MarshalAs;

            bool returnTypeIsContainer = false;
            var returnValueConvert = GenerateCppWrapperNativeToManaged(buildData, functionInfo.ReturnType, caller, out var returnValueType, functionInfo);
            if (functionInfo.Glue.UseReferenceForResult)
            {
                returnValueType = "void";
                functionInfo.Glue.CustomParameters.Add(new FunctionInfo.ParameterInfo
                {
                    Name = "__resultAsRef",
                    DefaultValue = "var __resultAsRef",
                    Type = new TypeInfo
                    {
                        Type = functionInfo.ReturnType.Type,
                        GenericArgs = functionInfo.ReturnType.GenericArgs,
                    },
                    IsOut = true,
                });
            }
#if USE_NETCORE
            else if (returnType.Type == "Array" || returnType.Type == "Span" || returnType.Type == "DataContainer" || returnType.Type == "BitArray" || returnType.Type == "BytesContainer")
            {
                returnTypeIsContainer = true;
                functionInfo.Glue.CustomParameters.Add(new FunctionInfo.ParameterInfo
                {
                    Name = "__returnCount",
                    DefaultValue = "var __returnCount",
                    Type = new TypeInfo("int"),
                    IsOut = true,
                });
            }
#endif

            var prevIndent = "    ";
            var indent = "        ";
            contents.Append(prevIndent);
            bool useSeparateImpl = false; // True if separate function declaration from implementation
            bool useLibraryExportInPlainC = false; // True if generate separate wrapper for library imports that uses plain-C style binding (without C++ name mangling)
#if USE_NETCORE
            string libraryEntryPoint;
            if (buildData.Toolchain?.Compiler == TargetCompiler.MSVC)
            {
                libraryEntryPoint = $"{caller.FullNameManaged}::Internal_{functionInfo.UniqueName}"; // MSVC allows to override exported symbol name
            }
            else
            {
                // For simple functions we can bind it directly (CppNameMangling is incomplete for complex cases)
                // TODO: improve this to use wrapper method directly from P/Invoke if possible
                /*if (functionInfo.Parameters.Count + functionInfo.Parameters.Count == 0)
                {
                    useSeparateImpl = true; // DLLEXPORT doesn't properly export function thus separate implementation from declaration
                    libraryEntryPoint = CppNameMangling.MangleFunctionName(buildData, functionInfo.Name, callerName, functionInfo.ReturnType, functionInfo.IsStatic ? null : new TypeInfo(caller.Name) { IsPtr = true }, functionInfo.Parameters, functionInfo.Glue.CustomParameters);
                }
                else*/
                {
                    useLibraryExportInPlainC = true;
                    libraryEntryPoint = $"{callerName}_{functionInfo.UniqueName}";
                }
            }
            if (useLibraryExportInPlainC)
                contents.AppendFormat("static {0} {1}(", returnValueType, functionInfo.UniqueName);
            else
                contents.AppendFormat("DLLEXPORT static {0} {1}(", returnValueType, functionInfo.UniqueName);
            functionInfo.Glue.LibraryEntryPoint = libraryEntryPoint;
#else
            contents.AppendFormat("static {0} {1}(", returnValueType, functionInfo.UniqueName);
#endif
            CppInternalCalls.Add(new KeyValuePair<string, string>(functionInfo.UniqueName, functionInfo.UniqueName));

            var separator = false;
            var signatureStart = contents.Length;
            if (!functionInfo.IsStatic)
            {
                contents.Append(caller.Name).Append("* __obj");
                separator = true;
            }

            var useInlinedReturn = true;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                CppParamsThatNeedConversion[i] = false;
                CppParamsWrappersCache[i] = GenerateCppWrapperManagedToNative(buildData, parameterInfo.Type, caller, out var managedType, out var apiType, functionInfo, out CppParamsThatNeedLocalVariable[i]);

                // Out parameters that need additional converting will be converted at the native side (eg. object reference)
                var isOutWithManagedConverter = parameterInfo.IsOut && !string.IsNullOrEmpty(GenerateCSharpManagedToNativeConverter(buildData, parameterInfo.Type, caller));
                if (isOutWithManagedConverter)
                    managedType = "MObject*";

                contents.Append(managedType);
                if (parameterInfo.IsRef || parameterInfo.IsOut || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append('*');
                contents.Append(' ');
                contents.Append(parameterInfo.Name);

                // Special case for output result parameters that needs additional converting from native to managed format (such as non-POD structures, output arrays, etc.)
                var isRefOut = parameterInfo.IsRef && parameterInfo.Type.IsRef && !parameterInfo.Type.IsConst;
                if (parameterInfo.IsOut || isRefOut)
                {
                    bool convertOutputParameter = false;
                    if (apiType != null)
                    {
                        // Non-POD structure passed as value (eg. it contains string or array inside)
                        if (apiType.IsStruct && !apiType.IsPod)
                        {
                            convertOutputParameter = true;
                        }
                        // Arrays, Scripting Objects, Dictionaries and other types that need to be converted into managed format if used as output parameter
                        else if (!apiType.IsPod)
                        {
                            convertOutputParameter = true;
                        }
                        else if (apiType.Name == "Variant")
                        {
                            convertOutputParameter = true;
                        }
                    }
                    // BytesContainer
                    else if (parameterInfo.Type.Type == "BytesContainer" && parameterInfo.Type.GenericArgs == null)
                    {
                        convertOutputParameter = true;
                    }
                    if (convertOutputParameter)
                    {
                        useInlinedReturn = false;
                        CppParamsThatNeedConversion[i] = true;
                        CppParamsThatNeedConversionWrappers[i] = GenerateCppWrapperNativeToManaged(buildData, parameterInfo.Type, caller, out CppParamsThatNeedConversionTypes[i], functionInfo);
                    }
                }
#if USE_NETCORE
                if (parameterInfo.Type.IsArray || parameterInfo.Type.Type == "Array" || parameterInfo.Type.Type == "Span" || parameterInfo.Type.Type == "BytesContainer" || parameterInfo.Type.Type == "DataContainer" || parameterInfo.Type.Type == "BitArray")
                {
                    // We need additional output parameters for array sizes
                    var name = $"__{parameterInfo.Name}Count";
                    functionInfo.Glue.CustomParameters.Add(new FunctionInfo.ParameterInfo
                    {
                        Name = name,
                        DefaultValue = parameterInfo.IsOut ? "int _" : name,
                        Type = new TypeInfo
                        {
                            Type = "int"
                        },
                        IsOut = parameterInfo.IsOut,
                        IsRef = isRefOut || parameterInfo.Type.IsRef,
                    });
                }
#endif
            }

            for (var i = 0; i < functionInfo.Glue.CustomParameters.Count; i++)
            {
                var parameterInfo = functionInfo.Glue.CustomParameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                GenerateCppWrapperManagedToNative(buildData, parameterInfo.Type, caller, out var managedType, out _, functionInfo, out _);
                contents.Append(managedType);
                if (parameterInfo.IsRef || parameterInfo.IsOut || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append('*');
                contents.Append(' ');
                contents.Append(parameterInfo.Name);
            }

            contents.Append(')');
            var signatureEnd = contents.Length;
            if (useSeparateImpl)
            {
                // Write declaration only, function definition wil be put in the end of the file
                CppContentsEnd.AppendFormat("{0} {2}::{1}(", returnValueType, functionInfo.UniqueName, callerName);
                CppContentsEnd.Append(contents.ToString(signatureStart, signatureEnd - signatureStart));
                contents.Append(';').AppendLine();
                contents = CppContentsEnd;
                prevIndent = null;
                indent = "    ";
            }
            contents.AppendLine();
            contents.Append(prevIndent).AppendLine("{");
#if USE_NETCORE
            if (buildData.Toolchain?.Compiler == TargetCompiler.MSVC && !useLibraryExportInPlainC)
                contents.Append(indent).AppendLine($"MSVC_FUNC_EXPORT(\"{libraryEntryPoint}\")"); // Export generated function binding under the C# name
#endif
            if (!functionInfo.IsStatic)
                contents.Append(indent).AppendLine("if (__obj == nullptr) DebugLog::ThrowNullReference();");

            string callBegin = indent;
            if (functionInfo.Glue.UseReferenceForResult)
            {
                callBegin += "*__resultAsRef = ";
            }
            else if (!returnType.IsVoid)
            {
                if (useInlinedReturn)
                    callBegin += "return ";
                else
                    callBegin += "auto __result = ";
            }

#if USE_NETCORE
            string callReturnCount = "";
            if (returnTypeIsContainer)
            {
                callReturnCount = indent;
                if (returnType.Type == "Span" || returnType.Type == "BytesContainer")
                    callReturnCount += "*__returnCount = {0}.Length();";
                else
                    callReturnCount += "*__returnCount = {0}.Count();";
            }
#endif
            string call;
            if (functionInfo.IsStatic)
            {
                // Call native static method
                string nativeType = caller.Tags != null && caller.Tags.ContainsKey("NativeInvokeUseName") ? caller.Name : caller.NativeName;
                if (caller.Parent != null && !(caller.Parent is FileInfo))
                    nativeType = caller.Parent.FullNameNative + "::" + nativeType;
                call = $"{nativeType}::{functionInfo.Name}";
            }
            else
            {
                // Call native member method
                call = $"__obj->{functionInfo.Name}";
            }
            string callParams = string.Empty;
            separator = false;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    callParams += ", ";
                separator = true;
                var name = parameterInfo.Name;
                if (CppParamsThatNeedConversion[i] && (!FindApiTypeInfo(buildData, parameterInfo.Type, caller)?.IsStruct ?? false))
                    name = '*' + name;

                string param = string.Empty;
                if (string.IsNullOrWhiteSpace(CppParamsWrappersCache[i]))
                {
                    // Pass value
                    if (UsePassByReference(buildData, parameterInfo.Type, caller))
                        param += '*';
                    param += name;
                }
                else
                {
                    // Convert value
                    param += string.Format(CppParamsWrappersCache[i], name);
                }

                // Special case for output result parameters that needs additional converting from native to managed format (such as non-POD structures or output array parameter)
                if (CppParamsThatNeedConversion[i])
                {
                    var apiType = FindApiTypeInfo(buildData, parameterInfo.Type, caller);
                    if (apiType != null)
                    {
                        if (parameterInfo.IsOut)
                            contents.Append(indent).AppendFormat("{1} {0}Temp;", parameterInfo.Name, parameterInfo.Type.GetFullNameNative(buildData, caller, false)).AppendLine();
                        else
                            contents.Append(indent).AppendFormat("auto {0}Temp = {1};", parameterInfo.Name, param).AppendLine();
                        if (parameterInfo.Type.IsPtr && !parameterInfo.Type.IsRef)
                            callParams += "&";
                        callParams += parameterInfo.Name;
                        callParams += "Temp";
                    }
                    // BytesContainer
                    else if (parameterInfo.Type.Type == "BytesContainer" && parameterInfo.Type.GenericArgs == null)
                    {
                        contents.Append(indent).AppendFormat("BytesContainer {0}Temp;", parameterInfo.Name).AppendLine();
                        callParams += parameterInfo.Name;
                        callParams += "Temp";
                    }
                }
                // Special case for parameter that cannot be passed directly to the function from the wrapper method input parameter (eg. MArray* converted into BytesContainer uses as BytesContainer&)
                else if (CppParamsThatNeedLocalVariable[i])
                {
                    contents.Append(indent).AppendFormat("auto {0}Temp = {1};", parameterInfo.Name, param).AppendLine();
                    if (parameterInfo.Type.IsPtr)
                        callParams += "&";
                    callParams += parameterInfo.Name;
                    callParams += "Temp";
                }
                // Instruct for more optoimized value move operation
                else if (parameterInfo.Type.IsMoveRef)
                {
                    callParams += $"MoveTemp({param})";
                }
                else
                {
                    callParams += param;
                }
            }

#if USE_NETCORE
            if (!string.IsNullOrEmpty(callReturnCount))
            {
                var tempVar = returnTypeIsContainer && returnType != functionInfo.ReturnType ? $"{returnType} __callTemp = " : "const auto& __callTemp = ";
                contents.Append(indent).Append(tempVar).Append(string.Format(callFormat, call, callParams)).Append(";").AppendLine();
                call = "__callTemp";
                contents.Append(string.Format(callReturnCount, call));
                contents.AppendLine();
                contents.Append(callBegin);
            }
            else
#endif
            {
                contents.Append(callBegin);
                call = string.Format(callFormat, call, callParams);
            }
            if (!string.IsNullOrEmpty(returnValueConvert))
            {
                contents.AppendFormat(returnValueConvert, call);
            }
            else
            {
                contents.Append(call);
            }

            contents.Append(';');
            contents.AppendLine();

            // Convert special parameters back to managed world
            if (!useInlinedReturn)
            {
                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];

                    // Special case for output result parameters that needs additional converting from native to managed format (such as non-POD structures or output array parameter)
                    if (CppParamsThatNeedConversion[i])
                    {
                        var value = string.Format(CppParamsThatNeedConversionWrappers[i], parameterInfo.Name + "Temp");

                        // MObject* parameters returned by reference need write barrier for GC
                        if (parameterInfo.IsOut)
                        {
                            var apiType = FindApiTypeInfo(buildData, parameterInfo.Type, caller);
                            if (apiType != null)
                            {
                                if (apiType.IsClass)
                                {
                                    contents.Append(indent).AppendFormat("MCore::GC::WriteRef({0}, (MObject*){1});", parameterInfo.Name, value).AppendLine();
#if USE_NETCORE
                                    if (parameterInfo.Type.Type == "Array")
                                    {
                                        contents.Append(indent).AppendFormat("*__{0}Count = {1}.Count();", parameterInfo.Name, parameterInfo.Name + "Temp").AppendLine();
                                    }
#endif
                                    continue;
                                }
                                if (apiType.IsStruct && !apiType.IsPod)
                                {
                                    // Structure that has reference to managed objects requries copy relevant for GC barriers (on Mono)
                                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                                    contents.Append(indent).AppendFormat("{{ auto __temp = {1}; MCore::GC::WriteValue({0}, &__temp, 1, {2}::TypeInitializer.GetClass()); }}", parameterInfo.Name, value, apiType.FullNameNative).AppendLine();
                                    continue;
                                }
                            }
                            else
                            {
                                // BytesContainer
                                if (parameterInfo.Type.Type == "BytesContainer" && parameterInfo.Type.GenericArgs == null)
                                {
                                    contents.Append(indent).AppendFormat("MCore::GC::WriteRef({0}, (MObject*){1});", parameterInfo.Name, value).AppendLine();
                                    contents.Append(indent).AppendFormat("*__{0}Count = {1}.Length();", parameterInfo.Name, parameterInfo.Name + "Temp").AppendLine();
                                    continue;
                                }

                                throw new Exception($"Unsupported type of parameter '{parameterInfo}' in method '{functionInfo}' to be passed using 'out'");
                            }
                        }
                        contents.Append(indent).AppendFormat("*{0} = {1};", parameterInfo.Name, value).AppendLine();
                    }
                }
            }

            if (!useInlinedReturn && !functionInfo.Glue.UseReferenceForResult && !returnType.IsVoid)
            {
                contents.Append(indent).Append("return __result;").AppendLine();
            }

            contents.Append(prevIndent).AppendLine("}");
            contents.AppendLine();

            if (useLibraryExportInPlainC)
            {
                // Write simple wrapper to bind internal method
                CppContentsEnd.AppendFormat("DEFINE_INTERNAL_CALL({0}) {1}(", returnValueType, libraryEntryPoint);
                var sig = contents.ToString(signatureStart, signatureEnd - signatureStart);
                CppContentsEnd.Append(sig).AppendLine();
                CppContentsEnd.AppendLine("{");
                CppContentsEnd.Append($"    return {callerName}::{functionInfo.UniqueName}(");
                while (sig.Contains("Function<"))
                {
                    // Hack for template args
                    int start = sig.IndexOf("Function<");
                    int end = sig.IndexOf(">::Signature");
                    sig = sig.Substring(0, start) + sig.Substring(end + 3);
                }
                var args = sig.Split(',', StringSplitOptions.TrimEntries);
                for (int i = 0; i < args.Length; i++)
                {
                    var arg = args[i];
                    if (i + 1 == args.Length)
                        arg = arg.Substring(0, arg.Length - 1);
                    if (arg.Length == 0)
                        continue;
                    var lastSpace = arg.LastIndexOf(' ');
                    if (lastSpace != -1)
                        arg = arg.Substring(lastSpace + 1);
                    if (i != 0)
                        CppContentsEnd.Append(", ");
                    CppContentsEnd.Append(arg);
                }
                CppContentsEnd.AppendLine(");");
                CppContentsEnd.AppendLine("}").AppendLine();
            }
        }

        public static void GenerateCppReturn(BuildData buildData, StringBuilder contents, string indent, TypeInfo type)
        {
            contents.Append(indent);
            if (type.IsVoid)
            {
                contents.AppendLine("return;");
                return;
            }
            if (type.IsPtr)
            {
                contents.AppendLine("return nullptr;");
                return;
            }
            contents.AppendLine($"{type} __return {{}};");
            contents.Append(indent).AppendLine("return __return;");
        }

        private static void GenerateCppManagedWrapperFunction(BuildData buildData, StringBuilder contents, VirtualClassInfo classInfo, FunctionInfo functionInfo, int scriptVTableSize, int scriptVTableIndex)
        {
            if (!EngineConfiguration.WithCSharp(buildData.TargetOptions))
                return;
            contents.AppendFormat("    {0} {1}_ManagedWrapper(", functionInfo.ReturnType, functionInfo.UniqueName);
            var separator = false;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                contents.Append(parameterInfo.Type);
                contents.Append(' ');
                contents.Append(parameterInfo.Name);
            }

            CppIncludeFiles.Add("Engine/Profiler/ProfilerCPU.h");

            contents.Append(')');
            contents.AppendLine();
            contents.AppendLine("    {");

            // Get object
            string scriptVTableOffset;
            if (classInfo.IsInterface)
            {
                contents.AppendLine($"        auto object = ScriptingObject::FromInterface(this, {classInfo.NativeName}::TypeInitializer);");
                contents.AppendLine("        if (object == nullptr)");
                contents.AppendLine("        {");
                contents.AppendLine($"            LOG(Error, \"Failed to cast interface {{0}} to scripting object\", TEXT(\"{classInfo.Name}\"));");
                GenerateCppReturn(buildData, contents, "            ", functionInfo.ReturnType);
                contents.AppendLine("        }");
                contents.AppendLine($"        const int32 scriptVTableOffset = {scriptVTableIndex} + object->GetType().GetInterface({classInfo.NativeName}::TypeInitializer)->ScriptVTableOffset;");
                scriptVTableOffset = "scriptVTableOffset";
            }
            else
            {
                contents.AppendLine($"        auto object = ({classInfo.NativeName}*)this;");
                scriptVTableOffset = scriptVTableIndex.ToString();
            }
            contents.AppendLine("        static THREADLOCAL void* WrapperCallInstance = nullptr;");

            // Base method call
            contents.AppendLine("        ScriptingTypeHandle managedTypeHandle = object->GetTypeHandle();");
            contents.AppendLine("        const ScriptingType* managedTypePtr = &managedTypeHandle.GetType();");
            contents.AppendLine("        while (managedTypePtr->Script.Spawn != &ManagedBinaryModule::ManagedObjectSpawn)");
            contents.AppendLine("        {");
            contents.AppendLine("            managedTypeHandle = managedTypePtr->GetBaseType();");
            contents.AppendLine("            managedTypePtr = &managedTypeHandle.GetType();");
            contents.AppendLine("        }");
            contents.AppendLine("        if (WrapperCallInstance == object)");
            contents.AppendLine("        {");
            GenerateCppVirtualWrapperCallBaseMethod(buildData, contents, classInfo, functionInfo, "managedTypePtr->Script.ScriptVTableBase", scriptVTableOffset);
            contents.AppendLine("        }");

            contents.AppendLine("        auto scriptVTable = (MMethod**)managedTypePtr->Script.ScriptVTable;");
            contents.AppendLine($"        ASSERT(scriptVTable && scriptVTable[{scriptVTableOffset}]);");
            contents.AppendLine($"        auto method = scriptVTable[{scriptVTableOffset}];");

            contents.AppendLine("        MObject* exception = nullptr;");
            contents.AppendLine("        auto prevWrapperCallInstance = WrapperCallInstance;");
            contents.AppendLine("        WrapperCallInstance = object;");

            if (functionInfo.Parameters.Count == 0)
                contents.AppendLine("        void** params = nullptr;");
            else
                contents.AppendLine($"        void* params[{functionInfo.Parameters.Count}];");

            // If platform supports JITed code execution then use method thunk, otherwise fallback to generic runtime invoke
            var returnType = functionInfo.ReturnType;
            var useThunk = buildData.Platform.HasDynamicCodeExecutionSupport && Configuration.AOTMode == DotNetAOTModes.None;
            if (useThunk)
            {
                //contents.AppendLine($"        PROFILE_CPU_NAMED(\"{classInfo.FullNameManaged}::{functionInfo.Name}\");");
                contents.AppendLine("        PROFILE_CPU_SRC_LOC(method->ProfilerData);");

                // Convert parameters into managed format as boxed values
                var thunkParams = string.Empty;
                var thunkCall = string.Empty;
                if (functionInfo.Parameters.Count != 0)
                {
                    separator = functionInfo.Parameters.Count != 0;
                    for (var i = 0; i < functionInfo.Parameters.Count; i++)
                    {
                        var parameterInfo = functionInfo.Parameters[i];
                        var paramIsRef = parameterInfo.IsRef || parameterInfo.IsOut;
                        var paramValue = GenerateCppWrapperNativeToBox(buildData, parameterInfo.Type, classInfo, out var apiType, parameterInfo.Name);
                        var useLocalVar = false;
                        if (paramIsRef)
                        {
                            // Pass as pointer to value when using ref/out parameter
                            contents.Append($"        auto __param_{parameterInfo.Name} = {paramValue};").AppendLine();
                            paramValue = $"&__param_{parameterInfo.Name}";
                            useLocalVar = true;
                        }
                        CppParamsThatNeedLocalVariable[i] = useLocalVar;

                        if (separator)
                            thunkParams += ", ";
                        if (separator)
                            thunkCall += ", ";
                        separator = true;
                        thunkParams += "void*";
                        contents.Append($"        params[{i}] = {paramValue};").AppendLine();
                        thunkCall += $"params[{i}]";
                    }
                }

                // Invoke method thunk
                if (returnType.IsVoid)
                {
                    contents.AppendLine($"        typedef void (*Thunk)(void* instance{thunkParams}, MObject** exception);");
                    contents.AppendLine("        const auto thunk = (Thunk)method->GetThunk();");
                    contents.AppendLine($"        thunk(object->GetOrCreateManagedInstance(){thunkCall}, &exception);");
                }
                else
                {
                    contents.AppendLine($"        typedef MObject* (*Thunk)(void* instance{thunkParams}, MObject** exception);");
                    contents.AppendLine("        const auto thunk = (Thunk)method->GetThunk();");
                    contents.AppendLine($"        MObject* __result = thunk(object->GetOrCreateManagedInstance(){thunkCall}, &exception);");
                }

                // Convert parameter values back from managed to native (could be modified there)
                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];
                    var paramIsRef = parameterInfo.IsRef || parameterInfo.IsOut;
                    if (paramIsRef && !parameterInfo.Type.IsConst)
                    {
                        // Unbox from MObject*
                        contents.Append($"        {parameterInfo.Name} = MUtils::Unbox<{parameterInfo.Type.ToString(false)}>(*(MObject**)params[{i}]);").AppendLine();
                    }
                }
            }
            else
            {
                // Convert parameters into managed format as pointers to value
                if (functionInfo.Parameters.Count != 0)
                {
                    for (var i = 0; i < functionInfo.Parameters.Count; i++)
                    {
                        var parameterInfo = functionInfo.Parameters[i];
                        var paramIsRef = parameterInfo.IsRef || parameterInfo.IsOut;
                        var paramValue = GenerateCppWrapperNativeToManagedParam(buildData, contents, parameterInfo.Type, parameterInfo.Name, classInfo, paramIsRef, out CppParamsThatNeedLocalVariable[i]);
                        contents.Append($"        params[{i}] = {paramValue};").AppendLine();
                    }
                }

                // Invoke method
                contents.AppendLine("        MObject* __result = method->Invoke(object->GetOrCreateManagedInstance(), params, &exception);");

                // Convert parameter values back from managed to native (could be modified there)
                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];
                    if ((parameterInfo.IsRef || parameterInfo.IsOut) && !parameterInfo.Type.IsConst)
                    {
                        // Direct value convert
                        var managedToNative = GenerateCppWrapperManagedToNative(buildData, parameterInfo.Type, classInfo, out var managedType, out var apiType, null, out _);
                        var passAsParamPtr = managedType.EndsWith("*");
                        var useLocalVarPointer = CppParamsThatNeedConversion[i] && !apiType.IsValueType;
                        var paramValue = useLocalVarPointer ? $"*({managedType}{(passAsParamPtr ? "" : "*")}*)params[{i}]" : $"({managedType}{(passAsParamPtr ? "" : "*")})params[{i}]";
                        if (!string.IsNullOrEmpty(managedToNative))
                        {
                            if (!passAsParamPtr)
                                paramValue = '*' + paramValue;
                            paramValue = string.Format(managedToNative, paramValue);
                        }
                        else if (!passAsParamPtr)
                            paramValue = '*' + paramValue;
                        contents.Append($"        {parameterInfo.Name} = {paramValue};").AppendLine();
                    }
                }
            }

            contents.AppendLine("        WrapperCallInstance = prevWrapperCallInstance;");
            contents.AppendLine("        if (exception)");
            contents.AppendLine("            DebugLog::LogException(exception);");

            // Unpack returned value
            if (!returnType.IsVoid)
            {
                if (returnType.IsRef)
                    throw new NotSupportedException($"Passing return value by reference is not supported for virtual API methods. Used on method '{functionInfo}'.");
                if (useThunk)
                {
                    // Thunk might return value within pointer (eg. as int or boolean)
                    switch (returnType.Type)
                    {
                    case "bool":
                        contents.AppendLine("        return __result != 0;");
                        break;
                    case "int8":
                    case "int16":
                    case "int32":
                    case "int64":
                        contents.AppendLine($"        return ({returnType.Type})(intptr)__result;");
                        break;
                    case "uint8":
                    case "uint16":
                    case "uint32":
                    case "uint64":
                        contents.AppendLine($"        return ({returnType.Type})(uintptr)__result;");
                        break;
                    default:
                        contents.AppendLine($"        return MUtils::Unbox<{returnType}>(__result);");
                        break;
                    }
                }
                else
                {
                    // Runtime invoke always returns boxed value as MObject*
                    contents.AppendLine($"        return MUtils::Unbox<{returnType}>(__result);");
                }
            }

            contents.AppendLine("    }");
            contents.AppendLine();
        }

        private static string GenerateCppScriptVTable(BuildData buildData, StringBuilder contents, VirtualClassInfo classInfo)
        {
            var scriptVTableSize = classInfo.GetScriptVTableSize(out var scriptVTableOffset);
            if (scriptVTableSize == 0)
                return "nullptr, nullptr";

            var scriptVTableIndex = scriptVTableOffset;
            foreach (var functionInfo in classInfo.Functions)
            {
                if (!functionInfo.IsVirtual)
                    continue;
                GenerateCppManagedWrapperFunction(buildData, contents, classInfo, functionInfo, scriptVTableSize, scriptVTableIndex);
                GenerateCppScriptWrapperFunction?.Invoke(buildData, classInfo, functionInfo, scriptVTableSize, scriptVTableIndex, contents);
                scriptVTableIndex++;
            }

            CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MMethod.h");
            CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");

            foreach (var functionInfo in classInfo.Functions)
            {
                if (!functionInfo.IsVirtual)
                    continue;
                var thunkParams = string.Empty;
                var separator = false;
                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];
                    if (separator)
                        thunkParams += ", ";
                    separator = true;
                    thunkParams += parameterInfo.Type;
                }
                var t = functionInfo.IsConst ? " const" : string.Empty;
                contents.AppendLine($"    typedef {functionInfo.ReturnType} ({classInfo.NativeName}::*{functionInfo.UniqueName}_Signature)({thunkParams}){t};");
                if (buildData.Toolchain?.Compiler != TargetCompiler.Clang)
                {
                    // MSVC or other compiler
                    contents.AppendLine($"    typedef {functionInfo.ReturnType} ({classInfo.NativeName}Internal::*{functionInfo.UniqueName}_Internal_Signature)({thunkParams}){t};");
                }
            }
            contents.AppendLine("");

            contents.AppendLine("    static void SetupScriptVTable(MClass* mclass, void**& scriptVTable, void**& scriptVTableBase)");
            contents.AppendLine("    {");
            if (classInfo.IsInterface)
            {
                contents.AppendLine("        ASSERT(scriptVTable);");
            }
            else
            {
                contents.AppendLine("        if (!scriptVTable)");
                contents.AppendLine("        {");
                contents.AppendLine($"            scriptVTable = (void**)Platform::Allocate(sizeof(void*) * {scriptVTableSize + 1}, 16);");
                contents.AppendLine($"            Platform::MemoryClear(scriptVTable, sizeof(void*) * {scriptVTableSize + 1});");
                contents.AppendLine($"            scriptVTableBase = (void**)Platform::Allocate(sizeof(void*) * {scriptVTableSize + 2}, 16);");
                contents.AppendLine("        }");
            }
            scriptVTableIndex = scriptVTableOffset;
            foreach (var functionInfo in classInfo.Functions)
            {
                if (!functionInfo.IsVirtual)
                    continue;
                contents.AppendLine($"        scriptVTable[{scriptVTableIndex++}] = mclass->GetMethod(\"{functionInfo.Name}\", {functionInfo.Parameters.Count});");
            }
            contents.AppendLine("    }");
            contents.AppendLine("");

            contents.AppendLine("    static void SetupScriptObjectVTable(void** scriptVTable, void** scriptVTableBase, void** vtable, int32 entriesCount, int32 wrapperIndex)");
            contents.AppendLine("    {");
            scriptVTableIndex = scriptVTableOffset;
            foreach (var functionInfo in classInfo.Functions)
            {
                if (!functionInfo.IsVirtual)
                    continue;
                contents.AppendLine($"        if (scriptVTable[{scriptVTableIndex}])");
                contents.AppendLine("        {");
                contents.AppendLine($"            {functionInfo.UniqueName}_Signature funcPtr = &{classInfo.NativeName}::{functionInfo.Name};");
                contents.AppendLine("            const int32 vtableIndex = GetVTableIndex(vtable, entriesCount, *(void**)&funcPtr);");
                contents.AppendLine("            if (vtableIndex >= 0 && vtableIndex < entriesCount)");
                contents.AppendLine("            {");
                contents.AppendLine($"                scriptVTableBase[{scriptVTableIndex} + 2] = vtable[vtableIndex];");
                for (int i = 0, count = 0; i < ScriptingLangInfos.Count; i++)
                {
                    var langInfo = ScriptingLangInfos[i];
                    if (!langInfo.Enabled)
                        continue;
                    if (count == 0)
                        contents.AppendLine("                if (wrapperIndex == 0)");
                    else
                        contents.AppendLine($"                else if (wrapperIndex == {count})");
                    contents.AppendLine("                {");
                    contents.AppendLine($"                    auto thunkPtr = &{classInfo.NativeName}Internal::{functionInfo.UniqueName}{langInfo.VirtualWrapperMethodsPostfix};");
                    contents.AppendLine("                    vtable[vtableIndex] = *(void**)&thunkPtr;");
                    contents.AppendLine("                }");
                    count++;
                }
                contents.AppendLine("            }");
                contents.AppendLine("            else");
                contents.AppendLine("            {");
                contents.AppendLine($"                LOG(Error, \"Failed to find the vtable entry for method {{0}} in class {{1}}\", TEXT(\"{functionInfo.Name}\"), TEXT(\"{classInfo.Name}\"));");
                contents.AppendLine("            }");
                contents.AppendLine("        }");

                scriptVTableIndex++;
            }

            // Native interfaces override in managed code requires vtables hacking which requires additional inject on Clang-platforms
            if (buildData.Toolchain?.Compiler == TargetCompiler.Clang && classInfo.IsClass && classInfo.Interfaces != null)
            {
                // Override vtable entries of interface methods (for each virtual function in each interface)
                foreach (var interfaceInfo in classInfo.Interfaces)
                {
                    if (interfaceInfo.Access == AccessLevel.Private)
                        continue;
                    foreach (var functionInfo in interfaceInfo.Functions)
                    {
                        if (!functionInfo.IsVirtual)
                            continue;
                        contents.AppendLine("        {");
                        {
                            var thunkParams = string.Empty;
                            var separator = false;
                            for (var i = 0; i < functionInfo.Parameters.Count; i++)
                            {
                                var parameterInfo = functionInfo.Parameters[i];
                                if (separator)
                                    thunkParams += ", ";
                                separator = true;
                                thunkParams += parameterInfo.Type;
                            }
                            var t = functionInfo.IsConst ? " const" : string.Empty;
                            contents.AppendLine($"            typedef {functionInfo.ReturnType} ({classInfo.NativeName}::*{functionInfo.UniqueName}_Signature)({thunkParams}){t};");
                        }
                        contents.AppendLine($"            {functionInfo.UniqueName}_Signature funcPtr = &{classInfo.NativeName}::{functionInfo.Name};");
                        contents.AppendLine("            const int32 vtableIndex = GetVTableIndex(vtable, entriesCount, *(void**)&funcPtr);");
                        contents.AppendLine("            if (vtableIndex >= 0 && vtableIndex < entriesCount)");
                        contents.AppendLine("            {");
                        contents.AppendLine($"                extern void {interfaceInfo.NativeName}Internal_{functionInfo.UniqueName}_VTableOverride(void*& vtableEntry, int32 wrapperIndex);");
                        contents.AppendLine($"                {interfaceInfo.NativeName}Internal_{functionInfo.UniqueName}_VTableOverride(vtable[vtableIndex], wrapperIndex);");
                        contents.AppendLine("            }");
                        contents.AppendLine("            else");
                        contents.AppendLine("            {");
                        contents.AppendLine($"                LOG(Error, \"Failed to find the vtable entry for method {{0}} in class {{1}}\", TEXT(\"{functionInfo.Name}\"), TEXT(\"{classInfo.Name}\"));");
                        contents.AppendLine("            }");
                        contents.AppendLine("        }");
                    }
                }
            }

            contents.AppendLine("    }");
            contents.AppendLine("");

            return $"&{classInfo.NativeName}Internal::SetupScriptVTable, &{classInfo.NativeName}Internal::SetupScriptObjectVTable";
        }

        private static string GenerateCppAutoSerializationDefineType(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, ApiTypeInfo caller, TypeInfo memberType, MemberInfo member)
        {
            if (memberType.IsBitField)
                return "_BIT";
            return string.Empty;
        }

        private static bool GenerateCppAutoSerializationSkip(BuildData buildData, ApiTypeInfo caller, MemberInfo memberInfo, TypeInfo typeInfo)
        {
            if (memberInfo.IsStatic || memberInfo.IsConstexpr)
                return true;
            if (memberInfo.Access != AccessLevel.Public && !memberInfo.HasAttribute("Serialize"))
                return true;
            if (memberInfo.HasAttribute("NoSerialize") || memberInfo.HasAttribute("NonSerialized"))
                return true;
            var apiTypeInfo = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiTypeInfo != null && apiTypeInfo.IsInterface)
                return true;

            // Add includes to properly compile bindings (eg. SoftObjectReference<class Texture>)
            GenerateCppAddFileReference(buildData, caller, typeInfo, apiTypeInfo);

            // TODO: find a better way to reference other include files for types that have separate serialization header
            if (typeInfo.Type.EndsWith("Curve") && typeInfo.GenericArgs != null)
                CppIncludeFilesList.Add("Engine/Animations/CurveSerialization.h");

            return false;
        }

        private static void GenerateCppAutoSerialization(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, ApiTypeInfo typeInfo, string typeNameNative)
        {
            var classInfo = typeInfo as ClassInfo;
            var structureInfo = typeInfo as StructureInfo;
            var baseType = classInfo?.BaseType ?? structureInfo?.BaseType;
            if (classInfo != null && classInfo.IsBaseTypeHidden)
                baseType = null;
            if (baseType != null && (baseType.Name == "PersistentScriptingObject" || baseType.Name == "ScriptingObject" || baseType.Name == "ManagedScriptingObject"))
                baseType = null;
            CppAutoSerializeFields.Clear();
            CppAutoSerializeProperties.Clear();
            CppIncludeFiles.Add("Engine/Serialization/Serialization.h");

            contents.AppendLine();
            contents.Append($"void {typeNameNative}::Serialize(SerializeStream& stream, const void* otherObj)").AppendLine();
            contents.Append('{').AppendLine();
            if (baseType != null)
                contents.Append($"    {baseType.FullNameNative}::Serialize(stream, otherObj);").AppendLine();
            contents.Append($"    SERIALIZE_GET_OTHER_OBJ({typeNameNative});").AppendLine();

            if (classInfo != null)
            {
                foreach (var fieldInfo in classInfo.Fields)
                {
                    if (GenerateCppAutoSerializationSkip(buildData, typeInfo, fieldInfo, fieldInfo.Type))
                        continue;

                    var typeHint = GenerateCppAutoSerializationDefineType(buildData, contents, moduleInfo, typeInfo, fieldInfo.Type, fieldInfo);
                    contents.Append($"    SERIALIZE{typeHint}({fieldInfo.Name});").AppendLine();
                    CppAutoSerializeFields.Add(fieldInfo);
                }

                foreach (var propertyInfo in classInfo.Properties)
                {
                    if (propertyInfo.Getter == null || propertyInfo.Setter == null)
                        continue;
                    if (GenerateCppAutoSerializationSkip(buildData, typeInfo, propertyInfo, propertyInfo.Type))
                        continue;
                    CppAutoSerializeProperties.Add(propertyInfo);

                    // Skip writing deprecated properties (read-only deserialization to allow reading old data)
                    if (propertyInfo.HasAttribute("Obsolete"))
                        continue;

                    contents.Append("    {");
                    contents.Append(" const auto");
                    if (propertyInfo.Getter.ReturnType.IsConstRef)
                        contents.Append('&');
                    contents.Append($" value = {propertyInfo.Getter.Name}();");
                    contents.Append(" if (other) { const auto");
                    if (propertyInfo.Getter.ReturnType.IsConstRef)
                        contents.Append('&');
                    contents.Append($" otherValue = other->{propertyInfo.Getter.Name}();");
                    contents.Append(" if (Serialization::ShouldSerialize(value, &otherValue)) { ");
                    contents.Append($"stream.JKEY(\"{propertyInfo.Name}\"); Serialization::Serialize(stream, value, &otherValue);");
                    contents.Append(" } } else if (Serialization::ShouldSerialize(value, nullptr)) { ");
                    contents.Append($"stream.JKEY(\"{propertyInfo.Name}\"); Serialization::Serialize(stream, value, nullptr);");
                    contents.Append(" }");
                    contents.Append('}').AppendLine();
                }
            }
            else if (structureInfo != null)
            {
                foreach (var fieldInfo in structureInfo.Fields)
                {
                    if (GenerateCppAutoSerializationSkip(buildData, typeInfo, fieldInfo, fieldInfo.Type))
                        continue;

                    var typeHint = GenerateCppAutoSerializationDefineType(buildData, contents, moduleInfo, typeInfo, fieldInfo.Type, fieldInfo);
                    contents.Append($"    SERIALIZE{typeHint}({fieldInfo.Name});").AppendLine();
                    CppAutoSerializeFields.Add(fieldInfo);
                }
            }

            contents.Append('}').AppendLine();

            contents.AppendLine();
            contents.Append($"void {typeNameNative}::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)").AppendLine();
            contents.Append('{').AppendLine();
            if (baseType != null)
                contents.Append($"    {baseType.FullNameNative}::Deserialize(stream, modifier);").AppendLine();

            foreach (var fieldInfo in CppAutoSerializeFields)
            {
                var typeHint = GenerateCppAutoSerializationDefineType(buildData, contents, moduleInfo, typeInfo, fieldInfo.Type, fieldInfo);
                contents.Append($"    DESERIALIZE{typeHint}({fieldInfo.Name});").AppendLine();
            }

            foreach (var propertyInfo in CppAutoSerializeProperties)
            {
                contents.AppendLine("    {");
                contents.AppendLine($"        const auto e = SERIALIZE_FIND_MEMBER(stream, \"{propertyInfo.Name}\");");
                contents.AppendLine("        if (e != stream.MemberEnd()) {");
                contents.AppendLine($"            auto p = {propertyInfo.Getter.Name}();");
                contents.AppendLine("            Serialization::Deserialize(e->value, p, modifier);");
                contents.AppendLine($"            {propertyInfo.Setter.Name}(p);");
                contents.AppendLine("        }");
                contents.AppendLine("    }");
            }

            contents.Append('}').AppendLine();
        }

        private static string GenerateCppInterfaceInheritanceTable(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, ClassStructInfo typeInfo, string typeNameNative, string typeNameInternal)
        {
            var interfacesPtr = "nullptr";
            var interfaces = typeInfo.Interfaces;
            if (interfaces != null)
            {
                var virtualTypeInfo = typeInfo as VirtualClassInfo;
                interfacesPtr = typeNameInternal + "_Interfaces";
                contents.Append("static const ScriptingType::InterfaceImplementation ").Append(interfacesPtr).AppendLine("[] = {");
                for (int i = 0; i < interfaces.Count; i++)
                {
                    var interfaceInfo = interfaces[i];
                    var scriptVTableOffset = virtualTypeInfo?.GetScriptVTableOffset(interfaceInfo) ?? 0;
                    contents.AppendLine($"    {{ &{interfaceInfo.NativeName}::TypeInitializer, (int16)VTABLE_OFFSET({typeNameNative}, {interfaceInfo.NativeName}), {scriptVTableOffset}, true }},");
                }
                contents.AppendLine("    { nullptr, 0 },");
                contents.AppendLine("};");
            }
            return interfacesPtr;
        }

        private static void GenerateCppClass(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, ClassInfo classInfo)
        {
            var classTypeNameNative = classInfo.FullNameNative;
            var classTypeNameManaged = classInfo.FullNameManaged;
            var classTypeNameManagedInternalCall = classTypeNameManaged.Replace('+', '/');
            var classTypeNameInternal = classInfo.FullNameNativeInternal;
            var internalTypeName = classTypeNameInternal + "Internal";
            var useScripting = classInfo.IsStatic || classInfo.IsScriptingObject;
            var useCSharp = EngineConfiguration.WithCSharp(buildData.TargetOptions);
            var hasInterface = classInfo.Interfaces != null && classInfo.Interfaces.Any(x => x.Access == AccessLevel.Public);
            CppInternalCalls.Clear();

            if (classInfo.IsAutoSerialization)
                GenerateCppAutoSerialization(buildData, contents, moduleInfo, classInfo, classTypeNameNative);
            GenerateCppTypeInternalsStatics?.Invoke(buildData, classInfo, contents);

            contents.AppendLine();
            contents.Append("class ").Append(internalTypeName).AppendLine();
            contents.Append('{').AppendLine();
            contents.AppendLine("public:");

            // Events
            foreach (var eventInfo in classInfo.Events)
            {
                if (!useScripting || eventInfo.IsHidden)
                    continue;
                var paramsCount = eventInfo.Type.GenericArgs?.Count ?? 0;
                CppIncludeFiles.Add("Engine/Profiler/ProfilerCPU.h");
                var bindPrefix = eventInfo.IsStatic ? classTypeNameNative + "::" : "__obj->";

                if (useCSharp)
                {
                    // C# event invoking wrapper (calls C# event from C++ delegate)
                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MEvent.h");
                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                    contents.Append("    ");
                    if (eventInfo.IsStatic)
                        contents.Append("static ");
                    contents.AppendFormat("void {0}_ManagedWrapper(", eventInfo.Name);
                    for (var i = 0; i < paramsCount; i++)
                    {
                        if (i != 0)
                            contents.Append(", ");
                        contents.Append(eventInfo.Type.GenericArgs[i].GetFullNameNative(buildData, classInfo)).Append(" arg" + i);
                    }
                    contents.Append(')').AppendLine();
                    contents.Append("    {").AppendLine();
                    contents.Append("        static MMethod* method = nullptr;").AppendLine();
                    contents.AppendFormat("        if (!method) {{ method = {1}::TypeInitializer.GetClass()->GetMethod(\"Internal_{0}_Invoke\", {2}); CHECK(method); }}", eventInfo.Name, classTypeNameNative, paramsCount).AppendLine();
                    contents.Append("        MObject* exception = nullptr;").AppendLine();
                    if (paramsCount == 0)
                        contents.AppendLine("        void** params = nullptr;");
                    else
                        contents.AppendLine($"        void* params[{paramsCount}];");
                    for (var i = 0; i < paramsCount; i++)
                    {
                        var paramType = eventInfo.Type.GenericArgs[i];
                        var paramName = "arg" + i;
                        var paramIsRef = paramType.IsRef && !paramType.IsConst;
                        var paramValue = GenerateCppWrapperNativeToManagedParam(buildData, contents, paramType, paramName, classInfo, paramIsRef, out CppParamsThatNeedConversion[i]);
                        contents.Append($"        params[{i}] = {paramValue};").AppendLine();
                    }
                    if (eventInfo.IsStatic)
                        contents.AppendLine("        MObject* instance = nullptr;");
                    else
                        contents.AppendLine($"        MObject* instance = (({classTypeNameNative}*)this)->GetManagedInstance();");
                    contents.Append("        method->Invoke(instance, params, &exception);").AppendLine();
                    contents.Append("        if (exception)").AppendLine();
                    contents.Append("            DebugLog::LogException(exception);").AppendLine();
                    for (var i = 0; i < paramsCount; i++)
                    {
                        var paramType = eventInfo.Type.GenericArgs[i];
                        if (paramType.IsRef && !paramType.IsConst)
                        {
                            // Convert value back from managed to native (could be modified there)
                            paramType.IsRef = false;
                            var managedToNative = GenerateCppWrapperManagedToNative(buildData, paramType, classInfo, out var managedType, out var apiType, null, out _);
                            var passAsParamPtr = managedType.EndsWith("*");
                            var useLocalVarPointer = CppParamsThatNeedConversion[i] && !apiType.IsValueType;
                            var paramValue = useLocalVarPointer ? $"*({managedType}{(passAsParamPtr ? "" : "*")}*)params[{i}]" : $"({managedType}{(passAsParamPtr ? "" : "*")})params[{i}]";
                            if (!string.IsNullOrEmpty(managedToNative))
                            {
                                if (!passAsParamPtr)
                                    paramValue = '*' + paramValue;
                                paramValue = string.Format(managedToNative, paramValue);
                            }
                            else if (!passAsParamPtr)
                                paramValue = '*' + paramValue;
                            contents.Append($"        arg{i} = {paramValue};").AppendLine();
                            paramType.IsRef = true;
                        }
                    }
                    contents.Append("    }").AppendLine().AppendLine();

                    // C# event wrapper binding method (binds/unbinds C# wrapper to C++ delegate)
                    CppInternalCalls.Add(new KeyValuePair<string, string>(eventInfo.Name + "_Bind", eventInfo.Name + "_ManagedBind"));
                    bool useSeparateImpl = false; // True if separate function declaration from implementation
                    contents.AppendFormat("    DLLEXPORT static void {0}_ManagedBind(", eventInfo.Name);
                    var signatureStart = contents.Length;
                    if (buildData.Toolchain?.Compiler == TargetCompiler.Clang)
                        useSeparateImpl = true; // DLLEXPORT doesn't properly export function thus separate implementation from declaration
                    if (!eventInfo.IsStatic)
                        contents.AppendFormat("{0}* __obj, ", classTypeNameNative);
                    contents.Append("bool bind)");
                    var contentsPrev = contents;
                    var indent = "    ";
                    if (useSeparateImpl)
                    {
                        // Write declaration only, function definition wil be put in the end of the file
                        CppContentsEnd.AppendFormat("void {1}::{0}_ManagedBind(", eventInfo.Name, internalTypeName);
                        var sig = contents.ToString(signatureStart, contents.Length - signatureStart);
                        CppContentsEnd.Append(contents.ToString(signatureStart, contents.Length - signatureStart));
                        contents.Append(';').AppendLine();
                        contents = CppContentsEnd;
                        indent = null;
                    }
                    contents.AppendLine().Append(indent).Append('{').AppendLine();
                    if (buildData.Toolchain?.Compiler == TargetCompiler.MSVC)
                        contents.Append(indent).AppendLine($"    MSVC_FUNC_EXPORT(\"{classTypeNameManaged}::Internal_{eventInfo.Name}_Bind\")"); // Export generated function binding under the C# name
                    if (!eventInfo.IsStatic)
                        contents.Append(indent).Append("    if (__obj == nullptr) return;").AppendLine();
                    contents.Append(indent).Append("    Function<void(");
                    for (var i = 0; i < paramsCount; i++)
                    {
                        if (i != 0)
                            contents.Append(", ");
                        contents.Append(eventInfo.Type.GenericArgs[i].GetFullNameNative(buildData, classInfo));
                    }
                    contents.Append(")> f;").AppendLine();
                    if (eventInfo.IsStatic)
                        contents.Append(indent).AppendFormat("    f.Bind<{0}_ManagedWrapper>();", eventInfo.Name).AppendLine();
                    else
                        contents.Append(indent).AppendFormat("    f.Bind<{1}, &{1}::{0}_ManagedWrapper>(({1}*)__obj);", eventInfo.Name, internalTypeName).AppendLine();
                    contents.Append(indent).Append("    if (bind)").AppendLine();
                    contents.Append(indent).AppendFormat("        {0}{1}.Bind(f);", bindPrefix, eventInfo.Name).AppendLine();
                    contents.Append(indent).Append("    else").AppendLine();
                    contents.Append(indent).AppendFormat("        {0}{1}.Unbind(f);", bindPrefix, eventInfo.Name).AppendLine();
                    contents.Append(indent).Append('}').AppendLine().AppendLine();
                    if (useSeparateImpl)
                        contents = contentsPrev;
                }

                // Generic scripting event invoking wrapper (calls scripting code from C++ delegate)
                CppIncludeFiles.Add("Engine/Scripting/Events.h");
                contents.Append("    ");
                if (eventInfo.IsStatic)
                    contents.Append("static ");
                contents.AppendFormat("void {0}_Wrapper(", eventInfo.Name);
                for (var i = 0; i < paramsCount; i++)
                {
                    if (i != 0)
                        contents.Append(", ");
                    contents.Append(eventInfo.Type.GenericArgs[i].GetFullNameNative(buildData, classInfo)).Append(" arg" + i);
                }
                contents.Append(')').AppendLine();
                contents.Append("    {").AppendLine();
                if (paramsCount == 0)
                    contents.AppendLine("        Variant* params = nullptr;");
                else
                    contents.AppendLine($"        Variant params[{paramsCount}];");
                for (var i = 0; i < paramsCount; i++)
                {
                    var paramType = eventInfo.Type.GenericArgs[i];
                    var paramName = "arg" + i;
                    var paramValue = GenerateCppWrapperNativeToVariant(buildData, paramType, classInfo, paramName);
                    contents.Append($"        params[{i}] = {paramValue};").AppendLine();
                }
                contents.AppendLine($"        ScriptingEvents::Event({(eventInfo.IsStatic ? "nullptr" : "(ScriptingObject*)this")}, Span<Variant>(params, {paramsCount}), {classTypeNameNative}::TypeInitializer, StringView(TEXT(\"{eventInfo.Name}\"), {eventInfo.Name.Length}));");
                contents.Append("    }").AppendLine().AppendLine();

                // Scripting event wrapper binding method (binds/unbinds generic wrapper to C++ delegate)
                contents.AppendFormat("    static void {0}_Bind(", eventInfo.Name);
                contents.AppendFormat("{0}* __obj, void* instance, bool bind)", classTypeNameNative).AppendLine();
                contents.Append("    {").AppendLine();
                contents.Append("        Function<void(");
                for (var i = 0; i < paramsCount; i++)
                {
                    if (i != 0)
                        contents.Append(", ");
                    contents.Append(eventInfo.Type.GenericArgs[i].GetFullNameNative(buildData, classInfo));
                }
                contents.Append(")> f;").AppendLine();
                if (eventInfo.IsStatic)
                    contents.AppendFormat("        f.Bind<{0}_Wrapper>();", eventInfo.Name).AppendLine();
                else
                    contents.AppendFormat("        f.Bind<{1}, &{1}::{0}_Wrapper>(({1}*)instance);", eventInfo.Name, internalTypeName).AppendLine();
                contents.Append("        if (bind)").AppendLine();
                contents.AppendFormat("            {0}{1}.Bind(f);", bindPrefix, eventInfo.Name).AppendLine();
                contents.Append("        else").AppendLine();
                contents.AppendFormat("            {0}{1}.Unbind(f);", bindPrefix, eventInfo.Name).AppendLine();
                contents.Append("    }").AppendLine().AppendLine();
            }

            // Fields
            foreach (var fieldInfo in classInfo.Fields)
            {
                if (!useScripting || !useCSharp || fieldInfo.IsHidden || fieldInfo.IsConstexpr)
                    continue;
                if (fieldInfo.Getter != null)
                    GenerateCppWrapperFunction(buildData, contents, classInfo, internalTypeName, fieldInfo.Getter, "{0}");
                if (fieldInfo.Setter != null)
                {
                    var callFormat = "{0} = {1}";
                    var type = fieldInfo.Setter.Parameters[0].Type;
                    if (type.IsArray)
                        callFormat = $"auto __tmp = {{1}}; for (int32 i = 0; i < {type.ArraySize}; i++) {{0}}[i] = __tmp[i]";
                    GenerateCppWrapperFunction(buildData, contents, classInfo, internalTypeName, fieldInfo.Setter, callFormat);
                }
            }

            // Properties
            foreach (var propertyInfo in classInfo.Properties)
            {
                if (!useScripting || !useCSharp || propertyInfo.IsHidden)
                    continue;
                if (propertyInfo.Getter != null)
                    GenerateCppWrapperFunction(buildData, contents, classInfo, internalTypeName, propertyInfo.Getter);
                if (propertyInfo.Setter != null)
                    GenerateCppWrapperFunction(buildData, contents, classInfo, internalTypeName, propertyInfo.Setter);
            }

            // Functions
            foreach (var functionInfo in classInfo.Functions)
            {
                if (!useCSharp || functionInfo.IsHidden)
                    continue;
                if (!useScripting)
                    throw new Exception($"Not supported function {functionInfo.Name} inside non-static and non-scripting class type {classInfo.Name}.");
                GenerateCppWrapperFunction(buildData, contents, classInfo, internalTypeName, functionInfo);
            }

            // Interface implementation
            if (hasInterface && useCSharp)
            {
                foreach (var interfaceInfo in classInfo.Interfaces)
                {
                    if (interfaceInfo.Access != AccessLevel.Public)
                        continue;
                    foreach (var functionInfo in interfaceInfo.Functions)
                    {
                        if (!classInfo.IsScriptingObject)
                            throw new Exception($"Class {classInfo.Name} cannot implement interface {interfaceInfo.Name} because it requires ScriptingObject as a base class.");
                        GenerateCppWrapperFunction(buildData, contents, classInfo, internalTypeName, functionInfo);
                    }
                }
            }

            GenerateCppTypeInternals?.Invoke(buildData, classInfo, contents);

            // Virtual methods overrides
            var setupScriptVTable = GenerateCppScriptVTable(buildData, contents, classInfo);

            // Runtime initialization (internal methods binding)
            contents.AppendLine("    static void InitRuntime()");
            contents.AppendLine("    {");
            if (useScripting)
            {
                foreach (var eventInfo in classInfo.Events)
                {
                    // Register scripting event binder
                    contents.AppendLine($"        ScriptingEvents::EventsTable[Pair<ScriptingTypeHandle, StringView>({classTypeNameNative}::TypeInitializer, StringView(TEXT(\"{eventInfo.Name}\"), {eventInfo.Name.Length}))] = (void(*)(ScriptingObject*, void*, bool)){classTypeNameInternal}Internal::{eventInfo.Name}_Bind;");
                }
            }
            if (useScripting && useCSharp)
            {
#if !USE_NETCORE
                foreach (var e in CppInternalCalls)
                {
                    contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{e.Key}\", &{e.Value});");
                }
#endif
            }
            GenerateCppTypeInitRuntime?.Invoke(buildData, classInfo, contents);

            contents.AppendLine("    }").AppendLine();

            if (!useScripting && !classInfo.IsAbstract)
            {
                // Constructor
                contents.AppendLine("    static void Ctor(void* ptr)");
                contents.AppendLine("    {");
                contents.AppendLine($"        new(ptr){classTypeNameNative}();");
                contents.AppendLine("    }").AppendLine();

                // Destructor
                contents.AppendLine("    static void Dtor(void* ptr)");
                contents.AppendLine("    {");
                contents.AppendLine($"        (({classTypeNameNative}*)ptr)->~{classInfo.NativeName}();");
                contents.AppendLine("    }").AppendLine();
            }

            contents.Append('}').Append(';').AppendLine();
            contents.AppendLine();

            // Interfaces
            var interfacesTable = GenerateCppInterfaceInheritanceTable(buildData, contents, moduleInfo, classInfo, classTypeNameNative, classTypeNameInternal);

            // Type initializer
            if (GenerateCppIsTemplateInstantiationType(classInfo))
                contents.Append("template<> ");
            contents.Append($"ScriptingTypeInitializer {classTypeNameNative}::TypeInitializer((BinaryModule*)GetBinaryModule{moduleInfo.Name}(), ");
            contents.Append($"StringAnsiView(\"{classTypeNameManaged}\", {classTypeNameManaged.Length}), ");
            contents.Append($"sizeof({classTypeNameNative}), ");
            contents.Append($"&{classTypeNameInternal}Internal::InitRuntime, ");
            if (useScripting)
            {
                if (classInfo.IsStatic || classInfo.NoSpawn)
                    contents.Append("&ScriptingType::DefaultSpawn, ");
                else
                    contents.Append($"(ScriptingType::SpawnHandler)&{classTypeNameNative}::Spawn, ");
                if (classInfo.BaseType != null)
                    contents.Append($"&{classInfo.BaseType.FullNameNative}::TypeInitializer, ");
                else
                    contents.Append("nullptr, ");
                contents.Append(setupScriptVTable);
            }
            else
            {
                if (classInfo.IsAbstract)
                    contents.Append("nullptr, nullptr, ");
                else
                    contents.Append($"&{classTypeNameInternal}Internal::Ctor, &{classTypeNameInternal}Internal::Dtor, ");
                if (classInfo.BaseType != null)
                    contents.Append($"&{classInfo.BaseType.FullNameNative}::TypeInitializer");
                else
                    contents.Append("nullptr");
            }
            contents.Append(", ").Append(interfacesTable).Append(");").AppendLine();

            // Nested types
            foreach (var apiTypeInfo in classInfo.Children)
            {
                GenerateCppType(buildData, contents, moduleInfo, apiTypeInfo);
            }
        }

        private static void GenerateCppStruct(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, StructureInfo structureInfo)
        {
            var structureTypeNameNative = structureInfo.FullNameNative;
            var structureTypeNameManaged = structureInfo.FullNameManaged;
            var structureTypeNameManagedInternalCall = structureTypeNameManaged.Replace('+', '/');
            var structureTypeNameInternal = structureInfo.FullNameNativeInternal;
            var internalTypeName = structureTypeNameInternal + "Internal";
            var useCSharp = EngineConfiguration.WithCSharp(buildData.TargetOptions);
            CppInternalCalls.Clear();

            if (structureInfo.IsAutoSerialization)
                GenerateCppAutoSerialization(buildData, contents, moduleInfo, structureInfo, structureTypeNameNative);
            GenerateCppTypeInternalsStatics?.Invoke(buildData, structureInfo, contents);

            contents.AppendLine();
            contents.AppendFormat("class {0}Internal", structureTypeNameInternal).AppendLine();
            contents.Append('{').AppendLine();
            contents.AppendLine("public:");

            // Fields
            foreach (var fieldInfo in structureInfo.Fields)
            {
                if (fieldInfo.IsConstexpr)
                    continue;

                // Static fields are using C++ static value accessed via getter function binding
                if (fieldInfo.IsStatic)
                {
                    fieldInfo.Getter = new FunctionInfo
                    {
                        Access = fieldInfo.Access,
                        IsStatic = true,
                        Parameters = new List<FunctionInfo.ParameterInfo>(),
                        ReturnType = fieldInfo.Type,
                        Name = fieldInfo.Name,
                        UniqueName = "Get" + fieldInfo.Name,
                    };

                    if (!fieldInfo.IsReadOnly)
                    {
                        fieldInfo.Setter = new FunctionInfo
                        {
                            Access = fieldInfo.Access,
                            IsStatic = true,
                            Parameters = new List<FunctionInfo.ParameterInfo>
                            {
                                new FunctionInfo.ParameterInfo
                                {
                                    Type = fieldInfo.Type,
                                    Name = "value",
                                },
                            },
                            ReturnType = new TypeInfo
                            {
                                Type = "void",
                            },
                            Name = fieldInfo.Name,
                            UniqueName = "Set" + fieldInfo.Name,
                        };
                    }
                }

                if (fieldInfo.Getter != null)
                    GenerateCppWrapperFunction(buildData, contents, structureInfo, internalTypeName, fieldInfo.Getter, "{0}");
                if (fieldInfo.Setter != null)
                    GenerateCppWrapperFunction(buildData, contents, structureInfo, internalTypeName, fieldInfo.Setter, "{0} = {1}");
            }

            // Functions
            foreach (var functionInfo in structureInfo.Functions)
            {
                // TODO: add support for API functions in structures
                throw new NotImplementedException($"TODO: add support for API functions in structures (function {functionInfo} in structure {structureInfo.Name})");
                //GenerateCppWrapperFunction(buildData, contents, structureInfo, structureTypeNameInternal, functionInfo);
            }

            GenerateCppTypeInternals?.Invoke(buildData, structureInfo, contents);

            contents.AppendLine("    static void InitRuntime()");
            contents.AppendLine("    {");

            if (useCSharp)
            {
#if !USE_NETCORE
                foreach (var e in CppInternalCalls)
                {
                    contents.AppendLine($"        ADD_INTERNAL_CALL(\"{structureTypeNameManagedInternalCall}::Internal_{e.Key}\", &{e.Value});");
                }
#endif
            }
            GenerateCppTypeInitRuntime?.Invoke(buildData, structureInfo, contents);

            contents.AppendLine("    }").AppendLine();

            // Constructor for structures
            contents.AppendLine("    static void Ctor(void* ptr)");
            contents.AppendLine("    {");
            contents.AppendLine($"        new(ptr){structureTypeNameNative}();");
            contents.AppendLine("    }").AppendLine();

            // Destructor for structures
            contents.AppendLine("    static void Dtor(void* ptr)");
            contents.AppendLine("    {");
            contents.AppendLine($"        (({structureTypeNameNative}*)ptr)->~{structureInfo.NativeName}();");
            contents.AppendLine("    }").AppendLine();

            // Copy operator for structures
            contents.AppendLine("    static void Copy(void* dst, void* src)");
            contents.AppendLine("    {");
            contents.AppendLine($"        *({structureTypeNameNative}*)dst = *({structureTypeNameNative}*)src;");
            contents.AppendLine("    }").AppendLine();

            if (useCSharp)
            {
                // Boxing structures from native data to managed object
                CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                if (!structureInfo.IsPod && !CppUsedNonPodTypes.Contains(structureInfo))
                    CppUsedNonPodTypes.Add(structureInfo);
                contents.AppendLine("    static MObject* Box(void* ptr)");
                contents.AppendLine("    {");
                if (structureInfo.MarshalAs != null)
                    contents.AppendLine($"        MISSING_CODE(\"Boxing native type {structureInfo.Name} as {structureInfo.MarshalAs}\"); return nullptr;"); // TODO: impl this
                else if (structureInfo.IsPod)
                    contents.AppendLine($"        return MCore::Object::Box(ptr, {structureTypeNameNative}::TypeInitializer.GetClass());");
                else
                    contents.AppendLine($"        return MUtils::Box(*({structureTypeNameNative}*)ptr, {structureTypeNameNative}::TypeInitializer.GetClass());");
                contents.AppendLine("    }").AppendLine();

                // Unboxing structures from managed object to native data
                contents.AppendLine("    static void Unbox(void* ptr, MObject* managed)");
                contents.AppendLine("    {");
                if (structureInfo.MarshalAs != null)
                    contents.AppendLine($"        MISSING_CODE(\"Boxing native type {structureInfo.Name} as {structureInfo.MarshalAs}\");"); // TODO: impl this
                else if (structureInfo.IsPod)
                    contents.AppendLine($"        Platform::MemoryCopy(ptr, MCore::Object::Unbox(managed), sizeof({structureTypeNameNative}));");
                else
                    contents.AppendLine($"        *({structureTypeNameNative}*)ptr = ToNative(*({GenerateCppManagedWrapperName(structureInfo)}*)MCore::Object::Unbox(managed));");
                contents.AppendLine("    }").AppendLine();
            }
            else
            {
                contents.AppendLine("    static MObject* Box(void* ptr)");
                contents.AppendLine("    {");
                contents.AppendLine("        return nullptr;");
                contents.AppendLine("    }").AppendLine();
                contents.AppendLine("    static void Unbox(void* ptr, MObject* managed)");
                contents.AppendLine("    {");
                contents.AppendLine("    }").AppendLine();
            }

            // Getter for structure field
            contents.AppendLine("    static void GetField(void* ptr, const String& name, Variant& value)");
            contents.AppendLine("    {");
            for (int i = 0, count = 0; i < structureInfo.Fields.Count; i++)
            {
                var fieldInfo = structureInfo.Fields[i];
                if (fieldInfo.IsReadOnly || fieldInfo.IsStatic || fieldInfo.IsConstexpr || fieldInfo.Access == AccessLevel.Private)
                    continue;
                if (count == 0)
                    contents.AppendLine($"        if (name == TEXT(\"{fieldInfo.Name}\"))");
                else
                    contents.AppendLine($"        else if (name == TEXT(\"{fieldInfo.Name}\"))");
                contents.AppendLine($"            value = {GenerateCppWrapperNativeToVariant(buildData, fieldInfo.Type, structureInfo, $"(({structureTypeNameNative}*)ptr)->{fieldInfo.Name}")};");
                count++;
            }
            contents.AppendLine("    }").AppendLine();

            // Setter for structure field
            contents.AppendLine("    static void SetField(void* ptr, const String& name, const Variant& value)");
            contents.AppendLine("    {");
            for (int i = 0, count = 0; i < structureInfo.Fields.Count; i++)
            {
                var fieldInfo = structureInfo.Fields[i];
                if (fieldInfo.IsReadOnly || fieldInfo.IsStatic || fieldInfo.IsConstexpr || fieldInfo.Access == AccessLevel.Private)
                    continue;
                if (count == 0)
                    contents.AppendLine($"        if (name == TEXT(\"{fieldInfo.Name}\"))");
                else
                    contents.AppendLine($"        else if (name == TEXT(\"{fieldInfo.Name}\"))");
                if (fieldInfo.Type.IsArray)
                {
                    // Fixed-size array need a special converting to unpack from Variant
                    contents.AppendLine("        {");
                    contents.AppendLine("            CHECK(value.Type.Type == VariantType::Array);");
                    contents.AppendLine("            auto* array = reinterpret_cast<const Array<Variant, HeapAllocation>*>(value.AsData);");
                    contents.AppendLine($"            for (int32 i = 0; i < {fieldInfo.Type.ArraySize} && i < array->Count(); i++)");
                    contents.AppendLine($"                (({structureTypeNameNative}*)ptr)->{fieldInfo.Name}[i] = *({fieldInfo.Type}*)array->At(i).AsBlob.Data;");
                    contents.AppendLine("        }");
                }
                else
                    contents.AppendLine($"            (({structureTypeNameNative}*)ptr)->{fieldInfo.Name} = {GenerateCppWrapperVariantToNative(buildData, fieldInfo.Type, structureInfo, "value")};");
                count++;
            }
            contents.AppendLine("    }");

            contents.Append('}').Append(';').AppendLine();
            contents.AppendLine();

            // Interfaces
            var interfacesTable = GenerateCppInterfaceInheritanceTable(buildData, contents, moduleInfo, structureInfo, structureTypeNameNative, structureTypeNameInternal);

            // Type initializer
            if (GenerateCppIsTemplateInstantiationType(structureInfo))
                contents.Append("template<> ");
            contents.Append($"ScriptingTypeInitializer {structureTypeNameNative}::TypeInitializer((BinaryModule*)GetBinaryModule{moduleInfo.Name}(), ");
            contents.Append($"StringAnsiView(\"{structureTypeNameManaged}\", {structureTypeNameManaged.Length}), ");
            contents.Append($"sizeof({structureTypeNameNative}), ");
            contents.Append($"&{structureTypeNameInternal}Internal::InitRuntime, ");
            contents.Append($"&{structureTypeNameInternal}Internal::Ctor, &{structureTypeNameInternal}Internal::Dtor, &{structureTypeNameInternal}Internal::Copy, &{structureTypeNameInternal}Internal::Box, &{structureTypeNameInternal}Internal::Unbox, &{structureTypeNameInternal}Internal::GetField, &{structureTypeNameInternal}Internal::SetField, ");
            if (structureInfo.BaseType != null)
                contents.Append($"&{structureInfo.BaseType.FullNameNative}::TypeInitializer");
            else
                contents.Append("nullptr");
            contents.Append(", ").Append(interfacesTable).Append(");").AppendLine();

            // Nested types
            foreach (var apiTypeInfo in structureInfo.Children)
            {
                GenerateCppType(buildData, contents, moduleInfo, apiTypeInfo);
            }
        }

        private static void GenerateCppEnum(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, EnumInfo enumInfo)
        {
            var enumTypeNameNative = enumInfo.FullNameNative;
            var enumTypeNameManaged = enumInfo.FullNameManaged;
            var enumTypeNameInternal = enumInfo.FullNameNativeInternal;

            contents.AppendLine();
            contents.AppendFormat("class {0}Internal", enumTypeNameInternal).AppendLine();
            contents.Append('{').AppendLine();
            contents.AppendLine("public:");
            contents.AppendLine("    static ScriptingType::EnumItem Items[];");
            contents.Append('}').Append(';').AppendLine();
            contents.AppendLine();

            // Items
            contents.AppendFormat("ScriptingType::EnumItem {0}Internal::Items[] = {{", enumTypeNameInternal).AppendLine();
            foreach (var entry in enumInfo.Entries)
                contents.AppendFormat("    {{ (uint64){0}::{1}, \"{1}\" }},", enumTypeNameNative, entry.Name).AppendLine();
            contents.AppendLine("    { 0, nullptr }");
            contents.AppendLine("};").AppendLine();

            contents.Append($"ScriptingTypeInitializer {enumTypeNameInternal}_TypeInitializer((BinaryModule*)GetBinaryModule{moduleInfo.Name}(), ");
            contents.Append($"StringAnsiView(\"{enumTypeNameManaged}\", {enumTypeNameManaged.Length}), ");
            contents.Append($"sizeof({enumTypeNameNative}), ");
            contents.Append($"{enumTypeNameInternal}Internal::Items);").AppendLine();

            contents.AppendLine($"template<> {moduleInfo.Name.ToUpperInvariant()}_API ScriptingTypeHandle StaticType<{enumTypeNameNative}>() {{ return {enumTypeNameInternal}_TypeInitializer; }}");
        }

        private static void GenerateCppInterface(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, InterfaceInfo interfaceInfo)
        {
            var interfaceTypeNameNative = interfaceInfo.FullNameNative;
            var interfaceTypeNameManaged = interfaceInfo.FullNameManaged;
            var interfaceTypeNameInternal = interfaceInfo.FullNameNativeInternal;

            GenerateCppTypeInternalsStatics?.Invoke(buildData, interfaceInfo, contents);

            // Wrapper interface implement to invoke scripting if inherited in C# or VS
            contents.AppendLine();
            contents.AppendFormat("class {0}Wrapper : public ", interfaceTypeNameInternal).Append(interfaceTypeNameNative).AppendLine();
            contents.Append('{').AppendLine();
            contents.AppendLine("public:");
            contents.AppendLine("    ScriptingObject* Object;");
            foreach (var functionInfo in interfaceInfo.Functions)
            {
                if (!functionInfo.IsVirtual)
                    continue;
                contents.AppendFormat("    {0} {1}(", functionInfo.ReturnType, functionInfo.Name);
                var separator = false;
                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];
                    if (separator)
                        contents.Append(", ");
                    separator = true;
                    contents.Append(parameterInfo.Type).Append(' ').Append(parameterInfo.Name);
                }
                contents.Append(')');
                if (functionInfo.IsConst)
                    contents.Append(" const");
                contents.Append(" override").AppendLine();
                contents.AppendLine("    {");
                // TODO: try to use ScriptVTable for interfaces implementation in scripting to call proper function instead of manually check at runtime
                if (functionInfo.Parameters.Count != 0)
                {
                    contents.AppendLine($"        Variant parameters[{functionInfo.Parameters.Count}];");
                    for (var i = 0; i < functionInfo.Parameters.Count; i++)
                    {
                        var parameterInfo = functionInfo.Parameters[i];
                        contents.AppendLine($"        parameters[{i}] = {GenerateCppWrapperNativeToVariant(buildData, parameterInfo.Type, interfaceInfo, parameterInfo.Name)};");
                    }
                }
                else
                {
                    contents.AppendLine("        Variant* parameters = nullptr;");
                }
                contents.AppendLine("        auto typeHandle = Object->GetTypeHandle();");
                contents.AppendLine("        while (typeHandle)");
                contents.AppendLine("        {");
                contents.AppendLine($"            auto method = typeHandle.Module->FindMethod(typeHandle, StringAnsiView(\"{functionInfo.Name}\", {functionInfo.Name.Length}), {functionInfo.Parameters.Count});");
                contents.AppendLine("            if (method)");
                contents.AppendLine("            {");
                contents.AppendLine("                Variant __result;");
                contents.AppendLine($"                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(parameters, {functionInfo.Parameters.Count}), __result);");

                // Convert parameter values back from scripting to native (could be modified there)
                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];
                    var paramIsRef = parameterInfo.IsRef || parameterInfo.IsOut;
                    if (paramIsRef && !parameterInfo.Type.IsConst)
                    {
                        contents.AppendLine($"                {parameterInfo.Name} = {GenerateCppWrapperVariantToNative(buildData, parameterInfo.Type, interfaceInfo, $"parameters[{i}]")};");
                    }
                }

                if (functionInfo.ReturnType.IsVoid)
                    contents.AppendLine("                return;");
                else
                    contents.AppendLine($"                return {GenerateCppWrapperVariantToNative(buildData, functionInfo.ReturnType, interfaceInfo, "__result")};");
                contents.AppendLine("            }");
                contents.AppendLine("            typeHandle = typeHandle.GetType().GetBaseType();");
                contents.AppendLine("        }");
                GenerateCppReturn(buildData, contents, "        ", functionInfo.ReturnType);
                contents.AppendLine("    }");
            }
            if (interfaceInfo.Name == "ISerializable")
            {
                // TODO: how to handle other interfaces that have some abstract native methods? maybe NativeOnly tag on interface? do it right and remove this hack
                contents.AppendLine("    void Serialize(SerializeStream& stream, const void* otherObj) override {} void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override {}");
            }
            contents.Append('}').Append(';').AppendLine();

            contents.AppendLine();
            contents.AppendFormat("class {0}Internal", interfaceTypeNameInternal).AppendLine();
            contents.Append('{').AppendLine();
            contents.AppendLine("public:");

            GenerateCppTypeInternals?.Invoke(buildData, interfaceInfo, contents);

            // Virtual methods overrides
            var setupScriptVTable = GenerateCppScriptVTable(buildData, contents, interfaceInfo);

            // Runtime initialization (internal methods binding)
            contents.AppendLine("    static void InitRuntime()");
            contents.AppendLine("    {");
            GenerateCppTypeInitRuntime?.Invoke(buildData, interfaceInfo, contents);
            contents.AppendLine("    }").AppendLine();

            // Interface implementation wrapper accessor for scripting types
            contents.AppendLine("    static void* GetInterfaceWrapper(ScriptingObject* __obj)");
            contents.AppendLine("    {");
            contents.AppendLine($"        auto wrapper = New<{interfaceTypeNameInternal}Wrapper>();");
            contents.AppendLine("        wrapper->Object = __obj;");
            contents.AppendLine("        return wrapper;");
            contents.AppendLine("    }");

            contents.Append('}').Append(';').AppendLine();
            contents.AppendLine();

            // Native interfaces override in managed code requires vtables hacking which requires additional inject on Clang-platforms
            if (buildData.Toolchain?.Compiler == TargetCompiler.Clang)
            {
                // Generate functions that inject script wrappers into vtable entry
                foreach (var functionInfo in interfaceInfo.Functions)
                {
                    if (!functionInfo.IsVirtual)
                        continue;
                    contents.AppendLine($"void {interfaceInfo.NativeName}Internal_{functionInfo.UniqueName}_VTableOverride(void*& vtableEntry, int32 wrapperIndex)");
                    contents.AppendLine("{");
                    for (int i = 0, count = 0; i < ScriptingLangInfos.Count; i++)
                    {
                        var langInfo = ScriptingLangInfos[i];
                        if (!langInfo.Enabled)
                            continue;
                        if (count == 0)
                            contents.AppendLine("    if (wrapperIndex == 0)");
                        else
                            contents.AppendLine($"    else if (wrapperIndex == {count})");
                        contents.AppendLine("    {");
                        contents.AppendLine($"        auto thunkPtr = &{interfaceInfo.NativeName}Internal::{functionInfo.UniqueName}{langInfo.VirtualWrapperMethodsPostfix};");
                        contents.AppendLine("        vtableEntry = *(void**)&thunkPtr;");
                        contents.AppendLine("    }");
                        count++;
                    }
                    contents.AppendLine("}");
                    contents.AppendLine("");
                }
            }

            // Type initializer
            contents.Append($"ScriptingTypeInitializer {interfaceTypeNameNative}::TypeInitializer((BinaryModule*)GetBinaryModule{moduleInfo.Name}(), ");
            contents.Append($"StringAnsiView(\"{interfaceTypeNameManaged}\", {interfaceTypeNameManaged.Length}), &{interfaceTypeNameInternal}Internal::InitRuntime,");
            contents.Append(setupScriptVTable).Append($", &{interfaceTypeNameInternal}Internal::GetInterfaceWrapper").Append(");");
            contents.AppendLine();

            // Nested types
            foreach (var apiTypeInfo in interfaceInfo.Children)
            {
                GenerateCppType(buildData, contents, moduleInfo, apiTypeInfo);
            }
        }

        private static void GenerateCppType(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, object type)
        {
            if (type is ApiTypeInfo apiTypeInfo && apiTypeInfo.SkipGeneration)
                return;

            try
            {
                if (type is ClassInfo classInfo)
                    GenerateCppClass(buildData, contents, moduleInfo, classInfo);
                else if (type is StructureInfo structureInfo)
                    GenerateCppStruct(buildData, contents, moduleInfo, structureInfo);
                else if (type is EnumInfo enumInfo)
                    GenerateCppEnum(buildData, contents, moduleInfo, enumInfo);
                else if (type is InterfaceInfo interfaceInfo)
                    GenerateCppInterface(buildData, contents, moduleInfo, interfaceInfo);
                else if (type is InjectCodeInfo injectCodeInfo && string.Equals(injectCodeInfo.Lang, "cpp", StringComparison.OrdinalIgnoreCase))
                    contents.AppendLine(injectCodeInfo.Code);
            }
            catch
            {
                Log.Error($"Failed to generate C++ bindings for {type}.");
                throw;
            }
        }

        private static void GenerateCppCppUsedNonPodTypes(BuildData buildData, ApiTypeInfo apiType)
        {
            if (CppUsedNonPodTypesList.Contains(apiType))
                return;
            if (apiType is ClassStructInfo classStructInfo)
            {
                if (classStructInfo.MarshalAs != null)
                    return;
                if (classStructInfo.IsTemplate)
                    throw new Exception($"Cannot use template type '{classStructInfo}' as non-POD type for cross-language bindings.");
            }
            if (apiType is StructureInfo structureInfo)
            {
                // Check all fields (if one of them is also non-POD structure then we need to generate wrappers for them too)
                for (var i = 0; i < structureInfo.Fields.Count; i++)
                {
                    var fieldInfo = structureInfo.Fields[i];
                    if (fieldInfo.IsStatic || fieldInfo.IsConstexpr)
                        continue;
                    var fieldApiType = FindApiTypeInfo(buildData, fieldInfo.Type, structureInfo);
                    if (fieldApiType != null && fieldApiType.IsStruct && !fieldApiType.IsPod)
                        GenerateCppCppUsedNonPodTypes(buildData, fieldApiType);
                    if (fieldInfo.Type.GenericArgs != null)
                    {
                        foreach (var fieldTypeArg in fieldInfo.Type.GenericArgs)
                        {
                            fieldApiType = FindApiTypeInfo(buildData, fieldTypeArg, structureInfo);
                            if (fieldApiType != null && fieldApiType.IsStruct && !fieldApiType.IsPod)
                                GenerateCppCppUsedNonPodTypes(buildData, fieldApiType);
                        }
                    }
                }
            }
            CppUsedNonPodTypesList.Add(apiType);
        }

        private static void GenerateCpp(BuildData buildData, ModuleInfo moduleInfo, ref BindingsResult bindings)
        {
            var contents = GetStringBuilder();
            CppContentsEnd = GetStringBuilder();
            CppUsedNonPodTypes.Clear();
            CppReferencesFiles.Clear();
            CppIncludeFiles.Clear();
            CppIncludeFilesList.Clear();
            CppVariantToTypes.Clear();
            CppVariantFromTypes.Clear();
            CurrentModule = moduleInfo;

            // Disable C# scripting based on configuration

            ScriptingLangInfos[0].Enabled = EngineConfiguration.WithCSharp(buildData.TargetOptions);

            contents.AppendLine("// This code was auto-generated. Do not modify it.");
            contents.AppendLine();
            contents.AppendLine("#include \"Engine/Core/Compiler.h\"");
            contents.AppendLine("PRAGMA_DISABLE_DEPRECATION_WARNINGS"); // Disable deprecated warnings in generated code
            contents.AppendLine("#include \"Engine/Scripting/Scripting.h\"");
            contents.AppendLine("#include \"Engine/Scripting/Internal/InternalCalls.h\"");
            contents.AppendLine("#include \"Engine/Scripting/ManagedCLR/MUtils.h\"");
            contents.AppendLine("#include \"Engine/Scripting/ManagedCLR/MCore.h\"");
            contents.AppendLine($"#include \"{moduleInfo.Name}.Gen.h\"");
            for (int i = 0; i < moduleInfo.Children.Count; i++)
            {
                if (moduleInfo.Children[i] is FileInfo fileInfo)
                {
                    CppReferencesFiles.Add(fileInfo);
                }
            }
            var headerPos = contents.Length;

            foreach (var child in moduleInfo.Children)
            {
                foreach (var apiTypeInfo in child.Children)
                {
                    GenerateCppType(buildData, contents, moduleInfo, apiTypeInfo);
                }
            }

            GenerateCppModuleSource?.Invoke(buildData, moduleInfo, contents);

            {
                var header = GetStringBuilder();

                // Variant converting helper methods
                foreach (var e in CppVariantToTypes)
                {
                    var wrapperName = e.Key;
                    var typeInfo = e.Value;
                    var name = typeInfo.ToString(false);
                    header.AppendLine();
                    header.AppendLine("namespace {");
                    header.Append($"{name} VariantTo{wrapperName}(const Variant& v)").AppendLine();
                    header.Append('{').AppendLine();
                    header.Append($"    {name} result;").AppendLine();
                    if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
                    {
                        header.Append("    const auto* array = reinterpret_cast<const Array<Variant, HeapAllocation>*>(v.AsData);").AppendLine();
                        header.Append("    const int32 length = v.Type.Type == VariantType::Array ? array->Count() : 0;").AppendLine();
                        header.Append("    result.Resize(length);").AppendLine();
                        header.Append("    for (int32 i = 0; i < length; i++)").AppendLine();
                        header.Append($"        result[i] = {GenerateCppWrapperVariantToNative(buildData, typeInfo.GenericArgs[0], moduleInfo, "array->At(i)")};").AppendLine();
                    }
                    else if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
                    {
                        header.Append("    const auto* dictionary = v.AsDictionary;").AppendLine();
                        header.Append("    if (dictionary)").AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        result.EnsureCapacity(dictionary->Count());").AppendLine();
                        header.Append("        for (auto& e : *dictionary)").AppendLine();
                        header.Append($"            result.Add({GenerateCppWrapperVariantToNative(buildData, typeInfo.GenericArgs[0], moduleInfo, "e.Key")}, {GenerateCppWrapperVariantToNative(buildData, typeInfo.GenericArgs[1], moduleInfo, "e.Value")});").AppendLine();
                        header.Append("    }").AppendLine();
                    }
                    else
                        throw new NotSupportedException($"Invalid type {typeInfo} to unpack from Variant.");
                    header.Append("    return result;").AppendLine();
                    header.Append('}').AppendLine();
                    header.AppendLine("}");
                }
                foreach (var e in CppVariantFromTypes)
                {
                    var wrapperName = e.Key;
                    var typeInfo = e.Value;
                    header.AppendLine();
                    header.AppendLine("namespace {");
                    header.Append("Variant VariantFrom");
                    if (typeInfo.IsArray)
                    {
                        typeInfo.IsArray = false;
                        header.Append($"{wrapperName}Array(const {typeInfo}* v, const int32 length)").AppendLine();
                        header.Append('{').AppendLine();
                        header.Append("    Variant result;").AppendLine();
                        header.Append("    result.SetType(VariantType(VariantType::Array));").AppendLine();
                        header.Append("    auto* array = reinterpret_cast<Array<Variant, HeapAllocation>*>(result.AsData);").AppendLine();
                        header.Append("    array->Resize(length);").AppendLine();
                        header.Append("    for (int32 i = 0; i < length; i++)").AppendLine();
                        header.Append($"        array->At(i) = {GenerateCppWrapperNativeToVariant(buildData, typeInfo, moduleInfo, "v[i]")};").AppendLine();
                        typeInfo.IsArray = true;
                    }
                    else if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
                    {
                        var valueType = typeInfo.GenericArgs[0];
                        header.Append($"{wrapperName}Array(const {valueType}* v, const int32 length)").AppendLine();
                        header.Append('{').AppendLine();
                        header.Append("    Variant result;").AppendLine();
                        header.Append("    result.SetType(VariantType(VariantType::Array));").AppendLine();
                        header.Append("    auto* array = reinterpret_cast<Array<Variant, HeapAllocation>*>(result.AsData);").AppendLine();
                        header.Append("    array->Resize(length);").AppendLine();
                        header.Append("    for (int32 i = 0; i < length; i++)").AppendLine();
                        header.Append($"        array->At(i) = {GenerateCppWrapperNativeToVariant(buildData, valueType, moduleInfo, "v[i]")};").AppendLine();
                    }
                    else if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
                    {
                        var keyType = typeInfo.GenericArgs[0];
                        var valueType = typeInfo.GenericArgs[1];
                        header.Append($"{wrapperName}Dictionary(const Dictionary<{keyType}, {valueType}>& v)").AppendLine();
                        header.Append('{').AppendLine();
                        header.Append("    Variant result;").AppendLine();
                        header.Append("    result.SetType(VariantType(VariantType::Dictionary));").AppendLine();
                        header.Append("    auto* dictionary = result.AsDictionary;").AppendLine();
                        header.Append("    dictionary->EnsureCapacity(v.Count());").AppendLine();
                        header.Append("    for (auto& e : v)").AppendLine();
                        header.Append($"        dictionary->Add({GenerateCppWrapperNativeToVariant(buildData, keyType, moduleInfo, "e.Key")}, {GenerateCppWrapperNativeToVariant(buildData, valueType, moduleInfo, "e.Value")});").AppendLine();
                    }
                    else
                        throw new NotSupportedException($"Invalid type {typeInfo} to pack to Variant.");
                    header.Append("    return result;").AppendLine();
                    header.Append('}').AppendLine();
                    header.AppendLine("}");
                }

                // Non-POD types
                CppUsedNonPodTypesList.Clear();
                if (EngineConfiguration.WithCSharp(buildData.TargetOptions))
                {
                    for (int k = 0; k < CppUsedNonPodTypes.Count; k++)
                        GenerateCppCppUsedNonPodTypes(buildData, CppUsedNonPodTypes[k]);
                }
                foreach (var apiType in CppUsedNonPodTypesList)
                {
                    header.AppendLine();
                    var wrapperName = GenerateCppManagedWrapperName(apiType);
                    var structureInfo = apiType as StructureInfo;
                    var classInfo = apiType as ClassInfo;
                    List<FieldInfo> fields;
                    if (structureInfo != null)
                        fields = structureInfo.Fields;
                    else if (classInfo != null)
                        fields = classInfo.Fields;
                    else
                        throw new Exception("Not supported Non-POD type " + apiType);
                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");

                    // Get the full typename with nested parent prefix
                    var fullName = apiType.FullNameNative;

                    if (structureInfo != null)
                    {
                        // Generate managed type memory layout
                        header.Append("struct ").Append(wrapperName).AppendLine();
                        header.Append('{').AppendLine();
                        if (classInfo != null)
                            header.AppendLine("    MObject obj;");
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic)
                                continue;
                            string type;

                            if (fieldInfo.NoArray && fieldInfo.Type.IsArray)
                            {
                                // Fixed-size array that needs to be inlined into structure instead of passing it as managed array
                                fieldInfo.Type.IsArray = false;
                                CppParamsWrappersCache[i] = GenerateCppWrapperNativeToManaged(buildData, fieldInfo.Type, apiType, out type, null);
                                fieldInfo.Type.IsArray = true;
                                header.AppendFormat("    {0} {1}[{2}];", type, fieldInfo.Name, fieldInfo.Type.ArraySize).AppendLine();
                                continue;
                            }

                            CppParamsWrappersCache[i] = GenerateCppWrapperNativeToManaged(buildData, fieldInfo.Type, apiType, out type, null);
                            header.AppendFormat("    {0} {1};", type, fieldInfo.Name).AppendLine();
                        }
                        header.Append('}').Append(';').AppendLine();

                        // Generate forward declarations of structure converting functions
                        header.AppendLine();
                        header.AppendLine("namespace {");
                        header.AppendFormat("{0} ToManaged(const {1}& value);", wrapperName, fullName).AppendLine();
                        header.AppendFormat("{1} ToNative(const {0}& value);", wrapperName, fullName).AppendLine();
                        header.AppendLine("}");

                        // Generate MConverter for a structure
                        header.Append("template<>").AppendLine();
                        header.AppendFormat("struct MConverter<{0}>", fullName).AppendLine();
                        header.Append('{').AppendLine();
                        header.AppendFormat("    MObject* Box(const {0}& data, const MClass* klass)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        auto managed = ToManaged(data);").AppendLine();
                        header.Append("        return MCore::Object::Box((void*)&managed, klass);").AppendLine();
                        header.Append("    }").AppendLine();
                        header.AppendFormat("    void Unbox({0}& result, MObject* data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        result = ToNative(*reinterpret_cast<{0}*>(MCore::Object::Unbox(data)));", wrapperName).AppendLine();
                        header.Append("    }").AppendLine();
                        header.AppendFormat("    void ToManagedArray(MArray* result, const Span<{0}>& data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        MClass* klass = {0}::TypeInitializer.GetClass();", fullName).AppendLine();
                        header.AppendFormat("        {0}* resultPtr = ({0}*)MCore::Array::GetAddress(result);", wrapperName).AppendLine();
                        header.Append("        for (int32 i = 0; i < data.Length(); i++)").AppendLine();
                        header.Append("        {").AppendLine();
                        header.Append("        	auto managed = ToManaged(data[i]);").AppendLine();
                        header.Append("        	MCore::GC::WriteValue(&resultPtr[i], &managed, 1, klass);").AppendLine();
                        header.Append("        }").AppendLine();
                        header.Append("    }").AppendLine();
                        header.AppendFormat("    void ToNativeArray(Span<{0}>& result, const MArray* data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        {0}* dataPtr = ({0}*)MCore::Array::GetAddress(data);", wrapperName).AppendLine();
                        header.Append("        for (int32 i = 0; i < result.Length(); i++)").AppendLine();
                        header.Append("    	    result[i] = ToNative(dataPtr[i]);").AppendLine();
                        header.Append("    }").AppendLine();
                        header.Append('}').Append(';').AppendLine();

                        // Generate converting function native -> managed
                        header.AppendLine();
                        header.AppendLine("namespace {");
                        header.AppendFormat("{0} ToManaged(const {1}& value)", wrapperName, fullName).AppendLine();
                        header.Append('{').AppendLine();
                        header.AppendFormat("    {0} result;", wrapperName).AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic || fieldInfo.IsConstexpr)
                                continue;

                            if (fieldInfo.NoArray && fieldInfo.Type.IsArray)
                            {
                                // Fixed-size array needs to unbox every item manually if not using managed array
                                if (fieldInfo.Type.IsPod(buildData, apiType))
                                    header.AppendFormat("    Platform::MemoryCopy(result.{0}, value.{0}, sizeof({2}) * {1});", fieldInfo.Name, fieldInfo.Type.ArraySize, fieldInfo.Type).AppendLine();
                                else
                                    header.AppendFormat("    for (int32 i = 0; i < {0}; i++)", fieldInfo.Type.ArraySize).AppendLine().AppendFormat("        result.{0}[i] = value.{0}[i];", fieldInfo.Name).AppendLine();
                                continue;
                            }

                            var wrapper = CppParamsWrappersCache[i];
                            header.AppendFormat("    result.{0} = ", fieldInfo.Name);
                            if (string.IsNullOrEmpty(wrapper))
                                header.Append("value." + fieldInfo.Name);
                            else
                                header.AppendFormat(wrapper, "value." + fieldInfo.Name);
                            header.Append(';').AppendLine();
                        }
                        header.Append("    return result;").AppendLine();
                        header.Append('}').AppendLine();

                        // Generate converting function managed -> native
                        header.AppendLine();
                        header.AppendFormat("{1} ToNative(const {0}& value)", wrapperName, fullName).AppendLine();
                        header.Append('{').AppendLine();
                        header.AppendFormat("    {0} result;", fullName).AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic || fieldInfo.IsConstexpr)
                                continue;

                            CppNonPodTypesConvertingGeneration = true;
                            var wrapper = GenerateCppWrapperManagedToNative(buildData, fieldInfo.Type, apiType, out _, out _, null, out _);
                            CppNonPodTypesConvertingGeneration = false;

                            if (fieldInfo.Type.IsArray)
                            {
                                // Fixed-size array needs to unbox every item manually
                                if (fieldInfo.NoArray)
                                {
                                    if (fieldInfo.Type.IsPod(buildData, apiType))
                                        header.AppendFormat("    Platform::MemoryCopy(result.{0}, value.{0}, sizeof({2}) * {1});", fieldInfo.Name, fieldInfo.Type.ArraySize, fieldInfo.Type).AppendLine();
                                    else
                                        header.AppendFormat("    for (int32 i = 0; i < {0}; i++)", fieldInfo.Type.ArraySize).AppendLine().AppendFormat("        result.{0}[i] = value.{0}[i];", fieldInfo.Name).AppendLine();
                                }
                                else
                                {
                                    wrapper = string.Format(wrapper.Remove(wrapper.Length - 6), string.Format("value.{0}", fieldInfo.Name));
                                    header.AppendFormat("    auto tmp{0} = {1};", fieldInfo.Name, wrapper).AppendLine();
                                    header.AppendFormat("    for (int32 i = 0; i < {0} && i < tmp{1}.Count(); i++)", fieldInfo.Type.ArraySize, fieldInfo.Name).AppendLine();
                                    header.AppendFormat("        result.{0}[i] = tmp{0}[i];", fieldInfo.Name).AppendLine();
                                }
                                continue;
                            }

                            if (string.IsNullOrEmpty(wrapper))
                                header.AppendFormat("    result.{0} = value.{0};", fieldInfo.Name).AppendLine();
                            else
                                header.AppendFormat("    result.{0} = {1};", fieldInfo.Name, string.Format(wrapper, string.Format("value.{0}", fieldInfo.Name))).AppendLine();
                        }
                        header.Append("    return result;").AppendLine();
                        header.Append('}').AppendLine();
                        header.AppendLine("}");
                    }
                    else if (classInfo != null)
                    {
                        // Generate MConverter for a class
                        header.Append("template<>").AppendLine();
                        header.AppendFormat("struct MConverter<{0}>", fullName).AppendLine();
                        header.Append('{').AppendLine();

                        header.AppendFormat("    static MObject* Box(const {0}& data, const MClass* klass)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        MObject* obj = MCore::Object::New(klass);").AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic || fieldInfo.IsConstexpr)
                                continue;

                            var fieldType = FindApiTypeInfo(buildData, fieldInfo.Type, apiType);
                            if (fieldInfo == null || fieldType.IsValueType) // TODO: support any value type (eg. by boxing)
                                throw new Exception($"Not supported field {fieldInfo.Type} {fieldInfo.Name} in class {classInfo.Name}.");

                            var wrapper = GenerateCppWrapperNativeToManaged(buildData, fieldInfo.Type, apiType, out var type, null);
                            var value = string.IsNullOrEmpty(wrapper) ? "data." + fieldInfo.Name : string.Format(wrapper, "data." + fieldInfo.Name);
                            header.AppendFormat("        klass->GetField(\"{0}\")->SetValue(obj, {1});", fieldInfo.Name, value).AppendLine();
                        }
                        header.Append("        return obj;").AppendLine();
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    static MObject* Box(const {0}& data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        MClass* klass = {0}::TypeInitializer.GetClass();", fullName).AppendLine();
                        header.Append("        return Box(data, klass);").AppendLine();
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    static void Unbox({0}& result, MObject* obj)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        MClass* klass = MCore::Object::GetClass(obj);").AppendLine();
                        header.Append("        void* v = nullptr;").AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic || fieldInfo.IsConstexpr)
                                continue;

                            CppNonPodTypesConvertingGeneration = true;
                            var wrapper = GenerateCppWrapperManagedToNative(buildData, fieldInfo.Type, apiType, out var type, out _, null, out _);
                            CppNonPodTypesConvertingGeneration = false;

                            CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MField.h");

                            header.AppendFormat("        klass->GetField(\"{0}\")->GetValue(obj, &v);", fieldInfo.Name).AppendLine();
                            var value = $"({type})v";
                            header.AppendFormat("        result.{0} = {1};", fieldInfo.Name, string.IsNullOrEmpty(wrapper) ? value : string.Format(wrapper, value)).AppendLine();
                        }
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    static {0} Unbox(MObject* data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        {0} result;", fullName).AppendLine();
                        header.Append("        Unbox(result, data);").AppendLine();
                        header.Append("        return result;").AppendLine();
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    void ToManagedArray(MArray* result, const Span<{0}>& data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        for (int32 i = 0; i < data.Length(); i++)").AppendLine();
                        header.Append("            MCore::GC::WriteArrayRef(result, Box(data[i]), i);").AppendLine();
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    void ToNativeArray(Span<{0}>& result, const MArray* data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        MObject** dataPtr = (MObject**)MCore::Array::GetAddress(data);").AppendLine();
                        header.Append("        for (int32 i = 0; i < result.Length(); i++)").AppendLine();
                        header.AppendFormat("            Unbox(result[i], dataPtr[i]);", fullName).AppendLine();
                        header.Append("    }").AppendLine();

                        header.Append('}').Append(';').AppendLine();
                    }
                }

                contents.Insert(headerPos, header.ToString());

                // Includes
                header.Clear();
                CppReferencesFiles.Remove(null);
                foreach (var fileInfo in CppReferencesFiles)
                    CppIncludeFilesList.Add(fileInfo.Name);
                CppIncludeFilesList.AddRange(CppIncludeFiles);
                CppIncludeFilesList.Sort();
                if (CppIncludeFilesList.Remove("Engine/Serialization/Serialization.h"))
                    CppIncludeFilesList.Add("Engine/Serialization/Serialization.h"); // Include serialization header as the last one to properly handle specialization of custom types serialization
                foreach (var path in CppIncludeFilesList)
                    header.AppendFormat("#include \"{0}\"", path).AppendLine();
                contents.Insert(headerPos, header.ToString());

                PutStringBuilder(header);
            }

            if (CppContentsEnd.Length != 0)
            {
                contents.AppendLine().Append(CppContentsEnd);
            }
            PutStringBuilder(CppContentsEnd);

            contents.AppendLine("PRAGMA_ENABLE_DEPRECATION_WARNINGS");

            Utilities.WriteFileIfChanged(bindings.GeneratedCppFilePath, contents.ToString());
            PutStringBuilder(contents);
        }

        private static void GenerateCpp(BuildData buildData, IGrouping<string, Module> binaryModule)
        {
            // Skip generating C++ bindings code for C#-only modules
            if (binaryModule.Any(x => !x.BuildNativeCode))
                return;
            var useCSharp = binaryModule.Any(x => x.BuildCSharp);

            var contents = GetStringBuilder();
            var binaryModuleName = binaryModule.Key;
            var binaryModuleNameUpper = binaryModuleName.ToUpperInvariant();
            var project = Builder.GetModuleProject(binaryModule.First(), buildData);
            var version = project.Version;

            // Generate C++ binary module header
            var binaryModuleHeaderPath = Path.Combine(project.ProjectFolderPath, "Source", binaryModuleName + ".Gen.h");
            contents.AppendLine("// This code was auto-generated. Do not modify it.");
            contents.AppendLine();
            contents.AppendLine("#pragma once");
            contents.AppendLine();
            contents.AppendLine($"#define {binaryModuleNameUpper}_NAME \"{binaryModuleName}\"");
            if (version.Build <= 0)
                contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION Version({version.Major}, {version.Minor})");
            else if (version.Revision <= 0)
                contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION Version({version.Major}, {version.Minor}, {version.Build})");
            else
                contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION Version({version.Major}, {version.Minor}, {version.Build}, {version.Revision})");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_TEXT \"{version}\"");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_MAJOR {version.Major}");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_MINOR {version.Minor}");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_BUILD {version.Build}");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_REVISION {version.Revision}");
            contents.AppendLine($"#define {binaryModuleNameUpper}_COMPANY \"{project.Company}\"");
            contents.AppendLine($"#define {binaryModuleNameUpper}_COPYRIGHT \"{project.Copyright}\"");
            contents.AppendLine();
            contents.AppendLine("class BinaryModule;");
            contents.AppendLine($"extern \"C\" {binaryModuleNameUpper}_API BinaryModule* GetBinaryModule{binaryModuleName}();");
            GenerateCppBinaryModuleHeader?.Invoke(buildData, binaryModule, contents);
            Utilities.WriteFileIfChanged(binaryModuleHeaderPath, contents.ToString());

            // Generate C++ binary module implementation
            contents.Clear();
            var binaryModuleSourcePath = Path.Combine(project.ProjectFolderPath, "Source", binaryModuleName + ".Gen.cpp");
            contents.AppendLine("// This code was auto-generated. Do not modify it.");
            contents.AppendLine();
            contents.AppendLine("#include \"Engine/Scripting/BinaryModule.h\"");
            contents.AppendLine($"#include \"{binaryModuleName}.Gen.h\"");
            contents.AppendLine();
            contents.AppendLine($"StaticallyLinkedBinaryModuleInitializer StaticallyLinkedBinaryModule{binaryModuleName}(GetBinaryModule{binaryModuleName});");
            contents.AppendLine();
            contents.AppendLine($"extern \"C\" BinaryModule* GetBinaryModule{binaryModuleName}()");
            contents.AppendLine("{");
            if (useCSharp)
            {
                contents.AppendLine($"    static NativeBinaryModule module(\"{binaryModuleName}\");");
            }
            else
            {
                contents.AppendLine($"    static NativeOnlyBinaryModule module(\"{binaryModuleName}\");");
            }
            contents.AppendLine("    return &module;");
            contents.AppendLine("}");
            GenerateCppBinaryModuleSource?.Invoke(buildData, binaryModule, contents);
            Utilities.WriteFileIfChanged(binaryModuleSourcePath, contents.ToString());
            PutStringBuilder(contents);
        }
    }
}

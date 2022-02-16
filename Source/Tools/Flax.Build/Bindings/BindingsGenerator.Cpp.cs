// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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
        public static readonly List<ApiTypeInfo> CppUsedNonPodTypes = new List<ApiTypeInfo>();
        private static readonly List<ApiTypeInfo> CppUsedNonPodTypesList = new List<ApiTypeInfo>();
        public static readonly HashSet<FileInfo> CppReferencesFiles = new HashSet<FileInfo>();
        private static readonly List<FieldInfo> CppAutoSerializeFields = new List<FieldInfo>();
        private static readonly List<PropertyInfo> CppAutoSerializeProperties = new List<PropertyInfo>();
        public static readonly HashSet<string> CppIncludeFiles = new HashSet<string>();
        private static readonly List<string> CppIncludeFilesList = new List<string>();
        private static readonly HashSet<TypeInfo> CppVariantToTypes = new HashSet<TypeInfo>();
        private static readonly HashSet<TypeInfo> CppVariantFromTypes = new HashSet<TypeInfo>();
        private static bool CppNonPodTypesConvertingGeneration = false;

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
        public static event Action<BuildData, VirtualClassInfo, StringBuilder> GenerateCppClassInternals;
        public static event Action<BuildData, VirtualClassInfo, StringBuilder> GenerateCppClassInitRuntime;
        public static event Action<BuildData, VirtualClassInfo, FunctionInfo, int, int, StringBuilder> GenerateCppScriptWrapperFunction;

        private static readonly List<string> CppInBuildVariantStructures = new List<string>
        {
            "Vector2",
            "Vector3",
            "Vector4",
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

        private static string GenerateCppWrapperNativeToVariantMethodName(TypeInfo typeInfo)
        {
            var sb = new StringBuilder();
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
            return sb.ToString();
        }

        private static string GenerateCppWrapperNativeToManagedParam(BuildData buildData, StringBuilder contents, TypeInfo paramType, string paramName, ApiTypeInfo caller, bool isOut)
        {
            var nativeToManaged = GenerateCppWrapperNativeToManaged(buildData, paramType, caller, out var managedTypeAsNative, null);
            string result;
            if (!string.IsNullOrEmpty(nativeToManaged))
            {
                result = string.Format(nativeToManaged, paramName);
                if (managedTypeAsNative[managedTypeAsNative.Length - 1] == '*')
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
                }
            }
            else
            {
                result = paramName;
                if (paramType.IsRef && !paramType.IsConst && !isOut)
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

        public static string GenerateCppWrapperNativeToVariant(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, string value)
        {
            if (typeInfo.Type == "Variant")
                return value;
            if (typeInfo.Type == "String")
                return $"Variant(StringView({value}))";
            if (typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference" || typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "SoftObjectReference")
                return $"Variant({value}.Get())";
            if (typeInfo.IsArray)
            {
                CppVariantFromTypes.Add(typeInfo);
                return $"VariantFrom{GenerateCppWrapperNativeToVariantMethodName(typeInfo)}Array({value}, {typeInfo.ArraySize})";
            }
            if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
            {
                CppVariantFromTypes.Add(typeInfo);
                return $"VariantFrom{GenerateCppWrapperNativeToVariantMethodName(typeInfo.GenericArgs[0])}Array({value}.Get(), {value}.Count())";
            }
            if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
            {
                CppVariantFromTypes.Add(typeInfo);
                return $"VariantFrom{GenerateCppWrapperNativeToVariantMethodName(typeInfo.GenericArgs[0])}{GenerateCppWrapperNativeToVariantMethodName(typeInfo.GenericArgs[1])}Dictionary({value})";
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
            if (typeInfo.IsPtr && typeInfo.IsConst && typeInfo.Type == "Char")
                return $"((StringView){value}).GetNonTerminatedText()"; // (StringView)Variant, if not empty, is guaranteed to point to a null-terminated buffer.
            if (typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference")
                return $"ScriptingObject::Cast<{typeInfo.GenericArgs[0].Type}>((Asset*){value})";
            if (typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "SoftObjectReference")
                return $"ScriptingObject::Cast<{typeInfo.GenericArgs[0].Type}>((ScriptingObject*){value})";
            if (typeInfo.IsArray)
                throw new Exception($"Not supported type to convert from the Variant to fixed-size array '{typeInfo}[{typeInfo.ArraySize}]'.");
            if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
            {
                CppVariantToTypes.Add(typeInfo);
                return $"MoveTemp(VariantTo{GenerateCppWrapperNativeToVariantMethodName(typeInfo)}({value}))";
            }
            if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
            {
                CppVariantToTypes.Add(typeInfo);
                return $"MoveTemp(VariantTo{GenerateCppWrapperNativeToVariantMethodName(typeInfo)}({value}))";
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
                    if (typeInfo.IsPtr)
                        return $"({apiType.FullNameNative}*){value}.AsBlob.Data";
                    else
                        return $"*({apiType.FullNameNative}*){value}.AsBlob.Data";
            }

            if (typeInfo.IsPtr)
                return $"({typeInfo})(void*){value}";
            return $"({typeInfo}){value}";
        }

        private static string GenerateCppGetNativeClass(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, FunctionInfo functionInfo)
        {
            // Optimal path for in-build types
            var managedType = GenerateCSharpNativeToManaged(buildData, typeInfo, caller);
            switch (managedType)
            {
            case "bool": return "mono_get_boolean_class()";
            case "sbyte": return "mono_get_sbyte_class()";
            case "byte": return "mono_get_byte_class()";
            case "short": return "mono_get_int16_class()";
            case "ushort": return "mono_get_uint16_class()";
            case "int": return "mono_get_int32_class()";
            case "uint": return "mono_get_uint32_class()";
            case "long": return "mono_get_int64_class()";
            case "ulong": return "mono_get_uint64_class()";
            case "float": return "mono_get_single_class()";
            case "double": return "mono_get_double_class()";
            case "string": return "mono_get_string_class()";
            case "object": return "mono_get_object_class()";
            case "void": return "mono_get_void_class()";
            case "char": return "mono_get_char_class()";
            case "IntPtr": return "mono_get_intptr_class()";
            case "UIntPtr": return "mono_get_uintptr_class()";
            }

            // Find API type
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                CppReferencesFiles.Add(apiType.File);
                if (apiType.IsStruct && !apiType.IsPod && !CppUsedNonPodTypes.Contains(apiType))
                    CppUsedNonPodTypes.Add(apiType);
                if (!apiType.IsInBuild && !apiType.IsEnum)
                {
                    // Use declared type initializer
                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                    return $"{apiType.FullNameNative}::TypeInitializer.GetType().ManagedClass->GetNative()";
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
                        Type = "MonoReflectionType",
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
            return "Scripting::FindClassNative(\"" + managedType + "\")";
        }

        private static string GenerateCppGetNativeType(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, FunctionInfo functionInfo)
        {
            // Optimal path for in-build types
            var managedType = GenerateCSharpNativeToManaged(buildData, typeInfo, caller);
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
            case "UIntPtr": return "mono_class_get_type(" + GenerateCppGetNativeClass(buildData, typeInfo, caller, null) + ')';
            }

            // Find API type
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                CppReferencesFiles.Add(apiType.File);
                if (apiType.IsStruct && !apiType.IsPod && !CppUsedNonPodTypes.Contains(apiType))
                    CppUsedNonPodTypes.Add(apiType);
                if (!apiType.IsInBuild && !apiType.IsEnum)
                {
                    // Use declared type initializer
                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                    return $"mono_class_get_type({apiType.FullNameNative}::TypeInitializer.GetType().ManagedClass->GetNative())";
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
                        Type = "MonoReflectionType",
                        IsPtr = true,
                    },
                };
                functionInfo.Glue.CustomParameters.Add(customParam);
                return "mono_reflection_type_get_type(" + customParam.Name + ')';
            }

            // Convert MonoClass* into MonoType*
            return "mono_class_get_type(" + GenerateCppGetNativeClass(buildData, typeInfo, caller, null) + ')';
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
                type = "MonoString*";
                return "MUtils::ToString({0})";
            case "Variant":
                type = "MonoObject*";
                return "MUtils::BoxVariant({0})";
            case "VariantType":
                type = "MonoReflectionType*";
                return "MUtils::BoxVariantType({0})";
            case "ScriptingObject":
            case "ManagedScriptingObject":
            case "PersistentScriptingObject":
                type = "MonoObject*";
                return "ScriptingObject::ToManaged((ScriptingObject*){0})";
            case "MClass":
                type = "MonoReflectionType*";
                return "MUtils::GetType({0})";
            case "CultureInfo":
                type = "void*";
                return "MUtils::ToManaged({0})";
            default:
                // ScriptingObjectReference or AssetReference or WeakAssetReference or SoftObjectReference
                if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference" || typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
                {
                    type = "MonoObject*";
                    return "{0}.GetManagedInstance()";
                }

                // Array or Span
                if ((typeInfo.Type == "Array" || typeInfo.Type == "Span") && typeInfo.GenericArgs != null)
                {
                    type = "MonoArray*";
                    return "MUtils::ToArray({0}, " + GenerateCppGetNativeClass(buildData, typeInfo.GenericArgs[0], caller, functionInfo) + ")";
                }

                // BytesContainer
                if (typeInfo.Type == "BytesContainer" && typeInfo.GenericArgs == null)
                {
                    type = "MonoArray*";
                    return "MUtils::ToArray({0})";
                }

                // Dictionary
                if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
                {
                    CppIncludeFiles.Add("Engine/Scripting/InternalCalls/ManagedDictionary.h");
                    type = "MonoObject*";
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
                    CppIncludeFiles.Add("Engine/Scripting/InternalCalls/ManagedBitArray.h");
                    type = "MonoObject*";
                    return "ManagedBitArray::ToManaged({0})";
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

                    // Scripting Object
                    if (apiType.IsScriptingObject)
                    {
                        type = "MonoObject*";
                        return "ScriptingObject::ToManaged((ScriptingObject*){0})";
                    }

                    // interface
                    if (apiType.IsInterface)
                    {
                        type = "MonoObject*";
                        return "ScriptingObject::ToManaged(ScriptingObject::FromInterface({0}, " + apiType.NativeName + "::TypeInitializer))";
                    }

                    // Non-POD structure passed as value (eg. it contains string or array inside)
                    if (apiType.IsStruct && !apiType.IsPod)
                    {
                        // Use wrapper structure that represents the memory layout of the managed data
                        if (!CppUsedNonPodTypes.Contains(apiType))
                            CppUsedNonPodTypes.Add(apiType);
                        if (functionInfo != null)
                            type = apiType.Name + "Managed*";
                        else
                            type = apiType.Name + "Managed";
                        return "ToManaged({0})";
                    }

                    // Managed class
                    if (apiType.IsClass)
                    {
                        // Use wrapper structure that represents the memory layout of the managed data
                        if (!CppUsedNonPodTypes.Contains(apiType))
                        {
                            CppUsedNonPodTypes.Add(apiType);
                            CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                        }
                        type = "MonoObject*";
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

        private static string GenerateCppWrapperManagedToNative(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, out string type, FunctionInfo functionInfo, out bool needLocalVariable)
        {
            needLocalVariable = false;

            // Register any API types usage
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            CppReferencesFiles.Add(apiType?.File);
            if (typeInfo.GenericArgs != null)
            {
                for (int i = 0; i < typeInfo.GenericArgs.Count; i++)
                {
                    var t = FindApiTypeInfo(buildData, typeInfo.GenericArgs[i], caller);
                    CppReferencesFiles.Add(t?.File);
                }
            }

            // Use dynamic array as wrapper container for fixed-size native arrays
            if (typeInfo.IsArray)
            {
                var arrayType = new TypeInfo { Type = "Array", GenericArgs = new List<TypeInfo> { new TypeInfo(typeInfo) { IsArray = false } } };
                var result = GenerateCppWrapperManagedToNative(buildData, arrayType, caller, out type, functionInfo, out needLocalVariable);
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
                type = "MonoString*";
                return "String(MUtils::ToString({0}))";
            case "StringView":
                type = "MonoString*";
                return "MUtils::ToString({0})";
            case "StringAnsi":
            case "StringAnsiView":
                type = "MonoString*";
                return "MUtils::ToStringAnsi({0})";
            case "Variant":
                type = "MonoObject*";
                return "MUtils::UnboxVariant({0})";
            case "VariantType":
                type = "MonoReflectionType*";
                return "MUtils::UnboxVariantType({0})";
            case "CultureInfo":
                type = "void*";
                return "MUtils::ToNative({0})";
            default:
                // ScriptingObjectReference or AssetReference or WeakAssetReference or SoftObjectReference
                if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference" || typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
                {
                    // For non-pod types converting only, other API converts managed to unmanaged object in C# wrapper code)
                    if (CppNonPodTypesConvertingGeneration)
                    {
                        type = "MonoObject*";
                        return "(" + typeInfo.GenericArgs[0].Type + "*)ScriptingObject::ToNative({0})";
                    }

                    type = typeInfo.GenericArgs[0].Type + '*';
                    return string.Empty;
                }

                // MClass
                if (typeInfo.Type == "MClass" && typeInfo.GenericArgs == null)
                {
                    type = "MonoReflectionType*";
                    return "Scripting::FindClass(MUtils::GetClass({0}))";
                }

                // Array
                if (typeInfo.Type == "Array" && typeInfo.GenericArgs != null)
                {
                    var T = typeInfo.GenericArgs[0].GetFullNameNative(buildData, caller);
                    type = "MonoArray*";
                    if (typeInfo.GenericArgs.Count != 1)
                        return "MUtils::ToArray<" + T + ", " + typeInfo.GenericArgs[1] + ">({0})";
                    return "MUtils::ToArray<" + T + ">({0})";
                }

                // Span
                if (typeInfo.Type == "Span" && typeInfo.GenericArgs != null)
                {
                    type = "MonoArray*";

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
                    CppIncludeFiles.Add("Engine/Scripting/InternalCalls/ManagedDictionary.h");
                    type = "MonoObject*";
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
                    type = "MonoArray*";
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
                        type = apiType.Name + "Managed";
                        if (functionInfo != null)
                            return "ToNative(*{0})";
                        return "ToNative({0})";
                    }

                    // Managed class
                    if (apiType.IsClass && !apiType.IsScriptingObject)
                    {
                        // Use wrapper structure that represents the memory layout of the managed data
                        if (!CppUsedNonPodTypes.Contains(apiType))
                        {
                            CppUsedNonPodTypes.Add(apiType);
                            CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                        }
                        type = "MonoObject*";
                        return "MConverter<" + apiType.Name + ">::Unbox({0})";
                    }

                    // Scripting Object
                    if (functionInfo == null && apiType.IsScriptingObject)
                    {
                        // Inside bindings function the managed runtime passes raw unamanged pointer
                        type = "MonoObject*";
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

        private static string GenerateCppWrapperNativeToBox(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, string value)
        {
            // Optimize passing scripting objects
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null && apiType.IsScriptingObject)
                return $"ScriptingObject::ToManaged((ScriptingObject*){value})";

            // Array or Span
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span") && typeInfo.GenericArgs != null && typeInfo.GenericArgs.Count >= 1)
                return $"MUtils::ToArray({value}, {GenerateCppGetNativeClass(buildData, typeInfo.GenericArgs[0], caller, null)})";

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
            return $"MUtils::Box<{nativeType}>({value}, {GenerateCppGetNativeClass(buildData, typeInfo, caller, null)})";
        }

        private static void GenerateCppWrapperFunction(BuildData buildData, StringBuilder contents, ApiTypeInfo caller, FunctionInfo functionInfo, string callFormat = "{0}({1})")
        {
            // Setup function binding glue to ensure that wrapper method signature matches for C++ and C#
            functionInfo.Glue = new FunctionInfo.GlueInfo
            {
                UseReferenceForResult = UsePassByReference(buildData, functionInfo.ReturnType, caller),
                CustomParameters = new List<FunctionInfo.ParameterInfo>(),
            };

            var returnValueConvert = GenerateCppWrapperNativeToManaged(buildData, functionInfo.ReturnType, caller, out var returnValueType, functionInfo);
            if (functionInfo.Glue.UseReferenceForResult)
            {
                returnValueType = "void";
                functionInfo.Glue.CustomParameters.Add(new FunctionInfo.ParameterInfo
                {
                    Name = "resultAsRef",
                    DefaultValue = "var resultAsRef",
                    Type = new TypeInfo
                    {
                        Type = functionInfo.ReturnType.Type,
                        GenericArgs = functionInfo.ReturnType.GenericArgs,
                    },
                    IsOut = true,
                });
            }

            contents.AppendFormat("    static {0} {1}(", returnValueType, functionInfo.UniqueName);

            var separator = false;
            if (!functionInfo.IsStatic)
            {
                contents.Append(string.Format("{0}* obj", caller.Name));
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
                CppParamsWrappersCache[i] = GenerateCppWrapperManagedToNative(buildData, parameterInfo.Type, caller, out var managedType, functionInfo, out CppParamsThatNeedLocalVariable[i]);

                // Out parameters that need additional converting will be converted at the native side (eg. object reference)
                var isOutWithManagedConverter = parameterInfo.IsOut && !string.IsNullOrEmpty(GenerateCSharpManagedToNativeConverter(buildData, parameterInfo.Type, caller));
                if (isOutWithManagedConverter)
                    managedType = "MonoObject*";

                contents.Append(managedType);
                if (parameterInfo.IsRef || parameterInfo.IsOut || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append('*');
                contents.Append(' ');
                contents.Append(parameterInfo.Name);

                // Special case for output result parameters that needs additional converting from native to managed format (such as non-POD structures, output arrays, etc.)
                if (parameterInfo.IsOut)
                {
                    bool convertOutputParameter = false;
                    var apiType = FindApiTypeInfo(buildData, parameterInfo.Type, caller);
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
            }

            for (var i = 0; i < functionInfo.Glue.CustomParameters.Count; i++)
            {
                var parameterInfo = functionInfo.Glue.CustomParameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                GenerateCppWrapperManagedToNative(buildData, parameterInfo.Type, caller, out var managedType, functionInfo, out _);
                contents.Append(managedType);
                if (parameterInfo.IsRef || parameterInfo.IsOut || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append('*');
                contents.Append(' ');
                contents.Append(parameterInfo.Name);
            }

            contents.Append(')');
            contents.AppendLine();
            contents.AppendLine("    {");
            if (!functionInfo.IsStatic)
                contents.AppendLine("        if (obj == nullptr) DebugLog::ThrowNullReference();");

            string callBegin = "        ";
            if (functionInfo.Glue.UseReferenceForResult)
            {
                callBegin += "*resultAsRef = ";
            }
            else if (!functionInfo.ReturnType.IsVoid)
            {
                if (useInlinedReturn)
                    callBegin += "return ";
                else
                    callBegin += "auto __result = ";
            }

            string call;
            if (functionInfo.IsStatic)
            {
                // Call native static method
                string nativeType = caller.Name;
                if (caller.Parent != null && !(caller.Parent is FileInfo))
                    nativeType = caller.Parent.FullNameNative + "::" + nativeType;
                call = $"{nativeType}::{functionInfo.Name}";
            }
            else
            {
                // Call native member method
                call = $"obj->{functionInfo.Name}";
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
                            contents.AppendFormat("        {1} {0}Temp;", parameterInfo.Name, new TypeInfo(parameterInfo.Type) { IsRef = false }.GetFullNameNative(buildData, caller)).AppendLine();
                        else
                            contents.AppendFormat("        auto {0}Temp = {1};", parameterInfo.Name, param).AppendLine();
                        if (parameterInfo.Type.IsPtr && !parameterInfo.Type.IsRef)
                            callParams += "&";
                        callParams += parameterInfo.Name;
                        callParams += "Temp";
                    }
                    // BytesContainer
                    else if (parameterInfo.Type.Type == "BytesContainer" && parameterInfo.Type.GenericArgs == null)
                    {
                        contents.AppendFormat("        BytesContainer {0}Temp;", parameterInfo.Name).AppendLine();
                        callParams += parameterInfo.Name;
                        callParams += "Temp";
                    }
                }
                // Special case for parameter that cannot be passed directly to the function from the wrapper method input parameter (eg. MonoArray* converted into BytesContainer uses as BytesContainer&)
                else if (CppParamsThatNeedLocalVariable[i])
                {
                    contents.AppendFormat("        auto {0}Temp = {1};", parameterInfo.Name, param).AppendLine();
                    if (parameterInfo.Type.IsPtr)
                        callParams += "&";
                    callParams += parameterInfo.Name;
                    callParams += "Temp";
                }
                else
                {
                    callParams += param;
                }
            }

            contents.Append(callBegin);
            call = string.Format(callFormat, call, callParams);
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

                        // MonoObject* parameters returned by reference need write barrier for GC
                        if (parameterInfo.IsOut)
                        {
                            var apiType = FindApiTypeInfo(buildData, parameterInfo.Type, caller);
                            if (apiType != null)
                            {
                                if (apiType.IsClass)
                                {
                                    contents.AppendFormat("        mono_gc_wbarrier_generic_store({0}, (MonoObject*){1});", parameterInfo.Name, value).AppendLine();
                                    continue;
                                }
                                if (apiType.IsStruct && !apiType.IsPod)
                                {
                                    CppIncludeFiles.Add("Engine/Scripting/ManagedCLR/MClass.h");
                                    contents.AppendFormat("        {{ auto _temp = {1}; mono_gc_wbarrier_value_copy({0}, &_temp, 1, {2}::TypeInitializer.GetType().ManagedClass->GetNative()); }}", parameterInfo.Name, value, apiType.FullNameNative).AppendLine();
                                    continue;
                                }
                            }
                            else
                            {
                                // BytesContainer
                                if (parameterInfo.Type.Type == "BytesContainer" && parameterInfo.Type.GenericArgs == null)
                                {
                                    contents.AppendFormat("        mono_gc_wbarrier_generic_store({0}, (MonoObject*){1});", parameterInfo.Name, value).AppendLine();
                                    continue;
                                }

                                throw new Exception($"Unsupported type of parameter '{parameterInfo}' in method '{functionInfo}' to be passed using 'out'");
                            }
                        }
                        contents.AppendFormat("        *{0} = {1};", parameterInfo.Name, value).AppendLine();
                    }
                }
            }

            if (!useInlinedReturn && !functionInfo.Glue.UseReferenceForResult && !functionInfo.ReturnType.IsVoid)
            {
                contents.Append("        return __result;").AppendLine();
            }

            contents.AppendLine("    }");
            contents.AppendLine();
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

            contents.AppendLine("        ScriptingTypeHandle managedTypeHandle = object->GetTypeHandle();");
            contents.AppendLine("        const ScriptingType* managedTypePtr = &managedTypeHandle.GetType();");
            contents.AppendLine("        while (managedTypePtr->Script.Spawn != &ManagedBinaryModule::ManagedObjectSpawn)");
            contents.AppendLine("        {");
            contents.AppendLine("            managedTypeHandle = managedTypePtr->GetBaseType();");
            contents.AppendLine("            managedTypePtr = &managedTypeHandle.GetType();");
            contents.AppendLine("        }");

            contents.AppendLine("        if (WrapperCallInstance == object)");
            contents.AppendLine("        {");
            contents.AppendLine("            // Prevent stack overflow by calling native base method");
            contents.AppendLine("            const auto scriptVTableBase = managedTypePtr->Script.ScriptVTableBase;");
            contents.Append($"            return (this->**({functionInfo.UniqueName}_Internal_Signature*)&scriptVTableBase[{scriptVTableOffset} + 2])(");
            separator = false;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;
                contents.Append(parameterInfo.Name);
            }
            contents.AppendLine(");");
            contents.AppendLine("        }");
            contents.AppendLine("        auto scriptVTable = (MMethod**)managedTypePtr->Script.ScriptVTable;");
            contents.AppendLine($"        ASSERT(scriptVTable && scriptVTable[{scriptVTableOffset}]);");
            contents.AppendLine($"        auto method = scriptVTable[{scriptVTableOffset}];");
            contents.AppendLine($"        PROFILE_CPU_NAMED(\"{classInfo.FullNameManaged}::{functionInfo.Name}\");");
            contents.AppendLine("        MonoObject* exception = nullptr;");

            contents.AppendLine("        auto prevWrapperCallInstance = WrapperCallInstance;");
            contents.AppendLine("        WrapperCallInstance = object;");
            contents.AppendLine("#if USE_MONO_AOT");

            if (functionInfo.Parameters.Count == 0)
                contents.AppendLine("        void** params = nullptr;");
            else
                contents.AppendLine($"        void* params[{functionInfo.Parameters.Count}];");

            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                var paramValue = GenerateCppWrapperNativeToManagedParam(buildData, contents, parameterInfo.Type, parameterInfo.Name, classInfo, parameterInfo.IsOut);
                contents.Append($"        params[{i}] = {paramValue};").AppendLine();
            }

            contents.AppendLine("        auto __result = mono_runtime_invoke(method->GetNative(), object->GetOrCreateManagedInstance(), params, &exception);");
            contents.AppendLine("#else");

            var thunkParams = string.Empty;
            var thunkCall = string.Empty;
            separator = functionInfo.Parameters.Count != 0;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    thunkParams += ", ";
                if (separator)
                    thunkCall += ", ";
                separator = true;
                thunkParams += "void*";

                // Mono thunk call uses boxed values as objects
                thunkCall += GenerateCppWrapperNativeToBox(buildData, parameterInfo.Type, classInfo, parameterInfo.Name);
            }

            if (functionInfo.ReturnType.IsVoid)
            {
                contents.AppendLine($"        typedef void (*Thunk)(void* instance{thunkParams}, MonoObject** exception);");
                contents.AppendLine("        const auto thunk = (Thunk)method->GetThunk();");
                contents.AppendLine($"        thunk(object->GetOrCreateManagedInstance(){thunkCall}, &exception);");
            }
            else
            {
                contents.AppendLine($"        typedef MonoObject* (*Thunk)(void* instance{thunkParams}, MonoObject** exception);");
                contents.AppendLine("        const auto thunk = (Thunk)method->GetThunk();");
                contents.AppendLine($"        auto __result = thunk(object->GetOrCreateManagedInstance(){thunkCall}, &exception);");
            }

            contents.AppendLine("#endif");
            contents.AppendLine("        WrapperCallInstance = prevWrapperCallInstance;");
            contents.AppendLine("        if (exception)");
            contents.AppendLine("            DebugLog::LogException(exception);");

            if (!functionInfo.ReturnType.IsVoid)
            {
                if (functionInfo.ReturnType.IsRef)
                    throw new NotSupportedException($"Passing return value by reference is not supported for virtual API methods. Used on method '{functionInfo}'.");

                contents.AppendLine($"        return MUtils::Unbox<{functionInfo.ReturnType}>(__result);");
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
                contents.AppendLine($"    typedef {functionInfo.ReturnType} ({classInfo.NativeName}Internal::*{functionInfo.UniqueName}_Internal_Signature)({thunkParams}){t};");
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
                contents.AppendLine("            if (vtableIndex > 0 && vtableIndex < entriesCount)");
                contents.AppendLine("            {");
                contents.AppendLine($"                scriptVTableBase[{scriptVTableIndex} + 2] = vtable[vtableIndex];");
                for (int i = 0, count = 0; i < ScriptingLangInfos.Count; i++)
                {
                    var langInfo = ScriptingLangInfos[i];
                    if (!langInfo.Enabled)
                        continue;
                    contents.AppendLine(count == 0 ? "                if (wrapperIndex == 0)" : $"                else if (wrapperIndex == {count})");
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
            if (memberInfo.IsStatic)
                return true;
            if (memberInfo.Access != AccessLevel.Public && !memberInfo.HasAttribute("Serialize"))
                return true;
            if (memberInfo.HasAttribute("NoSerialize") || memberInfo.HasAttribute("NonSerialized"))
                return true;
            var apiTypeInfo = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiTypeInfo != null && apiTypeInfo.IsInterface)
                return true;
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
                    CppAutoSerializeProperties.Add(propertyInfo);
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
                contents.Append("    {");
                contents.Append($"        const auto e = SERIALIZE_FIND_MEMBER(stream, \"{propertyInfo.Name}\");");
                contents.Append("        if (e != stream.MemberEnd()) {");
                contents.Append($"            auto p = {propertyInfo.Getter.Name}();");
                contents.Append("            Serialization::Deserialize(e->value, p, modifier);");
                contents.Append($"            {propertyInfo.Setter.Name}(p);");
                contents.Append("        }");
                contents.Append('}').AppendLine();
            }

            contents.Append('}').AppendLine();
        }

        private static string GenerateCppInterfaceInheritanceTable(BuildData buildData, StringBuilder contents, ModuleInfo moduleInfo, VirtualClassInfo typeInfo, string typeNameNative)
        {
            var interfacesPtr = "nullptr";
            var interfaces = typeInfo.Interfaces;
            if (interfaces != null)
            {
                interfacesPtr = typeNameNative + "_Interfaces";
                contents.Append("static const ScriptingType::InterfaceImplementation ").Append(interfacesPtr).AppendLine("[] = {");
                for (int i = 0; i < interfaces.Count; i++)
                {
                    var interfaceInfo = interfaces[i];
                    var scriptVTableOffset = typeInfo.GetScriptVTableOffset(interfaceInfo);
                    contents.AppendLine($"    {{ &{interfaceInfo.NativeName}::TypeInitializer, (int16)VTABLE_OFFSET({typeInfo.NativeName}, {interfaceInfo.NativeName}), {scriptVTableOffset}, true }},");
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
            var classTypeNameInternal = classInfo.NativeName;
            if (classInfo.Parent != null && !(classInfo.Parent is FileInfo))
                classTypeNameInternal = classInfo.Parent.FullNameNative + '_' + classTypeNameInternal;
            var useScripting = classInfo.IsStatic || classInfo.IsScriptingObject;
            var useCSharp = EngineConfiguration.WithCSharp(buildData.TargetOptions);
            var hasInterface = classInfo.Interfaces != null && classInfo.Interfaces.Any(x => x.Access == AccessLevel.Public);

            if (classInfo.IsAutoSerialization)
                GenerateCppAutoSerialization(buildData, contents, moduleInfo, classInfo, classTypeNameNative);

            contents.AppendLine();
            contents.AppendFormat("class {0}Internal", classTypeNameInternal).AppendLine();
            contents.Append('{').AppendLine();
            contents.AppendLine("public:");

            // Events
            foreach (var eventInfo in classInfo.Events)
            {
                if (!useScripting || eventInfo.IsHidden)
                    continue;
                var paramsCount = eventInfo.Type.GenericArgs?.Count ?? 0;
                CppIncludeFiles.Add("Engine/Profiler/ProfilerCPU.h");
                var bindPrefix = eventInfo.IsStatic ? classTypeNameNative + "::" : "obj->";

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
                        contents.Append(eventInfo.Type.GenericArgs[i]).Append(" arg" + i);
                    }
                    contents.Append(')').AppendLine();
                    contents.Append("    {").AppendLine();
                    contents.Append("        static MMethod* mmethod = nullptr;").AppendLine();
                    contents.Append("        if (!mmethod)").AppendLine();
                    contents.AppendFormat("            mmethod = {1}::TypeInitializer.GetType().ManagedClass->GetMethod(\"Internal_{0}_Invoke\", {2});", eventInfo.Name, classTypeNameNative, paramsCount).AppendLine();
                    contents.Append("        CHECK(mmethod);").AppendLine();
                    contents.Append($"        PROFILE_CPU_NAMED(\"{classInfo.FullNameManaged}::On{eventInfo.Name}\");").AppendLine();
                    contents.Append("        MonoObject* exception = nullptr;").AppendLine();
                    if (paramsCount == 0)
                        contents.AppendLine("        void** params = nullptr;");
                    else
                        contents.AppendLine($"        void* params[{paramsCount}];");
                    for (var i = 0; i < paramsCount; i++)
                    {
                        var paramType = eventInfo.Type.GenericArgs[i];
                        var paramName = "arg" + i;
                        var paramValue = GenerateCppWrapperNativeToManagedParam(buildData, contents, paramType, paramName, classInfo, false);
                        contents.Append($"        params[{i}] = {paramValue};").AppendLine();
                    }
                    if (eventInfo.IsStatic)
                        contents.AppendLine("        MonoObject* instance = nullptr;");
                    else
                        contents.AppendLine($"        MonoObject* instance = (({classTypeNameNative}*)this)->GetManagedInstance();");
                    contents.Append("        mono_runtime_invoke(mmethod->GetNative(), instance, params, &exception);").AppendLine();
                    contents.Append("        if (exception)").AppendLine();
                    contents.Append("            DebugLog::LogException(exception);").AppendLine();
                    for (var i = 0; i < paramsCount; i++)
                    {
                        var paramType = eventInfo.Type.GenericArgs[i];
                        if (paramType.IsRef && !paramType.IsConst)
                        {
                            // Convert value back from managed to native (could be modified there)
                            paramType.IsRef = false;
                            var managedToNative = GenerateCppWrapperManagedToNative(buildData, paramType, classInfo, out var managedType, null, out var _);
                            var passAsParamPtr = managedType.EndsWith("*");
                            var paramValue = $"({managedType}{(passAsParamPtr ? "" : "*")})params[{i}]";
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
                    contents.AppendFormat("    static void {0}_ManagedBind(", eventInfo.Name);
                    if (!eventInfo.IsStatic)
                        contents.AppendFormat("{0}* obj, ", classTypeNameNative);
                    contents.Append("bool bind)").AppendLine();
                    contents.Append("    {").AppendLine();
                    contents.Append("        Function<void(");
                    for (var i = 0; i < paramsCount; i++)
                    {
                        if (i != 0)
                            contents.Append(", ");
                        contents.Append(eventInfo.Type.GenericArgs[i]);
                    }
                    contents.Append(")> f;").AppendLine();
                    if (eventInfo.IsStatic)
                        contents.AppendFormat("        f.Bind<{0}_ManagedWrapper>();", eventInfo.Name).AppendLine();
                    else
                        contents.AppendFormat("        f.Bind<{1}Internal, &{1}Internal::{0}_ManagedWrapper>(({1}Internal*)obj);", eventInfo.Name, classTypeNameInternal).AppendLine();
                    contents.Append("        if (bind)").AppendLine();
                    contents.AppendFormat("            {0}{1}.Bind(f);", bindPrefix, eventInfo.Name).AppendLine();
                    contents.Append("        else").AppendLine();
                    contents.AppendFormat("            {0}{1}.Unbind(f);", bindPrefix, eventInfo.Name).AppendLine();
                    contents.Append("    }").AppendLine().AppendLine();
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
                    contents.Append(eventInfo.Type.GenericArgs[i]).Append(" arg" + i);
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
                contents.AppendFormat("{0}* obj, void* instance, bool bind)", classTypeNameNative).AppendLine();
                contents.Append("    {").AppendLine();
                contents.Append("        Function<void(");
                for (var i = 0; i < paramsCount; i++)
                {
                    if (i != 0)
                        contents.Append(", ");
                    contents.Append(eventInfo.Type.GenericArgs[i]);
                }
                contents.Append(")> f;").AppendLine();
                if (eventInfo.IsStatic)
                    contents.AppendFormat("        f.Bind<{0}_Wrapper>();", eventInfo.Name).AppendLine();
                else
                    contents.AppendFormat("        f.Bind<{1}Internal, &{1}Internal::{0}_Wrapper>(({1}Internal*)instance);", eventInfo.Name, classTypeNameInternal).AppendLine();
                contents.Append("        if (bind)").AppendLine();
                contents.AppendFormat("            {0}{1}.Bind(f);", bindPrefix, eventInfo.Name).AppendLine();
                contents.Append("        else").AppendLine();
                contents.AppendFormat("            {0}{1}.Unbind(f);", bindPrefix, eventInfo.Name).AppendLine();
                contents.Append("    }").AppendLine().AppendLine();
            }

            // Fields
            foreach (var fieldInfo in classInfo.Fields)
            {
                if (!useScripting || !useCSharp || fieldInfo.IsHidden)
                    continue;
                if (fieldInfo.Getter != null)
                    GenerateCppWrapperFunction(buildData, contents, classInfo, fieldInfo.Getter, "{0}");
                if (fieldInfo.Setter != null)
                {
                    var callFormat = "{0} = {1}";
                    var type = fieldInfo.Setter.Parameters[0].Type;
                    if (type.IsArray)
                        callFormat = $"auto __tmp = {{1}}; for (int32 i = 0; i < {type.ArraySize}; i++) {{0}}[i] = __tmp[i]";
                    GenerateCppWrapperFunction(buildData, contents, classInfo, fieldInfo.Setter, callFormat);
                }
            }

            // Properties
            foreach (var propertyInfo in classInfo.Properties)
            {
                if (!useScripting || !useCSharp || propertyInfo.IsHidden)
                    continue;
                if (propertyInfo.Getter != null)
                    GenerateCppWrapperFunction(buildData, contents, classInfo, propertyInfo.Getter);
                if (propertyInfo.Setter != null)
                    GenerateCppWrapperFunction(buildData, contents, classInfo, propertyInfo.Setter);
            }

            // Functions
            foreach (var functionInfo in classInfo.Functions)
            {
                if (!useCSharp || functionInfo.IsHidden)
                    continue;
                if (!useScripting)
                    throw new Exception($"Not supported function {functionInfo.Name} inside non-static and non-scripting class type {classInfo.Name}.");
                GenerateCppWrapperFunction(buildData, contents, classInfo, functionInfo);
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
                        GenerateCppWrapperFunction(buildData, contents, classInfo, functionInfo);
                    }
                }
            }

            GenerateCppClassInternals?.Invoke(buildData, classInfo, contents);

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
                foreach (var eventInfo in classInfo.Events)
                {
                    if (eventInfo.IsHidden)
                        continue;
                    contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{eventInfo.Name}_Bind\", &{eventInfo.Name}_ManagedBind);");
                }
                foreach (var fieldInfo in classInfo.Fields)
                {
                    if (fieldInfo.IsHidden)
                        continue;
                    if (fieldInfo.Getter != null)
                        contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{fieldInfo.Getter.UniqueName}\", &{fieldInfo.Getter.UniqueName});");
                    if (fieldInfo.Setter != null)
                        contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{fieldInfo.Setter.UniqueName}\", &{fieldInfo.Setter.UniqueName});");
                }
                foreach (var propertyInfo in classInfo.Properties)
                {
                    if (propertyInfo.IsHidden)
                        continue;
                    if (propertyInfo.Getter != null)
                        contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{propertyInfo.Getter.UniqueName}\", &{propertyInfo.Getter.UniqueName});");
                    if (propertyInfo.Setter != null)
                        contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{propertyInfo.Setter.UniqueName}\", &{propertyInfo.Setter.UniqueName});");
                }
                foreach (var functionInfo in classInfo.Functions)
                {
                    if (functionInfo.IsHidden)
                        continue;
                    contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{functionInfo.UniqueName}\", &{functionInfo.UniqueName});");
                }
                if (hasInterface)
                {
                    foreach (var interfaceInfo in classInfo.Interfaces)
                    {
                        if (interfaceInfo.Access != AccessLevel.Public)
                            continue;
                        foreach (var functionInfo in interfaceInfo.Functions)
                        {
                            contents.AppendLine($"        ADD_INTERNAL_CALL(\"{classTypeNameManagedInternalCall}::Internal_{functionInfo.UniqueName}\", &{functionInfo.UniqueName});");
                        }
                    }
                }
            }
            GenerateCppClassInitRuntime?.Invoke(buildData, classInfo, contents);

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
            var interfacesTable = GenerateCppInterfaceInheritanceTable(buildData, contents, moduleInfo, classInfo, classTypeNameNative);

            // Type initializer
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
                if (classInfo.BaseType != null && useScripting)
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
            contents.Append(", ").Append(interfacesTable);
            contents.Append(");");
            contents.AppendLine();

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
            var structureTypeNameInternal = structureInfo.NativeName;
            if (structureInfo.Parent != null && !(structureInfo.Parent is FileInfo))
                structureTypeNameInternal = structureInfo.Parent.FullNameNative + '_' + structureTypeNameInternal;
            var useCSharp = EngineConfiguration.WithCSharp(buildData.TargetOptions);

            if (structureInfo.IsAutoSerialization)
                GenerateCppAutoSerialization(buildData, contents, moduleInfo, structureInfo, structureTypeNameNative);

            contents.AppendLine();
            contents.AppendFormat("class {0}Internal", structureTypeNameInternal).AppendLine();
            contents.Append('{').AppendLine();
            contents.AppendLine("public:");

            // Fields
            foreach (var fieldInfo in structureInfo.Fields)
            {
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
                    GenerateCppWrapperFunction(buildData, contents, structureInfo, fieldInfo.Getter, "{0}");
                if (fieldInfo.Setter != null)
                    GenerateCppWrapperFunction(buildData, contents, structureInfo, fieldInfo.Setter, "{0} = {1}");
            }

            // Functions
            foreach (var functionInfo in structureInfo.Functions)
            {
                // TODO: add support for API functions in structures
                throw new NotImplementedException($"TODO: add support for API functions in structures (function {functionInfo} in structure {structureInfo.Name})");
                //GenerateCppWrapperFunction(buildData, contents, structureInfo, functionInfo);
            }

            contents.AppendLine("    static void InitRuntime()");
            contents.AppendLine("    {");

            if (useCSharp)
            {
                foreach (var fieldInfo in structureInfo.Fields)
                {
                    if (fieldInfo.Getter != null)
                        contents.AppendLine($"        ADD_INTERNAL_CALL(\"{structureTypeNameManagedInternalCall}::Internal_{fieldInfo.Getter.UniqueName}\", &{fieldInfo.Getter.UniqueName});");
                    if (fieldInfo.Setter != null)
                        contents.AppendLine($"        ADD_INTERNAL_CALL(\"{structureTypeNameManagedInternalCall}::Internal_{fieldInfo.Setter.UniqueName}\", &{fieldInfo.Setter.UniqueName});");
                }
            }

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
                contents.AppendLine($"        MonoObject* managed = mono_object_new(mono_domain_get(), {structureTypeNameNative}::TypeInitializer.GetType().ManagedClass->GetNative());");
                if (structureInfo.IsPod)
                    contents.AppendLine($"        Platform::MemoryCopy(mono_object_unbox(managed), ptr, sizeof({structureTypeNameNative}));");
                else
                    contents.AppendLine($"        *({structureInfo.NativeName}Managed*)mono_object_unbox(managed) = ToManaged(*({structureTypeNameNative}*)ptr);");
                contents.AppendLine("        return managed;");
                contents.AppendLine("    }").AppendLine();

                // Unboxing structures from managed object to native data
                contents.AppendLine("    static void Unbox(void* ptr, MObject* managed)");
                contents.AppendLine("    {");
                if (structureInfo.IsPod)
                    contents.AppendLine($"        Platform::MemoryCopy(ptr, mono_object_unbox(managed), sizeof({structureTypeNameNative}));");
                else
                    contents.AppendLine($"        *({structureTypeNameNative}*)ptr = ToNative(*({structureInfo.NativeName}Managed*)mono_object_unbox(managed));");
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
            for (var i = 0; i < structureInfo.Fields.Count; i++)
            {
                var fieldInfo = structureInfo.Fields[i];
                if (fieldInfo.IsReadOnly || fieldInfo.IsStatic || fieldInfo.Access == AccessLevel.Private)
                    continue;
                if (i == 0)
                    contents.AppendLine($"        if (name == TEXT(\"{fieldInfo.Name}\"))");
                else
                    contents.AppendLine($"        else if (name == TEXT(\"{fieldInfo.Name}\"))");
                contents.AppendLine($"            value = {GenerateCppWrapperNativeToVariant(buildData, fieldInfo.Type, structureInfo, $"(({structureTypeNameNative}*)ptr)->{fieldInfo.Name}")};");
            }
            contents.AppendLine("    }").AppendLine();

            // Setter for structure field
            contents.AppendLine("    static void SetField(void* ptr, const String& name, const Variant& value)");
            contents.AppendLine("    {");
            for (var i = 0; i < structureInfo.Fields.Count; i++)
            {
                var fieldInfo = structureInfo.Fields[i];
                if (fieldInfo.IsReadOnly || fieldInfo.IsStatic || fieldInfo.Access == AccessLevel.Private)
                    continue;
                if (i == 0)
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
            }
            contents.AppendLine("    }");

            contents.Append('}').Append(';').AppendLine();
            contents.AppendLine();

            contents.Append($"ScriptingTypeInitializer {structureTypeNameNative}::TypeInitializer((BinaryModule*)GetBinaryModule{moduleInfo.Name}(), ");
            contents.Append($"StringAnsiView(\"{structureTypeNameManaged}\", {structureTypeNameManaged.Length}), ");
            contents.Append($"sizeof({structureTypeNameNative}), ");
            contents.Append($"&{structureTypeNameInternal}Internal::InitRuntime, ");
            contents.Append($"&{structureTypeNameInternal}Internal::Ctor, &{structureTypeNameInternal}Internal::Dtor, &{structureTypeNameInternal}Internal::Copy, &{structureTypeNameInternal}Internal::Box, &{structureTypeNameInternal}Internal::Unbox, &{structureTypeNameInternal}Internal::GetField, &{structureTypeNameInternal}Internal::SetField);").AppendLine();

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
            var enumTypeNameInternal = enumInfo.NativeName;
            if (enumInfo.Parent != null && !(enumInfo.Parent is FileInfo))
                enumTypeNameInternal = enumInfo.Parent.FullNameNative + '_' + enumTypeNameInternal;

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
            var interfaceTypeNameInternal = interfaceInfo.NativeName;
            if (interfaceInfo.Parent != null && !(interfaceInfo.Parent is FileInfo))
                interfaceTypeNameInternal = interfaceInfo.Parent.FullNameNative + '_' + interfaceTypeNameInternal;

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
                contents.Append(") override").AppendLine();
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
                contents.AppendLine($"            auto method = typeHandle.Module->FindMethod(typeHandle, \"{functionInfo.Name}\", {functionInfo.Parameters.Count});");
                contents.AppendLine("            if (method)");
                contents.AppendLine("            {");
                contents.AppendLine("                Variant __result;");
                contents.AppendLine($"                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(parameters, {functionInfo.Parameters.Count}), __result);");
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

            GenerateCppClassInternals?.Invoke(buildData, interfaceInfo, contents);

            // Virtual methods overrides
            var setupScriptVTable = GenerateCppScriptVTable(buildData, contents, interfaceInfo);

            // Runtime initialization (internal methods binding)
            contents.AppendLine("    static void InitRuntime()");
            contents.AppendLine("    {");
            GenerateCppClassInitRuntime?.Invoke(buildData, interfaceInfo, contents);
            contents.AppendLine("    }").AppendLine();

            // Interface implementation wrapper accessor for scripting types
            contents.AppendLine("    static void* GetInterfaceWrapper(ScriptingObject* obj)");
            contents.AppendLine("    {");
            contents.AppendLine($"        auto wrapper = New<{interfaceTypeNameInternal}Wrapper>();");
            contents.AppendLine("        wrapper->Object = obj;");
            contents.AppendLine("        return wrapper;");
            contents.AppendLine("    }");

            contents.Append('}').Append(';').AppendLine();
            contents.AppendLine();

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
            if (type is ApiTypeInfo apiTypeInfo && apiTypeInfo.IsInBuild)
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
                else if (type is InjectCppCodeInfo injectCppCodeInfo)
                    contents.AppendLine(injectCppCodeInfo.Code);
            }
            catch
            {
                Log.Error($"Failed to generate C++ bindings for {type}.");
                throw;
            }
        }

        private static void GenerateCppCppUsedNonPodTypes(BuildData buildData, ApiTypeInfo apiType)
        {
            if (apiType is StructureInfo structureInfo)
            {
                // Check all fields (if one of them is also non-POD structure then we need to generate wrappers for them too)
                for (var i = 0; i < structureInfo.Fields.Count; i++)
                {
                    var fieldInfo = structureInfo.Fields[i];
                    if (fieldInfo.IsStatic)
                        continue;
                    var fieldType = fieldInfo.Type;
                    var fieldApiType = FindApiTypeInfo(buildData, fieldType, structureInfo);
                    if (fieldApiType != null && fieldApiType.IsStruct && !fieldApiType.IsPod)
                    {
                        GenerateCppCppUsedNonPodTypes(buildData, fieldApiType);
                    }
                }
            }
            if (!CppUsedNonPodTypesList.Contains(apiType))
                CppUsedNonPodTypesList.Add(apiType);
        }

        private static void GenerateCpp(BuildData buildData, ModuleInfo moduleInfo, ref BindingsResult bindings)
        {
            var contents = new StringBuilder();
            CppUsedNonPodTypes.Clear();
            CppReferencesFiles.Clear();
            CppIncludeFiles.Clear();
            CppIncludeFilesList.Clear();
            CppVariantToTypes.Clear();
            CppVariantFromTypes.Clear();

            // Disable C# scripting based on configuration
            ScriptingLangInfos[0].Enabled = EngineConfiguration.WithCSharp(buildData.TargetOptions);

            contents.AppendLine("// This code was auto-generated. Do not modify it.");
            contents.AppendLine();
            contents.AppendLine("#include \"Engine/Core/Compiler.h\"");
            contents.AppendLine("PRAGMA_DISABLE_DEPRECATION_WARNINGS"); // Disable deprecated warnings in generated code
            contents.AppendLine("#include \"Engine/Scripting/Scripting.h\"");
            contents.AppendLine("#include \"Engine/Scripting/InternalCalls.h\"");
            contents.AppendLine("#include \"Engine/Scripting/ManagedCLR/MUtils.h\"");
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
                var header = new StringBuilder();

                // Variant converting helper methods
                foreach (var typeInfo in CppVariantToTypes)
                {
                    header.AppendLine();
                    header.AppendLine("namespace {");
                    header.Append($"{typeInfo} VariantTo{GenerateCppWrapperNativeToVariantMethodName(typeInfo)}(const Variant& v)").AppendLine();
                    header.Append('{').AppendLine();
                    header.Append($"    {typeInfo} result;").AppendLine();
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
                foreach (var typeInfo in CppVariantFromTypes)
                {
                    header.AppendLine();
                    header.AppendLine("namespace {");
                    header.Append("Variant VariantFrom");
                    if (typeInfo.IsArray)
                    {
                        typeInfo.IsArray = false;
                        header.Append($"{GenerateCppWrapperNativeToVariantMethodName(typeInfo)}Array(const {typeInfo}* v, const int32 length)").AppendLine();
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
                        header.Append($"{GenerateCppWrapperNativeToVariantMethodName(valueType)}Array(const {valueType}* v, const int32 length)").AppendLine();
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
                        header.Append($"{GenerateCppWrapperNativeToVariantMethodName(keyType)}{GenerateCppWrapperNativeToVariantMethodName(valueType)}Dictionary(const Dictionary<{keyType}, {valueType}>& v)").AppendLine();
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

                    // Generate managed type memory layout
                    header.Append("struct ").Append(apiType.Name).Append("Managed").AppendLine();
                    header.Append('{').AppendLine();
                    if (classInfo != null)
                        header.AppendLine("    MonoObject obj;");
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

                    if (structureInfo != null)
                    {
                        // Generate forward declarations of structure converting functions
                        header.AppendLine();
                        header.AppendLine("namespace {");
                        header.AppendFormat("{0}Managed ToManaged(const {1}& value);", apiType.Name, fullName).AppendLine();
                        header.AppendFormat("{1} ToNative(const {0}Managed& value);", apiType.Name, fullName).AppendLine();
                        header.AppendLine("}");

                        // Generate MConverter
                        header.Append("template<>").AppendLine();
                        header.AppendFormat("struct MConverter<{0}>", fullName).AppendLine();
                        header.Append('{').AppendLine();
                        header.AppendFormat("    MonoObject* Box(const {0}& data, MonoClass* klass)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        auto managed = ToManaged(data);").AppendLine();
                        header.Append("        return mono_value_box(mono_domain_get(), klass, (void*)&managed);").AppendLine();
                        header.Append("    }").AppendLine();
                        header.AppendFormat("    void Unbox({0}& result, MonoObject* data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        result = ToNative(*reinterpret_cast<{0}Managed*>(mono_object_unbox(data)));", apiType.Name).AppendLine();
                        header.Append("    }").AppendLine();
                        header.AppendFormat("    void ToManagedArray(MonoArray* result, const Span<{0}>& data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        MonoClass* klass = {0}::TypeInitializer.GetType().ManagedClass->GetNative();", fullName).AppendLine();
                        header.Append("        ASSERT(klass);").AppendLine();
                        header.Append("        for (int32 i = 0; i < data.Length(); i++)").AppendLine();
                        header.Append("        {").AppendLine();
                        header.Append("        	auto managed = ToManaged(data[i]);").AppendLine();
                        header.AppendFormat("        	mono_value_copy(mono_array_addr(result, {0}Managed, i), &managed, klass);", apiType.Name).AppendLine();
                        header.Append("        }").AppendLine();
                        header.Append("    }").AppendLine();
                        header.Append("    template<typename AllocationType = HeapAllocation>").AppendLine();
                        header.AppendFormat("    void ToNativeArray(Array<{0}, AllocationType>& result, MonoArray* data, int32 length)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        for (int32 i = 0; i < length; i++)").AppendLine();
                        header.AppendFormat("    	    result.Add(ToNative(mono_array_get(data, {0}Managed, i)));", apiType.Name).AppendLine();
                        header.Append("    }").AppendLine();
                        header.Append('}').Append(';').AppendLine();

                        // Generate converting function native -> managed
                        header.AppendLine();
                        header.AppendLine("namespace {");
                        header.AppendFormat("{0}Managed ToManaged(const {1}& value)", apiType.Name, fullName).AppendLine();
                        header.Append('{').AppendLine();
                        header.AppendFormat("    {0}Managed result;", apiType.Name).AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic)
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
                        header.AppendFormat("{1} ToNative(const {0}Managed& value)", apiType.Name, fullName).AppendLine();
                        header.Append('{').AppendLine();
                        header.AppendFormat("    {0} result;", fullName).AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic)
                                continue;

                            CppNonPodTypesConvertingGeneration = true;
                            var wrapper = GenerateCppWrapperManagedToNative(buildData, fieldInfo.Type, apiType, out _, null, out _);
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
                        // Generate MConverter
                        header.Append("template<>").AppendLine();
                        header.AppendFormat("struct MConverter<{0}>", fullName).AppendLine();
                        header.Append('{').AppendLine();

                        header.AppendFormat("    static MonoObject* Box(const {0}& data, MonoClass* klass)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        auto obj = ({0}Managed*)mono_object_new(mono_domain_get(), klass);", fullName).AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic)
                                continue;

                            if (fieldInfo.NoArray && fieldInfo.Type.IsArray)
                            {
                                // Fixed-size array needs to unbox every item manually if not using managed array
                                if (fieldInfo.Type.IsPod(buildData, apiType))
                                    header.AppendFormat("    Platform::MemoryCopy(obj->{0}, data.{0}, sizeof({2}) * {1});", fieldInfo.Name, fieldInfo.Type.ArraySize, fieldInfo.Type).AppendLine();
                                else
                                    header.AppendFormat("    for (int32 i = 0; i < {0}; i++)", fieldInfo.Type.ArraySize).AppendLine().AppendFormat("        obj->{0}[i] = data.{0}[i];", fieldInfo.Name).AppendLine();
                                continue;
                            }

                            var wrapper = CppParamsWrappersCache[i];
                            var value = string.IsNullOrEmpty(wrapper) ? "data." + fieldInfo.Name : string.Format(wrapper, "data." + fieldInfo.Name);
                            if (fieldInfo.Type.IsPod(buildData, apiType))
                                header.AppendFormat("        obj->{0} = {1};", fieldInfo.Name, value).AppendLine();
                            else
                                header.AppendFormat("        MONO_OBJECT_SETREF(obj, {0}, {1});", fieldInfo.Name, value).AppendLine();
                        }
                        header.Append("        return (MonoObject*)obj;").AppendLine();
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    static MonoObject* Box(const {0}& data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        MonoClass* klass = {0}::TypeInitializer.GetType().ManagedClass->GetNative();", fullName).AppendLine();
                        header.Append("        return Box(data, klass);").AppendLine();
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    static void Unbox({0}& result, MonoObject* data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        auto obj = ({0}Managed*)data;", fullName).AppendLine();
                        for (var i = 0; i < fields.Count; i++)
                        {
                            var fieldInfo = fields[i];
                            if (fieldInfo.IsStatic)
                                continue;

                            CppNonPodTypesConvertingGeneration = true;
                            var wrapper = GenerateCppWrapperManagedToNative(buildData, fieldInfo.Type, apiType, out _, null, out _);
                            CppNonPodTypesConvertingGeneration = false;

                            if (fieldInfo.Type.IsArray)
                            {
                                // Fixed-size array needs to unbox every item manually
                                if (fieldInfo.NoArray)
                                {
                                    if (fieldInfo.Type.IsPod(buildData, apiType))
                                        header.AppendFormat("        Platform::MemoryCopy(result.{0}, obj->{0}, sizeof({2}) * {1});", fieldInfo.Name, fieldInfo.Type.ArraySize, fieldInfo.Type).AppendLine();
                                    else
                                        header.AppendFormat("        for (int32 i = 0; i < {0}; i++)", fieldInfo.Type.ArraySize).AppendLine().AppendFormat("            result.{0}[i] = obj->{0}[i];", fieldInfo.Name).AppendLine();
                                }
                                else
                                {
                                    wrapper = string.Format(wrapper.Remove(wrapper.Length - 6), string.Format("obj->{0}", fieldInfo.Name));
                                    header.AppendFormat("        auto tmp{0} = {1};", fieldInfo.Name, wrapper).AppendLine();
                                    header.AppendFormat("        for (int32 i = 0; i < {0} && i < tmp{1}.Count(); i++)", fieldInfo.Type.ArraySize, fieldInfo.Name).AppendLine();
                                    header.AppendFormat("            result.{0}[i] = tmp{0}[i];", fieldInfo.Name).AppendLine();
                                }
                                continue;
                            }

                            if (string.IsNullOrEmpty(wrapper))
                                header.AppendFormat("        result.{0} = obj->{0};", fieldInfo.Name).AppendLine();
                            else
                                header.AppendFormat("        result.{0} = {1};", fieldInfo.Name, string.Format(wrapper, string.Format("obj->{0}", fieldInfo.Name))).AppendLine();
                        }
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    static {0} Unbox(MonoObject* data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.AppendFormat("        {0} result;", fullName).AppendLine();
                        header.Append("        Unbox(result, data);").AppendLine();
                        header.Append("        return result;").AppendLine();
                        header.Append("    }").AppendLine();

                        header.AppendFormat("    void ToManagedArray(MonoArray* result, const Span<{0}>& data)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        for (int32 i = 0; i < data.Length(); i++)").AppendLine();
                        header.Append("            mono_array_setref(result, i, Box(data[i]));").AppendLine();
                        header.Append("    }").AppendLine();

                        header.Append("    template<typename AllocationType = HeapAllocation>").AppendLine();
                        header.AppendFormat("    void ToNativeArray(Array<{0}, AllocationType>& result, MonoArray* data, int32 length)", fullName).AppendLine();
                        header.Append("    {").AppendLine();
                        header.Append("        for (int32 i = 0; i < length; i++)").AppendLine();
                        header.AppendFormat("            Unbox(result[i], (MonoObject*)mono_array_addr_with_size(data, sizeof({0}Managed), length));", fullName).AppendLine();
                        header.Append("    }").AppendLine();

                        header.Append('}').Append(';').AppendLine();
                    }
                }

                contents.Insert(headerPos, header.ToString());

                // Includes
                header.Clear();
                CppReferencesFiles.Remove(null);
                CppIncludeFilesList.Clear();
                foreach (var fileInfo in CppReferencesFiles)
                    CppIncludeFilesList.Add(fileInfo.Name);
                CppIncludeFilesList.AddRange(CppIncludeFiles);
                CppIncludeFilesList.Sort();
                foreach (var path in CppIncludeFilesList)
                    header.AppendFormat("#include \"{0}\"", path).AppendLine();
                contents.Insert(headerPos, header.ToString());
            }

            contents.AppendLine("PRAGMA_ENABLE_DEPRECATION_WARNINGS");

            Utilities.WriteFileIfChanged(bindings.GeneratedCppFilePath, contents.ToString());
        }

        private static void GenerateCpp(BuildData buildData, IGrouping<string, Module> binaryModule)
        {
            // Skip generating C++ bindings code for C#-only modules
            if (binaryModule.Any(x => !x.BuildNativeCode))
                return;
            var useCSharp = binaryModule.Any(x => x.BuildCSharp);

            var contents = new StringBuilder();
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
            if (version.Build == -1)
                contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION Version({version.Major}, {version.Minor})");
            else
                contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION Version({version.Major}, {version.Minor}, {version.Build})");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_TEXT \"{version}\"");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_MAJOR {version.Major}");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_MINOR {version.Minor}");
            contents.AppendLine($"#define {binaryModuleNameUpper}_VERSION_BUILD {version.Build}");
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
                contents.AppendLine($"    static NativeBinaryModule module(\"{binaryModuleName}\", MAssemblyOptions());");
            }
            else
            {
                contents.AppendLine($"    static NativeOnlyBinaryModule module(\"{binaryModuleName}\");");
            }
            contents.AppendLine("    return &module;");
            contents.AppendLine("}");
            GenerateCppBinaryModuleSource?.Invoke(buildData, binaryModule, contents);
            Utilities.WriteFileIfChanged(binaryModuleSourcePath, contents.ToString());
        }
    }
}

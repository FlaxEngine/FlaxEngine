// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

//#define AUTO_DOC_TOOLTIPS
//#define MARSHALLER_FULL_NAME

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
        private static readonly HashSet<string> CSharpUsedNamespaces = new HashSet<string>();
        private static readonly List<string> CSharpUsedNamespacesSorted = new List<string>();
        private static readonly List<string> CSharpAdditionalCode = new List<string>();
        private static readonly Dictionary<string, string> CSharpAdditionalCodeCache = new Dictionary<string, string>();
#if USE_NETCORE
        private static readonly TypeInfo CSharpEventBindReturn = new TypeInfo("void");
        private static readonly List<FunctionInfo.ParameterInfo> CSharpEventBindParams = new List<FunctionInfo.ParameterInfo> { new FunctionInfo.ParameterInfo { Name = "bind", Type = new TypeInfo("bool") } };
#endif

        public static event Action<BuildData, ApiTypeInfo, StringBuilder, string> GenerateCSharpTypeInternals;

        internal static readonly Dictionary<string, string> CSharpNativeToManagedBasicTypes = new Dictionary<string, string>
        {
            // Language types
            { "bool", "bool" },
            { "int8", "sbyte" },
            { "int16", "short" },
            { "int32", "int" },
            { "short", "short" },
            { "int", "int" },
            { "int64", "long" },
            { "uint8", "byte" },
            { "byte", "byte" },
            { "uint16", "ushort" },
            { "uint32", "uint" },
            { "uint64", "ulong" },
            { "char", "sbyte" },
            { "Char", "char" },
            { "float", "float" },
            { "double", "double" },
        };

        internal static readonly Dictionary<string, string> CSharpNativeToManagedDefault = new Dictionary<string, string>
        {
            // Engine types
            { "String", "string" },
            { "StringView", "string" },
            { "StringAnsi", "string" },
            { "StringAnsiView", "string" },
            { "ScriptingObject", "FlaxEngine.Object" },
            { "ScriptingTypeHandle", "System.Type" },
            { "PersistentScriptingObject", "FlaxEngine.Object" },
            { "ManagedScriptingObject", "FlaxEngine.Object" },
            { "Variant", "object" },
            { "VariantType", "System.Type" },

            // Mono types
            { "MonoObject", "object" },
            { "MonoClass", "System.Type" },
            { "MonoReflectionType", "System.Type" },
            { "MonoType", "IntPtr" },
            { "MonoArray", "Array" },

            // Managed types
            { "MObject", "object" },
            { "MClass", "System.Type" },
            { "MType", "System.Type" },
            { "MTypeObject", "System.Type" },
            { "MArray", "Array" },
        };

        private static readonly string[] CSharpVectorTypes =
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
        };

        private static bool GenerateCSharpUseFixedBuffer(string managedType)
        {
            return managedType == "byte" || managedType == "char" ||
                   managedType == "short" || managedType == "ushort" ||
                   managedType == "int" || managedType == "uint" ||
                   managedType == "long" || managedType == "ulong" ||
                   managedType == "float" || managedType == "double";
        }

        private static string GenerateCSharpDefaultValueNativeToManaged(BuildData buildData, string value, ApiTypeInfo caller, TypeInfo valueType = null, bool attribute = false, string managedType = null)
        {
            ApiTypeInfo apiType = null;
            if (string.IsNullOrEmpty(value))
            {
                if (attribute && valueType != null && !valueType.IsArray)
                {
                    //if (valueType.Type == "")
                    //ScriptingObjectReference
                    apiType = FindApiTypeInfo(buildData, valueType, caller);

                    // Object reference
                    if (apiType != null && apiType.IsScriptingObject)
                        return "null";
                }

                return null;
            }

            // Special case for Engine TEXT macro
            if (value.StartsWith("TEXT(\"") && value.EndsWith("\")"))
                return value.Substring(5, value.Length - 6);

            // Pointer constant value
            if (managedType != null && managedType == "IntPtr" &&
                (value == "nullptr" || value == "NULL" || value == "0"))
            {
                return "new IntPtr()";
            }

            // In-built constants
            switch (value)
            {
            case "nullptr":
            case "NULL":
            case "String::Empty":
            case "StringAnsi::Empty":
            case "StringAnsiView::Empty":
            case "StringView::Empty": return "null";
            case "MAX_int8": return "sbyte.MaxValue";
            case "MAX_uint8": return "byte.MaxValue";
            case "MAX_int16": return "short.MaxValue";
            case "MAX_uint16": return "ushort.MaxValue";
            case "MAX_int32": return "int.MaxValue";
            case "MAX_uint32": return "uint.MaxValue";
            case "MAX_int64": return "long.MaxValue";
            case "MAX_uint64": return "ulong.MaxValue";
            case "MAX_float": return "float.MaxValue";
            case "MAX_double": return "double.MaxValue";
            case "true":
            case "false": return value;
            }

            // Handle float{_} style type of default values
            if (valueType != null && value.StartsWith($"{valueType.Type}") && value.EndsWith("}"))
            {
                value = value.Replace($"{valueType.Type}", "").Replace("{", "").Replace("}", "").Trim();
                if (string.IsNullOrEmpty(value))
                {
                    value = $"default({valueType.Type})";
                    return value;
                }
            }

            // Handle C++ bracket default values that are not arrays
            if (value.StartsWith("{") && value.EndsWith("}") && valueType != null && !valueType.IsArray && valueType.Type != "Array")
            {
                value = value.Replace("{", "").Replace("}", "").Trim();
                if (string.IsNullOrEmpty(value))
                {
                    value = $"default({valueType.Type})";
                    return value;
                }
            }

            // Numbers
            if (float.TryParse(value, out _) || (value[value.Length - 1] == 'f' && float.TryParse(value.Substring(0, value.Length - 1), out _)))
            {
                value = value.Replace(".f", ".0f");
                // If the value type is different than the value (eg. value is int but the field is float) then cast it for the [DefaultValue] attribute
                if (valueType != null && attribute)
                    return $"({GenerateCSharpNativeToManaged(buildData, valueType, caller)}){value}";
                return value;
            }

            value = value.Replace("::", ".");
            var dot = value.LastIndexOf('.');
            if (dot != -1)
            {
                var type = new TypeInfo(value.Substring(0, dot));
                apiType = FindApiTypeInfo(buildData, type, caller);
            }

            if (attribute)
            {
                // Value constructors (eg. Vector2(1, 2))
                if (value.Contains('(') && value.Contains(')'))
                {
                    // Support for in-built types
                    foreach (var vectorType in CSharpVectorTypes)
                    {
                        if (value.Length > vectorType.Length + 4 && value.StartsWith(vectorType) && value[vectorType.Length] == '(')
                            return $"typeof({vectorType}), \"{value.Substring(vectorType.Length + 1, value.Length - vectorType.Length - 2).Replace(".f", ".0").Replace("f", "")}\"";
                    }

                    return null;
                }

                // Constants (eg. Vector2::Zero)
                if (value.Contains('.'))
                {
                    // Support for in-built constants
                    switch (value)
                    {
                    case "Vector2.Zero": return "typeof(Vector2), \"0,0\"";
                    case "Vector2.One": return "typeof(Vector2), \"1,1\"";
                    case "Float2.Zero": return "typeof(Float2), \"0,0\"";
                    case "Float2.One": return "typeof(Float2), \"1,1\"";
                    case "Double2.Zero": return "typeof(Double2), \"0,0\"";
                    case "Double2.One": return "typeof(Double2), \"1,1\"";
                    case "Int2.Zero": return "typeof(Int2), \"0,0\"";
                    case "Int2.One": return "typeof(Int2), \"1,1\"";

                    case "Vector3.Zero": return "typeof(Vector3), \"0,0,0\"";
                    case "Vector3.One": return "typeof(Vector3), \"1,1,1\"";
                    case "Float3.Zero": return "typeof(Float3), \"0,0,0\"";
                    case "Float3.One": return "typeof(Float3), \"1,1,1\"";
                    case "Double3.Zero": return "typeof(Double3), \"0,0,0\"";
                    case "Double3.One": return "typeof(Double3), \"1,1,1\"";
                    case "Int3.Zero": return "typeof(Int3), \"0,0,0\"";
                    case "Int3.One": return "typeof(Int3), \"1,1,1\"";

                    case "Vector4.Zero": return "typeof(Vector4), \"0,0,0,0\"";
                    case "Vector4.One": return "typeof(Vector4), \"1,1,1,1\"";
                    case "Double4.Zero": return "typeof(Double4), \"0,0,0,0\"";
                    case "Double4.One": return "typeof(Double4), \"1,1,1,1\"";
                    case "Float4.Zero": return "typeof(Float4), \"0,0,0,0\"";
                    case "Float4.One": return "typeof(Float4), \"1,1,1,1\"";
                    case "Int4.Zero": return "typeof(Float4), \"0,0,0,0\"";
                    case "Int4.One": return "typeof(Float4), \"1,1,1,1\"";

                    case "Quaternion.Identity": return "typeof(Quaternion), \"0,0,0,1\"";
                    }

                    // Enums
                    if (apiType != null && apiType.IsEnum)
                        return value;

                    return null;
                }
            }

            // Skip constants unsupported in C#
            if (apiType != null && apiType.IsStruct)
                return null;

            // Special case for array initializers
            if (value.StartsWith("{") && value.EndsWith("}") && !attribute)
            {
                if (valueType == null)
                    throw new Exception("Unknown type for value initializer of default value " + value);
                TypeInfo itemType;
                if (valueType.Type == "Array")
                    itemType = valueType.GenericArgs[0];
                else if (valueType.IsArray)
                {
                    itemType = (TypeInfo)value.Clone();
                    itemType.IsArray = false;
                }
                else
                    throw new Exception("Cannot use array initializer as default value for type " + valueType);
                var sb = GetStringBuilder();
                var items = new List<string>();
                var braces = 0;
                for (int i = 1; i < value.Length - 1; i++)
                {
                    var c = value[i];
                    if (c == ',' && braces == 0)
                    {
                        items.Add(sb.ToString());
                        sb.Clear();
                        continue;
                    }
                    if (c == '(')
                        braces++;
                    else if (c == ')')
                        braces--;
                    sb.Append(c);
                }
                items.Add(sb.ToString());
                sb.Clear();
                sb.Append("new ").Append(GenerateCSharpNativeToManaged(buildData, valueType, caller)).Append('{');
                for (int i = 0; i < items.Count; i++)
                    sb.Append(GenerateCSharpDefaultValueNativeToManaged(buildData, items[i], caller, itemType, attribute)).Append(',');
                sb.Append('}');
                var result = sb.ToString();
                PutStringBuilder(sb);
                return result;
            }

            // Special case for value constructors
            if (value.Contains('(') && value.Contains(')'))
                return "new " + value;

            // Special case for non-strongly typed enums
            if (valueType != null && !value.Contains('.', StringComparison.Ordinal))
            {
                apiType = FindApiTypeInfo(buildData, valueType, caller);
                if (apiType is EnumInfo)
                    return $"{apiType.Name}.{value}";
            }

            return value;
        }

        private static string GenerateCSharpNativeToManaged(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, bool marshalling = false)
        {
            string result;
            if (typeInfo == null || typeInfo.Type == null)
                throw new ArgumentNullException();

            // Use dynamic array as wrapper container for fixed-size native arrays
            if (typeInfo.IsArray)
            {
                typeInfo.IsArray = false;
                result = GenerateCSharpNativeToManaged(buildData, typeInfo, caller, marshalling);
                typeInfo.IsArray = true;
                return result + "[]";
            }

            // Special case for bit-fields
            if (typeInfo.IsBitField)
            {
                if (typeInfo.BitSize == 1)
                    return "bool";
                throw new NotImplementedException("TODO: support bit-fields with more than 1 bit.");
            }

            // In-build basic
            if (CSharpNativeToManagedBasicTypes.TryGetValue(typeInfo.Type, out result))
            {
                if (typeInfo.IsPtr)
                    return result + '*';
                return result;
            }

            // In-build default
            if (CSharpNativeToManagedDefault.TryGetValue(typeInfo.Type, out result))
                return result;

            // Object reference property
            if (typeInfo.IsObjectRef)
                return GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller, marshalling);
            if (typeInfo.Type == "SoftTypeReference" || typeInfo.Type == "SoftObjectReference")
                return typeInfo.Type;

            // Array or Span or DataContainer
#if USE_NETCORE
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span" || typeInfo.Type == "DataContainer" || typeInfo.Type == "MonoArray" || typeInfo.Type == "MArray") && typeInfo.GenericArgs != null)
#else
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span" || typeInfo.Type == "DataContainer") && typeInfo.GenericArgs != null)
#endif
            {
                var arrayTypeInfo = typeInfo.GenericArgs[0];
                if (marshalling)
                {
                    // Convert array that uses different type for marshalling
                    var arrayApiType = FindApiTypeInfo(buildData, arrayTypeInfo, caller);
                    if (arrayApiType != null && arrayApiType.MarshalAs != null)
                        arrayTypeInfo = arrayApiType.MarshalAs;
                }
                return GenerateCSharpNativeToManaged(buildData, arrayTypeInfo, caller) + "[]";
            }

            // Dictionary
            if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
                return string.Format("System.Collections.Generic.Dictionary<{0}, {1}>", GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller, marshalling), GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[1], caller, marshalling));

            // HashSet
            if (typeInfo.Type == "HashSet" && typeInfo.GenericArgs != null)
                return string.Format("System.Collections.Generic.HashSet<{0}>", GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller, marshalling));

            // BitArray
            if (typeInfo.Type == "BitArray" && typeInfo.GenericArgs != null)
                return "bool[]";

            // BytesContainer
            if (typeInfo.Type == "BytesContainer" && typeInfo.GenericArgs == null)
                return "byte[]";

            // Function
            if (typeInfo.Type == "Function" && typeInfo.GenericArgs != null)
            {
                if (typeInfo.GenericArgs.Count == 0)
                    throw new Exception("Missing function return type.");
#if USE_NETCORE
                // NetCore doesn't allow using Marshal.GetDelegateForFunctionPointer on generic delegate type (eg. Action<int>) thus generate explicit delegate type for function parameter
                // https://github.com/dotnet/runtime/issues/36110
                if (typeInfo.GenericArgs.Count == 1 && typeInfo.GenericArgs[0].Type == "void")
                    return "Action";
                // TODO: generate delegates globally in the module namespace to share more code (smaller binary size)
                var key = string.Empty;
                for (int i = 0; i < typeInfo.GenericArgs.Count; i++)
                    key += GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[i], caller, marshalling);
                if (!CSharpAdditionalCodeCache.TryGetValue(key, out var delegateName))
                {
                    delegateName = "Delegate" + CSharpAdditionalCodeCache.Count;
                    var signature = $"public delegate {GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller, marshalling)} {delegateName}(";
                    for (int i = 1; i < typeInfo.GenericArgs.Count; i++)
                    {
                        if (i != 1)
                            signature += ", ";
                        signature += GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[i], caller, marshalling);
                        signature += $" arg{(i - 1)}";
                    }
                    signature += ");";
                    CSharpAdditionalCode.Add("/// <summary>Function delegate.</summary>");
                    CSharpAdditionalCode.Add(signature);
                    CSharpAdditionalCodeCache.Add(key, delegateName);
                }
                return delegateName;
#else
                if (typeInfo.GenericArgs.Count > 1)
                {
                    var args = string.Empty;
                    args += GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[1], caller);
                    for (int i = 2; i < typeInfo.GenericArgs.Count; i++)
                        args += ", " + GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[i], caller);
                    if (typeInfo.GenericArgs[0].Type == "void")
                        return string.Format("Action<{0}>", args);
                    return string.Format("Func<{0}, {1}>", args, GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller));
                }
                if (typeInfo.GenericArgs[0].Type == "void")
                    return "Action";
                return string.Format("Func<{0}>", GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller));
#endif
            }

            // Find API type info
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            var typeName = typeInfo.Type.Replace("::", ".");
            if (typeInfo.GenericArgs != null)
            {
                typeName += '<';
                foreach (var arg in typeInfo.GenericArgs)
                    typeName += GenerateCSharpNativeToManaged(buildData, arg, caller, marshalling);
                typeName += '>';
            }
            if (apiType != null)
            {
                if (marshalling && apiType.MarshalAs != null)
                    return GenerateCSharpNativeToManaged(buildData, apiType.MarshalAs, caller);

                // Add reference to the namespace
                CSharpUsedNamespaces.Add(apiType.Namespace);
                var apiTypeParent = apiType.Parent;
                while (apiTypeParent != null && !(apiTypeParent is FileInfo))
                {
                    CSharpUsedNamespaces.Add(apiTypeParent.Namespace);
                    apiTypeParent = apiTypeParent.Parent;
                }

                if (apiType.IsScriptingObject || apiType.IsInterface)
                    return typeName;
                if (typeInfo.IsPtr && apiType.IsPod)
                    return typeName + '*';
                if (apiType is LangType && CSharpNativeToManagedBasicTypes.TryGetValue(apiType.Name, out result))
                    return result;
            }

            // Pointer
            if (typeInfo.IsPtr)
                return "IntPtr";

            return typeName;
        }

        private static string GenerateCSharpManagedToNativeType(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller, bool marshalling = false)
        {
            // Fixed-size array
            if (typeInfo.IsArray)
                return GenerateCSharpNativeToManaged(buildData, typeInfo, caller, marshalling);

            // Find API type info
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                // Add reference to the namespace
                CSharpUsedNamespaces.Add(apiType.Namespace);
                var apiTypeParent = apiType.Parent;
                while (apiTypeParent != null && !(apiTypeParent is FileInfo))
                {
                    CSharpUsedNamespaces.Add(apiTypeParent.Namespace);
                    apiTypeParent = apiTypeParent.Parent;
                }

                if (apiType.MarshalAs != null)
                    return GenerateCSharpManagedToNativeType(buildData, apiType.MarshalAs, caller, marshalling);
                if (apiType.IsScriptingObject || apiType.IsInterface)
                    return "IntPtr";
            }

            // Object reference property
            if (typeInfo.IsObjectRef)
                return "IntPtr";

            // Function
            if (typeInfo.Type == "Function" && typeInfo.GenericArgs != null)
                return "IntPtr";

            return GenerateCSharpNativeToManaged(buildData, typeInfo, caller, marshalling);
        }

        private static string GenerateCSharpManagedToNativeConverter(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller)
        {
            // Use dynamic array as wrapper container for fixed-size native arrays
            if (typeInfo.IsArray)
                return string.Empty;

            // Special case for bit-fields
            if (typeInfo.IsBitField)
            {
                if (typeInfo.BitSize == 1)
                    return string.Empty;
                throw new NotImplementedException("TODO: support bit-fields with more than 1 bit.");
            }

            switch (typeInfo.Type)
            {
            case "String":
            case "StringView":
            case "StringAnsi":
            case "StringAnsiView":
                // String
                return string.Empty;
            case "ScriptingObject":
            case "PersistentScriptingObject":
            case "ManagedScriptingObject":
                // object
                return "FlaxEngine.Object.GetUnmanagedPtr({0})";
            case "Function":
                // delegate
                return "NativeInterop.GetFunctionPointerForDelegate({0})";
            case "Array":
            case "Span":
            case "DataContainer":
                if (typeInfo.GenericArgs != null)
                {
                    // Convert array that uses different type for marshalling
                    var arrayTypeInfo = typeInfo.GenericArgs[0];
                    var arrayApiType = FindApiTypeInfo(buildData, arrayTypeInfo, caller);
                    if (arrayApiType != null && arrayApiType.MarshalAs != null)
                        return $"{{0}}.ConvertArray(x => ({GenerateCSharpNativeToManaged(buildData, arrayApiType.MarshalAs, caller)})x)";
                }
                return string.Empty;
            default:
                var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
                if (apiType != null)
                {
                    // Scripting Object
                    if (apiType.IsScriptingObject)
                        return "FlaxEngine.Object.GetUnmanagedPtr({0})";

                    // interface
                    if (apiType.IsInterface)
                        return string.Format("FlaxEngine.Object.GetUnmanagedInterface({{0}}, typeof({0}))", apiType.Name);
                }

                // Object reference property
                if (typeInfo.IsObjectRef)
                    return "FlaxEngine.Object.GetUnmanagedPtr({0})";

                // Default
                return string.Empty;
            }
        }

        private static void GenerateCSharpManagedTypeInternals(BuildData buildData, ApiTypeInfo apiTypeInfo, StringBuilder contents, string indent)
        {
            if (CSharpAdditionalCode.Count != 0)
            {
                contents.AppendLine();
                foreach (var e in CSharpAdditionalCode)
                    contents.Append(indent).AppendLine(e);
                CSharpAdditionalCode.Clear();
                CSharpAdditionalCodeCache.Clear();
            }
            GenerateCSharpTypeInternals?.Invoke(buildData, apiTypeInfo, contents, indent);
        }

        private static void GenerateCSharpWrapperFunction(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo caller, FunctionInfo functionInfo)
        {
            string returnValueType;
            if (UsePassByReference(buildData, functionInfo.ReturnType, caller))
            {
                returnValueType = "void";
            }
            else
            {
                var apiType = FindApiTypeInfo(buildData, functionInfo.ReturnType, caller);
                if (apiType != null && apiType.MarshalAs != null)
                    returnValueType = GenerateCSharpNativeToManaged(buildData, apiType.MarshalAs, caller, true);
                else
                    returnValueType = GenerateCSharpNativeToManaged(buildData, functionInfo.ReturnType, caller, true);
            }

#if USE_NETCORE
            var returnNativeType = GenerateCSharpManagedToNativeType(buildData, functionInfo.ReturnType, caller);
            string returnMarshalType = "";
            if (returnValueType == "bool")
                returnMarshalType = "MarshalAs(UnmanagedType.U1)";
            else if (returnValueType == "System.Type")
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.SystemTypeMarshaller))";
            else if (returnValueType == "CultureInfo")
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.CultureInfoMarshaller))";
            else if (functionInfo.ReturnType.Type == "Variant") // object
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.ManagedHandleMarshaller))";
            else if (FindApiTypeInfo(buildData, functionInfo.ReturnType, caller)?.IsInterface ?? false)
            {
                var apiType = FindApiTypeInfo(buildData, functionInfo.ReturnType, caller);
                var fullReturnValueType = returnValueType;
                if (!string.IsNullOrEmpty(apiType?.Namespace))
                    fullReturnValueType = $"{apiType.Namespace}.Interop.{returnValueType}";

                // Interfaces are not supported by NativeMarshallingAttribute, marshal the parameter
                returnMarshalType = $"MarshalUsing(typeof({returnValueType}Marshaller))";
            }
            else if (functionInfo.ReturnType.Type == "MonoArray" || functionInfo.ReturnType.Type == "MArray")
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.SystemArrayMarshaller))";
            else if (returnValueType == "object[]")
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.SystemObjectArrayMarshaller))";
            else if (functionInfo.ReturnType.Type == "Array" || functionInfo.ReturnType.Type == "Span" || functionInfo.ReturnType.Type == "DataContainer" || functionInfo.ReturnType.Type == "BytesContainer" || returnNativeType == "Array")
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = nameof(__returnCount))";
            else if (functionInfo.ReturnType.Type == "Dictionary")
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.DictionaryMarshaller<,>), ConstantElementCount = 0)";
            else if (returnValueType == "byte[]")
                returnMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = \"__returnCount\")";
            else if (returnValueType == "bool[]")
            {
                // Boolean arrays does not support custom marshalling for some unknown reason
                returnMarshalType = $"MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = {functionInfo.Parameters.Count + (functionInfo.Glue.CustomParameters?.Count ?? 0)})";
            }
#endif
#if !USE_NETCORE
            contents.AppendLine().Append(indent).Append("[MethodImpl(MethodImplOptions.InternalCall)]");
            contents.AppendLine().Append(indent).Append("internal static partial ");
#else
            if (string.IsNullOrEmpty(functionInfo.Glue.LibraryEntryPoint))
                throw new Exception($"Function {caller.FullNameNative}::{functionInfo.Name} has missing entry point for library import.");
            contents.AppendLine().Append(indent).Append($"[LibraryImport(\"{caller.ParentModule.Module.BinaryModuleName}\", EntryPoint = \"{functionInfo.Glue.LibraryEntryPoint}\", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(FlaxEngine.Interop.StringMarshaller))]");
            if (!string.IsNullOrEmpty(returnMarshalType))
                contents.AppendLine().Append(indent).Append($"[return: {returnMarshalType}]");
            contents.AppendLine().Append(indent).Append("internal static partial ");
#endif
            contents.Append(returnValueType).Append(" Internal_").Append(functionInfo.UniqueName).Append('(');

            var separator = false;
            if (!functionInfo.IsStatic)
            {
                contents.Append("IntPtr __obj");
                separator = true;
            }

            foreach (var parameterInfo in functionInfo.Parameters)
            {
                if (separator)
                    contents.Append(", ");
                separator = true;

                var nativeType = GenerateCSharpManagedToNativeType(buildData, parameterInfo.Type, caller, true);
#if USE_NETCORE
                string parameterMarshalType = "";
                if (nativeType == "System.Type")
                    parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.SystemTypeMarshaller))";
                else if (parameterInfo.Type.Type == "CultureInfo")
                    parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.CultureInfoMarshaller))";
                else if (parameterInfo.Type.Type == "Variant") // object
                    parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.ManagedHandleMarshaller))";
                else if (parameterInfo.Type.Type == "MonoArray" || parameterInfo.Type.Type == "MArray")
                    parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.SystemArrayMarshaller))";
                else if (nativeType == "object[]")
                    parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.SystemObjectArrayMarshaller))";
                else if (parameterInfo.Type.Type == "Array" && parameterInfo.Type.GenericArgs.Count > 0 && parameterInfo.Type.GenericArgs[0].Type == "bool")
                    parameterMarshalType = $"MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = {(!functionInfo.IsStatic ? 1 : 0) + functionInfo.Parameters.Count + (functionInfo.Glue.CustomParameters.FindIndex(x => x.Name == $"__{parameterInfo.Name}Count"))})";
                else if (parameterInfo.Type.Type == "Array" || parameterInfo.Type.Type == "Span" || parameterInfo.Type.Type == "DataContainer" || parameterInfo.Type.Type == "BytesContainer" || nativeType == "Array")
                {
                    parameterMarshalType = $"MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = \"__{parameterInfo.Name}Count\")";
                    if (!parameterInfo.IsOut && !parameterInfo.IsRef)
                        parameterMarshalType += ", In"; // The usage of 'LibraryImportAttribute' does not follow recommendations. It is recommended to use explicit '[In]' and '[Out]' attributes on array parameters.
                }
                else if (parameterInfo.Type.Type == "Dictionary")
                    parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.DictionaryMarshaller<,>), ConstantElementCount = 0)";
                else if (nativeType == "bool")
                    parameterMarshalType = "MarshalAs(UnmanagedType.U1)";
                else if (nativeType == "char")
                    parameterMarshalType = "MarshalAs(UnmanagedType.I2)";
                else if (nativeType.EndsWith("[]"))
                {
                    parameterMarshalType = $"MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>))";
                }

                if (!string.IsNullOrEmpty(parameterMarshalType))
                    contents.Append($"[{parameterMarshalType}] ");
#endif
                if (parameterInfo.IsOut)
                    contents.Append("out ");
                else if (parameterInfo.IsRef || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append("ref ");

                // Out parameters that need additional converting will be converted at the native side (eg. object reference)
                if (parameterInfo.IsOut && !string.IsNullOrEmpty(GenerateCSharpManagedToNativeConverter(buildData, parameterInfo.Type, caller)))
                    nativeType = parameterInfo.Type.Type;

                contents.Append(nativeType);
                contents.Append(' ');
                contents.Append(parameterInfo.Name);
            }

            var customParametersCount = functionInfo.Glue.CustomParameters?.Count ?? 0;
            for (var i = 0; i < customParametersCount; i++)
            {
                var parameterInfo = functionInfo.Glue.CustomParameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                var nativeType = GenerateCSharpManagedToNativeType(buildData, parameterInfo.Type, caller, true);
#if USE_NETCORE
                string parameterMarshalType = "";
                if (parameterInfo.IsOut && parameterInfo.DefaultValue == "var __resultAsRef")
                {
                    if (parameterInfo.Type.Type == "Array" || parameterInfo.Type.Type == "Span" || parameterInfo.Type.Type == "DataContainer" || parameterInfo.Type.Type == "BytesContainer")
                        parameterMarshalType = $"MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = \"{parameterInfo.Name}Count\")";
                    else if (parameterInfo.Type.Type == "Dictionary")
                        parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.DictionaryMarshaller<,>), ConstantElementCount = 0)";
                }
                if (nativeType == "System.Type")
                    parameterMarshalType = "MarshalUsing(typeof(FlaxEngine.Interop.SystemTypeMarshaller))";

                if (!string.IsNullOrEmpty(parameterMarshalType))
                    contents.Append($"[{parameterMarshalType}] ");
#endif
                if (parameterInfo.IsOut)
                    contents.Append("out ");
                else if (parameterInfo.IsRef || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append("ref ");
                contents.Append(nativeType);
                contents.Append(' ');
                contents.Append(parameterInfo.Name);
            }

            contents.Append(')').Append(';').AppendLine();
        }

        private static void GenerateCSharpWrapperFunctionCall(BuildData buildData, StringBuilder contents, ApiTypeInfo caller, FunctionInfo functionInfo, bool isSetter = false)
        {
#if USE_NETCORE
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (parameterInfo.Type.IsArray || parameterInfo.Type.Type == "Array" || parameterInfo.Type.Type == "Span" || parameterInfo.Type.Type == "BytesContainer" || parameterInfo.Type.Type == "DataContainer" || parameterInfo.Type.Type == "BitArray")
                {
                    if (!parameterInfo.IsOut)
                        contents.Append($"var __{parameterInfo.Name}Count = {(isSetter ? "value" : parameterInfo.Name)}?.Length ?? 0; ");
                }
            }
#endif
            if (functionInfo.Glue.UseReferenceForResult)
            {
            }
            else if (!functionInfo.ReturnType.IsVoid)
            {
                contents.Append("return ");
            }
            contents.Append("Internal_").Append(functionInfo.UniqueName).Append('(');

            // Pass parameters
            var separator = false;
            if (!functionInfo.IsStatic)
            {
                contents.Append("__unmanagedPtr");
                separator = true;
            }
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                if (parameterInfo.IsOut)
                    contents.Append("out ");
                else if (parameterInfo.IsRef || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append("ref ");

                var convertFunc = GenerateCSharpManagedToNativeConverter(buildData, parameterInfo.Type, caller);
                var paramName = isSetter ? "value" : parameterInfo.Name;
                if (string.IsNullOrWhiteSpace(convertFunc) || parameterInfo.IsOut)
                {
                    // Pass value
                    contents.Append(paramName);
                }
                else
                {
                    if (parameterInfo.IsRef)
                        throw new Exception($"Cannot use Ref meta on parameter {parameterInfo} in function {functionInfo.Name} in {caller}. Use API_PARAM(Out) if you want to pass it as a result reference.");

                    // Convert value
                    contents.Append(string.Format(convertFunc, paramName));
                }
            }

            // Pass additional parameters
            var customParametersCount = functionInfo.Glue.CustomParameters?.Count ?? 0;
            for (var i = 0; i < customParametersCount; i++)
            {
                var parameterInfo = functionInfo.Glue.CustomParameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                var convertFunc = GenerateCSharpManagedToNativeConverter(buildData, parameterInfo.Type, caller);
                if (string.IsNullOrWhiteSpace(convertFunc) || parameterInfo.IsOut)
                {
                    if (parameterInfo.IsOut)
                        contents.Append("out ");
                    else if (parameterInfo.IsRef || UsePassByReference(buildData, parameterInfo.Type, caller))
                        contents.Append("ref ");

                    // Pass value
                    contents.Append(parameterInfo.DefaultValue);
                }
                else
                {
                    // Convert value
                    contents.Append(string.Format(convertFunc, parameterInfo.DefaultValue));
                }
            }

            contents.Append(')');
            if ((functionInfo.ReturnType.Type == "Array" || functionInfo.ReturnType.Type == "Span" || functionInfo.ReturnType.Type == "DataContainer") && functionInfo.ReturnType.GenericArgs != null)
            {
                // Convert array that uses different type for marshalling
                var arrayTypeInfo = functionInfo.ReturnType.GenericArgs[0];
                var arrayApiType = FindApiTypeInfo(buildData, arrayTypeInfo, caller);
                if (arrayApiType != null && arrayApiType.MarshalAs != null)
                    contents.Append($".ConvertArray(x => ({GenerateCSharpNativeToManaged(buildData, arrayTypeInfo, caller)})x)");
            }
            contents.Append(';');

            // Return result
            if (functionInfo.Glue.UseReferenceForResult)
            {
                contents.Append(" return __resultAsRef;");
            }
        }

        private static void GenerateCSharpComment(StringBuilder contents, string indent, string[] comment, bool skipMeta = false)
        {
            if (comment == null)
                return;
            foreach (var c in comment)
            {
                if (skipMeta && (c.Contains("/// <returns>") || c.Contains("<param name=")))
                    continue;
                contents.Append(indent).Append(c.Replace("::", ".")).AppendLine();
            }
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, string attributes = null, string[] comment = null, bool canUseTooltip = false, bool useUnmanaged = false, string defaultValue = null, string deprecatedMessage = null, TypeInfo defaultValueType = null)
        {
#if AUTO_DOC_TOOLTIPS
            var writeTooltip = true;
#endif
            var writeDefaultValue = true;
            if (!string.IsNullOrEmpty(attributes))
            {
                // Write attributes
                contents.Append(indent).Append('[').Append(attributes).Append(']').AppendLine();
#if AUTO_DOC_TOOLTIPS
                writeTooltip = !attributes.Contains("Tooltip(") && !attributes.Contains("HideInEditor");
#endif
                writeDefaultValue = !attributes.Contains("DefaultValue(");
            }

            if (useUnmanaged)
            {
                // Write attribute for C++ calling code
                contents.Append(indent).AppendLine("[Unmanaged]");

                // Skip boilerplate code when using debugger
                //contents.Append(indent).AppendLine("[System.Diagnostics.DebuggerStepThrough]");
            }
            if (deprecatedMessage != null || apiTypeInfo.IsDeprecated)
            {
                // Deprecated type
                if (!string.IsNullOrEmpty(apiTypeInfo.DeprecatedMessage))
                    contents.Append(indent).AppendLine($"[Obsolete(\"{apiTypeInfo.DeprecatedMessage}\")]");
                else if (!string.IsNullOrEmpty(deprecatedMessage))
                    contents.Append(indent).AppendLine($"[Obsolete(\"{deprecatedMessage}\")]");
                else
                    contents.Append(indent).AppendLine("[Obsolete]");
            }

#if AUTO_DOC_TOOLTIPS
            if (canUseTooltip &&
                writeTooltip &&
                buildData.Configuration != TargetConfiguration.Release &&
                comment != null &&
                comment.Length >= 3 &&
                comment[0] == "/// <summary>")
            {
                // Write documentation comment as tooltip
                string tooltip = null;
                for (int i = 1; i < comment.Length && comment[i] != "/// </summary>"; i++)
                {
                    if (comment[i].StartsWith("/// "))
                    {
                        if (tooltip == null)
                            tooltip = string.Empty;
                        else
                            tooltip += " ";
                        tooltip += comment[i].Substring(4);
                    }
                }
                if (tooltip != null)
                {
                    if (tooltip.StartsWith("Gets the "))
                        tooltip = "The " + tooltip.Substring(9);
                    tooltip = tooltip.Replace("\"", "\"\"");
                    contents.Append(indent).Append("[Tooltip(@\"").Append(tooltip).Append("\")]").AppendLine();
                }
            }
#endif

            if (writeDefaultValue)
            {
                // Write default value attribute
                defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, defaultValue, apiTypeInfo, defaultValueType, true);
                if (defaultValue != null)
                    contents.Append(indent).Append("[DefaultValue(").Append(defaultValue).Append(")]").AppendLine();
            }
            
            // Check if array has fixed allocation and add in MaxCount Collection attribute if a Collection attribute does not already exist.
            if (defaultValueType != null && (string.IsNullOrEmpty(attributes) || !attributes.Contains("Collection", StringComparison.Ordinal)))
            {
                // Array or Span or DataContainer
#if USE_NETCORE
                if ((defaultValueType.Type == "Array" || defaultValueType.Type == "Span" || defaultValueType.Type == "DataContainer" || defaultValueType.Type == "MonoArray" || defaultValueType.Type == "MArray") && defaultValueType.GenericArgs != null)
#else
                if ((defaultValueType.Type == "Array" || defaultValueType.Type == "Span" || defaultValueType.Type == "DataContainer") && defaultValueType.GenericArgs != null)
#endif
                {
                    if (defaultValueType.GenericArgs.Count > 1)
                    {
                        var allocationArg = defaultValueType.GenericArgs[1];
                        if (allocationArg.Type.Contains("FixedAllocation", StringComparison.Ordinal) && allocationArg.GenericArgs.Count > 0)
                        {
                            if (int.TryParse(allocationArg.GenericArgs[0].ToString(), out int allocation))
                            {
                                contents.Append(indent).Append($"[Collection(MaxCount={allocation.ToString()})]").AppendLine();
                            }
                        }
                    }
                }
            }
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, bool useUnmanaged, string defaultValue = null, TypeInfo defaultValueType = null)
        {
            GenerateCSharpAttributes(buildData, contents, indent, apiTypeInfo, apiTypeInfo.Attributes, apiTypeInfo.Comment, true, useUnmanaged, defaultValue, null, defaultValueType);
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, MemberInfo memberInfo, bool useUnmanaged, string defaultValue = null, TypeInfo defaultValueType = null)
        {
            GenerateCSharpAttributes(buildData, contents, indent, apiTypeInfo, memberInfo.Attributes, memberInfo.Comment, true, useUnmanaged, defaultValue, memberInfo.DeprecatedMessage, defaultValueType);
        }

        private static bool GenerateCSharpStructureUseDefaultInitialize(BuildData buildData, StructureInfo structureInfo)
        {
            foreach (var fieldInfo in structureInfo.Fields)
            {
                // Ignore non-member fields
                if (fieldInfo.IsStatic || fieldInfo.IsReadOnly || fieldInfo.IsConstexpr)
                    continue;

                // Check if any field has default value
                if (fieldInfo.HasAttribute("DefaultValue") ||
                    fieldInfo.HasDefaultValue)
                    return true;

                // Check any nested structure
                var apiType = FindApiTypeInfo(buildData, fieldInfo.Type, structureInfo);
                if (apiType is StructureInfo fieldTypeStruct && GenerateCSharpStructureUseDefaultInitialize(buildData, fieldTypeStruct))
                    return true;
            }
            return false;
        }

        private static string GenerateCSharpAccessLevel(AccessLevel access)
        {
            if (access == AccessLevel.Protected)
                return "protected ";
            if (access == AccessLevel.Private)
                return "private ";
            if (access == AccessLevel.Internal)
                return "internal ";
            return "public ";
        }

        private static void GenerateCSharpClass(BuildData buildData, StringBuilder contents, string indent, ClassInfo classInfo)
        {
            var useUnmanaged = classInfo.IsStatic || classInfo.IsScriptingObject;
            contents.AppendLine();

            // Namespace begin
            string interopNamespace = "";
            if (!string.IsNullOrEmpty(classInfo.Namespace))
            {
                interopNamespace = $"{classInfo.Namespace}.Interop";
                contents.AppendLine($"namespace {classInfo.Namespace}");
                contents.AppendLine("{");
                indent += "    ";
            }

            // Class docs
            GenerateCSharpComment(contents, indent, classInfo.Comment);

            // Class begin
            GenerateCSharpAttributes(buildData, contents, indent, classInfo, useUnmanaged);
#if USE_NETCORE
            string marshallerName = "";
            string marshallerFullName = "";
            if (!classInfo.IsStatic)
            {
                marshallerName = classInfo.Name + "Marshaller";
#if MARSHALLER_FULL_NAME
                marshallerFullName = !string.IsNullOrEmpty(interopNamespace) ? $"{interopNamespace}.{marshallerName}" : marshallerName;
#else
                if (!string.IsNullOrEmpty(interopNamespace))
                    CSharpUsedNamespaces.Add(interopNamespace);
                marshallerFullName = marshallerName;
#endif
                contents.Append(indent).AppendLine($"[NativeMarshalling(typeof({marshallerFullName}))]");
            }
#endif
            contents.Append(indent).Append(GenerateCSharpAccessLevel(classInfo.Access));
            if (classInfo.IsStatic)
                contents.Append("static ");
            else if (classInfo.IsSealed)
                contents.Append("sealed ");
            else if (classInfo.IsAbstract)
                contents.Append("abstract ");
            contents.Append("unsafe partial class ").Append(classInfo.Name);
            var hasBase = classInfo.BaseType != null && !classInfo.IsBaseTypeHidden;
            if (hasBase)
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, new TypeInfo(classInfo.BaseType), classInfo));
            var hasInterface = false;
            if (classInfo.Interfaces != null)
            {
                foreach (var interfaceInfo in classInfo.Interfaces)
                {
                    if (interfaceInfo.Access != AccessLevel.Public)
                        continue;
                    if (hasInterface || hasBase)
                        contents.Append(", ");
                    else
                        contents.Append(" : ");
                    hasInterface = true;
                    contents.Append(interfaceInfo.FullNameManaged);
                }
            }
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Constructor for managed objects
            if (!classInfo.IsStatic && !classInfo.NoConstructor)
            {
                contents.AppendLine();
                contents.Append(indent).AppendLine("/// <summary>");
                contents.Append(indent).Append("/// Initializes a new instance of the <see cref=\"").Append(classInfo.Name).AppendLine("\"/>.");
                contents.Append(indent).AppendLine("/// </summary>");
                contents.Append(indent).Append(classInfo.IsAbstract ? "protected " : "public ").Append(classInfo.Name).AppendLine("() : base()");
                contents.Append(indent + "{").AppendLine();
                contents.Append(indent + "}").AppendLine();
            }

            // Events
            foreach (var eventInfo in classInfo.Events)
            {
                if (eventInfo.IsHidden)
                    continue;
                if (!useUnmanaged)
                    throw new NotImplementedException("TODO: support events inside non-static and non-scripting API class types.");
                var paramsCount = eventInfo.Type.GenericArgs?.Count ?? 0;

                contents.AppendLine();

                var useCustomDelegateSignature = false;
                for (var i = 0; i < paramsCount; i++)
                {
                    var paramType = eventInfo.Type.GenericArgs[i];
                    var result = GenerateCSharpNativeToManaged(buildData, paramType, classInfo);
                    if ((paramType.IsRef && !paramType.IsConst) || result[result.Length - 1] == '*')
                        useCustomDelegateSignature = true;
                    CppParamsWrappersCache[i] = result;
                }

                string eventType;
                if (useCustomDelegateSignature)
                {
                    contents.Append(indent).Append($"/// <summary>The delegate for event {eventInfo.Name}.</summary>").AppendLine();
                    contents.Append(indent).Append("public delegate void ").Append(eventInfo.Name).Append("Delegate(");
                    for (var i = 0; i < paramsCount; i++)
                    {
                        var paramType = eventInfo.Type.GenericArgs[i];
                        if (i != 0)
                            contents.Append(", ");
                        if (paramType.IsRef && !paramType.IsConst)
                            contents.Append("ref ");
                        contents.Append(CppParamsWrappersCache[i]).Append(" arg").Append(i);
                    }
                    contents.Append(");").AppendLine().AppendLine();
                    eventType = eventInfo.Name + "Delegate";
                }
                else
                {
                    eventType = "Action";
                    if (paramsCount != 0)
                    {
                        eventType += '<';
                        for (var i = 0; i < paramsCount; i++)
                        {
                            if (i != 0)
                                eventType += ", ";
                            CppParamsWrappersCache[i] = GenerateCSharpNativeToManaged(buildData, eventInfo.Type.GenericArgs[i], classInfo);
                            eventType += CppParamsWrappersCache[i];
                        }
                        eventType += '>';
                    }
                }
                string eventSignature = "event " + eventType;

                GenerateCSharpComment(contents, indent, eventInfo.Comment, true);
                GenerateCSharpAttributes(buildData, contents, indent, classInfo, eventInfo, useUnmanaged);
                contents.Append(indent).Append(GenerateCSharpAccessLevel(eventInfo.Access));
                if (eventInfo.IsStatic)
                    contents.Append("static ");
                contents.Append(eventSignature);
                contents.Append(' ').AppendLine(eventInfo.Name);
                contents.Append(indent).Append('{').AppendLine();
                indent += "    ";
                var eventInstance = eventInfo.IsStatic ? string.Empty : "__unmanagedPtr, ";
                contents.Append(indent).Append($"add {{ Internal_{eventInfo.Name} += value; if (Internal_{eventInfo.Name}_Count++ == 0) Internal_{eventInfo.Name}_Bind({eventInstance}true); }}").AppendLine();
                contents.Append(indent).Append($"remove {{ var __delegate = ({eventType})Delegate.Remove(Internal_{eventInfo.Name}, value); if (__delegate != Internal_{eventInfo.Name}) {{ Internal_{eventInfo.Name} = __delegate; if (--Internal_{eventInfo.Name}_Count == 0) Internal_{eventInfo.Name}_Bind({eventInstance}false); }} }}").AppendLine();
                indent = indent.Substring(0, indent.Length - 4);
                contents.Append(indent).Append('}').AppendLine();

                contents.AppendLine();
                if (buildData.Configuration != TargetConfiguration.Release)
                    contents.Append(indent).Append("[System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]").AppendLine();
                contents.Append(indent).Append("private ");
                if (eventInfo.IsStatic)
                    contents.Append("static ");
                contents.Append($"int Internal_{eventInfo.Name}_Count;");

                contents.AppendLine();
                contents.Append("#pragma warning disable 67").AppendLine();
                contents.Append(indent).Append("private ");
                if (eventInfo.IsStatic)
                    contents.Append("static ");
                contents.Append($"{eventSignature} Internal_{eventInfo.Name};");
                contents.AppendLine();
                contents.Append("#pragma warning restore 67").AppendLine();

                contents.AppendLine();
                contents.Append(indent).Append("internal ");
                if (eventInfo.IsStatic)
                    contents.Append("static ");
                contents.Append($"void Internal_{eventInfo.Name}_Invoke(");
                for (var i = 0; i < paramsCount; i++)
                {
                    var paramType = eventInfo.Type.GenericArgs[i];
                    if (i != 0)
                        contents.Append(", ");
                    if (paramType.IsRef && !paramType.IsConst)
                        contents.Append("ref ");
                    contents.Append(CppParamsWrappersCache[i]);
                    contents.Append(" arg").Append(i);
                }
                contents.Append(')').AppendLine();
                contents.Append(indent).Append('{').AppendLine();
                contents.Append(indent).Append("    Internal_").Append(eventInfo.Name);
                contents.Append('(');
                for (var i = 0; i < paramsCount; i++)
                {
                    var paramType = eventInfo.Type.GenericArgs[i];
                    if (i != 0)
                        contents.Append(", ");
                    if (paramType.IsRef && !paramType.IsConst)
                        contents.Append("ref ");
                    contents.Append("arg").Append(i);
                }
                contents.Append(");").AppendLine();
                contents.Append(indent).Append('}').AppendLine();

                contents.AppendLine();
#if !USE_NETCORE
                contents.Append(indent).Append("[MethodImpl(MethodImplOptions.InternalCall)]").AppendLine();
                contents.Append(indent).Append($"internal static extern void Internal_{eventInfo.Name}_Bind(");
                if (!eventInfo.IsStatic)
                    contents.Append("IntPtr obj, ");
                contents.Append("bool bind);");
#else
                string libraryEntryPoint;
                if (buildData.Toolchain?.Compiler == TargetCompiler.MSVC)
                    libraryEntryPoint = $"{classInfo.FullNameManaged}::Internal_{eventInfo.Name}_Bind"; // MSVC allows to override exported symbol name
                else
                    libraryEntryPoint = CppNameMangling.MangleFunctionName(buildData, eventInfo.Name + "_ManagedBind", classInfo.FullNameNativeInternal + "Internal", CSharpEventBindReturn, eventInfo.IsStatic ? null : new TypeInfo(classInfo.FullNameNative) { IsPtr = true }, CSharpEventBindParams);
                contents.Append(indent).Append($"[LibraryImport(\"{classInfo.ParentModule.Module.BinaryModuleName}\", EntryPoint = \"{libraryEntryPoint}\", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(FlaxEngine.Interop.StringMarshaller))]").AppendLine();
                contents.Append(indent).Append($"internal static partial void Internal_{eventInfo.Name}_Bind(");
                if (!eventInfo.IsStatic)
                    contents.Append("IntPtr obj, ");
                contents.Append("[MarshalAs(UnmanagedType.U1)] bool bind);");
#endif
                contents.AppendLine();
            }

            // Fields
            foreach (var fieldInfo in classInfo.Fields)
            {
                if (fieldInfo.Getter == null || fieldInfo.IsHidden)
                    continue;
                contents.AppendLine();
                GenerateCSharpComment(contents, indent, fieldInfo.Comment, true);
                GenerateCSharpAttributes(buildData, contents, indent, classInfo, fieldInfo, useUnmanaged, fieldInfo.DefaultValue, fieldInfo.Type);
                contents.Append(indent).Append(GenerateCSharpAccessLevel(fieldInfo.Access));
                if (fieldInfo.IsConstexpr)
                    contents.Append("const ");
                else if (fieldInfo.IsStatic)
                    contents.Append("static ");

                var managedType = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, classInfo);
                contents.Append(managedType).Append(' ').Append(fieldInfo.Name);

                if (!useUnmanaged || fieldInfo.IsConstexpr)
                {
                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, fieldInfo.DefaultValue, classInfo, fieldInfo.Type, false, managedType);
                    if (!string.IsNullOrEmpty(defaultValue))
                        contents.Append(" = ").Append(defaultValue);
                    contents.AppendLine(";");
                    continue;
                }
                contents.AppendLine().AppendLine(indent + "{");
                indent += "    ";

                contents.Append(indent).Append("get { ");
                GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, fieldInfo.Getter);
                contents.Append(" }").AppendLine();

                if (!fieldInfo.IsReadOnly)
                {
                    contents.Append(indent).Append("set { ");
                    GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, fieldInfo.Setter, useUnmanaged);
                    contents.Append(" }").AppendLine();
                }

                indent = indent.Substring(0, indent.Length - 4);
                contents.AppendLine(indent + "}");

                GenerateCSharpWrapperFunction(buildData, contents, indent, classInfo, fieldInfo.Getter);
                if (!fieldInfo.IsReadOnly)
                    GenerateCSharpWrapperFunction(buildData, contents, indent, classInfo, fieldInfo.Setter);
            }

            // Properties
            foreach (var propertyInfo in classInfo.Properties)
            {
                if (propertyInfo.IsHidden || !useUnmanaged)
                    continue;

                contents.AppendLine();
                GenerateCSharpComment(contents, indent, propertyInfo.Comment, true);
                GenerateCSharpAttributes(buildData, contents, indent, classInfo, propertyInfo, useUnmanaged);
                contents.Append(indent).Append(GenerateCSharpAccessLevel(propertyInfo.Access));
                if (propertyInfo.IsStatic)
                    contents.Append("static ");
                var returnValueType = GenerateCSharpNativeToManaged(buildData, propertyInfo.Type, classInfo);
                contents.Append(returnValueType).Append(' ').AppendLine(propertyInfo.Name);
                contents.AppendLine(indent + "{");
                indent += "    ";

                if (propertyInfo.Getter != null)
                {
                    contents.Append(indent);
                    if (propertyInfo.Access != propertyInfo.Getter.Access)
                        contents.Append(GenerateCSharpAccessLevel(propertyInfo.Getter.Access));
                    contents.Append("get { ");
                    GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, propertyInfo.Getter);
                    contents.Append(" }").AppendLine();
                }

                if (propertyInfo.Setter != null)
                {
                    contents.Append(indent);
                    if (propertyInfo.Access != propertyInfo.Setter.Access)
                        contents.Append(GenerateCSharpAccessLevel(propertyInfo.Setter.Access));
                    contents.Append("set { ");
                    GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, propertyInfo.Setter, true);
                    contents.Append(" }").AppendLine();
                }

                indent = indent.Substring(0, indent.Length - 4);
                contents.AppendLine(indent + "}");

                if (propertyInfo.Getter != null)
                {
                    GenerateCSharpWrapperFunction(buildData, contents, indent, classInfo, propertyInfo.Getter);
                }

                if (propertyInfo.Setter != null)
                {
                    GenerateCSharpWrapperFunction(buildData, contents, indent, classInfo, propertyInfo.Setter);
                }
            }

            // Functions
            foreach (var functionInfo in classInfo.Functions)
            {
                if (functionInfo.IsHidden)
                    continue;
                if (!useUnmanaged)
                    throw new Exception($"Not supported function {functionInfo.Name} inside non-static and non-scripting class type {classInfo.Name}.");

                if (!functionInfo.NoProxy)
                {
                    contents.AppendLine();
                    GenerateCSharpComment(contents, indent, functionInfo.Comment);
                    GenerateCSharpAttributes(buildData, contents, indent, classInfo, functionInfo, true);
                    contents.Append(indent).Append(GenerateCSharpAccessLevel(functionInfo.Access));
                    if (functionInfo.IsStatic)
                        contents.Append("static ");
                    if (functionInfo.IsVirtual && !classInfo.IsSealed)
                        contents.Append("virtual ");
                    var returnValueType = GenerateCSharpNativeToManaged(buildData, functionInfo.ReturnType, classInfo);
                    contents.Append(returnValueType).Append(' ').Append(functionInfo.Name).Append('(');

                    for (var i = 0; i < functionInfo.Parameters.Count; i++)
                    {
                        var parameterInfo = functionInfo.Parameters[i];
                        if (i != 0)
                            contents.Append(", ");
                        if (!string.IsNullOrEmpty(parameterInfo.Attributes))
                            contents.Append('[').Append(parameterInfo.Attributes).Append(']').Append(' ');

                        var managedType = GenerateCSharpNativeToManaged(buildData, parameterInfo.Type, classInfo);
                        if (parameterInfo.IsOut)
                            contents.Append("out ");
                        else if (parameterInfo.IsRef)
                            contents.Append("ref ");
                        else if (parameterInfo.IsThis)
                            contents.Append("this ");
                        else if (parameterInfo.IsParams)
                            contents.Append("params ");
                        contents.Append(managedType);
                        contents.Append(' ');
                        contents.Append(parameterInfo.Name);

                        var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, classInfo, parameterInfo.Type, false, managedType);
                        if (!string.IsNullOrEmpty(defaultValue))
                            contents.Append(" = ").Append(defaultValue);
                    }

                    contents.Append(')').AppendLine().AppendLine(indent + "{");
                    indent += "    ";

                    contents.Append(indent);
                    GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, functionInfo);

                    indent = indent.Substring(0, indent.Length - 4);
                    contents.AppendLine();
                    contents.AppendLine(indent + "}");
                }

                GenerateCSharpWrapperFunction(buildData, contents, indent, classInfo, functionInfo);
            }

            // Interface implementation
            if (hasInterface)
            {
                foreach (var interfaceInfo in classInfo.Interfaces)
                {
                    if (interfaceInfo.Access != AccessLevel.Public)
                        continue;
                    foreach (var functionInfo in interfaceInfo.Functions)
                    {
                        if (!classInfo.IsScriptingObject)
                            throw new Exception($"Class {classInfo.Name} cannot implement interface {interfaceInfo.Name} because it requires ScriptingObject as a base class.");

                        contents.AppendLine();
                        if (functionInfo.Comment != null && functionInfo.Comment.Length != 0)
                            contents.Append(indent).AppendLine("/// <inheritdoc />");
                        GenerateCSharpAttributes(buildData, contents, indent, classInfo, functionInfo.Attributes, null, false, useUnmanaged);
                        contents.Append(indent).Append(GenerateCSharpAccessLevel(functionInfo.Access));
                        if (functionInfo.IsVirtual && !classInfo.IsSealed)
                            contents.Append("virtual ");
                        var returnValueType = GenerateCSharpNativeToManaged(buildData, functionInfo.ReturnType, classInfo);
                        contents.Append(returnValueType).Append(' ').Append(functionInfo.Name).Append('(');

                        for (var i = 0; i < functionInfo.Parameters.Count; i++)
                        {
                            var parameterInfo = functionInfo.Parameters[i];
                            if (i != 0)
                                contents.Append(", ");
                            if (!string.IsNullOrEmpty(parameterInfo.Attributes))
                                contents.Append('[').Append(parameterInfo.Attributes).Append(']').Append(' ');

                            var managedType = GenerateCSharpNativeToManaged(buildData, parameterInfo.Type, classInfo);
                            if (parameterInfo.IsOut)
                                contents.Append("out ");
                            else if (parameterInfo.IsRef)
                                contents.Append("ref ");
                            else if (parameterInfo.IsThis)
                                contents.Append("this ");
                            else if (parameterInfo.IsParams)
                                contents.Append("params ");
                            contents.Append(managedType);
                            contents.Append(' ');
                            contents.Append(parameterInfo.Name);

                            var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, classInfo, parameterInfo.Type, false, managedType);
                            if (!string.IsNullOrEmpty(defaultValue))
                                contents.Append(" = ").Append(defaultValue);
                        }

                        contents.Append(')').AppendLine().AppendLine(indent + "{");
                        indent += "    ";

                        contents.Append(indent);
                        GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, functionInfo);

                        indent = indent.Substring(0, indent.Length - 4);
                        contents.AppendLine();
                        contents.AppendLine(indent + "}");

                        GenerateCSharpWrapperFunction(buildData, contents, indent, classInfo, functionInfo);
                    }
                }
            }

            GenerateCSharpManagedTypeInternals(buildData, classInfo, contents, indent);

            // Abstract wrapper (to ensure C# class can be created for Visual Scripts, see NativeInterop.NewObject)
            if (classInfo.IsAbstract)
                contents.AppendLine().Append(indent).Append("[Unmanaged] [HideInEditor] private sealed class AbstractWrapper : ").Append(classInfo.Name).AppendLine(" { }");

            // Nested types
            foreach (var apiTypeInfo in classInfo.Children)
            {
                GenerateCSharpType(buildData, contents, indent, apiTypeInfo);
            }

            // Class end
            indent = indent.Substring(0, indent.Length - 4);
            contents.AppendLine(indent + "}");

#if USE_NETCORE
            if (!string.IsNullOrEmpty(marshallerName))
            {
                if (!string.IsNullOrEmpty(interopNamespace))
                {
                    contents.AppendLine("}");
                    contents.AppendLine($"namespace {interopNamespace}");
                    contents.AppendLine("{");
                }

                contents.AppendLine();
                contents.AppendLine(string.Join("\n" + indent, (indent + $$"""
                /// <summary>
                /// Marshaller for type <see cref="{{classInfo.Name}}"/>.
                /// </summary>
                #if FLAX_EDITOR
                [HideInEditor]
                #endif
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.ManagedToUnmanagedIn, typeof({{marshallerFullName}}.ManagedToNative))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.UnmanagedToManagedOut, typeof({{marshallerFullName}}.ManagedToNative))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.ElementIn, typeof({{marshallerFullName}}.ManagedToNative))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.ManagedToUnmanagedOut, typeof({{marshallerFullName}}.NativeToManaged))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.UnmanagedToManagedIn, typeof({{marshallerFullName}}.NativeToManaged))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.ElementOut, typeof({{marshallerFullName}}.NativeToManaged))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.ManagedToUnmanagedRef, typeof({{marshallerFullName}}.Bidirectional))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.UnmanagedToManagedRef, typeof({{marshallerFullName}}.Bidirectional))]
                [CustomMarshaller(typeof({{classInfo.Name}}), MarshalMode.ElementRef, typeof({{marshallerFullName}}))]
                {{GenerateCSharpAccessLevel(classInfo.Access)}}static class {{marshallerName}}
                {
                #pragma warning disable 1591
                #pragma warning disable 618
                #if FLAX_EDITOR
                    [HideInEditor]
                #endif
                    public static class NativeToManaged
                    {
                        public static {{classInfo.Name}} ConvertToManaged(IntPtr unmanaged) => Unsafe.As<{{classInfo.Name}}>(ManagedHandleMarshaller.NativeToManaged.ConvertToManaged(unmanaged));
                        public static IntPtr ConvertToUnmanaged({{classInfo.Name}} managed) => managed != null ? ManagedHandle.ToIntPtr(managed, GCHandleType.Weak) : IntPtr.Zero;
                        public static void Free(IntPtr unmanaged) {}
                    }
                #if FLAX_EDITOR
                    [HideInEditor]
                #endif
                    public static class ManagedToNative
                    {
                        public static {{classInfo.Name}} ConvertToManaged(IntPtr unmanaged) => Unsafe.As<{{classInfo.Name}}>(ManagedHandleMarshaller.NativeToManaged.ConvertToManaged(unmanaged));
                        public static IntPtr ConvertToUnmanaged({{classInfo.Name}} managed) => managed != null ? ManagedHandle.ToIntPtr(managed, GCHandleType.Weak) : IntPtr.Zero;
                        public static void Free(IntPtr unmanaged) {}
                    }
                #if FLAX_EDITOR
                    [HideInEditor]
                #endif
                    public struct Bidirectional
                    {
                        ManagedHandleMarshaller.Bidirectional marsh;
                        public void FromManaged({{classInfo.Name}} managed) => marsh.FromManaged(managed);
                        public IntPtr ToUnmanaged() => marsh.ToUnmanaged();
                        public void FromUnmanaged(IntPtr unmanaged) => marsh.FromUnmanaged(unmanaged);
                        public {{classInfo.Name}} ToManaged() => Unsafe.As<{{classInfo.Name}}>(marsh.ToManaged());
                        public void Free() => marsh.Free();
                    }
                    internal static {{classInfo.Name}} ConvertToManaged(IntPtr unmanaged) => Unsafe.As<{{classInfo.Name}}>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));
                    internal static IntPtr ConvertToUnmanaged({{classInfo.Name}} managed) => ManagedHandleMarshaller.ConvertToUnmanaged(managed);
                    internal static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.Free(unmanaged);

                    internal static {{classInfo.Name}} ToManaged(IntPtr managed) => Unsafe.As<{{classInfo.Name}}>(ManagedHandleMarshaller.ToManaged(managed));
                    internal static IntPtr ToNative({{classInfo.Name}} managed) => ManagedHandleMarshaller.ToNative(managed);
                #pragma warning restore 618
                #pragma warning restore 1591
                }
                """).Split(new char[] { '\n' })));
            }
#endif
            // Namespace end
            if (!string.IsNullOrEmpty(classInfo.Namespace))
                contents.AppendLine("}");
        }

        private static void GenerateCSharpStructure(BuildData buildData, StringBuilder contents, string indent, StructureInfo structureInfo)
        {
            contents.AppendLine();

#if USE_NETCORE
            // Generate blittable structure
            string structNativeMarshaling = "";
            if (UseCustomMarshalling(buildData, structureInfo, structureInfo))
            {
                // Namespace begin
                string interopNamespace = "";
                if (!string.IsNullOrEmpty(structureInfo.Namespace))
                {
                    interopNamespace = $"{structureInfo.Namespace}.Interop";
                    contents.AppendLine($"namespace {interopNamespace}");
                    contents.AppendLine("{");
                    indent += "    ";
                }

                // NOTE: Permanent FlaxEngine.Object GCHandles must not be released when marshalling from native to managed.

                string marshallerName = structureInfo.Name + "Marshaller";
#if MARSHALLER_FULL_NAME
                string marshallerFullName = !string.IsNullOrEmpty(interopNamespace) ? $"{interopNamespace}.{marshallerName}" : marshallerName;
#else
                if (!string.IsNullOrEmpty(interopNamespace))
                    CSharpUsedNamespaces.Add(interopNamespace);
                string marshallerFullName = marshallerName;
#endif
                structNativeMarshaling = $"[NativeMarshalling(typeof({marshallerFullName}))]";

                var toManagedContent = GetStringBuilder();
                var toNativeContent = GetStringBuilder();
                var freeContents = GetStringBuilder();
                var freeContents2 = GetStringBuilder();
                var structContents = GetStringBuilder();

                {
                    // Native struct begin
                    // TODO: skip using this utility structure if the auto-generated C# struct is already the same as XXXInternal here below
                    var structIndent = "";
                    if (buildData.Target != null & buildData.Target.IsEditor)
                        structContents.Append(structIndent).AppendLine("[HideInEditor]");
                    structContents.Append(structIndent).AppendLine("[StructLayout(LayoutKind.Sequential)]");
                    structContents.Append(structIndent).Append("public struct ").Append(structureInfo.Name).Append("Internal");
                    if (structureInfo.BaseType != null && structureInfo.IsPod)
                        structContents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, new TypeInfo { Type = structureInfo.BaseType.Name }, structureInfo));
                    structContents.AppendLine();
                    structContents.Append(structIndent + "{");
                    structIndent += "    ";

                    toNativeContent.Append($"var unmanaged = new {structureInfo.Name}Internal();").AppendLine();
                    toManagedContent.Append($"var managed = new {structureInfo.Name}();").AppendLine();

                    structContents.AppendLine();
                    foreach (var fieldInfo in structureInfo.Fields)
                    {
                        if (fieldInfo.IsStatic || fieldInfo.IsConstexpr)
                            continue;

                        var marshalType = fieldInfo.Type;
                        var apiType = FindApiTypeInfo(buildData, marshalType, structureInfo);
                        if (apiType != null && apiType.MarshalAs != null)
                            marshalType = apiType.MarshalAs;
                        bool internalType = apiType is StructureInfo fieldStructureInfo && UseCustomMarshalling(buildData, fieldStructureInfo, structureInfo);
                        string type, originalType;
                        if (marshalType.IsArray && (fieldInfo.NoArray || structureInfo.IsPod))
                        {
                            // Fixed-size array that needs to be inlined into structure instead of passing it as managed array
                            marshalType.IsArray = false;
                            originalType = type = GenerateCSharpNativeToManaged(buildData, marshalType, structureInfo);
                            marshalType.IsArray = true;
                        }
                        else
                            originalType = type = GenerateCSharpNativeToManaged(buildData, marshalType, structureInfo);
                        if (apiType != null && apiType.MarshalAs != null)
                            Log.Error("marshal as into type: " + type);
                        structContents.Append(structIndent).Append("public ");
                        string internalTypeMarshaller = "";
                        if (marshalType.IsArray && (fieldInfo.NoArray || structureInfo.IsPod))
                        {
#if USE_NETCORE
                            if (GenerateCSharpUseFixedBuffer(originalType))
                            {
                                // Use fixed statement with primitive types of buffers
                                structContents.Append($"fixed {originalType} {fieldInfo.Name}0[{marshalType.ArraySize}];").AppendLine();

                                // Copy fixed-size array
                                toManagedContent.AppendLine($"FlaxEngine.Utils.MemoryCopy(new IntPtr(managed.{fieldInfo.Name}0), new IntPtr(unmanaged.{fieldInfo.Name}0), sizeof({originalType}) * {marshalType.ArraySize}ul);");
                                toNativeContent.AppendLine($"FlaxEngine.Utils.MemoryCopy(new IntPtr(unmanaged.{fieldInfo.Name}0), new IntPtr(managed.{fieldInfo.Name}0), sizeof({originalType}) * {marshalType.ArraySize}ul);");
                            }
                            else
#endif
                            {
                                // Padding in structs for fixed-size array
                                structContents.Append(type).Append(' ').Append(fieldInfo.Name).Append("0;").AppendLine();
                                for (int i = 1; i < marshalType.ArraySize; i++)
                                {
                                    GenerateCSharpAttributes(buildData, structContents, structIndent, structureInfo, fieldInfo, fieldInfo.IsStatic);
                                    structContents.Append(structIndent).Append("public ");
                                    structContents.Append(type).Append(' ').Append(fieldInfo.Name + i).Append(';').AppendLine();
                                }

                                // Copy fixed-size array item one-by-one
                                if (fieldInfo.Access == AccessLevel.Public || fieldInfo.Access == AccessLevel.Internal)
                                {
                                    for (int i = 0; i < marshalType.ArraySize; i++)
                                    {
                                        toManagedContent.AppendLine($"managed.{fieldInfo.Name}{i} = unmanaged.{fieldInfo.Name}{i};");
                                        toNativeContent.AppendLine($"unmanaged.{fieldInfo.Name}{i} = managed.{fieldInfo.Name}{i};");
                                    }
                                }
                                else
                                {
                                    throw new NotImplementedException("TODO: generate utility method to copy private/protected array data items");
                                }
                            }
                            continue;
                        }
                        else
                        {
                            if (marshalType.IsObjectRef || marshalType.Type == "Dictionary")
                                type = "IntPtr";
                            else if (marshalType.IsPtr && !originalType.EndsWith("*"))
                                type = "IntPtr";
                            else if (marshalType.Type == "Array" || marshalType.Type == "Span" || marshalType.Type == "DataContainer" || marshalType.Type == "BytesContainer")
                            {
                                type = "IntPtr";
                                apiType = FindApiTypeInfo(buildData, marshalType.GenericArgs[0], structureInfo);
                                internalType = apiType is StructureInfo elementStructureInfo && UseCustomMarshalling(buildData, elementStructureInfo, structureInfo);
                            }
                            else if (marshalType.Type == "Version")
                                type = "IntPtr";
                            else if (type == "string")
                                type = "IntPtr";
                            else if (type == "bool")
                                type = "byte";
                            else if (marshalType.Type == "Variant")
                                type = "IntPtr";
                            else if (internalType)
                            {
                                internalTypeMarshaller = type + "Marshaller";
                                type = $"{internalTypeMarshaller}.{type}Internal";
                            }
                            //else if (type == "Guid")
                            //    type = "GuidNative";
                            structContents.Append($"{type} {fieldInfo.Name};").Append(type == "IntPtr" ? $" // {originalType}" : "").AppendLine();
                        }

                        // Generate struct constructor/getter and deconstructor/setter function
                        toManagedContent.Append("managed.").Append(fieldInfo.Name).Append(" = ");
                        toNativeContent.Append("unmanaged.").Append(fieldInfo.Name).Append(" = ");
                        if (marshalType.IsObjectRef)
                        {
                            var managedType = GenerateCSharpNativeToManaged(buildData, marshalType.GenericArgs[0], structureInfo);
                            toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? Unsafe.As<{managedType}>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target) : null;");
                            toNativeContent.AppendLine($"managed.{fieldInfo.Name} != null ? ManagedHandle.ToIntPtr(managed.{fieldInfo.Name}, GCHandleType.Weak) : IntPtr.Zero;");
                            freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");

                            // Permanent ScriptingObject handle is passed from native side, do not release it
                            //freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");
                        }
                        else if (marshalType.Type == "ScriptingObject")
                        {
                            toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? Unsafe.As<FlaxEngine.Object>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target) : null;");
                            toNativeContent.AppendLine($"managed.{fieldInfo.Name} != null ? ManagedHandle.ToIntPtr(managed.{fieldInfo.Name}, GCHandleType.Weak) : IntPtr.Zero;");
                            freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");

                            // Permanent ScriptingObject handle is passed from native side, do not release it
                            //freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");
                        }
                        else if (marshalType.IsPtr && originalType != "IntPtr" && !originalType.EndsWith("*"))
                        {
                            toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? Unsafe.As<{originalType}>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target) : null;");
                            toNativeContent.AppendLine($"managed.{fieldInfo.Name} != null ? ManagedHandle.ToIntPtr(managed.{fieldInfo.Name}, GCHandleType.Weak) : IntPtr.Zero;");
                            freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");

                            // Permanent ScriptingObject handle is passed from native side, do not release it
                            //freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");
                        }
                        else if (marshalType.Type == "Dictionary")
                        {
                            toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? Unsafe.As<{originalType}>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target) : null;");
                            toNativeContent.AppendLine($"ManagedHandle.ToIntPtr(managed.{fieldInfo.Name}, GCHandleType.Weak);");
                            freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");
                            freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");
                        }
                        else if (marshalType.Type == "Array" || marshalType.Type == "Span" || marshalType.Type == "DataContainer" || marshalType.Type == "BytesContainer")
                        {
                            string originalElementType = originalType.Substring(0, originalType.Length - 2);
                            if (internalType)
                            {
                                // Marshal blittable array elements back to original non-blittable elements
                                string originalElementTypeMarshaller = (!string.IsNullOrEmpty(apiType?.Namespace) ? $"{apiType?.Namespace}.Interop." : "") + $"{originalElementType}Marshaller";
                                string originalElementTypeName = originalElementType.Substring(originalElementType.LastIndexOf('.') + 1); // Strip namespace
                                string internalElementType = $"{originalElementTypeMarshaller}.{originalElementTypeName}Internal";
                                toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? NativeInterop.ConvertArray((Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target)).ToSpan<{internalElementType}>(), {originalElementTypeMarshaller}.ToManaged) : null;");
                                toNativeContent.AppendLine($"managed.{fieldInfo.Name}?.Length > 0 ? ManagedHandle.ToIntPtr(ManagedArray.WrapNewArray(NativeInterop.ConvertArray(managed.{fieldInfo.Name}, {originalElementTypeMarshaller}.ToNative)), GCHandleType.Weak) : IntPtr.Zero;");
                                freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle handle = ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}); Span<{internalElementType}> values = (Unsafe.As<ManagedArray>(handle.Target)).ToSpan<{internalElementType}>(); foreach (var value in values) {{ {originalElementTypeMarshaller}.Free(value); }} (Unsafe.As<ManagedArray>(handle.Target)).Free(); handle.Free(); }}");
                                freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle handle = ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}); Span<{internalElementType}> values = (Unsafe.As<ManagedArray>(handle.Target)).ToSpan<{internalElementType}>(); foreach (var value in values) {{ {originalElementTypeMarshaller}.NativeToManaged.Free(value); }} (Unsafe.As<ManagedArray>(handle.Target)).Free(); handle.Free(); }}");
                            }
                            else if (marshalType.GenericArgs[0].IsObjectRef)
                            {
                                // Array elements passed as GCHandles
                                toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? NativeInterop.GCHandleArrayToManagedArray<{originalElementType}>(Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target)) : null;");
                                toNativeContent.AppendLine($"managed.{fieldInfo.Name}?.Length > 0 ? ManagedHandle.ToIntPtr(NativeInterop.ManagedArrayToGCHandleWrappedArray(managed.{fieldInfo.Name}), GCHandleType.Weak) : IntPtr.Zero;");
                                freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle handle = ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}); Span<IntPtr> ptrs = (Unsafe.As<ManagedArray>(handle.Target)).ToSpan<IntPtr>(); foreach (var ptr in ptrs) {{ if (ptr != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(ptr).Free(); }} }} (Unsafe.As<ManagedArray>(handle.Target)).Free(); handle.Free(); }}");

                                // Permanent ScriptingObject handle is passed from native side, do not release it
                                //freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle handle = ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}); Span<IntPtr> ptrs = (Unsafe.As<ManagedArray>(handle.Target)).ToSpan<IntPtr>(); foreach (var ptr in ptrs) {{ if (ptr != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(ptr).Free(); }} }} (Unsafe.As<ManagedArray>(handle.Target)).Free(); handle.Free(); }}");
                            }
                            else
                            {
                                // Blittable array elements
                                toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? (Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target)).ToArray<{originalElementType}>() : null;");
                                toNativeContent.AppendLine($"managed.{fieldInfo.Name}?.Length > 0 ? ManagedHandle.ToIntPtr(ManagedArray.WrapNewArray(managed.{fieldInfo.Name}), GCHandleType.Weak) : IntPtr.Zero;");
                                freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle handle = ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}); (Unsafe.As<ManagedArray>(handle.Target)).Free(); handle.Free(); }}");
                                freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle handle = ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}); (Unsafe.As<ManagedArray>(handle.Target)).Free(); handle.Free(); }}");
                            }
                        }
                        else if (marshalType.Type == "Version")
                        {
                            toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != IntPtr.Zero ? Unsafe.As<{originalType}>(ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Target) : null;");
                            toNativeContent.AppendLine($"ManagedHandle.ToIntPtr(managed.{fieldInfo.Name}, GCHandleType.Weak);");
                            freeContents.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");
                            freeContents2.AppendLine($"if (unmanaged.{fieldInfo.Name} != IntPtr.Zero) {{ ManagedHandle.FromIntPtr(unmanaged.{fieldInfo.Name}).Free(); }}");
                        }
                        else if (originalType == "string")
                        {
                            toManagedContent.AppendLine($"ManagedString.ToManaged(unmanaged.{fieldInfo.Name});");
                            toNativeContent.AppendLine($"ManagedString.ToNative(managed.{fieldInfo.Name});");
                            freeContents.AppendLine($"ManagedString.Free(unmanaged.{fieldInfo.Name});");
                            freeContents2.AppendLine($"ManagedString.Free(unmanaged.{fieldInfo.Name});");
                        }
                        else if (originalType == "bool")
                        {
                            toManagedContent.AppendLine($"unmanaged.{fieldInfo.Name} != 0;");
                            toNativeContent.AppendLine($"managed.{fieldInfo.Name} ? (byte)1 : (byte)0;");
                        }
                        else if (marshalType.Type == "Variant")
                        {
                            // Variant passed as boxed object handle
                            toManagedContent.AppendLine($"ManagedHandleMarshaller.NativeToManaged.ConvertToManaged(unmanaged.{fieldInfo.Name});");
                            toNativeContent.AppendLine($"ManagedHandleMarshaller.NativeToManaged.ConvertToUnmanaged(managed.{fieldInfo.Name});");
                        }
                        else if (internalType)
                        {
                            toManagedContent.AppendLine($"{internalTypeMarshaller}.ToManaged(unmanaged.{fieldInfo.Name});");
                            toNativeContent.AppendLine($"{internalTypeMarshaller}.ToNative(managed.{fieldInfo.Name});");
                            freeContents.AppendLine($"{internalTypeMarshaller}.Free(unmanaged.{fieldInfo.Name});");
                            freeContents2.AppendLine($"{internalTypeMarshaller}.NativeToManaged.Free(unmanaged.{fieldInfo.Name});");
                        }
                        /*else if (originalType == "Guid")
                        {
                            toManagedContent.Append("(Guid)unmanaged.").Append(fieldInfo.Name);
                            toNativeContent.Append("(GuidNative)managed.").Append(fieldInfo.Name);
                        }*/
                        else
                        {
                            toManagedContent.Append("unmanaged.").Append(fieldInfo.Name).AppendLine(";");
                            toNativeContent.Append("managed.").Append(fieldInfo.Name).AppendLine(";");
                        }
                    }

                    // Native struct end
                    structIndent = structIndent.Substring(0, structIndent.Length - 4);
                    structContents.AppendLine(structIndent + "}").AppendLine();

                    toNativeContent.Append("return unmanaged;");
                    toManagedContent.Append("return managed;");
                }

                contents.AppendLine(string.Join("\n" + indent, (indent + $$"""
                /// <summary>
                /// Marshaller for type <see cref="{{structureInfo.Name}}"/>.
                /// </summary>
                {{InsertHideInEditorSection()}}
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.ManagedToUnmanagedIn, typeof({{marshallerFullName}}.ManagedToNative))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.UnmanagedToManagedOut, typeof({{marshallerFullName}}.ManagedToNative))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.ElementIn, typeof({{marshallerFullName}}.ManagedToNative))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.ManagedToUnmanagedOut, typeof({{marshallerFullName}}.NativeToManaged))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.UnmanagedToManagedIn, typeof({{marshallerFullName}}.NativeToManaged))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.ElementOut, typeof({{marshallerFullName}}.NativeToManaged))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.ManagedToUnmanagedRef, typeof({{marshallerFullName}}.Bidirectional))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.UnmanagedToManagedRef, typeof({{marshallerFullName}}.Bidirectional))]
                [CustomMarshaller(typeof({{structureInfo.Name}}), MarshalMode.ElementRef, typeof({{marshallerFullName}}))]
                {{GenerateCSharpAccessLevel(structureInfo.Access)}}static unsafe class {{marshallerName}}
                {
                #pragma warning disable 1591
                #pragma warning disable 618
                    {{structContents.Replace("\n", "\n" + "    ").ToString().TrimEnd()}}

                    {{InsertHideInEditorSection()}}
                    public static class NativeToManaged
                    {
                        public static {{structureInfo.Name}} ConvertToManaged({{structureInfo.Name}}Internal unmanaged) => {{marshallerFullName}}.ToManaged(unmanaged);
                        public static {{structureInfo.Name}}Internal ConvertToUnmanaged({{structureInfo.Name}} managed) => {{marshallerFullName}}.ToNative(managed);
                        public static void Free({{structureInfo.Name}}Internal unmanaged)
                        {
                            {{freeContents2.Replace("\n", "\n" + "            ").ToString().TrimEnd()}}
                        }
                    }
                    {{InsertHideInEditorSection()}}
                    public static class ManagedToNative
                    {
                        public static {{structureInfo.Name}} ConvertToManaged({{structureInfo.Name}}Internal unmanaged) => {{marshallerFullName}}.ToManaged(unmanaged);
                        public static {{structureInfo.Name}}Internal ConvertToUnmanaged({{structureInfo.Name}} managed) => {{marshallerFullName}}.ToNative(managed);
                        public static void Free({{structureInfo.Name}}Internal unmanaged) => {{marshallerFullName}}.Free(unmanaged);
                    }
                    {{InsertHideInEditorSection()}}
                    public struct Bidirectional
                    {
                        {{structureInfo.Name}} managed;
                        {{structureInfo.Name}}Internal unmanaged;
                        public void FromManaged({{structureInfo.Name}} managed) => this.managed = managed;
                        public {{structureInfo.Name}}Internal ToUnmanaged() { unmanaged = {{marshallerFullName}}.ToNative(managed); return unmanaged; }
                        public void FromUnmanaged({{structureInfo.Name}}Internal unmanaged) => this.unmanaged = unmanaged;
                        public {{structureInfo.Name}} ToManaged() { managed = {{marshallerFullName}}.ToManaged(unmanaged); return managed; }
                        public void Free() => NativeToManaged.Free(unmanaged);
                    }
                    internal static {{structureInfo.Name}} ConvertToManaged({{structureInfo.Name}}Internal unmanaged) => ToManaged(unmanaged);
                    internal static {{structureInfo.Name}}Internal ConvertToUnmanaged({{structureInfo.Name}} managed) => ToNative(managed);
                    internal static void Free({{structureInfo.Name}}Internal unmanaged)
                    {
                        {{freeContents.Replace("\n", "\n" + "        ").ToString().TrimEnd()}}
                    }

                    internal static {{structureInfo.Name}} ToManaged({{structureInfo.Name}}Internal unmanaged)
                    {
                        {{toManagedContent.Replace("\n", "\n" + "        ").ToString().TrimEnd()}}
                    }
                    internal static {{structureInfo.Name}}Internal ToNative({{structureInfo.Name}} managed)
                    {
                        {{toNativeContent.Replace("\n", "\n" + "        ").ToString().TrimEnd()}}
                    }
                #pragma warning restore 618
                #pragma warning restore 1591
                }
                """).Split(new char[] { '\n' })));

                string InsertHideInEditorSection()
                {
                    return (buildData.Target != null & buildData.Target.IsEditor) ? $$"""
                        [HideInEditor]
                        """ : "";
                }

                PutStringBuilder(toManagedContent);
                PutStringBuilder(toNativeContent);
                PutStringBuilder(freeContents);
                PutStringBuilder(freeContents2);
                PutStringBuilder(structContents);

                // Namespace end
                if (!string.IsNullOrEmpty(structureInfo.Namespace))
                {
                    contents.AppendLine("}");
                    indent = indent.Substring(0, indent.Length - 4);
                }
            }
#endif
            // Namespace begin
            if (!string.IsNullOrEmpty(structureInfo.Namespace))
            {
                contents.AppendLine($"namespace {structureInfo.Namespace}");
                contents.AppendLine("{");
                indent += "    ";
            }

            // Struct docs
            GenerateCSharpComment(contents, indent, structureInfo.Comment);

            // Struct begin
            GenerateCSharpAttributes(buildData, contents, indent, structureInfo, true);
            contents.Append(indent).AppendLine("[StructLayout(LayoutKind.Sequential)]");
#if USE_NETCORE
            if (!string.IsNullOrEmpty(structNativeMarshaling))
                contents.Append(indent).AppendLine(structNativeMarshaling);
#endif
            contents.Append(indent).Append(GenerateCSharpAccessLevel(structureInfo.Access));
            contents.Append("unsafe partial struct ").Append(structureInfo.Name);
            if (structureInfo.BaseType != null && structureInfo.IsPod)
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, new TypeInfo(structureInfo.BaseType), structureInfo));
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Fields
            bool hasDefaultMember = false;
            foreach (var fieldInfo in structureInfo.Fields)
            {
                contents.AppendLine();
                GenerateCSharpComment(contents, indent, fieldInfo.Comment);
                GenerateCSharpAttributes(buildData, contents, indent, structureInfo, fieldInfo, fieldInfo.IsStatic, fieldInfo.DefaultValue, fieldInfo.Type);
#if USE_NETCORE
                if (fieldInfo.Type.Type == "String")
                    contents.Append(indent).AppendLine("[MarshalAs(UnmanagedType.LPWStr)]");
                else if (fieldInfo.Type.Type == "bool")
                    contents.Append(indent).AppendLine("[MarshalAs(UnmanagedType.U1)]");
#endif
                contents.Append(indent).Append(GenerateCSharpAccessLevel(fieldInfo.Access));
                if (fieldInfo.IsConstexpr)
                    contents.Append("const ");
                else if (fieldInfo.IsStatic)
                    contents.Append("static ");
                hasDefaultMember |= string.Equals(fieldInfo.Name, "Default", StringComparison.Ordinal);
                string managedType;

                if (fieldInfo.Type.IsArray && (fieldInfo.NoArray || structureInfo.IsPod))
                {
                    fieldInfo.Type.IsArray = false;
                    managedType = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, structureInfo);
                    fieldInfo.Type.IsArray = true;
#if USE_NETCORE
                    if (GenerateCSharpUseFixedBuffer(managedType))
                    {
                        // Use fixed statement with primitive types of buffers
                        if (managedType == "char")
                            managedType = "short"; // char's are not blittable, store as short instead
                        contents.Append($"fixed {managedType} {fieldInfo.Name}0[{fieldInfo.Type.ArraySize}];").AppendLine();
                    }
                    else
#endif
                    {
                        // Padding in structs for fixed-size array
                        contents.Append(managedType).Append(' ').Append(fieldInfo.Name + "0;").AppendLine();
                        for (int i = 1; i < fieldInfo.Type.ArraySize; i++)
                        {
                            contents.AppendLine();
                            GenerateCSharpAttributes(buildData, contents, indent, structureInfo, fieldInfo, fieldInfo.IsStatic);
                            contents.Append(indent).Append(GenerateCSharpAccessLevel(fieldInfo.Access));
                            if (fieldInfo.IsStatic)
                                contents.Append("static ");
                            contents.Append(managedType).Append(' ').Append(fieldInfo.Name + i).Append(';').AppendLine();
                        }
                    }
                    continue;
                }

                managedType = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, structureInfo);
                contents.Append(managedType).Append(' ').Append(fieldInfo.Name);

                if (fieldInfo.IsConstexpr)
                {
                    // Compile-time constant
                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, fieldInfo.DefaultValue, structureInfo, fieldInfo.Type, false, managedType);
                    if (!string.IsNullOrEmpty(defaultValue))
                        contents.Append(" = ").Append(defaultValue);
                    contents.AppendLine(";");
                }
                else if (fieldInfo.IsStatic)
                {
                    // Static fields are using C++ static value accessed via getter function binding
                    contents.AppendLine();
                    contents.AppendLine(indent + "{");
                    indent += "    ";

                    contents.Append(indent).Append("get { ");
                    GenerateCSharpWrapperFunctionCall(buildData, contents, structureInfo, fieldInfo.Getter);
                    contents.Append(" }").AppendLine();

                    if (!fieldInfo.IsReadOnly)
                    {
                        contents.Append(indent).Append("set { ");
                        GenerateCSharpWrapperFunctionCall(buildData, contents, structureInfo, fieldInfo.Setter, true);
                        contents.Append(" }").AppendLine();
                    }

                    indent = indent.Substring(0, indent.Length - 4);
                    contents.AppendLine(indent + "}");

                    GenerateCSharpWrapperFunction(buildData, contents, indent, structureInfo, fieldInfo.Getter);
                    if (!fieldInfo.IsReadOnly)
                        GenerateCSharpWrapperFunction(buildData, contents, indent, structureInfo, fieldInfo.Setter);
                }
                else
                {
                    contents.Append(';').AppendLine();
                }
            }

            // Default value
            if (!structureInfo.NoDefault && !hasDefaultMember)
            {
                var useDefaultValueInit = GenerateCSharpStructureUseDefaultInitialize(buildData, structureInfo);
                if (useDefaultValueInit)
                {
                    contents.AppendLine();
                    contents.Append(indent).AppendLine("private static bool __defaultValid;");
                    contents.Append(indent).AppendLine($"private static {structureInfo.Name} __default;");
                }
                contents.AppendLine();
                contents.Append(indent).AppendLine("/// <summary>");
                contents.Append(indent).AppendLine($"/// The default <see cref=\"{structureInfo.Name}\"/>.");
                contents.Append(indent).AppendLine("/// </summary>");
                contents.Append(indent).AppendLine($"public static {structureInfo.Name} Default");
                contents.Append(indent).AppendLine("{");
                indent += "    ";
                contents.Append(indent).AppendLine("get");
                contents.Append(indent).AppendLine("{");
                if (useDefaultValueInit)
                {
                    indent += "    ";
                    contents.Append(indent).AppendLine("if (!__defaultValid)");
                    contents.Append(indent).AppendLine("{");
                    indent += "    ";
                    contents.Append(indent).AppendLine("__defaultValid = true;");
                    contents.Append(indent).AppendLine("object obj = __default;");
                    contents.Append(indent).AppendLine($"FlaxEngine.Utils.InitStructure(obj, typeof({structureInfo.Name}));");
                    contents.Append(indent).AppendLine($"__default = ({structureInfo.Name})obj;");
                    indent = indent.Substring(0, indent.Length - 4);
                    contents.Append(indent).AppendLine("}");
                    contents.Append(indent).AppendLine("return __default;");
                    indent = indent.Substring(0, indent.Length - 4);
                }
                else
                {
                    contents.Append(indent).AppendLine($"    return new {structureInfo.Name}();");
                }
                contents.Append(indent).AppendLine("}");
                indent = indent.Substring(0, indent.Length - 4);
                contents.Append(indent).AppendLine("}");
                if (useDefaultValueInit)
                {
                    contents.AppendLine();
                    contents.Append(indent).AppendLine("[System.Runtime.Serialization.OnDeserializing]");
                    contents.Append(indent).AppendLine("internal void OnDeserializing(System.Runtime.Serialization.StreamingContext context)");
                    contents.Append(indent).AppendLine("{");
                    contents.Append(indent).AppendLine("    // Initialize structure with default values to replicate C++ deserialization behavior");
                    contents.Append(indent).AppendLine("    this = Default;");
                    contents.Append(indent).AppendLine("}");
                }
            }

            GenerateCSharpManagedTypeInternals(buildData, structureInfo, contents, indent);

            // Nested types
            foreach (var apiTypeInfo in structureInfo.Children)
            {
                GenerateCSharpType(buildData, contents, indent, apiTypeInfo);
            }

            // Struct end
            indent = indent.Substring(0, indent.Length - 4);
            contents.AppendLine(indent + "}");

            // Namespace end
            if (!string.IsNullOrEmpty(structureInfo.Namespace))
            {
                contents.AppendLine("}");
            }
        }

        private static void GenerateCSharpEnum(BuildData buildData, StringBuilder contents, string indent, EnumInfo enumInfo)
        {
            contents.AppendLine();

            // Namespace begin
            if (!string.IsNullOrEmpty(enumInfo.Namespace))
            {
                contents.AppendLine($"namespace {enumInfo.Namespace}");
                contents.AppendLine("{");
                indent += "    ";
            }

            // Enum docs
            GenerateCSharpComment(contents, indent, enumInfo.Comment);

            // Enum begin
            GenerateCSharpAttributes(buildData, contents, indent, enumInfo, true);
            contents.Append(indent).Append(GenerateCSharpAccessLevel(enumInfo.Access));
            contents.Append("enum ").Append(enumInfo.Name);
            string managedType = string.Empty;
            if (enumInfo.UnderlyingType != null)
            {
                managedType = GenerateCSharpNativeToManaged(buildData, enumInfo.UnderlyingType, enumInfo);
                contents.Append(" : ").Append(managedType);
            }
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Entries
            bool usedMax = false;
            foreach (var entryInfo in enumInfo.Entries)
            {
                contents.AppendLine();
                GenerateCSharpComment(contents, indent, entryInfo.Comment);
                GenerateCSharpAttributes(buildData, contents, indent, enumInfo, entryInfo.Attributes, entryInfo.Comment, true, false);
                contents.Append(indent).Append(entryInfo.Name);
                if (!string.IsNullOrEmpty(entryInfo.Value))
                {
                    if (usedMax)
                        usedMax = false;

                    string value;
                    if (enumInfo.UnderlyingType != null)
                    {
                        value = GenerateCSharpDefaultValueNativeToManaged(buildData, entryInfo.Value, enumInfo, enumInfo.UnderlyingType, false, managedType);
                        if (value.Contains($"{managedType}.MaxValue", StringComparison.Ordinal))
                            usedMax = true;
                    }
                    else
                    {
                        value = entryInfo.Value;
                    }
                    contents.Append(" = ").Append(value);
                }
                // Handle case of next value after Max value being zero if a value is not defined.
                else if (string.IsNullOrEmpty(entryInfo.Value) && usedMax)
                {
                    contents.Append(" = ").Append('0');
                    usedMax = false;
                }
                contents.Append(',');
                contents.AppendLine();
            }

            // Enum end
            indent = indent.Substring(0, indent.Length - 4);
            contents.AppendLine(indent + "}");

            // Namespace end
            if (!string.IsNullOrEmpty(enumInfo.Namespace))
            {
                contents.AppendLine("}");
            }
        }

        private static void GenerateCSharpInterface(BuildData buildData, StringBuilder contents, string indent, InterfaceInfo interfaceInfo)
        {
            // Begin
            contents.AppendLine();
            string interopNamespace = "";
            if (!string.IsNullOrEmpty(interfaceInfo.Namespace))
            {
                interopNamespace = $"{interfaceInfo.Namespace}.Interop";
                contents.AppendLine($"namespace {interfaceInfo.Namespace}");
                contents.AppendLine("{");
                indent += "    ";
            }
            GenerateCSharpComment(contents, indent, interfaceInfo.Comment);
            GenerateCSharpAttributes(buildData, contents, indent, interfaceInfo, true);
            contents.Append(indent).Append(GenerateCSharpAccessLevel(interfaceInfo.Access));
            contents.Append("unsafe partial interface ").Append(interfaceInfo.Name);
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Functions
            foreach (var functionInfo in interfaceInfo.Functions)
            {
                if (functionInfo.IsStatic)
                    throw new Exception($"Not supported {"static"} function {functionInfo.Name} inside interface {interfaceInfo.Name}.");
                if (functionInfo.NoProxy)
                    throw new Exception($"Not supported {"NoProxy"} function {functionInfo.Name} inside interface {interfaceInfo.Name}.");
                if (!functionInfo.IsVirtual)
                    throw new Exception($"Not supported {"non-virtual"} function {functionInfo.Name} inside interface {interfaceInfo.Name}.");
                if (functionInfo.Access != AccessLevel.Public)
                    throw new Exception($"Not supported {"non-public"} function {functionInfo.Name} inside interface {interfaceInfo.Name}.");

                contents.AppendLine();
                GenerateCSharpComment(contents, indent, functionInfo.Comment);
                GenerateCSharpAttributes(buildData, contents, indent, interfaceInfo, functionInfo, true);
                var returnValueType = GenerateCSharpNativeToManaged(buildData, functionInfo.ReturnType, interfaceInfo);
                contents.Append(indent).Append(returnValueType).Append(' ').Append(functionInfo.Name).Append('(');

                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];
                    if (i != 0)
                        contents.Append(", ");
                    if (!string.IsNullOrEmpty(parameterInfo.Attributes))
                        contents.Append('[').Append(parameterInfo.Attributes).Append(']').Append(' ');

                    var managedType = GenerateCSharpNativeToManaged(buildData, parameterInfo.Type, interfaceInfo);
                    if (parameterInfo.IsOut)
                        contents.Append("out ");
                    else if (parameterInfo.IsRef)
                        contents.Append("ref ");
                    else if (parameterInfo.IsThis)
                        contents.Append("this ");
                    else if (parameterInfo.IsParams)
                        contents.Append("params ");
                    contents.Append(managedType);
                    contents.Append(' ');
                    contents.Append(parameterInfo.Name);

                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, interfaceInfo, parameterInfo.Type, false, managedType);
                    if (!string.IsNullOrEmpty(defaultValue))
                        contents.Append(" = ").Append(defaultValue);
                }

                contents.Append(");").AppendLine();
            }

            GenerateCSharpManagedTypeInternals(buildData, interfaceInfo, contents, indent);

            // End
            indent = indent.Substring(0, indent.Length - 4);
            contents.AppendLine(indent + "}");

#if USE_NETCORE
            {
                if (!string.IsNullOrEmpty(interopNamespace))
                {
                    contents.AppendLine("}");
                    contents.AppendLine($"namespace {interopNamespace}");
                    contents.AppendLine("{");
                }

                string marshallerName = interfaceInfo.Name + "Marshaller";
                string marshallerFullName = !string.IsNullOrEmpty(interopNamespace) ? $"{interopNamespace}.{marshallerName}" : marshallerName;
                contents.AppendLine();
                contents.Append(indent).AppendLine("/// <summary>");
                contents.Append(indent).AppendLine($"/// Marshaller for type <see cref=\"{interfaceInfo.Name}\"/>.");
                contents.Append(indent).AppendLine("/// </summary>");
                if (buildData.Target != null & buildData.Target.IsEditor)
                    contents.Append(indent).AppendLine("[HideInEditor]");
                contents.Append(indent).AppendLine($"[CustomMarshaller(typeof({interfaceInfo.Name}), MarshalMode.Default, typeof({marshallerName}))]");
                contents.Append(indent).AppendLine($"public static class {marshallerName}");
                contents.Append(indent).AppendLine("{");
                contents.AppendLine("#pragma warning disable 1591");
                contents.AppendLine("#pragma warning disable 618");
                contents.Append(indent).Append("    ").AppendLine($"internal static {interfaceInfo.Name} ConvertToManaged(IntPtr unmanaged) => ({interfaceInfo.Name})ManagedHandleMarshaller.ConvertToManaged(unmanaged);");
                contents.Append(indent).Append("    ").AppendLine($"internal static IntPtr ConvertToUnmanaged({interfaceInfo.Name} managed) => ManagedHandleMarshaller.ConvertToUnmanaged(managed);");
                contents.AppendLine("#pragma warning restore 618");
                contents.AppendLine("#pragma warning restore 1591");
                contents.Append(indent).AppendLine("}");
            }
#endif

            if (!string.IsNullOrEmpty(interfaceInfo.Namespace))
                contents.AppendLine("}");
        }

        private static bool GenerateCSharpType(BuildData buildData, StringBuilder contents, string indent, object type)
        {
            if (type is ApiTypeInfo apiTypeInfo && apiTypeInfo.SkipGeneration)
                return false;

            try
            {
                if (type is ClassInfo classInfo)
                    GenerateCSharpClass(buildData, contents, indent, classInfo);
                else if (type is StructureInfo structureInfo)
                    GenerateCSharpStructure(buildData, contents, indent, structureInfo);
                else if (type is EnumInfo enumInfo)
                    GenerateCSharpEnum(buildData, contents, indent, enumInfo);
                else if (type is InterfaceInfo interfaceInfo)
                    GenerateCSharpInterface(buildData, contents, indent, interfaceInfo);
                else if (type is InjectCodeInfo injectCodeInfo && string.Equals(injectCodeInfo.Lang, "csharp", StringComparison.OrdinalIgnoreCase))
                {
                    // `using` directives needs to go above the generated code
                    foreach (var code in injectCodeInfo.Code.Split(';'))
                    {
                        if (code.StartsWith("using"))
                            CSharpUsedNamespaces.Add(code.Substring(6));
                        else if (code.Length > 0)
                            contents.Append(code).AppendLine(";");
                    }
                }
                else
                    return false;
            }
            catch
            {
                Log.Error($"Failed to generate C# bindings for {type}.");
                throw;
            }

            return true;
        }

        private static void GenerateCSharp(BuildData buildData, ModuleInfo moduleInfo, ref BindingsResult bindings)
        {
            var contents = GetStringBuilder();
            buildData.Modules.TryGetValue(moduleInfo.Module, out var moduleBuildInfo);

            // Header
            contents.Append("#if ");
            foreach (var define in moduleBuildInfo.ScriptingAPI.Defines)
                contents.Append(define).Append(" && ");
            contents.Append("true").AppendLine();
            contents.AppendLine("// This code was auto-generated. Do not modify it.");
            contents.AppendLine();
            contents.AppendLine("#pragma warning disable 0612"); // Disable Obsolete warnings in generated code
            var headerPos = contents.Length;

            CSharpUsedNamespaces.Clear();
            CSharpUsedNamespaces.Add(null);
            CSharpUsedNamespaces.Add(string.Empty);
            CSharpUsedNamespaces.Add("System");
            CSharpUsedNamespaces.Add("System.ComponentModel");
            CSharpUsedNamespaces.Add("System.Globalization");
            CSharpUsedNamespaces.Add("System.Runtime.CompilerServices");
            CSharpUsedNamespaces.Add("System.Runtime.InteropServices");
#if USE_NETCORE
            CSharpUsedNamespaces.Add("System.Runtime.InteropServices.Marshalling");
#endif
            CSharpUsedNamespaces.Add("FlaxEngine");
            CSharpUsedNamespaces.Add("FlaxEngine.Interop");

            // Process all API types from the file
            var useBindings = false;
            foreach (var child in moduleInfo.Children)
            {
                foreach (var apiTypeInfo in child.Children)
                {
                    if (GenerateCSharpType(buildData, contents, string.Empty, apiTypeInfo))
                        useBindings = true;
                }
            }
            if (!useBindings)
                return;

            {
                var header = GetStringBuilder();

                // Using declarations
                CSharpUsedNamespacesSorted.Clear();
                CSharpUsedNamespacesSorted.AddRange(CSharpUsedNamespaces);
                CSharpUsedNamespacesSorted.Sort();
                for (var i = 2; i < CSharpUsedNamespacesSorted.Count; i++)
                    header.AppendLine($"using {CSharpUsedNamespacesSorted[i]};");

                contents.Insert(headerPos, header.ToString());
                PutStringBuilder(header);
            }

            // Save generated file
            contents.AppendLine();
            contents.AppendLine("#endif");
            Utilities.WriteFileIfChanged(bindings.GeneratedCSharpFilePath, contents.ToString());
            PutStringBuilder(contents);
        }

        internal struct GuidInterop
        {
            public uint A;
            public uint B;
            public uint C;
            public uint D;
        }

        private static unsafe void GenerateCSharp(BuildData buildData, IGrouping<string, Module> binaryModule)
        {
            // Skip generating C# bindings code for native-only modules
            if (binaryModule.All(x => !x.BuildCSharp))
                return;

            var contents = GetStringBuilder();
            var binaryModuleName = binaryModule.Key;
            var project = Builder.GetModuleProject(binaryModule.First(), buildData);

            // Generate binary module ID
            GuidInterop idInterop;
            idInterop.A = Utilities.GetHashCode(binaryModuleName);
            idInterop.B = Utilities.GetHashCode(project.Name);
            idInterop.C = Utilities.GetHashCode(project.Company);
            idInterop.D = Utilities.GetHashCode(project.Copyright);
            var id = *(Guid*)&idInterop;

            // Generate C# binary module information
            var dstFilePath = Path.Combine(project.ProjectFolderPath, "Source", binaryModuleName + ".Gen.cs");
            contents.AppendLine("// This code was auto-generated. Do not modify it.");
            contents.AppendLine();
            contents.AppendLine("using System.Reflection;");
            contents.AppendLine("using System.Runtime.InteropServices;");
#if USE_NETCORE
            contents.AppendLine("using System.Runtime.CompilerServices;");
#endif
            contents.AppendLine();
            contents.AppendLine($"[assembly: AssemblyTitle(\"{binaryModuleName}\")]");
            contents.AppendLine("[assembly: AssemblyDescription(\"\")]");
            contents.AppendLine("[assembly: AssemblyConfiguration(\"\")]");
            contents.AppendLine($"[assembly: AssemblyCompany(\"{project.Company}\")]");
            contents.AppendLine("[assembly: AssemblyProduct(\"FlaxEngine\")]");
            contents.AppendLine($"[assembly: AssemblyCopyright(\"{project.Copyright}\")]");
            contents.AppendLine("[assembly: AssemblyTrademark(\"\")]");
            contents.AppendLine("[assembly: AssemblyCulture(\"\")]");
            contents.AppendLine("[assembly: ComVisible(false)]");
            contents.AppendLine($"[assembly: Guid(\"{id:d}\")]");
            contents.AppendLine($"[assembly: AssemblyVersion(\"{project.Version}\")]");
            contents.AppendLine($"[assembly: AssemblyFileVersion(\"{project.Version}\")]");
#if USE_NETCORE
            contents.AppendLine("[assembly: DisableRuntimeMarshalling]");
#endif
            Utilities.WriteFileIfChanged(dstFilePath, contents.ToString());
            PutStringBuilder(contents);
        }
    }
}

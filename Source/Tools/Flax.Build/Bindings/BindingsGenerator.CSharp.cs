// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

//#define AUTO_DOC_TOOLTIPS

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

        public static event Action<BuildData, ApiTypeInfo, StringBuilder, string> GenerateCSharpTypeInternals;

        internal static readonly Dictionary<string, string> CSharpNativeToManagedBasicTypes = new Dictionary<string, string>()
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

        internal static readonly Dictionary<string, string> CSharpNativeToManagedDefault = new Dictionary<string, string>()
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
            { "MClass", "System.Type" },
            { "Variant", "object" },
            { "VariantType", "System.Type" },

            // Mono types
            { "MObject", "object" },
            { "MonoObject", "object" },
            { "MonoReflectionType", "System.Type" },
            { "MonoType", "IntPtr" },
            { "MonoArray", "Array" },
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

        private static string GenerateCSharpDefaultValueNativeToManaged(BuildData buildData, string value, ApiTypeInfo caller, TypeInfo valueType = null, bool attribute = false)
        {
            if (string.IsNullOrEmpty(value))
                return null;

            // Special case for Engine TEXT macro
            if (value.StartsWith("TEXT(\"") && value.EndsWith("\")"))
                return value.Substring(5, value.Length - 6);

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

            // Numbers
            if (float.TryParse(value, out _) || (value[value.Length - 1] == 'f' && float.TryParse(value.Substring(0, value.Length - 1), out _)))
            {
                // If the value type is different than the value (eg. value is int but the field is float) then cast it for the [DefaultValue] attribute
                if (valueType != null && attribute)
                    return $"({GenerateCSharpNativeToManaged(buildData, valueType, caller)}){value}";
                return value;
            }

            value = value.Replace("::", ".");
            var dot = value.LastIndexOf('.');
            ApiTypeInfo apiType = null;
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
                            return $"typeof({vectorType}), \"{value.Substring(vectorType.Length + 1, value.Length - vectorType.Length - 2).Replace("f", "")}\"";
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
                var sb = new StringBuilder();
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
                return sb.ToString();
            }

            // Special case for value constructors
            if (value.Contains('(') && value.Contains(')'))
                return "new " + value;

            return value;
        }

        private static string GenerateCSharpNativeToManaged(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller)
        {
            string result;
            if (typeInfo?.Type == null)
                throw new ArgumentNullException();

            // Use dynamic array as wrapper container for fixed-size native arrays
            if (typeInfo.IsArray)
            {
                typeInfo.IsArray = false;
                result = GenerateCSharpNativeToManaged(buildData, typeInfo, caller);
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
            {
                return result;
            }

            // Object reference property
            if ((typeInfo.Type == "ScriptingObjectReference" ||
                 typeInfo.Type == "AssetReference" ||
                 typeInfo.Type == "WeakAssetReference" ||
                 typeInfo.Type == "SoftAssetReference" ||
                 typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
                return GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller);

            // Array or Span or DataContainer
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span" || typeInfo.Type == "DataContainer") && typeInfo.GenericArgs != null)
                return GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller) + "[]";

            // Dictionary
            if (typeInfo.Type == "Dictionary" && typeInfo.GenericArgs != null)
                return string.Format("System.Collections.Generic.Dictionary<{0}, {1}>", GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller), GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[1], caller));

            // HashSet
            if (typeInfo.Type == "HashSet" && typeInfo.GenericArgs != null)
                return string.Format("System.Collections.Generic.HashSet<{0}>", GenerateCSharpNativeToManaged(buildData, typeInfo.GenericArgs[0], caller));

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
            }

            // Find API type info
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            var typeName = typeInfo.Type.Replace("::", ".");
            if (apiType != null)
            {
                CSharpUsedNamespaces.Add(apiType.Namespace);
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

        private static string GenerateCSharpManagedToNativeType(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller)
        {
            // Fixed-size array
            if (typeInfo.IsArray)
                return GenerateCSharpNativeToManaged(buildData, typeInfo, caller);

            // Find API type info
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                if (apiType.IsScriptingObject || apiType.IsInterface)
                    return "IntPtr";
            }

            // Object reference property
            if ((typeInfo.Type == "ScriptingObjectReference" ||
                 typeInfo.Type == "AssetReference" ||
                 typeInfo.Type == "WeakAssetReference" ||
                 typeInfo.Type == "SoftAssetReference" ||
                 typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
                return "IntPtr";

            // Function
            if (typeInfo.Type == "Function" && typeInfo.GenericArgs != null)
                return "IntPtr";

            return GenerateCSharpNativeToManaged(buildData, typeInfo, caller);
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
                return "Marshal.GetFunctionPointerForDelegate({0})";
            default:
                var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
                if (apiType != null)
                {
                    // Scripting Object
                    if (apiType.IsScriptingObject)
                        return "FlaxEngine.Object.GetUnmanagedPtr({0})";

                    // interface
                    if (apiType.IsInterface)
                        return string.Format("FlaxEngine.Object.GetUnmanagedInterface({{0}}, typeof({0}))", apiType.FullNameManaged);
                }

                // Object reference property
                if ((typeInfo.Type == "ScriptingObjectReference" ||
                     typeInfo.Type == "AssetReference" ||
                     typeInfo.Type == "WeakAssetReference" ||
                     typeInfo.Type == "SoftAssetReference" ||
                     typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
                    return "FlaxEngine.Object.GetUnmanagedPtr({0})";

                // Default
                return string.Empty;
            }
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
                returnValueType = GenerateCSharpNativeToManaged(buildData, functionInfo.ReturnType, caller);
            }

            contents.AppendLine().Append(indent).Append("[MethodImpl(MethodImplOptions.InternalCall)]");
            contents.AppendLine().Append(indent).Append("internal static extern ");
            contents.Append(returnValueType).Append(" Internal_").Append(functionInfo.UniqueName).Append('(');

            var separator = false;
            if (!functionInfo.IsStatic)
            {
                contents.Append("IntPtr obj");
                separator = true;
            }

            foreach (var parameterInfo in functionInfo.Parameters)
            {
                if (separator)
                    contents.Append(", ");
                separator = true;

                var nativeType = GenerateCSharpManagedToNativeType(buildData, parameterInfo.Type, caller);
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

                var nativeType = GenerateCSharpManagedToNativeType(buildData, parameterInfo.Type, caller);
                if (parameterInfo.IsOut)
                    contents.Append("out ");
                else if (parameterInfo.IsRef || UsePassByReference(buildData, parameterInfo.Type, caller))
                    contents.Append("ref ");
                contents.Append(nativeType);
                contents.Append(' ');
                contents.Append(parameterInfo.Name);
            }

            contents.Append(')');
            contents.Append(';');
            contents.AppendLine();
        }

        private static void GenerateCSharpWrapperFunctionCall(BuildData buildData, StringBuilder contents, ApiTypeInfo caller, FunctionInfo functionInfo, bool isSetter = false)
        {
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

            contents.Append(");");

            // Return result
            if (functionInfo.Glue.UseReferenceForResult)
            {
                contents.Append(" return resultAsRef;");
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

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, string attributes = null, string[] comment = null, bool canUseTooltip = false, bool useUnmanaged = false, string defaultValue = null, bool isDeprecated = false, TypeInfo defaultValueType = null)
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
            }
            if (isDeprecated || apiTypeInfo.IsDeprecated)
            {
                // Deprecated type
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
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, bool useUnmanaged, string defaultValue = null, TypeInfo defaultValueType = null)
        {
            GenerateCSharpAttributes(buildData, contents, indent, apiTypeInfo, apiTypeInfo.Attributes, apiTypeInfo.Comment, true, useUnmanaged, defaultValue, false, defaultValueType);
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, MemberInfo memberInfo, bool useUnmanaged, string defaultValue = null, TypeInfo defaultValueType = null)
        {
            GenerateCSharpAttributes(buildData, contents, indent, apiTypeInfo, memberInfo.Attributes, memberInfo.Comment, true, useUnmanaged, defaultValue, memberInfo.IsDeprecated, defaultValueType);
        }

        private static void GenerateCSharpAccessLevel(StringBuilder contents, AccessLevel access)
        {
            if (access == AccessLevel.Public)
                contents.Append("public ");
            else if (access == AccessLevel.Protected)
                contents.Append("protected ");
            else if (access == AccessLevel.Private)
                contents.Append("private ");
        }

        private static void GenerateCSharpClass(BuildData buildData, StringBuilder contents, string indent, ClassInfo classInfo)
        {
            var useUnmanaged = classInfo.IsStatic || classInfo.IsScriptingObject;
            contents.AppendLine();

            // Namespace begin
            if (!string.IsNullOrEmpty(classInfo.Namespace))
            {
                contents.AppendFormat("namespace ");
                contents.AppendLine(classInfo.Namespace);
                contents.AppendLine("{");
                indent += "    ";
            }

            // Class docs
            GenerateCSharpComment(contents, indent, classInfo.Comment);

            // Class begin
            GenerateCSharpAttributes(buildData, contents, indent, classInfo, useUnmanaged);
            contents.Append(indent);
            GenerateCSharpAccessLevel(contents, classInfo.Access);
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

                string eventSignature;
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
                    eventSignature = "event " + eventInfo.Name + "Delegate";
                }
                else
                {
                    eventSignature = "event Action";
                    if (paramsCount != 0)
                    {
                        eventSignature += '<';
                        for (var i = 0; i < paramsCount; i++)
                        {
                            if (i != 0)
                                eventSignature += ", ";
                            CppParamsWrappersCache[i] = GenerateCSharpNativeToManaged(buildData, eventInfo.Type.GenericArgs[i], classInfo);
                            eventSignature += CppParamsWrappersCache[i];
                        }
                        eventSignature += '>';
                    }
                }

                GenerateCSharpComment(contents, indent, eventInfo.Comment, true);
                GenerateCSharpAttributes(buildData, contents, indent, classInfo, eventInfo, useUnmanaged);
                contents.Append(indent);
                GenerateCSharpAccessLevel(contents, eventInfo.Access);
                if (eventInfo.IsStatic)
                    contents.Append("static ");
                contents.Append(eventSignature);
                contents.Append(' ').AppendLine(eventInfo.Name);
                contents.Append(indent).Append('{').AppendLine();
                indent += "    ";
                var eventInstance = eventInfo.IsStatic ? string.Empty : "__unmanagedPtr, ";
                contents.Append(indent).Append($"add {{ Internal_{eventInfo.Name} += value; if (Internal_{eventInfo.Name}_Count++ == 0) Internal_{eventInfo.Name}_Bind({eventInstance}true); }}").AppendLine();
                contents.Append(indent).Append("remove { ").AppendLine();
                contents.Append("#if FLAX_EDITOR || BUILD_DEBUG").AppendLine();
                contents.Append(indent).Append($"if (Internal_{eventInfo.Name} != null) {{ bool invalid = true; foreach (Delegate e in Internal_{eventInfo.Name}.GetInvocationList()) {{ if (e == (Delegate)value) {{ invalid = false; break; }} }} if (invalid) throw new Exception(\"Cannot unregister from event if not registered before.\"); }}").AppendLine();
                contents.Append("#endif").AppendLine();
                contents.Append(indent).Append($"Internal_{eventInfo.Name} -= value; if (--Internal_{eventInfo.Name}_Count == 0) Internal_{eventInfo.Name}_Bind({eventInstance}false); }}").AppendLine();
                indent = indent.Substring(0, indent.Length - 4);
                contents.Append(indent).Append('}').AppendLine();

                contents.AppendLine();
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
                contents.Append(indent).Append("[MethodImpl(MethodImplOptions.InternalCall)]").AppendLine();
                contents.Append(indent).Append($"internal static extern void Internal_{eventInfo.Name}_Bind(");
                if (!eventInfo.IsStatic)
                    contents.Append("IntPtr obj, ");
                contents.Append("bool bind);");
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
                contents.Append(indent);
                GenerateCSharpAccessLevel(contents, fieldInfo.Access);
                if (fieldInfo.IsConstexpr)
                    contents.Append("const ");
                else if (fieldInfo.IsStatic)
                    contents.Append("static ");

                var returnValueType = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, classInfo);
                contents.Append(returnValueType).Append(' ').Append(fieldInfo.Name);

                if (!useUnmanaged || fieldInfo.IsConstexpr)
                {
                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, fieldInfo.DefaultValue, classInfo, fieldInfo.Type);
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
                contents.Append(indent);
                GenerateCSharpAccessLevel(contents, propertyInfo.Access);
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
                        GenerateCSharpAccessLevel(contents, propertyInfo.Getter.Access);
                    contents.Append("get { ");
                    GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, propertyInfo.Getter);
                    contents.Append(" }").AppendLine();
                }

                if (propertyInfo.Setter != null)
                {
                    contents.Append(indent);
                    if (propertyInfo.Access != propertyInfo.Setter.Access)
                        GenerateCSharpAccessLevel(contents, propertyInfo.Setter.Access);
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
                    contents.Append(indent);
                    GenerateCSharpAccessLevel(contents, functionInfo.Access);
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
                        contents.Append(managedType);
                        contents.Append(' ');
                        contents.Append(parameterInfo.Name);

                        var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, classInfo, parameterInfo.Type);
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
                        if (functionInfo.Comment.Length != 0)
                            contents.Append(indent).AppendLine("/// <inheritdoc />");
                        GenerateCSharpAttributes(buildData, contents, indent, classInfo, functionInfo.Attributes, null, false, useUnmanaged);
                        contents.Append(indent);
                        GenerateCSharpAccessLevel(contents, functionInfo.Access);
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
                            contents.Append(managedType);
                            contents.Append(' ');
                            contents.Append(parameterInfo.Name);

                            var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, classInfo, parameterInfo.Type);
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

            GenerateCSharpTypeInternals?.Invoke(buildData, classInfo, contents, indent);

            // Nested types
            foreach (var apiTypeInfo in classInfo.Children)
            {
                GenerateCSharpType(buildData, contents, indent, apiTypeInfo);
            }

            // Class end
            indent = indent.Substring(0, indent.Length - 4);
            contents.AppendLine(indent + "}");

            // Namespace end
            if (!string.IsNullOrEmpty(classInfo.Namespace))
            {
                contents.AppendLine("}");
            }
        }

        private static void GenerateCSharpStructure(BuildData buildData, StringBuilder contents, string indent, StructureInfo structureInfo)
        {
            contents.AppendLine();

            // Namespace begin
            if (!string.IsNullOrEmpty(structureInfo.Namespace))
            {
                contents.AppendFormat("namespace ");
                contents.AppendLine(structureInfo.Namespace);
                contents.AppendLine("{");
                indent += "    ";
            }

            // Struct docs
            GenerateCSharpComment(contents, indent, structureInfo.Comment);

            // Struct begin
            GenerateCSharpAttributes(buildData, contents, indent, structureInfo, true);
            contents.Append(indent).AppendLine("[StructLayout(LayoutKind.Sequential)]");
            contents.Append(indent);
            GenerateCSharpAccessLevel(contents, structureInfo.Access);
            contents.Append("unsafe partial struct ").Append(structureInfo.Name);
            if (structureInfo.BaseType != null && structureInfo.IsPod)
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, new TypeInfo(structureInfo.BaseType), structureInfo));
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Fields
            var hasDefaultMember = false;
            foreach (var fieldInfo in structureInfo.Fields)
            {
                contents.AppendLine();
                GenerateCSharpComment(contents, indent, fieldInfo.Comment);
                GenerateCSharpAttributes(buildData, contents, indent, structureInfo, fieldInfo, fieldInfo.IsStatic, fieldInfo.DefaultValue, fieldInfo.Type);
                contents.Append(indent);
                GenerateCSharpAccessLevel(contents, fieldInfo.Access);
                if (fieldInfo.IsConstexpr)
                    contents.Append("const ");
                else if (fieldInfo.IsStatic)
                    contents.Append("static ");
                hasDefaultMember |= string.Equals(fieldInfo.Name, "Default", StringComparison.Ordinal);
                string type;

                if (fieldInfo.Type.IsArray && (fieldInfo.NoArray || structureInfo.IsPod))
                {
                    // Fixed-size array that needs to be inlined into structure instead of passing it as managed array
                    fieldInfo.Type.IsArray = false;
                    type = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, structureInfo);
                    fieldInfo.Type.IsArray = true;
                    contents.Append(type).Append(' ').Append(fieldInfo.Name + "0;").AppendLine();
                    for (int i = 1; i < fieldInfo.Type.ArraySize; i++)
                    {
                        contents.AppendLine();
                        GenerateCSharpComment(contents, indent, fieldInfo.Comment);
                        GenerateCSharpAttributes(buildData, contents, indent, structureInfo, fieldInfo, fieldInfo.IsStatic);
                        contents.Append(indent);
                        GenerateCSharpAccessLevel(contents, fieldInfo.Access);
                        if (fieldInfo.IsStatic)
                            contents.Append("static ");
                        contents.Append(type).Append(' ').Append(fieldInfo.Name + i).Append(';').AppendLine();
                    }
                    continue;
                }

                type = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, structureInfo);
                contents.Append(type).Append(' ').Append(fieldInfo.Name);

                if (fieldInfo.IsConstexpr)
                {
                    // Compile-time constant
                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, fieldInfo.DefaultValue, structureInfo, fieldInfo.Type);
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
                contents.AppendLine();
                contents.Append(indent).AppendLine("private static bool _defaultValid;");
                contents.Append(indent).AppendLine($"private static {structureInfo.Name} _default;");
                contents.AppendLine();
                contents.Append(indent).AppendLine("/// <summary>");
                contents.Append(indent).AppendLine($"/// The default <see cref=\"{structureInfo.Name}\"/>.");
                contents.Append(indent).AppendLine("/// </summary>");
                contents.Append(indent).AppendLine($"public static {structureInfo.Name} Default");
                contents.Append(indent).AppendLine("{");
                indent += "    ";
                contents.Append(indent).AppendLine("get");
                contents.Append(indent).AppendLine("{");
                indent += "    ";
                contents.Append(indent).AppendLine("if (!_defaultValid)");
                contents.Append(indent).AppendLine("{");
                indent += "    ";
                contents.Append(indent).AppendLine("_defaultValid = true;");
                contents.Append(indent).AppendLine("object obj = _default;");
                contents.Append(indent).AppendLine($"FlaxEngine.Utils.InitStructure(obj, typeof({structureInfo.Name}));");
                contents.Append(indent).AppendLine($"_default = ({structureInfo.Name})obj;");
                indent = indent.Substring(0, indent.Length - 4);
                contents.Append(indent).AppendLine("}");
                contents.Append(indent).AppendLine("return _default;");
                indent = indent.Substring(0, indent.Length - 4);
                contents.Append(indent).AppendLine("}");
                indent = indent.Substring(0, indent.Length - 4);
                contents.Append(indent).AppendLine("}");
                contents.AppendLine();
                contents.Append(indent).AppendLine("[System.Runtime.Serialization.OnDeserializing]");
                contents.Append(indent).AppendLine("internal void OnDeserializing(System.Runtime.Serialization.StreamingContext context)");
                contents.Append(indent).AppendLine("{");
                contents.Append(indent).AppendLine("    // Initialize structure with default values to replicate C++ deserialization behavior");
                contents.Append(indent).AppendLine("    this = Default;");
                contents.Append(indent).AppendLine("}");
            }

            GenerateCSharpTypeInternals?.Invoke(buildData, structureInfo, contents, indent);

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
                contents.AppendFormat("namespace ");
                contents.AppendLine(enumInfo.Namespace);
                contents.AppendLine("{");
                indent += "    ";
            }

            // Enum docs
            GenerateCSharpComment(contents, indent, enumInfo.Comment);

            // Enum begin
            GenerateCSharpAttributes(buildData, contents, indent, enumInfo, true);
            contents.Append(indent);
            GenerateCSharpAccessLevel(contents, enumInfo.Access);
            contents.Append("enum ").Append(enumInfo.Name);
            if (enumInfo.UnderlyingType != null)
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, enumInfo.UnderlyingType, enumInfo));
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Entries
            foreach (var entryInfo in enumInfo.Entries)
            {
                contents.AppendLine();
                GenerateCSharpComment(contents, indent, entryInfo.Comment);
                GenerateCSharpAttributes(buildData, contents, indent, enumInfo, entryInfo.Attributes, entryInfo.Comment, true, false);
                contents.Append(indent).Append(entryInfo.Name);
                if (!string.IsNullOrEmpty(entryInfo.Value))
                    contents.Append(" = ").Append(entryInfo.Value);
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
            if (!string.IsNullOrEmpty(interfaceInfo.Namespace))
            {
                contents.AppendFormat("namespace ");
                contents.AppendLine(interfaceInfo.Namespace);
                contents.AppendLine("{");
                indent += "    ";
            }
            GenerateCSharpComment(contents, indent, interfaceInfo.Comment);
            GenerateCSharpAttributes(buildData, contents, indent, interfaceInfo, true);
            contents.Append(indent);
            GenerateCSharpAccessLevel(contents, interfaceInfo.Access);
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
                    contents.Append(managedType);
                    contents.Append(' ');
                    contents.Append(parameterInfo.Name);

                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, interfaceInfo, parameterInfo.Type);
                    if (!string.IsNullOrEmpty(defaultValue))
                        contents.Append(" = ").Append(defaultValue);
                }

                contents.Append(");").AppendLine();
            }

            GenerateCSharpTypeInternals?.Invoke(buildData, interfaceInfo, contents, indent);

            // End
            indent = indent.Substring(0, indent.Length - 4);
            contents.AppendLine(indent + "}");
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
                            contents.Append(injectCodeInfo.Code).AppendLine(";");
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
            var contents = new StringBuilder();
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
            CSharpUsedNamespaces.Add("FlaxEngine");

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
                var header = new StringBuilder();

                // Using declarations
                CSharpUsedNamespacesSorted.Clear();
                CSharpUsedNamespacesSorted.AddRange(CSharpUsedNamespaces);
                CSharpUsedNamespacesSorted.Sort();
                for (var i = 2; i < CSharpUsedNamespacesSorted.Count; i++)
                    header.AppendLine($"using {CSharpUsedNamespacesSorted[i]};");

                contents.Insert(headerPos, header.ToString());
            }

            // Save generated file
            contents.AppendLine();
            contents.AppendLine("#endif");
            Utilities.WriteFileIfChanged(bindings.GeneratedCSharpFilePath, contents.ToString());
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

            var contents = new StringBuilder();
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
            Utilities.WriteFileIfChanged(dstFilePath, contents.ToString());
        }
    }
}

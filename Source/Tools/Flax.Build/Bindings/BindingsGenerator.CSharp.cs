// (c) 2012-2020 Wojciech Figat. All rights reserved.

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

        internal static readonly Dictionary<string, string> CSharpNativeToManagedBasicTypes = new Dictionary<string, string>()
        {
            // Language types
            { "int8", "sbyte" },
            { "int16", "short" },
            { "int32", "int" },
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
            { "PersistentScriptingObject", "FlaxEngine.Object" },
            { "ManagedScriptingObject", "FlaxEngine.Object" },
            { "MClass", "System.Type" },
            { "Variant", "object" },
            { "VariantType", "System.Type" },

            // Mono types
            { "MonoObject", "object" },
            { "MonoReflectionType", "System.Type" },
            { "MonoType", "IntPtr" },
            { "MonoArray", "Array" },
        };

        private static string GenerateCSharpDefaultValueNativeToManaged(BuildData buildData, string value, ApiTypeInfo caller, bool attribute = false)
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
                return value;

            value = value.Replace("::", ".");
            var dot = value.LastIndexOf('.');
            ApiTypeInfo apiType = null;
            if (dot != -1)
            {
                var type = new TypeInfo { Type = value.Substring(0, dot) };
                apiType = FindApiTypeInfo(buildData, type, caller);
            }

            if (attribute)
            {
                // Value constructors (eg. Vector2(1, 2))
                if (value.Contains('(') && value.Contains(')'))
                {
                    // Support for in-built types
                    if (value.StartsWith("Vector2("))
                        return $"typeof(Vector2), \"{value.Substring(8, value.Length - 9).Replace("f", "")}\"";
                    if (value.StartsWith("Vector3("))
                        return $"typeof(Vector3), \"{value.Substring(8, value.Length - 9).Replace("f", "")}\"";
                    if (value.StartsWith("Vector4("))
                        return $"typeof(Vector4), \"{value.Substring(8, value.Length - 9).Replace("f", "")}\"";

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
                    case "Vector3.Zero": return "typeof(Vector3), \"0,0,0\"";
                    case "Vector3.One": return "typeof(Vector3), \"1,1,1\"";
                    case "Vector4.Zero": return "typeof(Vector4), \"0,0,0,0\"";
                    case "Vector4.One": return "typeof(Vector4), \"1,1,1,1\"";
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

            // ScriptingObjectReference or AssetReference or WeakAssetReference or SoftObjectReference
            if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference" || typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
                return typeInfo.GenericArgs[0].Type.Replace("::", ".");

            // Array or Span
            if ((typeInfo.Type == "Array" || typeInfo.Type == "Span") && typeInfo.GenericArgs != null)
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
            if (apiType != null)
            {
                CSharpUsedNamespaces.Add(apiType.Namespace);

                if (apiType.IsScriptingObject || apiType.IsInterface)
                    return typeInfo.Type.Replace("::", ".");
                if (typeInfo.IsPtr && apiType.IsPod)
                    return typeInfo.Type.Replace("::", ".") + '*';
            }

            // Pointer
            if (typeInfo.IsPtr)
                return "IntPtr";

            return typeInfo.Type.Replace("::", ".");
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

            // ScriptingObjectReference or AssetReference or WeakAssetReference or SoftObjectReference
            if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference" || typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
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

                // ScriptingObjectReference or AssetReference or WeakAssetReference or SoftObjectReference
                if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference" || typeInfo.Type == "SoftObjectReference") && typeInfo.GenericArgs != null)
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

                var convertFunc = GenerateCSharpManagedToNativeConverter(buildData, parameterInfo.Type, caller);
                if (string.IsNullOrWhiteSpace(convertFunc))
                {
                    if (parameterInfo.IsOut)
                        contents.Append("out ");
                    else if (parameterInfo.IsRef || UsePassByReference(buildData, parameterInfo.Type, caller))
                        contents.Append("ref ");

                    // Pass value
                    contents.Append(isSetter ? "value" : parameterInfo.Name);
                }
                else
                {
                    if (parameterInfo.IsOut)
                        throw new Exception($"Cannot use Out meta on parameter {parameterInfo} in function {functionInfo.Name} in {caller}.");
                    if (parameterInfo.IsRef)
                        throw new Exception($"Cannot use Ref meta on parameter {parameterInfo} in function {functionInfo.Name} in {caller}.");

                    // Convert value
                    contents.Append(string.Format(convertFunc, isSetter ? "value" : parameterInfo.Name));
                }
            }

            var customParametersCount = functionInfo.Glue.CustomParameters?.Count ?? 0;
            for (var i = 0; i < customParametersCount; i++)
            {
                var parameterInfo = functionInfo.Glue.CustomParameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                var convertFunc = GenerateCSharpManagedToNativeConverter(buildData, parameterInfo.Type, caller);
                if (string.IsNullOrWhiteSpace(convertFunc))
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
            if (functionInfo.Glue.UseReferenceForResult)
            {
                contents.Append(" return resultAsRef;");
            }
        }

        private static void GenerateCSharpComment(StringBuilder contents, string indent, string[] comment)
        {
            foreach (var c in comment)
            {
                contents.Append(indent).Append(c.Replace("::", ".")).AppendLine();
            }
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, string attributes, string[] comment, bool canUseTooltip, bool useUnmanaged, string defaultValue = null)
        {
            var writeTooltip = true;
            var writeDefaultValue = true;
            if (!string.IsNullOrEmpty(attributes))
            {
                // Write attributes
                contents.Append(indent).Append('[').Append(attributes).Append(']').AppendLine();
                writeTooltip = !attributes.Contains("Tooltip(") && !attributes.Contains("HideInEditor");
                writeDefaultValue = !attributes.Contains("DefaultValue(");
            }

            if (useUnmanaged)
            {
                // Write attribute for C++ calling code
                contents.Append(indent).AppendLine("[Unmanaged]");
            }

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
            if (writeDefaultValue)
            {
                // Write default value attribute
                defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, defaultValue, apiTypeInfo, true);
                if (defaultValue != null)
                    contents.Append(indent).Append("[DefaultValue(").Append(defaultValue).Append(")]").AppendLine();
            }
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, bool useUnmanaged, string defaultValue = null)
        {
            GenerateCSharpAttributes(buildData, contents, indent, apiTypeInfo, apiTypeInfo.Attributes, apiTypeInfo.Comment, true, useUnmanaged, defaultValue);
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, MemberInfo memberInfo, bool useUnmanaged, string defaultValue = null)
        {
            GenerateCSharpAttributes(buildData, contents, indent, apiTypeInfo, memberInfo.Attributes, memberInfo.Comment, true, useUnmanaged, defaultValue);
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
            if (classInfo.Access == AccessLevel.Public)
                contents.Append("public ");
            else if (classInfo.Access == AccessLevel.Protected)
                contents.Append("protected ");
            else if (classInfo.Access == AccessLevel.Private)
                contents.Append("private ");
            if (classInfo.IsStatic)
                contents.Append("static ");
            else if (classInfo.IsSealed)
                contents.Append("sealed ");
            else if (classInfo.IsAbstract)
                contents.Append("abstract ");
            contents.Append("unsafe partial class ").Append(classInfo.Name);
            var hasBase = classInfo.BaseType != null && !classInfo.IsBaseTypeHidden;
            if (hasBase)
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, new TypeInfo { Type = classInfo.BaseType.Name }, classInfo));
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
                if (!useUnmanaged)
                    throw new NotImplementedException("TODO: support events inside non-static and non-scripting API class types.");
                var paramsCount = eventInfo.Type.GenericArgs?.Count ?? 0;

                contents.AppendLine();

                var useCustomDelegateSignature = false;
                for (var i = 0; i < paramsCount; i++)
                {
                    var paramType = eventInfo.Type.GenericArgs[i];
                    var result = GenerateCSharpNativeToManaged(buildData, paramType, classInfo);
                    if ((paramType.IsRef && !paramType.IsConst && paramType.IsPod(buildData, classInfo)) || result[result.Length - 1] == '*')
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
                        if (paramType.IsRef && !paramType.IsConst && paramType.IsPod(buildData, classInfo))
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

                foreach (var comment in eventInfo.Comment)
                {
                    if (comment.Contains("/// <returns>"))
                        continue;
                    contents.Append(indent).Append(comment.Replace("::", ".")).AppendLine();
                }

                GenerateCSharpAttributes(buildData, contents, indent, classInfo, eventInfo, useUnmanaged);
                contents.Append(indent);
                if (eventInfo.Access == AccessLevel.Public)
                    contents.Append("public ");
                else if (eventInfo.Access == AccessLevel.Protected)
                    contents.Append("protected ");
                else if (eventInfo.Access == AccessLevel.Private)
                    contents.Append("private ");
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
                    contents.Append(CppParamsWrappersCache[i]);
                    if (paramType.IsRef && !paramType.IsConst && paramType.IsPod(buildData, classInfo))
                        contents.Append("*");
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
                    if (paramType.IsRef && !paramType.IsConst && paramType.IsPod(buildData, classInfo))
                        contents.Append("ref *");
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
                if (fieldInfo.Getter == null)
                    continue;
                contents.AppendLine();

                foreach (var comment in fieldInfo.Comment)
                {
                    if (comment.Contains("/// <returns>"))
                        continue;
                    contents.Append(indent).Append(comment.Replace("::", ".")).AppendLine();
                }

                GenerateCSharpAttributes(buildData, contents, indent, classInfo, fieldInfo, useUnmanaged, fieldInfo.DefaultValue);
                contents.Append(indent);
                if (fieldInfo.Access == AccessLevel.Public)
                    contents.Append("public ");
                else if (fieldInfo.Access == AccessLevel.Protected)
                    contents.Append("protected ");
                else if (fieldInfo.Access == AccessLevel.Private)
                    contents.Append("private ");
                if (fieldInfo.IsStatic)
                    contents.Append("static ");
                var returnValueType = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, classInfo);
                contents.Append(returnValueType).Append(' ').Append(fieldInfo.Name);
                if (!useUnmanaged)
                {
                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, fieldInfo.DefaultValue, classInfo);
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
                if (!useUnmanaged)
                    throw new NotImplementedException("TODO: support properties inside non-static and non-scripting API class types.");

                contents.AppendLine();

                foreach (var comment in propertyInfo.Comment)
                {
                    if (comment.Contains("/// <returns>") || comment.Contains("<param name="))
                        continue;
                    contents.Append(indent).Append(comment.Replace("::", ".")).AppendLine();
                }

                GenerateCSharpAttributes(buildData, contents, indent, classInfo, propertyInfo, useUnmanaged);
                contents.Append(indent);
                if (propertyInfo.Access == AccessLevel.Public)
                    contents.Append("public ");
                else if (propertyInfo.Access == AccessLevel.Protected)
                    contents.Append("protected ");
                else if (propertyInfo.Access == AccessLevel.Private)
                    contents.Append("private ");
                if (propertyInfo.IsStatic)
                    contents.Append("static ");
                var returnValueType = GenerateCSharpNativeToManaged(buildData, propertyInfo.Type, classInfo);
                contents.Append(returnValueType).Append(' ').AppendLine(propertyInfo.Name);
                contents.AppendLine(indent + "{");
                indent += "    ";

                if (propertyInfo.Getter != null)
                {
                    contents.Append(indent).Append("get { ");
                    GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, propertyInfo.Getter);
                    contents.Append(" }").AppendLine();
                }

                if (propertyInfo.Setter != null)
                {
                    contents.Append(indent).Append("set { ");
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
                if (!useUnmanaged)
                    throw new Exception($"Not supported function {functionInfo.Name} inside non-static and non-scripting class type {classInfo.Name}.");

                if (!functionInfo.NoProxy)
                {
                    contents.AppendLine();
                    GenerateCSharpComment(contents, indent, functionInfo.Comment);
                    GenerateCSharpAttributes(buildData, contents, indent, classInfo, functionInfo, true);
                    contents.Append(indent);
                    if (functionInfo.Access == AccessLevel.Public)
                        contents.Append("public ");
                    else if (functionInfo.Access == AccessLevel.Protected)
                        contents.Append("protected ");
                    else if (functionInfo.Access == AccessLevel.Private)
                        contents.Append("private ");
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
                        contents.Append(managedType);
                        contents.Append(' ');
                        contents.Append(parameterInfo.Name);

                        var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, classInfo);
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
                        if (functionInfo.Access == AccessLevel.Public)
                            contents.Append("public ");
                        else if (functionInfo.Access == AccessLevel.Protected)
                            contents.Append("protected ");
                        else if (functionInfo.Access == AccessLevel.Private)
                            contents.Append("private ");
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
                            contents.Append(managedType);
                            contents.Append(' ');
                            contents.Append(parameterInfo.Name);

                            var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, classInfo);
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
            if (structureInfo.Access == AccessLevel.Public)
                contents.Append("public ");
            else if (structureInfo.Access == AccessLevel.Protected)
                contents.Append("protected ");
            else if (structureInfo.Access == AccessLevel.Private)
                contents.Append("private ");
            contents.Append("unsafe partial struct ").Append(structureInfo.Name);
            if (structureInfo.BaseType != null && structureInfo.IsPod)
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, new TypeInfo { Type = structureInfo.BaseType.Name }, structureInfo));
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Fields
            foreach (var fieldInfo in structureInfo.Fields)
            {
                contents.AppendLine();

                foreach (var comment in fieldInfo.Comment)
                    contents.Append(indent).Append(comment).AppendLine();

                GenerateCSharpAttributes(buildData, contents, indent, structureInfo, fieldInfo, fieldInfo.IsStatic, fieldInfo.DefaultValue);
                contents.Append(indent);
                if (fieldInfo.Access == AccessLevel.Public)
                    contents.Append("public ");
                else if (fieldInfo.Access == AccessLevel.Protected)
                    contents.Append("protected ");
                else if (fieldInfo.Access == AccessLevel.Private)
                    contents.Append("private ");
                if (fieldInfo.IsStatic)
                    contents.Append("static ");
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
                        foreach (var comment in fieldInfo.Comment)
                            contents.Append(indent).Append(comment).AppendLine();
                        GenerateCSharpAttributes(buildData, contents, indent, structureInfo, fieldInfo, fieldInfo.IsStatic);
                        contents.Append(indent);
                        if (fieldInfo.Access == AccessLevel.Public)
                            contents.Append("public ");
                        else if (fieldInfo.Access == AccessLevel.Protected)
                            contents.Append("protected ");
                        else if (fieldInfo.Access == AccessLevel.Private)
                            contents.Append("private ");
                        if (fieldInfo.IsStatic)
                            contents.Append("static ");
                        contents.Append(type).Append(' ').Append(fieldInfo.Name + i).Append(';').AppendLine();
                    }
                    continue;
                }

                type = GenerateCSharpNativeToManaged(buildData, fieldInfo.Type, structureInfo);
                contents.Append(type).Append(' ').Append(fieldInfo.Name);

                // Static fields are using C++ static value accessed via getter function binding
                if (fieldInfo.IsStatic)
                {
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
            if (enumInfo.Access == AccessLevel.Public)
                contents.Append("public ");
            else if (enumInfo.Access == AccessLevel.Protected)
                contents.Append("protected ");
            else if (enumInfo.Access == AccessLevel.Private)
                contents.Append("private ");
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

                foreach (var comment in entryInfo.Comment)
                    contents.Append(indent).Append(comment).AppendLine();

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
            if (interfaceInfo.Access == AccessLevel.Public)
                contents.Append("public ");
            else if (interfaceInfo.Access == AccessLevel.Protected)
                contents.Append("protected ");
            else if (interfaceInfo.Access == AccessLevel.Private)
                contents.Append("private ");
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
                    contents.Append(managedType);
                    contents.Append(' ');
                    contents.Append(parameterInfo.Name);

                    var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, interfaceInfo);
                    if (!string.IsNullOrEmpty(defaultValue))
                        contents.Append(" = ").Append(defaultValue);
                }

                contents.Append(");").AppendLine();
            }

            // End
            indent = indent.Substring(0, indent.Length - 4);
            contents.AppendLine(indent + "}");
            if (!string.IsNullOrEmpty(interfaceInfo.Namespace))
                contents.AppendLine("}");
        }

        private static bool GenerateCSharpType(BuildData buildData, StringBuilder contents, string indent, object type)
        {
            if (type is ApiTypeInfo apiTypeInfo && apiTypeInfo.IsInBuild)
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

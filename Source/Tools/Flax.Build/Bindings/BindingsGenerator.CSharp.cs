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
        private static readonly Dictionary<string, string> CSharpNativeToManagedBasicTypes = new Dictionary<string, string>()
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

        private static readonly Dictionary<string, string> CSharpNativeToManagedDefault = new Dictionary<string, string>()
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

        private static string GenerateCSharpDefaultValueNativeToManaged(BuildData buildData, string value, ApiTypeInfo caller)
        {
            if (string.IsNullOrEmpty(value))
                return null;
            value = value.Replace("::", ".");

            // Skip constants unsupported in C#
            var dot = value.LastIndexOf('.');
            if (dot != -1)
            {
                var type = new TypeInfo { Type = value.Substring(0, dot) };
                var apiType = FindApiTypeInfo(buildData, type, caller);
                if (apiType != null && apiType.IsStruct)
                    return null;
            }

            // Convert from C++ to C#
            switch (value)
            {
            case "nullptr":
            case "NULL":
            case "String.Empty":
            case "StringView.Empty": return "null";

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

            default: return value;
            }
        }

        private static string GenerateCSharpNativeToManaged(BuildData buildData, TypeInfo typeInfo, ApiTypeInfo caller)
        {
            string result;

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

            // ScriptingObjectReference or AssetReference or WeakAssetReference
            if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference") && typeInfo.GenericArgs != null)
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

            // Find API type info
            var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
            if (apiType != null)
            {
                if (apiType.IsScriptingObject)
                    return typeInfo.Type.Replace("::", ".");

                if (typeInfo.IsPtr && (apiType is LangType || apiType.IsPod))
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
                if (apiType.IsScriptingObject)
                    return "IntPtr";
            }

            // ScriptingObjectReference or AssetReference or WeakAssetReference
            if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference") && typeInfo.GenericArgs != null)
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
                return "FlaxEngine.Object.GetUnmanagedPtr";
            default:
                var apiType = FindApiTypeInfo(buildData, typeInfo, caller);
                if (apiType != null)
                {
                    // Scripting Object
                    if (apiType.IsScriptingObject)
                        return "FlaxEngine.Object.GetUnmanagedPtr";
                }

                // ScriptingObjectReference or AssetReference or WeakAssetReference
                if ((typeInfo.Type == "ScriptingObjectReference" || typeInfo.Type == "AssetReference" || typeInfo.Type == "WeakAssetReference") && typeInfo.GenericArgs != null)
                    return "FlaxEngine.Object.GetUnmanagedPtr";

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

            foreach (var parameterInfo in functionInfo.Glue.CustomParameters)
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
                    contents.Append(convertFunc);
                    contents.Append('(');
                    contents.Append(isSetter ? "value" : parameterInfo.Name);
                    contents.Append(')');
                }
            }

            foreach (var parameterInfo in functionInfo.Glue.CustomParameters)
            {
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
                    contents.Append(convertFunc);
                    contents.Append('(');
                    contents.Append(parameterInfo.DefaultValue);
                    contents.Append(')');
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

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, string attributes, string[] comment, bool canUseTooltip, bool useUnmanaged)
        {
            var writeTooltip = true;
            if (!string.IsNullOrEmpty(attributes))
            {
                // Write attributes
                contents.Append(indent).Append('[').Append(attributes).Append(']').AppendLine();
                writeTooltip = !attributes.Contains("Tooltip(");
            }

            if (useUnmanaged)
            {
                // Write attribute for C++ calling code
                contents.Append(indent).AppendLine("[Unmanaged]");
            }

            if (canUseTooltip && comment != null && writeTooltip && buildData.Configuration != TargetConfiguration.Release)
            {
                // Write documentation comment as tooltip
                if (comment.Length >= 3 &&
                    comment[0] == "/// <summary>" &&
                    comment[1].StartsWith("/// ") &&
                    comment[2] == "/// </summary>")
                {
                    var tooltip = comment[1].Substring(4);
                    if (tooltip.StartsWith("Gets the "))
                        tooltip = "The " + tooltip.Substring(9);
                    if (tooltip.IndexOf('\"') != -1)
                        tooltip = tooltip.Replace("\"", "\\\"");
                    contents.Append(indent).Append("[Tooltip(\"").Append(tooltip).Append("\")]").AppendLine();
                }
            }
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, ApiTypeInfo apiTypeInfo, bool useUnmanaged)
        {
            GenerateCSharpAttributes(buildData, contents, indent, apiTypeInfo.Attributes, apiTypeInfo.Comment, true, useUnmanaged);
        }

        private static void GenerateCSharpAttributes(BuildData buildData, StringBuilder contents, string indent, MemberInfo memberInfo, bool useUnmanaged)
        {
            GenerateCSharpAttributes(buildData, contents, indent, memberInfo.Attributes, memberInfo.Comment, true, useUnmanaged);
        }

        private static void GenerateCSharpClass(BuildData buildData, StringBuilder contents, string indent, ClassInfo classInfo)
        {
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
            GenerateCSharpAttributes(buildData, contents, indent, classInfo, true);
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
            if (classInfo.BaseType != null && classInfo.BaseTypeInheritance != AccessLevel.Private)
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, classInfo.BaseType, classInfo));
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
                contents.AppendLine();

                foreach (var comment in eventInfo.Comment)
                {
                    if (comment.Contains("/// <returns>"))
                        continue;
                    contents.Append(indent).Append(comment.Replace("::", ".")).AppendLine();
                }

                GenerateCSharpAttributes(buildData, contents, indent, eventInfo, true);
                contents.Append(indent);
                if (eventInfo.Access == AccessLevel.Public)
                    contents.Append("public ");
                else if (eventInfo.Access == AccessLevel.Protected)
                    contents.Append("protected ");
                else if (eventInfo.Access == AccessLevel.Private)
                    contents.Append("private ");
                if (eventInfo.IsStatic)
                    contents.Append("static ");
                string eventSignature = "event Action";
                if (eventInfo.Type.GenericArgs.Count != 0)
                {
                    eventSignature += '<';
                    for (var i = 0; i < eventInfo.Type.GenericArgs.Count; i++)
                    {
                        if (i != 0)
                            eventSignature += ", ";
                        CppParamsWrappersCache[i] = GenerateCSharpNativeToManaged(buildData, eventInfo.Type.GenericArgs[i], classInfo);
                        eventSignature += CppParamsWrappersCache[i];
                    }
                    eventSignature += '>';
                }
                contents.Append(eventSignature);
                contents.Append(' ').AppendLine(eventInfo.Name);
                contents.Append(indent).Append('{').AppendLine();
                indent += "    ";
                var eventInstance = eventInfo.IsStatic ? string.Empty : "__unmanagedPtr, ";
                contents.Append(indent).Append($"add {{ Internal_{eventInfo.Name} += value; if (Internal_{eventInfo.Name}_Count++ == 0) Internal_{eventInfo.Name}_Bind({eventInstance}true); }}").AppendLine();
                contents.Append(indent).Append($"remove {{ Internal_{eventInfo.Name} -= value; if (--Internal_{eventInfo.Name}_Count == 0) Internal_{eventInfo.Name}_Bind({eventInstance}false); }}").AppendLine();
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
                for (var i = 0; i < eventInfo.Type.GenericArgs.Count; i++)
                {
                    if (i != 0)
                        contents.Append(", ");
                    contents.Append(CppParamsWrappersCache[i]);
                    contents.Append(" arg").Append(i);
                }
                contents.Append(')').AppendLine();
                contents.Append(indent).Append('{').AppendLine();
                contents.Append(indent).Append("    Internal_").Append(eventInfo.Name);
                contents.Append('(');
                for (var i = 0; i < eventInfo.Type.GenericArgs.Count; i++)
                {
                    if (i != 0)
                        contents.Append(", ");
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

                GenerateCSharpAttributes(buildData, contents, indent, fieldInfo, true);
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
                contents.Append(returnValueType).Append(' ').AppendLine(fieldInfo.Name);
                contents.AppendLine(indent + "{");
                indent += "    ";

                contents.Append(indent).Append("get { ");
                GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, fieldInfo.Getter);
                contents.Append(" }").AppendLine();

                if (!fieldInfo.IsReadOnly)
                {
                    contents.Append(indent).Append("set { ");
                    GenerateCSharpWrapperFunctionCall(buildData, contents, classInfo, fieldInfo.Setter, true);
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
                contents.AppendLine();

                foreach (var comment in propertyInfo.Comment)
                {
                    if (comment.Contains("/// <returns>") || comment.Contains("<param name="))
                        continue;
                    contents.Append(indent).Append(comment.Replace("::", ".")).AppendLine();
                }

                GenerateCSharpAttributes(buildData, contents, indent, propertyInfo, true);
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
                if (!functionInfo.NoProxy)
                {
                    contents.AppendLine();
                    GenerateCSharpComment(contents, indent, functionInfo.Comment);
                    GenerateCSharpAttributes(buildData, contents, indent, functionInfo, true);
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

                        if (parameterInfo.DefaultValue != null)
                        {
                            var defaultValue = GenerateCSharpDefaultValueNativeToManaged(buildData, parameterInfo.DefaultValue, classInfo);
                            if (!string.IsNullOrEmpty(defaultValue))
                                contents.Append(" = ").Append(defaultValue);
                        }
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
                contents.Append(" : ").Append(GenerateCSharpNativeToManaged(buildData, structureInfo.BaseType, structureInfo));
            contents.AppendLine();
            contents.Append(indent + "{");
            indent += "    ";

            // Fields
            foreach (var fieldInfo in structureInfo.Fields)
            {
                contents.AppendLine();

                foreach (var comment in fieldInfo.Comment)
                    contents.Append(indent).Append(comment).AppendLine();

                GenerateCSharpAttributes(buildData, contents, indent, fieldInfo, fieldInfo.IsStatic);
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

                if (fieldInfo.NoArray && fieldInfo.Type.IsArray)
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
                        GenerateCSharpAttributes(buildData, contents, indent, fieldInfo, fieldInfo.IsStatic);
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

                GenerateCSharpAttributes(buildData, contents, indent, entryInfo.Attributes, entryInfo.Comment, true, false);
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
            if (!bindings.UseBindings)
                return;

            var contents = new StringBuilder();
            buildData.Modules.TryGetValue(moduleInfo.Module, out var moduleBuildInfo);

            // Header
            contents.Append("#if ");
            foreach (var define in moduleBuildInfo.ScriptingAPI.Defines)
                contents.Append(define).Append(" && ");
            contents.Append("true").AppendLine();
            contents.AppendLine("// This code was auto-generated. Do not modify it.");
            contents.AppendLine();

            // Using declarations
            contents.AppendLine("using System;");
            contents.AppendLine("using System.ComponentModel;");
            contents.AppendLine("using System.Runtime.CompilerServices;");
            contents.AppendLine("using System.Runtime.InteropServices;");
            foreach (var e in moduleInfo.Children)
            {
                bool tmp = false;
                foreach (var apiTypeInfo in e.Children)
                {
                    if (apiTypeInfo.Namespace != "FlaxEngine")
                    {
                        tmp = true;
                        contents.AppendLine("using FlaxEngine;");
                        break;
                    }
                }
                if (tmp)
                    break;
            }
            // TODO: custom using declarations support
            // TODO: generate using declarations based on references modules (eg. using FlaxEngine, using Plugin1 in game API)

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

            // Save generated file
            contents.AppendLine("#endif");
            Utilities.WriteFileIfChanged(bindings.GeneratedCSharpFilePath, contents.ToString());

            // Ensure that generated file is included into build
            if (useBindings && moduleBuildInfo != null && !moduleBuildInfo.SourceFiles.Contains(bindings.GeneratedCSharpFilePath))
            {
                moduleBuildInfo.SourceFiles.Add(bindings.GeneratedCSharpFilePath);
            }
        }

        internal struct GuidInterop
        {
            public int A;
            public int B;
            public int C;
            public int D;
        }

        private static unsafe void GenerateCSharp(BuildData buildData, IGrouping<string, Module> binaryModule)
        {
            var contents = new StringBuilder();
            var binaryModuleName = binaryModule.Key;
            var project = Builder.GetModuleProject(binaryModule.First(), buildData);

            // Generate binary module ID
            GuidInterop idInterop;
            idInterop.A = binaryModuleName.GetHashCode();
            idInterop.B = project.Name.GetHashCode();
            idInterop.C = project.Company.GetHashCode();
            idInterop.D = project.Copyright.GetHashCode();
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

// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Modules.SourceCodeEditing
{
    /// <summary>
    /// Source code documentation module.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class CodeDocsModule : EditorModule
    {
        private Dictionary<ScriptType, string> _typeCache = new Dictionary<ScriptType, string>();
        private Dictionary<ScriptMemberInfo, string> _memberCache = new Dictionary<ScriptMemberInfo, string>();
        private Dictionary<Assembly, Dictionary<string, string>> _xmlCache = new Dictionary<Assembly, Dictionary<string, string>>();

        internal CodeDocsModule(Editor editor)
        : base(editor)
        {
        }

        /// <summary>
        /// Gets the tooltip text for the type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="attributes">The type attributes. Optional, if null type attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(Type type, object[] attributes = null)
        {
            return GetTooltip(new ScriptType(type), attributes);
        }

        /// <summary>
        /// Gets the tooltip text for the type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="attributes">The type attributes. Optional, if null type attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(ScriptType type, object[] attributes = null)
        {
            // Try to use cache
            if (_typeCache.TryGetValue(type, out var text))
                return text;

            // Try to use tooltip attribute
            if (attributes == null)
                attributes = type.GetAttributes(false);
            text = type.TypeName;
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text += '\n' + tooltip.Text;
            else if (type.Type != null)
            {
                // Try to use xml docs for managed type
                var xml = GetXmlDocs(type.Type.Assembly);
                if (xml != null)
                {
                    var key = "T:" + GetXmlKey(type.Type.FullName);
                    if (xml.TryGetValue(key, out var xmlDoc))
                        text += '\n' + FilterWhitespaces(xmlDoc);
                }
            }

            _typeCache.Add(type, text);
            return text;
        }

        /// <summary>
        /// Gets the tooltip text for the type member.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="memberName">The member name.</param>
        /// <param name="attributes">The member attributes. Optional, if null member attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(Type type, string memberName, object[] attributes = null)
        {
            var member = new ScriptType(type).GetMember(memberName, MemberTypes.All, BindingFlags.Public | BindingFlags.Static | BindingFlags.Instance);
            return GetTooltip(member, attributes);
        }

        /// <summary>
        /// Gets the tooltip text for the type member.
        /// </summary>
        /// <param name="member">The type member.</param>
        /// <param name="attributes">The member attributes. Optional, if null member attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(ScriptMemberInfo member, object[] attributes = null)
        {
            // Try to use cache
            if (_memberCache.TryGetValue(member, out var text))
                return text;

            // Try to use tooltip attribute
            if (attributes == null)
                attributes = member.GetAttributes(true);
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text = tooltip.Text;
            else if (member.Type != null)
            {
                // Try to use xml docs for managed member
                var memberInfo = member.Type;
                var xml = GetXmlDocs(memberInfo.DeclaringType.Assembly);
                if (xml != null)
                {
                    // [Reference: MSDN Magazine, October 2019, Volume 34 Number 10, "Accessing XML Documentation via Reflection"]
                    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection
                    var memberType = memberInfo.MemberType;
                    string key = null;
                    if (memberType.HasFlag(MemberTypes.Field))
                    {
                        var fieldInfo = (FieldInfo)memberInfo;
                        key = "F:" + GetXmlKey(fieldInfo.DeclaringType.FullName) + "." + fieldInfo.Name;
                    }
                    else if (memberType.HasFlag(MemberTypes.Property))
                    {
                        var propertyInfo = (PropertyInfo)memberInfo;
                        key = "P:" + GetXmlKey(propertyInfo.DeclaringType.FullName) + "." + propertyInfo.Name;
                    }
                    else if (memberType.HasFlag(MemberTypes.Event))
                    {
                        var eventInfo = (EventInfo)memberInfo;
                        key = "E:" + GetXmlKey(eventInfo.DeclaringType.FullName) + "." + eventInfo.Name;
                    }
                    else if (memberType.HasFlag(MemberTypes.Constructor))
                    {
                        var constructorInfo = (ConstructorInfo)memberInfo;
                        key = GetXmlKey(constructorInfo);
                    }
                    else if (memberType.HasFlag(MemberTypes.Method))
                    {
                        var methodInfo = (MethodInfo)memberInfo;
                        key = GetXmlKey(methodInfo);
                    }
                    else if (memberType.HasFlag(MemberTypes.TypeInfo) || memberType.HasFlag(MemberTypes.NestedType))
                    {
                        var typeInfo = (TypeInfo)memberInfo;
                        key = "T:" + GetXmlKey(typeInfo.FullName);
                    }
                    if (key != null)
                        xml.TryGetValue(key, out text);

                    // Customize tooltips for properties to be more human-readable in UI
                    if (text != null && memberType.HasFlag(MemberTypes.Property) && text.StartsWith("Gets or sets ", StringComparison.Ordinal))
                    {
                        text = text.Substring(13);
                        unsafe
                        {
                            fixed (char* e = text)
                                e[0] = char.ToUpper(e[0]);
                        }
                    }
                }
            }

            _memberCache.Add(member, text);
            return text;
        }

        // [Reference: MSDN Magazine, October 2019, Volume 34 Number 10, "Accessing XML Documentation via Reflection"]
        // https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection

        private string GetXmlKey(MethodInfo methodInfo)
        {
            var typeGenericMap = new Dictionary<string, int>();
            var methodGenericMap = new Dictionary<string, int>();
            ParameterInfo[] parameterInfos = methodInfo.GetParameters();

            if (methodInfo.DeclaringType.IsGenericType)
            {
                var methods = methodInfo.DeclaringType.GetGenericTypeDefinition().GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.Instance | BindingFlags.NonPublic);
                methodInfo = methods.First(x => x.MetadataToken == methodInfo.MetadataToken);
            }

            Type[] typeGenericArguments = methodInfo.DeclaringType.GetGenericArguments();
            for (int i = 0; i < typeGenericArguments.Length; i++)
            {
                Type typeGeneric = typeGenericArguments[i];
                typeGenericMap[typeGeneric.Name] = i;
            }

            Type[] methodGenericArguments = methodInfo.GetGenericArguments();
            for (int i = 0; i < methodGenericArguments.Length; i++)
            {
                Type methodGeneric = methodGenericArguments[i];
                methodGenericMap[methodGeneric.Name] = i;
            }

            string declarationTypeString = GetXmlKey(methodInfo.DeclaringType, false, typeGenericMap, methodGenericMap);
            string memberNameString = methodInfo.Name;
            string methodGenericArgumentsString = methodGenericMap.Count > 0 ? "``" + methodGenericMap.Count : string.Empty;
            string parametersString = parameterInfos.Length > 0 ? "(" + string.Join(",", methodInfo.GetParameters().Select(x => GetXmlKey(x.ParameterType, true, typeGenericMap, methodGenericMap))) + ")" : string.Empty;

            string key = "M:" + declarationTypeString + "." + memberNameString + methodGenericArgumentsString + parametersString;
            if (methodInfo.Name is "op_Implicit" || methodInfo.Name is "op_Explicit")
            {
                key += "~" + GetXmlKey(methodInfo.ReturnType, true, typeGenericMap, methodGenericMap);
            }
            return key;
        }

        private string GetXmlKey(ConstructorInfo constructorInfo)
        {
            var typeGenericMap = new Dictionary<string, int>();
            var methodGenericMap = new Dictionary<string, int>();
            ParameterInfo[] parameterInfos = constructorInfo.GetParameters();

            Type[] typeGenericArguments = constructorInfo.DeclaringType.GetGenericArguments();
            for (int i = 0; i < typeGenericArguments.Length; i++)
            {
                Type typeGeneric = typeGenericArguments[i];
                typeGenericMap[typeGeneric.Name] = i;
            }

            string declarationTypeString = GetXmlKey(constructorInfo.DeclaringType, false, typeGenericMap, methodGenericMap);
            string methodGenericArgumentsString = methodGenericMap.Count > 0 ? "``" + methodGenericMap.Count : string.Empty;
            string parametersString = parameterInfos.Length > 0 ? "(" + string.Join(",", constructorInfo.GetParameters().Select(x => GetXmlKey(x.ParameterType, true, typeGenericMap, methodGenericMap))) + ")" : string.Empty;

            return "M:" + declarationTypeString + "." + "#ctor" + methodGenericArgumentsString + parametersString;
        }

        internal static string GetXmlKey(Type type, bool isMethodParameter, Dictionary<string, int> typeGenericMap, Dictionary<string, int> methodGenericMap)
        {
            if (type.IsGenericParameter)
            {
                if (methodGenericMap.TryGetValue(type.Name, out var methodIndex))
                    return "``" + methodIndex;
                return "`" + typeGenericMap[type.Name];
            }
            if (type.HasElementType)
            {
                string elementTypeString = GetXmlKey(type.GetElementType(), isMethodParameter, typeGenericMap, methodGenericMap);
                if (type.IsPointer)
                    return elementTypeString + "*";
                if (type.IsByRef)
                    return elementTypeString + "@";
                if (type.IsArray)
                {
                    int rank = type.GetArrayRank();
                    string arrayDimensionsString = rank > 1 ? "[" + string.Join(",", Enumerable.Repeat("0:", rank)) + "]" : "[]";
                    return elementTypeString + arrayDimensionsString;
                }
                throw new Exception();
            }

            string prefaceString = type.IsNested ? GetXmlKey(type.DeclaringType, isMethodParameter, typeGenericMap, methodGenericMap) + "." : type.Namespace + ".";
            string typeNameString = isMethodParameter ? Regex.Replace(type.Name, @"`\d+", string.Empty) : type.Name;
            string genericArgumentsString = type.IsGenericType && isMethodParameter ? "{" + string.Join(",", type.GetGenericArguments().Select(argument => GetXmlKey(argument, true, typeGenericMap, methodGenericMap))) + "}" : string.Empty;
            return prefaceString + typeNameString + genericArgumentsString;
        }

        private static string GetXmlKey(string typeFullNameString)
        {
            return Regex.Replace(typeFullNameString, @"\[.*\]", string.Empty).Replace('+', '.');
        }

        private static string FilterWhitespaces(string str)
        {
            if (str.Contains("  ", StringComparison.Ordinal))
            {
                var sb = new StringBuilder();
                var prev = str[0];
                sb.Append(prev);
                for (int i = 1; i < str.Length; i++)
                {
                    var c = str[i];
                    if (prev != ' ' || c != ' ')
                    {
                        sb.Append(c);
                    }
                    prev = c;
                }
                str = sb.ToString();
            }
            return str;
        }

        private Dictionary<string, string> GetXmlDocs(Assembly assembly)
        {
            if (!_xmlCache.TryGetValue(assembly, out var result))
            {
                Profiler.BeginEvent("GetXmlDocs");

                var assemblyPath = Utils.GetAssemblyLocation(assembly);
                var assemblyName = assembly.GetName().Name;
                var xmlFilePath = Path.ChangeExtension(assemblyPath, ".xml");
                if (!File.Exists(assemblyPath) && !string.IsNullOrEmpty(assemblyPath))
                {
                    var uri = new UriBuilder(assemblyPath);
                    var path = Uri.UnescapeDataString(uri.Path);
                    xmlFilePath = Path.Combine(Path.GetDirectoryName(path), assemblyName + ".xml");
                }
                if (File.Exists(xmlFilePath))
                {
                    Profiler.BeginEvent(assemblyName);
                    try
                    {
                        // Parse xml documentation
                        using (var xmlReader = XmlReader.Create(new StreamReader(xmlFilePath)))
                        {
                            result = new Dictionary<string, string>();
                            while (xmlReader.Read())
                            {
                                if (xmlReader.NodeType == XmlNodeType.Element && string.Equals(xmlReader.Name, "member", StringComparison.Ordinal))
                                {
                                    string rawName = xmlReader["name"];
                                    var memberReader = xmlReader.ReadSubtree();
                                    if (memberReader.ReadToDescendant("summary"))
                                    {
                                        // Remove <see cref=""/> and replace them with the captured group (the content of the cref). Additionally, getting rid of prefixes
                                        const string crefPattern = @"<see\s+cref=""(?:[A-Z]:FlaxEngine\.)?([^""]+)""\s*\/>";
                                        result[rawName] = Regex.Replace(memberReader.ReadInnerXml(), crefPattern, "$1").Replace('\n', ' ').Trim();
                                    }
                                }
                            }
                        }
                    }
                    catch
                    {
                        // Ignore errors
                    }
                    Profiler.EndEvent();
                }

                _xmlCache[assembly] = result;
                Profiler.EndEvent();
            }
            return result;
        }

        private void OnTypesCleared()
        {
            _typeCache.Clear();
            _memberCache.Clear();
            _xmlCache.Clear();
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            base.OnInit();

            Editor.CodeEditing.TypesCleared += OnTypesCleared;
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            OnTypesCleared();

            base.OnExit();
        }
    }
}

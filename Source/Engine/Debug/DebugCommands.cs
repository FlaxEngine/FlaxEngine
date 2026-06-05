// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_EDITOR || !BUILD_RELEASE
#define WITH_HELP
#endif

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace FlaxEngine
{
    /// <summary>
    /// Marks static method as debug command that can be executed from the command line or via console.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method | AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class DebugCommand : Attribute
    {
    }

    partial class DebugCommands
    {
#if WITH_HELP
        private static Dictionary<Assembly, Dictionary<string, string>> _xmlCache;
        private static StringBuilder _sb;

        internal static void ClearXml()
        {
            if (_xmlCache == null)
                return;
            foreach (var asm in _xmlCache.Keys.ToArray())
            {
                if (asm.IsCollectible)
                    _xmlCache.Remove(asm);
            }
        }

        internal static string GetXmlInternal(Type type, string memberName)
        {
            // Redirect into type when no member specified
            if (string.IsNullOrEmpty(memberName))
                return GetXml(type);

            // Redirect property function getter/setter into owning property docs
            if (memberName.StartsWith("get_") || memberName.StartsWith("set_"))
                memberName = memberName.Substring(4);

            // Find member of that name
            var members = type.GetMember(memberName, BindingFlags.Static | BindingFlags.Public);
            if (members.Length == 0)
                return null;
            return GetXml(members[0]);
        }

        /// <summary>
        /// Gets the XML docs text for the type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The documentation help.</returns>
        public static string GetXml(Type type)
        {
            var xml = GetXmlDocs(type.Assembly);
            if (xml != null)
            {
                var key = "T:" + GetXmlKey(type.FullName);
                if (xml.TryGetValue(key, out var xmlDoc))
                    return FilterWhitespaces(xmlDoc);
            }
            return null;
        }

        /// <summary>
        /// Gets the XML docs text for the type member.
        /// </summary>
        /// <param name="member">The type member.</param>
        /// <returns>The documentation help.</returns>
        public static string GetXml(MemberInfo member)
        {
            string text = null;
            var xml = GetXmlDocs(member.DeclaringType.Assembly);
            if (xml != null)
            {
                // [Reference: MSDN Magazine, October 2019, Volume 34 Number 10, "Accessing XML Documentation via Reflection"]
                // https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection
                var memberType = member.MemberType;
                string key = null;
                if (memberType.HasFlag(MemberTypes.Field))
                {
                    var fieldInfo = (FieldInfo)member;
                    key = "F:" + GetXmlKey(fieldInfo.DeclaringType.FullName) + "." + fieldInfo.Name;
                }
                else if (memberType.HasFlag(MemberTypes.Property))
                {
                    var propertyInfo = (PropertyInfo)member;
                    key = "P:" + GetXmlKey(propertyInfo.DeclaringType.FullName) + "." + propertyInfo.Name;
                }
                else if (memberType.HasFlag(MemberTypes.Event))
                {
                    var eventInfo = (EventInfo)member;
                    key = "E:" + GetXmlKey(eventInfo.DeclaringType.FullName) + "." + eventInfo.Name;
                }
                else if (memberType.HasFlag(MemberTypes.Constructor))
                {
                    var constructorInfo = (ConstructorInfo)member;
                    key = GetXmlKey(constructorInfo);
                }
                else if (memberType.HasFlag(MemberTypes.Method))
                {
                    var methodInfo = (MethodInfo)member;
                    key = GetXmlKey(methodInfo);
                }
                else if (memberType.HasFlag(MemberTypes.TypeInfo) || memberType.HasFlag(MemberTypes.NestedType))
                {
                    var typeInfo = (TypeInfo)member;
                    key = "T:" + GetXmlKey(typeInfo.FullName);
                }
                if (key != null)
                {
                    xml.TryGetValue(key, out text);
                    text = FilterWhitespaces(text);
                }

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
            return text;
        }

        private static string FilterWhitespaces(string str)
        {
            if (str != null && str.Contains("  ", StringComparison.Ordinal))
            {
                if (_sb == null)
                    _sb = new StringBuilder();
                else
                    _sb.Clear();
                var sb = _sb;
                var prev = str[0];
                sb.Append(prev);
                for (int i = 1; i < str.Length; i++)
                {
                    var c = str[i];
                    if (prev != ' ' || c != ' ')
                        sb.Append(c);
                    prev = c;
                }
                str = sb.ToString();
            }
            return str;
        }

        // [Reference: MSDN Magazine, October 2019, Volume 34 Number 10, "Accessing XML Documentation via Reflection"]
        // https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection

        private static string GetXmlKey(MethodInfo methodInfo)
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

        private static string GetXmlKey(ConstructorInfo constructorInfo)
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
                if (typeGenericMap.TryGetValue(type.Name, out var typeKey))
                    return "`" + typeKey;
                return "`";
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

        private static Dictionary<string, string> GetXmlDocs(Assembly assembly)
        {
            if (_xmlCache == null)
                _xmlCache = new Dictionary<Assembly, Dictionary<string, string>>();
            if (!_xmlCache.TryGetValue(assembly, out var result))
            {
                Profiler.BeginEvent("GetXmlDocs");

                // Find XML file path (based on assembly location)
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
                            StringBuilder content = new StringBuilder(2048);
                            while (xmlReader.Read())
                            {
                                if (xmlReader.NodeType == XmlNodeType.Element && string.Equals(xmlReader.Name, "member", StringComparison.Ordinal))
                                {
                                    string rawName = xmlReader["name"];
                                    var memberReader = xmlReader.ReadSubtree();
                                    if (memberReader.ReadToDescendant("summary"))
                                    {
                                        content.Clear();
                                        do
                                        {
                                            if (memberReader.NodeType == XmlNodeType.Element && memberReader.Read())
                                            {
                                                while (memberReader.NodeType == XmlNodeType.Text)
                                                {
                                                    content.Append(memberReader.Value);
                                                    if (memberReader.Read() && memberReader.NodeType == XmlNodeType.Element)
                                                    {
                                                        var nodeRef = TrimRef(memberReader.GetAttribute("cref")); // <see cref=""/>
                                                        if (nodeRef == null)
                                                            nodeRef = memberReader.GetAttribute("name"); // <paramref name=""/>
                                                        content.Append(nodeRef);
                                                        memberReader.Read();

                                                        string TrimRef(string str)
                                                        {
                                                            if (str == null)
                                                                return null;
                                                            if (str.IndexOf(":FlaxEngine.") == 1)
                                                                return str.Substring("T:FlaxEngine.".Length);
                                                            return str.Substring("T:".Length);
                                                        }
                                                    }
                                                }
                                            }

                                            if (memberReader.NodeType == XmlNodeType.EndElement && memberReader.Name == "summary")
                                                break;
                                        } while (memberReader.Read());

                                        result[rawName] = content.ToString().Trim(' ', '\r', '\n');
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
#endif
    }
}

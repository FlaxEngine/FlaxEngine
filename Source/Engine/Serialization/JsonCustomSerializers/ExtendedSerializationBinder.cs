// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using System.Runtime.Serialization;
using FlaxEngine.Interop;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

#nullable enable

namespace FlaxEngine.Json.JsonCustomSerializers
{
    internal class ExtendedSerializationBinder : SerializationBinder, ISerializationBinder
    {
        private record struct TypeKey(string? assemblyName, string typeName);

        private ConcurrentDictionary<TypeKey, Type> _cache;
        private Func<TypeKey, Type> _resolve;

        /// <summary>Clear the cache</summary>
        /// <remarks>Should be cleared on scripting domain reload to avoid out of date types participating in dynamic type resolution</remarks>
        public void ResetCache()
        {
            _cache.Clear();
        }

        public override Type BindToType(string? assemblyName, string typeName)
        {
            return FindCachedType(new(assemblyName, typeName));
        }

        public override void BindToName(Type serializedType, out string? assemblyName, out string? typeName)
        {
            assemblyName = serializedType.Assembly.FullName;
            typeName = serializedType.FullName;
        }


        public ExtendedSerializationBinder()
        {
            _resolve = ResolveBind;
            _cache = new();
        }

        Type ResolveBind(TypeKey key)
        {
            Type? type = null;
            if (key.assemblyName is null) { // No assembly name, attempt to find globally
                type = FindTypeGlobal(key.typeName);
            }

            if (type is null && key.assemblyName is not null) { // Type not found yet, but we have assembly name
                Assembly? assembly = null;

                assembly = FindScriptingAssembly(key.assemblyName); // Attempt to load from scripting assembly

                if (assembly is null)
                    assembly = FindLoadedAssembly(key.assemblyName); // Attempt to load from loaded assemblies

                if (assembly is null)
                    assembly = FindUnloadedAssembly(key.assemblyName); // Attempt to load from unloaded assemblies

                if (assembly is null)
                    throw MakeAsmResolutionException(key.assemblyName); // Assembly failed to resolve

                type = FindTypeInAssembly(key.typeName, assembly); // We have assembly, attempt to load from assembly
            }

            //if (type is null)
            //    type = _fallBack.BindToType(key.assemblyName, key.typeName); // Use fallback

            if (type is null)
                throw MakeTypeResolutionException(key.assemblyName, key.typeName);

            return type;
        }

        /// <summary>Attempt to find the assembly among loaded scripting assemblies</summary>
        Assembly? FindScriptingAssembly(string assemblyName)
        {
            return NativeInterop.ResolveScriptingAssemblyByName(new AssemblyName(assemblyName), allowPartial: true);
        }

        /// <summary>Attempt to find the assembly by name</summary>
        Assembly? FindLoadedAssembly(string assemblyName) // TODO
        {
            return null;
        }

        /// <summary>Attempt to find the assembly by name</summary>
        Assembly? FindUnloadedAssembly(string assemblyName)
        {
            Assembly? assembly = null;

            assembly = Assembly.Load(new AssemblyName(assemblyName));

            if (assembly is null)
                assembly = Assembly.LoadWithPartialName(assemblyName); // Copying behavior of DefaultSerializationBinder


            return assembly;
        }



        Type? FindTypeInAssembly(string typeName, Assembly assembly)
        {
            var type = assembly.GetType(typeName); // Attempt to load directly

            if (type is null && typeName.IndexOf('`') >= 0) // Attempt failed, but name has generic variant tick, try resolving generic manually
                type = FindTypeGeneric(typeName, assembly);

            return type;
        }

        /// <summary>Attempt to find unqualified type by only name</summary>
        Type? FindTypeGlobal(string typeName)
        {
            return Type.GetType(typeName);
        }

        /// <summary>Get type from the cache</summary>
        private Type FindCachedType(TypeKey key)
        {
            return _cache.GetOrAdd(key, _resolve);
        }



        /*********************************************
         ** Below code is adapted from Newtonsoft.Json
         *********************************************/

        /// <summary>Attempt to recursively resolve a generic type</summary>
        private Type? FindTypeGeneric(string typeName, Assembly assembly)
        {
            Type? type = null;
            int openBracketIndex = typeName.IndexOf('[', StringComparison.Ordinal);
            if (openBracketIndex >= 0) {
                string genericTypeDefName = typeName.Substring(0, openBracketIndex); // Find the unspecialized type
                Type? genericTypeDef = assembly.GetType(genericTypeDefName);
                if (genericTypeDef != null) {
                    List<Type> genericTypeArguments = new List<Type>(); // Recursively resolve the arguments
                    int scope = 0;
                    int typeArgStartIndex = 0;
                    int endIndex = typeName.Length - 1;
                    for (int i = openBracketIndex + 1; i < endIndex; ++i) {
                        char current = typeName[i];
                        switch (current) {
                        case '[':
                            if (scope == 0) {
                                typeArgStartIndex = i + 1;
                            }
                            ++scope;
                            break;
                        case ']':
                            --scope;
                            if (scope == 0) { // All arguments resolved, compose our type
                                string typeArgAssemblyQualifiedName = typeName.Substring(typeArgStartIndex, i - typeArgStartIndex);

                                TypeKey typeNameKey = SplitFullyQualifiedTypeName(typeArgAssemblyQualifiedName);
                                genericTypeArguments.Add(FindCachedType(typeNameKey));
                            }
                            break;
                        }
                    }

                    type = genericTypeDef.MakeGenericType(genericTypeArguments.ToArray());
                }
            }

            return type;
        }

        /// <summary>Split a fully qualified type name into assembly name, and type name</summary>
        private static TypeKey SplitFullyQualifiedTypeName(string fullyQualifiedTypeName)
        {
            int? assemblyDelimiterIndex = GetAssemblyDelimiterIndex(fullyQualifiedTypeName);

            ReadOnlySpan<char> typeName;
            ReadOnlySpan<char> assemblyName;

            if (assemblyDelimiterIndex != null) {
                typeName = fullyQualifiedTypeName.AsSpan().Slice(0, assemblyDelimiterIndex ?? 0);
                assemblyName = fullyQualifiedTypeName.AsSpan().Slice((assemblyDelimiterIndex ?? 0) + 1);
            } else {
                typeName = fullyQualifiedTypeName;
                assemblyName = null;
            }
            
            return new(new(assemblyName), new(typeName));
        }

        /// <summary>Find the assembly name inside a fully qualified type name</summary>
        private static int? GetAssemblyDelimiterIndex(string fullyQualifiedTypeName)
        {
            // we need to get the first comma following all surrounded in brackets because of generic types
            // e.g. System.Collections.Generic.Dictionary`2[[System.String, mscorlib,Version=2.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089],[System.String, mscorlib, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089]], mscorlib, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089
            int scope = 0;
            for (int i = 0; i < fullyQualifiedTypeName.Length; i++) {
                char current = fullyQualifiedTypeName[i];
                switch (current) {
                case '[':
                    scope++;
                    break;
                case ']':
                    scope--;
                    break;
                case ',':
                    if (scope == 0) {
                        return i;
                    }
                    break;
                }
            }

            return null;
        }


        private static JsonSerializationException MakeAsmResolutionException(string asmName)
        {
            return new($"Could not load assembly '{asmName}'.");
        }
        
        private static JsonSerializationException MakeTypeResolutionException(string? asmName, string typeName)
        {
            if (asmName is null)
                return new($"Could not find '{typeName}'");
            else
                return new($"Could not find '{typeName}' in assembly '{asmName}'.");
        }
    }
}

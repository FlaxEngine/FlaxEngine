// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Mono.Cecil;
using CustomAttributeNamedArgument = Mono.Cecil.CustomAttributeNamedArgument;
using ICustomAttributeProvider = Mono.Cecil.ICustomAttributeProvider;

namespace Flax.Build
{
    /// <summary>
    /// The utilities for Mono.Cecil library usage.
    /// </summary>
    internal static class MonoCecil
    {
        public sealed class BasicAssemblyResolver : IAssemblyResolver
        {
            private readonly Dictionary<string, AssemblyDefinition> _cache = new();

            public HashSet<string> SearchDirectories = new();

            public AssemblyDefinition Resolve(string path)
            {
                var name = Path.GetFileNameWithoutExtension(path);
                foreach (var e in _cache)
                {
                    if (string.Equals(e.Value.Name.Name, name, StringComparison.OrdinalIgnoreCase))
                        return e.Value;
                }
                var assembly = ModuleDefinition.ReadModule(path, new ReaderParameters()).Assembly;
                _cache[assembly.FullName] = assembly;
                return assembly;
            }

            public AssemblyDefinition Resolve(AssemblyNameReference name)
            {
                return Resolve(name, new ReaderParameters());
            }

            public AssemblyDefinition Resolve(AssemblyNameReference name, ReaderParameters parameters)
            {
                if (_cache.TryGetValue(name.FullName, out var assembly))
                    return assembly;

                if (parameters.AssemblyResolver == null)
                    parameters.AssemblyResolver = this;
                foreach (var searchDirectory in SearchDirectories)
                {
                    if (TryLoad(name, parameters, searchDirectory, out assembly))
                        return assembly;
                }

                throw new AssemblyResolutionException(name);
            }

            public void Dispose()
            {
                foreach (var assembly in _cache.Values)
                    assembly.Dispose();
                _cache.Clear();
            }

            private bool TryLoad(AssemblyNameReference name, ReaderParameters parameters, string directory, out AssemblyDefinition assembly)
            {
                assembly = null;

                var file = Path.Combine(directory, name.Name + ".dll");
                if (!File.Exists(file))
                    return false;

                try
                {
                    assembly = ModuleDefinition.ReadModule(file, parameters).Assembly;
                }
                catch (BadImageFormatException)
                {
                    return false;
                }

                _cache[name.FullName] = assembly;
                return true;
            }
        }

        public static void CompilationError(string message)
        {
            Log.Error(message);
        }

        public static void CompilationError(string message, MethodDefinition method)
        {
            if (method != null && method.DebugInformation.HasSequencePoints)
            {
                var sp = method.DebugInformation.SequencePoints[0];
                message = $"{sp.Document.Url}({sp.StartLine},{sp.StartColumn},{sp.EndLine},{sp.EndColumn}): error: {message}";
            }
            Log.Error(message);
        }

        public static void CompilationError(string message, PropertyDefinition property)
        {
            if (property != null)
            {
                if (property.GetMethod != null)
                {
                    CompilationError(message, property.GetMethod);
                    return;
                }
                else if (property.SetMethod != null)
                {
                    CompilationError(message, property.SetMethod);
                    return;
                }
            }
            Log.Error(message);
        }

        public static void CompilationError(string message, FieldDefinition field)
        {
            if (field != null && field.DeclaringType != null)
            {
                // Just include potential filename
                var methods = field.DeclaringType.Methods;
                if (methods != null && methods.Count != 0)
                {
                    var method = methods[0];
                    if (method != null && method.DebugInformation.HasSequencePoints)
                    {
                        var sp = method.DebugInformation.SequencePoints[0];
                        message = $"{sp.Document.Url}({0},{0},{0},{0}): error: {message}";
                    }
                }
            }
            Log.Error(message);
        }

        public static bool HasAttribute(this ICustomAttributeProvider type, string fullName)
        {
            return type.CustomAttributes.Any(x => x.AttributeType.FullName == fullName);
        }

        public static bool HasInterface(this TypeDefinition type, string fullName)
        {
            return type.Interfaces.Any(x => x.InterfaceType.FullName == fullName);
        }

        public static bool HasMethod(this TypeDefinition type, string name)
        {
            return type.Methods.Any(x => x.Name == name);
        }

        public static MethodDefinition GetMethod(this TypeDefinition type, string name)
        {
            var result = type.Methods.FirstOrDefault(x => x.Name == name);
            if (result == null)
                throw new Exception($"Failed to find method '{name}' in '{type.FullName}'.");
            return result;
        }

        public static MethodDefinition GetMethod(this TypeDefinition type, string name, int argCount)
        {
            var result = type.Methods.FirstOrDefault(x => x.Name == name && x.Parameters.Count == argCount);
            if (result == null)
                throw new Exception($"Failed to find method '{name}' (args={argCount}) in '{type.FullName}'.");
            return result;
        }

        public static FieldDefinition GetField(this TypeDefinition type, string name)
        {
            var result = type.Fields.FirstOrDefault(x => x.Name == name);
            if (result == null)
                throw new Exception($"Failed to find field '{name}' in '{type.FullName}'.");
            return result;
        }

        public static CustomAttributeNamedArgument GetField(this CustomAttribute attribute, string name)
        {
            foreach (var f in attribute.Fields)
            {
                if (f.Name == name)
                    return f;
            }
            throw new Exception($"Failed to find field '{name}' in '{attribute.AttributeType.FullName}'.");
        }

        public static object GetFieldValue(this CustomAttribute attribute, string name, object defaultValue)
        {
            foreach (var f in attribute.Fields)
            {
                if (f.Name == name)
                    return f.Argument.Value;
            }
            return defaultValue;
        }

        public static bool IsScriptingObject(this TypeDefinition type)
        {
            if (type.FullName == "FlaxEngine.Object")
                return true;
            return type.BaseType.IsScriptingObject();
        }

        public static bool IsScriptingObject(this TypeReference type)
        {
            if (type == null)
                return false;
            if (type.FullName == "FlaxEngine.Object")
                return true;
            try
            {
                return type.Resolve().IsScriptingObject();
            }
            catch
            {
                return false;
            }
        }

        public static bool CanBeResolved(this TypeReference type)
        {
            while (type != null)
            {
                if (type.Scope.Name == "Windows")
                    return false;
                if (type.Scope.Name == "mscorlib")
                    return type.Resolve() != null;

                try
                {
                    type = type.Resolve().BaseType;
                }
                catch
                {
                    return false;
                }
            }
            return true;
        }

        public static MethodReference InflateGeneric(this MethodReference generic, TypeReference T)
        {
            var instance = new GenericInstanceMethod(generic);
            instance.GenericArguments.Add(T);
            return instance;
        }

        public static void GetType(this ModuleDefinition module, string fullName, out TypeReference type)
        {
            //if (!module.TryGetTypeReference(fullName, out type)) // TODO: this seams to return 'FlaxEngine.Networking.NetworkManagerMode' as a Class instead of Enum
            {
                // Do manual search
                foreach (var a in module.AssemblyReferences)
                {
                    var assembly = module.AssemblyResolver.Resolve(a);
                    if (assembly == null || assembly.MainModule == null)
                        continue;
                    foreach (var t in assembly.MainModule.Types)
                    {
                        if (t.FindTypeWithin(fullName, out type))
                        {
                            module.ImportReference(type);
                            return;
                        }
                    }
                }

                throw new Exception($"Failed to find type {fullName} from module {module.FileName}.");
            }
        }

        private static bool FindTypeWithin(this TypeDefinition t, string fullName, out TypeReference type)
        {
            if (t.FullName == fullName)
            {
                type = t;
                return true;
            }
            foreach (var n in t.NestedTypes)
            {
                if (n.FindTypeWithin(fullName, out type))
                    return true;
            }
            type = null;
            return false;
        }
    }
}

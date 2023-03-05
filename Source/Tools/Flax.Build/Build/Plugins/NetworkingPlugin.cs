// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Text;
using System.Linq;
using System.IO;
using System.Collections.Generic;
using Flax.Build.Graph;
using Flax.Build.Bindings;
using Mono.Cecil;
using Mono.Cecil.Cil;

namespace Flax.Build.Plugins
{
    /// <summary>
    /// Flax.Build plugin for Networking extenrions support. Generates required bindings glue code for automatic types replication and RPCs invoking. 
    /// </summary>
    /// <seealso cref="Flax.Build.Plugin" />
    internal sealed class NetworkingPlugin : Plugin
    {
        private struct InBuildSerializer
        {
            public string WriteMethod;
            public string ReadMethod;

            public InBuildSerializer(string write, string read)
            {
                WriteMethod = write;
                ReadMethod = read;
            }
        }

        private struct TypeSerializer
        {
            public TypeDefinition Type;
            public MethodDefinition Serialize;
            public MethodDefinition Deserialize;
        }

        private struct MethodRPC
        {
            public TypeDefinition Type;
            public MethodDefinition Method;
            public bool IsServer;
            public bool IsClient;
            public int Channel;
            public MethodDefinition Execute;
        }

        internal const string Network = "Network";
        internal const string NetworkReplicated = "NetworkReplicated";
        internal const string NetworkReplicatedAttribute = "FlaxEngine.NetworkReplicatedAttribute";
        internal const string NetworkRpc = "NetworkRpc";
        private const string Thunk1 = "INetworkSerializable_Serialize";
        private const string Thunk2 = "INetworkSerializable_Deserialize";
        private static readonly Dictionary<string, InBuildSerializer> _inBuildSerializers = new Dictionary<string, InBuildSerializer>()
        {
            { "System.Boolean", new InBuildSerializer("WriteBoolean", "ReadBoolean") },
            { "System.Single", new InBuildSerializer("WriteSingle", "ReadSingle") },
            { "System.Double", new InBuildSerializer("WriteDouble", "ReadDouble") },
            { "System.Int64", new InBuildSerializer("WriteInt64", "ReadInt64") },
            { "System.Int32", new InBuildSerializer("WriteInt32", "ReadInt32") },
            { "System.Int16", new InBuildSerializer("WriteInt16", "ReadInt16") },
            { "System.SByte", new InBuildSerializer("WriteSByte", "ReadSByte") },
            { "System.UInt64", new InBuildSerializer("WriteUInt64", "ReadUInt64") },
            { "System.UInt32", new InBuildSerializer("WriteUInt32", "ReadUInt32") },
            { "System.UInt16", new InBuildSerializer("WriteUInt16", "ReadUInt16") },
            { "System.Byte", new InBuildSerializer("WriteByte", "ReadByte") },
            { "System.String", new InBuildSerializer("WriteString", "ReadString") },
            { "System.Guid", new InBuildSerializer("WriteGuid", "ReadGuid") },
            { "FlaxEngine.Vector2", new InBuildSerializer("WriteVector2", "ReadVector2") },
            { "FlaxEngine.Vector3", new InBuildSerializer("WriteVector3", "ReadVector3") },
            { "FlaxEngine.Vector4", new InBuildSerializer("WriteVector4", "ReadVector4") },
            { "FlaxEngine.Float2", new InBuildSerializer("WriteFloat2", "ReadFloat2") },
            { "FlaxEngine.Float3", new InBuildSerializer("WriteFloat3", "ReadFloat3") },
            { "FlaxEngine.Float4", new InBuildSerializer("WriteFloat4", "ReadFloat4") },
            { "FlaxEngine.Quaternion", new InBuildSerializer("WriteQuaternion", "ReadQuaternion") },
            { "FlaxEngine.Ray", new InBuildSerializer("WriteRay", "ReadRay") },
        };

        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            BindingsGenerator.ParseMemberTag += OnParseMemberTag;
            BindingsGenerator.GenerateCppTypeInternals += OnGenerateCppTypeInternals;
            BindingsGenerator.GenerateCppTypeInitRuntime += OnGenerateCppTypeInitRuntime;
            BindingsGenerator.GenerateCSharpTypeInternals += OnGenerateCSharpTypeInternals;
            Builder.BuildDotNetAssembly += OnBuildDotNetAssembly;
        }

        private void OnParseMemberTag(ref bool valid, BindingsGenerator.TagParameter tag, MemberInfo memberInfo)
        {
            if (string.Equals(tag.Tag, NetworkReplicated, StringComparison.OrdinalIgnoreCase))
            {
                // Mark member as replicated
                valid = true;
                memberInfo.SetTag(NetworkReplicated, string.Empty);
            }
            else if (string.Equals(tag.Tag, NetworkRpc, StringComparison.OrdinalIgnoreCase))
            {
                // Mark member as rpc
                valid = true;
                memberInfo.SetTag(NetworkRpc, tag.Value);
            }
        }

        private void OnGenerateCppTypeInternals(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents)
        {
            // Skip modules that don't use networking
            var module = BindingsGenerator.CurrentModule;
            if (module.GetTag(Network) == null)
                return;

            // Check if type uses automated network replication/RPCs
            List<FieldInfo> fields = null;
            List<PropertyInfo> properties = null;
            List<FunctionInfo> functions = null;
            if (typeInfo is ClassInfo classInfo)
            {
                fields = classInfo.Fields;
                properties = classInfo.Properties;
                functions = classInfo.Functions;
            }
            else if (typeInfo is StructureInfo structInfo)
            {
                fields = structInfo.Fields;
            }
            bool useReplication = false, useRpc = false;
            if (fields != null)
            {
                foreach (var fieldInfo in fields)
                {
                    if (fieldInfo.GetTag(NetworkReplicated) != null)
                    {
                        useReplication = true;
                        break;
                    }
                }
            }
            if (properties != null)
            {
                foreach (var propertyInfo in properties)
                {
                    if (propertyInfo.GetTag(NetworkReplicated) != null)
                    {
                        useReplication = true;
                        break;
                    }
                }
            }
            if (functions != null)
            {
                foreach (var functionInfo in functions)
                {
                    if (functionInfo.GetTag(NetworkRpc) != null)
                    {
                        useRpc = true;
                        break;
                    }
                }
            }
            if (useReplication)
            {
                typeInfo.SetTag(NetworkReplicated, string.Empty);
            
                // Generate C++ wrapper functions to serialize/deserialize type
                BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkReplicator.h");
                BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkStream.h");
                OnGenerateCppTypeSerialize(buildData, typeInfo, contents, fields, properties, true);
                OnGenerateCppTypeSerialize(buildData, typeInfo, contents, fields, properties, false);
            }
            if (useRpc)
            {
                typeInfo.SetTag(NetworkRpc, string.Empty);
            
                // Generate C++ wrapper functions to invoke/execute RPC
                BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkStream.h");
                BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkReplicator.h");
                BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkChannelType.h");
                BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkRpc.h");
                foreach (var functionInfo in functions)
                {
                    var tag = functionInfo.GetTag(NetworkRpc);
                    if (tag == null)
                        continue;
                    if (functionInfo.UniqueName != functionInfo.Name)
                        throw new Exception($"Invalid network RPC method {functionInfo.Name} name in type {typeInfo.Name}. Network RPC functions names have to be unique.");
                    bool isServer = tag.IndexOf("Server", StringComparison.OrdinalIgnoreCase) != -1;
                    bool isClient = tag.IndexOf("Client", StringComparison.OrdinalIgnoreCase) != -1;
                    if (isServer && isClient)
                        throw new Exception($"Network RPC {functionInfo.Name} in {typeInfo.Name} cannot be both Server and Client.");
                    if (!isServer && !isClient)
                        throw new Exception($"Network RPC {functionInfo.Name} in {typeInfo.Name} needs to have Server or Client specifier.");
                    var channelType = "ReliableOrdered";
                    if (tag.IndexOf("UnreliableOrdered", StringComparison.OrdinalIgnoreCase) != -1)
                        channelType = "UnreliableOrdered";
                    else if (tag.IndexOf("ReliableOrdered", StringComparison.OrdinalIgnoreCase) != -1)
                        channelType = "ReliableOrdered";
                    else if (tag.IndexOf("Unreliable", StringComparison.OrdinalIgnoreCase) != -1)
                        channelType = "Unreliable";
                    else if (tag.IndexOf("Reliable", StringComparison.OrdinalIgnoreCase) != -1)
                        channelType = "Reliable";

                    // Generated method thunk to execute RPC from network
                    {
                        contents.Append("    static void ").Append(functionInfo.Name).AppendLine("_Execute(ScriptingObject* obj, NetworkStream* stream, void* tag)");
                        contents.AppendLine("    {");
                        string argNames = string.Empty;
                        for (int i = 0; i < functionInfo.Parameters.Count; i++)
                        {
                            var arg = functionInfo.Parameters[i];
                            if (i != 0)
                                argNames += ", ";
                            argNames += arg.Name;

                            // Deserialize arguments
                            contents.AppendLine($"        {arg.Type.Type} {arg.Name};");
                            contents.AppendLine($"        stream->Read({arg.Name});");
                        }

                        // Call method locally
                        contents.AppendLine($"        ASSERT(obj && obj->Is<{typeInfo.NativeName}>());");
                        contents.AppendLine($"        (({typeInfo.NativeName}*)obj)->{functionInfo.Name}({argNames});");
                        contents.AppendLine("    }");
                    }
                    contents.AppendLine();

                    // Generated method thunk to invoke RPC to network
                    {
                        contents.Append("    static void ").Append(functionInfo.Name).AppendLine("_Invoke(ScriptingObject* obj, void** args)");
                        contents.AppendLine("    {");
                        contents.AppendLine("        NetworkStream* stream = NetworkReplicator::BeginInvokeRPC();");
                        for (int i = 0; i < functionInfo.Parameters.Count; i++)
                        {
                            var arg = functionInfo.Parameters[i];

                            // Serialize arguments
                            contents.AppendLine($"        stream->Write(*({arg.Type.Type}*)args[{i}]);");
                        }

                        // Invoke RPC
                        contents.AppendLine($"        NetworkReplicator::EndInvokeRPC(obj, {typeInfo.NativeName}::TypeInitializer, StringAnsiView(\"{functionInfo.Name}\", {functionInfo.Name.Length}), stream);");
                        contents.AppendLine("    }");
                    }
                    contents.AppendLine();

                    // Generated info about RPC implementation
                    {
                        contents.Append("    static NetworkRpcInfo ").Append(functionInfo.Name).AppendLine("_Info()");
                        contents.AppendLine("    {");
                        contents.AppendLine("        NetworkRpcInfo info;");
                        contents.AppendLine($"        info.Server = {(isServer ? "1" : "0")};");
                        contents.AppendLine($"        info.Client = {(isClient ? "1" : "0")};");
                        contents.AppendLine($"        info.Execute = {functionInfo.Name}_Execute;");
                        contents.AppendLine($"        info.Invoke = {functionInfo.Name}_Invoke;");
                        contents.AppendLine($"        info.Channel = (uint8)NetworkChannelType::{channelType};");
                        contents.AppendLine($"        info.Tag = nullptr;");
                        contents.AppendLine("        return info;");
                        contents.AppendLine("    }");
                    }
                    contents.AppendLine();
                }
            }
        }

        private void OnGenerateCppTypeSerialize(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents, List<FieldInfo> fields, List<PropertyInfo> properties, bool serialize)
        {
            contents.Append("    static void ").Append(serialize ? Thunk1 : Thunk2).AppendLine("(void* instance, NetworkStream* stream, void* tag)");
            contents.AppendLine("    {");
            contents.AppendLine($"        {typeInfo.NativeName}& obj = *({typeInfo.NativeName}*)instance;");
            if (IsRawPOD(buildData, typeInfo))
            {
                // POD types as raw bytes
                OnGenerateCppWriteRaw(contents, "obj", serialize);
            }
            else
            {
                if (typeInfo is ClassStructInfo classStructInfo && classStructInfo.BaseType != null)
                {
                    // Replicate base type
                    OnGenerateCppWriteSerializer(contents, classStructInfo.BaseType.NativeName, "obj", serialize);
                }
                
                // Replicate all marked fields and properties
                if (fields != null)
                {
                    foreach (var fieldInfo in fields)
                    {
                        if (fieldInfo.GetTag(NetworkReplicated) == null)
                            continue;
                        OnGenerateCppTypeSerializeData(buildData, typeInfo, contents, fieldInfo.Type, $"obj.{fieldInfo.Name}", serialize);
                    }
                }
                if (properties != null)
                {
                    foreach (var propertyInfo in properties)
                    {
                        if (propertyInfo.GetTag(NetworkReplicated) == null)
                            continue;
                        if (!serialize)
                            contents.AppendLine($"        {{{propertyInfo.Setter.Parameters[0].Type} value{propertyInfo.Name};");
                        var name = serialize ? $"obj.{propertyInfo.Getter.Name}()" : $"value{propertyInfo.Name}";
                        OnGenerateCppTypeSerializeData(buildData, typeInfo, contents, propertyInfo.Type, name, serialize);
                        if (!serialize)
                            contents.AppendLine($"        obj.{propertyInfo.Setter.Name}(value{propertyInfo.Name});}}");
                    }
                }
            }
            contents.AppendLine("    }");
            contents.AppendLine();
        }

        private bool IsRawPOD(Builder.BuildData buildData, ApiTypeInfo type)
        {
            // TODO: what if type fields have custom replication settings (eg. compression)?
            type.EnsureInited(buildData);
            return type.IsPod;
        }

        private bool IsRawPOD(Builder.BuildData buildData, ApiTypeInfo caller, ApiTypeInfo apiType, TypeInfo type)
        {
            if (type.IsPod(buildData, caller))
            {
                if (apiType != null)
                    return IsRawPOD(buildData, apiType);
            }
            return false;
        }
        
        private void OnGenerateCppTypeSerializeData(Builder.BuildData buildData, ApiTypeInfo caller, StringBuilder contents, TypeInfo type, string name, bool serialize)
        {
            var apiType = BindingsGenerator.FindApiTypeInfo(buildData, type, caller);
            if (apiType == null || IsRawPOD(buildData, caller, apiType, type))
            {
                // POD types as raw bytes
                if (type.IsPtr)
                    throw new Exception($"Invalid pointer type '{type}' that cannot be serialized for replication of {caller.Name}.");
                if (type.IsRef)
                    throw new Exception($"Invalid reference type '{type}' that cannot be serialized for replication of {caller.Name}.");
                OnGenerateCppWriteRaw(contents, name, serialize);
            }
            else if (apiType.IsScriptingObject)
            {
                // Object ID
                if (serialize)
                {
                    contents.AppendLine($"        {{Guid id = {name} ? {name}->GetID() : Guid::Empty;");
                    contents.AppendLine($"        const auto idsMapping = Scripting::ObjectsLookupIdMapping.Get();");
                    contents.AppendLine($"        if (idsMapping) idsMapping->KeyOf(id, &id);"); // Perform inverse mapping from clientId into serverId (NetworkReplicator binds ObjectsLookupIdMapping table)
                    OnGenerateCppWriteRaw(contents, "id", serialize);
                    contents.AppendLine("        }");
                }
                else
                {
                    contents.AppendLine($"        {{Guid id;");
                    OnGenerateCppWriteRaw(contents, "id", serialize);
                    contents.AppendLine($"        {name} = ({type})Scripting::TryFindObject(id);}}");
                }
            }
            else if (apiType.IsStruct)
            {
                if (type.IsPtr)
                    throw new Exception($"Invalid pointer type '{type}' that cannot be serialized for replication of {caller.Name}.");
                if (type.IsRef)
                    throw new Exception($"Invalid reference type '{type}' that cannot be serialized for replication of {caller.Name}.");
                
                // Structure serializer
                OnGenerateCppWriteSerializer(contents, apiType.NativeName, name, serialize);
            }
            else
            {
                // In-built serialization route (compiler will warn if type is not supported)
                OnGenerateCppWriteRaw(contents, name, serialize);
            }
        }
        
        private void OnGenerateCppWriteRaw(StringBuilder contents, string data, bool serialize)
        {
            var method = serialize ? "Write" : "Read";
            contents.AppendLine($"        stream->{method}({data});");
        }
        
        private void OnGenerateCppWriteSerializer(StringBuilder contents, string type, string data, bool serialize)
        {
            if (type == "ScriptingObject" || type == "Script" || type == "Actor")
                return;
            var mode = serialize ? "true" : "false";
            contents.AppendLine($"        NetworkReplicator::InvokeSerializer({type}::TypeInitializer, &{data}, stream, {mode});");
        }

        private void OnGenerateCppTypeInitRuntime(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents)
        {
            // Skip types that don't use networking
            var replicatedTag = typeInfo.GetTag(NetworkReplicated);
            var rpcTag = typeInfo.GetTag(NetworkRpc);
            if (replicatedTag == null && rpcTag == null)
                return;
            var typeNameNative = typeInfo.FullNameNative;
            var typeNameInternal = typeInfo.FullNameNativeInternal;

            if (replicatedTag != null)
            {
                // Register generated serializer functions
                contents.AppendLine($"        NetworkReplicator::AddSerializer(ScriptingTypeHandle({typeNameNative}::TypeInitializer), {typeNameInternal}Internal::INetworkSerializable_Serialize, {typeNameInternal}Internal::INetworkSerializable_Deserialize);");
            }
            if (rpcTag != null)
            {
                // Register generated RPCs
                List<FunctionInfo> functions = null;
                if (typeInfo is ClassInfo classInfo)
                {
                    functions = classInfo.Functions;
                }
                if (functions != null)
                {
                    foreach (var functionInfo in functions)
                    {
                        if (functionInfo.GetTag(NetworkRpc) == null)
                            continue;
                        contents.AppendLine($"        NetworkRpcInfo::RPCsTable[NetworkRpcName({typeNameNative}::TypeInitializer, StringAnsiView(\"{functionInfo.Name}\", {functionInfo.Name.Length}))] = {functionInfo.Name}_Info();");
                    }
                }
            }
        }

        private void OnGenerateCSharpTypeInternals(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents, string indent)
        {
            // Skip types that don't use networking
            if (typeInfo.GetTag(NetworkReplicated) == null)
                return;
            
            if (typeInfo is ClassInfo)
                return;

            // Generate C# wrapper functions to serialize/deserialize type directly from managed code
            OnGenerateCSharpTypeSerialize(buildData, typeInfo, contents, indent, true);
            OnGenerateCSharpTypeSerialize(buildData, typeInfo, contents, indent, false);

        }
        
        private void OnGenerateCSharpTypeSerialize(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents, string indent, bool serialize)
        {
            var mode = serialize ? "true" : "false";
            contents.AppendLine();
            contents.Append(indent).Append("public void ").Append(serialize ? Thunk1 : Thunk2).AppendLine("(FlaxEngine.Networking.NetworkStream stream)");
            contents.Append(indent).AppendLine("{");
            if (typeInfo is ClassInfo)
            {
                contents.Append(indent).AppendLine($"    FlaxEngine.Networking.NetworkReplicator.InvokeSerializer(typeof({typeInfo.Name}), this, stream, {mode});");
            }
            else
            {
                if (typeInfo.IsPod)
                {
                    contents.Append(indent).AppendLine($"    fixed ({typeInfo.Name}* ptr = &this)");
                    contents.Append(indent).AppendLine($"        FlaxEngine.Networking.NetworkReplicator.InvokeSerializer(typeof({typeInfo.Name}), new IntPtr(ptr), stream, {mode});");
                }
                else
                {
                    // TODO: generate C# data serialization (eg. in OnPatchAssembly) or generate internal call with managed -> native data conversion
                    contents.Append(indent).AppendLine($"    throw new NotImplementedException(\"Not supported native structure with references used in managed code for replication.\");");
                }
            }
            contents.Append(indent).AppendLine("}");
        }

        private void OnBuildDotNetAssembly(TaskGraph graph, Builder.BuildData buildData, NativeCpp.BuildOptions buildOptions, Task buildTask, IGrouping<string, Module> binaryModule)
        {
            // Skip FlaxEngine.dll
            if (string.Equals(binaryModule.Key, "FlaxEngine", StringComparison.Ordinal))
                return;

            // Skip assemblies not using networking
            if (!binaryModule.Any(module => module.Tags.ContainsKey(Network)))
                return;

            // Generate netoworking code inside assembly after it's being compiled
            var assemblyPath = buildTask.ProducedFiles[0];
            var task = graph.Add<Task>();
            task.ProducedFiles.Add(assemblyPath);
            task.WorkingDirectory = buildTask.WorkingDirectory;
            task.Command = () => OnPatchDotNetAssembly(buildData, buildOptions, buildTask, assemblyPath);
            task.CommandPath = null;
            task.InfoMessage = $"Generating netowrking code for {Path.GetFileName(assemblyPath)}...";
            task.Cost = 50;
            task.DisableCache = true;
            task.DependentTasks = new HashSet<Task>();
            task.DependentTasks.Add(buildTask);
        }

        private void OnPatchDotNetAssembly(Builder.BuildData buildData, NativeCpp.BuildOptions buildOptions, Task buildTask, string assemblyPath)
        {
            using (DefaultAssemblyResolver assemblyResolver = new DefaultAssemblyResolver())
            using (AssemblyDefinition assembly = AssemblyDefinition.ReadAssembly(assemblyPath, new ReaderParameters{ ReadWrite = true, ReadSymbols = true, AssemblyResolver = assemblyResolver }))
            {
                // Setup module search locations
                var searchDirectories = new HashSet<string>();
                searchDirectories.Add(Path.GetDirectoryName(assemblyPath));
                foreach (var file in buildTask.PrerequisiteFiles)
                {
                    if (file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase))
                        searchDirectories.Add(Path.GetDirectoryName(file));
                }
                foreach (var e in searchDirectories)
                    assemblyResolver.AddSearchDirectory(e);

                ModuleDefinition module = assembly.MainModule;
                TypeReference voidType = module.ImportReference(typeof(void));
                module.GetType("FlaxEngine.Networking.NetworkStream", out var networkStreamType);

                // Process all types within a module
                bool modified = false;
                bool failed = false;
                var addSerializers = new List<TypeSerializer>();
                var methodRPCs = new List<MethodRPC>();
                foreach (TypeDefinition type in module.Types)
                {
                    if (type.IsInterface || type.IsEnum)
                        continue;
                    var isNative = type.HasAttribute("FlaxEngine.UnmanagedAttribute");
                    if (isNative)
                        continue;

                    // Generate RPCs
                    var methods = type.Methods;
                    var methodsCount = methods.Count; // methods list can be modified during RPCs generation
                    for (int i = 0; i < methodsCount; i++)
                    {
                        MethodDefinition method = methods[i];
                        var attribute = method.CustomAttributes.FirstOrDefault(x => x.AttributeType.FullName == "FlaxEngine.NetworkRpcAttribute");
                        if (attribute != null)
                        {
                            GenerateDotNetRPCBody(type, method, attribute, ref failed, networkStreamType, methodRPCs);
                        }
                    }

                    // Generate serializers
                    if (type.HasMethod(Thunk1) || type.HasMethod(Thunk2))
                        continue;
                    var isINetworkSerializable = type.HasInterface("FlaxEngine.Networking.INetworkSerializable");
                    MethodDefinition serializeINetworkSerializable = null, deserializeINetworkSerializable = null;
                    if (isINetworkSerializable)
                    {
                        foreach (MethodDefinition m in type.Methods)
                        {
                            if (m.HasBody && m.Parameters.Count == 1 && m.Parameters[0].ParameterType.FullName == "FlaxEngine.Networking.NetworkStream")
                            {
                                if (m.Name == "Serialize")
                                    serializeINetworkSerializable = m;
                                else if (m.Name == "Deserialize")
                                    deserializeINetworkSerializable = m;
                            }
                        }
                    }
                    var isNetworkReplicated = false;
                    foreach (FieldDefinition f in type.Fields)
                    {
                        if (!f.HasAttribute(NetworkReplicatedAttribute))
                            continue;
                        isNetworkReplicated = true;
                        break;
                    }
                    foreach (PropertyDefinition p in type.Properties)
                    {
                        if (!p.HasAttribute(NetworkReplicatedAttribute))
                            continue;
                        isNetworkReplicated = true;
                        break;
                    }
                    if (type.IsValueType)
                    {
                        if (isINetworkSerializable)
                        {
                            // Generate INetworkSerializable interface method calls
                            GenerateCallINetworkSerializable(type, Thunk1, voidType, networkStreamType, serializeINetworkSerializable);
                            GenerateCallINetworkSerializable(type, Thunk2, voidType, networkStreamType, deserializeINetworkSerializable);
                            modified = true;
                        }
                        else if (isNetworkReplicated)
                        {
                            // Generate serializization methods
                            GenerateSerializer(type, true, ref failed, Thunk1, voidType, networkStreamType);
                            GenerateSerializer(type, false, ref failed, Thunk2, voidType, networkStreamType);
                            modified = true;
                        }
                    }
                    else if (!isINetworkSerializable && isNetworkReplicated)
                    {
                        // Generate serializization methods
                        var addSerializer = new TypeSerializer();
                        addSerializer.Type = type;
                        addSerializer.Serialize = GenerateNativeSerializer(type, true, ref failed, Thunk1, voidType, networkStreamType);
                        addSerializer.Deserialize = GenerateNativeSerializer(type, false, ref failed, Thunk2, voidType, networkStreamType);
                        addSerializers.Add(addSerializer);
                        modified = true;
                    }
                }
                if (failed)
                    throw new Exception($"Failed to generate network replication for assembly {assemblyPath}");
                if (!modified)
                    return;

                // Generate serializers initializer (invoked on module load)
                if (addSerializers.Count != 0 || methodRPCs.Count != 0)
                {
                    // Create class
                    var name = "Initializer";
                    var idx = 0;
                    while (module.Types.Any(x => x.Name == name))
                        name = "Initializer" + idx++;
                    var c = new TypeDefinition("", name, TypeAttributes.Class | TypeAttributes.AutoLayout | TypeAttributes.AnsiClass | TypeAttributes.Abstract | TypeAttributes.Sealed | TypeAttributes.BeforeFieldInit);
                    module.GetType("System.Object", out var objectType);
                    c.BaseType = module.ImportReference(objectType);
                    module.Types.Add(c);

                    // Add ModuleInitializer attribute
                    module.GetType("FlaxEngine.ModuleInitializerAttribute", out var moduleInitializerAttribute);
                    var ctor1 = moduleInitializerAttribute.Resolve();
                    MethodDefinition ctor = moduleInitializerAttribute.Resolve().Methods[0];
                    var attribute = new CustomAttribute(module.ImportReference(ctor));
                    c.CustomAttributes.Add(attribute);

                    // Add Init method
                    var m = new MethodDefinition("Init", MethodAttributes.Private | MethodAttributes.Static | MethodAttributes.HideBySig, voidType);
                    ILProcessor il = m.Body.GetILProcessor();
                    il.Emit(OpCodes.Nop);
                    module.GetType("System.Type", out var typeType);
                    var getTypeFromHandle = typeType.Resolve().GetMethod("GetTypeFromHandle");
                    module.GetType("FlaxEngine.Networking.NetworkReplicator", out var networkReplicatorType);
                    var addSerializer = networkReplicatorType.Resolve().GetMethod("AddSerializer", 3);
                    module.ImportReference(addSerializer);
                    var serializeFuncType = addSerializer.Parameters[1].ParameterType;
                    var serializeFuncCtor = serializeFuncType.Resolve().GetMethod(".ctor");
                    var addRPC = networkReplicatorType.Resolve().GetMethod("AddRPC", 6);
                    module.ImportReference(addRPC);
                    var executeRPCFuncType = addRPC.Parameters[2].ParameterType;
                    var executeRPCFuncCtor = executeRPCFuncType.Resolve().GetMethod(".ctor");
                    foreach (var e in addSerializers)
                    {
                        // NetworkReplicator.AddSerializer(typeof(<type>), <type>.INetworkSerializable_SerializeNative, <type>.INetworkSerializable_DeserializeNative);
                        il.Emit(OpCodes.Ldtoken, e.Type);
                        il.Emit(OpCodes.Call, module.ImportReference(getTypeFromHandle));
                        il.Emit(OpCodes.Ldnull);
                        il.Emit(OpCodes.Ldftn, e.Serialize);
                        il.Emit(OpCodes.Newobj, module.ImportReference(serializeFuncCtor));
                        il.Emit(OpCodes.Ldnull);
                        il.Emit(OpCodes.Ldftn, e.Deserialize);
                        il.Emit(OpCodes.Newobj, module.ImportReference(serializeFuncCtor));
                        il.Emit(OpCodes.Call, module.ImportReference(addSerializer));
                    }
                    foreach (var e in methodRPCs)
                    {
                        // NetworkReplicator.AddRPC(typeof(<type>), "<name>", <name>_Execute, <isServer>, <isClient>, <channel>);
                        il.Emit(OpCodes.Ldtoken, e.Type);
                        il.Emit(OpCodes.Call, module.ImportReference(getTypeFromHandle));
                        il.Emit(OpCodes.Ldstr, e.Method.Name);
                        il.Emit(OpCodes.Ldnull);
                        il.Emit(OpCodes.Ldftn, e.Execute);
                        il.Emit(OpCodes.Newobj, module.ImportReference(executeRPCFuncCtor));
                        il.Emit(OpCodes.Ldc_I4, e.IsServer ? 1 : 0);
                        il.Emit(OpCodes.Ldc_I4, e.IsClient ? 1 : 0);
                        il.Emit(OpCodes.Ldc_I4, e.Channel);
                        il.Emit(OpCodes.Call, module.ImportReference(addRPC));
                    }
                    il.Emit(OpCodes.Nop);
                    il.Emit(OpCodes.Ret);
                    c.Methods.Add(m);
                }

                // Serialize assembly back to the file
                assembly.Write(new WriterParameters { WriteSymbols = true } );
            }
        }

        private static void GenerateCallINetworkSerializable(TypeDefinition type, string name, TypeReference voidType, TypeReference networkStreamType, MethodDefinition method)
        {
            var m = new MethodDefinition(name, MethodAttributes.Public | MethodAttributes.HideBySig, voidType);
            m.Parameters.Add(new ParameterDefinition("stream", ParameterAttributes.None, networkStreamType));
            ILProcessor il = m.Body.GetILProcessor();
            il.Emit(OpCodes.Nop);
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, method);
            il.Emit(OpCodes.Nop);
            il.Emit(OpCodes.Ret);
            type.Methods.Add(m);
        }

        private static MethodDefinition GenerateSerializer(TypeDefinition type, bool serialize, ref bool failed, string name, TypeReference voidType, TypeReference networkStreamType)
        {
            ModuleDefinition module = type.Module;
            var m = new MethodDefinition(name, MethodAttributes.Public | MethodAttributes.HideBySig, voidType);
            m.Parameters.Add(new ParameterDefinition("stream", ParameterAttributes.None, module.ImportReference(networkStreamType)));
            TypeDefinition networkStream = networkStreamType.Resolve();
            ILProcessor il = m.Body.GetILProcessor();
            il.Emit(OpCodes.Nop);
            
            // Serialize base type
            if (type.BaseType != null && type.BaseType.FullName != "System.ValueType" && type.BaseType.FullName != "FlaxEngine.Object" && type.BaseType.CanBeResolved())
            {
                GenerateSerializeCallback(module, il, type.BaseType.Resolve(), serialize);
            }

            // Serialize all type fields marked with NetworkReplicated attribute
            foreach (FieldDefinition f in type.Fields)
            {
                if (!f.HasAttribute(NetworkReplicatedAttribute))
                    continue;
                GenerateSerializerType(type, serialize, ref failed, f, null, f.FieldType, il, networkStream);
            }

            // Serialize all type properties marked with NetworkReplicated attribute
            foreach (PropertyDefinition p in type.Properties)
            {
                if (!p.HasAttribute(NetworkReplicatedAttribute))
                    continue;
                GenerateSerializerType(type, serialize, ref failed, null, p, p.PropertyType, il, networkStream);
            }

            if (serialize)
                il.Emit(OpCodes.Nop);
            il.Emit(OpCodes.Ret);
            type.Methods.Add(m);
            return m;
        }

        private static MethodDefinition GenerateNativeSerializer(TypeDefinition type, bool serialize, ref bool failed, string name, TypeReference voidType, TypeReference networkStreamType)
        {
            ModuleDefinition module = type.Module;
            module.GetType("System.IntPtr", out var intPtrType);
            module.GetType("FlaxEngine.Object", out var scriptingObjectType);
            var fromUnmanagedPtr = scriptingObjectType.Resolve().GetMethod("FromUnmanagedPtr");

            var m = new MethodDefinition(name + "Native", MethodAttributes.Public | MethodAttributes.Static | MethodAttributes.HideBySig, voidType);
            m.Parameters.Add(new ParameterDefinition("instancePtr", ParameterAttributes.None, intPtrType));
            m.Parameters.Add(new ParameterDefinition("streamPtr", ParameterAttributes.None, intPtrType));
            TypeReference networkStream = module.ImportReference(networkStreamType);
            ILProcessor il = m.Body.GetILProcessor();
            il.Emit(OpCodes.Nop);
            il.Body.InitLocals = true;

            // <type> instance = (<type>)FlaxEngine.Object.FromUnmanagedPtr(instancePtr)
            il.Body.Variables.Add(new VariableDefinition(type));
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Call, module.ImportReference(fromUnmanagedPtr));
            il.Emit(OpCodes.Castclass, type);
            il.Emit(OpCodes.Stloc_0);
            
            // NetworkStream stream = (NetworkStream)FlaxEngine.Object.FromUnmanagedPtr(streamPtr)
            il.Body.Variables.Add(new VariableDefinition(networkStream));
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, module.ImportReference(fromUnmanagedPtr));
            il.Emit(OpCodes.Castclass, module.ImportReference(networkStream));
            il.Emit(OpCodes.Stloc_1);
            
            // Generate normal serializer
            var serializer = GenerateSerializer(type, serialize, ref failed, name, voidType, networkStreamType);

            // Call serializer
            il.Emit(OpCodes.Ldloc_0);
            il.Emit(OpCodes.Ldloc_1);
            il.Emit(OpCodes.Callvirt, serializer);

            if (serialize)
                il.Emit(OpCodes.Nop);
            il.Emit(OpCodes.Ret);
            type.Methods.Add(m);
            return m;
        }

        private static void GenerateSerializeCallback(ModuleDefinition module, ILProcessor il, TypeDefinition type, bool serialize)
        {
            if (type.IsScriptingObject())
            {
                // NetworkReplicator.InvokeSerializer(typeof(<type>), instance, stream, <serialize>)
                il.Emit(OpCodes.Ldtoken, module.ImportReference(type));
                module.GetType("System.Type", out var typeType);
                var getTypeFromHandle = typeType.Resolve().GetMethod("GetTypeFromHandle");
                il.Emit(OpCodes.Call, module.ImportReference(getTypeFromHandle));
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Ldarg_1);
                if (serialize)
                    il.Emit(OpCodes.Ldc_I4_1);
                else
                    il.Emit(OpCodes.Ldc_I4_0);
                module.GetType("FlaxEngine.Networking.NetworkReplicator", out var networkReplicatorType);
                var invokeSerializer = networkReplicatorType.Resolve().GetMethod("InvokeSerializer", 4);
                il.Emit(OpCodes.Call, module.ImportReference(invokeSerializer));
                il.Emit(OpCodes.Pop);
            }
            else
            {
                throw new Exception($"Not supported base type for network serializer '{type.FullName}'");
            }
        }

        private static void GenerateSerializerType(TypeDefinition type, bool serialize, ref bool failed, FieldReference field, PropertyDefinition property, TypeReference valueType, ILProcessor il, TypeDefinition networkStreamType)
        {
            if (field == null && property == null)
                throw new ArgumentException();
            var propertyGetOpCode = OpCodes.Call;
            var propertySetOpCode = OpCodes.Call;
            if (property != null)
            {
                if (property.GetMethod == null)
                {
                    MonoCecil.CompilationError($"Missing getter method for property '{property.Name}' of type {valueType.FullName} in {type.FullName} for automatic replication.", property);
                    failed = true;
                    return;
                }
                if (property.SetMethod == null)
                {
                    MonoCecil.CompilationError($"Missing setter method for property '{property.Name}' of type {valueType.FullName} in {type.FullName} for automatic replication.", property);
                    failed = true;
                    return;
                }
                if (property.GetMethod.IsVirtual)
                    propertyGetOpCode = OpCodes.Callvirt;
                if (property.SetMethod.IsVirtual)
                    propertySetOpCode = OpCodes.Callvirt;
            }
            ModuleDefinition module = type.Module;
            TypeDefinition valueTypeDef = valueType.Resolve();
            if (_inBuildSerializers.TryGetValue(valueType.FullName, out var serializer))
            {
                // Call NetworkStream method to write/read data
                MethodDefinition m;
                if (serialize)
                {
                    il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldarg_0);
                    if (field != null)
                        il.Emit(OpCodes.Ldfld, field);
                    else
                        il.Emit(propertyGetOpCode, property.GetMethod);
                    m = networkStreamType.GetMethod(serializer.WriteMethod);
                }
                else
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldarg_1);
                    m = networkStreamType.GetMethod(serializer.ReadMethod);
                }
                il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                if (!serialize)
                {
                    if (field != null)
                        il.Emit(OpCodes.Stfld, field);
                    else
                        il.Emit(propertySetOpCode, property.SetMethod);
                }
            }
            else if (valueType.IsScriptingObject())
            {
                // Replicate ScriptingObject as Guid ID
                module.GetType("System.Guid", out var guidType);
                module.GetType("FlaxEngine.Object", out var scriptingObjectType);
                if (serialize)
                {
                    il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldarg_0);
                    if (field != null)
                        il.Emit(OpCodes.Ldfld, field);
                    else
                        il.Emit(propertyGetOpCode, property.GetMethod);
                    il.Emit(OpCodes.Dup);
                    Instruction jmp1 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brtrue_S, jmp1);
                    il.Emit(OpCodes.Pop);
                    var guidEmpty = guidType.Resolve().GetField("Empty");
                    il.Emit(OpCodes.Ldsfld, module.ImportReference(guidEmpty));
                    Instruction jmp2 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Br_S, jmp2);
                    il.Append(jmp1);
                    var getID = scriptingObjectType.Resolve().GetMethod("get_ID");
                    il.Emit(OpCodes.Call, module.ImportReference(getID));
                    il.Append(jmp2);
                    var writeGuid = networkStreamType.GetMethod("WriteGuid");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(writeGuid));
                }
                else
                {
                    var m = networkStreamType.GetMethod("ReadGuid");
                    module.GetType("System.Type", out var typeType);
                    il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    il.Emit(OpCodes.Stloc_0);
                    il.Emit(OpCodes.Ldarg_0);
                    var varStart = il.Body.Variables.Count;
                    var reference = module.ImportReference(guidType);
                    reference.IsValueType = true; // Fix locals init to have valuetype for Guid instead of class
                    il.Body.Variables.Add(new VariableDefinition(reference));
                    il.Body.InitLocals = true;
                    il.Emit(OpCodes.Ldloca_S, (byte)varStart);
                    il.Emit(OpCodes.Ldtoken, valueType);
                    var getTypeFromHandle = typeType.Resolve().GetMethod("GetTypeFromHandle");
                    il.Emit(OpCodes.Call, module.ImportReference(getTypeFromHandle));
                    var tryFind = scriptingObjectType.Resolve().GetMethod("TryFind", 2);
                    il.Emit(OpCodes.Call, module.ImportReference(tryFind));
                    il.Emit(OpCodes.Castclass, valueType);
                    if (field != null)
                        il.Emit(OpCodes.Stfld, field);
                    else
                        il.Emit(propertySetOpCode, property.SetMethod);
                }
            }
            else if (valueTypeDef.IsEnum)
            {
                // Replicate enum as bits
                // TODO: use smaller uint depending on enum values range
                if (serialize)
                {
                    il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldarg_0);
                    if (field != null)
                        il.Emit(OpCodes.Ldfld, field);
                    else
                        il.Emit(propertyGetOpCode, property.GetMethod);
                    var m = networkStreamType.GetMethod("WriteUInt32");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                }
                else
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldarg_1);
                    var m = networkStreamType.GetMethod("ReadUInt32");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    if (field != null)
                        il.Emit(OpCodes.Stfld, field);
                    else
                        il.Emit(propertySetOpCode, property.SetMethod);
                }
            }
            else if (valueType.IsValueType)
            {
                // Invoke structure generated serializer
                // TODO: check if this type has generated serialization code
                il.Emit(OpCodes.Ldarg_0);
                if (field != null)
                    il.Emit(OpCodes.Ldflda, field);
                else
                    il.Emit(propertyGetOpCode, property.GetMethod);
                il.Emit(OpCodes.Ldarg_1);
                var m = valueTypeDef.GetMethod(serialize ? Thunk1 : Thunk2);
                il.Emit(OpCodes.Call, module.ImportReference(m));
            }
            else if (valueType.IsArray && valueType.GetElementType().IsValueType)
            {
                // TODO: support any array type by iterating over elements (separate serialize for each one)
                var elementType = valueType.GetElementType();
                var varStart = il.Body.Variables.Count;
                module.GetType("System.Int32", out var intType);
                il.Body.Variables.Add(new VariableDefinition(intType));
                il.Body.Variables.Add(new VariableDefinition(new PointerType(elementType)));
                il.Body.Variables.Add(new VariableDefinition(new PinnedType(valueType)));
                il.Body.InitLocals = true;
                if (serialize)
                {
                    // <elementType>[] array2 = Array1;
                    il.Emit(OpCodes.Nop);
                    il.Emit(OpCodes.Ldarg_0);
                    if (field != null)
                        il.Emit(OpCodes.Ldfld, field);
                    else
                        il.Emit(propertyGetOpCode, property.GetMethod);

		            // int num2 = ((array2 != null) ? array2.Length : 0);
                    il.Emit(OpCodes.Dup);
                    Instruction jmp1 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brtrue_S, jmp1);
                    il.Emit(OpCodes.Pop);
                    il.Emit(OpCodes.Ldc_I4_0);
                    Instruction jmp2 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Br_S, jmp2);
                    il.Append(jmp1);
                    il.Emit(OpCodes.Ldlen);
                    il.Emit(OpCodes.Conv_I4);
                    il.Append(jmp2);
                    il.Emit(OpCodes.Stloc, varStart + 0);
                
		            // stream.WriteInt32(num2);
                    il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldloc, varStart + 0);
                    var m = networkStreamType.GetMethod("WriteInt32");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));

                    // fixed (<elementType>* bytes2 = Array1)
                    il.Emit(OpCodes.Nop);
                    il.Emit(OpCodes.Ldarg_0);
                    if (field != null)
                        il.Emit(OpCodes.Ldfld, field);
                    else
                        il.Emit(propertyGetOpCode, property.GetMethod);
                    il.Emit(OpCodes.Dup);
                    il.Emit(OpCodes.Stloc, varStart + 2);
                    Instruction jmp3 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brfalse_S, jmp3);
                    il.Emit(OpCodes.Ldloc_2);
                    il.Emit(OpCodes.Ldlen);
                    il.Emit(OpCodes.Conv_I4);
                    Instruction jmp4 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brtrue_S, jmp4);
                    il.Append(jmp3);
                    il.Emit(OpCodes.Ldc_I4_0);
                    il.Emit(OpCodes.Conv_U);
                    il.Emit(OpCodes.Stloc, varStart + 1);
                    Instruction jmp5 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Br_S, jmp5);

                    // stream.WriteBytes((byte*)bytes, num * sizeof(<elementType>)));
                    il.Append(jmp4);
                    il.Emit(OpCodes.Ldloc, varStart + 2);
                    il.Emit(OpCodes.Ldc_I4_0);
                    il.Emit(OpCodes.Ldelema, elementType);
                    il.Emit(OpCodes.Conv_U);
                    il.Emit(OpCodes.Stloc, varStart + 1);
                    il.Append(jmp5);
                    il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldloc, varStart + 1);
                    il.Emit(OpCodes.Ldloc, varStart + 0);
                    il.Emit(OpCodes.Sizeof, elementType);
                    il.Emit(OpCodes.Mul);
                    m = networkStreamType.GetMethod("WriteBytes", 2);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    il.Emit(OpCodes.Nop);
                    il.Emit(OpCodes.Ldnull);
                    il.Emit(OpCodes.Stloc, varStart + 2);
                }
                else
                {
                    if (field == null)
                        throw new NotImplementedException("TODO: add support for array property replication");

                    // int num = stream.ReadInt32();
                    il.Emit(OpCodes.Nop);
                    il.Emit(OpCodes.Ldarg_1);
                    var m = networkStreamType.GetMethod("ReadInt32");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    il.Emit(OpCodes.Stloc, varStart + 0);
                    
		            // System.Array.Resize(ref Array1, num);
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldflda, field);
                    il.Emit(OpCodes.Ldloc, varStart + 0);
                    module.TryGetTypeReference("System.Array", out var arrayType);
                    m = arrayType.Resolve().GetMethod("Resize", 2);
                    il.Emit(OpCodes.Call, module.ImportReference(m.InflateGeneric(elementType)));
                    
		            // fixed (int* buffer = Array1)
                    il.Emit(OpCodes.Nop);
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldfld, field);
                    il.Emit(OpCodes.Dup);
                    il.Emit(OpCodes.Stloc, varStart + 2);
                    Instruction jmp1 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brfalse_S, jmp1);
                    il.Emit(OpCodes.Ldloc, varStart + 2);
                    il.Emit(OpCodes.Ldlen);
                    il.Emit(OpCodes.Conv_I4);
                    Instruction jmp2 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brtrue_S, jmp2);
                    il.Append(jmp1);
                    il.Emit(OpCodes.Ldc_I4_0);
                    il.Emit(OpCodes.Conv_U);
                    il.Emit(OpCodes.Stloc, varStart + 1);
                    Instruction jmp3 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Br_S, jmp3);
                    
		            // stream.ReadBytes((byte*)buffer, num * sizeof(<elementType>));
                    il.Append(jmp2);
                    il.Emit(OpCodes.Ldloc, varStart + 2);
                    il.Emit(OpCodes.Ldc_I4_0);
                    il.Emit(OpCodes.Ldelema, elementType);
                    il.Emit(OpCodes.Conv_U);
                    il.Emit(OpCodes.Stloc, varStart + 1);
                    il.Append(jmp3);
                    il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldloc, varStart + 1);
                    il.Emit(OpCodes.Ldloc, varStart + 0);
                    il.Emit(OpCodes.Sizeof, elementType);
                    il.Emit(OpCodes.Mul);
                    m = networkStreamType.GetMethod("ReadBytes", 2);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    il.Emit(OpCodes.Nop);
                    il.Emit(OpCodes.Ldnull);
                    il.Emit(OpCodes.Stloc, varStart + 2);
                }
            }
            else
            {
                // Unknown type
                if (property != null)
                    MonoCecil.CompilationError($"Not supported type '{valueType.FullName}' on {property.Name} in {type.FullName} for automatic replication.", property);
                else if (field != null)
                    MonoCecil.CompilationError($"Not supported type '{valueType.FullName}' on {field.Name} in {type.FullName} for automatic replication.", field.Resolve());
                else
                    MonoCecil.CompilationError($"Not supported type '{valueType.FullName}' for automatic replication.");
                failed = true;
            }
        }

        private static void GenerateDotNetRPCSerializerType(TypeDefinition type, bool serialize, ref bool failed, int localIndex, TypeReference valueType, ILProcessor il, TypeDefinition networkStreamType, int streamLocalIndex, Instruction ilStart)
        {
            ModuleDefinition module = type.Module;
            TypeDefinition valueTypeDef = valueType.Resolve();
            if (_inBuildSerializers.TryGetValue(valueType.FullName, out var serializer))
            {
                // Call NetworkStream method to write/read data
                if (serialize)
                {
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, streamLocalIndex));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldarg, localIndex));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod(serializer.WriteMethod))));
                }
                else
                {
                    il.Emit(OpCodes.Ldloc_1);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod(serializer.ReadMethod)));
                    il.Emit(OpCodes.Stloc, localIndex);
                }
            }
            else if (valueType.IsScriptingObject())
            {
                // Replicate ScriptingObject as Guid ID
                module.GetType("System.Guid", out var guidType);
                module.GetType("FlaxEngine.Object", out var scriptingObjectType);
                if (serialize)
                {  
                    il.InsertBefore(ilStart, il.Create(OpCodes.Nop));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, streamLocalIndex));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldarg, localIndex));
                    Instruction jmp1 = il.Create(OpCodes.Nop);
                    il.InsertBefore(ilStart, il.Create(OpCodes.Brtrue_S, jmp1));
                    var guidEmpty = guidType.Resolve().GetField("Empty");
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldsfld, module.ImportReference(guidEmpty)));
                    Instruction jmp2 = il.Create(OpCodes.Nop);
                    il.InsertBefore(ilStart, il.Create(OpCodes.Br_S, jmp2));
                    il.InsertBefore(ilStart, jmp1);
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldarg, localIndex));
                    var getID = scriptingObjectType.Resolve().GetMethod("get_ID");
                    il.InsertBefore(ilStart, il.Create(OpCodes.Call, module.ImportReference(getID)));
                    il.InsertBefore(ilStart, jmp2);
                    var writeGuid = networkStreamType.GetMethod("WriteGuid");
                    il.InsertBefore(ilStart, il.Create(OpCodes.Callvirt, module.ImportReference(writeGuid)));
                }
                else
                {
                    var m = networkStreamType.GetMethod("ReadGuid");
                    module.GetType("System.Type", out var typeType);
                    il.Emit(OpCodes.Ldloc_1);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    var varStart = il.Body.Variables.Count;
                    var reference = module.ImportReference(guidType);
                    reference.IsValueType = true; // Fix locals init to have valuetype for Guid instead of class
                    il.Body.Variables.Add(new VariableDefinition(reference));
                    il.Body.InitLocals = true;
                    il.Emit(OpCodes.Stloc_S, (byte)varStart);
                    il.Emit(OpCodes.Ldloca_S, (byte)varStart);
                    il.Emit(OpCodes.Ldtoken, valueType);
                    var getTypeFromHandle = typeType.Resolve().GetMethod("GetTypeFromHandle");
                    il.Emit(OpCodes.Call, module.ImportReference(getTypeFromHandle));
                    var tryFind = scriptingObjectType.Resolve().GetMethod("TryFind", 2);
                    il.Emit(OpCodes.Call, module.ImportReference(tryFind));
                    il.Emit(OpCodes.Castclass, valueType);
                    il.Emit(OpCodes.Stloc, localIndex);
                }
            }
            else if (valueTypeDef.IsEnum)
            {
                // Replicate enum as bits
                // TODO: use smaller uint depending on enum values range
                if (serialize)
                {
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, streamLocalIndex));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldarg, localIndex));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("WriteUInt32"))));
                }
                else
                {
                    il.Emit(OpCodes.Ldloc_1);
                    var m = networkStreamType.GetMethod("ReadUInt32");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    il.Emit(OpCodes.Stloc, localIndex);
                }
            }
            else if (valueType.IsValueType)
            {
                // Invoke structure generated serializer
                // TODO: check if this type has generated serialization code
                if (serialize)
                {
                    il.InsertBefore(ilStart, il.Create(OpCodes.Nop));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldarga, localIndex));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, streamLocalIndex));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Call, module.ImportReference(valueTypeDef.GetMethod(Thunk1))));
                }
                else
                {
                    il.Emit(OpCodes.Ldloca_S, (byte)localIndex);
                    il.Emit(OpCodes.Initobj, valueType);
                    il.Emit(OpCodes.Ldloca_S, (byte)localIndex);
                    il.Emit(OpCodes.Ldloc_1);
                    il.Emit(OpCodes.Call, module.ImportReference(valueTypeDef.GetMethod(Thunk2)));
                }
            }
            else
            {
                // Unknown type
                Log.Error($"Not supported type '{valueType.FullName}' for RPC parameter in {type.FullName}.");
                failed = true;
            }
        }

        private static void GenerateDotNetRPCBody(TypeDefinition type, MethodDefinition method, CustomAttribute attribute, ref bool failed, TypeReference networkStreamType, List<MethodRPC> methodRPCs)
        {
            // Validate RPC usage
            if (method.IsAbstract)
            {
                MonoCecil.CompilationError($"Not supported abstract RPC method '{method.FullName}'.", method);
                failed = true;
                return;
            }
            if (method.IsVirtual)
            {
                MonoCecil.CompilationError($"Not supported virtual RPC method '{method.FullName}'.", method);
                failed = true;
                return;
            }
            ModuleDefinition module = type.Module;
            var voidType = module.TypeSystem.Void;
            if (method.ReturnType != voidType)
            {
                MonoCecil.CompilationError($"Not supported non-void RPC method '{method.FullName}'.", method);
                failed = true;
                return;
            }
            if (method.IsStatic)
            {
                MonoCecil.CompilationError($"Not supported static RPC method '{method.FullName}'.", method);
                failed = true;
                return;
            }
            var methodRPC = new MethodRPC();
            methodRPC.Type = type;
            methodRPC.Method = method;
            methodRPC.Channel = 4; // int as NetworkChannelType (default is ReliableOrdered=4)
            if (attribute.HasConstructorArguments && attribute.ConstructorArguments.Count >= 3)
            {
                methodRPC.IsServer = (bool)attribute.ConstructorArguments[0].Value;
                methodRPC.IsClient = (bool)attribute.ConstructorArguments[1].Value;
                methodRPC.Channel = (int)attribute.ConstructorArguments[2].Value;
            }
            methodRPC.IsServer = (bool)attribute.GetFieldValue("Server", methodRPC.IsServer);
            methodRPC.IsClient = (bool)attribute.GetFieldValue("Client", methodRPC.IsClient);
            methodRPC.Channel = (int)attribute.GetFieldValue("Channel", methodRPC.Channel);
            if (methodRPC.IsServer && methodRPC.IsClient)
            {
                MonoCecil.CompilationError($"Network RPC {method.Name} in {type.FullName} cannot be both Server and Client.", method);
                failed = true;
                return;
            }
            if (!methodRPC.IsServer && !methodRPC.IsClient)
            {
                MonoCecil.CompilationError($"Network RPC {method.Name} in {type.FullName} needs to have Server or Client specifier.", method);
                failed = true;
                return;
            }
            module.GetType("System.IntPtr", out var intPtrType);
            module.GetType("FlaxEngine.Object", out var scriptingObjectType);
            var fromUnmanagedPtr = scriptingObjectType.Resolve().GetMethod("FromUnmanagedPtr");
            TypeReference networkStream = module.ImportReference(networkStreamType);

            // Generate static method to execute RPC locally
            {
                var m = new MethodDefinition(method.Name + "_Execute", MethodAttributes.Static | MethodAttributes.Assembly | MethodAttributes.HideBySig, voidType);
                m.Parameters.Add(new ParameterDefinition("instancePtr", ParameterAttributes.None, intPtrType));
                m.Parameters.Add(new ParameterDefinition("streamPtr", ParameterAttributes.None, module.ImportReference(intPtrType)));
                ILProcessor il = m.Body.GetILProcessor();
                il.Emit(OpCodes.Nop);
                il.Body.InitLocals = true;

                // <type> instance = (<type>)FlaxEngine.Object.FromUnmanagedPtr(instancePtr)
                il.Body.Variables.Add(new VariableDefinition(type));
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Call, module.ImportReference(fromUnmanagedPtr));
                il.Emit(OpCodes.Castclass, type);
                il.Emit(OpCodes.Stloc_0);

                // NetworkStream stream = (NetworkStream)FlaxEngine.Object.FromUnmanagedPtr(streamPtr)
                il.Body.Variables.Add(new VariableDefinition(networkStream));
                il.Emit(OpCodes.Ldarg_1);
                il.Emit(OpCodes.Call, module.ImportReference(fromUnmanagedPtr));
                il.Emit(OpCodes.Castclass, networkStream);
                il.Emit(OpCodes.Stloc_1);
                
                // Add locals for each RPC parameter
                var argsStart = il.Body.Variables.Count;
                for (int i = 0; i < method.Parameters.Count; i++)
                {
                    var parameter = method.Parameters[i];
                    if (parameter.IsOut)
                    {
                        MonoCecil.CompilationError($"Network RPC {method.Name} in {type.FullName} parameter {parameter.Name} cannot be 'out'.", method);
                        failed = true;
                        return;
                    }
                    var parameterType = parameter.ParameterType;
                    il.Body.Variables.Add(new VariableDefinition(parameterType));
                }

                // Deserialize parameters from the stream
                for (int i = 0; i < method.Parameters.Count; i++)
                {
                    var parameter = method.Parameters[i];
                    var parameterType = parameter.ParameterType;
                    GenerateDotNetRPCSerializerType(type, false, ref failed, argsStart + i, parameterType, il, networkStream.Resolve(), 1, null);
                }

                // Call RPC method body
                il.Emit(OpCodes.Ldloc_0);
                for (int i = 0; i < method.Parameters.Count; i++)
                {
                    il.Emit(OpCodes.Ldloc, argsStart + i);
                }
                il.Emit(OpCodes.Callvirt, method);
                
                il.Emit(OpCodes.Nop);
                il.Emit(OpCodes.Ret);
                type.Methods.Add(m);
                methodRPC.Execute = m;
            }

            // Inject custom code before RPC method body to invoke it
            {
                ILProcessor il = method.Body.GetILProcessor();
                Instruction ilStart = il.Body.Instructions[0];
                module.GetType("System.Boolean", out var boolType);
                module.GetType("FlaxEngine.Networking.NetworkManagerMode", out var networkManagerModeType);
                module.GetType("FlaxEngine.Networking.NetworkManager", out var networkManagerType);
                var networkManagerGetMode = networkManagerType.Resolve().GetMethod("get_Mode", 0);
                il.Body.InitLocals = true;
                var varsStart = il.Body.Variables.Count;

                // Is Server/Is Client boolean constants
                il.Body.Variables.Add(new VariableDefinition(module.ImportReference(boolType))); // [0]
                il.Body.Variables.Add(new VariableDefinition(module.ImportReference(boolType))); // [1]
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldc_I4, methodRPC.IsServer ? 1 : 0));
                il.InsertBefore(ilStart, il.Create(OpCodes.Stloc, varsStart + 0)); // isServer loc=0
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldc_I4, methodRPC.IsClient ? 1 : 0));
                il.InsertBefore(ilStart, il.Create(OpCodes.Stloc, varsStart + 1)); // isClient loc=1

                // NetworkManagerMode mode = NetworkManager.Mode;
                il.Body.Variables.Add(new VariableDefinition(module.ImportReference(networkManagerModeType))); // [2]
                il.InsertBefore(ilStart, il.Create(OpCodes.Call, module.ImportReference(networkManagerGetMode)));
                il.InsertBefore(ilStart, il.Create(OpCodes.Stloc, varsStart + 2)); // mode loc=2

                // if ((server && networkMode == NetworkManagerMode.Client) || (client && networkMode != NetworkManagerMode.Client))
                var jumpIfBodyStart = il.Create(OpCodes.Nop); // if block body
                var jumpIf2Start = il.Create(OpCodes.Nop); // 2nd part of the if
                var jumpBodyStart = il.Create(OpCodes.Nop); // original method body start
                var jumpBodyEnd = il.Body.Instructions.Last(x => x.OpCode == OpCodes.Ret && x.Previous != null);
                if (jumpBodyEnd == null)
                    throw new Exception("Missing IL Return op code in method " + method.Name);
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, varsStart + 0));
                il.InsertBefore(ilStart, il.Create(OpCodes.Brfalse_S, jumpIf2Start));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, varsStart + 2));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldc_I4_2));
                il.InsertBefore(ilStart, il.Create(OpCodes.Beq_S, jumpIfBodyStart));
                // ||
                il.InsertBefore(ilStart, jumpIf2Start);
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, varsStart + 1));
                il.InsertBefore(ilStart, il.Create(OpCodes.Brfalse_S, jumpBodyStart));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, varsStart + 2));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldc_I4_2));
                il.InsertBefore(ilStart, il.Create(OpCodes.Beq_S, jumpBodyStart));
                // {
                il.InsertBefore(ilStart, jumpIfBodyStart);

                // NetworkStream stream = NetworkReplicator.BeginInvokeRPC();
                il.Body.Variables.Add(new VariableDefinition(module.ImportReference(networkStream))); // [3]
                var streamLocalIndex = varsStart + 3;
                module.GetType("FlaxEngine.Networking.NetworkReplicator", out var networkReplicatorType);
                var beginInvokeRPC = networkReplicatorType.Resolve().GetMethod("BeginInvokeRPC", 0);
                il.InsertBefore(ilStart, il.Create(OpCodes.Call, module.ImportReference(beginInvokeRPC)));
                il.InsertBefore(ilStart, il.Create(OpCodes.Stloc, streamLocalIndex)); // stream loc=3

                // Serialize all RPC parameters
                for (int i = 0; i < method.Parameters.Count; i++)
                {
                    var parameter = method.Parameters[i];
                    var parameterType = parameter.ParameterType;
                    GenerateDotNetRPCSerializerType(type, true, ref failed, i + 1, parameterType, il, networkStream.Resolve(), streamLocalIndex, ilStart);
                }

                // NetworkReplicator.EndInvokeRPC(this, typeof(<type>), "<name>", stream);
                il.InsertBefore(ilStart, il.Create(OpCodes.Nop));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldarg_0));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldtoken, type));
                module.GetType("System.Type", out var typeType);
                var getTypeFromHandle = typeType.Resolve().GetMethod("GetTypeFromHandle");
                il.InsertBefore(ilStart, il.Create(OpCodes.Call, module.ImportReference(getTypeFromHandle)));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldstr, method.Name));
                il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, streamLocalIndex));
                var endInvokeRPC = networkReplicatorType.Resolve().GetMethod("EndInvokeRPC", 4);
                il.InsertBefore(ilStart, il.Create(OpCodes.Call, module.ImportReference(endInvokeRPC)));

                // if (server && networkMode == NetworkManagerMode.Client) return;
                if (methodRPC.IsServer)
                {
                    il.InsertBefore(ilStart, il.Create(OpCodes.Nop));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, varsStart + 2));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldc_I4_2));
                    var tmp = il.Create(OpCodes.Nop);
                    il.InsertBefore(ilStart, il.Create(OpCodes.Beq_S, tmp));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Br, jumpBodyStart));
                    il.InsertBefore(ilStart, tmp);
                    //il.InsertBefore(ilStart, il.Create(OpCodes.Ret));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Br, jumpBodyEnd));

                }

                // if (client && networkMode == NetworkManagerMode.Server) return;
                if (methodRPC.IsClient)
                {
                    il.InsertBefore(ilStart, il.Create(OpCodes.Nop));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldloc, varsStart + 2));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Ldc_I4_1));
                    var tmp = il.Create(OpCodes.Nop);
                    il.InsertBefore(ilStart, il.Create(OpCodes.Beq_S, tmp));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Br, jumpBodyStart));
                    il.InsertBefore(ilStart, tmp);
                    //il.InsertBefore(ilStart, il.Create(OpCodes.Ret));
                    il.InsertBefore(ilStart, il.Create(OpCodes.Br, jumpBodyEnd));
                }

                // Continue to original method body
                il.InsertBefore(ilStart, jumpBodyStart);
            }

            methodRPCs.Add(methodRPC);
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

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
    /// Flax.Build plugin for Networking extensions support. Generates required bindings glue code for automatic types replication and RPCs invoking. 
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

        private struct DotnetContext
        {
            public bool Modified;
            public bool Failed;
            public AssemblyDefinition Assembly;
            public List<TypeSerializer> AddSerializers;
            public List<MethodRPC> MethodRPCs;
            public HashSet<TypeDefinition> GeneratedSerializers;
            public TypeReference VoidType;
            public TypeReference NetworkStreamType;
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

                            // Special handling of Rpc Params
                            if (!arg.Type.IsPtr && arg.Type.Type == "NetworkRpcParams")
                            {
                                argNames += "NetworkRpcParams(stream)";
                                continue;
                            }

                            // Deserialize arguments
                            argNames += arg.Name;
                            contents.AppendLine($"        {arg.Type.GetFullNameNative(buildData, typeInfo, false, false)} {arg.Name};");
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
                        contents.Append("    static bool ").Append(functionInfo.Name).AppendLine("_Invoke(ScriptingObject* obj, void** args)");
                        contents.AppendLine("    {");
                        contents.AppendLine("        NetworkStream* stream = NetworkReplicator::BeginInvokeRPC();");
                        contents.AppendLine("        Span<uint32> targetIds;");
                        for (int i = 0; i < functionInfo.Parameters.Count; i++)
                        {
                            var arg = functionInfo.Parameters[i];

                            // Special handling of Rpc Params
                            if (!arg.Type.IsPtr && arg.Type.Type == "NetworkRpcParams")
                            {
                                contents.AppendLine($"        targetIds = ((NetworkRpcParams*)args[{i}])->TargetIds;");
                                continue;
                            }

                            // Serialize arguments
                            contents.AppendLine($"        stream->Write(*(const {arg.Type.GetFullNameNative(buildData, typeInfo, false, false)}*)args[{i}]);");
                        }

                        // Invoke RPC
                        contents.AppendLine($"        return NetworkReplicator::EndInvokeRPC(obj, {typeInfo.NativeName}::TypeInitializer, StringAnsiView(\"{functionInfo.Name}\", {functionInfo.Name.Length}), stream, targetIds);");
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

        private static bool IsRawPOD(Builder.BuildData buildData, ApiTypeInfo type)
        {
            // TODO: what if type fields have custom replication settings (eg. compression)?
            type.EnsureInited(buildData);
            return type.IsPod;
        }

        private static bool IsRawPOD(Builder.BuildData buildData, ApiTypeInfo caller, ApiTypeInfo apiType, TypeInfo type)
        {
            if (type.IsPod(buildData, caller))
            {
                if (apiType != null)
                    return IsRawPOD(buildData, apiType);
            }

            return false;
        }

        private static bool IsRawPOD(TypeReference type)
        {
            if (type.FullName == "System.ValueType")
                return true;
            if (type.IsValueType)
            {
                var typeDef = type.Resolve();
                if (typeDef.IsEnum)
                    return true;
                var baseType = typeDef.BaseType;
                if (baseType != null && !IsRawPOD(baseType))
                    return false;
                foreach (var field in typeDef.Fields)
                {
                    if (!field.IsStatic && field.FieldType != type && !IsRawPOD(field.FieldType))
                        return false;
                }
                return true;
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

            // Generate networking code inside assembly after it's being compiled
            var assemblyPath = buildTask.ProducedFiles[0];
            var task = graph.Add<Task>();
            task.ProducedFiles.Add(assemblyPath);
            task.WorkingDirectory = buildTask.WorkingDirectory;
            task.Command = () => OnPatchDotNetAssembly(buildData, buildOptions, buildTask, assemblyPath);
            task.CommandPath = null;
            task.InfoMessage = $"Generating networking code for {Path.GetFileName(assemblyPath)}...";
            task.Cost = 50;
            task.DependentTasks = new HashSet<Task>();
            task.DependentTasks.Add(buildTask);
        }

        private void OnPatchDotNetAssembly(Builder.BuildData buildData, NativeCpp.BuildOptions buildOptions, Task buildTask, string assemblyPath)
        {
            using (DefaultAssemblyResolver assemblyResolver = new DefaultAssemblyResolver())
            using (AssemblyDefinition assembly = AssemblyDefinition.ReadAssembly(assemblyPath, new ReaderParameters { ReadWrite = true, ReadSymbols = true, AssemblyResolver = assemblyResolver }))
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

                // Process all types within a module
                var context = new DotnetContext
                {
                    Modified = false,
                    Failed = false,
                    Assembly = assembly,
                    AddSerializers = new List<TypeSerializer>(),
                    MethodRPCs = new List<MethodRPC>(),
                    GeneratedSerializers = new HashSet<TypeDefinition>(),
                    VoidType = module.ImportReference(typeof(void)),
                };
                module.GetType("FlaxEngine.Networking.NetworkStream", out context.NetworkStreamType);
                foreach (TypeDefinition type in module.Types)
                {
                    GenerateTypeNetworking(ref context, type);
                }

                if (context.Failed)
                    throw new Exception($"Failed to generate network replication for assembly {assemblyPath}");
                if (!context.Modified)
                    return;

                // Generate serializers initializer (invoked on module load)
                if (context.AddSerializers.Count != 0 || context.MethodRPCs.Count != 0)
                {
                    // Create class
                    var name = "NetworkingPlugin";
                    if (module.Types.Any(x => x.Name == name))
                        throw new Exception($"Failed to generate network replication for assembly '{Path.GetFileName(assemblyPath)}' that already has net code generated. Rebuild project.");
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
                    var m = new MethodDefinition("Init", MethodAttributes.Private | MethodAttributes.Static | MethodAttributes.HideBySig, context.VoidType);
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
                    foreach (var e in context.AddSerializers)
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

                    foreach (var e in context.MethodRPCs)
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
                assembly.Write(new WriterParameters { WriteSymbols = true });
            }
        }

        private static void GenerateTypeNetworking(ref DotnetContext context, TypeDefinition type)
        {
            if (type.IsInterface || type.IsEnum)
                return;

            // Process nested types
            foreach (var nestedType in type.NestedTypes)
            {
                GenerateTypeNetworking(ref context, nestedType);
            }

            if (type.IsClass)
            {
                // Generate RPCs
                var methods = type.Methods;
                var methodsCount = methods.Count; // methods list can be modified during RPCs generation
                for (int i = 0; i < methodsCount; i++)
                {
                    MethodDefinition method = methods[i];
                    var attribute = method.CustomAttributes.FirstOrDefault(x => x.AttributeType.FullName == "FlaxEngine.NetworkRpcAttribute");
                    if (attribute != null)
                    {
                        GenerateDotNetRPCBody(ref context, type, method, attribute, context.NetworkStreamType);
                        context.Modified = true;
                    }
                }
            }

            GenerateTypeSerialization(ref context, type);
        }

        private static void GenerateTypeSerialization(ref DotnetContext context, TypeDefinition type)
        {
            // Skip types outside from current assembly
            if (context.Assembly.MainModule != type.Module)
                return;

            // Skip if already generated serialization for this type (eg. via referenced RPC in other type)
            if (context.GeneratedSerializers.Contains(type))
                return;
            context.GeneratedSerializers.Add(type);

            // Skip native types
            var isNative = type.HasAttribute("FlaxEngine.UnmanagedAttribute");
            if (isNative)
                return;

            // Skip if manually implemented serializers
            if (type.HasMethod(Thunk1) || type.HasMethod(Thunk2))
                return;

            // Generate serializers
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
                    GenerateCallINetworkSerializable(ref context, type, Thunk1, serializeINetworkSerializable);
                    GenerateCallINetworkSerializable(ref context, type, Thunk2, deserializeINetworkSerializable);
                    context.Modified = true;
                }
                else if (isNetworkReplicated)
                {
                    // Generate serialization methods
                    GenerateDotnetSerializer(ref context, type, true, Thunk1);
                    GenerateDotnetSerializer(ref context, type, false, Thunk2);
                    context.Modified = true;
                }
            }
            else if (!isINetworkSerializable && isNetworkReplicated)
            {
                // Generate serialization methods
                var addSerializer = new TypeSerializer();
                addSerializer.Type = type;
                addSerializer.Serialize = GenerateNativeSerializer(ref context, type, true, Thunk1);
                addSerializer.Deserialize = GenerateNativeSerializer(ref context, type, false, Thunk2);
                context.AddSerializers.Add(addSerializer);
                context.Modified = true;
            }
        }

        private static void GenerateCallINetworkSerializable(ref DotnetContext context, TypeDefinition type, string name, MethodDefinition method)
        {
            var m = new MethodDefinition(name, MethodAttributes.Public | MethodAttributes.HideBySig, context.VoidType);
            m.Parameters.Add(new ParameterDefinition("stream", ParameterAttributes.None, type.Module.ImportReference(context.NetworkStreamType)));
            ILProcessor il = m.Body.GetILProcessor();
            il.Emit(OpCodes.Nop);
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, method);
            il.Emit(OpCodes.Nop);
            il.Emit(OpCodes.Ret);
            type.Methods.Add(m);
        }

        private static MethodDefinition GenerateDotnetSerializer(ref DotnetContext context, TypeDefinition type, bool serialize, string name)
        {
            ModuleDefinition module = type.Module;
            var m = new MethodDefinition(name, MethodAttributes.Public | MethodAttributes.HideBySig, context.VoidType);
            m.Parameters.Add(new ParameterDefinition("stream", ParameterAttributes.None, module.ImportReference(context.NetworkStreamType)));
            ILProcessor il = m.Body.GetILProcessor();
            il.Emit(OpCodes.Nop);

            // Serialize base type
            if (type.BaseType != null && type.BaseType.FullName != "System.ValueType" && type.BaseType.FullName != "FlaxEngine.Object" && type.BaseType.CanBeResolved())
            {
                GenerateSerializeCallback(module, il, type.BaseType, serialize);
            }

            if (type.HasGenericParameters) // TODO: implement network replication for generic classes
                MonoCecil.CompilationError($"Not supported generic type '{type.FullName}' for network replication.");

            var ildContext = new DotnetIlContext(il);

            // Serialize all type fields marked with NetworkReplicated attribute
            foreach (FieldDefinition f in type.Fields)
            {
                if (!f.HasAttribute(NetworkReplicatedAttribute))
                    continue;
                GenerateDotnetSerialization(ref context, serialize, ref ildContext, new DotnetValueContext(type, f));
            }

            // Serialize all type properties marked with NetworkReplicated attribute
            foreach (PropertyDefinition p in type.Properties)
            {
                if (!p.HasAttribute(NetworkReplicatedAttribute))
                    continue;
                GenerateDotnetSerialization(ref context, serialize, ref ildContext, new DotnetValueContext(type, p));
            }

            if (serialize)
                il.Emit(OpCodes.Nop);
            il.Emit(OpCodes.Ret);
            type.Methods.Add(m);
            return m;
        }

        private static MethodDefinition GenerateNativeSerializer(ref DotnetContext context, TypeDefinition type, bool serialize, string name)
        {
            ModuleDefinition module = type.Module;
            module.GetType("System.IntPtr", out var intPtrType);
            module.GetType("FlaxEngine.Object", out var scriptingObjectType);
            var fromUnmanagedPtr = scriptingObjectType.Resolve().GetMethod("FromUnmanagedPtr");
            TypeReference intPtr = module.ImportReference(intPtrType);

            var m = new MethodDefinition(name + "Native", MethodAttributes.Public | MethodAttributes.Static | MethodAttributes.HideBySig, context.VoidType);
            m.Parameters.Add(new ParameterDefinition("instancePtr", ParameterAttributes.None, intPtr));
            m.Parameters.Add(new ParameterDefinition("streamPtr", ParameterAttributes.None, intPtr));
            TypeReference networkStream = module.ImportReference(context.NetworkStreamType);
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
            var serializer = GenerateDotnetSerializer(ref context, type, serialize, name);

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

        private static void GenerateSerializeCallback(ModuleDefinition module, ILProcessor il, TypeReference type, bool serialize)
        {
            if (type.IsScriptingObject())
            {
                // NetworkReplicator.InvokeSerializer(typeof(<type>), instance, stream, <serialize>)
                module.ImportReference(type);
                il.Emit(OpCodes.Ldtoken, type);
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

        private struct DotnetIlContext
        {
            public ILProcessor Il;
            public MethodDefinition RPC;
            public Instruction IlStart;
            public int StreamLocalIndex;

            public bool IsRPC => RPC != null;
            public Mono.Collections.Generic.Collection<VariableDefinition> Variables => Il.Body.Variables;

            public bool InitLocals
            {
                get => Il.Body.InitLocals;
                set => Il.Body.InitLocals = value;
            }

            public DotnetIlContext(ILProcessor il, MethodDefinition rpc = null, Instruction ilStart = null)
            {
                Il = il;
                RPC = rpc;
                IlStart = ilStart;
                StreamLocalIndex = -1;
            }

            public Instruction Create(OpCode opcode)
            {
                return Il.Create(opcode);
            }

            public void Emit(Instruction instruction)
            {
                if (IlStart != null)
                    Il.InsertBefore(IlStart, instruction);
                else
                    Il.Append(instruction);
            }

            public void Emit(OpCode opcode)
            {
                Emit(Il.Create(opcode));
            }

            public void Emit(OpCode opcode, TypeReference type)
            {
                Emit(Il.Create(opcode, type));
            }

            public void Emit(OpCode opcode, MethodReference method)
            {
                Emit(Il.Create(opcode, method));
            }

            public void Emit(OpCode opcode, CallSite site)
            {
                Emit(Il.Create(opcode, site));
            }

            public void Emit(OpCode opcode, FieldReference field)
            {
                Emit(Il.Create(opcode, field));
            }

            public void Emit(OpCode opcode, string value)
            {
                Emit(Il.Create(opcode, value));
            }

            public void Emit(OpCode opcode, byte value)
            {
                Emit(Il.Create(opcode, value));
            }

            public void Emit(OpCode opcode, sbyte value)
            {
                Emit(Il.Create(opcode, value));
            }

            public void Emit(OpCode opcode, int value)
            {
                Emit(Il.Create(opcode, value));
            }

            public void Emit(OpCode opcode, long value)
            {
                Emit(Il.Create(opcode, value));
            }

            public void Emit(OpCode opcode, float value)
            {
                Emit(Il.Create(opcode, value));
            }

            public void Emit(OpCode opcode, double value)
            {
                Emit(Il.Create(opcode, value));
            }

            public void Emit(OpCode opcode, Instruction target)
            {
                Emit(Il.Create(opcode, target));
            }

            public void Emit(OpCode opcode, Instruction[] targets)
            {
                Emit(Il.Create(opcode, targets));
            }

            public void Emit(OpCode opcode, VariableDefinition variable)
            {
                Emit(Il.Create(opcode, variable));
            }

            public void Emit(OpCode opcode, ParameterDefinition parameter)
            {
                Emit(Il.Create(opcode, parameter));
            }
        }

        private struct DotnetValueContext
        {
            public TypeReference Type;
            public TypeReference ValueType;
            public FieldReference Field;
            public PropertyDefinition Property;
            public int LocalVarIndex;
            public int ArgIndex;

            public OpCode PropertyGetOpCode
            {
                get
                {
                    var propertyGetOpCode = OpCodes.Call;
                    if (Property != null && Property.GetMethod.IsVirtual)
                        propertyGetOpCode = OpCodes.Callvirt;
                    return propertyGetOpCode;
                }
            }

            public OpCode PropertySetOpCode
            {
                get
                {
                    var propertyGetOpCode = OpCodes.Call;
                    if (Property != null && Property.GetMethod.IsVirtual)
                        propertyGetOpCode = OpCodes.Callvirt;
                    return propertyGetOpCode;
                }
            }

            public DotnetValueContext(TypeReference type, FieldDefinition field)
            {
                Type = type;
                ValueType = field.FieldType;
                Field = field;
                Property = null;
                LocalVarIndex = -1;
                ArgIndex = -1;
            }

            public DotnetValueContext(TypeReference type, PropertyDefinition property)
            {
                Type = type;
                ValueType = property.PropertyType;
                Field = null;
                Property = property;
                LocalVarIndex = -1;
                ArgIndex = -1;
            }

            public DotnetValueContext(TypeReference type, int localVarIndex, TypeReference valueType)
            {
                Type = type;
                ValueType = valueType;
                Field = null;
                Property = null;
                LocalVarIndex = localVarIndex;
                ArgIndex = -1;
            }

            public bool Validate()
            {
                if (Property != null)
                {
                    if (Property.GetMethod == null)
                    {
                        MonoCecil.CompilationError($"Missing getter method for property '{Property.Name}' of type {ValueType.FullName} in {Type.FullName} for automatic replication.", Property);
                        return true;
                    }
                    if (Property.SetMethod == null)
                    {
                        MonoCecil.CompilationError($"Missing setter method for property '{Property.Name}' of type {ValueType.FullName} in {Type.FullName} for automatic replication.", Property);
                        return true;
                    }
                }
                return false;
            }

            public void GetProperty(ref DotnetIlContext il, int propertyVar)
            {
                if (Property != null)
                {
                    // <elementType>[] array = ArrayProperty;
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(PropertyGetOpCode, Property.GetMethod);
                    il.Emit(OpCodes.Stloc, propertyVar);
                    LocalVarIndex = propertyVar;
                }
            }

            public void SetProperty(ref DotnetIlContext il)
            {
                if (Property != null)
                {
                    // ArrayProperty = array
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldloc, LocalVarIndex);
                    il.Emit(PropertySetOpCode, Property.SetMethod);
                }
            }

            public void Load(ref DotnetIlContext il)
            {
                if (ArgIndex != -1)
                {
                    il.Emit(OpCodes.Ldarg, ArgIndex);
                }
                else if (Field != null)
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldfld, Field);
                }
                else if (Property != null && LocalVarIndex == -1)
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(PropertyGetOpCode, Property.GetMethod);
                }
                else
                {
                    il.Emit(OpCodes.Ldloc, LocalVarIndex);
                }
            }

            public void LoadAddress(ref DotnetIlContext il)
            {
                if (ArgIndex != -1)
                {
                    il.Emit(OpCodes.Ldarga, ArgIndex);
                }
                else if (Field != null)
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldflda, Field);
                }
                else if (Property != null && LocalVarIndex == -1)
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(PropertyGetOpCode, Property.GetMethod);
                }
                else
                {
                    il.Emit(OpCodes.Ldloca_S, (byte)LocalVarIndex);
                }
            }

            public void Store(ref DotnetIlContext il)
            {
                if (ArgIndex != -1)
                {
                    il.Emit(OpCodes.Starg, ArgIndex);
                }
                else if (Field != null)
                {
                    il.Emit(OpCodes.Stfld, Field);
                }
                else if (Property != null)
                {
                    il.Emit(PropertySetOpCode, Property.SetMethod);
                }
                else
                {
                    il.Emit(OpCodes.Stloc, LocalVarIndex);
                }
            }
        }

        private static void GenerateDotnetSerialization(ref DotnetContext context, bool serialize, ref DotnetIlContext il, DotnetValueContext valueContext)
        {
            if (valueContext.Validate())
            {
                context.Failed = true;
                return;
            }
            ModuleDefinition module = valueContext.Type.Module;
            TypeDefinition valueTypeDef = valueContext.ValueType.Resolve();
            TypeDefinition networkStreamType = context.NetworkStreamType.Resolve();

            // Ensure to have valid serialization already generated for that value type (eg. when using custom structure field serialization)
            GenerateTypeSerialization(ref context, valueTypeDef);

            if (valueContext.ValueType.IsArray)
            {
                var elementType = valueContext.ValueType.GetElementType();
                var isRawPod = IsRawPOD(elementType); // Whether to use raw memory copy (eg. int, enum, Vector2)
                var varStart = il.Variables.Count;
                module.GetType("System.Int32", out var intType);
                il.Variables.Add(new VariableDefinition(intType)); // [0] int length
                if (isRawPod)
                {
                    il.Variables.Add(new VariableDefinition(new PointerType(elementType))); // [1] <elementType>*
                    il.Variables.Add(new VariableDefinition(new PinnedType(valueContext.ValueType))); // [2] <elementType>[] pinned
                }
                else
                {
                    il.Variables.Add(new VariableDefinition(intType)); // [1] int idx
                    il.Variables.Add(new VariableDefinition(elementType)); // [2] <elementType>
                }

                if (valueContext.Property != null)
                    il.Variables.Add(new VariableDefinition(valueContext.ValueType)); // [3] <elementType>[]
                il.InitLocals = true;
                valueContext.GetProperty(ref il, varStart + 3);
                if (serialize)
                {
                    // <elementType>[] array = Array;
                    il.Emit(OpCodes.Nop);
                    valueContext.Load(ref il);

                    // int length = ((array != null) ? array.Length : 0);
                    il.Emit(OpCodes.Dup);
                    Instruction jmp1 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brtrue_S, jmp1);
                    il.Emit(OpCodes.Pop);
                    il.Emit(OpCodes.Ldc_I4_0);
                    Instruction jmp2 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Br_S, jmp2);
                    il.Emit(jmp1);
                    il.Emit(OpCodes.Ldlen);
                    il.Emit(OpCodes.Conv_I4);
                    il.Emit(jmp2);
                    il.Emit(OpCodes.Stloc, varStart + 0);

                    // stream.WriteInt32(length);
                    if (il.IsRPC)
                        il.Emit(OpCodes.Ldloc, il.StreamLocalIndex);
                    else
                        il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldloc, varStart + 0);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("WriteInt32")));

                    il.Emit(OpCodes.Nop);
                    if (isRawPod)
                    {
                        // fixed (<elementType>* bytes2 = Array)
                        valueContext.Load(ref il);
                        il.Emit(OpCodes.Dup);
                        il.Emit(OpCodes.Stloc, varStart + 2);
                        Instruction jmp3 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Brfalse_S, jmp3);
                        il.Emit(OpCodes.Ldloc, varStart + 2);
                        il.Emit(OpCodes.Ldlen);
                        il.Emit(OpCodes.Conv_I4);
                        Instruction jmp4 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Brtrue_S, jmp4);
                        il.Emit(jmp3);
                        il.Emit(OpCodes.Ldc_I4_0);
                        il.Emit(OpCodes.Conv_U);
                        il.Emit(OpCodes.Stloc, varStart + 1); // <elementType>*
                        Instruction jmp5 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Br_S, jmp5);

                        // stream.WriteBytes((byte*)bytes, length * sizeof(<elementType>)));
                        il.Emit(jmp4);
                        il.Emit(OpCodes.Ldloc, varStart + 2);
                        il.Emit(OpCodes.Ldc_I4_0);
                        il.Emit(OpCodes.Ldelema, elementType);
                        il.Emit(OpCodes.Conv_U);
                        il.Emit(OpCodes.Stloc, varStart + 1); // <elementType>*
                        il.Emit(jmp5);
                        if (il.IsRPC)
                            il.Emit(OpCodes.Ldloc, il.StreamLocalIndex);
                        else
                            il.Emit(OpCodes.Ldarg_1);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // <elementType>*
                        il.Emit(OpCodes.Ldloc, varStart + 0);
                        il.Emit(OpCodes.Sizeof, elementType);
                        il.Emit(OpCodes.Mul);
                        il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("WriteBytes", 2)));
                        il.Emit(OpCodes.Nop);
                        il.Emit(OpCodes.Ldnull);
                        il.Emit(OpCodes.Stloc, varStart + 2);
                    }
                    else
                    {
                        // int idx = 0
                        il.Emit(OpCodes.Ldc_I4_0);
                        il.Emit(OpCodes.Stloc, varStart + 1); // idx
                        Instruction jmp3 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Br_S, jmp3);

                        // <elementType> element = array[idx]
                        Instruction jmp4 = il.Create(OpCodes.Nop);
                        il.Emit(jmp4);
                        valueContext.Load(ref il);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // idx
                        if (elementType.IsValueType)
                            il.Emit(OpCodes.Ldelem_Any, elementType);
                        else
                            il.Emit(OpCodes.Ldelem_Ref);
                        il.Emit(OpCodes.Stloc, varStart + 2); // <elementType>

                        // Serialize item value
                        il.Emit(OpCodes.Nop);
                        GenerateDotnetSerialization(ref context, serialize, ref il, new DotnetValueContext(valueContext.Type, varStart + 2, elementType));

                        // idx++
                        il.Emit(OpCodes.Nop);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // idx
                        il.Emit(OpCodes.Ldc_I4_1);
                        il.Emit(OpCodes.Add);
                        il.Emit(OpCodes.Stloc, varStart + 1); // idx

                        // idx < length
                        il.Emit(jmp3);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // idx
                        il.Emit(OpCodes.Ldloc, varStart + 0); // length
                        il.Emit(OpCodes.Clt);
                        il.Emit(OpCodes.Brtrue_S, jmp4);
                    }
                }
                else
                {
                    // int length = stream.ReadInt32();
                    il.Emit(OpCodes.Nop);
                    if (il.IsRPC)
                        il.Emit(OpCodes.Ldloc_1);
                    else
                        il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("ReadInt32")));
                    il.Emit(OpCodes.Stloc, varStart + 0); // length

                    // System.Array.Resize(ref Array, length);
                    valueContext.LoadAddress(ref il);
                    il.Emit(OpCodes.Ldloc, varStart + 0); // length
                    module.TryGetTypeReference("System.Array", out var arrayType);
                    if (arrayType == null)
                        module.GetType("System.Array", out arrayType);
                    il.Emit(OpCodes.Call, module.ImportReference(arrayType.Resolve().GetMethod("Resize", 2).InflateGeneric(elementType)));

                    il.Emit(OpCodes.Nop);
                    if (isRawPod)
                    {
                        // fixed (<elementType>* buffer = Array)
                        valueContext.Load(ref il);
                        il.Emit(OpCodes.Dup);
                        il.Emit(OpCodes.Stloc, varStart + 2);
                        Instruction jmp1 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Brfalse_S, jmp1);
                        il.Emit(OpCodes.Ldloc, varStart + 2);
                        il.Emit(OpCodes.Ldlen);
                        il.Emit(OpCodes.Conv_I4);
                        Instruction jmp2 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Brtrue_S, jmp2);
                        il.Emit(jmp1);
                        il.Emit(OpCodes.Ldc_I4_0);
                        il.Emit(OpCodes.Conv_U);
                        il.Emit(OpCodes.Stloc, varStart + 1); // <elementType>* buffer
                        Instruction jmp3 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Br_S, jmp3);

                        // stream.ReadBytes((byte*)buffer, length * sizeof(<elementType>));
                        il.Emit(jmp2);
                        il.Emit(OpCodes.Ldloc, varStart + 2);
                        il.Emit(OpCodes.Ldc_I4_0);
                        il.Emit(OpCodes.Ldelema, elementType);
                        il.Emit(OpCodes.Conv_U);
                        il.Emit(OpCodes.Stloc, varStart + 1); // <elementType>* buffer
                        il.Emit(jmp3);
                        if (il.IsRPC)
                            il.Emit(OpCodes.Ldloc_1);
                        else
                            il.Emit(OpCodes.Ldarg_1);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // <elementType>* buffer
                        il.Emit(OpCodes.Ldloc, varStart + 0); // length
                        il.Emit(OpCodes.Sizeof, elementType);
                        il.Emit(OpCodes.Mul);
                        il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("ReadBytes", 2)));
                        il.Emit(OpCodes.Nop);
                        il.Emit(OpCodes.Ldnull);
                        il.Emit(OpCodes.Stloc, varStart + 2);
                    }
                    else
                    {
                        // int idx = 0
                        il.Emit(OpCodes.Ldc_I4_0);
                        il.Emit(OpCodes.Stloc, varStart + 1); // idx
                        Instruction jmp3 = il.Create(OpCodes.Nop);
                        il.Emit(OpCodes.Br_S, jmp3);

                        // Deserialize item value
                        Instruction jmp4 = il.Create(OpCodes.Nop);
                        il.Emit(jmp4);
                        GenerateDotnetSerialization(ref context, serialize, ref il, new DotnetValueContext(valueContext.Type, varStart + 2, elementType));

                        // array[idx] = element
                        il.Emit(OpCodes.Nop);
                        valueContext.Load(ref il);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // idx
                        il.Emit(OpCodes.Ldloc, varStart + 2); // <elementType>
                        if (elementType.IsValueType)
                            il.Emit(OpCodes.Stelem_Any, elementType);
                        else
                            il.Emit(OpCodes.Stelem_Ref);

                        // idx++
                        il.Emit(OpCodes.Nop);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // idx
                        il.Emit(OpCodes.Ldc_I4_1);
                        il.Emit(OpCodes.Add);
                        il.Emit(OpCodes.Stloc, varStart + 1); // idx

                        // idx < length
                        il.Emit(jmp3);
                        il.Emit(OpCodes.Ldloc, varStart + 1); // idx
                        il.Emit(OpCodes.Ldloc, varStart + 0); // length
                        il.Emit(OpCodes.Clt);
                        il.Emit(OpCodes.Brtrue_S, jmp4);
                    }

                    valueContext.SetProperty(ref il);
                }
            }
            else if (_inBuildSerializers.TryGetValue(valueContext.ValueType.FullName, out var serializer))
            {
                // Call NetworkStream method to write/read data
                if (serialize)
                {
                    if (il.IsRPC)
                        il.Emit(OpCodes.Ldloc, il.StreamLocalIndex);
                    else
                        il.Emit(OpCodes.Ldarg_1);
                    valueContext.Load(ref il);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod(serializer.WriteMethod)));
                }
                else
                {
                    if (il.IsRPC)
                    {
                        il.Emit(OpCodes.Ldloc_1);
                    }
                    else
                    {
                        il.Emit(OpCodes.Ldarg_0);
                        il.Emit(OpCodes.Ldarg_1);
                    }
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod(serializer.ReadMethod)));
                    valueContext.Store(ref il);
                }
            }
            else if (valueContext.ValueType.IsScriptingObject())
            {
                // Replicate ScriptingObject as Guid ID
                module.GetType("System.Guid", out var guidType);
                module.GetType("FlaxEngine.Object", out var scriptingObjectType);
                var varStart = il.Variables.Count;
                var reference = module.ImportReference(guidType);
                reference.IsValueType = true; // Fix locals init to have valuetype for Guid instead of class
                il.Variables.Add(new VariableDefinition(reference));
                il.InitLocals = true;
                if (serialize)
                {
                    valueContext.Load(ref il);
                    il.Emit(OpCodes.Dup);
                    Instruction jmp1 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Brtrue_S, jmp1);
                    il.Emit(OpCodes.Pop);
                    il.Emit(OpCodes.Ldsfld, module.ImportReference(guidType.Resolve().GetField("Empty")));
                    Instruction jmp2 = il.Create(OpCodes.Nop);
                    il.Emit(OpCodes.Br_S, jmp2);
                    il.Emit(jmp1);
                    il.Emit(OpCodes.Call, module.ImportReference(scriptingObjectType.Resolve().GetMethod("get_ID")));
                    il.Emit(jmp2);
                    il.Emit(OpCodes.Stloc, varStart);
                    il.Emit(OpCodes.Ldloca_S, (byte)varStart);
                    il.Emit(OpCodes.Call, module.ImportReference(scriptingObjectType.Resolve().GetMethod("RemapObjectID", 1)));
                    if (il.IsRPC)
                        il.Emit(OpCodes.Ldloc, il.StreamLocalIndex);
                    else
                        il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Ldloc, varStart);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("WriteGuid")));
                }
                else
                {
                    module.GetType("System.Type", out var typeType);
                    if (il.IsRPC)
                        il.Emit(OpCodes.Ldloc_1);
                    else
                        il.Emit(OpCodes.Ldarg_1);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("ReadGuid")));
                    il.Emit(OpCodes.Stloc_S, (byte)varStart);
                    if (valueContext.Field != null)
                        il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldloca_S, (byte)varStart);
                    il.Emit(OpCodes.Ldtoken, valueContext.ValueType);
                    il.Emit(OpCodes.Call, module.ImportReference(typeType.Resolve().GetMethod("GetTypeFromHandle")));
                    il.Emit(OpCodes.Call, module.ImportReference(scriptingObjectType.Resolve().GetMethod("TryFind", 2)));
                    il.Emit(OpCodes.Castclass, valueContext.ValueType);
                    valueContext.Store(ref il);
                }
            }
            else if (valueTypeDef.IsEnum)
            {
                // Replicate enum as bits
                // TODO: use smaller uint depending on enum values range
                if (serialize)
                {
                    if (il.IsRPC)
                        il.Emit(OpCodes.Ldloc, il.StreamLocalIndex);
                    else
                        il.Emit(OpCodes.Ldarg_1);
                    valueContext.Load(ref il);
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("WriteUInt32")));
                }
                else
                {
                    if (il.IsRPC)
                    {
                        il.Emit(OpCodes.Ldloc_1);
                    }
                    else
                    {
                        il.Emit(OpCodes.Ldarg_0);
                        il.Emit(OpCodes.Ldarg_1);
                    }
                    il.Emit(OpCodes.Callvirt, module.ImportReference(networkStreamType.GetMethod("ReadUInt32")));
                    valueContext.Store(ref il);
                }
            }
            else if (valueContext.ValueType.IsValueType)
            {
                // Invoke structure generated serializer
                valueContext.LoadAddress(ref il);
                if (il.IsRPC)
                {
                    if (serialize)
                    {
                        il.Emit(OpCodes.Ldloc, il.StreamLocalIndex);
                    }
                    else
                    {
                        il.Emit(OpCodes.Initobj, valueContext.ValueType);
                        valueContext.LoadAddress(ref il);
                        il.Emit(OpCodes.Ldloc_1);
                    }
                }
                else
                {
                    il.Emit(OpCodes.Ldarg_1);
                }
                il.Emit(OpCodes.Call, module.ImportReference(valueTypeDef.GetMethod(serialize ? Thunk1 : Thunk2)));
            }
            else
            {
                // Unknown type
                if (valueContext.Property != null)
                    MonoCecil.CompilationError($"Not supported type '{valueContext.ValueType.FullName}' on {valueContext.Property.Name} in {valueContext.Type.FullName} for automatic replication.", valueContext.Property);
                else if (valueContext.Field != null)
                    MonoCecil.CompilationError($"Not supported type '{valueContext.ValueType.FullName}' on {valueContext.Field.Name} in {valueContext.Type.FullName} for automatic replication.", valueContext.Field.Resolve());
                else if (il.IsRPC)
                    MonoCecil.CompilationError($"Not supported parameter type '{valueContext.ValueType.FullName}' on RPC method {il.RPC.Name} in {valueContext.Type.FullName} for automatic replication.", il.RPC);
                else
                    MonoCecil.CompilationError($"Not supported type '{valueContext.ValueType.FullName}' for automatic replication.");
                context.Failed = true;
            }
        }

        private static void GenerateDotNetRPCBody(ref DotnetContext context, TypeDefinition type, MethodDefinition method, CustomAttribute attribute, TypeReference networkStreamType)
        {
            // Validate RPC usage
            if (method.IsAbstract)
            {
                MonoCecil.CompilationError($"Not supported abstract RPC method '{method.FullName}'.", method);
                context.Failed = true;
                return;
            }
            if (method.IsVirtual)
            {
                MonoCecil.CompilationError($"Not supported virtual RPC method '{method.FullName}'.", method);
                context.Failed = true;
                return;
            }
            if (method.CustomAttributes.FirstOrDefault(x => x.AttributeType.FullName == "System.Runtime.CompilerServices.AsyncStateMachineAttribute") != null)
            {
                MonoCecil.CompilationError($"Not supported async RPC method '{method.FullName}'.", method);
                context.Failed = true;
                return;
            }
            ModuleDefinition module = type.Module;
            var voidType = module.TypeSystem.Void;
            if (method.ReturnType != voidType)
            {
                MonoCecil.CompilationError($"Not supported non-void RPC method '{method.FullName}'.", method);
                context.Failed = true;
                return;
            }
            if (method.IsStatic)
            {
                MonoCecil.CompilationError($"Not supported static RPC method '{method.FullName}'.", method);
                context.Failed = true;
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
                context.Failed = true;
                return;
            }
            if (!methodRPC.IsServer && !methodRPC.IsClient)
            {
                MonoCecil.CompilationError($"Network RPC {method.Name} in {type.FullName} needs to have Server or Client specifier.", method);
                context.Failed = true;
                return;
            }

            module.GetType("System.IntPtr", out var intPtrType);
            module.GetType("FlaxEngine.Object", out var scriptingObjectType);
            var fromUnmanagedPtr = scriptingObjectType.Resolve().GetMethod("FromUnmanagedPtr");
            TypeReference networkStream = module.ImportReference(networkStreamType);
            TypeReference intPtr = module.ImportReference(intPtrType);

            // Generate static method to execute RPC locally
            {
                var m = new MethodDefinition(method.Name + "_Execute", MethodAttributes.Static | MethodAttributes.Assembly | MethodAttributes.HideBySig, voidType);
                m.Parameters.Add(new ParameterDefinition("instancePtr", ParameterAttributes.None, intPtr));
                m.Parameters.Add(new ParameterDefinition("streamPtr", ParameterAttributes.None, intPtr));
                ILProcessor ilp = m.Body.GetILProcessor();
                var il = new DotnetIlContext(ilp, method);
                il.Emit(OpCodes.Nop);
                il.InitLocals = true;

                // <type> instance = (<type>)FlaxEngine.Object.FromUnmanagedPtr(instancePtr)
                il.Variables.Add(new VariableDefinition(type));
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Call, module.ImportReference(fromUnmanagedPtr));
                il.Emit(OpCodes.Castclass, type);
                il.Emit(OpCodes.Stloc_0);

                // NetworkStream stream = (NetworkStream)FlaxEngine.Object.FromUnmanagedPtr(streamPtr)
                il.Variables.Add(new VariableDefinition(networkStream));
                il.Emit(OpCodes.Ldarg_1);
                il.Emit(OpCodes.Call, module.ImportReference(fromUnmanagedPtr));
                il.Emit(OpCodes.Castclass, networkStream);
                il.Emit(OpCodes.Stloc_1);

                // Add locals for each RPC parameter
                var argsStart = il.Variables.Count;
                for (int i = 0; i < method.Parameters.Count; i++)
                {
                    var parameter = method.Parameters[i];
                    if (parameter.IsOut)
                    {
                        MonoCecil.CompilationError($"Network RPC {method.Name} in {type.FullName} parameter {parameter.Name} cannot be 'out'.", method);
                        context.Failed = true;
                        return;
                    }

                    var parameterType = parameter.ParameterType;
                    il.Variables.Add(new VariableDefinition(parameterType));
                }

                // Deserialize parameters from the stream
                for (int i = 0; i < method.Parameters.Count; i++)
                {
                    var parameter = method.Parameters[i];
                    var parameterType = parameter.ParameterType;

                    // Special handling of Rpc Params
                    if (string.Equals(parameterType.FullName, "FlaxEngine.Networking.NetworkRpcParams", StringComparison.OrdinalIgnoreCase))
                    {
                        // new NetworkRpcParams { SenderId = networkStream.SenderId }
                        il.Emit(OpCodes.Ldloca_S, (byte)(argsStart + i));
                        il.Emit(OpCodes.Initobj, parameterType);
                        il.Emit(OpCodes.Ldloca_S, (byte)(argsStart + i));
                        il.Emit(OpCodes.Ldloc_1);
                        var getSenderId = networkStreamType.Resolve().GetMethod("get_SenderId");
                        il.Emit(OpCodes.Callvirt, module.ImportReference(getSenderId));
                        var senderId = parameterType.Resolve().GetField("SenderId");
                        il.Emit(OpCodes.Stfld, module.ImportReference(senderId));
                        continue;
                    }

                    GenerateDotnetSerialization(ref context, false, ref il, new DotnetValueContext(type, argsStart + i, parameterType));
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
                ILProcessor ilp = method.Body.GetILProcessor();
                Instruction ilStart = ilp.Body.Instructions[0];
                var il = new DotnetIlContext(ilp, method, ilStart);
                module.GetType("System.Boolean", out var boolType);
                module.GetType("FlaxEngine.Networking.NetworkManagerMode", out var networkManagerModeType);
                module.GetType("FlaxEngine.Networking.NetworkManager", out var networkManagerType);
                var networkManagerGetMode = networkManagerType.Resolve().GetMethod("get_Mode", 0);
                il.InitLocals = true;
                var varsStart = il.Variables.Count;

                il.Emit(OpCodes.Nop);

                // Is Server/Is Client boolean constants
                il.Variables.Add(new VariableDefinition(module.ImportReference(boolType))); // [0]
                il.Variables.Add(new VariableDefinition(module.ImportReference(boolType))); // [1]
                il.Emit(OpCodes.Ldc_I4, methodRPC.IsServer ? 1 : 0);
                il.Emit(OpCodes.Stloc, varsStart + 0); // isServer loc=0
                il.Emit(OpCodes.Ldc_I4, methodRPC.IsClient ? 1 : 0);
                il.Emit(OpCodes.Stloc, varsStart + 1); // isClient loc=1

                // NetworkManagerMode mode = NetworkManager.Mode;
                il.Variables.Add(new VariableDefinition(module.ImportReference(networkManagerModeType))); // [2]
                il.Emit(OpCodes.Call, module.ImportReference(networkManagerGetMode));
                il.Emit(OpCodes.Stloc, varsStart + 2); // mode loc=2

                // if ((server && networkMode == NetworkManagerMode.Client) || (client && networkMode != NetworkManagerMode.Client))
                var jumpIfBodyStart = ilp.Create(OpCodes.Nop); // if block body
                var jumpIf2Start = ilp.Create(OpCodes.Nop); // 2nd part of the if
                var jumpBodyStart = ilp.Create(OpCodes.Nop); // original method body start
                var jumpBodyEnd = ilp.Body.Instructions.Last(x => x.OpCode == OpCodes.Ret && x.Previous != null);
                if (jumpBodyEnd == null)
                    throw new Exception("Missing IL Return op code in method " + method.Name);
                il.Emit(OpCodes.Ldloc, varsStart + 0);
                il.Emit(OpCodes.Brfalse, jumpIf2Start);
                il.Emit(OpCodes.Ldloc, varsStart + 2);
                il.Emit(OpCodes.Ldc_I4_2);
                il.Emit(OpCodes.Beq, jumpIfBodyStart);
                // ||
                il.Emit(jumpIf2Start);
                il.Emit(OpCodes.Ldloc, varsStart + 1);
                il.Emit(OpCodes.Brfalse, jumpBodyStart);
                il.Emit(OpCodes.Ldloc, varsStart + 2);
                il.Emit(OpCodes.Ldc_I4_2);
                il.Emit(OpCodes.Beq, jumpBodyStart);
                // {
                il.Emit(jumpIfBodyStart);

                // NetworkStream stream = NetworkReplicator.BeginInvokeRPC();
                il.Variables.Add(new VariableDefinition(module.ImportReference(networkStream))); // [3]
                var streamLocalIndex = varsStart + 3;
                il.StreamLocalIndex = streamLocalIndex;
                module.GetType("FlaxEngine.Networking.NetworkReplicator", out var networkReplicatorType);
                var beginInvokeRPC = networkReplicatorType.Resolve().GetMethod("BeginInvokeRPC", 0);
                il.Emit(OpCodes.Call, module.ImportReference(beginInvokeRPC));
                il.Emit(OpCodes.Stloc, streamLocalIndex); // stream loc=3

                // Serialize all RPC parameters
                var targetIdsArgIndex = -1;
                FieldDefinition targetIdsField = null;
                for (int i = 0; i < method.Parameters.Count; i++)
                {
                    var parameter = method.Parameters[i];
                    var parameterType = parameter.ParameterType;

                    // Special handling of Rpc Params
                    if (string.Equals(parameterType.FullName, "FlaxEngine.Networking.NetworkRpcParams", StringComparison.OrdinalIgnoreCase))
                    {
                        targetIdsArgIndex = i + 1; // NetworkRpcParams value argument index (starts at 1, 0 holds this)
                        targetIdsField = parameterType.Resolve().GetField("TargetIds");
                        continue;
                    }

                    GenerateDotnetSerialization(ref context, true, ref il, new DotnetValueContext(type, -1, parameterType) { ArgIndex = i + 1 });
                }

                // NetworkReplicator.EndInvokeRPC(this, typeof(<type>), "<name>", stream, targetIds);
                il.Emit(OpCodes.Nop);
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Ldtoken, type);
                module.GetType("System.Type", out var typeType);
                var getTypeFromHandle = typeType.Resolve().GetMethod("GetTypeFromHandle");
                il.Emit(OpCodes.Call, module.ImportReference(getTypeFromHandle));
                il.Emit(OpCodes.Ldstr, method.Name);
                il.Emit(OpCodes.Ldloc, streamLocalIndex);
                if (targetIdsArgIndex != -1)
                {
                    il.Emit(OpCodes.Ldarg, targetIdsArgIndex);
                    il.Emit(OpCodes.Ldfld, module.ImportReference(targetIdsField));
                }
                else
                    il.Emit(OpCodes.Ldnull);
                var endInvokeRPC = networkReplicatorType.Resolve().GetMethod("EndInvokeRPC", 5);
                if (endInvokeRPC.ReturnType.FullName != boolType.FullName)
                    throw new Exception("Invalid EndInvokeRPC return type. Remove any 'Binaries' folders to force project recompile.");
                il.Emit(OpCodes.Call, module.ImportReference(endInvokeRPC));

                // if (EndInvokeRPC) return
                il.Emit(OpCodes.Brtrue, jumpBodyEnd);

                // Continue to original method body
                il.Emit(jumpBodyStart);
            }

            context.MethodRPCs.Add(methodRPC);
        }
    }
}

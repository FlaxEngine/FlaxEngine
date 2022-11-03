// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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

        internal const string NetworkReplicated = "NetworkReplicated";
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
            if (tag.Tag != NetworkReplicated)
                return;

            // Mark member as replicated
            valid = true;
            memberInfo.SetTag(NetworkReplicated, string.Empty);
        }
        
        private void OnGenerateCppTypeInternals(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents)
        {
            // Skip modules that don't use networking
            var module = BindingsGenerator.CurrentModule;
            if (module.GetTag(NetworkReplicated) == null)
                return;

            // Check if type uses automated network replication
            List<FieldInfo> fields = null;
            List<PropertyInfo> properties = null;
            if (typeInfo is ClassInfo classInfo)
            {
                fields = classInfo.Fields;
                properties = classInfo.Properties;
            }
            else if (typeInfo is StructureInfo structInfo)
            {
                fields = structInfo.Fields;
            }
            bool useReplication = false;
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
            if (!useReplication)
                return;

            typeInfo.SetTag(NetworkReplicated, string.Empty);
            
            // Generate C++ wrapper functions to serialize/deserialize type
            BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkReplicator.h");
            BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkStream.h");
            OnGenerateCppTypeSerialize(buildData, typeInfo, contents, fields, properties, true);
            OnGenerateCppTypeSerialize(buildData, typeInfo, contents, fields, properties, false);
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
            if (typeInfo.GetTag(NetworkReplicated) == null)
                return;
            var typeNameNative = typeInfo.FullNameNative;
            var typeNameInternal = typeInfo.FullNameNativeInternal;

            // Register generated serializer functions
            contents.AppendLine($"        NetworkReplicator::AddSerializer(ScriptingTypeHandle({typeNameNative}::TypeInitializer), {typeNameInternal}Internal::INetworkSerializable_Serialize, {typeNameInternal}Internal::INetworkSerializable_Deserialize);");
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
            // Skip assemblies not using netowrking
            if (!binaryModule.Any(module => module.Tags.ContainsKey(NetworkReplicated)))
                return;

            // Generate netoworking code inside assembly after it's being compiled
            var assemblyPath = buildTask.ProducedFiles[0];
            var task = graph.Add<Task>();
            task.ProducedFiles.Add(assemblyPath);
            task.WorkingDirectory = buildTask.WorkingDirectory;
            task.Command = () => OnPatchAssembly(buildData, buildOptions, buildTask, assemblyPath);
            task.CommandPath = null;
            task.InfoMessage = $"Generating netowrking code for {Path.GetFileName(assemblyPath)}...";
            task.Cost = 50;
            task.DisableCache = true;
            task.DependentTasks = new HashSet<Task>();
            task.DependentTasks.Add(buildTask);
        }

        private void OnPatchAssembly(Builder.BuildData buildData, NativeCpp.BuildOptions buildOptions, Task buildTask, string assemblyPath)
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
                foreach (TypeDefinition type in module.Types)
                {
                    if (type.IsInterface || type.IsEnum)
                        continue;
                    var isNative = type.HasAttribute("FlaxEngine.UnmanagedAttribute");
                    if (isNative)
                        continue;
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
                        if (!f.HasAttribute("FlaxEngine.NetworkReplicatedAttribute"))
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
                if (addSerializers.Count != 0)
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
                if (!f.HasAttribute("FlaxEngine.NetworkReplicatedAttribute"))
                    continue;
                GenerateSerializerType(type, serialize, ref failed, f, f.FieldType, il, networkStream);
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

        private static void GenerateSerializerType(TypeDefinition type, bool serialize, ref bool failed, FieldReference field, TypeReference valueType, ILProcessor il, TypeDefinition networkStreamType)
        {
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
                    il.Emit(OpCodes.Ldfld, field);
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
                    il.Emit(OpCodes.Stfld, field);
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
                    il.Emit(OpCodes.Ldfld, field);
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
                    var m = networkStreamType.GetMethod("WriteGuid");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
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
                    il.Emit(OpCodes.Stfld, field);
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
                    il.Emit(OpCodes.Ldfld, field);
                    var m = networkStreamType.GetMethod("WriteUInt32");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                }
                else
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldarg_1);
                    var m = networkStreamType.GetMethod("ReadUInt32");
                    il.Emit(OpCodes.Callvirt, module.ImportReference(m));
                    il.Emit(OpCodes.Stfld, field);
                }
            }
            else if (valueType.IsValueType)
            {
                // Invoke structure generated serializer
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Ldflda, field);
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
                    il.Emit(OpCodes.Ldfld, field);

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
                    il.Emit(OpCodes.Ldfld, field);
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
                Log.Error($"Not supported type '{valueType.FullName}' on {field.Name} in {type.FullName} for automatic replication.");
                failed = true;
            }
        }
    }
}

// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Text;
using System.Collections.Generic;
using Flax.Build.Bindings;

namespace Flax.Build.Plugins
{
    /// <summary>
    /// Flax.Build plugin for Networking extenrions support. Generates required bindings glue code for automatic types replication and RPCs invoking. 
    /// </summary>
    /// <seealso cref="Flax.Build.Plugin" />
    internal sealed class NetworkingPlugin : Plugin
    {
        internal const string NetworkReplicated = "NetworkReplicated";

        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            BindingsGenerator.ParseMemberTag += OnParseMemberTag;
            BindingsGenerator.GenerateCppTypeInternals += OnGenerateCppTypeInternals;
            BindingsGenerator.GenerateCppTypeInitRuntime += OnGenerateCppTypeInitRuntime;
        }

        private void OnParseMemberTag(ref bool valid, BindingsGenerator.TagParameter tag, MemberInfo memberInfo)
        {
            if (tag.Tag != NetworkReplicated)
                return;
            if (memberInfo is FieldInfo)
            {
                // Mark member as replicated
                valid = true;
                memberInfo.SetTag(NetworkReplicated, string.Empty);
            }
        }
        
        private void OnGenerateCppTypeInternals(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents)
        {
            // Skip modules that don't use networking
            var module = BindingsGenerator.CurrentModule;
            if (module.GetTag(NetworkReplicated) == null)
                return;

            // Check if type uses automated network replication
            List<FieldInfo> fields = null;
            if (typeInfo is ClassInfo classInfo)
            {
                fields = classInfo.Fields;
                // TODO: add support for class properties replication
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
            if (!useReplication)
                return;

            typeInfo.SetTag(NetworkReplicated, string.Empty);
            
            // Generate C++ wrapper functions to serialize/deserialize type
            BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkReplicator.h");
            BindingsGenerator.CppIncludeFiles.Add("Engine/Networking/NetworkStream.h");
            OnGenerateCppTypeSerialize(buildData, typeInfo, contents, fields, true);
            OnGenerateCppTypeSerialize(buildData, typeInfo, contents, fields, false);
        }

        private void OnGenerateCppTypeSerialize(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents, List<FieldInfo> fields, bool serialize)
        {
            var thunk1 = "INetworkSerializable_Serialize";
            var thunk2 = "INetworkSerializable_Deserialize";
            contents.Append("    static void ").Append(serialize ? thunk1 : thunk2).AppendLine("(void* instance, NetworkStream* stream)");
            contents.AppendLine("    {");
            contents.AppendLine($"        {typeInfo.NativeName}& object = *({typeInfo.NativeName}*)instance;");
            if (IsRawPOD(buildData, typeInfo))
            {
                // POD types as raw bytes
                OnGenerateCppWriteRaw(contents, "object", serialize);
            }
            else
            {
                if (typeInfo is ClassStructInfo classStructInfo && classStructInfo.BaseType != null && classStructInfo.BaseType.Name != "ScriptingObject")
                {
                    // Replicate base type
                    OnGenerateCppWriteSerializer(contents, classStructInfo.BaseType.NativeName, "object", serialize);
                }
                
                // Replicate all marked fields
                if (fields != null)
                {
                    foreach (var fieldInfo in fields)
                    {
                        if (fieldInfo.GetTag(NetworkReplicated) == null)
                            continue;
                        OnGenerateCppTypeSerializeData(buildData, typeInfo, contents, fieldInfo.Type, $"object.{fieldInfo.Name}", serialize);
                    }
                }
                // TODO: add support for class properties replication
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
                OnGenerateCppWriteRaw(contents, "&" + name, serialize);
            }
            else if (apiType.IsScriptingObject)
            {
                // Object ID
                if (serialize)
                {
                    contents.AppendLine($"        {{Guid id = {name} ? {name}->GetID() : Guid::Empty;");
                    OnGenerateCppWriteRaw(contents, "&id", serialize);
                    contents.AppendLine("        }");
                }
                else
                {
                    contents.AppendLine($"        {{Guid id;");
                    OnGenerateCppWriteRaw(contents, "&id", serialize);
                    contents.AppendLine($"        {name} = Scripting::TryFindObject(id);}}");
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
            var comp = serialize ? "First" : "Second";
            contents.AppendLine($"        {{const auto serializer = NetworkReplicator::GetSerializer({type}::TypeInitializer);");
            contents.AppendLine($"        if (serializer.{comp})");
            contents.AppendLine($"            serializer.{comp}(&{data}, stream);}}");
        }

        private void OnGenerateCppTypeInitRuntime(Builder.BuildData buildData, ApiTypeInfo typeInfo, StringBuilder contents)
        {
            if (typeInfo.GetTag(NetworkReplicated) == null)
                return;
            var typeNameNative = typeInfo.FullNameNative;
            var typeNameInternal = typeInfo.FullNameNativeInternal;

            // Register generated serializer functions
            contents.AppendLine($"        NetworkReplicator::SerializersTable[ScriptingTypeHandle({typeNameNative}::TypeInitializer)] = NetworkReplicator::SerializeFuncPair({typeNameInternal}Internal::INetworkSerializable_Serialize, {typeNameInternal}Internal::INetworkSerializable_Deserialize);");
        }
    }
}

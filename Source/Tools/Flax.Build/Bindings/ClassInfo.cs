// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class information for bindings generator.
    /// </summary>
    public class ClassInfo : ClassStructInfo
    {
        private static readonly HashSet<string> InBuildScriptingObjectTypes = new HashSet<string>
        {
            "ScriptingObject",
            "ManagedScriptingObject",
            "PersistentScriptingObject",
            "BinaryAsset",
            "SceneObject",
            "Asset",
            "Script",
            "Actor",
        };

        public bool IsBaseTypeHidden;
        public bool IsStatic;
        public bool IsSealed;
        public bool IsAbstract;
        public bool IsAutoSerialization;
        public bool NoSpawn;
        public bool NoConstructor;
        public List<FunctionInfo> Functions = new List<FunctionInfo>();
        public List<PropertyInfo> Properties = new List<PropertyInfo>();
        public List<FieldInfo> Fields = new List<FieldInfo>();
        public List<EventInfo> Events = new List<EventInfo>();

        internal HashSet<string> UniqueFunctionNames;

        private bool _isScriptingObject;
        private int _scriptVTableSize = -1;
        private int _scriptVTableOffset;

        public override bool IsClass => true;

        public override bool IsScriptingObject => _isScriptingObject;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            // Internal base types are usually hidden from bindings (used in core-only internally)
            IsBaseTypeHidden = BaseTypeInheritance == AccessLevel.Private || BaseType == null;

            // Cache if it it Scripting Object type
            if (InBuildScriptingObjectTypes.Contains(Name))
                _isScriptingObject = true;
            else if (BaseType == null)
                _isScriptingObject = false;
            else if (InBuildScriptingObjectTypes.Contains(BaseType.Type))
                _isScriptingObject = true;
            else
            {
                var baseApiTypeInfo = BindingsGenerator.FindApiTypeInfo(buildData, BaseType, this);
                if (baseApiTypeInfo != null)
                {
                    if (!baseApiTypeInfo.IsInited)
                        baseApiTypeInfo.Init(buildData);
                    _isScriptingObject = baseApiTypeInfo.IsScriptingObject;
                }
                else
                {
                    _isScriptingObject = false;
                }
            }
        }

        public override void Write(BinaryWriter writer)
        {
            // TODO: convert into flags
            writer.Write(IsStatic);
            writer.Write(IsSealed);
            writer.Write(IsAbstract);
            writer.Write(IsAutoSerialization);
            writer.Write(NoSpawn);
            writer.Write(NoConstructor);
            BindingsGenerator.Write(writer, Functions);
            BindingsGenerator.Write(writer, Properties);
            BindingsGenerator.Write(writer, Fields);
            BindingsGenerator.Write(writer, Events);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            // TODO: convert into flags
            IsStatic = reader.ReadBoolean();
            IsSealed = reader.ReadBoolean();
            IsAbstract = reader.ReadBoolean();
            IsAutoSerialization = reader.ReadBoolean();
            NoSpawn = reader.ReadBoolean();
            NoConstructor = reader.ReadBoolean();
            Functions = BindingsGenerator.Read(reader, Functions);
            Properties = BindingsGenerator.Read(reader, Properties);
            Fields = BindingsGenerator.Read(reader, Fields);
            Events = BindingsGenerator.Read(reader, Events);

            base.Read(reader);
        }

        public int GetScriptVTableSize(Builder.BuildData buildData, out int offset)
        {
            if (_scriptVTableSize == -1)
            {
                if (BindingsGenerator.FindApiTypeInfo(buildData, BaseType, this) is ClassInfo baseApiTypeInfo)
                {
                    _scriptVTableOffset = baseApiTypeInfo.GetScriptVTableSize(buildData, out _);
                }
                _scriptVTableSize = _scriptVTableOffset + Functions.Count(x => x.IsVirtual);
            }
            offset = _scriptVTableOffset;
            return _scriptVTableSize;
        }

        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            apiTypeInfo.Namespace = null;

            base.AddChild(apiTypeInfo);
        }

        public override string ToString()
        {
            return "class " + Name;
        }
    }
}

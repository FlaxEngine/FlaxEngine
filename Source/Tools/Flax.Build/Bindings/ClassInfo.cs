// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class information for bindings generator.
    /// </summary>
    public class ClassInfo : VirtualClassInfo
    {
        private static readonly HashSet<string> InBuildScriptingObjectTypes = new HashSet<string>
        {
            "ScriptingObject",
            "ManagedScriptingObject",
            "PersistentScriptingObject",
            "ScriptingObjectReference",
            "AssetReference",
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
        public List<PropertyInfo> Properties = new List<PropertyInfo>();
        public List<FieldInfo> Fields = new List<FieldInfo>();
        public List<EventInfo> Events = new List<EventInfo>();

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
            else if (InBuildScriptingObjectTypes.Contains(BaseType.Name))
                _isScriptingObject = true;
            else
                _isScriptingObject = BaseType != null && BaseType.IsScriptingObject;

            if (UniqueFunctionNames == null)
                UniqueFunctionNames = new HashSet<string>();

            if (BaseType is ClassInfo baseClass)
            {
                if (baseClass.IsStatic)
                    throw new Exception(string.Format("Class {0} inherits from the class {1} that is {2}.", FullNameNative, baseClass.FullNameNative, "static"));
                if (baseClass.IsSealed)
                    throw new Exception(string.Format("Class {0} inherits from the class {1} that is {2}.", FullNameNative, baseClass.FullNameNative, "sealed"));
            }

            foreach (var fieldInfo in Fields)
            {
                if (fieldInfo.IsHidden)
                    continue;

                fieldInfo.Getter = new FunctionInfo
                {
                    Name = "Get" + fieldInfo.Name,
                    Comment = fieldInfo.Comment,
                    IsStatic = fieldInfo.IsStatic,
                    Access = fieldInfo.Access,
                    Attributes = fieldInfo.Attributes,
                    ReturnType = fieldInfo.Type,
                    Parameters = new List<FunctionInfo.ParameterInfo>(),
                    IsVirtual = false,
                    IsConst = true,
                    Glue = new FunctionInfo.GlueInfo()
                };
                ProcessAndValidate(fieldInfo.Getter);
                fieldInfo.Getter.Name = fieldInfo.Name;

                if (!fieldInfo.IsReadOnly)
                {
                    fieldInfo.Setter = new FunctionInfo
                    {
                        Name = "Set" + fieldInfo.Name,
                        Comment = fieldInfo.Comment,
                        IsStatic = fieldInfo.IsStatic,
                        Access = fieldInfo.Access,
                        Attributes = fieldInfo.Attributes,
                        ReturnType = new TypeInfo
                        {
                            Type = "void",
                        },
                        Parameters = new List<FunctionInfo.ParameterInfo>
                        {
                            new FunctionInfo.ParameterInfo
                            {
                                Name = "value",
                                Type = fieldInfo.Type,
                            },
                        },
                        IsVirtual = false,
                        IsConst = true,
                        Glue = new FunctionInfo.GlueInfo()
                    };
                    ProcessAndValidate(fieldInfo.Setter);
                    fieldInfo.Setter.Name = fieldInfo.Name;
                }
            }

            foreach (var propertyInfo in Properties)
            {
                if (propertyInfo.Getter != null)
                    ProcessAndValidate(propertyInfo.Getter);
                if (propertyInfo.Setter != null)
                    ProcessAndValidate(propertyInfo.Setter);
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
            Properties = BindingsGenerator.Read(reader, Properties);
            Fields = BindingsGenerator.Read(reader, Fields);
            Events = BindingsGenerator.Read(reader, Events);

            base.Read(reader);
        }

        public override int GetScriptVTableSize(out int offset)
        {
            if (_scriptVTableSize == -1)
            {
                if (BaseType is ClassInfo baseApiTypeInfo)
                {
                    _scriptVTableOffset = baseApiTypeInfo.GetScriptVTableSize(out _);
                }
                if (Interfaces != null)
                {
                    foreach (var interfaceInfo in Interfaces)
                    {
                        if (interfaceInfo.Access != AccessLevel.Public)
                            continue;
                        _scriptVTableOffset += interfaceInfo.GetScriptVTableSize(out _);
                    }
                }
                _scriptVTableSize = _scriptVTableOffset + Functions.Count(x => x.IsVirtual);
                if (IsSealed)
                {
                    // Skip vtables for sealed classes
                    _scriptVTableSize = _scriptVTableOffset = 0;
                }
            }
            offset = _scriptVTableOffset;
            return _scriptVTableSize;
        }

        public override int GetScriptVTableOffset(VirtualClassInfo classInfo)
        {
            if (classInfo == BaseType)
                return 0;
            if (Interfaces != null)
            {
                var offset = BaseType is ClassInfo baseApiTypeInfo ? baseApiTypeInfo.GetScriptVTableSize(out _) : 0;
                foreach (var interfaceInfo in Interfaces)
                {
                    if (interfaceInfo.Access != AccessLevel.Public)
                        continue;
                    if (interfaceInfo == classInfo)
                        return offset;
                    offset += interfaceInfo.GetScriptVTableSize(out _);
                }
            }
            throw new Exception($"Cannot get Script VTable offset for {classInfo} that is not part of {this}");
        }

        public override string ToString()
        {
            return "class " + Name;
        }
    }
}

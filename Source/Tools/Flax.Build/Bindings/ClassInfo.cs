// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
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
            else if (InBuildScriptingObjectTypes.Contains(BaseType.Name))
                _isScriptingObject = true;
            else
                _isScriptingObject = BaseType != null && BaseType.IsScriptingObject;

            if (UniqueFunctionNames == null)
                UniqueFunctionNames = new HashSet<string>();

            if (BaseType is ClassInfo baseClass)
            {
                if (baseClass.IsStatic)
                    throw new Exception(string.Format("Class {0} inherits from thr class {1} that is {2}.", FullNameNative, baseClass.FullNameNative, "static"));
                if (baseClass.IsSealed)
                    throw new Exception(string.Format("Class {0} inherits from the class {1} that is {2}.", FullNameNative, baseClass.FullNameNative, "sealed"));
            }

            foreach (var fieldInfo in Fields)
            {
                if (fieldInfo.Access == AccessLevel.Private)
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

            foreach (var functionInfo in Functions)
                ProcessAndValidate(functionInfo);
        }

        private void ProcessAndValidate(FunctionInfo functionInfo)
        {
            // Ensure that methods have unique names for bindings
            if (UniqueFunctionNames == null)
                UniqueFunctionNames = new HashSet<string>();
            int idx = 1;
            functionInfo.UniqueName = functionInfo.Name;
            while (UniqueFunctionNames.Contains(functionInfo.UniqueName))
                functionInfo.UniqueName = functionInfo.Name + idx++;
            UniqueFunctionNames.Add(functionInfo.UniqueName);
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
                if (BaseType is ClassInfo baseApiTypeInfo)
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

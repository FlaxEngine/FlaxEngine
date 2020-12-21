// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class information for bindings generator.
    /// </summary>
    public class ClassInfo : ApiTypeInfo
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

        public AccessLevel Access;
        public TypeInfo BaseType;
        public AccessLevel BaseTypeInheritance;
        public bool IsBaseTypeHidden;
        public bool IsStatic;
        public bool IsSealed;
        public bool IsAbstract;
        public bool IsAutoSerialization;
        public bool NoSpawn;
        public bool NoConstructor;
        public List<FunctionInfo> Functions;
        public List<PropertyInfo> Properties;
        public List<FieldInfo> Fields;
        public List<EventInfo> Events;

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
            IsBaseTypeHidden = BaseTypeInheritance == AccessLevel.Private || BaseType.Type == "ISerializable";

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

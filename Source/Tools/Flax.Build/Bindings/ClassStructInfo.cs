// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class/structure information for bindings generator.
    /// </summary>
    public abstract class ClassStructInfo : ApiTypeInfo
    {
        public AccessLevel Access;
        public AccessLevel BaseTypeInheritance;
        public bool IsTemplate;
        public ClassStructInfo BaseType;
        public List<InterfaceInfo> Interfaces;
        public List<TypeInfo> Inheritance; // Data from parsing, used to interfaces and base type construct in Init
        public List<FunctionInfo> Functions = new List<FunctionInfo>();

        public override bool SkipGeneration => IsInBuild || IsTemplate;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            if (BaseType == null && Interfaces == null && Inheritance != null)
            {
                // Extract base class and interfaces from inheritance info
                for (int i = 0; i < Inheritance.Count; i++)
                {
                    var apiTypeInfo = BindingsGenerator.FindApiTypeInfo(buildData, Inheritance[i], Parent);
                    if (apiTypeInfo is InterfaceInfo interfaceInfo)
                    {
                        if (Interfaces == null)
                            Interfaces = new List<InterfaceInfo>();
                        Interfaces.Add(interfaceInfo);
                    }
                    else if (apiTypeInfo is ClassStructInfo otherInfo)
                    {
                        if (otherInfo == this)
                            throw new Exception($"Type '{Name}' inherits from itself.");
                        if (BaseType != null)
                            throw new Exception($"Invalid '{Name}' inheritance (only single base class is allowed for scripting types, excluding interfaces).");
                        BaseType = otherInfo;
                    }
                }
            }
            BaseType?.EnsureInited(buildData);
        }

        public override void Write(BinaryWriter writer)
        {
            writer.Write((byte)Access);
            writer.Write((byte)BaseTypeInheritance);
            writer.Write(IsTemplate);
            BindingsGenerator.Write(writer, BaseType);
            BindingsGenerator.Write(writer, Inheritance);
            BindingsGenerator.Write(writer, Functions);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Access = (AccessLevel)reader.ReadByte();
            BaseTypeInheritance = (AccessLevel)reader.ReadByte();
            IsTemplate = reader.ReadBoolean();
            BaseType = BindingsGenerator.Read(reader, BaseType);
            Inheritance = BindingsGenerator.Read(reader, Inheritance);
            Functions = BindingsGenerator.Read(reader, Functions);

            base.Read(reader);
        }
    }

    /// <summary>
    /// The native class or interface information for bindings generator that contains virtual functions.
    /// </summary>
    public abstract class VirtualClassInfo : ClassStructInfo
    {
        internal HashSet<string> UniqueFunctionNames;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            foreach (var functionInfo in Functions)
                ProcessAndValidate(functionInfo);
        }

        protected void ProcessAndValidate(FunctionInfo functionInfo)
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

        public abstract int GetScriptVTableSize(out int offset);

        public abstract int GetScriptVTableOffset(VirtualClassInfo classInfo);

        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            apiTypeInfo.Namespace = null;

            base.AddChild(apiTypeInfo);
        }
    }
}

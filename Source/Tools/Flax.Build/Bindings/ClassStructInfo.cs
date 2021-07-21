// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        public ClassStructInfo BaseType;
        public List<InterfaceInfo> Interfaces;
        public List<TypeInfo> Inheritance; // Data from parsing, used to interfaces and base type construct in Init

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
            BindingsGenerator.Write(writer, BaseType);
            BindingsGenerator.Write(writer, Inheritance);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Access = (AccessLevel)reader.ReadByte();
            BaseTypeInheritance = (AccessLevel)reader.ReadByte();
            BaseType = BindingsGenerator.Read(reader, BaseType);
            Inheritance = BindingsGenerator.Read(reader, Inheritance);

            base.Read(reader);
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native property information for bindings generator.
    /// </summary>
    public class PropertyInfo : MemberInfo
    {
        public TypeInfo Type;
        public FunctionInfo Getter;
        public FunctionInfo Setter;

        public override void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, Type);
            BindingsGenerator.Write(writer, Getter);
            BindingsGenerator.Write(writer, Setter);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Type = BindingsGenerator.Read(reader, Type);
            Getter = BindingsGenerator.Read(reader, Getter);
            Setter = BindingsGenerator.Read(reader, Setter);

            base.Read(reader);
        }

        public override string ToString()
        {
            return Type + " " + Name;
        }
    }
}

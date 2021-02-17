// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native member information for bindings generator.
    /// </summary>
    public class MemberInfo : IBindingsCache
    {
        public string Name;
        public string[] Comment;
        public bool IsStatic;
        public AccessLevel Access;
        public string Attributes;

        public bool HasAttribute(string name)
        {
            return Attributes != null && Attributes.Contains(name);
        }

        public virtual void Write(BinaryWriter writer)
        {
            writer.Write(Name);
            BindingsGenerator.Write(writer, Comment);
            writer.Write(IsStatic);
            writer.Write((byte)Access);
            BindingsGenerator.Write(writer, Attributes);
        }

        public virtual void Read(BinaryReader reader)
        {
            Name = reader.ReadString();
            Comment = BindingsGenerator.Read(reader, Comment);
            IsStatic = reader.ReadBoolean();
            Access = (AccessLevel)reader.ReadByte();
            Attributes = BindingsGenerator.Read(reader, Attributes);
        }
    }
}

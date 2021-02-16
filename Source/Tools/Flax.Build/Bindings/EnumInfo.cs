// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native enumeration information for bindings generator.
    /// </summary>
    public class EnumInfo : ApiTypeInfo
    {
        public struct EntryInfo : IBindingsCache
        {
            public string Name;
            public string[] Comment;
            public string Value;
            public string Attributes;

            public override string ToString()
            {
                return Name + (string.IsNullOrEmpty(Value) ? string.Empty : " = " + Value);
            }

            public void Write(BinaryWriter writer)
            {
                writer.Write(Name);
                BindingsGenerator.Write(writer, Comment);
                BindingsGenerator.Write(writer, Value);
                BindingsGenerator.Write(writer, Attributes);
            }

            public void Read(BinaryReader reader)
            {
                Name = reader.ReadString();
                Comment = BindingsGenerator.Read(reader, Comment);
                Value = BindingsGenerator.Read(reader, Value);
                Attributes = BindingsGenerator.Read(reader, Attributes);
            }
        }

        public AccessLevel Access;
        public TypeInfo UnderlyingType;
        public List<EntryInfo> Entries = new List<EntryInfo>();

        public override bool IsValueType => true;
        public override bool IsEnum => true;
        public override bool IsPod => true;

        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            throw new NotSupportedException("API enums cannot have sub-types.");
        }

        public override void Write(BinaryWriter writer)
        {
            writer.Write((byte)Access);
            BindingsGenerator.Write(writer, UnderlyingType);
            BindingsGenerator.Write(writer, Entries);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Access = (AccessLevel)reader.ReadByte();
            UnderlyingType = BindingsGenerator.Read(reader, UnderlyingType);
            Entries = BindingsGenerator.Read(reader, Entries);

            base.Read(reader);
        }

        public override string ToString()
        {
            return "enum " + Name;
        }
    }
}

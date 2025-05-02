// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
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
        public bool IsConstexpr;
        public string DeprecatedMessage;
        public bool IsHidden;
        public AccessLevel Access;
        public string Attributes;
        public Dictionary<string, string> Tags;

        public virtual bool IsDeprecated => DeprecatedMessage != null;

        public bool HasAttribute(string name)
        {
            return Attributes != null && Attributes.Contains(name);
        }

        public string GetTag(string tag)
        {
            if (Tags != null && Tags.TryGetValue(tag, out var value))
                return value;
            return null;
        }

        public void SetTag(string tag, string value)
        {
            if (Tags == null)
                Tags = new Dictionary<string, string>();
            Tags[tag] = value;
        }


        public virtual void Write(BinaryWriter writer)
        {
            writer.Write(Name);
            BindingsGenerator.Write(writer, Comment);
            writer.Write(IsStatic);
            writer.Write(IsConstexpr);
            BindingsGenerator.Write(writer, DeprecatedMessage);
            writer.Write(IsHidden);
            writer.Write((byte)Access);
            BindingsGenerator.Write(writer, Attributes);
            BindingsGenerator.Write(writer, Tags);
        }

        public virtual void Read(BinaryReader reader)
        {
            Name = reader.ReadString();
            Comment = BindingsGenerator.Read(reader, Comment);
            IsStatic = reader.ReadBoolean();
            IsConstexpr = reader.ReadBoolean();
            DeprecatedMessage = BindingsGenerator.Read(reader, DeprecatedMessage);
            IsHidden = reader.ReadBoolean();
            Access = (AccessLevel)reader.ReadByte();
            Attributes = BindingsGenerator.Read(reader, Attributes);
            Tags = BindingsGenerator.Read(reader, Tags);
        }
    }
}

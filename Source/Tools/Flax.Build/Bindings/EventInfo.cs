// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native event information for bindings generator.
    /// </summary>
    public class EventInfo : MemberInfo
    {
        public TypeInfo Type;

        public override void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, Type);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Type = BindingsGenerator.Read(reader, Type);

            base.Read(reader);
        }

        public override string ToString()
        {
            var result = string.Empty;
            if (IsStatic)
                result += "static ";
            result += Type + " " + Name;
            return result;
        }
    }
}

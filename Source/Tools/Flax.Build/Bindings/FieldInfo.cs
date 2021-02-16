// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native field information for bindings generator.
    /// </summary>
    public class FieldInfo : MemberInfo
    {
        public TypeInfo Type;
        public bool IsReadOnly;
        public bool NoArray;
        public FunctionInfo Getter;
        public FunctionInfo Setter;
        public string DefaultValue;

        public bool HasDefaultValue => !string.IsNullOrEmpty(DefaultValue);

        public override void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, Type);
            // TODO: convert into flags
            writer.Write(IsReadOnly);
            writer.Write(NoArray);
            BindingsGenerator.Write(writer, DefaultValue);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Type = BindingsGenerator.Read(reader, Type);
            // TODO: convert into flags
            IsReadOnly = reader.ReadBoolean();
            NoArray = reader.ReadBoolean();
            DefaultValue = BindingsGenerator.Read(reader, DefaultValue);

            base.Read(reader);
        }

        public override string ToString()
        {
            var result = string.Empty;
            if (IsStatic)
                result += "static ";
            result += Type + " " + Name;
            if (HasDefaultValue)
                result += " = " + DefaultValue;
            return result;
        }
    }
}

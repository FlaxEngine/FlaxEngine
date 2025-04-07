// Copyright (c) Wojciech Figat. All rights reserved.

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

        /// <summary>
        /// Gets a value indicating whether this type is POD (plain old data).
        /// </summary>
        public bool IsPod(Builder.BuildData buildData, ApiTypeInfo caller)
        {
            // Fixed array in C++ is converted into managed array in C# by default (char Data[100] -> char[] Data)
            if (Type.IsArray && !NoArray)
                return false;

            return Type.IsPod(buildData, caller);
        }

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

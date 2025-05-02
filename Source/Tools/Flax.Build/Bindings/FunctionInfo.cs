// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native method information for bindings generator.
    /// </summary>
    public class FunctionInfo : MemberInfo
    {
        public struct ParameterInfo : IBindingsCache
        {
            public string Name;
            public TypeInfo Type;
            public string DefaultValue;
            public string Attributes;
            public bool IsRef;
            public bool IsOut;
            public bool IsThis;
            public bool IsParams;

            public bool HasDefaultValue => !string.IsNullOrEmpty(DefaultValue);

            public bool HasAttribute(string name)
            {
                return Attributes != null && Attributes.Contains(name);
            }

            public void Write(BinaryWriter writer)
            {
                writer.Write(Name);
                BindingsGenerator.Write(writer, Type);
                BindingsGenerator.Write(writer, DefaultValue);
                BindingsGenerator.Write(writer, Attributes);
                // TODO: convert into flags
                writer.Write(IsRef);
                writer.Write(IsOut);
                writer.Write(IsThis);
                writer.Write(IsParams);
            }

            public void Read(BinaryReader reader)
            {
                Name = reader.ReadString();
                Type = BindingsGenerator.Read(reader, Type);
                DefaultValue = BindingsGenerator.Read(reader, DefaultValue);
                Attributes = BindingsGenerator.Read(reader, Attributes);
                // TODO: convert into flags
                IsRef = reader.ReadBoolean();
                IsOut = reader.ReadBoolean();
                IsThis = reader.ReadBoolean();
                IsParams = reader.ReadBoolean();
            }

            public override string ToString()
            {
                var result = Type + " " + Name;
                if (HasDefaultValue)
                    result += " = " + DefaultValue;
                return result;
            }
        }

        public struct GlueInfo
        {
            public bool UseReferenceForResult;
            public string LibraryEntryPoint;
            public List<ParameterInfo> CustomParameters;
        }

        public string UniqueName;
        public TypeInfo ReturnType;
        public List<ParameterInfo> Parameters = new List<ParameterInfo>();
        public bool IsVirtual;
        public bool IsConst;
        public bool NoProxy;
        public GlueInfo Glue;

        public override void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, ReturnType);
            BindingsGenerator.Write(writer, Parameters);
            // TODO: convert into flags
            writer.Write(IsVirtual);
            writer.Write(IsConst);
            writer.Write(NoProxy);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            ReturnType = BindingsGenerator.Read(reader, ReturnType);
            Parameters = BindingsGenerator.Read(reader, Parameters);
            // TODO: convert into flags
            IsVirtual = reader.ReadBoolean();
            IsConst = reader.ReadBoolean();
            NoProxy = reader.ReadBoolean();

            base.Read(reader);
        }

        public override string ToString()
        {
            var result = string.Empty;

            if (IsStatic)
                result += "static ";
            else if (IsVirtual)
                result += "virtual ";

            result += ReturnType + " " + Name + "(";

            for (int i = 0; i < Parameters.Count; i++)
            {
                if (i > 0)
                    result += ", ";
                result += Parameters[i];
            }

            result += ")";
            if (IsConst)
                result += " const";

            return result;
        }
    }
}

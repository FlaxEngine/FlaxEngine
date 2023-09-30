// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native method information for the bindings generator.
    /// </summary>
    public class FunctionInfo : MemberInfo
    {
        /// <summary>Decorators which are allowed on parameters</summary>
        [Flags]
        public enum ParameterDecoration : int
        {
            Ref = 0x1,
            In = 0x2,
            Out = 0x4,
            This = 0x8,
            Params = 0x10,
        }

        /// <summary>Decorators which are allowed on functions</summary>
        [Flags]
        public enum FunctionDecoration : int
        {
            Virtual = 0x1,
            Const = 0x2,
            NoProxy = 0x4,
        }

        /// <summary>
        /// The native parameter information for the bindings generator.
        /// </summary>
        public struct ParameterInfo : IBindingsCache
        {
            public string Name;
            public TypeInfo Type;
            public string DefaultValue;
            public string Attributes;
            public ParameterDecoration Decoration;


            /// <summary>The parameter is passed by ref</summary>
            public bool IsRef {
                readonly get => (Decoration & ParameterDecoration.Ref) is not 0;
                set => Decoration = (Decoration & ~ParameterDecoration.Ref) | (value ? ParameterDecoration.Ref : 0);
            }
            /// <summary>The parameter is passed by in-ref</summary>
            public bool IsIn {
                readonly get => (Decoration & ParameterDecoration.In) is not 0;
                set => Decoration = (Decoration & ~ParameterDecoration.In) | (value ? ParameterDecoration.In : 0);
            }
            /// <summary>The parameter is passed by out-ref</summary>
            public bool IsOut {
                readonly get => (Decoration & ParameterDecoration.Out) is not 0;
                set => Decoration = (Decoration & ~ParameterDecoration.Out) | (value ? ParameterDecoration.Out : 0);
            }
            /// <summary>The parameter is the this-parameter of an extension method</summary>
            public bool IsThis {
                readonly get => (Decoration & ParameterDecoration.This) is not 0;
                set => Decoration = (Decoration & ~ParameterDecoration.This) | (value ? ParameterDecoration.This : 0);
            }
            /// <summary>The parameter is a param array</summary>
            public bool IsParams {
                readonly get => (Decoration & ParameterDecoration.Params) is not 0;
                set => Decoration = (Decoration & ~ParameterDecoration.Params) | (value ? ParameterDecoration.Params : 0);
            }


            /// <summary>The parameter is passed by-ref</summary>
            public readonly bool IsByRef => IsRef || IsOut || IsIn;

            /// <summary>The parameter is passed by-ref through ref or in</summary>
            public readonly bool IsByRefIn => IsRef || IsIn;

            /// <summary>The parameter is passed by-ref through ref or out</summary>
            public readonly bool IsByRefOut => IsRef || IsOut;


            public readonly bool HasDefaultValue => !string.IsNullOrEmpty(DefaultValue);

            public readonly bool HasAttribute(string name)
            {
                return Attributes != null && Attributes.Contains(name);
            }

            public void Write(BinaryWriter writer)
            {
                writer.Write(Name);
                BindingsGenerator.Write(writer, Type);
                BindingsGenerator.Write(writer, DefaultValue);
                BindingsGenerator.Write(writer, Attributes);
                writer.Write((int)Decoration);
            }

            public void Read(BinaryReader reader)
            {
                Name = reader.ReadString();
                Type = BindingsGenerator.Read(reader, Type);
                DefaultValue = BindingsGenerator.Read(reader, DefaultValue);
                Attributes = BindingsGenerator.Read(reader, Attributes);
                Decoration = (ParameterDecoration)reader.ReadInt32();
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
        public FunctionDecoration Decoration;
        public GlueInfo Glue;

        public bool IsVirtual {
            get => (Decoration & FunctionDecoration.Virtual) is not 0;
            set => Decoration = (Decoration & ~FunctionDecoration.Virtual) | (value ? FunctionDecoration.Virtual : 0);
        }
        public bool IsConst {
            get => (Decoration & FunctionDecoration.Const) is not 0;
            set => Decoration = (Decoration & ~FunctionDecoration.Const) | (value ? FunctionDecoration.Const : 0);
        }
        public bool NoProxy {
            get => (Decoration & FunctionDecoration.NoProxy) is not 0;
            set => Decoration = (Decoration & ~FunctionDecoration.NoProxy) | (value ? FunctionDecoration.NoProxy : 0);
        }

        public override void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, ReturnType);
            BindingsGenerator.Write(writer, Parameters);
            writer.Write((int)Decoration);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            ReturnType = BindingsGenerator.Read(reader, ReturnType);
            Parameters = BindingsGenerator.Read(reader, Parameters);
            Decoration = (FunctionDecoration)reader.ReadInt32();

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

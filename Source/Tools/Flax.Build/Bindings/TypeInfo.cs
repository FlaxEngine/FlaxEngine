// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native type information for bindings generator.
    /// </summary>
    public class TypeInfo : IEquatable<TypeInfo>, IBindingsCache
    {
        public string Type;
        public bool IsConst;
        public bool IsRef;
        public bool IsPtr;
        public bool IsArray;
        public bool IsBitField;
        public int ArraySize;
        public int BitSize;
        public List<TypeInfo> GenericArgs;

        /// <summary>
        /// Gets a value indicating whether this type is void.
        /// </summary>
        public bool IsVoid => Type == "void" && !IsPtr;

        /// <summary>
        /// Gets a value indicating whether this type is POD (plain old data).
        /// </summary>
        public bool IsPod(Builder.BuildData buildData, ApiTypeInfo caller)
        {
            var apiType = BindingsGenerator.FindApiTypeInfo(buildData, this, caller);
            if (apiType != null)
            {
                apiType.EnsureInited(buildData);
                return apiType.IsPod;
            }

            if (IsPtr || IsRef)
                return true;

            // Hardcoded cases
            if (Type == "String" || Type == "Array" || Type == "StringView" || Type == "StringAnsi" || Type == "StringAnsiView")
                return false;

            // Fallback to default
            return true;
        }

        public void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, Type);
            // TODO: pack as flags
            writer.Write(IsConst);
            writer.Write(IsRef);
            writer.Write(IsPtr);
            writer.Write(IsArray);
            writer.Write(IsBitField);
            writer.Write(ArraySize);
            writer.Write(BitSize);
            BindingsGenerator.Write(writer, GenericArgs);
        }

        public void Read(BinaryReader reader)
        {
            Type = BindingsGenerator.Read(reader, Type);
            // TODO: convert into flags
            IsConst = reader.ReadBoolean();
            IsRef = reader.ReadBoolean();
            IsPtr = reader.ReadBoolean();
            IsArray = reader.ReadBoolean();
            IsBitField = reader.ReadBoolean();
            ArraySize = reader.ReadInt32();
            BitSize = reader.ReadInt32();
            GenericArgs = BindingsGenerator.Read(reader, GenericArgs);
        }

        public string GetFullNameNative(Builder.BuildData buildData, ApiTypeInfo caller)
        {
            var type = BindingsGenerator.FindApiTypeInfo(buildData, this, caller);
            if (type == null)
                return ToString();

            var sb = new StringBuilder(64);
            if (IsConst)
                sb.Append("const ");
            sb.Append(type.FullNameNative);
            if (GenericArgs != null)
            {
                sb.Append('<');
                for (var i = 0; i < GenericArgs.Count; i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    sb.Append(GenericArgs[i]);
                }
                sb.Append('>');
            }
            if (IsRef)
                sb.Append('&');
            if (IsPtr)
                sb.Append('*');
            return sb.ToString();
        }

        public override string ToString()
        {
            var sb = new StringBuilder(64);
            if (IsConst)
                sb.Append("const ");
            sb.Append(Type);
            if (GenericArgs != null)
            {
                sb.Append('<');
                for (var i = 0; i < GenericArgs.Count; i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    sb.Append(GenericArgs[i]);
                }
                sb.Append('>');
            }
            if (IsRef)
                sb.Append('&');
            if (IsPtr)
                sb.Append('*');
            return sb.ToString();
        }

        public static bool Equals(List<TypeInfo> a, List<TypeInfo> b)
        {
            if (a == null && b == null)
                return true;
            if (a == null || b == null)
                return false;
            return a.SequenceEqual(b);
        }

        public bool Equals(TypeInfo other)
        {
            if (other == null)
                return false;

            return string.Equals(Type, other.Type) &&
                   IsConst == other.IsConst &&
                   IsRef == other.IsRef &&
                   IsPtr == other.IsPtr &&
                   IsArray == other.IsArray &&
                   IsBitField == other.IsBitField &&
                   ArraySize == other.ArraySize &&
                   BitSize == other.BitSize &&
                   Equals(GenericArgs, other.GenericArgs);
        }

        public override bool Equals(object obj)
        {
            return obj is TypeInfo other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = (Type != null ? Type.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ IsConst.GetHashCode();
                hashCode = (hashCode * 397) ^ IsRef.GetHashCode();
                hashCode = (hashCode * 397) ^ IsPtr.GetHashCode();
                hashCode = (hashCode * 397) ^ IsArray.GetHashCode();
                hashCode = (hashCode * 397) ^ IsBitField.GetHashCode();
                hashCode = (hashCode * 397) ^ ArraySize.GetHashCode();
                hashCode = (hashCode * 397) ^ BitSize.GetHashCode();
                if (GenericArgs != null)
                    hashCode = (hashCode * 397) ^ GenericArgs.GetHashCode();
                return hashCode;
            }
        }
    }
}

// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    public class TypeInfo : IEquatable<TypeInfo>, IBindingsCache, ICloneable
    {
        public string Type;
        public bool IsConst;
        public bool IsRef;
        public bool IsMoveRef;
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
        /// Gets a value indicating whether this type is constant reference to a value.
        /// </summary>
        public bool IsConstRef => IsRef && IsConst;

        /// <summary>
        /// Gets a value indicating whether this type is a reference to another object.
        /// </summary>
        public bool IsObjectRef => (Type == "ScriptingObjectReference" ||
                                    Type == "AssetReference" ||
                                    Type == "WeakAssetReference" ||
                                    Type == "SoftAssetReference" ||
                                    Type == "SoftObjectReference") && GenericArgs != null;

        public TypeInfo()
        {
        }

        public TypeInfo(string type)
        {
            Type = type;
        }

        public TypeInfo(TypeInfo other)
        {
            Type = other.Type;
            IsConst = other.IsConst;
            IsRef = other.IsRef;
            IsMoveRef = other.IsMoveRef;
            IsPtr = other.IsPtr;
            IsArray = other.IsArray;
            IsBitField = other.IsBitField;
            ArraySize = other.ArraySize;
            BitSize = other.BitSize;
            GenericArgs = other.GenericArgs != null ? new List<TypeInfo>(other.GenericArgs) : null;
        }

        public TypeInfo(ApiTypeInfo apiTypeInfo)
        {
            Type = apiTypeInfo.Name;
        }

        /// <summary>
        /// Inflates the type with typedefs for generic arguments.
        /// </summary>
        public TypeInfo Inflate(Builder.BuildData buildData, ApiTypeInfo caller)
        {
            if (GenericArgs != null && GenericArgs.Count != 0)
            {
                var inflated = new TypeInfo(this);
                for (int i = 0; i < inflated.GenericArgs.Count; i++)
                {
                    var arg = new TypeInfo(inflated.GenericArgs[i]);
                    var argType = BindingsGenerator.FindApiTypeInfo(buildData, arg, caller);
                    if (argType != null)
                    {
                        arg.Type = argType.Name;
                        arg.GenericArgs = null;
                    }
                    inflated.GenericArgs[i] = arg;
                }
                return inflated;
            }
            return this;
        }

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
            writer.Write(IsMoveRef);
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
            IsMoveRef = reader.ReadBoolean();
            IsPtr = reader.ReadBoolean();
            IsArray = reader.ReadBoolean();
            IsBitField = reader.ReadBoolean();
            ArraySize = reader.ReadInt32();
            BitSize = reader.ReadInt32();
            GenericArgs = BindingsGenerator.Read(reader, GenericArgs);
        }

        public string GetFullNameNative(Builder.BuildData buildData, ApiTypeInfo caller, bool canRef = true, bool canConst = true)
        {
            var type = BindingsGenerator.FindApiTypeInfo(buildData, this, caller);
            if (type == null)
                return ToString();

            // Optimization for simple type
            if (!IsConst && GenericArgs == null && !IsPtr && !IsRef)
                return type.FullNameNative;

            var sb = new StringBuilder(64);
            if (IsConst && canConst)
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
            if (IsPtr)
                sb.Append('*');
            if (IsRef && canRef)
                sb.Append('&');
            return sb.ToString();
        }

        public static TypeInfo FromString(string text)
        {
            var result = new TypeInfo(text.Trim());
            if (result.Type.StartsWith("const"))
            {
                // Const
                result.IsConst = true;
                result.Type = result.Type.Substring(5).Trim();
            }
            if (result.Type.EndsWith('*'))
            {
                // Pointer
                result.IsPtr = true;
                result.Type = result.Type.Substring(0, result.Type.Length - 1).Trim();
            }
            if (result.Type.EndsWith('&'))
            {
                // Reference
                result.IsRef = true;
                result.Type = result.Type.Substring(0, result.Type.Length - 1).Trim();
                if (result.Type.EndsWith('&'))
                {
                    // Move reference
                    result.IsMoveRef = true;
                    result.Type = result.Type.Substring(0, result.Type.Length - 1).Trim();
                }
            }
            var idx = result.Type.IndexOf('<');
            if (idx != -1)
            {
                // Generic
                result.GenericArgs = new List<TypeInfo>();
                var generics = result.Type.Substring(idx + 1, result.Type.Length - idx - 2);
                foreach (var generic in generics.Split(','))
                    result.GenericArgs.Add(FromString(generic));
                result.Type = result.Type.Substring(0, idx);
            }
            return result;
        }

        public string ToString(bool canRef = true)
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
            if (IsPtr)
                sb.Append('*');
            if (IsRef && canRef)
                sb.Append('&');
            if (IsMoveRef && canRef)
                sb.Append('&');
            return sb.ToString();
        }

        public override string ToString()
        {
            return ToString(true);
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
                   IsMoveRef == other.IsMoveRef &&
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
                hashCode = (hashCode * 397) ^ IsMoveRef.GetHashCode();
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

        /// <inheritdoc />
        public object Clone()
        {
            return new TypeInfo(this);
        }
    }
}

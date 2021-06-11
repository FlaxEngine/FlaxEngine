// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEditor.Scripting;
using FlaxEngine;
using Newtonsoft.Json;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Editor utilities and helper functions for Variant type.
    /// </summary>
    public static class VariantUtils
    {
        internal enum VariantType
        {
            Null = 0,
            Void,

            Bool,
            Int,
            Uint,
            Int64,
            Uint64,
            Float,
            Double,
            Pointer,

            String,
            Object,
            Structure,
            Asset,
            Blob,
            Enum,

            Vector2,
            Vector3,
            Vector4,
            Color,
            Guid,
            BoundingBox,
            BoundingSphere,
            Quaternion,
            Transform,
            Rectangle,
            Ray,
            Matrix,

            Array,
            Dictionary,
            ManagedObject,
            Typename,

            Int2,
            Int3,
            Int4,

            Int16,
            Uint16,
        }

        internal static VariantType ToVariantType(this Type type)
        {
            VariantType variantType;
            if (type == null)
                variantType = VariantType.Null;
            else if (type == typeof(void))
                variantType = VariantType.Void;
            else if (type == typeof(bool))
                variantType = VariantType.Bool;
            else if (type == typeof(short))
                variantType = VariantType.Int16;
            else if (type == typeof(ushort))
                variantType = VariantType.Uint16;
            else if (type == typeof(int))
                variantType = VariantType.Int;
            else if (type == typeof(uint))
                variantType = VariantType.Uint;
            else if (type == typeof(long))
                variantType = VariantType.Int64;
            else if (type == typeof(ulong))
                variantType = VariantType.Uint64;
            else if (type.IsEnum)
                variantType = VariantType.Enum;
            else if (type == typeof(float))
                variantType = VariantType.Float;
            else if (type == typeof(double))
                variantType = VariantType.Double;
            else if (type == typeof(IntPtr))
                variantType = VariantType.Pointer;
            else if (type == typeof(string))
                variantType = VariantType.String;
            else if (type == typeof(Type) || type == typeof(ScriptType))
                variantType = VariantType.Typename;
            else if (typeof(Asset).IsAssignableFrom(type))
                variantType = VariantType.Asset;
            else if (typeof(FlaxEngine.Object).IsAssignableFrom(type))
                variantType = VariantType.Object;
            else if (type == typeof(BoundingBox))
                variantType = VariantType.BoundingBox;
            else if (type == typeof(Transform))
                variantType = VariantType.Transform;
            else if (type == typeof(Ray))
                variantType = VariantType.Ray;
            else if (type == typeof(Matrix))
                variantType = VariantType.Matrix;
            else if (type == typeof(Vector2))
                variantType = VariantType.Vector2;
            else if (type == typeof(Vector3))
                variantType = VariantType.Vector3;
            else if (type == typeof(Vector4))
                variantType = VariantType.Vector4;
            else if (type == typeof(Int2))
                variantType = VariantType.Int2;
            else if (type == typeof(Int3))
                variantType = VariantType.Int3;
            else if (type == typeof(Int4))
                variantType = VariantType.Int4;
            else if (type == typeof(Color))
                variantType = VariantType.Color;
            else if (type == typeof(Guid))
                variantType = VariantType.Guid;
            else if (type == typeof(Quaternion))
                variantType = VariantType.Quaternion;
            else if (type == typeof(Rectangle))
                variantType = VariantType.Rectangle;
            else if (type == typeof(BoundingSphere))
                variantType = VariantType.BoundingSphere;
            else if (type.IsValueType)
                variantType = VariantType.Structure;
            else if (type == typeof(byte[]))
                variantType = VariantType.Blob;
            else if (type == typeof(object[]))
                variantType = VariantType.Array;
            else if (type == typeof(Dictionary<object, object>))
                variantType = VariantType.Dictionary;
            else if (type.IsPointer || type.IsByRef)
            {
                // Find underlying type without `*` or `&`
                var typeName = type.FullName;
                type = TypeUtils.GetManagedType(typeName.Substring(0, typeName.Length - 1));
                return ToVariantType(type);
            }
            else
                variantType = VariantType.ManagedObject;
            return variantType;
        }

        internal static void WriteVariantType(this BinaryWriter stream, ScriptType type)
        {
            if (type == ScriptType.Null)
            {
                stream.Write((byte)0);
                stream.Write(0);
                return;
            }
            if (type.Type != null)
            {
                WriteVariantType(stream, type.Type);
            }
            else
            {
                if (type.IsEnum)
                    stream.Write((byte)VariantType.Enum);
                else if (type.IsValueType)
                    stream.Write((byte)VariantType.Structure);
                else
                    stream.Write((byte)VariantType.Object);
                stream.Write(int.MaxValue);
                stream.WriteStrAnsi(type.TypeName, 77);
            }
        }

        internal static void WriteVariantType(this BinaryWriter stream, Type type)
        {
            if (type == null)
            {
                stream.Write((byte)0);
                stream.Write(0);
                return;
            }
            var variantType = ToVariantType(type);
            stream.Write((byte)variantType);
            switch (variantType)
            {
            case VariantType.Object:
                if (type != typeof(FlaxEngine.Object))
                {
                    stream.Write(int.MaxValue);
                    stream.WriteStrAnsi(type.FullName, 77);
                }
                else
                    stream.Write(0);
                break;
            case VariantType.Asset:
                if (type != typeof(Asset))
                {
                    stream.Write(int.MaxValue);
                    stream.WriteStrAnsi(type.FullName, 77);
                }
                else
                    stream.Write(0);
                break;
            case VariantType.Enum:
            case VariantType.Structure:
            case VariantType.ManagedObject:
            case VariantType.Typename:
                stream.Write(int.MaxValue);
                stream.WriteStrAnsi(type.FullName, 77);
                break;
            default:
                stream.Write(0);
                break;
            }
        }

        internal static ScriptType ReadVariantScriptType(this BinaryReader stream)
        {
            var variantType = (VariantType)stream.ReadByte();
            var typeNameLength = stream.ReadInt32();
            if (typeNameLength == int.MaxValue)
            {
                typeNameLength = stream.ReadInt32();
                var data = new byte[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadByte();
                    data[i] = (byte)(c ^ 77);
                }
                var typeName = System.Text.Encoding.ASCII.GetString(data);
                return TypeUtils.GetType(typeName);
            }
            if (typeNameLength > 0)
            {
                // [Deprecated on 27.08.2020, expires on 27.08.2021]
                var data = new char[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadUInt16();
                    data[i] = (char)(c ^ 77);
                }
                var typeName = new string(data);
                return TypeUtils.GetType(typeName);
            }
            switch (variantType)
            {
            case VariantType.Null: return ScriptType.Null;
            case VariantType.Void: return new ScriptType(typeof(void));
            case VariantType.Bool: return new ScriptType(typeof(bool));
            case VariantType.Int16: return new ScriptType(typeof(short));
            case VariantType.Uint16: return new ScriptType(typeof(ushort));
            case VariantType.Int: return new ScriptType(typeof(int));
            case VariantType.Uint: return new ScriptType(typeof(uint));
            case VariantType.Int64: return new ScriptType(typeof(long));
            case VariantType.Uint64: return new ScriptType(typeof(ulong));
            case VariantType.Float: return new ScriptType(typeof(float));
            case VariantType.Double: return new ScriptType(typeof(double));
            case VariantType.Pointer: return new ScriptType(typeof(IntPtr));
            case VariantType.String: return new ScriptType(typeof(string));
            case VariantType.Typename: return new ScriptType(typeof(Type));
            case VariantType.Object: return new ScriptType(typeof(FlaxEngine.Object));
            case VariantType.Asset: return new ScriptType(typeof(Asset));
            case VariantType.Vector2: return new ScriptType(typeof(Vector2));
            case VariantType.Vector3: return new ScriptType(typeof(Vector3));
            case VariantType.Vector4: return new ScriptType(typeof(Vector4));
            case VariantType.Int2: return new ScriptType(typeof(Int2));
            case VariantType.Int3: return new ScriptType(typeof(Int3));
            case VariantType.Int4: return new ScriptType(typeof(Int4));
            case VariantType.Color: return new ScriptType(typeof(Color));
            case VariantType.Guid: return new ScriptType(typeof(Guid));
            case VariantType.BoundingBox: return new ScriptType(typeof(BoundingBox));
            case VariantType.BoundingSphere: return new ScriptType(typeof(BoundingSphere));
            case VariantType.Quaternion: return new ScriptType(typeof(Quaternion));
            case VariantType.Transform: return new ScriptType(typeof(Transform));
            case VariantType.Rectangle: return new ScriptType(typeof(Rectangle));
            case VariantType.Ray: return new ScriptType(typeof(Ray));
            case VariantType.Matrix: return new ScriptType(typeof(Matrix));
            case VariantType.Array: return new ScriptType(typeof(object[]));
            case VariantType.Dictionary: return new ScriptType(typeof(Dictionary<object, object>));
            case VariantType.ManagedObject: return new ScriptType(typeof(object));
            case VariantType.Blob: return new ScriptType(typeof(byte[]));
            default: throw new ArgumentOutOfRangeException($"Unknown Variant Type {variantType} without typename.");
            }
        }

        internal static Type ReadVariantType(this BinaryReader stream)
        {
            var variantType = (VariantType)stream.ReadByte();
            var typeNameLength = stream.ReadInt32();
            if (typeNameLength == int.MaxValue)
            {
                typeNameLength = stream.ReadInt32();
                var data = new byte[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadByte();
                    data[i] = (byte)(c ^ 77);
                }
                var typeName = System.Text.Encoding.ASCII.GetString(data);
                return TypeUtils.GetManagedType(typeName);
            }
            if (typeNameLength > 0)
            {
                // [Deprecated on 27.08.2020, expires on 27.08.2021]
                var data = new char[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadUInt16();
                    data[i] = (char)(c ^ 77);
                }
                var typeName = new string(data);
                return TypeUtils.GetManagedType(typeName);
            }
            switch (variantType)
            {
            case VariantType.Null: return null;
            case VariantType.Void: return typeof(void);
            case VariantType.Bool: return typeof(bool);
            case VariantType.Int16: return typeof(short);
            case VariantType.Uint16: return typeof(ushort);
            case VariantType.Int: return typeof(int);
            case VariantType.Uint: return typeof(uint);
            case VariantType.Int64: return typeof(long);
            case VariantType.Uint64: return typeof(ulong);
            case VariantType.Float: return typeof(float);
            case VariantType.Double: return typeof(double);
            case VariantType.Pointer: return typeof(IntPtr);
            case VariantType.String: return typeof(string);
            case VariantType.Typename: return typeof(Type);
            case VariantType.Object: return typeof(FlaxEngine.Object);
            case VariantType.Asset: return typeof(Asset);
            case VariantType.Vector2: return typeof(Vector2);
            case VariantType.Vector3: return typeof(Vector3);
            case VariantType.Vector4: return typeof(Vector4);
            case VariantType.Int2: return typeof(Int2);
            case VariantType.Int3: return typeof(Int3);
            case VariantType.Int4: return typeof(Int4);
            case VariantType.Color: return typeof(Color);
            case VariantType.Guid: return typeof(Guid);
            case VariantType.BoundingBox: return typeof(BoundingBox);
            case VariantType.BoundingSphere: return typeof(BoundingSphere);
            case VariantType.Quaternion: return typeof(Quaternion);
            case VariantType.Transform: return typeof(Transform);
            case VariantType.Rectangle: return typeof(Rectangle);
            case VariantType.Ray: return typeof(Ray);
            case VariantType.Matrix: return typeof(Matrix);
            case VariantType.Array: return typeof(object[]);
            case VariantType.Dictionary: return typeof(Dictionary<object, object>);
            case VariantType.ManagedObject: return typeof(object);
            case VariantType.Blob: return typeof(byte[]);
            default: throw new ArgumentOutOfRangeException($"Unknown Variant Type {variantType} without typename.");
            }
        }

        internal static unsafe object ReadVariant(this BinaryReader stream)
        {
            Type type = null;
            var variantType = (VariantType)stream.ReadByte();
            var typeNameLength = stream.ReadInt32();
            if (typeNameLength == int.MaxValue)
            {
                typeNameLength = stream.ReadInt32();
                var data = new byte[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadByte();
                    data[i] = (byte)(c ^ 77);
                }
                var typeName = System.Text.Encoding.ASCII.GetString(data);
                type = TypeUtils.GetManagedType(typeName);
            }
            else if (typeNameLength > 0)
            {
                // [Deprecated on 27.08.2020, expires on 27.08.2021]
                var data = new char[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadUInt16();
                    data[i] = (char)(c ^ 77);
                }
                var typeName = new string(data);
                type = TypeUtils.GetManagedType(typeName);
            }
            switch (variantType)
            {
            case VariantType.Null:
            case VariantType.ManagedObject:
            case VariantType.Void: return null;
            case VariantType.Bool: return stream.ReadByte() != 0;
            case VariantType.Int16: return stream.ReadInt16();
            case VariantType.Uint16: return stream.ReadUInt16();
            case VariantType.Int: return stream.ReadInt32();
            case VariantType.Uint: return stream.ReadUInt32();
            case VariantType.Int64: return stream.ReadInt64();
            case VariantType.Uint64: return stream.ReadUInt64();
            case VariantType.Float: return stream.ReadSingle();
            case VariantType.Double: return stream.ReadDouble();
            case VariantType.Pointer: return new IntPtr((void*)stream.ReadUInt64());
            case VariantType.String:
            {
                typeNameLength = stream.ReadInt32();
                var data = new char[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadUInt16();
                    data[i] = (char)(c ^ -14);
                }
                return new string(data);
            }
            case VariantType.Object:
            {
                var id = stream.ReadGuid();
                return FlaxEngine.Object.Find(ref id, type ?? typeof(FlaxEngine.Object));
            }
            case VariantType.Structure:
            {
                if (type == null)
                    throw new Exception("Missing structure type of the Variant.");
                var data = stream.ReadBytes(stream.ReadInt32());
                return Utils.ByteArrayToStructure(data, type);
            }
            case VariantType.Asset:
            {
                var id = stream.ReadGuid();
                return FlaxEngine.Object.Find(ref id, type ?? typeof(Asset));
            }
            case VariantType.Blob: return stream.ReadBytes(stream.ReadInt32());
            case VariantType.Enum:
            {
                if (type == null)
                    throw new Exception("Missing enum type of the Variant.");
                return Enum.ToObject(type, stream.ReadUInt64());
            }
            case VariantType.Vector2: return stream.ReadVector2();
            case VariantType.Vector3: return stream.ReadVector3();
            case VariantType.Vector4: return stream.ReadVector4();
            case VariantType.Int2: return stream.ReadInt2();
            case VariantType.Int3: return stream.ReadInt3();
            case VariantType.Int4: return stream.ReadInt4();
            case VariantType.Color: return stream.ReadColor();
            case VariantType.Guid: return stream.ReadGuid();
            case VariantType.BoundingBox: return stream.ReadBoundingBox();
            case VariantType.BoundingSphere: return stream.ReadBoundingSphere();
            case VariantType.Quaternion: return stream.ReadQuaternion();
            case VariantType.Transform: return stream.ReadTransform();
            case VariantType.Rectangle: return stream.ReadRectangle();
            case VariantType.Ray: return stream.ReadRay();
            case VariantType.Matrix: return stream.ReadMatrix();
            case VariantType.Array:
            {
                var count = stream.ReadInt32();
                var result = new object[count];
                for (int i = 0; i < count; i++)
                    result[i] = stream.ReadVariant();
                return result;
            }
            case VariantType.Dictionary:
            {
                var count = stream.ReadInt32();
                var result = new Dictionary<object, object>();
                for (int i = 0; i < count; i++)
                    result.Add(stream.ReadVariant(), stream.ReadVariant());
                return result;
            }
            case VariantType.Typename:
            {
                typeNameLength = stream.ReadInt32();
                var data = new byte[typeNameLength];
                for (int i = 0; i < typeNameLength; i++)
                {
                    var c = stream.ReadByte();
                    data[i] = (byte)(c ^ -14);
                }
                var typeName = System.Text.Encoding.ASCII.GetString(data);
                return TypeUtils.GetType(typeName);
            }
            default: throw new ArgumentOutOfRangeException($"Unknown Variant Type {variantType}." + (type != null ? $" Type: {type.FullName}" : string.Empty));
            }
        }

        internal static void WriteVariant(this BinaryWriter stream, object value)
        {
            if (value == null)
            {
                stream.Write((byte)0);
                stream.Write(0);
                return;
            }
            var type = value.GetType();
            var variantType = ToVariantType(type);
            stream.Write((byte)variantType);
            switch (variantType)
            {
            case VariantType.Object:
                if (type != typeof(FlaxEngine.Object))
                {
                    stream.Write(int.MaxValue);
                    stream.WriteStrAnsi(type.FullName, 77);
                }
                else
                    stream.Write(0);
                break;
            case VariantType.Asset:
                if (type != typeof(Asset))
                {
                    stream.Write(int.MaxValue);
                    stream.WriteStrAnsi(type.FullName, 77);
                }
                else
                    stream.Write(0);
                break;
            case VariantType.Enum:
            case VariantType.Structure:
                stream.Write(int.MaxValue);
                stream.WriteStrAnsi(type.FullName, 77);
                break;
            default:
                stream.Write(0);
                break;
            }
            Guid id;
            switch (variantType)
            {
            case VariantType.Bool:
                stream.Write((byte)((bool)value ? 1 : 0));
                break;
            case VariantType.Int16:
                stream.Write((short)value);
                break;
            case VariantType.Uint16:
                stream.Write((ushort)value);
                break;
            case VariantType.Int:
                stream.Write((int)value);
                break;
            case VariantType.Uint:
                stream.Write((uint)value);
                break;
            case VariantType.Int64:
                stream.Write((long)value);
                break;
            case VariantType.Uint64:
                stream.Write((ulong)value);
                break;
            case VariantType.Float:
                stream.Write((float)value);
                break;
            case VariantType.Double:
                stream.Write((double)value);
                break;
            case VariantType.Pointer:
                stream.Write((ulong)(IntPtr)value);
                break;
            case VariantType.String:
                stream.WriteStr((string)value, -14);
                break;
            case VariantType.Object:
                id = ((FlaxEngine.Object)value).ID;
                stream.WriteGuid(ref id);
                break;
            case VariantType.Structure:
            {
                var data = Utils.StructureToByteArray(value, type);
                stream.Write(data.Length);
                stream.Write(data);
                break;
            }
            case VariantType.Asset:
                id = ((Asset)value).ID;
                stream.WriteGuid(ref id);
                break;
            case VariantType.Blob:
                stream.Write(((byte[])value).Length);
                stream.Write((byte[])value);
                break;
            case VariantType.Enum:
                stream.Write(Convert.ToUInt64(value));
                break;
            case VariantType.Vector2:
                stream.Write((Vector2)value);
                break;
            case VariantType.Vector3:
                stream.Write((Vector3)value);
                break;
            case VariantType.Vector4:
                stream.Write((Vector4)value);
                break;
            case VariantType.Int2:
                stream.Write((Int2)value);
                break;
            case VariantType.Int3:
                stream.Write((Int3)value);
                break;
            case VariantType.Int4:
                stream.Write((Int4)value);
                break;
            case VariantType.Color:
                stream.Write((Color)value);
                break;
            case VariantType.Guid:
                id = (Guid)value;
                stream.WriteGuid(ref id);
                break;
            case VariantType.BoundingBox:
                stream.Write((BoundingBox)value);
                break;
            case VariantType.BoundingSphere:
                stream.Write((BoundingSphere)value);
                break;
            case VariantType.Quaternion:
                stream.Write((Quaternion)value);
                break;
            case VariantType.Transform:
                stream.Write((Transform)value);
                break;
            case VariantType.Rectangle:
                stream.Write((Rectangle)value);
                break;
            case VariantType.Ray:
                stream.Write((Ray)value);
                break;
            case VariantType.Matrix:
                stream.Write((Matrix)value);
                break;
            case VariantType.Array:
                stream.Write(((object[])value).Length);
                foreach (var e in (object[])value)
                    stream.WriteVariant(e);
                break;
            case VariantType.Dictionary:
                stream.Write(((Dictionary<object, object>)value).Count);
                foreach (var e in (Dictionary<object, object>)value)
                {
                    stream.WriteVariant(e.Key);
                    stream.WriteVariant(e.Value);
                }
                break;
            case VariantType.Typename:
                if (value is Type)
                    stream.WriteStrAnsi(((Type)value).FullName, -14);
                else if (value is ScriptType)
                    stream.WriteStrAnsi(((ScriptType)value).TypeName, -14);
                break;
            }
        }

        internal static void WriteVariantType(this JsonWriter stream, Type value)
        {
            var variantType = ToVariantType(value);
            var withoutTypeName = true;
            switch (variantType)
            {
            case VariantType.Object:
                if (value != typeof(FlaxEngine.Object))
                    withoutTypeName = false;
                break;
            case VariantType.Asset:
                if (value != typeof(Asset))
                    withoutTypeName = false;
                break;
            case VariantType.Enum:
            case VariantType.Structure:
                withoutTypeName = false;
                break;
            }
            if (withoutTypeName)
            {
                stream.WriteValue((int)variantType);
            }
            else
            {
                stream.WriteStartObject();

                stream.WritePropertyName("Type");
                stream.WriteValue((int)variantType);

                stream.WritePropertyName("TypeName");
                stream.WriteValue(value.FullName);

                stream.WriteEndObject();
            }
        }

        internal static void WriteVariant(this JsonWriter stream, object value)
        {
            var type = value?.GetType();

            stream.WriteStartObject();

            stream.WritePropertyName("Type");
            stream.WriteVariantType(type);

            stream.WritePropertyName("Value");
            var variantType = ToVariantType(type);
            // ReSharper disable PossibleNullReferenceException
            switch (variantType)
            {
            case VariantType.Null:
            case VariantType.ManagedObject:
            case VariantType.Void:
                stream.WriteStartObject();
                stream.WriteEndObject();
                break;
            case VariantType.Bool:
                stream.WriteValue((bool)value);
                break;
            case VariantType.Int16:
                stream.WriteValue((short)value);
                break;
            case VariantType.Uint16:
                stream.WriteValue((ushort)value);
                break;
            case VariantType.Int:
                stream.WriteValue((int)value);
                break;
            case VariantType.Uint:
                stream.WriteValue((uint)value);
                break;
            case VariantType.Int64:
                stream.WriteValue((long)value);
                break;
            case VariantType.Uint64:
                stream.WriteValue((ulong)value);
                break;
            case VariantType.Float:
                stream.WriteValue((float)value);
                break;
            case VariantType.Double:
                stream.WriteValue((double)value);
                break;
            case VariantType.Pointer:
                stream.WriteValue((ulong)(IntPtr)value);
                break;
            case VariantType.String:
                stream.WriteValue((string)value);
                break;
            case VariantType.Object:
                stream.WriteValue(((FlaxEngine.Object)value).ID);
                break;
            case VariantType.Asset:
                stream.WriteValue(((Asset)value).ID);
                break;
            case VariantType.Blob:
                stream.WriteValue(Convert.ToBase64String((byte[])value));
                break;
            case VariantType.Enum:
                stream.WriteValue(Convert.ToUInt64(value));
                break;
            case VariantType.Vector2:
            {
                var asVector2 = (Vector2)value;
                stream.WriteStartObject();

                stream.WritePropertyName("X");
                stream.WriteValue(asVector2.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asVector2.Y);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Vector3:
            {
                var asVector3 = (Vector3)value;
                stream.WriteStartObject();

                stream.WritePropertyName("X");
                stream.WriteValue(asVector3.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asVector3.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asVector3.Z);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Vector4:
            {
                var asVector4 = (Vector4)value;
                stream.WriteStartObject();

                stream.WritePropertyName("X");
                stream.WriteValue(asVector4.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asVector4.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asVector4.Z);
                stream.WritePropertyName("W");
                stream.WriteValue(asVector4.W);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Int2:
            {
                var asInt2 = (Int2)value;
                stream.WriteStartObject();

                stream.WritePropertyName("X");
                stream.WriteValue(asInt2.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asInt2.Y);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Int3:
            {
                var asInt3 = (Int3)value;
                stream.WriteStartObject();

                stream.WritePropertyName("X");
                stream.WriteValue(asInt3.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asInt3.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asInt3.Z);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Int4:
            {
                var asInt4 = (Int4)value;
                stream.WriteStartObject();

                stream.WritePropertyName("X");
                stream.WriteValue(asInt4.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asInt4.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asInt4.Z);
                stream.WritePropertyName("W");
                stream.WriteValue(asInt4.W);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Color:
            {
                var asColor = (Color)value;
                stream.WriteStartObject();

                stream.WritePropertyName("R");
                stream.WriteValue(asColor.R);
                stream.WritePropertyName("G");
                stream.WriteValue(asColor.G);
                stream.WritePropertyName("B");
                stream.WriteValue(asColor.B);
                stream.WritePropertyName("A");
                stream.WriteValue(asColor.A);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Guid:
                stream.WriteValue((Guid)value);
                break;
            case VariantType.BoundingBox:
            {
                var asBoundingBox = (BoundingBox)value;
                stream.WriteStartObject();

                stream.WritePropertyName("Minimum");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asBoundingBox.Minimum.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asBoundingBox.Minimum.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asBoundingBox.Minimum.Z);
                stream.WriteEndObject();

                stream.WritePropertyName("Maximum");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asBoundingBox.Maximum.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asBoundingBox.Maximum.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asBoundingBox.Maximum.Z);
                stream.WriteEndObject();

                stream.WriteEndObject();
                break;
            }
            case VariantType.BoundingSphere:
            {
                var asBoundingSphere = (BoundingSphere)value;
                stream.WriteStartObject();

                stream.WritePropertyName("Center");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asBoundingSphere.Center.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asBoundingSphere.Center.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asBoundingSphere.Center.Z);
                stream.WriteEndObject();

                stream.WritePropertyName("Radius");
                stream.WriteValue(asBoundingSphere.Radius);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Quaternion:
            {
                var asQuaternion = (Quaternion)value;
                stream.WriteStartObject();

                stream.WritePropertyName("X");
                stream.WriteValue(asQuaternion.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asQuaternion.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asQuaternion.Z);
                stream.WritePropertyName("W");
                stream.WriteValue(asQuaternion.W);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Transform:
            {
                var asTransform = (Transform)value;
                stream.WriteStartObject();

                stream.WritePropertyName("Translation");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asTransform.Translation.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asTransform.Translation.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asTransform.Translation.Z);
                stream.WriteEndObject();

                stream.WritePropertyName("Orientation");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asTransform.Orientation.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asTransform.Orientation.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asTransform.Orientation.Z);
                stream.WritePropertyName("W");
                stream.WriteValue(asTransform.Orientation.W);
                stream.WriteEndObject();

                stream.WritePropertyName("Scale");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asTransform.Scale.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asTransform.Scale.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asTransform.Scale.Z);
                stream.WriteEndObject();

                stream.WriteEndObject();
                break;
            }
            case VariantType.Rectangle:
            {
                var asRectangle = (Rectangle)value;
                stream.WriteStartObject();

                stream.WritePropertyName("Location");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asRectangle.Location.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asRectangle.Location.Y);
                stream.WriteEndObject();

                stream.WritePropertyName("Size");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asRectangle.Size.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asRectangle.Size.Y);
                stream.WriteEndObject();

                stream.WriteEndObject();
                break;
            }
            case VariantType.Ray:
            {
                var asRay = (Ray)value;
                stream.WriteStartObject();

                stream.WritePropertyName("Position");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asRay.Position.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asRay.Position.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asRay.Position.Z);
                stream.WriteEndObject();

                stream.WritePropertyName("Direction");
                stream.WriteStartObject();
                stream.WritePropertyName("X");
                stream.WriteValue(asRay.Direction.X);
                stream.WritePropertyName("Y");
                stream.WriteValue(asRay.Direction.Y);
                stream.WritePropertyName("Z");
                stream.WriteValue(asRay.Direction.Z);
                stream.WriteEndObject();

                stream.WriteEndObject();
                break;
            }
            case VariantType.Matrix:
            {
                var asMatrix = (Matrix)value;
                stream.WriteStartObject();

                stream.WritePropertyName("M11");
                stream.WriteValue(asMatrix.M11);
                stream.WritePropertyName("M12");
                stream.WriteValue(asMatrix.M12);
                stream.WritePropertyName("M13");
                stream.WriteValue(asMatrix.M13);
                stream.WritePropertyName("M14");
                stream.WriteValue(asMatrix.M14);

                stream.WritePropertyName("M21");
                stream.WriteValue(asMatrix.M21);
                stream.WritePropertyName("M22");
                stream.WriteValue(asMatrix.M22);
                stream.WritePropertyName("M23");
                stream.WriteValue(asMatrix.M23);
                stream.WritePropertyName("M24");
                stream.WriteValue(asMatrix.M24);

                stream.WritePropertyName("M31");
                stream.WriteValue(asMatrix.M31);
                stream.WritePropertyName("M32");
                stream.WriteValue(asMatrix.M32);
                stream.WritePropertyName("M33");
                stream.WriteValue(asMatrix.M33);
                stream.WritePropertyName("M34");
                stream.WriteValue(asMatrix.M34);

                stream.WritePropertyName("M41");
                stream.WriteValue(asMatrix.M41);
                stream.WritePropertyName("M42");
                stream.WriteValue(asMatrix.M42);
                stream.WritePropertyName("M43");
                stream.WriteValue(asMatrix.M43);
                stream.WritePropertyName("M44");
                stream.WriteValue(asMatrix.M44);

                stream.WriteEndObject();
                break;
            }
            case VariantType.Array:
            {
                stream.WriteStartArray();
                foreach (var e in (object[])value)
                    stream.WriteVariant(e);
                stream.WriteEndArray();
                break;
            }
            case VariantType.Dictionary:
            {
                stream.WriteStartArray();
                foreach (var e in (Dictionary<object, object>)value)
                {
                    stream.WritePropertyName("Key");
                    stream.WriteVariant(e.Key);
                    stream.WritePropertyName("Value");
                    stream.WriteVariant(e.Value);
                }
                stream.WriteEndArray();
                break;
            }
            case VariantType.Typename:
                if (value is Type)
                    stream.WriteValue(((Type)value).FullName);
                else if (value is ScriptType)
                    stream.WriteValue(((ScriptType)value).TypeName);
                break;
            default: throw new NotImplementedException($"TODO: serialize {variantType} to Json");
            }
            // ReSharper restore PossibleNullReferenceException

            stream.WriteEndObject();
        }
    }
}

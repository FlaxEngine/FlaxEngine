// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using Newtonsoft.Json;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Editor utilities and helper functions.
    /// </summary>
    public static class Utils
    {
        private static readonly string[] MemorySizePostfixes =
        {
            "B",
            "kB",
            "MB",
            "GB",
            "TB",
            "PB"
        };

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
        }

        /// <summary>
        /// The name of the Flax Engine C# assembly name.
        /// </summary>
        public static readonly string FlaxEngineAssemblyName = "FlaxEngine.CSharp";

        /// <summary>
        /// Formats the amount of bytes to get a human-readable data size in bytes with abbreviation. Eg. 32 kB
        /// </summary>
        /// <param name="bytes">The bytes.</param>
        /// <returns>The formatted amount of bytes.</returns>
        public static string FormatBytesCount(int bytes)
        {
            int order = 0;
            while (bytes >= 1024 && order < MemorySizePostfixes.Length - 1)
            {
                order++;
                bytes /= 1024;
            }

            return string.Format("{0:0.##} {1}", bytes, MemorySizePostfixes[order]);
        }

        /// <summary>
        /// Formats the amount of bytes to get a human-readable data size in bytes with abbreviation. Eg. 32 kB
        /// </summary>
        /// <param name="bytes">The bytes.</param>
        /// <returns>The formatted amount of bytes.</returns>
        public static string FormatBytesCount(ulong bytes)
        {
            int order = 0;
            while (bytes >= 1024 && order < MemorySizePostfixes.Length - 1)
            {
                order++;
                bytes /= 1024;
            }

            return string.Format("{0:0.##} {1}", bytes, MemorySizePostfixes[order]);
        }

        /// <summary>
        /// The colors for the keyframes used by the curve editor.
        /// </summary>
        internal static readonly Color[] CurveKeyframesColors =
        {
            Color.OrangeRed,
            Color.ForestGreen,
            Color.CornflowerBlue,
            Color.White,
        };

        /// <summary>
        /// The time/value axes tick steps for editors with timeline.
        /// </summary>
        internal static readonly float[] CurveTickSteps =
        {
            0.0000001f, 0.0000005f, 0.000001f, 0.000005f, 0.00001f,
            0.00005f, 0.0001f, 0.0005f, 0.001f, 0.005f,
            0.01f, 0.05f, 0.1f, 0.5f, 1,
            5, 10, 50, 100, 500,
            1000, 5000, 10000, 50000, 100000,
            500000, 1000000, 5000000, 10000000, 100000000
        };

        /// <summary>
        /// Determines whether the specified path string contains any invalid character.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns><c>true</c> if the given string cannot be used as a path because it contains one or more illegal characters; otherwise, <c>false</c>.</returns>
        public static bool HasInvalidPathChar(string path)
        {
            char[] illegalChars =
            {
                '?',
                '\\',
                '/',
                '\"',
                '<',
                '>',
                '|',
                ':',
                '*',
                '\u0001',
                '\u0002',
                '\u0003',
                '\u0004',
                '\u0005',
                '\u0006',
                '\a',
                '\b',
                '\t',
                '\n',
                '\v',
                '\f',
                '\r',
                '\u000E',
                '\u000F',
                '\u0010',
                '\u0011',
                '\u0012',
                '\u0013',
                '\u0014',
                '\u0015',
                '\u0016',
                '\u0017',
                '\u0018',
                '\u0019',
                '\u001A',
                '\u001B',
                '\u001C',
                '\u001D',
                '\u001E',
                '\u001F'
            };
            return path.IndexOfAny(illegalChars) != -1;
        }

        internal static Transform[] GetTransformsAndBounds(List<SceneGraphNode> nodes, out BoundingBox bounds)
        {
            var transforms = new Transform[nodes.Count];
            bounds = BoundingBox.Empty;
            for (int i = 0; i < nodes.Count; i++)
            {
                transforms[i] = nodes[i].Transform;
                if (nodes[i] is ActorNode actorNode)
                {
                    bounds = BoundingBox.Merge(bounds, actorNode.Actor.BoxWithChildren);
                }
            }
            return transforms;
        }

        /// <summary>
        /// Removes the file if it exists.
        /// </summary>
        /// <param name="file">The file path.</param>
        public static void RemoveFileIfExists(string file)
        {
            if (File.Exists(file))
                File.Delete(file);
        }

        /// <summary>
        /// Copies the directory. Supports subdirectories copy with files override option.
        /// </summary>
        /// <param name="srcDirectoryPath">The source directory path.</param>
        /// <param name="dstDirectoryPath">The destination directory path.</param>
        /// <param name="copySubDirs">If set to <c>true</c> copy subdirectories.</param>
        /// <param name="overrideFiles">if set to <c>true</c> override existing files.</param>
        public static void DirectoryCopy(string srcDirectoryPath, string dstDirectoryPath, bool copySubDirs = true, bool overrideFiles = false)
        {
            var dir = new DirectoryInfo(srcDirectoryPath);

            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException("Missing source directory to copy. " + srcDirectoryPath);
            }

            if (!Directory.Exists(dstDirectoryPath))
            {
                Directory.CreateDirectory(dstDirectoryPath);
            }

            var files = dir.GetFiles();
            for (int i = 0; i < files.Length; i++)
            {
                string tmp = Path.Combine(dstDirectoryPath, files[i].Name);
                files[i].CopyTo(tmp, overrideFiles);
            }

            if (copySubDirs)
            {
                var dirs = dir.GetDirectories();
                for (int i = 0; i < dirs.Length; i++)
                {
                    string tmp = Path.Combine(dstDirectoryPath, dirs[i].Name);
                    DirectoryCopy(dirs[i].FullName, tmp, true, overrideFiles);
                }
            }
        }

        /// <summary>
        /// Converts the raw bytes into the structure. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <typeparam name="T">The structure type.</typeparam>
        /// <param name="bytes">The data bytes.</param>
        /// <returns>The structure.</returns>
        public static T ByteArrayToStructure<T>(byte[] bytes) where T : struct
        {
            // #stupid c#
            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            T stuff = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
            handle.Free();
            return stuff;
        }

        /// <summary>
        /// Converts the raw bytes into the structure. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <param name="bytes">The data bytes.</param>
        /// <param name="type">The structure type.</param>
        /// <returns>The structure.</returns>
        public static object ByteArrayToStructure(byte[] bytes, Type type)
        {
            // #stupid c#
            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            object stuff = Marshal.PtrToStructure(handle.AddrOfPinnedObject(), type);
            handle.Free();
            return stuff;
        }

        /// <summary>
        /// Converts the raw bytes into the structure. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <typeparam name="T">The structure type.</typeparam>
        /// <param name="bytes">The data bytes.</param>
        /// <param name="result">The result.</param>
        public static void ByteArrayToStructure<T>(byte[] bytes, out T result) where T : struct
        {
            // #stupid c#
            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            result = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
            handle.Free();
        }

        /// <summary>
        /// Converts the structure to the raw bytes. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <typeparam name="T">The structure type.</typeparam>
        /// <param name="value">The structure value.</param>
        /// <returns>The bytes array that contains a structure data.</returns>
        public static byte[] StructureToByteArray<T>(ref T value) where T : struct
        {
            // #stupid c#
            int size = Marshal.SizeOf(typeof(T));
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(value, ptr, true);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        /// <summary>
        /// Converts the structure to the raw bytes. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <param name="value">The structure value.</param>
        /// <param name="type">The structure type.</param>
        /// <returns>The bytes array that contains a structure data.</returns>
        public static byte[] StructureToByteArray(object value, Type type)
        {
            // #stupid c#
            int size = Marshal.SizeOf(type);
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(value, ptr, true);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        internal static unsafe string ReadStr(this BinaryReader stream, int check)
        {
            int length = stream.ReadInt32();
            if (length > 0 && length < 4 * 1024)
            {
                var str = stream.ReadBytes(length * 2);
                fixed (byte* strPtr = str)
                {
                    var ptr = (char*)strPtr;
                    for (int j = 0; j < length; j++)
                        ptr[j] = (char)(ptr[j] ^ check);
                }
                return System.Text.Encoding.Unicode.GetString(str);
            }
            return string.Empty;
        }

        internal static unsafe string ReadStrAnsi(this BinaryReader stream, int check)
        {
            int length = stream.ReadInt32();
            if (length > 0 && length < 4 * 1024)
            {
                var str = stream.ReadBytes(length);
                fixed (byte* strPtr = str)
                {
                    var ptr = strPtr;
                    for (int j = 0; j < length; j++)
                        ptr[j] = (byte)(ptr[j] ^ check);
                }
                return System.Text.Encoding.ASCII.GetString(str);
            }
            return string.Empty;
        }

        internal static unsafe void WriteStr(this BinaryWriter stream, string str, int check)
        {
            int length = str?.Length ?? 0;
            stream.Write(length);
            if (length == 0)
                return;
            var bytes = System.Text.Encoding.Unicode.GetBytes(str);
            if (bytes.Length != length * 2)
                throw new ArgumentException();
            fixed (byte* bytesPtr = bytes)
            {
                var ptr = (char*)bytesPtr;
                for (int j = 0; j < length; j++)
                    ptr[j] = (char)(ptr[j] ^ check);
            }
            stream.Write(bytes);
        }

        internal static unsafe void WriteStrAnsi(this BinaryWriter stream, string str, int check)
        {
            int length = str?.Length ?? 0;
            stream.Write(length);
            if (length == 0)
                return;
            var bytes = System.Text.Encoding.ASCII.GetBytes(str);
            if (bytes.Length != length)
                throw new ArgumentException();
            fixed (byte* bytesPtr = bytes)
            {
                var ptr = bytesPtr;
                for (int j = 0; j < length; j++)
                    ptr[j] = (byte)(ptr[j] ^ check);
            }
            stream.Write(bytes);
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

        internal static Guid ReadGuid(this BinaryReader stream)
        {
            // TODO: use static bytes array to reduce dynamic allocs
            return new Guid(stream.ReadBytes(16));
        }

        internal static void WriteGuid(this BinaryWriter stream, ref Guid value)
        {
            // TODO: use static bytes array to reduce dynamic allocs
            stream.Write(value.ToByteArray());
        }

        internal static Rectangle ReadRectangle(this BinaryReader stream)
        {
            return new Rectangle(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        internal static void WriteRectangle(this BinaryWriter stream, ref Rectangle value)
        {
            stream.Write(value.Location.X);
            stream.Write(value.Location.Y);
            stream.Write(value.Size.X);
            stream.Write(value.Size.Y);
        }

        internal static void ReadCommonValue(this BinaryReader stream, ref object value)
        {
            byte type = stream.ReadByte();

            switch (type)
            {
            case 0: // CommonType::Bool:
                value = stream.ReadByte() != 0;
                break;
            case 1: // CommonType::Integer:
            {
                value = stream.ReadInt32();
            }
                break;
            case 2: // CommonType::Float:
            {
                value = stream.ReadSingle();
            }
                break;
            case 3: // CommonType::Vector2:
            {
                value = stream.ReadVector2();
            }
                break;
            case 4: // CommonType::Vector3:
            {
                value = stream.ReadVector3();
            }
                break;
            case 5: // CommonType::Vector4:
            {
                value = stream.ReadVector4();
            }
                break;
            case 6: // CommonType::Color:
            {
                value = stream.ReadColor();
            }
                break;
            case 7: // CommonType::Guid:
            {
                value = stream.ReadGuid();
            }
                break;
            case 8: // CommonType::String:
            {
                int length = stream.ReadInt32();
                if (length <= 0)
                {
                    value = string.Empty;
                }
                else
                {
                    var data = new char[length];
                    for (int i = 0; i < length; i++)
                    {
                        var c = stream.ReadUInt16();
                        data[i] = (char)(c ^ 953);
                    }
                    value = new string(data);
                }
                break;
            }
            case 9: // CommonType::Box:
            {
                value = new BoundingBox(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                        new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()));
            }
                break;
            case 10: // CommonType::Rotation:
            {
                value = stream.ReadQuaternion();
            }
                break;
            case 11: // CommonType::Transform:
            {
                value = new Transform(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                      new Quaternion(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                      new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()));
            }
                break;
            case 12: // CommonType::Sphere:
            {
                value = new BoundingSphere(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                           stream.ReadSingle());
            }
                break;
            case 13: // CommonType::Rect:
            {
                value = new Rectangle(new Vector2(stream.ReadSingle(), stream.ReadSingle()),
                                      new Vector2(stream.ReadSingle(), stream.ReadSingle()));
            }
                break;
            case 15: // CommonType::Matrix
            {
                value = new Matrix(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(),
                                   stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(),
                                   stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(),
                                   stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
                break;
            }
            case 16: // CommonType::Blob
            {
                int length = stream.ReadInt32();
                value = stream.ReadBytes(length);
                break;
            }
            case 18: // CommonType::Ray
            {
                value = new Ray(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()));
                break;
            }
            default: throw new SystemException();
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
                return ByteArrayToStructure(data, type);
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

        internal static void WriteCommonValue(this BinaryWriter stream, object value)
        {
            if (value is bool asBool)
            {
                stream.Write((byte)0);
                stream.Write((byte)(asBool ? 1 : 0));
            }
            else if (value is int asInt)
            {
                stream.Write((byte)1);
                stream.Write(asInt);
            }
            else if (value is long asLong)
            {
                stream.Write((byte)1);
                stream.Write((int)asLong);
            }
            else if (value is float asFloat)
            {
                stream.Write((byte)2);
                stream.Write(asFloat);
            }
            else if (value is double asDouble)
            {
                stream.Write((byte)2);
                stream.Write((float)asDouble);
            }
            else if (value is Vector2 asVector2)
            {
                stream.Write((byte)3);
                stream.Write(asVector2.X);
                stream.Write(asVector2.Y);
            }
            else if (value is Vector3 asVector3)
            {
                stream.Write((byte)4);
                stream.Write(asVector3.X);
                stream.Write(asVector3.Y);
                stream.Write(asVector3.Z);
            }
            else if (value is Vector4 asVector4)
            {
                stream.Write((byte)5);
                stream.Write(asVector4.X);
                stream.Write(asVector4.Y);
                stream.Write(asVector4.Z);
                stream.Write(asVector4.W);
            }
            else if (value is Color asColor)
            {
                stream.Write((byte)6);
                stream.Write(asColor.R);
                stream.Write(asColor.G);
                stream.Write(asColor.B);
                stream.Write(asColor.A);
            }
            else if (value is Guid asGuid)
            {
                stream.Write((byte)7);
                stream.WriteGuid(ref asGuid);
            }
            else if (value is string asString)
            {
                stream.Write((byte)8);
                stream.Write(asString.Length);
                for (int i = 0; i < asString.Length; i++)
                    stream.Write((ushort)(asString[i] ^ -14));
            }
            else if (value is BoundingBox asBox)
            {
                stream.Write((byte)9);
                stream.Write(asBox.Minimum.X);
                stream.Write(asBox.Minimum.Y);
                stream.Write(asBox.Minimum.Z);
                stream.Write(asBox.Maximum.X);
                stream.Write(asBox.Maximum.Y);
                stream.Write(asBox.Maximum.Z);
            }
            else if (value is Quaternion asRotation)
            {
                stream.Write((byte)10);
                stream.Write(asRotation.X);
                stream.Write(asRotation.Y);
                stream.Write(asRotation.Z);
                stream.Write(asRotation.X);
            }
            else if (value is Transform asTransform)
            {
                stream.Write((byte)11);
                stream.Write(asTransform.Translation.X);
                stream.Write(asTransform.Translation.Y);
                stream.Write(asTransform.Translation.Z);
                stream.Write(asTransform.Orientation.X);
                stream.Write(asTransform.Orientation.Y);
                stream.Write(asTransform.Orientation.Z);
                stream.Write(asTransform.Orientation.X);
                stream.Write(asTransform.Scale.X);
                stream.Write(asTransform.Scale.Y);
                stream.Write(asTransform.Scale.Z);
            }
            else if (value is BoundingSphere asSphere)
            {
                stream.Write((byte)12);
                stream.Write(asSphere.Center.X);
                stream.Write(asSphere.Center.Y);
                stream.Write(asSphere.Center.Z);
                stream.Write(asSphere.Radius);
            }
            else if (value is Rectangle asRect)
            {
                stream.Write((byte)13);
                stream.Write(asRect.Location.X);
                stream.Write(asRect.Location.Y);
                stream.Write(asRect.Size.X);
                stream.Write(asRect.Size.Y);
            }
            else if (value is Matrix asMatrix)
            {
                stream.Write((byte)15);
                stream.Write(asMatrix.M11);
                stream.Write(asMatrix.M12);
                stream.Write(asMatrix.M13);
                stream.Write(asMatrix.M14);
                stream.Write(asMatrix.M21);
                stream.Write(asMatrix.M22);
                stream.Write(asMatrix.M23);
                stream.Write(asMatrix.M24);
                stream.Write(asMatrix.M31);
                stream.Write(asMatrix.M32);
                stream.Write(asMatrix.M33);
                stream.Write(asMatrix.M34);
                stream.Write(asMatrix.M41);
                stream.Write(asMatrix.M42);
                stream.Write(asMatrix.M43);
                stream.Write(asMatrix.M44);
            }
            else if (value is byte[] asBlob)
            {
                stream.Write((byte)16);
                stream.Write(asBlob.Length);
                stream.Write(asBlob);
            }
            else if (value is Ray asRay)
            {
                stream.Write((byte)18);
                stream.Write(asRay.Position.X);
                stream.Write(asRay.Position.Y);
                stream.Write(asRay.Position.Z);
                stream.Write(asRay.Direction.X);
                stream.Write(asRay.Direction.Y);
                stream.Write(asRay.Direction.Z);
            }
            else
            {
                throw new NotSupportedException(string.Format("Invalid Common Value type {0}", value != null ? value.GetType().ToString() : "null"));
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
                var data = StructureToByteArray(value, type);
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

        [SuppressMessage("ReSharper", "PossibleNullReferenceException")]
        [SuppressMessage("ReSharper", "AssignNullToNotNullAttribute")]
        internal static void WriteVariant(this JsonWriter stream, object value)
        {
            var type = value?.GetType();

            stream.WriteStartObject();

            stream.WritePropertyName("Type");
            stream.WriteVariantType(type);

            stream.WritePropertyName("Value");
            var variantType = ToVariantType(type);
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

            stream.WriteEndObject();
        }

        /// <summary>
        /// Shows the source code window.
        /// </summary>
        /// <param name="source">The source code.</param>
        /// <param name="title">The window title.</param>
        /// <param name="parentWindow">The optional parent window.</param>
        public static void ShowSourceCodeWindow(string source, string title, Window parentWindow = null)
        {
            if (string.IsNullOrEmpty(source))
            {
                MessageBox.Show("No generated shader source code.", "No source.");
                return;
            }

            var settings = CreateWindowSettings.Default;
            settings.ActivateWhenFirstShown = true;
            settings.AllowMaximize = false;
            settings.AllowMinimize = false;
            settings.HasSizingFrame = false;
            settings.StartPosition = WindowStartPosition.CenterParent;
            settings.Size = new Vector2(500, 600) * (parentWindow?.DpiScale ?? Platform.DpiScale);
            settings.Parent = parentWindow;
            settings.Title = title;
            var dialog = Platform.CreateWindow(ref settings);

            var copyButton = new Button(4, 4, 100)
            {
                Text = "Copy",
                Parent = dialog.GUI,
            };
            copyButton.Clicked += () => Clipboard.Text = source;

            var sourceTextBox = new TextBox(true, 2, copyButton.Bottom + 4, settings.Size.X - 4);
            sourceTextBox.Height = settings.Size.Y - sourceTextBox.Top - 2;
            sourceTextBox.Text = source;
            sourceTextBox.Parent = dialog.GUI;

            dialog.Show();
            dialog.Focus();
        }

        private static OrientedBoundingBox GetWriteBox(ref Vector3 min, ref Vector3 max, float margin)
        {
            var box = new OrientedBoundingBox();
            Vector3 vec = max - min;
            Vector3 dir = Vector3.Normalize(vec);
            Quaternion orientation;
            if (Vector3.Dot(dir, Vector3.Up) >= 0.999f)
                orientation = Quaternion.RotationAxis(Vector3.Left, Mathf.PiOverTwo);
            else
                orientation = Quaternion.LookRotation(dir, Vector3.Cross(Vector3.Cross(dir, Vector3.Up), dir));
            Vector3 up = Vector3.Up * orientation;
            box.Transformation = Matrix.CreateWorld(min + vec * 0.5f, dir, up);
            Matrix.Invert(ref box.Transformation, out Matrix inv);
            Vector3 vecLocal = Vector3.TransformNormal(vec * 0.5f, inv);
            box.Extents.X = margin;
            box.Extents.Y = margin;
            box.Extents.Z = vecLocal.Z;
            return box;
        }

        /// <summary>
        /// Checks if given ray intersects with the oriented bounding wire box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="ray">The ray.</param>
        /// <param name="distance">The result intersection distance.</param>
        /// <param name="viewPosition">The view position used to scale the wires thickness depending on the wire distance from the view.</param>
        /// <returns>True ray hits bounds, otherwise false.</returns>
        public static unsafe bool RayCastWire(ref OrientedBoundingBox box, ref Ray ray, out float distance, ref Vector3 viewPosition)
        {
            var corners = stackalloc Vector3[8];
            box.GetCorners(corners);

            var minDistance = Vector3.DistanceSquared(ref viewPosition, ref corners[0]);
            for (int i = 1; i < 8; i++)
                minDistance = Mathf.Min(minDistance, Vector3.DistanceSquared(ref viewPosition, ref corners[i]));
            minDistance = Mathf.Sqrt(minDistance);
            var margin = Mathf.Clamp(minDistance / 80.0f, 0.1f, 100.0f);

            if (GetWriteBox(ref corners[0], ref corners[1], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[0], ref corners[3], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[0], ref corners[4], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[1], ref corners[2], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[1], ref corners[5], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[2], ref corners[3], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[2], ref corners[6], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[3], ref corners[7], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[4], ref corners[5], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[4], ref corners[7], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[5], ref corners[6], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[6], ref corners[7], margin).Intersects(ref ray, out distance))
                return true;

            distance = 0;
            return false;
        }

        /// <summary>
        /// Initializes the object fields and properties with their default values based on <see cref="System.ComponentModel.DefaultValueAttribute"/>.
        /// </summary>
        /// <param name="obj">The object.</param>
        public static void InitDefaultValues(object obj)
        {
            var scriptType = TypeUtils.GetObjectType(obj);
            if (!scriptType)
                return;
            var isStructure = scriptType.IsStructure;

            var fields = scriptType.GetFields(BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public);
            for (var i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                var attr = field.GetAttribute<System.ComponentModel.DefaultValueAttribute>();
                if (attr != null)
                {
                    field.SetValue(obj, attr.Value);
                }
                else if (isStructure)
                {
                    // C# doesn't support default values for structure members so initialize them
                    field.SetValue(obj, TypeUtils.GetDefaultValue(field.ValueType));
                }
            }

            var properties = scriptType.GetProperties(BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public);
            for (var i = 0; i < properties.Length; i++)
            {
                var property = properties[i];
                var attr = property.GetAttribute<System.ComponentModel.DefaultValueAttribute>();
                if (attr != null)
                {
                    property.SetValue(obj, attr.Value);
                }
            }
        }
    }
}

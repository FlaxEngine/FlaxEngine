// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using FlaxEngine.Interop;

namespace FlaxEngine
{
    /// <summary>
    /// Class with helper functions.
    /// </summary>
    public static partial class Utils
    {
        /// <summary>
        /// Copies data from one memory location to another using an unmanaged memory pointers.
        /// </summary>
        /// <remarks>Uses low-level platform impl.</remarks>
        /// <param name="source">The source location.</param>
        /// <param name="destination">The destination location.</param>
        /// <param name="length">The length (amount of bytes to copy).</param>
        [Obsolete("Use MemoryCopy with long length and source/destination parameters swapped instead")]
        public static void MemoryCopy(IntPtr source, IntPtr destination, int length)
        {
            // [Deprecated on 30.05.2021, expires on 30.05.2022]
            MemoryCopy(destination, source, (ulong)length);
        }

        /// <summary>
        /// Copies data from one memory location to another using an unmanaged memory pointers.
        /// </summary>
        /// <remarks>Uses low-level platform impl.</remarks>
        /// <param name="source">The source location.</param>
        /// <param name="destination">The destination location.</param>
        /// <param name="length">The length (amount of bytes to copy).</param>
        [LibraryImport("FlaxEngine", EntryPoint = "PlatformInternal_MemoryCopy")]
        public static partial void MemoryCopy(IntPtr destination, IntPtr source, ulong length);

        /// <summary>
        /// Clears the memory region with zeros.
        /// </summary>
        /// <remarks>Uses low-level platform impl.</remarks>
        /// <param name="dst">Destination memory address</param>
        /// <param name="size">Size of the memory to clear in bytes</param>
        [LibraryImport("FlaxEngine", EntryPoint = "PlatformInternal_MemoryClear")]
        public static partial void MemoryClear(IntPtr dst, ulong size);

        /// <summary>
        /// Compares two blocks of the memory.
        /// </summary>
        /// <remarks>Uses low-level platform impl.</remarks>
        /// <param name="buf1">The first buffer address.</param>
        /// <param name="buf2">The second buffer address.</param>
        /// <param name="size">Size of the memory to compare in bytes.</param>
        [LibraryImport("FlaxEngine", EntryPoint = "PlatformInternal_MemoryCompare")]
        public static partial int MemoryCompare(IntPtr buf1, IntPtr buf2, ulong size);

        /// <summary>
        /// Rounds the floating point value up to 1 decimal place.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The rounded result.</returns>
        public static float RoundTo1DecimalPlace(float value)
        {
            return (float)Math.Round(value * 10) / 10;
        }

        /// <summary>
        /// Rounds the floating point value up to 2 decimal places.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The rounded result.</returns>
        public static float RoundTo2DecimalPlaces(float value)
        {
            return (float)Math.Round(value * 100) / 100;
        }

        /// <summary>
        /// Rounds the floating point value up to 3 decimal places.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The rounded result.</returns>
        public static float RoundTo3DecimalPlaces(float value)
        {
            return (float)Math.Round(value * 1000) / 1000;
        }

        /// <summary>
        /// Gets the empty array of the given type (shared one).
        /// </summary>
        /// <typeparam name="T">The type.</typeparam>
        /// <returns>The empty array object.</returns>
        public static T[] GetEmptyArray<T>()
        {
#if USE_NETCORE
            return Array.Empty<T>();
#else
            return Enumerable.Empty<T>() as T[];
#endif
        }

        /// <summary>
        /// Determines whether two arrays are equal by comparing the elements by using the default equality comparer for their type.
        /// </summary>
        /// <typeparam name="T">The type of the elements of the input sequences.</typeparam>
        /// <param name="a1">The first array.</param>
        /// <param name="a2">The second array.</param>
        /// <returns><c>true</c> if the two source sequences are of equal length and their corresponding elements are equal according to the default equality comparer for their type; otherwise, <c>false</c>.</returns>
        public static bool ArraysEqual<T>(T[] a1, T[] a2)
        {
            if (ReferenceEquals(a1, a2))
                return true;

            if (a1 == null || a2 == null)
                return false;

            if (a1.Length != a2.Length)
                return false;

            var comparer = EqualityComparer<T>.Default;
            for (int i = 0; i < a1.Length; i++)
            {
                if (!comparer.Equals(a1[i], a2[i]))
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Determines whether two arrays are equal by comparing the elements by using the default equality comparer for their type.
        /// </summary>
        /// <typeparam name="T">The type of the elements of the input sequences.</typeparam>
        /// <param name="a1">The first array.</param>
        /// <param name="a2">The second array.</param>
        /// <returns><c>true</c> if the two source sequences are of equal length and their corresponding elements are equal according to the default equality comparer for their type; otherwise, <c>false</c>.</returns>
        public static bool ArraysEqual<T>(T[] a1, IReadOnlyList<T> a2)
        {
            if (a1 == null || a2 == null)
                return false;

            if (a1.Length != a2.Count)
                return false;

            var comparer = EqualityComparer<T>.Default;
            for (int i = 0; i < a1.Length; i++)
            {
                if (!comparer.Equals(a1[i], a2[i]))
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Determines whether two arrays are equal by comparing the elements by using the default equality comparer for their type.
        /// </summary>
        /// <typeparam name="T">The type of the elements of the input sequences.</typeparam>
        /// <param name="a1">The first array.</param>
        /// <param name="a2">The second array.</param>
        /// <returns><c>true</c> if the two source sequences are of equal length and their corresponding elements are equal according to the default equality comparer for their type; otherwise, <c>false</c>.</returns>
        public static bool ArraysEqual<T>(List<T> a1, List<T> a2)
        {
            if (ReferenceEquals(a1, a2))
                return true;

            if (a1 == null || a2 == null)
                return false;

            if (a1.Count != a2.Count)
                return false;

            var comparer = EqualityComparer<T>.Default;
            for (int i = 0; i < a1.Count; i++)
            {
                if (!comparer.Equals(a1[i], a2[i]))
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Gets all currently loaded assemblies in the runtime.
        /// </summary>
        /// <returns>List of assemblies</returns>
        public static Assembly[] GetAssemblies()
        {
#if USE_NETCORE
            return AssemblyLoadContext.Default.Assemblies.Concat(NativeInterop.scriptingAssemblyLoadContext.Assemblies).ToArray();
#else
            return AppDomain.CurrentDomain.GetAssemblies();
#endif
        }

        /// <summary>
        /// Gets the assembly with the given name.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>The assembly or null if not found.</returns>
        public static Assembly GetAssemblyByName(string name)
        {
            return GetAssemblyByName(name, GetAssemblies());
        }

        /// <summary>
        /// Gets the assembly with the given name.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="assemblies">The assemblies collection to search for.</param>
        /// <returns>The assembly or null if not found.</returns>
        public static Assembly GetAssemblyByName(string name, Assembly[] assemblies)
        {
            Assembly result = null;
            for (int i = 0; i < assemblies.Length; i++)
            {
                var assemblyName = assemblies[i].GetName();
                if (assemblyName.Name == name)
                {
                    result = assemblies[i];
                    break;
                }
            }
            return result;
        }

        /// <summary>
        /// Gets the location of the assembly.
        /// </summary>
        /// <param name="assembly">The assembly.</param>
        /// <returns>Path in the filesystem</returns>
        public static string GetAssemblyLocation(Assembly assembly)
        {
#if USE_NETCORE
            var location = assembly.Location;
            if (!string.IsNullOrEmpty(location))
                return location;
            if (Interop.NativeInterop.AssemblyLocations.TryGetValue(assembly.FullName, out location))
                return location;
            return null;
#else
            return assembly.Location;
#endif
        }

#if USE_MONO
        internal static T[] ExtractArrayFromList<T>(List<T> list)
        {
            return list != null ? (T[])Internal_ExtractArrayFromList(list) : null;
        }
#else
        private class ExtractArrayFromListContext<T>
        {
            public static FieldInfo itemsField;
        }

        internal static T[] ExtractArrayFromList<T>(List<T> list)
        {
            if (list == null)
                return null;

            if (ExtractArrayFromListContext<T>.itemsField == null)
            {
                Type listType = typeof(List<T>);
                ExtractArrayFromListContext<T>.itemsField = listType.GetField("_items", BindingFlags.NonPublic | BindingFlags.Instance);
            }

            return (T[])ExtractArrayFromListContext<T>.itemsField.GetValue(list); // boxing is slower;
        }
#endif

        internal static Float2[] ConvertCollection(Vector2[] v)
        {
            // [Deprecated on 26.05.2022, expires on 26.05.2024]
            Float2[] result = null;
            if (v != null && v.Length != 0)
            {
                result = new Float2[v.Length];
                for (int i = 0; i < v.Length; i++)
                    result[i] = v[i];
            }
            return result;
        }

        internal static List<Float2> ConvertCollection(List<Vector2> v)
        {
            // [Deprecated on 26.05.2022, expires on 26.05.2024]
            List<Float2> result = null;
            if (v != null && v.Count != 0)
            {
                result = new List<Float2>();
                result.Capacity = v.Count;
                for (int i = 0; i < v.Count; i++)
                    result.Add(v[i]);
            }
            return result;
        }

        internal static Float3[] ConvertCollection(Vector3[] v)
        {
            // [Deprecated on 26.05.2022, expires on 26.05.2024]
            Float3[] result = null;
            if (v != null && v.Length != 0)
            {
                result = new Float3[v.Length];
                for (int i = 0; i < v.Length; i++)
                    result[i] = v[i];
            }
            return result;
        }

        internal static List<Float3> ConvertCollection(List<Vector3> v)
        {
            // [Deprecated on 26.05.2022, expires on 26.05.2024]
            List<Float3> result = null;
            if (v != null && v.Count != 0)
            {
                result = new List<Float3>();
                result.Capacity = v.Count;
                for (int i = 0; i < v.Count; i++)
                    result.Add(v[i]);
            }
            return result;
        }

        internal static Float4[] ConvertCollection(Vector4[] v)
        {
            // [Deprecated on 26.05.2022, expires on 26.05.2024]
            Float4[] result = null;
            if (v != null && v.Length != 0)
            {
                result = new Float4[v.Length];
                for (int i = 0; i < v.Length; i++)
                    result[i] = v[i];
            }
            return result;
        }

        internal static List<Float4> ConvertCollection(List<Vector4> v)
        {
            // [Deprecated on 26.05.2022, expires on 26.05.2024]
            List<Float4> result = null;
            if (v != null && v.Count != 0)
            {
                result = new List<Float4>();
                result.Capacity = v.Count;
                for (int i = 0; i < v.Count; i++)
                    result.Add(v[i]);
            }
            return result;
        }

#if USE_NETCORE
#else
        [LibraryImport("FlaxEngine", EntryPoint = "UtilsInternal_ExtractArrayFromList")]
        [return: MarshalUsing(typeof(FlaxEngine.SystemArrayMarshaller))]
        internal static partial Array Internal_ExtractArrayFromList([MarshalUsing(typeof(FlaxEngine.GCHandleMarshaller))] object list);
#endif

        /// <summary>
        /// Reads the color from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Color32 ReadColor32(this BinaryReader stream)
        {
            return new Color32(stream.ReadByte(), stream.ReadByte(), stream.ReadByte(), stream.ReadByte());
        }

        /// <summary>
        /// Reads the color from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Color ReadColor(this BinaryReader stream)
        {
            return new Color(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Vector2 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Vector2 ReadVector2(this BinaryReader stream)
        {
            return new Vector2(stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Vector3 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Vector3 ReadVector3(this BinaryReader stream)
        {
            return new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Vector4 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Vector4 ReadVector4(this BinaryReader stream)
        {
            return new Vector4(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Vector2 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="useDouble">True to explicitly use 64-bit precision for serialized data, otherwise will use 32-bit precision.</param>
        /// <returns>The value.</returns>
        public static Vector2 ReadVector2(this BinaryReader stream, bool useDouble)
        {
            if (useDouble)
                return new Vector2(stream.ReadDouble(), stream.ReadDouble());
            return new Vector2(stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Vector3 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="useDouble">True to explicitly use 64-bit precision for serialized data, otherwise will use 32-bit precision.</param>
        /// <returns>The value.</returns>
        public static Vector3 ReadVector3(this BinaryReader stream, bool useDouble)
        {
            if (useDouble)
                return new Vector3(stream.ReadDouble(), stream.ReadDouble(), stream.ReadDouble());
            return new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Vector4 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="useDouble">True to explicitly use 64-bit precision for serialized data, otherwise will use 32-bit precision.</param>
        /// <returns>The value.</returns>
        public static Vector4 ReadVector4(this BinaryReader stream, bool useDouble)
        {
            if (useDouble)
                return new Vector4(stream.ReadDouble(), stream.ReadDouble(), stream.ReadDouble(), stream.ReadDouble());
            return new Vector4(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Float2 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Float2 ReadFloat2(this BinaryReader stream)
        {
            return new Float2(stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Float3 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Float3 ReadFloat3(this BinaryReader stream)
        {
            return new Float3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Float4 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Float4 ReadFloat4(this BinaryReader stream)
        {
            return new Float4(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Double2 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Double2 ReadDouble2(this BinaryReader stream)
        {
            return new Double2(stream.ReadDouble(), stream.ReadDouble());
        }

        /// <summary>
        /// Reads the Double3 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Double3 ReadDouble3(this BinaryReader stream)
        {
            return new Double3(stream.ReadDouble(), stream.ReadDouble(), stream.ReadDouble());
        }

        /// <summary>
        /// Reads the Double4 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Double4 ReadDouble4(this BinaryReader stream)
        {
            return new Double4(stream.ReadDouble(), stream.ReadDouble(), stream.ReadDouble(), stream.ReadDouble());
        }

        /// <summary>
        /// Reads the Int2 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Int2 ReadInt2(this BinaryReader stream)
        {
            return new Int2(stream.ReadInt32(), stream.ReadInt32());
        }

        /// <summary>
        /// Reads the Int3 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Int3 ReadInt3(this BinaryReader stream)
        {
            return new Int3(stream.ReadInt32(), stream.ReadInt32(), stream.ReadInt32());
        }

        /// <summary>
        /// Reads the Int4 from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Int4 ReadInt4(this BinaryReader stream)
        {
            return new Int4(stream.ReadInt32(), stream.ReadInt32(), stream.ReadInt32(), stream.ReadInt32());
        }

        /// <summary>
        /// Reads the Quaternion from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Quaternion ReadQuaternion(this BinaryReader stream)
        {
            return new Quaternion(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the BoundingBox from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static BoundingBox ReadBoundingBox(this BinaryReader stream)
        {
            return new BoundingBox(stream.ReadVector3(), stream.ReadVector3());
        }

        /// <summary>
        /// Reads the BoundingSphere from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static BoundingSphere ReadBoundingSphere(this BinaryReader stream)
        {
            return new BoundingSphere(stream.ReadVector3(), stream.ReadSingle());
        }

        /// <summary>
        /// Reads the Ray from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Ray ReadRay(this BinaryReader stream)
        {
            return new Ray(stream.ReadVector3(), stream.ReadVector3());
        }

        /// <summary>
        /// Reads the Transform from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Transform ReadTransform(this BinaryReader stream)
        {
            return new Transform(stream.ReadVector3(), stream.ReadQuaternion(), stream.ReadVector3());
        }

        /// <summary>
        /// Reads the Matrix from the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>The value.</returns>
        public static Matrix ReadMatrix(this BinaryReader stream)
        {
            return new Matrix(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        internal static byte[] ReadJsonBytes(this BinaryReader stream)
        {
            // ReadStream::ReadJson
            var engineBuild = stream.ReadInt32();
            var size = stream.ReadInt32();
            return stream.ReadBytes(size);
        }

        /// <summary>
        /// Deserializes object from Json by reading it as a raw data (ver+length+bytes).
        /// </summary>
        /// <remarks>Reads version number, data length and actual data bytes from the stream.</remarks>
        /// <param name="stream">The stream.</param>
        /// <param name="obj">The object to deserialize.</param>
        public static void ReadJson(this BinaryReader stream, ISerializable obj)
        {
            // ReadStream::ReadJson
            var engineBuild = stream.ReadInt32();
            var size = stream.ReadInt32();
            if (obj != null)
            {
                var data = stream.ReadBytes(size);
                Json.JsonSerializer.LoadFromBytes(obj, data, engineBuild);
            }
            else
                stream.BaseStream.Seek(size, SeekOrigin.Current);
        }

        /// <summary>
        /// Writes the color to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Color32 value)
        {
            stream.Write(value.R);
            stream.Write(value.G);
            stream.Write(value.B);
            stream.Write(value.A);
        }

        /// <summary>
        /// Writes the color to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Color value)
        {
            stream.Write(value.R);
            stream.Write(value.G);
            stream.Write(value.B);
            stream.Write(value.A);
        }

        /// <summary>
        /// Writes the Vector2 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Vector2 value)
        {
            stream.Write((float)value.X);
            stream.Write((float)value.Y);
        }

        /// <summary>
        /// Writes the Vector3 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Vector3 value)
        {
            stream.Write((float)value.X);
            stream.Write((float)value.Y);
            stream.Write((float)value.Z);
        }

        /// <summary>
        /// Writes the Vector4 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Vector4 value)
        {
            stream.Write((float)value.X);
            stream.Write((float)value.Y);
            stream.Write((float)value.Z);
            stream.Write((float)value.W);
        }

        /// <summary>
        /// Writes the Vector2 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        /// <param name="useDouble">True to explicitly use 64-bit precision for serialized data, otherwise will use 32-bit precision.</param>
        public static void Write(this BinaryWriter stream, Vector2 value, bool useDouble)
        {
            if (useDouble)
            {
                stream.Write((double)value.X);
                stream.Write((double)value.Y);
            }
            else
            {
                stream.Write((float)value.X);
                stream.Write((float)value.Y);
            }
        }

        /// <summary>
        /// Writes the Vector3 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        /// <param name="useDouble">True to explicitly use 64-bit precision for serialized data, otherwise will use 32-bit precision.</param>
        public static void Write(this BinaryWriter stream, Vector3 value, bool useDouble)
        {
            if (useDouble)
            {
                stream.Write((double)value.X);
                stream.Write((double)value.Y);
                stream.Write((double)value.Z);
            }
            else
            {
                stream.Write((float)value.X);
                stream.Write((float)value.Y);
                stream.Write((float)value.Z);
            }
        }

        /// <summary>
        /// Writes the Vector4 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        /// <param name="useDouble">True to explicitly use 64-bit precision for serialized data, otherwise will use 32-bit precision.</param>
        public static void Write(this BinaryWriter stream, Vector4 value, bool useDouble)
        {
            if (useDouble)
            {
                stream.Write((double)value.X);
                stream.Write((double)value.Y);
                stream.Write((double)value.Z);
                stream.Write((double)value.W);
            }
            else
            {
                stream.Write((float)value.X);
                stream.Write((float)value.Y);
                stream.Write((float)value.Z);
                stream.Write((float)value.W);
            }
        }

        /// <summary>
        /// Writes the Vector2 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Float2 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
        }

        /// <summary>
        /// Writes the Vector3 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Float3 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
            stream.Write(value.Z);
        }

        /// <summary>
        /// Writes the Vector4 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Float4 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
            stream.Write(value.Z);
            stream.Write(value.W);
        }

        /// <summary>
        /// Writes the Vector2 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Double2 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
        }

        /// <summary>
        /// Writes the Vector3 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Double3 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
            stream.Write(value.Z);
        }

        /// <summary>
        /// Writes the Vector4 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Double4 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
            stream.Write(value.Z);
            stream.Write(value.W);
        }

        /// <summary>
        /// Writes the Int2 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Int2 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
        }

        /// <summary>
        /// Writes the Int3 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Int3 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
            stream.Write(value.Z);
        }

        /// <summary>
        /// Writes the Int4 to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Int4 value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
            stream.Write(value.Z);
            stream.Write(value.W);
        }

        /// <summary>
        /// Writes the Quaternion to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Quaternion value)
        {
            stream.Write(value.X);
            stream.Write(value.Y);
            stream.Write(value.Z);
            stream.Write(value.W);
        }

        /// <summary>
        /// Writes the BoundingBox to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, BoundingBox value)
        {
            stream.Write(value.Minimum);
            stream.Write(value.Maximum);
        }

        /// <summary>
        /// Writes the BoundingSphere to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, BoundingSphere value)
        {
            stream.Write(value.Center);
            stream.Write(value.Radius);
        }

        /// <summary>
        /// Writes the Transform to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Transform value)
        {
            stream.Write(value.Translation);
            stream.Write(value.Orientation);
            stream.Write(value.Scale);
        }

        /// <summary>
        /// Writes the Rectangle to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Rectangle value)
        {
            stream.Write(value.Location);
            stream.Write(value.Size);
        }

        /// <summary>
        /// Writes the Ray to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Ray value)
        {
            stream.Write(value.Position);
            stream.Write(value.Direction);
        }

        /// <summary>
        /// Writes the Matrix to the binary stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="value">The value to write.</param>
        public static void Write(this BinaryWriter stream, Matrix value)
        {
            stream.Write(value.M11);
            stream.Write(value.M12);
            stream.Write(value.M13);
            stream.Write(value.M14);
            stream.Write(value.M21);
            stream.Write(value.M22);
            stream.Write(value.M23);
            stream.Write(value.M24);
            stream.Write(value.M31);
            stream.Write(value.M32);
            stream.Write(value.M33);
            stream.Write(value.M34);
            stream.Write(value.M41);
            stream.Write(value.M42);
            stream.Write(value.M43);
            stream.Write(value.M44);
        }

        internal static void WriteJsonBytes(this BinaryWriter stream, byte[] bytes)
        {
            // WriteStream::WriteJson
            stream.Write(Globals.EngineBuildNumber);
            if (bytes != null)
            {
                stream.Write(bytes.Length);
                stream.Write(bytes);
            }
            else
                stream.Write(0);
        }

        /// <summary>
        /// Serializes object to Json and writes it as a raw data (ver+length+bytes).
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <remarks>Writes version number, data length and actual data bytes to the stream.</remarks>
        /// <param name="obj">The object to serialize.</param>
        public static void WriteJson(this BinaryWriter stream, ISerializable obj)
        {
            // WriteStream::WriteJson
            stream.Write(Globals.EngineBuildNumber);
            if (obj != null)
            {
                var bytes = Json.JsonSerializer.SaveToBytes(obj);
                stream.Write(bytes.Length);
                stream.Write(bytes);
            }
            else
                stream.Write(0);
        }

        /// <summary>
        /// Initializes the structure (value type) by inflating it with values from <see cref="System.ComponentModel.DefaultValueAttribute"/> (recursive).
        /// </summary>
        /// <param name="obj">The object to initialize.</param>
        /// <param name="type">The structure type.</param>
        public static void InitStructure(object obj, Type type)
        {
            var fields = type.GetFields(BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public);
            for (var i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                var fieldType = field.FieldType;
                var attr = field.GetCustomAttribute<System.ComponentModel.DefaultValueAttribute>();
                if (attr != null)
                {
                    var value = attr.Value;
                    if (value != null && value.GetType() != fieldType)
                        value = Convert.ChangeType(value, fieldType);
                    field.SetValue(obj, value);
                }
                else if (fieldType.IsValueType)
                {
                    var fieldValue = Activator.CreateInstance(fieldType);
                    InitStructure(fieldValue, fieldType);
                    field.SetValue(obj, fieldValue);
                }
            }
        }

        /// <summary>
        /// Gets the array of method parameter types.
        /// </summary>
        /// <param name="method">The method to get it's parameters.</param>
        /// <returns>Method parameters array.</returns>
        public static Type[] GetParameterTypes(this MethodBase method)
        {
            Type[] parameterTypes;
            var parameters = method.GetParameters();
            if (parameters.Length != 0)
            {
                parameterTypes = new Type[parameters.Length];
                for (int i = 0; i < parameters.Length; i++)
                    parameterTypes[i] = parameters[i].ParameterType;
            }
            else
                parameterTypes = Array.Empty<Type>();
            return parameterTypes;
        }

        /// <summary>
        /// A category of number values used for formatting and input fields.
        /// </summary>
        public enum ValueCategory
        {
            /// <summary>
            /// Nothing.
            /// </summary>
            None,

            /// <summary>
            /// Distance (eg. meters).
            /// </summary>
            Distance,

            /// <summary>
            /// Area (eg. m^2).
            /// </summary>
            Area,

            /// <summary>
            /// Volume (eg. m^3).
            /// </summary>
            Volume,

            /// <summary>
            /// Mass (eg. kilograms).
            /// </summary>
            Mass,

            /// <summary>
            /// Angle (eg. degrees).
            /// </summary>
            Angle,

            /// <summary>
            /// Speed (distance / time).
            /// </summary>
            Speed,

            /// <summary>
            /// Acceleration (distance^2 / time).
            /// </summary>
            Acceleration,

            /// <summary>
            /// Time (eg. seconds).
            /// </summary>
            Time,

            /// <summary>
            /// Force (mass * distance / time^2).
            /// </summary>
            Force,

            /// <summary>
            /// Torque (mass * distance^2 / time^2).
            /// </summary>
            Torque,
        }
    }
}

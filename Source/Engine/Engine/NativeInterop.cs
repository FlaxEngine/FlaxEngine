// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if USE_NETCORE

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.IO;
using System.Runtime.CompilerServices;
using FlaxEngine.Assertions;
using FlaxEngine.Utilities;
using System.Runtime.InteropServices.Marshalling;
using System.Buffers;
using System.Collections.Concurrent;
using System.Text;
using System.Threading;

#pragma warning disable 1591

namespace FlaxEngine
{
    #region Native structures

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeClassDefinitions
    {
        internal ManagedHandle typeHandle;
        internal IntPtr name;
        internal IntPtr fullname;
        internal IntPtr @namespace;
        internal uint typeAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeMethodDefinitions
    {
        internal IntPtr name;
        internal int numParameters;
        internal ManagedHandle typeHandle;
        internal uint methodAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeFieldDefinitions
    {
        internal IntPtr name;
        internal ManagedHandle fieldHandle;
        internal ManagedHandle fieldTypeHandle;
        internal uint fieldAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativePropertyDefinitions
    {
        internal IntPtr name;
        internal ManagedHandle getterHandle;
        internal ManagedHandle setterHandle;
        internal uint getterFlags;
        internal uint setterFlags;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeAttributeDefinitions
    {
        internal IntPtr name;
        internal ManagedHandle attributeHandle;
        internal ManagedHandle attributeTypeHandle;
    }

    [StructLayout(LayoutKind.Explicit)]
    public struct VariantNative
    {
        [StructLayout(LayoutKind.Sequential)]
        internal struct VariantNativeType
        {
            internal VariantUtils.VariantType types;
            internal IntPtr TypeName; // char*
        }

        [FieldOffset(0)]
        VariantNativeType Type;

        [FieldOffset(8)]
        byte AsBool;

        [FieldOffset(8)]
        short AsInt16;

        [FieldOffset(8)]
        ushort AsUint16;

        [FieldOffset(8)]
        int AsInt;

        [FieldOffset(8)]
        uint AsUint;

        [FieldOffset(8)]
        long AsInt64;

        [FieldOffset(8)]
        ulong AsUint64;

        [FieldOffset(8)]
        float AsFloat;

        [FieldOffset(8)]
        double AsDouble;

        [FieldOffset(8)]
        IntPtr AsPointer;

        [FieldOffset(8)]
        int AsData0;

        [FieldOffset(12)]
        int AsData1;

        [FieldOffset(16)]
        int AsData2;

        [FieldOffset(20)]
        int AsData3;

        [FieldOffset(24)]
        int AsData4;

        [FieldOffset(28)]
        int AsData5;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct VersionNative
    {
        internal int _Major;
        internal int _Minor;
        internal int _Build;
        internal int _Revision;

        internal VersionNative(Version ver)
        {
            _Major = ver.Major;
            _Minor = ver.Minor;
            _Build = ver.Build;
            _Revision = ver.Revision;
        }

        internal Version GetVersion()
        {
            return new Version(_Major, _Minor, _Build, _Revision);
        }
    }

    #endregion

    #region Wrappers

    /// <summary>
    /// Wrapper for managed arrays which are passed to unmanaged code.
    /// </summary>
    internal unsafe class ManagedArray
    {
        private ManagedHandle pinnedArrayHandle;
        private IntPtr unmanagedData;
        private int elementSize;
        private int length;

        internal static ManagedArray WrapNewArray(Array arr) => new ManagedArray(arr);

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>
        /// The resources must be released by calling FreePooled() instead of Free()-method.
        /// </remarks>
        internal static ManagedArray WrapPooledArray(Array arr)
        {
            ManagedArray managedArray = ManagedArrayPool.Get();
            managedArray.WrapArray(arr);
            return managedArray;
        }

        internal static ManagedArray AllocateNewArray(int length, int elementSize)
            => new ManagedArray((IntPtr)NativeInterop.NativeAlloc(length, elementSize), length, elementSize);

        internal static ManagedArray AllocateNewArray(IntPtr ptr, int length, int elementSize)
            => new ManagedArray(ptr, length, elementSize);

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>
        /// The resources must be released by calling FreePooled() instead of Free()-method.
        /// </remarks>
        internal static ManagedArray AllocatePooledArray<T>(T* ptr, int length) where T : unmanaged
        {
            ManagedArray managedArray = ManagedArrayPool.Get();
            managedArray.Allocate(ptr, length);
            return managedArray;
        }

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>
        /// The resources must be released by calling FreePooled() instead of Free()-method.
        /// </remarks>
        internal static ManagedArray AllocatePooledArray<T>(int length) where T : unmanaged
        {
            ManagedArray managedArray = ManagedArrayPool.Get();
            managedArray.Allocate((IntPtr)NativeInterop.NativeAlloc(length, Unsafe.SizeOf<T>()), length, Unsafe.SizeOf<T>());
            return managedArray;
        }

        internal ManagedArray(Array arr) => WrapArray(arr);

        internal void WrapArray(Array arr)
        {
            pinnedArrayHandle = ManagedHandle.Alloc(arr, GCHandleType.Pinned);
            unmanagedData = Marshal.UnsafeAddrOfPinnedArrayElement(arr, 0);
            length = arr.Length;
            elementSize = Marshal.SizeOf(arr.GetType().GetElementType());
        }

        internal void Allocate<T>(T* ptr, int length) where T : unmanaged
        {
            unmanagedData = new IntPtr(ptr);
            this.length = length;
            elementSize = Unsafe.SizeOf<T>();
        }

        internal void Allocate(IntPtr ptr, int length, int elementSize)
        {
            unmanagedData = ptr;
            this.length = length;
            this.elementSize = elementSize;
        }

        private ManagedArray() { }

        private ManagedArray(IntPtr ptr, int length, int elementSize) => Allocate(ptr, length, elementSize);

        ~ManagedArray()
        {
            if (unmanagedData != IntPtr.Zero)
                Free();
        }

        internal void Free()
        {
            GC.SuppressFinalize(this);
            if (pinnedArrayHandle.IsAllocated)
            {
                pinnedArrayHandle.Free();
                unmanagedData = IntPtr.Zero;
            }
            if (unmanagedData != IntPtr.Zero)
            {
                NativeInterop.NativeFree(unmanagedData.ToPointer());
                unmanagedData = IntPtr.Zero;
            }
        }

        internal void FreePooled()
        {
            Free();
            ManagedArrayPool.Put(this);
        }

        internal IntPtr GetPointer => unmanagedData;

        internal int Length => length;

        internal int ElementSize => elementSize;

        internal Span<T> GetSpan<T>() where T : struct => new Span<T>(unmanagedData.ToPointer(), length);

        internal T[] GetArray<T>() where T : struct => new Span<T>(unmanagedData.ToPointer(), length).ToArray();

        /// <summary>
        /// Provides a pool of pre-allocated ManagedArray that can be re-used.
        /// </summary>
        private static class ManagedArrayPool
        {
            [ThreadStatic] private static List<ValueTuple<bool, ManagedArray>> pool;

            internal static ManagedArray Get()
            {
                if (pool == null)
                    pool = new List<ValueTuple<bool, ManagedArray>>();
                for (int i = 0; i < pool.Count; i++)
                {
                    if (!pool[i].Item1)
                    {
                        var tuple = pool[i];
                        tuple.Item1 = true;
                        pool[i] = tuple;
                        return tuple.Item2;
                    }
                }

                var newTuple = (true, new ManagedArray());
                pool.Add(newTuple);
                return newTuple.Item2;
            }

            internal static void Put(ManagedArray obj)
            {
                for (int i = 0; i < pool.Count; i++)
                {
                    if (pool[i].Item2 == obj)
                    {
                        var tuple = pool[i];
                        tuple.Item1 = false;
                        pool[i] = tuple;
                        return;
                    }
                }

                throw new Exception("Tried to free non-pooled ManagedArray as pooled ManagedArray");
            }
        }
    }

    internal static class ManagedString
    {
        internal static ManagedHandle EmptyStringHandle = ManagedHandle.Alloc(string.Empty);

        [System.Diagnostics.DebuggerStepThrough]
        internal static unsafe IntPtr ToNative(string str)
        {
            if (str == null)
                return IntPtr.Zero;
            else if (str == string.Empty)
                return ManagedHandle.ToIntPtr(EmptyStringHandle);
            Assert.IsTrue(str.Length > 0);
            return ManagedHandle.ToIntPtr(ManagedHandle.Alloc(str));
        }

        [System.Diagnostics.DebuggerStepThrough]
        internal static unsafe IntPtr ToNativeWeak(string str)
        {
            if (str == null)
                return IntPtr.Zero;
            else if (str == string.Empty)
                return ManagedHandle.ToIntPtr(EmptyStringHandle);
            Assert.IsTrue(str.Length > 0);
            return ManagedHandle.ToIntPtr(ManagedHandle.Alloc(str, GCHandleType.Weak));
        }

        [System.Diagnostics.DebuggerStepThrough]
        internal static string ToManaged(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return null;
            return Unsafe.As<string>(ManagedHandle.FromIntPtr(ptr).Target);
        }

        [System.Diagnostics.DebuggerStepThrough]
        internal static void Free(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return;
            ManagedHandle handle = ManagedHandle.FromIntPtr(ptr);
            if (handle == EmptyStringHandle)
                return;
            handle.Free();
        }
    }

    /// <summary>
    /// Handle to managed objects which can be stored in native code.
    /// </summary>
    public struct ManagedHandle
    {
        private IntPtr handle;

        private ManagedHandle(IntPtr handle)
        {
            this.handle = handle;
        }

        private ManagedHandle(object value, GCHandleType type)
        {
            handle = ManagedHandlePool.AllocateHandle(value, type);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle Alloc(object value) => new ManagedHandle(value, GCHandleType.Normal);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle Alloc(object value, GCHandleType type) => new ManagedHandle(value, type);

        public void Free()
        {
            if (handle != IntPtr.Zero)
            {
                ManagedHandlePool.FreeHandle(handle);
                handle = IntPtr.Zero;
            }

            ManagedHandlePool.TryCollectWeakHandles();
        }

        public object Target
        {
            get => ManagedHandlePool.GetObject(handle);
            set => ManagedHandlePool.SetObject(handle, value);
        }

        public bool IsAllocated => handle != IntPtr.Zero;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static explicit operator ManagedHandle(IntPtr value) => FromIntPtr(value);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle FromIntPtr(IntPtr value) => new ManagedHandle(value);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static explicit operator IntPtr(ManagedHandle value) => ToIntPtr(value);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntPtr ToIntPtr(ManagedHandle value) => value.handle;

        public override int GetHashCode() => handle.GetHashCode();

        public override bool Equals(object o) => o is ManagedHandle other && Equals(other);

        public bool Equals(ManagedHandle other) => handle == other.handle;

        public static bool operator ==(ManagedHandle a, ManagedHandle b) => a.handle == b.handle;

        public static bool operator !=(ManagedHandle a, ManagedHandle b) => a.handle != b.handle;

        private static class ManagedHandlePool
        {
            private static ulong normalHandleAccumulator = 0;
            private static ulong pinnedHandleAccumulator = 0;
            private static ulong weakHandleAccumulator = 0;

            private static object poolLock = new object();
            private static Dictionary<IntPtr, object> persistentPool = new Dictionary<nint, object>();
            private static Dictionary<IntPtr, GCHandle> pinnedPool = new Dictionary<nint, GCHandle>();

            private static Dictionary<IntPtr, object> weakPool1 = new Dictionary<nint, object>();
            private static Dictionary<IntPtr, object> weakPool2 = new Dictionary<nint, object>();
            private static Dictionary<IntPtr, object> weakPool = weakPool1;
            private static Dictionary<IntPtr, object> weakPoolOther = weakPool2;

            private static int nextCollection = GC.CollectionCount(0) + 1;

            /// <summary>
            /// Tries to free all references to old weak handles so GC can collect them.
            /// </summary>
            internal static void TryCollectWeakHandles()
            {
                if (GC.CollectionCount(0) < nextCollection)
                    return;

                lock (poolLock)
                {
                    nextCollection = GC.CollectionCount(0) + 1;

                    var swap = weakPoolOther;
                    weakPoolOther = weakPool;
                    weakPool = swap;

                    weakPool.Clear();
                }
            }

            private static IntPtr NewHandle(GCHandleType type)
            {
                IntPtr handle;
                if (type == GCHandleType.Normal)
                    handle = (IntPtr)Interlocked.Increment(ref normalHandleAccumulator);
                else if (type == GCHandleType.Pinned)
                    handle = (IntPtr)Interlocked.Increment(ref pinnedHandleAccumulator);
                else //if (type == GCHandleType.Weak || type == GCHandleType.WeakTrackResurrection)
                    handle = (IntPtr)Interlocked.Increment(ref weakHandleAccumulator);

                // Two bits reserved for the type
                handle |= (IntPtr)(((ulong)type << 62) & 0xC000000000000000);
                return handle;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static GCHandleType GetHandleType(IntPtr handle)
            {
                return (GCHandleType)(((ulong)handle & 0xC000000000000000) >> 62);
            }

            internal static IntPtr AllocateHandle(object value, GCHandleType type)
            {
                IntPtr handle = NewHandle(type);
                lock (poolLock)
                {
                    if (type == GCHandleType.Normal)
                        persistentPool.Add(handle, value);
                    else if (type == GCHandleType.Pinned)
                        pinnedPool.Add(handle, GCHandle.Alloc(value, GCHandleType.Pinned));
                    else if (type == GCHandleType.Weak || type == GCHandleType.WeakTrackResurrection)
                        weakPool.Add(handle, value);
                }

                return handle;
            }

            internal static object GetObject(IntPtr handle)
            {
                object value;
                GCHandleType type = GetHandleType(handle);
                lock (poolLock)
                {
                    if (type == GCHandleType.Normal && persistentPool.TryGetValue(handle, out value))
                        return value;
                    else if (type == GCHandleType.Pinned && pinnedPool.TryGetValue(handle, out GCHandle gchandle))
                        return gchandle.Target;
                    else if (weakPool.TryGetValue(handle, out value))
                        return value;
                    else if (weakPoolOther.TryGetValue(handle, out value))
                        return value;
                }

                throw new Exception("Invalid ManagedHandle");
            }

            internal static void SetObject(IntPtr handle, object value)
            {
                GCHandleType type = GetHandleType(handle);
                lock (poolLock)
                {
                    if (type == GCHandleType.Normal && persistentPool.ContainsKey(handle))
                        persistentPool[handle] = value;
                    else if (type == GCHandleType.Pinned && pinnedPool.TryGetValue(handle, out GCHandle gchandle))
                        gchandle.Target = value;
                    else if (weakPool.ContainsKey(handle))
                        weakPool[handle] = value;
                    else if (weakPoolOther.ContainsKey(handle))
                        weakPoolOther[handle] = value;
                    else
                        throw new Exception("Invalid ManagedHandle");
                }
            }

            internal static void FreeHandle(IntPtr handle)
            {
                GCHandleType type = GetHandleType(handle);
                lock (poolLock)
                {
                    if (type == GCHandleType.Normal && persistentPool.Remove(handle))
                        return;
                    else if (type == GCHandleType.Pinned && pinnedPool.Remove(handle, out GCHandle gchandle))
                    {
                        gchandle.Free();
                        return;
                    }
                    else if (weakPool.Remove(handle))
                        return;
                    else if (weakPoolOther.Remove(handle))
                        return;
                }

                throw new Exception("Invalid ManagedHandle");
            }
        }
    }

    #endregion

    #region Marshallers

    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedIn, typeof(ManagedHandleMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedOut, typeof(ManagedHandleMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementIn, typeof(ManagedHandleMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedOut, typeof(ManagedHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedIn, typeof(ManagedHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementOut, typeof(ManagedHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedRef, typeof(ManagedHandleMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedRef, typeof(ManagedHandleMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementRef, typeof(ManagedHandleMarshaller))]
    public static class ManagedHandleMarshaller
    {
        public static class NativeToManaged
        {
            public static object ConvertToManaged(IntPtr unmanaged) => unmanaged == IntPtr.Zero ? null : ManagedHandle.FromIntPtr(unmanaged).Target;
            public static void Free(IntPtr unmanaged)
            {
                // This is a permanent handle, do not release it
            }
        }
        public static class ManagedToNative
        {
            public static IntPtr ConvertToUnmanaged(object managed) => managed != null ? ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed)) : IntPtr.Zero;
            public static void Free(IntPtr unmanaged)
            {
                if (unmanaged == IntPtr.Zero)
                    return;
                ManagedHandle.FromIntPtr(unmanaged).Free();
            }
        }
        public struct Bidirectional
        {
            object managed;
            IntPtr unmanaged;
            public void FromManaged(object managed) { this.managed = managed; }
            public IntPtr ToUnmanaged() { unmanaged = ManagedHandleMarshaller.ToNative(managed); return unmanaged; }
            public void FromUnmanaged(IntPtr unmanaged) { this.unmanaged = unmanaged; }
            public object ToManaged() { managed = ManagedHandleMarshaller.ToManaged(unmanaged); unmanaged = IntPtr.Zero; return managed; }
            public void Free()
            {
                // FIXME, might be a permanent handle or a temporary one
                throw new NotImplementedException();
                /*if (unmanaged == IntPtr.Zero)
                    return;
                unmanaged.Free();*/
            }
        }
        public static object ConvertToManaged(IntPtr unmanaged) => ToManaged(unmanaged);
        public static IntPtr ConvertToUnmanaged(object managed) => ToNative(managed);
        public static object ToManaged(IntPtr managed) => managed != IntPtr.Zero ? ManagedHandle.FromIntPtr(managed).Target : null;
        public static IntPtr ToNative(object managed) => managed != null ? ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed)) : IntPtr.Zero;
        public static void Free(IntPtr unmanaged)
        {
            if (unmanaged == IntPtr.Zero)
                return;
            ManagedHandle.FromIntPtr(unmanaged).Free();
        }
    }

    [CustomMarshaller(typeof(Type), MarshalMode.Default, typeof(SystemTypeMarshaller))]
    public static class SystemTypeMarshaller
    {
        public static Type ConvertToManaged(IntPtr unmanaged) => Unsafe.As<Type>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));

        public static IntPtr ConvertToUnmanaged(Type managed)
        {
            if (managed == null)
                return IntPtr.Zero;
            ManagedHandle handle = NativeInterop.GetTypeGCHandle(managed);
            return ManagedHandle.ToIntPtr(handle);
        }

        public static void Free(IntPtr unmanaged)
        {
            // Cached handle, do not release
        }
    }

    [CustomMarshaller(typeof(Exception), MarshalMode.Default, typeof(ExceptionMarshaller))]
    public static class ExceptionMarshaller
    {
        public static Exception ConvertToManaged(IntPtr unmanaged) => Unsafe.As<Exception>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));
        public static IntPtr ConvertToUnmanaged(Exception managed) => ManagedHandleMarshaller.ConvertToUnmanaged(managed);
        public static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.Free(unmanaged);
    }

    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ManagedToUnmanagedIn, typeof(ObjectMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.UnmanagedToManagedOut, typeof(ObjectMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ElementIn, typeof(ObjectMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ManagedToUnmanagedOut, typeof(ObjectMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.UnmanagedToManagedIn, typeof(ObjectMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ElementOut, typeof(ObjectMarshaller.NativeToManaged))]
    public static class ObjectMarshaller
    {
        public static class NativeToManaged
        {
            public static FlaxEngine.Object ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<FlaxEngine.Object>(ManagedHandle.FromIntPtr(unmanaged).Target) : null;
        }
        public static class ManagedToNative
        {
            public static IntPtr ConvertToUnmanaged(FlaxEngine.Object managed) => managed != null ? ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed)) : IntPtr.Zero;
        }
    }

    [CustomMarshaller(typeof(CultureInfo), MarshalMode.Default, typeof(CultureInfoMarshaller))]
    public static class CultureInfoMarshaller
    {
        public static CultureInfo ConvertToManaged(IntPtr unmanaged) => Unsafe.As<CultureInfo>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));
        public static IntPtr ConvertToUnmanaged(CultureInfo managed) => ManagedHandleMarshaller.ConvertToUnmanaged(managed);
        public static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.Free(unmanaged);
    }

    [CustomMarshaller(typeof(Array), MarshalMode.ManagedToUnmanagedIn, typeof(SystemArrayMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(Array), MarshalMode.UnmanagedToManagedOut, typeof(SystemArrayMarshaller.ManagedToNative))]
    public static class SystemArrayMarshaller
    {
        public struct ManagedToNative
        {
            ManagedArray managedArray;
            ManagedHandle handle;

            public void FromManaged(Array managed)
            {
                if (managed != null)
                    managedArray = ManagedArray.WrapPooledArray(managed);
            }

            public IntPtr ToUnmanaged()
            {
                if (managedArray == null)
                    return IntPtr.Zero;
                handle = ManagedHandle.Alloc(managedArray);
                return ManagedHandle.ToIntPtr(handle);
            }

            public void Free()
            {
                if (managedArray != null)
                {
                    managedArray.FreePooled();
                    handle.Free();
                }
            }
        }
    }

    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.ManagedToUnmanagedIn, typeof(DictionaryMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.UnmanagedToManagedOut, typeof(DictionaryMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.ElementIn, typeof(DictionaryMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.ManagedToUnmanagedOut, typeof(DictionaryMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.UnmanagedToManagedIn, typeof(DictionaryMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.ElementOut, typeof(DictionaryMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.ManagedToUnmanagedRef, typeof(DictionaryMarshaller<,>.Bidirectional))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.UnmanagedToManagedRef, typeof(DictionaryMarshaller<,>.Bidirectional))]
    [CustomMarshaller(typeof(Dictionary<,>), MarshalMode.ElementRef, typeof(DictionaryMarshaller<,>))]
    public static unsafe class DictionaryMarshaller<T, U>
    {
        public static class NativeToManaged
        {
            public static Dictionary<T, U> ConvertToManaged(IntPtr unmanaged) => DictionaryMarshaller<T, U>.ToManaged(unmanaged);
            public static void Free(IntPtr unmanaged) => DictionaryMarshaller<T, U>.Free(unmanaged);
        }
        public static class ManagedToNative
        {
            public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => DictionaryMarshaller<T, U>.ToNative(managed);
            public static void Free(IntPtr unmanaged) => DictionaryMarshaller<T, U>.Free(unmanaged);
        }

        public struct Bidirectional
        {
            Dictionary<T, U> managed;
            IntPtr unmanaged;

            public void FromManaged(Dictionary<T, U> managed) => this.managed = managed;

            public IntPtr ToUnmanaged()
            {
                unmanaged = DictionaryMarshaller<T, U>.ToNative(managed);
                return unmanaged;
            }

            public void FromUnmanaged(IntPtr unmanaged) => this.unmanaged = unmanaged;

            public Dictionary<T, U> ToManaged()
            {
                managed = DictionaryMarshaller<T, U>.ToManaged(unmanaged);
                unmanaged = IntPtr.Zero;
                return managed;
            }

            public void Free() => DictionaryMarshaller<T, U>.Free(unmanaged);
        }

        public static Dictionary<T, U> ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<Dictionary<T, U>>(ManagedHandle.FromIntPtr(unmanaged).Target) : null;
        public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => managed != null ? ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed)) : IntPtr.Zero;

        public static Dictionary<T, U> ToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<Dictionary<T, U>>(ManagedHandle.FromIntPtr(unmanaged).Target) : null;
        public static IntPtr ToNative(Dictionary<T, U> managed) => managed != null ? ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed)) : IntPtr.Zero;
        public static void Free(IntPtr unmanaged)
        {
            if (unmanaged != IntPtr.Zero)
                ManagedHandle.FromIntPtr(unmanaged).Free();
        }
    }

    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ManagedToUnmanagedIn, typeof(ArrayMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.UnmanagedToManagedOut, typeof(ArrayMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ElementIn, typeof(ArrayMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ManagedToUnmanagedOut, typeof(ArrayMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.UnmanagedToManagedIn, typeof(ArrayMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ElementOut, typeof(ArrayMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ManagedToUnmanagedRef, typeof(ArrayMarshaller<,>.Bidirectional))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.UnmanagedToManagedRef, typeof(ArrayMarshaller<,>.Bidirectional))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ElementRef, typeof(ArrayMarshaller<,>))]
    [ContiguousCollectionMarshaller]
    public static unsafe class ArrayMarshaller<T, TUnmanagedElement> where TUnmanagedElement : unmanaged
    {
        public static class NativeToManaged
        {
            public static T[]? AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged is null)
                    return null;
                return new T[numElements];
            }

            public static Span<T> GetManagedValuesDestination(T[]? managed) => managed;

            public static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return ReadOnlySpan<TUnmanagedElement>.Empty;
                ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.GetSpan<TUnmanagedElement>();
            }

            public static void Free(TUnmanagedElement* unmanaged)
            {
                if (unmanaged == null)
                    return;
                ManagedHandle handle = ManagedHandle.FromIntPtr(new IntPtr(unmanaged));
                (Unsafe.As<ManagedArray>(handle.Target)).Free();
                handle.Free();
            }

            public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;
                ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.GetSpan<TUnmanagedElement>();
            }
        }

        public static class ManagedToNative
        {
            public static TUnmanagedElement* AllocateContainerForUnmanagedElements(T[]? managed, out int numElements)
            {
                if (managed is null)
                {
                    numElements = 0;
                    return null;
                }
                numElements = managed.Length;
                ManagedArray managedArray = ManagedArray.AllocatePooledArray<TUnmanagedElement>(managed.Length);
                var ptr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managedArray));
                return (TUnmanagedElement*)ptr;
            }

            public static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed) => managed;

            public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;
                ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.GetSpan<TUnmanagedElement>();
            }

            public static void Free(TUnmanagedElement* unmanaged)
            {
                if (unmanaged == null)
                    return;
                ManagedHandle handle = ManagedHandle.FromIntPtr(new IntPtr(unmanaged));
                (Unsafe.As<ManagedArray>(handle.Target)).FreePooled();
                handle.Free();
            }
        }

        public struct Bidirectional
        {
            T[] managedArray;
            ManagedArray unmanagedArray;
            ManagedHandle handle;

            public void FromManaged(T[]? managed)
            {
                if (managed == null)
                    return;
                managedArray = managed;
                unmanagedArray = ManagedArray.AllocatePooledArray<TUnmanagedElement>(managed.Length);
                handle = ManagedHandle.Alloc(unmanagedArray);
            }

            public ReadOnlySpan<T> GetManagedValuesSource() => managedArray;

            public Span<TUnmanagedElement> GetUnmanagedValuesDestination()
            {
                if (unmanagedArray == null)
                    return Span<TUnmanagedElement>.Empty;
                return unmanagedArray.GetSpan<TUnmanagedElement>();
            }

            public TUnmanagedElement* ToUnmanaged() => (TUnmanagedElement*)ManagedHandle.ToIntPtr(handle);

            public void FromUnmanaged(TUnmanagedElement* unmanaged)
            {
                ManagedArray arr = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                if (managedArray == null || managedArray.Length != arr.Length)
                    managedArray = new T[arr.Length];
            }

            public ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(int numElements)
            {
                if (unmanagedArray == null)
                    return ReadOnlySpan<TUnmanagedElement>.Empty;
                return unmanagedArray.GetSpan<TUnmanagedElement>();
            }

            public Span<T> GetManagedValuesDestination(int numElements) => managedArray;

            public T[] ToManaged() => managedArray;

            public void Free()
            {
                unmanagedArray.FreePooled();
                handle.Free();
            }
        }

        public static TUnmanagedElement* AllocateContainerForUnmanagedElements(T[]? managed, out int numElements)
        {
            if (managed is null)
            {
                numElements = 0;
                return null;
            }
            numElements = managed.Length;
            ManagedArray managedArray = ManagedArray.AllocatePooledArray<TUnmanagedElement>(managed.Length);
            IntPtr handle = ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managedArray));
            return (TUnmanagedElement*)handle;
        }

        public static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed) => managed;

        public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return Span<TUnmanagedElement>.Empty;
            ManagedArray unmanagedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
            return unmanagedArray.GetSpan<TUnmanagedElement>();
        }

        public static T[]? AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements) => unmanaged is null ? null : new T[numElements];

        public static Span<T> GetManagedValuesDestination(T[]? managed) => managed;

        public static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return ReadOnlySpan<TUnmanagedElement>.Empty;
            ManagedArray array = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
            return array.GetSpan<TUnmanagedElement>();
        }

        public static void Free(TUnmanagedElement* unmanaged)
        {
            if (unmanaged == null)
                return;
            ManagedHandle handle = ManagedHandle.FromIntPtr(new IntPtr(unmanaged));
            Unsafe.As<ManagedArray>(handle.Target).FreePooled();
            handle.Free();
        }
    }

    [CustomMarshaller(typeof(string), MarshalMode.ManagedToUnmanagedIn, typeof(StringMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(string), MarshalMode.UnmanagedToManagedOut, typeof(StringMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(string), MarshalMode.ElementIn, typeof(StringMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(string), MarshalMode.ManagedToUnmanagedOut, typeof(StringMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(string), MarshalMode.UnmanagedToManagedIn, typeof(StringMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(string), MarshalMode.ElementOut, typeof(StringMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(string), MarshalMode.ManagedToUnmanagedRef, typeof(StringMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(string), MarshalMode.UnmanagedToManagedRef, typeof(StringMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(string), MarshalMode.ElementRef, typeof(StringMarshaller))]
    public static class StringMarshaller
    {
        public static class NativeToManaged
        {
            public static string ConvertToManaged(IntPtr unmanaged) => ManagedString.ToManaged(unmanaged);
            public static void Free(IntPtr unmanaged) => ManagedString.Free(unmanaged);
        }

        public static class ManagedToNative
        {
            public static unsafe IntPtr ConvertToUnmanaged(string managed)
            {
                if (managed == null)
                    return IntPtr.Zero;
                return ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed));
            }

            public static void Free(IntPtr unmanaged) => ManagedString.Free(unmanaged);
        }

        public struct Bidirectional
        {
            string managed;
            IntPtr unmanaged;

            public void FromManaged(string managed) => this.managed = managed;

            public IntPtr ToUnmanaged()
            {
                unmanaged = ManagedString.ToNative(managed);
                return unmanaged;
            }

            public void FromUnmanaged(IntPtr unmanaged) => this.unmanaged = unmanaged;

            public string ToManaged()
            {
                managed = ManagedString.ToManaged(unmanaged);
                unmanaged = IntPtr.Zero;
                return managed;
            }

            public void Free() => ManagedString.Free(unmanaged);
        }

        public static string ConvertToManaged(IntPtr unmanaged) => ManagedString.ToManaged(unmanaged);
        public static IntPtr ConvertToUnmanaged(string managed) => ManagedString.ToNative(managed);
        public static void Free(IntPtr unmanaged) => ManagedString.Free(unmanaged);

        public static string ToManaged(IntPtr unmanaged) => ManagedString.ToManaged(unmanaged);
        public static IntPtr ToNative(string managed) => ManagedString.ToNative(managed);
    }

    #endregion

    /// <summary>
    /// Provides a Mono-like API for native code to access managed runtime.
    /// </summary>
    internal unsafe static partial class NativeInterop
    {
        internal static Dictionary<string, string> AssemblyLocations = new();

        private static bool firstAssemblyLoaded = false;

        private static Dictionary<string, Type> typeCache = new();

        private static IntPtr boolTruePtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc((int)1, GCHandleType.Pinned));
        private static IntPtr boolFalsePtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc((int)0, GCHandleType.Pinned));

        private static List<ManagedHandle> methodHandles = new();
        private static List<ManagedHandle> methodHandlesCollectible = new();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegates = new();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegatesCollectible = new();
        private static Dictionary<Type, ManagedHandle> typeHandleCache = new();
        private static Dictionary<Type, ManagedHandle> typeHandleCacheCollectible = new();
        private static List<ManagedHandle> fieldHandleCache = new();
        private static List<ManagedHandle> fieldHandleCacheCollectible = new();
        private static Dictionary<object, ManagedHandle> classAttributesCacheCollectible = new();
        private static Dictionary<Assembly, ManagedHandle> assemblyHandles = new();

        private static Dictionary<string, IntPtr> loadedNativeLibraries = new();
        internal static Dictionary<string, string> nativeLibraryPaths = new();
        private static Dictionary<Assembly, string> assemblyOwnedNativeLibraries = new();
        internal static AssemblyLoadContext scriptingAssemblyLoadContext;

        [System.Diagnostics.DebuggerStepThrough]
        private static IntPtr NativeLibraryImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? dllImportSearchPath)
        {
            if (!loadedNativeLibraries.TryGetValue(libraryName, out IntPtr nativeLibrary))
            {
                nativeLibrary = NativeLibrary.Load(nativeLibraryPaths[libraryName], assembly, dllImportSearchPath);
                loadedNativeLibraries.Add(libraryName, nativeLibrary);
                assemblyOwnedNativeLibraries.Add(assembly, libraryName);
            }
            return nativeLibrary;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void Init()
        {
            NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), NativeLibraryImportResolver);

            // Change default culture to match with Mono runtime default culture
            CultureInfo.DefaultThreadCurrentCulture = CultureInfo.InvariantCulture;
            CultureInfo.DefaultThreadCurrentUICulture = CultureInfo.InvariantCulture;
            System.Threading.Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
            System.Threading.Thread.CurrentThread.CurrentUICulture = CultureInfo.InvariantCulture;

#if FLAX_EDITOR
            var isCollectible = true;
#else
            var isCollectible = false;
#endif
            scriptingAssemblyLoadContext = new AssemblyLoadContext("Flax", isCollectible);
            DelegateHelpers.InitMethods();
        }

        [UnmanagedCallersOnly]
        internal static unsafe void Exit()
        {
        }

        [UnmanagedCallersOnly]
        internal static void RegisterNativeLibrary(IntPtr moduleName_, IntPtr modulePath_)
        {
            string moduleName = Marshal.PtrToStringAnsi(moduleName_);
            string modulePath = Marshal.PtrToStringAnsi(modulePath_);
            nativeLibraryPaths[moduleName] = modulePath;
        }

        internal static void* NativeAlloc(int byteCount)
        {
            return NativeMemory.AlignedAlloc((UIntPtr)byteCount, 16);
        }

        internal static void* NativeAlloc(int elementCount, int elementSize)
        {
            return NativeMemory.AlignedAlloc((UIntPtr)(elementCount * elementSize), 16);
        }

        internal static IntPtr NativeAllocStringAnsi(string str)
        {
            if (str is null)
                return IntPtr.Zero;

            int length = str.Length + 1;
            void* ptr = NativeMemory.AlignedAlloc((UIntPtr)length, 16);
            Span<byte> byteSpan = new Span<byte>(ptr, length);
            Encoding.ASCII.GetBytes(str, byteSpan);
            byteSpan[length - 1] = 0;

            return (IntPtr)ptr;
        }

        internal static void NativeFree(void* ptr)
        {
            NativeMemory.AlignedFree(ptr);
        }

        internal static T[] GCHandleArrayToManagedArray<T>(ManagedArray ptrArray) where T : class
        {
            Span<IntPtr> span = ptrArray.GetSpan<IntPtr>();
            T[] managedArray = new T[ptrArray.Length];
            for (int i = 0; i < managedArray.Length; i++)
                managedArray[i] = span[i] != IntPtr.Zero ? (T)ManagedHandle.FromIntPtr(span[i]).Target : default;
            return managedArray;
        }

        internal static IntPtr[] ManagedArrayToGCHandleArray(Array array)
        {
            IntPtr[] pointerArray = new IntPtr[array.Length];
            for (int i = 0; i < pointerArray.Length; i++)
            {
                var obj = array.GetValue(i);
                if (obj != null)
                    pointerArray.SetValue(ManagedHandle.ToIntPtr(ManagedHandle.Alloc(obj)), i);
            }
            return pointerArray;
        }

        internal static T[] NativeArrayToManagedArray<T, U>(Span<U> nativeSpan, Func<U, T> toManagedFunc)
        {
            T[] managedArray = new T[nativeSpan.Length];
            for (int i = 0; i < nativeSpan.Length; i++)
                managedArray[i] = toManagedFunc(nativeSpan[i]);
            return managedArray;
        }

        private static Type FindType(string typeName)
        {
            if (typeCache.TryGetValue(typeName, out Type type))
                return type;

            type = Type.GetType(typeName, ResolveAssemblyByName, null);
            if (type == null)
            {
                foreach (var assembly in scriptingAssemblyLoadContext.Assemblies)
                {
                    type = assembly.GetType(typeName);
                    if (type != null)
                        break;
                }
            }

            if (type == null)
            {
                string oldTypeName = typeName;
                typeName = typeName.Substring(0, typeName.IndexOf(','));
                type = Type.GetType(typeName, ResolveAssemblyByName, null);
                if (type == null)
                {
                    foreach (var assembly in scriptingAssemblyLoadContext.Assemblies)
                    {
                        type = assembly.GetType(typeName);
                        if (type != null)
                            break;
                    }
                }
                typeName = oldTypeName;
            }

            typeCache.Add(typeName, type);

            return type;
        }

        private static Assembly ResolveAssemblyByName(AssemblyName assemblyName)
        {
            foreach (Assembly assembly in scriptingAssemblyLoadContext.Assemblies)
                if (assembly.GetName() == assemblyName)
                    return assembly;
            return null;
        }

        internal static bool IsBlittable(Type type)
        {
            if (type.IsPrimitive || type.IsEnum)
                return true;
            if (type.IsArray && IsBlittable(type.GetElementType()))
                return true;
            if (type.IsPointer && type.HasElementType && type.GetElementType().IsPrimitive)
                return true;
            if (type.IsClass)
                return false;
            if (type.IsValueType)
            {
                var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
                foreach (FieldInfo field in fields)
                {
                    if (!IsBlittable(field.FieldType))
                        return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Returns blittable internal type for given type.
        /// </summary>
        internal static Type GetInternalType(Type type)
        {
            string[] splits = type.AssemblyQualifiedName.Split(',');
            string @namespace = string.Join('.', splits[0].Split('.').SkipLast(1));
            string className = @namespace.Length > 0 ? splits[0].Substring(@namespace.Length + 1) : splits[0];
            string parentClassName = "";
            if (className.Contains('+'))
            {
                parentClassName = className.Substring(0, className.LastIndexOf('+') + 1);
                className = className.Substring(parentClassName.Length);
            }
            string marshallerName = className + "Marshaller";
            string internalAssemblyQualifiedName = $"{@namespace}.{parentClassName}{marshallerName}+{className}Internal,{String.Join(',', splits.Skip(1))}";
            return FindType(internalAssemblyQualifiedName);
        }

        internal class ReferenceTypePlaceholder { }
        internal struct ValueTypePlaceholder { }

        internal delegate object MarshalToManagedDelegate(IntPtr nativePtr, bool byRef);
        internal delegate void MarshalToNativeDelegate(object managedObject, IntPtr nativePtr);
        internal delegate void MarshalToNativeFieldDelegate(FieldInfo field, object fieldOwner, IntPtr nativePtr, out int fieldOffset);

        internal static ConcurrentDictionary<Type, MarshalToManagedDelegate> toManagedMarshallers = new ConcurrentDictionary<Type, MarshalToManagedDelegate>(1, 3);
        internal static ConcurrentDictionary<Type, MarshalToNativeDelegate> toNativeMarshallers = new ConcurrentDictionary<Type, MarshalToNativeDelegate>(1, 3);
        internal static ConcurrentDictionary<Type, MarshalToNativeFieldDelegate> toNativeFieldMarshallers = new ConcurrentDictionary<Type, MarshalToNativeFieldDelegate>(1, 3);

        internal static object MarshalToManaged(IntPtr nativePtr, Type type)
        {
            static MarshalToManagedDelegate Factory(Type type)
            {
                Type marshalType = type;
                if (marshalType.IsByRef)
                    marshalType = marshalType.GetElementType();
                else if (marshalType.IsPointer)
                    marshalType = typeof(IntPtr);

                MethodInfo method = typeof(MarshalHelper<>).MakeGenericType(marshalType).GetMethod(nameof(MarshalHelper<ReferenceTypePlaceholder>.ToManagedWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                return method.CreateDelegate<MarshalToManagedDelegate>();
            }

            if (!toManagedMarshallers.TryGetValue(type, out var deleg))
                deleg = toManagedMarshallers.GetOrAdd(type, Factory);
            return deleg(nativePtr, type.IsByRef);
        }

        internal static void MarshalToNative(object managedObject, IntPtr nativePtr, Type type)
        {
            static MarshalToNativeDelegate Factory(Type type)
            {
                MethodInfo method;
                if (type.IsValueType)
                    method = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNativeWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    method = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativeWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                return method.CreateDelegate<MarshalToNativeDelegate>();
            }

            if (!toNativeMarshallers.TryGetValue(type, out var deleg))
                deleg = toNativeMarshallers.GetOrAdd(type, Factory);
            deleg(managedObject, nativePtr);
        }

        internal static MarshalToNativeFieldDelegate GetToNativeFieldMarshallerDelegate(Type type)
        {
            static MarshalToNativeFieldDelegate Factory(Type type)
            {
                MethodInfo method;
                if (type.IsValueType)
                    method = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNativeFieldWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    method = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativeFieldWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                return method.CreateDelegate<MarshalToNativeFieldDelegate>();
            }

            if (toNativeFieldMarshallers.TryGetValue(type, out var deleg))
                return deleg;
            return toNativeFieldMarshallers.GetOrAdd(type, Factory);
        }

        internal static void MarshalToNativeField(FieldInfo field, object fieldOwner, IntPtr nativePtr, out int fieldOffset)
        {
            GetToNativeFieldMarshallerDelegate(fieldOwner.GetType())(field, fieldOwner, nativePtr, out fieldOffset);
        }

        /// <summary>
        /// Helper class for managing stored marshaller delegates for each type.
        /// </summary>
        internal static class MarshalHelper<T>
        {
            private delegate void MarshalToNativeTypedDelegate(ref T managedValue, IntPtr nativePtr);
            private delegate void MarshalToManagedTypedDelegate(ref T managedValue, IntPtr nativePtr, bool byRef);
            internal delegate void MarshalFieldTypedDelegate(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset);

            internal static FieldInfo[] marshallableFields;
            internal static MarshalFieldTypedDelegate[] toManagedFieldMarshallers;
            internal static MarshalFieldTypedDelegate[] toNativeFieldMarshallers;

            private static MarshalToNativeTypedDelegate toNativeTypedMarshaller;
            private static MarshalToManagedTypedDelegate toManagedTypedMarshaller;

            static MarshalHelper()
            {
                Type type = typeof(T);

                // Setup marshallers for managed and native directions
                MethodInfo toManagedMethod;
                if (type.IsValueType)
                    toManagedMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManaged), BindingFlags.Static | BindingFlags.NonPublic);
                else if (type.IsArray && type.GetElementType().IsValueType)
                    toManagedMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type.GetElementType()).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManagedArray), BindingFlags.Static | BindingFlags.NonPublic);
                else if (type.IsArray && !type.GetElementType().IsValueType)
                    toManagedMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type.GetElementType()).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManagedArray), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    toManagedMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManaged), BindingFlags.Static | BindingFlags.NonPublic);
                toManagedTypedMarshaller = toManagedMethod.CreateDelegate<MarshalToManagedTypedDelegate>();

                MethodInfo toNativeMethod;
                if (type.IsValueType)
                    toNativeMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNative), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    toNativeMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNative), BindingFlags.Static | BindingFlags.NonPublic);
                toNativeTypedMarshaller = toNativeMethod.CreateDelegate<MarshalToNativeTypedDelegate>();

                if (!type.IsPrimitive && !type.IsPointer && type != typeof(bool))
                {
                    // Setup field-by-field marshallers for reference types or structures containing references
                    marshallableFields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
                    if (type.IsValueType && !marshallableFields.Any(x => (x.FieldType.IsClass && !x.FieldType.IsPointer) || x.FieldType.Name == "Boolean"))
                        marshallableFields = null;
                    else if (!type.IsValueType && !marshallableFields.Any())
                        marshallableFields = null;

                    if (marshallableFields != null)
                    {
                        toManagedFieldMarshallers = new MarshalFieldTypedDelegate[marshallableFields.Length];
                        toNativeFieldMarshallers = new MarshalFieldTypedDelegate[marshallableFields.Length];
                        for (int i = 0; i < marshallableFields.Length; i++)
                        {
                            FieldInfo field = marshallableFields[i];
                            MethodInfo toManagedFieldMethod;
                            MethodInfo toNativeFieldMethod;

                            if (field.FieldType.IsPointer)
                            {
                                toManagedFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToManagedFieldPointer), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                                toNativeFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToNativeFieldPointer), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                            }
                            else if (field.FieldType.IsValueType)
                            {
                                toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, field.FieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedField), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                                toNativeFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, field.FieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToNativeField), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                            }
                            else if (field.FieldType.IsArray)
                            {
                                Type arrayElementType = field.FieldType.GetElementType();
                                if (arrayElementType.IsValueType)
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, arrayElementType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedFieldArray), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, field.FieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeField), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                                }
                                else
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, field.FieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedField), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, field.FieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeField), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                                }
                            }
                            else
                            {
                                toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, field.FieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedField), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                                toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, field.FieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeField), BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
                            }
                            toManagedFieldMarshallers[i] = toManagedFieldMethod.CreateDelegate<MarshalFieldTypedDelegate>();
                            toNativeFieldMarshallers[i] = toNativeFieldMethod.CreateDelegate<MarshalFieldTypedDelegate>();
                        }
                    }
                }
            }

            internal static object ToManagedWrapper(IntPtr nativePtr, bool byRef)
            {
                T managed = default;
                toManagedTypedMarshaller(ref managed, nativePtr, byRef);
                return managed;
            }

            internal static void ToManaged(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                toManagedTypedMarshaller(ref managedValue, nativePtr, byRef);
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            internal static T ToManagedUnbox(IntPtr nativePtr)
            {
                T managed = default;
                if (nativePtr != IntPtr.Zero)
                {
                    Type type = typeof(T);
                    if (type.IsArray)
                        managed = (T)MarshalToManaged(nativePtr, type); // Array might be in internal format of custom structs so unbox if need to
                    else
                        managed = (T)ManagedHandle.FromIntPtr(nativePtr).Target;
                }
                return managed;
            }

            internal static Array ToManagedArray(Span<IntPtr> ptrSpan)
            {
                T[] arr = new T[ptrSpan.Length];
                for (int i = 0; i < arr.Length; i++)
                    toManagedTypedMarshaller(ref arr[i], ptrSpan[i], false);
                return arr;
            }

            internal static Array ToManagedArray(ManagedArray nativeArray)
            {
                T[] arr = new T[nativeArray.Length];
                IntPtr nativePtr = nativeArray.GetPointer;
                for (int i = 0; i < arr.Length; i++)
                {
                    toManagedTypedMarshaller(ref arr[i], nativePtr, false);
                    nativePtr += nativeArray.ElementSize;
                }
                return arr;
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                toNativeTypedMarshaller(ref managedValue, nativePtr);
            }

            internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr nativePtr, out int fieldOffset)
            {
                if (marshallableFields != null)
                {
                    for (int i = 0; i < marshallableFields.Length; i++)
                    {
                        if (marshallableFields[i] == field)
                        {
                            toNativeFieldMarshallers[i](marshallableFields[i], ref fieldOwner, nativePtr, out fieldOffset);
                            return;
                        }
                    }
                }

                throw new Exception($"Invalid field {field.Name} to marshal for type {typeof(T).Name}");
            }

            private static void ToManagedFieldPointer(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
            {
                ref IntPtr fieldValueRef = ref GetFieldReference<IntPtr>(field, ref fieldOwner);
                fieldValueRef = Unsafe.Read<IntPtr>(fieldPtr.ToPointer());
                fieldOffset = IntPtr.Size;
            }

            private static void ToNativeFieldPointer(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
            {
                ref IntPtr fieldValueRef = ref GetFieldReference<IntPtr>(field, ref fieldOwner);
                Unsafe.Write<IntPtr>(fieldPtr.ToPointer(), fieldValueRef);
                fieldOffset = IntPtr.Size;
            }

            /// <summary>
            /// Returns a reference to the value of the field.
            /// </summary>           
            private static ref TField GetFieldReference<TField>(FieldInfo field, ref T fieldOwner)
            {
                // Get the address of the field, source: https://stackoverflow.com/a/56512720
                if (typeof(T).IsValueType)
                {
                    byte* fieldPtr = (byte*)Unsafe.AsPointer(ref fieldOwner) + (Marshal.ReadInt32(field.FieldHandle.Value + 4 + IntPtr.Size) & 0xFFFFFF);
                    return ref Unsafe.AsRef<TField>(fieldPtr);
                }
                else
                {
                    byte* fieldPtr = (byte*)Unsafe.As<T, IntPtr>(ref fieldOwner) + IntPtr.Size + (Marshal.ReadInt32(field.FieldHandle.Value + 4 + IntPtr.Size) & 0xFFFFFF);
                    return ref Unsafe.AsRef<TField>(fieldPtr);
                }
            }

            private static IntPtr EnsureAlignment(IntPtr ptr, int alignment)
            {
                if (ptr % alignment != 0)
                    ptr = IntPtr.Add(ptr, alignment - (int)(ptr % alignment));
                return ptr;
            }

            private static class ValueTypeField<TField> where TField : struct
            {
                private static int fieldAlignment;

                static ValueTypeField()
                {
                    Type fieldType = typeof(TField);
                    if (fieldType.IsEnum)
                        fieldType = fieldType.GetEnumUnderlyingType();
                    else if (fieldType == typeof(bool))
                        fieldType = typeof(byte);

                    if (fieldType.IsValueType && !fieldType.IsEnum && !fieldType.IsPrimitive) // Is it a structure?
                    { }
                    else if (fieldType.IsClass || fieldType.IsPointer)
                        fieldAlignment = IntPtr.Size;
                    else
                        fieldAlignment = Marshal.SizeOf(fieldType);
                }

                internal static void ToManagedField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr fieldStartPtr = fieldPtr;
                        fieldPtr = EnsureAlignment(fieldPtr, fieldAlignment);
                        fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();
                    }

                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperValueType<TField>.ToManaged(ref fieldValueRef, fieldPtr, false);
                }

                internal static void ToManagedFieldArray(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    // Follows the same marshalling semantics with reference types
                    fieldOffset = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = fieldPtr;
                    fieldPtr = EnsureAlignment(fieldPtr, IntPtr.Size);
                    fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();

                    ref TField[] fieldValueRef = ref GetFieldReference<TField[]>(field, ref fieldOwner);
                    MarshalHelperValueType<TField>.ToManagedArray(ref fieldValueRef, Unsafe.Read<IntPtr>(fieldPtr.ToPointer()), false);
                }

                internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr startPtr = fieldPtr;
                        fieldPtr = EnsureAlignment(fieldPtr, fieldAlignment);
                        fieldOffset += (fieldPtr - startPtr).ToInt32();
                    }

                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperValueType<TField>.ToNative(ref fieldValueRef, fieldPtr);
                }
            }

            private static class ReferenceTypeField<TField> where TField : class
            {
                internal static void ToManagedField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = fieldPtr;
                    fieldPtr = EnsureAlignment(fieldPtr, IntPtr.Size);
                    fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();

                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperReferenceType<TField>.ToManaged(ref fieldValueRef, Unsafe.Read<IntPtr>(fieldPtr.ToPointer()), false);
                }

                internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = fieldPtr;
                    fieldPtr = EnsureAlignment(fieldPtr, IntPtr.Size);
                    fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();

                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperReferenceType<TField>.ToNative(ref fieldValueRef, fieldPtr);
                }
            }
        }

        internal static class MarshalHelperValueType<T> where T : struct
        {
            internal static void ToNativeWrapper(object managedObject, IntPtr nativePtr)
            {
                ToNative(ref Unsafe.Unbox<T>(managedObject), nativePtr);
            }

            internal static void ToNativeFieldWrapper(FieldInfo field, object fieldOwner, IntPtr nativePtr, out int fieldOffset)
            {
                MarshalHelper<T>.ToNativeField(field, ref Unsafe.Unbox<T>(fieldOwner), nativePtr, out fieldOffset);
            }

            internal static void ToManaged(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                Type type = typeof(T);
                if (type.IsByRef || byRef)
                {
                    if (type.IsByRef)
                        type = type.GetElementType();
                    Assert.IsTrue(type.IsValueType);
                }

                if (type == typeof(IntPtr))
                    managedValue = (T)(object)nativePtr;
                else if (type == typeof(ManagedHandle))
                    managedValue = (T)(object)ManagedHandle.FromIntPtr(nativePtr);
                else if (MarshalHelper<T>.marshallableFields != null)
                {
                    IntPtr fieldPtr = nativePtr;
                    for (int i = 0; i < MarshalHelper<T>.marshallableFields.Length; i++)
                    {
                        MarshalHelper<T>.toManagedFieldMarshallers[i](MarshalHelper<T>.marshallableFields[i], ref managedValue, fieldPtr, out int fieldOffset);
                        fieldPtr += fieldOffset;
                    }
                    Assert.IsTrue((fieldPtr - nativePtr) == Unsafe.SizeOf<T>());
                }
                else
                    managedValue = Unsafe.Read<T>(nativePtr.ToPointer());
            }

            internal static void ToManagedArray(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Marshal.ReadIntPtr(nativePtr);

                Type elementType = typeof(T);
                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(nativePtr).Target);
                    if (ArrayFactory.GetMarshalledType(elementType) == elementType)
                        managedValue = Unsafe.As<T[]>(managedArray.GetArray<T>());
                    else if (elementType.IsValueType)
                        managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray));
                    else
                        managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray.GetSpan<IntPtr>()));
                }
                else
                    managedValue = null;
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                if (typeof(T).IsByRef)
                    throw new NotImplementedException();

                if (MarshalHelper<T>.marshallableFields != null)
                {
                    IntPtr fieldPtr = nativePtr;
                    for (int i = 0; i < MarshalHelper<T>.marshallableFields.Length; i++)
                    {
                        MarshalHelper<T>.toNativeFieldMarshallers[i](MarshalHelper<T>.marshallableFields[i], ref managedValue, nativePtr, out int fieldOffset);
                        nativePtr += fieldOffset;
                    }
                    Assert.IsTrue((nativePtr - fieldPtr) == Unsafe.SizeOf<T>());
                }
                else
                    Unsafe.AsRef<T>(nativePtr.ToPointer()) = managedValue;
            }
        }

        internal static class MarshalHelperReferenceType<T> where T : class
        {
            internal static void ToNativeWrapper(object managedObject, IntPtr nativePtr)
            {
                T managedValue = Unsafe.As<T>(managedObject);
                ToNative(ref managedValue, nativePtr);
            }

            internal static void ToNativeFieldWrapper(FieldInfo field, object managedObject, IntPtr nativePtr, out int fieldOffset)
            {
                T managedValue = Unsafe.As<T>(managedObject);
                MarshalHelper<T>.ToNativeField(field, ref managedValue, nativePtr, out fieldOffset);
            }

            internal static void ToManaged(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                Type type = typeof(T);
                if (byRef)
                    nativePtr = Marshal.ReadIntPtr(nativePtr);

                if (type == typeof(string))
                    managedValue = Unsafe.As<T>(ManagedString.ToManaged(nativePtr));
                else if (type.IsClass)
                    managedValue = nativePtr != IntPtr.Zero ? Unsafe.As<T>(ManagedHandle.FromIntPtr(nativePtr).Target) : null;
                else if (type.IsInterface) // Dictionary
                    managedValue = nativePtr != IntPtr.Zero ? Unsafe.As<T>(ManagedHandle.FromIntPtr(nativePtr).Target) : null;
                else
                    throw new NotImplementedException();
            }

            internal static void ToManagedArray(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Marshal.ReadIntPtr(nativePtr);

                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(nativePtr).Target);
                    managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray.GetSpan<IntPtr>()));
                }
                else
                    managedValue = null;
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                Type type = typeof(T);

                IntPtr managedPtr;
                if (type == typeof(string))
                    managedPtr = ManagedString.ToNativeWeak(managedValue as string);
                else if (type.IsPointer)
                {
                    if (Pointer.Unbox(managedValue) == null)
                        managedPtr = IntPtr.Zero;
                    else if (managedValue is FlaxEngine.Object flaxObj)
                        managedPtr = FlaxEngine.Object.GetUnmanagedPtr(flaxObj);
                    else
                        managedPtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managedValue, GCHandleType.Weak));
                }
                else if (type == typeof(Type))
                    managedPtr = managedValue != null ? ManagedHandle.ToIntPtr(GetTypeGCHandle((Type)(object)managedValue)) : IntPtr.Zero;
                else if (type.IsArray)
                {
                    if (managedValue == null)
                        managedPtr = IntPtr.Zero;
                    else
                    {
                        var elementType = type.GetElementType();
                        var arr = Unsafe.As<Array>(managedValue);
                        var marshalledType = ArrayFactory.GetMarshalledType(elementType);
                        ManagedArray managedArray;
                        if (marshalledType == elementType)
                            managedArray = ManagedArray.WrapNewArray(arr);
                        else if (elementType.IsValueType)
                        {
                            // Convert array of custom structures into internal native layout
                            managedArray = ManagedArray.AllocateNewArray(arr.Length, Marshal.SizeOf(marshalledType));
                            IntPtr managedArrayPtr = managedArray.GetPointer;
                            for (int i = 0; i < arr.Length; i++)
                            {
                                MarshalToNative(arr.GetValue(i), managedArrayPtr, elementType);
                                managedArrayPtr += managedArray.ElementSize;
                            }
                        }
                        else
                            managedArray = ManagedArray.WrapNewArray(ManagedArrayToGCHandleArray(arr));
                        managedPtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managedArray, GCHandleType.Weak));
                    }
                }
                else
                    managedPtr = managedValue != null ? ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managedValue, GCHandleType.Weak)) : IntPtr.Zero;

                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), managedPtr);
            }
        }

        internal class MethodHolder
        {
            internal Type[] parameterTypes;
            internal MethodInfo method;
            private Invoker.MarshalAndInvokeDelegate invokeDelegate;
            private object delegInvoke;

            internal MethodHolder(MethodInfo method)
            {
                this.method = method;
                parameterTypes = method.GetParameterTypes();
            }

            internal bool TryGetDelegate(out Invoker.MarshalAndInvokeDelegate outDeleg, out object outDelegInvoke)
            {
                if (invokeDelegate == null)
                {
                    List<Type> methodTypes = new List<Type>();
                    if (!method.IsStatic)
                        methodTypes.Add(method.DeclaringType);
                    if (method.ReturnType != typeof(void))
                        methodTypes.Add(method.ReturnType);
                    methodTypes.AddRange(parameterTypes);

                    List<Type> genericParamTypes = new List<Type>();
                    foreach (var type in methodTypes)
                    {
                        if (type.IsByRef)
                            genericParamTypes.Add(type.GetElementType());
                        else if (type.IsPointer)
                            genericParamTypes.Add(typeof(IntPtr));
                        else
                            genericParamTypes.Add(type);
                    }

                    string invokerTypeName = $"FlaxEngine.NativeInterop+Invoker+Invoker{(method.IsStatic ? "Static" : "")}{(method.ReturnType != typeof(void) ? "Ret" : "NoRet")}{parameterTypes.Length}{(genericParamTypes.Count > 0 ? "`" + genericParamTypes.Count : "")}";
                    Type invokerType = Type.GetType(invokerTypeName);
                    if (invokerType != null)
                    {
                        if (genericParamTypes.Count != 0)
                            invokerType = invokerType.MakeGenericType(genericParamTypes.ToArray());
                        invokeDelegate = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.MarshalAndInvoke), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Invoker.MarshalAndInvokeDelegate>();
                        delegInvoke = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.CreateDelegate), BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { method });
                    }
                }

                outDeleg = invokeDelegate;
                outDelegInvoke = delegInvoke;

                return outDeleg != null;
            }
        }

        internal static ManagedHandle GetMethodGCHandle(MethodInfo method)
        {
            MethodHolder methodHolder = new MethodHolder(method);
            ManagedHandle handle = ManagedHandle.Alloc(methodHolder);
            if (methodHolder.parameterTypes.Any(x => x.IsCollectible) || method.IsCollectible)
                methodHandlesCollectible.Add(handle);
            else
                methodHandles.Add(handle);
            return handle;
        }

        internal static ManagedHandle GetAssemblyHandle(Assembly assembly)
        {
            if (!assemblyHandles.TryGetValue(assembly, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(assembly);
                assemblyHandles.Add(assembly, handle);
            }
            return handle;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetManagedClasses(ManagedHandle assemblyHandle, NativeClassDefinitions** managedClasses, int* managedClassCount)
        {
            Assembly assembly = Unsafe.As<Assembly>(assemblyHandle.Target);
            var assemblyTypes = GetAssemblyTypes(assembly);

            NativeClassDefinitions* arr = (NativeClassDefinitions*)NativeAlloc(assemblyTypes.Length, Unsafe.SizeOf<NativeClassDefinitions>());

            for (int i = 0; i < assemblyTypes.Length; i++)
            {
                var type = assemblyTypes[i];
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeClassDefinitions>() * i);
                bool isStatic = type.IsAbstract && type.IsSealed;
                bool isInterface = type.IsInterface;
                bool isAbstract = type.IsAbstract;

                ManagedHandle typeHandle = GetTypeGCHandle(type);

                NativeClassDefinitions managedClass = new NativeClassDefinitions()
                {
                    typeHandle = typeHandle,
                    name = NativeAllocStringAnsi(type.Name),
                    fullname = NativeAllocStringAnsi(type.FullName),
                    @namespace = NativeAllocStringAnsi(type.Namespace ?? ""),
                    typeAttributes = (uint)type.Attributes,
                };
                Unsafe.Write(ptr.ToPointer(), managedClass);
            }

            *managedClasses = arr;
            *managedClassCount = assemblyTypes.Length;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetManagedClassFromType(ManagedHandle typeHandle, NativeClassDefinitions* managedClass, ManagedHandle* assemblyHandle)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            ManagedHandle classTypeHandle = GetTypeGCHandle(type);
            *managedClass = new NativeClassDefinitions()
            {
                typeHandle = classTypeHandle,
                name = NativeAllocStringAnsi(type.Name),
                fullname = NativeAllocStringAnsi(type.FullName),
                @namespace = NativeAllocStringAnsi(type.Namespace ?? ""),
                typeAttributes = (uint)type.Attributes,
            };
            *assemblyHandle = GetAssemblyHandle(type.Assembly);
        }

        [UnmanagedCallersOnly]
        internal static void GetClassMethods(ManagedHandle typeHandle, NativeMethodDefinitions** classMethods, int* classMethodsCount)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);

            List<MethodInfo> methods = new List<MethodInfo>();
            var staticMethods = type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            var instanceMethods = type.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            foreach (MethodInfo method in staticMethods)
                methods.Add(method);
            foreach (MethodInfo method in instanceMethods)
                methods.Add(method);

            NativeMethodDefinitions* arr = (NativeMethodDefinitions*)NativeAlloc(methods.Count, Unsafe.SizeOf<NativeMethodDefinitions>());
            for (int i = 0; i < methods.Count; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeMethodDefinitions>() * i);
                NativeMethodDefinitions classMethod = new NativeMethodDefinitions()
                {
                    name = NativeAllocStringAnsi(methods[i].Name),
                    numParameters = methods[i].GetParameters().Length,
                    methodAttributes = (uint)methods[i].Attributes,
                };
                classMethod.typeHandle = GetMethodGCHandle(methods[i]);
                Unsafe.Write(ptr.ToPointer(), classMethod);
            }
            *classMethods = arr;
            *classMethodsCount = methods.Count;
        }

        internal class FieldHolder
        {
            internal FieldInfo field;
            internal MarshalToNativeFieldDelegate toNativeMarshaller;

            internal FieldHolder(FieldInfo field, Type type)
            {
                this.field = field;
                toNativeMarshaller = GetToNativeFieldMarshallerDelegate(type);
            }
        }

        [UnmanagedCallersOnly]
        internal static void GetClassFields(ManagedHandle typeHandle, NativeFieldDefinitions** classFields, int* classFieldsCount)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            NativeFieldDefinitions* arr = (NativeFieldDefinitions*)NativeAlloc(fields.Length, Unsafe.SizeOf<NativeFieldDefinitions>());
            for (int i = 0; i < fields.Length; i++)
            {
                FieldHolder fieldHolder = new FieldHolder(fields[i], type);

                ManagedHandle fieldHandle = ManagedHandle.Alloc(fieldHolder);
                if (type.IsCollectible)
                    fieldHandleCacheCollectible.Add(fieldHandle);
                else
                    fieldHandleCache.Add(fieldHandle);

                NativeFieldDefinitions classField = new NativeFieldDefinitions()
                {
                    name = NativeAllocStringAnsi(fieldHolder.field.Name),
                    fieldHandle = fieldHandle,
                    fieldTypeHandle = GetTypeGCHandle(fieldHolder.field.FieldType),
                    fieldAttributes = (uint)fieldHolder.field.Attributes,
                };
                Unsafe.Write(IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeFieldDefinitions>() * i).ToPointer(), classField);
            }
            *classFields = arr;
            *classFieldsCount = fields.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassProperties(ManagedHandle typeHandle, NativePropertyDefinitions** classProperties, int* classPropertiesCount)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            var properties = type.GetProperties(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            NativePropertyDefinitions* arr = (NativePropertyDefinitions*)NativeAlloc(properties.Length, Unsafe.SizeOf<NativePropertyDefinitions>());
            for (int i = 0; i < properties.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativePropertyDefinitions>() * i);

                var getterMethod = properties[i].GetGetMethod(true);
                var setterMethod = properties[i].GetSetMethod(true);

                NativePropertyDefinitions classProperty = new NativePropertyDefinitions()
                {
                    name = NativeAllocStringAnsi(properties[i].Name),
                };
                if (getterMethod != null)
                {
                    var getterHandle = GetMethodGCHandle(getterMethod);
                    classProperty.getterHandle = getterHandle;
                }
                if (setterMethod != null)
                {
                    var setterHandle = GetMethodGCHandle(setterMethod);
                    classProperty.setterHandle = setterHandle;
                }
                Unsafe.Write(ptr.ToPointer(), classProperty);
            }
            *classProperties = arr;
            *classPropertiesCount = properties.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassAttributes(ManagedHandle typeHandle, NativeAttributeDefinitions** classAttributes, int* classAttributesCount)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            object[] attributeValues = type.GetCustomAttributes(false);
            Type[] attributeTypes = type.GetCustomAttributes(false).Select(x => x.GetType()).ToArray();

            NativeAttributeDefinitions* arr = (NativeAttributeDefinitions*)NativeAlloc(attributeTypes.Length, Unsafe.SizeOf<NativeAttributeDefinitions>());
            for (int i = 0; i < attributeTypes.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeAttributeDefinitions>() * i);

                if (!classAttributesCacheCollectible.TryGetValue(attributeValues[i], out ManagedHandle attributeHandle))
                {
                    attributeHandle = ManagedHandle.Alloc(attributeValues[i]);
                    classAttributesCacheCollectible.Add(attributeValues[i], attributeHandle);
                }
                ManagedHandle attributeTypeHandle = GetTypeGCHandle(attributeTypes[i]);

                NativeAttributeDefinitions classAttribute = new NativeAttributeDefinitions()
                {
                    name = NativeAllocStringAnsi(attributeTypes[i].Name),
                    attributeTypeHandle = attributeTypeHandle,
                    attributeHandle = attributeHandle,
                };
                Unsafe.Write(ptr.ToPointer(), classAttribute);
            }
            *classAttributes = arr;
            *classAttributesCount = attributeTypes.Length;
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetCustomAttribute(ManagedHandle typeHandle, ManagedHandle attribHandle)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            Type attribType = Unsafe.As<Type>(attribHandle.Target);
            object attrib = type.GetCustomAttributes(false).FirstOrDefault(x => x.GetType() == attribType);
            if (attrib != null)
                return ManagedHandle.Alloc(attrib, GCHandleType.Weak);
            return new ManagedHandle();
        }

        [UnmanagedCallersOnly]
        internal static void GetClassInterfaces(ManagedHandle typeHandle, IntPtr* classInterfaces, int* classInterfacesCount)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            Type[] interfaces = type.GetInterfaces();

            // Match mono_class_get_interfaces which doesn't return interfaces from base class
            var baseTypeInterfaces = type.BaseType?.GetInterfaces();
            if (baseTypeInterfaces != null)
            {
                if (baseTypeInterfaces.Length == interfaces.Length)
                {
                    // Both base and this type have the same amount of interfaces so assume that this one has none unique
                    interfaces = Array.Empty<Type>();
                }
                else if (baseTypeInterfaces.Length != 0)
                {
                    // Count unique interface types
                    int uniqueCount = interfaces.Length;
                    for (int i = 0; i < interfaces.Length; i++)
                    {
                        for (int j = 0; j < baseTypeInterfaces.Length; j++)
                        {
                            if (interfaces[i] == baseTypeInterfaces[j])
                            {
                                uniqueCount--;
                                break;
                            }
                        }
                    }

                    // Get only unique interface types
                    var unique = new Type[uniqueCount];
                    uniqueCount = 0;
                    for (int i = 0; i < interfaces.Length; i++)
                    {
                        bool isUnique = true;
                        for (int j = 0; j < baseTypeInterfaces.Length; j++)
                        {
                            if (interfaces[i] == baseTypeInterfaces[j])
                            {
                                isUnique = false;
                                break;
                            }
                        }
                        if (isUnique)
                            unique[uniqueCount++] = interfaces[i];
                    }
                    interfaces = unique;
                }
            }

            IntPtr arr = (IntPtr)NativeAlloc(interfaces.Length, IntPtr.Size);
            for (int i = 0; i < interfaces.Length; i++)
            {
                ManagedHandle handle = GetTypeGCHandle(interfaces[i]);
                Unsafe.Write<ManagedHandle>(IntPtr.Add(arr, IntPtr.Size * i).ToPointer(), handle);
            }
            *classInterfaces = arr;
            *classInterfacesCount = interfaces.Length;
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetMethodReturnType(ManagedHandle methodHandle)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            Type returnType = methodHolder.method.ReturnType;

            return GetTypeGCHandle(returnType);
        }

        [UnmanagedCallersOnly]
        internal static void GetMethodParameterTypes(ManagedHandle methodHandle, IntPtr* typeHandles)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            Type returnType = methodHolder.method.ReturnType;
            IntPtr arr = (IntPtr)NativeAlloc(methodHolder.parameterTypes.Length, IntPtr.Size);
            for (int i = 0; i < methodHolder.parameterTypes.Length; i++)
            {
                ManagedHandle typeHandle = GetTypeGCHandle(methodHolder.parameterTypes[i]);
                Unsafe.Write<ManagedHandle>(IntPtr.Add(new IntPtr(arr), IntPtr.Size * i).ToPointer(), typeHandle);
            }
            *typeHandles = arr;
        }

        /// <summary>
        /// Returns pointer to the string's internal structure, containing the buffer and length of the string.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static IntPtr GetStringPointer(ManagedHandle stringHandle)
        {
            string str = Unsafe.As<string>(stringHandle.Target);
            IntPtr ptr = (Unsafe.Read<IntPtr>(Unsafe.AsPointer(ref str)) + sizeof(int) * 2);
            if (ptr < 0x1024)
                throw new Exception("null string ptr");
            return ptr;
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewObject(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            if (type.IsAbstract)
            {
                // Dotnet doesn't allow to instantiate abstract type thus allow to use generated mock class usage (eg. for Script or GPUResource) for generated abstract types
                var abstractWrapper = type.GetNestedType("AbstractWrapper", BindingFlags.NonPublic);
                if (abstractWrapper != null)
                    type = abstractWrapper;
            }
            object value = RuntimeHelpers.GetUninitializedObject(type);
            return ManagedHandle.Alloc(value);
        }

        internal static class ArrayFactory
        {
            private delegate Array CreateArrayDelegate(long size);

            private static ConcurrentDictionary<Type, Type> marshalledTypes = new ConcurrentDictionary<Type, Type>(1, 3);
            private static ConcurrentDictionary<Type, CreateArrayDelegate> createArrayDelegates = new ConcurrentDictionary<Type, CreateArrayDelegate>(1, 3);

            internal static Type GetMarshalledType(Type elementType)
            {
                static Type Factory(Type type)
                {
                    Type marshalType;
                    if (IsBlittable(type))
                        marshalType = type;
                    else
                        marshalType = GetInternalType(type) ?? typeof(IntPtr);
                    return marshalType;
                }

                if (marshalledTypes.TryGetValue(elementType, out var marshalledType))
                    return marshalledType;
                return marshalledTypes.GetOrAdd(elementType, Factory);
            }

            internal static Array CreateArray(Type type, long size)
            {
                static CreateArrayDelegate Factory(Type type)
                {
                    Type marshalledType = GetMarshalledType(type);
                    MethodInfo method = typeof(Internal<>).MakeGenericType(marshalledType).GetMethod(nameof(Internal<ValueTypePlaceholder>.CreateArrayDelegate), BindingFlags.Static | BindingFlags.NonPublic);
                    return method.CreateDelegate<CreateArrayDelegate>();
                }

                if (createArrayDelegates.TryGetValue(type, out var deleg))
                    return deleg(size);
                return createArrayDelegates.GetOrAdd(type, Factory)(size);
            }


            private static class Internal<T>
            {
                internal static Array CreateArrayDelegate(long size) => new T[size];
            }
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewArray(ManagedHandle typeHandle, long size)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            Type marshalledType = ArrayFactory.GetMarshalledType(type);
            if (marshalledType.IsValueType)
            {
                ManagedArray managedArray = ManagedArray.AllocateNewArray((int)size, Marshal.SizeOf(marshalledType));
                return ManagedHandle.Alloc(managedArray/*, GCHandleType.Weak*/);
            }
            else
            {
                Array arr = ArrayFactory.CreateArray(type, size);
                ManagedArray managedArray = ManagedArray.WrapNewArray(arr);
                return ManagedHandle.Alloc(managedArray/*, GCHandleType.Weak*/);
            }
        }

        [UnmanagedCallersOnly]
        internal static unsafe IntPtr GetArrayPointerToElement(ManagedHandle arrayHandle, int size, int index)
        {
            if (!arrayHandle.IsAllocated)
                return IntPtr.Zero;
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            if (managedArray.Length == 0)
                return IntPtr.Zero;
            Assert.IsTrue(index >= 0 && index < managedArray.Length);
            return IntPtr.Add(managedArray.GetPointer, size * index);
        }

        [UnmanagedCallersOnly]
        internal static int GetArrayLength(ManagedHandle arrayHandle)
        {
            if (!arrayHandle.IsAllocated)
                return 0;
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            return managedArray.Length;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetStringEmpty()
        {
            return ManagedHandle.ToIntPtr(ManagedString.EmptyStringHandle);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewStringUTF16(char* text, int length)
        {
            return ManagedString.ToNativeWeak(new string(new ReadOnlySpan<char>(text, length)));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewString(sbyte* text)
        {
            return ManagedString.ToNativeWeak(new string(text));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewStringLength(sbyte* text, int length)
        {
            return ManagedString.ToNativeWeak(new string(text, 0, length));
        }

        /// <summary>
        /// Creates a managed copy of the value, and stores it in a boxed reference.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static ManagedHandle BoxValue(ManagedHandle typeHandle, IntPtr valuePtr)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            object value = MarshalToManaged(valuePtr, type);
            return ManagedHandle.Alloc(value, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetObjectType(ManagedHandle handle)
        {
            var obj = handle.Target;
            Type classType = obj.GetType();
            return GetTypeGCHandle(classType);
        }

        internal static class ValueTypeUnboxer
        {
            private static ConcurrentDictionary<Type, UnboxerDelegate> unboxers = new ConcurrentDictionary<Type, UnboxerDelegate>(1, 3);
            private static MethodInfo unboxerMethod = typeof(ValueTypeUnboxer).GetMethod(nameof(ValueTypeUnboxer.UnboxPointer), BindingFlags.Static | BindingFlags.NonPublic);
            private delegate IntPtr UnboxerDelegate(object value);

            private static UnboxerDelegate UnboxerDelegateFactory(Type type)
            {
                return unboxerMethod.MakeGenericMethod(type).CreateDelegate<UnboxerDelegate>();
            }

            internal static IntPtr GetPointer(object value)
            {
                Type type = value.GetType();
                if (unboxers.TryGetValue(type, out var deleg))
                    return deleg(value);
                return unboxers.GetOrAdd(type, UnboxerDelegateFactory)(value);
            }

            private static unsafe IntPtr UnboxPointer<T>(object value) where T : struct
            {
                return new IntPtr(Unsafe.AsPointer(ref Unsafe.Unbox<T>(value)));
            }
        }

        /// <summary>
        /// Returns the address of the boxed value type.
        /// </summary>
        [UnmanagedCallersOnly]
        internal unsafe static IntPtr UnboxValue(ManagedHandle handle)
        {
            object value = handle.Target;
            Type type = value.GetType();
            if (!type.IsValueType)
                return ManagedHandle.ToIntPtr(handle);

            // HACK: Get the address of a non-pinned value
            return ValueTypeUnboxer.GetPointer(value);
        }

        [UnmanagedCallersOnly]
        internal static void RaiseException(ManagedHandle exceptionHandle)
        {
            Exception exception = Unsafe.As<Exception>(exceptionHandle.Target);
            throw exception;
        }

        [UnmanagedCallersOnly]
        internal static void ObjectInit(ManagedHandle objectHandle)
        {
            object obj = objectHandle.Target;
            Type type = obj.GetType();

            ConstructorInfo ctor = type.GetMember(".ctor", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance).First() as ConstructorInfo;
            ctor.Invoke(obj, null);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr InvokeMethod(ManagedHandle instanceHandle, ManagedHandle methodHandle, IntPtr paramPtr, IntPtr exceptionPtr)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            if (methodHolder.TryGetDelegate(out var methodDelegate, out var methodDelegateContext))
            {
                // Fast path, invoke the method with minimal allocations
                IntPtr returnValue;
                try
                {
                    returnValue = methodDelegate(methodDelegateContext, instanceHandle, paramPtr);
                }
                catch (Exception exception)
                {
                    if (exceptionPtr != IntPtr.Zero)
                        Marshal.WriteIntPtr(exceptionPtr, ManagedHandle.ToIntPtr(ManagedHandle.Alloc(exception, GCHandleType.Weak)));
                    return IntPtr.Zero;
                }
                return returnValue;
            }
            else
            {
                // Slow path, method parameters needs to be stored in heap
                object returnObject;
                int numParams = methodHolder.parameterTypes.Length;
                object[] methodParameters = new object[numParams];

                for (int i = 0; i < numParams; i++)
                {
                    IntPtr nativePtr = Marshal.ReadIntPtr(IntPtr.Add(paramPtr, sizeof(IntPtr) * i));
                    methodParameters[i] = MarshalToManaged(nativePtr, methodHolder.parameterTypes[i]);
                }

                try
                {
                    returnObject = methodHolder.method.Invoke(instanceHandle.IsAllocated ? instanceHandle.Target : null, methodParameters);
                }
                catch (Exception exception)
                {
                    // The internal exception thrown in MethodInfo.Invoke is caught here
                    Exception realException = exception;
                    if (exception.InnerException != null && exception.TargetSite.ReflectedType.Name == "MethodInvoker")
                        realException = exception.InnerException;

                    if (exceptionPtr != IntPtr.Zero)
                        Marshal.WriteIntPtr(exceptionPtr, ManagedHandle.ToIntPtr(ManagedHandle.Alloc(realException, GCHandleType.Weak)));
                    else
                        throw realException;
                    return IntPtr.Zero;
                }

                // Marshal reference parameters back to original unmanaged references
                for (int i = 0; i < numParams; i++)
                {
                    Type parameterType = methodHolder.parameterTypes[i];
                    if (parameterType.IsByRef)
                    {
                        IntPtr nativePtr = Marshal.ReadIntPtr(IntPtr.Add(paramPtr, sizeof(IntPtr) * i));
                        MarshalToNative(methodParameters[i], nativePtr, parameterType.GetElementType());
                    }
                }

                // Return value
                return Invoker.MarshalReturnValueGeneric(methodHolder.method.ReturnType, returnObject);
            }
        }

        private delegate IntPtr InvokeThunkDelegate(ManagedHandle instanceHandle, IntPtr param1, IntPtr param2, IntPtr param3, IntPtr param4, IntPtr param5, IntPtr param6, IntPtr param7);

        [UnmanagedCallersOnly]
        internal static IntPtr GetMethodUnmanagedFunctionPointer(ManagedHandle methodHandle)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);

            // Wrap the method call, this is needed to get the object instance from ManagedHandle and to pass the exception back to native side
            ThunkContext context = new ThunkContext(methodHolder.method);
            Delegate methodDelegate = typeof(ThunkContext).GetMethod(nameof(ThunkContext.InvokeThunk)).CreateDelegate<InvokeThunkDelegate>(context);
            IntPtr functionPtr = Marshal.GetFunctionPointerForDelegate(methodDelegate);

            // Keep a reference to the delegate to prevent it from being garbage collected
            if (methodHolder.method.IsCollectible)
                cachedDelegatesCollectible[functionPtr] = methodDelegate;
            else
                cachedDelegates[functionPtr] = methodDelegate;

            return functionPtr;
        }

        [UnmanagedCallersOnly]
        internal static void FieldSetValue(ManagedHandle fieldOwnerHandle, ManagedHandle fieldHandle, IntPtr valuePtr)
        {
            object fieldOwner = fieldOwnerHandle.Target;
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            field.field.SetValue(fieldOwner, Marshal.PtrToStructure(valuePtr, field.field.FieldType));
        }

        [UnmanagedCallersOnly]
        internal static void FieldGetValue(ManagedHandle fieldOwnerHandle, ManagedHandle fieldHandle, IntPtr valuePtr)
        {
            object fieldOwner = fieldOwnerHandle.Target;
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            field.toNativeMarshaller(field.field, fieldOwner, valuePtr, out int fieldOffset);
        }

        [UnmanagedCallersOnly]
        internal static void SetArrayValueReference(ManagedHandle arrayHandle, IntPtr elementPtr, IntPtr valueHandle)
        {
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            int index = (int)(elementPtr.ToInt64() - managedArray.GetPointer.ToInt64()) / managedArray.ElementSize;
            managedArray.GetSpan<IntPtr>()[index] = valueHandle;
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle LoadAssemblyFromPath(IntPtr assemblyPath_, IntPtr* assemblyName, IntPtr* assemblyFullName)
        {
            if (!firstAssemblyLoaded)
            {
                // This assembly was already loaded when initializing the host context
                firstAssemblyLoaded = true;

                Assembly flaxEngineAssembly = AssemblyLoadContext.Default.Assemblies.First(x => x.GetName().Name == "FlaxEngine.CSharp");
                *assemblyName = NativeAllocStringAnsi(flaxEngineAssembly.GetName().Name);
                *assemblyFullName = NativeAllocStringAnsi(flaxEngineAssembly.FullName);
                return GetAssemblyHandle(flaxEngineAssembly);
            }
            string assemblyPath = Marshal.PtrToStringAnsi(assemblyPath_);

            Assembly assembly = scriptingAssemblyLoadContext.LoadFromAssemblyPath(assemblyPath);
            NativeLibrary.SetDllImportResolver(assembly, NativeLibraryImportResolver);

            *assemblyName = NativeAllocStringAnsi(assembly.GetName().Name);
            *assemblyFullName = NativeAllocStringAnsi(assembly.FullName);
            return GetAssemblyHandle(assembly);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle LoadAssemblyImage(IntPtr data, int len, IntPtr assemblyPath_, IntPtr* assemblyName, IntPtr* assemblyFullName)
        {
            if (!firstAssemblyLoaded)
            {
                // This assembly was already loaded when initializing the host context
                firstAssemblyLoaded = true;

                Assembly flaxEngineAssembly = AssemblyLoadContext.Default.Assemblies.First(x => x.GetName().Name == "FlaxEngine.CSharp");
                *assemblyName = NativeAllocStringAnsi(flaxEngineAssembly.GetName().Name);
                *assemblyFullName = NativeAllocStringAnsi(flaxEngineAssembly.FullName);
                return GetAssemblyHandle(flaxEngineAssembly);
            }

            string assemblyPath = Marshal.PtrToStringAnsi(assemblyPath_);

            byte[] raw = new byte[len];
            Marshal.Copy(data, raw, 0, len);

            using MemoryStream stream = new MemoryStream(raw);
            Assembly assembly;
#if !BUILD_RELEASE
            var pdbPath = Path.ChangeExtension(assemblyPath, "pdb");
            if (File.Exists(pdbPath))
            {
                using FileStream pdbStream = new FileStream(Path.ChangeExtension(assemblyPath, "pdb"), FileMode.Open);
                assembly = scriptingAssemblyLoadContext.LoadFromStream(stream, pdbStream);
            }
            else
#endif
            {
                assembly = scriptingAssemblyLoadContext.LoadFromStream(stream);
            }
            if (assembly == null)
                return new ManagedHandle();
            NativeLibrary.SetDllImportResolver(assembly, NativeLibraryImportResolver);

            // Assemblies loaded via streams have no Location: https://github.com/dotnet/runtime/issues/12822
            AssemblyLocations.Add(assembly.FullName, assemblyPath);

            *assemblyName = NativeAllocStringAnsi(assembly.GetName().Name);
            *assemblyFullName = NativeAllocStringAnsi(assembly.FullName);
            return GetAssemblyHandle(assembly);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetAssemblyByName(IntPtr name_, IntPtr* assemblyName, IntPtr* assemblyFullName)
        {
            string name = Marshal.PtrToStringAnsi(name_);
            Assembly assembly = Utils.GetAssemblies().FirstOrDefault(x => x.GetName().Name == name);
            if (assembly == null)
                return new ManagedHandle();

            *assemblyName = NativeAllocStringAnsi(assembly.GetName().Name);
            *assemblyFullName = NativeAllocStringAnsi(assembly.FullName);
            return GetAssemblyHandle(assembly);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetRuntimeInformation()
        {
            return NativeAllocStringAnsi(System.Runtime.InteropServices.RuntimeInformation.FrameworkDescription);
        }

        [UnmanagedCallersOnly]
        internal static void CloseAssembly(ManagedHandle assemblyHandle)
        {
            Assembly assembly = Unsafe.As<Assembly>(assemblyHandle.Target);
            assemblyHandle.Free();
            assemblyHandles.Remove(assembly);

            AssemblyLocations.Remove(assembly.FullName);

            // Clear all caches which might hold references to closing assembly
            cachedDelegatesCollectible.Clear();
            typeCache.Clear();

            // Release all GCHandles in collectible ALC
            foreach (var pair in typeHandleCacheCollectible)
                pair.Value.Free();
            typeHandleCacheCollectible.Clear();

            foreach (var handle in methodHandlesCollectible)
                handle.Free();
            methodHandlesCollectible.Clear();

            foreach (var handle in fieldHandleCacheCollectible)
                handle.Free();
            fieldHandleCacheCollectible.Clear();

            foreach (var pair in classAttributesCacheCollectible)
                pair.Value.Free();
            classAttributesCacheCollectible.Clear();

            // Unload native library handles associated for this assembly
            string nativeLibraryName = assemblyOwnedNativeLibraries.GetValueOrDefault(assembly);
            if (nativeLibraryName != null && loadedNativeLibraries.TryGetValue(nativeLibraryName, out IntPtr nativeLibrary))
            {
                NativeLibrary.Free(nativeLibrary);
                loadedNativeLibraries.Remove(nativeLibraryName);
            }
            if (nativeLibraryName != null)
                nativeLibraryPaths.Remove(nativeLibraryName);

            // Unload the ALC
            bool unloading = true;
            scriptingAssemblyLoadContext.Unloading += (alc) => { unloading = false; };
            scriptingAssemblyLoadContext.Unload();

            while (unloading)
                System.Threading.Thread.Sleep(1);

            // TODO: benchmark collectible setting performance, maybe enable it only in editor builds?
            scriptingAssemblyLoadContext = new AssemblyLoadContext("Flax", true);
            DelegateHelpers.InitMethods();
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetAssemblyObject(IntPtr assemblyName_)
        {
            string assemblyName = Marshal.PtrToStringAnsi(assemblyName_);
            Assembly assembly = Utils.GetAssemblies().FirstOrDefault(x => x.FullName == assemblyName);
            if (assembly == null)
                return new ManagedHandle();
            return ManagedHandle.Alloc(assembly);
        }

        [UnmanagedCallersOnly]
        internal static unsafe int NativeSizeOf(ManagedHandle typeHandle, uint* align)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            Type nativeType = GetInternalType(type) ?? type;
            if (nativeType == typeof(Version))
                nativeType = typeof(VersionNative);

            int size = Marshal.SizeOf(nativeType);
            *align = (uint)size; // Is this correct?
            return size;
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsSubclassOf(ManagedHandle typeHandle, ManagedHandle othertypeHandle, byte checkInterfaces)
        {
            if (typeHandle == othertypeHandle)
                return 1;

            Type type = Unsafe.As<Type>(typeHandle.Target);
            Type otherType = Unsafe.As<Type>(othertypeHandle.Target);

            if (type == otherType)
                return 1;

            if (checkInterfaces != 0 && otherType.IsInterface)
            {
                if (type.GetInterfaces().Contains(otherType))
                    return 1;
            }

            return type.IsSubclassOf(otherType) ? (byte)1 : (byte)0;
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsValueType(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            return (byte)(type.IsValueType ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsEnum(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            return (byte)(type.IsEnum ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetClassParent(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);
            return GetTypeGCHandle(type.BaseType);
        }

        [UnmanagedCallersOnly]
        internal static byte GetMethodParameterIsOut(ManagedHandle methodHandle, int parameterNum)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            ParameterInfo parameterInfo = methodHolder.method.GetParameters()[parameterNum];
            return (byte)(parameterInfo.IsOut ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetNullReferenceException()
        {
            var exception = new NullReferenceException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetNotSupportedException()
        {
            var exception = new NotSupportedException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArgumentNullException()
        {
            var exception = new ArgumentNullException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArgumentException()
        {
            var exception = new ArgumentException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArgumentOutOfRangeException()
        {
            var exception = new ArgumentOutOfRangeException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewGCHandle(ManagedHandle valueHandle, byte pinned)
        {
            object value = valueHandle.Target;
            ManagedHandle handle = ManagedHandle.Alloc(value, pinned != 0 ? GCHandleType.Pinned : GCHandleType.Normal);
            return handle;
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewGCHandleWeakref(ManagedHandle valueHandle, byte track_resurrection)
        {
            object value = valueHandle.Target;
            ManagedHandle handle = ManagedHandle.Alloc(value, track_resurrection != 0 ? GCHandleType.WeakTrackResurrection : GCHandleType.Weak);
            return handle;
        }

        [UnmanagedCallersOnly]
        internal static void FreeGCHandle(ManagedHandle valueHandle)
        {
            valueHandle.Free();
        }

        internal enum MonoType
        {
            MONO_TYPE_END = 0x00,
            MONO_TYPE_VOID = 0x01,
            MONO_TYPE_BOOLEAN = 0x02,
            MONO_TYPE_CHAR = 0x03,
            MONO_TYPE_I1 = 0x04,
            MONO_TYPE_U1 = 0x05,
            MONO_TYPE_I2 = 0x06,
            MONO_TYPE_U2 = 0x07,
            MONO_TYPE_I4 = 0x08,
            MONO_TYPE_U4 = 0x09,
            MONO_TYPE_I8 = 0x0a,
            MONO_TYPE_U8 = 0x0b,
            MONO_TYPE_R4 = 0x0c,
            MONO_TYPE_R8 = 0x0d,
            MONO_TYPE_STRING = 0x0e,
            MONO_TYPE_PTR = 0x0f,
            MONO_TYPE_BYREF = 0x10,
            MONO_TYPE_VALUETYPE = 0x11,
            MONO_TYPE_CLASS = 0x12,
            MONO_TYPE_VAR = 0x13,
            MONO_TYPE_ARRAY = 0x14,
            MONO_TYPE_GENERICINST = 0x15,
            MONO_TYPE_TYPEDBYREF = 0x16,
            MONO_TYPE_I = 0x18,
            MONO_TYPE_U = 0x19,
            MONO_TYPE_FNPTR = 0x1b,
            MONO_TYPE_OBJECT = 0x1c,
            MONO_TYPE_SZARRAY = 0x1d,
            MONO_TYPE_MVAR = 0x1e,
            MONO_TYPE_CMOD_REQD = 0x1f,
            MONO_TYPE_CMOD_OPT = 0x20,
            MONO_TYPE_INTERNAL = 0x21,
            MONO_TYPE_MODIFIER = 0x40,
            MONO_TYPE_SENTINEL = 0x41,
            MONO_TYPE_PINNED = 0x45,
            MONO_TYPE_ENUM = 0x55,
        };

        [UnmanagedCallersOnly]
        internal static int GetTypeMonoTypeEnum(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<Type>(typeHandle.Target);

            MonoType monoType;
            switch (type)
            {
                case Type _ when type == typeof(bool):
                    monoType = MonoType.MONO_TYPE_BOOLEAN;
                    break;
                case Type _ when type == typeof(sbyte):
                case Type _ when type == typeof(short):
                    monoType = MonoType.MONO_TYPE_I2;
                    break;
                case Type _ when type == typeof(byte):
                case Type _ when type == typeof(ushort):
                    monoType = MonoType.MONO_TYPE_U2;
                    break;
                case Type _ when type == typeof(int):
                    monoType = MonoType.MONO_TYPE_I4;
                    break;
                case Type _ when type == typeof(uint):
                    monoType = MonoType.MONO_TYPE_U4;
                    break;
                case Type _ when type == typeof(long):
                    monoType = MonoType.MONO_TYPE_I8;
                    break;
                case Type _ when type == typeof(ulong):
                    monoType = MonoType.MONO_TYPE_U8;
                    break;
                case Type _ when type == typeof(float):
                    monoType = MonoType.MONO_TYPE_R4;
                    break;
                case Type _ when type == typeof(double):
                    monoType = MonoType.MONO_TYPE_R8;
                    break;
                case Type _ when type == typeof(string):
                    monoType = MonoType.MONO_TYPE_STRING;
                    break;
                case Type _ when type == typeof(IntPtr):
                    monoType = MonoType.MONO_TYPE_PTR;
                    break;
                case Type _ when type.IsEnum:
                    {
                        var elementType = type.GetEnumUnderlyingType();
                        if (elementType == typeof(sbyte) || elementType == typeof(short))
                            monoType = MonoType.MONO_TYPE_I2;
                        else if (elementType == typeof(byte) || elementType == typeof(ushort))
                            monoType = MonoType.MONO_TYPE_U2;
                        else if (elementType == typeof(int))
                            monoType = MonoType.MONO_TYPE_I4;
                        else if (elementType == typeof(uint))
                            monoType = MonoType.MONO_TYPE_U4;
                        else if (elementType == typeof(long))
                            monoType = MonoType.MONO_TYPE_I8;
                        else if (elementType == typeof(ulong))
                            monoType = MonoType.MONO_TYPE_U8;
                        else
                            throw new Exception($"GetTypeMonoTypeEnum: Unsupported type '{type.FullName}'");
                        break;
                    }
                case Type _ when type.IsValueType && !type.IsEnum && !type.IsPrimitive:
                    monoType = MonoType.MONO_TYPE_VALUETYPE;
                    break;
                case Type _ when type.IsClass:
                    monoType = MonoType.MONO_TYPE_OBJECT;
                    break;
                case Type _ when type.IsGenericType:
                    monoType = MonoType.MONO_TYPE_GENERICINST;
                    break;

                default: throw new Exception($"GetTypeMonoTypeEnum: Unsupported type '{type.FullName}'");
            }

            return (int)monoType;
        }

        /// <summary>
        /// Returns all types that that owned by this assembly.
        /// </summary>
        private static Type[] GetAssemblyTypes(Assembly assembly)
        {
            var referencedAssemblies = assembly.GetReferencedAssemblies();
            var allAssemblies = AppDomain.CurrentDomain.GetAssemblies();
            var referencedTypes = new List<string>();
            foreach (var assemblyName in referencedAssemblies)
            {
                var asm = allAssemblies.FirstOrDefault(x => x.GetName().Name == assemblyName.Name);
                if (asm == null)
                    continue;
                referencedTypes.AddRange(asm.DefinedTypes.Select(x => x.FullName).ToArray());
            }

            // TODO: use MetadataReader to read types without loading any of the referenced assemblies?
            // https://makolyte.com/csharp-get-a-list-of-types-defined-in-an-assembly-without-loading-it/

            // We need private types of this assembly too, DefinedTypes contains a lot of types from other assemblies...
            var types = referencedTypes.Any() ? assembly.DefinedTypes.Where(x => !referencedTypes.Contains(x.FullName)).ToArray() : assembly.DefinedTypes.ToArray();

            Assert.IsTrue(Utils.GetAssemblies().Where(x => x.GetName().Name == "FlaxEngine.CSharp").Count() == 1);
            return types;
        }

        /// <summary>
        /// Returns a static ManagedHandle for given Type, and caches it if needed.
        /// </summary>
        internal static ManagedHandle GetTypeGCHandle(Type type)
        {
            if (typeHandleCache.TryGetValue(type, out ManagedHandle handle))
                return handle;
            if (typeHandleCacheCollectible.TryGetValue(type, out handle))
                return handle;

            handle = ManagedHandle.Alloc(type);
            if (type.IsCollectible) // check if generic parameters are also collectible?
                typeHandleCacheCollectible.Add(type, handle);
            else
                typeHandleCache.Add(type, handle);

            return handle;
        }

        private static class DelegateHelpers
        {
            private static Func<Type[], Type> MakeNewCustomDelegateFunc;
            private static Func<Type[], Type> MakeNewCustomDelegateFuncCollectible;

            internal static void InitMethods()
            {
                MakeNewCustomDelegateFunc =
                    typeof(Expression).Assembly.GetType("System.Linq.Expressions.Compiler.DelegateHelpers")
                    .GetMethod("MakeNewCustomDelegate", BindingFlags.NonPublic | BindingFlags.Static).CreateDelegate<Func<Type[], Type>>();

                // Load System.Linq.Expressions assembly to collectible ALC.
                // The dynamic assembly where delegates are stored is cached in the DelegateHelpers class, so we should
                // use the DelegateHelpers in collectible ALC to make sure the delegates are also stored in the same ALC.
                Assembly assembly = scriptingAssemblyLoadContext.LoadFromAssemblyPath(typeof(Expression).Assembly.Location);
                MakeNewCustomDelegateFuncCollectible =
                    assembly.GetType("System.Linq.Expressions.Compiler.DelegateHelpers")
                    .GetMethod("MakeNewCustomDelegate", BindingFlags.NonPublic | BindingFlags.Static).CreateDelegate<Func<Type[], Type>>();

                // Create dummy delegates to force the dynamic Snippets assembly to be loaded in correcet ALCs
                MakeNewCustomDelegateFunc(new[] { typeof(void) });
                {
                    // Ensure the new delegate is placed in the collectible ALC
                    using var ctx = scriptingAssemblyLoadContext.EnterContextualReflection();
                    MakeNewCustomDelegateFuncCollectible(new[] { typeof(void) });
                }
            }

            internal static Type MakeNewCustomDelegate(Type[] parameters)
            {
                if (parameters.Any(x => x.IsCollectible))
                    return MakeNewCustomDelegateFuncCollectible(parameters);
                return MakeNewCustomDelegateFunc(parameters);
            }
        }

        /// <summary>
        /// Wrapper class for invoking function pointers from unmanaged code.
        /// </summary>
        internal class ThunkContext
        {
            internal MethodInfo method;
            internal Type[] parameterTypes;
            internal Invoker.InvokeThunkDelegate methodDelegate;
            internal object methodDelegateContext;

            internal ThunkContext(MethodInfo method)
            {
                this.method = method;
                parameterTypes = method.GetParameterTypes();

                // Thunk delegates don't support IsByRef parameters (use egenric invocation for now)
                foreach (var type in parameterTypes)
                {
                    if (type.IsByRef)
                        return;
                }

                List<Type> methodTypes = new List<Type>();
                if (!method.IsStatic)
                    methodTypes.Add(method.DeclaringType);
                if (method.ReturnType != typeof(void))
                    methodTypes.Add(method.ReturnType);
                methodTypes.AddRange(parameterTypes);

                List<Type> genericParamTypes = new List<Type>();
                foreach (var type in methodTypes)
                {
                    if (type.IsByRef)
                        genericParamTypes.Add(type.GetElementType());
                    else if (type.IsPointer)
                        genericParamTypes.Add(typeof(IntPtr));
                    else
                        genericParamTypes.Add(type);
                }

                string invokerTypeName = $"FlaxEngine.NativeInterop+Invoker+Invoker{(method.IsStatic ? "Static" : "")}{(method.ReturnType != typeof(void) ? "Ret" : "NoRet")}{parameterTypes.Length}{(genericParamTypes.Count > 0 ? "`" + genericParamTypes.Count : "")}";
                Type invokerType = Type.GetType(invokerTypeName);
                if (invokerType != null)
                {
                    if (genericParamTypes.Count != 0)
                        invokerType = invokerType.MakeGenericType(genericParamTypes.ToArray());
                    methodDelegate = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.InvokeThunk), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Invoker.InvokeThunkDelegate>();
                    methodDelegateContext = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.CreateInvokerDelegate), BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { method });
                }

                if (methodDelegate != null)
                    Assert.IsTrue(methodDelegateContext != null);
            }

            public IntPtr InvokeThunk(ManagedHandle instanceHandle, IntPtr param1, IntPtr param2, IntPtr param3, IntPtr param4, IntPtr param5, IntPtr param6, IntPtr param7)
            {
                IntPtr* nativePtrs = stackalloc IntPtr[] { param1, param2, param3, param4, param5, param6, param7 };
                if (methodDelegate != null)
                {
                    IntPtr returnValue;
                    try
                    {
                        returnValue = methodDelegate(methodDelegateContext, instanceHandle, nativePtrs);
                    }
                    catch (Exception exception)
                    {
                        // Returned exception is the last parameter
                        IntPtr exceptionPtr = nativePtrs[parameterTypes.Length];
                        if (exceptionPtr != IntPtr.Zero)
                            Marshal.WriteIntPtr(exceptionPtr, ManagedHandle.ToIntPtr(ManagedHandle.Alloc(exception, GCHandleType.Weak)));
                        return IntPtr.Zero;
                    }
                    return returnValue;
                }
                else
                {
                    // The parameters are wrapped in GCHandles
                    object returnObject;
                    int numParams = parameterTypes.Length;
                    object[] methodParameters = new object[numParams];
                    for (int i = 0; i < numParams; i++)
                    {
                        IntPtr nativePtr = nativePtrs[i];
                        object managed = null;
                        if (nativePtr != IntPtr.Zero)
                        {
                            Type type = parameterTypes[i];
                            Type elementType = type.GetElementType();
                            if (type.IsByRef && !elementType.IsValueType)
                            {
                                nativePtr = Marshal.ReadIntPtr(nativePtr);
                                type = elementType;
                            }
                            if (type.IsArray)
                                managed = MarshalToManaged(nativePtr, type); // Array might be in internal format of custom structs so unbox if need to
                            else
                                managed = ManagedHandle.FromIntPtr(nativePtr).Target;
                        }
                        methodParameters[i] = managed;
                    }

                    try
                    {
                        returnObject = method.Invoke(instanceHandle.IsAllocated ? instanceHandle.Target : null, methodParameters);
                    }
                    catch (Exception exception)
                    {
                        // Returned exception is the last parameter
                        IntPtr exceptionPtr = nativePtrs[numParams];
                        if (exceptionPtr != IntPtr.Zero)
                            Marshal.WriteIntPtr(exceptionPtr, ManagedHandle.ToIntPtr(ManagedHandle.Alloc(exception, GCHandleType.Weak)));
                        return IntPtr.Zero;
                    }

                    // Marshal reference parameters back to original unmanaged references
                    for (int i = 0; i < numParams; i++)
                    {
                        IntPtr nativePtr = nativePtrs[i];
                        Type type = parameterTypes[i];
                        Type elementType = type.GetElementType();
                        if (nativePtr != IntPtr.Zero && type.IsByRef)
                        {
                            if (elementType.IsValueType)
                            {
                                // Return directly to the original value
                                var ęxistingValue = ManagedHandle.FromIntPtr(nativePtr).Target;
                                nativePtr = ValueTypeUnboxer.GetPointer(ęxistingValue);
                            }
                            MarshalToNative(methodParameters[i], nativePtr, elementType);
                        }
                    }

                    // Return value
                    return Invoker.MarshalReturnValueThunkGeneric(method.ReturnType, returnObject);
                }
            }
        }
    }
}

#endif

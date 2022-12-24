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
using FlaxEngine.Visject;
using System.Diagnostics;
using System.Collections;
using System.Buffers;
using System.Collections.Concurrent;

#pragma warning disable 1591

namespace FlaxEngine
{
    #region Native structures

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeClassDefinitions
    {
        internal IntPtr typeHandle;
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
        internal IntPtr typeHandle;
        internal uint methodAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeFieldDefinitions
    {
        internal IntPtr name;
        internal IntPtr fieldHandle;
        internal IntPtr fieldTypeHandle;
        internal uint fieldAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativePropertyDefinitions
    {
        internal IntPtr name;
        internal IntPtr getterHandle;
        internal IntPtr setterHandle;
        internal uint getterFlags;
        internal uint setterFlags;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeAttributeDefinitions
    {
        internal IntPtr name;
        internal IntPtr attributeHandle;
        internal IntPtr attributeTypeHandle;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct VariantNative
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
        private GCHandle pinnedArrayHandle;
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

        internal static ManagedArray AllocateNewArray<T>(T* ptr, int length) where T : unmanaged => new ManagedArray(ptr, length, Unsafe.SizeOf<T>());

        internal static ManagedArray AllocateNewArray(IntPtr ptr, int length, int elementSize) => new ManagedArray(ptr.ToPointer(), length, elementSize);

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

        internal ManagedArray(Array arr) => WrapArray(arr);

        internal void WrapArray(Array arr)
        {
            pinnedArrayHandle = GCHandle.Alloc(arr, GCHandleType.Pinned);
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

        private ManagedArray(void* ptr, int length, int elementSize) => Allocate(new IntPtr(ptr), length, elementSize);

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
                Marshal.FreeHGlobal(unmanagedData);
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
            private static List<ValueTuple<bool, ManagedArray>> pool = new List<ValueTuple<bool, ManagedArray>>();

            internal static ManagedArray Get()
            {
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
        internal static GCHandle EmptyStringHandle = GCHandle.Alloc(string.Empty);

        [System.Diagnostics.DebuggerStepThrough]
        internal static unsafe IntPtr ToNative(string str)
        {
            if (str == null)
                return IntPtr.Zero;
            else if (str == string.Empty)
                return GCHandle.ToIntPtr(EmptyStringHandle);

            Assert.IsTrue(str.Length > 0);

            return GCHandle.ToIntPtr(GCHandle.Alloc(str, GCHandleType.Weak));
        }

        [System.Diagnostics.DebuggerStepThrough]
        internal static string ToManaged(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return null;

            return Unsafe.As<string>(GCHandle.FromIntPtr(ptr).Target);
        }

        [System.Diagnostics.DebuggerStepThrough]
        internal static void Free(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return;

            GCHandle handle = GCHandle.FromIntPtr(ptr);
            if (handle == EmptyStringHandle)
                return;

            handle.Free();
        }
    }

#endregion

    #region Marshallers

    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedIn, typeof(GCHandleMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedOut, typeof(GCHandleMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementIn, typeof(GCHandleMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedOut, typeof(GCHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedIn, typeof(GCHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementOut, typeof(GCHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedRef, typeof(GCHandleMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedRef, typeof(GCHandleMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementRef, typeof(GCHandleMarshaller))]
    public static class GCHandleMarshaller
    {
        public static class NativeToManaged
        {
            public static object ConvertToManaged(IntPtr unmanaged) => unmanaged == IntPtr.Zero ? null : GCHandle.FromIntPtr(unmanaged).Target;
            public static void Free(IntPtr unmanaged)
            {
                // This is a permanent handle, do not release it
            }
        }
        public static class ManagedToNative
        {
            public static IntPtr ConvertToUnmanaged(object managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;
            public static void Free(IntPtr unmanaged)
            {
                if (unmanaged == IntPtr.Zero)
                    return;
                GCHandle.FromIntPtr(unmanaged).Free();
            }
        }
        public struct Bidirectional
        {
            object managed;
            IntPtr unmanaged;
            public void FromManaged(object managed) { this.managed = managed; }
            public IntPtr ToUnmanaged() { unmanaged = GCHandleMarshaller.ToNative(managed); return unmanaged; }
            public void FromUnmanaged(IntPtr unmanaged) { this.unmanaged = unmanaged; }
            public object ToManaged() { managed = GCHandleMarshaller.ToManaged(unmanaged); unmanaged = IntPtr.Zero; return managed; }
            public void Free()
            {
                // FIXME, might be a permanent handle or a temporary one
                throw new NotImplementedException();
                /*if (unmanaged == IntPtr.Zero)
                    return;
                GCHandle.FromIntPtr(unmanaged).Free();*/
            }
        }
        public static object ConvertToManaged(IntPtr unmanaged) => ToManaged(unmanaged);
        public static IntPtr ConvertToUnmanaged(object managed) => ToNative(managed);
        public static object ToManaged(IntPtr managed) => managed != IntPtr.Zero ? GCHandle.FromIntPtr(managed).Target : null;
        public static IntPtr ToNative(object managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;
        public static void Free(IntPtr unmanaged)
        {
            if (unmanaged == IntPtr.Zero)
                return;
            GCHandle.FromIntPtr(unmanaged).Free();
        }
    }

    [CustomMarshaller(typeof(Type), MarshalMode.Default, typeof(SystemTypeMarshaller))]
    public static class SystemTypeMarshaller
    {
        internal static Type ConvertToManaged(IntPtr unmanaged) => Unsafe.As<Type>(GCHandleMarshaller.ConvertToManaged(unmanaged));

        internal static IntPtr ConvertToUnmanaged(Type managed)
        {
            if (managed == null)
                return IntPtr.Zero;

            GCHandle handle = NativeInterop.GetTypeGCHandle(managed);
            return GCHandle.ToIntPtr(handle);
        }

        internal static void Free(IntPtr unmanaged)
        {
            // Cached handle, do not release
        }
    }

    [CustomMarshaller(typeof(Exception), MarshalMode.Default, typeof(ExceptionMarshaller))]
    public static class ExceptionMarshaller
    {
        internal static Exception ConvertToManaged(IntPtr unmanaged) => Unsafe.As<Exception>(GCHandleMarshaller.ConvertToManaged(unmanaged));
        internal static IntPtr ConvertToUnmanaged(Exception managed) => GCHandleMarshaller.ConvertToUnmanaged(managed);
        internal static void Free(IntPtr unmanaged) => GCHandleMarshaller.Free(unmanaged);
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
            public static FlaxEngine.Object ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<FlaxEngine.Object>(GCHandle.FromIntPtr(unmanaged).Target) : null;
        }
        public static class ManagedToNative
        {
            public static IntPtr ConvertToUnmanaged(FlaxEngine.Object managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;
        }
    }

    [CustomMarshaller(typeof(CultureInfo), MarshalMode.Default, typeof(CultureInfoMarshaller))]
    public static class CultureInfoMarshaller
    {
        internal static CultureInfo ConvertToManaged(IntPtr unmanaged) => Unsafe.As<CultureInfo>(GCHandleMarshaller.ConvertToManaged(unmanaged));
        internal static IntPtr ConvertToUnmanaged(CultureInfo managed) => GCHandleMarshaller.ConvertToUnmanaged(managed);
        internal static void Free(IntPtr unmanaged) => GCHandleMarshaller.Free(unmanaged);
    }

    [CustomMarshaller(typeof(Array), MarshalMode.ManagedToUnmanagedIn, typeof(SystemArrayMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(Array), MarshalMode.UnmanagedToManagedOut, typeof(SystemArrayMarshaller.ManagedToNative))]
    public static class SystemArrayMarshaller
    {
        public struct ManagedToNative
        {
            ManagedArray managedArray;
            GCHandle handle;

            public void FromManaged(Array managed)
            {
                if (managed != null)
                    managedArray = ManagedArray.WrapPooledArray(managed);
            }

            public IntPtr ToUnmanaged()
            {
                if (managedArray == null)
                    return IntPtr.Zero;

                handle = GCHandle.Alloc(managedArray);
                return GCHandle.ToIntPtr(handle);
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

        public static Dictionary<T, U> ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<Dictionary<T, U>>(GCHandle.FromIntPtr(unmanaged).Target) : null;
        public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;

        public static Dictionary<T, U> ToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<Dictionary<T, U>>(GCHandle.FromIntPtr(unmanaged).Target) : null;
        public static IntPtr ToNative(Dictionary<T, U> managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;
        public static void Free(IntPtr unmanaged)
        {
            if (unmanaged != IntPtr.Zero)
                GCHandle.FromIntPtr(unmanaged).Free();
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
            internal static T[]? AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged is null)
                    return null;

                return new T[numElements];
            }

            internal static Span<T> GetManagedValuesDestination(T[]? managed) => managed;

            internal static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return ReadOnlySpan<TUnmanagedElement>.Empty;

                ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.GetSpan<TUnmanagedElement>();
            }

            internal static void Free(TUnmanagedElement* unmanaged)
            {
                if (unmanaged == null)
                    return;

                GCHandle handle = GCHandle.FromIntPtr(new IntPtr(unmanaged));
                (Unsafe.As<ManagedArray>(handle.Target)).Free();
                handle.Free();
            }

            internal static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;

                ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.GetSpan<TUnmanagedElement>();
            }
        }

        public static class ManagedToNative
        {
            internal static TUnmanagedElement* AllocateContainerForUnmanagedElements(T[]? managed, out int numElements)
            {
                if (managed is null)
                {
                    numElements = 0;
                    return null;
                }

                numElements = managed.Length;

                ManagedArray managedArray = ManagedArray.AllocatePooledArray((TUnmanagedElement*)Marshal.AllocHGlobal(sizeof(TUnmanagedElement) * managed.Length), managed.Length);
                var ptr = GCHandle.ToIntPtr(GCHandle.Alloc(managedArray));

                return (TUnmanagedElement*)ptr;
            }

            internal static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed) => managed;

            internal static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;

                ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.GetSpan<TUnmanagedElement>();
            }

            internal static void Free(TUnmanagedElement* unmanaged)
            {
                if (unmanaged == null)
                    return;

                GCHandle handle = GCHandle.FromIntPtr(new IntPtr(unmanaged));
                (Unsafe.As<ManagedArray>(handle.Target)).FreePooled();
                handle.Free();
            }
        }

        public struct Bidirectional
        {
            T[] managedArray;
            ManagedArray unmanagedArray;
            GCHandle handle;

            public void FromManaged(T[]? managed)
            {
                if (managed == null)
                    return;

                managedArray = managed;

                unmanagedArray = ManagedArray.AllocatePooledArray((TUnmanagedElement*)Marshal.AllocHGlobal(sizeof(TUnmanagedElement) * managed.Length), managed.Length);
                handle = GCHandle.Alloc(unmanagedArray);
            }

            public ReadOnlySpan<T> GetManagedValuesSource() => managedArray;

            public Span<TUnmanagedElement> GetUnmanagedValuesDestination()
            {
                if (unmanagedArray == null)
                    return Span<TUnmanagedElement>.Empty;

                return unmanagedArray.GetSpan<TUnmanagedElement>();
            }

            public TUnmanagedElement* ToUnmanaged() => (TUnmanagedElement*)GCHandle.ToIntPtr(handle);

            public void FromUnmanaged(TUnmanagedElement* unmanaged)
            {
                ManagedArray arr = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
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

            ManagedArray managedArray = ManagedArray.AllocatePooledArray((TUnmanagedElement*)Marshal.AllocHGlobal(sizeof(TUnmanagedElement) * managed.Length), managed.Length);
            IntPtr handle = GCHandle.ToIntPtr(GCHandle.Alloc(managedArray));

            return (TUnmanagedElement*)handle;
        }

        public static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed) => managed;

        public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return Span<TUnmanagedElement>.Empty;

            ManagedArray unmanagedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
            return unmanagedArray.GetSpan<TUnmanagedElement>();
        }

        public static T[]? AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements) => unmanaged is null ? null : new T[numElements];

        public static Span<T> GetManagedValuesDestination(T[]? managed) => managed;

        public static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return ReadOnlySpan<TUnmanagedElement>.Empty;

            ManagedArray array = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
            return array.GetSpan<TUnmanagedElement>();
        }

        public static void Free(TUnmanagedElement* unmanaged)
        {
            if (unmanaged == null)
                return;

            GCHandle handle = GCHandle.FromIntPtr(new IntPtr(unmanaged));
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

                return GCHandle.ToIntPtr(GCHandle.Alloc(managed, GCHandleType.Weak));
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
        internal static Dictionary<string, string> AssemblyLocations = new Dictionary<string, string>();

        private static bool firstAssemblyLoaded = false;

        private static Dictionary<string, Type> typeCache = new Dictionary<string, Type>();

        private static IntPtr boolTruePtr = GCHandle.ToIntPtr(GCHandle.Alloc((int)1, GCHandleType.Pinned));
        private static IntPtr boolFalsePtr = GCHandle.ToIntPtr(GCHandle.Alloc((int)0, GCHandleType.Pinned));

        private static List<GCHandle> methodHandles = new();
        private static List<GCHandle> methodHandlesCollectible = new();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegates = new ConcurrentDictionary<IntPtr, Delegate>();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegatesCollectible = new ConcurrentDictionary<IntPtr, Delegate>();
        private static Dictionary<Type, GCHandle> typeHandleCache = new Dictionary<Type, GCHandle>();
        private static Dictionary<Type, GCHandle> typeHandleCacheCollectible = new Dictionary<Type, GCHandle>();
        private static List<GCHandle> fieldHandleCache = new();
        private static List<GCHandle> fieldHandleCacheCollectible = new();
        private static Dictionary<object, GCHandle> classAttributesCacheCollectible = new();
        private static Dictionary<Assembly, GCHandle> assemblyHandles = new Dictionary<Assembly, GCHandle>();

        private static Dictionary<string, IntPtr> loadedNativeLibraries = new Dictionary<string, IntPtr>();
        private static Dictionary<string, string> nativeLibraryPaths = new Dictionary<string, string>();
        private static Dictionary<Assembly, string> assemblyOwnedNativeLibraries = new Dictionary<Assembly, string>();
        private static AssemblyLoadContext scriptingAssemblyLoadContext;

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

            // TODO: benchmark collectible setting performance, maybe enable it only in editor builds?
            scriptingAssemblyLoadContext = new AssemblyLoadContext(null, true);

            DelegateHelpers.Init();
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

        internal static T[] GCHandleArrayToManagedArray<T>(ManagedArray ptrArray)
        {
            Span<IntPtr> span = ptrArray.GetSpan<IntPtr>();
            T[] managedArray = new T[ptrArray.Length];
            for (int i = 0; i < managedArray.Length; i++)
                managedArray[i] = span[i] != IntPtr.Zero ? (T)GCHandle.FromIntPtr(span[i]).Target : default;
            return managedArray;
        }

        internal static IntPtr[] ManagedArrayToGCHandleArray(Array array)
        {
            IntPtr[] pointerArray = new IntPtr[array.Length];
            for (int i = 0; i < pointerArray.Length; i++)
            {
                var obj = array.GetValue(i);
                if (obj != null)
                    pointerArray.SetValue(GCHandle.ToIntPtr(GCHandle.Alloc(obj)), i);
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

            if (toManagedMarshallers.TryGetValue(type, out var deleg))
                return deleg(nativePtr, type.IsByRef);
            return toManagedMarshallers.GetOrAdd(type, Factory)(nativePtr, type.IsByRef);
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

            if (toNativeMarshallers.TryGetValue(type, out var deleg))
                deleg(managedObject, nativePtr);
            else
                toNativeMarshallers.GetOrAdd(type, Factory)(managedObject, nativePtr);
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

            internal static Array ToManagedArray(Span<IntPtr> ptrSpan)
            {
                T[] arr = new T[ptrSpan.Length];
                for (int i = 0; i < arr.Length; i++)
                    toManagedTypedMarshaller(ref arr[i], ptrSpan[i], false);
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
                    fieldOffset = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr fieldStartPtr = fieldPtr;
                        fieldPtr = EnsureAlignment(fieldPtr, fieldAlignment);
                        fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();
                    }

                    ref TField[] fieldValueRef = ref GetFieldReference<TField[]>(field, ref fieldOwner);
                    MarshalHelperValueType<TField>.ToManagedArray(ref fieldValueRef, fieldPtr, false);
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
                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperReferenceType<TField>.ToManaged(ref fieldValueRef, Marshal.ReadIntPtr(fieldPtr), false);
                    fieldOffset = IntPtr.Size;
                }

                internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperReferenceType<TField>.ToNative(ref fieldValueRef, fieldPtr);
                    fieldOffset = IntPtr.Size;
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
                Assert.IsTrue(!byRef);

                Type elementType = typeof(T);
                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(nativePtr).Target);
                    if (ArrayFactory.GetMarshalledType(elementType) == elementType)
                        managedValue = Unsafe.As<T[]>(managedArray.GetArray<T>());
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
                    managedValue = nativePtr != IntPtr.Zero ? Unsafe.As<T>(GCHandle.FromIntPtr(nativePtr).Target) : null;
                else if (type.IsInterface) // Dictionary
                    managedValue = nativePtr != IntPtr.Zero ? Unsafe.As<T>(GCHandle.FromIntPtr(nativePtr).Target) : null;
                else
                    throw new NotImplementedException();
            }

            internal static void ToManagedArray(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                Assert.IsTrue(!byRef);

                Type elementType = typeof(T);
                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(nativePtr).Target);
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
                    managedPtr = ManagedString.ToNative(managedValue as string);
                else if (type.IsPointer)
                {
                    if (Pointer.Unbox(managedValue) == null)
                        managedPtr = IntPtr.Zero;
                    else if (managedValue is FlaxEngine.Object flaxObj)
                        managedPtr = FlaxEngine.Object.GetUnmanagedPtr(flaxObj);
                    else
                        managedPtr = GCHandle.ToIntPtr(GCHandle.Alloc(managedValue, GCHandleType.Weak));
                }
                else if (type == typeof(Type))
                    managedPtr = managedValue != null ? GCHandle.ToIntPtr(GetTypeGCHandle((Type)(object)managedValue)) : IntPtr.Zero;
                else
                    managedPtr = managedValue != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managedValue, GCHandleType.Weak)) : IntPtr.Zero;

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
                parameterTypes = method.GetParameters().Select(x => x.ParameterType).ToArray();
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

                    string typeName = $"FlaxEngine.NativeInterop+Invoker+Invoker{(method.IsStatic ? "Static" : "")}{(method.ReturnType != typeof(void) ? "Ret" : "NoRet")}{parameterTypes.Length}{(genericParamTypes.Count > 0 ? "`" + genericParamTypes.Count : "")}";
                    Type invokerType = genericParamTypes.Count == 0 ? Type.GetType(typeName) : Type.GetType(typeName).MakeGenericType(genericParamTypes.ToArray());
                    invokeDelegate = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.MarshalAndInvoke), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Invoker.MarshalAndInvokeDelegate>();
                    delegInvoke = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.CreateDelegate), BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { method });
                }

                outDeleg = invokeDelegate;
                outDelegInvoke = delegInvoke;

                return outDeleg != null;
            }
        }

        internal static GCHandle GetMethodGCHandle(MethodInfo method)
        {
            MethodHolder methodHolder = new MethodHolder(method);

            GCHandle handle = GCHandle.Alloc(methodHolder);
            if (methodHolder.parameterTypes.Any(x => x.IsCollectible) || method.IsCollectible)
                methodHandlesCollectible.Add(handle);
            else
                methodHandles.Add(handle);
            return handle;
        }

        internal static GCHandle GetAssemblyHandle(Assembly assembly)
        {
            if (!assemblyHandles.TryGetValue(assembly, out GCHandle handle))
            {
                handle = GCHandle.Alloc(assembly);
                assemblyHandles.Add(assembly, handle);
            }
            return handle;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetManagedClasses(IntPtr assemblyHandle, NativeClassDefinitions** managedClasses, int* managedClassCount)
        {
            Assembly assembly = Unsafe.As<Assembly>(GCHandle.FromIntPtr(assemblyHandle).Target);
            var assemblyTypes = GetAssemblyTypes(assembly);

            NativeClassDefinitions* arr = (NativeClassDefinitions*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<NativeClassDefinitions>() * assemblyTypes.Length).ToPointer();

            for (int i = 0; i < assemblyTypes.Length; i++)
            {
                var type = assemblyTypes[i];
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeClassDefinitions>() * i);
                bool isStatic = type.IsAbstract && type.IsSealed;
                bool isInterface = type.IsInterface;
                bool isAbstract = type.IsAbstract;

                GCHandle typeHandle = GetTypeGCHandle(type);

                NativeClassDefinitions managedClass = new NativeClassDefinitions()
                {
                    typeHandle = GCHandle.ToIntPtr(typeHandle),
                    name = Marshal.StringToCoTaskMemAnsi(type.Name),
                    fullname = Marshal.StringToCoTaskMemAnsi(type.FullName),
                    @namespace = Marshal.StringToCoTaskMemAnsi(type.Namespace ?? ""),
                    typeAttributes = (uint)type.Attributes,
                };
                Unsafe.Write(ptr.ToPointer(), managedClass);
            }

            *managedClasses = arr;
            *managedClassCount = assemblyTypes.Length;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetManagedClassFromType(IntPtr typeHandle, NativeClassDefinitions* managedClass, IntPtr* assemblyHandle)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            GCHandle classTypeHandle = GetTypeGCHandle(type);

            *managedClass = new NativeClassDefinitions()
            {
                typeHandle = GCHandle.ToIntPtr(classTypeHandle),
                name = Marshal.StringToCoTaskMemAnsi(type.Name),
                fullname = Marshal.StringToCoTaskMemAnsi(type.FullName),
                @namespace = Marshal.StringToCoTaskMemAnsi(type.Namespace ?? ""),
                typeAttributes = (uint)type.Attributes,
            };
            *assemblyHandle = GCHandle.ToIntPtr(GetAssemblyHandle(type.Assembly));
        }

        [UnmanagedCallersOnly]
        internal static void GetClassMethods(IntPtr typeHandle, NativeMethodDefinitions** classMethods, int* classMethodsCount)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);

            List<MethodInfo> methods = new List<MethodInfo>();
            var staticMethods = type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            var instanceMethods = type.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            foreach (MethodInfo method in staticMethods)
                methods.Add(method);
            foreach (MethodInfo method in instanceMethods)
                methods.Add(method);

            NativeMethodDefinitions* arr = (NativeMethodDefinitions*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<NativeMethodDefinitions>() * methods.Count).ToPointer();
            for (int i = 0; i < methods.Count; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeMethodDefinitions>() * i);
                NativeMethodDefinitions classMethod = new NativeMethodDefinitions()
                {
                    name = Marshal.StringToCoTaskMemAnsi(methods[i].Name),
                    numParameters = methods[i].GetParameters().Length,
                    methodAttributes = (uint)methods[i].Attributes,
                };
                classMethod.typeHandle = GCHandle.ToIntPtr(GetMethodGCHandle(methods[i]));
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
        internal static void GetClassFields(IntPtr typeHandle, NativeFieldDefinitions** classFields, int* classFieldsCount)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            NativeFieldDefinitions* arr = (NativeFieldDefinitions*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<NativeFieldDefinitions>() * fields.Length).ToPointer();
            for (int i = 0; i < fields.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeFieldDefinitions>() * i);

                FieldHolder fieldHolder = new FieldHolder(fields[i], type);

                GCHandle fieldHandle = GCHandle.Alloc(fieldHolder);
                if (type.IsCollectible)
                    fieldHandleCacheCollectible.Add(fieldHandle);
                else
                    fieldHandleCache.Add(fieldHandle);

                NativeFieldDefinitions classField = new NativeFieldDefinitions()
                {
                    name = Marshal.StringToCoTaskMemAnsi(fieldHolder.field.Name),
                    fieldHandle = GCHandle.ToIntPtr(fieldHandle),
                    fieldTypeHandle = GCHandle.ToIntPtr(GetTypeGCHandle(fieldHolder.field.FieldType)),
                    fieldAttributes = (uint)fieldHolder.field.Attributes,
                };
                Unsafe.Write(ptr.ToPointer(), classField);
            }
            *classFields = arr;
            *classFieldsCount = fields.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassProperties(IntPtr typeHandle, NativePropertyDefinitions** classProperties, int* classPropertiesCount)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            var properties = type.GetProperties(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            NativePropertyDefinitions* arr = (NativePropertyDefinitions*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<NativePropertyDefinitions>() * properties.Length).ToPointer();
            for (int i = 0; i < properties.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativePropertyDefinitions>() * i);

                var getterMethod = properties[i].GetGetMethod(true);
                var setterMethod = properties[i].GetSetMethod(true);

                NativePropertyDefinitions classProperty = new NativePropertyDefinitions()
                {
                    name = Marshal.StringToCoTaskMemAnsi(properties[i].Name),
                };
                if (getterMethod != null)
                {
                    var getterHandle = GetMethodGCHandle(getterMethod);
                    classProperty.getterHandle = GCHandle.ToIntPtr(getterHandle);
                }
                if (setterMethod != null)
                {
                    var setterHandle = GetMethodGCHandle(setterMethod);
                    classProperty.setterHandle = GCHandle.ToIntPtr(setterHandle);
                }
                Unsafe.Write(ptr.ToPointer(), classProperty);
            }
            *classProperties = arr;
            *classPropertiesCount = properties.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassAttributes(IntPtr typeHandle, NativeAttributeDefinitions** classAttributes, int* classAttributesCount)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            object[] attributeValues = type.GetCustomAttributes(false);
            Type[] attributeTypes = type.GetCustomAttributes(false).Select(x => x.GetType()).ToArray();

            NativeAttributeDefinitions* arr = (NativeAttributeDefinitions*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<NativeAttributeDefinitions>() * attributeTypes.Length).ToPointer();
            for (int i = 0; i < attributeTypes.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeAttributeDefinitions>() * i);

                if (!classAttributesCacheCollectible.TryGetValue(attributeValues[i], out GCHandle attributeHandle))
                {
                    attributeHandle = GCHandle.Alloc(attributeValues[i]);
                    classAttributesCacheCollectible.Add(attributeValues[i], attributeHandle);
                }
                GCHandle attributeTypeHandle = GetTypeGCHandle(attributeTypes[i]);

                NativeAttributeDefinitions classAttribute = new NativeAttributeDefinitions()
                {
                    name = Marshal.StringToCoTaskMemAnsi(attributeTypes[i].Name),
                    attributeTypeHandle = GCHandle.ToIntPtr(attributeTypeHandle),
                    attributeHandle = GCHandle.ToIntPtr(attributeHandle),
                };
                Unsafe.Write(ptr.ToPointer(), classAttribute);
            }
            *classAttributes = arr;
            *classAttributesCount = attributeTypes.Length;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetCustomAttribute(IntPtr typeHandle, IntPtr attribHandle)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            Type attribType = Unsafe.As<Type>(GCHandle.FromIntPtr(attribHandle).Target);
            object attrib = type.GetCustomAttributes(false).FirstOrDefault(x => x.GetType() == attribType);

            if (attrib != null)
                return GCHandle.ToIntPtr(GCHandle.Alloc(attrib, GCHandleType.Weak));
            return IntPtr.Zero;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassInterfaces(IntPtr typeHandle, IntPtr* classInterfaces, int* classInterfacesCount)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            Type[] interfaces = type.GetInterfaces();

            // mono doesn't seem to return any interfaces, or returns only "immediate" interfaces? // FIXME?
            //foreach (Type interfaceType in type.BaseType.GetInterfaces())
            //    interfaces.Remove(interfaceType.FullName);

            IntPtr arr = Marshal.AllocCoTaskMem(IntPtr.Size * interfaces.Length);
            for (int i = 0; i < interfaces.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(arr, IntPtr.Size * i);

                GCHandle handle = GetTypeGCHandle(interfaces[i]);
                Marshal.WriteIntPtr(ptr, GCHandle.ToIntPtr(handle));
            }
            *classInterfaces = arr;
            *classInterfacesCount = interfaces.Length;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetMethodReturnType(IntPtr methodHandle)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(GCHandle.FromIntPtr(methodHandle).Target);
            Type returnType = methodHolder.method.ReturnType;

            return GCHandle.ToIntPtr(GetTypeGCHandle(returnType));
        }

        [UnmanagedCallersOnly]
        internal static void GetMethodParameterTypes(IntPtr methodHandle, IntPtr* typeHandles)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(GCHandle.FromIntPtr(methodHandle).Target);
            Type returnType = methodHolder.method.ReturnType;

            IntPtr arr = Marshal.AllocCoTaskMem(IntPtr.Size * methodHolder.parameterTypes.Length);
            for (int i = 0; i < methodHolder.parameterTypes.Length; i++)
            {
                GCHandle typeHandle = GetTypeGCHandle(methodHolder.parameterTypes[i]);
                Marshal.WriteIntPtr(IntPtr.Add(new IntPtr(arr), IntPtr.Size * i), GCHandle.ToIntPtr(typeHandle));
            }
            *typeHandles = arr;
        }

        /// <summary>
        /// Returns pointer to the string's internal structure, containing the buffer and length of the string.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static IntPtr GetStringPointer(IntPtr stringHandle)
        {
            string str = Unsafe.As<string>(GCHandle.FromIntPtr(stringHandle).Target);
            IntPtr ptr = (Unsafe.Read<IntPtr>(Unsafe.AsPointer(ref str)) + sizeof(int) * 2);
            if (ptr < 0x1024)
                throw new Exception("null string ptr");
            return ptr;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewObject(IntPtr typeHandle)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            if (type == typeof(Script))
            {
                // FIXME: Script is an abstract type which can not be instantiated
                type = typeof(VisjectScript);
            }

            object value = RuntimeHelpers.GetUninitializedObject(type);
            return GCHandle.ToIntPtr(GCHandle.Alloc(value));
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
        internal static IntPtr NewArray(IntPtr typeHandle, long size)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);

            Type marshalledType = ArrayFactory.GetMarshalledType(type);
            if (marshalledType.IsValueType)
            {
                ManagedArray managedArray = ManagedArray.AllocateNewArray(Marshal.AllocHGlobal(Marshal.SizeOf(marshalledType) * (int)size), (int)size, Marshal.SizeOf(marshalledType));
                return GCHandle.ToIntPtr(GCHandle.Alloc(managedArray/*, GCHandleType.Weak*/));
            }
            else
            {
                Array arr = ArrayFactory.CreateArray(type, size);
                ManagedArray managedArray = ManagedArray.WrapNewArray(arr);
                return GCHandle.ToIntPtr(GCHandle.Alloc(managedArray/*, GCHandleType.Weak*/));
            }
        }

        [UnmanagedCallersOnly]
        internal static unsafe IntPtr GetArrayPointerToElement(IntPtr arrayHandle, int size, int index)
        {
            ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(arrayHandle).Target);
            if (managedArray.Length == 0)
                return IntPtr.Zero;

            Assert.IsTrue(index >= 0 && index < managedArray.Length);
            return IntPtr.Add(managedArray.GetPointer, size * index);
        }

        [UnmanagedCallersOnly]
        internal static int GetArrayLength(IntPtr arrayHandle)
        {
            ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(arrayHandle).Target);
            return managedArray.Length;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetStringEmpty()
        {
            return ManagedString.ToNative(string.Empty);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewStringUTF16(char* text, int length)
        {
            return ManagedString.ToNative(new string(new ReadOnlySpan<char>(text, length)));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewString(sbyte* text)
        {
            return ManagedString.ToNative(new string(text));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewStringLength(sbyte* text, int length)
        {
            return ManagedString.ToNative(new string(text, 0, length));
        }

        /// <summary>
        /// Creates a managed copy of the value, and stores it in a boxed reference.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static IntPtr BoxValue(IntPtr typeHandle, IntPtr valuePtr)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            object value = MarshalToManaged(valuePtr, type);
            return GCHandle.ToIntPtr(GCHandle.Alloc(value, GCHandleType.Weak));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetObjectType(IntPtr handle)
        {
            var obj = GCHandle.FromIntPtr(handle).Target;
            Type classType = obj.GetType();
            return GCHandle.ToIntPtr(GetTypeGCHandle(classType));
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
        internal unsafe static IntPtr UnboxValue(IntPtr handlePtr)
        {
            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
            object value = handle.Target;
            Type type = value.GetType();

            if (!type.IsValueType)
                return handlePtr;

            // HACK: Get the address of a non-pinned value
            return ValueTypeUnboxer.GetPointer(value);
        }

        [UnmanagedCallersOnly]
        internal static void RaiseException(IntPtr exceptionHandle)
        {
            Exception exception = Unsafe.As<Exception>(GCHandle.FromIntPtr(exceptionHandle).Target);
            throw exception;
        }

        [UnmanagedCallersOnly]
        internal static void ObjectInit(IntPtr objectHandle)
        {
            object obj = GCHandle.FromIntPtr(objectHandle).Target;
            Type type = obj.GetType();

            ConstructorInfo ctor = type.GetMember(".ctor", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance).First() as ConstructorInfo;
            ctor.Invoke(obj, null);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr InvokeMethod(IntPtr instancePtr, IntPtr methodHandle, IntPtr paramPtr, IntPtr exceptionPtr)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(GCHandle.FromIntPtr(methodHandle).Target);
            if (methodHolder.TryGetDelegate(out var methodDelegate, out var methodDelegateContext))
            {
                // Fast path, invoke the method with minimal allocations
                IntPtr returnValue;
                try
                {
                    returnValue = methodDelegate(methodDelegateContext, instancePtr, paramPtr);
                }
                catch (Exception exception)
                {
                    Marshal.WriteIntPtr(exceptionPtr, GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak)));
                    return IntPtr.Zero;
                }
                return returnValue;
            }
            else
            {
                // Slow path, method parameters needs to be stored in heap
                object returnObject;
                int numParams = methodHolder.parameterTypes.Length;
                IntPtr* nativePtrs = stackalloc IntPtr[numParams];
                object[] methodParameters = new object[numParams];

                for (int i = 0; i < numParams; i++)
                {
                    nativePtrs[i] = Marshal.ReadIntPtr(IntPtr.Add(paramPtr, sizeof(IntPtr) * i));
                    methodParameters[i] = MarshalToManaged(nativePtrs[i], methodHolder.parameterTypes[i]);
                }

                try
                {
                    returnObject = methodHolder.method.Invoke(instancePtr != IntPtr.Zero ? GCHandle.FromIntPtr(instancePtr).Target : null, methodParameters);
                }
                catch (Exception exception)
                {
                    // The internal exception thrown in MethodInfo.Invoke is caught here
                    Exception realException = exception;
                    if (exception.InnerException != null && exception.TargetSite.ReflectedType.Name == "MethodInvoker")
                        realException = exception.InnerException;

                    if (exceptionPtr != IntPtr.Zero)
                        Marshal.WriteIntPtr(exceptionPtr, GCHandle.ToIntPtr(GCHandle.Alloc(realException, GCHandleType.Weak)));
                    else
                        throw realException;
                    return IntPtr.Zero;
                }

                // Marshal reference parameters back to original unmanaged references
                for (int i = 0; i < numParams; i++)
                {
                    Type parameterType = methodHolder.parameterTypes[i];
                    if (parameterType.IsByRef)
                        MarshalToNative(methodParameters[i], nativePtrs[i], parameterType.GetElementType());
                }

                if (returnObject is not null)
                {
                    if (methodHolder.method.ReturnType == typeof(string))
                        return ManagedString.ToNative(Unsafe.As<string>(returnObject));
                    else if (methodHolder.method.ReturnType == typeof(IntPtr))
                        return (IntPtr)returnObject;
                    else if (methodHolder.method.ReturnType == typeof(bool))
                        return (bool)returnObject ? boolTruePtr : boolFalsePtr;
                    else if (methodHolder.method.ReturnType == typeof(Type))
                        return GCHandle.ToIntPtr(GetTypeGCHandle(Unsafe.As<Type>(returnObject)));
                    else if (methodHolder.method.ReturnType == typeof(object[]))
                        return GCHandle.ToIntPtr(GCHandle.Alloc(ManagedArray.WrapNewArray(ManagedArrayToGCHandleArray(Unsafe.As<object[]>(returnObject))), GCHandleType.Weak));
                    else
                        return GCHandle.ToIntPtr(GCHandle.Alloc(returnObject, GCHandleType.Weak));
                }
                return IntPtr.Zero;
            }
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetMethodUnmanagedFunctionPointer(IntPtr methodHandle)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(GCHandle.FromIntPtr(methodHandle).Target);

            // Wrap the method call, this is needed to get the object instance from GCHandle and to pass the exception back to native side
            MethodInfo invokeThunk = typeof(ThunkContext).GetMethod(nameof(ThunkContext.InvokeThunk));
            Type delegateType = DelegateHelpers.MakeNewCustomDelegate(invokeThunk.GetParameters().Select(x => x.ParameterType).Append(invokeThunk.ReturnType).ToArray());

            ThunkContext context = new ThunkContext(methodHolder.method);
            Delegate methodDelegate = invokeThunk.CreateDelegate(delegateType, context);
            IntPtr functionPtr = Marshal.GetFunctionPointerForDelegate(methodDelegate);

            // Keep a reference to the delegate to prevent it from being garbage collected
            if (methodHolder.method.IsCollectible)
                cachedDelegatesCollectible[functionPtr] = methodDelegate;
            else
                cachedDelegates[functionPtr] = methodDelegate;

            return functionPtr;
        }

        [UnmanagedCallersOnly]
        internal static void FieldSetValue(IntPtr fieldOwnerHandle, IntPtr fieldHandle, IntPtr valuePtr)
        {
            object fieldOwner = GCHandle.FromIntPtr(fieldOwnerHandle).Target;
            FieldHolder field = Unsafe.As<FieldHolder>(GCHandle.FromIntPtr(fieldHandle).Target);
            field.field.SetValue(fieldOwner, Marshal.PtrToStructure(valuePtr, field.field.FieldType));
        }

        [UnmanagedCallersOnly]
        internal static void FieldGetValue(IntPtr fieldOwnerHandle, IntPtr fieldHandle, IntPtr valuePtr)
        {
            object fieldOwner = GCHandle.FromIntPtr(fieldOwnerHandle).Target;
            FieldHolder field = Unsafe.As<FieldHolder>(GCHandle.FromIntPtr(fieldHandle).Target);
            field.toNativeMarshaller(field.field, fieldOwner, valuePtr, out int fieldOffset);
        }

        [UnmanagedCallersOnly]
        internal static void SetArrayValueReference(IntPtr arrayHandle, IntPtr elementPtr, IntPtr valueHandle)
        {
            ManagedArray managedArray = Unsafe.As<ManagedArray>(GCHandle.FromIntPtr(arrayHandle).Target);
            int index = (int)(elementPtr.ToInt64() - managedArray.GetPointer.ToInt64()) / managedArray.ElementSize;
            managedArray.GetSpan<IntPtr>()[index] = valueHandle;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr LoadAssemblyFromPath(IntPtr assemblyPath_, IntPtr* assemblyName, IntPtr* assemblyFullName)
        {
            if (!firstAssemblyLoaded)
            {
                // This assembly was already loaded when initializing the host context
                firstAssemblyLoaded = true;

                Assembly flaxEngineAssembly = AppDomain.CurrentDomain.GetAssemblies().First(x => x.GetName().Name == "FlaxEngine.CSharp");
                *assemblyName = Marshal.StringToCoTaskMemAnsi(flaxEngineAssembly.GetName().Name);
                *assemblyFullName = Marshal.StringToCoTaskMemAnsi(flaxEngineAssembly.FullName);
                return GCHandle.ToIntPtr(GetAssemblyHandle(flaxEngineAssembly));
            }
            string assemblyPath = Marshal.PtrToStringAnsi(assemblyPath_);

            Assembly assembly = scriptingAssemblyLoadContext.LoadFromAssemblyPath(assemblyPath);
            NativeLibrary.SetDllImportResolver(assembly, NativeLibraryImportResolver);

            *assemblyName = Marshal.StringToCoTaskMemAnsi(assembly.GetName().Name);
            *assemblyFullName = Marshal.StringToCoTaskMemAnsi(assembly.FullName);
            return GCHandle.ToIntPtr(GetAssemblyHandle(assembly));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr LoadAssemblyImage(IntPtr data, int len, IntPtr assemblyPath_, IntPtr* assemblyName, IntPtr* assemblyFullName)
        {
            if (!firstAssemblyLoaded)
            {
                // This assembly was already loaded when initializing the host context
                firstAssemblyLoaded = true;

                Assembly flaxEngineAssembly = AppDomain.CurrentDomain.GetAssemblies().First(x => x.GetName().Name == "FlaxEngine.CSharp");
                *assemblyName = Marshal.StringToCoTaskMemAnsi(flaxEngineAssembly.GetName().Name);
                *assemblyFullName = Marshal.StringToCoTaskMemAnsi(flaxEngineAssembly.FullName);
                return GCHandle.ToIntPtr(GetAssemblyHandle(flaxEngineAssembly));
            }

            string assemblyPath = Marshal.PtrToStringAnsi(assemblyPath_);

            byte[] raw = new byte[len];
            Marshal.Copy(data, raw, 0, len);

            using MemoryStream stream = new MemoryStream(raw);
            Assembly assembly = scriptingAssemblyLoadContext.LoadFromStream(stream);
            NativeLibrary.SetDllImportResolver(assembly, NativeLibraryImportResolver);

            // Assemblies loaded via streams have no Location: https://github.com/dotnet/runtime/issues/12822
            AssemblyLocations.Add(assembly.FullName, assemblyPath);

            *assemblyName = Marshal.StringToCoTaskMemAnsi(assembly.GetName().Name);
            *assemblyFullName = Marshal.StringToCoTaskMemAnsi(assembly.FullName);
            return GCHandle.ToIntPtr(GetAssemblyHandle(assembly));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetAssemblyByName(IntPtr name_, IntPtr* assemblyName, IntPtr* assemblyFullName)
        {
            string name = Marshal.PtrToStringAnsi(name_);

            Assembly assembly = AppDomain.CurrentDomain.GetAssemblies().FirstOrDefault(x => x.GetName().Name == name);
            if (assembly == null)
                assembly = scriptingAssemblyLoadContext.Assemblies.FirstOrDefault(x => x.GetName().Name == name);

            *assemblyName = Marshal.StringToCoTaskMemAnsi(assembly.GetName().Name);
            *assemblyFullName = Marshal.StringToCoTaskMemAnsi(assembly.FullName);
            return GCHandle.ToIntPtr(GetAssemblyHandle(assembly));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetRuntimeInformation()
        {
            return Marshal.StringToCoTaskMemAnsi(System.Runtime.InteropServices.RuntimeInformation.FrameworkDescription);
        }

        [UnmanagedCallersOnly]
        internal static void CloseAssembly(IntPtr assemblyHandle)
        {
            Assembly assembly = Unsafe.As<Assembly>(GCHandle.FromIntPtr(assemblyHandle).Target);
            GCHandle.FromIntPtr(assemblyHandle).Free();
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
            scriptingAssemblyLoadContext = new AssemblyLoadContext(null, true);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetAssemblyObject(IntPtr assemblyName_)
        {
            string assemblyName = Marshal.PtrToStringAnsi(assemblyName_);
            Assembly assembly = null;
            assembly = scriptingAssemblyLoadContext.Assemblies.FirstOrDefault(x => x.FullName == assemblyName);
            if (assembly == null)
                assembly = System.AppDomain.CurrentDomain.GetAssemblies().First(x => x.FullName == assemblyName);

            return GCHandle.ToIntPtr(GCHandle.Alloc(assembly));
        }

        [UnmanagedCallersOnly]
        internal static unsafe int NativeSizeOf(IntPtr typeHandle, uint* align)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            Type nativeType = GetInternalType(type) ?? type;

            if (nativeType == typeof(Version))
                nativeType = typeof(VersionNative);

            int size = Marshal.SizeOf(nativeType);
            *align = (uint)size; // Is this correct?
            return size;
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsSubclassOf(IntPtr typeHandle, IntPtr othertypeHandle, byte checkInterfaces)
        {
            if (typeHandle == othertypeHandle)
                return 1;

            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            Type otherType = Unsafe.As<Type>(GCHandle.FromIntPtr(othertypeHandle).Target);

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
        internal static byte TypeIsValueType(IntPtr typeHandle)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            return (byte)(type.IsValueType ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsEnum(IntPtr typeHandle)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            return (byte)(type.IsEnum ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetClassParent(IntPtr typeHandle)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);
            return GCHandle.ToIntPtr(GetTypeGCHandle(type.BaseType));
        }

        [UnmanagedCallersOnly]
        internal static byte GetMethodParameterIsOut(IntPtr methodHandle, int parameterNum)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(GCHandle.FromIntPtr(methodHandle).Target);
            ParameterInfo parameterInfo = methodHolder.method.GetParameters()[parameterNum];
            return (byte)(parameterInfo.IsOut ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetNullReferenceException()
        {
            var exception = new NullReferenceException();
            return GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetNotSupportedException()
        {
            var exception = new NotSupportedException();
            return GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetArgumentNullException()
        {
            var exception = new ArgumentNullException();
            return GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetArgumentException()
        {
            var exception = new ArgumentException();
            return GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetArgumentOutOfRangeException()
        {
            var exception = new ArgumentOutOfRangeException();
            return GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewGCHandle(IntPtr ptr, byte pinned)
        {
            object value = GCHandle.FromIntPtr(ptr).Target;
            GCHandle handle = GCHandle.Alloc(value, pinned != 0 ? GCHandleType.Pinned : GCHandleType.Normal);
            return GCHandle.ToIntPtr(handle);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewGCHandleWeakref(IntPtr ptr, byte track_resurrection)
        {
            object value = GCHandle.FromIntPtr(ptr).Target;
            GCHandle handle = GCHandle.Alloc(value, track_resurrection != 0 ? GCHandleType.WeakTrackResurrection : GCHandleType.Weak);
            return GCHandle.ToIntPtr(handle);
        }

        [UnmanagedCallersOnly]
        internal static void FreeGCHandle(IntPtr ptr)
        {
            GCHandle handle = GCHandle.FromIntPtr(ptr);
            handle.Free();
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
        internal static int GetTypeMonoTypeEnum(IntPtr typeHandle)
        {
            Type type = Unsafe.As<Type>(GCHandle.FromIntPtr(typeHandle).Target);

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

            List<string> referencedTypes = new List<string>();
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

            Assert.IsTrue(AppDomain.CurrentDomain.GetAssemblies().Where(x => x.GetName().Name == "FlaxEngine.CSharp").Count() == 1);

            return types;
        }

        /// <summary>
        /// Returns a static GCHandle for given Type, and caches it if needed.
        /// </summary>
        internal static GCHandle GetTypeGCHandle(Type type)
        {
            if (typeHandleCache.TryGetValue(type, out GCHandle handle))
                return handle;

            if (typeHandleCacheCollectible.TryGetValue(type, out handle))
                return handle;

            handle = GCHandle.Alloc(type);
            if (type.IsCollectible) // check if generic parameters are also collectible?
                typeHandleCacheCollectible.Add(type, handle);
            else
                typeHandleCache.Add(type, handle);

            return handle;
        }

        private static class DelegateHelpers
        {
            private static readonly Func<Type[], Type> MakeNewCustomDelegateFunc =
                typeof(Expression).Assembly.GetType("System.Linq.Expressions.Compiler.DelegateHelpers")
                .GetMethod("MakeNewCustomDelegate", BindingFlags.NonPublic | BindingFlags.Static)
                .CreateDelegate<Func<Type[], Type>>();

            internal static void Init()
            {
                // Ensures the MakeNewCustomDelegate is put in the collectible ALC?
                using var ctx = scriptingAssemblyLoadContext.EnterContextualReflection();

                MakeNewCustomDelegateFunc(new[] { typeof(void) });
            }

            internal static Type MakeNewCustomDelegate(Type[] parameters)
            {
                if (parameters.Any(x => scriptingAssemblyLoadContext.Assemblies.Contains(x.Module.Assembly)))
                {
                    // Ensure the new delegate is placed in the collectible ALC
                    //using var ctx = scriptingAssemblyLoadContext.EnterContextualReflection();
                    return MakeNewCustomDelegateFunc(parameters);
                }

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
                parameterTypes = method.GetParameters().Select(x => x.ParameterType).ToArray();

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

                string typeName = $"FlaxEngine.NativeInterop+Invoker+Invoker{(method.IsStatic ? "Static" : "")}{(method.ReturnType != typeof(void) ? "Ret" : "NoRet")}{parameterTypes.Length}{(genericParamTypes.Count > 0 ? "`" + genericParamTypes.Count : "")}";
                Type invokerType = genericParamTypes.Count == 0 ? Type.GetType(typeName) : Type.GetType(typeName).MakeGenericType(genericParamTypes.ToArray());
                methodDelegate = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.InvokeThunk), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Invoker.InvokeThunkDelegate>();
                methodDelegateContext = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.CreateInvokerDelegate), BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { method });

                if (methodDelegate != null)
                    Assert.IsTrue(methodDelegateContext != null);
            }

            public IntPtr InvokeThunk(IntPtr instancePtr, IntPtr param1, IntPtr param2, IntPtr param3, IntPtr param4, IntPtr param5, IntPtr param6, IntPtr param7)
            {
                IntPtr* nativePtrs = stackalloc IntPtr[] { param1, param2, param3, param4, param5, param6, param7 };

                if (methodDelegate != null)
                {
                    IntPtr returnValue;
                    try
                    {
                        returnValue = methodDelegate(methodDelegateContext, instancePtr, nativePtrs);
                    }
                    catch (Exception exception)
                    {
                        // Returned exception is the last parameter
                        IntPtr exceptionPtr = nativePtrs[parameterTypes.Length];
                        Marshal.WriteIntPtr(exceptionPtr, GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak)));
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
                        methodParameters[i] = nativePtrs[i] == IntPtr.Zero ? null : GCHandle.FromIntPtr(nativePtrs[i]).Target;

                    try
                    {
                        returnObject = method.Invoke(instancePtr != IntPtr.Zero ? GCHandle.FromIntPtr(instancePtr).Target : null, methodParameters);
                    }
                    catch (Exception exception)
                    {
                        // Returned exception is the last parameter
                        IntPtr exceptionPtr = nativePtrs[numParams];
                        Marshal.WriteIntPtr(exceptionPtr, GCHandle.ToIntPtr(GCHandle.Alloc(exception, GCHandleType.Weak)));
                        return IntPtr.Zero;
                    }

                    if (returnObject is not null)
                    {
                        if (method.ReturnType == typeof(string))
                            return ManagedString.ToNative(Unsafe.As<string>(returnObject));
                        else if (method.ReturnType == typeof(IntPtr))
                            return (IntPtr)returnObject;
                        else if (method.ReturnType == typeof(bool))
                            return (bool)returnObject ? boolTruePtr : boolFalsePtr;
                        else if (method.ReturnType == typeof(Type))
                            return GCHandle.ToIntPtr(GetTypeGCHandle(Unsafe.As<Type>(returnObject)));
                        else if (method.ReturnType == typeof(object[]))
                            return GCHandle.ToIntPtr(GCHandle.Alloc(ManagedArray.WrapNewArray(ManagedArrayToGCHandleArray(Unsafe.As<object[]>(returnObject))), GCHandleType.Weak));
                        else
                            return GCHandle.ToIntPtr(GCHandle.Alloc(returnObject, GCHandleType.Weak));
                    }
                    return IntPtr.Zero;
                }
            }
        }
    }
}
#endif

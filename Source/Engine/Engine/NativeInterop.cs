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
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using FlaxEngine.Visject;
using System.Diagnostics;
using System.Collections;
using FlaxEditor.Content;

#pragma warning disable CS1591
#pragma warning disable CS8632

[assembly: DisableRuntimeMarshalling]

namespace FlaxEngine
{
    #region Native structures

    [StructLayout(LayoutKind.Sequential)]
    internal struct ManagedClass
    {
        internal IntPtr typeHandle;
        internal IntPtr name;
        internal IntPtr fullname;
        internal IntPtr @namespace;
        internal uint typeAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ClassMethod
    {
        internal IntPtr name;
        internal int numParameters;
        internal IntPtr typeHandle;
        internal uint methodAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ClassField
    {
        internal IntPtr name;
        internal IntPtr fieldHandle;
        internal IntPtr fieldTypeHandle;
        internal uint fieldAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ClassProperty
    {
        internal IntPtr name;
        internal IntPtr getterHandle;
        internal IntPtr setterHandle;
        internal uint getterFlags;
        internal uint setterFlags;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ClassAttribute
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

#if false
    [StructLayout(LayoutKind.Sequential)]
    internal struct GuidNative
    {
        internal int A;
        internal short B;
        internal short C;
        internal byte D;
        internal byte E;
        internal byte F;
        internal byte G;
        internal byte H;
        internal byte I;
        internal byte J;
        internal byte K;

        internal GuidNative(Guid guid)
        {
            byte[] bytes = guid.ToByteArray();
            A = MemoryMarshal.Cast<byte, int>(bytes.AsSpan())[0];
            B = MemoryMarshal.Cast<byte, short>(bytes.AsSpan())[4];
            C = MemoryMarshal.Cast<byte, short>(bytes.AsSpan())[6];
            D = bytes[8];
            E = bytes[9];
            F = bytes[10];
            G = bytes[11];
            H = bytes[12];
            I = bytes[13];
            J = bytes[14];
            K = bytes[15];
        }

        internal static implicit operator Guid(GuidNative guid) => new Guid(guid.A, guid.B, guid.C, guid.D, guid.E, guid.F, guid.G, guid.H, guid.I, guid.J, guid.K);
        internal static explicit operator GuidNative(Guid guid) => new GuidNative(guid);
    }
#endif

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
    /// Wrapper for managed arrays that needs to be pinned.
    /// </summary>
    internal class ManagedArray
    {
        internal Array array = null;
        internal GCHandle handle;
        internal int elementSize;

        internal ManagedArray(Array arr)
        {
            handle = GCHandle.Alloc(arr, GCHandleType.Pinned);
            elementSize = Marshal.SizeOf(arr.GetType().GetElementType());
            array = arr;
        }

        ~ManagedArray()
        {
            if (array != null)
                Release();
        }

        internal void Release()
        {
            handle.Free();
            array = null;
        }

        internal IntPtr PointerToPinnedArray => Marshal.UnsafeAddrOfPinnedArrayElement(array, 0);

        internal int Length => array.Length;

        internal static ManagedArray Get<T>(ref T[] arr)
        {
            return new ManagedArray(arr);
        }

        internal static ManagedArray Get(Array arr)
        {
            ManagedArray managedArray = new ManagedArray(arr);
            return managedArray;
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
#if false
            // HACK: Pin the string and pass the address to it including the length, no marshalling required
            GCHandle handle = GCHandle.Alloc(str, GCHandleType.Pinned);
            IntPtr ptr = handle.AddrOfPinnedObject() - sizeof(int);

            pinnedStrings.TryAdd(ptr, handle);
            return ptr;
#endif

#if false
            // HACK: Return address to the allocated string structure which includes the string length.
            // We assume the string content is copied in native side before GC frees it.
            IntPtr ptr = (Unsafe.Read<IntPtr>(Unsafe.AsPointer(ref str)) + sizeof(int) * 2);
#endif
            IntPtr ptr = GCHandle.ToIntPtr(GCHandle.Alloc(str, GCHandleType.Weak));
            return ptr;
        }

        [System.Diagnostics.DebuggerStepThrough]
        internal static string ToManaged(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return null;

            return (string)GCHandle.FromIntPtr(ptr).Target;
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
    internal static class GCHandleMarshaller
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
        internal static object ConvertToManaged(IntPtr unmanaged) => ToManaged(unmanaged);
        internal static IntPtr ConvertToUnmanaged(object managed) => ToNative(managed);
        internal static object ToManaged(IntPtr managed) => managed != IntPtr.Zero ? GCHandle.FromIntPtr(managed).Target : null;
        internal static IntPtr ToNative(object managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;
        internal static void Free(IntPtr unmanaged)
        {
            if (unmanaged == IntPtr.Zero)
                return;
            GCHandle.FromIntPtr(unmanaged).Free();
        }
    }

    [CustomMarshaller(typeof(System.Type), MarshalMode.Default, typeof(SystemTypeMarshaller))]
    internal static class SystemTypeMarshaller
    {
        internal static System.Type ConvertToManaged(IntPtr unmanaged) => (System.Type)GCHandleMarshaller.ConvertToManaged(unmanaged);

        internal static IntPtr ConvertToUnmanaged(System.Type managed)
        {
            if (managed == null)
                return IntPtr.Zero;

            GCHandle handle = NativeInterop.GetOrAddTypeGCHandle(managed);
            return GCHandle.ToIntPtr(handle);
        }

        internal static void Free(IntPtr unmanaged)
        {
            // Cached handle, do not release
        }
    }

    [CustomMarshaller(typeof(Exception), MarshalMode.Default, typeof(ExceptionMarshaller))]
    internal static class ExceptionMarshaller
    {
        internal static Exception ConvertToManaged(IntPtr unmanaged) => (Exception)GCHandleMarshaller.ConvertToManaged(unmanaged);
        internal static IntPtr ConvertToUnmanaged(Exception managed) => GCHandleMarshaller.ConvertToUnmanaged(managed);
        internal static void Free(IntPtr unmanaged) => GCHandleMarshaller.Free(unmanaged);
    }

    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ManagedToUnmanagedIn, typeof(ObjectMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.UnmanagedToManagedOut, typeof(ObjectMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ElementIn, typeof(ObjectMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ManagedToUnmanagedOut, typeof(ObjectMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.UnmanagedToManagedIn, typeof(ObjectMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ElementOut, typeof(ObjectMarshaller.NativeToManaged))]
    internal static class ObjectMarshaller
    {
        public static class NativeToManaged
        {
            public static FlaxEngine.Object ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? (FlaxEngine.Object)GCHandle.FromIntPtr(unmanaged).Target : null;
        }
        public static class ManagedToNative
        {
            public static IntPtr ConvertToUnmanaged(FlaxEngine.Object managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;
        }
    }

    [CustomMarshaller(typeof(CultureInfo), MarshalMode.Default, typeof(CultureInfoMarshaller))]
    internal static class CultureInfoMarshaller
    {
        internal static CultureInfo ConvertToManaged(IntPtr unmanaged) => (CultureInfo)GCHandleMarshaller.ConvertToManaged(unmanaged);
        internal static IntPtr ConvertToUnmanaged(CultureInfo managed) => GCHandleMarshaller.ConvertToUnmanaged(managed);
        internal static void Free(IntPtr unmanaged) => GCHandleMarshaller.Free(unmanaged);
    }

    [CustomMarshaller(typeof(Array), MarshalMode.Default, typeof(SystemArrayMarshaller))]
    internal static class SystemArrayMarshaller
    {
        internal static Array ConvertToManaged(IntPtr unmanaged) => (Array)GCHandleMarshaller.ConvertToManaged(unmanaged);
        internal static IntPtr ConvertToUnmanaged(Array managed) => GCHandleMarshaller.ConvertToUnmanaged(managed);
        internal static void Free(IntPtr unmanaged) => GCHandleMarshaller.Free(unmanaged);
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
    internal static unsafe class DictionaryMarshaller<T, U>
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
            public void FromManaged(Dictionary<T, U> managed) { this.managed = managed; }
            public IntPtr ToUnmanaged() { unmanaged = DictionaryMarshaller<T, U>.ToNative(managed); return unmanaged; }
            public void FromUnmanaged(IntPtr unmanaged) { this.unmanaged = unmanaged; }
            public Dictionary<T, U> ToManaged() { managed = DictionaryMarshaller<T, U>.ToManaged(unmanaged); unmanaged = IntPtr.Zero; return managed; }
            public void Free() => DictionaryMarshaller<T, U>.Free(unmanaged);
        }

        public static Dictionary<T, U> ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? (Dictionary<T, U>)GCHandle.FromIntPtr(unmanaged).Target : null;
        public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => managed != null ? GCHandle.ToIntPtr(GCHandle.Alloc(managed)) : IntPtr.Zero;

        public static Dictionary<T, U> ToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? (Dictionary<T, U>)GCHandle.FromIntPtr(unmanaged).Target : null;
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
    internal static unsafe class ArrayMarshaller<T, TUnmanagedElement>
    where TUnmanagedElement : unmanaged
    {
        public static class NativeToManaged
        {
            internal static T[]? AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged is null)
                    return null;

                ManagedArray array = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target;
                return new T[numElements];
            }

            internal static Span<T> GetManagedValuesDestination(T[]? managed)
                => managed;

            internal static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanagedValue, int numElements)
            {
                if (unmanagedValue == null)
                    return ReadOnlySpan<TUnmanagedElement>.Empty;

                ManagedArray array = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(unmanagedValue)).Target;
                return new ReadOnlySpan<TUnmanagedElement>(array.array as TUnmanagedElement[]);
            }

            internal static void Free(TUnmanagedElement* unmanaged)
            {
                if (unmanaged == null)
                    return;

                GCHandle handle = GCHandle.FromIntPtr(new IntPtr(unmanaged));
                ((ManagedArray)handle.Target).Release();
                handle.Free();
            }

            internal static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;

                ManagedArray array = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target;
                return new Span<TUnmanagedElement>(array.array as TUnmanagedElement[]);
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

                ManagedArray array = ManagedArray.Get(new TUnmanagedElement[numElements]);
                var ptr = GCHandle.ToIntPtr(GCHandle.Alloc(array));

                return (TUnmanagedElement*)ptr;
            }

            internal static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed)
                => managed;

            internal static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;

                ManagedArray array = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target;
                return new Span<TUnmanagedElement>(array.array as TUnmanagedElement[]);
            }

            internal static void Free(TUnmanagedElement* unmanaged)
            {
                if (unmanaged == null)
                    return;

                GCHandle handle = GCHandle.FromIntPtr(new IntPtr(unmanaged));
                ((ManagedArray)handle.Target).Release();
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
                unmanagedArray = ManagedArray.Get(new TUnmanagedElement[managed.Length]);
                handle = GCHandle.Alloc(unmanagedArray);
            }

            public ReadOnlySpan<T> GetManagedValuesSource()
                => managedArray;

            public Span<TUnmanagedElement> GetUnmanagedValuesDestination()
            {
                if (unmanagedArray == null)
                    return Span<TUnmanagedElement>.Empty;

                return new Span<TUnmanagedElement>(unmanagedArray.array as TUnmanagedElement[]);
            }

            public TUnmanagedElement* ToUnmanaged()
            {
                return (TUnmanagedElement*)GCHandle.ToIntPtr(handle);
            }

            public void FromUnmanaged(TUnmanagedElement* value)
            {
                ManagedArray arr = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(value)).Target;
                managedArray = new T[arr.Length];
            }

            public ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(int numElements)
            {
                if (unmanagedArray == null)
                    return ReadOnlySpan<TUnmanagedElement>.Empty;

                return new ReadOnlySpan<TUnmanagedElement>(unmanagedArray.array as TUnmanagedElement[]);
            }

            public Span<T> GetManagedValuesDestination(int numElements)
                => managedArray;

            public T[] ToManaged()
                => managedArray;

            public void Free()
            {
                unmanagedArray.Release();
                handle.Free();
            }
        }

        internal static TUnmanagedElement* AllocateContainerForUnmanagedElements(T[]? managed, out int numElements)
        {
            if (managed is null)
            {
                numElements = 0;
                return null;
            }

            numElements = managed.Length;

            ManagedArray array = ManagedArray.Get(new TUnmanagedElement[numElements]);
            var ptr = GCHandle.ToIntPtr(GCHandle.Alloc(array));

            return (TUnmanagedElement*)ptr;
        }

        internal static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed)
            => managed;

        internal static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return Span<TUnmanagedElement>.Empty;

            ManagedArray array = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target;
            return new Span<TUnmanagedElement>(array.array as TUnmanagedElement[]);
        }

        internal static T[]? AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged is null)
                return null;

            ManagedArray array = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(unmanaged)).Target;
            return new T[numElements];
        }

        internal static Span<T> GetManagedValuesDestination(T[]? managed)
            => managed;

        internal static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanagedValue, int numElements)
        {
            if (unmanagedValue == null)
                return ReadOnlySpan<TUnmanagedElement>.Empty;

            ManagedArray array = (ManagedArray)GCHandle.FromIntPtr(new IntPtr(unmanagedValue)).Target;
            return new ReadOnlySpan<TUnmanagedElement>(array.array as TUnmanagedElement[]);
        }

        internal static void Free(TUnmanagedElement* unmanaged)
        {
            if (unmanaged == null)
                return;

            GCHandle handle = GCHandle.FromIntPtr(new IntPtr(unmanaged));
            ((ManagedArray)handle.Target).Release();
            handle.Free();
        }

        internal static ref T GetPinnableReference(T[]? array)
        {
            if (array is null)
                return ref Unsafe.NullRef<T>();

            ManagedArray managedArray = ManagedArray.Get(array);
            var ptr = GCHandle.ToIntPtr(GCHandle.Alloc(managedArray));
            return ref Unsafe.AsRef<T>(ptr.ToPointer());
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
    internal static class StringMarshaller
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
#if false
                // HACK: Pin the string and pass the address to it including the length, no marshalling required
                GCHandle handle = GCHandle.Alloc(managed, GCHandleType.Pinned);
                IntPtr ptr = handle.AddrOfPinnedObject() - sizeof(int);

                pinnedStrings.TryAdd(ptr, handle);
                return ptr;
#endif

#if false
                // HACK: Return address to the allocated string structure which includes the string length.
                // We assume the string content is copied in native side before GC frees it.
                IntPtr ptr = (Unsafe.Read<IntPtr>(Unsafe.AsPointer(ref managed)) + sizeof(int) * 2);
#endif
                IntPtr ptr = GCHandle.ToIntPtr(GCHandle.Alloc(managed, GCHandleType.Pinned));
                return ptr;
            }
            public static void Free(IntPtr unmanaged) => ManagedString.Free(unmanaged);
        }
        public struct Bidirectional
        {
            string managed;
            IntPtr unmanaged;
            public void FromManaged(string managed) { this.managed = managed; }
            public IntPtr ToUnmanaged() { unmanaged = ManagedString.ToNative(managed); return unmanaged; }
            public void FromUnmanaged(IntPtr unmanaged) { this.unmanaged = unmanaged; }
            public string ToManaged() { managed = ManagedString.ToManaged(unmanaged); unmanaged = IntPtr.Zero; return managed; }
            public void Free() => ManagedString.Free(unmanaged);
        }
        internal static string ConvertToManaged(IntPtr unmanaged) => ManagedString.ToManaged(unmanaged);
        internal static IntPtr ConvertToUnmanaged(string managed) => ManagedString.ToNative(managed);
        internal static void Free(IntPtr unmanaged) => ManagedString.Free(unmanaged);

        internal static string ToManaged(IntPtr unmanaged) => ManagedString.ToManaged(unmanaged);
        internal static IntPtr ToNative(string managed) => ManagedString.ToNative(managed);
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

        private static Dictionary<Type, FieldInfo[]> marshallableStructCache = new Dictionary<Type, FieldInfo[]>();

        private static IntPtr boolTruePtr = GCHandle.ToIntPtr(GCHandle.Alloc((int)1, GCHandleType.Pinned));
        private static IntPtr boolFalsePtr = GCHandle.ToIntPtr(GCHandle.Alloc((int)0, GCHandleType.Pinned));

        private static List<GCHandle> methodHandles = new();
        private static List<GCHandle> methodHandlesCollectible = new();
        private static Dictionary<IntPtr, Delegate> cachedDelegates = new Dictionary<IntPtr, Delegate>();
        private static Dictionary<IntPtr, Delegate> cachedDelegatesCollectible = new Dictionary<IntPtr, Delegate>();
        private static Dictionary<string, GCHandle> typeHandleCache = new Dictionary<string, GCHandle>();
        private static Dictionary<string, GCHandle> typeHandleCacheCollectible = new Dictionary<string, GCHandle>();
        private static List<GCHandle> fieldHandleCache = new();
        private static List<GCHandle> fieldHandleCacheCollectible = new();
        private static Dictionary<object, GCHandle> classAttributesCacheCollectible = new();
        private static Dictionary<Assembly, GCHandle> assemblyHandles = new Dictionary<Assembly, GCHandle>();

        private static string hostExecutable;
        private static IntPtr hostExecutableHandle = IntPtr.Zero;
        private static AssemblyLoadContext scriptingAssemblyLoadContext;

        [System.Diagnostics.DebuggerStepThrough]
        private static IntPtr InternalDllResolver(string libraryName, Assembly assembly, DllImportSearchPath? dllImportSearchPath)
        {
            if (libraryName == "FlaxEngine")
            {
                if (hostExecutableHandle == IntPtr.Zero)
                    hostExecutableHandle = NativeLibrary.Load(hostExecutable, assembly, dllImportSearchPath);
                return hostExecutableHandle;
            }
            return IntPtr.Zero;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void Init(IntPtr hostExecutableName)
        {
            hostExecutable = Marshal.PtrToStringUni(hostExecutableName);
            NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), InternalDllResolver);

            // TODO: benchmark collectible setting performance, maybe enable it only in editor builds?
            scriptingAssemblyLoadContext = new AssemblyLoadContext(null, true);

            DelegateHelpers.Init();
        }

        [UnmanagedCallersOnly]
        internal static unsafe void Exit()
        {
        }

        internal static T[] GCHandleArrayToManagedArray<T>(ManagedArray array)
        {
            IntPtr[] ptrArray = (IntPtr[])array.array;
            T[] managedArray = new T[array.Length];
            for (int i = 0; i < managedArray.Length; i++)
            {
                IntPtr ptr = ptrArray[i];
                managedArray[i] = ptr != IntPtr.Zero ? (T)GCHandle.FromIntPtr(ptr).Target : default(T);
            }
            return managedArray;
        }

        internal static IntPtr[] ManagedArrayToGCHandleArray(Array array)
        {
            IntPtr[] pointerArray = new IntPtr[array.Length];
            for (int i = 0; i < pointerArray.Length; i++)
            {
                var obj = array.GetValue(i);
                pointerArray.SetValue(obj != null ? GCHandle.ToIntPtr(GCHandle.Alloc(obj)) : IntPtr.Zero, i);
            }
            return pointerArray;
        }

        internal static T[] NativeArrayToManagedArray<T, U>(U[] nativeArray, Func<U, T> toManagedFunc)
        {
            T[] managedArray = new T[nativeArray.Length];
            if (nativeArray.Length > 0)
            {
                Assert.IsTrue(toManagedFunc != null);
                for (int i = 0; i < nativeArray.Length; i++)
                    managedArray[i] = toManagedFunc(nativeArray[i]);
            }

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
            if (className.Contains("+"))
            {
                parentClassName = className.Substring(0, className.LastIndexOf('+') + 1);
                className = className.Substring(parentClassName.Length);
            }
            string marshallerName = className + "Marshaller";
            string internalAssemblyQualifiedName = $"{@namespace}.{parentClassName}{marshallerName}+{className}Internal,{String.Join(',', splits.Skip(1))}";
            return FindType(internalAssemblyQualifiedName);
        }

        private static IntPtr EnsureAlignment(IntPtr ptr, int alignment)
        {
            if (ptr % alignment != 0)
                ptr = IntPtr.Add(ptr, alignment - (int)(ptr % alignment));
            return ptr;
        }

        internal static void MarshalFromObject(object obj, Type objectType, ref IntPtr targetPtr)
        {
            int readOffset = 0;
            if (objectType == typeof(IntPtr))
            {
                targetPtr = EnsureAlignment(targetPtr, IntPtr.Size);
                Marshal.WriteIntPtr(targetPtr, (IntPtr)obj);
                readOffset = IntPtr.Size;
            }
            else if (objectType == typeof(byte))
            {
                targetPtr = EnsureAlignment(targetPtr, sizeof(byte));
                Marshal.WriteByte(targetPtr, (byte)obj);
                readOffset = sizeof(byte);
            }
            else if (objectType == typeof(bool))
            {
                targetPtr = EnsureAlignment(targetPtr, sizeof(byte));
                Marshal.WriteByte(targetPtr, ((bool)obj) ? (byte)1 : (byte)0);
                readOffset = sizeof(byte);
            }
            else if (objectType.IsEnum)
            {
                MarshalFromObject(obj, objectType.GetEnumUnderlyingType(), ref targetPtr);
            }
            else if (objectType == typeof(int) || objectType.Name == "Int32&")
            {
                targetPtr = EnsureAlignment(targetPtr, sizeof(int));
                Marshal.WriteInt32(targetPtr, (int)obj);
                readOffset = sizeof(int);
            }
            else if (objectType == typeof(long))
            {
                targetPtr = EnsureAlignment(targetPtr, sizeof(long));
                Marshal.WriteInt64(targetPtr, (long)obj);
                readOffset = sizeof(long);
            }
            else if (objectType == typeof(short))
            {
                targetPtr = EnsureAlignment(targetPtr, sizeof(short));
                Marshal.WriteInt16(targetPtr, (short)obj);
                readOffset = sizeof(short);
            }
            else if (objectType == typeof(string))
            {
                targetPtr = EnsureAlignment(targetPtr, IntPtr.Size);
                IntPtr strPtr = ManagedString.ToNative(obj as string);
                Marshal.WriteIntPtr(targetPtr, strPtr);
                readOffset = IntPtr.Size;
            }
            else if (objectType.IsByRef)
                throw new NotImplementedException();
            else if (objectType.IsClass)
            {
                if (objectType.IsPointer)
                {
                    targetPtr = EnsureAlignment(targetPtr, IntPtr.Size);
                    var unboxed = Pointer.Unbox(obj);
                    if (unboxed == null)
                        Marshal.WriteIntPtr(targetPtr, IntPtr.Zero);
                    else if (obj is FlaxEngine.Object fobj)
                        Marshal.WriteIntPtr(targetPtr, fobj.__unmanagedPtr);
                    else
                        Marshal.WriteIntPtr(targetPtr, GCHandle.ToIntPtr(GCHandle.Alloc(obj, GCHandleType.Weak)));
                    readOffset = IntPtr.Size;
                }
                else
                {
                    targetPtr = EnsureAlignment(targetPtr, IntPtr.Size);
                    Marshal.WriteIntPtr(targetPtr, obj != null ? GCHandle.ToIntPtr(GCHandle.Alloc(obj, GCHandleType.Weak)) : IntPtr.Zero);
                    readOffset = IntPtr.Size;
                }
            }
            else if (objectType.IsValueType)
            {
                if (GetMarshallableStructFields(objectType, out FieldInfo[] fields))
                {
                    IntPtr origPtr = targetPtr;

                    foreach (var field in fields)
                        MarshalFromObject(field.GetValue(obj), field.FieldType, ref targetPtr);

                    Assert.IsTrue((targetPtr-origPtr) == Marshal.SizeOf(objectType));
                }
                else
                {
                    Marshal.StructureToPtr(obj, targetPtr, false);
                    readOffset = Marshal.SizeOf(objectType);
                }
            }
            else
                throw new NotImplementedException(objectType.FullName);

            targetPtr = IntPtr.Add(targetPtr, readOffset);
        }

        private static bool GetMarshallableStructFields(Type type, out FieldInfo[] fields)
        {
            if (marshallableStructCache.TryGetValue(type, out fields))
                return fields != null;

            fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            if (!fields.Any(x => x.FieldType.IsClass || x.FieldType.Name == "Boolean"))
                fields = null;

            marshallableStructCache.Add(type, fields);
            return fields != null;
        }

        internal static object MarshalToObject(Type type, IntPtr ptr, out int readSize)
        {
            readSize = 0;
            if (type.Name == "IntPtr&" || type.Name == "Byte*" || type.Name == "IntPtr")
            {
                readSize += IntPtr.Size;
                return ptr;
            }
            else if (type.IsByRef)
            {
                return MarshalToObject(type.GetElementType(), ptr, out readSize);
            }
            else if (type == typeof(string))
            {
                readSize += IntPtr.Size;
                return ManagedString.ToManaged(ptr);
            }
            else if (type == typeof(bool))
            {
                readSize += sizeof(byte);
                return Marshal.ReadByte(ptr) != 0;
            }
            else if (type.IsEnum)
            {
                return Enum.ToObject(type, MarshalToObject(type.GetEnumUnderlyingType(), ptr, out readSize));
            }
            else if (type.IsArray)
            {
                readSize += IntPtr.Size;
                if (ptr == IntPtr.Zero)
                    return null;

                Type elementType = type.GetElementType();
                ManagedArray arr = (ManagedArray)GCHandle.FromIntPtr(ptr).Target;
                if (arr.array.GetType().GetElementType() == elementType)
                    return arr.array;

                IntPtr[] ptrs = (IntPtr[])arr.array;
                Array marshalledArray = Array.CreateInstance(elementType, ptrs.Length);
                for (int i = 0; i < marshalledArray.Length; i++)
                    marshalledArray.SetValue(MarshalToObject(elementType, ptrs[i], out _), i);
                return marshalledArray;
            }
            else if (type.IsClass)
            {
                readSize += IntPtr.Size;
                if (ptr == IntPtr.Zero)
                    return null;

                var obj = GCHandle.FromIntPtr(ptr).Target;
                return obj;
            }
            else if (type.IsValueType)
            {
                if (GetMarshallableStructFields(type, out FieldInfo[] fields))
                {
                    object target = RuntimeHelpers.GetUninitializedObject(type);
                    IntPtr fieldPtr = ptr;

                    foreach (var field in fields)
                    {
                        object fieldValue;
                        int size;

                        if (field.FieldType.IsClass || field.FieldType == typeof(IntPtr))
                            fieldValue = MarshalToObject(field.FieldType, Marshal.ReadIntPtr(fieldPtr), out size);
                        else
                        {
                            int fieldSize;
                            Type fieldType = field.FieldType;
                            if (field.FieldType.IsEnum)
                                fieldType = field.FieldType.GetEnumUnderlyingType();
                            else if (field.FieldType == typeof(bool))
                                fieldType = typeof(byte);

                            if (fieldType.IsValueType && !fieldType.IsEnum && !fieldType.IsPrimitive) // Is it a structure?
                                fieldSize = 1;
                            else if (fieldType.IsClass || fieldType.IsPointer)
                                fieldSize = IntPtr.Size;
                            else
                                fieldSize = Marshal.SizeOf(fieldType);
                            fieldPtr = EnsureAlignment(fieldPtr, fieldSize);

                            fieldValue = MarshalToObject(field.FieldType, fieldPtr, out size);
                        }

                        field.SetValue(target, fieldValue);

                        fieldPtr = IntPtr.Add(fieldPtr, size);
                    }

                    var totalReadSize = fieldPtr.ToInt64() - ptr.ToInt64();
                    Assert.IsTrue(totalReadSize == Marshal.SizeOf(type));
                    readSize += (int)totalReadSize;
                    return target;
                }
            }
            else if (type.Name == "IDictionary")
            {
                readSize += IntPtr.Size;
                if (ptr == IntPtr.Zero)
                    return null;

                return GCHandle.FromIntPtr(ptr).Target;
            }

            readSize += Marshal.SizeOf(type);
            return Marshal.PtrToStructure(ptr, type);
        }

        internal static GCHandle GetMethodGCHandle(MethodInfo method)
        {
            Type[] parameterTypes = method.GetParameters().Select(x => x.ParameterType).ToArray();

            GCHandle delegTupleHandle = GCHandle.Alloc(new Tuple<Type[], MethodInfo>(parameterTypes, method));

            if (parameterTypes.Any(x => x.IsCollectible) || method.IsCollectible)
                methodHandlesCollectible.Add(delegTupleHandle);
            else
                methodHandles.Add(delegTupleHandle);

            return delegTupleHandle;
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
        internal static unsafe void GetManagedClasses(IntPtr assemblyHandle, ManagedClass** managedClasses, int* managedClassCount)
        {
            Assembly assembly = (Assembly)GCHandle.FromIntPtr(assemblyHandle).Target;
            var assemblyTypes = GetAssemblyTypes(assembly);

            ManagedClass* arr = (ManagedClass*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<ManagedClass>() * assemblyTypes.Length).ToPointer();

            for (int i = 0; i < assemblyTypes.Length; i++)
            {
                var type = assemblyTypes[i];
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<ManagedClass>() * i);
                bool isStatic = type.IsAbstract && type.IsSealed;
                bool isInterface = type.IsInterface;
                bool isAbstract = type.IsAbstract;

                GCHandle typeHandle = GetOrAddTypeGCHandle(type);

                ManagedClass managedClass = new ManagedClass()
                {
                    typeHandle = GCHandle.ToIntPtr(typeHandle),
                    name = Marshal.StringToCoTaskMemAnsi(type.Name),
                    fullname = Marshal.StringToCoTaskMemAnsi(type.FullName),
                    @namespace = Marshal.StringToCoTaskMemAnsi(type.Namespace ?? ""),
                    typeAttributes = (uint)type.Attributes,
                };
                Marshal.StructureToPtr(managedClass, ptr, false);
            }

            *managedClasses = arr;
            *managedClassCount = assemblyTypes.Length;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetManagedClassFromType(IntPtr typeHandle, ManagedClass* managedClass, IntPtr* assemblyHandle)
        {
            var type = (Type)GCHandle.FromIntPtr(typeHandle).Target;

            GCHandle classTypeHandle = GetOrAddTypeGCHandle(type);

            *managedClass = new ManagedClass()
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
        internal static void GetClassMethods(IntPtr typeHandle, ClassMethod** classMethods, int* classMethodsCount)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;

            List<MethodInfo> methods = new List<MethodInfo>();
            var staticMethods = type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            var instanceMethods = type.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            foreach (MethodInfo methodInfo in staticMethods)
                methods.Add(methodInfo);
            foreach (MethodInfo methodInfo in instanceMethods)
                methods.Add(methodInfo);

            ClassMethod* arr = (ClassMethod*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<ClassMethod>() * methods.Count).ToPointer();
            for (int i = 0; i < methods.Count; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<ClassMethod>() * i);
                ClassMethod classMethod = new ClassMethod()
                {
                    name = Marshal.StringToCoTaskMemAnsi(methods[i].Name),
                    numParameters = methods[i].GetParameters().Length,
                    methodAttributes = (uint)methods[i].Attributes,
                };
                classMethod.typeHandle = GCHandle.ToIntPtr(GetMethodGCHandle(methods[i]));
                Marshal.StructureToPtr(classMethod, ptr, false);
            }
            *classMethods = arr;
            *classMethodsCount = methods.Count;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassFields(IntPtr typeHandle, ClassField** classFields, int* classFieldsCount)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            ClassField* arr = (ClassField*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<ClassField>() * fields.Length).ToPointer();
            for (int i = 0; i < fields.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<ClassField>() * i);

                GCHandle fieldHandle = GCHandle.Alloc(fields[i]);
                if (type.IsCollectible)
                    fieldHandleCacheCollectible.Add(fieldHandle);
                else
                    fieldHandleCache.Add(fieldHandle);

                ClassField classField = new ClassField()
                {
                    name = Marshal.StringToCoTaskMemAnsi(fields[i].Name),
                    fieldHandle = GCHandle.ToIntPtr(fieldHandle),
                    fieldTypeHandle = GCHandle.ToIntPtr(GetOrAddTypeGCHandle(fields[i].FieldType)),
                    fieldAttributes = (uint)fields[i].Attributes,
                };
                Marshal.StructureToPtr(classField, ptr, false);
            }
            *classFields = arr;
            *classFieldsCount = fields.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassProperties(IntPtr typeHandle, ClassProperty** classProperties, int* classPropertiesCount)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            var properties = type.GetProperties(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            ClassProperty* arr = (ClassProperty*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<ClassProperty>() * properties.Length).ToPointer();
            for (int i = 0; i < properties.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<ClassProperty>() * i);

                var getterMethod = properties[i].GetGetMethod(true);
                var setterMethod = properties[i].GetSetMethod(true);

                ClassProperty classProperty = new ClassProperty()
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
                Marshal.StructureToPtr(classProperty, ptr, false);
            }
            *classProperties = arr;
            *classPropertiesCount = properties.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassAttributes(IntPtr typeHandle, ClassAttribute** classAttributes, int* classAttributesCount)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            object[] attributeValues = type.GetCustomAttributes(false);
            Type[] attributeTypes = type.GetCustomAttributes(false).Select(x => x.GetType()).ToArray();

            ClassAttribute* arr = (ClassAttribute*)Marshal.AllocCoTaskMem(Unsafe.SizeOf<ClassAttribute>() * attributeTypes.Length).ToPointer();
            for (int i = 0; i < attributeTypes.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<ClassAttribute>() * i);

                if (!classAttributesCacheCollectible.TryGetValue(attributeValues[i], out GCHandle attributeHandle))
                {
                    attributeHandle = GCHandle.Alloc(attributeValues[i]);
                    classAttributesCacheCollectible.Add(attributeValues[i], attributeHandle);
                }
                GCHandle attributeTypeHandle = GetOrAddTypeGCHandle(attributeTypes[i]);

                ClassAttribute classAttribute = new ClassAttribute()
                {
                    name = Marshal.StringToCoTaskMemAnsi(attributeTypes[i].Name),
                    attributeTypeHandle = GCHandle.ToIntPtr(attributeTypeHandle),
                    attributeHandle = GCHandle.ToIntPtr(attributeHandle),
                };
                Marshal.StructureToPtr(classAttribute, ptr, false);
            }
            *classAttributes = arr;
            *classAttributesCount = attributeTypes.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassInterfaces(IntPtr typeHandle, IntPtr* classInterfaces, int* classInterfacesCount)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;

            var interfaces = type.GetInterfaces();

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
            (Type[] parameterTypes, MethodInfo methodInfo) = (Tuple<Type[], MethodInfo>)GCHandle.FromIntPtr(methodHandle).Target;
            Type returnType = methodInfo.ReturnType;

            return GCHandle.ToIntPtr(GetTypeGCHandle(returnType));
        }

        [UnmanagedCallersOnly]
        internal static void GetMethodParameterTypes(IntPtr methodHandle, IntPtr* typeHandles)
        {
            (Type[] parameterTypes, MethodInfo methodInfo) = (Tuple<Type[], MethodInfo>)GCHandle.FromIntPtr(methodHandle).Target;
            Type returnType = methodInfo.ReturnType;

            IntPtr arr = Marshal.AllocCoTaskMem(IntPtr.Size * parameterTypes.Length);
            for (int i = 0; i < parameterTypes.Length; i++)
            {
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), IntPtr.Size * i);

                GCHandle typeHandle = GetOrAddTypeGCHandle(parameterTypes[i]);

                Marshal.WriteIntPtr(ptr, GCHandle.ToIntPtr(typeHandle));
            }
            *typeHandles = arr;
        }

        /// <summary>
        /// Returns pointer to the string's internal structure, containing the buffer and length of the string.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static IntPtr GetStringPointer(IntPtr stringHandle)
        {
            string str = (string)GCHandle.FromIntPtr(stringHandle).Target;
            IntPtr ptr = (Unsafe.Read<IntPtr>(Unsafe.AsPointer(ref str)) + sizeof(int) * 2);
            return ptr;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewObject(IntPtr typeHandle)
        {
            Type classType = (Type)GCHandle.FromIntPtr(typeHandle).Target;

            if (classType == typeof(Script))
            {
                // FIXME: Script is an abstract type which can not be instantiated
                classType = typeof(VisjectScript);
            }

            object obj = RuntimeHelpers.GetUninitializedObject(classType);

            IntPtr handle = GCHandle.ToIntPtr(GCHandle.Alloc(obj));
            return handle;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewArray(IntPtr typeHandle, long size)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            Type elementType;
            if (IsBlittable(type))
                elementType = type;
            else
                elementType = GetInternalType(type) ?? typeof(IntPtr);
            Array arr = Array.CreateInstance(elementType, size);
            ManagedArray managedArray = new ManagedArray(arr);
            return GCHandle.ToIntPtr(GCHandle.Alloc(managedArray/*, GCHandleType.Weak*/));
        }

        [UnmanagedCallersOnly]
        internal static unsafe IntPtr GetArrayPointerToElement(IntPtr arrayHandle, int size, int index)
        {
            object obj = GCHandle.FromIntPtr(arrayHandle).Target;
            if (obj is ManagedArray managedArray)
            {
                if (managedArray.Length == 0)
                    return IntPtr.Zero;
                Assert.IsTrue(index >= 0 && index < managedArray.Length);
                IntPtr ptr = IntPtr.Add(managedArray.PointerToPinnedArray, size * index);
                return ptr;
            }
            else
            {
                Array array = (Array)obj;
                if (array.Length == 0)
                    return IntPtr.Zero;
                Assert.IsTrue(index >= 0 && index < array.Length);
                IntPtr ptr = Marshal.UnsafeAddrOfPinnedArrayElement(array, index);
                return ptr;
            }
        }

        [UnmanagedCallersOnly]
        internal static int GetArrayLength(IntPtr handlePtr)
        {
            object obj = GCHandle.FromIntPtr(handlePtr).Target;
            if (obj is ManagedArray managedArray)
                return managedArray.Length;
            else
                return ((Array)obj).Length;
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
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;

            object value = MarshalToObject(type, valuePtr, out _);
            return GCHandle.ToIntPtr(GCHandle.Alloc(value, GCHandleType.Weak));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetObjectType(IntPtr handle)
        {
            var obj = GCHandle.FromIntPtr(handle).Target;
            Type classType = obj.GetType();
            return GCHandle.ToIntPtr(GetOrAddTypeGCHandle(classType));
        }

        internal unsafe class StructUnboxer<T> where T : struct
        {
            public StructUnboxer(object obj, IntPtr ptr)
            {
                IntPtr* pin = (IntPtr*)ptr;
                *pin = new IntPtr(Unsafe.AsPointer(ref Unsafe.Unbox<T>(obj)));
            }
        }

        [UnmanagedCallersOnly]
        internal unsafe static IntPtr UnboxValue(IntPtr handlePtr)
        {
            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
            object obj = handle.Target;
            Type type = obj.GetType();

            if (!type.IsValueType)
                return handlePtr;

            // HACK: Get the address of a non-pinned value
            IntPtr ptr = IntPtr.Zero;
            IntPtr* addr = &ptr;
            Activator.CreateInstance(typeof(StructUnboxer<>).MakeGenericType(type), obj, new IntPtr(addr));

            return ptr;
        }

        [UnmanagedCallersOnly]
        internal static void RaiseException(IntPtr exceptionHandle)
        {
            var exception = (Exception)GCHandle.FromIntPtr(exceptionHandle).Target;
            throw exception;
        }

        [UnmanagedCallersOnly]
        internal static void ObjectInit(IntPtr handle)
        {
            object obj = GCHandle.FromIntPtr(handle).Target;
            Type classType = obj.GetType();

            var ctors = classType.GetMember(".ctor", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
            var ctor = ctors[0] as ConstructorInfo;
            ctor.Invoke(obj, null);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr InvokeMethod(IntPtr instance, IntPtr methodHandle, IntPtr paramData, IntPtr exceptionPtr)
        {
            (Type[] parameterTypes, MethodInfo methodInfo) = (Tuple<Type[], MethodInfo>)GCHandle.FromIntPtr(methodHandle).Target;
            int numParams = parameterTypes.Length;

            var intPtrParams = stackalloc IntPtr[numParams];
            var objParams = new object[numParams];

            IntPtr curptr = paramData;
            for (int i = 0; i < numParams; i++)
            {
                intPtrParams[i] = Marshal.ReadIntPtr(curptr);
                curptr = IntPtr.Add(curptr, sizeof(IntPtr));
                objParams[i] = MarshalToObject(parameterTypes[i], intPtrParams[i], out _);
            }

            object returnObject;
            try
            {
                object inst = instance != IntPtr.Zero ? GCHandle.FromIntPtr(instance).Target : null;
                returnObject = methodInfo.Invoke(inst, objParams);
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
                Type parameterType = parameterTypes[i];
                if (parameterType.IsByRef)
                {
                    IntPtr paramPtr = intPtrParams[i];
                    MarshalFromObject(objParams[i], parameterType.GetElementType(), ref paramPtr);
                }
            }

            if (returnObject is not null)
            {
                if (returnObject is string returnStr)
                    return ManagedString.ToNative(returnStr);
                else if (returnObject is IntPtr returnPtr)
                    return returnPtr;
                else if (returnObject is bool boolValue)
                    return boolValue ? boolTruePtr : boolFalsePtr;
                else if (returnObject is Type typeValue)
                    return GCHandle.ToIntPtr(GetOrAddTypeGCHandle(typeValue));
                else if (returnObject is object[] objectArray)
                    return GCHandle.ToIntPtr(GCHandle.Alloc(ManagedArray.Get(ManagedArrayToGCHandleArray(objectArray)), GCHandleType.Weak));
                else
                    return GCHandle.ToIntPtr(GCHandle.Alloc(returnObject, GCHandleType.Weak));
            }
            return IntPtr.Zero;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetMethodUnmanagedFunctionPointer(IntPtr methodHandle)
        {
            (Type[] parameterTypes, MethodInfo methodInfo) = (Tuple<Type[], MethodInfo>)GCHandle.FromIntPtr(methodHandle).Target;
            int numParams = parameterTypes.Length;
            parameterTypes = parameterTypes.Append(methodInfo.ReturnType).ToArray();

            // Wrap the method call, this is needed to get the object instance from GCHandle and to pass the exception back to native side
            var invokeThunk = typeof(ThunkContext).GetMethod("InvokeThunk");
            parameterTypes = invokeThunk.GetParameters().Select(x => x.ParameterType).Append(invokeThunk.ReturnType).ToArray();
            Type delegateType = DelegateHelpers.NewDelegateType(parameterTypes);

            var context = new ThunkContext();
            context.methodInfo = methodInfo;
            context.numParams = numParams;
            context.objParams = new object[numParams];
            context.parameterTypes = methodInfo.GetParameters().Select(x => x.ParameterType).ToArray();

            Delegate methodDelegate = Delegate.CreateDelegate(delegateType, context, invokeThunk);

            IntPtr functionPtr = Marshal.GetFunctionPointerForDelegate(methodDelegate);

            // Keep a reference to the delegate to prevent it from being garbage collected
            var delegates = cachedDelegates;
            if (parameterTypes.Any(x => scriptingAssemblyLoadContext.Assemblies.Contains(x.Module.Assembly)))
                delegates = cachedDelegatesCollectible;
            lock (delegates)
            {
                delegates[functionPtr] = methodDelegate;
            }

            return functionPtr;
        }

        [UnmanagedCallersOnly]
        internal static void FieldSetValue(IntPtr handle, IntPtr fieldHandle, IntPtr value)
        {
            var obj = GCHandle.FromIntPtr(handle).Target;
            FieldInfo fieldInfo = (FieldInfo)GCHandle.FromIntPtr(fieldHandle).Target;

            var val = Marshal.PtrToStructure(value, fieldInfo.FieldType);
            fieldInfo.SetValue(obj, val);
        }

        [UnmanagedCallersOnly]
        internal static void FieldGetValue(IntPtr handle, IntPtr fieldHandle, IntPtr value_)
        {
            var obj = GCHandle.FromIntPtr(handle).Target;
            FieldInfo fieldInfo = (FieldInfo)GCHandle.FromIntPtr(fieldHandle).Target;
            object fieldValue = fieldInfo.GetValue(obj);

            IntPtr value;
            if (fieldValue is Type type)
                value = GCHandle.ToIntPtr(GetOrAddTypeGCHandle(type));
            else if (fieldInfo.FieldType == typeof(IntPtr))
                value = (IntPtr)fieldValue;
            else
                value = GCHandle.ToIntPtr(GCHandle.Alloc(fieldValue, GCHandleType.Weak));
            Marshal.WriteIntPtr(value_, value);
        }

        [UnmanagedCallersOnly]
        internal static void SetArrayValueReference(IntPtr arrayHandle, IntPtr elementPtr, IntPtr valueHandle)
        {
            ManagedArray arr = (ManagedArray)GCHandle.FromIntPtr(arrayHandle).Target;
            int index = (int)(elementPtr.ToInt64() - arr.PointerToPinnedArray.ToInt64()) / arr.elementSize;
            arr.array.SetValue(valueHandle, index);
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
            Assembly assembly = (Assembly)GCHandle.FromIntPtr(assemblyHandle).Target;
            GCHandle.FromIntPtr(assemblyHandle).Free();
            assemblyHandles.Remove(assembly);

            AssemblyLocations.Remove(assembly.FullName);

            // Clear all caches which might hold references to closing assembly
            cachedDelegatesCollectible.Clear();
            typeCache.Clear();
            marshallableStructCache.Clear();

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
            Type originalType = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            Type type = GetInternalType(originalType) ?? originalType;

            if (type == typeof(Version))
                type = typeof(VersionNative);

            int size = Marshal.SizeOf(type);
            *align = (uint)size; // Is this correct?
            return size;
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsSubclassOf(IntPtr typeHandle, IntPtr othertypeHandle, byte checkInterfaces)
        {
            if (typeHandle == othertypeHandle)
                return 1;

            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            Type otherType = (Type)GCHandle.FromIntPtr(othertypeHandle).Target;

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
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            return (byte)(type.IsValueType ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsEnum(IntPtr typeHandle)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            return (byte)(type.IsEnum ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetClassParent(IntPtr typeHandle)
        {
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;
            return GCHandle.ToIntPtr(GetTypeGCHandle(type.BaseType));
        }

        [UnmanagedCallersOnly]
        internal static byte GetMethodParameterIsOut(IntPtr methodHandle, int parameterNum)
        {
            (Type[] parameterTypes, MethodInfo methodInfo) = (Tuple<Type[], MethodInfo>)GCHandle.FromIntPtr(methodHandle).Target;
            ParameterInfo parameterInfo = methodInfo.GetParameters()[parameterNum];
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
            object obj = GCHandle.FromIntPtr(ptr).Target;
            GCHandle handle = GCHandle.Alloc(obj, pinned != 0 ? GCHandleType.Pinned : GCHandleType.Normal);
            return GCHandle.ToIntPtr(handle);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewGCHandleWeakref(IntPtr ptr, byte track_resurrection)
        {
            object obj = GCHandle.FromIntPtr(ptr).Target;
            GCHandle handle = GCHandle.Alloc(obj, track_resurrection != 0 ? GCHandleType.WeakTrackResurrection : GCHandleType.Weak);
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
            Type type = (Type)GCHandle.FromIntPtr(typeHandle).Target;

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

            /*var assemblyPath = Utils.GetAssemblyLocation(assembly);
            List<TypeInfo> types = new List<TypeInfo>();
            using (var sr = new StreamReader(assemblyPath))
            {
	            using (var portableExecutableReader = new PEReader(sr.BaseStream))
	            {   
		            var metadataReader = portableExecutableReader.GetMetadataReader();

		            foreach (var typeDefHandle in metadataReader.TypeDefinitions)
		            {
			            var typeDef = metadataReader.GetTypeDefinition(typeDefHandle);

			            if (string.IsNullOrEmpty(metadataReader.GetString(typeDef.Namespace)))
				            continue; //if it's namespace is blank, it's not a user-defined type

			            if (typeDef.Attributes.HasFlag(TypeAttributes.Abstract) || !typeDef.Attributes.HasFlag(TypeAttributes.Public))
				            continue; //Not a internal concrete type

                        types.Add(typeDef);
		            }
	            }
            }*/

            // We need private types of this assembly too, DefinedTypes contains a lot of types from other assemblies...
            var types = referencedTypes.Any() ? assembly.DefinedTypes.Where(x => !referencedTypes.Contains(x.FullName)).ToArray() : assembly.DefinedTypes.ToArray();

            Assert.IsTrue(AppDomain.CurrentDomain.GetAssemblies().Where(x => x.GetName().Name == "FlaxEngine.CSharp").Count() == 1);

            return types;
        }

        internal static GCHandle GetOrAddTypeGCHandle(Type type)
        {
            GCHandle handle;
            if (typeHandleCache.TryGetValue(type.AssemblyQualifiedName, out handle))
                return handle;

            if (typeHandleCacheCollectible.TryGetValue(type.AssemblyQualifiedName, out handle))
                return handle;

            handle = GCHandle.Alloc(type);
            if (type.IsCollectible) // check if generic parameters are also collectible?
                typeHandleCacheCollectible.Add(type.AssemblyQualifiedName, handle);
            else
                typeHandleCache.Add(type.AssemblyQualifiedName, handle);

            return handle;
        }

        internal static GCHandle GetTypeGCHandle(Type type)
        {
            GCHandle handle;
            if (typeHandleCache.TryGetValue(type.AssemblyQualifiedName, out handle))
                return handle;

            if (typeHandleCacheCollectible.TryGetValue(type.AssemblyQualifiedName, out handle))
                return handle;

            throw new Exception($"GCHandle not found for type '{type.FullName}'");
        }

        internal static class DelegateHelpers
        {
            private static readonly Func<Type[], Type> MakeNewCustomDelegate =
            (Func<Type[], Type>)Delegate.CreateDelegate(typeof(Func<Type[], Type>),
                                                        typeof(Expression).Assembly.GetType("System.Linq.Expressions.Compiler.DelegateHelpers")
                                                                          .GetMethod("MakeNewCustomDelegate", BindingFlags.NonPublic | BindingFlags.Static));

            internal static void Init()
            {
                // Ensures the MakeNewCustomDelegate is put in the collectible ALC?
                using var ctx = scriptingAssemblyLoadContext.EnterContextualReflection();

                MakeNewCustomDelegate(new[] { typeof(void) });
            }

            internal static Type NewDelegateType(params Type[] parameters)
            {
                if (parameters.Any(x => scriptingAssemblyLoadContext.Assemblies.Contains(x.Module.Assembly)))
                {
                    // Ensure the new delegate is placed in the collectible ALC
                    //using var ctx = scriptingAssemblyLoadContext.EnterContextualReflection();
                    return MakeNewCustomDelegate(parameters);
                }

                return MakeNewCustomDelegate(parameters);
            }
        }

        /// <summary>
        /// Wrapper class for invoking function pointers from unmanaged code.
        /// </summary>
        internal class ThunkContext
        {
            internal MethodInfo methodInfo;
            internal int numParams;
            internal Type[] parameterTypes;
            internal object[] objParams;

            private static MethodInfo castMethod = typeof(ThunkContext).GetMethod("Cast", BindingFlags.Static | BindingFlags.NonPublic);

            private static object CastParameter(Type type, IntPtr ptr)
            {
                if (type.IsValueType)
                {
                    var closeCast = castMethod.MakeGenericMethod(type);
                    return closeCast.Invoke(ptr, new[] { (object)ptr });
                }
                else if (type.IsClass)
                    return ptr == IntPtr.Zero ? null : GCHandle.FromIntPtr(ptr).Target;
                return null;
            }

            public IntPtr InvokeThunk(IntPtr instance, IntPtr param1, IntPtr param2, IntPtr param3, IntPtr param4, IntPtr param5, IntPtr param6, IntPtr param7)
            {
                var intPtrParams = stackalloc IntPtr[] { param1, param2, param3, param4, param5, param6, param7 };

                for (int i = 0; i < numParams; i++)
                    objParams[i] = CastParameter(parameterTypes[i], intPtrParams[i]);

                object returnObject;
                try
                {
                    returnObject = methodInfo.Invoke(instance != IntPtr.Zero ? GCHandle.FromIntPtr(instance).Target : null, objParams);
                }
                catch (Exception ex)
                {
                    IntPtr exceptionPtr = intPtrParams[numParams];
                    Marshal.WriteIntPtr(exceptionPtr, GCHandle.ToIntPtr(GCHandle.Alloc(ex, GCHandleType.Weak)));
                    return IntPtr.Zero;
                }

                // Marshal reference parameters
                /*for (int i = 0; i < numParams; i++)
                {
                    Type parameterType = parameterTypes[i];
                    if (parameterType.IsByRef)
                    {
                        MarshalFromObject(parameterType.GetElementType(), objParams[i], intPtrParams[i], out int writeSize);
                    }
                }*/

                if (returnObject is not null)
                {
                    if (returnObject is string returnStr)
                        return ManagedString.ToNative(returnStr);
                    else if (returnObject is IntPtr returnPtr)
                        return returnPtr;
                    else if (returnObject is bool boolValue)
                        return boolValue ? boolTruePtr : boolFalsePtr;
                    else
                        return GCHandle.ToIntPtr(GCHandle.Alloc(returnObject, GCHandleType.Weak));
                }
                return IntPtr.Zero;
            }
        }
    }
}
#endif

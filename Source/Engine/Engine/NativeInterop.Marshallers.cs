// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_NETCORE
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;

#pragma warning disable 1591

namespace FlaxEngine.Interop
{
#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedIn, typeof(ManagedHandleMarshaller.ManagedToNativeState))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedOut, typeof(ManagedHandleMarshaller.ManagedToNativeState))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementIn, typeof(ManagedHandleMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedOut, typeof(ManagedHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedIn, typeof(ManagedHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementOut, typeof(ManagedHandleMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object), MarshalMode.ManagedToUnmanagedRef, typeof(ManagedHandleMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(object), MarshalMode.UnmanagedToManagedRef, typeof(ManagedHandleMarshaller.Bidirectional))]
    [CustomMarshaller(typeof(object), MarshalMode.ElementRef, typeof(ManagedHandleMarshaller))]
    public static class ManagedHandleMarshaller
    {
#if FLAX_EDITOR
        [HideInEditor]
#endif
        public static class NativeToManaged
        {
            public static object ConvertToManaged(IntPtr unmanaged)
            {
                if (unmanaged == IntPtr.Zero)
                    return null;
                object managed = ManagedHandle.FromIntPtr(unmanaged).Target;
                if (managed is ManagedArray managedArray)
                {
                    var managedArrayHandle = ManagedHandle.Alloc(managedArray, GCHandleType.Normal);
                    managed = NativeInterop.MarshalToManaged((IntPtr)managedArrayHandle, managedArray.ArrayType);
                    managedArrayHandle.Free();
                }
                return managed;
            }

            public static IntPtr ConvertToUnmanaged(object managed) => managed != null ? ManagedHandle.ToIntPtr(managed, GCHandleType.Weak) : IntPtr.Zero;

            public static void Free(IntPtr unmanaged)
            {
                // This is a permanent handle, do not release it
            }
        }


#if FLAX_EDITOR
        [HideInEditor]
#endif
        public struct ManagedToNativeState
        {
            ManagedArray managedArray;
            IntPtr handle;

            public void FromManaged(object managed)
            {
                if (managed == null)
                    return;
                if (managed is Array arr)
                {
                    var type = managed.GetType();
                    var elementType = type.GetElementType();
                    if (NativeInterop.ArrayFactory.GetMarshalledType(elementType) == elementType)
                    {
                        // Use pooled managed array wrapper to be passed around as handle to it
                        (ManagedHandle tmp, managedArray) = ManagedArray.WrapPooledArray(arr);
                        handle = ManagedHandle.ToIntPtr(tmp);
                    }
                    else
                    {
                        // Convert array contents to be properly accessed by the native code (as GCHandles array)
                        managedArray = NativeInterop.ManagedArrayToGCHandleWrappedArray(arr);
                        handle = ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managedArray));
                        managedArray = null; // It's not pooled
                    }
                }
                else
                    handle = ManagedHandle.ToIntPtr(managed, GCHandleType.Weak);
            }

            public IntPtr ToUnmanaged()
            {
                return handle;
            }

            public void Free()
            {
                managedArray?.FreePooled();
            }
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public static class ManagedToNative
        {
            public static object ConvertToManaged(IntPtr unmanaged) => unmanaged == IntPtr.Zero ? null : ManagedHandle.FromIntPtr(unmanaged).Target;
            public static IntPtr ConvertToUnmanaged(object managed) => managed != null ? ManagedHandle.ToIntPtr(managed, GCHandleType.Weak) : IntPtr.Zero;

            public static void Free(IntPtr unmanaged)
            {
            }
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public struct Bidirectional
        {
            object managed;
            IntPtr unmanaged;

            public void FromManaged(object managed)
            {
                this.managed = managed;
            }

            public IntPtr ToUnmanaged()
            {
                unmanaged = ManagedHandleMarshaller.ToNative(managed);
                return unmanaged;
            }

            public void FromUnmanaged(IntPtr unmanaged)
            {
                this.unmanaged = unmanaged;
            }

            public object ToManaged()
            {
                managed = ManagedHandleMarshaller.ToManaged(unmanaged);
                unmanaged = IntPtr.Zero;
                return managed;
            }

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
        public static IntPtr ToNative(object managed) => managed != null ? ManagedHandle.ToIntPtr(managed) : IntPtr.Zero;

        public static void Free(IntPtr unmanaged)
        {
            if (unmanaged == IntPtr.Zero)
                return;
            ManagedHandle.FromIntPtr(unmanaged).Free();
        }
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(Type), MarshalMode.Default, typeof(SystemTypeMarshaller))]
    public static class SystemTypeMarshaller
    {
        public static Type ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<FlaxEngine.Interop.NativeInterop.TypeHolder>(ManagedHandleMarshaller.ConvertToManaged(unmanaged)).type : null;

        public static IntPtr ConvertToUnmanaged(Type managed)
        {
            if (managed == null)
                return IntPtr.Zero;
            ManagedHandle handle = NativeInterop.GetTypeManagedHandle(managed);
            return ManagedHandle.ToIntPtr(handle);
        }

        public static void Free(IntPtr unmanaged)
        {
            // Cached handle, do not release
        }
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(Exception), MarshalMode.Default, typeof(ExceptionMarshaller))]
    public static class ExceptionMarshaller
    {
        public static Exception ConvertToManaged(IntPtr unmanaged) => Unsafe.As<Exception>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));
        public static IntPtr ConvertToUnmanaged(Exception managed) => ManagedHandleMarshaller.ConvertToUnmanaged(managed);
        public static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.Free(unmanaged);
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ManagedToUnmanagedIn, typeof(ObjectMarshaller))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.UnmanagedToManagedOut, typeof(ObjectMarshaller))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ElementIn, typeof(ObjectMarshaller))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ManagedToUnmanagedOut, typeof(ObjectMarshaller))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.UnmanagedToManagedIn, typeof(ObjectMarshaller))]
    [CustomMarshaller(typeof(FlaxEngine.Object), MarshalMode.ElementOut, typeof(ObjectMarshaller))]
    public static class ObjectMarshaller
    {
        public static FlaxEngine.Object ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<FlaxEngine.Object>(ManagedHandle.FromIntPtr(unmanaged).Target) : null;
        public static IntPtr ConvertToUnmanaged(FlaxEngine.Object managed) => Unsafe.As<object>(managed) != null ? ManagedHandle.ToIntPtr(managed) : IntPtr.Zero;
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(CultureInfo), MarshalMode.Default, typeof(CultureInfoMarshaller))]
    public static class CultureInfoMarshaller
    {
        public static CultureInfo ConvertToManaged(IntPtr unmanaged) => Unsafe.As<CultureInfo>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));
        public static IntPtr ConvertToUnmanaged(CultureInfo managed) => ManagedHandleMarshaller.ConvertToUnmanaged(managed);
        public static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.Free(unmanaged);
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(Version), MarshalMode.Default, typeof(VersionMarshaller))]
    public static class VersionMarshaller
    {
        public static Version ConvertToManaged(IntPtr unmanaged) => Unsafe.As<Version>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));
        public static IntPtr ConvertToUnmanaged(Version managed) => ManagedHandleMarshaller.ConvertToUnmanaged(managed);
        public static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.Free(unmanaged);
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(Array), MarshalMode.ManagedToUnmanagedIn, typeof(SystemArrayMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(Array), MarshalMode.UnmanagedToManagedOut, typeof(SystemArrayMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(Array), MarshalMode.ManagedToUnmanagedOut, typeof(SystemArrayMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(Array), MarshalMode.UnmanagedToManagedIn, typeof(SystemArrayMarshaller.NativeToManaged))]
    public static unsafe class SystemArrayMarshaller
    {
#if FLAX_EDITOR
        [HideInEditor]
#endif
        public struct ManagedToNative
        {
            ManagedArray managedArray;
            ManagedHandle handle;

            public void FromManaged(Array managed)
            {
                if (managed != null)
                    (handle, managedArray) = ManagedArray.WrapPooledArray(managed);
            }

            public IntPtr ToUnmanaged()
            {
                if (managedArray == null)
                    return IntPtr.Zero;
                return ManagedHandle.ToIntPtr(handle);
            }

            public void Free()
            {
                if (managedArray == null)
                    return;
                managedArray.FreePooled();
            }
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public struct NativeToManaged
        {
            ManagedHandle handle;
            ManagedArray managedArray;

            public void FromUnmanaged(IntPtr unmanaged)
            {
                if (unmanaged == IntPtr.Zero)
                    return;
                handle = ManagedHandle.FromIntPtr(unmanaged);
            }

            public Array ToManaged()
            {
                if (!handle.IsAllocated)
                    return null;
                managedArray = Unsafe.As<ManagedArray>(handle.Target);
                return managedArray.ToArray();
            }

            public void Free()
            {
                if (!handle.IsAllocated)
                    return;
                managedArray.Free();
                handle.Free();
            }
        }
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(object[]), MarshalMode.ManagedToUnmanagedIn, typeof(SystemObjectArrayMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object[]), MarshalMode.UnmanagedToManagedOut, typeof(SystemObjectArrayMarshaller.ManagedToNative))]
    [CustomMarshaller(typeof(object[]), MarshalMode.ManagedToUnmanagedOut, typeof(SystemObjectArrayMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(object[]), MarshalMode.UnmanagedToManagedIn, typeof(SystemObjectArrayMarshaller.NativeToManaged))]
    public static unsafe class SystemObjectArrayMarshaller
    {
#if FLAX_EDITOR
        [HideInEditor]
#endif
        public struct ManagedToNative
        {
            ManagedHandle handle;

            public void FromManaged(object[] managed)
            {
                if (managed != null)
                {
                    var managedArray = NativeInterop.ManagedArrayToGCHandleWrappedArray(managed);
                    handle = ManagedHandle.Alloc(managedArray, GCHandleType.Weak);
                }
            }

            public IntPtr ToUnmanaged()
            {
                return ManagedHandle.ToIntPtr(handle);
            }

            public void Free()
            {
                handle.Free();
            }
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public struct NativeToManaged
        {
            ManagedHandle handle;

            public void FromUnmanaged(IntPtr unmanaged)
            {
                if (unmanaged == IntPtr.Zero)
                    return;
                handle = ManagedHandle.FromIntPtr(unmanaged);
            }

            public object[] ToManaged()
            {
                object[] result = null;
                if (handle.IsAllocated)
                {
                    var managedArray = Unsafe.As<ManagedArray>(handle.Target);
                    result = NativeInterop.GCHandleArrayToManagedArray<object>(managedArray);
                }
                return result;
            }

            public void Free()
            {
                handle.Free();
            }
        }
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
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
#if FLAX_EDITOR
        [HideInEditor]
#endif
        public static class NativeToManaged
        {
            public static Dictionary<T, U> ConvertToManaged(IntPtr unmanaged) => DictionaryMarshaller<T, U>.ToManaged(unmanaged);
            public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => DictionaryMarshaller<T, U>.ToNative(managed, GCHandleType.Weak);
            public static void Free(IntPtr unmanaged) => DictionaryMarshaller<T, U>.Free(unmanaged);
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public static class ManagedToNative
        {
            public static Dictionary<T, U> ConvertToManaged(IntPtr unmanaged) => DictionaryMarshaller<T, U>.ToManaged(unmanaged);
            public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => DictionaryMarshaller<T, U>.ToNative(managed, GCHandleType.Weak);

            public static void Free(IntPtr unmanaged)
            {
                //DictionaryMarshaller<T, U>.Free(unmanaged); // No need to free weak handles
            }
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
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
        public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => managed != null ? ManagedHandle.ToIntPtr(managed) : IntPtr.Zero;

        public static Dictionary<T, U> ToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<Dictionary<T, U>>(ManagedHandle.FromIntPtr(unmanaged).Target) : null;
        public static IntPtr ToNative(Dictionary<T, U> managed, GCHandleType handleType = GCHandleType.Normal) => managed != null ? ManagedHandle.ToIntPtr(managed, handleType) : IntPtr.Zero;

        public static void Free(IntPtr unmanaged)
        {
            if (unmanaged != IntPtr.Zero)
                ManagedHandle.FromIntPtr(unmanaged).Free();
        }
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ManagedToUnmanagedIn, typeof(ArrayMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.UnmanagedToManagedOut, typeof(ArrayMarshaller<,>.ManagedToNative))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ManagedToUnmanagedOut, typeof(ArrayMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.UnmanagedToManagedIn, typeof(ArrayMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ElementOut, typeof(ArrayMarshaller<,>.NativeToManaged))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ManagedToUnmanagedRef, typeof(ArrayMarshaller<,>.Bidirectional))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.UnmanagedToManagedRef, typeof(ArrayMarshaller<,>.Bidirectional))]
    [CustomMarshaller(typeof(CustomMarshallerAttribute.GenericPlaceholder[]), MarshalMode.ElementRef, typeof(ArrayMarshaller<,>))]
    [ContiguousCollectionMarshaller]
    public static unsafe class ArrayMarshaller<T, TUnmanagedElement> where TUnmanagedElement : unmanaged
    {
#if FLAX_EDITOR
        [HideInEditor]
#endif
        public static class NativeToManaged
        {
            public static T[] AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged is null)
                    return null;
                return new T[numElements];
            }

            public static TUnmanagedElement* AllocateContainerForUnmanagedElements(T[] managed, out int numElements)
            {
                if (managed is null)
                {
                    numElements = 0;
                    return null;
                }
                numElements = managed.Length;
                (ManagedHandle managedArrayHandle, _) = ManagedArray.AllocatePooledArray<TUnmanagedElement>(managed.Length);
                return (TUnmanagedElement*)ManagedHandle.ToIntPtr(managedArrayHandle);
            }

            public static ReadOnlySpan<T> GetManagedValuesSource(T[] managed) => managed;

            public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;
                ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.ToSpan<TUnmanagedElement>();
            }

            public static Span<T> GetManagedValuesDestination(T[] managed) => managed;

            public static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return ReadOnlySpan<TUnmanagedElement>.Empty;
                ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.ToSpan<TUnmanagedElement>();
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
                return managedArray.ToSpan<TUnmanagedElement>();
            }
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public ref struct ManagedToNative
        {
            T[] sourceArray;
            ManagedArray managedArray;
            ManagedHandle managedHandle;

            public void FromManaged(T[] managed)
            {
                if (managed is null)
                    return;
                sourceArray = managed;
                (managedHandle, managedArray) = ManagedArray.AllocatePooledArray<TUnmanagedElement>(managed.Length);
            }

            public ReadOnlySpan<T> GetManagedValuesSource() => sourceArray;

            public Span<TUnmanagedElement> GetUnmanagedValuesDestination() => managedArray != null ? managedArray.ToSpan<TUnmanagedElement>() : Span<TUnmanagedElement>.Empty;

            public TUnmanagedElement* ToUnmanaged() => (TUnmanagedElement*)ManagedHandle.ToIntPtr(managedHandle);

            public void Free() => managedArray?.FreePooled();
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public struct Bidirectional
        {
            T[] sourceArray;
            ManagedArray managedArray;
            ManagedHandle handle; // Valid only for pooled array

            public void FromManaged(T[] managed)
            {
                if (managed == null)
                    return;
                sourceArray = managed;
                (handle, managedArray) = ManagedArray.AllocatePooledArray<TUnmanagedElement>(managed.Length);
            }

            public ReadOnlySpan<T> GetManagedValuesSource() => sourceArray;

            public Span<TUnmanagedElement> GetUnmanagedValuesDestination()
            {
                if (managedArray == null)
                    return Span<TUnmanagedElement>.Empty;
                return managedArray.ToSpan<TUnmanagedElement>();
            }

            public TUnmanagedElement* ToUnmanaged() => (TUnmanagedElement*)ManagedHandle.ToIntPtr(handle);

            public void FromUnmanaged(TUnmanagedElement* unmanaged)
            {
                ManagedArray arr = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                if (sourceArray == null || sourceArray.Length != arr.Length)
                {
                    // Array was resized when returned from native code (as ref parameter)
                    managedArray.FreePooled();
                    sourceArray = new T[arr.Length];
                    managedArray = arr;
                    handle = new ManagedHandle(); // Invalidate as it's not pooled array anymore
                }
            }

            public ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(int numElements)
            {
                if (managedArray == null)
                    return ReadOnlySpan<TUnmanagedElement>.Empty;
                return managedArray.ToSpan<TUnmanagedElement>();
            }

            public Span<T> GetManagedValuesDestination(int numElements) => sourceArray;

            public T[] ToManaged() => sourceArray;

            public void Free()
            {
                if (handle.IsAllocated)
                    managedArray.FreePooled();
            }
        }

        public static TUnmanagedElement* AllocateContainerForUnmanagedElements(T[] managed, out int numElements)
        {
            if (managed is null)
            {
                numElements = 0;
                return null;
            }
            numElements = managed.Length;
            (ManagedHandle managedArrayHandle, _) = ManagedArray.AllocatePooledArray<TUnmanagedElement>(managed.Length);
            return (TUnmanagedElement*)ManagedHandle.ToIntPtr(managedArrayHandle);
        }

        public static ReadOnlySpan<T> GetManagedValuesSource(T[] managed) => managed;

        public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return Span<TUnmanagedElement>.Empty;
            ManagedArray unmanagedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
            return unmanagedArray.ToSpan<TUnmanagedElement>();
        }

        public static T[] AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements) => unmanaged is null ? null : new T[numElements];

        public static Span<T> GetManagedValuesDestination(T[] managed) => managed;

        public static ReadOnlySpan<TUnmanagedElement> GetUnmanagedValuesSource(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return ReadOnlySpan<TUnmanagedElement>.Empty;
            ManagedArray array = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
            return array.ToSpan<TUnmanagedElement>();
        }

        public static void Free(TUnmanagedElement* unmanaged)
        {
            if (unmanaged == null)
                return;
            ManagedHandle handle = ManagedHandle.FromIntPtr(new IntPtr(unmanaged));
            Unsafe.As<ManagedArray>(handle.Target).FreePooled();
        }
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
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
#if FLAX_EDITOR
        [HideInEditor]
#endif
        public static class NativeToManaged
        {
            public static string ConvertToManaged(IntPtr unmanaged) => ManagedString.ToManaged(unmanaged);
            public static unsafe IntPtr ConvertToUnmanaged(string managed) => managed == null ? IntPtr.Zero : ManagedHandle.ToIntPtr(managed, GCHandleType.Weak);
            public static void Free(IntPtr unmanaged) => ManagedString.Free(unmanaged);
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
        public static class ManagedToNative
        {
            public static string ConvertToManaged(IntPtr unmanaged) => ManagedString.ToManaged(unmanaged);
            public static unsafe IntPtr ConvertToUnmanaged(string managed) => managed == null ? IntPtr.Zero : ManagedHandle.ToIntPtr(managed, GCHandleType.Weak);

            public static void Free(IntPtr unmanaged)
            {
                //ManagedString.Free(unmanaged); // No need to free weak handles
            }
        }

#if FLAX_EDITOR
        [HideInEditor]
#endif
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
}

#endif

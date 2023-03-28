// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if USE_NETCORE
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.Marshalling;

#pragma warning disable 1591

namespace FlaxEngine.Interop
{
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
            public static IntPtr ConvertToUnmanaged(object managed) => managed != null ? ManagedHandle.ToIntPtr(managed) : IntPtr.Zero;

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
            public static IntPtr ConvertToUnmanaged(FlaxEngine.Object managed) => managed != null ? ManagedHandle.ToIntPtr(managed) : IntPtr.Zero;
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
    [CustomMarshaller(typeof(Array), MarshalMode.ManagedToUnmanagedOut, typeof(SystemArrayMarshaller.NativeToManaged))]
    [CustomMarshaller(typeof(Array), MarshalMode.UnmanagedToManagedIn, typeof(SystemArrayMarshaller.NativeToManaged))]
    public static unsafe class SystemArrayMarshaller
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
                if (managedArray == null)
                    return;
                managedArray.FreePooled();
                handle.Free();
            }
        }

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
        public static IntPtr ConvertToUnmanaged(Dictionary<T, U> managed) => managed != null ? ManagedHandle.ToIntPtr(managed) : IntPtr.Zero;

        public static Dictionary<T, U> ToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? Unsafe.As<Dictionary<T, U>>(ManagedHandle.FromIntPtr(unmanaged).Target) : null;
        public static IntPtr ToNative(Dictionary<T, U> managed) => managed != null ? ManagedHandle.ToIntPtr(managed) : IntPtr.Zero;

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
                var ptr = ManagedHandle.ToIntPtr(managedArray);
                return (TUnmanagedElement*)ptr;
            }

            public static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed) => managed;

            public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
            {
                if (unmanaged == null)
                    return Span<TUnmanagedElement>.Empty;
                ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
                return managedArray.ToSpan<TUnmanagedElement>();
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
                return unmanagedArray.ToSpan<TUnmanagedElement>();
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
                return unmanagedArray.ToSpan<TUnmanagedElement>();
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
            IntPtr handle = ManagedHandle.ToIntPtr(managedArray);
            return (TUnmanagedElement*)handle;
        }

        public static ReadOnlySpan<T> GetManagedValuesSource(T[]? managed) => managed;

        public static Span<TUnmanagedElement> GetUnmanagedValuesDestination(TUnmanagedElement* unmanaged, int numElements)
        {
            if (unmanaged == null)
                return Span<TUnmanagedElement>.Empty;
            ManagedArray unmanagedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(new IntPtr(unmanaged)).Target);
            return unmanagedArray.ToSpan<TUnmanagedElement>();
        }

        public static T[]? AllocateContainerForManagedElements(TUnmanagedElement* unmanaged, int numElements) => unmanaged is null ? null : new T[numElements];

        public static Span<T> GetManagedValuesDestination(T[]? managed) => managed;

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
                return managed == null ? IntPtr.Zero : ManagedHandle.ToIntPtr(managed);
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
}

#endif

// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_NETCORE
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Threading;
using FlaxEngine.Assertions;

#pragma warning disable 1591

namespace FlaxEngine.Interop
{
    /// <summary>
    /// Wrapper for managed arrays which are passed to unmanaged code.
    /// </summary>
#if FLAX_EDITOR
    [HideInEditor]
#endif
    public unsafe class ManagedArray
    {
        private ManagedHandle _managedHandle;
        private IntPtr _unmanagedData;
        private int _unmanagedAllocationSize;
        private Type _arrayType;
        private Type _elementType;
        private int _elementSize;
        private int _length;

        [ThreadStatic] private static Dictionary<ManagedArray, ManagedHandle> pooledArrayHandles;

        public static ManagedArray WrapNewArray(Array arr) => new ManagedArray(arr, arr.GetType());

        public static ManagedArray WrapNewArray(Array arr, Type arrayType) => new ManagedArray(arr, arrayType);

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>The resources must be released by calling FreePooled() instead of Free()-method.</remarks>
        public static (ManagedHandle managedHandle, ManagedArray managedArray) WrapPooledArray(Array arr)
        {
            ManagedArray managedArray = ManagedArrayPool.Get();
            managedArray.WrapArray(arr, arr.GetType());

            if (pooledArrayHandles == null)
                pooledArrayHandles = new();
            if (!pooledArrayHandles.TryGetValue(managedArray, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(managedArray);
                pooledArrayHandles.Add(managedArray, handle);
            }
            return (handle, managedArray);
        }

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>The resources must be released by calling FreePooled() instead of Free()-method.</remarks>
        public static ManagedHandle WrapPooledArray(Array arr, Type arrayType)
        {
            ManagedArray managedArray = ManagedArrayPool.Get(arr.Length * NativeInterop.GetTypeSize(arr.GetType().GetElementType()));
            managedArray.WrapArray(arr, arrayType);

            if (pooledArrayHandles == null)
                pooledArrayHandles = new();
            if (!pooledArrayHandles.TryGetValue(managedArray, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(managedArray);
                pooledArrayHandles.Add(managedArray, handle);
            }
            return handle;
        }

        internal static ManagedArray AllocateNewArray(int length, Type arrayType, Type elementType)
            => new ManagedArray((IntPtr)NativeInterop.NativeAlloc(length, NativeInterop.GetTypeSize(elementType)), length, arrayType, elementType);

        internal static ManagedArray AllocateNewArray(IntPtr ptr, int length, Type arrayType, Type elementType)
            => new ManagedArray(ptr, length, arrayType, elementType);

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>The resources must be released by calling FreePooled() instead of Free()-method. Do not release the returned ManagedHandle.</remarks>
        public static (ManagedHandle managedHandle, ManagedArray managedArray) AllocatePooledArray<T>(int length) where T : unmanaged
        {
            ManagedArray managedArray = ManagedArrayPool.Get(length * Unsafe.SizeOf<T>());
            managedArray.Allocate<T>(length);

            if (pooledArrayHandles == null)
                pooledArrayHandles = new();
            if (!pooledArrayHandles.TryGetValue(managedArray, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(managedArray);
                pooledArrayHandles.Add(managedArray, handle);
            }
            return (handle, managedArray);
        }

        public ManagedArray(Array arr, Type elementType) => WrapArray(arr, elementType);

        internal void WrapArray(Array arr, Type arrayType)
        {
            if (_unmanagedData != IntPtr.Zero)
                NativeInterop.NativeFree(_unmanagedData.ToPointer());
            if (_managedHandle.IsAllocated)
                _managedHandle.Free();
            _managedHandle = ManagedHandle.Alloc(arr, GCHandleType.Pinned);
            _unmanagedData = Marshal.UnsafeAddrOfPinnedArrayElement(arr, 0);
            _unmanagedAllocationSize = 0;
            _length = arr.Length;
            _arrayType = arrayType;
            _elementType = arr.GetType().GetElementType();
            _elementSize = NativeInterop.GetTypeSize(_elementType);
        }

        internal void Allocate<T>(int length) where T : unmanaged
        {
            _length = length;
            _arrayType = typeof(T[]);
            _elementType = typeof(T);
            _elementSize = Unsafe.SizeOf<T>();

            // Try to reuse existing allocated buffer
            if (length * _elementSize > _unmanagedAllocationSize)
            {
                if (_unmanagedAllocationSize > 0)
                    NativeInterop.NativeFree(_unmanagedData.ToPointer());
                _unmanagedData = (IntPtr)NativeInterop.NativeAlloc(length, _elementSize);
                _unmanagedAllocationSize = _elementSize * length;
            }
        }

        private ManagedArray()
        {
        }

        private ManagedArray(IntPtr ptr, int length, Type arrayType, Type elementType)
        {
            Assert.IsTrue(arrayType.IsArray);
            _elementType = elementType;
            _elementSize = NativeInterop.GetTypeSize(elementType);
            _unmanagedData = ptr;
            _unmanagedAllocationSize = _elementSize * length;
            _length = length;
            _arrayType = arrayType;
        }

        ~ManagedArray()
        {
            if (_unmanagedData != IntPtr.Zero)
                Free();
        }

        public void Free()
        {
            GC.SuppressFinalize(this);
            if (_managedHandle.IsAllocated)
            {
                _managedHandle.Free();
                _unmanagedData = IntPtr.Zero;
            }
            if (_unmanagedData != IntPtr.Zero)
            {
                NativeInterop.NativeFree(_unmanagedData.ToPointer());
                _unmanagedData = IntPtr.Zero;
                _unmanagedAllocationSize = 0;
            }
        }

        public void FreePooled()
        {
            if (_managedHandle.IsAllocated)
            {
                _managedHandle.Free();
                _unmanagedData = IntPtr.Zero;
            }
            _arrayType = _elementType = null;
            ManagedArrayPool.Put(this);
        }

        internal IntPtr Pointer => _unmanagedData;

        internal int Length => _length;

        internal int ElementSize => _elementSize;

        internal Type ElementType => _elementType;

        internal Type ArrayType => _arrayType;

        public Span<T> ToSpan<T>() where T : struct => new Span<T>(_unmanagedData.ToPointer(), _length);

        public T[] ToArray<T>() where T : struct => new Span<T>(_unmanagedData.ToPointer(), _length).ToArray();

        public Array ToArray() => ArrayCast.ToArray(new Span<byte>(_unmanagedData.ToPointer(), _length * _elementSize), _elementType);

        /// <summary>
        /// Creates an Array of the specified type from span of bytes.
        /// </summary>
        private static class ArrayCast
        {
            delegate Array GetDelegate(Span<byte> span);

            [ThreadStatic]
            private static Dictionary<Type, GetDelegate> delegates;

            internal static Array ToArray(Span<byte> span, Type type)
            {
                if (delegates == null)
                    delegates = new();
                if (!delegates.TryGetValue(type, out var deleg))
                {
                    deleg = typeof(ArrayCastInternal<>).MakeGenericType(type).GetMethod(nameof(ArrayCastInternal<int>.ToArray), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<GetDelegate>();
                    delegates.Add(type, deleg);
                }
                return deleg(span);
            }

            private static class ArrayCastInternal<T> where T : struct
            {
                internal static Array ToArray(Span<byte> span)
                {
                    return MemoryMarshal.Cast<byte, T>(span).ToArray();
                }
            }
        }

        /// <summary>
        /// Provides a pool of pre-allocated ManagedArray that can be re-used.
        /// </summary>
        private static class ManagedArrayPool
        {
            [ThreadStatic]
            private static List<(bool inUse, ManagedArray array)> pool;

            /// <summary>
            /// Borrows an array from the pool. 
            /// </summary>
            /// <param name="minimumSize">Minimum size in bytes for the borrowed array. With value of 0, the returned array allocation is always zero.</param>
            /// <remarks>The returned array size may be smaller than the requested minimumSize.</remarks>
            internal static ManagedArray Get(int minimumSize = 0)
            {
                if (pool == null)
                    pool = new();

                int smallest = -1;
                int smallestSize = int.MaxValue;
                var poolSpan = CollectionsMarshal.AsSpan(pool);
                for (int i = 0; i < poolSpan.Length; i++)
                {
                    ref var tuple = ref poolSpan[i];
                    if (tuple.inUse)
                        continue;

                    // Try to get larger arrays than requested in order to avoid reallocations
                    if (minimumSize > 0)
                    {
                        if (tuple.array._unmanagedAllocationSize >= minimumSize && tuple.array._unmanagedAllocationSize < smallestSize)
                            smallest = i;
                        continue;
                    }
                    else if (minimumSize == 0 && tuple.Item2._unmanagedAllocationSize != 0)
                        continue;

                    tuple.inUse = true;
                    return tuple.array;
                }
                if (minimumSize > 0 && smallest != -1)
                {
                    ref var tuple = ref poolSpan[smallest];
                    tuple.inUse = true;
                    return tuple.array;
                }

                var newTuple = (inUse: true, array: new ManagedArray());
                pool.Add(newTuple);
                return newTuple.array;
            }

            /// <summary>
            /// Returns the borrowed ManagedArray back to pool.
            /// </summary>
            /// <param name="obj">The array borrowed from the pool</param>
            internal static void Put(ManagedArray obj)
            {
                foreach (ref var tuple in CollectionsMarshal.AsSpan(pool))
                {
                    if (tuple.array != obj)
                        continue;

                    tuple.inUse = false;
                    return;
                }

                throw new NativeInteropException("Tried to free non-pooled ManagedArray as pooled ManagedArray");
            }
        }
    }

#if FLAX_EDITOR
    [HideInEditor]
#endif
    public static class ManagedString
    {
        internal static ManagedHandle EmptyStringHandle = ManagedHandle.Alloc(string.Empty);

        [System.Diagnostics.DebuggerStepThrough]
        public static unsafe IntPtr ToNative(string str)
        {
            if (str == null)
                return IntPtr.Zero;
            else if (str == string.Empty)
                return ManagedHandle.ToIntPtr(EmptyStringHandle);
            Assert.IsTrue(str.Length > 0);
            return ManagedHandle.ToIntPtr(str);
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static unsafe IntPtr ToNativeWeak(string str)
        {
            if (str == null)
                return IntPtr.Zero;
            else if (str == string.Empty)
                return ManagedHandle.ToIntPtr(EmptyStringHandle);
            Assert.IsTrue(str.Length > 0);
            return ManagedHandle.ToIntPtr(str, GCHandleType.Weak);
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static string ToManaged(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return null;
            return Unsafe.As<string>(ManagedHandle.FromIntPtr(ptr).Target);
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static void Free(IntPtr ptr)
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
#if FLAX_EDITOR
    [HideInEditor]
#endif
    public struct ManagedHandle
    {
        private IntPtr handle;

        private ManagedHandle(IntPtr handle) => this.handle = handle;

        private ManagedHandle(object value, GCHandleType type) => handle = ManagedHandlePool.AllocateHandle(value, type);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle Alloc(object value) => new ManagedHandle(value, GCHandleType.Normal);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle Alloc(object value, GCHandleType type) => new ManagedHandle(value, type);

        public void Free()
        {
            if (handle == IntPtr.Zero)
                return;
            ManagedHandlePool.FreeHandle(handle);
            handle = IntPtr.Zero;
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
        public static IntPtr ToIntPtr(object value) => ManagedHandlePool.AllocateHandle(value, GCHandleType.Normal);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntPtr ToIntPtr(object value, GCHandleType type) => ManagedHandlePool.AllocateHandle(value, type);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntPtr ToIntPtr(ManagedHandle value) => value.handle;

        public override int GetHashCode() => handle.GetHashCode();

        public override bool Equals(object obj) => obj is ManagedHandle other && handle == other.handle;

        public bool Equals(ManagedHandle other) => handle == other.handle;

        public static bool operator ==(ManagedHandle a, ManagedHandle b) => a.handle == b.handle;

        public static bool operator !=(ManagedHandle a, ManagedHandle b) => a.handle != b.handle;

        internal static class ManagedHandlePool
        {
            private const int WeakPoolCollectionSizeThreshold = 10000000;
            private const int WeakPoolCollectionTimeThreshold = 500;

            // Rolling numbers for handles, two bits reserved for the type
            private static ulong normalHandleAccumulator = ((ulong)GCHandleType.Normal << 62) & 0xC000000000000000;
            private static ulong pinnedHandleAccumulator = ((ulong)GCHandleType.Pinned << 62) & 0xC000000000000000;
            private static ulong weakHandleAccumulator = ((ulong)GCHandleType.Weak << 62) & 0xC000000000000000;

            // Dictionaries for storing the valid handles.
            // Note: Using locks seems to be generally the fastest when adding or fetching from the dictionary.
            // Concurrent dictionaries could also be considered, but they perform much slower when adding to the dictionary.
            private static Dictionary<IntPtr, object> persistentPool = new();
            private static Dictionary<IntPtr, GCHandle> pinnedPool = new();

            // TODO: Performance of pinned handles are poor at the moment due to GCHandle wrapping.
            // TODO: .NET8: Experiment with pinned arrays for faster pinning: https://github.com/dotnet/runtime/pull/89293

            // Manage double-buffered pool for weak handles in order to avoid collecting in-flight handles.
            // Periodically when the pools are being accessed and conditions are met, the other pool is cleared and swapped.
            private static Dictionary<IntPtr, object> weakPool = new();
            private static Dictionary<IntPtr, object> weakPoolOther = new();
            private static object weakPoolLock = new object();
            private static ulong nextWeakPoolCollection;
            private static int nextWeakPoolGCCollection;
            private static long lastWeakPoolCollectionTime;

            /// <summary>
            /// Tries to free all references to old weak handles so GC can collect them.
            /// </summary>
            internal static void TryCollectWeakHandles(bool force = false)
            {
                if (!force)
                {
                    if (weakHandleAccumulator < nextWeakPoolCollection)
                        return;

                    nextWeakPoolCollection = weakHandleAccumulator + 1000;

                    // Try to swap pools after garbage collection or whenever the pool gets too large
                    var gc0CollectionCount = GC.CollectionCount(0);
                    if (gc0CollectionCount < nextWeakPoolGCCollection && weakPool.Count < WeakPoolCollectionSizeThreshold)
                        return;
                    nextWeakPoolGCCollection = gc0CollectionCount + 1;

                    // Prevent huge allocations from swapping the pools in the middle of the operation
                    if (System.Diagnostics.Stopwatch.GetElapsedTime(lastWeakPoolCollectionTime).TotalMilliseconds < WeakPoolCollectionTimeThreshold)
                        return;
                }
                lastWeakPoolCollectionTime = System.Diagnostics.Stopwatch.GetTimestamp();

                // Swap the pools and release the oldest pool for GC
                (weakPool, weakPoolOther) = (weakPoolOther, weakPool);
                weakPool.Clear();
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static IntPtr NewHandle(GCHandleType type) => type switch
            {
                GCHandleType.Normal => (IntPtr)Interlocked.Increment(ref normalHandleAccumulator),
                GCHandleType.Pinned => (IntPtr)Interlocked.Increment(ref pinnedHandleAccumulator),
                GCHandleType.Weak => (IntPtr)Interlocked.Increment(ref weakHandleAccumulator),
                GCHandleType.WeakTrackResurrection => (IntPtr)Interlocked.Increment(ref weakHandleAccumulator),
                _ => throw new NotImplementedException(type.ToString())
            };

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static GCHandleType GetHandleType(IntPtr handle) => (GCHandleType)(((ulong)handle & 0xC000000000000000) >> 62);

            internal static IntPtr AllocateHandle(object value, GCHandleType type)
            {
                IntPtr handle = NewHandle(type);
                switch (type)
                {
                case GCHandleType.Normal:
                    lock (persistentPool)
                        persistentPool.Add(handle, value);
                    break;
                case GCHandleType.Pinned:
                    lock (pinnedPool)
                        pinnedPool.Add(handle, GCHandle.Alloc(value, GCHandleType.Pinned));
                    break;
                case GCHandleType.Weak:
                case GCHandleType.WeakTrackResurrection:
                    lock (weakPoolLock)
                    {
                        TryCollectWeakHandles();
                        weakPool.Add(handle, value);
                    }
                    break;
                }
                return handle;
            }

            internal static object GetObject(IntPtr handle)
            {
                switch (GetHandleType(handle))
                {
                case GCHandleType.Normal:
                    lock (persistentPool)
                    {
                        if (persistentPool.TryGetValue(handle, out object value))
                            return value;
                    }
                    break;
                case GCHandleType.Pinned:
                    lock (pinnedPool)
                    {
                        if (pinnedPool.TryGetValue(handle, out GCHandle gcHandle))
                            return gcHandle.Target;
                    }
                    break;
                case GCHandleType.Weak:
                case GCHandleType.WeakTrackResurrection:
                    lock (weakPoolLock)
                    {
                        TryCollectWeakHandles();
                        if (weakPool.TryGetValue(handle, out object value))
                            return value;
                        else if (weakPoolOther.TryGetValue(handle, out value))
                            return value;
                    }
                    break;
                }
                throw new NativeInteropException("Invalid ManagedHandle");
            }

            internal static void SetObject(IntPtr handle, object value)
            {
                switch (GetHandleType(handle))
                {
                case GCHandleType.Normal:
                    lock (persistentPool)
                    {
                        ref object obj = ref CollectionsMarshal.GetValueRefOrNullRef(persistentPool, handle);
                        if (!Unsafe.IsNullRef(ref obj))
                        {
                            obj = value;
                            return;
                        }
                    }
                    break;
                case GCHandleType.Pinned:
                    lock (pinnedPool)
                    {
                        ref GCHandle gcHandle = ref CollectionsMarshal.GetValueRefOrNullRef(pinnedPool, handle);
                        if (!Unsafe.IsNullRef(ref gcHandle))
                        {
                            gcHandle.Target = value;
                            return;
                        }
                    }
                    break;
                case GCHandleType.Weak:
                case GCHandleType.WeakTrackResurrection:
                    lock (weakPoolLock)
                    {
                        TryCollectWeakHandles();
                        {
                            ref object obj = ref CollectionsMarshal.GetValueRefOrNullRef(weakPool, handle);
                            if (!Unsafe.IsNullRef(ref obj))
                            {
                                obj = value;
                                return;
                            }
                        }
                        {
                            ref object obj = ref CollectionsMarshal.GetValueRefOrNullRef(weakPoolOther, handle);
                            if (!Unsafe.IsNullRef(ref obj))
                            {
                                obj = value;
                                return;
                            }
                        }
                    }
                    break;
                }
                throw new NativeInteropException("Invalid ManagedHandle");
            }

            internal static void FreeHandle(IntPtr handle)
            {
                switch (GetHandleType(handle))
                {
                case GCHandleType.Normal:
                    lock (persistentPool)
                    {
                        if (persistentPool.Remove(handle))
                            return;
                    }
                    break;
                case GCHandleType.Pinned:
                    lock (pinnedPool)
                    {
                        if (pinnedPool.Remove(handle, out GCHandle gcHandle))
                        {
                            gcHandle.Free();
                            return;
                        }
                    }
                    break;
                case GCHandleType.Weak:
                case GCHandleType.WeakTrackResurrection:
                    lock (weakPoolLock)
                        TryCollectWeakHandles();
                    return;
                }
                throw new NativeInteropException("Invalid ManagedHandle");
            }
        }
    }
}

#endif

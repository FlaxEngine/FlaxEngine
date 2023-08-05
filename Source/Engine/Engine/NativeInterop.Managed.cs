// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
            // Try to reuse existing allocated buffer
            if (length * Unsafe.SizeOf<T>() > _unmanagedAllocationSize)
            {
                if (_unmanagedAllocationSize > 0)
                    NativeInterop.NativeFree(_unmanagedData.ToPointer());
                _unmanagedData = (IntPtr)NativeInterop.NativeAlloc(length, Unsafe.SizeOf<T>());
                _unmanagedAllocationSize = Unsafe.SizeOf<T>() * length;
            }
            _length = length;
            _arrayType = typeof(T).MakeArrayType();
            _elementType = typeof(T);
            _elementSize = Unsafe.SizeOf<T>();
        }

        private ManagedArray()
        {
        }

        private ManagedArray(IntPtr ptr, int length, Type arrayType, Type elementType)
        {
            Assert.IsTrue(arrayType.IsArray);
            _unmanagedData = ptr;
            _unmanagedAllocationSize = Marshal.SizeOf(elementType) * length;
            _length = length;
            _arrayType = arrayType;
            _elementType = elementType;
            _elementSize = NativeInterop.GetTypeSize(_elementType);
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

        public override bool Equals(object o) => o is ManagedHandle other && Equals(other);

        public bool Equals(ManagedHandle other) => handle == other.handle;

        public static bool operator ==(ManagedHandle a, ManagedHandle b) => a.handle == b.handle;

        public static bool operator !=(ManagedHandle a, ManagedHandle b) => a.handle != b.handle;

        private static class ManagedHandlePool
        {
            private const int WeakPoolCollectionSizeThreshold = 10000000;
            private const int WeakPoolCollectionTimeThreshold = 500;

            private static ulong normalHandleAccumulator = 0;
            private static ulong pinnedHandleAccumulator = 0;
            private static ulong weakHandleAccumulator = 0;

            private static object poolLock = new object();
            private static Dictionary<IntPtr, object> persistentPool = new Dictionary<nint, object>();
            private static Dictionary<IntPtr, GCHandle> pinnedPool = new Dictionary<nint, GCHandle>();

            // Manage double-buffered pool for weak handles in order to avoid collecting in-flight handles
            [ThreadStatic] private static Dictionary<IntPtr, object> weakPool;
            [ThreadStatic] private static Dictionary<IntPtr, object> weakPoolOther;
            [ThreadStatic] private static ulong nextWeakPoolCollection;
            [ThreadStatic] private static int nextWeakPoolGCCollection;
            [ThreadStatic] private static long lastWeakPoolCollectionTime;

            /// <summary>
            /// Tries to free all references to old weak handles so GC can collect them.
            /// </summary>
            private static void TryCollectWeakHandles()
            {
                if (weakHandleAccumulator < nextWeakPoolCollection)
                    return;

                nextWeakPoolCollection = weakHandleAccumulator + 1000;
                if (weakPool == null)
                {
                    weakPool = new Dictionary<nint, object>();
                    weakPoolOther = new Dictionary<nint, object>();
                    nextWeakPoolGCCollection = GC.CollectionCount(0) + 1;
                    return;
                }
                // Try to swap pools after garbage collection or whenever the pool gets too large
                var gc0CollectionCount = GC.CollectionCount(0);
                if (gc0CollectionCount < nextWeakPoolGCCollection && weakPool.Count < WeakPoolCollectionSizeThreshold)
                    return;
                nextWeakPoolGCCollection = gc0CollectionCount + 1;

                // Prevent huge allocations from swapping the pools in the middle of the operation
                if (System.Diagnostics.Stopwatch.GetElapsedTime(lastWeakPoolCollectionTime).TotalMilliseconds < WeakPoolCollectionTimeThreshold)
                    return;
                lastWeakPoolCollectionTime = System.Diagnostics.Stopwatch.GetTimestamp();

                // Swap the pools and release the oldest pool for GC
                (weakPool, weakPoolOther) = (weakPoolOther, weakPool);
                weakPool.Clear();
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
                TryCollectWeakHandles();
                IntPtr handle = NewHandle(type);
                if (type == GCHandleType.Normal)
                {
                    lock (poolLock)
                        persistentPool.Add(handle, value);
                }
                else if (type == GCHandleType.Pinned)
                {
                    lock (poolLock)
                        pinnedPool.Add(handle, GCHandle.Alloc(value, GCHandleType.Pinned));
                }
                else if (type == GCHandleType.Weak || type == GCHandleType.WeakTrackResurrection)
                    weakPool.Add(handle, value);

                return handle;
            }

            internal static object GetObject(IntPtr handle)
            {
                TryCollectWeakHandles();
                object value;
                GCHandleType type = GetHandleType(handle);
                if (type == GCHandleType.Normal)
                {
                    lock (poolLock)
                    {
                        if (persistentPool.TryGetValue(handle, out value))
                            return value;
                    }
                }
                else if (type == GCHandleType.Pinned)
                {
                    lock (poolLock)
                    {
                        if (pinnedPool.TryGetValue(handle, out GCHandle gchandle))
                            return gchandle.Target;
                    }
                }
                else if (weakPool.TryGetValue(handle, out value))
                    return value;
                else if (weakPoolOther.TryGetValue(handle, out value))
                    return value;

                throw new NativeInteropException("Invalid ManagedHandle");
            }

            internal static void SetObject(IntPtr handle, object value)
            {
                TryCollectWeakHandles();
                GCHandleType type = GetHandleType(handle);
                if (type == GCHandleType.Normal)
                {
                    lock (poolLock)
                    {
                        if (persistentPool.ContainsKey(handle))
                            persistentPool[handle] = value;
                    }
                }
                else if (type == GCHandleType.Pinned)
                {
                    lock (poolLock)
                    {
                        if (pinnedPool.TryGetValue(handle, out GCHandle gchandle))
                            gchandle.Target = value;
                    }
                }
                else if (weakPool.ContainsKey(handle))
                    weakPool[handle] = value;
                else if (weakPoolOther.ContainsKey(handle))
                    weakPoolOther[handle] = value;

                throw new NativeInteropException("Invalid ManagedHandle");
            }

            internal static void FreeHandle(IntPtr handle)
            {
                TryCollectWeakHandles();
                GCHandleType type = GetHandleType(handle);
                if (type == GCHandleType.Normal)
                {
                    lock (poolLock)
                    {
                        if (persistentPool.Remove(handle))
                            return;
                    }
                }
                else if (type == GCHandleType.Pinned)
                {
                    lock (poolLock)
                    {
                        if (pinnedPool.Remove(handle, out GCHandle gchandle))
                        {
                            gchandle.Free();
                            return;
                        }
                    }
                }
                else
                    return;

                throw new NativeInteropException("Invalid ManagedHandle");
            }
        }
    }
}

#endif

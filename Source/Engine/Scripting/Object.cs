// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

// ReSharper disable UnassignedReadonlyField
// ReSharper disable InconsistentNaming
#pragma warning disable 649

namespace FlaxEngine
{
    /// <summary>
    /// Base class for all objects Flax can reference. Every object has unique identifier.
    /// </summary>
    [Serializable]
    public abstract class Object
    {
        /// <summary>
        /// The pointer to the unmanaged object (native C++ instance).
        /// </summary>
        [NonSerialized]
        protected readonly IntPtr __unmanagedPtr;

        /// <summary>
        /// The object unique identifier.
        /// </summary>
        [NonSerialized]
        protected readonly Guid __internalId;

        /// <summary>
        /// Gets the unique object ID.
        /// </summary>
        public Guid ID => __internalId;

        /// <summary>
        /// Gets a full name of the object type including leading namespace and any nested types names. Uniquely identifies the object type and can be used to find it via name.
        /// </summary>
        public string TypeName => Internal_GetTypeName(__unmanagedPtr);

        /// <summary>
        /// Initializes a new instance of the <see cref="Object"/>.
        /// </summary>
        protected Object()
        {
            // Construct missing native object if managed objects gets created in managed world
            if (__unmanagedPtr == IntPtr.Zero)
            {
                Internal_ManagedInstanceCreated(this);
                if (__unmanagedPtr == IntPtr.Zero)
                    throw new FlaxException($"Failed to create native instance for object of type {GetType().FullName} (assembly: {GetType().Assembly.FullName}).");
            }
        }

        /// <summary>
        /// Notifies the unmanaged interop object that the managed instance was finalized.
        /// </summary>
        ~Object()
        {
            // Inform unmanaged world that managed instance has been deleted
            if (__unmanagedPtr != IntPtr.Zero)
            {
                Internal_ManagedInstanceDeleted(__unmanagedPtr);
            }
        }

        /// <summary>
        /// Casts this object instance to the given object type.
        /// </summary>
        /// <typeparam name="T">object actor type.</typeparam>
        /// <returns>The object instance cast to the given actor type.</returns>
        public T As<T>() where T : Actor
        {
            return (T)this;
        }

        /// <summary>
        /// Creates the new instance of the C# object.
        /// </summary>
        /// <param name="type">Type of the object.</param>
        /// <returns>Created object.</returns>
        public static object NewValue([TypeReference(typeof(object))] Type type)
        {
            return Activator.CreateInstance(type);
        }

        /// <summary>
        /// Creates the new instance of the Object.
        /// All unused objects should be released using <see cref="Destroy"/>.
        /// </summary>
        /// <param name="typeName">Full name of the type of the object.</param>
        /// <returns>Created object.</returns>
        public static Object New([TypeReference(typeof(Object))] string typeName)
        {
            return Internal_Create2(typeName);
        }

        /// <summary>
        /// Creates the new instance of the Object.
        /// All unused objects should be released using <see cref="Destroy"/>.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Created object.</returns>
        [HideInEditor]
        public static T New<T>() where T : Object
        {
            return Internal_Create1(typeof(T)) as T;
        }

        /// <summary>
        /// Creates the new instance of the Object.
        /// All unused objects should be released using <see cref="Destroy"/>.
        /// </summary>
        /// <param name="type">Type of the object.</param>
        /// <returns>Created object.</returns>
        [HideInEditor]
        public static Object New(Type type)
        {
            return Internal_Create1(type);
        }

        /// <summary>
        /// Finds the object with the given ID. Searches registered scene objects and assets.
        /// </summary>
        /// <param name="id">Unique ID of the object.</param>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found object or null if missing.</returns>
        public static T Find<T>(ref Guid id) where T : Object
        {
            return Internal_FindObject(ref id, typeof(T)) as T;
        }

        /// <summary>
        /// Finds the object with the given ID. Searches registered scene objects and assets.
        /// </summary>
        /// <param name="id">Unique ID of the object.</param>
        /// <param name="type">Type of the object.</param>
        /// <returns>Found object or null if missing.</returns>
        public static Object Find(ref Guid id, Type type)
        {
            return Internal_FindObject(ref id, type);
        }

        /// <summary>
        /// Tries to find the object by the given identifier. Searches only registered scene objects.
        /// </summary>
        /// <param name="id">Unique ID of the object.</param>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found object or null if missing.</returns>
        public static T TryFind<T>(ref Guid id) where T : Object
        {
            return Internal_TryFindObject(ref id, typeof(T)) as T;
        }

        /// <summary>
        /// Destroys the specified object and clears the reference variable.
        /// The object obj will be destroyed now or after the time specified in seconds from now.
        /// If obj is a Script it will be removed from the Actor and deleted.
        /// If obj is an Actor it will be removed from the Scene and deleted as well as all its Scripts and all children of the Actor.
        /// Actual object destruction is always delayed until after the current Update loop, but will always be done before rendering.
        /// </summary>
        /// <param name="obj">The object to destroy.</param>
        /// <param name="timeLeft">The time left to destroy object (in seconds).</param>
        public static void Destroy(Object obj, float timeLeft = 0.0f)
        {
            Internal_Destroy(GetUnmanagedPtr(obj), timeLeft);
        }

        /// <summary>
        /// Destroys the specified object and clears the reference variable.
        /// The object obj will be destroyed now or after the time specified in seconds from now.
        /// If obj is a Script it will be removed from the Actor and deleted.
        /// If obj is an Actor it will be removed from the Scene and deleted as well as all its Scripts and all children of the Actor.
        /// Actual object destruction is always delayed until after the current Update loop, but will always be done before rendering.
        /// </summary>
        /// <param name="obj">The object to destroy.</param>
        /// <param name="timeLeft">The time left to destroy object (in seconds).</param>
        public static void Destroy<T>(ref T obj, float timeLeft = 0.0f) where T : Object
        {
            if (obj)
            {
                Internal_Destroy(obj.__unmanagedPtr, timeLeft);
                obj = null;
            }
        }

        /// <summary>
        /// Checks if the object exists (reference is not null and the unmanaged object pointer is valid).
        /// </summary>
        /// <param name="obj">The object to check.</param>
        /// <returns>True if object is valid, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator bool(Object obj)
        {
            return obj != null && obj.__unmanagedPtr != IntPtr.Zero;
        }

        /// <summary>
        /// Gets the pointer to the native object. Handles null object reference (returns zero).
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <returns>The native object pointer.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntPtr GetUnmanagedPtr(Object obj)
        {
            return obj?.__unmanagedPtr ?? IntPtr.Zero;
        }

        internal static IntPtr GetUnmanagedPtr(SoftObjectReference reference)
        {
            return GetUnmanagedPtr(reference.Get<Object>());
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return __unmanagedPtr.GetHashCode();
        }

        #region Internal Calls

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Object Internal_Create1(Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Object Internal_Create2(string typeName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_ManagedInstanceCreated(Object managedInstance);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_ManagedInstanceDeleted(IntPtr nativeInstance);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_Destroy(IntPtr obj, float timeLeft);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string Internal_GetTypeName(IntPtr obj);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Object Internal_FindObject(ref Guid id, Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Object Internal_TryFindObject(ref Guid id, Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_ChangeID(IntPtr obj, ref Guid id);

        #endregion
    }
}

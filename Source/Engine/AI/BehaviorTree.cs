// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
#if FLAX_EDITOR
using FlaxEngine.Utilities;
using FlaxEditor.Scripting;
using FlaxEngine.GUI;
#endif

namespace FlaxEngine
{
    partial class BehaviorKnowledge
    {
        /// <summary>
        /// Checks if knowledge has a given goal (exact type match without base class check).
        /// </summary>
        /// <typeparam name="T"> goal type.</typeparam>
        /// <returns>True if ahs a given goal, otherwise false.</returns>
        [Unmanaged]
        public bool HasGoal<T>()
        {
            return HasGoal(typeof(T));
        }

        /// <summary>
        /// Removes the goal from the knowledge. Does nothing if goal of the given type doesn't exist in the knowledge.
        /// </summary>
        /// <typeparam name="T"> goal type.</typeparam>
        [Unmanaged]
        public void RemoveGoal<T>()
        {
            RemoveGoal(typeof(T));
        }
    }

    partial class BehaviorTreeRootNode
    {
#if FLAX_EDITOR
        private static bool IsValidBlackboardType(ScriptType type)
        {
            if (new ScriptType(typeof(SceneObject)).IsAssignableFrom(type))
                return false;
            if (type.Type != null)
            {
                if (type.Type.IsDelegate())
                    return false;
                if (typeof(Control).IsAssignableFrom(type.Type))
                    return false;
                if (typeof(Attribute).IsAssignableFrom(type.Type))
                    return false;
                if (type.Type.FullName.StartsWith("FlaxEditor.", StringComparison.Ordinal))
                    return false;
            }
            return !type.IsGenericType &&
                   !type.IsInterface &&
                   !type.IsStatic &&
                   !type.IsAbstract &&
                   !type.IsArray &&
                   !type.IsVoid &&
                   (type.IsStructure || ScriptType.FlaxObject.IsAssignableFrom(type)) &&
                   type.IsPublic &&
                   type.CanCreateInstance;
        }
#endif
    }

    unsafe partial class BehaviorTreeNode
    {
        /// <summary>
        /// Gets the size in bytes of the given typed node state structure.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int GetStateSize<T>()
        {
            // C# nodes state is stored via pinned GCHandle to support holding managed references (eg. string or Vector3[])
            return sizeof(IntPtr);
        }

        /// <summary>
        /// Sets the node state at the given memory address.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void NewState(IntPtr memory, object state)
        {
            var handle = GCHandle.Alloc(state);
            var ptr = IntPtr.Add(memory, _memoryOffset).ToPointer();
            Unsafe.Write<IntPtr>(ptr, (IntPtr)handle);
        }

        /// <summary>
        /// Returns the typed node state at the given memory address.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public ref T GetState<T>(IntPtr memory) where T : struct
        {
            var ptr = Unsafe.Read<IntPtr>(IntPtr.Add(memory, _memoryOffset).ToPointer());
#if !BUILD_RELEASE
            if (ptr == IntPtr.Zero)
                throw new Exception($"Missing state '{typeof(T).FullName}' for node '{GetType().FullName}'");
#endif
            var handle = GCHandle.FromIntPtr(ptr);
            var state = handle.Target;
#if !BUILD_RELEASE
            if (state == null)
                throw new Exception($"Missing state '{typeof(T).FullName}' for node '{GetType().FullName}'");
#endif
            return ref Unsafe.Unbox<T>(state);
        }

        /// <summary>
        /// Frees the allocated node state at the given memory address.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void FreeState(IntPtr memory)
        {
            var ptr = Unsafe.Read<IntPtr>(IntPtr.Add(memory, _memoryOffset).ToPointer());
            if (ptr == IntPtr.Zero)
                return;
            var handle = GCHandle.FromIntPtr(ptr);
            handle.Free();
        }
    }
}

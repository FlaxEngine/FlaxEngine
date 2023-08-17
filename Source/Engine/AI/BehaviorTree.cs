// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
#if FLAX_EDITOR
using System;
using FlaxEngine.Utilities;
using FlaxEditor.Scripting;
using FlaxEngine.GUI;
#endif

namespace FlaxEngine
{
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
        public void AllocState(IntPtr memory, object state)
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
            var ptr = IntPtr.Add(memory, _memoryOffset).ToPointer();
            var handle = GCHandle.FromIntPtr(Unsafe.Read<IntPtr>(ptr));
            var state = handle.Target;
#if !BUILD_RELEASE
            if (state == null)
                throw new NullReferenceException();
#endif
            return ref Unsafe.Unbox<T>(state);
        }

        /// <summary>
        /// Frees the allocated node state at the given memory address.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void FreeState(IntPtr memory)
        {
            var ptr = IntPtr.Add(memory, _memoryOffset).ToPointer();
            var handle = GCHandle.FromIntPtr(Unsafe.Read<IntPtr>(ptr));
            handle.Free();
        }
    }
}

// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace FlaxEngine.Networking
{
    partial class NetworkReplicator
    {
        private static Dictionary<Type, KeyValuePair<SerializeFunc, SerializeFunc>> _managedSerializers;

#if FLAX_EDITOR
        private static void OnScriptsReloadBegin()
        {
            // Clear refs to managed types that will be hot-reloaded
            _managedSerializers.Clear();
            _managedSerializers = null;
            FlaxEditor.ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
        }
#endif

        /// <summary>
        /// Network object replication serialization/deserialization delegate.
        /// </summary>
        /// <remarks>
        /// Use Object.FromUnmanagedPtr(instancePtr/streamPtr) to get object or NetworkStream from raw native pointers.
        /// </remarks>
        /// <param name="instancePtr">var instance = Object.FromUnmanagedPtr(instancePtr)</param>
        /// <param name="streamPtr">var stream = (NetworkStream)Object.FromUnmanagedPtr(streamPtr)</param>
        public delegate void SerializeFunc(IntPtr instancePtr, IntPtr streamPtr);

        /// <summary>
        /// Registers a new serialization methods for a given C# type.
        /// </summary>
        /// <remarks>
        /// Use Object.FromUnmanagedPtr(instancePtr/streamPtr) to get object or NetworkStream from raw native pointers.
        /// </remarks>
        /// <param name="type">The C# type (class or structure).</param>
        /// <param name="serialize">Function to call for value serialization.</param>
        /// <param name="deserialize">Function to call for value deserialization.</param>
        [Unmanaged]
        public static void AddSerializer(Type type, SerializeFunc serialize, SerializeFunc deserialize)
        {
            // C#-only types (eg. custom C# structures) cannot use native serializers due to missing ScriptingType
            if (typeof(FlaxEngine.Object).IsAssignableFrom(type))
            {
                Internal_AddSerializer(type, Marshal.GetFunctionPointerForDelegate(serialize), Marshal.GetFunctionPointerForDelegate(deserialize));
            }
            else
            {
                if (_managedSerializers == null)
                {
                    _managedSerializers = new Dictionary<Type, KeyValuePair<SerializeFunc, SerializeFunc>>();
#if FLAX_EDITOR
                    FlaxEditor.ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
#endif
                }
                _managedSerializers[type] = new KeyValuePair<SerializeFunc, SerializeFunc>(serialize, deserialize);
            }
        }

        /// <summary>
        /// Invokes the network replication serializer for a given type.
        /// </summary>
        /// <param name="type">The scripting type to serialize.</param>
        /// <param name="instance">The value instance to serialize.</param>
        /// <param name="stream">The input/output stream to use for serialization.</param>
        /// <param name="serialize">True if serialize, otherwise deserialize mode.</param>
        /// <returns>True if failed, otherwise false.</returns>
        [Unmanaged]
        public static bool InvokeSerializer(Type type, FlaxEngine.Object instance, NetworkStream stream, bool serialize)
        {
            return Internal_InvokeSerializer(type, FlaxEngine.Object.GetUnmanagedPtr(instance), FlaxEngine.Object.GetUnmanagedPtr(stream), serialize);
        }

        /// <summary>
        /// Invokes the network replication serializer for a given type.
        /// </summary>
        /// <param name="type">The scripting type to serialize.</param>
        /// <param name="instance">The value instance to serialize.</param>
        /// <param name="stream">The input/output stream to use for serialization.</param>
        /// <param name="serialize">True if serialize, otherwise deserialize mode.</param>
        /// <returns>True if failed, otherwise false.</returns>
        [Unmanaged]
        public static bool InvokeSerializer(System.Type type, IntPtr instance, NetworkStream stream, bool serialize)
        {
            if (_managedSerializers != null && _managedSerializers.TryGetValue(type, out var e))
            {
                var serializer = serialize ? e.Key : e.Value;
                serializer(instance, FlaxEngine.Object.GetUnmanagedPtr(stream));
                return false;
            }
            return Internal_InvokeSerializer(type, instance, FlaxEngine.Object.GetUnmanagedPtr(stream), serialize);
        }
    }
}

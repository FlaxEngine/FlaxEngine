// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace FlaxEngine.Networking
{
    partial class NetworkReplicator
    {
        private static Dictionary<Type, KeyValuePair<SerializeFunc, SerializeFunc>> _managedSerializers;
        private static List<ExecuteRPCFunc> _managedExecuteRpcFuncs;

#if FLAX_EDITOR
        private static void ClearManagedReferences()
        {
            // Clear refs to managed types that will be hot-reloaded
            if (_managedSerializers != null)
            {
                _managedSerializers.Clear();
                _managedSerializers = null;
            }
            if (_managedExecuteRpcFuncs != null)
            {
                _managedExecuteRpcFuncs.Clear();
                _managedExecuteRpcFuncs = null;
            }
            FlaxEditor.ScriptsBuilder.ScriptsReloadBegin -= ClearManagedReferences;
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
        /// Network RPC executing delegate.
        /// </summary>
        /// <remarks>
        /// Use Object.FromUnmanagedPtr(objPtr/streamPtr) to get object or NetworkStream from raw native pointers.
        /// </remarks>
        /// <param name="instancePtr">var instance = Object.FromUnmanagedPtr(instancePtr)</param>
        /// <param name="streamPtr">var stream = (NetworkStream)Object.FromUnmanagedPtr(streamPtr)</param>
        public delegate void ExecuteRPCFunc(IntPtr instancePtr, IntPtr streamPtr);

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
            if (_managedSerializers == null)
            {
                _managedSerializers = new Dictionary<Type, KeyValuePair<SerializeFunc, SerializeFunc>>();
                _managedExecuteRpcFuncs = new List<ExecuteRPCFunc>();
#if FLAX_EDITOR
                FlaxEditor.ScriptsBuilder.ScriptsReloadBegin += ClearManagedReferences;
#endif
            }
            _managedSerializers[type] = new KeyValuePair<SerializeFunc, SerializeFunc>(serialize, deserialize);

            // C#-only types (eg. custom C# structures) cannot use native serializers due to missing ScriptingType
            if (typeof(FlaxEngine.Object).IsAssignableFrom(type))
            {
                Internal_AddSerializer(type, Marshal.GetFunctionPointerForDelegate(serialize), Marshal.GetFunctionPointerForDelegate(deserialize));
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

        /// <summary>
        /// Ends invoking the RPC.
        /// </summary>
        /// <param name="obj">The target object to invoke RPC.</param>
        /// <param name="type">The RPC type.</param>
        /// <param name="name">The RPC name.</param>
        /// <param name="argsStream">The RPC serialized arguments stream returned from BeginInvokeRPC.</param>
        /// <param name="targetIds">Optional list with network client IDs that should receive RPC. Empty to send on all clients. Ignored by Server RPCs.</param>
        /// <returns>True if RPC cannot be executed locally, false if execute it locally too (checks RPC mode and target client ids).</returns>
        [Unmanaged]
        public static bool EndInvokeRPC(Object obj, Type type, string name, NetworkStream argsStream, uint[] targetIds = null)
        {
            return Internal_CSharpEndInvokeRPC(FlaxEngine.Object.GetUnmanagedPtr(obj), type, name, FlaxEngine.Object.GetUnmanagedPtr(argsStream), targetIds);
        }

        /// <summary>
        /// Registers a RPC method for a given C# method.
        /// </summary>
        /// <param name="type">The C# type (FlaxEngine.Object).</param>
        /// <param name="name">The RPC method name (from that type).</param>
        /// <param name="execute">Function to call for RPC execution.</param>
        /// <param name="isServer">Server RPC.</param>
        /// <param name="isClient">Client RPC.</param>
        /// <param name="channel">Network channel to use for RPC transport.</param>
        [Unmanaged]
        public static unsafe void AddRPC(Type type, string name, ExecuteRPCFunc execute, bool isServer = true, bool isClient = false, NetworkChannelType channel = NetworkChannelType.ReliableOrdered)
        {
            if (!typeof(FlaxEngine.Object).IsAssignableFrom(type))
                throw new ArgumentException("Not supported type for RPC. Only FlaxEngine.Object types are valid.");

            if (_managedExecuteRpcFuncs == null)
            {
                _managedSerializers = new Dictionary<Type, KeyValuePair<SerializeFunc, SerializeFunc>>();
                _managedExecuteRpcFuncs = new List<ExecuteRPCFunc>();
#if FLAX_EDITOR
                FlaxEditor.ScriptsBuilder.ScriptsReloadBegin += ClearManagedReferences;
#endif
            }
            // Store the reference to prevent garbage collection
            _managedExecuteRpcFuncs.Add(execute);

            Internal_AddRPC(type, name, Marshal.GetFunctionPointerForDelegate(execute), isServer, isClient, channel);
        }
    }
}

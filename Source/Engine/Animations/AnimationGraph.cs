// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using FlaxEngine.Interop;

namespace FlaxEngine
{
    partial class AnimationGraph
    {
        /// <summary>
        /// The custom attribute that allows to specify the class that contains node archetype getter methods.
        /// </summary>
        /// <seealso cref="System.Attribute" />
        [AttributeUsage(AttributeTargets.Class)]
        public class CustomNodeArchetypeFactoryAttribute : Attribute
        {
        }

        /// <summary>
        /// Base class for all custom nodes. Allows to override it and define own Anim Graph nodes in game scripts or via plugins.
        /// </summary>
        /// <remarks>See official documentation to learn more how to use and create custom nodes in Anim Graph.</remarks>
        public abstract class CustomNode
        {
            /// <summary>
            /// The initial node data container structure.
            /// </summary>
            [StructLayout(LayoutKind.Sequential)]
            public struct InitData
            {
                /// <summary>
                /// The node values array. The first item is always the typename of the custom node type, second one is node group name, others are customizable by editor node archetype.
                /// </summary>
                public object[] Values;

                /// <summary>
                /// The skinned model asset that is a base model for the graph (source of the skeleton).
                /// </summary>
                public SkinnedModel BaseModel;
            }

            /// <summary>
            /// The node evaluation context structure.
            /// </summary>
            [StructLayout(LayoutKind.Sequential)]
            [NativeMarshalling(typeof(ContextMarshaller))]
            public struct Context
            {
                /// <summary>
                /// The graph pointer (unmanaged).
                /// </summary>
                public IntPtr Graph;

                /// <summary>
                /// The graph executor pointer (unmanaged).
                /// </summary>
                public IntPtr GraphExecutor;

                /// <summary>
                /// The node pointer (unmanaged).
                /// </summary>
                public IntPtr Node;

                /// <summary>
                /// The graph node identifier (unique per graph).
                /// </summary>
                public uint NodeId;

                /// <summary>
                /// The requested box identifier to evaluate its value.
                /// </summary>
                public int BoxId;

                /// <summary>
                /// The absolute time delta since last anim graph update for the current instance (in seconds). Can be used to animate or blend node logic over time.
                /// </summary>
                public float DeltaTime;

                /// <summary>
                /// The index of the current update frame. Can be used to detect if custom node hasn't been updated for more than one frame to reinitialize it in some cases.
                /// </summary>
                public ulong CurrentFrameIndex;

                /// <summary>
                /// The skinned model asset that is a base model for the graph (source of the skeleton).
                /// </summary>
                public SkinnedModel BaseModel;

                /// <summary>
                /// The instance of the animated model that during update.
                /// </summary>
                public AnimatedModel Instance;
            }

            [HideInEditor]
            [CustomMarshaller(typeof(Context), MarshalMode.Default, typeof(ContextMarshaller))]
            internal static class ContextMarshaller
            {
                [HideInEditor]
                [StructLayout(LayoutKind.Sequential)]
                public struct ContextNative
                {
                    public IntPtr Graph;
                    public IntPtr GraphExecutor;
                    public IntPtr Node;
                    public uint NodeId;
                    public int BoxId;
                    public float DeltaTime;
                    public ulong CurrentFrameIndex;
                    public IntPtr BaseModel;
                    public IntPtr Instance;
                }

                internal static Context ConvertToManaged(ContextNative unmanaged) => ToManaged(unmanaged);
                internal static ContextNative ConvertToUnmanaged(Context managed) => ToNative(managed);

                internal static Context ToManaged(ContextNative managed)
                {
                    return new Context
                    {
                        Graph = managed.Graph,
                        GraphExecutor = managed.GraphExecutor,
                        Node = managed.Node,
                        NodeId = managed.NodeId,
                        BoxId = managed.BoxId,
                        DeltaTime = managed.DeltaTime,
                        CurrentFrameIndex = managed.CurrentFrameIndex,
                        BaseModel = SkinnedModelMarshaller.ConvertToManaged(managed.BaseModel),
                        Instance = AnimatedModelMarshaller.ConvertToManaged(managed.Instance),
                    };
                }

                internal static ContextNative ToNative(Context managed)
                {
                    return new ContextNative
                    {
                        Graph = managed.Graph,
                        GraphExecutor = managed.GraphExecutor,
                        Node = managed.Node,
                        NodeId = managed.NodeId,
                        BoxId = managed.BoxId,
                        DeltaTime = managed.DeltaTime,
                        CurrentFrameIndex = managed.CurrentFrameIndex,
                        BaseModel = SkinnedModelMarshaller.ConvertToUnmanaged(managed.BaseModel),
                        Instance = AnimatedModelMarshaller.ConvertToUnmanaged(managed.Instance),
                    };
                }

                internal static void Free(ContextNative unmanaged)
                {
                }
            }

            /// <summary>
            /// The animation graph 'impulse' connections data container (the actual transfer is done via pointer as it gives better performance). 
            /// Container for skeleton nodes transformation hierarchy and any other required data. 
            /// Unified layout for both local and world transformation spaces.
            /// </summary>
            [StructLayout(LayoutKind.Sequential)]
            public unsafe struct Impulse
            {
                /// <summary>
                /// The nodes array size (elements count).
                /// </summary>
                public int NodesCount;

                /// <summary>
                /// The unused field.
                /// </summary>
                public int Unused;

                /// <summary>
                /// The skeleton nodes transformation hierarchy nodes. Size always matches the Anim Graph skeleton description (access size from <see cref="NodesCount"/>). It's pointer to the unmanaged allocation (read-only).
                /// </summary>
                public Transform* Nodes;

                /// <summary>
                /// The root motion data.
                /// </summary>
                public Transform RootMotion;

                /// <summary>
                /// The animation time position (in seconds).
                /// </summary>
                public float Position;

                /// <summary>
                /// The animation length (in seconds).
                /// </summary>
                public float Length;
            }

            /// <summary>
            /// Loads the node data from the serialized values and prepares the node to run. In most cases this method is called from the content loading thread (not the main game thread).
            /// </summary>
            public abstract void Load(ref InitData initData);

            /// <summary>
            /// Evaluates the node based on inputs and node data.
            /// </summary>
            /// <param name="context">The evaluation context.</param>
            /// <returns>The node value for the given context (node values, output box id, etc.).</returns>
            public abstract object Evaluate(ref Context context);

            /// <summary>
            /// Checks if th box of the given ID has valid connection to get its value.
            /// </summary>
            /// <param name="context">The context.</param>
            /// <param name="boxId">The input box identifier.</param>
            /// <returns>True if has connection, otherwise false.</returns>
            public static bool HasConnection(ref Context context, int boxId)
            {
                return Internal_HasConnection(ref context, boxId);
            }

            /// <summary>
            /// Gets the value of the input box of the given ID. Throws the exception if box has no valid connection.
            /// </summary>
            /// <param name="context">The context.</param>
            /// <param name="boxId">The input box identifier.</param>
            /// <returns>The value.</returns>
            public static object GetInputValue(ref Context context, int boxId)
            {
                return Internal_GetInputValue(ref context, boxId);
            }

            /// <summary>
            /// Gets the data for the output skeleton nodes hierarchy. Each node can have only one cached nodes output. Use this method if your node performs skeleton nodes modifications.
            /// </summary>
            /// <param name="context">The context.</param>
            /// <returns>The impulse data. It contains empty nodes hierarchy allocated per-node. Modify it to adjust output custom skeleton nodes transformations.</returns>
            public static unsafe Impulse* GetOutputImpulseData(ref Context context)
            {
                return (Impulse*)Internal_GetOutputImpulseData(ref context);
            }

            /// <summary>
            /// Copies the impulse data from the source to the destination container.
            /// </summary>
            /// <param name="destination">The destination data.</param>
            /// <param name="source">The source data.</param>
            public static unsafe void CopyImpulseData(Impulse* source, Impulse* destination)
            {
                if (source == null)
                    throw new ArgumentNullException(nameof(source));
                if (destination == null)
                    throw new ArgumentNullException(nameof(destination));
                if (source->NodesCount <= 0 || source->NodesCount > 4096)
                    throw new ArgumentOutOfRangeException(nameof(source));
                if (destination->NodesCount <= 0 || destination->NodesCount > 4096)
                    throw new ArgumentOutOfRangeException(nameof(destination));
                if (source->NodesCount != destination->NodesCount)
                    throw new ArgumentOutOfRangeException();
                destination->NodesCount = source->NodesCount;
                destination->Unused = source->Unused;
                Utils.MemoryCopy(new IntPtr(destination->Nodes), new IntPtr(source->Nodes), (ulong)(source->NodesCount * sizeof(Transform)));
                destination->RootMotion = source->RootMotion;
                destination->Position = source->Position;
                destination->Length = source->Length;
            }
        }

        #region Internal Calls

        [LibraryImport("FlaxEngine", EntryPoint = "AnimGraphInternal_HasConnection")]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_HasConnection(ref AnimationGraph.CustomNode.Context context, int boxId);

        [LibraryImport("FlaxEngine", EntryPoint = "AnimGraphInternal_GetInputValue")]
        [return: MarshalUsing(typeof(FlaxEngine.Interop.ManagedHandleMarshaller))]
        internal static partial object Internal_GetInputValue(ref AnimationGraph.CustomNode.Context context, int boxId);

        [LibraryImport("FlaxEngine", EntryPoint = "AnimGraphInternal_GetOutputImpulseData")]
        internal static partial IntPtr Internal_GetOutputImpulseData(ref AnimationGraph.CustomNode.Context context);

        #endregion
    }
}

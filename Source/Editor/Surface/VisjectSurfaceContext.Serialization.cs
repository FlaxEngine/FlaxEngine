// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Utilities;
using FlaxEngine;
using Utils = FlaxEditor.Utilities.Utils;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The missing node. Cached the node group, type and stored values information.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
    internal class MissingNode : SurfaceNode
    {
        /// <inheritdoc />
        public MissingNode(uint id, VisjectSurfaceContext context, ushort originalGroupId, ushort originalNodeId)
        : base(id, context, new NodeArchetype
        {
            TypeID = originalNodeId,
            Title = "Missing Node :(",
            Description = ":(",
            Flags = NodeFlags.AllGraphs,
            Size = new Vector2(200, 70),
            Elements = new NodeElementArchetype[0],
            DefaultValues = new object[32],
        }, new GroupArchetype
        {
            GroupID = originalGroupId,
            Name = string.Empty,
            Color = Color.Black,
            Archetypes = new NodeArchetype[0]
        })
        {
        }
    }

    /// <summary>
    /// The dummy custom node used to help custom surface nodes management (loading and layout preserving on missing type).
    /// </summary>
    internal class DummyCustomNode : SurfaceNode
    {
        /// <inheritdoc />
        public DummyCustomNode(uint id, VisjectSurfaceContext context)
        : base(id, context, new NodeArchetype(), new GroupArchetype())
        {
        }
    }

    public partial class VisjectSurfaceContext
    {
        // Note: surface serialization is port from c++ code base (also a legacy)
        // Refactor this in future together with c++ backend

        private struct ConnectionHint
        {
            public uint NodeA;
            public byte BoxA;
            public uint NodeB;
            public byte BoxB;
        }

        private readonly ThreadLocal<List<Box>> _cachedBoxes = new ThreadLocal<List<Box>>(() => new List<Box>());
        private readonly ThreadLocal<List<ConnectionHint>> _cachedConnections = new ThreadLocal<List<ConnectionHint>>(() => new List<ConnectionHint>());

        /// <summary>
        /// Loads the surface from bytes. Clears the surface before and uses context source data as a surface bytes source.
        /// </summary>
        /// <remarks>
        /// Assume this method does not throw exceptions but uses return value as a error code.
        /// </remarks>
        /// <returns>True if failed, otherwise false.</returns>
        public bool Load()
        {
            Surface._isUpdatingBoxTypes++;

            try
            {
                // Prepare
                Clear();

                Loading?.Invoke(this);

                // Load bytes
                var bytes = Context.SurfaceData;
                if (bytes == null)
                    throw new Exception("Failed to load surface data.");

                // Load graph (empty bytes data means empty graph for simplicity when using subgraphs)
                if (bytes.Length > 0)
                {
                    using (var stream = new MemoryStream(bytes))
                    using (var reader = new BinaryReader(stream))
                    {
                        LoadGraph(reader);
                    }
                }

                // Load surface meta
                var meta = _meta.GetEntry(10);
                if (meta.Data != null)
                {
                    Utils.ByteArrayToStructure(meta.Data, out CachedSurfaceMeta);
                }
                else
                {
                    // Reset view
                    CachedSurfaceMeta.ViewCenterPosition = Vector2.Zero;
                    CachedSurfaceMeta.Scale = 1.0f;
                }

                // [Deprecated on 04.07.2019] Load surface comments
                var commentsData = _meta.GetEntry(666);
                if (commentsData.Data != null)
                {
                    using (var stream = new MemoryStream(commentsData.Data))
                    using (var reader = new BinaryReader(stream))
                    {
                        var commentsCount = reader.ReadInt32();

                        for (int i = 0; i < commentsCount; i++)
                        {
                            var title = reader.ReadStr(71);
                            var color = new Color(reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle());
                            var bounds = new Rectangle(reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle());

                            var comment = SpawnComment(ref bounds, title, color);
                            if (comment == null)
                                throw new InvalidOperationException("Failed to create comment.");

                            OnControlLoaded(comment);
                        }
                    }
                }

                // Post load
                for (int i = 0; i < RootControl.Children.Count; i++)
                {
                    if (RootControl.Children[i] is SurfaceControl control)
                        control.OnSurfaceLoaded();
                }

                RootControl.UnlockChildrenRecursive();

                // Update boxes types for nodes that dependant box types based on incoming connections
                {
                    bool keepUpdating = false;
                    int updateLimit = 100;
                    do
                    {
                        for (int i = 0; i < RootControl.Children.Count; i++)
                        {
                            if (RootControl.Children[i] is SurfaceNode node && !node.HasDependentBoxesSetup)
                            {
                                node.UpdateBoxesTypes();
                                keepUpdating = true;
                            }
                        }
                    } while (keepUpdating && updateLimit-- > 0);
                }

                Loaded?.Invoke(this);

                // Clear modification flag
                _isModified = false;
            }
            catch (Exception ex)
            {
                // Error
                Editor.LogWarning("Loading Visject Surface data failed.");
                Editor.LogWarning(ex);
                return true;
            }
            finally
            {
                Surface._isUpdatingBoxTypes--;
            }

            return false;
        }

        /// <summary>
        /// Saves the surface to bytes. Performs also modified child surfaces saving before.
        /// </summary>
        /// <remarks>
        /// Assume this method does not throw exceptions but uses return value as a error code.
        /// </remarks>
        /// <returns>True if failed, otherwise false.</returns>
        public bool Save()
        {
            // Save all children modified before saving the current surface
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i].IsModified && Children[i].Save())
                    return true;
            }

            Saving?.Invoke(this);

            // Save surface meta
            _meta.AddEntry(10, Utils.StructureToByteArray(ref CachedSurfaceMeta));

            // Save all nodes meta
            VisjectSurface.Meta11 meta11;
            for (int i = 0; i < Nodes.Count; i++)
            {
                var node = Nodes[i];
                meta11.Position = node.Location;
                meta11.Selected = false; // don't save selection to prevent stupid binary diffs on asset
                // TODO: reuse byte[] array for all nodes to reduce dynamic memory allocations
                node.Meta.AddEntry(11, Utils.StructureToByteArray(ref meta11));
            }

            // Save graph
            try
            {
                // Save graph
                using (var stream = new MemoryStream())
                using (var writer = new BinaryWriter(stream))
                {
                    // Save graph to bytes
                    SaveGraph(writer);
                    var bytes = stream.ToArray();

                    // Send data to the container
                    Context.SurfaceData = bytes;

                    Saved?.Invoke(this);

                    // Clear modification flag
                    _isModified = false;
                }
            }
            catch (Exception ex)
            {
                // Error
                Editor.LogWarning("Saving Visject Surface data failed.");
                Editor.LogWarning(ex);
                return true;
            }

            return false;
        }

        // [Deprecated on 31.07.2020, expires on 31.07.2022]
        internal enum GraphParamType_Deprecated
        {
            Bool = 0,
            Integer = 1,
            Float = 2,
            Vector2 = 3,
            Vector3 = 4,
            Vector4 = 5,
            Color = 6,
            Texture = 7,
            NormalMap = 8,
            String = 9,
            Box = 10,
            Rotation = 11,
            Transform = 12,
            Asset = 13,
            Actor = 14,
            Rectangle = 15,
            CubeTexture = 16,
            SceneTexture = 17,
            GPUTexture = 18,
            Matrix = 19,
            GPUTextureArray = 20,
            GPUTextureVolume = 21,
            GPUTextureCube = 22,
            ChannelMask = 23,
        };

        // [Deprecated on 31.07.2020, expires on 31.07.2022]
        internal enum GraphConnectionType_Deprecated : uint
        {
            Invalid = 0,
            Impulse = 1 << 0,
            Bool = 1 << 1,
            Integer = 1 << 2,
            Float = 1 << 3,
            Vector2 = 1 << 4,
            Vector3 = 1 << 5,
            Vector4 = 1 << 6,
            String = 1 << 7,
            Object = 1 << 8,
            Rotation = 1 << 9,
            Transform = 1 << 10,
            Box = 1 << 11,
            ImpulseSecondary = 1 << 12,
            UnsignedInteger = 1 << 13,
            Scalar = Bool | Integer | Float | UnsignedInteger,
            Vector = Vector2 | Vector3 | Vector4,
            Variable = Scalar | Vector | String | Object | Rotation | Transform | Box,
            All = Variable | Impulse,
        };

        internal static Type GetGraphParameterValueType(GraphParamType_Deprecated type)
        {
            // [Deprecated on 31.07.2020, expires on 31.07.2022]
            switch (type)
            {
            case GraphParamType_Deprecated.Bool: return typeof(bool);
            case GraphParamType_Deprecated.Integer: return typeof(int);
            case GraphParamType_Deprecated.Float: return typeof(float);
            case GraphParamType_Deprecated.Vector2: return typeof(Vector2);
            case GraphParamType_Deprecated.Vector3: return typeof(Vector3);
            case GraphParamType_Deprecated.Vector4: return typeof(Vector4);
            case GraphParamType_Deprecated.Color: return typeof(Color);
            case GraphParamType_Deprecated.Texture: return typeof(Texture);
            case GraphParamType_Deprecated.NormalMap: return typeof(Texture);
            case GraphParamType_Deprecated.String: return typeof(string);
            case GraphParamType_Deprecated.Box: return typeof(BoundingBox);
            case GraphParamType_Deprecated.Rotation: return typeof(Quaternion);
            case GraphParamType_Deprecated.Transform: return typeof(Transform);
            case GraphParamType_Deprecated.Asset: return typeof(Asset);
            case GraphParamType_Deprecated.Actor: return typeof(Actor);
            case GraphParamType_Deprecated.Rectangle: return typeof(Rectangle);
            case GraphParamType_Deprecated.CubeTexture: return typeof(CubeTexture);
            case GraphParamType_Deprecated.SceneTexture: return typeof(MaterialSceneTextures);
            case GraphParamType_Deprecated.GPUTexture: return typeof(GPUTexture);
            case GraphParamType_Deprecated.Matrix: return typeof(Matrix);
            case GraphParamType_Deprecated.GPUTextureArray: return typeof(GPUTexture);
            case GraphParamType_Deprecated.GPUTextureVolume: return typeof(GPUTexture);
            case GraphParamType_Deprecated.GPUTextureCube: return typeof(GPUTexture);
            case GraphParamType_Deprecated.ChannelMask: return typeof(ChannelMask);
            default: throw new ArgumentOutOfRangeException(nameof(type), type, null);
            }
        }

        internal static Type GetGraphConnectionType(GraphConnectionType_Deprecated connectionType)
        {
            // [Deprecated on 31.07.2020, expires on 31.07.2022]
            switch (connectionType)
            {
            case GraphConnectionType_Deprecated.Impulse: return typeof(void);
            case GraphConnectionType_Deprecated.Bool: return typeof(bool);
            case GraphConnectionType_Deprecated.Integer: return typeof(int);
            case GraphConnectionType_Deprecated.Float: return typeof(float);
            case GraphConnectionType_Deprecated.Vector2: return typeof(Vector2);
            case GraphConnectionType_Deprecated.Vector3: return typeof(Vector3);
            case GraphConnectionType_Deprecated.Vector4: return typeof(Vector4);
            case GraphConnectionType_Deprecated.String: return typeof(string);
            case GraphConnectionType_Deprecated.Object: return typeof(FlaxEngine.Object);
            case GraphConnectionType_Deprecated.Rotation: return typeof(Quaternion);
            case GraphConnectionType_Deprecated.Transform: return typeof(Transform);
            case GraphConnectionType_Deprecated.Box: return typeof(BoundingBox);
            case GraphConnectionType_Deprecated.ImpulseSecondary: return typeof(void);
            case GraphConnectionType_Deprecated.UnsignedInteger: return typeof(uint);
            default: return null;
            }
        }

        private void SaveGraph(BinaryWriter stream)
        {
            // Magic Code
            stream.Write(1963542358);

            // Version
            stream.Write(7000);

            // Nodes count
            stream.Write(Nodes.Count);

            // Parameters count
            stream.Write(Parameters.Count);

            // For each node
            for (int i = 0; i < Nodes.Count; i++)
            {
                var node = Nodes[i];
                stream.Write(node.ID);
                stream.Write(node.Type);
            }

            // For each param
            for (int i = 0; i < Parameters.Count; i++)
            {
                var param = Parameters[i];
                stream.WriteVariantType(param.Type);
                stream.WriteGuid(ref param.ID);
                stream.WriteStr(param.Name, 97);
                stream.Write((byte)(param.IsPublic ? 1 : 0));
                stream.WriteVariant(param.Value);
                param.Meta.Save(stream);
            }

            // For each node
            var boxes = _cachedBoxes.Value;
            boxes.Clear();
            for (int i = 0; i < Nodes.Count; i++)
            {
                var node = Nodes[i];

                // Values
                if (node.Values != null)
                {
                    stream.Write(node.Values.Length);
                    for (int j = 0; j < node.Values.Length; j++)
                        stream.WriteVariant(node.Values[j]);
                }
                else
                {
                    stream.Write(0);
                }

                // Boxes
                node.GetBoxes(boxes);
                stream.Write((ushort)boxes.Count);
                for (int j = 0; j < boxes.Count; j++)
                {
                    var box = boxes[j];

                    stream.Write((byte)box.ID);
                    stream.WriteVariantType(box.DefaultType);
                    stream.Write((ushort)box.Connections.Count);
                    for (int k = 0; k < box.Connections.Count; k++)
                    {
                        var targetBox = box.Connections[k];
                        if (targetBox == null)
                            throw new Exception("Missing target box.");
                        stream.Write(targetBox.ParentNode.ID);
                        stream.Write((byte)targetBox.ID);
                    }
                }

                // Meta
                node.Meta.Save(stream);
            }
            boxes.Clear();

            // Visject Meta
            _meta.Save(stream);

            // Ending char
            stream.Write((byte)'\t');
        }

        private void LoadGraph(BinaryReader stream)
        {
            // IMPORTANT! This must match C++ Graph format

            // Magic Code
            int tmp = stream.ReadInt32();
            if (tmp != 1963542358)
            {
                // Error
                throw new Exception("Invalid Graph format version");
            }

            // Version
            var version = stream.ReadUInt32();
            var tmpHints = _cachedConnections.Value;
            var guidBytes = new byte[16];
            if (version < 7000)
            {
                // Time saved (not used anymore to prevent binary diffs after saving unmodified surface)
                stream.ReadInt64();

                // Nodes count
                int nodesCount = stream.ReadInt32();
                if (Nodes.Capacity < nodesCount)
                    Nodes.Capacity = nodesCount;
                tmpHints.Clear();
                tmpHints.Capacity = Mathf.Max(tmpHints.Capacity, nodesCount * 4);

                // Parameters count
                int parametersCount = stream.ReadInt32();
                if (Parameters.Capacity < parametersCount)
                    Parameters.Capacity = parametersCount;

                // For each node
                for (int i = 0; i < nodesCount; i++)
                {
                    // ID
                    uint id = stream.ReadUInt32();

                    // Type
                    ushort typeId = stream.ReadUInt16();
                    ushort groupId = stream.ReadUInt16();

                    // Create node
                    SurfaceNode node;
                    if (groupId == Archetypes.Custom.GroupID)
                        node = new DummyCustomNode(id, this);
                    else
                        node = NodeFactory.CreateNode(_surface.NodeArchetypes, id, this, groupId, typeId);
                    if (node == null)
                        node = new MissingNode(id, this, groupId, typeId);
                    Nodes.Add(node);
                }

                // For each param
                for (int i = 0; i < parametersCount; i++)
                {
                    // Create param
                    var param = new SurfaceParameter();
                    Parameters.Add(param);

                    // Properties
                    param.Type = new ScriptType(GetGraphParameterValueType((GraphParamType_Deprecated)stream.ReadByte()));
                    stream.Read(guidBytes, 0, 16);
                    param.ID = new Guid(guidBytes);
                    param.Name = stream.ReadStr(97);
                    param.IsPublic = stream.ReadByte() != 0;
                    bool isStatic = stream.ReadByte() != 0;
                    bool isUIVisible = stream.ReadByte() != 0;
                    bool isUIEditable = stream.ReadByte() != 0;

                    // References [Deprecated]
                    int refsCount = stream.ReadInt32();
                    for (int j = 0; j < refsCount; j++)
                    {
                        uint refID = stream.ReadUInt32();
                    }

                    // Value
                    stream.ReadCommonValue(ref param.Value);

                    // Meta
                    param.Meta.Load(stream);
                }

                // For each node
                for (int i = 0; i < nodesCount; i++)
                {
                    var node = Nodes[i];

                    int valuesCnt = stream.ReadInt32();
                    int firstValueReadIdx = 0;

                    // Special case for missing nodes
                    if (node is DummyCustomNode customNode)
                    {
                        node = null;

                        // Values check
                        if (valuesCnt < 2)
                            throw new Exception("Missing custom nodes data.");

                        // Node typename check
                        object typeNameValue = null;
                        stream.ReadCommonValue(ref typeNameValue);
                        firstValueReadIdx = 1;
                        string typeName = typeNameValue as string ?? string.Empty;

                        // Find custom node archetype that matches this node type (it must be unique)
                        var customNodes = _surface.GetCustomNodes();
                        if (customNodes?.Archetypes != null && typeName.Length != 0)
                        {
                            NodeArchetype arch = null;
                            foreach (var nodeArchetype in customNodes.Archetypes)
                            {
                                if (string.Equals(Archetypes.Custom.GetNodeTypeName(nodeArchetype), typeName, StringComparison.OrdinalIgnoreCase))
                                {
                                    arch = nodeArchetype;
                                    break;
                                }
                            }
                            if (arch != null)
                                node = NodeFactory.CreateNode(customNode.ID, this, customNodes, arch);
                        }

                        // Fallback to the
                        if (node == null)
                        {
                            Editor.LogWarning(string.Format("Cannot find custom node archetype for {0}", typeName));
                            node = new MissingNode(customNode.ID, this, Archetypes.Custom.GroupID, customNode.Archetype.TypeID);
                        }
                        Nodes[i] = node;

                        // Store node typename in values container
                        node.Values[0] = typeName;
                    }
                    if (node is MissingNode)
                    {
                        // Read all values
                        Array.Resize(ref node.Values, valuesCnt);
                        for (int j = firstValueReadIdx; j < valuesCnt; j++)
                        {
                            // ReSharper disable once PossibleNullReferenceException
                            stream.ReadCommonValue(ref node.Values[j]);
                        }
                        firstValueReadIdx = valuesCnt = node.Values.Length;
                    }

                    // Values
                    int nodeValuesCnt = node.Values?.Length ?? 0;
                    if (valuesCnt == nodeValuesCnt)
                    {
                        for (int j = firstValueReadIdx; j < valuesCnt; j++)
                        {
                            // ReSharper disable once PossibleNullReferenceException
                            stream.ReadCommonValue(ref node.Values[j]);
                        }
                    }
                    else
                    {
                        Editor.LogWarning(string.Format("Invalid node values. Loaded: {0}, expected: {1}. Type: {2}, {3}", valuesCnt, nodeValuesCnt, node.Archetype.Title, node.Archetype.TypeID));

                        object dummy = null;
                        for (int j = firstValueReadIdx; j < valuesCnt; j++)
                        {
                            stream.ReadCommonValue(ref dummy);

                            if (j < nodeValuesCnt &&
                                dummy != null &&
                                node.Values[j] != null &&
                                node.Values[j].GetType() == dummy.GetType())
                            {
                                node.Values[j] = dummy;
                            }
                        }
                    }

                    // Boxes
                    ushort boxesCount = stream.ReadUInt16();
                    for (int j = 0; j < boxesCount; j++)
                    {
                        var id = stream.ReadByte();
                        stream.ReadUInt32(); // Skip type
                        ushort connectionsCnt = stream.ReadUInt16();

                        ConnectionHint hint;
                        hint.NodeB = node.ID;
                        hint.BoxB = id;

                        for (int k = 0; k < connectionsCnt; k++)
                        {
                            uint targetNodeID = stream.ReadUInt32();
                            byte targetBoxID = stream.ReadByte();

                            hint.NodeA = targetNodeID;
                            hint.BoxA = targetBoxID;

                            tmpHints.Add(hint);
                        }
                    }

                    // Meta
                    node.Meta.Load(stream);

                    OnControlLoaded(node);
                }
            }
            else if (version == 7000)
            {
                // Nodes count
                int nodesCount = stream.ReadInt32();
                if (Nodes.Capacity < nodesCount)
                    Nodes.Capacity = nodesCount;
                tmpHints.Clear();
                tmpHints.Capacity = Mathf.Max(tmpHints.Capacity, nodesCount * 4);

                // Parameters count
                int parametersCount = stream.ReadInt32();
                if (Parameters.Capacity < parametersCount)
                    Parameters.Capacity = parametersCount;

                // For each node
                for (int i = 0; i < nodesCount; i++)
                {
                    uint id = stream.ReadUInt32();
                    ushort typeId = stream.ReadUInt16();
                    ushort groupId = stream.ReadUInt16();

                    // Create node
                    SurfaceNode node;
                    if (groupId == Archetypes.Custom.GroupID)
                        node = new DummyCustomNode(id, this);
                    else
                        node = NodeFactory.CreateNode(_surface.NodeArchetypes, id, this, groupId, typeId);
                    if (node == null)
                        node = new MissingNode(id, this, groupId, typeId);
                    Nodes.Add(node);
                }

                // For each param
                for (int i = 0; i < parametersCount; i++)
                {
                    // Create param
                    var param = new SurfaceParameter();
                    Parameters.Add(param);
                    param.Type = stream.ReadVariantScriptType();
                    stream.Read(guidBytes, 0, 16);
                    param.ID = new Guid(guidBytes);
                    param.Name = stream.ReadStr(97);
                    param.IsPublic = stream.ReadByte() != 0;

                    // Value
                    param.Value = stream.ReadVariant();

                    // Meta
                    param.Meta.Load(stream);
                }

                // For each node
                for (int i = 0; i < nodesCount; i++)
                {
                    var node = Nodes[i];

                    int valuesCnt = stream.ReadInt32();
                    int firstValueReadIdx = 0;

                    // Special case for missing nodes
                    if (node is DummyCustomNode customNode)
                    {
                        node = null;

                        // Values check
                        if (valuesCnt < 2)
                            throw new Exception("Missing custom nodes data.");

                        // Node typename check
                        object typeNameValue = stream.ReadVariant();
                        firstValueReadIdx = 1;
                        string typeName = typeNameValue as string ?? string.Empty;

                        // Find custom node archetype that matches this node type (it must be unique)
                        var customNodes = _surface.GetCustomNodes();
                        if (customNodes?.Archetypes != null && typeName.Length != 0)
                        {
                            NodeArchetype arch = null;
                            foreach (var nodeArchetype in customNodes.Archetypes)
                            {
                                if (string.Equals(Archetypes.Custom.GetNodeTypeName(nodeArchetype), typeName, StringComparison.OrdinalIgnoreCase))
                                {
                                    arch = nodeArchetype;
                                    break;
                                }
                            }
                            if (arch != null)
                                node = NodeFactory.CreateNode(customNode.ID, this, customNodes, arch);
                        }

                        // Fallback to the
                        if (node == null)
                        {
                            Editor.LogWarning(string.Format("Cannot find custom node archetype for {0}", typeName));
                            node = new MissingNode(customNode.ID, this, Archetypes.Custom.GroupID, customNode.Archetype.TypeID);
                        }
                        Nodes[i] = node;

                        // Store node typename in values container
                        node.Values[0] = typeName;
                    }
                    if (node is MissingNode)
                    {
                        // Read all values
                        Array.Resize(ref node.Values, valuesCnt);
                        for (int j = firstValueReadIdx; j < valuesCnt; j++)
                            node.Values[j] = stream.ReadVariant();
                        firstValueReadIdx = valuesCnt = node.Values.Length;
                    }

                    // Values
                    int nodeValuesCnt = node.Values?.Length ?? 0;
                    if (valuesCnt == nodeValuesCnt)
                    {
                        for (int j = firstValueReadIdx; j < valuesCnt; j++)
                            node.Values[j] = stream.ReadVariant();
                    }
                    else
                    {
                        Editor.LogWarning(string.Format("Invalid node values. Loaded: {0}, expected: {1}. Type: {2}, {3}", valuesCnt, nodeValuesCnt, node.Archetype.Title, node.Archetype.TypeID));

                        object dummy;
                        for (int j = firstValueReadIdx; j < valuesCnt; j++)
                        {
                            dummy = stream.ReadVariant();

                            if (j < nodeValuesCnt &&
                                dummy != null &&
                                node.Values[j] != null &&
                                node.Values[j].GetType() == dummy.GetType())
                            {
                                node.Values[j] = dummy;
                            }
                        }
                    }

                    // Boxes
                    ushort boxesCount = stream.ReadUInt16();
                    for (int j = 0; j < boxesCount; j++)
                    {
                        var id = stream.ReadByte();
                        stream.ReadVariantType(); // Skip type
                        var connectionsCnt = stream.ReadUInt16();

                        ConnectionHint hint;
                        hint.NodeB = node.ID;
                        hint.BoxB = id;

                        for (int k = 0; k < connectionsCnt; k++)
                        {
                            uint targetNodeID = stream.ReadUInt32();
                            byte targetBoxID = stream.ReadByte();
                            hint.NodeA = targetNodeID;
                            hint.BoxA = targetBoxID;
                            tmpHints.Add(hint);
                        }
                    }

                    // Meta
                    node.Meta.Load(stream);

                    OnControlLoaded(node);
                }
            }
            else
            {
                throw new Exception($"Unsupported graph version {version}.");
            }

            // Visject Meta
            _meta.Load(stream);

            // Setup connections
            for (int i = 0; i < tmpHints.Count; i++)
            {
                var c = tmpHints[i];

                var nodeA = FindNode(c.NodeA);
                var nodeB = FindNode(c.NodeB);
                if (nodeA == null || nodeB == null)
                {
                    // Error
                    Editor.LogWarning("Invalid connected node id.");
                    continue;
                }

                var boxA = nodeA.GetBox(c.BoxA);
                var boxB = nodeB.GetBox(c.BoxB);
                if (boxA != null && boxB != null)
                {
                    boxA.Connections.Add(boxB);
                }
            }

            // Ending char
            byte end = stream.ReadByte();
            if (end != '\t')
                throw new Exception("Invalid data.");
        }

        /// <summary>
        /// Called when control gets added to the surface as spawn operation (eg. add new comment or add new node).
        /// </summary>
        /// <param name="control">The control.</param>
        public virtual void OnControlSpawned(SurfaceControl control)
        {
            control.OnSpawned();
            ControlSpawned?.Invoke(control);
            if (control is SurfaceNode node)
                Surface.OnNodeSpawned(node);
        }

        /// <summary>
        /// Called when control gets removed from the surface as delete/cut operation (eg. remove comment or cut node).
        /// </summary>
        /// <param name="control">The control.</param>
        public virtual void OnControlDeleted(SurfaceControl control)
        {
            ControlDeleted?.Invoke(control);
            control.OnDeleted();
            if (control is SurfaceNode node)
                Surface.OnNodeDeleted(node);
        }

        /// <summary>
        /// Called when control gets loaded and should be added to the surface. Handles surface nodes initialization.
        /// </summary>
        /// <param name="control">The control.</param>
        public virtual void OnControlLoaded(SurfaceControl control)
        {
            if (control is SurfaceNode node)
            {
                // Initialize node
                OnNodeLoaded(node);
            }

            // Link control
            control.OnLoaded();
            control.Parent = RootControl;

            if (control is SurfaceComment)
            {
                // Move comments to the background
                control.IndexInParent = 0;
            }
        }

        /// <summary>
        /// Called when node gets loaded and should be added to the surface. Creates node elements from the archetype.
        /// </summary>
        /// <param name="node">The node.</param>
        public virtual void OnNodeLoaded(SurfaceNode node)
        {
            // Create child elements of the node based on it's archetype
            int elementsCount = node.Archetype.Elements?.Length ?? 0;
            for (int i = 0; i < elementsCount; i++)
            {
                // ReSharper disable once PossibleNullReferenceException
                node.AddElement(node.Archetype.Elements[i]);
            }

            // Load metadata
            var meta = node.Meta.GetEntry(11);
            if (meta.Data != null)
            {
                var meta11 = Utils.ByteArrayToStructure<VisjectSurface.Meta11>(meta.Data);
                node.Location = meta11.Position;
                //node.IsSelected = meta11.Selected;
            }
        }
    }
}

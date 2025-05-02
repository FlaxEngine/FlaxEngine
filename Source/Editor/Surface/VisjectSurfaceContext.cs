// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Surface.Undo;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Visject Surface visual representation context. Contains context and deserialized graph data.
    /// </summary>
    [HideInEditor]
    public partial class VisjectSurfaceContext
    {
        /// <summary>
        /// Visject context delegate type.
        /// </summary>
        /// <param name="context">The context.</param>
        public delegate void ContextDelegate(VisjectSurfaceContext context);

        /// <summary>
        /// Visject context modification delegate type.
        /// </summary>
        /// <param name="context">The context.</param>
        /// <param name="graphEdited">True if graph has been edited (nodes structure or parameter value). Otherwise just UI elements has been modified (node moved, comment resized).</param>
        public delegate void ContextModifiedDelegate(VisjectSurfaceContext context, bool graphEdited);

        private bool _isModified;
        private VisjectSurface _surface;
        private SurfaceMeta _meta = new SurfaceMeta();
        internal Float2 _cachedViewCenterPosition;
        internal float _cachedViewScale = -1; // Negative scale to indicate missing data (will show whole surface on start)

        /// <summary>
        /// The parent context. Defines the higher key surface graph context. May be null for the top-level context.
        /// </summary>
        public readonly VisjectSurfaceContext Parent;

        /// <summary>
        /// The children of this context (loaded and opened in editor only).
        /// </summary>
        public readonly List<VisjectSurfaceContext> Children = new List<VisjectSurfaceContext>();

        /// <summary>
        /// The context.
        /// </summary>
        public readonly ISurfaceContext Context;

        /// <summary>
        /// The root control for the GUI. Used to navigate around the view (scale and move it). Contains all surface controls including nodes and comments.
        /// </summary>
        public readonly SurfaceRootControl RootControl;

        /// <summary>
        /// The nodes collection. Read-only.
        /// </summary>
        public readonly List<SurfaceNode> Nodes = new List<SurfaceNode>(64);

        /// <summary>
        /// The collection of the surface parameters.
        /// </summary>
        public readonly List<SurfaceParameter> Parameters = new List<SurfaceParameter>();

        /// <summary>
        /// Gets the meta for the surface context.
        /// </summary>
        public SurfaceMeta Meta => _meta;

        /// <summary>
        /// Gets the list of the surface comments.
        /// </summary>
        /// <remarks>
        /// Don't call it too often. It does memory allocation and iterates over the surface controls to find comments in the graph.
        /// </remarks>
        public List<SurfaceComment> Comments
        {
            get
            {
                var result = new List<SurfaceComment>();
                for (int i = 0; i < RootControl.Children.Count; i++)
                {
                    if (RootControl.Children[i] is SurfaceComment comment)
                        result.Add(comment);
                }
                return result;
            }
        }

        /// <summary>
        /// Gets the amount of surface comments
        /// </summary>
        /// <remarks>
        /// This is used as an alternative to <see cref="Comments"/>, if only the amount of comments is important.
        /// Is faster and doesn't allocate as much memory
        /// </remarks>
        public int CommentCount
        {
            get
            {
                int count = 0;
                for (int i = 0; i < RootControl.Children.Count; i++)
                {
                    if (RootControl.Children[i] is SurfaceComment)
                        count++;
                }
                return count;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this context is modified (needs saving and flushing with surface data context source).
        /// </summary>
        public bool IsModified => _isModified;

        /// <summary>
        /// Gets the parent Visject surface.
        /// </summary>
        public VisjectSurface Surface => _surface;

        /// <summary>
        /// The surface meta (cached after opening the context, used to store it back into the data container).
        /// </summary>
        internal VisjectSurface.Meta10 CachedSurfaceMeta;

        /// <summary>
        /// Occurs when surface starts saving graph to bytes. Can be used to inject or cleanup surface data.
        /// </summary>
        public event ContextDelegate Saving;

        /// <summary>
        /// Occurs when surface ends saving graph to bytes. Can be used to inject or cleanup surface data.
        /// </summary>
        public event ContextDelegate Saved;

        /// <summary>
        /// Occurs when surface starts loading graph from data.
        /// </summary>
        public event ContextDelegate Loading;

        /// <summary>
        /// Occurs when surface graph gets loaded from data. Can be used to post-process it or perform validation.
        /// </summary>
        public event ContextDelegate Loaded;

        /// <summary>
        /// Occurs when surface gets modified (graph edited, node moved, comment resized).
        /// </summary>
        public event ContextModifiedDelegate Modified;

        /// <summary>
        /// Occurs when node gets added to the surface as spawn operation (eg. add new comment or add new node).
        /// </summary>
        public event Action<SurfaceControl> ControlSpawned;

        /// <summary>
        /// Occurs when node gets removed from the surface as delete/cut operation (eg. remove comment or cut node).
        /// </summary>
        public event Action<SurfaceControl> ControlDeleted;

        /// <summary>
        /// Identifier of the node that 'owns' this context (eg. State Machine which created this graph of state nodes).
        /// </summary>
        public uint OwnerNodeID;

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectSurfaceContext"/> class.
        /// </summary>
        /// <param name="surface">The Visject surface using this context.</param>
        /// <param name="parent">The parent context. Defines the higher key surface graph context. May be null for the top-level context.</param>
        /// <param name="context">The context.</param>
        public VisjectSurfaceContext(VisjectSurface surface, VisjectSurfaceContext parent, ISurfaceContext context)
        : this(surface, parent, context, new SurfaceRootControl())
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectSurfaceContext"/> class.
        /// </summary>
        /// <param name="surface">The Visject surface using this context.</param>
        /// <param name="parent">The parent context. Defines the higher key surface graph context. May be null for the top-level context.</param>
        /// <param name="context">The context.</param>
        /// <param name="rootControl">The surface root control.</param>
        public VisjectSurfaceContext(VisjectSurface surface, VisjectSurfaceContext parent, ISurfaceContext context, SurfaceRootControl rootControl)
        {
            _surface = surface;
            Parent = parent;
            Context = context ?? throw new ArgumentNullException(nameof(context));
            RootControl = rootControl ?? throw new ArgumentNullException(nameof(rootControl));

            // Set initial scale to provide nice zoom in effect on startup
            RootControl.Scale = new Float2(0.5f);
        }

        /// <summary>
        /// Finds the node of the given type.
        /// </summary>
        /// <param name="groupId">The group identifier.</param>
        /// <param name="typeId">The type identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        public SurfaceNode FindNode(ushort groupId, ushort typeId)
        {
            SurfaceNode result = null;
            uint type = ((uint)groupId << 16) | typeId;
            for (int i = 0; i < Nodes.Count; i++)
            {
                var node = Nodes[i];
                if (node.Type == type)
                {
                    result = node;
                    break;
                }
            }
            return result;
        }

        /// <summary>
        /// Finds the node with the given ID.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        public SurfaceNode FindNode(int id)
        {
            if (id < 0)
                return null;
            return FindNode((uint)id);
        }

        /// <summary>
        /// Finds the node with the given ID.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        public SurfaceNode FindNode(uint id)
        {
            SurfaceNode result = null;
            for (int i = 0; i < Nodes.Count; i++)
            {
                var node = Nodes[i];
                if (node.ID == id)
                {
                    result = node;
                    break;
                }
            }
            return result;
        }

        /// <summary>
        /// Gets the parameter by the given ID.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>Found parameter instance or null if missing.</returns>
        public SurfaceParameter GetParameter(Guid id)
        {
            SurfaceParameter result = null;
            for (int i = 0; i < Parameters.Count; i++)
            {
                var parameter = Parameters[i];
                if (parameter.ID == id)
                {
                    result = parameter;
                    break;
                }
            }
            return result;
        }

        /// <summary>
        /// Gets the parameter by the given name.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>Found parameter instance or null if missing.</returns>
        public SurfaceParameter GetParameter(string name)
        {
            SurfaceParameter result = null;
            for (int i = 0; i < Parameters.Count; i++)
            {
                var parameter = Parameters[i];
                if (parameter.Name == name)
                {
                    result = parameter;
                    break;
                }
            }
            return result;
        }

        private uint GetFreeNodeID()
        {
            uint result = 1;
            while (true)
            {
                bool valid = true;
                for (int i = 0; i < Nodes.Count; i++)
                {
                    if (Nodes[i].ID == result)
                    {
                        result++;
                        valid = false;
                        break;
                    }
                }
                if (valid)
                    break;
            }
            return result;
        }

        /// <summary>
        /// Spawns the comment object. Used by the <see cref="CreateComment"/> and loading method. Can be overriden to provide custom comment object implementations.
        /// </summary>
        /// <param name="surfaceArea">The surface area to create comment.</param>
        /// <param name="title">The comment title.</param>
        /// <param name="color">The comment color.</param>
        /// <param name="customOrder">The comment order or -1 to use default.</param>
        /// <returns>The comment object</returns>
        public virtual SurfaceComment SpawnComment(ref Rectangle surfaceArea, string title, Color color, int customOrder = -1)
        {
            var values = new object[]
            {
                title, // Title
                color, // Color
                surfaceArea.Size, // Size
                customOrder, // Order
            };
            return (SurfaceComment)SpawnNode(7, 11, surfaceArea.Location, values);
        }

        /// <summary>
        /// Creates the comment.
        /// </summary>
        /// <param name="surfaceArea">The surface area to create comment.</param>
        /// <param name="title">The comment title.</param>
        /// <param name="color">The comment color.</param>
        /// <param name="customOrder">The comment order or -1 to use default.</param>
        /// <returns>The comment object</returns>
        public SurfaceComment CreateComment(ref Rectangle surfaceArea, string title, Color color, int customOrder = -1)
        {
            // Create comment
            var comment = SpawnComment(ref surfaceArea, title, color, customOrder);
            if (comment == null)
            {
                Editor.LogWarning("Failed to create comment.");
                return null;
            }

            // Initialize
            OnControlLoaded(comment, SurfaceNodeActions.User);
            comment.OnSurfaceLoaded(SurfaceNodeActions.User);
            OnControlSpawned(comment, SurfaceNodeActions.User);

            MarkAsModified();

            return comment;
        }

        /// <summary>
        /// Spawns the node.
        /// </summary>
        /// <param name="groupID">The group archetype ID.</param>
        /// <param name="typeID">The node archetype ID.</param>
        /// <param name="location">The location.</param>
        /// <param name="customValues">The custom values array. Must match node archetype <see cref="NodeArchetype.DefaultValues"/> size. Pass null to use default values.</param>
        /// <param name="beforeSpawned">The custom callback action to call after node creation but just before invoking spawn event. Can be used to initialize custom node data.</param>
        /// <returns>Created node.</returns>
        public SurfaceNode SpawnNode(ushort groupID, ushort typeID, Float2 location, object[] customValues = null, Action<SurfaceNode> beforeSpawned = null)
        {
            var nodeArchetypes = _surface?.NodeArchetypes ?? NodeFactory.DefaultGroups;
            if (NodeFactory.GetArchetype(nodeArchetypes, groupID, typeID, out var groupArchetype, out var nodeArchetype))
            {
                return SpawnNode(groupArchetype, nodeArchetype, location, customValues, beforeSpawned);
            }
            return null;
        }

        /// <summary>
        /// Spawns the node.
        /// </summary>
        /// <param name="groupArchetype">The group archetype.</param>
        /// <param name="nodeArchetype">The node archetype.</param>
        /// <param name="location">The location.</param>
        /// <param name="customValues">The custom values array. Must match node archetype <see cref="NodeArchetype.DefaultValues"/> size. Pass null to use default values.</param>
        /// <param name="beforeSpawned">The custom callback action to call after node creation but just before invoking spawn event. Can be used to initialize custom node data.</param>
        /// <returns>Created node.</returns>
        public SurfaceNode SpawnNode(GroupArchetype groupArchetype, NodeArchetype nodeArchetype, Float2 location, object[] customValues = null, Action<SurfaceNode> beforeSpawned = null)
        {
            if (groupArchetype == null || nodeArchetype == null)
                throw new ArgumentNullException();

            // Check if cannot use that node in this surface type (ignore NoSpawnViaGUI)
            var flags = nodeArchetype.Flags;
            nodeArchetype.Flags &= ~NodeFlags.NoSpawnViaGUI;
            nodeArchetype.Flags &= ~NodeFlags.NoSpawnViaPaste;
            if (_surface != null && !_surface.CanUseNodeType(groupArchetype, nodeArchetype))
            {
                nodeArchetype.Flags = flags;
                Editor.LogWarning("Cannot spawn given node type. Title: " + nodeArchetype.Title);
                return null;
            }
            nodeArchetype.Flags = flags;

            var id = GetFreeNodeID();

            // Create node
            var node = NodeFactory.CreateNode(id, this, groupArchetype, nodeArchetype);
            if (node == null)
            {
                Editor.LogWarning("Failed to create node.");
                return null;
            }
            Nodes.Add(node);

            // Initialize
            if (customValues != null)
            {
                if (node.Values != null && node.Values.Length == customValues.Length)
                    Array.Copy(customValues, node.Values, customValues.Length);
                else
                    throw new InvalidOperationException("Invalid node custom values.");
            }
            node.Location = location;
            OnControlLoaded(node, SurfaceNodeActions.User);
            beforeSpawned?.Invoke(node);
            node.OnSurfaceLoaded(SurfaceNodeActions.User);
            OnControlSpawned(node, SurfaceNodeActions.User);

            // Undo action
            if (Surface != null)
                Surface.AddBatchedUndoAction(new AddRemoveNodeAction(node, true));

            MarkAsModified();

            return node;
        }

        /// <summary>
        /// Marks the context as modified and sends the event to the parent context.
        /// </summary>
        /// <param name="graphEdited">True if graph has been edited (nodes structure or parameter value). Otherwise just UI elements has been modified (node moved, comment resized).</param>
        public void MarkAsModified(bool graphEdited = true)
        {
            _isModified = true;

            Modified?.Invoke(this, graphEdited);

            Parent?.MarkAsModified(graphEdited);
        }

        /// <summary>
        /// Clears the surface data. Disposed all surface nodes, comments, parameters and more.
        /// </summary>
        public void Clear()
        {
            Parameters.Clear();
            Nodes.Clear();
            RootControl.DisposeChildren();
        }
    }
}

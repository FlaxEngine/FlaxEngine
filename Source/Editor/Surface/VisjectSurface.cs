// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Options;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Archetypes;
using FlaxEditor.Surface.ContextMenu;
using FlaxEditor.Surface.GUI;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Visject Surface control for editing Nodes Graph.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    /// <seealso cref="IParametersDependantNode" />
    [HideInEditor]
    public partial class VisjectSurface : ContainerControl, IParametersDependantNode
    {
        private static readonly List<VisjectSurfaceContext> NavUpdateCache = new List<VisjectSurfaceContext>(8);

        /// <summary>
        /// The surface control.
        /// </summary>
        protected SurfaceRootControl _rootControl;

        /// <summary>
        /// Is grid snapping enabled for this surface?
        /// </summary>
        public bool GridSnappingEnabled
        {
            get => _gridSnappingEnabled;
            set
            {
                _gridSnappingEnabled = value;
                _gridRoundingDelta = Float2.Zero;
            }
        }

        /// <summary>
        /// The size of the snapping grid.
        /// </summary>
        public float GridSnappingSize = 20f;

        private float _targetScale = 1.0f;
        private float _moveViewWithMouseDragSpeed = 1.0f;
        private bool _canEdit = true;
        private readonly bool _supportsDebugging;
        private bool _isReleasing, _gridSnappingEnabled;
        private VisjectCM _activeVisjectCM;
        private GroupArchetype _customNodesGroup;
        private List<NodeArchetype> _customNodes;
        private List<IUndoAction> _batchedUndoActions;
        private Action _onSave;
        private int _selectedConnectionIndex;

        internal int _isUpdatingBoxTypes;

        /// <summary>
        /// True if surface supports implicit casting of the FlaxEngine.Object types into Boolean value (as simple validate check).
        /// </summary>
        protected bool _supportsImplicitCastFromObjectToBoolean = false;

        /// <summary>
        /// The left mouse down flag.
        /// </summary>
        protected bool _leftMouseDown;

        /// <summary>
        /// The right mouse down flag.
        /// </summary>
        protected bool _rightMouseDown;

        /// <summary>
        /// The middle mouse down flag.
        /// </summary>
        protected bool _middleMouseDown;

        /// <summary>
        /// The left mouse down position.
        /// </summary>
        protected Float2 _leftMouseDownPos = Float2.Minimum;

        /// <summary>
        /// The right mouse down position.
        /// </summary>
        protected Float2 _rightMouseDownPos = Float2.Minimum;

        /// <summary>
        /// The middle mouse down position.
        /// </summary>
        protected Float2 _middleMouseDownPos = Float2.Minimum;

        /// <summary>
        /// The mouse position.
        /// </summary>
        protected Float2 _mousePos = Float2.Minimum;

        /// <summary>
        /// The mouse movement amount.
        /// </summary>
        protected float _mouseMoveAmount;

        /// <summary>
        /// The is moving selection flag.
        /// </summary>
        protected bool _isMovingSelection;

        /// <summary>
        /// The moving selection view position.
        /// </summary>
        protected Float2 _movingSelectionViewPos;

        /// <summary>
        /// The connection start.
        /// </summary>
        protected IConnectionInstigator _connectionInstigator;

        /// <summary>
        /// The last connection instigator under mouse.
        /// </summary>
        protected IConnectionInstigator _lastInstigatorUnderMouse;

        /// <summary>
        /// The primary context menu.
        /// </summary>
        protected VisjectCM _cmPrimaryMenu;

        /// <summary>
        /// The context menu start position.
        /// </summary>
        protected Float2 _cmStartPos = Float2.Minimum;

        /// <summary>
        /// Occurs when selection gets changed.
        /// </summary>
        public event Action SelectionChanged;

        /// <summary>
        /// The surface owner.
        /// </summary>
        public readonly IVisjectSurfaceOwner Owner;

        /// <summary>
        /// The style used by the surface.
        /// </summary>
        public readonly SurfaceStyle Style;

        /// <summary>
        /// The undo system to use for the history actions recording (optional, can be null).
        /// </summary>
        public readonly FlaxEditor.Undo Undo;

        /// <summary>
        /// If false, the surface editing is blocked (UI display in read-only mode).
        /// </summary>
        public bool CanEdit
        {
            get => _canEdit;
            set
            {
                if (_canEdit == value)
                    return;

                _canEdit = value;
                for (int i = 0; i < _rootControl.Children.Count; i++)
                {
                    if (_rootControl.Children[i] is SurfaceControl control)
                    {
                        control.OnSurfaceCanEditChanged(value);
                    }
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether surface is edited.
        /// </summary>
        public bool IsEdited => RootContext.IsModified;

        /// <summary>
        /// Gets the current context surface root control (nodes and all other surface elements container).
        /// </summary>
        public SurfaceRootControl SurfaceRoot => _rootControl;

        /// <summary>
        /// Gets or sets the view position (upper left corner of the view) in the surface space.
        /// </summary>
        public Float2 ViewPosition
        {
            get => _rootControl.Location / -ViewScale;
            set => _rootControl.Location = value * -ViewScale;
        }

        /// <summary>
        /// Gets or sets the view center position (middle point of the view) in the surface space.
        /// </summary>
        public Float2 ViewCenterPosition
        {
            get => (_rootControl.Location - Size * 0.5f) / -ViewScale;
            set => _rootControl.Location = Size * 0.5f + value * -ViewScale;
        }

        /// <summary>
        /// Gets or sets the view scale.
        /// </summary>
        public float ViewScale
        {
            get => _targetScale;
            set
            {
                // Clamp
                value = Mathf.Clamp(value, 0.05f, 1.6f);

                // Check if value will change
                if (Mathf.Abs(value - _targetScale) > 0.0001f)
                {
                    // Set new target scale
                    _targetScale = value;
                }

                // disable view scale animation
                _rootControl.Scale = new Float2(_targetScale);
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is box selecting nodes.
        /// </summary>
        public bool IsBoxSelecting => _leftMouseDown && !_isMovingSelection && _connectionInstigator == null;

        /// <summary>
        /// Gets a value indicating whether user was previously box selecting nodes.
        /// </summary>
        public bool WasBoxSelecting { get; private set; }

        /// <summary>
        /// Gets a value indicating whether user is moving selected nodes.
        /// </summary>
        public bool IsMovingSelection => _leftMouseDown && _isMovingSelection && _connectionInstigator == null;

        /// <summary>
        /// Gets a value indicating whether user was previously moving selected nodes.
        /// </summary>
        public bool WasMovingSelection { get; private set; }

        /// <summary>
        /// Gets a value indicating whether user is connecting nodes.
        /// </summary>
        public bool IsConnecting => _connectionInstigator != null;

        /// <summary>
        /// Gets a value indicating whether the left mouse button is down.
        /// </summary>
        public bool IsLeftMouseButtonDown => _leftMouseDown;

        /// <summary>
        /// Gets a value indicating whether the right mouse button is down.
        /// </summary>
        public bool IsRightMouseButtonDown => _rightMouseDown;

        /// <summary>
        /// Gets a value indicating whether this Surface supports debugging.
        /// </summary>
        public bool SupportsDebugging => _supportsDebugging;

        /// <summary>
        /// Returns true if any node is selected by the user (one or more).
        /// </summary>
        public bool HasNodesSelection
        {
            get
            {
                for (int i = 0; i < Nodes.Count; i++)
                {
                    if (Nodes[i].IsSelected)
                        return true;
                }

                return false;
            }
        }

        /// <summary>
        /// Gets the list of the selected nodes.
        /// </summary>
        /// <remarks>
        /// Don't call it too often. It does memory allocation and iterates over the surface controls to find selected nodes in the graph.
        /// </remarks>
        public List<SurfaceNode> SelectedNodes
        {
            get
            {
                var selection = new List<SurfaceNode>();
                for (int i = 0; i < Nodes.Count; i++)
                {
                    if (Nodes[i].IsSelected)
                        selection.Add(Nodes[i]);
                }
                return selection;
            }
        }

        /// <summary>
        /// Gets the list of the selected controls (comments and nodes).
        /// </summary>
        /// <remarks>
        /// Don't call it too often. It does memory allocation and iterates over the surface controls to find selected nodes in the graph.
        /// </remarks>
        public List<SurfaceControl> SelectedControls
        {
            get
            {
                var selection = new List<SurfaceControl>();
                for (int i = 0; i < _rootControl.Children.Count; i++)
                {
                    if (_rootControl.Children[i] is SurfaceControl control && control.IsSelected)
                        selection.Add(control);
                }
                return selection;
            }
        }

        /// <summary>
        /// Gets the list of the surface comments.
        /// </summary>
        /// <remarks>
        /// Don't call it too often. It does memory allocation and iterates over the surface controls to find comments in the graph.
        /// </remarks>
        public List<SurfaceComment> Comments => _context?.Comments;

        /// <summary>
        /// The current surface context nodes collection. Read-only.
        /// </summary>
        public List<SurfaceNode> Nodes => _context?.Nodes;

        /// <summary>
        /// The surface node descriptors collection.
        /// </summary>
        public readonly List<GroupArchetype> NodeArchetypes;

        /// <summary>
        /// Occurs when node gets spawned in the surface (via UI).
        /// </summary>
        public event Action<SurfaceNode> NodeSpawned;

        /// <summary>
        /// Occurs when node gets removed from the surface (via UI).
        /// </summary>
        public event Action<SurfaceNode> NodeDeleted;

        /// <summary>
        /// Occurs when node values gets modified in the surface (via UI).
        /// </summary>
        public event Action<SurfaceNode> NodeValuesEdited;

        /// <summary>
        /// Occurs when node breakpoint state gets modified in the surface (via UI).
        /// </summary>
        public event Action<SurfaceNode> NodeBreakpointEdited;

        /// <summary>
        /// Occurs when two nodes gets connected (via UI).
        /// </summary>
        public event Action<IConnectionInstigator, IConnectionInstigator> NodesConnected;

        /// <summary>
        /// Occurs when two nodes gets disconnected (via UI).
        /// </summary>
        public event Action<IConnectionInstigator, IConnectionInstigator> NodesDisconnected;

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectSurface"/> class.
        /// </summary>
        /// <param name="owner">The owner.</param>
        /// <param name="onSave">The save action called when user wants to save the surface.</param>
        /// <param name="undo">The undo/redo to use for the history actions recording. Optional, can be null to disable undo support.</param>
        /// <param name="style">The custom surface style. Use null to create the default style.</param>
        /// <param name="groups">The custom surface node types. Pass null to use the default nodes set.</param>
        /// <param name="supportsDebugging">True if surface supports debugging features (breakpoints, etc.).</param>
        public VisjectSurface(IVisjectSurfaceOwner owner, Action onSave = null, FlaxEditor.Undo undo = null, SurfaceStyle style = null, List<GroupArchetype> groups = null, bool supportsDebugging = false)
        {
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
            AutoFocus = false; // Disable to prevent autofocus and event handling on OnMouseDown event
            CullChildren = false;
            _supportsDebugging = supportsDebugging;

            Owner = owner;
            Style = style ?? SurfaceStyle.CreateStyleHandler(Editor.Instance);
            if (Style == null)
                throw new InvalidOperationException("Missing visject surface style.");
            NodeArchetypes = groups ?? NodeFactory.DefaultGroups;
            Undo = undo;
            _onSave = onSave;

            // Initialize with the root context
            OpenContext(owner);
            RootContext.Modified += OnRootContextModified;

            // Setup input actions
            InputActions = new InputActionsContainer(new[]
            {
                new InputActionsContainer.Binding(options => options.Delete, Delete),
                new InputActionsContainer.Binding(options => options.SelectAll, SelectAll),
                new InputActionsContainer.Binding(options => options.DeselectAll, DeselectAll),
                new InputActionsContainer.Binding(options => options.Copy, Copy),
                new InputActionsContainer.Binding(options => options.Paste, Paste),
                new InputActionsContainer.Binding(options => options.Cut, Cut),
                new InputActionsContainer.Binding(options => options.Duplicate, Duplicate),
                new InputActionsContainer.Binding(options => options.NodesAutoFormat, () => { FormatGraph(SelectedNodes); }),
                new InputActionsContainer.Binding(options => options.NodesAlignTop, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Top); }),
                new InputActionsContainer.Binding(options => options.NodesAlignMiddle, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Middle); }),
                new InputActionsContainer.Binding(options => options.NodesAlignBottom, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Bottom); }),
                new InputActionsContainer.Binding(options => options.NodesAlignLeft, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Left); }),
                new InputActionsContainer.Binding(options => options.NodesAlignCenter, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Center); }),
                new InputActionsContainer.Binding(options => options.NodesAlignRight, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Right); }),
                new InputActionsContainer.Binding(options => options.NodesDistributeHorizontal, () => {  DistributeNodes(SelectedNodes, false); }),
                new InputActionsContainer.Binding(options => options.NodesDistributeVertical, () => {  DistributeNodes(SelectedNodes, true); }),
            });

            Context.ControlSpawned += OnSurfaceControlSpawned;
            Context.ControlDeleted += OnSurfaceControlDeleted;

            SelectionChanged += () => { _selectedConnectionIndex = 0; };

            // Init drag handlers
            DragHandlers.Add(_dragAssets = new DragAssets<DragDropEventArgs>(ValidateDragItem));
            DragHandlers.Add(_dragParameters = new DragNames<DragDropEventArgs>(SurfaceParameter.DragPrefix, ValidateDragParameter));

            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
        }

        private void OnScriptsReloadBegin()
        {
            _activeVisjectCM = null;
            _cmPrimaryMenu?.Dispose();
            _cmPrimaryMenu = null;
        }

        /// <summary>
        /// Gets the display name of the connection type used in the surface.
        /// </summary>
        /// <param name="type">The graph connection type.</param>
        /// <returns>The display name (for UI).</returns>
        public virtual string GetTypeName(ScriptType type)
        {
            return type == ScriptType.Null ? null : type.Name;
        }

        /// <summary>
        /// Gets the debugger tooltip for the given box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="text">The result tooltip text.</param>
        /// <returns>True if processed output text should be used to display it on a box tooltip for debugger UI, otherwise false.</returns>
        public virtual bool GetBoxDebuggerTooltip(Elements.Box box, out string text)
        {
            text = null;
            return false;
        }

        private void OnRootContextModified(VisjectSurfaceContext context, bool graphEdited)
        {
            Owner.OnSurfaceEditedChanged();

            if (graphEdited)
            {
                Owner.OnSurfaceGraphEdited();
            }
        }

        /// <summary>
        /// Gets the custom nodes group archetype with custom nodes archetypes. May be null if no custom nodes in use.
        /// </summary>
        /// <returns>The custom nodes or null if no used.</returns>
        public GroupArchetype GetCustomNodes()
        {
            return _customNodesGroup;
        }

        /// <summary>
        /// Adds the custom nodes archetypes to the surface (user can spawn them and surface can deserialize).
        /// </summary>
        /// <remarks>Custom nodes has to have a node logic typename in DefaultValues[0] and group name in DefaultValues[1].</remarks>
        /// <param name="archetypes">The archetypes.</param>
        public void AddCustomNodes(IEnumerable<NodeArchetype> archetypes)
        {
            if (_customNodes == null)
            {
                // First time setup
                _customNodes = new List<NodeArchetype>(archetypes);
                _customNodesGroup = new GroupArchetype
                {
                    GroupID = Custom.GroupID,
                    Name = "Custom",
                    Color = Color.Wheat
                };
            }
            else
            {
                // Add more nodes
                _customNodes.AddRange(archetypes);
            }

            // Update collection
            _customNodesGroup.Archetypes = _customNodes;
        }

        /// <summary>
        /// Updates the navigation bar of the toolstrip from window that uses this surface. Updates the navigation bar panel buttons to match the current view path.
        /// </summary>
        /// <param name="navigationBar">The navigation bar to update.</param>
        /// <param name="toolStrip">The toolstrip to use as layout reference.</param>
        /// <param name="hideIfRoot">True if skip showing nav button if the current context is the root location (user has no option to change context).</param>
        public void UpdateNavigationBar(NavigationBar navigationBar, ToolStrip toolStrip, bool hideIfRoot = true)
        {
            if (navigationBar == null || toolStrip == null)
                return;

            bool wasLayoutLocked = navigationBar.IsLayoutLocked;
            navigationBar.IsLayoutLocked = true;

            // Remove previous buttons
            navigationBar.DisposeChildren();

            // Spawn buttons
            var nodes = NavUpdateCache;
            nodes.Clear();
            var context = Context;
            if (hideIfRoot && context == RootContext)
                context = null;
            while (context != null)
            {
                nodes.Add(context);
                context = context.Parent;
            }
            float x = NavigationBar.DefaultButtonsMargin;
            float h = toolStrip.ItemsHeight - 2 * ToolStrip.DefaultMarginV;
            for (int i = nodes.Count - 1; i >= 0; i--)
            {
                var button = new VisjectContextNavigationButton(this, nodes[i].Context, x, ToolStrip.DefaultMarginV, h);
                button.PerformLayout();
                x += button.Width + NavigationBar.DefaultButtonsMargin;
                navigationBar.AddChild(button);
            }
            nodes.Clear();

            // Update
            navigationBar.IsLayoutLocked = wasLayoutLocked;
            navigationBar.PerformLayout();
        }

        /// <summary>
        /// Gets all nodes bounds or empty if surface is empty.
        /// </summary>
        public Rectangle AllNodesBounds
        {
            get
            {
                var area = Rectangle.Empty;
                if (Nodes.Count != 0)
                {
                    area = Nodes[0].Bounds;
                    for (int i = 1; i < Nodes.Count; i++)
                        area = Rectangle.Union(area, Nodes[i].Bounds);
                }
                return area;
            }
        }

        /// <summary>
        /// Gets a value indicating whether surface parameters are not read-only and can be modified in a surface graph.
        /// </summary>
        public virtual bool CanSetParameters => false;

        /// <summary>
        /// True of the context menu should make use of a description panel drawn at the bottom of the menu
        /// </summary>
        public virtual bool UseContextMenuDescriptionPanel => false;

        /// <summary>
        /// Gets a value indicating whether surface supports/allows live previewing graph modifications due to value sliders and color pickers. True by default but disabled for shader surfaces that generate and compile shader source at flight.
        /// </summary>
        public virtual bool CanLivePreviewValueChanges => true;

        /// <summary>
        /// Determines whether the specified node archetype can be used in the surface.
        /// </summary>
        /// <param name="groupID">The nodes group archetype identifier.</param>
        /// <param name="typeID">The node archetype identifier.</param>
        /// <returns>True if can use this node archetype, otherwise false.</returns>
        public bool CanUseNodeType(ushort groupID, ushort typeID)
        {
            var result = false;
            var nodeArchetypes = NodeArchetypes ?? NodeFactory.DefaultGroups;
            if (NodeFactory.GetArchetype(nodeArchetypes, groupID, typeID, out var groupArchetype, out var nodeArchetype))
            {
                var flags = nodeArchetype.Flags;
                nodeArchetype.Flags &= ~NodeFlags.NoSpawnViaGUI;
                nodeArchetype.Flags &= ~NodeFlags.NoSpawnViaPaste;
                result = CanUseNodeType(groupArchetype, nodeArchetype);
                nodeArchetype.Flags = flags;
            }
            return result;
        }

        /// <summary>
        /// Determines whether the specified node archetype can be used in the surface.
        /// </summary>
        /// <param name="groupArchetype">The nodes group archetype.</param>
        /// <param name="nodeArchetype">The node archetype.</param>
        /// <returns>True if can use this node archetype, otherwise false.</returns>
        public virtual bool CanUseNodeType(GroupArchetype groupArchetype, NodeArchetype nodeArchetype)
        {
            return (nodeArchetype.Flags & NodeFlags.NoSpawnViaPaste) == 0;
        }

        /// <summary>
        /// Shows the whole graph by changing the view scale and the position.
        /// </summary>
        public void ShowWholeGraph()
        {
            ShowArea(AllNodesBounds.MakeExpanded(100.0f));
        }

        /// <summary>
        /// Shows the given surface area by changing the view scale and the position.
        /// </summary>
        /// <param name="areaRect">The area rectangle.</param>
        public void ShowArea(Rectangle areaRect)
        {
            ViewScale = (Size / areaRect.Size).MinValue * 0.95f;
            ViewCenterPosition = areaRect.Center;
        }

        /// <summary>
        /// Shows the given surface node by changing the view scale and the position and focuses the node.
        /// </summary>
        /// <param name="node">The node to show.</param>
        public void FocusNode(SurfaceNode node)
        {
            if (node != null)
            {
                ShowArea(node.Bounds.MakeExpanded(500.0f));
                Select(node);
            }
        }

        /// <summary>
        /// Mark surface as edited.
        /// </summary>
        /// <param name="graphEdited">True if graph has been edited (nodes structure or parameter value).</param>
        public void MarkAsEdited(bool graphEdited = true)
        {
            _context.MarkAsModified(graphEdited);
        }

        private void BulkSelectUpdate(bool select = true)
        {
            bool selectionChanged = false;
            for (int i = 0; i < _rootControl.Children.Count; i++)
            {
                if (_rootControl.Children[i] is SurfaceControl control && control.IsSelected != select)
                {
                    control.IsSelected = select;
                    selectionChanged = true;
                }
            }
            if (selectionChanged)
                SelectionChanged?.Invoke();
        }

        internal void ToggleGridSnapping()
        {
            GridSnappingEnabled = !GridSnappingEnabled;
        }

        /// <summary>
        /// Selects all the nodes.
        /// </summary>
        public void SelectAll()
        {
            BulkSelectUpdate(true);
        }

        /// <summary>
        /// Deelects all the nodes.
        /// </summary>
        public void DeselectAll()
        {
            BulkSelectUpdate(false);
        }

        /// <summary>
        /// Clears the selection.
        /// </summary>
        public void ClearSelection()
        {
            bool selectionChanged = false;
            for (int i = 0; i < _rootControl.Children.Count; i++)
            {
                if (_rootControl.Children[i] is SurfaceControl control && control.IsSelected)
                {
                    control.IsSelected = false;
                    selectionChanged = true;
                }
            }
            if (selectionChanged)
                SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Adds the specified control to the selection.
        /// </summary>
        /// <param name="control">The control.</param>
        public void AddToSelection(SurfaceControl control)
        {
            if (control.IsSelected)
                return;
            control.IsSelected = true;
            SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Removes the specified control from the selection.
        /// </summary>
        /// <param name="control">The control.</param>
        public void RemoveFromSelection(SurfaceControl control)
        {
            if (!control.IsSelected)
                return;
            control.IsSelected = false;
            SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Selects the specified control.
        /// </summary>
        /// <param name="control">The control.</param>
        public void Select(SurfaceControl control)
        {
            bool selectionChanged = false;
            for (int i = 0; i < _rootControl.Children.Count; i++)
            {
                if (_rootControl.Children[i] is SurfaceControl c && c != control && c.IsSelected)
                {
                    c.IsSelected = false;
                    selectionChanged = true;
                }
            }
            if (!control.IsSelected)
            {
                control.IsSelected = true;
                selectionChanged = true;
            }
            if (selectionChanged)
                SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Selects the specified controls collection.
        /// </summary>
        /// <param name="controls">The controls.</param>
        public void Select(IEnumerable<SurfaceControl> controls)
        {
            var newSelection = controls.ToList();
            var prevSelection = SelectedControls;
            if (Utils.ArraysEqual(newSelection, prevSelection))
                return;
            ClearSelection();
            foreach (var control in newSelection)
                control.IsSelected = true;
            SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Deselects the specified control.
        /// </summary>
        /// <param name="control">The control.</param>
        public void Deselect(SurfaceControl control)
        {
            if (!control.IsSelected)
                return;
            control.IsSelected = false;
            SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Creates the comment around the selected nodes.
        /// </summary>
        public SurfaceComment CommentSelection(string text = "")
        {
            var selection = SelectedNodes;
            if (selection.Count == 0)
                return null;
            Rectangle surfaceArea = GetNodesBounds(selection).MakeExpanded(80.0f);

            // Order below other selected comments
            bool hasCommentsSelected = false;
            int lowestCommentOrder = int.MaxValue;
            for (int i = 0; i < selection.Count; i++)
            {
                if (selection[i] is not SurfaceComment || selection[i].IndexInParent >= lowestCommentOrder)
                    continue;
                hasCommentsSelected = true;
                lowestCommentOrder = selection[i].IndexInParent;
            }

            return _context.CreateComment(ref surfaceArea, string.IsNullOrEmpty(text) ? "Comment" : text, new Color(1.0f, 1.0f, 1.0f, 0.2f), hasCommentsSelected ? lowestCommentOrder : -1);
        }

        private static Rectangle GetNodesBounds(List<SurfaceNode> nodes)
        {
            if (nodes.Count == 0)
                return Rectangle.Empty;

            Rectangle surfaceArea = nodes[0].Bounds;
            for (int i = 1; i < nodes.Count; i++)
            {
                surfaceArea = Rectangle.Union(surfaceArea, nodes[i].Bounds);
            }

            return surfaceArea;
        }

        /// <summary>
        /// Deletes the specified collection of the controls.
        /// </summary>
        /// <param name="controls">The controls.</param>
        /// <param name="withUndo">True if use undo/redo action for node removing.</param>
        public void Delete(IEnumerable<SurfaceControl> controls, bool withUndo = true)
        {
            if (!CanEdit || controls == null || !controls.Any())
                return;

            var selectionChanged = false;
            List<SurfaceNode> nodes = null;
            foreach (var control in controls)
            {
                if (control.IsSelected)
                {
                    selectionChanged = true;
                    control.IsSelected = false;
                }

                if (control is SurfaceNode node)
                {
                    if ((node.Archetype.Flags & NodeFlags.NoRemove) == 0)
                    {
                        if (nodes == null)
                            nodes = new List<SurfaceNode>();
                        var sealedNodes = node.SealedNodes;
                        if (sealedNodes != null)
                        {
                            foreach (var sealedNode in sealedNodes)
                            {
                                if (sealedNode != null)
                                {
                                    if (sealedNode.IsSelected)
                                    {
                                        selectionChanged = true;
                                        sealedNode.IsSelected = false;
                                    }
                                    if (!nodes.Contains(sealedNode))
                                        nodes.Add(sealedNode);
                                }
                            }
                        }
                        if (!nodes.Contains(node))
                            nodes.Add(node);
                    }
                }
                else
                {
                    Context.OnControlDeleted(control, SurfaceNodeActions.User);
                }
            }
            if (selectionChanged)
                SelectionChanged?.Invoke();

            if (nodes != null)
            {
                if (Undo == null || !withUndo)
                {
                    // Remove all nodes
                    foreach (var node in nodes)
                    {
                        node.RemoveConnections();
                        Nodes.Remove(node);
                        Context.OnControlDeleted(node, SurfaceNodeActions.User);
                    }
                }
                else
                {
                    var actions = new List<IUndoAction>();

                    // Break connections for all nodes
                    foreach (var node in nodes)
                    {
                        var action = new EditNodeConnections(Context, node);
                        node.RemoveConnections();
                        action.End();
                        actions.Add(action);
                    }

                    // Remove all nodes
                    foreach (var node in nodes)
                    {
                        var action = new AddRemoveNodeAction(node, false);
                        action.Do();
                        actions.Add(action);
                    }

                    AddBatchedUndoAction(new MultiUndoAction(actions, nodes.Count == 1 ? "Remove node" : "Remove nodes"));
                }
            }

            MarkAsEdited();
        }

        /// <summary>
        /// Deletes the specified control.
        /// </summary>
        /// <param name="control">The control.</param>
        /// <param name="withUndo">True if use undo/redo action for node removing.</param>
        public void Delete(SurfaceControl control, bool withUndo = true)
        {
            if (!CanEdit)
                return;
            Delete(new[] { control }, withUndo);
        }

        /// <summary>
        /// Deletes the selected controls.
        /// </summary>
        public void Delete()
        {
            if (!CanEdit)
                return;
            Delete(SelectedControls, true);
        }

        /// <summary>
        /// Finds the node of the given type.
        /// </summary>
        /// <param name="groupId">The group identifier.</param>
        /// <param name="typeId">The type identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        public SurfaceNode FindNode(ushort groupId, ushort typeId)
        {
            return _context.FindNode(groupId, typeId);
        }

        /// <summary>
        /// Finds the node with the given ID.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        public SurfaceNode FindNode(int id)
        {
            return _context.FindNode(id);
        }

        /// <summary>
        /// Finds the node with the given ID.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        public SurfaceNode FindNode(uint id)
        {
            return _context.FindNode(id);
        }

        /// <summary>
        /// Adds the undo action to be batched (eg. if multiple undo actions is performed in a sequence during single update).
        /// </summary>
        /// <param name="action">The action.</param>
        public void AddBatchedUndoAction(IUndoAction action)
        {
            if (Undo == null || !Undo.Enabled)
                return;
            if (_batchedUndoActions == null)
                _batchedUndoActions = new List<IUndoAction>();
            _batchedUndoActions.Add(action);
        }

        /// <summary>
        /// Called when node gets spawned in the surface (via UI).
        /// </summary>
        public virtual void OnNodeSpawned(SurfaceNode node)
        {
            NodeSpawned?.Invoke(node);
        }

        /// <summary>
        /// Called when node values gets modified in the surface (via UI).
        /// </summary>
        public virtual void OnNodeValuesEdited(SurfaceNode node)
        {
            NodeValuesEdited?.Invoke(node);
        }

        /// <summary>
        /// Called when node breakpoint gets modified.
        /// </summary>
        public virtual void OnNodeBreakpointEdited(SurfaceNode node)
        {
            NodeBreakpointEdited?.Invoke(node);
        }

        /// <summary>
        /// Called when node gets removed from the surface (via UI).
        /// </summary>
        public virtual void OnNodeDeleted(SurfaceNode node)
        {
            NodeDeleted?.Invoke(node);
        }

        /// <summary>
        /// Called when two nodes gets connected (via UI).
        /// </summary>
        public virtual void OnNodesConnected(IConnectionInstigator a, IConnectionInstigator b)
        {
            NodesConnected?.Invoke(a, b);
            MarkAsEdited();
        }

        /// <summary>
        /// Called when two nodes gets disconnected (via UI).
        /// </summary>
        public virtual void OnNodesDisconnected(IConnectionInstigator a, IConnectionInstigator b)
        {
            NodesDisconnected?.Invoke(a, b);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            _isReleasing = true;

            // Cleanup context cache
            _root = null;
            _context = null;
            _onSave = null;
            ContextStack.Clear();
            foreach (var context in _contextCache.Values)
            {
                context.Clear();
            }
            _contextCache.Clear();

            // Cleanup
            _activeVisjectCM = null;
            _cmPrimaryMenu?.Dispose();

            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;

            base.OnDestroy();
        }
    }
}

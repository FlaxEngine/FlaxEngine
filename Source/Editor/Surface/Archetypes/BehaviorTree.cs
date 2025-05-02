// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors.Dedicated;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Behavior Tree group.
    /// </summary>
    [HideInEditor]
    public static class BehaviorTree
    {
        /// <summary>
        /// Base class for Behavior Tree nodes wrapped inside <see cref="SurfaceNode" />.
        /// </summary>
        internal class NodeBase : SurfaceNode
        {
            protected const float ConnectionAreaMargin = 12.0f;
            protected const float ConnectionAreaHeight = 12.0f;
            protected const float DecoratorsMarginX = 5.0f;
            protected const float DecoratorsMarginY = 2.0f;

            protected bool _debugRelevant;
            protected string _debugInfo;
            protected Float2 _debugInfoSize;
            protected ScriptType _type;
            internal bool _isValueEditing;

            public BehaviorTreeNode Instance;

            protected NodeBase(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public static string GetTitle(ScriptType scriptType)
            {
                var title = scriptType.Name;
                if (title.StartsWith("BehaviorTree"))
                    title = title.Substring(12);
                if (title.EndsWith("Node"))
                    title = title.Substring(0, title.Length - 4);
                if (title.EndsWith("Decorator"))
                    title = title.Substring(0, title.Length - 9);
                title = Utilities.Utils.GetPropertyNameUI(title);
                return title;
            }

            public virtual void UpdateDebug(Behavior behavior)
            {
                BehaviorTreeNode instance = null;
                if (behavior)
                {
                    // Try to use instance from the currently debugged behavior
                    // TODO: support nodes from nested trees
                    instance = behavior.Tree.GetNodeInstance(ID);
                }
                var size = _debugInfoSize;
                UpdateDebugInfo(instance, behavior);
                if (size != _debugInfoSize)
                    ResizeAuto();
            }

            protected virtual void UpdateTitle()
            {
                string title = null;
                if (Instance != null)
                {
                    title = Instance.Name;
                    if (string.IsNullOrEmpty(title))
                        title = GetTitle(_type);
                }
                else
                {
                    var typeName = (string)Values[0];
                    title = "Missing Type " + typeName;
                }
                Title = title;
            }

            protected virtual void UpdateDebugInfo(BehaviorTreeNode instance = null, Behavior behavior = null)
            {
                _debugRelevant = false;
                _debugInfo = null;
                _debugInfoSize = Float2.Zero;
                if (!instance)
                    instance = Instance;
                if (instance)
                {
                    // Get debug description for the node based on the current settings
                    _debugRelevant = Behavior.GetNodeDebugRelevancy(instance, behavior);
                    _debugInfo = Behavior.GetNodeDebugInfo(instance, behavior);
                    if (!string.IsNullOrEmpty(_debugInfo))
                        _debugInfoSize = Style.Current.FontSmall.MeasureText(_debugInfo);
                }
            }

            private void OnTypeDisposing(ScriptType type)
            {
                if (_type == type && !IsDisposing)
                {
                    // Turn into missing script
                    _type = ScriptType.Null;
                    Instance = null;
                }
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                // Setup node type and data
                var typeName = (string)Values[0];
                _type = TypeUtils.GetType(typeName);
                if (_type != null)
                {
                    _type.TrackLifetime(OnTypeDisposing);
                    TooltipText = Editor.Instance.CodeDocs.GetTooltip(_type);
                    try
                    {
                        // Load node instance from data
                        Instance = (BehaviorTreeNode)_type.CreateInstance();
                        var instanceData = (byte[])Values[1];
                        FlaxEngine.Json.JsonSerializer.LoadFromBytes(Instance, instanceData, Globals.EngineBuildNumber);
                    }
                    catch (Exception ex)
                    {
                        Editor.LogError("Failed to load Behavior Tree node of type " + typeName);
                        Editor.LogWarning(ex);
                    }
                }
                else
                {
                    Instance = null;
                }

                UpdateDebugInfo();
                UpdateTitle();
            }

            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                // Skip updating instance when it's being edited by user via UI
                if (!_isValueEditing)
                {
                    try
                    {
                        if (Instance != null)
                        {
                            // Reload node instance from data
                            var instanceData = (byte[])Values[1];
                            if (instanceData == null || instanceData.Length == 0)
                            {
                                // Recreate instance data to default state if previous state was empty
                                var defaultInstance = (BehaviorTreeNode)_type.CreateInstance(); // TODO: use default instance from native ScriptingType
                                instanceData = FlaxEngine.Json.JsonSerializer.SaveToBytes(defaultInstance);
                            }
                            FlaxEngine.Json.JsonSerializer.LoadFromBytes(Instance, instanceData, Globals.EngineBuildNumber);
                        }
                    }
                    catch (Exception ex)
                    {
                        Editor.LogError("Failed to load Behavior Tree node of type " + _type);
                        Editor.LogWarning(ex);
                    }
                }

                UpdateDebugInfo();
                UpdateTitle();
            }

            public override void OnSpawned(SurfaceNodeActions action)
            {
                base.OnSpawned(action);

                ResizeAuto();
            }

            public override void Draw()
            {
                base.Draw();

                // Debug Info
                if (!string.IsNullOrEmpty(_debugInfo))
                {
                    var style = Style.Current;
                    Render2D.DrawText(style.FontSmall, _debugInfo, new Rectangle(4, _headerRect.Bottom + 4, _debugInfoSize), style.Foreground);
                }

                // Debug relevancy outline
                if (_debugRelevant)
                {
                    var colorTop = Color.LightYellow;
                    var colorBottom = Color.Yellow;
                    var backgroundRect = new Rectangle(Float2.One, Size - new Float2(2.0f));
                    Render2D.DrawRectangle(backgroundRect, colorTop, colorTop, colorBottom, colorBottom);
                }
            }

            public override void OnDestroy()
            {
                if (IsDisposing)
                    return;
                _debugInfo = null;
                _type = ScriptType.Null;
                FlaxEngine.Object.Destroy(ref Instance);

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the Behavior Tree node.
        /// </summary>
        internal class Node : NodeBase
        {
            private InputBox _input;
            private OutputBox _output;
            internal List<Decorator> _decorators;

            internal static SurfaceNode Create(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            {
                return new Node(id, context, nodeArch, groupArch);
            }

            internal Node(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public unsafe List<uint> DecoratorIds
            {
                get
                {
                    var result = new List<uint>();
                    var ids = Values.Length >= 3 ? Values[2] as byte[] : null;
                    if (ids != null)
                    {
                        fixed (byte* data = ids)
                        {
                            uint* ptr = (uint*)data;
                            int count = ids.Length / sizeof(uint);
                            for (int i = 0; i < count; i++)
                                result.Add(ptr[i]);
                        }
                    }
                    return result;
                }
                set => SetDecoratorIds(value, true);
            }

            public unsafe List<Decorator> Decorators
            {
                get
                {
                    if (_decorators == null)
                    {
                        _decorators = new List<Decorator>();
                        var ids = Values.Length >= 3 ? Values[2] as byte[] : null;
                        if (ids != null)
                        {
                            fixed (byte* data = ids)
                            {
                                uint* ptr = (uint*)data;
                                int count = ids.Length / sizeof(uint);
                                for (int i = 0; i < count; i++)
                                {
                                    var decorator = Surface.FindNode(ptr[i]) as Decorator;
                                    if (decorator != null)
                                        _decorators.Add(decorator);
                                }
                            }
                        }
                    }
                    return _decorators;
                }
                set
                {
                    _decorators = null;
                    var ids = new byte[sizeof(uint) * value.Count];
                    if (value != null)
                    {
                        fixed (byte* data = ids)
                        {
                            uint* ptr = (uint*)data;
                            for (var i = 0; i < value.Count; i++)
                                ptr[i] = value[i].ID;
                        }
                    }
                    SetValue(2, ids);

                    // Force refresh UI
                    ResizeAuto();
                }
            }

            public unsafe void SetDecoratorIds(List<uint> value, bool withUndo)
            {
                var ids = new byte[sizeof(uint) * value.Count];
                if (value != null)
                {
                    fixed (byte* data = ids)
                    {
                        uint* ptr = (uint*)data;
                        for (var i = 0; i < value.Count; i++)
                            ptr[i] = value[i];
                    }
                }
                if (withUndo)
                    SetValue(2, ids);
                else
                {
                    Values[2] = ids;
                    OnValuesChanged();
                    Surface?.MarkAsEdited();
                }
            }

            public override unsafe SurfaceNode[] SealedNodes
            {
                get
                {
                    // Return decorator nodes attached to this node to be moved/copied/pasted as a one
                    SurfaceNode[] result = null;
                    var ids = Values.Length >= 3 ? Values[2] as byte[] : null;
                    if (ids != null)
                    {
                        fixed (byte* data = ids)
                        {
                            uint* ptr = (uint*)data;
                            int count = ids.Length / sizeof(uint);
                            result = new SurfaceNode[count];
                            for (int i = 0; i < count; i++)
                            {
                                var decorator = Surface.FindNode(ptr[i]) as Decorator;
                                if (decorator != null)
                                    result[i] = decorator;
                            }
                        }
                    }
                    return result;
                }
            }

            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Float2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                if (!Surface.CanEdit)
                    return;

                menu.AddSeparator();

                var nodeTypes = Editor.Instance.CodeEditing.BehaviorTreeNodes.Get();

                if (_input.Enabled) // Root node cannot have decorators
                {
                    var decorators = menu.AddChildMenu("Add Decorator");
                    var decoratorType = new ScriptType(typeof(BehaviorTreeDecorator));
                    foreach (var nodeType in nodeTypes)
                    {
                        if (nodeType != decoratorType && decoratorType.IsAssignableFrom(nodeType))
                        {
                            var button = decorators.ContextMenu.AddButton(GetTitle(nodeType));
                            button.Tag = nodeType;
                            button.TooltipText = Editor.Instance.CodeDocs.GetTooltip(nodeType);
                            button.ButtonClicked += OnAddDecoratorButtonClicked;
                        }
                    }
                }
            }

            private void OnAddDecoratorButtonClicked(ContextMenuButton button)
            {
                var nodeType = (ScriptType)button.Tag;

                // Spawn decorator
                var decorator = Context.SpawnNode(19, 3, Location, new object[]
                {
                    nodeType.TypeName,
                    Utils.GetEmptyArray<byte>(),
                });

                // Add decorator to the node
                var decorators = Decorators;
                decorators.Add((Decorator)decorator);
                Decorators = decorators;
            }

            public override void OnValuesChanged()
            {
                // Reject cached value
                _decorators = null;

                base.OnValuesChanged();

                ResizeAuto();
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                // Setup boxes
                _input = (InputBox)GetBox(0);
                _output = (OutputBox)GetBox(1);
                _input.ConnectionOffset = new Float2(0, FlaxEditor.Surface.Constants.BoxSize * -0.5f);
                _output.ConnectionOffset = new Float2(0, FlaxEditor.Surface.Constants.BoxSize * 0.5f);

                // Setup node type and data
                var flagsRoot = NodeFlags.NoRemove | NodeFlags.NoCloseButton | NodeFlags.NoSpawnViaPaste;
                var flags = Archetype.Flags & ~flagsRoot;
                if (_type != null)
                {
                    bool isRoot = _type.Type == typeof(BehaviorTreeRootNode);
                    _input.Enabled = _input.Visible = !isRoot;
                    _output.Enabled = _output.Visible = new ScriptType(typeof(BehaviorTreeCompoundNode)).IsAssignableFrom(_type);
                    if (isRoot)
                        flags |= flagsRoot;
                }
                if (Archetype.Flags != flags)
                {
                    // Apply custom flags
                    Archetype = (NodeArchetype)Archetype.Clone();
                    Archetype.Flags = flags;
                }

                ResizeAuto();
            }

            public override unsafe void OnPasted(Dictionary<uint, uint> idsMapping)
            {
                base.OnPasted(idsMapping);

                // Update decorators
                var ids = Values.Length >= 3 ? Values[2] as byte[] : null;
                if (ids != null)
                {
                    _decorators = null;
                    fixed (byte* data = ids)
                    {
                        uint* ptr = (uint*)data;
                        int count = ids.Length / sizeof(uint);
                        for (int i = 0; i < count; i++)
                        {
                            if (idsMapping.TryGetValue(ptr[i], out var id))
                            {
                                // Fix previous parent node to re-apply layout (in case it was forced by spawned decorator)
                                var decorator = Surface.FindNode(ptr[i]) as Decorator;
                                var decoratorNode = decorator?.Node;
                                if (decoratorNode != null)
                                    decoratorNode.ResizeAuto();

                                // Update mapping to the new node
                                ptr[i] = id;
                            }
                        }
                    }
                    Values[2] = ids;
                    ResizeAuto();
                }
            }

            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                ResizeAuto();
                Surface.NodeDeleted += OnNodeDeleted;
            }

            private void OnNodeDeleted(SurfaceNode node)
            {
                if (node is Decorator decorator && Decorators.Contains(decorator))
                {
                    // Decorator was spawned (eg. via undo)
                    _decorators = null;
                    ResizeAuto();
                }
            }

            public override void ResizeAuto()
            {
                if (Surface == null)
                    return;
                var width = 0.0f;
                var height = 0.0f;
                var titleLabelFont = Style.Current.FontLarge;
                width = Mathf.Max(width, 100.0f);
                width = Mathf.Max(width, titleLabelFont.MeasureText(Title).X + 30);
                if (_debugInfoSize.X > 0)
                {
                    width = Mathf.Max(width, _debugInfoSize.X + 8.0f);
                    height += _debugInfoSize.Y + 8.0f;
                }
                if (_input != null && _input.Visible)
                    height += ConnectionAreaHeight;
                if (_output != null && _output.Visible)
                    height += ConnectionAreaHeight;
                var decorators = Decorators;
                foreach (var decorator in decorators)
                {
                    decorator.ResizeAuto();
                    height += decorator.Height + DecoratorsMarginY;
                    width = Mathf.Max(width, decorator.Width - FlaxEditor.Surface.Constants.NodeCloseButtonSize - 2 * DecoratorsMarginX);
                }
                Size = new Float2(width + FlaxEditor.Surface.Constants.NodeMarginX * 2 + FlaxEditor.Surface.Constants.NodeCloseButtonSize, height + FlaxEditor.Surface.Constants.NodeHeaderSize + FlaxEditor.Surface.Constants.NodeFooterSize);
                UpdateRectangles();
            }

            protected override void UpdateRectangles()
            {
                Rectangle bounds = Bounds;
                if (_input != null && _input.Visible)
                {
                    _input.Bounds = new Rectangle(ConnectionAreaMargin, 0, Width - ConnectionAreaMargin * 2, ConnectionAreaHeight);
                    bounds.Location.Y += _input.Height;
                }
                var decorators = Decorators;
                var indexInParent = IndexInParent;
                foreach (var decorator in decorators)
                {
                    decorator.Bounds = new Rectangle(bounds.Location.X + DecoratorsMarginX, bounds.Location.Y, bounds.Width - 2 * DecoratorsMarginX, decorator.Height);
                    bounds.Location.Y += decorator.Height + DecoratorsMarginY;
                    if (decorator.IndexInParent < indexInParent)
                        decorator.IndexInParent = indexInParent + 1; // Push elements above the node
                }
                const float footerSize = FlaxEditor.Surface.Constants.NodeFooterSize;
                const float headerSize = FlaxEditor.Surface.Constants.NodeHeaderSize;
                const float closeButtonMargin = FlaxEditor.Surface.Constants.NodeCloseButtonMargin;
                const float closeButtonSize = FlaxEditor.Surface.Constants.NodeCloseButtonSize;
                _headerRect = new Rectangle(0, bounds.Y - Y, bounds.Width, headerSize);
                _closeButtonRect = new Rectangle(bounds.Width - closeButtonSize - closeButtonMargin, _headerRect.Y + closeButtonMargin, closeButtonSize, closeButtonSize);
                _footerRect = new Rectangle(0, bounds.Height - footerSize, bounds.Width, footerSize);
                if (_output != null && _output.Visible)
                {
                    _footerRect.Y -= ConnectionAreaHeight;
                    _output.Bounds = new Rectangle(ConnectionAreaMargin, bounds.Height - ConnectionAreaHeight, bounds.Width - ConnectionAreaMargin * 2, ConnectionAreaHeight);
                }
            }

            protected override void OnLocationChanged()
            {
                base.OnLocationChanged();

                // Sync attached elements placement
                UpdateRectangles();
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the Behavior Tree decorator.
        /// </summary>
        internal class Decorator : NodeBase
        {
            private sealed class DragDecorator : DragHelper<uint, DragEventArgs>
            {
                public const string DragPrefix = "DECORATOR!?";

                public DragDecorator(Func<uint, bool> validateFunction)
                : base(validateFunction)
                {
                }

                public override DragData ToDragData(uint item) => new DragDataText(DragPrefix + item);

                public override DragData ToDragData(IEnumerable<uint> items)
                {
                    throw new NotImplementedException();
                }

                public override IEnumerable<uint> FromDragData(DragData data)
                {
                    if (data is DragDataText dataText)
                    {
                        if (dataText.Text.StartsWith(DragPrefix))
                        {
                            var id = dataText.Text.Remove(0, DragPrefix.Length).Split('\n');
                            return new[] { uint.Parse(id[0]) };
                        }
                    }
                    return Utils.GetEmptyArray<uint>();
                }
            }

            private sealed class ReorderDecoratorAction : IUndoAction
            {
                public VisjectSurface Surface;
                public uint DecoratorId, PrevNodeId, NewNodeId;
                public int PrevIndex, NewIndex;

                public string ActionString => "Reorder decorator";

                private void Do(uint nodeId, int index)
                {
                    var decorator = Surface.FindNode(DecoratorId) as Decorator;
                    if (decorator == null)
                        throw new Exception("Missing decorator");
                    var node = decorator.Node;
                    var decorators = node.DecoratorIds;
                    decorators.Remove(DecoratorId);
                    if (node.ID != nodeId)
                    {
                        node.SetDecoratorIds(decorators, false);
                        node = Surface.FindNode(nodeId) as Node;
                        decorators = node.DecoratorIds;
                    }
                    if (index < 0 || index >= decorators.Count)
                        decorators.Add(DecoratorId);
                    else
                        decorators.Insert(index, DecoratorId);
                    node.SetDecoratorIds(decorators, false);
                }

                public void Do()
                {
                    Do(NewNodeId, NewIndex);
                }

                public void Undo()
                {
                    Do(PrevNodeId, PrevIndex);
                }

                public void Dispose()
                {
                    Surface = null;
                }
            }

            internal static SurfaceNode Create(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            {
                return new Decorator(id, context, nodeArch, groupArch);
            }

            private DragImage _dragIcon;
            private DragDecorator _dragDecorator;
            private float _dragLocation = -1;

            internal Decorator(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _dragDecorator = new DragDecorator(ValidateDrag);
            }

            public Node Node
            {
                get
                {
                    foreach (var node in Surface.Nodes)
                    {
                        if (node is Node n && n.DecoratorIds.Contains(ID))
                            return n;
                    }
                    return null;
                }
            }

            protected override Color FooterColor => Color.Transparent;

            protected override Float2 CalculateNodeSize(float width, float height)
            {
                if (_debugInfoSize.X > 0)
                {
                    width = Mathf.Max(width, _debugInfoSize.X + 8.0f);
                    height += _debugInfoSize.Y + 8.0f;
                }
                return new Float2(width + FlaxEditor.Surface.Constants.NodeCloseButtonSize * 2 + DecoratorsMarginX * 2, height + FlaxEditor.Surface.Constants.NodeHeaderSize);
            }

            protected override void UpdateRectangles()
            {
                base.UpdateRectangles();

                _footerRect = Rectangle.Empty;
                if (_dragIcon != null)
                    _dragIcon.Bounds = new Rectangle(_closeButtonRect.X - _closeButtonRect.Width, _closeButtonRect.Y, _closeButtonRect.Size);
            }

            protected override void UpdateTitle()
            {
                // Update parent node on title change
                var title = Title;
                base.UpdateTitle();
                if (title != Title)
                    Node?.ResizeAuto();
            }

            protected override void UpdateDebugInfo(BehaviorTreeNode instance, Behavior behavior)
            {
                // Update parent node on debug text change
                var debugInfoSize = _debugInfoSize;
                base.UpdateDebugInfo(instance, behavior);
                if (debugInfoSize != _debugInfoSize)
                    Node?.ResizeAuto();
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                // Add drag button to reorder decorator
                _dragIcon = new DragImage
                {
                    AnchorPreset = AnchorPresets.TopRight,
                    Color = Style.Current.ForegroundGrey,
                    Parent = this,
                    Margin = new Margin(1),
                    Visible = Surface.CanEdit,
                    Brush = new SpriteBrush(Editor.Instance.Icons.DragBar12),
                    Tag = this,
                    Drag = img => { img.DoDragDrop(_dragDecorator.ToDragData(ID)); }
                };

                base.OnLoaded(action);
            }

            private bool ValidateDrag(uint id)
            {
                return Surface.FindNode(id) != null;
            }

            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;

                // Outline
                if (!_isSelected)
                {
                    var rect = new Rectangle(Float2.Zero, Size);
                    Render2D.DrawRectangle(rect, style.BorderHighlighted);
                }

                // Drag hint
                if (IsDragOver && _dragDecorator.HasValidDrag)
                {
                    var rect = new Rectangle(0, _dragLocation < Height * 0.5f ? 0 : Height - 6, Width, 6);
                    Render2D.FillRectangle(rect, style.BackgroundSelected);
                }
            }

            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                if (action == SurfaceNodeActions.Undo)
                {
                    // Update parent node layout when restoring decorator from undo
                    var node = Node;
                    if (node != null)
                    {
                        node._decorators = null;
                        node.ResizeAuto();
                    }
                }
            }

            /// <inheritdoc />
            public override void OnDeleted(SurfaceNodeActions action)
            {
                // Unlink from the current parent (when deleted by user)
                var node = Node;
                if (node != null)
                {
                    if (action == SurfaceNodeActions.User)
                    {
                        var decorators = node.DecoratorIds;
                        decorators.Remove(ID);
                        node.DecoratorIds = decorators;
                    }
                    else
                    {
                        node._decorators = null;
                        node.ResizeAuto();
                    }
                }

                base.OnDeleted(action);
            }

            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                if (_dragIcon != null)
                    _dragIcon.Visible = canEdit;
            }

            public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
            {
                var result = base.OnDragEnter(ref location, data);
                if (result != DragDropEffect.None)
                    return result;
                if (_dragDecorator.OnDragEnter(data))
                {
                    _dragLocation = location.Y;
                    result = DragDropEffect.Move;
                }
                return result;
            }

            public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
            {
                var result = base.OnDragMove(ref location, data);
                if (result != DragDropEffect.None)
                    return result;
                if (_dragDecorator.HasValidDrag)
                {
                    _dragLocation = location.Y;
                    result = DragDropEffect.Move;
                }
                return result;
            }

            public override void OnDragLeave()
            {
                _dragLocation = -1;
                _dragDecorator.OnDragLeave();
                base.OnDragLeave();
            }

            public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
            {
                var result = base.OnDragDrop(ref location, data);
                if (result != DragDropEffect.None)
                    return result;
                if (_dragDecorator.HasValidDrag)
                {
                    // Reorder or reparent decorator
                    var decorator = (Decorator)Surface.FindNode(_dragDecorator.Objects[0]);
                    var prevNode = decorator.Node;
                    var prevIndex = prevNode.Decorators.IndexOf(decorator);
                    var newNode = Node;
                    var newIndex = newNode.Decorators.IndexOf(this);
                    if (_dragLocation >= Height * 0.5f)
                        newIndex++;
                    if (prevIndex != newIndex || prevNode != newNode)
                    {
                        var action = new ReorderDecoratorAction
                        {
                            Surface = Surface,
                            DecoratorId = decorator.ID,
                            PrevNodeId = prevNode.ID,
                            PrevIndex = prevIndex,
                            NewNodeId = newNode.ID,
                            NewIndex = newIndex,
                        };
                        action.Do();
                        Surface.Undo?.AddAction(action);
                    }
                    _dragLocation = -1;
                    _dragDecorator.OnDragDrop();
                    result = DragDropEffect.Move;
                }
                return result;
            }
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            new NodeArchetype
            {
                TypeID = 1, // Task Node
                Create = Node.Create,
                Flags = NodeFlags.BehaviorTreeGraph | NodeFlags.NoSpawnViaGUI,
                DefaultValues = new object[]
                {
                    string.Empty, // Type Name
                    Utils.GetEmptyArray<byte>(), // Instance Data
                    null, // List of Decorator Nodes IDs
                },
                Size = new Float2(100, 0),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, ScriptType.Void, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, ScriptType.Void, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 2, // Root Node
                Create = Node.Create,
                Flags = NodeFlags.BehaviorTreeGraph | NodeFlags.NoSpawnViaGUI,
                DefaultValues = new object[]
                {
                    typeof(BehaviorTreeRootNode).FullName, // Root node
                    Utils.GetEmptyArray<byte>(), // Instance Data
                },
                Size = new Float2(100, 0),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, ScriptType.Void, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, ScriptType.Void, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 3, // Decorator Node
                Create = Decorator.Create,
                Flags = NodeFlags.BehaviorTreeGraph | NodeFlags.NoSpawnViaGUI | NodeFlags.NoMove,
                DefaultValues = new object[]
                {
                    string.Empty, // Type Name
                    Utils.GetEmptyArray<byte>(), // Instance Data
                },
                Size = new Float2(100, 0),
            },
        };
    }
}

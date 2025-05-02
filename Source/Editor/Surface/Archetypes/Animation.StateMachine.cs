// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Archetypes
{
    public static partial class Animation
    {
        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the state machine output node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        /// <seealso cref="FlaxEditor.Surface.ISurfaceContext" />
        internal class StateMachine : SurfaceNode, ISurfaceContext
        {
            private IntValueBox _maxTransitionsPerUpdate;
            private CheckBox _reinitializeOnBecomingRelevant;
            private CheckBox _skipFirstUpdateTransition;

            /// <summary>
            /// Flag for editor UI updating. Used to skip value change events to prevent looping data flow.
            /// </summary>
            protected bool _isUpdatingUI;

            /// <summary>
            /// Gets or sets the node title text.
            /// </summary>
            public string StateMachineTitle
            {
                get => (string)Values[0];
                set
                {
                    if (!string.Equals(value, (string)Values[0], StringComparison.Ordinal))
                    {
                        SetValue(0, value);
                    }
                }
            }

            /// <summary>
            /// Gets or sets the maximum amount of active transitions per update.
            /// </summary>
            public int MaxTransitionsPerUpdate
            {
                get => (int)Values[2];
                set => SetValue(2, value);
            }

            /// <summary>
            /// Gets or sets a value indicating whether reinitialize state machine on becoming relevant (used for blending, etc.).
            /// </summary>
            public bool ReinitializeOnBecomingRelevant
            {
                get => (bool)Values[3];
                set => SetValue(3, value);
            }

            /// <summary>
            /// Gets or sets a value indicating whether skip any triggered transitions during first animation state machine update.
            /// </summary>
            public bool SkipFirstUpdateTransition
            {
                get => (bool)Values[4];
                set => SetValue(4, value);
            }

            /// <inheritdoc />
            public StateMachine(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                var marginX = FlaxEditor.Surface.Constants.NodeMarginX;
                var uiStartPosY = FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize;

                var editButton = new Button(marginX, uiStartPosY, 246, 20)
                {
                    Text = "Edit",
                    Parent = this
                };
                editButton.Clicked += Edit;

                var maxTransitionsPerUpdateLabel = new Label(marginX, editButton.Bottom + 4, 153, TextBox.DefaultHeight)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "Max Transitions Per Update:",
                    Parent = this,
                };

                _maxTransitionsPerUpdate = new IntValueBox(3, maxTransitionsPerUpdateLabel.Right + 4, maxTransitionsPerUpdateLabel.Y, 40, 1, 32, 0.1f);
                _maxTransitionsPerUpdate.ValueChanged += () => MaxTransitionsPerUpdate = _maxTransitionsPerUpdate.Value;
                _maxTransitionsPerUpdate.Parent = this;

                var reinitializeOnBecomingRelevantLabel = new Label(marginX, maxTransitionsPerUpdateLabel.Bottom + 4, 185, TextBox.DefaultHeight)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "Reinitialize On Becoming Relevant:",
                    Parent = this,
                };

                _reinitializeOnBecomingRelevant = new CheckBox(reinitializeOnBecomingRelevantLabel.Right + 4, reinitializeOnBecomingRelevantLabel.Y, true, TextBox.DefaultHeight);
                _reinitializeOnBecomingRelevant.StateChanged += (checkbox) => ReinitializeOnBecomingRelevant = checkbox.Checked;
                _reinitializeOnBecomingRelevant.Parent = this;

                var skipFirstUpdateTransitionLabel = new Label(marginX, reinitializeOnBecomingRelevantLabel.Bottom + 4, 152, TextBox.DefaultHeight)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "Skip First Update Transition:",
                    Parent = this,
                };

                _skipFirstUpdateTransition = new CheckBox(skipFirstUpdateTransitionLabel.Right + 4, skipFirstUpdateTransitionLabel.Y, true, TextBox.DefaultHeight);
                _skipFirstUpdateTransition.StateChanged += (checkbox) => SkipFirstUpdateTransition = checkbox.Checked;
                _skipFirstUpdateTransition.Parent = this;
            }

            /// <summary>
            /// Opens the state machine editing UI.
            /// </summary>
            public void Edit()
            {
                Surface.OpenContext(this);
            }

            /// <summary>
            /// Starts the state machine renaming by showing a rename popup to the user.
            /// </summary>
            public void StartRenaming()
            {
                Surface.Select(this);
                var dialog = RenamePopup.Show(this, _headerRect, Title, false);
                dialog.Validate += OnRenameValidate;
                dialog.Renamed += OnRenamed;
            }

            private bool OnRenameValidate(RenamePopup popup, string value)
            {
                return Context.Nodes.All(node =>
                {
                    if (node != this && node is StateMachine stateMachine)
                        return stateMachine.StateMachineTitle != value;
                    return true;
                });
            }

            private void OnRenamed(RenamePopup renamePopup)
            {
                StateMachineTitle = renamePopup.Text;
            }

            /// <summary>
            /// Updates the editor UI.
            /// </summary>
            protected void UpdateUI()
            {
                if (_isUpdatingUI)
                    return;
                _isUpdatingUI = true;

                _maxTransitionsPerUpdate.Value = MaxTransitionsPerUpdate;
                _reinitializeOnBecomingRelevant.Checked = ReinitializeOnBecomingRelevant;
                _skipFirstUpdateTransition.Checked = SkipFirstUpdateTransition;
                Title = StateMachineTitle;

                _isUpdatingUI = false;
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateUI();
            }

            /// <inheritdoc />
            public override void OnSpawned(SurfaceNodeActions action)
            {
                base.OnSpawned(action);

                // Ensure to have unique name
                var title = StateMachineTitle;
                var value = title;
                int count = 1;
                while (!OnRenameValidate(null, value))
                    value = title + " " + count++;
                Values[0] = value;
                Title = value;

                // Let user pick a name
                StartRenaming();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateUI();
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                if (_headerRect.Contains(ref location))
                {
                    StartRenaming();
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                Surface?.RemoveContext(this);

                _maxTransitionsPerUpdate = null;
                _reinitializeOnBecomingRelevant = null;
                _skipFirstUpdateTransition = null;

                base.OnDestroy();
            }

            /// <inheritdoc />
            public Asset SurfaceAsset => null;

            /// <inheritdoc />
            public string SurfaceName => StateMachineTitle;

            /// <inheritdoc />
            public byte[] SurfaceData
            {
                get => (byte[])Values[1];
                set => Values[1] = value;
            }

            /// <inheritdoc />
            public VisjectSurfaceContext ParentContext => Context;

            /// <inheritdoc />
            public void OnContextCreated(VisjectSurfaceContext context)
            {
                context.Loaded += OnSurfaceLoaded;
            }

            private void OnSurfaceLoaded(VisjectSurfaceContext context)
            {
                // Ensure that loaded surface has entry node for state machine
                if (context.FindNode(9, 19) == null)
                {
                    var wasEnabled = true;
                    if (Surface.Undo != null)
                    {
                        wasEnabled = Surface.Undo.Enabled;
                        Surface.Undo.Enabled = false;
                    }

                    context.SpawnNode(9, 19, new Float2(100.0f));

                    if (Surface.Undo != null)
                    {
                        Surface.Undo.Enabled = wasEnabled;
                    }
                }
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the state machine entry node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        /// <seealso cref="FlaxEditor.Surface.IConnectionInstigator" />
        class StateMachineEntry : SurfaceNode, IConnectionInstigator
        {
            private bool _isMouseDown;
            private Rectangle _textRect;
            private Rectangle _dragAreaRect;
            private bool _cursorChanged = false;
            private bool _textRectHovered = false;

            /// <summary>
            /// Gets or sets the first state node identifier for the state machine pointed by the entry node.
            /// </summary>
            public int FirstStateId
            {
                get => (int)Values[0];
                set => SetValue(0, value);
            }

            /// <summary>
            /// Gets or sets the first state for the state machine pointed by the entry node.
            /// </summary>
            public StateMachineState FirstState
            {
                get => Surface.FindNode(FirstStateId) as StateMachineState;
                set
                {
                    if (FirstState != value)
                    {
                        FirstStateId = value != null ? (int)value.ID : -1;
                    }
                }
            }

            /// <inheritdoc />
            public StateMachineEntry(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private void StartCreatingTransition()
            {
                Surface.ConnectingStart(this);
            }

            /// <inheritdoc />
            protected override void UpdateRectangles()
            {
                base.UpdateRectangles();

                _textRect = new Rectangle(Float2.Zero, Size);

                var style = Style.Current;
                var titleSize = style.FontLarge.MeasureText(Title);
                var width = Mathf.Max(100, titleSize.X + 50);
                Resize(width, 0);
                titleSize.X += 8.0f;
                var padding = new Float2(8, 8);
                _dragAreaRect = new Rectangle(padding, Size - padding * 2);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;

                // Paint Background
                if (_isSelected)
                    Render2D.DrawRectangle(_textRect, style.SelectionBorder);

                BackgroundColor = style.BackgroundNormal;
                var dragAreaColor = BackgroundColor / 2.0f;

                if (_textRectHovered)
                    BackgroundColor *= 1.5f;

                Render2D.FillRectangle(_textRect, BackgroundColor);
                Render2D.FillRectangle(_dragAreaRect, dragAreaColor);

                // Push clipping mask
                if (ClipChildren)
                {
                    GetDesireClientArea(out var clientArea);
                    Render2D.PushClip(ref clientArea);
                }

                DrawChildren();

                // Pop clipping mask
                if (ClipChildren)
                {
                    Render2D.PopClip();
                }

                // Name
                Render2D.DrawText(style.FontLarge, Title, _textRect, style.Foreground, TextAlignment.Center, TextAlignment.Center);
            }

            /// <inheritdoc />
            public override bool CanSelect(ref Float2 location)
            {
                return _dragAreaRect.MakeOffsetted(Location).Contains(ref location);
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && !_dragAreaRect.Contains(ref location))
                {
                    _isMouseDown = true;
                    Cursor = CursorType.Hand;
                    _cursorChanged = true;
                    Focus();
                    return true;
                }

                if (base.OnMouseDown(location, button))
                    return true;

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    _isMouseDown = false;
                    Cursor = CursorType.Default;
                    _cursorChanged = false;
                    Surface.ConnectingEnd(this);
                }

                if (base.OnMouseUp(location, button))
                    return true;

                return false;
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                Surface.ConnectingOver(this);
                if (_dragAreaRect.Contains(location) && !_cursorChanged && !Input.GetMouseButton(MouseButton.Left))
                {
                    Cursor = CursorType.SizeAll;
                    _cursorChanged = true;
                }
                else if (!_dragAreaRect.Contains(location) && _cursorChanged)
                {
                    Cursor = CursorType.Default;
                    _cursorChanged = false;
                }

                if (_textRect.Contains(location) && !_dragAreaRect.Contains(location) && !_textRectHovered)
                {
                    _textRectHovered = true;
                }
                else if (_textRectHovered && (!_textRect.Contains(location) || _dragAreaRect.Contains(location)))
                {
                    _textRectHovered = false;
                }
                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                base.OnMouseLeave();

                if (_cursorChanged)
                {
                    Cursor = CursorType.Default;
                    _cursorChanged = false;
                }

                if (_textRectHovered)
                    _textRectHovered = false;

                if (_isMouseDown)
                {
                    _isMouseDown = false;
                    Cursor = CursorType.Default;

                    StartCreatingTransition();
                }
            }

            /// <inheritdoc />
            public override void DrawConnections(ref Float2 mousePosition)
            {
                var style = Style.Current;
                var targetState = FirstState;
                if (targetState != null)
                {
                    // Draw the connection
                    var center = Size * 0.5f;
                    var startPos = PointToParent(ref center);
                    targetState.GetConnectionEndPoint(ref startPos, out var endPos);
                    var color = style.Foreground;
                    SurfaceStyle.DrawStraightConnection(startPos, endPos, color);
                }
            }

            /// <inheritdoc />
            public override void RemoveConnections()
            {
                base.RemoveConnections();

                FirstState = null;
            }

            /// <inheritdoc />
            public Float2 ConnectionOrigin => Center;

            /// <inheritdoc />
            public bool AreConnected(IConnectionInstigator other)
            {
                return other is StateMachineState state && (int)state.ID == (int)Values[0];
            }

            /// <inheritdoc />
            public bool CanConnectWith(IConnectionInstigator other)
            {
                return other is StateMachineState;
            }

            /// <inheritdoc />
            public void DrawConnectingLine(ref Float2 startPos, ref Float2 endPos, ref Color color)
            {
                SurfaceStyle.DrawStraightConnection(startPos, endPos, color);
            }

            /// <inheritdoc />
            public void Connect(IConnectionInstigator other)
            {
                var state = (StateMachineState)other;

                FirstState = state;
                Surface?.OnNodesConnected(this, other);
            }
        }

        /// <summary>
        /// Base class for state machine state node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        /// <seealso cref="FlaxEditor.Surface.IConnectionInstigator" />
        /// <seealso cref="FlaxEditor.Surface.ISurfaceContext" />
        internal abstract class StateMachineStateBase : SurfaceNode, IConnectionInstigator
        {
            internal class AddRemoveTransitionAction : IUndoAction
            {
                private readonly bool _isAdd;
                private VisjectSurface _surface;
                private ContextHandle _context;
                private readonly uint _srcStateId;
                private readonly uint _dstStateId;
                private StateMachineTransition.Data _data;
                private byte[] _ruleGraph;

                public AddRemoveTransitionAction(StateMachineTransition transition)
                {
                    _surface = transition.SourceState.Surface;
                    _context = new ContextHandle(transition.SourceState.Context);
                    _srcStateId = transition.SourceState.ID;
                    _dstStateId = transition.DestinationState.ID;
                    _isAdd = false;
                    transition.GetData(out _data);
                    _ruleGraph = (byte[])transition.RuleGraph.Clone();
                }

                public AddRemoveTransitionAction(SurfaceNode src, SurfaceNode dst)
                {
                    _surface = src.Surface;
                    _context = new ContextHandle(src.Context);
                    _srcStateId = src.ID;
                    _dstStateId = dst.ID;
                    _isAdd = true;
                    _data = new StateMachineTransition.Data
                    {
                        Flags = StateMachineTransition.Data.FlagTypes.Enabled,
                        Order = 0,
                        BlendDuration = 0.1f,
                        BlendMode = AlphaBlendMode.HermiteCubic,
                    };
                }

                /// <inheritdoc />
                public string ActionString => _isAdd ? "Add transition" : "Remove transition";

                /// <inheritdoc />
                public void Do()
                {
                    if (_isAdd)
                        Add();
                    else
                        Remove();
                }

                /// <inheritdoc />
                public void Undo()
                {
                    if (_isAdd)
                        Remove();
                    else
                        Add();
                }

                private void Add()
                {
                    var context = _context.Get(_surface);

                    var src = context.FindNode(_srcStateId) as StateMachineStateBase;
                    if (src == null)
                        throw new Exception("Missing source state.");

                    var dst = context.FindNode(_dstStateId) as StateMachineStateBase;
                    if (dst == null)
                        throw new Exception("Missing destination state.");

                    var transition = new StateMachineTransition(src, dst, ref _data, _ruleGraph);
                    src.Transitions.Add(transition);

                    src.UpdateTransitionsOrder();
                    src.UpdateTransitions();
                    src.UpdateTransitionsColors();

                    src.SaveTransitions();
                }

                private void Remove()
                {
                    var context = _context.Get(_surface);

                    if (!(context.FindNode(_srcStateId) is StateMachineStateBase src))
                        throw new Exception("Missing source state.");

                    if (!(context.FindNode(_dstStateId) is StateMachineStateBase dst))
                        throw new Exception("Missing destination state.");

                    var transition = src.Transitions.Find(x => x.DestinationState == dst);
                    if (transition == null)
                        throw new Exception("Missing transition.");

                    _surface.RemoveContext(transition);
                    src.Transitions.Remove(transition);

                    src.UpdateTransitionsOrder();
                    src.UpdateTransitions();
                    src.UpdateTransitionsColors();

                    src.SaveTransitions();
                }

                /// <inheritdoc />
                public void Dispose()
                {
                    _surface = null;
                    _context = new ContextHandle();
                    _ruleGraph = null;
                }
            }

            private bool _isSavingData;
            private bool _isMouseDown;
            protected Rectangle _textRect;
            protected Rectangle _dragAreaRect;
            protected Rectangle _renameButtonRect;
            private bool _cursorChanged = false;
            private bool _textRectHovered = false;
            private bool _debugActive;

            /// <summary>
            /// The transitions list from this state to the others.
            /// </summary>
            public readonly List<StateMachineTransition> Transitions = new List<StateMachineTransition>();

            /// <summary>
            /// The transitions rectangle (in surface-space).
            /// </summary>
            public Rectangle TransitionsRectangle = Rectangle.Empty;

            /// <summary>
            /// State transitions data value index (from node values).
            /// </summary>
            public abstract int TransitionsDataIndex { get; }

            /// <inheritdoc />
            protected StateMachineStateBase(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <summary>
            /// Gets the connection end point for the given input position. Puts the end point near the edge of the node bounds.
            /// </summary>
            /// <param name="startPos">The start position (in surface space).</param>
            /// <param name="endPos">The end position (in surface space).</param>
            public void GetConnectionEndPoint(ref Float2 startPos, out Float2 endPos)
            {
                var bounds = new Rectangle(Float2.Zero, Size);
                bounds.Expand(4.0f);
                var upperLeft = bounds.UpperLeft;
                var bottomRight = bounds.BottomRight;
                bounds = Rectangle.FromPoints(PointToParent(ref upperLeft), PointToParent(ref bottomRight));
                CollisionsHelper.ClosestPointRectanglePoint(ref bounds, ref startPos, out endPos);
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                if (!_isSavingData)
                    LoadTransitions();
            }

            /// <inheritdoc />
            protected override void UpdateRectangles()
            {
                base.UpdateRectangles();

                _textRect = new Rectangle(Float2.Zero, Size);
                var padding = new Float2(8, 8);
                _dragAreaRect = new Rectangle(padding, _textRect.Size - padding * 2);
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                LoadTransitions();

                if (Surface != null)
                {
                    // Register for surface mouse events to handle transition arrows interactions
                    Surface.CustomMouseUp += OnSurfaceMouseUp;
                    Surface.CustomMouseDoubleClick += OnSurfaceMouseDoubleClick;
                }
            }

            private void OnSurfaceMouseUp(ref Float2 mouse, MouseButton buttons, ref bool handled)
            {
                if (handled)
                    return;

                // Check click over the connection
                var mousePosition = Surface.SurfaceRoot.PointFromParent(ref mouse);
                if (!TransitionsRectangle.Contains(ref mousePosition))
                    return;
                for (int i = 0; i < Transitions.Count; i++)
                {
                    var t = Transitions[i];
                    if (t.Bounds.Contains(ref mousePosition))
                    {
                        CollisionsHelper.ClosestPointPointLine(ref mousePosition, ref t.StartPos, ref t.EndPos, out var point);
                        if (Float2.DistanceSquared(ref mousePosition, ref point) < 25.0f)
                        {
                            OnTransitionClicked(t, ref mouse, ref mousePosition, buttons);
                            handled = true;
                            return;
                        }
                    }
                }
            }

            private void OnSurfaceMouseDoubleClick(ref Float2 mouse, MouseButton buttons, ref bool handled)
            {
                if (handled)
                    return;

                // Check double click over the connection
                var mousePosition = Surface.SurfaceRoot.PointFromParent(ref mouse);
                if (!TransitionsRectangle.Contains(ref mousePosition))
                    return;
                for (int i = 0; i < Transitions.Count; i++)
                {
                    var t = Transitions[i];
                    if (t.Bounds.Contains(ref mousePosition))
                    {
                        CollisionsHelper.ClosestPointPointLine(ref mousePosition, ref t.StartPos, ref t.EndPos, out var point);
                        if (Float2.DistanceSquared(ref mousePosition, ref point) < 25.0f)
                        {
                            t.EditRule();
                            handled = true;
                            return;
                        }
                    }
                }
            }

            private void OnTransitionClicked(StateMachineTransition transition, ref Float2 mouse, ref Float2 mousePosition, MouseButton button)
            {
                switch (button)
                {
                case MouseButton.Left:
                    transition.Edit();
                    break;
                case MouseButton.Right:
                    var contextMenu = new FlaxEditor.GUI.ContextMenu.ContextMenu();
                    contextMenu.AddButton("Edit").Clicked += transition.Edit;
                    contextMenu.AddSeparator();
                    contextMenu.AddButton("Delete").Clicked += transition.Delete;
                    contextMenu.AddButton("Select source state").Clicked += transition.SelectSourceState;
                    contextMenu.AddButton("Select destination state").Clicked += transition.SelectDestinationState;
                    contextMenu.Show(Surface, mouse);
                    break;
                case MouseButton.Middle:
                    transition.Delete();
                    break;
                }
            }

            /// <summary>
            /// Loads the state data from the node value (reads transitions and related information).
            /// </summary>
            public void LoadTransitions()
            {
                ClearTransitions();

                var bytes = (byte[])Values[TransitionsDataIndex];
                if (bytes == null || bytes.Length == 0)
                {
                    // Empty state
                    return;
                }

                try
                {
                    StateMachineTransition.Data data;
                    using (var stream = new MemoryStream(bytes))
                    using (var reader = new BinaryReader(stream))
                    {
                        int version = reader.ReadInt32();
                        if (version != 1)
                        {
                            Editor.LogError("Invalid state machine state data version.");
                            return;
                        }

                        int tCount = reader.ReadInt32();
                        Transitions.Capacity = Mathf.Max(Transitions.Capacity, tCount);

                        for (int i = 0; i < tCount; i++)
                        {
                            data.Destination = reader.ReadUInt32();
                            data.Flags = (StateMachineTransition.Data.FlagTypes)reader.ReadInt32();
                            data.Order = reader.ReadInt32();
                            data.BlendDuration = reader.ReadSingle();
                            data.BlendMode = (AlphaBlendMode)reader.ReadInt32();
                            data.Unused0 = reader.ReadInt32();
                            data.Unused1 = reader.ReadInt32();
                            data.Unused2 = reader.ReadInt32();

                            int ruleSize = reader.ReadInt32();
                            byte[] rule = null;
                            if (ruleSize != 0)
                                rule = reader.ReadBytes(ruleSize);

                            var destination = Context.FindNode(data.Destination) as StateMachineStateBase;
                            if (destination == null)
                            {
                                Editor.LogWarning("Missing state machine state destination node.");
                                continue;
                            }

                            var t = new StateMachineTransition(this, destination, ref data, rule);
                            Transitions.Add(t);
                        }
                    }
                }
                finally
                {
                    UpdateTransitionsOrder();
                    UpdateTransitions();
                    UpdateTransitionsColors();
                }
            }

            /// <summary>
            /// Saves the state transitions data to the node value.
            /// </summary>
            /// <param name="withUndo">True if save data via node parameter editing via undo or without undo action.</param>
            public void SaveTransitions(bool withUndo = false)
            {
                try
                {
                    _isSavingData = true;

                    byte[] value;
                    if (Transitions.Count == 0)
                    {
                        value = Utils.GetEmptyArray<byte>();
                    }
                    else
                    {
                        StateMachineTransition.Data data;
                        using (var stream = new MemoryStream(512))
                        using (var writer = new BinaryWriter(stream))
                        {
                            writer.Write(1);
                            writer.Write(Transitions.Count);

                            for (int i = 0; i < Transitions.Count; i++)
                            {
                                var t = Transitions[i];
                                t.GetData(out data);
                                var rule = t.RuleGraph;

                                writer.Write(data.Destination);
                                writer.Write((int)data.Flags);
                                writer.Write(data.Order);
                                writer.Write(data.BlendDuration);
                                writer.Write((int)data.BlendMode);
                                writer.Write(data.Unused0);
                                writer.Write(data.Unused1);
                                writer.Write(data.Unused2);

                                if (rule == null || rule.Length == 0)
                                {
                                    writer.Write(0);
                                }
                                else
                                {
                                    writer.Write(rule.Length);
                                    writer.Write(rule);
                                }
                            }

                            value = stream.ToArray();
                        }
                    }

                    if (withUndo)
                        SetValue(TransitionsDataIndex, value);
                    else
                        Values[TransitionsDataIndex] = value;
                }
                finally
                {
                    _isSavingData = false;
                }
            }

            /// <summary>
            /// Clears the state transitions.
            /// </summary>
            public void ClearTransitions()
            {
                Transitions.Clear();
                TransitionsRectangle = Rectangle.Empty;
            }

            private bool IsSoloAndEnabled(StateMachineTransition t)
            {
                return t.Solo && t.Enabled;
            }

            /// <summary>
            /// Updates the transitions order in the list by using the <see cref="StateMachineTransition.Order"/> property.
            /// </summary>
            public void UpdateTransitionsOrder()
            {
                Transitions.Sort((a, b) => b.Order - a.Order);
            }

            /// <summary>
            /// Updates the transitions colors (for disabled/enabled/solo transitions matching).
            /// </summary>
            public void UpdateTransitionsColors()
            {
                var style = Style.Current;
                if (Transitions.Count == 0)
                    return;

                bool anySolo = Transitions.Any(IsSoloAndEnabled);
                if (anySolo)
                {
                    var firstSolo = Transitions.First(IsSoloAndEnabled);
                    for (int i = 0; i < Transitions.Count; i++)
                    {
                        var t = Transitions[i];
                        t.LineColor = t == firstSolo ? style.Foreground : style.ForegroundGrey;
                    }
                }
                else
                {
                    for (int i = 0; i < Transitions.Count; i++)
                    {
                        var t = Transitions[i];
                        t.LineColor = t.Enabled ? style.Foreground : style.ForegroundGrey;
                    }
                }
            }

            /// <summary>
            /// Updates the transitions rectangles.
            /// </summary>
            public void UpdateTransitions()
            {
                for (int i = 0; i < Transitions.Count; i++)
                {
                    var t = Transitions[i];
                    var sourceState = this;
                    var targetState = t.DestinationState;
                    var isBothDirection = targetState.Transitions.Any(x => x.DestinationState == this);

                    Float2 startPos, endPos;
                    if (isBothDirection)
                    {
                        bool diff = string.Compare(sourceState.Title, targetState.Title, StringComparison.Ordinal) > 0;
                        var s1 = diff ? sourceState : targetState;
                        var s2 = diff ? targetState : sourceState;

                        // Two aligned arrows in the opposite direction
                        var center = s1.Size * 0.5f;
                        startPos = s1.PointToParent(ref center);
                        s2.GetConnectionEndPoint(ref startPos, out endPos);
                        s1.GetConnectionEndPoint(ref endPos, out startPos);

                        // Offset a little to not overlap
                        var offset = diff ? -6.0f : 6.0f;
                        var dir = startPos - endPos;
                        dir.Normalize();
                        Float2.Perpendicular(ref dir, out var nrm);
                        nrm *= offset;
                        startPos += nrm;
                        endPos += nrm;

                        // Swap to the other arrow
                        if (!diff)
                        {
                            var tmp = startPos;
                            startPos = endPos;
                            endPos = tmp;
                        }
                    }
                    else
                    {
                        // Single connection over the closest path
                        var center = Size * 0.5f;
                        startPos = PointToParent(ref center);
                        targetState.GetConnectionEndPoint(ref startPos, out endPos);
                        sourceState.GetConnectionEndPoint(ref endPos, out startPos);
                    }

                    t.StartPos = startPos;
                    t.EndPos = endPos;
                    Rectangle.FromPoints(ref startPos, ref endPos, out t.Bounds);
                    t.Bounds.Expand(10.0f);
                }

                if (Transitions.Count > 0)
                {
                    TransitionsRectangle = Transitions[0].Bounds;
                    for (int i = 1; i < Transitions.Count; i++)
                    {
                        Rectangle.Union(ref TransitionsRectangle, ref Transitions[i].Bounds, out TransitionsRectangle);
                    }
                }
                else
                {
                    TransitionsRectangle = Rectangle.Empty;
                }
            }

            private void StartCreatingTransition()
            {
                Surface.ConnectingStart(this);
            }

            /// <inheritdoc />
            public override void Update(float deltaTime)
            {
                base.Update(deltaTime);

                // TODO: maybe update only on actual transitions change?
                UpdateTransitions();

                // Debug current state
                if (((AnimGraphSurface)Surface).TryGetTraceEvent(this, out var traceEvent))
                {
                    _debugActive = true;
                }
                else
                {
                    _debugActive = false;
                }
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;

                // Paint Background
                if (_isSelected)
                    Render2D.DrawRectangle(_textRect, style.SelectionBorder);

                BackgroundColor = style.BackgroundNormal;
                var dragAreaColor = BackgroundColor / 2.0f;

                if (_textRectHovered)
                    BackgroundColor *= 1.5f;

                Render2D.FillRectangle(_textRect, BackgroundColor);
                Render2D.FillRectangle(_dragAreaRect, dragAreaColor);

                // Push clipping mask
                if (ClipChildren)
                {
                    GetDesireClientArea(out var clientArea);
                    Render2D.PushClip(ref clientArea);
                }

                DrawChildren();

                // Pop clipping mask
                if (ClipChildren)
                {
                    Render2D.PopClip();
                }

                // Name
                Render2D.DrawText(style.FontLarge, Title, _textRect, style.Foreground, TextAlignment.Center, TextAlignment.Center);

                // Close button
                Render2D.DrawSprite(style.Cross, _closeButtonRect, _closeButtonRect.Contains(_mousePosition) ? style.Foreground : style.ForegroundGrey);

                // Debug outline
                if (_debugActive)
                    Render2D.DrawRectangle(_textRect.MakeExpanded(1.0f), style.ProgressNormal);
            }

            /// <inheritdoc />
            public override bool CanSelect(ref Float2 location)
            {
                return _dragAreaRect.MakeOffsetted(Location).Contains(ref location);
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                if (_renameButtonRect.Contains(ref location) || _closeButtonRect.Contains(ref location))
                    return true;

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && !_dragAreaRect.Contains(ref location))
                {
                    _isMouseDown = true;
                    Cursor = CursorType.Hand;
                    _cursorChanged = true;
                    Focus();
                    return true;
                }

                if (base.OnMouseDown(location, button))
                    return true;

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    _isMouseDown = false;
                    Cursor = CursorType.Default;
                    _cursorChanged = false;
                    Surface.ConnectingEnd(this);
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                Surface.ConnectingOver(this);
                if (_dragAreaRect.Contains(location) && !_cursorChanged && !_renameButtonRect.Contains(location) && !_closeButtonRect.Contains(location) && !Input.GetMouseButton(MouseButton.Left))
                {
                    Cursor = CursorType.SizeAll;
                    _cursorChanged = true;
                }
                else if ((!_dragAreaRect.Contains(location) || _renameButtonRect.Contains(location) || _closeButtonRect.Contains(location)) && _cursorChanged)
                {
                    Cursor = CursorType.Default;
                    _cursorChanged = false;
                }

                if (_textRect.Contains(location) && !_dragAreaRect.Contains(location) && !_textRectHovered)
                {
                    _textRectHovered = true;
                }
                else if (_textRectHovered && (!_textRect.Contains(location) || _dragAreaRect.Contains(location)))
                {
                    _textRectHovered = false;
                }

                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                base.OnMouseLeave();

                if (_cursorChanged)
                {
                    Cursor = CursorType.Default;
                    _cursorChanged = false;
                }

                if (_textRectHovered)
                    _textRectHovered = false;

                if (_isMouseDown)
                {
                    _isMouseDown = false;
                    Cursor = CursorType.Default;

                    StartCreatingTransition();
                }
            }

            /// <inheritdoc />
            public override void RemoveConnections()
            {
                base.RemoveConnections();

                if (Transitions.Count != 0)
                {
                    Transitions.Clear();
                    UpdateTransitions();
                    SaveTransitions(true);
                }

                for (int i = 0; i < Surface.Nodes.Count; i++)
                {
                    if (Surface.Nodes[i] is StateMachineEntry entry && entry.FirstStateId == ID)
                    {
                        // Break link
                        entry.FirstState = null;
                    }
                    else if (Surface.Nodes[i] is StateMachineStateBase state)
                    {
                        bool modified = false;
                        for (int j = 0; j < state.Transitions.Count && state.Transitions.Count > 0; j++)
                        {
                            if (state.Transitions[j].DestinationState == this)
                            {
                                // Break link
                                state.Transitions.RemoveAt(j--);
                                modified = true;
                            }
                        }
                        if (modified)
                        {
                            state.UpdateTransitions();
                            state.SaveTransitions(true);
                        }
                    }
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                ClearTransitions();

                base.OnDestroy();
            }

            /// <inheritdoc />
            public override void DrawConnections(ref Float2 mousePosition)
            {
                for (int i = 0; i < Transitions.Count; i++)
                {
                    var t = Transitions[i];
                    var isMouseOver = t.Bounds.Contains(ref mousePosition);
                    if (isMouseOver)
                    {
                        CollisionsHelper.ClosestPointPointLine(ref mousePosition, ref t.StartPos, ref t.EndPos, out var point);
                        isMouseOver = Float2.DistanceSquared(ref mousePosition, ref point) < 25.0f;
                    }
                    var color = isMouseOver ? Color.Wheat : t.LineColor;
                    SurfaceStyle.DrawStraightConnection(t.StartPos, t.EndPos, color);
                }
            }

            /// <inheritdoc />
            public Float2 ConnectionOrigin => Center;

            /// <inheritdoc />
            public bool AreConnected(IConnectionInstigator other)
            {
                if (other is StateMachineStateBase otherState)
                    return Transitions.Any(x => x.DestinationState == otherState);
                return false;
            }

            /// <inheritdoc />
            public bool CanConnectWith(IConnectionInstigator other)
            {
                if (other is StateMachineState otherState)
                {
                    // Can connect not connected states
                    return Transitions.All(x => x.DestinationState != otherState);
                }
                return false;
            }

            /// <inheritdoc />
            public void DrawConnectingLine(ref Float2 startPos, ref Float2 endPos, ref Color color)
            {
                SurfaceStyle.DrawStraightConnection(startPos, endPos, color);
            }

            /// <inheritdoc />
            public void Connect(IConnectionInstigator other)
            {
                var state = (StateMachineStateBase)other;
                var action = new AddRemoveTransitionAction(this, state);
                Surface?.AddBatchedUndoAction(action);
                action.Do();
                Surface?.OnNodesConnected(this, other);
                Surface?.MarkAsEdited();
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the state machine state node.
        /// </summary>
        internal class StateMachineState : StateMachineStateBase, ISurfaceContext
        {
            /// <inheritdoc />
            public StateMachineState(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <summary>
            /// Gets or sets the node title text.
            /// </summary>
            public string StateTitle
            {
                get => (string)Values[0];
                set
                {
                    if (!string.Equals(value, (string)Values[0], StringComparison.Ordinal))
                    {
                        SetValue(0, value);
                    }
                }
            }

            /// <summary>
            /// Opens the state editing UI.
            /// </summary>
            public void Edit()
            {
                Cursor = CursorType.Default;
                Surface.OpenContext(this);
            }

            /// <summary>
            /// Starts the state renaming by showing a rename popup to the user.
            /// </summary>
            public void StartRenaming()
            {
                Surface.Select(this);
                var dialog = RenamePopup.Show(this, _textRect, Title, false);
                dialog.Validate += OnRenameValidate;
                dialog.Renamed += OnRenamed;
            }

            private bool OnRenameValidate(RenamePopup popup, string value)
            {
                return Context.Nodes.All(node =>
                {
                    if (node != this && node is StateMachineState state)
                        return state.StateTitle != value;
                    return true;
                });
            }

            private void OnRenamed(RenamePopup renamePopup)
            {
                StateTitle = renamePopup.Text;
            }

            private void UpdateTitle()
            {
                Title = StateTitle;
                var style = Style.Current;
                var titleSize = style.FontLarge.MeasureText(Title);
                var width = Mathf.Max(100, titleSize.X + 50);
                Resize(width, 0);
                titleSize.X += 8.0f;
                var padding = new Float2(8, 8);
                _dragAreaRect = new Rectangle(padding, Size - padding * 2);
            }

            private void OnSurfaceLoaded(VisjectSurfaceContext context)
            {
                // Ensure that loaded surface has output node for state
                if (context.FindNode(9, 21) == null)
                {
                    var wasEnabled = true;
                    var undo = Surface?.Undo;
                    if (undo != null)
                    {
                        wasEnabled = Surface.Undo.Enabled;
                        Surface.Undo.Enabled = false;
                    }

                    context.SpawnNode(9, 21, new Float2(100.0f));

                    if (undo != null)
                    {
                        Surface.Undo.Enabled = wasEnabled;
                    }
                }
            }

            /// <inheritdoc />
            public override int TransitionsDataIndex => 2;

            /// <inheritdoc />
            public override void OnSpawned(SurfaceNodeActions action)
            {
                base.OnSpawned(action);

                // Ensure to have unique name
                var title = StateTitle;
                var value = title;
                int count = 1;
                while (!OnRenameValidate(null, value))
                {
                    value = title + " " + count++;
                }
                Values[0] = value;
                Title = value;

                // Let user pick a name
                StartRenaming();
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateTitle();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateTitle();
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;

                // Rename button
                Render2D.DrawSprite(style.Settings, _renameButtonRect, _renameButtonRect.Contains(_mousePosition) ? style.Foreground : style.ForegroundGrey);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (base.OnMouseUp(location, button))
                    return true;

                // Rename
                if (_renameButtonRect.Contains(ref location))
                {
                    StartRenaming();
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                Edit();
                return true;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                Surface?.RemoveContext(this);

                base.OnDestroy();
            }

            /// <inheritdoc />
            protected override void UpdateRectangles()
            {
                base.UpdateRectangles();

                const float buttonMargin = FlaxEditor.Surface.Constants.NodeCloseButtonMargin;
                const float buttonSize = FlaxEditor.Surface.Constants.NodeCloseButtonSize;
                _renameButtonRect = new Rectangle(_closeButtonRect.Left - buttonSize - buttonMargin, buttonMargin, buttonSize, buttonSize);
            }

            /// <inheritdoc />
            public Asset SurfaceAsset => null;

            /// <inheritdoc />
            public string SurfaceName => StateTitle;

            /// <inheritdoc />
            public byte[] SurfaceData
            {
                get => (byte[])Values[1];
                set => Values[1] = value;
            }

            /// <inheritdoc />
            public VisjectSurfaceContext ParentContext => Context;

            /// <inheritdoc />
            public void OnContextCreated(VisjectSurfaceContext context)
            {
                context.Loaded += OnSurfaceLoaded;
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the state machine Any state node.
        /// </summary>
        internal class StateMachineAny : StateMachineStateBase
        {
            /// <inheritdoc />
            public StateMachineAny(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override int TransitionsDataIndex => 0;
        }

        /// <summary>
        /// State machine transition data container object.
        /// </summary>
        /// <seealso cref="StateMachineStateBase"/>
        /// <seealso cref="ISurfaceContext"/>
        internal class StateMachineTransition : ISurfaceContext
        {
            /// <summary>
            /// State transition interruption flags.
            /// </summary>
            [Flags]
            public enum InterruptionFlags
            {
                /// <summary>
                /// Nothing.
                /// </summary>
                None = 0,

                /// <summary>
                /// Transition rule will be rechecked during active transition with option to interrupt transition (to go back to the source state).
                /// </summary>
                RuleRechecking = 1,

                /// <summary>
                /// Interrupted transition is immediately stopped without blending out (back to the source/destination state).
                /// </summary>
                Instant = 2,

                /// <summary>
                /// Enables checking other transitions in the source state that might interrupt this one.
                /// </summary>
                SourceState = 4,

                /// <summary>
                /// Enables checking transitions in the destination state that might interrupt this one.
                /// </summary>
                DestinationState = 8,
            }

            /// <summary>
            /// The packed data container for the transition data storage. Helps with serialization and versioning the data.
            /// Must match AnimGraphBase::LoadStateTransition in C++
            /// </summary>
            /// <remarks>
            /// It does not store GC objects references to make it more lightweight. Transition rule bytes data is stores in a separate way.
            /// </remarks>
            [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 32)]
            internal struct Data
            {
                // Must match AnimGraphStateTransition::FlagTypes
                [Flags]
                public enum FlagTypes
                {
                    None = 0,
                    Enabled = 1,
                    Solo = 2,
                    UseDefaultRule = 4,
                    InterruptionRuleRechecking = 8,
                    InterruptionInstant = 16,
                    InterruptionSourceState = 32,
                    InterruptionDestinationState = 64,
                }

                /// <summary>
                /// The destination state node ID.
                /// </summary>
                public uint Destination;

                /// <summary>
                /// The flags.
                /// </summary>
                public FlagTypes Flags;

                /// <summary>
                /// The order.
                /// </summary>
                public int Order;

                /// <summary>
                /// The blend duration (in seconds).
                /// </summary>
                public float BlendDuration;

                /// <summary>
                /// The blend mode.
                /// </summary>
                public AlphaBlendMode BlendMode;

                /// <summary>
                /// The unused data 0.
                /// </summary>
                public int Unused0;

                /// <summary>
                /// The unused data 1.
                /// </summary>
                public int Unused1;

                /// <summary>
                /// The unused data 2.
                /// </summary>
                public int Unused2;

                public bool HasFlag(FlagTypes flag)
                {
                    return (Flags & flag) == flag;
                }

                public void SetFlag(FlagTypes flag, bool value)
                {
                    if (value)
                        Flags |= flag;
                    else
                        Flags &= ~flag;
                }
            }

            private Data _data;
            private byte[] _ruleGraph;

            /// <summary>
            /// The transition start state.
            /// </summary>
            [HideInEditor]
            public readonly StateMachineStateBase SourceState;

            /// <summary>
            /// The transition end state.
            /// </summary>
            [HideInEditor]
            public readonly StateMachineStateBase DestinationState;

            /// <summary>
            /// If checked, the transition can be triggered, otherwise it will be ignored.
            /// </summary>
            [EditorOrder(10), DefaultValue(true)]
            public bool Enabled
            {
                get => _data.HasFlag(Data.FlagTypes.Enabled);
                set
                {
                    _data.SetFlag(Data.FlagTypes.Enabled, value);
                    SourceState.UpdateTransitionsColors();
                    SourceState.SaveTransitions(true);
                }
            }

            /// <summary>
            /// If checked, animation graph will ignore other transitions from the source state and use only this transition.
            /// </summary>
            [EditorOrder(20), DefaultValue(false)]
            public bool Solo
            {
                get => _data.HasFlag(Data.FlagTypes.Solo);
                set
                {
                    _data.SetFlag(Data.FlagTypes.Solo, value);
                    SourceState.UpdateTransitionsColors();
                    SourceState.SaveTransitions(true);
                }
            }

            /// <summary>
            /// If checked, animation graph will perform automatic transition based on the state animation pose (single shot animation play).
            /// </summary>
            [EditorOrder(30), DefaultValue(false)]
            public bool UseDefaultRule
            {
                get => _data.HasFlag(Data.FlagTypes.UseDefaultRule);
                set
                {
                    _data.SetFlag(Data.FlagTypes.UseDefaultRule, value);
                    SourceState.SaveTransitions(true);
                }
            }

            /// <summary>
            /// The transition order. Transitions with the higher order are handled before the ones with the lower order.
            /// </summary>
            [EditorOrder(40), DefaultValue(0)]
            public int Order
            {
                get => _data.Order;
                set
                {
                    _data.Order = value;
                    SourceState.UpdateTransitionsOrder();
                    SourceState.UpdateTransitionsColors();
                    SourceState.SaveTransitions(true);
                }
            }

            /// <summary>
            /// The blend duration (in seconds).
            /// </summary>
            [EditorOrder(50), DefaultValue(0.1f), Limit(0, 20.0f, 0.1f)]
            public float BlendDuration
            {
                get => _data.BlendDuration;
                set
                {
                    _data.BlendDuration = value;
                    SourceState.SaveTransitions(true);
                }
            }

            /// <summary>
            /// Transition blending mode for blend alpha.
            /// </summary>
            [EditorOrder(60), DefaultValue(AlphaBlendMode.HermiteCubic)]
            public AlphaBlendMode BlendMode
            {
                get => _data.BlendMode;
                set
                {
                    _data.BlendMode = value;
                    SourceState.SaveTransitions(true);
                }
            }

            /// <summary>
            /// Transition interruption options (flags, can select multiple values).
            /// </summary>
            [EditorOrder(70), DefaultValue(InterruptionFlags.None)]
            public InterruptionFlags Interruption
            {
                get
                {
                    var flags = InterruptionFlags.None;
                    if (_data.HasFlag(Data.FlagTypes.InterruptionRuleRechecking))
                        flags |= InterruptionFlags.RuleRechecking;
                    if (_data.HasFlag(Data.FlagTypes.InterruptionInstant))
                        flags |= InterruptionFlags.Instant;
                    if (_data.HasFlag(Data.FlagTypes.InterruptionSourceState))
                        flags |= InterruptionFlags.SourceState;
                    if (_data.HasFlag(Data.FlagTypes.InterruptionDestinationState))
                        flags |= InterruptionFlags.DestinationState;
                    return flags;
                }
                set
                {
                    _data.SetFlag(Data.FlagTypes.InterruptionRuleRechecking, value.HasFlag(InterruptionFlags.RuleRechecking));
                    _data.SetFlag(Data.FlagTypes.InterruptionInstant, value.HasFlag(InterruptionFlags.Instant));
                    _data.SetFlag(Data.FlagTypes.InterruptionSourceState, value.HasFlag(InterruptionFlags.SourceState));
                    _data.SetFlag(Data.FlagTypes.InterruptionDestinationState, value.HasFlag(InterruptionFlags.DestinationState));
                    SourceState.SaveTransitions(true);
                }
            }

            /// <summary>
            /// The rule graph data.
            /// </summary>
            [HideInEditor]
            public byte[] RuleGraph
            {
                get => _ruleGraph;
                set
                {
                    _ruleGraph = value ?? Utils.GetEmptyArray<byte>();
                    SourceState.SaveTransitions();
                }
            }

            /// <summary>
            /// The start position (cached).
            /// </summary>
            [HideInEditor]
            public Float2 StartPos;

            /// <summary>
            /// The end position (cached).
            /// </summary>
            [HideInEditor]
            public Float2 EndPos;

            /// <summary>
            /// The bounds of the transition connection line (cached).
            /// </summary>
            [HideInEditor]
            public Rectangle Bounds;

            /// <summary>
            /// The color of the transition connection line (cached).
            /// </summary>
            [HideInEditor]
            public Color LineColor;

            /// <summary>
            /// Initializes a new instance of the <see cref="StateMachineTransition"/> class.
            /// </summary>
            /// <param name="source">The source.</param>
            /// <param name="destination">The destination.</param>
            /// <param name="data">The transition data container.</param>
            /// <param name="ruleGraph">The transition rule graph. Can be null.</param>
            public StateMachineTransition(StateMachineStateBase source, StateMachineStateBase destination, ref Data data, byte[] ruleGraph = null)
            {
                SourceState = source;
                DestinationState = destination;
                _data = data;
                _data.Destination = destination.ID;
                _ruleGraph = ruleGraph ?? Utils.GetEmptyArray<byte>();
            }

            /// <summary>
            /// Gets the transition data.
            /// </summary>
            /// <param name="data">The data.</param>
            public void GetData(out Data data)
            {
                data = _data;
            }

            /// <inheritdoc />
            public Asset SurfaceAsset => null;

            /// <inheritdoc />
            public string SurfaceName => string.Format("{0} to {1}", SourceState.Title, DestinationState.Title);

            /// <inheritdoc />
            [HideInEditor]
            public byte[] SurfaceData
            {
                get => RuleGraph;
                set => RuleGraph = value;
            }

            /// <inheritdoc />
            public VisjectSurfaceContext ParentContext => SourceState.Context;

            /// <inheritdoc />
            public void OnContextCreated(VisjectSurfaceContext context)
            {
                context.Loaded += OnSurfaceLoaded;
            }

            private void OnSurfaceLoaded(VisjectSurfaceContext context)
            {
                // Ensure that loaded surface has rule output node
                if (context.FindNode(9, 22) == null)
                {
                    var wasEnabled = true;
                    var undo = SourceState.Surface?.Undo;
                    if (undo != null)
                    {
                        wasEnabled = undo.Enabled;
                        undo.Enabled = false;
                    }

                    context.SpawnNode(9, 22, new Float2(100.0f));

                    // TODO: add default rule nodes for easier usage

                    if (undo != null)
                    {
                        undo.Enabled = wasEnabled;
                    }
                }
            }

            /// <summary>
            /// Removes the transition.
            /// </summary>
            public void Delete()
            {
                var action = new StateMachineStateBase.AddRemoveTransitionAction(this);
                SourceState.Surface?.AddBatchedUndoAction(action);
                SourceState.Surface?.MarkAsEdited();
                action.Do();
            }

            /// <summary>
            /// Selects the source state node of the transition
            /// </summary>
            public void SelectSourceState()
            {
                SourceState.Surface.Select(SourceState);
            }

            /// <summary>
            /// Selects the destination state node of the transition
            /// </summary>
            public void SelectDestinationState()
            {
                DestinationState.Surface.Select(DestinationState);
            }

            /// <summary>
            /// Opens the transition editor popup.
            /// </summary>
            public void Edit()
            {
                var surface = SourceState.Surface;
                var center = Bounds.Center + new Float2(3.0f);
                var editor = new TransitionEditor(this);
                editor.Show(surface, surface.SurfaceRoot.PointToParent(ref center));
            }

            /// <summary>
            /// Opens the transition rule editing UI.
            /// </summary>
            public void EditRule()
            {
                SourceState.Surface.OpenContext(this);
            }
        }
    }
}

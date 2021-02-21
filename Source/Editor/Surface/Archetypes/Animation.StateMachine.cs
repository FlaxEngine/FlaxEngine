// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                UpdateUI();
            }

            /// <inheritdoc />
            public override void OnSpawned()
            {
                base.OnSpawned();

                // Ensure to have unique name
                var title = StateMachineTitle;
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
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateUI();
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
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
                Surface.RemoveContext(this);

                _maxTransitionsPerUpdate = null;
                _reinitializeOnBecomingRelevant = null;
                _skipFirstUpdateTransition = null;

                base.OnDestroy();
            }

            /// <inheritdoc />
            public string SurfaceName => StateMachineTitle;

            /// <inheritdoc />
            public byte[] SurfaceData
            {
                get => (byte[])Values[1];
                set => Values[1] = value;
            }

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

                    context.SpawnNode(9, 19, new Vector2(100.0f));

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

                _textRect = new Rectangle(Vector2.Zero, Size);

                var style = Style.Current;
                var titleSize = style.FontLarge.MeasureText(Title);
                var width = Mathf.Max(100, titleSize.X + 50);
                Resize(width, 0);
                titleSize.X += 8.0f;
                _dragAreaRect = new Rectangle((Size - titleSize) * 0.5f, titleSize);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;

                // Paint Background
                BackgroundColor = _isSelected ? Color.OrangeRed : style.BackgroundNormal;
                if (IsMouseOver)
                    BackgroundColor *= 1.2f;
                Render2D.FillRectangle(_textRect, BackgroundColor);

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
            public override bool CanSelect(ref Vector2 location)
            {
                return _dragAreaRect.MakeOffsetted(Location).Contains(ref location);
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
            {
                if (button == MouseButton.Left && !_dragAreaRect.Contains(ref location))
                {
                    _isMouseDown = true;
                    Cursor = CursorType.Hand;
                    Focus();
                    return true;
                }

                if (base.OnMouseDown(location, button))
                    return true;

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Vector2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    _isMouseDown = false;
                    Cursor = CursorType.Default;
                    Surface.ConnectingEnd(this);
                }

                if (base.OnMouseUp(location, button))
                    return true;

                return false;
            }

            /// <inheritdoc />
            public override void OnMouseMove(Vector2 location)
            {
                Surface.ConnectingOver(this);
                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                base.OnMouseLeave();

                if (_isMouseDown)
                {
                    _isMouseDown = false;
                    Cursor = CursorType.Default;

                    StartCreatingTransition();
                }
            }

            /// <inheritdoc />
            public override void DrawConnections(ref Vector2 mousePosition)
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
                    StateMachineState.DrawConnection(Surface, ref startPos, ref endPos, ref color);
                }
            }

            /// <inheritdoc />
            public override void RemoveConnections()
            {
                base.RemoveConnections();

                FirstState = null;
            }

            /// <inheritdoc />
            public Vector2 ConnectionOrigin => Center;

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
            public void DrawConnectingLine(ref Vector2 startPos, ref Vector2 endPos, ref Color color)
            {
                StateMachineState.DrawConnection(Surface, ref startPos, ref endPos, ref color);
            }

            /// <inheritdoc />
            public void Connect(IConnectionInstigator other)
            {
                var state = (StateMachineState)other;

                FirstState = state;
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the state machine state node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        /// <seealso cref="FlaxEditor.Surface.IConnectionInstigator" />
        /// <seealso cref="FlaxEditor.Surface.ISurfaceContext" />
        internal class StateMachineState : SurfaceNode, ISurfaceContext, IConnectionInstigator
        {
            public class AddRemoveTransitionAction : IUndoAction
            {
                private readonly bool _isAdd;
                private VisjectSurface _surface;
                private ContextHandle _context;
                private readonly uint _srcStateId;
                private readonly uint _dstStateId;
                private StateMachineTransition.Data _data;

                public AddRemoveTransitionAction(StateMachineTransition transition)
                {
                    _surface = transition.SourceState.Surface;
                    _context = new ContextHandle(transition.SourceState.Context);
                    _srcStateId = transition.SourceState.ID;
                    _dstStateId = transition.DestinationState.ID;
                    _isAdd = false;
                    transition.GetData(out _data);
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

                    var src = context.FindNode(_srcStateId) as StateMachineState;
                    if (src == null)
                        throw new Exception("Missing source state.");

                    var dst = context.FindNode(_dstStateId) as StateMachineState;
                    if (dst == null)
                        throw new Exception("Missing destination state.");

                    var transition = new StateMachineTransition(src, dst, ref _data);
                    src.Transitions.Add(transition);

                    src.UpdateTransitionsOrder();
                    src.UpdateTransitions();
                    src.UpdateTransitionsColors();

                    src.SaveData();
                }

                private void Remove()
                {
                    var context = _context.Get(_surface);

                    if (!(context.FindNode(_srcStateId) is StateMachineState src))
                        throw new Exception("Missing source state.");

                    if (!(context.FindNode(_dstStateId) is StateMachineState dst))
                        throw new Exception("Missing destination state.");

                    var transition = src.Transitions.Find(x => x.DestinationState == dst);
                    if (transition == null)
                        throw new Exception("Missing transition.");

                    _surface.RemoveContext(transition);
                    src.Transitions.Remove(transition);

                    src.UpdateTransitionsOrder();
                    src.UpdateTransitions();
                    src.UpdateTransitionsColors();

                    src.SaveData();
                }

                /// <inheritdoc />
                public void Dispose()
                {
                    _surface = null;
                }
            }

            private bool _isSavingData;
            private bool _isMouseDown;
            private Rectangle _textRect;
            private Rectangle _dragAreaRect;
            private Rectangle _renameButtonRect;

            /// <summary>
            /// The transitions list from this state to the others.
            /// </summary>
            public readonly List<StateMachineTransition> Transitions = new List<StateMachineTransition>();

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
            /// Gets or sets the state data (transitions list with rules graph and other options).
            /// </summary>
            public byte[] StateData
            {
                get => (byte[])Values[2];
                set => Values[2] = value;
            }

            /// <summary>
            /// The transitions rectangle (in surface-space).
            /// </summary>
            public Rectangle TransitionsRectangle;

            /// <inheritdoc />
            public StateMachineState(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                TransitionsRectangle = Rectangle.Empty;
            }

            /// <summary>
            /// Draws the connection between two state machine nodes.
            /// </summary>
            /// <param name="surface">The surface.</param>
            /// <param name="startPos">The start position.</param>
            /// <param name="endPos">The end position.</param>
            /// <param name="color">The line color.</param>
            public static void DrawConnection(VisjectSurface surface, ref Vector2 startPos, ref Vector2 endPos, ref Color color)
            {
                var sub = endPos - startPos;
                var length = sub.Length;
                if (length > Mathf.Epsilon)
                {
                    var dir = sub / length;
                    var arrowRect = new Rectangle(0, 0, 16.0f, 16.0f);
                    float rotation = Vector2.Dot(dir, Vector2.UnitY);
                    if (endPos.X < startPos.X)
                        rotation = 2 - rotation;
                    // TODO: make it look better (fix the math)
                    var arrowTransform = Matrix3x3.Translation2D(new Vector2(-16.0f, -8.0f)) * Matrix3x3.RotationZ(rotation * Mathf.PiOverTwo) * Matrix3x3.Translation2D(endPos);

                    Render2D.PushTransform(ref arrowTransform);
                    Render2D.DrawSprite(surface.Style.Icons.ArrowClose, arrowRect, color);
                    Render2D.PopTransform();

                    endPos -= dir * 4.0f;
                }
                Render2D.DrawLine(startPos, endPos, color);
            }

            /// <summary>
            /// Gets the connection end point for the given input position. Puts the end point near the edge of the node bounds.
            /// </summary>
            /// <param name="startPos">The start position (in surface space).</param>
            /// <param name="endPos">The end position (in surface space).</param>
            public void GetConnectionEndPoint(ref Vector2 startPos, out Vector2 endPos)
            {
                var bounds = new Rectangle(Vector2.Zero, Size);
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
                    LoadData();
                UpdateTitle();
            }

            private void UpdateTitle()
            {
                Title = StateTitle;
                var style = Style.Current;
                var titleSize = style.FontLarge.MeasureText(Title);
                var width = Mathf.Max(100, titleSize.X + 50);
                Resize(width, 0);
                titleSize.X += 8.0f;
                _dragAreaRect = new Rectangle((Size - titleSize) * 0.5f, titleSize);
            }

            /// <inheritdoc />
            protected override void UpdateRectangles()
            {
                base.UpdateRectangles();

                const float buttonMargin = FlaxEditor.Surface.Constants.NodeCloseButtonMargin;
                const float buttonSize = FlaxEditor.Surface.Constants.NodeCloseButtonSize;
                _renameButtonRect = new Rectangle(_closeButtonRect.Left - buttonSize - buttonMargin, buttonMargin, buttonSize, buttonSize);
                _textRect = new Rectangle(Vector2.Zero, Size);
                _dragAreaRect = _headerRect;
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                UpdateTitle();
                LoadData();

                // Register for surface mouse events to handle transition arrows interactions
                Surface.CustomMouseUp += OnSurfaceMouseUp;
                Surface.CustomMouseDoubleClick += OnSurfaceMouseDoubleClick;
            }

            private void OnSurfaceMouseUp(ref Vector2 mouse, MouseButton buttons, ref bool handled)
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
                        if (Vector2.DistanceSquared(ref mousePosition, ref point) < 25.0f)
                        {
                            OnTransitionClicked(t, ref mouse, ref mousePosition, buttons);
                            handled = true;
                            return;
                        }
                    }
                }
            }

            private void OnSurfaceMouseDoubleClick(ref Vector2 mouse, MouseButton buttons, ref bool handled)
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
                        if (Vector2.DistanceSquared(ref mousePosition, ref point) < 25.0f)
                        {
                            t.EditRule();
                            handled = true;
                            return;
                        }
                    }
                }
            }

            private void OnTransitionClicked(StateMachineTransition transition, ref Vector2 mouse, ref Vector2 mousePosition, MouseButton button)
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

            /// <inheritdoc />
            public override void OnSpawned()
            {
                base.OnSpawned();

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

            /// <summary>
            /// Loads the state data from the node value (reads transitions and related information).
            /// </summary>
            public void LoadData()
            {
                ClearData();

                var bytes = StateData;
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

                            var destination = Context.FindNode(data.Destination) as StateMachineState;
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
            /// Saves the state data to the node value (writes transitions and related information).
            /// </summary>
            /// <param name="withUndo">True if save data via node parameter editing via undo or without undo action.</param>
            public void SaveData(bool withUndo = false)
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
                        SetValue(2, value);
                    else
                        Values[2] = value;
                }
                finally
                {
                    _isSavingData = false;
                }
            }

            /// <summary>
            /// Clears the state data (removes transitions and related information).
            /// </summary>
            public void ClearData()
            {
                Transitions.Clear();
                TransitionsRectangle = Rectangle.Empty;
            }

            /// <summary>
            /// Opens the state editing UI.
            /// </summary>
            public void Edit()
            {
                Surface.OpenContext(this);
            }

            private bool IsSoloAndEnabled(StateMachineTransition t)
            {
                return t.Solo && t.Enabled;
            }

            /// <summary>
            /// Updates the transitions order in the list vy using the <see cref="StateMachineTransition.Order"/> property.
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

                    Vector2 startPos, endPos;
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
                        Vector2.Perpendicular(ref dir, out var nrm);
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
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;

                // Paint Background
                BackgroundColor = _isSelected ? Color.OrangeRed : style.BackgroundNormal;
                if (IsMouseOver)
                    BackgroundColor *= 1.2f;
                Render2D.FillRectangle(_textRect, BackgroundColor);

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

                // Rename button
                Render2D.DrawSprite(style.Settings, _renameButtonRect, _renameButtonRect.Contains(_mousePosition) ? style.Foreground : style.ForegroundGrey);
            }

            /// <inheritdoc />
            public override bool CanSelect(ref Vector2 location)
            {
                return _dragAreaRect.MakeOffsetted(Location).Contains(ref location);
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                if (_renameButtonRect.Contains(ref location) || _closeButtonRect.Contains(ref location))
                    return true;

                Edit();
                return true;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
            {
                if (button == MouseButton.Left && !_dragAreaRect.Contains(ref location))
                {
                    _isMouseDown = true;
                    Cursor = CursorType.Hand;
                    Focus();
                    return true;
                }

                if (base.OnMouseDown(location, button))
                    return true;

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Vector2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    _isMouseDown = false;
                    Cursor = CursorType.Default;
                    Surface.ConnectingEnd(this);
                }

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
            public override void OnMouseMove(Vector2 location)
            {
                Surface.ConnectingOver(this);
                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                base.OnMouseLeave();

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
                    SaveData(true);
                }

                for (int i = 0; i < Surface.Nodes.Count; i++)
                {
                    if (Surface.Nodes[i] is StateMachineEntry entry && entry.FirstStateId == ID)
                    {
                        // Break link
                        entry.FirstState = null;
                    }
                    else if (Surface.Nodes[i] is StateMachineState state)
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
                            state.SaveData(true);
                        }
                    }
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                Surface.RemoveContext(this);

                ClearData();

                base.OnDestroy();
            }

            /// <inheritdoc />
            public string SurfaceName => StateTitle;

            /// <inheritdoc />
            public byte[] SurfaceData
            {
                get => (byte[])Values[1];
                set => Values[1] = value;
            }

            /// <inheritdoc />
            public void OnContextCreated(VisjectSurfaceContext context)
            {
                context.Loaded += OnSurfaceLoaded;
            }

            private void OnSurfaceLoaded(VisjectSurfaceContext context)
            {
                // Ensure that loaded surface has output node for state
                if (context.FindNode(9, 21) == null)
                {
                    var wasEnabled = true;
                    if (Surface.Undo != null)
                    {
                        wasEnabled = Surface.Undo.Enabled;
                        Surface.Undo.Enabled = false;
                    }

                    context.SpawnNode(9, 21, new Vector2(100.0f));

                    if (Surface.Undo != null)
                    {
                        Surface.Undo.Enabled = wasEnabled;
                    }
                }
            }

            /// <inheritdoc />
            public override void DrawConnections(ref Vector2 mousePosition)
            {
                for (int i = 0; i < Transitions.Count; i++)
                {
                    var t = Transitions[i];
                    var isMouseOver = t.Bounds.Contains(ref mousePosition);
                    if (isMouseOver)
                    {
                        CollisionsHelper.ClosestPointPointLine(ref mousePosition, ref t.StartPos, ref t.EndPos, out var point);
                        isMouseOver = Vector2.DistanceSquared(ref mousePosition, ref point) < 25.0f;
                    }
                    var color = isMouseOver ? Color.Wheat : t.LineColor;
                    DrawConnection(Surface, ref t.StartPos, ref t.EndPos, ref color);
                }
            }

            /// <inheritdoc />
            public Vector2 ConnectionOrigin => Center;

            /// <inheritdoc />
            public bool AreConnected(IConnectionInstigator other)
            {
                if (other is StateMachineState otherState)
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
            public void DrawConnectingLine(ref Vector2 startPos, ref Vector2 endPos, ref Color color)
            {
                DrawConnection(Surface, ref startPos, ref endPos, ref color);
            }

            /// <inheritdoc />
            public void Connect(IConnectionInstigator other)
            {
                var state = (StateMachineState)other;

                var action = new AddRemoveTransitionAction(this, state);
                Surface?.Undo.AddAction(action);
                action.Do();
            }
        }

        /// <summary>
        /// State machine transition data container object.
        /// </summary>
        /// <seealso cref="StateMachineState"/>
        /// <seealso cref="ISurfaceContext"/>
        internal class StateMachineTransition : ISurfaceContext
        {
            /// <summary>
            /// The packed data container for the transition data storage. Helps with serialization and versioning the data.
            /// </summary>
            /// <remarks>
            /// It does not store GC objects references to make it more lightweight. Transition rule bytes data is stores in a separate way.
            /// </remarks>
            [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 32)]
            public struct Data
            {
                /// <summary>
                /// The transition flag types.
                /// </summary>
                [Flags]
                public enum FlagTypes
                {
                    /// <summary>
                    /// The none.
                    /// </summary>
                    None = 0,

                    /// <summary>
                    /// The enabled flag.
                    /// </summary>
                    Enabled = 1,

                    /// <summary>
                    /// The solo flag.
                    /// </summary>
                    Solo = 2,

                    /// <summary>
                    /// The use default rule flag.
                    /// </summary>
                    UseDefaultRule = 4,
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

                /// <summary>
                /// Determines whether the data has a given flag set.
                /// </summary>
                /// <param name="flag">The flag.</param>
                /// <returns><c>true</c> if the specified flag is set; otherwise, <c>false</c>.</returns>
                public bool HasFlag(FlagTypes flag)
                {
                    return (Flags & flag) == flag;
                }

                /// <summary>
                /// Sets the flag to the given value.
                /// </summary>
                /// <param name="flag">The flag.</param>
                /// <param name="value">If set to <c>true</c> the flag will be set, otherwise it will be cleared.</param>
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
            public readonly StateMachineState SourceState;

            /// <summary>
            /// The transition end state.
            /// </summary>
            [HideInEditor]
            public readonly StateMachineState DestinationState;

            /// <summary>
            /// If checked, the transition can be triggered, otherwise it will be ignored.
            /// </summary>
            [EditorOrder(10), DefaultValue(true), Tooltip("If checked, the transition can be triggered, otherwise it will be ignored.")]
            public bool Enabled
            {
                get => _data.HasFlag(Data.FlagTypes.Enabled);
                set
                {
                    _data.SetFlag(Data.FlagTypes.Enabled, value);
                    SourceState.UpdateTransitionsColors();
                    SourceState.SaveData(true);
                }
            }

            /// <summary>
            /// If checked, animation graph will ignore other transitions from the source state and use only this transition.
            /// </summary>
            [EditorOrder(20), DefaultValue(false), Tooltip("If checked, animation graph will ignore other transitions from the source state and use only this transition.")]
            public bool Solo
            {
                get => _data.HasFlag(Data.FlagTypes.Solo);
                set
                {
                    _data.SetFlag(Data.FlagTypes.Solo, value);
                    SourceState.UpdateTransitionsColors();
                    SourceState.SaveData(true);
                }
            }

            /// <summary>
            /// If checked, animation graph will perform automatic transition based on the state animation pose (single shot animation play).
            /// </summary>
            [EditorOrder(30), DefaultValue(false), Tooltip("If checked, animation graph will perform automatic transition based on the state animation pose (single shot animation play).")]
            public bool UseDefaultRule
            {
                get => _data.HasFlag(Data.FlagTypes.UseDefaultRule);
                set
                {
                    _data.SetFlag(Data.FlagTypes.UseDefaultRule, value);
                    SourceState.SaveData(true);
                }
            }

            /// <summary>
            /// The transition order (higher first).
            /// </summary>
            [EditorOrder(40), DefaultValue(0), Tooltip("The transition order. Transitions with the higher order are handled before the ones with the lower order.")]
            public int Order
            {
                get => _data.Order;
                set
                {
                    _data.Order = value;
                    SourceState.UpdateTransitionsOrder();
                    SourceState.UpdateTransitionsColors();
                    SourceState.SaveData(true);
                }
            }

            /// <summary>
            /// The blend duration (in seconds).
            /// </summary>
            [EditorOrder(50), DefaultValue(0.1f), Limit(0, 20.0f, 0.1f), Tooltip("Transition blend duration (in seconds).")]
            public float BlendDuration
            {
                get => _data.BlendDuration;
                set
                {
                    _data.BlendDuration = value;
                    SourceState.SaveData(true);
                }
            }

            /// <summary>
            /// The blend mode.
            /// </summary>
            [EditorOrder(60), DefaultValue(AlphaBlendMode.HermiteCubic), Tooltip("Transition blending mode for blend alpha.")]
            public AlphaBlendMode BlendMode
            {
                get => _data.BlendMode;
                set
                {
                    _data.BlendMode = value;
                    SourceState.SaveData(true);
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
                    SourceState.SaveData();
                }
            }

            /// <summary>
            /// The start position (cached).
            /// </summary>
            [HideInEditor]
            public Vector2 StartPos;

            /// <summary>
            /// The end position (cached).
            /// </summary>
            [HideInEditor]
            public Vector2 EndPos;

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
            public StateMachineTransition(StateMachineState source, StateMachineState destination, ref Data data, byte[] ruleGraph = null)
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
            public string SurfaceName => string.Format("{0} to {1}", SourceState.StateTitle, DestinationState.StateTitle);

            /// <inheritdoc />
            [HideInEditor]
            public byte[] SurfaceData
            {
                get => RuleGraph;
                set => RuleGraph = value;
            }

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
                    var undo = SourceState.Surface.Undo;
                    if (undo != null)
                    {
                        wasEnabled = undo.Enabled;
                        undo.Enabled = false;
                    }

                    context.SpawnNode(9, 22, new Vector2(100.0f));

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
                var action = new StateMachineState.AddRemoveTransitionAction(this);
                SourceState.Surface?.Undo.AddAction(action);
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
                var center = Bounds.Center + new Vector2(3.0f);
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

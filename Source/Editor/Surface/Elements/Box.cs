// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.Assertions;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Surface boxes base class (for input and output boxes). Boxes can be connected.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.SurfaceNodeElementControl" />
    /// <seealso cref="IConnectionInstigator" />
    [HideInEditor]
    public abstract class Box : SurfaceNodeElementControl, IConnectionInstigator
    {
        private bool _isMouseDown, _isSingle;
        private DateTime _lastHighlightConnectionsTime = DateTime.MinValue;
        private string _originalTooltipText;

        /// <summary>
        /// The current connection type. It's subset or equal to <see cref="DefaultType"/>.
        /// </summary>
        protected ScriptType _currentType;

        /// <summary>
        /// The collection of the attributes used by the box. Assigned externally. Can be used to control the default value editing for the <see cref="InputBox"/> or to provide more metadata for the surface UI.
        /// </summary>
        protected object[] _attributes;

        /// <summary>
        /// The cached color for the current box type.
        /// </summary>
        protected Color _currentTypeColor;

        /// <summary>
        /// The is selected flag for the box.
        /// </summary>
        protected bool _isSelected;

        /// <summary>
        /// The is active flag for the box. Unlike <see cref="FlaxEngine.GUI.Control.Enabled"/>, inactive boxes can still be interacted with, they just will be drawn like disabled boxes
        /// </summary>
        protected bool _isActive = true;

        /// <summary>
        /// Unique box ID within single node.
        /// </summary>
        public int ID => Archetype.BoxID;

        /// <summary>
        /// Allowed connections type.
        /// </summary>
        public ScriptType DefaultType => Archetype.ConnectionsType;

        /// <summary>
        /// List with all connections to other boxes.
        /// </summary>
        public readonly List<Box> Connections = new List<Box>();

        /// <summary>
        /// The box text.
        /// </summary>
        public string Text;

        /// <summary>
        /// Gets a value indicating whether this box has any connection.
        /// </summary>
        public bool HasAnyConnection => Connections.Count > 0;

        /// <summary>
        /// Gets a value indicating whether this box has single connection.
        /// </summary>
        public bool HasSingleConnection => Connections.Count == 1;

        /// <summary>
        /// Gets a value indicating whether this instance is output box.
        /// </summary>
        public abstract bool IsOutput { get; }

        /// <summary>
        /// Gets or sets the current type of the box connections.
        /// </summary>
        public ScriptType CurrentType
        {
            get => _currentType;
            set
            {
                if (_currentType != value)
                {
                    // Set new value
                    var prev = _currentType;
                    _currentType = value;

                    // Check if will need to update box connections due to type change
                    if ((Surface == null || Surface._isUpdatingBoxTypes == 0) && HasAnyConnection && !prev.CanCastTo(_currentType))
                    {
                        // Remove all invalid connections and update those which still can be valid
                        var connections = Connections.ToArray();
                        for (int i = 0; i < connections.Length; i++)
                        {
                            var targetBox = connections[i];

                            // Break connection
                            Connections.Remove(targetBox);
                            targetBox.Connections.Remove(this);

                            // Check if can connect them
                            if (CanConnectWith(targetBox))
                            {
                                // Connect again
                                Connections.Add(targetBox);
                                targetBox.Connections.Add(this);
                            }
                            else
                            {
                                Surface?.OnNodesDisconnected(this, targetBox);
                            }

                            targetBox.OnConnectionsChanged();
                        }
                        OnConnectionsChanged();

                        // Update
                        for (int i = 0; i < connections.Length; i++)
                        {
                            connections[i].ConnectionTick();
                        }
                        ConnectionTick();
                    }

                    // Fire event
                    OnCurrentTypeChanged();
                }
            }
        }

        /// <summary>
        /// Cached color for <see cref="CurrentType"/>.
        /// </summary>
        public Color CurrentTypeColor => _currentTypeColor;

        /// <summary>
        /// The collection of the attributes used by the box. Assigned externally. Can be used to control the default value editing for the <see cref="InputBox"/> or to provide more metadata for the surface UI.
        /// </summary>
        public object[] Attributes
        {
            get => _attributes;
            set
            {
                if (ReferenceEquals(_attributes, value))
                    return;
                if (_attributes == null && value != null && value.Length == 0)
                    return;
                if (_attributes != null && value != null && Utils.ArraysEqual(_attributes, value))
                    return;
                _attributes = value;
                OnAttributesChanged();
            }
        }

        /// <summary>
        /// Event called when the current type of the box gets changed.
        /// </summary>
        public Action<Box> CurrentTypeChanged;

        /// <summary>
        /// The box connections highlight strength (normalized to range 0-1).
        /// </summary>
        public float ConnectionsHighlightIntensity => Mathf.Saturate(1.0f - (float)(DateTime.Now - _lastHighlightConnectionsTime).TotalSeconds);

        /// <summary>
        /// Gets a value indicating whether this box is selected.
        /// </summary>
        public bool IsSelected
        {
            get => _isSelected;
            internal set
            {
                _isSelected = value;
                OnSelectionChanged();
            }
        }

        /// <summary>
        /// Gets or sets the active state of the box. Unlike <see cref="FlaxEngine.GUI.Control.Enabled"/>, inactive boxes can still be interacted with, they just will be drawn like disabled boxes
        /// </summary>
        public bool IsActive
        {
            get => _isActive;
            set => _isActive = value;
        }

        /// <inheritdoc />
        protected Box(SurfaceNode parentNode, NodeElementArchetype archetype, Float2 location)
        : base(parentNode, archetype, location, new Float2(Constants.BoxSize), false)
        {
            _currentType = DefaultType;
            _isSingle = Archetype.Single;
            Text = Archetype.Text;
            if (Surface != null)
            {
                var hints = parentNode.Archetype.ConnectionsHints;
                Surface.Style.GetConnectionColor(_currentType, hints, out _currentTypeColor);
                TooltipText = Surface.GetTypeName(CurrentType) ?? GetConnectionHintTypeName(hints);
            }
        }

        private static string GetConnectionHintTypeName(ConnectionsHint hint)
        {
            if ((hint & ConnectionsHint.Anything) == ConnectionsHint.Anything)
                return "Anything";
            if ((hint & ConnectionsHint.Value) == ConnectionsHint.Value)
                return "Value";
            if ((hint & ConnectionsHint.Enum) == ConnectionsHint.Enum)
                return "Enum";
            if ((hint & ConnectionsHint.Numeric) == ConnectionsHint.Numeric)
                return "Numeric";
            if ((hint & ConnectionsHint.Vector) == ConnectionsHint.Vector)
                return "Vector";
            if ((hint & ConnectionsHint.Scalar) == ConnectionsHint.Scalar)
                return "Scalar";
            if ((hint & ConnectionsHint.Array) == ConnectionsHint.Array)
                return "Array";
            if ((hint & ConnectionsHint.Dictionary) == ConnectionsHint.Dictionary)
                return "Dictionary";
            return null;
        }

        /// <summary>
        /// Determines whether this box can use the specified type as a connection.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns><c>true</c> if this box can use the specified type; otherwise, <c>false</c>.</returns>
        public bool CanUseType(ScriptType type)
        {
            // Check direct connection
            if (Surface != null)
            {
                if (Surface.CanUseDirectCast(type, _currentType))
                    return true;
            }
            else
            {
                if (VisjectSurface.CanUseDirectCastStatic(type, _currentType))
                    return true;
            }

            // Check using connection hints
            if (VisjectSurface.IsTypeCompatible(Archetype.ConnectionsType, type, ParentNode.Archetype.ConnectionsHints))
                return true;

            // Check independent and if there is box with bigger potential because it may block current one from changing type
            var parentArch = ParentNode.Archetype;
            var boxes = parentArch.IndependentBoxes;
            if (boxes != null)
            {
                for (int i = 0; i < boxes.Length; i++)
                {
                    if (boxes[i] == -1)
                        break;
                    var b = ParentNode.GetBox(boxes[i]);

                    // Check if its the same and tested type matches the default value type
                    if (b == this && parentArch.DefaultType.CanCastTo(type))
                    {
                        // Can
                        return true;
                    }

                    // Check if box exists and has any connection
                    if (b != null && b.HasAnyConnection)
                    {
                        // Cannot
                        return false;
                    }
                }
            }

            // Cannot
            return false;
        }

        /// <summary>
        /// Removes all existing connections of that box.
        /// </summary>
        /// <param name="skipCount">Amount of connection to skip from removing.</param>
        public void RemoveConnections(int skipCount = 0)
        {
            if (Connections.Count > skipCount)
            {
                // Remove all connections
                var toUpdate = new List<Box>(1 + skipCount)
                {
                    this
                };

                for (int i = skipCount; i < Connections.Count; i++)
                {
                    var targetBox = Connections[i];
                    targetBox.Connections.Remove(this);
                    toUpdate.Add(targetBox);
                    targetBox.OnConnectionsChanged();
                    Surface?.OnNodesDisconnected(this, targetBox);
                }
                Connections.Clear();
                OnConnectionsChanged();

                // Update
                for (int i = 0; i < toUpdate.Count; i++)
                {
                    toUpdate[i].ConnectionTick();
                }
            }
        }

        /// <summary>
        /// Updates state on connection data changed.
        /// </summary>
        public void ConnectionTick()
        {
            // Update node boxes types management
            ParentNode.ConnectionTick(this);
        }

        /// <summary>
        /// Checks if box is connected with the other one.
        /// </summary>
        /// <param name="box">The other box.</param>
        /// <returns>True if both boxes are connected, otherwise false.</returns>
        public bool AreConnected(Box box)
        {
            bool result = Connections.Contains(box);
            Assert.IsTrue(box == null || result == box.Connections.Contains(this));
            return result;
        }

        /// <summary>
        /// Break connection to the other box (works in a both ways).
        /// </summary>
        /// <param name="box">The other box.</param>
        public void BreakConnection(Box box)
        {
            // Break link
            bool r1 = box.Connections.Remove(this);
            bool r2 = Connections.Remove(box);

            // Ensure data was fine and connection was valid
            Assert.AreEqual(r1, r2);

            // Update
            ConnectionTick();
            box.ConnectionTick();
            OnConnectionsChanged();
            box.OnConnectionsChanged();
            Surface?.OnNodesDisconnected(this, box);
        }

        /// <summary>
        /// Create connection to the other box (works in a both ways).
        /// </summary>
        /// <param name="box">The other box.</param>
        public void CreateConnection(Box box)
        {
            // Check if any box can have only single connection
            if (box.IsSingle)
                box.RemoveConnections();
            if (IsSingle)
                RemoveConnections();

            // Add link
            box.Connections.Add(this);
            Connections.Add(box);

            // Ensure data is fine and connection is valid
            Assert.IsTrue(AreConnected(box));

            // Update
            ConnectionTick();
            box.ConnectionTick();
            OnConnectionsChanged();
            box.OnConnectionsChanged();
            Surface?.OnNodesConnected(this, box);
        }

        /// <summary>
        /// True if box can use only single connection.
        /// </summary>
        public bool IsSingle
        {
            get => _isSingle;
            set
            {
                if (_isSingle != value)
                {
                    _isSingle = value;

                    // Limit connections COUNT
                    if (_isSingle && Connections.Count > 0)
                    {
                        if (Surface.Undo != null)
                        {
                            var action = new EditNodeConnections(ParentNode.Context, ParentNode);
                            RemoveConnections(1);
                            action.End();
                            Surface.AddBatchedUndoAction(action);
                        }
                        else
                        {
                            RemoveConnections(1);
                        }
                        Surface.MarkAsEdited();
                    }
                }
            }
        }

        /// <summary>
        /// True if box type depends on other boxes types of the node.
        /// </summary>
        public bool IsDependentBox
        {
            get
            {
                var boxes = ParentNode.Archetype.DependentBoxes;
                if (boxes != null)
                {
                    for (int i = 0; i < boxes.Length; i++)
                    {
                        int index = boxes[i];
                        if (index == -1)
                            break;
                        if (index == ID)
                            return true;
                    }
                }

                return false;
            }
        }

        /// <summary>
        /// True if box type doesn't depend on other boxes types of the node.
        /// </summary>
        public bool IsIndependentBox
        {
            get
            {
                var boxes = ParentNode.Archetype.IndependentBoxes;
                if (boxes != null)
                {
                    for (int i = 0; i < boxes.Length; i++)
                    {
                        int index = boxes[i];
                        if (index == -1)
                            break;
                        if (index == ID)
                            return true;
                    }
                }

                return false;
            }
        }

        /// <summary>
        /// Highlights this box connections. Used to visualize signals and data transfer on the graph at runtime during debugging.
        /// </summary>
        public void HighlightConnections()
        {
            _lastHighlightConnectionsTime = DateTime.Now;
        }

        /// <summary>
        /// Called when current box type changed.
        /// </summary>
        protected virtual void OnCurrentTypeChanged()
        {
            if (Surface != null)
            {
                var hints = ParentNode.Archetype.ConnectionsHints;
                Surface.Style.GetConnectionColor(_currentType, hints, out _currentTypeColor);
                TooltipText = Surface.GetTypeName(CurrentType) ?? GetConnectionHintTypeName(hints);
            }
            CurrentTypeChanged?.Invoke(this);
        }

        /// <summary>
        /// Called when attributes collection gets changed.
        /// </summary>
        protected virtual void OnAttributesChanged()
        {
        }

        /// <summary>
        /// Called when connections array gets changed (also called after surface deserialization)
        /// </summary>
        public virtual void OnConnectionsChanged()
        {
        }

        /// <summary>
        /// Called when box gets selected or deselected.
        /// </summary>
        protected virtual void OnSelectionChanged()
        {
            if (IsSelected && !ParentNode.IsSelected)
            {
                ParentNode.Surface.AddToSelection(ParentNode);
            }
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            if (Surface.GetBoxDebuggerTooltip(this, out var debuggerTooltip))
            {
                _originalTooltipText = TooltipText;
                TooltipText = debuggerTooltip;
            }

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                _isMouseDown = true;
                Focus();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (_originalTooltipText != null)
            {
                TooltipText = _originalTooltipText;
            }
            if (_isMouseDown)
            {
                _isMouseDown = false;
                if (Surface.CanEdit)
                {
                    if (!IsOutput && HasSingleConnection)
                    {
                        var connectedBox = Connections[0];
                        if (Surface.Undo != null && Surface.Undo.Enabled)
                        {
                            var action = new ConnectBoxesAction((InputBox)this, (OutputBox)connectedBox, false);
                            BreakConnection(connectedBox);
                            action.End();
                            Surface.AddBatchedUndoAction(action);
                            Surface.MarkAsEdited();
                        }
                        else
                        {
                            BreakConnection(connectedBox);
                        }
                        Surface.ConnectingStart(connectedBox);
                    }
                    else
                    {
                        Surface.ConnectingStart(this);
                    }
                }
            }
            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            Surface.ConnectingOver(this);
            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                _isMouseDown = false;
                if (Surface.IsConnecting)
                {
                    Surface.ConnectingEnd(this);
                }
                else if (Surface.CanEdit)
                {
                    // Click
                    if (Root.GetKey(KeyboardKeys.Alt))
                    {
                        // Break connections
                        if (Surface.Undo != null)
                        {
                            var action = new EditNodeConnections(ParentNode.Context, ParentNode);
                            RemoveConnections();
                            action.End();
                            Surface.AddBatchedUndoAction(action);
                        }
                        else
                        {
                            RemoveConnections();
                        }
                        Surface.MarkAsEdited();
                    }
                    else if (Root.GetKey(KeyboardKeys.Control))
                    {
                        // Add to selection
                        if (!ParentNode.IsSelected)
                        {
                            ParentNode.Surface.AddToSelection(ParentNode);
                        }
                        ParentNode.AddBoxToSelection(this);
                    }
                    else
                    {
                        // Forcibly select the node
                        ParentNode.Surface.Select(ParentNode);
                        ParentNode.SelectBox(this);
                    }
                }
                return true;
            }

            return false;
        }

        /// <summary>
        /// Connections origin offset.
        /// </summary>
        public Float2 ConnectionOffset;

        /// <inheritdoc />
        public Float2 ConnectionOrigin
        {
            get
            {
                var center = Center + ConnectionOffset;
                return Parent.PointToParent(ref center);
            }
        }

        /// <inheritdoc />
        public bool AreConnected(IConnectionInstigator other)
        {
            return Connections.Contains(other as Box);
        }

        /// <inheritdoc />
        public bool CanConnectWith(IConnectionInstigator other)
        {
            if (other is Archetypes.Tools.RerouteNode reroute)
                return reroute.CanConnectWith(this);

            var start = this;
            var end = other as Box;

            // Allow only box with box connection
            if (end == null)
            {
                // Cannot
                return false;
            }

            // Disable for the same box
            if (start == end)
            {
                // Cannot
                return false;
            }

            // Check if boxes are connected
            bool areConnected = start.AreConnected(end);

            // Check if boxes are different or (one of them is disabled and both are disconnected)
            if (end.IsOutput == start.IsOutput || !((end.Enabled && start.Enabled) || areConnected))
            {
                // Cannot
                return false;
            }

            // Cache Input and Output box (since connection may be made in a different way)
            InputBox iB;
            OutputBox oB;
            if (start.IsOutput)
            {
                iB = (InputBox)end;
                oB = (OutputBox)start;
            }
            else
            {
                iB = (InputBox)start;
                oB = (OutputBox)end;
            }

            // Validate connection type (also check if any of boxes parent can manage that connections types)
            if (oB.CurrentType != ScriptType.Null)
            {
                if (!iB.CanUseType(oB.CurrentType))
                {
                    if (!oB.CurrentType.CanCastTo(iB.CurrentType))
                    {
                        // Cannot
                        return false;
                    }
                }
            }
            else
            {
                if (!oB.CanUseType(iB.CurrentType))
                {
                    if (!oB.CurrentType.CanCastTo(iB.CurrentType))
                    {
                        // Cannot
                        return false;
                    }
                }
            }

            // Can
            return true;
        }

        /// <inheritdoc />
        public void DrawConnectingLine(ref Float2 startPos, ref Float2 endPos, ref Color color)
        {
            OutputBox.DrawConnection(Surface.Style, ref startPos, ref endPos, ref color, OutputBox.ConnectingConnectionThickness);
        }

        /// <inheritdoc />
        public void Connect(IConnectionInstigator other)
        {
            if (other is Archetypes.Tools.RerouteNode reroute)
            {
                reroute.Connect(this);
                return;
            }

            var start = this;
            var end = (Box)other;
            var areConnected = start.AreConnected(end);

            // Check if boxes are different or (one of them is disabled and both are disconnected)
            if (end.IsOutput == start.IsOutput || !((end.Enabled && start.Enabled) || areConnected))
                return;

            // Cache Input and Output box (since connection may be made in a different way)
            InputBox iB;
            OutputBox oB;
            if (start.IsOutput)
            {
                iB = (InputBox)end;
                oB = (OutputBox)start;
            }
            else
            {
                iB = (InputBox)start;
                oB = (OutputBox)end;
            }

            // Check if they are already connected
            if (areConnected)
            {
                // Break link
                if (Surface.Undo != null && Surface.Undo.Enabled)
                {
                    var action = new ConnectBoxesAction(iB, oB, false);
                    start.BreakConnection(end);
                    action.End();
                    Surface.AddBatchedUndoAction(action);
                }
                else
                {
                    start.BreakConnection(end);
                }
                Surface.MarkAsEdited();
                Surface?.OnNodesDisconnected(this, other);
                return;
            }

            // Validate connection type (also check if any of boxes parent can manage that connections types)
            bool useCaster = false;
            if (!iB.CanUseType(oB.CurrentType))
            {
                if (oB.CurrentType.CanCastTo(iB.CurrentType))
                    useCaster = true;
                else
                    return;
            }

            // Connect boxes
            if (useCaster)
            {
                // Connect via Caster
                const float casterXOffset = 250;
                if (Surface.Undo != null && Surface.Undo.Enabled)
                {
                    bool undoEnabled = Surface.Undo.Enabled;
                    Surface.Undo.Enabled = false;
                    SurfaceNode node = Surface.Context.SpawnNode(7, 22, Float2.Zero); // 22 AsNode, 25 CastNode
                    Surface.Undo.Enabled = undoEnabled;
                    if (node is not Archetypes.Tools.AsNode castNode)
                        throw new Exception("Node is not a casting node!");

                    // Set the type of the casting node
                    undoEnabled = castNode.Surface.Undo.Enabled;
                    castNode.Surface.Undo.Enabled = false;
                    castNode.SetPickerValue(iB.CurrentType);
                    castNode.Surface.Undo.Enabled = undoEnabled;
                    if (node.GetBox(0) is not OutputBox castOutputBox || node.GetBox(1) is not InputBox castInputBox)
                        throw new NullReferenceException("Casting failed. Cast node is invalid!");

                    // We set the position of the cast node here to set it relative to the target nodes input box
                    undoEnabled = castNode.Surface.Undo.Enabled;
                    castNode.Surface.Undo.Enabled = false;
                    var wantedOffset = iB.ParentNode.Location - new Float2(casterXOffset, -(iB.LocalY - castOutputBox.LocalY));
                    castNode.Location = Surface.Root.PointFromParent(ref wantedOffset);
                    castNode.Surface.Undo.Enabled = undoEnabled;

                    var spawnNodeAction = new AddRemoveNodeAction(castNode, true);

                    var connectToCastNodeAction = new ConnectBoxesAction(castInputBox, oB, true);
                    castInputBox.CreateConnection(oB);
                    connectToCastNodeAction.End();

                    var connectCastToTargetNodeAction = new ConnectBoxesAction(iB, castOutputBox, true);
                    iB.CreateConnection(castOutputBox);
                    connectCastToTargetNodeAction.End();

                    Surface.AddBatchedUndoAction(new MultiUndoAction(spawnNodeAction, connectToCastNodeAction, connectCastToTargetNodeAction));
                }
                else
                {
                    SurfaceNode node = Surface.Context.SpawnNode(7, 22, Float2.Zero); // 22 AsNode, 25 CastNode
                    if (node is not Archetypes.Tools.AsNode castNode)
                        throw new Exception("Node is not a casting node!");

                    // Set the type of the casting node
                    castNode.SetPickerValue(iB.CurrentType);
                    if (node.GetBox(0) is not OutputBox castOutputBox || node.GetBox(1) is not InputBox castInputBox)
                        throw new NullReferenceException("Casting failed. Cast node is invalid!");

                    // We set the position of the cast node here to set it relative to the target nodes input box
                    var wantedOffset = iB.ParentNode.Location - new Float2(casterXOffset, -(iB.LocalY - castOutputBox.LocalY));
                    castNode.Location = Surface.Root.PointFromParent(ref wantedOffset);

                    castInputBox.CreateConnection(oB);
                    iB.CreateConnection(castOutputBox);
                }
                Surface.MarkAsEdited();
            }
            else
            {
                // Connect directly
                if (Surface.Undo != null && Surface.Undo.Enabled)
                {
                    var action = new ConnectBoxesAction(iB, oB, true);
                    iB.CreateConnection(oB);
                    action.End();
                    Surface.AddBatchedUndoAction(action);
                }
                else
                {
                    iB.CreateConnection(oB);
                }
                Surface.MarkAsEdited();
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _currentType = ScriptType.Null;
            CurrentTypeChanged = null;
            Attributes = null;

            base.OnDestroy();
        }
    }
}

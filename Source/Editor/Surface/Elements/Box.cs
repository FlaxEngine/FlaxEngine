// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        private bool _isMouseDown;
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
                    if (Surface._isUpdatingBoxTypes == 0 && HasAnyConnection && !CanCast(prev, _currentType))
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

        /// <inheritdoc />
        protected Box(SurfaceNode parentNode, NodeElementArchetype archetype, Vector2 location)
        : base(parentNode, archetype, location, new Vector2(Constants.BoxSize), false)
        {
            _currentType = DefaultType;
            Text = Archetype.Text;
            var hints = parentNode.Archetype.ConnectionsHints;
            Surface.Style.GetConnectionColor(_currentType, hints, out _currentTypeColor);
            TooltipText = Surface.GetTypeName(CurrentType) ?? GetConnectionHintTypeName(hints);
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
            if (Surface.CanUseDirectCast(type, _currentType))
            {
                // Can
                return true;
            }

            // Check using connection hints
            var connectionsHints = ParentNode.Archetype.ConnectionsHints;
            if (Archetype.ConnectionsType == ScriptType.Null && connectionsHints != ConnectionsHint.None)
            {
                if ((connectionsHints & ConnectionsHint.Anything) == ConnectionsHint.Anything)
                {
                    // Can
                    return true;
                }
                if ((connectionsHints & ConnectionsHint.Value) == ConnectionsHint.Value && type.Type != typeof(void))
                {
                    // Can
                    return true;
                }
                if ((connectionsHints & ConnectionsHint.Enum) == ConnectionsHint.Enum && type.IsEnum)
                {
                    // Can
                    return true;
                }
                if ((connectionsHints & ConnectionsHint.Vector) == ConnectionsHint.Vector)
                {
                    var t = type.Type;
                    if (t == typeof(Vector2) ||
                        t == typeof(Vector3) ||
                        t == typeof(Vector4) ||
                        t == typeof(Color))
                    {
                        // Can
                        return true;
                    }
                }
                if ((connectionsHints & ConnectionsHint.Scalar) == ConnectionsHint.Scalar)
                {
                    var t = type.Type;
                    if (t == typeof(bool) ||
                        t == typeof(char) ||
                        t == typeof(byte) ||
                        t == typeof(short) ||
                        t == typeof(ushort) ||
                        t == typeof(int) ||
                        t == typeof(uint) ||
                        t == typeof(long) ||
                        t == typeof(ulong) ||
                        t == typeof(float) ||
                        t == typeof(double))
                    {
                        // Can
                        return true;
                    }
                }
            }

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
                    if (b == this && CanCast(parentArch.DefaultType, type))
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
        public void RemoveConnections()
        {
            // Check if sth is connected
            if (HasAnyConnection)
            {
                // Remove all connections
                var toUpdate = new List<Box>(1 + Connections.Count);
                toUpdate.Add(this);
                for (int i = 0; i < Connections.Count; i++)
                {
                    var targetBox = Connections[i];
                    targetBox.Connections.Remove(this);
                    toUpdate.Add(targetBox);
                    targetBox.OnConnectionsChanged();
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
        }

        /// <summary>
        /// True if box can use only single connection.
        /// </summary>
        public bool IsSingle => Archetype.Single;

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
            var hints = ParentNode.Archetype.ConnectionsHints;
            Surface.Style.GetConnectionColor(_currentType, hints, out _currentTypeColor);
            TooltipText = Surface.GetTypeName(CurrentType) ?? GetConnectionHintTypeName(hints);
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

        /// <summary>
        /// Draws the box GUI using <see cref="Render2D"/>.
        /// </summary>
        protected void DrawBox()
        {
            var rect = new Rectangle(Vector2.Zero, Size);

            // Size culling
            const float minBoxSize = 5.0f;
            if (rect.Size.LengthSquared < minBoxSize * minBoxSize)
                return;

            // Debugging boxes size
            //Render2D.DrawRectangle(rect, Color.Orange); return;

            // Draw icon
            bool hasConnections = HasAnyConnection;
            float alpha = Enabled ? 1.0f : 0.6f;
            Color color = _currentTypeColor * alpha;
            var style = Surface.Style;
            SpriteHandle icon;
            if (_currentType.Type == typeof(void))
                icon = hasConnections ? style.Icons.ArrowClose : style.Icons.ArrowOpen;
            else
                icon = hasConnections ? style.Icons.BoxClose : style.Icons.BoxOpen;
            color *= ConnectionsHighlightIntensity + 1;
            Render2D.DrawSprite(icon, rect, color);

            // Draw selection hint
            if (_isSelected)
            {
                float outlineAlpha = Mathf.Sin(Time.TimeSinceStartup * 4.0f) * 0.5f + 0.5f;
                float outlineWidth = Mathf.Lerp(1.5f, 4.0f, outlineAlpha);
                var outlineRect = new Rectangle(rect.X - outlineWidth, rect.Y - outlineWidth, rect.Width + outlineWidth * 2, rect.Height + outlineWidth * 2);
                Render2D.DrawSprite(icon, outlineRect, FlaxEngine.GUI.Style.Current.BorderSelected.RGBMultiplied(1.0f + outlineAlpha * 0.4f));
            }
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            if (Surface.GetBoxDebuggerTooltip(this, out var debuggerTooltip))
            {
                _originalTooltipText = TooltipText;
                TooltipText = debuggerTooltip;
            }

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
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
                        if (Surface.Undo != null)
                        {
                            var action = new ConnectBoxesAction((InputBox)this, (OutputBox)connectedBox, false);
                            BreakConnection(connectedBox);
                            action.End();
                            Surface.Undo.AddAction(action);
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
        public override void OnMouseMove(Vector2 location)
        {
            Surface.ConnectingOver(this);
            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
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
                            Surface.Undo.AddAction(action);
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

        /// <inheritdoc />
        public Vector2 ConnectionOrigin
        {
            get
            {
                var center = Center;
                return Parent.PointToParent(ref center);
            }
        }

        private static bool CanCast(ScriptType oB, ScriptType iB)
        {
            if (oB == iB)
                return true;
            if (oB == ScriptType.Null || iB == ScriptType.Null)
                return false;
            return (oB.Type != typeof(void) && oB.Type != typeof(FlaxEngine.Object)) &&
                   (iB.Type != typeof(void) && iB.Type != typeof(FlaxEngine.Object)) &&
                   oB.IsAssignableFrom(iB);
        }

        /// <inheritdoc />
        public bool AreConnected(IConnectionInstigator other)
        {
            return Connections.Contains(other as Box);
        }

        /// <inheritdoc />
        public bool CanConnectWith(IConnectionInstigator other)
        {
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
                    if (!CanCast(oB.CurrentType, iB.CurrentType))
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
                    if (!CanCast(oB.CurrentType, iB.CurrentType))
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
        public void DrawConnectingLine(ref Vector2 startPos, ref Vector2 endPos, ref Color color)
        {
            OutputBox.DrawConnection(ref startPos, ref endPos, ref color, 2);
        }

        /// <inheritdoc />
        public void Connect(IConnectionInstigator other)
        {
            var start = this;
            var end = (Box)other;

            // Check if boxes are connected
            bool areConnected = start.AreConnected(end);

            // Check if boxes are different or (one of them is disabled and both are disconnected)
            if (end.IsOutput == start.IsOutput || !((end.Enabled && start.Enabled) || areConnected))
            {
                // Back
                return;
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

            // Check if they are already connected
            if (areConnected)
            {
                // Break link
                if (Surface.Undo != null)
                {
                    var action = new ConnectBoxesAction(iB, oB, false);
                    start.BreakConnection(end);
                    action.End();
                    Surface.Undo.AddAction(action);
                }
                else
                {
                    start.BreakConnection(end);
                }
                Surface.MarkAsEdited();
                return;
            }

            // Validate connection type (also check if any of boxes parent can manage that connections types)
            bool useCaster = false;
            if (!iB.CanUseType(oB.CurrentType))
            {
                if (CanCast(oB.CurrentType, iB.CurrentType))
                    useCaster = true;
                else
                    return;
            }

            // Connect boxes
            if (useCaster)
            {
                // Connect via Caster
                //AddCaster(oB, iB);
                throw new NotImplementedException("AddCaster(..) function");
            }
            else
            {
                // Connect directly
                if (Surface.Undo != null)
                {
                    var action = new ConnectBoxesAction(iB, oB, true);
                    iB.CreateConnection(oB);
                    action.End();
                    Surface.Undo?.AddAction(action);
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

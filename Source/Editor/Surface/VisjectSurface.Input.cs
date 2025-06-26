// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Options;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        /// <summary>
        /// The input actions collection to processed during user input.
        /// </summary>
        public readonly InputActionsContainer InputActions;

        /// <summary>
        /// Optional feature.
        /// </summary>
        public bool PanWithMiddleMouse = false;

        private string _currentInputText = string.Empty;
        private Float2 _movingNodesDelta;
        private Float2 _gridRoundingDelta;
        private HashSet<SurfaceNode> _movingNodes;
        private HashSet<SurfaceNode> _temporarySelectedNodes;
        private readonly Stack<InputBracket> _inputBrackets = new Stack<InputBracket>();

        private class InputBracket
        {
            private readonly float DefaultWidth = 120f;
            private readonly Margin _padding = new Margin(10f);

            public Box Box { get; }
            public Float2 EndBracketPosition { get; }
            public List<SurfaceNode> Nodes { get; } = new List<SurfaceNode>();
            public Rectangle Area { get; private set; }

            public InputBracket(Box box, Float2 nodePosition)
            {
                Box = box;
                EndBracketPosition = nodePosition + new Float2(DefaultWidth, 0);
                Update();
            }


            public void Update()
            {
                Rectangle area;
                if (Nodes.Count > 0)
                {
                    area = VisjectSurface.GetNodesBounds(Nodes);
                }
                else
                {
                    area = new Rectangle(EndBracketPosition, new Float2(DefaultWidth, 80f));
                }
                _padding.ExpandRectangle(ref area);
                var offset = EndBracketPosition - area.UpperRight;
                area.Location += offset;
                Area = area;
                if (!offset.IsZero)
                {
                    foreach (var node in Nodes)
                    {
                        node.Location += offset;
                    }
                }
            }
        }

        private string InputText
        {
            get => _currentInputText;
            set
            {
                _currentInputText = value;
                CurrentInputTextChanged(_currentInputText);
            }
        }

        /// <summary>
        /// Occurs when handling custom mouse button down event.
        /// </summary>
        public event Window.MouseButtonDelegate CustomMouseDown;

        /// <summary>
        /// Occurs when handling custom mouse button up event.
        /// </summary>
        public event Window.MouseButtonDelegate CustomMouseUp;

        /// <summary>
        /// Occurs when handling custom mouse double click event.
        /// </summary>
        public event Window.MouseButtonDelegate CustomMouseDoubleClick;

        /// <summary>
        /// Occurs when handling custom mouse move event.
        /// </summary>
        public event Window.MouseMoveDelegate CustomMouseMove;

        /// <summary>
        /// Occurs when handling custom mouse wheel event.
        /// </summary>
        public event Window.MouseWheelDelegate CustomMouseWheel;

        /// <summary>
        /// Gets the control under the mouse location.
        /// </summary>
        /// <returns>The control or null if no intersection.</returns>
        public SurfaceControl GetControlUnderMouse()
        {
            var pos = _rootControl.PointFromParent(ref _mousePos);
            return _rootControl.GetChildAt(pos) as SurfaceControl;
        }

        private void UpdateSelectionRectangle()
        {
            var p1 = _rootControl.PointFromParent(ref _leftMouseDownPos);
            var p2 = _rootControl.PointFromParent(ref _mousePos);
            var selectionRect = Rectangle.FromPoints(p1, p2);
            var selectionChanged = false;

            // Find controls to select
            for (int i = 0; i < _rootControl.Children.Count; i++)
            {
                if (_rootControl.Children[i] is SurfaceControl control)
                {
                    var select = control.IsSelectionIntersecting(ref selectionRect);

                    if (Root.GetKey(KeyboardKeys.Shift))
                    {
                        if (select == control.IsSelected && _temporarySelectedNodes.Contains(control))
                        {
                            control.IsSelected = !select;
                            selectionChanged = true;
                        }
                    }
                    else if (Root.GetKey(KeyboardKeys.Control))
                    {
                        if (select != control.IsSelected && !_temporarySelectedNodes.Contains(control))
                        {
                            control.IsSelected = select;
                            selectionChanged = true;
                        }
                    }
                    else
                    {
                        if (select != control.IsSelected)
                        {
                            control.IsSelected = select;
                            selectionChanged = true;
                        }
                    }
                }
            }

            if (selectionChanged)
                SelectionChanged?.Invoke();
        }

        private void OnSurfaceControlSpawned(SurfaceControl control)
        {
            if (_inputBrackets.Count > 0 && control is SurfaceNode node)
            {
                _inputBrackets.Peek().Nodes.Add(node);
                _inputBrackets.Peek().Update();
            }
        }

        private void OnSurfaceControlDeleted(SurfaceControl control)
        {
            if (_inputBrackets.Count > 0 && control is SurfaceNode node)
            {
                _inputBrackets.Peek().Nodes.Remove(node);
                _inputBrackets.Peek().Update();
            }
        }

        private void OnGetNodesToMove()
        {
            if (_movingNodes == null)
                _movingNodes = new HashSet<SurfaceNode>();
            else
                _movingNodes.Clear();
            if (CanEdit)
            {
                _isMovingSelection = true;
                for (int i = 0; i < _rootControl.Children.Count; i++)
                {
                    if (_rootControl.Children[i] is SurfaceNode node && node.IsSelected && (node.Archetype.Flags & NodeFlags.NoMove) != NodeFlags.NoMove)
                    {
                        _movingNodes.Add(node);

                        // Move nodes inside the comment
                        if (node is SurfaceComment comment)
                        {
                            var commentBounds = comment.Bounds;
                            for (int j = 0; j < _rootControl.Children.Count; j++)
                            {
                                if (_rootControl.Children[j] is SurfaceNode childNode && commentBounds.Contains(childNode.Bounds))
                                {
                                    _movingNodes.Add(childNode);
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Snaps a coordinate point to the grid.
        /// </summary>
        /// <param name="point">The point to be rounded.</param>
        /// <param name="ceil">Round to ceiling instead?</param>
        /// <returns>Rounded coordinate.</returns>
        public Float2 SnapToGrid(Float2 point, bool ceil = false)
        {
            float gridSize = GridSnappingSize;
            Float2 snapped = point.Absolute / gridSize;
            snapped = ceil ? Float2.Ceil(snapped) : Float2.Floor(snapped);
            snapped.X = (float)Math.CopySign(snapped.X * gridSize, point.X);
            snapped.Y = (float)Math.CopySign(snapped.Y * gridSize, point.Y);
            return snapped;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            _lastInstigatorUnderMouse = null;

            // Cache mouse location
            _mousePos = location;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _lastInstigatorUnderMouse = null;

            // Cache mouse location
            _mousePos = location;

            // Moving around surface with mouse
            if (_rightMouseDown)
            {
                // Calculate delta
                var delta = location - _rightMouseDownPos;
                if (delta.LengthSquared > 0.01f)
                {
                    // Move view
                    _mouseMoveAmount += delta.Length;
                    _rootControl.Location += delta;
                    _rightMouseDownPos = location;
                    Cursor = CursorType.SizeAll;
                }

                // Handled
                return;
            }

            if (_middleMouseDown)
            {
                if (PanWithMiddleMouse)
                {
                    // Calculate delta
                    var delta = location - _middleMouseDownPos;
                    if (delta.LengthSquared > 0.01f)
                    {
                        // Move view
                        _mouseMoveAmount += delta.Length;
                        _rootControl.Location += delta;
                        _middleMouseDownPos = location;
                        Cursor = CursorType.SizeAll;
                    }
                }

                // Handled
                return;
            }

            // Check if user is selecting or moving node(s)
            if (_leftMouseDown)
            {
                // Connecting
                if (_connectionInstigator != null)
                {
                }
                // Moving
                else if (_isMovingSelection)
                {
                    var gridSnap = GridSnappingEnabled;
                    if (!gridSnap)
                        _gridRoundingDelta = Float2.Zero; // Reset in case user toggled option between frames.

                    // Calculate delta (apply view offset)
                    var viewDelta = _rootControl.Location - _movingSelectionViewPos;
                    _movingSelectionViewPos = _rootControl.Location;
                    var delta = location - _leftMouseDownPos - viewDelta + _gridRoundingDelta;
                    var deltaLengthSquared = delta.LengthSquared;

                    delta /= _targetScale;
                    if ((!gridSnap || Mathf.Abs(delta.X) >= GridSnappingSize || (Mathf.Abs(delta.Y) >= GridSnappingSize))
                        && deltaLengthSquared > 0.01f)
                    {
                        if (gridSnap)
                        {
                            Float2 unroundedDelta = delta;
                            delta = SnapToGrid(unroundedDelta);
                            _gridRoundingDelta = (unroundedDelta - delta) * _targetScale; // Standardize unit of the rounding delta, in case user zooms between node movements.
                        }

                        foreach (var node in _movingNodes)
                        {
                            if (gridSnap)
                            {
                                Float2 unroundedLocation = node.Location;
                                node.Location = SnapToGrid(unroundedLocation);
                            }
                            node.Location += delta;
                        }

                        _leftMouseDownPos = location;
                        _movingNodesDelta += delta; // TODO: Figure out how to handle undo for differing values of _gridRoundingDelta between selected nodes. For now it will be a small error in undo.
                        if (_movingNodes.Count > 0)
                        {
                            Cursor = CursorType.SizeAll;
                            MarkAsEdited(false);
                        }
                    }

                    // Handled
                    return;
                }
                // Selecting
                else
                {
                    UpdateSelectionRectangle();

                    // Handled
                    return;
                }
            }

            base.OnMouseMove(location);
            CustomMouseMove?.Invoke(ref location);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            // Clear flags and state
            if (_leftMouseDown)
            {
                _leftMouseDown = false;
            }
            if (_rightMouseDown)
            {
                _rightMouseDown = false;
                Cursor = CursorType.Default;
            }
            if (_middleMouseDown)
            {
                _middleMouseDown = false;
                if (PanWithMiddleMouse)
                    Cursor = CursorType.Default;
            }
            _isMovingSelection = false;
            ConnectingEnd(null);

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            // Base
            bool handled = base.OnMouseWheel(location, delta);
            if (!handled)
                CustomMouseWheel?.Invoke(ref location, delta, ref handled);
            if (handled)
            {
                return true;
            }

            // Change scale (disable scaling during selecting nodes)
            if (IsMouseOver && !_leftMouseDown && !_rightMouseDown && !IsPrimaryMenuOpened)
            {
                var nextViewScale = ViewScale + delta * 0.1f;

                // Scale towards/ away from mouse when zooming in/ out
                var nextCenterPosition = ViewPosition + location / ViewScale;
                ViewScale = nextViewScale;
                ViewPosition = nextCenterPosition - (location / ViewScale);

                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            // Base
            bool handled = base.OnMouseDoubleClick(location, button);
            if (!handled)
                CustomMouseDoubleClick?.Invoke(ref location, button, ref handled);

            // Insert reroute node
            if (!handled && CanEdit && CanUseNodeType(7, 29))
            {
                var mousePos = _rootControl.PointFromParent(ref _mousePos);
                if (IntersectsConnection(mousePos, out InputBox inputBox, out OutputBox outputBox) && GetControlUnderMouse() == null)
                {
                    if (Undo != null)
                    {
                        bool undoEnabled = Undo.Enabled;
                        Undo.Enabled = false;
                        var rerouteNode = Context.SpawnNode(7, 29, mousePos);
                        Undo.Enabled = undoEnabled;

                        var spawnNodeAction = new AddRemoveNodeAction(rerouteNode, true);

                        var disconnectBoxesAction = new ConnectBoxesAction(inputBox, outputBox, false);
                        inputBox.BreakConnection(outputBox);
                        disconnectBoxesAction.End();

                        var addConnectionsAction = new EditNodeConnections(Context, rerouteNode);
                        outputBox.CreateConnection(rerouteNode.GetBoxes().First(b => !b.IsOutput));
                        rerouteNode.GetBoxes().First(b => b.IsOutput).CreateConnection(inputBox);
                        addConnectionsAction.End();

                        Undo.AddAction(new MultiUndoAction(spawnNodeAction, disconnectBoxesAction, addConnectionsAction));
                    }
                    else
                    {
                        var rerouteNode = Context.SpawnNode(7, 29, mousePos);
                        inputBox.BreakConnection(outputBox);
                        outputBox.CreateConnection(rerouteNode.GetBoxes().First(b => !b.IsOutput));
                        rerouteNode.GetBoxes().First(b => b.IsOutput).CreateConnection(inputBox);
                    }
                    MarkAsEdited();

                    handled = true;
                }
            }

            return handled;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            // Check if user is connecting boxes
            if (_connectionInstigator != null)
                return true;

            // Base
            bool handled = base.OnMouseDown(location, button);
            if (!handled)
                CustomMouseDown?.Invoke(ref location, button, ref handled);
            if (handled)
            {
                // Clear flags
                _isMovingSelection = false;
                _rightMouseDown = false;
                _leftMouseDown = false;
                _middleMouseDown = false;
                return true;
            }

            // Just reset the input text whenever the user presses anywhere
            ResetInput();

            // Cache data
            _isMovingSelection = false;
            _mousePos = location;
            if(_temporarySelectedNodes == null)
                _temporarySelectedNodes = new HashSet<SurfaceNode>();
            else
                _temporarySelectedNodes.Clear();

            for (int i = 0; i < _rootControl.Children.Count; i++)
            {
                if (_rootControl.Children[i] is SurfaceNode node && node.IsSelected)
                {
                    _temporarySelectedNodes.Add(node);
                }
            }

            if (button == MouseButton.Left)
            {
                _leftMouseDown = true;
                _leftMouseDownPos = location;
            }
            if (button == MouseButton.Right)
            {
                _rightMouseDown = true;
                _rightMouseDownPos = location;
            }
            if (button == MouseButton.Middle)
            {
                _middleMouseDown = true;
                _middleMouseDownPos = location;
            }

            // Check if any node is under the mouse
            SurfaceControl controlUnderMouse = GetControlUnderMouse();
            var cLocation = _rootControl.PointFromParent(ref location);
            if (controlUnderMouse != null)
            {
                // Check if mouse is over header and user is pressing mouse left button
                if (_leftMouseDown && controlUnderMouse.CanSelect(ref cLocation))
                {
                    // Check if user is pressing control
                    if (Root.GetKey(KeyboardKeys.Control))
                    {
                        AddToSelection(controlUnderMouse);
                    }
                    else if (Root.GetKey(KeyboardKeys.Shift))
                    {
                        RemoveFromSelection(controlUnderMouse);
                    }
                    // Check if node isn't selected
                    else if (!controlUnderMouse.IsSelected)
                    {
                        // Select node
                        Select(controlUnderMouse);
                    }

                    // Start moving selected nodes
                    if (!Root.GetKey(KeyboardKeys.Shift))
                    {
                        StartMouseCapture();
                        _movingSelectionViewPos = _rootControl.Location;
                        _movingNodesDelta = Float2.Zero;
                        OnGetNodesToMove();
                    }

                    Focus();
                    return true;
                }
            }
            else
            {
                // Cache flags and state
                if (_leftMouseDown)
                {
                    // Start selecting or commenting
                    StartMouseCapture();

                    if (!Root.GetKey(KeyboardKeys.Control) && !Root.GetKey(KeyboardKeys.Shift))
                    {
                        ClearSelection();
                    }

                    Focus();
                    return true;
                }
                if (_rightMouseDown || _middleMouseDown)
                {
                    // Start navigating
                    StartMouseCapture();
                    Focus();
                    return true;
                }
            }

            Focus();
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // Cache mouse location
            _mousePos = location;

            // Check if any control is under the mouse
            SurfaceControl controlUnderMouse = GetControlUnderMouse();

            // Cache flags and state
            if (_leftMouseDown && button == MouseButton.Left)
            {
                _leftMouseDown = false;
                EndMouseCapture();
                Cursor = CursorType.Default;

                // Moving nodes
                if (_isMovingSelection)
                {
                    if (_movingNodes != null && _movingNodes.Count > 0)
                    {
                        if (Undo != null && !_movingNodesDelta.IsZero && CanEdit)
                            Undo.AddAction(new MoveNodesAction(Context, _movingNodes.Select(x => x.ID).ToArray(), _movingNodesDelta));
                        _movingNodes.Clear();
                    }
                    _movingNodesDelta = Float2.Zero;
                }
                // Connecting
                else if (_connectionInstigator != null)
                {
                }
                // Selecting
                else
                {
                    UpdateSelectionRectangle();
                }
            }
            bool showPrimaryMenu = false;
            if (_rightMouseDown && button == MouseButton.Right)
            {
                _rightMouseDown = false;
                EndMouseCapture();
                Cursor = CursorType.Default;

                // Check if no move has been made at all
                if (_mouseMoveAmount < 3.0f)
                {
                    // Check if any control is under the mouse
                    _cmStartPos = location;
                    if (controlUnderMouse == null)
                    {
                        showPrimaryMenu = true;
                    }
                }
                _mouseMoveAmount = 0;
            }
            if (_middleMouseDown && button == MouseButton.Middle)
            {
                _middleMouseDown = false;
                if (_middleMouseDown)
                {
                    EndMouseCapture();
                    Cursor = CursorType.Default;
                }
                if (_mouseMoveAmount > 0 && _middleMouseDown)
                    _mouseMoveAmount = 0;
                else if (CanEdit)
                {
                    // Surface was not moved with MMB so try to remove connection underneath
                    var mousePos = _rootControl.PointFromParent(ref location);
                    if (IntersectsConnection(mousePos, out InputBox inputBox, out OutputBox outputBox))
                    {
                        var action = new EditNodeConnections(inputBox.ParentNode.Context, inputBox.ParentNode);
                        inputBox.BreakConnection(outputBox);
                        action.End();
                        AddBatchedUndoAction(action);
                        MarkAsEdited();
                    }
                }
            }

            // Base
            bool handled = base.OnMouseUp(location, button);
            if (!handled)
                CustomMouseUp?.Invoke(ref location, button, ref handled);
            if (handled)
            {
                // Clear flags
                _rightMouseDown = false;
                _leftMouseDown = false;
                _middleMouseDown = false;
                return true;
            }

            // If none of the child controls handled this show the primary context menu
            if (showPrimaryMenu)
            {
                ShowPrimaryMenu(_cmStartPos);
            }
            // Letting go of a connection or right clicking while creating a connection
            else if (!_isMovingSelection && _connectionInstigator != null && !IsPrimaryMenuOpened)
            {
                _cmStartPos = location;
                Cursor = CursorType.Default;
                EndMouseCapture();
                ShowPrimaryMenu(_cmStartPos);
            }

            return true;
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (base.OnCharInput(c))
                return true;

            InputText += c;
            return true;
        }

        private void MoveSelectedNodes(Float2 delta)
        {
            // TODO: undo
            delta /= _targetScale;
            OnGetNodesToMove();
            foreach (var node in _movingNodes)
                node.Location += delta;
            _isMovingSelection = false;
            MarkAsEdited(false);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            if (InputActions.Process(Editor.Instance, this, key))
                return true;

            if (HasNodesSelection)
            {
                var keyMoveRange = 50;
                switch (key)
                {
                case KeyboardKeys.Backspace:
                    if (InputText.Length > 0)
                        InputText = InputText.Substring(0, InputText.Length - 1);
                    return true;
                case KeyboardKeys.Escape:
                    ClearSelection();
                    return true;
                case KeyboardKeys.Return:
                {
                    Box selectedBox = GetSelectedBox(SelectedNodes, false);
                    Box toSelect = selectedBox?.ParentNode.GetNextBox(selectedBox);
                    if (toSelect != null)
                    {
                        Select(toSelect.ParentNode);
                        toSelect.ParentNode.SelectBox(toSelect);
                    }
                    return true;
                }
                case KeyboardKeys.ArrowUp:
                case KeyboardKeys.ArrowDown:
                {
                    // Selected box navigation
                    Box selectedBox = GetSelectedBox(SelectedNodes);
                    if (selectedBox != null)
                    {
                        Box toSelect = (key == KeyboardKeys.ArrowUp) ? selectedBox?.ParentNode.GetPreviousBox(selectedBox) : selectedBox?.ParentNode.GetNextBox(selectedBox);
                        if (toSelect != null && toSelect.IsOutput == selectedBox.IsOutput)
                        {
                            Select(toSelect.ParentNode);
                            toSelect.ParentNode.SelectBox(toSelect);
                        }
                    }
                    else if (!IsMovingSelection && CanEdit)
                    {
                        // Move selected nodes
                        var delta = new Float2(0, key == KeyboardKeys.ArrowUp ? -keyMoveRange : keyMoveRange);
                        MoveSelectedNodes(delta);
                    }
                    return true;
                }
                case KeyboardKeys.ArrowRight:
                case KeyboardKeys.ArrowLeft:
                {
                    // Selected box navigation
                    Box selectedBox = GetSelectedBox(SelectedNodes);
                    if (selectedBox != null)
                    {
                        Box toSelect = null;
                        if ((key == KeyboardKeys.ArrowRight && selectedBox.IsOutput) || (key == KeyboardKeys.ArrowLeft && !selectedBox.IsOutput))
                        {
                            if (_selectedConnectionIndex < 0 || _selectedConnectionIndex >= selectedBox.Connections.Count)
                            {
                                _selectedConnectionIndex = 0;
                            }
                            toSelect = selectedBox.Connections[_selectedConnectionIndex];
                        }
                        else
                        {
                            // Use the node with the closest Y-level
                            // Since there are cases like 3 nodes on one side and only 1 node on the other side
                            var elements = selectedBox.ParentNode.Elements;
                            float minDistance = float.PositiveInfinity;
                            for (int i = 0; i < elements.Count; i++)
                            {
                                if (elements[i] is Box box && box.IsOutput != selectedBox.IsOutput && Mathf.Abs(box.Y - selectedBox.Y) < minDistance)
                                {
                                    toSelect = box;
                                    minDistance = Mathf.Abs(box.Y - selectedBox.Y);
                                }
                            }
                        }

                        if (toSelect != null)
                        {
                            Select(toSelect.ParentNode);
                            toSelect.ParentNode.SelectBox(toSelect);
                        }
                    }
                    else if (!IsMovingSelection && CanEdit)
                    {
                        // Move selected nodes
                        var delta = new Float2(key == KeyboardKeys.ArrowLeft ? -keyMoveRange : keyMoveRange, 0);
                        MoveSelectedNodes(delta);
                    }
                    return true;
                }
                case KeyboardKeys.Tab:
                {
                    Box selectedBox = GetSelectedBox(SelectedNodes, false);
                    if (selectedBox == null)
                        return true;

                    int connectionCount = selectedBox.Connections.Count;
                    if (connectionCount == 0)
                        return true;

                    if (Root.GetKey(KeyboardKeys.Shift))
                    {
                        _selectedConnectionIndex = ((_selectedConnectionIndex - 1) % connectionCount + connectionCount) % connectionCount;
                    }
                    else
                    {
                        _selectedConnectionIndex = (_selectedConnectionIndex + 1) % connectionCount;
                    }
                    return true;
                }
                }
            }

            return false;
        }

        private void ResetInput()
        {
            InputText = "";
            _inputBrackets.Clear();
        }

        private void CurrentInputTextChanged(string currentInputText)
        {
            if (string.IsNullOrEmpty(currentInputText))
                return;
            if (IsPrimaryMenuOpened || !CanEdit)
            {
                InputText = "";
                return;
            }

            var selection = SelectedNodes;
            if (selection.Count == 0)
            {
                if (_inputBrackets.Count == 0)
                {
                    if (currentInputText.StartsWith(' '))
                        currentInputText = "";
                    ResetInput();
                    ShowPrimaryMenu(_mousePos, false, currentInputText);
                }
                else
                {
                    InputText = "";
                    ShowPrimaryMenu(_rootControl.PointToParent(_inputBrackets.Peek().Area.Location), true, currentInputText);
                }
                return;
            }

            // Multi-Node Editing
            const string Comment = "//";
            if (currentInputText.StartsWith(Comment))
            {
                InputText = "";
                var comment = CommentSelection(currentInputText.Substring(Comment.Length));
                comment.StartRenaming();
                return;
            }

            // TODO: What should happen when multiple nodes or multiple boxes are selected?
            if (selection.Count != 1)
                return;

            // Single Box Editing
            Box selectedBox = GetSelectedBox(selection, false);

            if (selectedBox == null)
                return;

            // TODO: Editing a primitive
            /* #    => color
             * 1,43 => Vector2
             * =    => set node name
             * etc.
             */
            if (currentInputText.StartsWith("("))
            {
                InputText = InputText.Substring(1);

                // Opening bracket
                if (!selectedBox.IsOutput)
                {
                    var bracket = new InputBracket(selectedBox, FindEmptySpace(selectedBox));
                    _inputBrackets.Push(bracket);
                    Deselect(selectedBox.ParentNode);
                }
            }
            else if (currentInputText.StartsWith(")"))
            {
                InputText = InputText.Substring(1);

                // Closing bracket
                if (_inputBrackets.Count > 0)
                {
                    var bracket = _inputBrackets.Pop();
                    bracket.Update();

                    if (selectedBox.IsOutput)
                    {
                        TryConnect(selectedBox, bracket.Box);
                    }
                }
            }
            else
            {
                InputText = "";

                // Add a new node
                ConnectingStart(selectedBox);
                Cursor = CursorType.Default; // Do I need this?
                EndMouseCapture();
                ShowPrimaryMenu(_rootControl.PointToParent(FindEmptySpace(selectedBox)), true, currentInputText);
            }
        }

        private Box GetSelectedBox(List<SurfaceNode> selection, bool onlyIfSelected = true)
        {
            if (selection.Count != 1)
                return null; // TODO: Handle multiple selected nodes

            // Get selected box
            SurfaceNode selectedNode = selection[0];
            Box selectedBox = null;
            for (int i = 0; i < selectedNode.Elements.Count; i++)
            {
                if (selectedNode.Elements[i] is Box box && box.IsSelected)
                {
                    if (selectedBox == null)
                    {
                        selectedBox = box;
                    }
                    else
                    {
                        // TODO: Multiple boxes are selected. How should this be handled?
                        return null;
                    }
                }
            }

            // Or get the first output box when a node with only output boxes is selected
            if (selectedBox == null && !onlyIfSelected)
            {
                for (int i = 0; i < selectedNode.Elements.Count; i++)
                {
                    if (selectedNode.Elements[i] is Box box)
                    {
                        if (box.IsOutput)
                        {
                            selectedBox = box;
                            break;
                        }
                    }
                }

                for (int i = 0; i < selectedNode.Elements.Count; i++)
                {
                    if (selectedNode.Elements[i] is Box box)
                    {
                        if (!box.IsOutput)
                        {
                            selectedBox = null;
                            break;
                        }
                    }
                }
            }

            return selectedBox;
        }

        private Float2 FindEmptySpace(Box box)
        {
            var distanceBetweenNodes = new Float2(30, 30);

            var node = box.ParentNode;

            // Same height as node
            float yLocation = node.Top;

            for (int i = 0; i < node.Elements.Count; i++)
            {
                if (node.Elements[i] is Box nodeBox && nodeBox.IsOutput == box.IsOutput && nodeBox.Y < box.Y)
                {
                    // Below connected node
                    yLocation = Mathf.Max(yLocation, nodeBox.ParentNode.Bottom + distanceBetweenNodes.Y);
                }
            }

            // TODO: Dodge the other nodes

            float xLocation = node.Location.X;
            if (box.IsOutput)
            {
                xLocation += node.Width + distanceBetweenNodes.X;
            }
            else
            {
                xLocation += -120 - distanceBetweenNodes.X;
            }

            return new Float2(xLocation, yLocation);
        }

        private bool IntersectsConnection(Float2 mousePosition, out InputBox inputBox, out OutputBox outputBox)
        {
            for (int i = 0; i < Nodes.Count; i++)
            {
                for (int j = 0; j < Nodes[i].Elements.Count; j++)
                {
                    if (Nodes[i].Elements[j] is OutputBox ob)
                    {
                        for (int k = 0; k < ob.Connections.Count; k++)
                        {
                            if (ob.IntersectsConnection(ob.Connections[k], ref mousePosition))
                            {
                                outputBox = ob;
                                inputBox = ob.Connections[k] as InputBox;
                                return true;
                            }
                        }
                    }
                }
            }

            outputBox = null;
            inputBox = null;
            return false;
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Surface.Elements;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Update scale
            var currentScale = _rootControl.Scale.X;
            if (Mathf.Abs(_targetScale - currentScale) > 0.001f)
            {
                var scale = new Float2(Mathf.Lerp(currentScale, _targetScale, deltaTime * 10.0f));
                _rootControl.Scale = scale;
            }

            // Navigate when mouse is near the edge and is doing sth
            bool isMovingWithMouse = false;
            if (IsMovingSelection || (IsConnecting && !IsPrimaryMenuOpened))
            {
                var moveVector = Float2.Zero;
                float edgeDetectDistance = 22.0f;
                if (_mousePos.X < edgeDetectDistance)
                {
                    moveVector.X -= 1;
                }
                if (_mousePos.Y < edgeDetectDistance)
                {
                    moveVector.Y -= 1;
                }
                if (_mousePos.X > Width - edgeDetectDistance)
                {
                    moveVector.X += 1;
                }
                if (_mousePos.Y > Height - edgeDetectDistance)
                {
                    moveVector.Y += 1;
                }
                moveVector.Normalize();
                isMovingWithMouse = moveVector.LengthSquared > Mathf.Epsilon;
                if (isMovingWithMouse)
                {
                    _rootControl.Location -= moveVector * _moveViewWithMouseDragSpeed;
                }
            }
            _moveViewWithMouseDragSpeed = isMovingWithMouse ? Mathf.Clamp(_moveViewWithMouseDragSpeed + deltaTime * 20.0f, 1.0f, 8.0f) : 1.0f;

            base.Update(deltaTime);

            // Batch undo actions
            if (_batchedUndoActions != null && _batchedUndoActions.Count != 0)
            {
                Undo.AddAction(_batchedUndoActions.Count == 1 ? _batchedUndoActions[0] : new MultiUndoAction(_batchedUndoActions));
                _batchedUndoActions.Clear();
            }
        }

        /// <summary>
        /// Draws the surface background.
        /// </summary>
        protected virtual void DrawBackground()
        {
            DrawBackgroundDefault(Style.Background, Width, Height);
        }

        internal static void DrawBackgroundDefault(Texture background, float width, float height)
        {
            if (background && background.ResidentMipLevels > 0)
            {
                var bSize = background.Size;
                float bw = bSize.X;
                float bh = bSize.Y;
                var pos = Float2.Mod(bSize);

                if (pos.X > 0)
                    pos.X -= bw;
                if (pos.Y > 0)
                    pos.Y -= bh;

                int maxI = Mathf.CeilToInt(width / bw + 1.0f);
                int maxJ = Mathf.CeilToInt(height / bh + 1.0f);

                for (int i = 0; i < maxI; i++)
                {
                    for (int j = 0; j < maxJ; j++)
                    {
                        Render2D.DrawTexture(background, new Rectangle(pos.X + i * bw, pos.Y + j * bh, bw, bh), Color.White);
                    }
                }
            }
        }

        /// <summary>
        /// Draws the selection background.
        /// </summary>
        /// <remarks>Called only when user is selecting nodes using rectangle tool.</remarks>
        protected virtual void DrawSelection()
        {
            var style = FlaxEngine.GUI.Style.Current;
            var selectionRect = Rectangle.FromPoints(_leftMouseDownPos, _mousePos);
            Render2D.FillRectangle(selectionRect, style.Selection);
            Render2D.DrawRectangle(selectionRect, style.SelectionBorder);
        }

        /// <summary>
        /// Draws all the connections between surface nodes.
        /// </summary>
        /// <param name="mousePosition">The current mouse position (in surface-space).</param>
        protected virtual void DrawConnections(ref Float2 mousePosition)
        {
            // Draw all connections at once to boost batching process
            for (int i = 0; i < Nodes.Count; i++)
            {
                Nodes[i].DrawConnections(ref mousePosition);
                Nodes[i].DrawSelectedConnections(_selectedConnectionIndex);
            }
        }

        /// <summary>
        /// Draws the connecting line.
        /// </summary>
        /// <remarks>Called only when user is connecting nodes.</remarks>
        protected virtual void DrawConnectingLine()
        {
            // Get start position
            var startPos = _connectionInstigator.ConnectionOrigin;

            // Check if mouse is over any of box
            var cmVisible = _activeVisjectCM != null && _activeVisjectCM.Visible;
            var endPos = cmVisible ? _rootControl.PointFromParent(ref _cmStartPos) : _rootControl.PointFromParent(ref _mousePos);
            Color lineColor = Style.Colors.Connecting;
            if (_lastInstigatorUnderMouse != null && !cmVisible)
            {
                // Check if can connect objects
                bool canConnect = _connectionInstigator.CanConnectWith(_lastInstigatorUnderMouse);
                lineColor = canConnect ? Style.Colors.ConnectingValid : Style.Colors.ConnectingInvalid;
                endPos = _lastInstigatorUnderMouse.ConnectionOrigin;
            }

            Float2 actualStartPos = startPos;
            Float2 actualEndPos = endPos;

            if (_connectionInstigator is Archetypes.Tools.RerouteNode)
            {
                if (endPos.X < startPos.X && _lastInstigatorUnderMouse is null or Box { IsOutput: true })
                {
                    actualStartPos = endPos;
                    actualEndPos = startPos;
                }
            }
            else if (_connectionInstigator is Box { IsOutput: false })
            {
                actualStartPos = endPos;
                actualEndPos = startPos;
            }

            // Draw connection
            _connectionInstigator.DrawConnectingLine(ref actualStartPos, ref actualEndPos, ref lineColor);
        }

        /// <summary>
        /// Draws the brackets and connections
        /// </summary>
        protected virtual void DrawInputBrackets()
        {
            var style = FlaxEngine.GUI.Style.Current;
            var fadedColor = style.ForegroundGrey;
            foreach (var inputBracket in _inputBrackets)
            {
                // Draw brackets
                var upperLeft = _rootControl.PointToParent(inputBracket.Area.UpperLeft);
                var bottomRight = _rootControl.PointToParent(inputBracket.Area.BottomRight);

                var upperRight = new Float2(bottomRight.X, upperLeft.Y);
                var bottomLeft = new Float2(upperLeft.X, bottomRight.Y);

                // Calculate control points
                float height = bottomLeft.Y - upperLeft.Y;
                float offsetX = height / 10f;
                var leftControl1 = new Float2(bottomLeft.X - offsetX, bottomLeft.Y);
                var leftControl2 = new Float2(upperLeft.X - offsetX, upperLeft.Y);

                // Draw left bracket
                Render2D.DrawBezier(bottomLeft, leftControl1, leftControl2, upperLeft, style.Foreground, 2.2f);

                // Calculate control points
                var rightControl1 = new Float2(bottomRight.X + offsetX, bottomRight.Y);
                var rightControl2 = new Float2(upperRight.X + offsetX, upperRight.Y);

                // Draw right bracket
                Render2D.DrawBezier(bottomRight, rightControl1, rightControl2, upperRight, fadedColor, 2.2f);

                // Draw connection bezier
                // X-offset at 75%
                var bezierStartPoint = new Float2(upperRight.X + offsetX * 0.75f, (upperRight.Y + bottomRight.Y) * 0.5f);
                var bezierEndPoint = inputBracket.Box.ParentNode.PointToParent(_rootControl.Parent, inputBracket.Box.Center);

                Elements.OutputBox.DrawConnection(Style, ref bezierStartPoint, ref bezierEndPoint, ref fadedColor);

                // Debug Area
                //Rectangle drawRect = Rectangle.FromPoints(upperLeft, bottomRight);
                //Render2D.FillRectangle(drawRect, Color.Green * 0.5f);
            }
        }

        /// <summary>
        /// Draws the contents of the surface (nodes, connections, comments, etc.).
        /// </summary>
        protected virtual void DrawContents()
        {
            base.Draw();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = FlaxEngine.GUI.Style.Current;
            var rect = new Rectangle(Float2.Zero, Size);

            DrawBackground();

            _rootControl.DrawComments();

            // Reset input flags here because this is the closest to Update we have
            WasBoxSelecting = IsBoxSelecting;
            WasMovingSelection = IsMovingSelection;

            if (IsBoxSelecting)
            {
                DrawSelection();
            }

            // Push surface view transform (scale and offset)
            Render2D.PushTransform(ref _rootControl._cachedTransform);
            var features = Render2D.Features;
            Render2D.Features = Render2D.RenderingFeatures.None;

            var mousePos = _rootControl.PointFromParent(ref _mousePos);
            DrawConnections(ref mousePos);

            if (IsConnecting)
            {
                DrawConnectingLine();
            }

            Render2D.Features = features;
            Render2D.PopTransform();

            DrawInputBrackets();

            DrawContents();

            //Render2D.DrawText(style.FontTitle, string.Format("Scale: {0}", _rootControl.Scale), rect, Enabled ? Color.Red : Color.Black);

            // Draw border
            if (ContainsFocus)
            {
                Render2D.DrawRectangle(new Rectangle(1, 1, rect.Width - 2, rect.Height - 2), Editor.IsPlayMode ? Color.OrangeRed : style.BackgroundSelected);
            }

            // Draw disabled overlay
            //if (!Enabled)
            //    Render2D.FillRectangle(rect, new Color(0.2f, 0.2f, 0.2f, 0.5f), true);
        }
    }
}

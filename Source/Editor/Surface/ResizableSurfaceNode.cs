// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Visject Surface node control that can be resized.
    /// </summary>
    /// <seealso cref="SurfaceNode" />
    [HideInEditor]
    public class ResizableSurfaceNode : SurfaceNode
    {
        /// <summary>
        /// Helper class for <see cref="ResizableSurfaceNode"/> that handles mouse interactions resizing the node itself.
        /// </summary>
        public class ResizeBorder : ContainerControl
        {
            /// <summary>
            /// Distance to each of the 4 node edges that the cursor has to be so the user can resize along the direction of the edge.
            /// </summary>
            private const float BorderWidth = 15f;

            private readonly VisjectSurface _surface;
            private Float2 _lastSurfaceMouseLoc;
            private Float2 startResizingSize;
            private Float2 noClampedSize;

            /// <summary>
            /// Whether to ignore the surface index in parent when updating the cursor type. Set to <code>false</code> for nodes that have order like <see cref="SurfaceComment"/>.
            /// </summary>
            internal bool IgnoreSurfaceIndex = true;

            /// <summary>
            /// The resizable node that this <see cref="ResizeBorder"/> controls.
            /// </summary>
            public readonly ResizableSurfaceNode ResizableNode;

            /// <summary>
            /// True if the mouse is at the border of the resizable node and not further away from the border than <see cref="BorderWidth"/>.
            /// </summary>
            public bool IsMouseOverResizeBorder { get; private set; }

            /// <summary>
            /// True if <see cref="ResizableNode"/> is being resized.
            /// </summary>
            public bool IsResizing { get; private set; }

            /// <summary>
            /// The direction in which to resize the node. Should be either -1, 0 or 1 on both axes.
            /// </summary>
            public Float2 ResizeDirection { get; private set; }

            /// <summary>
            /// The type of cursor to show to hint to the user that they can resize the node in a given direction.
            /// </summary>
            public CursorType CursorType
            {
                get
                {
                    if ((ResizeDirection.X == 1 && ResizeDirection.Y == 0) || (ResizeDirection.X == -1 && ResizeDirection.Y == 0))
                        return CursorType.SizeWE;
                    if ((ResizeDirection.X == 0 && ResizeDirection.Y == 1) || (ResizeDirection.X == 0 && ResizeDirection.Y == -1))
                        return CursorType.SizeNS;
                    if ((ResizeDirection.X == -1 && ResizeDirection.Y == -1) || (ResizeDirection.X == 1 && ResizeDirection.Y == 1))
                        return CursorType.SizeNWSE;
                    if ((ResizeDirection.X == 1 && ResizeDirection.Y == -1) || (ResizeDirection.X == -1 && ResizeDirection.Y == 1))
                        return CursorType.SizeNESW;

                    return CursorType.Default;
                }
            }

            /// <summary>
            /// Creates a new instance of <see cref="ResizeBorder"/>.
            /// </summary>
            /// <param name="surface">The surface.</param>
            /// <param name="resizableNode">The <see cref="ResizableSurfaceNode "/> this controls.</param>
            public ResizeBorder(VisjectSurface surface, ResizableSurfaceNode resizableNode)
            {
                _surface = surface;
                ResizableNode = resizableNode;
            }

            /// <summary>
            /// Updates location and size to match the resizable node with the additional padding.
            /// </summary>
            /// <param name="nodeSize">The node size.</param>
            /// <param name="nodeLocation">The node location.</param>
            public void MatchResizableNode(Float2 nodeSize, Float2 nodeLocation)
            {
                Size = nodeSize + new Float2(BorderWidth * 2);
                Location = nodeLocation - new Float2(BorderWidth);
            }

            private void UpdateResizeFlags(Float2 mouseLocation)
            {
                var borderRect = Bounds with { Location = Float2.Zero };
                bool onBorder = borderRect.Contains(mouseLocation);
                // Check this the way we do because some resizable nodes (like comments) have an implementation of `Control.ContainsPoint`
                // that does not check for the size you would think they have based on their visual appearance
                bool inNode = borderRect.MakeExpanded(-BorderWidth * 2f).Contains(mouseLocation);

                Float2 rawResizeDirection = (mouseLocation - borderRect.Center);
                var nodeHalfSizeNoBorder = ResizableNode.Size * 0.5f - BorderWidth;
                ResizeDirection = new Float2(Mathf.Abs(rawResizeDirection.X) >= nodeHalfSizeNoBorder.X ? Mathf.Sign(rawResizeDirection.X) : 0,
                                             Mathf.Abs(rawResizeDirection.Y) >= nodeHalfSizeNoBorder.Y ? Mathf.Sign(rawResizeDirection.Y) : 0);

                IsMouseOverResizeBorder = onBorder && !inNode;
            }

            private Float2 GetControlDelta(Control control, Float2 start, Float2 end)
            {
                var pointOrigin = control.Parent ?? control;
                var startPos = pointOrigin.PointFromParent(ResizableNode, start);
                var endPos = pointOrigin.PointFromParent(ResizableNode, end);
                return endPos - startPos;
            }

            private void EndResizing()
            {
                EndMouseCapture();
                IsResizing = false;
                if (startResizingSize != ResizableNode.Size)
                {
                    var emptySize = ResizableNode.CalculateNodeSize(0, 0);
                    ResizableNode.SizeValue = ResizableNode.Size - emptySize;
                    _surface.MarkAsEdited(false);
                }
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                if (!IsResizing)
                {
                    UpdateResizeFlags(location);
                }
                else if (_surface.CanEdit)
                {
                    var resizeAxisAbs = ResizeDirection.Absolute;
                    var resizeAxisPos = Float2.Clamp(ResizeDirection, Float2.Zero, Float2.One);
                    var resizeAxisNeg = Float2.Clamp(-ResizeDirection, Float2.Zero, Float2.One);

                    var currentSurfaceMouseLoc = _surface.PointFromScreen(Input.MouseScreenPosition);
                    var delta = currentSurfaceMouseLoc - _lastSurfaceMouseLoc;

                    // TODO: Snapping
                    delta *= resizeAxisAbs;
                    var moveLocation = currentSurfaceMouseLoc;
                    var uiControlDelta = GetControlDelta(this, _lastSurfaceMouseLoc, moveLocation);

                    // This ensures that the node is not resized below min size, but also keeps the resize behavior like expected (which just Max() -ing the value does not)
                    var emptySize = ResizableNode.CalculateNodeSize(0, 0);
                    var minSize = ResizableNode.sizeMin;
                    noClampedSize = noClampedSize + uiControlDelta * resizeAxisPos - uiControlDelta * resizeAxisNeg;
                    if (noClampedSize.X < minSize.X && noClampedSize.X < ResizableNode.Size.X)
                        resizeAxisAbs.X = resizeAxisPos.X = resizeAxisNeg.X = 0f;
                    if (noClampedSize.Y < minSize.Y && noClampedSize.Y < ResizableNode.Size.Y)
                        resizeAxisAbs.Y = resizeAxisPos.Y = resizeAxisNeg.Y = 0f;

                    ResizableNode.Size += uiControlDelta * resizeAxisPos - uiControlDelta * resizeAxisNeg;
                    ResizableNode.Location += uiControlDelta * resizeAxisNeg;
                    ResizableNode.SizeValue = ResizableNode.Size - emptySize;

                    ResizableNode.CalculateNodeSize(ResizableNode.Size.X, ResizableNode.Size.Y);
                    MatchResizableNode(ResizableNode.Size, ResizableNode.Location);

                    _lastSurfaceMouseLoc = currentSurfaceMouseLoc;
                }

                // Update the cursor shape
                if ((_surface.resizeableNodeIndexInParent <= IndexInParent || IgnoreSurfaceIndex) && !_surface.IsConnecting && _surface.CanEdit)
                {
                    if (!IgnoreSurfaceIndex)
                        _surface.resizeableNodeIndexInParent = IndexInParent;

                    if (IsMouseOverResizeBorder)
                        Cursor = CursorType;
                    else
                        Cursor = CursorType.Default;
                }

                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override void OnMouseEnter(Float2 location)
            {
                Cursor = CursorType.Default;
                base.OnMouseEnter(location);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                Cursor = CursorType.Default;
                IsMouseOverResizeBorder = false;
                _surface.resizeableNodeIndexInParent = -1; // Will get updated by MouseMove again to match current index
                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && IsMouseOverResizeBorder && !IsResizing)
                {
                    // Start resizing
                    _lastSurfaceMouseLoc = _surface.PointFromScreen(Input.MouseScreenPosition);
                    noClampedSize = ResizableNode.Size;
                    IsResizing = true;
                    startResizingSize = ResizableNode.Size;
                    StartMouseCapture();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && IsResizing)
                {
                    Cursor = CursorType.Default;
                    EndResizing();
                    return true;
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                if (IsResizing)
                    EndResizing();

                base.OnLostFocus();
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                if (IsResizing)
                    EndResizing();

                base.OnEndMouseCapture();
            }
        }

        /// <summary>
        /// Represents the border control used for resizing the associated element.
        /// </summary>
        public ResizeBorder ResizeBorderControl;

        /// <summary>
        /// Index of the Float2 value in the node values list to store node size.
        /// </summary>
        protected int _sizeValueIndex = -1;

        /// <summary>
        /// Minimum node size.
        /// </summary>
        protected Float2 sizeMin = new Float2(240, 160);

        private Float2 SizeValue
        {
            get => (Float2)Values[_sizeValueIndex];
            set => SetValue(_sizeValueIndex, value, false);
        }

        /// <inheritdoc />
        public ResizableSurfaceNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
        : base(id, context, nodeArch, groupArch)
        {
            ResizeBorderControl = new ResizeBorder(Surface, this)
            {
                Parent = Surface.SurfaceRoot,
            };

            Parent = ResizeBorderControl;
            ResizeBorderControl.MatchResizableNode(Size, Location);
        }

        /// <inheritdoc />
        protected override void OnLocationChanged()
        {
            ResizeBorderControl.MatchResizableNode(Size, Location);
            base.OnLocationChanged();
        }

        /// <inheritdoc />
        public override void OnSurfaceLoaded(SurfaceNodeActions action)
        {
            // Reapply the node size
            var size = SizeValue;
            if (Surface != null && Surface.GridSnappingEnabled)
                size = Surface.SnapToGrid(size, true);
            Resize(size.X, size.Y);
            ResizeBorderControl.MatchResizableNode(Size, Location);

            base.OnSurfaceLoaded(action);
        }

        /// <inheritdoc />
        public override void OnValuesChanged()
        {
            base.OnValuesChanged();

            var size = SizeValue;
            Resize(size.X, size.Y);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (Surface.CanEdit && !Surface.IsConnecting && (ResizeBorderControl.IsResizing || ResizeBorderControl.IsMouseOverResizeBorder))
                Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Style.Current.Foreground, 0.5f);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            ResizeBorderControl.Parent = null;
            base.OnDestroy();
        }
    }
}

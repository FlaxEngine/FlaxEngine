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
        private class ResizeBorder : Control
        {
            private const float BorderWidth = 15f;
            private const float ResizeOnAxisThreshold = 0.85f;

            public VisjectSurface Surface;
            public ResizableSurfaceNode ResizeableNode;

            private Float2 _surfaceMouseLocation;
            private Float2 startResizingSize;

            public bool IsMouseOverResizeBorder { get; private set; }
            public bool IsResizing { get; private set; }

            public Action StartResize;
            public Action EndResize;

            public Float2 ResizeWeight { get; private set; }
            public CursorType CursorType
            {
                get
                {
                    if ((ResizeWeight.X == 1 && ResizeWeight.Y == 0) || (ResizeWeight.X == -1 && ResizeWeight.Y == 0))
                        return CursorType.SizeWE;
                    if ((ResizeWeight.X == 0 && ResizeWeight.Y == 1) || (ResizeWeight.X == 0 && ResizeWeight.Y == -1))
                        return CursorType.SizeNS;
                    if ((ResizeWeight.X == -1 && ResizeWeight.Y == -1) || (ResizeWeight.X == 1 && ResizeWeight.Y == 1))
                        return CursorType.SizeNWSE;
                    if ((ResizeWeight.X == 1 && ResizeWeight.Y == -1) || (ResizeWeight.X == -1 && ResizeWeight.Y == 1))
                        return CursorType.SizeNESW;

                    return CursorType.Default;
                }
            }
            


            public ResizeBorder(VisjectSurface surface, ResizableSurfaceNode resizeableNode)
            {
                Surface = surface;
                ResizeableNode = resizeableNode;
            }

            /// <summary>
            /// Updates location and size to match the resizeable node with the additional padding.
            /// </summary>
            /// <param name="nodeSize">The node size.</param>
            /// <param name="nodeLocation">The node location.</param>
            public void MatchResizeableNode(Float2 nodeSize, Float2 nodeLocation)
            {
                Size = nodeSize + new Float2(BorderWidth);
                Location = nodeLocation - new Float2(BorderWidth * 0.5f);
            }

            private void UpdateSurfaceMouseLocation()
            {
                _surfaceMouseLocation = Surface.PointFromScreen(Input.MouseScreenPosition);
            }

            private void UpdateResizeFlags(Float2 location)
            {
                var borderRect = Bounds with { Location = Float2.Zero };
                bool onBorder = borderRect.Contains(location);

                Float2 rawResizeWeight = (location - borderRect.Center) / (borderRect.Size * 0.5f);
                ResizeWeight = new Float2(Mathf.Abs(rawResizeWeight.X) >= ResizeOnAxisThreshold ? Mathf.Sign(rawResizeWeight.X) : 0,
                                        Mathf.Abs(rawResizeWeight.Y) >= ResizeOnAxisThreshold ? Mathf.Sign(rawResizeWeight.Y) : 0);

                IsMouseOverResizeBorder = onBorder && !ResizeableNode.IsMouseOver;
            }

            private Float2 GetControlDelta(Control control, ref Float2 start, ref Float2 end)
            {
                var pointOrigin = control.Parent ?? control;
                var startPos = pointOrigin.PointFromParent(ResizeableNode, start);
                var endPos = pointOrigin.PointFromParent(ResizeableNode, end);
                return endPos - startPos;
            }

            private void EndResizing()
            {
                EndMouseCapture();
                IsResizing = false;
                if (startResizingSize != ResizeableNode.Size)
                {
                    var emptySize = ResizeableNode.CalculateNodeSize(0, 0);
                    ResizeableNode.SizeValue = ResizeableNode.Size - emptySize;
                    Surface.MarkAsEdited(false);
                }
            }

            public override void OnMouseLeave()
            {
                Cursor = CursorType.Default;
                IsMouseOverResizeBorder = false;
                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                if (!IsResizing)
                {
                    UpdateResizeFlags(location);
                }
                else
                {
                    var resizeAxisAbs = ResizeWeight.Absolute;
                    var resizeAxisPos = Float2.Clamp(ResizeWeight, Float2.Zero, Float2.One);
                    var resizeAxisNeg = Float2.Clamp(-ResizeWeight, Float2.Zero, Float2.One);

                    var currentSurfaceMouse = Surface.PointFromScreen(Input.MouseScreenPosition);
                    var delta = currentSurfaceMouse - _surfaceMouseLocation;
                    // TODO: scale/size snapping?
                    delta *= resizeAxisAbs;

                    var moveLocation = _surfaceMouseLocation + delta;

                    // TODO: Do I need GetControlDelta?
                    var uiControlDelta = GetControlDelta(this, ref _surfaceMouseLocation, ref moveLocation);
                    var emptySize = ResizeableNode.CalculateNodeSize(0, 0);
                    ResizeableNode.Size += uiControlDelta * resizeAxisPos - uiControlDelta * resizeAxisNeg;
                    Float2 oldSize = ResizeableNode.Size;
                    ResizeableNode.Size = new Float2(Mathf.Max(ResizeableNode.Size.X, ResizeableNode.sizeMin.X), Mathf.Max(ResizeableNode.Size.Y, ResizeableNode.sizeMin.Y));
                    if (oldSize == ResizeableNode.Size) // Only move if size wasn't clamped
                    {
                        ResizeableNode.Location += uiControlDelta * resizeAxisNeg;
                    }
                    ResizeableNode.SizeValue = ResizeableNode.Size - emptySize;
                    ResizeableNode.SizeValue = new Float2(Mathf.Max(ResizeableNode.SizeValue.X, ResizeableNode.sizeMin.X), Mathf.Max(ResizeableNode.SizeValue.Y, ResizeableNode.sizeMin.Y));
                    ResizeableNode.CalculateNodeSize(ResizeableNode.Size.X, ResizeableNode.Size.Y);
                    UpdateSurfaceMouseLocation();
                    MatchResizeableNode(ResizeableNode.Size, ResizeableNode.Location);
                }

                if (IsMouseOverResizeBorder)
                    Cursor = CursorType;
                else
                    Cursor = CursorType.Default;
                


                base.OnMouseMove(location);
            }

            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && IsMouseOverResizeBorder && !IsResizing)
                {
                    // Start resizing
                    UpdateSurfaceMouseLocation();
                    IsResizing = true;
                    startResizingSize = ResizeableNode.Size;
                    StartMouseCapture();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

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

            //public override void Draw()
            //{
            //    Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Color.Blue, 1f);
            //}
        }


        private ResizeBorder resizeBorder;

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
            CullChildren = false;
            ClipChildren = false;
            resizeBorder = new ResizeBorder(Surface, this)
            {
                Parent = Surface.SurfaceRoot,
            };

            resizeBorder.MatchResizeableNode(Size, Location);
        }

        /// <inheritdoc />
        protected override void OnLocationChanged()
        {
            resizeBorder.MatchResizeableNode(Size, Location);
            base.OnLocationChanged();
        }

        /// <inheritdoc />
        public override bool CanSelect(ref Float2 location)
        {
            return base.CanSelect(ref location);
        }

        /// <inheritdoc />
        public override void OnSurfaceLoaded(SurfaceNodeActions action)
        {
            // Reapply the curve node size
            var size = SizeValue;
            if (Surface != null && Surface.GridSnappingEnabled)
                size = Surface.SnapToGrid(size, true);
            Resize(size.X, size.Y);
            resizeBorder.MatchResizeableNode(Size, Location);

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

            if (Surface.CanEdit && (resizeBorder.IsResizing || resizeBorder.IsMouseOverResizeBorder))
            {
                Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Style.Current.Foreground, 2f);
            }
        }
    }
}

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
        private Float2 _resizeWeight;
        private Float2 _startResizingSize;
        private Float2 _surfaceMouseLocation;
        private bool _isMouseOverResizeBorder;

        /// <summary>
        /// Indicates whether the node is currently being resized.
        /// </summary>
        protected bool _isResizing;

        /// <summary>
        /// Index of the Float2 value in the node values list to store node size.
        /// </summary>
        protected int _sizeValueIndex = -1;

        /// <summary>
        /// Minimum node size.
        /// </summary>
        protected Float2 _sizeMin = new Float2(240, 160);

        private Float2 SizeValue
        {
            get => (Float2)Values[_sizeValueIndex];
            set => SetValue(_sizeValueIndex, value, false);
        }

        /// <inheritdoc />
        public ResizableSurfaceNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
        : base(id, context, nodeArch, groupArch)
        {
        }

        /// <inheritdoc />
        public override bool CanSelect(ref Float2 location)
        {
            return base.CanSelect(ref location);// && !_resizeButtonRect.MakeOffsetted(Location).Contains(ref location);
        }

        /// <inheritdoc />
        public override void OnSurfaceLoaded(SurfaceNodeActions action)
        {
            // Reapply the curve node size
            var size = SizeValue;
            if (Surface != null && Surface.GridSnappingEnabled)
                size = Surface.SnapToGrid(size, true);
            Resize(size.X, size.Y);

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

            if (Surface.CanEdit && (_isResizing || _isMouseOverResizeBorder))
            {
                Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Style.Current.Foreground, 2f);
            }
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            if (_isResizing)
                EndResizing();

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            if (_isResizing)
                EndResizing();

            base.OnEndMouseCapture();
        }

        private bool UpdateIsOverResizeBorder(Float2 location)
        {
            const float ResizeBorderHalfWidth = 20f;
            const float ResizeOnAxisThreshold = 0.85f;

            var nodeRect = new Rectangle(Float2.Zero, Size);
            bool onBorder = nodeRect.MakeExpanded(ResizeBorderHalfWidth).Contains(location);
            var inNodeRect = nodeRect.MakeExpanded(-ResizeBorderHalfWidth);
            bool inNode = inNodeRect.Contains(location);

            Float2 rawResizeWeight = (location - nodeRect.Center) / (nodeRect.Size * 0.5f);
            _resizeWeight = new Float2(Mathf.Abs(rawResizeWeight.X) >= ResizeOnAxisThreshold ? Mathf.Sign(rawResizeWeight.X) : 0,
                                    Mathf.Abs(rawResizeWeight.Y) >= ResizeOnAxisThreshold ? Mathf.Sign(rawResizeWeight.Y) : 0);

            _isMouseOverResizeBorder = onBorder && !inNode;
            return onBorder && !inNode;
        }

        private CursorType ResizeWeightToCursorType()
        {
            if ((_resizeWeight.X == 1 && _resizeWeight.Y == 0) || (_resizeWeight.X == -1 && _resizeWeight.Y == 0))
                return CursorType.SizeWE;
            if ((_resizeWeight.X == 0 && _resizeWeight.Y == 1) || (_resizeWeight.X == 0 && _resizeWeight.Y == -1))
                return CursorType.SizeNS;
            if ((_resizeWeight.X == -1 && _resizeWeight.Y == -1) || (_resizeWeight.X == 1 && _resizeWeight.Y == 1))
                return CursorType.SizeNWSE;
            if ((_resizeWeight.X == 1 && _resizeWeight.Y == -1) || (_resizeWeight.X == -1 && _resizeWeight.Y == 1))
                return CursorType.SizeNESW;
            return CursorType.Default;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left && UpdateIsOverResizeBorder(location))
            {
                // Start resizing
                UpdateSurfaceMouseLocation();
                _isResizing = true;
                _startResizingSize = Size;
                StartMouseCapture();
                return true;
            }

            return false;
        }

        private Float2 GetControlDelta(Control control, ref Float2 start, ref Float2 end)
        {
            var pointOrigin = control.Parent ?? control;
            var startPos = pointOrigin.PointFromParent(this, start);
            var endPos = pointOrigin.PointFromParent(this, end);
            return endPos - startPos;
        }

        private void UpdateSurfaceMouseLocation()
        {
            _surfaceMouseLocation = Surface.PointFromScreen(Input.MouseScreenPosition);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isResizing)
            {
                var resizeAxisAbs = _resizeWeight.Absolute;
                var resizeAxisPos = Float2.Clamp(_resizeWeight, Float2.Zero, Float2.One);
                var resizeAxisNeg = Float2.Clamp(-_resizeWeight, Float2.Zero, Float2.One);

                var delta = PointToParent(Surface, location) - _surfaceMouseLocation;
                // TODO: scale/size snapping?
                delta *= resizeAxisAbs;

                var moveLocation = _surfaceMouseLocation + delta;

                // TODO: Do I need GetControlDelta?
                var uiControlDelta = GetControlDelta(this, ref _surfaceMouseLocation, ref moveLocation);
                var emptySize = CalculateNodeSize(0, 0);
                Location += uiControlDelta * resizeAxisNeg;
                Size += uiControlDelta * resizeAxisPos - uiControlDelta * resizeAxisNeg;
                Size = new Float2(Mathf.Max(Size.X, _sizeMin.X), Mathf.Max(Size.Y, _sizeMin.Y));
                SizeValue = Size - emptySize;
                SizeValue = new Float2(Mathf.Max(SizeValue.X, _sizeMin.X), Mathf.Max(SizeValue.Y, _sizeMin.Y));
                CalculateNodeSize(Size.X, Size.Y);
                UpdateSurfaceMouseLocation();
            }
            else if (UpdateIsOverResizeBorder(location))
            {
                Cursor = ResizeWeightToCursorType();
            }
            else
            {
                Cursor = CursorType.Default;
                base.OnMouseMove(location);
            }
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            Cursor = CursorType.Default;
            _isMouseOverResizeBorder = false;
            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isResizing)
            {
                Cursor = CursorType.Default;
                EndResizing();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        private void EndResizing()
        {
            EndMouseCapture();
            _isResizing = false;
            if (_startResizingSize != Size)
            {
                var emptySize = CalculateNodeSize(0, 0);
                SizeValue = Size - emptySize;
                Surface.MarkAsEdited(false);
            }
        }
    }
}

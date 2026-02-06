// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Visject Surface node control that cna be resized.
    /// </summary>
    /// <seealso cref="SurfaceNode" />
    [HideInEditor]
    public class ResizableSurfaceNode : SurfaceNode
    {
        private Float2 _startResizingSize;
        private Float2 _startResizingCornerOffset;

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

        /// <summary>
        /// Node resizing rectangle bounds.
        /// </summary>
        protected Rectangle _resizeButtonRect;

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
            return base.CanSelect(ref location) && !_resizeButtonRect.MakeOffsetted(Location).Contains(ref location);
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

            if (Surface.CanEdit)
            {
                var style = Style.Current;
                if (_isResizing)
                {
                    Render2D.FillRectangle(_resizeButtonRect, style.Selection);
                    Render2D.DrawRectangle(_resizeButtonRect, style.SelectionBorder);
                }
                Render2D.DrawSprite(style.Scale, _resizeButtonRect, _resizeButtonRect.Contains(_mousePosition) ? style.Foreground : style.ForegroundGrey);
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

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left && _resizeButtonRect.Contains(ref location) && Surface.CanEdit)
            {
                // Start resizing
                _isResizing = true;
                _startResizingSize = Size;
                _startResizingCornerOffset = Size - location;
                StartMouseCapture();
                Cursor = CursorType.SizeNWSE;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isResizing)
            {
                var emptySize = CalculateNodeSize(0, 0);
                var size = Float2.Max(location - emptySize + _startResizingCornerOffset, _sizeMin);
                Resize(size.X, size.Y);
            }
            else
            {
                base.OnMouseMove(location);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isResizing)
            {
                EndResizing();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        protected override void UpdateRectangles()
        {
            base.UpdateRectangles();

            const float buttonMargin = Constants.NodeCloseButtonMargin;
            const float buttonSize = Constants.NodeCloseButtonSize;
            _resizeButtonRect = new Rectangle(_closeButtonRect.Left, Height - buttonSize - buttonMargin - 4, buttonSize, buttonSize);
        }

        private void EndResizing()
        {
            Cursor = CursorType.Default;
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

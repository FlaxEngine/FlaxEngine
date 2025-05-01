// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.GUI
{
    /// <summary>
    /// Custom property name label that fires mouse events for label and supports dragging.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.GUI.PropertyNameLabel" />
    [HideInEditor]
    public class DraggablePropertyNameLabel : ClickablePropertyNameLabel
    {
        private bool _isLeftMouseButtonDown;

        /// <summary>
        /// Mouse drag action delegate.
        /// </summary>
        /// <param name="label">The label.</param>
        /// <returns>The drag data or null if not use drag.</returns>
        public delegate DragData DragDelegate(DraggablePropertyNameLabel label);

        /// <summary>
        /// The mouse starts the drag. Callbacks gets the drag data.
        /// </summary>
        public DragDelegate Drag;

        /// <inheritdoc />
        public DraggablePropertyNameLabel(string name)
        : base(name)
        {
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _isLeftMouseButtonDown = false;
            }

            base.OnMouseUp(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _isLeftMouseButtonDown = true;
            }

            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            if (_isLeftMouseButtonDown)
            {
                _isLeftMouseButtonDown = false;

                var data = Drag?.Invoke(this);
                if (data != null)
                {
                    DoDragDrop(data);
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Unlink event
            Drag = null;

            base.OnDestroy();
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The label that contains events for mouse interaction.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Label" />
    [HideInEditor]
    public class ClickableLabel : Label
    {
        private bool _leftClick;
        private bool _isRightDown;

        /// <summary>
        /// The double click event.
        /// </summary>
        public Action DoubleClick;

        /// <summary>
        /// The left mouse button click event.
        /// </summary>
        public Action LeftClick;

        /// <summary>
        /// The right mouse button click event.
        /// </summary>
        public Action RightClick;

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            DoubleClick?.Invoke();

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
                _leftClick = true;
            else if (button == MouseButton.Right)
                _isRightDown = true;

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _leftClick)
            {
                _leftClick = false;
                LeftClick?.Invoke();
            }
            else if (button == MouseButton.Right && _isRightDown)
            {
                _isRightDown = false;
                RightClick?.Invoke();
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _leftClick = false;
            _isRightDown = false;

            base.OnMouseLeave();
        }
    }
}

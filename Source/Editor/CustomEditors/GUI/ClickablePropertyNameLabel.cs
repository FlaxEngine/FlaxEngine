// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.GUI
{
    /// <summary>
    /// Custom property name label that fires mouse events for label.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.GUI.PropertyNameLabel" />
    [HideInEditor]
    public class ClickablePropertyNameLabel : PropertyNameLabel
    {
        /// <summary>
        /// Mouse action delegate.
        /// </summary>
        /// <param name="label">The label.</param>
        /// <param name="location">The mouse location.</param>
        public delegate void MouseDelegate(ClickablePropertyNameLabel label, Float2 location);

        /// <summary>
        /// The mouse left button clicks on the label.
        /// </summary>
        public MouseDelegate MouseLeftClick;

        /// <summary>
        /// The mouse right button clicks on the label.
        /// </summary>
        public MouseDelegate MouseRightClick;

        /// <summary>
        /// The mouse left button double clicks on the label.
        /// </summary>
        public MouseDelegate MouseLeftDoubleClick;

        /// <summary>
        /// The mouse right button double clicks on the label.
        /// </summary>
        public MouseDelegate MouseRightDoubleClick;

        /// <inheritdoc />
        public ClickablePropertyNameLabel(string name)
        : base(name)
        {
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // Fire events
            if (button == MouseButton.Left)
            {
                if (MouseLeftClick != null)
                {
                    MouseLeftClick.Invoke(this, location);
                    return true;
                }
            }
            else if (button == MouseButton.Right)
            {
                if (MouseRightClick != null)
                {
                    MouseRightClick.Invoke(this, location);
                    return true;
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            // Fire events
            if (button == MouseButton.Left)
            {
                if (MouseLeftDoubleClick != null)
                {
                    MouseLeftDoubleClick.Invoke(this, location);
                    return true;
                }
            }
            else if (button == MouseButton.Right)
            {
                if (MouseRightDoubleClick != null)
                {
                    MouseRightDoubleClick.Invoke(this, location);
                    return true;
                }
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Unlink events
            MouseLeftClick = null;
            MouseRightClick = null;
            MouseLeftDoubleClick = null;
            MouseRightDoubleClick = null;

            base.OnDestroy();
        }
    }
}

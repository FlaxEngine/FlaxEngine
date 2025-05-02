// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.ContextMenu
{
    /// <summary>
    /// Context Menu child control.
    /// </summary>
    /// <seealso cref="ContainerControl" />
    [HideInEditor]
    public abstract class ContextMenuItem : ContainerControl
    {
        /// <summary>
        /// Gets the parent context menu.
        /// </summary>
        public ContextMenu ParentContextMenu { get; }

        /// <summary>
        /// Gets the minimum width of this item.
        /// </summary>
        public virtual float MinimumWidth
        {
            get
            {
                float width = 0;

                for (int i = 0; i < _children.Count; i++)
                {
                    var c = _children[i];
                    if (c.Visible)
                    {
                        width = Mathf.Max(width, c.Right + 4.0f);
                    }
                }

                return width;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextMenuItem"/> class.
        /// </summary>
        /// <param name="parent">The parent context menu.</param>
        /// <param name="width">The initial width.</param>
        /// <param name="height">The initial height.</param>
        protected ContextMenuItem(ContextMenu parent, float width, float height)
        : base(0, 0, width, height)
        {
            AutoFocus = false;
            ParentContextMenu = parent;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            ParentContextMenu?.HideChild();

            base.OnMouseEnter(location);
        }
    }
}

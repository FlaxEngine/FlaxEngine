// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Base class for <see cref="SurfaceNode"/> element controls. Implements <see cref="ISurfaceNodeElement"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    /// <seealso cref="FlaxEditor.Surface.ISurfaceNodeElement" />
    [HideInEditor]
    public abstract class SurfaceNodeElementControl : Control, ISurfaceNodeElement
    {
        /// <inheritdoc />
        public SurfaceNode ParentNode { get; }

        /// <inheritdoc />
        public NodeElementArchetype Archetype { get; }

        /// <summary>
        /// Gets the surface.
        /// </summary>
        public VisjectSurface Surface => ParentNode.Surface;

        /// <summary>
        /// Initializes a new instance of the <see cref="SurfaceNodeElementControl"/> class.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="archetype">The element archetype.</param>
        /// <param name="location">The location.</param>
        /// <param name="size">The size.</param>
        /// <param name="autoFocus">if set to <c>true</c> can focus this control.</param>
        protected SurfaceNodeElementControl(SurfaceNode parentNode, NodeElementArchetype archetype, Float2 location, Float2 size, bool autoFocus)
        : base(location, size)
        {
            AutoFocus = autoFocus;
            TooltipText = archetype.Tooltip;
            ParentNode = parentNode;
            Archetype = archetype;
        }

        /// <summary>
        /// Called when surface can edit state gets changed. Can be used to enable/disable UI elements that are surface data editing.
        /// </summary>
        /// <param name="canEdit">True if can edit surface data, otherwise false.</param>
        public virtual void OnSurfaceCanEditChanged(bool canEdit)
        {
        }
    }
}

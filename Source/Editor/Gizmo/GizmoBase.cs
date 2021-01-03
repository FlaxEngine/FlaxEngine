// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Base class for all Gizmo controls that can be attached to the <see cref="IGizmoOwner"/>.
    /// </summary>
    [HideInEditor]
    public abstract class GizmoBase
    {
        /// <summary>
        /// Gets the gizmo owner.
        /// </summary>
        public IGizmoOwner Owner { get; }

        /// <summary>
        /// Gets a value indicating whether this gizmo is active.
        /// </summary>
        public bool IsActive => Owner.Gizmos.Active == this;

        /// <summary>
        /// Initializes a new instance of the <see cref="GizmoBase"/> class.
        /// </summary>
        /// <param name="owner">The gizmos owner.</param>
        protected GizmoBase(IGizmoOwner owner)
        {
            // Link
            Owner = owner;
            Owner.Gizmos.Add(this);
        }

        /// <summary>
        /// Called when selected objects collection gets changed.
        /// </summary>
        /// <param name="newSelection">The new selection pool.</param>
        public virtual void OnSelectionChanged(List<SceneGraphNode> newSelection)
        {
        }

        /// <summary>
        /// Called when gizmo gets activated.
        /// </summary>
        public virtual void OnActivated()
        {
        }

        /// <summary>
        /// Called when gizmo gets deactivated.
        /// </summary>
        public virtual void OnDeactivated()
        {
        }

        /// <summary>
        /// Updates the gizmo logic (called even if not active).
        /// </summary>
        /// <param name="dt">Update delta time (in seconds).</param>
        public virtual void Update(float dt)
        {
        }

        /// <summary>
        /// Performs scene object picking. Called by the viewport on left mouse button released.
        /// </summary>
        public virtual void Pick()
        {
        }

        /// <summary>
        /// Draws the gizmo.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        public virtual void Draw(ref RenderContext renderContext)
        {
        }
    }
}

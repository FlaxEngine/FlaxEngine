// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Viewport.Modes
{
    /// <summary>
    /// The editor gizmo editor mode that does nothing. Can be used to hide other gizmos when using a specific editor tool.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    public class NoGizmoMode : EditorGizmoMode
    {
        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = null;
        }
    }
}

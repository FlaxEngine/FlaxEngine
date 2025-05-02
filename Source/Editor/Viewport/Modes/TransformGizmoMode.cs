// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Viewport.Modes
{
    /// <summary>
    /// The default editor mode that uses <see cref="FlaxEditor.Gizmo.TransformGizmoBase"/> for objects transforming.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    public class TransformGizmoMode : EditorGizmoMode
    {
        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = ((MainEditorGizmoViewport)Owner).TransformGizmo;
        }
    }
}

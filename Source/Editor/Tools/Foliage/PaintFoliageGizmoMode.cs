// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Foliage painting tool mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    public class PaintFoliageGizmoMode : EditorGizmoMode
    {
        /// <summary>
        /// The foliage painting gizmo.
        /// </summary>
        public PaintFoliageGizmo Gizmo;

        /// <summary>
        /// Gets the current brush.
        /// </summary>
        public readonly Brush CurrentBrush = new Brush();

        /// <summary>
        /// The last valid cursor position of the brush (in world space).
        /// </summary>
        public Vector3 CursorPosition { get; private set; }

        /// <summary>
        /// The last valid cursor hit point normal vector of the brush (in world space).
        /// </summary>
        public Vector3 CursorNormal { get; private set; }

        /// <summary>
        /// Flag used to indicate whenever last cursor position of the brush is valid.
        /// </summary>
        public bool HasValidHit { get; private set; }

        /// <summary>
        /// Gets the selected foliage actor (see <see cref="Modules.SceneEditingModule"/>).
        /// </summary>
        public FlaxEngine.Foliage SelectedFoliage
        {
            get
            {
                var sceneEditing = Editor.Instance.SceneEditing;
                var foliageNode = sceneEditing.SelectionCount == 1 ? sceneEditing.Selection[0] as FoliageNode : null;
                return (FlaxEngine.Foliage)foliageNode?.Actor;
            }
        }

        /// <summary>
        /// Gets the world bounds of the brush located at the current cursor position (defined by <see cref="CursorPosition"/>). Valid only if <see cref="HasValidHit"/> is set to true.
        /// </summary>
        public BoundingBox CursorBrushBounds
        {
            get
            {
                float brushSizeHalf = CurrentBrush.Size * 0.5f;
                Vector3 center = CursorPosition;

                BoundingBox box;
                box.Minimum = new Vector3(center.X - brushSizeHalf, center.Y - brushSizeHalf - brushSizeHalf, center.Z - brushSizeHalf);
                box.Maximum = new Vector3(center.X + brushSizeHalf, center.Y + brushSizeHalf + brushSizeHalf, center.Z + brushSizeHalf);
                return box;
            }
        }

        /// <inheritdoc />
        public override void Init(IGizmoOwner owner)
        {
            base.Init(owner);

            Gizmo = new PaintFoliageGizmo(owner, this);
        }

        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = Gizmo;
            ClearCursor();
        }

        /// <summary>
        /// Clears the cursor location information cached within the gizmo mode.
        /// </summary>
        public void ClearCursor()
        {
            HasValidHit = false;
        }

        /// <summary>
        /// Sets the cursor location in the world space. Updates the brush location and cached affected chunks.
        /// </summary>
        /// <param name="hitPosition">The cursor hit location on the selected foliage.</param>
        /// <param name="hitNormal">The cursor hit location normal vector fot he surface.</param>
        public void SetCursor(ref Vector3 hitPosition, ref Vector3 hitNormal)
        {
            HasValidHit = true;
            CursorPosition = hitPosition;
            CursorNormal = hitNormal;
        }
    }
}

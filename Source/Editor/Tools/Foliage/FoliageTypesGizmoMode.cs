// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Modes;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Foliage types editing mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    public class FoliageTypesGizmoMode : EditorGizmoMode
    {
        /// <summary>
        /// The foliage types gizmo.
        /// </summary>
        public FoliageTypesGizmo Gizmo;

        /// <summary>
        /// The foliage type editing selection outline.
        /// </summary>
        public FoliageTypesOutline SelectionOutline;

        /// <summary>
        /// The selected foliage type index.
        /// </summary>
        public int SelectedTypeIndex
        {
            get => Editor.Instance.Windows.ToolboxWin.Foliage.SelectedFoliageTypeIndex;
            set => Editor.Instance.Windows.ToolboxWin.Foliage.SelectedFoliageTypeIndex = value;
        }

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

        /// <inheritdoc />
        public override void Init(IGizmoOwner owner)
        {
            base.Init(owner);

            Gizmo = new FoliageTypesGizmo(owner, this);
        }

        /// <inheritdoc />
        public override void Dispose()
        {
            FlaxEngine.Object.Destroy(ref SelectionOutline);

            base.Dispose();
        }

        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = Gizmo;
            if (SelectionOutline == null)
            {
                SelectionOutline = FlaxEngine.Object.New<FoliageTypesOutline>();
                SelectionOutline.GizmoMode = this;
            }
            ((MainEditorGizmoViewport)Owner).OverrideSelectionOutline(SelectionOutline);
        }

        /// <inheritdoc />
        public override void OnDeactivated()
        {
            ((MainEditorGizmoViewport)Owner).OverrideSelectionOutline(null);

            base.OnDeactivated();
        }
    }
}

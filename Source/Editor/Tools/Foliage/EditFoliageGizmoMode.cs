// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Modes;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Foliage instances editing mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    public class EditFoliageGizmoMode : EditorGizmoMode
    {
        private int _selectedInstanceIndex = -1;

        /// <summary>
        /// The foliage painting gizmo.
        /// </summary>
        public EditFoliageGizmo Gizmo;

        /// <summary>
        /// The foliage editing selection outline.
        /// </summary>
        public EditFoliageSelectionOutline SelectionOutline;

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
        /// The selected foliage instance index.
        /// </summary>
        public int SelectedInstanceIndex
        {
            get => _selectedInstanceIndex;
            set
            {
                if (_selectedInstanceIndex == value)
                    return;

                _selectedInstanceIndex = value;
                SelectedInstanceIndexChanged?.Invoke();
            }
        }

        /// <summary>
        /// Occurs when selected instance index gets changed.
        /// </summary>
        public event Action SelectedInstanceIndexChanged;

        /// <inheritdoc />
        public override void Init(IGizmoOwner owner)
        {
            base.Init(owner);

            Gizmo = new EditFoliageGizmo(owner, this);
            SelectionOutline = FlaxEngine.Object.New<EditFoliageSelectionOutline>();
            SelectionOutline.GizmoMode = this;
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
            ((MainEditorGizmoViewport)Owner).OverrideSelectionOutline(SelectionOutline);
            SelectedInstanceIndex = -1;
        }

        /// <inheritdoc />
        public override void OnDeactivated()
        {
            ((MainEditorGizmoViewport)Owner).OverrideSelectionOutline(null);

            base.OnDeactivated();
        }
    }
}

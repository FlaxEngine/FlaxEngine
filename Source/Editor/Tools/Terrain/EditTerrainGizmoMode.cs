// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Gizmo;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Terrain management and editing tool. 
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    [HideInEditor]
    public class EditTerrainGizmoMode : EditorGizmoMode
    {
        /// <summary>
        /// The terrain properties editing modes.
        /// </summary>
        public enum Modes
        {
            /// <summary>
            /// Terrain chunks editing mode.
            /// </summary>
            Edit,

            /// <summary>
            /// Terrain patches adding mode.
            /// </summary>
            Add,

            /// <summary>
            /// Terrain patches removing mode.
            /// </summary>
            Remove,

            /// <summary>
            /// Terrain exporting mode.
            /// </summary>
            Export,
        }

        private Modes _mode;

        /// <summary>
        /// The terrain editing gizmo.
        /// </summary>
        public EditTerrainGizmo Gizmo;

        /// <summary>
        /// The patch coordinates of the last picked patch.
        /// </summary>
        public Int2 SelectedPatchCoord { get; private set; }

        /// <summary>
        /// The chunk coordinates (relative to the patch) of the last picked chunk.
        /// </summary>
        public Int2 SelectedChunkCoord { get; private set; }

        /// <summary>
        /// Occurs when mode gets changed.
        /// </summary>
        public event Action ModeChanged;

        /// <summary>
        /// The active edit mode.
        /// </summary>
        public Modes EditMode
        {
            get => _mode;
            set
            {
                if (_mode != value)
                {
                    _mode = value;
                    ModeChanged?.Invoke();
                }
            }
        }

        /// <summary>
        /// Occurs when selected patch or/and chunk coord gets changed (after picking by user).
        /// </summary>
        public event Action SelectedChunkCoordChanged;

        /// <inheritdoc />
        public override void Init(IGizmoOwner owner)
        {
            base.Init(owner);

            EditMode = Modes.Edit;
            Gizmo = new EditTerrainGizmo(owner, this);
        }

        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = Gizmo;
        }

        /// <summary>
        /// Sets the selected chunk coordinates.
        /// </summary>
        /// <param name="patchCoord">The patch coord.</param>
        /// <param name="chunkCoord">The chunk coord.</param>
        public void SetSelectedChunk(ref Int2 patchCoord, ref Int2 chunkCoord)
        {
            if (SelectedPatchCoord != patchCoord || SelectedChunkCoord != chunkCoord)
            {
                SelectedPatchCoord = patchCoord;
                SelectedChunkCoord = chunkCoord;
                OnSelectedTerrainChunkChanged();
            }
        }

        private void OnSelectedTerrainChunkChanged()
        {
            SelectedChunkCoordChanged?.Invoke();
        }
    }
}

// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Tools.Terrain.Brushes;
using FlaxEditor.Tools.Terrain.Sculpt;
using FlaxEditor.Tools.Terrain.Undo;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Terrain carving tool mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    [HideInEditor]
    public class SculptTerrainGizmoMode : EditorGizmoMode
    {
        private IntPtr _cachedHeightmapData;
        private int _cachedHeightmapDataSize;
        private EditTerrainMapAction _activeAction;

        /// <summary>
        /// The terrain carving gizmo.
        /// </summary>
        public SculptTerrainGizmo Gizmo;

        /// <summary>
        /// The tool modes.
        /// </summary>
        public enum ModeTypes
        {
            /// <summary>
            /// The sculpt mode.
            /// </summary>
            [Tooltip("Sculpt tool mode. Edits terrain heightmap by moving area affected by brush up or down.")]
            Sculpt,

            /// <summary>
            /// The smooth mode.
            /// </summary>
            [Tooltip("Sculpt tool mode that smooths the terrain heightmap area affected by brush.")]
            Smooth,

            /// <summary>
            /// The flatten mode.
            /// </summary>
            [Tooltip("Sculpt tool mode that flattens the terrain heightmap area affected by brush to the target value.")]
            Flatten,

            /// <summary>
            /// The noise mode.
            /// </summary>
            [Tooltip("Sculpt tool mode that applies the noise to the terrain heightmap area affected by brush.")]
            Noise,

            /// <summary>
            /// The holes mode.
            /// </summary>
            [Tooltip("Terrain holes creating tool mode edits terrain holes mask by changing area affected by brush.")]
            Holes,
        }

        /// <summary>
        /// The brush types.
        /// </summary>
        public enum BrushTypes
        {
            /// <summary>
            /// The circle brush.
            /// </summary>
            CircleBrush,
        }

        private readonly Mode[] _modes =
        {
            new SculptMode(),
            new SmoothMode(),
            new FlattenMode(),
            new NoiseMode(),
            new HolesMode(),
        };

        private readonly Brush[] _brushes =
        {
            new CircleBrush(),
        };

        private ModeTypes _modeType = ModeTypes.Sculpt;
        private BrushTypes _brushType = BrushTypes.CircleBrush;

        /// <summary>
        /// Occurs when tool mode gets changed.
        /// </summary>
        public event Action ToolModeChanged;

        /// <summary>
        /// Gets the current tool mode (enum).
        /// </summary>
        public ModeTypes ToolModeType
        {
            get => _modeType;
            set
            {
                if (_modeType != value)
                {
                    if (_activeAction != null)
                        throw new InvalidOperationException("Cannot change sculpt tool mode during terrain editing.");

                    _modeType = value;
                    ToolModeChanged?.Invoke();
                }
            }
        }

        /// <summary>
        /// Gets the current tool mode.
        /// </summary>
        public Mode CurrentMode => _modes[(int)_modeType];

        /// <summary>
        /// Gets the sculpt mode instance.
        /// </summary>
        public SculptMode SculptMode => _modes[(int)ModeTypes.Sculpt] as SculptMode;

        /// <summary>
        /// Gets the smooth mode instance.
        /// </summary>
        public SmoothMode SmoothMode => _modes[(int)ModeTypes.Smooth] as SmoothMode;

        /// <summary>
        /// Occurs when tool brush gets changed.
        /// </summary>
        public event Action ToolBrushChanged;

        /// <summary>
        /// Gets the current tool brush (enum).
        /// </summary>
        public BrushTypes ToolBrushType
        {
            get => _brushType;
            set
            {
                if (_brushType != value)
                {
                    if (_activeAction != null)
                        throw new InvalidOperationException("Cannot change sculpt tool brush type during terrain editing.");

                    _brushType = value;
                    ToolBrushChanged?.Invoke();
                }
            }
        }

        /// <summary>
        /// Gets the current brush.
        /// </summary>
        public Brush CurrentBrush => _brushes[(int)_brushType];

        /// <summary>
        /// Gets the circle brush instance.
        /// </summary>
        public CircleBrush CircleBrush => _brushes[(int)BrushTypes.CircleBrush] as CircleBrush;

        /// <summary>
        /// The last valid cursor position of the brush (in world space).
        /// </summary>
        public Vector3 CursorPosition { get; private set; }

        /// <summary>
        /// Flag used to indicate whenever last cursor position of the brush is valid.
        /// </summary>
        public bool HasValidHit { get; private set; }

        /// <summary>
        /// Describes the terrain patch link.
        /// </summary>
        public struct PatchLocation
        {
            /// <summary>
            /// The patch coordinates.
            /// </summary>
            public Int2 PatchCoord;
        }

        /// <summary>
        /// The selected terrain patches collection that are under cursor (affected by the brush).
        /// </summary>
        public readonly List<PatchLocation> PatchesUnderCursor = new List<PatchLocation>();

        /// <summary>
        /// Describes the terrain chunk link.
        /// </summary>
        public struct ChunkLocation
        {
            /// <summary>
            /// The patch coordinates.
            /// </summary>
            public Int2 PatchCoord;

            /// <summary>
            /// The chunk coordinates.
            /// </summary>
            public Int2 ChunkCoord;
        }

        /// <summary>
        /// The selected terrain chunk collection that are under cursor (affected by the brush).
        /// </summary>
        public readonly List<ChunkLocation> ChunksUnderCursor = new List<ChunkLocation>();

        /// <summary>
        /// Gets the selected terrain actor (see <see cref="Modules.SceneEditingModule"/>).
        /// </summary>
        public FlaxEngine.Terrain SelectedTerrain
        {
            get
            {
                var sceneEditing = Editor.Instance.SceneEditing;
                var terrainNode = sceneEditing.SelectionCount == 1 ? sceneEditing.Selection[0] as TerrainNode : null;
                return (FlaxEngine.Terrain)terrainNode?.Actor;
            }
        }

        /// <summary>
        /// Gets the world bounds of the brush located at the current cursor position (defined by <see cref="CursorPosition"/>). Valid only if <see cref="HasValidHit"/> is set to true.
        /// </summary>
        public BoundingBox CursorBrushBounds
        {
            get
            {
                const float brushExtentY = 10000.0f;
                float brushSizeHalf = CurrentBrush.Size * 0.5f;
                Vector3 center = CursorPosition;

                BoundingBox box;
                box.Minimum = new Vector3(center.X - brushSizeHalf, center.Y - brushSizeHalf - brushExtentY, center.Z - brushSizeHalf);
                box.Maximum = new Vector3(center.X + brushSizeHalf, center.Y + brushSizeHalf + brushExtentY, center.Z + brushSizeHalf);
                return box;
            }
        }

        /// <summary>
        /// Gets the heightmap temporary scratch memory buffer used to modify terrain samples. Allocated memory is unmanaged by GC.
        /// </summary>
        /// <param name="size">The minimum buffer size (in bytes).</param>
        /// <returns>The allocated memory using <see cref="Marshal"/> interface.</returns>
        public IntPtr GetHeightmapTempBuffer(int size)
        {
            if (_cachedHeightmapDataSize < size)
            {
                if (_cachedHeightmapData != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(_cachedHeightmapData);
                }
                _cachedHeightmapData = Marshal.AllocHGlobal(size);
                _cachedHeightmapDataSize = size;
            }

            return _cachedHeightmapData;
        }

        /// <summary>
        /// Gets the current edit terrain undo system action. Use it to record the data for the undo restoring after terrain editing.
        /// </summary>
        internal EditTerrainMapAction CurrentEditUndoAction => _activeAction;

        /// <inheritdoc />
        public override void Init(MainEditorGizmoViewport viewport)
        {
            base.Init(viewport);

            Gizmo = new SculptTerrainGizmo(viewport, this);
            Gizmo.PaintStarted += OnPaintStarted;
            Gizmo.PaintEnded += OnPaintEnded;
        }

        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            Viewport.Gizmos.Active = Gizmo;
            ClearCursor();
        }

        /// <inheritdoc />
        public override void OnDeactivated()
        {
            base.OnDeactivated();

            // Free temporary memory buffer
            if (_cachedHeightmapData != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(_cachedHeightmapData);
                _cachedHeightmapData = IntPtr.Zero;
                _cachedHeightmapDataSize = 0;
            }
        }

        /// <summary>
        /// Clears the cursor location information cached within the gizmo mode.
        /// </summary>
        public void ClearCursor()
        {
            HasValidHit = false;
            PatchesUnderCursor.Clear();
            ChunksUnderCursor.Clear();
        }

        /// <summary>
        /// Sets the cursor location in the world space. Updates the brush location and cached affected chunks.
        /// </summary>
        /// <param name="hitPosition">The cursor hit location on the selected terrain.</param>
        public void SetCursor(ref Vector3 hitPosition)
        {
            HasValidHit = true;
            CursorPosition = hitPosition;
            PatchesUnderCursor.Clear();
            ChunksUnderCursor.Clear();

            // Find patches and chunks affected by the brush
            var terrain = SelectedTerrain;
            if (terrain == null)
                throw new InvalidOperationException("Cannot set cursor then no terrain is selected.");
            var brushBounds = CursorBrushBounds;
            var patchesCount = terrain.PatchesCount;
            for (int patchIndex = 0; patchIndex < patchesCount; patchIndex++)
            {
                terrain.GetPatchBounds(patchIndex, out BoundingBox tmp);
                if (!tmp.Intersects(ref brushBounds))
                    continue;

                terrain.GetPatchCoord(patchIndex, out var patchCoord);
                PatchesUnderCursor.Add(new PatchLocation { PatchCoord = patchCoord });

                for (int chunkIndex = 0; chunkIndex < FlaxEngine.Terrain.PatchChunksCount; chunkIndex++)
                {
                    terrain.GetChunkBounds(patchIndex, chunkIndex, out tmp);
                    if (!tmp.Intersects(ref brushBounds))
                        continue;

                    var chunkCoord = new Int2(chunkIndex % FlaxEngine.Terrain.PatchEdgeChunksCount, chunkIndex / FlaxEngine.Terrain.PatchEdgeChunksCount);
                    ChunksUnderCursor.Add(new ChunkLocation { PatchCoord = patchCoord, ChunkCoord = chunkCoord });
                }
            }
        }

        private void OnPaintStarted()
        {
            if (_activeAction != null)
                throw new InvalidOperationException("Terrain paint start/end resynchronization.");

            var terrain = SelectedTerrain;
            if (CurrentMode.EditHoles)
                _activeAction = new EditTerrainHolesMapAction(terrain);
            else
                _activeAction = new EditTerrainHeightMapAction(terrain);
        }

        private void OnPaintEnded()
        {
            if (_activeAction != null)
            {
                if (_activeAction.HasAnyModification)
                {
                    _activeAction.OnEditingEnd();
                    Editor.Instance.Undo.AddAction(_activeAction);
                }
                _activeAction = null;
            }
        }
    }
}

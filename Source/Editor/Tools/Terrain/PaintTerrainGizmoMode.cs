// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Tools.Terrain.Brushes;
using FlaxEditor.Tools.Terrain.Paint;
using FlaxEditor.Tools.Terrain.Undo;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Terrain painting tool mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Modes.EditorGizmoMode" />
    [HideInEditor]
    public class PaintTerrainGizmoMode : EditorGizmoMode
    {
        private struct SplatmapData
        {
            public IntPtr DataPtr;
            public int Size;

            public void EnsureCapacity(int size)
            {
                if (Size < size)
                {
                    if (DataPtr != IntPtr.Zero)
                        Marshal.FreeHGlobal(DataPtr);
                    DataPtr = Marshal.AllocHGlobal(size);
                    Utils.MemoryClear(DataPtr, (ulong)size);
                    Size = size;
                }
            }

            public void Free()
            {
                if (DataPtr == IntPtr.Zero)
                    return;
                Marshal.FreeHGlobal(DataPtr);
                DataPtr = IntPtr.Zero;
                Size = 0;
            }
        }

        private EditTerrainMapAction _activeAction;
        private SplatmapData[] _cachedSplatmapData = new SplatmapData[2];

        /// <summary>
        /// The terrain painting gizmo.
        /// </summary>
        public PaintTerrainGizmo Gizmo;

        /// <summary>
        /// The tool modes.
        /// </summary>
        public enum ModeTypes
        {
            /// <summary>
            /// The single layer mode.
            /// </summary>
            SingleLayer,
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
            new SingleLayerMode(),
        };

        private readonly Brush[] _brushes =
        {
            new CircleBrush(),
        };

        private ModeTypes _modeType = ModeTypes.SingleLayer;
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
                        throw new InvalidOperationException("Cannot change paint tool mode during terrain editing.");

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
        /// Gets the single layer mode instance.
        /// </summary>
        public SingleLayerMode SingleLayerMode => _modes[(int)ModeTypes.SingleLayer] as SingleLayerMode;

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
        /// Gets the splatmap temporary scratch memory buffer used to modify terrain samples. Allocated memory is unmanaged by GC.
        /// </summary>
        /// <param name="size">The minimum buffer size (in bytes).</param>
        /// <param name="splatmapIndex">The splatmap index for which to return/create the temp buffer.</param>
        /// <returns>The allocated memory using <see cref="Marshal"/> interface.</returns>
        public IntPtr GetSplatmapTempBuffer(int size, int splatmapIndex)
        {
            ref var splatmapData = ref _cachedSplatmapData[splatmapIndex];
            splatmapData.EnsureCapacity(size);
            return splatmapData.DataPtr;
        }

        /// <summary>
        /// Gets the current edit terrain undo system action. Use it to record the data for the undo restoring after terrain editing.
        /// </summary>
        internal EditTerrainMapAction CurrentEditUndoAction => _activeAction;

        /// <inheritdoc />
        public override void Init(IGizmoOwner owner)
        {
            base.Init(owner);

            Gizmo = new PaintTerrainGizmo(owner, this);
            Gizmo.PaintStarted += OnPaintStarted;
            Gizmo.PaintEnded += OnPaintEnded;
        }

        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = Gizmo;
            ClearCursor();
        }

        /// <inheritdoc />
        public override void OnDeactivated()
        {
            base.OnDeactivated();

            // Free temporary memory buffer
            foreach (ref var splatmapData in _cachedSplatmapData.AsSpan())
                splatmapData.Free();
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
                PatchesUnderCursor.Add(new PatchLocation() { PatchCoord = patchCoord });

                for (int chunkIndex = 0; chunkIndex < FlaxEngine.Terrain.PatchChunksCount; chunkIndex++)
                {
                    terrain.GetChunkBounds(patchIndex, chunkIndex, out tmp);
                    if (!tmp.Intersects(ref brushBounds))
                        continue;

                    var chunkCoord = new Int2(chunkIndex % FlaxEngine.Terrain.PatchEdgeChunksCount, chunkIndex / FlaxEngine.Terrain.PatchEdgeChunksCount);
                    ChunksUnderCursor.Add(new ChunkLocation() { PatchCoord = patchCoord, ChunkCoord = chunkCoord });
                }
            }
        }

        private void OnPaintStarted()
        {
            if (_activeAction != null)
                throw new InvalidOperationException("Terrain paint start/end resynchronization.");

            var terrain = SelectedTerrain;
            _activeAction = new EditTerrainSplatMapAction(terrain);
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

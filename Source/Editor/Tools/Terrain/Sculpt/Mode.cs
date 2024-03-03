// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Tools.Terrain.Brushes;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Sculpt
{
    /// <summary>
    /// The base class for terran sculpt tool modes.
    /// </summary>
    [HideInEditor]
    public abstract class Mode
    {
        /// <summary>
        /// The options container for the terrain editing apply.
        /// </summary>
        public struct Options
        {
            /// <summary>
            /// If checked, modification apply method should be inverted.
            /// </summary>
            public bool Invert;

            /// <summary>
            /// The master strength parameter to apply when editing the terrain.
            /// </summary>
            public float Strength;

            /// <summary>
            /// The delta time (in seconds) for the terrain modification apply. Used to scale the strength. Adjusted to handle low-FPS.
            /// </summary>
            public float DeltaTime;
        }

        /// <summary>
        /// The tool strength (normalized to range 0-1). Defines the intensity of the sculpt operation to make it stronger or more subtle.
        /// </summary>
        [EditorOrder(0), Limit(0, 6, 0.01f), Tooltip("The tool strength (normalized to range 0-1). Defines the intensity of the sculpt operation to make it stronger or more subtle.")]
        public float Strength = 1.2f;

        /// <summary>
        /// Gets a value indicating whether this mode supports negative apply for terrain modification.
        /// </summary>
        public virtual bool SupportsNegativeApply => false;

        /// <summary>
        /// Gets a value indicating whether this mode modifies the terrain holes mask rather than heightmap.
        /// </summary>
        public virtual bool EditHoles => false;

        /// <summary>
        /// Gets all patches that will be affected by the brush
        /// </summary>
        /// <param name="brush">The brush.</param>
        /// <param name="options">The options.</param>
        /// <param name="gizmo">The gizmo.</param>
        /// <param name="terrain">The terrain.</param>
        public virtual unsafe List<ApplyParams> GetAffectedPatches(Brush brush, ref Options options, SculptTerrainGizmoMode gizmo, FlaxEngine.Terrain terrain)
        {
            List<ApplyParams> affectedPatches = new();

            // Combine final apply strength
            float strength = Strength * options.Strength * options.DeltaTime;
            if (strength <= 0.0f)
                return affectedPatches;
            if (options.Invert && SupportsNegativeApply)
                strength *= -1;

            // Prepare
            var chunkSize = terrain.ChunkSize;
            var heightmapSize = chunkSize * FlaxEngine.Terrain.PatchEdgeChunksCount + 1;
            var heightmapLength = heightmapSize * heightmapSize;
            var patchSize = chunkSize * FlaxEngine.Terrain.UnitsPerVertex * FlaxEngine.Terrain.PatchEdgeChunksCount;
            var tempBuffer = (float*)gizmo.GetHeightmapTempBuffer(heightmapLength * sizeof(float)).ToPointer();
            var unitsPerVertexInv = 1.0f / FlaxEngine.Terrain.UnitsPerVertex;

            // Get brush bounds in terrain local space
            var brushBounds = gizmo.CursorBrushBounds;
            terrain.GetLocalToWorldMatrix(out var terrainWorld);
            terrain.GetWorldToLocalMatrix(out var terrainInvWorld);
            BoundingBox.Transform(ref brushBounds, ref terrainInvWorld, out var brushBoundsLocal);

            // TODO: try caching brush weights before apply to reduce complexity and batch brush sampling

            // Process all the patches under the cursor
            for (int patchIndex = 0; patchIndex < gizmo.PatchesUnderCursor.Count; patchIndex++)
            {
                var patch = gizmo.PatchesUnderCursor[patchIndex];
                var patchPositionLocal = new Vector3(patch.PatchCoord.X * patchSize, 0, patch.PatchCoord.Y * patchSize);

                // Transform brush bounds from local terrain space into local patch vertex space
                var brushBoundsPatchLocalMin = (brushBoundsLocal.Minimum - patchPositionLocal) * unitsPerVertexInv;
                var brushBoundsPatchLocalMax = (brushBoundsLocal.Maximum - patchPositionLocal) * unitsPerVertexInv;

                // Calculate patch heightmap area to modify by brush
                var brushPatchMin = new Int2((int)Math.Floor(brushBoundsPatchLocalMin.X), (int)Math.Floor(brushBoundsPatchLocalMin.Z));
                var brushPatchMax = new Int2((int)Math.Ceiling(brushBoundsPatchLocalMax.X), (int)Math.Ceiling(brushBoundsPatchLocalMax.Z));
                var modifiedOffset = brushPatchMin;
                var modifiedSize = brushPatchMax - brushPatchMin;

                // Expand the modification area by one vertex in each direction to ensure normal vectors are updated for edge cases, also clamp to prevent overflows
                if (modifiedOffset.X < 0)
                {
                    modifiedSize.X += modifiedOffset.X;
                    modifiedOffset.X = 0;
                }
                if (modifiedOffset.Y < 0)
                {
                    modifiedSize.Y += modifiedOffset.Y;
                    modifiedOffset.Y = 0;
                }
                modifiedSize.X = Mathf.Min(modifiedSize.X + 2, heightmapSize - modifiedOffset.X);
                modifiedSize.Y = Mathf.Min(modifiedSize.Y + 2, heightmapSize - modifiedOffset.Y);

                // Skip patch won't be modified at all
                if (modifiedSize.X <= 0 || modifiedSize.Y <= 0)
                    continue;

                // Get the patch data (cached internally by the c++ core in editor)
                float* sourceHeights = EditHoles ? null : TerrainTools.GetHeightmapData(terrain, ref patch.PatchCoord);
                byte* sourceHoles = EditHoles ? TerrainTools.GetHolesMaskData(terrain, ref patch.PatchCoord) : null;
                if (sourceHeights == null && sourceHoles == null)
                    throw new Exception("Cannot modify terrain. Loading heightmap failed. See log for more info.");

                ApplyParams p = new ApplyParams
                {
                    Terrain = terrain,
                    TerrainWorld = terrainWorld,
                    Brush = brush,
                    Gizmo = gizmo,
                    Options = options,
                    Strength = strength,
                    HeightmapSize = heightmapSize,
                    TempBuffer = tempBuffer,
                    ModifiedOffset = modifiedOffset,
                    ModifiedSize = modifiedSize,
                    PatchCoord = patch.PatchCoord,
                    PatchPositionLocal = patchPositionLocal,
                    SourceHeightMap = sourceHeights,
                    SourceHolesMask = sourceHoles,
                };

                affectedPatches.Add(p);
            }

            return affectedPatches;
        }

        /// <summary>
        /// Applies the modification to the terrain.
        /// </summary>
        /// <param name="brush">The brush.</param>
        /// <param name="options">The options.</param>
        /// <param name="gizmo">The gizmo.</param>
        /// <param name="terrain">The terrain.</param>
        public void Apply(Brush brush, ref Options options, SculptTerrainGizmoMode gizmo, FlaxEngine.Terrain terrain)
        {
            var affectedPatches = GetAffectedPatches(brush, ref options, gizmo, terrain);

            if (affectedPatches.Count == 0)
            {
                return;
            }

            ApplyBrush(gizmo, affectedPatches);

            // Auto NavMesh rebuild
            var brushBounds = gizmo.CursorBrushBounds;
            gizmo.CurrentEditUndoAction.AddDirtyBounds(ref brushBounds);
        }

        /// <summary>
        /// Applies the brush to all affected patches
        /// </summary>
        /// <param name="gizmo"></param>
        /// <param name="affectedPatches"></param>
        public virtual void ApplyBrush(SculptTerrainGizmoMode gizmo, List<ApplyParams> affectedPatches)
        {
            for (int i = 0; i < affectedPatches.Count; i++)
            {
                ApplyParams patchApplyParams = affectedPatches[i];

                // Record patch data before editing it
                if (!gizmo.CurrentEditUndoAction.HashPatch(ref patchApplyParams.PatchCoord))
                {
                    gizmo.CurrentEditUndoAction.AddPatch(ref patchApplyParams.PatchCoord);
                }

                ApplyBrushToPatch(ref patchApplyParams);

                // Auto NavMesh rebuild
                var brushBounds = gizmo.CursorBrushBounds;
                gizmo.CurrentEditUndoAction.AddDirtyBounds(ref brushBounds);
            }
        }

        /// <summary>
        /// The mode apply parameters.
        /// </summary>
        public unsafe struct ApplyParams
        {
            /// <summary>
            /// The brush.
            /// </summary>
            public Brush Brush;

            /// <summary>
            /// The options.
            /// </summary>
            public Options Options;

            /// <summary>
            /// The gizmo.
            /// </summary>
            public SculptTerrainGizmoMode Gizmo;

            /// <summary>
            /// The terrain.
            /// </summary>
            public FlaxEngine.Terrain Terrain;

            /// <summary>
            /// The patch coordinates.
            /// </summary>
            public Int2 PatchCoord;

            /// <summary>
            /// The modified offset.
            /// </summary>
            public Int2 ModifiedOffset;

            /// <summary>
            /// The modified size.
            /// </summary>
            public Int2 ModifiedSize;

            /// <summary>
            /// The final calculated strength of the effect to apply (can be negative for inverted terrain modification if <see cref="SupportsNegativeApply"/> is set).
            /// </summary>
            public float Strength;

            /// <summary>
            /// The temporary data buffer (for modified data). Has size of array of floats that has size of heightmap length.
            /// </summary>
            public float* TempBuffer;

            /// <summary>
            /// The source heightmap data buffer. May be null if modified is holes mask.
            /// </summary>
            public float* SourceHeightMap;

            /// <summary>
            /// The source holes mask data buffer. May be null if modified is.
            /// </summary>
            public byte* SourceHolesMask;

            /// <summary>
            /// The heightmap size (edge).
            /// </summary>
            public int HeightmapSize;

            /// <summary>
            /// The patch position in terrain local-space.
            /// </summary>
            public Vector3 PatchPositionLocal;

            /// <summary>
            /// The terrain local-to-world matrix.
            /// </summary>
            public Matrix TerrainWorld;
        }

        /// <summary>
        /// Applies the modification to the terrain.
        /// </summary>
        /// <param name="p">The parameters to use.</param>
        public abstract void ApplyBrushToPatch(ref ApplyParams p);
    }
}

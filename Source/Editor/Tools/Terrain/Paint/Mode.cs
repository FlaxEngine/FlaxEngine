// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Tools.Terrain.Brushes;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Paint
{
    /// <summary>
    /// The base class for terran paint tool modes.
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
        /// The tool strength (normalized to range 0-1). Defines the intensity of the paint operation to make it stronger or more subtle.
        /// </summary>
        [EditorOrder(0), Limit(0, 10, 0.01f), Tooltip("The tool strength (normalized to range 0-1). Defines the intensity of the paint operation to make it stronger or more subtle.")]
        public float Strength = 1.0f;

        /// <summary>
        /// Gets a value indicating whether this mode supports negative apply for terrain modification.
        /// </summary>
        public virtual bool SupportsNegativeApply => false;

        /// <summary>
        /// Gets the index of the active splatmap texture to modify by the tool. It must be equal or higher than zero bu less than <see cref="FlaxEngine.Terrain.MaxSplatmapsCount"/>.
        /// </summary>
        public abstract int ActiveSplatmapIndex { get; }

        /// <summary>
        /// Applies the modification to the terrain.
        /// </summary>
        /// <param name="brush">The brush.</param>
        /// <param name="options">The options.</param>
        /// <param name="gizmo">The gizmo.</param>
        /// <param name="terrain">The terrain.</param>
        public unsafe void Apply(Brush brush, ref Options options, PaintTerrainGizmoMode gizmo, FlaxEngine.Terrain terrain)
        {
            // Combine final apply strength
            float strength = Strength * options.Strength * options.DeltaTime * 10.0f;
            if (strength <= 0.0f)
                return;
            if (options.Invert && SupportsNegativeApply)
                strength *= -1;

            // Prepare
            var splatmapIndex = ActiveSplatmapIndex;
            var splatmapIndexOther = (splatmapIndex + 1) % 2;
            var chunkSize = terrain.ChunkSize;
            var heightmapSize = chunkSize * FlaxEngine.Terrain.PatchEdgeChunksCount + 1;
            var heightmapLength = heightmapSize * heightmapSize;
            var patchSize = chunkSize * FlaxEngine.Terrain.UnitsPerVertex * FlaxEngine.Terrain.PatchEdgeChunksCount;
            var tempBuffer = (Color32*)gizmo.GetSplatmapTempBuffer(heightmapLength * Color32.SizeInBytes, splatmapIndex).ToPointer();
            var tempBufferOther = (Color32*)gizmo.GetSplatmapTempBuffer(heightmapLength * Color32.SizeInBytes, (splatmapIndex + 1) % 2).ToPointer();
            var unitsPerVertexInv = 1.0f / FlaxEngine.Terrain.UnitsPerVertex;
            ApplyParams p = new ApplyParams
            {
                Terrain = terrain,
                Brush = brush,
                Gizmo = gizmo,
                Options = options,
                Strength = strength,
                SplatmapIndex = splatmapIndex,
                SplatmapIndexOther = splatmapIndexOther,
                HeightmapSize = heightmapSize,
                TempBuffer = tempBuffer,
                TempBufferOther = tempBufferOther,
            };

            // Get brush bounds in terrain local space
            var brushBounds = gizmo.CursorBrushBounds;
            terrain.GetLocalToWorldMatrix(out p.TerrainWorld);
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

                // Clamp to prevent overflows
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
                modifiedSize.X = Mathf.Min(modifiedSize.X, heightmapSize - modifiedOffset.X);
                modifiedSize.Y = Mathf.Min(modifiedSize.Y, heightmapSize - modifiedOffset.Y);

                // Skip patch won't be modified at all
                if (modifiedSize.X <= 0 || modifiedSize.Y <= 0)
                    continue;

                // Get the patch data (cached internally by the c++ core in editor)
                var sourceData = TerrainTools.GetSplatMapData(terrain, ref patch.PatchCoord, splatmapIndex);
                if (sourceData == null)
                    throw new Exception("Cannot modify terrain. Loading splatmap failed. See log for more info.");
                
                var sourceDataOther = TerrainTools.GetSplatMapData(terrain, ref patch.PatchCoord, splatmapIndexOther);
                if (sourceDataOther == null)
                    throw new Exception("Cannot modify terrain. Loading splatmap failed. See log for more info.");

                // Record patch data before editing it
                if (!gizmo.CurrentEditUndoAction.HashPatch(ref patch.PatchCoord))
                {
                    gizmo.CurrentEditUndoAction.AddPatch(ref patch.PatchCoord, splatmapIndex);
                    gizmo.CurrentEditUndoAction.AddPatch(ref patch.PatchCoord, splatmapIndexOther);
                }

                // Apply modification
                p.ModifiedOffset = modifiedOffset;
                p.ModifiedSize = modifiedSize;
                p.PatchCoord = patch.PatchCoord;
                p.PatchPositionLocal = patchPositionLocal;
                p.SourceData = sourceData;
                p.SourceDataOther = sourceDataOther;
                Apply(ref p);
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
            public PaintTerrainGizmoMode Gizmo;

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
            /// The splatmap texture index.
            /// </summary>
            public int SplatmapIndex;
            
            /// <summary>
            /// The splatmap texture index. If <see cref="SplatmapIndex"/> is 0, this will be 1. If <see cref="SplatmapIndex"/> is 1, this will be 0.
            /// </summary>
            public int SplatmapIndexOther;

            /// <summary>
            /// The temporary data buffer (for modified data).
            /// </summary>
            public Color32* TempBuffer;
            
            /// <summary>
            /// The 'other' temporary data buffer (for modified data). If <see cref="TempBuffer"/> refersto the splatmap with index 0, this one will refer to the one with index 1.
            /// </summary>
            public Color32* TempBufferOther;

            /// <summary>
            /// The source data buffer.
            /// </summary>
            public Color32* SourceData;
            
            /// <summary>
            /// The 'other' source data buffer. If <see cref="SourceData"/> refers
            /// to the splatmap with index 0, this one will refer to the one with index 1.
            /// </summary>
            public Color32* SourceDataOther;

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
        public abstract void Apply(ref ApplyParams p);
    }
}

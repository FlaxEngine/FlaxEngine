// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;
using System;

namespace FlaxEditor.Tools.Terrain.Sculpt
{
    /// <summary>
    /// Sculpt tool mode that smooths the terrain heightmap area affected by brush.
    /// </summary>
    /// <seealso cref="FlaxEditor.Tools.Terrain.Sculpt.Mode" />
    [HideInEditor]
    public sealed class SmoothMode : Mode
    {
        /// <summary>
        /// The tool smoothing radius. Defines the size of smoothing kernel, the higher value the more nearby samples is included into normalized sum. Scaled by the brush size.
        /// </summary>
        [EditorOrder(10), Limit(0, 1, 0.01f), Tooltip("The tool smoothing radius. Defines the size of smoothing kernel, the higher value the more nearby samples is included into normalized sum. Scaled by the brush size.")]
        public float FilterRadius = 0.4f;

        /// <inheritdoc />
        public override unsafe void ApplyBrush(SculptTerrainGizmoMode gizmo, List<ApplyParams> affectedPatches)
        {
            Profiler.BeginEvent("Apply Brush");

            // TODO: don't need these on each patch; just need them once
            var heightmapSize = affectedPatches[0].HeightmapSize;
            var radius = Mathf.Max(Mathf.CeilToInt(FilterRadius * 0.01f * affectedPatches[0].Brush.Size), 2);

            // Calculate bounding coordinates of the total affected area
            Int2 modifieedAreaMinCoord = Int2.Maximum;
            Int2 modifiedAreaMaxCoord = Int2.Minimum;
            for (int i = 0; i < affectedPatches.Count; i++)
            {
                var patch = affectedPatches[i];

                var tl = (patch.PatchCoord * (heightmapSize - 1)) + patch.ModifiedOffset;
                var br = tl + patch.ModifiedSize;

                if (tl.X <= modifieedAreaMinCoord.X && tl.Y <= modifieedAreaMinCoord.Y)
                    modifieedAreaMinCoord = tl;
                if (br.X >= modifiedAreaMaxCoord.X && br.Y >= modifiedAreaMaxCoord.Y)
                    modifiedAreaMaxCoord = br;
            }
            var totalModifiedSize = modifiedAreaMaxCoord - modifieedAreaMinCoord;

            // Build map of heights in affected area
            var modifiedHeights = new float[totalModifiedSize.X * totalModifiedSize.Y];
            for (int i = 0; i < affectedPatches.Count; i++)
            {
                var patch = affectedPatches[i];

                for (int z = 0; z < patch.ModifiedSize.Y; z++)
                {
                    for (int x = 0; x < patch.ModifiedSize.X; x++)
                    {
                        // Read height from current patch
                        var localCoordX = (x + patch.ModifiedOffset.X);
                        var localCoordY = (z + patch.ModifiedOffset.Y);
                        var coordHeight = patch.SourceHeightMap[(localCoordY * heightmapSize) + localCoordX];

                        // Calculate the absolute coordinate of the terrain point
                        var absoluteCurrentPointCoord = patch.PatchCoord * (heightmapSize - 1) + patch.ModifiedOffset + new Int2(x, z);
                        var currentPointCoordRelativeToModifiedArea = absoluteCurrentPointCoord - modifieedAreaMinCoord;

                        // Store height
                        var index = (currentPointCoordRelativeToModifiedArea.Y * totalModifiedSize.X) + currentPointCoordRelativeToModifiedArea.X;
                        modifiedHeights[index] = coordHeight;
                    }
                }
            }

            // Iterate through modified points and smooth now that we have height information for all necessary points
            for (int i = 0; i < affectedPatches.Count; i++)
            {
                var patch = affectedPatches[i];
                var brushPosition = patch.Gizmo.CursorPosition;
                var strength = Mathf.Saturate(patch.Strength);

                for (int z = 0; z < patch.ModifiedSize.Y; z++)
                {
                    for (int x = 0; x < patch.ModifiedSize.X; x++)
                    {
                        // Read height from current patch
                        var localCoordX = (x + patch.ModifiedOffset.X);
                        var localCoordY = (z + patch.ModifiedOffset.Y);
                        var coordHeight = patch.SourceHeightMap[(localCoordY * heightmapSize) + localCoordX];

                        // Calculate the absolute coordinate of the terrain point
                        var absoluteCurrentPointCoord = patch.PatchCoord * (heightmapSize - 1) + patch.ModifiedOffset + new Int2(x, z);
                        var currentPointCoordRelativeToModifiedArea = absoluteCurrentPointCoord - modifieedAreaMinCoord;

                        // Calculate brush influence at the current position
                        var samplePositionLocal = patch.PatchPositionLocal + new Vector3(localCoordX * FlaxEngine.Terrain.UnitsPerVertex, coordHeight, localCoordY * FlaxEngine.Terrain.UnitsPerVertex);
                        Vector3.Transform(ref samplePositionLocal, ref patch.TerrainWorld, out Vector3 samplePositionWorld);
                        var paintAmount = patch.Brush.Sample(ref brushPosition, ref samplePositionWorld) * strength;

                        if (paintAmount == 0)
                        {
                            patch.TempBuffer[z * patch.ModifiedSize.X + x] = coordHeight;
                            continue;
                        }

                        // Record patch data before editing it
                        if (!gizmo.CurrentEditUndoAction.HashPatch(ref patch.PatchCoord))
                        {
                            gizmo.CurrentEditUndoAction.AddPatch(ref patch.PatchCoord);
                        }

                        // Sum the nearby values
                        float smoothValue = 0;
                        int smoothValueSamples = 0;

                        var minX = Math.Max(0, currentPointCoordRelativeToModifiedArea.X - radius);
                        var maxX = Math.Min(totalModifiedSize.X - 1, currentPointCoordRelativeToModifiedArea.X + radius);
                        var minZ = Math.Max(0, currentPointCoordRelativeToModifiedArea.Y - radius);
                        var maxZ = Math.Min(totalModifiedSize.Y - 1, currentPointCoordRelativeToModifiedArea.Y + radius);

                        for (int dz = minZ; dz <= maxZ; dz++)
                        {
                            for (int dx = minX; dx <= maxX; dx++)
                            {
                                var coordIndex = (dz * totalModifiedSize.X) + dx;
                                var height = modifiedHeights[coordIndex];
                                smoothValue += height;
                                smoothValueSamples++;
                            }
                        }

                        // Normalize
                        smoothValue /= smoothValueSamples;

                        // Blend between the height and smooth value
                        var newHeight = Mathf.Lerp(coordHeight, smoothValue, paintAmount);
                        patch.TempBuffer[z * patch.ModifiedSize.X + x] = newHeight;
                    }
                }

                // Update terrain patch
                TerrainTools.ModifyHeightMap(patch.Terrain, ref patch.PatchCoord, patch.TempBuffer, ref patch.ModifiedOffset, ref patch.ModifiedSize);
            }

            // Auto NavMesh rebuild
            var brushBounds = gizmo.CursorBrushBounds;
            gizmo.CurrentEditUndoAction.AddDirtyBounds(ref brushBounds);

            Profiler.EndEvent();
        }

        /// <inheritdoc />
        public override unsafe void ApplyBrushToPatch(ref ApplyParams p)
        {
            // noop; unused
        }
    }
}

// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

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
        public override unsafe void Apply(ref ApplyParams p)
        {
            // Prepare
            var brushPosition = p.Gizmo.CursorPosition;
            var radius = Mathf.Max(Mathf.CeilToInt(FilterRadius * 0.01f * p.Brush.Size), 2);
            var max = p.HeightmapSize - 1;
            var strength = Mathf.Saturate(p.Strength);

            // Apply brush modification
            Profiler.BeginEvent("Apply Brush");
            for (int z = 0; z < p.ModifiedSize.Y; z++)
            {
                var zz = z + p.ModifiedOffset.Y;
                for (int x = 0; x < p.ModifiedSize.X; x++)
                {
                    var xx = x + p.ModifiedOffset.X;
                    var sourceHeight = p.SourceHeightMap[zz * p.HeightmapSize + xx];

                    var samplePositionLocal = p.PatchPositionLocal + new Vector3(xx * FlaxEngine.Terrain.UnitsPerVertex, sourceHeight, zz * FlaxEngine.Terrain.UnitsPerVertex);
                    Vector3.Transform(ref samplePositionLocal, ref p.TerrainWorld, out Vector3 samplePositionWorld);

                    var paintAmount = p.Brush.Sample(ref brushPosition, ref samplePositionWorld) * strength;

                    if (paintAmount > 0)
                    {
                        // Sum the nearby values
                        float smoothValue = 0;
                        int smoothValueSamples = 0;
                        int minX = Math.Max(x - radius + p.ModifiedOffset.X, 0);
                        int minZ = Math.Max(z - radius + p.ModifiedOffset.Y, 0);
                        int maxX = Math.Min(x + radius + p.ModifiedOffset.X, max);
                        int maxZ = Math.Min(z + radius + p.ModifiedOffset.Y, max);
                        for (int dz = minZ; dz <= maxZ; dz++)
                        {
                            for (int dx = minX; dx <= maxX; dx++)
                            {
                                var height = p.SourceHeightMap[dz * p.HeightmapSize + dx];
                                smoothValue += height;
                                smoothValueSamples++;
                            }
                        }

                        // Normalize
                        smoothValue /= smoothValueSamples;

                        // Blend between the height and smooth value
                        p.TempBuffer[z * p.ModifiedSize.X + x] = Mathf.Lerp(sourceHeight, smoothValue, paintAmount);
                    }
                    else
                    {
                        p.TempBuffer[z * p.ModifiedSize.X + x] = sourceHeight;
                    }
                }
            }
            Profiler.EndEvent();

            // Update terrain patch
            TerrainTools.ModifyHeightMap(p.Terrain, ref p.PatchCoord, p.TempBuffer, ref p.ModifiedOffset, ref p.ModifiedSize);
        }
    }
}

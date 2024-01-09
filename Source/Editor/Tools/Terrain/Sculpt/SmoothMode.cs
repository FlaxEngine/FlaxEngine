// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Security.Cryptography.X509Certificates;
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

                    if (paintAmount <= 0)
                    {
                        p.TempBuffer[z * p.ModifiedSize.X + x] = sourceHeight;
                        continue;
                    }

                    float smoothValue = 0;
                    smoothValue = SampleHeightAverage(p.Terrain, p.HeightmapSize, ref p.ModifiedOffset, radius);

                    // Blend between the height and smooth value
                    p.TempBuffer[z * p.ModifiedSize.X + x] = Mathf.Lerp(sourceHeight, smoothValue, paintAmount);

                }
            }
            Profiler.EndEvent();

            // Update terrain patch
            TerrainTools.ModifyHeightMap(p.Terrain, ref p.PatchCoord, p.TempBuffer, ref p.ModifiedOffset, ref p.ModifiedSize);
        }

        float SampleHeightAverage(FlaxEngine.Terrain terrain, int heightmapSize, ref Int2 startPosition, int radius)
        {
            int minPatchOffsetX = Mathf.FloorToInt((float)startPosition.X - radius / heightmapSize);
            int minPatchOffsetZ = Mathf.FloorToInt((float)startPosition.Y - radius / heightmapSize);

            int maxPatchOffsetX = Mathf.CeilToInt(((float)startPosition.X + radius) / heightmapSize);
            int maxPatchOffsetZ = Mathf.CeilToInt(((float)startPosition.Y + radius) / heightmapSize);

            float total = 0;
            int totalPoints = 0;

            for (int patchOffsetZ = minPatchOffsetZ; patchOffsetZ <= maxPatchOffsetZ; patchOffsetZ++)
            {
                for (int patchOffsetX = minPatchOffsetX; patchOffsetX <= maxPatchOffsetX; patchOffsetX++)
                {
                    var patchCoords = new Int2(startPosition.X + patchOffsetX, startPosition.Y + patchOffsetZ);
                    var relativeStartPosition = new Int2(-patchOffsetX * heightmapSize, -patchOffsetZ * heightmapSize);

                    ReadHeightsFromPatch(terrain, heightmapSize, ref patchCoords, ref relativeStartPosition, radius, out var accumulation, out var points);

                    total += accumulation;
                    totalPoints += points;
                }
            }

            if (totalPoints == 0)
            {
                return 0;
            }
            else
            {
                return total / totalPoints;
            }
        }

        unsafe void ReadHeightsFromPatch(FlaxEngine.Terrain terrain, int heightmapSize, ref Int2 patchCoords, ref Int2 startPosition, int radius, out float accumulation, out int points)
        {
            accumulation = 0;
            points = 0;

            if (!terrain.HasPatch(ref patchCoords))
            {
                return;
            }

            var patch = TerrainTools.GetHeightmapData(terrain, ref patchCoords);

            var max = heightmapSize - 1;
            var minX = Math.Max(startPosition.X - radius, 0);
            var minZ = Math.Max(startPosition.Y - radius, 0);
            var maxX = Math.Min(startPosition.X + radius, max);
            var maxZ = Math.Min(startPosition.Y + radius, max);

            for (int dz = minZ; dz <= maxZ; dz++)
            {
                for (int dx = minX; dx <= maxX; dx++)
                {
                    var height = patch[dz * heightmapSize + dx];
                    accumulation += height;
                    points++;
                }
            }
        }
    }
}

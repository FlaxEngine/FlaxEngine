// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Sculpt
{
    /// <summary>
    /// Sculpt tool mode. Edits terrain heightmap by moving area affected by brush up or down.
    /// </summary>
    /// <seealso cref="FlaxEditor.Tools.Terrain.Sculpt.Mode" />
    [HideInEditor]
    public sealed class SculptMode : Mode
    {
        /// <inheritdoc />
        public override bool SupportsNegativeApply => true;

        /// <inheritdoc />
        public override unsafe void ApplyBrushToPatch(ref ApplyParams p)
        {
            var strength = p.Strength * 1000.0f;
            var brushPosition = p.Gizmo.CursorPosition;

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

                    var paintAmount = p.Brush.Sample(ref brushPosition, ref samplePositionWorld);

                    p.TempBuffer[z * p.ModifiedSize.X + x] = sourceHeight + paintAmount * strength;
                }
            }
            Profiler.EndEvent();

            // Update terrain patch
            TerrainTools.ModifyHeightMap(p.Terrain, ref p.PatchCoord, p.TempBuffer, ref p.ModifiedOffset, ref p.ModifiedSize);
        }
    }
}

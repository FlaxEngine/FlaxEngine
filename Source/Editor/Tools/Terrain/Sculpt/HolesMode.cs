// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Sculpt
{
    /// <summary>
    /// Terrain holes creating tool mode edits terrain holes mask by changing area affected by brush.
    /// </summary>
    /// <seealso cref="FlaxEditor.Tools.Terrain.Sculpt.Mode" />
    [HideInEditor]
    public sealed class HolesMode : Mode
    {
        /// <inheritdoc />
        public override bool SupportsNegativeApply => true;

        /// <inheritdoc />
        public override bool EditHoles => true;

        /// <summary>
        /// Initializes a new instance of the <see cref="HolesMode"/> class.
        /// </summary>
        public HolesMode()
        {
            Strength = 6.0f;
        }

        /// <inheritdoc />
        public override unsafe void ApplyBrushToPatch(ref ApplyParams p)
        {
            var strength = p.Strength * -10.0f;
            var brushPosition = p.Gizmo.CursorPosition;
            var tempBuffer = (byte*)p.TempBuffer;

            // Apply brush modification
            Profiler.BeginEvent("Apply Brush");
            for (int z = 0; z < p.ModifiedSize.Y; z++)
            {
                var zz = z + p.ModifiedOffset.Y;
                for (int x = 0; x < p.ModifiedSize.X; x++)
                {
                    var xx = x + p.ModifiedOffset.X;
                    var sourceMask = p.SourceHolesMask[zz * p.HeightmapSize + xx] != 0 ? 1.0f : 0.0f;

                    var samplePositionLocal = p.PatchPositionLocal + new Vector3(xx * FlaxEngine.Terrain.UnitsPerVertex, 0, zz * FlaxEngine.Terrain.UnitsPerVertex);
                    Vector3.Transform(ref samplePositionLocal, ref p.TerrainWorld, out Vector3 samplePositionWorld);
                    samplePositionWorld.Y = brushPosition.Y;

                    var paintAmount = p.Brush.Sample(ref brushPosition, ref samplePositionWorld);

                    tempBuffer[z * p.ModifiedSize.X + x] = (byte)((sourceMask + paintAmount * strength) < 0.8f ? 0 : 255);
                }
            }
            Profiler.EndEvent();

            // Update terrain patch
            TerrainTools.ModifyHolesMask(p.Terrain, ref p.PatchCoord, tempBuffer, ref p.ModifiedOffset, ref p.ModifiedSize);
        }
    }
}

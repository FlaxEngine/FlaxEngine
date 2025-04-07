// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Sculpt
{
    /// <summary>
    /// Sculpt tool mode that flattens the terrain heightmap area affected by brush to the target value.
    /// </summary>
    /// <seealso cref="FlaxEditor.Tools.Terrain.Sculpt.Mode" />
    [HideInEditor]
    public sealed class FlattenMode : Mode
    {
        /// <summary>
        /// The target terrain height to blend to.
        /// </summary>
        [EditorOrder(10), Tooltip("The target terrain height to blend to.")]
        public float TargetHeight = 0.0f;

        /// <inheritdoc />
        public override unsafe void ApplyBrushToPatch(ref ApplyParams p)
        {
            // If used with invert mode pick the target height level
            if (p.Options.Invert)
            {
                var center = p.ModifiedOffset + p.ModifiedSize / 2;
                TargetHeight = p.SourceHeightMap[center.Y * p.HeightmapSize + center.X];
                return;
            }

            // Prepare
            var brushPosition = p.Gizmo.CursorPosition;
            var targetHeight = TargetHeight;
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

                    // Blend between the height and the target value
                    p.TempBuffer[z * p.ModifiedSize.X + x] = Mathf.Lerp(sourceHeight, targetHeight, paintAmount);
                }
            }
            Profiler.EndEvent();

            // Update terrain patch
            TerrainTools.ModifyHeightMap(p.Terrain, ref p.PatchCoord, p.TempBuffer, ref p.ModifiedOffset, ref p.ModifiedSize);
        }
    }
}

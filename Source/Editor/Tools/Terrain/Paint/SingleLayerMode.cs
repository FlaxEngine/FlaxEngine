// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Paint
{
    /// <summary>
    /// Paint tool mode. Edits terrain splatmap by painting with the single layer on top of the others.
    /// </summary>
    /// <seealso cref="FlaxEditor.Tools.Terrain.Paint.Mode" />
    [HideInEditor]
    public sealed class SingleLayerMode : Mode
    {
        /// <summary>
        /// The paint layers.
        /// </summary>
        public enum Layers
        {
            /// <summary>
            /// The layer 0.
            /// </summary>
            Layer0,

            /// <summary>
            /// The layer 0.
            /// </summary>
            Layer1,

            /// <summary>
            /// The layer 2.
            /// </summary>
            Layer2,

            /// <summary>
            /// The layer 3.
            /// </summary>
            Layer3,

            /// <summary>
            /// The layer 4.
            /// </summary>
            Layer4,

            /// <summary>
            /// The layer 5.
            /// </summary>
            Layer5,

            /// <summary>
            /// The layer 6.
            /// </summary>
            Layer6,

            /// <summary>
            /// The layer 7.
            /// </summary>
            Layer7,
        }

        /// <summary>
        /// The layer to paint with it.
        /// </summary>
        [EditorOrder(10), Tooltip("The layer to paint on. Terrain material can access a per-layer blend weight to perform material or texture blending."), CustomEditorAlias("FlaxEditor.CustomEditors.Editors.TerrainLayerEditor")]
        public Layers Layer = Layers.Layer0;

        /// <inheritdoc />
        public override int ActiveSplatmapIndex => (int)Layer < 4 ? 0 : 1;

        /// <inheritdoc />
        public override unsafe void Apply(ref ApplyParams p)
        {
            var strength = p.Strength;
            var layer = (int)Layer;
            var brushPosition = p.Gizmo.CursorPosition;
            var c = layer % 4;

            // Apply brush modification
            Profiler.BeginEvent("Apply Brush");
            bool otherModified = false;
            for (int z = 0; z < p.ModifiedSize.Y; z++)
            {
                var zz = z + p.ModifiedOffset.Y;
                for (int x = 0; x < p.ModifiedSize.X; x++)
                {
                    var xx = x + p.ModifiedOffset.X;
                    var src = (Color)p.SourceData[zz * p.HeightmapSize + xx];

                    var samplePositionLocal = p.PatchPositionLocal + new Vector3(xx * FlaxEngine.Terrain.UnitsPerVertex, 0, zz * FlaxEngine.Terrain.UnitsPerVertex);
                    Vector3.Transform(ref samplePositionLocal, ref p.TerrainWorld, out Vector3 samplePositionWorld);
                    var sample = Mathf.Saturate(p.Brush.Sample(ref brushPosition, ref samplePositionWorld));

                    var paintAmount = sample * strength;
                    if (paintAmount < 0.0f)
                        continue; // Skip when pixel won't be affected

                    // Other layers reduction based on their sum and current paint intensity
                    var srcOther = (Color)p.SourceDataOther[zz * p.HeightmapSize + xx];
                    var otherLayersSum = src.ValuesSum + srcOther.ValuesSum - src[c];
                    var decreaseAmount = paintAmount / otherLayersSum;

                    // Paint on the active splatmap texture
                    var srcNew = Color.Clamp(src - src * decreaseAmount, Color.Zero, Color.White);
                    srcNew[c] = Mathf.Saturate(src[c] + paintAmount);
                    p.TempBuffer[z * p.ModifiedSize.X + x] = srcNew;

                    //if (other.ValuesSum > 0.0f) // Skip editing the other splatmap if it's empty
                    {
                        // Remove 'paint' from the other splatmap texture
                        srcOther = Color.Clamp(srcOther - srcOther * decreaseAmount, Color.Zero, Color.White);
                        p.TempBufferOther[z * p.ModifiedSize.X + x] = srcOther;
                        otherModified = true;
                    }
                }
            }
            Profiler.EndEvent();

            // Update terrain patch
            TerrainTools.ModifySplatMap(p.Terrain, ref p.PatchCoord, p.SplatmapIndex, p.TempBuffer, ref p.ModifiedOffset, ref p.ModifiedSize);
            if (otherModified)
                TerrainTools.ModifySplatMap(p.Terrain, ref p.PatchCoord, p.SplatmapIndexOther, p.TempBufferOther, ref p.ModifiedOffset, ref p.ModifiedSize);
        }
    }
}

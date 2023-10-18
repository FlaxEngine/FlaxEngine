// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
        [EditorOrder(10), Tooltip("The layer to paint with it. Terrain material can access per-layer blend weight to perform materials or textures blending.")]
        public Layers Layer = Layers.Layer0;

        /// <inheritdoc />
        public override int ActiveSplatmapIndex => (int)Layer < 4 ? 0 : 1;

        /// <inheritdoc />
        public override unsafe void Apply(ref ApplyParams p)
        {
            var strength = p.Strength;
            var layer = (int)Layer;
            var brushPosition = p.Gizmo.CursorPosition;
            var layerComponent = layer % 4;

            // Apply brush modification
            Profiler.BeginEvent("Apply Brush");
            for (int z = 0; z < p.ModifiedSize.Y; z++)
            {
                var zz = z + p.ModifiedOffset.Y;
                for (int x = 0; x < p.ModifiedSize.X; x++)
                {
                    var xx = x + p.ModifiedOffset.X;
                    var src = p.SourceData[zz * p.HeightmapSize + xx];

                    var samplePositionLocal = p.PatchPositionLocal + new Vector3(xx * FlaxEngine.Terrain.UnitsPerVertex, 0, zz * FlaxEngine.Terrain.UnitsPerVertex);
                    Vector3.Transform(ref samplePositionLocal, ref p.TerrainWorld, out Vector3 samplePositionWorld);
                    
                    var sample = Mathf.Clamp(p.Brush.Sample(ref brushPosition, ref samplePositionWorld), 0f, 1f);
                    var paintAmount = sample * strength * (1f - src[layerComponent] / 255f);

                    src[layerComponent] = (byte)Mathf.Clamp(src[layerComponent] + paintAmount * 255, 0, 255);
                    src[(layerComponent + 1) % 4] = (byte)Mathf.Clamp(src[(layerComponent + 1) % 4] - paintAmount * 255, 0, 255);
                    src[(layerComponent + 2) % 4] = (byte)Mathf.Clamp(src[(layerComponent + 2) % 4] - paintAmount * 255, 0, 255);
                    src[(layerComponent + 3) % 4] = (byte)Mathf.Clamp(src[(layerComponent + 3) % 4] - paintAmount * 255, 0, 255);
                    
                    p.TempBuffer[z * p.ModifiedSize.X + x] = src;
                }
            }
            Profiler.EndEvent();

            // Update terrain patch
            TerrainTools.ModifySplatMap(p.Terrain, ref p.PatchCoord, p.SplatmapIndex, p.TempBuffer, ref p.ModifiedOffset, ref p.ModifiedSize);
        }
    }
}

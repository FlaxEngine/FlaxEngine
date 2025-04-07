// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Foliage painting brush.
    /// </summary>
    [HideInEditor]
    public class Brush
    {
        /// <summary>
        /// The cached material instance for the brush usage.
        /// </summary>
        protected MaterialInstance _material;

        /// <summary>
        /// The brush size (in world units). Within this area, the brush will have effect.
        /// </summary>
        [EditorOrder(0), Limit(0.0f, 1000000.0f, 10.0f), Tooltip("The brush size (in world units). Within this area, the brush will have effect.")]
        public float Size = 800.0f;

        /// <summary>
        /// If checked, brush will apply only once on painting start instead of continuous painting.
        /// </summary>
        [EditorOrder(10), Tooltip("If checked, brush will apply only once on painting start instead of continuous painting.")]
        public bool SingleClick = false;

        /// <summary>
        /// The additional scale for foliage density when painting. Can be used to increase or decrease foliage density during painting.
        /// </summary>
        [EditorOrder(20), Limit(0.0f, 1000.0f, 0.01f), Tooltip("The additional scale for foliage density when painting. Can be used to increase or decrease foliage density during painting.")]
        public float DensityScale = 1.0f;

        /// <summary>
        /// Gets the brush material for the terrain chunk rendering. It must have domain set to Terrain. Setup material parameters within this call.
        /// </summary>
        /// <param name="position">The world-space brush position.</param>
        /// <param name="color">The brush position.</param>
        /// <param name="sceneDepth">The scene depth buffer (used for manual brush pixels clipping with rendered scene).</param>
        /// <returns>The ready to render material for terrain chunks overlay on top of the terrain.</returns>
        public MaterialInstance GetBrushMaterial(ref Vector3 position, ref Color color, GPUTexture sceneDepth)
        {
            if (!_material)
            {
                var material = FlaxEngine.Content.LoadAsyncInternal<Material>(EditorAssets.FoliageBrushMaterial);
                material.WaitForLoaded();
                _material = material.CreateVirtualInstance();
            }

            if (_material)
            {
                _material.SetParameterValue("Color", color);
                _material.SetParameterValue("DepthBuffer", sceneDepth);
            }

            return _material;
        }
    }
}

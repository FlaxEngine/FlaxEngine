// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Brushes
{
    /// <summary>
    /// Terrain sculpture or paint brush logic descriptor.
    /// </summary>
    [HideInEditor]
    public abstract class Brush
    {
        /// <summary>
        /// The cached material instance for the brush usage.
        /// </summary>
        protected MaterialInstance _material;

        /// <summary>
        /// The brush size (in world units). Within this area, the brush will have at least some effect.
        /// </summary>
        [EditorOrder(0), Limit(0.0f, 1000000.0f, 10.0f), Tooltip("The brush size (in world units). Within this area, the brush will have at least some effect.")]
        public float Size = 4000.0f;

        /// <summary>
        /// Gets the brush material for the terrain chunk rendering. It must have domain set to Terrain. Setup material parameters within this call.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="position">The world-space brush position.</param>
        /// <param name="color">The brush position.</param>
        /// <returns>The ready to render material for terrain chunks overlay on top of the terrain.</returns>
        public abstract MaterialInstance GetBrushMaterial(ref RenderContext renderContext, ref Vector3 position, ref Color color);

        /// <summary>
        /// Loads the brush material from the internal location. It's later cached by the object and reused.
        /// </summary>
        /// <param name="internalPath">The brush material path (for in-build editor brushes).</param>
        /// <returns>The brush material instance or null if cannot load or missing.</returns>
        protected MaterialInstance CacheMaterial(string internalPath)
        {
            if (!_material)
            {
                var material = FlaxEngine.Content.LoadAsyncInternal<Material>(internalPath);
                material.WaitForLoaded();
                _material = material.CreateVirtualInstance();
            }
            return _material;
        }

        /// <summary>
        /// Samples the brush intensity at the specified position.
        /// </summary>
        /// <param name="brushPosition">The brush center position (world-space).</param>
        /// <param name="samplePosition">The sample position (world-space).</param>
        /// <returns>The sampled value. Normalized to range 0-1 as an intensity to apply.</returns>
        public abstract float Sample(ref Vector3 brushPosition, ref Vector3 samplePosition);
    }
}

// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.GPUTexture"/>.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class GPUTextureBrush : IBrush
    {
        /// <summary>
        /// The GPU texture.
        /// </summary>
        [HideInEditor]
        public GPUTexture Texture;

        /// <summary>
        /// Initializes a new instance of the <see cref="GPUTextureBrush"/> class.
        /// </summary>
        public GPUTextureBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GPUTextureBrush"/> struct.
        /// </summary>
        /// <param name="texture">The GPU texture.</param>
        public GPUTextureBrush(GPUTexture texture)
        {
            Texture = texture;
        }

        /// <inheritdoc />
        public Vector2 Size => Texture?.Size ?? Vector2.Zero;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            Render2D.DrawTexture(Texture, rect, color);
        }
    }
}

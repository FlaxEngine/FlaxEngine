// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        /// The texture sampling filter mode.
        /// </summary>
        [ExpandGroups, Tooltip("The texture sampling filter mode.")]
        public BrushFilter Filter = BrushFilter.Linear;

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
            if (Filter == BrushFilter.Point)
                Render2D.DrawTexturePoint(Texture, rect, color);
            else
                Render2D.DrawTexture(Texture, rect, color);
        }
    }
}

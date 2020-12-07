// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.Texture"/>.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class TextureBrush : IBrush
    {
        /// <summary>
        /// The texture.
        /// </summary>
        [ExpandGroups, Tooltip("The texture asset.")]
        public Texture Texture;

        /// <summary>
        /// Initializes a new instance of the <see cref="TextureBrush"/> class.
        /// </summary>
        public TextureBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TextureBrush"/> struct.
        /// </summary>
        /// <param name="texture">The texture.</param>
        public TextureBrush(Texture texture)
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

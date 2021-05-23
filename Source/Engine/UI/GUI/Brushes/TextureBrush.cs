// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

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
        [ExpandGroups, EditorOrder(0), Tooltip("The texture asset.")]
        public Texture Texture;

        /// <summary>
        /// The texture sampling filter mode.
        /// </summary>
        [ExpandGroups, EditorOrder(1), Tooltip("The texture sampling filter mode.")]
        public BrushFilter Filter = BrushFilter.Linear;

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
            if (Filter == BrushFilter.Point)
                Render2D.DrawTexturePoint(Texture?.Texture, rect, color);
            else
                Render2D.DrawTexture(Texture, rect, color);
        }
    }
}

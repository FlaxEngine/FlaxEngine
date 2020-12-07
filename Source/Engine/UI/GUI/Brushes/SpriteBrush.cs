// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.Sprite"/>.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class SpriteBrush : IBrush
    {
        /// <summary>
        /// The sprite.
        /// </summary>
        [ExpandGroups]
        public SpriteHandle Sprite;

        /// <summary>
        /// Initializes a new instance of the <see cref="SpriteBrush"/> class.
        /// </summary>
        public SpriteBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SpriteBrush"/> struct.
        /// </summary>
        /// <param name="sprite">The sprite.</param>
        public SpriteBrush(SpriteHandle sprite)
        {
            Sprite = sprite;
        }

        /// <inheritdoc />
        public Vector2 Size => Sprite.IsValid ? Sprite.Size : Vector2.Zero;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            Render2D.DrawSprite(Sprite, rect, color);
        }
    }
}

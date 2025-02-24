// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        [ExpandGroups, EditorOrder(0), Tooltip("The sprite.")]
        public SpriteHandle Sprite;

        /// <summary>
        /// The texture sampling filter mode.
        /// </summary>
        [ExpandGroups, EditorOrder(1), Tooltip("The texture sampling filter mode.")]
        public BrushFilter Filter = BrushFilter.Linear;

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
        public Float2 Size => Sprite.IsValid ? Sprite.Size : Float2.Zero;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            if (Filter == BrushFilter.Point)
                Render2D.DrawSpritePoint(Sprite, rect, color);
            else
                Render2D.DrawSprite(Sprite, rect, color);
        }
    }

    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.Sprite"/> using 9-slicing.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class Sprite9SlicingBrush : IBrush
    {
        /// <summary>
        /// The sprite.
        /// </summary>
        [ExpandGroups, EditorOrder(0), Tooltip("The sprite.")]
        public SpriteHandle Sprite;

        /// <summary>
        /// The texture sampling filter mode.
        /// </summary>
        [ExpandGroups, EditorOrder(1), Tooltip("The texture sampling filter mode.")]
        public BrushFilter Filter = BrushFilter.Linear;

        /// <summary>
        /// The border size.
        /// </summary>
        [ExpandGroups, EditorOrder(2), Limit(0), Tooltip("The border size.")]
        public float BorderSize = 10.0f;

        /// <summary>
        /// The sprite borders (in texture space, range 0-1).
        /// </summary>
        [ExpandGroups, EditorOrder(3), Limit(0, 1), Tooltip("The sprite borders (in texture space, range 0-1).")]
        public Margin Border = new Margin(0.1f);

#if FLAX_EDITOR
        /// <summary>
        /// Displays borders (editor only).
        /// </summary>
        [NoSerialize, EditorOrder(4), Tooltip("Displays borders (editor only).")]
        public bool ShowBorders;
#endif

        /// <summary>
        /// Initializes a new instance of the <see cref="Sprite9SlicingBrush"/> class.
        /// </summary>
        public Sprite9SlicingBrush()
        {
        }

        /// <inheritdoc />
        public Float2 Size => Sprite.IsValid ? Sprite.Size : Float2.Zero;

        /// <inheritdoc />
        public unsafe void Draw(Rectangle rect, Color color)
        {
            if (!Sprite.IsValid)
                return;
            var border = Border;
            var borderUV = *(Float4*)&border;
            var borderSize = new Float4(BorderSize, BorderSize, BorderSize, BorderSize);
            if (Filter == BrushFilter.Point)
                Render2D.Draw9SlicingSpritePoint(Sprite, rect, borderSize, borderUV, color);
            else
                Render2D.Draw9SlicingSprite(Sprite, rect, borderSize, borderUV, color);
#if FLAX_EDITOR
            if (ShowBorders)
            {
                var bordersRect = rect;
                bordersRect.Location.X += borderSize.X;
                bordersRect.Location.Y += borderSize.Z;
                bordersRect.Size.X -= borderSize.X + borderSize.Y;
                bordersRect.Size.Y -= borderSize.Z + borderSize.W;
                Render2D.DrawRectangle(bordersRect, Color.YellowGreen, 2.0f);
            }
#endif
        }
    }
}

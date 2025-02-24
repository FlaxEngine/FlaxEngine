// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        public Float2 Size => Texture != null && !Texture.WaitForLoaded() ? Texture.Size : Float2.Zero;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            if (Filter == BrushFilter.Point)
                Render2D.DrawTexturePoint(Texture?.Texture, rect, color);
            else
                Render2D.DrawTexture(Texture, rect, color);
        }
    }

    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.Texture"/> using 9-slicing.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class Texture9SlicingBrush : IBrush
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
        /// The border size.
        /// </summary>
        [ExpandGroups, EditorOrder(2), Limit(0), Tooltip("The border size.")]
        public float BorderSize = 10.0f;

        /// <summary>
        /// The texture borders (in texture space, range 0-1).
        /// </summary>
        [ExpandGroups, EditorOrder(3), Limit(0, 1), Tooltip("The texture borders (in texture space, range 0-1).")]
        public Margin Border = new Margin(0.1f);

#if FLAX_EDITOR
        /// <summary>
        /// Displays borders (editor only).
        /// </summary>
        [NoSerialize, EditorOrder(4), Tooltip("Displays borders (editor only).")]
        public bool ShowBorders;
#endif

        /// <summary>
        /// Initializes a new instance of the <see cref="Texture9SlicingBrush"/> class.
        /// </summary>
        public Texture9SlicingBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Texture9SlicingBrush"/> struct.
        /// </summary>
        /// <param name="texture">The texture.</param>
        public Texture9SlicingBrush(Texture texture)
        {
            Texture = texture;
        }

        /// <inheritdoc />
        public Float2 Size => Texture != null ? Texture.Size : Float2.Zero;

        /// <inheritdoc />
        public unsafe void Draw(Rectangle rect, Color color)
        {
            if (Texture == null)
                return;
            var border = Border;
            var borderUV = *(Float4*)&border;
            var borderSize = new Float4(BorderSize, BorderSize, BorderSize, BorderSize);
            if (Filter == BrushFilter.Point)
                Render2D.Draw9SlicingTexturePoint(Texture, rect, borderSize, borderUV, color);
            else
                Render2D.Draw9SlicingTexture(Texture, rect, borderSize, borderUV, color);
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

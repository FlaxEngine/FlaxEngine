// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for single color fill.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class SolidColorBrush : IBrush
    {
        /// <summary>
        /// The brush color.
        /// </summary>
        [ExpandGroups, Tooltip("The brush color.")]
        public Color Color;

        /// <summary>
        /// Initializes a new instance of the <see cref="SolidColorBrush"/> class.
        /// </summary>
        public SolidColorBrush()
        {
            Color = Color.Black;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SolidColorBrush"/> struct.
        /// </summary>
        /// <param name="color">The color.</param>
        public SolidColorBrush(Color color)
        {
            Color = color;
        }

        /// <inheritdoc />
        public Float2 Size => Float2.One;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            Render2D.FillRectangle(rect, Color * color);
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for linear color gradient (made of 2 color).
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class LinearGradientBrush : IBrush
    {
        /// <summary>
        /// The brush start color.
        /// </summary>
        [EditorOrder(0), ExpandGroups, Tooltip("The brush start color.")]
        public Color StartColor;

        /// <summary>
        /// The brush end color.
        /// </summary>
        [EditorOrder(1), Tooltip("The brush end color.")]
        public Color EndColor;

        /// <summary>
        /// Initializes a new instance of the <see cref="LinearGradientBrush"/> class.
        /// </summary>
        public LinearGradientBrush()
        {
            StartColor = Color.White;
            EndColor = Color.Black;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="LinearGradientBrush"/> struct.
        /// </summary>
        /// <param name="startColor">The start color.</param>
        /// <param name="endColor">The end color.</param>
        public LinearGradientBrush(Color startColor, Color endColor)
        {
            StartColor = startColor;
            EndColor = endColor;
        }

        /// <inheritdoc />
        public Float2 Size => Float2.One;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            var startColor = StartColor * color;
            var endColor = EndColor * color;
            Render2D.FillRectangle(rect, startColor, startColor, endColor, endColor);
        }
    }
}

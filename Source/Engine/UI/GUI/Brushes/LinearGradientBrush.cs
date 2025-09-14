// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for linear color gradient (made of 2 color).
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class LinearGradientBrush : IBrush, IEquatable<LinearGradientBrush>
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

        /// <inheritdoc />
        public bool Equals(LinearGradientBrush other)
        {
            return other != null && StartColor == other.StartColor && EndColor == other.EndColor;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is LinearGradientBrush other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return HashCode.Combine(StartColor, EndColor);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return Equals(obj) ? 1 : 0;
        }
    }
}

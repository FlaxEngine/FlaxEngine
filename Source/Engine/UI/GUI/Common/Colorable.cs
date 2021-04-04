// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The basic GUI image control. Shows texture, sprite or render target.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class Colorable : ContainerControl
    {
        /// <inheritdoc />
        public Colorable(float x, float y, float width, float height) : base(x, y, width, height){}

        /// <inheritdoc />
        public Colorable(Vector2 location, Vector2 size) : base(location, size) { }

        /// <inheritdoc />
        public Colorable(Rectangle bounds) : base(bounds) { }

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color Color { get; set; } = Color.White;

        /// <summary>
        /// useful for overrides
        /// </summary>
        /// <returns></returns>
        public virtual Color GetColorToDraw()
        {
            return Color;
        }

    }
}

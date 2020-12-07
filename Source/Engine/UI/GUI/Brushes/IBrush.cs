// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Interface that unifies input source textures, sprites, render targets, and any other brushes to be used in a more generic way.
    /// </summary>
    public interface IBrush
    {
        /// <summary>
        /// Gets the size of the image brush in pixels (if relevant).
        /// </summary>
        Vector2 Size { get; }

        /// <summary>
        /// Draws the specified image using <see cref="Render2D"/> graphics backend.
        /// </summary>
        /// <param name="rect">The draw area rectangle.</param>
        /// <param name="color">The color.</param>
        void Draw(Rectangle rect, Color color);
    }
}

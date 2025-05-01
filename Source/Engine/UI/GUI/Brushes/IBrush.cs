// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Texture brush sampling modes.
    /// </summary>
    public enum BrushFilter
    {
        /// <summary>
        /// The point sampling without blending.
        /// </summary>
        [Tooltip("The point sampling without blending.")]
        Point = 0,

        /// <summary>
        /// The linear color sampling.
        /// </summary>
        [Tooltip("The linear color sampling.")]
        Linear = 1,
    };

    /// <summary>
    /// Interface that unifies input source textures, sprites, render targets, and any other brushes to be used in a more generic way.
    /// </summary>
    public interface IBrush
    {
        /// <summary>
        /// Gets the size of the image brush in pixels (if relevant).
        /// </summary>
        Float2 Size { get; }

        /// <summary>
        /// Draws the specified image using <see cref="Render2D"/> graphics backend.
        /// </summary>
        /// <param name="rect">The draw area rectangle.</param>
        /// <param name="color">The color.</param>
        void Draw(Rectangle rect, Color color);
    }
}

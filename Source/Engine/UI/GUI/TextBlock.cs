// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The text block.
    /// </summary>
    public struct TextBlock
    {
        /// <summary>
        /// The text range.
        /// </summary>
        public TextRange Range;

        /// <summary>
        /// The text style.
        /// </summary>
        public TextBlockStyle Style;

        /// <summary>
        /// The text location and size.
        /// </summary>
        public Rectangle Bounds;

        /// <summary>
        /// Custom ascender value for the line layout (block size above the baseline). Set to 0 to use ascender from the font.
        /// </summary>
        public float Ascender;

        /// <summary>
        /// The custom tag.
        /// </summary>
        public object Tag;
    }
}

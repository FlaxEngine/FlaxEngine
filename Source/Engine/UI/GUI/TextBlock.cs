// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
        /// The custom tag.
        /// </summary>
        public object Tag;
    }
}

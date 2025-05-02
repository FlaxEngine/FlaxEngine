// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct TextLayoutOptions
    {
        /// <summary>
        /// Gets the default layout.
        /// </summary>
        public static TextLayoutOptions Default => new TextLayoutOptions
        {
            Bounds = new Rectangle(0, 0, float.MaxValue, float.MaxValue),
            Scale = 1.0f,
            BaseLinesGapScale = 1.0f,
        };
    }
}

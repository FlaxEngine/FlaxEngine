// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Set of static properties for the surface.
    /// </summary>
    [HideInEditor]
    public static class Constants
    {
        /// <summary>
        /// The node close button size.
        /// </summary>
        public const float NodeCloseButtonSize = 12.0f;

        /// <summary>
        /// The node close button margin from the edges.
        /// </summary>
        public const float NodeCloseButtonMargin = 2.0f;

        /// <summary>
        /// The node header height.
        /// </summary>
        public const float NodeHeaderSize = 28.0f;

        /// <summary>
        /// The node footer height.
        /// </summary>
        public const float NodeFooterSize = 4.0f;

        /// <summary>
        /// The node left margin.
        /// </summary>
        public const float NodeMarginX = 5.0f;

        /// <summary>
        /// The node right margin.
        /// </summary>
        public const float NodeMarginY = 5.0f;

        /// <summary>
        /// The box position offset on the x axis.
        /// </summary>
        public const float BoxOffsetX = 2.0f;

        /// <summary>
        /// The box size (with and height).
        /// </summary>
        public const float BoxSize = 20.0f;

        /// <summary>
        /// The node layout offset on the y axis (height of the boxes rows, etc.). It's used to make the design more consistent.
        /// </summary>
        public const float LayoutOffsetY = 20.0f;
    }
}

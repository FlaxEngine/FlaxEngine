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
        public const float NodeCloseButtonSize = 16.0f;

        /// <summary>
        /// The node close button margin from the edges.
        /// </summary>
        public const float NodeCloseButtonMargin = 2.0f;

        /// <summary>
        /// The node header height.
        /// </summary>
        public const float NodeHeaderHeight = 20.0f;

        /// <summary>
        /// The scale of the header text.
        /// </summary>
        public const float NodeHeaderTextScale = 0.65f;

        /// <summary>
        /// The node footer height.
        /// </summary>
        public const float NodeFooterSize = 2.0f;

        /// <summary>
        /// The horizontal node margin.
        /// </summary>
        public const float NodeMarginX = 8.0f;

        /// <summary>
        /// The vertical node right margin.
        /// </summary>
        public const float NodeMarginY = 7.0f;

        /// <summary>
        /// The box position offset on the x axis.
        /// </summary>
        public const float BoxOffsetX = 0.0f;

        /// <summary>
        /// The width of the row that is started by a box.
        /// </summary>
        public const float BoxRowHeight = 19.0f;

        /// <summary>
        /// The box size (with and height).
        /// </summary>
        public const float BoxSize = 13.0f;

        /// <summary>
        /// The node layout offset on the y axis (height of the boxes rows, etc.). It's used to make the design more consistent.
        /// </summary>
        public const float LayoutOffsetY = 24.0f;

        /// <summary>
        /// The offset between the box text and the box
        /// </summary>
        public const float BoxTextOffset = 4.0f;

        /// <summary>
        /// The width of the rectangle used to draw the box text.
        /// </summary>
        public const float BoxTextRectWidth = 500.0f;

        /// <summary>
        /// The scale of text of boxes.
        /// </summary>
        public const float BoxTextScale = 1.175f;
    }
}

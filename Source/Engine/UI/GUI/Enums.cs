// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// UI control anchors presets.
    /// </summary>
    public enum AnchorPresets
    {
        /// <summary>
        /// The empty preset.
        /// </summary>
        Custom,

        /// <summary>
        /// The top left corner of the parent control.
        /// </summary>
        TopLeft,

        /// <summary>
        /// The center of the top edge of the parent control.
        /// </summary>
        TopCenter,

        /// <summary>
        /// The top right corner of the parent control.
        /// </summary>
        TopRight,

        /// <summary>
        /// The middle of the left edge of the parent control.
        /// </summary>
        MiddleLeft,

        /// <summary>
        /// The middle center! Right in the middle of the parent control.
        /// </summary>
        MiddleCenter,

        /// <summary>
        /// The middle of the right edge of the parent control.
        /// </summary>
        MiddleRight,

        /// <summary>
        /// The bottom left corner of the parent control.
        /// </summary>
        BottomLeft,

        /// <summary>
        /// The center of the bottom edge of the parent control.
        /// </summary>
        BottomCenter,

        /// <summary>
        /// The bottom right corner of the parent control.
        /// </summary>
        BottomRight,

        /// <summary>
        /// The vertical stretch on the left of the parent control.
        /// </summary>
        VerticalStretchLeft,

        /// <summary>
        /// The vertical stretch on the center of the parent control.
        /// </summary>
        VerticalStretchCenter,

        /// <summary>
        /// The vertical stretch on the right of the parent control.
        /// </summary>
        VerticalStretchRight,

        /// <summary>
        /// The horizontal stretch on the top of the parent control.
        /// </summary>
        HorizontalStretchTop,

        /// <summary>
        /// The horizontal stretch in the middle of the parent control.
        /// </summary>
        HorizontalStretchMiddle,

        /// <summary>
        /// The horizontal stretch on the bottom of the parent control.
        /// </summary>
        HorizontalStretchBottom,

        /// <summary>
        /// All parent control edges.
        /// </summary>
        StretchAll,
    }

    /// <summary>
    /// Specifies which scroll bars will be visible on a control
    /// </summary>
    [Flags]
    public enum ScrollBars
    {
        /// <summary>
        /// Don't use scroll bars.
        /// </summary>
        [Tooltip("Don't use scroll bars.")]
        None = 0,

        /// <summary>
        /// Use horizontal scrollbar.
        /// </summary>
        [Tooltip("Use horizontal scrollbar.")]
        Horizontal = 1,

        /// <summary>
        /// Use vertical scrollbar.
        /// </summary>
        [Tooltip("Use vertical scrollbar.")]
        Vertical = 2,

        /// <summary>
        /// Use horizontal and vertical scrollbar.
        /// </summary>
        [Tooltip("Use horizontal and vertical scrollbar.")]
        Both = Horizontal | Vertical
    }

    /// <summary>
    /// The drag item positioning modes.
    /// </summary>
    public enum DragItemPositioning
    {
        /// <summary>
        /// The none.
        /// </summary>
        None = 0,

        /// <summary>
        /// At the item.
        /// </summary>
        At,

        /// <summary>
        /// Above the item (near the upper/left edge).
        /// </summary>
        Above,

        /// <summary>
        /// Below the item (near the bottom/right edge)
        /// </summary>
        Below
    }

    /// <summary>
    /// Specifies the orientation of controls or elements of controls
    /// </summary>
    public enum Orientation
    {
        /// <summary>
        /// The horizontal.
        /// </summary>
        Horizontal = 0,

        /// <summary>
        /// The vertical.
        /// </summary>
        Vertical = 1,
    }

    /// <summary>
    /// The navigation directions in the user interface layout.
    /// </summary>
    public enum NavDirection
    {
        /// <summary>
        /// No direction to skip navigation.
        /// </summary>
        None,

        /// <summary>
        /// The up direction.
        /// </summary>
        Up,

        /// <summary>
        /// The down direction.
        /// </summary>
        Down,

        /// <summary>
        /// The left direction.
        /// </summary>
        Left,

        /// <summary>
        /// The right direction.
        /// </summary>
        Right,

        /// <summary>
        /// The next item (right with layout wrapping).
        /// </summary>
        Next,
        
        /// <summary>
        /// The previous item (left with layout wrapping).
        /// </summary>
        Previous,
    }
}

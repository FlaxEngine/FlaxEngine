// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The styling descriptor for the text block.
    /// </summary>
    public struct TextBlockStyle
    {
        /// <summary>
        /// Text block alignments modes.
        /// </summary>
        public enum Alignments
        {
            /// <summary>
            /// Block will be aligned to the baseline of the text (vertically).
            /// </summary>
            Baseline,

            /// <summary>
            /// Block will be aligned to the top edge of the line (vertically).
            /// </summary>
            Top = 1,
            
            /// <summary>
            /// Block will be aligned to center of the line (vertically).
            /// </summary>
            Middle = 2,
            
            /// <summary>
            /// Block will be aligned to the bottom edge of the line (vertically).
            /// </summary>
            Bottom = 4,

            /// <summary>
            /// Block will be aligned to the left edge of the layout (horizontally).
            /// </summary>
            Left = 8,
            
            /// <summary>
            /// Block will be aligned to center of the layout (horizontally).
            /// </summary>
            Center = 16,
            
            /// <summary>
            /// Block will be aligned to the right edge of the layout (horizontally).
            /// </summary>
            Right = 32,

            /// <summary>
            /// Mask with vertical alignment flags.
            /// </summary>
            [HideInEditor]
            VerticalMask = Top | Middle | Bottom,

            /// <summary>
            /// Mask with horizontal alignment flags.
            /// </summary>
            [HideInEditor]
            HorizontalMask = Left | Center | Right,
        }

        /// <summary>
        /// The text font.
        /// </summary>
        [EditorOrder(0)]
        public FontReference Font;

        /// <summary>
        /// The custom material for the text rendering (must be GUI domain).
        /// </summary>
        [EditorOrder(10)]
        public MaterialBase CustomMaterial;

        /// <summary>
        /// The text color (tint and opacity).
        /// </summary>
        [EditorOrder(20)]
        public Color Color;

        /// <summary>
        /// The text shadow color (tint and opacity). Transparent color disables shadow drawing.
        /// </summary>
        [EditorOrder(30)]
        public Color ShadowColor;

        /// <summary>
        /// The text shadow offset from the text location. Set to zero to disable shadow drawing.
        /// </summary>
        [EditorOrder(40)]
        public Float2 ShadowOffset;

        /// <summary>
        /// The background brush for the text range.
        /// </summary>
        [EditorOrder(45)]
        public IBrush BackgroundBrush;

        /// <summary>
        /// The background brush for the selected text range.
        /// </summary>
        [EditorOrder(50)]
        public IBrush BackgroundSelectedBrush;

        /// <summary>
        /// The underline line brush.
        /// </summary>
        [EditorOrder(60)]
        public IBrush UnderlineBrush;

        /// <summary>
        /// The text block alignment.
        /// </summary>
        [EditorOrder(100)]
        public Alignments Alignment;
    }
}

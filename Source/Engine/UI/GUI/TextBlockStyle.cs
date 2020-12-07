// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The styling descriptor for the text block.
    /// </summary>
    public struct TextBlockStyle
    {
        /// <summary>
        /// The text font.
        /// </summary>
        [EditorOrder(0), Tooltip("The text font.")]
        public FontReference Font;

        /// <summary>
        /// The custom material for the text rendering (must be GUI domain).
        /// </summary>
        [EditorOrder(10), Tooltip("The custom material for the text rendering (must be GUI domain).")]
        public MaterialBase CustomMaterial;

        /// <summary>
        /// The text color (tint and opacity).
        /// </summary>
        [EditorOrder(20), Tooltip("The text color (tint and opacity).")]
        public Color Color;

        /// <summary>
        /// The text shadow color (tint and opacity). Set to transparent to disable shadow drawing.
        /// </summary>
        [EditorOrder(30), Tooltip("The text shadow color (tint and opacity). Set to transparent to disable shadow drawing.")]
        public Color ShadowColor;

        /// <summary>
        /// The text shadow offset from the text location. Set to zero to disable shadow drawing.
        /// </summary>
        [EditorOrder(40), Tooltip("The text shadow offset from the text location. Set to zero to disable shadow drawing.")]
        public Vector2 ShadowOffset;

        /// <summary>
        /// The background brush for the selected text range.
        /// </summary>
        [EditorOrder(50), Tooltip("The background brush for the selected text range.")]
        public IBrush BackgroundSelectedBrush;

        /// <summary>
        /// The underline line brush.
        /// </summary>
        [EditorOrder(60), Tooltip("The underline line brush.")]
        public IBrush UnderlineBrush;
    }
}

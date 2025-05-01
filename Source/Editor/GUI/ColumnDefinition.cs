// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Table column descriptor.
    /// </summary>
    [HideInEditor]
    public class ColumnDefinition
    {
        /// <summary>
        /// Converts raw cell value to the string used by the column formatting policy.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The value string.</returns>
        public delegate string ValueFormatDelegate(object value);

        /// <summary>
        /// True if use expand/collapse rows feature for this column. See <see cref="Row.Depth"/> property which is used to describe the rows hierarchy.
        /// </summary>
        public bool UseExpandCollapseMode;

        /// <summary>
        /// The cell text alignment horizontally.
        /// </summary>
        public TextAlignment CellAlignment = TextAlignment.Far;

        /// <summary>
        /// The column title.
        /// </summary>
        public string Title;

        /// <summary>
        /// The title font.
        /// </summary>
        public Font TitleFont;

        /// <summary>
        /// The column title text color.
        /// </summary>
        public Color TitleColor = Color.White;

        /// <summary>
        /// The column title background color.
        /// </summary>
        public Color TitleBackgroundColor = Color.Brown;

        /// <summary>
        /// The column title horizontal text alignment
        /// </summary>
        public TextAlignment TitleAlignment = TextAlignment.Near;

        /// <summary>
        /// The column title margin.
        /// </summary>
        public Margin TitleMargin = new Margin(4, 4, 0, 0);

        /// <summary>
        /// The minimum size (in pixels) of the column.
        /// </summary>
        public float MinSize = 10.0f;

        /// <summary>
        /// The minimum size percentage of the column (in range 0-100).
        /// </summary>
        public float MinSizePercentage = 0.0f;

        /// <summary>
        /// The maximum size (in pixels) of the column.
        /// </summary>
        public float MaxSize = float.MaxValue;

        /// <summary>
        /// The maximum size percentage of the column (in range 0-100).
        /// </summary>
        public float MaxSizePercentage = 1.0f;

        /// <summary>
        /// The value formatting delegate.
        /// </summary>
        public ValueFormatDelegate FormatValue;

        /// <summary>
        /// Clamps the size of the column (in percentage size of the table).
        /// </summary>
        /// <param name="value">The percentage size of the column (split value).</param>
        /// <param name="tableSize">Size of the table (width in pixels).</param>
        /// <returns>The clamped percentage size of the column (split value).</returns>
        public float ClampColumnSize(float value, float tableSize)
        {
            float width = Mathf.Clamp(value, MinSizePercentage, MaxSizePercentage) * tableSize;
            return Mathf.Clamp(width, MinSize, MaxSize) / tableSize;
        }
    }
}

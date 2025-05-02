// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Inserts a header control with a custom text into the editor layout.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class HeaderAttribute : Attribute
    {
        /// <summary>
        /// The header text.
        /// </summary>
        public string Text;

        /// <summary>
        /// The custom header font size.
        /// </summary>
        public int FontSize;

        /// <summary>
        /// The custom header color (as 32-bit uint in RGB order, bottom bits contain Blue).
        /// </summary>
        public uint Color;

        /// <summary>
        /// Initializes a new instance of the <see cref="HeaderAttribute"/> class.
        /// </summary>
        public HeaderAttribute()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="HeaderAttribute"/> class.
        /// </summary>
        /// <param name="text">The header text.</param>
        /// <param name="fontSize">The header text font size (-1 to use default which is 14).</param>
        /// <param name="color">The header color (as 32-bit uint, 0 to use default).</param>
        public HeaderAttribute(string text, int fontSize = -1, uint color = 0)
        {
            Text = text;
            FontSize = fontSize;
            Color = color;
        }
    }
}

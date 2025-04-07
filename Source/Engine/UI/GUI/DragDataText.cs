// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The drag and drop text data.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.DragData" />
    public class DragDataText : DragData
    {
        /// <summary>
        /// The text.
        /// </summary>
        public readonly string Text;

        /// <summary>
        /// Initializes a new instance of the <see cref="DragDataText"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        public DragDataText(string text)
        {
            Text = text;
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The textbox element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class TextBoxElement : LayoutElement
    {
        /// <summary>
        /// The text box.
        /// </summary>
        public readonly TextBox TextBox;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        public string Text
        {
            get => TextBox.Text;
            set => TextBox.Text = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TextBoxElement"/> class.
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        public TextBoxElement(bool isMultiline = false)
        {
            TextBox = new TextBox(isMultiline, 0, 0);
            if (isMultiline)
                TextBox.Height = TextBox.DefaultHeight * 4;
        }

        /// <inheritdoc />
        public override Control Control => TextBox;
    }
}

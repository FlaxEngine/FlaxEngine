// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The button element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class ButtonElement : LayoutElement
    {
        /// <summary>
        /// The button.
        /// </summary>
        public readonly Button Button = new Button();

        /// <summary>
        /// Initializes the element.
        /// </summary>
        /// <param name="text">The text.</param>
        public void Init(string text)
        {
            Button.Text = text;
        }

        /// <summary>
        /// Initializes the element.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="color">The color.</param>
        public void Init(string text, Color color)
        {
            Button.Text = text;
            Button.SetColors(color);
        }

        /// <inheritdoc />
        public override Control Control => Button;
    }
}

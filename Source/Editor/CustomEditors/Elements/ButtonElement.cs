// Copyright (c) Wojciech Figat. All rights reserved.

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

        /// <inheritdoc />
        public override Control Control => Button;
    }
}

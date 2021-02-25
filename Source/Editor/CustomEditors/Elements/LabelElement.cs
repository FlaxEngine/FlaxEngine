// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The label element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class LabelElement : LayoutElement
    {
        /// <summary>
        /// The label.
        /// </summary>
        public readonly Label Label;

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckBoxElement"/> class.
        /// </summary>
        public LabelElement()
        {
            Label = new Label(0, 0, 100, 18)
            {
                HorizontalAlignment = TextAlignment.Near
            };
            // TODO: auto height for label
        }

        /// <inheritdoc />
        public override Control Control => Label;
    }
}

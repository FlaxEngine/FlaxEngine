// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The combobox element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class ComboBoxElement : LayoutElement
    {
        /// <summary>
        /// The combo box.
        /// </summary>
        public readonly ComboBox ComboBox;

        /// <summary>
        /// Initializes a new instance of the <see cref="ComboBoxElement"/> class.
        /// </summary>
        public ComboBoxElement()
        {
            ComboBox = new ComboBox();
        }

        /// <inheritdoc />
        public override Control Control => ComboBox;
    }
}

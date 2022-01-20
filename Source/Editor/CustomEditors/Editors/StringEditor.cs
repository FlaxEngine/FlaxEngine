// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit string properties.
    /// </summary>
    [CustomEditor(typeof(string)), DefaultEditor]
    public sealed class StringEditor : CustomEditor
    {
        private TextBoxElement _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            bool isMultiLine = false;

            var attributes = Values.GetAttributes();
            var multiLine = attributes?.FirstOrDefault(x => x is MultilineTextAttribute);
            if (multiLine != null)
            {
                isMultiLine = true;
            }

            _element = layout.TextBox(isMultiLine);
            _element.TextBox.EditEnd += () => SetValue(_element.Text);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                _element.TextBox.Text = string.Empty;
                _element.TextBox.WatermarkText = "Different values";
            }
            else
            {
                _element.TextBox.Text = (string)Values[0];
                _element.TextBox.WatermarkText = string.Empty;
            }
        }
    }
}

// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Guid properties.
    /// </summary>
    [CustomEditor(typeof(Guid)), DefaultEditor]
    public sealed class GuidEditor : CustomEditor
    {
        private TextBoxElement _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = layout.TextBox();
            _element.TextBox.EditEnd += OnEditEnd;
        }

        private void OnEditEnd()
        {
            if (Guid.TryParse(_element.Text, out Guid value))
            {
                SetValue(value);
            }
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
                _element.TextBox.Text = ((Guid)Values[0]).ToString("D");
                _element.TextBox.WatermarkText = string.Empty;
            }
        }
    }
}

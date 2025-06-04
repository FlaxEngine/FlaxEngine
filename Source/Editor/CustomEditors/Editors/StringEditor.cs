// Copyright (c) Wojciech Figat. All rights reserved.

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
        private string _watermarkText;
        private Color _watermarkColor;
        private Color _defaultWatermarkColor;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            bool isMultiLine = false;
            _watermarkText = string.Empty;

            var attributes = Values.GetAttributes();
            var multiLine = attributes?.FirstOrDefault(x => x is MultilineTextAttribute);
            var watermarkAttribute = attributes?.FirstOrDefault(x => x is WatermarkAttribute);
            if (multiLine != null)
            {
                isMultiLine = true;
            }

            _element = layout.TextBox(isMultiLine);
            _watermarkColor = _defaultWatermarkColor = _element.TextBox.WatermarkTextColor;
            if (watermarkAttribute is WatermarkAttribute watermark)
            {
                _watermarkText = watermark.WatermarkText;
                var watermarkColor = watermark.WatermarkColor > 0 ? Color.FromRGB(watermark.WatermarkColor) : FlaxEngine.GUI.Style.Current.ForegroundDisabled;
                _watermarkColor = watermarkColor;
                _element.TextBox.WatermarkText = watermark.WatermarkText;
                _element.TextBox.WatermarkTextColor = watermarkColor;
            }
            _element.TextBox.EditEnd += () => SetValue(_element.Text);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                _element.TextBox.Text = string.Empty;
                _element.TextBox.WatermarkTextColor = _defaultWatermarkColor;
                _element.TextBox.WatermarkText = "Different values";
            }
            else
            {
                _element.TextBox.Text = (string)Values[0];
                _element.TextBox.WatermarkTextColor = _watermarkColor;
                _element.TextBox.WatermarkText = _watermarkText;
            }
        }
    }
}

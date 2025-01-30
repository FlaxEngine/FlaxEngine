// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for picking terrain layers. Instead of choosing bit mask or layer index it shows a combo box with simple layer picking by name.
    /// </summary>
    public sealed class TerrainLayerEditor : CustomEditor
    {
        private ComboBoxElement element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            element = layout.ComboBox();
            element.ComboBox.SetItems(LayersAndTagsSettings.GetCurrentTerrainLayers());
            element.ComboBox.SelectedIndex = (int)Values[0];
            element.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            int value = comboBox.SelectedIndex;
            if (value == -1)
                value = 0;

            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            element.ComboBox.SelectedIndex = (int)Values[0];
        }
    }
}

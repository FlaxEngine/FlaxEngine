// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="LayersMask"/>.
    /// </summary>
    [CustomEditor(typeof(LayersMask)), DefaultEditor]
    internal class LayersMaskEditor : CustomEditor
    {
        private CheckBox[] _checkBoxes;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var layers = LayersAndTagsSettings.GetCurrentLayers();
            if (layers == null || layers.Length == 0)
            {
                layout.Label("Missing layers and tags settings");
                return;
            }

            _checkBoxes = new CheckBox[layers.Length];
            for (int i = 0; i < layers.Length; i++)
            {
                var layer = layers[i];
                var property = layout.AddPropertyItem(layer);
                var checkbox = property.Checkbox().CheckBox;
                UpdateCheckbox(checkbox, i);
                checkbox.Tag = i;
                checkbox.StateChanged += OnCheckboxStateChanged;
                _checkBoxes[i] = checkbox;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _checkBoxes = null;

            base.Deinitialize();
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            if (_checkBoxes != null)
            {
                for (int i = 0; i < _checkBoxes.Length; i++)
                {
                    UpdateCheckbox(_checkBoxes[i], i);
                }
            }

            base.Refresh();
        }

        private void OnCheckboxStateChanged(CheckBox checkBox)
        {
            var i = (int)checkBox.Tag;
            var value = (LayersMask)Values[0];
            var mask = 1u << i;
            value.Mask &= ~mask;
            value.Mask |= checkBox.Checked ? mask : 0;
            SetValue(value);
        }

        private void UpdateCheckbox(CheckBox checkbox, int i)
        {
            for (var j = 0; j < Values.Count; j++)
            {
                var value = (((LayersMask)Values[j]).Mask & (1 << i)) != 0;
                if (j == 0)
                {
                    checkbox.Checked = value;
                }
                else if (checkbox.State != CheckBoxState.Intermediate)
                {
                    if (checkbox.Checked != value)
                        checkbox.State = CheckBoxState.Intermediate;
                }
            }
        }
    }
}

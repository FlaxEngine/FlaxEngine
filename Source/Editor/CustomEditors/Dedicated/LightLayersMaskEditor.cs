// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content.Settings;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor Light Layer Masks.
    /// </summary>
    /// TODO: Need a LightLayersMask - Source/Engine/Core/Types/LightLayersMask.h & Source/Engine/Core/Types/LightLayersMask.cs 
    //[CustomEditor(typeof(LayersMask)), DefaultEditor]
    internal class LightLayersMaskEditor : CustomEditor
    {
        private List<CheckBox> _checkBoxes;
        private bool _refreshing;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var layers = LayersAndTagsSettings.GetCurrentLightLayers();
            if (layers == null || layers.Length == 0)
            {
                layout.Label("Missing layers and tags settings");
                return;
            }

            _checkBoxes = new List<CheckBox>();
            
            for (int i = 0; i < layers.Length; i++)
            {
                var layer = layers[i];
                if (string.IsNullOrEmpty(layer))
                    continue;
                var property = layout.AddPropertyItem($"{i}: {layer}");
                var checkbox = property.Checkbox().CheckBox;
                UpdateCheckbox(checkbox, i);
                checkbox.Tag = i;
                checkbox.StateChanged += OnCheckboxStateChanged;
                _checkBoxes.Add(checkbox);
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
            _refreshing = true;
            try
            {
                if (_checkBoxes != null)
                {
                    foreach (var checkBox in _checkBoxes)
                        UpdateCheckbox(checkBox, (int)checkBox.Tag);
                }
            }
            finally
            {
                _refreshing = false;
            }

            base.Refresh();
        }

        private void OnCheckboxStateChanged(CheckBox checkBox)
        {
            if (_refreshing)
                return;

            uint bit = 1u << (int)checkBox.Tag;
            bool enabled = checkBox.Checked;

            if (Values.Count == 1)
            {
                uint current = (uint)Values[0];
                SetValue(enabled ? current | bit : current & ~bit);
                return;
            }

            var updated = new uint[Values.Count];
            for (int i = 0; i < updated.Length; i++)
            {
                uint current = (uint)Values[i];
                updated[i] = enabled ? current | bit : current & ~bit;
            }
            SetValue(updated);
        }

        private void UpdateCheckbox(CheckBox checkBox, int slot)
        {
            uint bit = 1u << slot;
            bool enabled = ((uint)Values[0] & bit) != 0;
            for (int i = 1; i < Values.Count; i++)
            {
                bool otherEnabled = ((uint)Values[i] & bit) != 0;
                if (otherEnabled != enabled)
                {
                    checkBox.State = CheckBoxState.Intermediate;
                    return;
                }
            }

            checkBox.Checked = enabled;
        }
    }
}

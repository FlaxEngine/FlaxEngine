// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using System.Collections.Generic;
using System.Linq;
using System.Xml.Linq;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="TextureGroup"/> index.
    /// </summary>
    internal class TextureGroupEditor : CustomEditor
    {
        private const string AddOrEditOption = "Add or edit Texture Groups...";
        private ComboBoxElement _element;
        private bool _updatingItems;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = layout.ComboBox();
            UpdateItems((int)Values[0]);
            _element.ComboBox.PopupShowing += OnPopupShowing;
            _element.ComboBox.PopupShown += OnPopupShown;
            _element.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnPopupShowing(ComboBox comboBox)
        {
            UpdateItems(HasDifferentValues ? -1 : (int)Values[0]);
        }

        private void OnPopupShown(ComboBox comboBox)
        {
            var addOrEditOption = (ContextMenuButton)comboBox.Popup.Items.FirstOrDefault(x => x is ContextMenuButton b && b.Text == AddOrEditOption);
            if (addOrEditOption != null)
                addOrEditOption.Icon = Editor.Instance.Icons.Settings12;
        }

        private void UpdateItems(int selectedValue)
        {
            _updatingItems = true;
            var layers = LayersAndTagsSettings.GetCurrentLayers();
            _element.ComboBox.ClearItems();
            _element.ComboBox.AddItem("None");
            var groups = GameSettings.Load<StreamingSettings>();
            if (groups?.TextureGroups != null)
            {
                var tooltips = new string[groups.TextureGroups.Length + 2];
                tooltips[0] = "No Texture Group set (uses -1 index).";
                tooltips[groups.TextureGroups.Length + 1] = "Opens Streaming Settings asset to view/edit Texture Groups configuration.";
                for (int i = 0; i < groups.TextureGroups.Length; i++)
                {
                    ref var group = ref groups.TextureGroups[i];
                    _element.ComboBox.AddItem(group.Name);
                    tooltips[i + 1] = $"Sampler: {group.SamplerFilter}, Quality: {group.Quality}";
                    if (group.SamplerFilter == GPUSamplerFilter.Anisotropic)
                        tooltips[i + 1] += $", Max Anisotropy: {group.MaxAnisotropy}";
                }
                _element.ComboBox.Tooltips = tooltips;
            }
            _element.ComboBox.AddItem(AddOrEditOption);
            int selectedIndex = selectedValue + 1; // Skip 'None' element
            _element.ComboBox.SelectedIndex = selectedIndex >= 0 && selectedIndex < layers.Length ? selectedIndex : -1;
            _updatingItems = false;
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            if (_updatingItems)
                return;

            if (comboBox.SelectedIndex + 1 == comboBox.Items.Count)
            {
                Editor.Instance.ContentEditing.OpenSettings<StreamingSettings>();
                return;
            }

            var value = comboBox.HasSelection ? comboBox.SelectedIndex - 1 : -1;
            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values on many actor selected
            }
            else
            {
                UpdateItems((int)Values[0]);
            }
        }
    }
}

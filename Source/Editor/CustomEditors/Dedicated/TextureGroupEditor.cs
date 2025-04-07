// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="TextureGroup"/> index.
    /// </summary>
    internal class TextureGroupEditor : CustomEditor
    {
        private ComboBoxElement _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = layout.ComboBox();
            _element.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;
            _element.ComboBox.AddItem("None");
            var groups = GameSettings.Load<StreamingSettings>();
            if (groups?.TextureGroups != null)
            {
                for (int i = 0; i < groups.TextureGroups.Length; i++)
                {
                    _element.ComboBox.AddItem(groups.TextureGroups[i].Name);
                }
            }
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            var value = comboBox.HasSelection ? comboBox.SelectedIndex - 1 : -1;
            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            var value = (int)Values[0];
            _element.ComboBox.SelectedIndex = value + 1;
        }
    }
}

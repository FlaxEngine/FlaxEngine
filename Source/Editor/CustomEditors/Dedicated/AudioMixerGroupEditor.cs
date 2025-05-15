// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="AudioMixerGroup"/> index.
    /// </summary>
    internal class AudioMixerGroupEditor : CustomEditor
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
            var groups = GameSettings.Load<AudioSettings>();
            if (groups?.AudioMixerGroups != null)
            {
                for (int i = 0; i < groups.AudioMixerGroups.Length; i++)
                {
                    _element.ComboBox.AddItem(groups.AudioMixerGroups[i].Name);
                }
            }
            _element.ComboBox.SelectedIndex = 0;
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            var value = comboBox.HasSelection ? comboBox.SelectedIndex : 0;
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

// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="AudioMixerGroup"/> index.
    /// </summary>
    internal class AudioMixerGroupEditor : CustomEditor
    {
        private ComboBoxElement _element;
        private int AudioMixerIndex;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = layout.ComboBox();
            _element.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;
            _element.ComboBox.AddItem("Master");
            var groups = GameSettings.Load<AudioSettings>();
            if (groups?.AudioMixerGroups != null)
            {
                groups.AudioMixerGroups.ForEach(mixer => _element.ComboBox.AddItem(mixer.Name));
            }
            _element.ComboBox.SelectedIndex = AudioMixerIndex;
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            if (comboBox.HasSelection)
            {
                AudioMixerIndex= comboBox.SelectedIndex;
                SetValue(AudioMixerIndex);
            }
        }

        public string GetAudioMixerGroup() => _element.ComboBox.SelectedItem;

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();
            _element.ComboBox.SelectedIndex = (AudioMixerIndex = (int)Values[0]) + 1;
        }
    }
}

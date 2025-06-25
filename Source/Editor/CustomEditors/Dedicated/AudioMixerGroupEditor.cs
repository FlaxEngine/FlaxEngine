// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.Utilities;
using System.Linq;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="AudioMixerGroup"/> index.
    /// </summary>
    internal class AudioMixerGroupEditor : CustomEditor
    {
        private ComboBoxElement _element;
        int AudioMixerIndex;
        public string MixerChannel;

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
            _element.ComboBox.SelectedIndex = AudioMixerIndex = _element.ComboBox.Items.FindIndex(item => item == MixerChannel);
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

        void UpdateAudioMixerGroup()
        {
            var groups = GameSettings.Load<AudioSettings>();
            if ((_element.ComboBox.Items.Count - 1) == groups?.AudioMixerGroups.Length) return;
            if (groups?.AudioMixerGroups != null)
            {
                foreach (string NameGroup in _element.ComboBox.Items)
                {
                    groups.AudioMixerGroups.Where(groupElement => groupElement.Name != NameGroup).ForEach(mixerName => _element.ComboBox.AddItem(mixerName.Name));
                }
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            UpdateAudioMixerGroup();
            _element.ComboBox.SelectedIndex = (AudioMixerIndex = (int)Values[0]) + 1;
        }
    }
}

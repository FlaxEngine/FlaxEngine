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
        private int AudioMixerIndex;

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
                foreach (var mixer in groups.AudioMixerGroups)
                {
                    _element.ComboBox.AddItem(mixer.Name);
                }
            }
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            AudioMixerIndex = comboBox.HasSelection ? comboBox.SelectedIndex : 0;
            SetValue(AudioMixerIndex);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

           AudioMixerIndex = (int)Values[0];
            _element.ComboBox.SelectedIndex = AudioMixerIndex + 1;
        }
    }
}

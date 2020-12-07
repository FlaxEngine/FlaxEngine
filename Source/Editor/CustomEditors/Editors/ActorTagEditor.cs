// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for picking actor tag. Instead of choosing tag index or entering tag text it shows a combo box with simple tag picking by name.
    /// </summary>
    public sealed class ActorTagEditor : CustomEditor
    {
        private ComboBoxElement element;
        private const string NoTagText = "Untagged";

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            element = layout.ComboBox();
            element.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;

            // Set tag names
            element.ComboBox.AddItem(NoTagText);
            element.ComboBox.AddItems(LayersAndTagsSettings.GetCurrentTags());
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            string value = comboBox.SelectedItem;
            if (value == NoTagText)
                value = string.Empty;
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
                string value = (string)Values[0];
                if (string.IsNullOrEmpty(value))
                    value = NoTagText;
                element.ComboBox.SelectedItem = value;
            }
        }
    }
}

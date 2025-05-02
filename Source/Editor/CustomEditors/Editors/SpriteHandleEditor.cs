// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit <see cref="SpriteHandle"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
    [CustomEditor(typeof(SpriteHandle)), DefaultEditor]
    public class SpriteHandleEditor : CustomEditor
    {
        private ComboBox _spritePicker;
        private ValueContainer _atlasValues;
        private ValueContainer _indexValues;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // Atlas
            var atlasField = typeof(SpriteHandle).GetField("Atlas");
            var atlasValues = new ValueContainer(new ScriptMemberInfo(atlasField), Values);
            layout.Property("Atlas", atlasValues, null, "The target atlas texture used as a sprite image source.");
            _atlasValues = atlasValues;

            // Sprite
            var spriteIndexField = typeof(SpriteHandle).GetField("Index");
            _indexValues = new ValueContainer(new ScriptMemberInfo(spriteIndexField), Values);
            var spriteLabel = layout.AddPropertyItem("Sprite", "The selected sprite from the atlas.");

            // Check state
            if (atlasValues.HasDifferentValues)
            {
                spriteLabel.Label("Different values");
                return;
            }
            var value = (SpriteAtlas)atlasValues[0];
            if (value == null)
            {
                spriteLabel.Label("Pick atlas first");
                return;
            }

            // TODO: don't stall use Refresh to rebuild UI when sprite atlas gets loaded
            if (value.WaitForLoaded())
            {
                return;
            }

            // List all sprites from the atlas asset
            var spritesCount = value.SpritesCount;
            var spritePicker = spriteLabel.ComboBox();
            spritePicker.ComboBox.Items.Capacity = spritesCount;
            for (int i = 0; i < spritesCount; i++)
            {
                spritePicker.ComboBox.AddItem(value.GetSprite(i).Name);
            }
            spritePicker.ComboBox.SupportMultiSelect = false;
            spritePicker.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;
            _spritePicker = spritePicker.ComboBox;
        }

        private void OnSelectedIndexChanged(ComboBox obj)
        {
            if (IsSetBlocked)
                return;

            SpriteHandle value;
            value.Atlas = (SpriteAtlas)_atlasValues[0];
            value.Index = _spritePicker.SelectedIndex;
            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues || _spritePicker == null)
                return;

            // Fetch the instance values
            _atlasValues.Refresh(Values);
            _indexValues.Refresh(Values);

            // Update selection
            int selectedIndex = -1;
            for (int i = 0; i < Values.Count; i++)
            {
                var idx = (int)_indexValues[i];
                if (idx != -1 && idx < _spritePicker.Items.Count)
                    selectedIndex = idx;
            }
            _spritePicker.SelectedIndex = selectedIndex;
        }

        /// <inheritdoc />
        protected override bool OnDirty(CustomEditor editor, object value, object token = null)
        {
            // Check if Atlas has been changed
            if (editor is AssetRefEditor)
            {
                // Rebuild layout to update the dropdown menu with sprites list
                RebuildLayoutOnRefresh();
            }

            return base.OnDirty(editor, value, token);
        }
    }
}

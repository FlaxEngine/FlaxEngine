// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit input event properties.
    /// </summary>
    [CustomEditor(typeof(InputEvent)), DefaultEditor]
    public class InputEventEditor : CustomEditor
    {
        private ComboBox _comboBox;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (LinkedLabel != null)
                LinkedLabel.SetupContextMenu += OnSetupContextMenu;
            var comboBoxElement = layout.ComboBox();
            _comboBox = comboBoxElement.ComboBox;
            var names = new List<string>();
            foreach (var mapping in Input.ActionMappings)
            {
                if (!names.Contains(mapping.Name))
                    names.Add(mapping.Name);
            }
            _comboBox.Items = names;
            if (Values[0] is InputEvent inputEvent && names.Contains(inputEvent.Name))
                _comboBox.SelectedItem = inputEvent.Name;
            _comboBox.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
        {
            var button = menu.AddButton("Set to null");
            button.Clicked += () => _comboBox.SelectedItem = null;
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            SetValue(comboBox.SelectedItem == null ? null : new InputEvent(comboBox.SelectedItem));
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
            }
            else
            {
                if (Values[0] is InputEvent inputEvent && _comboBox.Items.Contains(inputEvent.Name))
                    _comboBox.SelectedItem = inputEvent.Name;
                else
                    _comboBox.SelectedItem = null;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (LinkedLabel != null)
                LinkedLabel.SetupContextMenu -= OnSetupContextMenu;
            if (_comboBox != null)
                _comboBox.SelectedIndexChanged -= OnSelectedIndexChanged;
            _comboBox = null;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit input axis properties.
    /// </summary>
    [CustomEditor(typeof(InputAxis)), DefaultEditor]
    public class InputAxisEditor : CustomEditor
    {
        private ComboBox _comboBox;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            LinkedLabel.SetupContextMenu += OnSetupContextMenu;
            var comboBoxElement = layout.ComboBox();
            _comboBox = comboBoxElement.ComboBox;
            var names = new List<string>();
            foreach (var mapping in Input.AxisMappings)
            {
                if (!names.Contains(mapping.Name))
                    names.Add(mapping.Name);
            }
            _comboBox.Items = names;
            if (Values[0] is InputAxis inputAxis && names.Contains(inputAxis.Name))
                _comboBox.SelectedItem = inputAxis.Name;
            _comboBox.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
        {
            var button = menu.AddButton("Set to null");
            button.Clicked += () => _comboBox.SelectedItem = null;
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            SetValue(comboBox.SelectedItem == null ? null : new InputAxis(comboBox.SelectedItem));
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
            }
            else
            {
                if (Values[0] is InputAxis inputAxis && _comboBox.Items.Contains(inputAxis.Name))
                    _comboBox.SelectedItem = inputAxis.Name;
                else
                    _comboBox.SelectedItem = null;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (LinkedLabel != null)
                LinkedLabel.SetupContextMenu -= OnSetupContextMenu;
            if (_comboBox != null)
                _comboBox.SelectedIndexChanged -= OnSelectedIndexChanged;
            _comboBox = null;
        }
    }
}

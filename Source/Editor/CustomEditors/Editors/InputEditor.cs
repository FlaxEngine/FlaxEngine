// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit input event properties.
    /// </summary>
    [CustomEditor(typeof(InputEvent)), DefaultEditor]
    public class InputEventEditor : CustomEditor
    {
        private Dropdown _dropdown;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var dropdownElement = layout.Custom<Dropdown>();
            _dropdown = dropdownElement.CustomControl;
            var names = new List<LocalizedString>();
            foreach (var mapping in Input.ActionMappings)
            {
                if (!names.Contains(mapping.Name))
                    names.Add(mapping.Name);
            }
            _dropdown.Items = names;
            if (Values[0] is InputEvent inputEvent && names.Contains(inputEvent.Name))
                _dropdown.SelectedItem = inputEvent.Name;
            _dropdown.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnSelectedIndexChanged(Dropdown dropdown)
        {
            SetValue(new InputEvent(dropdown.SelectedItem));
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
                if (Values[0] is InputEvent inputEvent && _dropdown.Items.Contains(inputEvent.Name))
                    _dropdown.SelectedItem = inputEvent.Name;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (_dropdown != null)
                _dropdown.SelectedIndexChanged -= OnSelectedIndexChanged;
            _dropdown = null;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit input event properties.
    /// </summary>
    [CustomEditor(typeof(InputEventState)), DefaultEditor]
    public class InputEventStateEditor : CustomEditor
    {
        private Dropdown _dropdown;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var dropdownElement = layout.Custom<Dropdown>();
            _dropdown = dropdownElement.CustomControl;
            var names = new List<LocalizedString>();
            foreach (var mapping in Input.ActionMappings)
            {
                if (!names.Contains(mapping.Name))
                    names.Add(mapping.Name);
            }
            _dropdown.Items = names;
            if (Values[0] is InputEventState inputEvent && names.Contains(inputEvent.Name))
                _dropdown.SelectedItem = inputEvent.Name;
            _dropdown.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnSelectedIndexChanged(Dropdown dropdown)
        {
            SetValue(new InputEventState(dropdown.SelectedItem));
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
                if (Values[0] is InputEventState inputEvent && _dropdown.Items.Contains(inputEvent.Name))
                    _dropdown.SelectedItem = inputEvent.Name;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (_dropdown != null)
                _dropdown.SelectedIndexChanged -= OnSelectedIndexChanged;
            _dropdown = null;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit input axis properties.
    /// </summary>
    [CustomEditor(typeof(InputAxis)), DefaultEditor]
    public class InputAxisEditor : CustomEditor
    {
        private Dropdown _dropdown;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var dropdownElement = layout.Custom<Dropdown>();
            _dropdown = dropdownElement.CustomControl;
            var names = new List<LocalizedString>();
            foreach (var mapping in Input.AxisMappings)
            {
                if (!names.Contains(mapping.Name))
                    names.Add(mapping.Name);
            }
            _dropdown.Items = names;
            if (Values[0] is InputAxis inputAxis && names.Contains(inputAxis.Name))
                _dropdown.SelectedItem = inputAxis.Name;
            _dropdown.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnSelectedIndexChanged(Dropdown dropdown)
        {
            SetValue(new InputAxis(dropdown.SelectedItem));
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
                if (Values[0] is InputAxis inputAxis && _dropdown.Items.Contains(inputAxis.Name))
                    _dropdown.SelectedItem = inputAxis.Name;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (_dropdown != null)
                _dropdown.SelectedIndexChanged -= OnSelectedIndexChanged;
            _dropdown = null;
        }
    }
}

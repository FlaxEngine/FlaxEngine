
using System;
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
            var eventNames = new List<LocalizedString>();
            foreach (var mapping in Input.ActionMappings)
            {
                if (eventNames.Contains(mapping.Name))
                    continue;
                eventNames.Add(mapping.Name);
            }
            _dropdown.Items = eventNames;
            var value = Values[0] as InputEvent;
            if (value != null)
            {
                if (eventNames.Contains(value.Name))
                {
                    _dropdown.SelectedItem = value.Name;
                }
            }
            
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
                if (Values[0] is InputEvent eventValue)
                {
                    if (_dropdown.Items.Contains(eventValue.Name) && string.Equals(_dropdown.SelectedItem, eventValue.Name, StringComparison.Ordinal))
                    {
                        _dropdown.SelectedItem = eventValue.Name;
                    }
                }
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (_dropdown != null)
                _dropdown.SelectedIndexChanged -= OnSelectedIndexChanged;
            _dropdown = null;
            base.Deinitialize();
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
            var axisNames = new List<LocalizedString>();
            foreach (var mapping in Input.AxisMappings)
            {
                if (axisNames.Contains(mapping.Name))
                    continue;
                axisNames.Add(mapping.Name);
            }
            _dropdown.Items = axisNames;
            var value = Values[0] as InputAxis;
            if (value != null)
            {
                if (axisNames.Contains(value.Name))
                {
                    _dropdown.SelectedItem = value.Name;
                }
            }
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
                if (Values[0] is InputAxis axisValue)
                {
                    if (_dropdown.Items.Contains(axisValue.Name) && string.Equals(_dropdown.SelectedItem, axisValue.Name, StringComparison.Ordinal))
                    {
                        _dropdown.SelectedItem = axisValue.Name;
                    }
                }
            }
        }
        
        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (_dropdown != null)
                _dropdown.SelectedIndexChanged -= OnSelectedIndexChanged;
            _dropdown = null;
            base.Deinitialize();
        }
    }
}

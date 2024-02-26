// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit double value type properties.
    /// </summary>
    [CustomEditor(typeof(double)), DefaultEditor]
    public sealed class DoubleEditor : CustomEditor
    {
        private DoubleValueElement _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = null;

            // Try get limit attribute for value min/max range setting and slider speed
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    // Use double value editor with limit
                    var doubleValue = layout.DoubleValue();
                    doubleValue.SetLimits((LimitAttribute)limit);
                    doubleValue.ValueBox.ValueChanged += OnValueChanged;
                    doubleValue.ValueBox.SlidingEnd += ClearToken;
                    _element = doubleValue;
                    return;
                }
            }
            if (_element == null)
            {
                // Use double value editor
                var doubleValue = layout.DoubleValue();
                doubleValue.ValueBox.ValueChanged += OnValueChanged;
                doubleValue.ValueBox.SlidingEnd += ClearToken;
                _element = doubleValue;
            }
        }

        private void OnValueChanged()
        {
            var isSliding = _element.IsSliding;
            var token = isSliding ? this : null;
            SetValue(_element.Value, token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values for ValueBox<T>
            }
            else
            {
                var value = Values[0];
                if (value is double asDouble)
                    _element.Value = (float)asDouble;
                else if (value is float asFloat)
                    _element.Value = asFloat;
                else
                    throw new Exception(string.Format("Invalid value type {0}.", value?.GetType().ToString() ?? "<null>"));
            }
        }
    }
}

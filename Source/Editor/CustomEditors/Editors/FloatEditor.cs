// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit float value type properties.
    /// </summary>
    [CustomEditor(typeof(float)), DefaultEditor]
    public sealed class FloatEditor : CustomEditor
    {
        private IFloatValueEditor _element;

        /// <summary>
        /// Gets the element.
        /// </summary>
        public IFloatValueEditor Element => _element;

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
                var range = attributes.FirstOrDefault(x => x is RangeAttribute);
                if (range != null)
                {
                    // Use slider
                    var slider = layout.Slider();
                    slider.SetLimits((RangeAttribute)range);
                    slider.Slider.ValueChanged += OnValueChanged;
                    slider.Slider.SlidingEnd += ClearToken;
                    _element = slider;
                    return;
                }
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    // Use float value editor with limit
                    var floatValue = layout.FloatValue();
                    floatValue.SetLimits((LimitAttribute)limit);
                    floatValue.ValueBox.ValueChanged += OnValueChanged;
                    floatValue.ValueBox.SlidingEnd += ClearToken;
                    _element = floatValue;
                    return;
                }
            }
            if (_element == null)
            {
                // Use float value editor
                var floatValue = layout.FloatValue();
                floatValue.ValueBox.ValueChanged += OnValueChanged;
                floatValue.ValueBox.SlidingEnd += ClearToken;
                _element = floatValue;
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
                if (value is float asFloat)
                    _element.Value = asFloat;
                else if (value is double asDouble)
                    _element.Value = (float)asDouble;
                else
                    throw new Exception(string.Format("Invalid value type {0}.", value?.GetType().ToString() ?? "<null>"));
            }
        }
    }
}

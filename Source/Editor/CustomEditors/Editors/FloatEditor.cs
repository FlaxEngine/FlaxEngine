// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using Utils = FlaxEngine.Utils;

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
            var attributes = Values.GetAttributes();
            var range = (RangeAttribute)attributes?.FirstOrDefault(x => x is RangeAttribute);
            if (range != null)
            {
                // Use slider
                var slider = layout.Slider();
                slider.Slider.SetLimits(range);
                slider.Slider.ValueChanged += OnValueChanged;
                slider.Slider.SlidingEnd += ClearToken;
                _element = slider;
                return;
            }

            var floatValue = layout.FloatValue();
            floatValue.ValueBox.ValueChanged += OnValueChanged;
            floatValue.ValueBox.SlidingEnd += ClearToken;
            _element = floatValue;
            if (attributes != null)
            {
                var limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
                floatValue.SetLimits(limit);
                var valueCategory = ((ValueCategoryAttribute)attributes.FirstOrDefault(x => x is ValueCategoryAttribute))?.Category ?? Utils.ValueCategory.None;
                if (valueCategory != Utils.ValueCategory.None)
                {
                    floatValue.SetCategory(valueCategory);
                    if (LinkedLabel != null)
                    {
                        LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                        {
                            menu.AddSeparator();
                            var mb = menu.AddButton("Show formatted", bt => { floatValue.SetCategory(bt.Checked ? valueCategory : Utils.ValueCategory.None); });
                            mb.AutoCheck = true;
                            mb.Checked = floatValue.ValueBox.Category != Utils.ValueCategory.None;
                        };
                    }
                }
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
                else if (value is int asInt)
                    _element.Value = (float)asInt;
                else
                    throw new Exception(string.Format("Invalid value type {0}.", value?.GetType().ToString() ?? "<null>"));
            }
        }
    }
}

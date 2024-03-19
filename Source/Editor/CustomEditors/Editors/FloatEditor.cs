// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.ContextMenu;
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

            // Try get limit attribute for value min/max range setting and slider speed
            var attributes = Values.GetAttributes();
            var categoryAttribute = attributes.FirstOrDefault(x => x is NumberCategoryAttribute);
            var valueCategory = ((NumberCategoryAttribute)categoryAttribute)?.Category ?? Utils.ValueCategory.None;
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
                    floatValue.SetCategory(valueCategory);
                    floatValue.ValueBox.ValueChanged += OnValueChanged;
                    floatValue.ValueBox.SlidingEnd += ClearToken;
                    _element = floatValue;
                    LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                    {
                        menu.AddSeparator();
                        var mb = menu.AddButton("Show formatted", bt => { floatValue.SetCategory(bt.Checked ? valueCategory : Utils.ValueCategory.None);});
                        mb.AutoCheck = true;
                        mb.Checked = floatValue.ValueBox.Category != Utils.ValueCategory.None;
                    };
                    return;
                }
            }
            if (_element == null)
            {
                // Use float value editor
                var floatValue = layout.FloatValue();
                floatValue.SetCategory(valueCategory);
                floatValue.ValueBox.ValueChanged += OnValueChanged;
                floatValue.ValueBox.SlidingEnd += ClearToken;
                _element = floatValue;
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                {
                    menu.AddSeparator();
                    var mb = menu.AddButton("Show formatted", bt => { floatValue.SetCategory(bt.Checked ? valueCategory : Utils.ValueCategory.None);});
                    mb.AutoCheck = true;
                    mb.Checked = floatValue.ValueBox.Category != Utils.ValueCategory.None;
                };
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

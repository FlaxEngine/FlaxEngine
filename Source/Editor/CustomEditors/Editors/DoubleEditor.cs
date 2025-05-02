// Copyright (c) Wojciech Figat. All rights reserved.

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
            var doubleValue = layout.DoubleValue();
            doubleValue.ValueBox.ValueChanged += OnValueChanged;
            doubleValue.ValueBox.SlidingEnd += ClearToken;
            _element = doubleValue;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                var limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
                doubleValue.SetLimits(limit);
                var valueCategory = ((ValueCategoryAttribute)attributes.FirstOrDefault(x => x is ValueCategoryAttribute))?.Category ?? Utils.ValueCategory.None;
                if (valueCategory != Utils.ValueCategory.None)
                {
                    doubleValue.SetCategory(valueCategory);
                    if (LinkedLabel != null)
                    {
                        LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                        {
                            menu.AddSeparator();
                            var mb = menu.AddButton("Show formatted", bt => { doubleValue.SetCategory(bt.Checked ? valueCategory : Utils.ValueCategory.None); });
                            mb.AutoCheck = true;
                            mb.Checked = doubleValue.ValueBox.Category != Utils.ValueCategory.None;
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

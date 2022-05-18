// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Double4 value type properties.
    /// </summary>
    [CustomEditor(typeof(Double4)), DefaultEditor]
    public class Double4Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected DoubleValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected DoubleValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected DoubleValueElement ZElement;

        /// <summary>
        /// The W component editor.
        /// </summary>
        protected DoubleValueElement WElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 4;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
            }

            XElement = grid.DoubleValue();
            XElement.SetLimits(limit);
            XElement.DoubleValue.ValueChanged += OnValueChanged;
            XElement.DoubleValue.SlidingEnd += ClearToken;

            YElement = grid.DoubleValue();
            YElement.SetLimits(limit);
            YElement.DoubleValue.ValueChanged += OnValueChanged;
            YElement.DoubleValue.SlidingEnd += ClearToken;

            ZElement = grid.DoubleValue();
            ZElement.SetLimits(limit);
            ZElement.DoubleValue.ValueChanged += OnValueChanged;
            ZElement.DoubleValue.SlidingEnd += ClearToken;

            WElement = grid.DoubleValue();
            WElement.SetLimits(limit);
            WElement.DoubleValue.ValueChanged += OnValueChanged;
            WElement.DoubleValue.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding || WElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Double4(XElement.DoubleValue.Value, YElement.DoubleValue.Value, ZElement.DoubleValue.Value, WElement.DoubleValue.Value);
            SetValue(value, token);
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
                var value = (Double4)Values[0];
                XElement.DoubleValue.Value = value.X;
                YElement.DoubleValue.Value = value.Y;
                ZElement.DoubleValue.Value = value.Z;
                WElement.DoubleValue.Value = value.W;
            }
        }
    }
}

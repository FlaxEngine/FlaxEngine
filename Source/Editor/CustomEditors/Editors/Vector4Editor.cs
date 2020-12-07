// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Vector4 value type properties.
    /// </summary>
    [CustomEditor(typeof(Vector4)), DefaultEditor]
    public class Vector4Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected FloatValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected FloatValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected FloatValueElement ZElement;

        /// <summary>
        /// The W component editor.
        /// </summary>
        protected FloatValueElement WElement;

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

            XElement = grid.FloatValue();
            XElement.SetLimits(limit);
            XElement.FloatValue.ValueChanged += OnValueChanged;
            XElement.FloatValue.SlidingEnd += ClearToken;

            YElement = grid.FloatValue();
            YElement.SetLimits(limit);
            YElement.FloatValue.ValueChanged += OnValueChanged;
            YElement.FloatValue.SlidingEnd += ClearToken;

            ZElement = grid.FloatValue();
            ZElement.SetLimits(limit);
            ZElement.FloatValue.ValueChanged += OnValueChanged;
            ZElement.FloatValue.SlidingEnd += ClearToken;

            WElement = grid.FloatValue();
            WElement.SetLimits(limit);
            WElement.FloatValue.ValueChanged += OnValueChanged;
            WElement.FloatValue.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding || WElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Vector4(
                XElement.FloatValue.Value,
                YElement.FloatValue.Value,
                ZElement.FloatValue.Value,
                WElement.FloatValue.Value);
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
                var value = (Vector4)Values[0];
                XElement.FloatValue.Value = value.X;
                YElement.FloatValue.Value = value.Y;
                ZElement.FloatValue.Value = value.Z;
                WElement.FloatValue.Value = value.W;
            }
        }
    }
}

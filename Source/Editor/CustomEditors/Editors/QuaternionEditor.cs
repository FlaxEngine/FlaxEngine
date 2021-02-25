// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Quaternion value type properties.
    /// </summary>
    [CustomEditor(typeof(Quaternion)), DefaultEditor]
    public class QuaternionEditor : CustomEditor
    {
        /// <summary>
        /// The X component element
        /// </summary>
        protected FloatValueElement XElement;

        /// <summary>
        /// The Y component element
        /// </summary>
        protected FloatValueElement YElement;

        /// <summary>
        /// The Z component element
        /// </summary>
        protected FloatValueElement ZElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 3;
            gridControl.SlotsVertically = 1;

            XElement = grid.FloatValue();
            XElement.FloatValue.ValueChanged += OnValueChanged;
            XElement.FloatValue.SlidingEnd += ClearToken;

            YElement = grid.FloatValue();
            YElement.FloatValue.ValueChanged += OnValueChanged;
            YElement.FloatValue.SlidingEnd += ClearToken;

            ZElement = grid.FloatValue();
            ZElement.FloatValue.ValueChanged += OnValueChanged;
            ZElement.FloatValue.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            float x = XElement.FloatValue.Value;
            float y = YElement.FloatValue.Value;
            float z = ZElement.FloatValue.Value;
            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            Quaternion.Euler(x, y, z, out Quaternion value);
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
                var value = (Quaternion)Values[0];
                var euler = value.EulerAngles;
                XElement.FloatValue.Value = euler.X;
                YElement.FloatValue.Value = euler.Y;
                ZElement.FloatValue.Value = euler.Z;
            }
        }
    }
}

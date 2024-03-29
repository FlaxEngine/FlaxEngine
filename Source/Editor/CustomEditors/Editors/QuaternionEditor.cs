// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Quaternion value type properties.
    /// </summary>
    [CustomEditor(typeof(Quaternion)), DefaultEditor]
    public class QuaternionEditor : CustomEditor
    {
        private Float3 _cachedAngles = Float3.Zero;
        private object _cachedToken;

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
            XElement.ValueBox.Category = Utils.ValueCategory.Angle;
            XElement.ValueBox.ValueChanged += OnValueChanged;
            XElement.ValueBox.SlidingEnd += ClearToken;

            YElement = grid.FloatValue();
            YElement.ValueBox.Category = Utils.ValueCategory.Angle;
            YElement.ValueBox.ValueChanged += OnValueChanged;
            YElement.ValueBox.SlidingEnd += ClearToken;

            ZElement = grid.FloatValue();
            ZElement.ValueBox.Category = Utils.ValueCategory.Angle;
            ZElement.ValueBox.ValueChanged += OnValueChanged;
            ZElement.ValueBox.SlidingEnd += ClearToken;

            if (LinkedLabel != null)
            {
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                {
                    menu.AddSeparator();
                    var value = ((Quaternion)Values[0]).EulerAngles;
                    menu.AddButton("Copy Euler", () => { Clipboard.Text = JsonSerializer.Serialize(value); }).TooltipText = "Copy the Euler Angles in Degrees";
                };
            }
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var useCachedAngles = isSliding && token == _cachedToken;

            float x = (useCachedAngles && !XElement.IsSliding) ? _cachedAngles.X : XElement.ValueBox.Value;
            float y = (useCachedAngles && !YElement.IsSliding) ? _cachedAngles.Y : YElement.ValueBox.Value;
            float z = (useCachedAngles && !ZElement.IsSliding) ? _cachedAngles.Z : ZElement.ValueBox.Value;

            x = Mathf.UnwindDegrees(x);
            y = Mathf.UnwindDegrees(y);
            z = Mathf.UnwindDegrees(z);

            if (!useCachedAngles)
            {
                _cachedAngles = new Float3(x, y, z);
            }

            _cachedToken = token;

            Quaternion.Euler(x, y, z, out Quaternion value);
            SetValue(value, token);
        }

        /// <inheritdoc />
        protected override void ClearToken()
        {
            _cachedToken = null;
            base.ClearToken();
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
                XElement.ValueBox.Value = euler.X;
                YElement.ValueBox.Value = euler.Y;
                ZElement.ValueBox.Value = euler.Z;
            }
        }
    }
}

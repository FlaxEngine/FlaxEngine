// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
        
        /// <summary>
        /// The X label.
        /// </summary>
        protected LabelElement XLabel;
        
        /// <summary>
        /// The Y label.
        /// </summary>
        protected LabelElement YLabel;
        
        /// <summary>
        /// The Z label.
        /// </summary>
        protected LabelElement ZLabel;

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

            var xContainer = CustomEditorUtils.CreateGridContainer(grid, "X", out var xLabel);
            XLabel = xLabel;
            
            XElement = xContainer.FloatValue();
            XElement.ValueBox.ValueChanged += OnValueChanged;
            XElement.ValueBox.SlidingEnd += ClearToken;

            var yContainer = CustomEditorUtils.CreateGridContainer(grid, "Y", out var yLabel);
            YLabel = yLabel;
            
            YElement = yContainer.FloatValue();
            YElement.ValueBox.ValueChanged += OnValueChanged;
            YElement.ValueBox.SlidingEnd += ClearToken;

            var zContainer = CustomEditorUtils.CreateGridContainer(grid, "Z", out var zLabel);
            ZLabel = zLabel;
            
            ZElement = zContainer.FloatValue();
            ZElement.ValueBox.ValueChanged += OnValueChanged;
            ZElement.ValueBox.SlidingEnd += ClearToken;

            LinkedLabel.SetupContextMenu += (label, menu, editor) =>
            {
                menu.AddSeparator();
                var value = ((Quaternion)Values[0]).EulerAngles;
                menu.AddButton("Copy Euler", () => { Clipboard.Text = JsonSerializer.Serialize(value); }).TooltipText = "Copy the Euler Angles in Degrees";
            };
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

// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.Dialogs;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom implementation of the inspector used to edit Float4 color value type properties with color grading trackball.
    /// </summary>
    public sealed class ColorTrackball : CustomEditor
    {
        private FloatValueElement _xElement;
        private FloatValueElement _yElement;
        private FloatValueElement _zElement;
        private FloatValueElement _wElement;
        private ColorValueBox _colorBox;
        private CustomElement<ColorSelector> _trackball;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            float trackBallSize = 100f;
            float margin = 4.0f;

            // Panel
            var masterPanel = layout.CustomContainer<GridPanel>();
            var masterPanelControl = masterPanel.CustomControl;
            masterPanelControl.ClipChildren = false;
            masterPanelControl.SlotPadding = new Margin(0, 0, margin, margin);
            masterPanelControl.Height = trackBallSize + margin + margin;
            masterPanelControl.ColumnFill = new[]
            {
                -trackBallSize,
                1.0f
            };
            masterPanelControl.RowFill = new[] { 1.0f };

            // Trackball
            _trackball = masterPanel.Custom<ColorSelector>();
            _trackball.CustomControl.ColorChanged += OnColorWheelChanged;
            _trackball.CustomControl.SlidingEnd += ClearToken;

            // Scale editor
            {
                var grid = masterPanel.UniformGrid();
                var gridControl = grid.CustomControl;
                gridControl.SlotPadding = new Margin(4, 2, 2, 2);
                gridControl.ClipChildren = false;
                gridControl.SlotsHorizontally = 1;
                gridControl.SlotsVertically = 5;

                LimitAttribute limit = null;
                var attributes = Values.GetAttributes();
                if (attributes != null)
                {
                    limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
                }
                _colorBox = grid.Custom<ColorValueBox>().CustomControl;
                _colorBox.ValueChanged += OnColorBoxChanged;
                _xElement = CreateFloatEditor(grid, limit, Color.Red);
                _yElement = CreateFloatEditor(grid, limit, Color.Green);
                _zElement = CreateFloatEditor(grid, limit, Color.Blue);
                _wElement = CreateFloatEditor(grid, limit, Color.White);
            }
        }

        private FloatValueElement CreateFloatEditor(LayoutElementsContainer layout, LimitAttribute limit, Color borderColor)
        {
            var element = layout.FloatValue();
            element.SetLimits(limit);
            element.ValueBox.ValueChanged += OnValueChanged;
            element.ValueBox.SlidingEnd += ClearToken;
            var back = FlaxEngine.GUI.Style.Current.TextBoxBackground;
            var grayOutFactor = 0.6f;
            element.ValueBox.BorderColor = Color.Lerp(borderColor, back, grayOutFactor);
            element.ValueBox.BorderSelectedColor = borderColor;
            return element;
        }

        private void OnColorWheelChanged(Color color)
        {
            if (IsSetBlocked)
                return;

            var isSliding = _trackball.CustomControl.IsSliding;
            var token = isSliding ? this : null;
            var value = new Float4(color.R, color.G, color.B, _wElement.ValueBox.Value);
            SetValue(value, token);
        }

        private void OnColorBoxChanged()
        {
            var token = _colorBox.IsSliding ? this : null;
            var color = _colorBox.Value;
            SetValue(new Float4(color.R, color.G, color.B, color.A), token);
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = _xElement.IsSliding || _yElement.IsSliding || _zElement.IsSliding || _wElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Float4(_xElement.ValueBox.Value, _yElement.ValueBox.Value, _zElement.ValueBox.Value, _wElement.ValueBox.Value);
            SetValue(value, token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
            {
                var value = (Float4)Values[0];
                var color = new Float3(value);
                var scale = value.W;
                float min = color.MinValue;
                float max = color.MaxValue;
                if (min < 0.0f)
                {
                    // Negative color case (e.g. offset)
                }
                else if (max > 1.0f)
                {
                    // HDR color case
                    color /= max;
                    scale *= max;
                }
                _xElement.Value = color.X;
                _yElement.Value = color.Y;
                _zElement.Value = color.Z;
                _wElement.Value = scale;
                _colorBox.Value = new Color(color.X, color.Y, color.Z, scale);
                _trackball.CustomControl.Color = Float3.Abs(color);
            }
        }
    }
}

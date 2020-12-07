// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.Dialogs;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom implementation of the inspector used to edit Vector4 color value type properties with color grading trackball.
    /// </summary>
    public sealed class ColorTrackball : CustomEditor
    {
        private FloatValueElement _xElement;
        private FloatValueElement _yElement;
        private FloatValueElement _zElement;
        private FloatValueElement _wElement;
        private CustomElement<ColorSelector> _trackball;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            float trackBallSize = 80.0f;
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

            // Scale editor
            {
                var grid = masterPanel.CustomContainer<UniformGridPanel>();
                var gridControl = grid.CustomControl;
                gridControl.SlotPadding = new Margin(4, 2, 2, 2);
                gridControl.ClipChildren = false;
                gridControl.SlotsHorizontally = 1;
                gridControl.SlotsVertically = 4;

                LimitAttribute limit = null;
                var attributes = Values.GetAttributes();
                if (attributes != null)
                {
                    limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
                }

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
            element.FloatValue.ValueChanged += OnValueChanged;
            var back = FlaxEngine.GUI.Style.Current.TextBoxBackground;
            var grayOutFactor = 0.6f;
            element.FloatValue.BorderColor = Color.Lerp(borderColor, back, grayOutFactor);
            element.FloatValue.BorderSelectedColor = borderColor;
            return element;
        }

        private void OnColorWheelChanged(Color color)
        {
            if (IsSetBlocked)
                return;

            SetValue(new Vector4(color.R, color.G, color.B, _wElement.FloatValue.Value));
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            SetValue(new Vector4(
                         _xElement.FloatValue.Value,
                         _yElement.FloatValue.Value,
                         _zElement.FloatValue.Value,
                         _wElement.FloatValue.Value));
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
            {
                var value = (Vector4)Values[0];
                var color = new Vector3(value);
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
                _trackball.CustomControl.Color = Vector3.Abs(color);
            }
        }
    }
}

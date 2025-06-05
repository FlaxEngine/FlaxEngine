// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Float value editor with fixed size text box and slider.
    /// </summary>
    [HideInEditor]
    public class SliderControl : ContainerControl
    {
        /// <summary>
        /// The horizontal slider control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Control" />
        [HideInEditor]
        protected class Slider : Control
        {
            /// <summary>
            /// The default size.
            /// </summary>
            public const int DefaultSize = 16;

            /// <summary>
            /// The default thickness.
            /// </summary>
            public const int DefaultThickness = 6;

            /// <summary>
            /// The minimum value (constant)
            /// </summary>
            public const float Minimum = 0.0f;

            /// <summary>
            /// The maximum value (constant).
            /// </summary>
            public const float Maximum = 100.0f;

            private float _value;
            private Rectangle _thumbRect;
            private float _thumbCenter, _thumbSize;
            private bool _isSliding;

            /// <summary>
            /// Gets or sets the value (normalized to range 0-100).
            /// </summary>
            public float Value
            {
                get => _value;
                set
                {
                    value = Mathf.Clamp(value, Minimum, Maximum);
                    if (value != _value)
                    {
                        _value = value;

                        // Update
                        UpdateThumb();
                        ValueChanged?.Invoke();
                    }
                }
            }

            /// <summary>
            /// Occurs when value gets changed.
            /// </summary>
            public Action ValueChanged;

            /// <summary>
            /// The color of the slider track line.
            /// </summary>
            public Color TrackLineColor { get; set; }

            /// <summary>
            /// The color of the slider thumb when it's not selected.
            /// </summary>
            public Color ThumbColor { get; set; }

            /// <summary>
            /// The color of the slider thumb when it's selected.
            /// </summary>
            public Color ThumbColorSelected { get; set; }

            /// <summary>
            /// The color of the slider thumb when it's hovered.
            /// </summary>
            public Color ThumbColorHovered { get; set; }

            /// <summary>
            /// Gets a value indicating whether user is using a slider.
            /// </summary>
            public bool IsSliding => _isSliding;

            /// <summary>
            /// Occurs when sliding starts.
            /// </summary>
            public Action SlidingStart;

            /// <summary>
            /// Occurs when sliding ends.
            /// </summary>
            public Action SlidingEnd;

            /// <summary>
            /// Initializes a new instance of the <see cref="Slider"/> class.
            /// </summary>
            /// <param name="width">The width.</param>
            /// <param name="height">The height.</param>
            public Slider(float width, float height)
            : base(0, 0, width, height)
            {
                var style = Style.Current;
                TrackLineColor = style.BackgroundHighlighted;
                ThumbColor = style.BackgroundNormal;
                ThumbColorSelected = style.BackgroundSelected;
                ThumbColorHovered = style.BackgroundHighlighted;
            }

            private void UpdateThumb()
            {
                // Cache data
                float trackSize = TrackSize;
                float range = Maximum - Minimum;
                _thumbSize = Mathf.Min(trackSize, Mathf.Max(trackSize / range * 10.0f, 30.0f));
                float pixelRange = trackSize - _thumbSize;
                float perc = (_value - Minimum) / range;
                float thumbPosition = (int)(perc * pixelRange);
                _thumbCenter = thumbPosition + _thumbSize / 2;
                _thumbRect = new Rectangle(thumbPosition + 4, (Height - DefaultThickness) / 2, _thumbSize - 8, DefaultThickness);
            }

            private void EndSliding()
            {
                _isSliding = false;
                EndMouseCapture();
                SlidingEnd?.Invoke();
                Defocus();
                Parent?.Focus();
            }

            /// <summary>
            /// Gets the size of the track.
            /// </summary>
            private float TrackSize => Width;

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Draw track line
                var lineRect = new Rectangle(4, Height / 2, Width - 8, 1);
                Render2D.FillRectangle(lineRect, TrackLineColor);

                // Draw thumb
                bool mouseOverThumb = _thumbRect.Contains(PointFromWindow(Root.MousePosition));
                Render2D.FillRectangle(_thumbRect, _isSliding ? ThumbColorSelected : mouseOverThumb ? ThumbColorHovered : ThumbColor);
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                if (_isSliding)
                {
                    EndSliding();
                }

                base.OnLostFocus();
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    Focus();
                    float mousePosition = location.X;

                    if (_thumbRect.Contains(ref location))
                    {
                        // Start sliding
                        _isSliding = true;
                        StartMouseCapture();
                        SlidingStart?.Invoke();
                        return true;
                    }
                    else
                    {
                        // Click change
                        Value += (mousePosition < _thumbCenter ? -1 : 1) * 10;
                        Defocus();
                        Parent?.Focus();
                    }
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                if (_isSliding)
                {
                    // Update sliding
                    var slidePosition = location + Root.TrackingMouseOffset;
                    Value = Mathf.Remap(slidePosition.X, 4, TrackSize - 4, Minimum, Maximum);
                    if (Mathf.NearEqual(Value, Maximum))
                        Value = Maximum;
                    else if (Mathf.NearEqual(Value, Minimum))
                        Value = Minimum;
                }
                else
                {
                    base.OnMouseMove(location);
                }
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _isSliding)
                {
                    EndSliding();
                    return true;
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                // Check if was sliding
                if (_isSliding)
                {
                    EndSliding();
                }
                else
                {
                    base.OnEndMouseCapture();
                }
            }

            /// <inheritdoc />
            protected override void OnSizeChanged()
            {
                base.OnSizeChanged();

                UpdateThumb();
            }
        }

        /// <summary>
        /// The slider.
        /// </summary>
        protected Slider _slider;

        /// <summary>
        /// The text box.
        /// </summary>
        protected TextBox _textBox;

        /// <summary>
        /// The text box size (rest will be the slider area).
        /// </summary>
        protected const float TextBoxSize = 30.0f;

        private float _value;
        private float _min, _max;

        private bool _valueIsChanging;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public float Value
        {
            get => _value;
            set
            {
                value = Mathf.Clamp(value, _min, _max);
                if (Math.Abs(_value - value) > Mathf.Epsilon)
                {
                    // Set value
                    _value = value;

                    // Update
                    _valueIsChanging = true;
                    UpdateText();
                    UpdateSlider();
                    _valueIsChanging = false;
                    OnValueChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        public float MinValue
        {
            get => _min;
            set
            {
                if (_min != value)
                {
                    if (value > _max)
                        throw new ArgumentException();

                    _min = value;
                    Value = Value;
                }
            }
        }

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        public float MaxValue
        {
            get => _max;
            set
            {
                if (_max != value)
                {
                    if (value < _min)
                        throw new ArgumentException();

                    _max = value;
                    Value = Value;
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => _slider.IsSliding;

        /// <summary>
        /// Occurs when sliding starts.
        /// </summary>
        public event Action SlidingStart;

        /// <summary>
        /// Occurs when sliding ends.
        /// </summary>
        public event Action SlidingEnd;

        /// <summary>
        /// Initializes a new instance of the <see cref="SliderControl"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The position x.</param>
        /// <param name="y">The position y.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        public SliderControl(float value, float x = 0, float y = 0, float width = 120, float min = float.MinValue, float max = float.MaxValue)
        : base(x, y, width, TextBox.DefaultHeight)
        {
            _min = min;
            _max = max;
            _value = Mathf.Clamp(value, min, max);

            float split = Width - TextBoxSize;
            _slider = new Slider(split, Height)
            {
                Parent = this,
            };
            _slider.ValueChanged += SliderOnValueChanged;
            _slider.SlidingStart += SlidingStart;
            _slider.SlidingEnd += SliderOnSliderEnd;
            _textBox = new TextBox(false, split, 0)
            {
                Text = _value.ToString(CultureInfo.InvariantCulture),
                Parent = this,
                Location = new Float2(split, 0),
                Size = new Float2(Height, TextBoxSize),
            };
            _textBox.EditEnd += OnTextBoxEditEnd;
        }

        private void SliderOnSliderEnd()
        {
            SlidingEnd?.Invoke();
            Defocus();
            Parent?.Focus();
        }

        private void SliderOnValueChanged()
        {
            if (_valueIsChanging)
                return;

            Value = Mathf.Remap(_slider.Value, Slider.Minimum, Slider.Maximum, MinValue, MaxValue);
        }

        private void OnTextBoxEditEnd()
        {
            if (_valueIsChanging)
                return;

            var text = _textBox.Text.Replace(',', '.');
            if (double.TryParse(text, NumberStyles.Float | NumberStyles.AllowThousands, CultureInfo.InvariantCulture, out var value))
            {
                Value = (float)Math.Round(value, 5);
            }
            else
            {
                UpdateText();
            }
            Defocus();
            Parent?.Focus();
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(RangeAttribute limits)
        {
            _min = limits.Min;
            _max = Mathf.Max(_min, limits.Max);
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min;
            _max = Mathf.Max(_min, limits.Max);
            Value = Value;
        }

        /// <summary>
        /// Updates the text of the textbox.
        /// </summary>
        protected virtual void UpdateText()
        {
            _textBox.Text = _value.ToString(CultureInfo.InvariantCulture);
        }

        /// <summary>
        /// Updates the slider value.
        /// </summary>
        protected virtual void UpdateSlider()
        {
            _slider.Value = Mathf.Remap(_value, MinValue, MaxValue, Slider.Minimum, Slider.Maximum);
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            float split = Width - TextBoxSize;
            _slider.Bounds = new Rectangle(0, 0, split, Height);
            _textBox.Bounds = new Rectangle(split, 0, TextBoxSize, Height);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _slider = null;
            _textBox = null;

            base.OnDestroy();
        }
    }
}

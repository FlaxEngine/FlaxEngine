// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Double precision floating point value editor.
    /// </summary>
    /// <seealso cref="double" />
    [HideInEditor]
    public class DoubleValueBox : ValueBox<double>
    {
        /// <inheritdoc />
        public override double Value
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
                    UpdateText();
                    OnValueChanged();
                }
            }
        }

        /// <inheritdoc />
        public override double MinValue
        {
            get => _min;
            set
            {
                if (!Mathf.NearEqual(_min, value))
                {
                    if (value > _max)
                        throw new ArgumentException();

                    _min = value;
                    Value = Value;
                }
            }
        }

        /// <inheritdoc />
        public override double MaxValue
        {
            get => _max;
            set
            {
                if (!Mathf.NearEqual(_max, value))
                {
                    if (value < _min)
                        throw new ArgumentException();

                    _max = value;
                    Value = Value;
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatValueBox"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x location.</param>
        /// <param name="y">The y location.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="slideSpeed">The slide speed.</param>
        public DoubleValueBox(double value, float x = 0, float y = 0, float width = 120, double min = double.MinValue, double max = double.MaxValue, float slideSpeed = 1)
        : base(Mathf.Clamp(value, min, max), x, y, width, min, max, slideSpeed)
        {
            UpdateText();
        }

        /// <summary>
        /// Sets the value limits.
        /// </summary>
        /// <param name="min">The minimum value (bottom range).</param>
        /// <param name="max">The maximum value (upper range).</param>
        public void SetLimits(double min, double max)
        {
            _min = min;
            _max = Math.Max(_min, max);
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(RangeAttribute limits)
        {
            _min = limits.Min;
            _max = Math.Max(_min, limits.Max);
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min;
            _max = Math.Max(_min, (double)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the other <see cref="DoubleValueBox"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        public void SetLimits(DoubleValueBox other)
        {
            _min = other._min;
            _max = other._max;
            _slideSpeed = other._slideSpeed;
            Value = Value;
        }

        /// <inheritdoc />
        protected sealed override void UpdateText()
        {
            string text;
            if (double.IsPositiveInfinity(_value) || _value == double.MaxValue)
                text = "Infinity";
            else if (double.IsNegativeInfinity(_value) || _value == double.MinValue)
                text = "-Infinity";
            else
                text = _value.ToString(CultureInfo.InvariantCulture);

            SetText(text);
        }

        /// <inheritdoc />
        protected override void TryGetValue()
        {
            try
            {
                Value = ShuntingYard.Parse(Text);
            }
            catch (Exception ex)
            {
                // Fall back to previous value
                Editor.LogWarning(ex);
            }
        }

        /// <inheritdoc />
        protected override void ApplySliding(float delta)
        {
            Value = _startSlideValue + delta;
        }
    }
}

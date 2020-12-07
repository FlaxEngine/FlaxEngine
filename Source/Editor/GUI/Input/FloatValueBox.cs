// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Floating point value editor.
    /// </summary>
    /// <seealso cref="float" />
    [HideInEditor]
    public class FloatValueBox : ValueBox<float>
    {
        /// <inheritdoc />
        public override float Value
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
        public override float MinValue
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
        public override float MaxValue
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
        public FloatValueBox(float value, float x = 0, float y = 0, float width = 120, float min = float.MinValue, float max = float.MaxValue, float slideSpeed = 1)
        : base(Mathf.Clamp(value, min, max), x, y, width, min, max, slideSpeed)
        {
            UpdateText();
        }

        /// <summary>
        /// Sets the value limits.
        /// </summary>
        /// <param name="min">The minimum value (bottom range).</param>
        /// <param name="max">The maximum value (upper range).</param>
        public void SetLimits(float min, float max)
        {
            _min = min;
            _max = Mathf.Max(_min, max);
            Value = Value;
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
            _slideSpeed = limits.SliderSpeed;
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the other <see cref="FloatValueBox"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        public void SetLimits(FloatValueBox other)
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
            if (float.IsPositiveInfinity(_value) || _value == float.MaxValue)
                text = "Infinity";
            else if (float.IsNegativeInfinity(_value) || _value == float.MinValue)
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
                var value = ShuntingYard.Parse(Text);
                Value = (float)Math.Round(value, 5);
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

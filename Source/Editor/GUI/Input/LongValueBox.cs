// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Integer (long type) value editor.
    /// </summary>
    /// <seealso cref="long" />
    [HideInEditor]
    public class LongValueBox : ValueBox<long>
    {
        /// <inheritdoc />
        public override long Value
        {
            get => _value;
            set
            {
                value = Mathf.Clamp(value, _min, _max);
                if (_value != value)
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
        public override long MinValue
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

        /// <inheritdoc />
        public override long MaxValue
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
        /// Initializes a new instance of the <see cref="LongValueBox"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x location.</param>
        /// <param name="y">The y location.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="slideSpeed">The slide speed.</param>
        public LongValueBox(long value, float x = 0, float y = 0, float width = 120, long min = long.MinValue, long max = long.MaxValue, float slideSpeed = 1)
        : base(Mathf.Clamp(value, min, max), x, y, width, min, max, slideSpeed)
        {
            UpdateText();
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min == int.MinValue ? long.MinValue : (long)limits.Min;
            _max = Math.Max(_min, limits.Max == int.MaxValue ? long.MaxValue : (long)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            Value = Value;
        }

        /// <summary>
        /// Sets the limits at once.
        /// </summary>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The minimum value.</param>
        public void SetLimits(long min, long max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            Value = Value;
        }

        /// <inheritdoc />
        protected sealed override void UpdateText()
        {
            var text = _value.ToString();

            SetText(text);
        }

        /// <inheritdoc />
        protected override void TryGetValue()
        {
            // Try to parse long
            if (long.TryParse(Text, out long value))
            {
                // Set value
                Value = value;
            }
        }

        /// <inheritdoc />
        protected override void ApplySliding(float delta)
        {
            Value = _startSlideValue + (long)delta;
        }
    }
}

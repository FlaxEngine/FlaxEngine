// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Unsigned integer (ulong type) value editor.
    /// </summary>
    /// <seealso cref="ulong" />
    [HideInEditor]
    public class ULongValueBox : ValueBox<ulong>
    {
        /// <inheritdoc />
        public override ulong Value
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
        public override ulong MinValue
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
        public override ulong MaxValue
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
        /// Initializes a new instance of the <see cref="ULongValueBox"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x location.</param>
        /// <param name="y">The y location.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="slideSpeed">The slide speed.</param>
        public ULongValueBox(ulong value, float x = 0, float y = 0, float width = 120, ulong min = ulong.MinValue, ulong max = ulong.MaxValue, float slideSpeed = 1)
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
            _min = limits.Min < 0.0f ? 0 : (ulong)limits.Min;
            _max = Math.Max(_min, limits.Max == float.MaxValue ? ulong.MaxValue : (ulong)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            Value = Value;
        }

        /// <summary>
        /// Sets the limits at once.
        /// </summary>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The minimum value.</param>
        public void SetLimits(ulong min, ulong max)
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
            try
            {
                var value = ShuntingYard.Parse(Text);
                Value = (ulong)value;
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
            // Check for negative overflow to positive numbers
            if (delta < 0 && _startSlideValue > delta)
                Value = (ulong)Mathf.Max(0, (long)_startSlideValue + (long)delta);
            else if (delta < 0)
                Value = _startSlideValue - (ulong)(-delta);
            else
                Value = _startSlideValue + (ulong)delta;
        }
    }
}

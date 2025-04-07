// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Unsigned Integer value editor.
    /// </summary>
    /// <seealso cref="uint" />
    [HideInEditor]
    public class UIntValueBox : ValueBox<uint>
    {
        /// <inheritdoc />
        public override uint Value
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
        public override uint MinValue
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
        public override uint MaxValue
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
        /// Initializes a new instance of the <see cref="UIntValueBox"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x location.</param>
        /// <param name="y">The y location.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="slideSpeed">The slide speed.</param>
        public UIntValueBox(uint value, float x = 0, float y = 0, float width = 120, uint min = uint.MinValue, uint max = uint.MaxValue, float slideSpeed = 1)
        : base(Mathf.Clamp(value, min, max), x, y, width, min, max, slideSpeed)
        {
            UpdateText();
        }

        /// <summary>
        /// Sets the value limits.
        /// </summary>
        /// <param name="min">The minimum value (bottom range).</param>
        /// <param name="max">The maximum value (upper range).</param>
        public void SetLimits(uint min, uint max)
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
            _min = limits.Min <= 0 ? uint.MinValue : (uint)limits.Min;
            _max = Math.Max(_min, limits.Max == float.MaxValue ? uint.MaxValue : (uint)limits.Max);
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min <= 0 ? uint.MinValue : (uint)limits.Min;
            _max = Math.Max(_min, limits.Max == float.MaxValue ? uint.MaxValue : (uint)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the other <see cref="UIntValueBox"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        public void SetLimits(UIntValueBox other)
        {
            _min = other._min;
            _max = other._max;
            _slideSpeed = other._slideSpeed;
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
                if (value < 0)
                    value = 0;
                Value = (uint)value;
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
            Value = _startSlideValue + (uint)delta;
        }
    }
}

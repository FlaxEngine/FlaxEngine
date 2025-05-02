// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit integer value type properties.
    /// </summary>
    [CustomEditor(typeof(int)), DefaultEditor]
    public sealed class IntegerEditor : CustomEditor
    {
        private IIntegerValueEditor _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = null;

            // Try get limit attribute for value min/max range setting and slider speed
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                var range = attributes.FirstOrDefault(x => x is RangeAttribute);
                if (range != null)
                {
                    // Use slider
                    var element = layout.Slider();
                    element.SetLimits((RangeAttribute)range);
                    element.Slider.ValueChanged += OnValueChanged;
                    element.Slider.SlidingEnd += ClearToken;
                    _element = element;
                    return;
                }
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    // Use int value editor with limit
                    var element = layout.IntegerValue();
                    element.SetLimits((LimitAttribute)limit);
                    element.IntValue.ValueChanged += OnValueChanged;
                    element.IntValue.SlidingEnd += ClearToken;
                    _element = element;
                    return;
                }
            }
            {
                // Use int value editor
                var element = layout.IntegerValue();
                element.IntValue.ValueChanged += OnValueChanged;
                element.IntValue.SlidingEnd += ClearToken;
                _element = element;
            }
        }

        private void OnValueChanged()
        {
            var isSliding = _element.IsSliding;
            var token = isSliding ? this : null;
            SetValue(_element.Value, token);
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
                var value = Values[0];
                if (value is int asInt)
                    _element.Value = asInt;
                else if (value is float asFloat)
                    _element.Value = (int)asFloat;
                else if (value is double asDouble)
                    _element.Value = (int)asDouble;
                else if (value is uint asUint)
                    _element.Value = (int)asUint;
                else if (value is long asLong)
                    _element.Value = (int)asLong;
                else if (value is ulong asULong)
                    _element.Value = (int)asULong;
                else if (value is short asShort)
                    _element.Value = asShort;
                else if (value is ushort asUshort)
                    _element.Value = asUshort;
                else if (value is byte asByte)
                    _element.Value = asByte;
                else if (value is sbyte asSbyte)
                    _element.Value = asSbyte;
                else
                    throw new Exception(string.Format("Invalid value type {0}.", value?.GetType().ToString() ?? "<null>"));
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit signed integer value type properties (maps to the full range of long type).
    /// </summary>
    public abstract class SignedIntegerValueEditor : CustomEditor
    {
        private SignedIntegerValueElement _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = null;

            GetLimits(out var min, out var max);

            // Try get limit attribute for value min/max range setting and slider speed
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    // Use int value editor with limit
                    var element = layout.SignedIntegerValue();
                    element.LongValue.SetLimits((LimitAttribute)limit);
                    element.LongValue.SetLimits(Mathf.Max(element.LongValue.MinValue, min), Mathf.Min(element.LongValue.MaxValue, max));
                    element.LongValue.ValueChanged += OnValueChanged;
                    element.LongValue.SlidingEnd += ClearToken;
                    _element = element;
                    return;
                }
            }
            if (_element == null)
            {
                // Use int value editor
                var element = layout.SignedIntegerValue();
                element.LongValue.SetLimits(Mathf.Max(element.LongValue.MinValue, min), Mathf.Min(element.LongValue.MaxValue, max));
                element.LongValue.ValueChanged += OnValueChanged;
                element.LongValue.SlidingEnd += ClearToken;
                _element = element;
            }
        }

        private void OnValueChanged()
        {
            var isSliding = _element.IsSliding;
            var token = isSliding ? this : null;
            SetValue(SetValue(_element.Value), token);
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
                _element.Value = GetValue(Values[0]);
            }
        }

        /// <summary>
        /// Gets the value limits.
        /// </summary>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        protected abstract void GetLimits(out long min, out long max);

        /// <summary>
        /// Gets the value as long.
        /// </summary>
        /// <param name="value">The value from object.</param>
        /// <returns>The value for editor.</returns>
        protected abstract long GetValue(object value);

        /// <summary>
        /// Gets the value from long.
        /// </summary>
        /// <param name="value">The value from editor.</param>
        /// <returns>The value to object.</returns>
        protected abstract object SetValue(long value);
    }

    /// <summary>
    /// Default implementation of the inspector used to edit sbyte value type properties.
    /// </summary>
    [CustomEditor(typeof(sbyte)), DefaultEditor]
    public sealed class SByteEditor : SignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out long min, out long max)
        {
            min = sbyte.MinValue;
            max = sbyte.MaxValue;
        }

        /// <inheritdoc />
        protected override long GetValue(object value)
        {
            return (sbyte)value;
        }

        /// <inheritdoc />
        protected override object SetValue(long value)
        {
            return (sbyte)value;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit short value type properties.
    /// </summary>
    [CustomEditor(typeof(short)), DefaultEditor]
    public sealed class ShortEditor : SignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out long min, out long max)
        {
            min = short.MinValue;
            max = short.MaxValue;
        }

        /// <inheritdoc />
        protected override long GetValue(object value)
        {
            return (short)value;
        }

        /// <inheritdoc />
        protected override object SetValue(long value)
        {
            return (short)value;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit long value type properties.
    /// </summary>
    [CustomEditor(typeof(long)), DefaultEditor]
    public sealed class LongEditor : SignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out long min, out long max)
        {
            min = long.MinValue;
            max = long.MaxValue;
        }

        /// <inheritdoc />
        protected override long GetValue(object value)
        {
            return (long)value;
        }

        /// <inheritdoc />
        protected override object SetValue(long value)
        {
            return value;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit unsigned integer value type properties (maps to the full range of ulong type).
    /// </summary>
    public abstract class UnsignedIntegerValueEditor : CustomEditor
    {
        private UnsignedIntegerValueElement _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = null;

            GetLimits(out var min, out var max);

            // Try get limit attribute for value min/max range setting and slider speed
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    // Use int value editor with limit
                    var element = layout.UnsignedIntegerValue();
                    element.ULongValue.SetLimits((LimitAttribute)limit);
                    element.ULongValue.SetLimits(Mathf.Max(element.ULongValue.MinValue, min), Mathf.Min(element.ULongValue.MaxValue, max));
                    element.ULongValue.ValueChanged += OnValueChanged;
                    element.ULongValue.SlidingEnd += ClearToken;
                    _element = element;
                    return;
                }
            }
            if (_element == null)
            {
                // Use int value editor
                var element = layout.UnsignedIntegerValue();
                element.ULongValue.SetLimits(Mathf.Max(element.ULongValue.MinValue, min), Mathf.Min(element.ULongValue.MaxValue, max));
                element.ULongValue.ValueChanged += OnValueChanged;
                element.ULongValue.SlidingEnd += ClearToken;
                _element = element;
            }
        }

        private void OnValueChanged()
        {
            var isSliding = _element.IsSliding;
            var token = isSliding ? this : null;
            SetValue(SetValue(_element.Value), token);
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
                _element.Value = GetValue(Values[0]);
            }
        }

        /// <summary>
        /// Gets the value limits.
        /// </summary>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        protected abstract void GetLimits(out ulong min, out ulong max);

        /// <summary>
        /// Gets the value as long.
        /// </summary>
        /// <param name="value">The value from object.</param>
        /// <returns>The value for editor.</returns>
        protected abstract ulong GetValue(object value);

        /// <summary>
        /// Sets the value from long.
        /// </summary>
        /// <param name="value">The value from editor.</param>
        /// <returns>The value to object.</returns>
        protected abstract object SetValue(ulong value);
    }

    /// <summary>
    /// Default implementation of the inspector used to edit byte value type properties.
    /// </summary>
    [CustomEditor(typeof(byte)), DefaultEditor]
    public sealed class ByteEditor : UnsignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out ulong min, out ulong max)
        {
            min = byte.MinValue;
            max = byte.MaxValue;
        }

        /// <inheritdoc />
        protected override ulong GetValue(object value)
        {
            return (byte)value;
        }

        /// <inheritdoc />
        protected override object SetValue(ulong value)
        {
            return (byte)value;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit char value type properties.
    /// </summary>
    [CustomEditor(typeof(char)), DefaultEditor]
    public sealed class CharEditor : UnsignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out ulong min, out ulong max)
        {
            min = char.MinValue;
            max = char.MaxValue;
        }

        /// <inheritdoc />
        protected override ulong GetValue(object value)
        {
            return (char)value;
        }

        /// <inheritdoc />
        protected override object SetValue(ulong value)
        {
            return (char)value;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit ushort value type properties.
    /// </summary>
    [CustomEditor(typeof(ushort)), DefaultEditor]
    public sealed class UShortEditor : UnsignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out ulong min, out ulong max)
        {
            min = ushort.MinValue;
            max = ushort.MaxValue;
        }

        /// <inheritdoc />
        protected override ulong GetValue(object value)
        {
            return (ushort)value;
        }

        /// <inheritdoc />
        protected override object SetValue(ulong value)
        {
            return (ushort)value;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit uint value type properties.
    /// </summary>
    [CustomEditor(typeof(uint)), DefaultEditor]
    public sealed class UintEditor : UnsignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out ulong min, out ulong max)
        {
            min = uint.MinValue;
            max = uint.MaxValue;
        }

        /// <inheritdoc />
        protected override ulong GetValue(object value)
        {
            return (uint)value;
        }

        /// <inheritdoc />
        protected override object SetValue(ulong value)
        {
            return (uint)value;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit ulong value type properties.
    /// </summary>
    [CustomEditor(typeof(ulong)), DefaultEditor]
    public sealed class ULongEditor : UnsignedIntegerValueEditor
    {
        /// <inheritdoc />
        protected override void GetLimits(out ulong min, out ulong max)
        {
            min = ulong.MinValue;
            max = ulong.MaxValue;
        }

        /// <inheritdoc />
        protected override ulong GetValue(object value)
        {
            return (ulong)value;
        }

        /// <inheritdoc />
        protected override object SetValue(ulong value)
        {
            return (ulong)value;
        }
    }
}

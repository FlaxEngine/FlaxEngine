// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using System.Reflection;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The integer value element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class IntegerValueElement : LayoutElement, IIntegerValueEditor
    {
        /// <summary>
        /// The integer value box.
        /// </summary>
        public readonly IntValueBox IntValue;

        /// <summary>
        /// Initializes a new instance of the <see cref="IntegerValueElement"/> class.
        /// </summary>
        public IntegerValueElement()
        {
            IntValue = new IntValueBox(0);
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="member">The member.</param>
        public void SetLimits(MemberInfo member)
        {
            // Try get limit attribute for value min/max range setting and slider speed
            if (member != null)
            {
                var attributes = member.GetCustomAttributes(true);
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    IntValue.SetLimits((LimitAttribute)limit);
                }
            }
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="limit">The limit.</param>
        public void SetLimits(LimitAttribute limit)
        {
            if (limit != null)
            {
                IntValue.SetLimits(limit);
            }
        }

        /// <summary>
        /// Sets the editor limits from the other <see cref="IntegerValueElement"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        public void SetLimits(IntegerValueElement other)
        {
            if (other != null)
            {
                IntValue.SetLimits(other.IntValue);
            }
        }

        /// <inheritdoc />
        public override Control Control => IntValue;

        /// <inheritdoc />
        public int Value
        {
            get => IntValue.Value;
            set => IntValue.Value = value;
        }

        /// <inheritdoc />
        public bool IsSliding => IntValue.IsSliding;
    }

    /// <summary>
    /// The signed integer value element (maps to the full range of long type).
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class SignedIntegerValueElement : LayoutElement
    {
        /// <summary>
        /// The signed integer (long) value box.
        /// </summary>
        public readonly LongValueBox LongValue;

        /// <summary>
        /// Initializes a new instance of the <see cref="SignedIntegerValueElement"/> class.
        /// </summary>
        public SignedIntegerValueElement()
        {
            LongValue = new LongValueBox(0);
        }

        /// <inheritdoc />
        public override Control Control => LongValue;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public long Value
        {
            get => LongValue.Value;
            set => LongValue.Value = value;
        }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => LongValue.IsSliding;
    }

    /// <summary>
    /// The unsigned integer value element (maps to the full range of ulong type).
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class UnsignedIntegerValueElement : LayoutElement
    {
        /// <summary>
        /// The unsigned integer (ulong) value box.
        /// </summary>
        public readonly ULongValueBox ULongValue;

        /// <summary>
        /// Initializes a new instance of the <see cref="UnsignedIntegerValueElement"/> class.
        /// </summary>
        public UnsignedIntegerValueElement()
        {
            ULongValue = new ULongValueBox(0);
        }

        /// <inheritdoc />
        public override Control Control => ULongValue;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public ulong Value
        {
            get => ULongValue.Value;
            set => ULongValue.Value = value;
        }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => ULongValue.IsSliding;
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using System.Reflection;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The double precision floating point value element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class DoubleValueElement : LayoutElement
    {
        /// <summary>
        /// The double value box.
        /// </summary>
        public readonly DoubleValueBox ValueBox;

        /// <summary>
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        [System.Obsolete("Use ValueBox instead")]
        public DoubleValueBox DoubleValue => ValueBox;

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatValueElement"/> class.
        /// </summary>
        public DoubleValueElement()
        {
            ValueBox = new DoubleValueBox(0);
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
                    ValueBox.SetLimits((LimitAttribute)limit);
                }
            }
        }

        /// <summary>
        /// Sets the editor value category.
        /// </summary>
        /// <param name="category">The category.</param>
        public void SetCategory(Utils.ValueCategory category)
        {
            ValueBox.Category = category;
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="limit">The limit.</param>
        public void SetLimits(LimitAttribute limit)
        {
            if (limit != null)
            {
                ValueBox.SetLimits(limit);
            }
        }

        /// <summary>
        /// Sets the editor limits from the other <see cref="DoubleValueElement"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        public void SetLimits(DoubleValueElement other)
        {
            if (other != null)
            {
                ValueBox.SetLimits(other.ValueBox);
            }
        }

        /// <inheritdoc />
        public override Control Control => ValueBox;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public double Value
        {
            get => ValueBox.Value;
            set => ValueBox.Value = value;
        }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => ValueBox.IsSliding;
    }
}
